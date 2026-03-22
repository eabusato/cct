/*
 * CCT — Clavicula Turing
 * Formatter Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_FORMATTER_H
#define CCT_FORMATTER_H

#include "../common/errors.h"
#include "../common/types.h"
#include <stdio.h>

typedef struct {
    int indent_size;
} cct_formatter_options_t;

typedef struct {
    bool success;
    bool changed;
    cct_error_code_t error_code;
    char *original_source;
    char *formatted_source;
    char *error_message;
} cct_formatter_result_t;

cct_formatter_options_t cct_formatter_default_options(void);

cct_formatter_result_t cct_formatter_format_file(
    const char *file_path,
    const cct_formatter_options_t *options
);

bool cct_formatter_write_file(const char *file_path, const char *content);

void cct_formatter_print_diff(
    FILE *out,
    const char *file_path,
    const char *original_source,
    const char *formatted_source
);

void cct_formatter_result_dispose(cct_formatter_result_t *result);

#endif /* CCT_FORMATTER_H */
