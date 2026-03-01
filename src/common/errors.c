/*
 * CCT — Clavicula Turing
 * Error Handling System Implementation
 *
 * FASE 0: Base error system implementation
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "errors.h"
#include "diagnostic.h"
#include <stdio.h>
#include <stdarg.h>

/*
 * Get human-readable error message for error code
 */
const char* cct_error_string(cct_error_code_t code) {
    switch (code) {
        case CCT_OK:
            return "Success";

        /* CLI errors */
        case CCT_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case CCT_ERROR_UNKNOWN_COMMAND:
            return "Unknown command";
        case CCT_ERROR_MISSING_ARGUMENT:
            return "Missing required argument";

        /* File I/O errors */
        case CCT_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case CCT_ERROR_FILE_READ:
            return "Error reading file";
        case CCT_ERROR_FILE_WRITE:
            return "Error writing file";

        /* Compilation errors (placeholders for future phases) */
        case CCT_ERROR_LEXICAL:
            return "Lexical error";
        case CCT_ERROR_SYNTAX:
            return "Syntax error";
        case CCT_ERROR_SEMANTIC:
            return "Semantic error";
        case CCT_ERROR_CODEGEN:
            return "Code generation error";

        /* Internal errors */
        case CCT_ERROR_INTERNAL:
            return "Internal compiler error";
        case CCT_ERROR_NOT_IMPLEMENTED:
            return "Feature not yet implemented";
        case CCT_ERROR_OUT_OF_MEMORY:
            return "Out of memory";

        default:
            return "Unknown error";
    }
}

/*
 * Print error message to stderr
 */
void cct_error_print(cct_error_code_t code, const char *message) {
    fprintf(stderr, "cct: error: %s", message);
    if (code != CCT_OK) {
        fprintf(stderr, " [%s]", cct_error_string(code));
    }
    fprintf(stderr, "\n");
}

/*
 * Print error with formatted message
 */
void cct_error_printf(cct_error_code_t code, const char *format, ...) {
    va_list args;

    fprintf(stderr, "cct: error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    if (code != CCT_OK) {
        fprintf(stderr, " [%s]", cct_error_string(code));
    }
    fprintf(stderr, "\n");
}

/*
 * Print error with file location context
 * This will be heavily used in future phases (lexer, parser, semantic)
 */
void cct_error_at_location(
    cct_error_code_t code,
    const char *filename,
    u32 line,
    u32 column,
    const char *message
) {
    cct_error_at_location_with_suggestion(code, filename, line, column, message, NULL);
}

void cct_error_at_location_with_suggestion(
    cct_error_code_t code,
    const char *filename,
    u32 line,
    u32 column,
    const char *message,
    const char *suggestion
) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_ERROR,
        .message = message ? message : "unknown error",
        .file_path = filename,
        .line = line,
        .column = column,
        .suggestion = suggestion,
        .code_label = (code != CCT_OK) ? cct_error_string(code) : NULL,
    };
    cct_diagnostic_emit(&diag);
}
