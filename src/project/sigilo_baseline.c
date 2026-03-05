/*
 * CCT — Clavicula Turing
 * Sigilo Baseline Persistence (FASE 13A.4)
 */

#include "sigilo_baseline.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static char* sb_strdup(const char *s) {
    if (!s) return NULL;
    return strdup(s);
}

static char* sb_strdup_printf1(const char *fmt, const char *a) {
    if (!fmt || !a) return NULL;
    size_t need = (size_t)snprintf(NULL, 0, fmt, a) + 1;
    char *buf = (char*)malloc(need);
    if (!buf) return NULL;
    (void)snprintf(buf, need, fmt, a);
    return buf;
}

static char* sb_errorf(const char *fmt, const char *a) {
    if (!fmt || !a) return NULL;
    size_t need = (size_t)snprintf(NULL, 0, fmt, a) + 1;
    char *buf = (char*)malloc(need);
    if (!buf) return NULL;
    (void)snprintf(buf, need, fmt, a);
    return buf;
}

static bool sb_ensure_parent_dirs(const char *path, char **out_error) {
    if (!path) {
        if (out_error) *out_error = sb_strdup("invalid baseline path");
        return false;
    }

    char *tmp = sb_strdup(path);
    if (!tmp) {
        if (out_error) *out_error = sb_strdup("out of memory while preparing baseline directories");
        return false;
    }

    char *last_slash = strrchr(tmp, '/');
    if (!last_slash) {
        free(tmp);
        return true;
    }
    *last_slash = '\0';

    if (tmp[0] == '\0') {
        free(tmp);
        return true;
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            if (out_error) *out_error = sb_errorf("could not create directory: %s", tmp);
            free(tmp);
            return false;
        }
        *p = '/';
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        if (out_error) *out_error = sb_errorf("could not create directory: %s", tmp);
        free(tmp);
        return false;
    }

    free(tmp);
    return true;
}

bool cct_sigilo_baseline_file_exists(const char *path) {
    if (!path) return false;
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

bool cct_sigilo_baseline_resolve_path(
    const cct_sigil_document_t *artifact_doc,
    const char *override_path,
    char **out_baseline_path,
    char **out_error
) {
    if (out_baseline_path) *out_baseline_path = NULL;
    if (out_error) *out_error = NULL;

    if (!out_baseline_path) return false;

    if (override_path && override_path[0] != '\0') {
        *out_baseline_path = sb_strdup(override_path);
        if (!*out_baseline_path) {
            if (out_error) *out_error = sb_strdup("out of memory while resolving baseline path");
            return false;
        }
        return true;
    }

    const char *scope = (artifact_doc && artifact_doc->sigilo_scope) ? artifact_doc->sigilo_scope : "local";
    const char *name = (strcmp(scope, "system") == 0) ? "system" : "local";
    *out_baseline_path = sb_strdup_printf1("docs/sigilo/baseline/%s.sigil", name);
    if (!*out_baseline_path) {
        if (out_error) *out_error = sb_strdup("out of memory while resolving default baseline path");
        return false;
    }
    return true;
}

bool cct_sigilo_baseline_compute_meta_path(
    const char *baseline_path,
    char **out_meta_path,
    char **out_error
) {
    if (out_meta_path) *out_meta_path = NULL;
    if (out_error) *out_error = NULL;

    if (!baseline_path || !out_meta_path) return false;

    size_t n = strlen(baseline_path);
    size_t ext = (n >= 6 && strcmp(baseline_path + n - 6, ".sigil") == 0) ? 6 : 0;
    size_t root_n = n - ext;

    char *meta = (char*)malloc(root_n + strlen(".baseline.meta") + 1);
    if (!meta) {
        if (out_error) *out_error = sb_strdup("out of memory while computing baseline meta path");
        return false;
    }

    memcpy(meta, baseline_path, root_n);
    meta[root_n] = '\0';
    strcat(meta, ".baseline.meta");
    *out_meta_path = meta;
    return true;
}

bool cct_sigilo_baseline_validate_meta(
    const char *meta_path,
    bool *out_exists,
    char **out_error
) {
    if (out_exists) *out_exists = false;
    if (out_error) *out_error = NULL;

    if (!meta_path) return false;
    if (!cct_sigilo_baseline_file_exists(meta_path)) {
        if (out_exists) *out_exists = false;
        return true;
    }

    if (out_exists) *out_exists = true;

    FILE *f = fopen(meta_path, "rb");
    if (!f) {
        if (out_error) *out_error = sb_errorf("could not open baseline metadata file: %s", meta_path);
        return false;
    }

    char line[512];
    bool has_format = false;
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
        if (*s == '\0' || *s == '#') continue;

        if (strncmp(s, "format", 6) == 0) {
            char *eq = strchr(s, '=');
            if (!eq) continue;
            eq++;
            while (*eq == ' ' || *eq == '\t') eq++;
            char *eol = eq + strlen(eq);
            while (eol > eq && (eol[-1] == '\r' || eol[-1] == '\n' || eol[-1] == ' ' || eol[-1] == '\t')) {
                eol--;
            }
            *eol = '\0';
            if (strcmp(eq, "cct.sigil.baseline.meta.v1") != 0) {
                fclose(f);
                if (out_error) *out_error = sb_errorf("unsupported baseline metadata format in: %s", meta_path);
                return false;
            }
            has_format = true;
            break;
        }
    }

    fclose(f);

    if (!has_format) {
        if (out_error) *out_error = sb_errorf("baseline metadata missing required field 'format': %s", meta_path);
        return false;
    }

    return true;
}

static bool sb_copy_file(const char *from, const char *to, char **out_error) {
    FILE *src = fopen(from, "rb");
    if (!src) {
        if (out_error) *out_error = sb_errorf("could not read artifact file: %s", from);
        return false;
    }

    FILE *dst = fopen(to, "wb");
    if (!dst) {
        fclose(src);
        if (out_error) *out_error = sb_errorf("could not write baseline file: %s", to);
        return false;
    }

    char buf[4096];
    bool ok = true;
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            ok = false;
            break;
        }
    }
    if (ferror(src)) ok = false;

    fclose(src);
    if (fclose(dst) != 0) ok = false;

    if (!ok) {
        if (out_error) *out_error = sb_errorf("failed to copy baseline from artifact: %s", from);
        return false;
    }
    return true;
}

bool cct_sigilo_baseline_write(
    const char *artifact_path,
    const cct_sigil_document_t *artifact_doc,
    const char *baseline_path,
    bool force,
    char **out_meta_path,
    char **out_error
) {
    if (out_meta_path) *out_meta_path = NULL;
    if (out_error) *out_error = NULL;

    if (!artifact_path || !artifact_doc || !baseline_path) {
        if (out_error) *out_error = sb_strdup("invalid arguments for baseline update");
        return false;
    }

    if (cct_sigilo_baseline_file_exists(baseline_path) && !force) {
        if (out_error) *out_error = sb_errorf("baseline already exists (use --force to overwrite): %s", baseline_path);
        return false;
    }

    if (!sb_ensure_parent_dirs(baseline_path, out_error)) {
        return false;
    }

    if (!sb_copy_file(artifact_path, baseline_path, out_error)) {
        return false;
    }

    char *meta_path = NULL;
    if (!cct_sigilo_baseline_compute_meta_path(baseline_path, &meta_path, out_error)) {
        return false;
    }

    FILE *meta = fopen(meta_path, "wb");
    if (!meta) {
        if (out_error) *out_error = sb_errorf("could not write baseline metadata file: %s", meta_path);
        free(meta_path);
        return false;
    }

    const char *scope = artifact_doc->sigilo_scope ? artifact_doc->sigilo_scope : "local";
    const char *sig_format = artifact_doc->format ? artifact_doc->format : "";

    fprintf(meta, "format = cct.sigil.baseline.meta.v1\n");
    fprintf(meta, "baseline_schema = 1\n");
    fprintf(meta, "sigilo_scope = %s\n", scope);
    fprintf(meta, "artifact_format = %s\n", sig_format);
    fprintf(meta, "source_artifact = %s\n", artifact_path);

    if (fclose(meta) != 0) {
        if (out_error) *out_error = sb_errorf("could not finalize baseline metadata file: %s", meta_path);
        free(meta_path);
        return false;
    }

    if (out_meta_path) {
        *out_meta_path = meta_path;
    } else {
        free(meta_path);
    }

    return true;
}
