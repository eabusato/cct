/*
 * CCT — Clavicula Turing
 * Project Runner Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "project_runner.h"

#include "project_cache.h"
#include "sigilo_baseline.h"
#include "../common/errors.h"
#include "../sigilo/sigil_diff.h"
#include "../sigilo/sigil_parse.h"

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

static bool pr_sigil_path_from_source(const char *source_path, char *out, size_t out_size) {
    if (!source_path || !out || out_size == 0) return false;
    size_t len = strlen(source_path);
    if (len < 4 || strcmp(source_path + len - 4, ".cct") != 0) return false;
    if (len - 4 + strlen(".sigil") + 1 > out_size) return false;
    memcpy(out, source_path, len - 4);
    out[len - 4] = '\0';
    strcat(out, ".sigil");
    return true;
}

static bool pr_system_sigil_path_from_source(const char *source_path, char *out, size_t out_size) {
    if (!source_path || !out || out_size == 0) return false;
    size_t len = strlen(source_path);
    if (len < 4 || strcmp(source_path + len - 4, ".cct") != 0) return false;
    if (len - 4 + strlen(".system.sigil") + 1 > out_size) return false;
    memcpy(out, source_path, len - 4);
    out[len - 4] = '\0';
    strcat(out, ".system.sigil");
    return true;
}

static bool pr_source_references_imports(const char *source_path) {
    if (!source_path) return false;
    FILE *f = fopen(source_path, "rb");
    if (!f) return false;

    char line[512];
    bool has_imports = false;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "ADVOCARE")) {
            has_imports = true;
            break;
        }
    }
    fclose(f);
    return has_imports;
}

static bool pr_select_sigilo_artifact_path(const char *source_path, char *out, size_t out_size) {
    char system_sigil[CCT_PROJECT_PATH_MAX];
    if (pr_system_sigil_path_from_source(source_path, system_sigil, sizeof(system_sigil))) {
        if (pr_source_references_imports(source_path) || cct_project_path_exists(system_sigil)) {
            snprintf(out, out_size, "%s", system_sigil);
            return true;
        }
    }
    return pr_sigil_path_from_source(source_path, out, out_size);
}

static bool pr_is_absolute_path(const char *path) {
    if (!path || path[0] == '\0') return false;
#ifdef _WIN32
    if ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) {
        if (path[1] == ':' && (path[2] == '/' || path[2] == '\\')) return true;
    }
    if (path[0] == '\\' && path[1] == '\\') return true;
#endif
    return path[0] == '/';
}

static bool pr_resolve_sigilo_baseline_path(const cct_project_layout_t *layout,
                                            const cct_project_options_t *options,
                                            const char *artifact_path,
                                            char *baseline,
                                            size_t baseline_size) {
    if (!layout || !options || !baseline || baseline_size == 0) return false;
    baseline[0] = '\0';

    if (options->sigilo_baseline_override && options->sigilo_baseline_override[0] != '\0') {
        if (pr_is_absolute_path(options->sigilo_baseline_override)) {
            snprintf(baseline, baseline_size, "%s", options->sigilo_baseline_override);
            return true;
        }
        return cct_project_join_path(layout->root_path, options->sigilo_baseline_override, baseline, baseline_size);
    }

    char docs_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout->root_path, "docs/sigilo/baseline", docs_dir, sizeof(docs_dir))) {
        return false;
    }
    const char *default_name = pr_ends_with(artifact_path, ".system.sigil") ? "system.sigil" : "local.sigil";
    return cct_project_join_path(docs_dir, default_name, baseline, baseline_size);
}

static bool pr_sigilo_artifact_has_scope_header(const char *artifact_sigil) {
    if (!artifact_sigil) return false;
    FILE *f = fopen(artifact_sigil, "rb");
    if (!f) return false;
    char line[512];
    bool has_scope = false;
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (strncmp(s, "sigilo_scope", strlen("sigilo_scope")) == 0) {
            has_scope = true;
            break;
        }
    }
    fclose(f);
    return has_scope;
}

static const char* pr_sigilo_report_mode_str(cct_project_sigilo_report_mode_t mode);

static int pr_run_sigilo_baseline_check(const char *self_path,
                                        const char *artifact_sigil,
                                        const char *baseline_path,
                                        bool strict,
                                        cct_project_sigilo_report_mode_t report_mode,
                                        bool explain,
                                        const char *command_tag) {
    if (!pr_sigilo_artifact_has_scope_header(artifact_sigil)) {
        fprintf(stderr,
                "[%s] warning: sigilo-check skipped for legacy artifact schema (missing sigilo_scope): %s\n",
                command_tag ? command_tag : "project",
                artifact_sigil ? artifact_sigil : "<none>");
        return 0;
    }

    char *argv_local[11];
    size_t idx = 0;

    argv_local[idx++] = (char *)self_path;
    argv_local[idx++] = "sigilo";
    argv_local[idx++] = "baseline";
    argv_local[idx++] = "check";
    argv_local[idx++] = (char *)artifact_sigil;
    argv_local[idx++] = "--baseline";
    argv_local[idx++] = (char *)baseline_path;
    argv_local[idx++] = "--summary";
    if (strict) {
        argv_local[idx++] = "--strict";
    }
    argv_local[idx] = NULL;

    int rc = pr_spawn_argv(argv_local);
    bool blocked = (rc != 0);
    fprintf(stderr,
            "[%s] sigilo.report format=cct.sigilo.report.v1 command=%s profile=baseline mode=%s artifact=%s baseline=%s highest=%s decision=%s\n",
            command_tag ? command_tag : "project",
            command_tag ? command_tag : "project",
            pr_sigilo_report_mode_str(report_mode),
            artifact_sigil ? artifact_sigil : "<none>",
            baseline_path ? baseline_path : "<none>",
            (rc == 2) ? "review-required-or-behavioral-risk" : "none-or-informational",
            blocked ? "fail" : "pass");
    fprintf(stderr,
            "[%s] sigilo.report.summary next_action=%s\n",
            command_tag ? command_tag : "project",
            blocked ? "run `cct sigilo baseline check <artifact> --baseline <path> --format structured` and review drift"
                    : "no action required");
    if (report_mode == CCT_PROJECT_SIGILO_REPORT_DETAILED) {
        fprintf(stderr,
                "[%s] sigilo.report.detail note=baseline-mode-details-are-available-via-`cct sigilo baseline check ... --format structured`\n",
                command_tag ? command_tag : "project");
    }
    if (explain) {
        fprintf(stderr,
                "[%s] sigilo.explain probable_cause=%s recommended_action=%s docs=docs/sigilo_troubleshooting_13b4.md blocked=%s profile=baseline\n",
                command_tag ? command_tag : "project",
                blocked ? "baseline drift blocked by strict policy or artifact/baseline contract error"
                        : "baseline comparison completed without blocking drift",
                blocked ? "inspect drift output and update baseline explicitly only after review"
                        : "no immediate action required",
                blocked ? "true" : "false");
    }
    if (rc != 0) {
        fprintf(stderr,
                "[%s] error: sigilo-check failed (artifact=%s baseline=%s exit=%d)\n",
                command_tag ? command_tag : "project",
                artifact_sigil ? artifact_sigil : "<none>",
                baseline_path ? baseline_path : "<none>",
                rc);
    }
    return rc;
}

static const char* pr_sigilo_ci_profile_str(cct_project_sigilo_ci_profile_t profile) {
    switch (profile) {
        case CCT_PROJECT_SIGILO_CI_PROFILE_ADVISORY: return "advisory";
        case CCT_PROJECT_SIGILO_CI_PROFILE_GATED: return "gated";
        case CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE: return "release";
        case CCT_PROJECT_SIGILO_CI_PROFILE_NONE:
        default: return "none";
    }
}

static const char* pr_sigilo_report_mode_str(cct_project_sigilo_report_mode_t mode) {
    return mode == CCT_PROJECT_SIGILO_REPORT_DETAILED ? "detailed" : "summary";
}

static const char* pr_sigilo_next_action(cct_sigil_diff_severity_t highest,
                                         cct_project_sigilo_ci_profile_t profile,
                                         bool baseline_missing,
                                         bool blocked) {
    if (baseline_missing) {
        if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE) {
            return "create baseline via `cct sigilo baseline update <artifact> --baseline <path>`";
        }
        return "baseline missing is informative in this profile; create baseline to start drift tracking";
    }
    if (blocked) {
        if (highest == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK) {
            return "inspect behavioral drift; update baseline only after explicit review";
        }
        if (highest == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
            return "review diff and update baseline explicitly if approved";
        }
    }
    if (highest == CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL) {
        return "record informational drift; consider baseline update when accepted";
    }
    return "no action required";
}

static void pr_print_sigilo_ci_report_header(const char *command_tag,
                                             const cct_project_options_t *options,
                                             const char *profile,
                                             const char *artifact_sigil,
                                             const char *baseline_path,
                                             const char *highest,
                                             bool blocked) {
    fprintf(stderr,
            "[%s] sigilo.report format=cct.sigilo.report.v1 command=%s profile=%s mode=%s artifact=%s baseline=%s highest=%s decision=%s\n",
            command_tag ? command_tag : "project",
            command_tag ? command_tag : "project",
            profile ? profile : "none",
            pr_sigilo_report_mode_str(options ? options->sigilo_report_mode : CCT_PROJECT_SIGILO_REPORT_SUMMARY),
            artifact_sigil ? artifact_sigil : "<none>",
            baseline_path ? baseline_path : "<none>",
            highest ? highest : "none",
            blocked ? "fail" : "pass");
}

static void pr_print_sigilo_ci_report_detail(const char *command_tag,
                                             const cct_sigil_diff_result_t *diff) {
    if (!diff) return;
    for (size_t i = 0; i < diff->count; i++) {
        const cct_sigil_diff_item_t *it = &diff->items[i];
        fprintf(stderr,
                "[%s] sigilo.report.item[%zu] severity=%s domain=%s/%s kind=%s before=%s after=%s\n",
                command_tag ? command_tag : "project",
                i,
                cct_sigil_diff_severity_str(it->severity),
                (it->section && it->section[0] != '\0') ? it->section : "root",
                it->key ? it->key : "<none>",
                cct_sigil_diff_kind_str(it->kind),
                it->left_value ? it->left_value : "<none>",
                it->right_value ? it->right_value : "<none>");
    }
}

static void pr_print_sigilo_ci_explain(const char *command_tag,
                                       cct_sigil_diff_severity_t highest,
                                       cct_project_sigilo_ci_profile_t profile,
                                       bool baseline_missing,
                                       bool blocked) {
    const char *cause = "no actionable drift detected";
    if (baseline_missing) {
        cause = "baseline file is missing";
    } else if (highest == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK) {
        cause = "critical metadata drift (format/scope contract) was detected";
    } else if (highest == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
        cause = "review-required metadata drift (hash/topology contract) was detected";
    } else if (highest == CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL) {
        cause = "informational metadata drift was detected";
    }

    fprintf(stderr,
            "[%s] sigilo.explain probable_cause=%s recommended_action=%s docs=docs/sigilo_troubleshooting_13b4.md blocked=%s profile=%s\n",
            command_tag ? command_tag : "project",
            cause,
            pr_sigilo_next_action(highest, profile, baseline_missing, blocked),
            blocked ? "true" : "false",
            pr_sigilo_ci_profile_str(profile));
}

static int pr_run_sigilo_ci_gate(const cct_project_options_t *options,
                                 const char *artifact_sigil,
                                 const char *baseline_path,
                                 const char *command_tag) {
    cct_project_sigilo_ci_profile_t profile = options ? options->sigilo_ci_profile : CCT_PROJECT_SIGILO_CI_PROFILE_NONE;
    if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_NONE) return 0;

    if (!pr_sigilo_artifact_has_scope_header(artifact_sigil)) {
        fprintf(stderr,
                "[%s] warning: sigilo CI profile '%s' skipped for legacy artifact schema (missing sigilo_scope): %s\n",
                command_tag ? command_tag : "project",
                pr_sigilo_ci_profile_str(profile),
                artifact_sigil ? artifact_sigil : "<none>");
        return 0;
    }

    cct_sigil_parse_mode_t parse_mode =
        (profile == CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE || (options && options->sigilo_strict))
            ? CCT_SIGIL_PARSE_MODE_STRICT
            : CCT_SIGIL_PARSE_MODE_TOLERANT;

    cct_sigil_document_t artifact_doc;
    if (!cct_sigil_parse_file(artifact_sigil, parse_mode, &artifact_doc)) {
        fprintf(stderr, "[%s] error: sigilo CI gate failed while parsing artifact\n", command_tag ? command_tag : "project");
        cct_sigil_document_dispose(&artifact_doc);
        return 1;
    }
    if (cct_sigil_document_has_errors(&artifact_doc)) {
        fprintf(stderr, "[%s] error: sigilo CI gate detected invalid artifact schema (%s)\n",
                command_tag ? command_tag : "project", artifact_sigil ? artifact_sigil : "<none>");
        cct_sigil_document_dispose(&artifact_doc);
        return 1;
    }

    if (!cct_sigilo_baseline_file_exists(baseline_path)) {
        bool blocked = (profile == CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE);
        pr_print_sigilo_ci_report_header(command_tag,
                                         options,
                                         pr_sigilo_ci_profile_str(profile),
                                         artifact_sigil,
                                         baseline_path,
                                         "none",
                                         blocked);
        fprintf(stderr,
                "[%s] sigilo.ci profile=%s status=missing baseline=%s artifact=%s\n",
                command_tag ? command_tag : "project",
                pr_sigilo_ci_profile_str(profile),
                baseline_path ? baseline_path : "<none>",
                artifact_sigil ? artifact_sigil : "<none>");
        cct_sigil_document_dispose(&artifact_doc);
        if (options && options->sigilo_explain) {
            pr_print_sigilo_ci_explain(command_tag,
                                       CCT_SIGIL_DIFF_SEVERITY_NONE,
                                       profile,
                                       true,
                                       blocked);
        }
        if (blocked) {
            fprintf(stderr, "[%s] error: release profile requires baseline presence\n", command_tag ? command_tag : "project");
            return (int)CCT_ERROR_CONTRACT_VIOLATION;
        }
        return 0;
    }

    char *meta_path = NULL;
    char *meta_error = NULL;
    bool meta_exists = false;
    if (!cct_sigilo_baseline_compute_meta_path(baseline_path, &meta_path, &meta_error)) {
        fprintf(stderr,
                "[%s] error: sigilo CI gate could not resolve baseline metadata path (%s)\n",
                command_tag ? command_tag : "project",
                meta_error ? meta_error : "unknown");
        free(meta_error);
        cct_sigil_document_dispose(&artifact_doc);
        return 1;
    }
    if (!cct_sigilo_baseline_validate_meta(meta_path, &meta_exists, &meta_error)) {
        fprintf(stderr,
                "[%s] error: sigilo CI gate rejected baseline metadata (%s)\n",
                command_tag ? command_tag : "project",
                meta_error ? meta_error : "unknown");
        free(meta_error);
        free(meta_path);
        cct_sigil_document_dispose(&artifact_doc);
        return 1;
    }

    cct_sigil_document_t baseline_doc;
    if (!cct_sigil_parse_file(baseline_path, parse_mode, &baseline_doc)) {
        fprintf(stderr, "[%s] error: sigilo CI gate failed while parsing baseline\n", command_tag ? command_tag : "project");
        free(meta_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return 1;
    }
    if (cct_sigil_document_has_errors(&baseline_doc)) {
        fprintf(stderr, "[%s] error: sigilo CI gate detected invalid baseline schema\n", command_tag ? command_tag : "project");
        free(meta_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return 1;
    }

    cct_sigil_diff_result_t diff;
    if (!cct_sigil_diff_documents(&baseline_doc, &artifact_doc, &diff)) {
        fprintf(stderr, "[%s] error: sigilo CI gate failed during diff computation\n", command_tag ? command_tag : "project");
        free(meta_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return 1;
    }

    const char *highest = cct_sigil_diff_severity_str(diff.highest_severity);
    fprintf(stderr,
            "[%s] sigilo.ci profile=%s highest=%s total=%zu informational=%zu review_required=%zu behavioral_risk=%zu baseline=%s meta=%s\n",
            command_tag ? command_tag : "project",
            pr_sigilo_ci_profile_str(profile),
            highest ? highest : "unknown",
            diff.count,
            diff.informational_count,
            diff.review_required_count,
            diff.behavioral_risk_count,
            baseline_path ? baseline_path : "<none>",
            meta_exists ? "present" : "missing");

    bool allow = true;
    bool audit_override = false;
    if (diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK) {
        if (options && options->sigilo_override_behavioral_risk) {
            allow = true;
            audit_override = true;
        } else {
            allow = false;
        }
    } else if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_GATED || profile == CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE) {
        if (diff.highest_severity >= CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
            allow = false;
        }
    } else if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_ADVISORY) {
        allow = true;
    }

    if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_RELEASE &&
        diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL) {
        fprintf(stderr, "[%s] warning: release profile observed informational sigilo drift\n", command_tag ? command_tag : "project");
    }
    if (profile == CCT_PROJECT_SIGILO_CI_PROFILE_ADVISORY &&
        diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
        fprintf(stderr, "[%s] warning: advisory profile detected review-required drift (non-blocking)\n", command_tag ? command_tag : "project");
    }
    if (audit_override) {
        fprintf(stderr,
                "[%s] warning: sigilo override engaged (--sigilo-override-behavioral-risk). audit=true\n",
                command_tag ? command_tag : "project");
    }

    int rc = allow ? 0 : 2;
    pr_print_sigilo_ci_report_header(command_tag,
                                     options,
                                     pr_sigilo_ci_profile_str(profile),
                                     artifact_sigil,
                                     baseline_path,
                                     highest,
                                     !allow);
    fprintf(stderr,
            "[%s] sigilo.report.summary total=%zu informational=%zu review_required=%zu behavioral_risk=%zu next_action=%s\n",
            command_tag ? command_tag : "project",
            diff.count,
            diff.informational_count,
            diff.review_required_count,
            diff.behavioral_risk_count,
            pr_sigilo_next_action(diff.highest_severity, profile, false, !allow));
    if (options && options->sigilo_report_mode == CCT_PROJECT_SIGILO_REPORT_DETAILED) {
        pr_print_sigilo_ci_report_detail(command_tag, &diff);
    }
    if (options && options->sigilo_explain) {
        pr_print_sigilo_ci_explain(command_tag,
                                   diff.highest_severity,
                                   profile,
                                   false,
                                   !allow);
    }

    if (!allow) {
        fprintf(stderr, "[%s] error: sigilo CI gate blocked by profile '%s'\n",
                command_tag ? command_tag : "project",
                pr_sigilo_ci_profile_str(profile));
    }

    cct_sigil_diff_result_dispose(&diff);
    free(meta_path);
    cct_sigil_document_dispose(&artifact_doc);
    cct_sigil_document_dispose(&baseline_doc);
    return rc;
}

static int pr_run_sigilo_stage(const char *self_path,
                               const cct_project_options_t *options,
                               const char *artifact_sigil,
                               const char *baseline_path,
                               const char *command_tag) {
    if (options && options->sigilo_ci_profile != CCT_PROJECT_SIGILO_CI_PROFILE_NONE) {
        return pr_run_sigilo_ci_gate(options, artifact_sigil, baseline_path, command_tag);
    }
    return pr_run_sigilo_baseline_check(self_path,
                                        artifact_sigil,
                                        baseline_path,
                                        options ? options->sigilo_strict : false,
                                        options ? options->sigilo_report_mode : CCT_PROJECT_SIGILO_REPORT_SUMMARY,
                                        options ? options->sigilo_explain : false,
                                        command_tag);
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

    char entry_sigil_path[CCT_PROJECT_PATH_MAX];
    if (options->sigilo_check &&
        !pr_select_sigilo_artifact_path(layout.entry_path, entry_sigil_path, sizeof(entry_sigil_path))) {
        fprintf(stderr, "[build] error: could not derive sigilo artifact path for entry module\n");
        return 1;
    }

    char sigilo_baseline_path[CCT_PROJECT_PATH_MAX];
    if (options->sigilo_check &&
        !pr_resolve_sigilo_baseline_path(&layout, options, entry_sigil_path, sigilo_baseline_path, sizeof(sigilo_baseline_path))) {
        fprintf(stderr, "[build] error: invalid --sigilo-baseline path\n");
        return 1;
    }

    if (options->run_lint) {
        int lint_rc = pr_run_cct_lint(self_path, layout.entry_path);
        if (lint_rc != 0) {
            fprintf(stderr, "[build] error: lint gate failed for entry module\n");
            return (int)CCT_ERROR_CONTRACT_VIOLATION;
        }
    }

    if (options->fmt_check) {
        int fmt_rc = pr_run_cct_fmt_check(self_path, layout.entry_path);
        if (fmt_rc != 0) {
            fprintf(stderr, "[build] error: fmt-check gate failed for entry module\n");
            return (int)CCT_ERROR_CONTRACT_VIOLATION;
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

    bool cache_up_to_date = has_old && new_record.fingerprint[0] != '\0' &&
                            cct_project_cache_is_up_to_date(&old_record, &new_record, out_exists);
    bool missing_sigilo_for_cache = options->sigilo_check && !cct_project_path_exists(entry_sigil_path);
    if (cache_up_to_date && missing_sigilo_for_cache) {
        fprintf(stderr, "[build] warning: incremental cache hit but sigilo artifact is missing; forcing rebuild\n");
    }

    if (cache_up_to_date && !missing_sigilo_for_cache) {
        printf("[build] status: up-to-date\n");
        printf("[build] output: %s\n", output_path);
        if (built_output && built_output_size > 0) {
            snprintf(built_output, built_output_size, "%s", output_path);
        }
        if (options->sigilo_check) {
            int sigilo_rc = pr_run_sigilo_stage(self_path,
                                                options,
                                                entry_sigil_path,
                                                sigilo_baseline_path,
                                                "build");
            if (sigilo_rc != 0) return sigilo_rc;
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

    if (options->sigilo_check) {
        if (!pr_select_sigilo_artifact_path(layout.entry_path, entry_sigil_path, sizeof(entry_sigil_path))) {
            fprintf(stderr, "[build] error: could not resolve sigilo artifact path after build\n");
            return 1;
        }
        if (!pr_resolve_sigilo_baseline_path(&layout, options, entry_sigil_path, sigilo_baseline_path, sizeof(sigilo_baseline_path))) {
            fprintf(stderr, "[build] error: invalid --sigilo-baseline path\n");
            return 1;
        }
        int sigilo_rc = pr_run_sigilo_stage(self_path,
                                            options,
                                            entry_sigil_path,
                                            sigilo_baseline_path,
                                            "build");
        if (sigilo_rc != 0) return sigilo_rc;
    }

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
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, NULL, &layout, err, sizeof(err))) {
        fprintf(stderr, "[test] error: %s\n", err);
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    char tests_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout.root_path, "tests", tests_dir, sizeof(tests_dir))) {
        fprintf(stderr, "[test] error: tests directory path is too long\n");
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    cct_project_path_list_t files = {0};
    if (cct_project_path_is_dir(tests_dir)) {
        if (!pr_collect_with_suffix_recursive(tests_dir, ".test.cct", &files)) {
            fprintf(stderr, "[test] error: failed while scanning tests directory\n");
            pr_path_list_dispose(&files);
            return (int)CCT_ERROR_CONTRACT_VIOLATION;
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
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
        }

        if (options->strict_lint) {
            int lint_rc = pr_run_cct_lint_strict(self_path, path);
            if (lint_rc != 0) {
                fprintf(stderr, "[test] quality gate (strict-lint) failed: %s\n", path);
                pr_path_list_dispose(&files);
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
        }

        int compile_rc = pr_run_cct_compile(self_path, path);
        if (compile_rc != 0) {
            printf("[test] FAIL %s (compile)\n", path);
            fail++;
            continue;
        }

        if (options->sigilo_check) {
            char sigil_path[CCT_PROJECT_PATH_MAX];
            if (!pr_select_sigilo_artifact_path(path, sigil_path, sizeof(sigil_path))) {
                fprintf(stderr, "[test] error: could not derive sigilo artifact path for %s\n", path);
                pr_path_list_dispose(&files);
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
            char sigilo_baseline_path[CCT_PROJECT_PATH_MAX];
            if (!pr_resolve_sigilo_baseline_path(&layout, options, sigil_path, sigilo_baseline_path, sizeof(sigilo_baseline_path))) {
                fprintf(stderr, "[test] error: invalid --sigilo-baseline path\n");
                pr_path_list_dispose(&files);
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
            int sigilo_rc = pr_run_sigilo_stage(self_path,
                                                options,
                                                sigil_path,
                                                sigilo_baseline_path,
                                                "test");
            if (sigilo_rc != 0) {
                pr_path_list_dispose(&files);
                return sigilo_rc;
            }
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
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, options->project_override, NULL, &layout, err, sizeof(err))) {
        fprintf(stderr, "[bench] error: %s\n", err);
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    char bench_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(layout.root_path, "bench", bench_dir, sizeof(bench_dir))) {
        fprintf(stderr, "[bench] error: bench directory path is too long\n");
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
    }

    cct_project_path_list_t files = {0};
    if (cct_project_path_is_dir(bench_dir)) {
        if (!pr_collect_with_suffix_recursive(bench_dir, ".bench.cct", &files)) {
            fprintf(stderr, "[bench] error: failed while scanning bench directory\n");
            pr_path_list_dispose(&files);
            return (int)CCT_ERROR_CONTRACT_VIOLATION;
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

        if (options->sigilo_check) {
            char sigil_path[CCT_PROJECT_PATH_MAX];
            if (!pr_select_sigilo_artifact_path(path, sigil_path, sizeof(sigil_path))) {
                fprintf(stderr, "[bench] error: could not derive sigilo artifact path for %s\n", path);
                pr_path_list_dispose(&files);
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
            char sigilo_baseline_path[CCT_PROJECT_PATH_MAX];
            if (!pr_resolve_sigilo_baseline_path(&layout, options, sigil_path, sigilo_baseline_path, sizeof(sigilo_baseline_path))) {
                fprintf(stderr, "[bench] error: invalid --sigilo-baseline path\n");
                pr_path_list_dispose(&files);
                return (int)CCT_ERROR_CONTRACT_VIOLATION;
            }
            int sigilo_rc = pr_run_sigilo_stage(self_path,
                                                options,
                                                sigil_path,
                                                sigilo_baseline_path,
                                                "bench");
            if (sigilo_rc != 0) {
                pr_path_list_dispose(&files);
                return sigilo_rc;
            }
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
