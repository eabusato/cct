/*
 * CCT — Clavicula Turing
 * Sigil Validation Engine (FASE 13C.4)
 *
 * Strict/tolerant validation rules for parsed `.sigil` documents.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_SIGIL_VALIDATE_H
#define CCT_SIGIL_VALIDATE_H

#include "sigil_parse.h"

#define CCT_SIGIL_VALIDATE_MAX_ISSUES 128
#define CCT_SIGIL_VALIDATE_MAX_MESSAGE 256

typedef struct {
    cct_sigil_diag_level_t level;
    cct_sigil_diag_kind_t kind;
    u32 line;
    u32 column;
    char message[CCT_SIGIL_VALIDATE_MAX_MESSAGE];
} cct_sigil_validation_issue_t;

typedef struct {
    cct_sigil_validation_issue_t items[CCT_SIGIL_VALIDATE_MAX_ISSUES];
    size_t count;
} cct_sigil_validation_result_t;

bool cct_sigil_validate_collect(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    cct_sigil_validation_result_t *out_result
);

#endif /* CCT_SIGIL_VALIDATE_H */
