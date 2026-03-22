/*
 * CCT — Clavicula Turing
 * Sigilo Baseline Persistence Definitions
 *
 * FASE 13A.4: Sigilo baseline persistence
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PROJECT_SIGILO_BASELINE_H
#define CCT_PROJECT_SIGILO_BASELINE_H

#include <stdbool.h>
#include <stddef.h>

#include "../sigilo/sigil_parse.h"

bool cct_sigilo_baseline_file_exists(const char *path);

bool cct_sigilo_baseline_resolve_path(
    const cct_sigil_document_t *artifact_doc,
    const char *override_path,
    char **out_baseline_path,
    char **out_error
);

bool cct_sigilo_baseline_compute_meta_path(
    const char *baseline_path,
    char **out_meta_path,
    char **out_error
);

bool cct_sigilo_baseline_validate_meta(
    const char *meta_path,
    bool *out_exists,
    char **out_error
);

bool cct_sigilo_baseline_write(
    const char *artifact_path,
    const cct_sigil_document_t *artifact_doc,
    const char *baseline_path,
    bool force,
    char **out_meta_path,
    char **out_error
);

#endif /* CCT_PROJECT_SIGILO_BASELINE_H */
