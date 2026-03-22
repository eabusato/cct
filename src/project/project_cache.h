/*
 * CCT — Clavicula Turing
 * Project Cache Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PROJECT_CACHE_H
#define CCT_PROJECT_CACHE_H

#include "project_discovery.h"

#include <stdbool.h>
#include <stddef.h>

#define CCT_PROJECT_FINGERPRINT_HEX_LEN 16

typedef struct {
    char fingerprint[CCT_PROJECT_FINGERPRINT_HEX_LEN + 1];
    char profile[16];
    char output_path[CCT_PROJECT_PATH_MAX];
} cct_project_cache_record_t;

bool cct_project_cache_compute_fingerprint(
    const cct_project_layout_t *layout,
    const char *profile,
    char *out_fingerprint,
    size_t out_fingerprint_size,
    char *error_message,
    size_t error_message_size
);

bool cct_project_cache_load(const char *cache_file, cct_project_cache_record_t *out_record);

bool cct_project_cache_store(const char *cache_file, const cct_project_cache_record_t *record);

bool cct_project_cache_is_up_to_date(
    const cct_project_cache_record_t *old_record,
    const cct_project_cache_record_t *new_record,
    bool output_exists
);

#endif
