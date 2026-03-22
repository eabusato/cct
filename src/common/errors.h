/*
 * CCT — Clavicula Turing
 * Error Handling Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_COMMON_ERRORS_H
#define CCT_COMMON_ERRORS_H

#include "types.h"

/*
 * Error codes
 *
 * These are designed to be extensible for future phases:
 * - FASE 1: Lexical errors
 * - FASE 2: Syntax errors
 * - FASE 3: Semantic errors
 * - FASE 4+: Codegen and runtime errors
 */
typedef enum {
    CCT_OK = 0,

    /* CLI and usage/contract errors (1-19) */
    CCT_ERROR_INVALID_ARGUMENT = 1,
    CCT_ERROR_CONTRACT_VIOLATION = 2,
    CCT_ERROR_MISSING_ARGUMENT = 3,
    CCT_ERROR_UNKNOWN_COMMAND = 4,

    /* File I/O errors (20-39) */
    CCT_ERROR_FILE_NOT_FOUND = 20,
    CCT_ERROR_FILE_READ = 21,
    CCT_ERROR_FILE_WRITE = 22,

    /* Compilation errors (40-99) - reserved for future phases */
    CCT_ERROR_LEXICAL = 40,     /* FASE 1 */
    CCT_ERROR_SYNTAX = 50,      /* FASE 2 */
    CCT_ERROR_SEMANTIC = 60,    /* FASE 3 */
    CCT_ERROR_CODEGEN = 70,     /* FASE 4 */

    /* Internal errors (100+) */
    CCT_ERROR_INTERNAL = 100,
    CCT_ERROR_NOT_IMPLEMENTED = 101,
    CCT_ERROR_OUT_OF_MEMORY = 102,

} cct_error_code_t;

/*
 * Error reporting functions
 */

/* Print error message to stderr */
void cct_error_print(cct_error_code_t code, const char *message);

/* Print error with formatted message */
void cct_error_printf(cct_error_code_t code, const char *format, ...);

/* Print error with file location context (for future phases) */
void cct_error_at_location(
    cct_error_code_t code,
    const char *filename,
    u32 line,
    u32 column,
    const char *message
);

/* Print error with file location context + correction suggestion */
void cct_error_at_location_with_suggestion(
    cct_error_code_t code,
    const char *filename,
    u32 line,
    u32 column,
    const char *message,
    const char *suggestion
);

/* Get error message string for a given error code */
const char* cct_error_string(cct_error_code_t code);

#endif /* CCT_COMMON_ERRORS_H */
