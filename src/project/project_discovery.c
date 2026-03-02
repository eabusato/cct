#include "project_discovery.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void pd_set_error(char *out, size_t out_size, const char *message) {
    if (!out || out_size == 0) return;
    snprintf(out, out_size, "%s", message ? message : "unknown project discovery error");
}

bool cct_project_join_path(const char *left, const char *right, char *out, size_t out_size) {
    if (!left || !right || !out || out_size == 0) return false;
    size_t left_len = strlen(left);
    bool needs_slash = (left_len > 0 && left[left_len - 1] != '/');
    int wrote = snprintf(out, out_size, "%s%s%s", left, needs_slash ? "/" : "", right);
    return wrote > 0 && (size_t)wrote < out_size;
}

bool cct_project_path_exists(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0;
}

bool cct_project_path_is_dir(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool pd_path_is_file(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void pd_trim(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
}

static bool pd_manifest_get_value(const char *manifest_path,
                                  const char *section,
                                  const char *key,
                                  char *out,
                                  size_t out_size) {
    if (!manifest_path || !section || !key || !out || out_size == 0) return false;

    FILE *f = fopen(manifest_path, "r");
    if (!f) return false;

    char line[1024];
    char current_section[1024] = "";
    bool found = false;

    while (fgets(line, sizeof(line), f)) {
        pd_trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        size_t line_len = strlen(line);
        if (line[0] == '[' && line_len > 2 && line[line_len - 1] == ']') {
            line[line_len - 1] = '\0';
            snprintf(current_section, sizeof(current_section), "%s", line + 1);
            continue;
        }

        if (strcmp(current_section, section) != 0) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        char lhs[256];
        char rhs[768];
        snprintf(lhs, sizeof(lhs), "%s", line);
        snprintf(rhs, sizeof(rhs), "%s", eq + 1);
        pd_trim(lhs);
        pd_trim(rhs);

        if (strcmp(lhs, key) != 0) continue;

        if (rhs[0] == '"') {
            size_t rlen = strlen(rhs);
            if (rlen >= 2 && rhs[rlen - 1] == '"') {
                rhs[rlen - 1] = '\0';
                memmove(rhs, rhs + 1, strlen(rhs));
            }
        }

        snprintf(out, out_size, "%s", rhs);
        found = true;
        break;
    }

    fclose(f);
    return found;
}

static bool pd_resolve_dir(const char *input, char *out, size_t out_size) {
    if (!input || !out || out_size == 0) return false;

    char stack_buf[PATH_MAX];
    if (realpath(input, stack_buf)) {
        snprintf(out, out_size, "%s", stack_buf);
        return true;
    }

    if (!cct_project_path_is_dir(input)) return false;
    snprintf(out, out_size, "%s", input);
    return true;
}

static bool pd_find_project_root_from(const char *start_dir, char *out_root, size_t out_size) {
    if (!start_dir || !out_root || out_size == 0) return false;

    char cursor[CCT_PROJECT_PATH_MAX];
    if (!pd_resolve_dir(start_dir, cursor, sizeof(cursor))) {
        return false;
    }

    while (true) {
        char manifest[CCT_PROJECT_PATH_MAX];
        char src_main[CCT_PROJECT_PATH_MAX];

        if (!cct_project_join_path(cursor, "cct.toml", manifest, sizeof(manifest))) return false;
        if (!cct_project_join_path(cursor, "src/main.cct", src_main, sizeof(src_main))) return false;

        if (pd_path_is_file(manifest) || pd_path_is_file(src_main)) {
            snprintf(out_root, out_size, "%s", cursor);
            return true;
        }

        if (strcmp(cursor, "/") == 0) {
            break;
        }

        char *slash = strrchr(cursor, '/');
        if (!slash) break;
        if (slash == cursor) {
            cursor[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    return false;
}

static void pd_basename(const char *path, char *out, size_t out_size) {
    if (!path || !out || out_size == 0) return;
    const char *last = strrchr(path, '/');
    const char *base = last ? last + 1 : path;
    if (base[0] == '\0') base = "cct_project";
    snprintf(out, out_size, "%s", base);
}

bool cct_project_discover(const char *cwd,
                          const char *project_override,
                          const char *entry_override,
                          cct_project_layout_t *out_layout,
                          char *error_message,
                          size_t error_message_size) {
    if (!out_layout) {
        pd_set_error(error_message, error_message_size, "internal project discovery error");
        return false;
    }
    memset(out_layout, 0, sizeof(*out_layout));

    char root[CCT_PROJECT_PATH_MAX];
    memset(root, 0, sizeof(root));

    if (project_override) {
        if (!pd_resolve_dir(project_override, root, sizeof(root))) {
            pd_set_error(error_message, error_message_size,
                         "project root from --project was not found or is not a directory");
            return false;
        }
    } else {
        const char *start = cwd;
        char cwd_buf[CCT_PROJECT_PATH_MAX];
        if (!start) {
            if (!getcwd(cwd_buf, sizeof(cwd_buf))) {
                pd_set_error(error_message, error_message_size,
                             "could not determine current directory");
                return false;
            }
            start = cwd_buf;
        }

        if (!pd_find_project_root_from(start, root, sizeof(root))) {
            pd_set_error(error_message, error_message_size,
                         "project root not found (expected cct.toml or src/main.cct); use --project <dir>");
            return false;
        }
    }

    char manifest_path[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(root, "cct.toml", manifest_path, sizeof(manifest_path))) {
        pd_set_error(error_message, error_message_size, "project root path is too long");
        return false;
    }

    out_layout->has_manifest = pd_path_is_file(manifest_path);

    char project_name[128] = "";
    if (out_layout->has_manifest) {
        (void)pd_manifest_get_value(manifest_path, "project", "name", project_name, sizeof(project_name));
    }
    if (project_name[0] == '\0') {
        pd_basename(root, project_name, sizeof(project_name));
    }

    char manifest_entry[CCT_PROJECT_PATH_MAX] = "";
    if (out_layout->has_manifest) {
        (void)pd_manifest_get_value(manifest_path, "project", "entry", manifest_entry, sizeof(manifest_entry));
    }

    char entry_path[CCT_PROJECT_PATH_MAX];
    if (entry_override && entry_override[0] != '\0') {
        if (entry_override[0] == '/') {
            snprintf(entry_path, sizeof(entry_path), "%s", entry_override);
        } else if (!cct_project_join_path(root, entry_override, entry_path, sizeof(entry_path))) {
            pd_set_error(error_message, error_message_size, "entry path is too long");
            return false;
        }
    } else if (manifest_entry[0] != '\0') {
        if (manifest_entry[0] == '/') {
            snprintf(entry_path, sizeof(entry_path), "%s", manifest_entry);
        } else if (!cct_project_join_path(root, manifest_entry, entry_path, sizeof(entry_path))) {
            pd_set_error(error_message, error_message_size, "manifest entry path is too long");
            return false;
        }
    } else if (!cct_project_join_path(root, "src/main.cct", entry_path, sizeof(entry_path))) {
        pd_set_error(error_message, error_message_size, "default entry path is too long");
        return false;
    }

    if (!pd_path_is_file(entry_path)) {
        pd_set_error(error_message, error_message_size,
                     "entry file not found (expected src/main.cct or explicit --entry)");
        return false;
    }

    char out_dir_rel[CCT_PROJECT_PATH_MAX] = "";
    if (out_layout->has_manifest) {
        (void)pd_manifest_get_value(manifest_path, "build", "out_dir", out_dir_rel, sizeof(out_dir_rel));
    }
    if (out_dir_rel[0] == '\0') {
        snprintf(out_dir_rel, sizeof(out_dir_rel), "dist");
    }

    char out_dir_abs[CCT_PROJECT_PATH_MAX];
    if (out_dir_rel[0] == '/') {
        snprintf(out_dir_abs, sizeof(out_dir_abs), "%s", out_dir_rel);
    } else if (!cct_project_join_path(root, out_dir_rel, out_dir_abs, sizeof(out_dir_abs))) {
        pd_set_error(error_message, error_message_size, "output directory path is too long");
        return false;
    }

    snprintf(out_layout->root_path, sizeof(out_layout->root_path), "%s", root);
    snprintf(out_layout->entry_path, sizeof(out_layout->entry_path), "%s", entry_path);
    snprintf(out_layout->out_dir, sizeof(out_layout->out_dir), "%s", out_dir_abs);
    snprintf(out_layout->project_name, sizeof(out_layout->project_name), "%s", project_name);
    return true;
}
