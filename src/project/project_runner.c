#include "project_runner.h"

#include "project_cache.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#ifdef _WIN32
#  include <direct.h>
#  include <process.h>
#else
#  include <ftw.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#ifdef _WIN32
#  define pr_mkdir(path, mode) _mkdir(path)
#else
#  define pr_mkdir(path, mode) mkdir(path, mode)
#endif

#ifndef CCT_ARRAY_LEN
#define CCT_ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} cct_project_path_list_t;

static void pr_path_list_dispose(cct_project_path_list_t *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool pr_path_list_push(cct_project_path_list_t *list, const char *path) {
    if (!list || !path) return false;
    if (list->count == list->capacity) {
        size_t next = (list->capacity == 0) ? 8 : list->capacity * 2;
        char **grown = (char **)realloc(list->items, next * sizeof(char *));
        if (!grown) return false;
        list->items = grown;
        list->capacity = next;
    }
    list->items[list->count] = strdup(path);
    if (!list->items[list->count]) return false;
    list->count++;
    return true;
}

static int pr_cmp_str(const void *a, const void *b) {
    const char *const *sa = (const char *const *)a;
    const char *const *sb = (const char *const *)b;
    return strcmp(*sa, *sb);
}

static bool pr_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    return sl >= su && strcmp(s + sl - su, suffix) == 0;
}

static bool pr_ensure_dir(const char *path) {
    if (!path || path[0] == '\0') return false;

    if (cct_project_path_is_dir(path)) return true;

    char tmp[CCT_PROJECT_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (tmp[0] != '\0' && !cct_project_path_is_dir(tmp)) {
                if (pr_mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
            }
            *p = '/';
        }
    }

    if (!cct_project_path_is_dir(tmp)) {
        if (pr_mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
    }

    return true;
}

static bool pr_copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buf[8192];
    size_t n = 0;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }
    }

    fclose(in);
    fclose(out);
    return ok;
}

static int pr_spawn_argv(char *const argv[]) {
#ifdef _WIN32
    intptr_t rc = _spawnv(_P_WAIT, argv[0], (const char *const *)argv);
    return (rc == -1) ? 1 : (int)rc;
#else
    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid == 0) {
        execv(argv[0], argv);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
#endif
}

static int pr_run_cct_compile(const char *self_path, const char *file) {
    char *argv_local[3];
    argv_local[0] = (char *)self_path;
    argv_local[1] = (char *)file;
    argv_local[2] = NULL;
    return pr_spawn_argv(argv_local);
}

static int pr_run_cct_lint_strict(const char *self_path, const char *file) {
    char *argv_local[5];
    argv_local[0] = (char *)self_path;
    argv_local[1] = "lint";
    argv_local[2] = "--strict";
    argv_local[3] = (char *)file;
    argv_local[4] = NULL;
    return pr_spawn_argv(argv_local);
}

static int pr_run_cct_lint(const char *self_path, const char *file) {
    char *argv_local[4];
    argv_local[0] = (char *)self_path;
    argv_local[1] = "lint";
    argv_local[2] = (char *)file;
    argv_local[3] = NULL;
    return pr_spawn_argv(argv_local);
}

static int pr_run_cct_fmt_check(const char *self_path, const char *file) {
    char *argv_local[5];
    argv_local[0] = (char *)self_path;
    argv_local[1] = "fmt";
    argv_local[2] = "--check";
    argv_local[3] = (char *)file;
    argv_local[4] = NULL;
    return pr_spawn_argv(argv_local);
}

static bool pr_binary_path_from_source(const char *source_path, char *out, size_t out_size) {
    if (!source_path || !out || out_size == 0) return false;
    size_t len = strlen(source_path);
    if (len < 4 || strcmp(source_path + len - 4, ".cct") != 0) return false;
    if (len - 4 + 1 > out_size) return false;
    memcpy(out, source_path, len - 4);
    out[len - 4] = '\0';
    return true;
}

static bool pr_collect_with_suffix_recursive(const char *dir,
                                             const char *suffix,
                                             cct_project_path_list_t *out) {
    DIR *dp = opendir(dir);
    if (!dp) return false;

    struct dirent *ent = NULL;
    while ((ent = readdir(dp)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char path[CCT_PROJECT_PATH_MAX];
        if (!cct_project_join_path(dir, ent->d_name, path, sizeof(path))) {
            closedir(dp);
            return false;
        }

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (!pr_collect_with_suffix_recursive(path, suffix, out)) {
                closedir(dp);
                return false;
            }
            continue;
        }

        if (S_ISREG(st.st_mode) && pr_ends_with(path, suffix)) {
            if (!pr_path_list_push(out, path)) {
                closedir(dp);
                return false;
            }
        }
    }

    closedir(dp);
    return true;
}

static bool pr_filter_pattern(cct_project_path_list_t *list, const char *pattern) {
    if (!list || !pattern || pattern[0] == '\0') return true;

    size_t out_idx = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (strstr(list->items[i], pattern)) {
            list->items[out_idx++] = list->items[i];
        } else {
            free(list->items[i]);
        }
    }
    list->count = out_idx;
    return true;
}

static bool pr_compute_output_path(const cct_project_layout_t *layout,
                                   const cct_project_options_t *options,
                                   char *out,
                                   size_t out_size) {
    if (!layout || !options || !out || out_size == 0) return false;

    if (options->out_override && options->out_override[0] != '\0') {
        if (options->out_override[0] == '/') {
            snprintf(out, out_size, "%s", options->out_override);
        } else {
            if (!cct_project_join_path(layout->root_path, options->out_override, out, out_size)) {
                return false;
            }
        }
        return true;
    }

    char name[196];
    if (options->profile == CCT_PROJECT_PROFILE_RELEASE) {
        snprintf(name, sizeof(name), "%s_release", layout->project_name);
    } else {
        snprintf(name, sizeof(name), "%s", layout->project_name);
    }

    return cct_project_join_path(layout->out_dir, name, out, out_size);
}

static bool pr_make_cache_path(const cct_project_layout_t *layout,
                               char *cache_dir,
                               size_t cache_dir_size,
                               char *cache_file,
                               size_t cache_file_size) {
    char dot_cct[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout->root_path, ".cct", dot_cct, sizeof(dot_cct))) return false;

    if (!cct_project_join_path(dot_cct, "cache", cache_dir, cache_dir_size)) return false;
    if (!cct_project_join_path(cache_dir, "manifest.txt", cache_file, cache_file_size)) return false;
    return true;
}

#ifdef _WIN32
static void pr_remove_tree_recursive(const char *path) {
    DIR *dp = opendir(path);
    if (!dp) { remove(path); return; }
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char child[CCT_PROJECT_PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        struct stat cst;
        if (stat(child, &cst) == 0 && S_ISDIR(cst.st_mode)) {
            pr_remove_tree_recursive(child);
        } else {
            remove(child);
        }
    }
    closedir(dp);
    rmdir(path);
}
#else
static int pr_remove_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb;
    (void)typeflag;
    (void)ftwbuf;
    return remove(fpath);
}
#endif

static void pr_remove_tree_if_exists(const char *path) {
    if (!path || !cct_project_path_exists(path)) return;
    struct stat st;
    if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
        (void)remove(path);
        return;
    }
#ifdef _WIN32
    pr_remove_tree_recursive(path);
#else
    nftw(path, pr_remove_cb, 64, FTW_DEPTH | FTW_PHYS);
#endif
}

int cct_project_cmd_build(const char *self_path,
                          const cct_project_options_t *options,
                          char *built_output,
                          size_t built_output_size) {
    char cwd[CCT_PROJECT_PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "[build] error: could not determine current directory\n");
        return 1;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, options->entry_override,
                              &layout, err, sizeof(err))) {
        fprintf(stderr, "[build] error: %s\n", err);
        return 1;
    }

    char output_path[CCT_PROJECT_PATH_MAX];
    if (!pr_compute_output_path(&layout, options, output_path, sizeof(output_path))) {
        fprintf(stderr, "[build] error: output path is too long\n");
        return 1;
    }

    printf("[build] project root: %s\n", layout.root_path);
    printf("[build] profile: %s\n", options->profile == CCT_PROJECT_PROFILE_RELEASE ? "release" : "debug");
    printf("[build] entry: %s\n", layout.entry_path);

    if (options->run_lint) {
        int lint_rc = pr_run_cct_lint(self_path, layout.entry_path);
        if (lint_rc != 0) {
            fprintf(stderr, "[build] error: lint gate failed for entry module\n");
            return 2;
        }
    }

    if (options->fmt_check) {
        int fmt_rc = pr_run_cct_fmt_check(self_path, layout.entry_path);
        if (fmt_rc != 0) {
            fprintf(stderr, "[build] error: fmt-check gate failed for entry module\n");
            return 2;
        }
    }

    char fingerprint[CCT_PROJECT_FINGERPRINT_HEX_LEN + 1];
    if (!cct_project_cache_compute_fingerprint(&layout,
                                               options->profile == CCT_PROJECT_PROFILE_RELEASE ? "release" : "debug",
                                               fingerprint,
                                               sizeof(fingerprint),
                                               err,
                                               sizeof(err))) {
        fprintf(stderr, "[build] warning: incremental fingerprint unavailable (%s), forcing rebuild\n", err);
        fingerprint[0] = '\0';
    }

    cct_project_cache_record_t new_record;
    memset(&new_record, 0, sizeof(new_record));
    snprintf(new_record.fingerprint, sizeof(new_record.fingerprint), "%s", fingerprint);
    snprintf(new_record.profile, sizeof(new_record.profile), "%s",
             options->profile == CCT_PROJECT_PROFILE_RELEASE ? "release" : "debug");
    snprintf(new_record.output_path, sizeof(new_record.output_path), "%s", output_path);

    char cache_dir[CCT_PROJECT_PATH_MAX];
    char cache_file[CCT_PROJECT_PATH_MAX];
    if (!pr_make_cache_path(&layout, cache_dir, sizeof(cache_dir), cache_file, sizeof(cache_file))) {
        fprintf(stderr, "[build] warning: cache path unavailable, build will proceed without cache\n");
        cache_dir[0] = '\0';
        cache_file[0] = '\0';
    }

    cct_project_cache_record_t old_record;
    bool has_old = cache_file[0] != '\0' && cct_project_cache_load(cache_file, &old_record);
    bool out_exists = cct_project_path_exists(output_path);

    if (has_old && new_record.fingerprint[0] != '\0' &&
        cct_project_cache_is_up_to_date(&old_record, &new_record, out_exists)) {
        printf("[build] status: up-to-date\n");
        printf("[build] output: %s\n", output_path);
        if (built_output && built_output_size > 0) {
            snprintf(built_output, built_output_size, "%s", output_path);
        }
        return 0;
    }

    int compile_rc = pr_run_cct_compile(self_path, layout.entry_path);
    if (compile_rc != 0) {
        fprintf(stderr, "[build] error: compilation failed for %s\n", layout.entry_path);
        return 1;
    }

    char entry_bin[CCT_PROJECT_PATH_MAX];
    if (!pr_binary_path_from_source(layout.entry_path, entry_bin, sizeof(entry_bin))) {
        fprintf(stderr, "[build] error: could not derive entry binary path\n");
        return 1;
    }

    char output_dir[CCT_PROJECT_PATH_MAX];
    snprintf(output_dir, sizeof(output_dir), "%s", output_path);
    char *slash = strrchr(output_dir, '/');
    if (slash) {
        *slash = '\0';
        if (!pr_ensure_dir(output_dir)) {
            fprintf(stderr, "[build] error: could not create output directory: %s\n", output_dir);
            return 1;
        }
    }

    if (!pr_copy_file(entry_bin, output_path)) {
        fprintf(stderr, "[build] error: could not copy build artifact to %s\n", output_path);
        return 1;
    }
    chmod(output_path, 0755);

    if (cache_file[0] != '\0') {
        if (!pr_ensure_dir(cache_dir) || !cct_project_cache_store(cache_file, &new_record)) {
            fprintf(stderr, "[build] warning: could not persist cache manifest\n");
        }
    }

    printf("[build] status: rebuilt\n");
    printf("[build] output: %s\n", output_path);

    if (built_output && built_output_size > 0) {
        snprintf(built_output, built_output_size, "%s", output_path);
    }
    return 0;
}

int cct_project_cmd_run(const char *self_path, const cct_project_options_t *options) {
    char output_path[CCT_PROJECT_PATH_MAX];
    int build_rc = cct_project_cmd_build(self_path, options, output_path, sizeof(output_path));
    if (build_rc != 0) return build_rc;

    size_t argv_count = (size_t)options->passthrough_argc + 2;
    char **argv_exec = (char **)calloc(argv_count, sizeof(char *));
    if (!argv_exec) {
        fprintf(stderr, "[run] error: out of memory\n");
        return 1;
    }

    argv_exec[0] = output_path;
    for (int i = 0; i < options->passthrough_argc; i++) {
        argv_exec[i + 1] = options->passthrough_argv[i];
    }
    argv_exec[options->passthrough_argc + 1] = NULL;

    int rc = pr_spawn_argv(argv_exec);
    free(argv_exec);
    return rc;
}

static int pr_execute_compiled_binary(const char *source_path) {
    char bin[CCT_PROJECT_PATH_MAX];
    if (!pr_binary_path_from_source(source_path, bin, sizeof(bin))) return 1;

    char *argv_local[2];
    argv_local[0] = bin;
    argv_local[1] = NULL;
    return pr_spawn_argv(argv_local);
}

int cct_project_cmd_test(const char *self_path, const cct_project_options_t *options) {
    char cwd[CCT_PROJECT_PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "[test] error: could not determine current directory\n");
        return 2;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, NULL, &layout, err, sizeof(err))) {
        fprintf(stderr, "[test] error: %s\n", err);
        return 2;
    }

    char tests_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout.root_path, "tests", tests_dir, sizeof(tests_dir))) {
        fprintf(stderr, "[test] error: tests directory path is too long\n");
        return 2;
    }

    cct_project_path_list_t files = {0};
    if (cct_project_path_is_dir(tests_dir)) {
        if (!pr_collect_with_suffix_recursive(tests_dir, ".test.cct", &files)) {
            fprintf(stderr, "[test] error: failed while scanning tests directory\n");
            pr_path_list_dispose(&files);
            return 2;
        }
    }

    qsort(files.items, files.count, sizeof(char *), pr_cmp_str);
    pr_filter_pattern(&files, options->pattern);

    printf("[test] discovered: %zu\n", files.count);
    if (options->pattern && options->pattern[0] != '\0') {
        printf("[test] selected: %zu (pattern=\"%s\")\n", files.count, options->pattern);
    }

    if (files.count == 0) {
        pr_path_list_dispose(&files);
        return 0;
    }

    int pass = 0;
    int fail = 0;

    for (size_t i = 0; i < files.count; i++) {
        const char *path = files.items[i];

        if (options->fmt_check) {
            int fmt_rc = pr_run_cct_fmt_check(self_path, path);
            if (fmt_rc != 0) {
                fprintf(stderr, "[test] quality gate (fmt-check) failed: %s\n", path);
                pr_path_list_dispose(&files);
                return 2;
            }
        }

        if (options->strict_lint) {
            int lint_rc = pr_run_cct_lint_strict(self_path, path);
            if (lint_rc != 0) {
                fprintf(stderr, "[test] quality gate (strict-lint) failed: %s\n", path);
                pr_path_list_dispose(&files);
                return 2;
            }
        }

        int compile_rc = pr_run_cct_compile(self_path, path);
        if (compile_rc != 0) {
            printf("[test] FAIL %s (compile)\n", path);
            fail++;
            continue;
        }

        int run_rc = pr_execute_compiled_binary(path);
        if (run_rc == 0) {
            printf("[test] PASS %s\n", path);
            pass++;
        } else {
            printf("[test] FAIL %s (exit=%d)\n", path, run_rc);
            fail++;
        }
    }

    printf("[test] summary: pass=%d fail=%d\n", pass, fail);

    pr_path_list_dispose(&files);
    return (fail == 0) ? 0 : 1;
}

static double pr_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int cct_project_cmd_bench(const char *self_path, const cct_project_options_t *options) {
    int iterations = options->iterations > 0 ? options->iterations : 5;

    char cwd[CCT_PROJECT_PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "[bench] error: could not determine current directory\n");
        return 2;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, NULL, &layout, err, sizeof(err))) {
        fprintf(stderr, "[bench] error: %s\n", err);
        return 2;
    }

    char bench_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout.root_path, "bench", bench_dir, sizeof(bench_dir))) {
        fprintf(stderr, "[bench] error: bench directory path is too long\n");
        return 2;
    }

    cct_project_path_list_t files = {0};
    if (cct_project_path_is_dir(bench_dir)) {
        if (!pr_collect_with_suffix_recursive(bench_dir, ".bench.cct", &files)) {
            fprintf(stderr, "[bench] error: failed while scanning bench directory\n");
            pr_path_list_dispose(&files);
            return 2;
        }
    }

    qsort(files.items, files.count, sizeof(char *), pr_cmp_str);
    pr_filter_pattern(&files, options->pattern);

    printf("[bench] selected: %zu\n", files.count);
    if (files.count == 0) {
        pr_path_list_dispose(&files);
        return 0;
    }

    int failures = 0;

    for (size_t i = 0; i < files.count; i++) {
        const char *path = files.items[i];
        int compile_rc = pr_run_cct_compile(self_path, path);
        if (compile_rc != 0) {
            printf("[bench] FAIL %s (compile)\n", path);
            failures++;
            continue;
        }

        double total_ms = 0.0;
        int iter_fail = 0;
        for (int j = 0; j < iterations; j++) {
            double start_ms = pr_now_ms();
            int run_rc = pr_execute_compiled_binary(path);
            double end_ms = pr_now_ms();
            if (run_rc != 0) {
                iter_fail = run_rc;
                break;
            }
            total_ms += (end_ms - start_ms);
        }

        if (iter_fail != 0) {
            printf("[bench] FAIL %s (exit=%d)\n", path, iter_fail);
            failures++;
            continue;
        }

        printf("[bench] %s avg=%.3fms total=%.3fms\n", path, total_ms / (double)iterations, total_ms);
    }

    pr_path_list_dispose(&files);
    return failures == 0 ? 0 : 1;
}

int cct_project_cmd_clean(const cct_project_options_t *options) {
    char cwd[CCT_PROJECT_PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "[clean] error: could not determine current directory\n");
        return 1;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, NULL, &layout, err, sizeof(err))) {
        fprintf(stderr, "[clean] error: %s\n", err);
        return 1;
    }

    const char *targets[] = {
        ".cct/build",
        ".cct/cache",
        ".cct/test-bin",
        ".cct/bench-bin"
    };

    for (size_t i = 0; i < CCT_ARRAY_LEN(targets); i++) {
        char path[CCT_PROJECT_PATH_MAX];
        if (!cct_project_join_path(layout.root_path, targets[i], path, sizeof(path))) {
            continue;
        }
        if (cct_project_path_exists(path)) {
            pr_remove_tree_if_exists(path);
            printf("[clean] removed %s\n", path);
        }
    }

    if (options->clean_all) {
        char dist_dir[CCT_PROJECT_PATH_MAX];
        if (cct_project_join_path(layout.root_path, "dist", dist_dir, sizeof(dist_dir)) &&
            cct_project_path_is_dir(dist_dir)) {
            DIR *dp = opendir(dist_dir);
            if (dp) {
                struct dirent *ent = NULL;
                while ((ent = readdir(dp)) != NULL) {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
                    if (strncmp(ent->d_name, layout.project_name, strlen(layout.project_name)) != 0) continue;
                    char path[CCT_PROJECT_PATH_MAX];
                    if (!cct_project_join_path(dist_dir, ent->d_name, path, sizeof(path))) continue;
                    pr_remove_tree_if_exists(path);
                    printf("[clean] removed %s\n", path);
                }
                closedir(dp);
            }
        }
    }

    printf("[clean] done\n");
    return 0;
}
