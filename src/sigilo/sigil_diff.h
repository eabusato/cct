/*
 * CCT — Clavicula Turing
 * Sigilo Diff Definitions
 *
 * FASE 14A: Sigilo inspection, validation, and diff tooling
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_SIGIL_DIFF_H
#define CCT_SIGIL_DIFF_H

#include "sigil_parse.h"
#include <stdio.h>

typedef enum {
    CCT_SIGIL_DIFF_SEVERITY_NONE = 0,
    CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL,
    CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED,
    CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK
} cct_sigil_diff_severity_t;

typedef enum {
    CCT_SIGIL_DIFF_ADDED = 0,
    CCT_SIGIL_DIFF_REMOVED,
    CCT_SIGIL_DIFF_CHANGED
} cct_sigil_diff_kind_t;

typedef struct {
    cct_sigil_diff_kind_t kind;
    cct_sigil_diff_severity_t severity;
    char *section;
    char *key;
    char *left_value;
    char *right_value;
} cct_sigil_diff_item_t;

typedef struct {
    cct_sigil_diff_item_t *items;
    size_t count;
    size_t capacity;
    cct_sigil_diff_severity_t highest_severity;
    size_t informational_count;
    size_t review_required_count;
    size_t behavioral_risk_count;
} cct_sigil_diff_result_t;

bool cct_sigil_diff_documents(
    const cct_sigil_document_t *left,
    const cct_sigil_document_t *right,
    cct_sigil_diff_result_t *out_result
);

void cct_sigil_diff_result_dispose(cct_sigil_diff_result_t *result);

bool cct_sigil_diff_render_text(
    FILE *out,
    const cct_sigil_diff_result_t *result,
    bool summary_only
);

bool cct_sigil_diff_render_structured(
    FILE *out,
    const cct_sigil_diff_result_t *result
);

const char* cct_sigil_diff_severity_str(cct_sigil_diff_severity_t severity);
const char* cct_sigil_diff_kind_str(cct_sigil_diff_kind_t kind);

#endif /* CCT_SIGIL_DIFF_H */
