/*
 * CCT — Clavicula Turing
 * Diagnostic Helpers Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_COMMON_DIAGNOSTIC_H
#define CCT_COMMON_DIAGNOSTIC_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CCT_DIAG_ERROR = 0,
    CCT_DIAG_WARNING,
    CCT_DIAG_NOTE,
    CCT_DIAG_HINT,
} cct_diagnostic_level_t;

typedef struct {
    cct_diagnostic_level_t level;
    const char *message;
    const char *file_path;
    u32 line;
    u32 column;
    const char *suggestion;      /* optional */
    const char *code_label;      /* optional, e.g. "Semantic error" */
} cct_diagnostic_t;

void cct_diagnostic_set_colors(bool enabled);
bool cct_diagnostic_colors_enabled(void);
const char* cct_diagnostic_level_text(cct_diagnostic_level_t level);

void cct_diagnostic_emit(const cct_diagnostic_t *diag);

void cct_error(const char *message);
void cct_error_at(const char *file, u32 line, u32 column, const char *message);
void cct_error_with_suggestion(const char *file, u32 line, u32 column, const char *message, const char *suggestion);
void cct_warning(const char *message);
void cct_note(const char *message);
void cct_hint(const char *message);

char* cct_read_source_line(const char *file_path, u32 line_number);

#endif /* CCT_COMMON_DIAGNOSTIC_H */
