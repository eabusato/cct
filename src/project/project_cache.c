/*
 * CCT — Clavicula Turing
 * Project Cache Implementation
 *
 * FASE 12C: Build cache and fingerprinting for incremental compilation
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "project_cache.h"

#include "../common/types.h"
#include "../module/module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pc_set_error(char *out, size_t out_size, const char *message) {
    if (!out || out_size == 0) return;
    snprintf(out, out_size, "%s", message ? message : "unknown cache error");
}

static void pc_hash_bytes(u64 *state, const unsigned char *bytes, size_t n) {
    if (!state || !bytes) return;
    const u64 prime = 1099511628211ULL;
    for (size_t i = 0; i < n; i++) {
        *state ^= (u64)bytes[i];
        *state *= prime;
    }
}

static void pc_hash_cstr(u64 *state, const char *s) {
    if (!state || !s) return;
    pc_hash_bytes(state, (const unsigned char *)s, strlen(s));
}

static bool pc_hash_file(u64 *state, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    unsigned char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        pc_hash_bytes(state, buf, n);
    }
    fclose(f);
    return true;
}

bool cct_project_cache_compute_fingerprint(
    const cct_project_layout_t *layout,
    const char *profile,
    char *out_fingerprint,
    size_t out_fingerprint_size,
    char *error_message,
    size_t error_message_size
) {
    if (!layout || !profile || !out_fingerprint || out_fingerprint_size < CCT_PROJECT_FINGERPRINT_HEX_LEN + 1) {
        pc_set_error(error_message, error_message_size, "invalid fingerprint arguments");
        return false;
    }

    cct_module_bundle_t bundle;
    cct_error_code_t build_status = CCT_OK;
    if (!cct_module_bundle_build(layout->entry_path, CCT_PROFILE_HOST, &bundle, &build_status)) {
        (void)build_status;
        pc_set_error(error_message, error_message_size,
                     "could not compute project fingerprint from module closure");
        return false;
    }

    u64 h = 1469598103934665603ULL;
    pc_hash_cstr(&h, profile);
    pc_hash_cstr(&h, CCT_VERSION_STRING);
    pc_hash_cstr(&h, layout->entry_path);

    for (u32 i = 0; i < bundle.module_path_count; i++) {
        const char *path = bundle.module_paths[i];
        if (!path) continue;
        pc_hash_cstr(&h, path);
        if (!pc_hash_file(&h, path)) {
            cct_module_bundle_dispose(&bundle);
            pc_set_error(error_message, error_message_size,
                         "could not hash one of the module files");
            return false;
        }
    }

    snprintf(out_fingerprint, out_fingerprint_size, "%016llx", (unsigned long long)h);
    cct_module_bundle_dispose(&bundle);
    return true;
}

bool cct_project_cache_load(const char *cache_file, cct_project_cache_record_t *out_record) {
    if (!cache_file || !out_record) return false;

    FILE *f = fopen(cache_file, "r");
    if (!f) return false;

    memset(out_record, 0, sizeof(*out_record));
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line;
        char *value = eq + 1;

        size_t n = strlen(value);
        while (n > 0 && (value[n - 1] == '\n' || value[n - 1] == '\r')) {
            value[--n] = '\0';
        }

        if (strcmp(key, "fingerprint") == 0) {
            snprintf(out_record->fingerprint, sizeof(out_record->fingerprint), "%s", value);
        } else if (strcmp(key, "profile") == 0) {
            snprintf(out_record->profile, sizeof(out_record->profile), "%s", value);
        } else if (strcmp(key, "output") == 0) {
            snprintf(out_record->output_path, sizeof(out_record->output_path), "%s", value);
        }
    }

    fclose(f);

    return out_record->fingerprint[0] != '\0';
}

bool cct_project_cache_store(const char *cache_file, const cct_project_cache_record_t *record) {
    if (!cache_file || !record) return false;
    FILE *f = fopen(cache_file, "w");
    if (!f) return false;

    fprintf(f, "fingerprint=%s\n", record->fingerprint);
    fprintf(f, "profile=%s\n", record->profile);
    fprintf(f, "output=%s\n", record->output_path);
    fclose(f);
    return true;
}

bool cct_project_cache_is_up_to_date(
    const cct_project_cache_record_t *old_record,
    const cct_project_cache_record_t *new_record,
    bool output_exists
) {
    if (!old_record || !new_record || !output_exists) return false;
    if (strcmp(old_record->fingerprint, new_record->fingerprint) != 0) return false;
    if (strcmp(old_record->profile, new_record->profile) != 0) return false;
    if (strcmp(old_record->output_path, new_record->output_path) != 0) return false;
    return true;
}
