/*
 * CCT — Clavicula Turing
 * Sigilo Parser Definitions
 *
 * FASE 14A: Sigilo inspection, validation, and diff tooling
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_SIGIL_PARSE_H
#define CCT_SIGIL_PARSE_H

#include "../common/types.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    CCT_SIGIL_PARSE_MODE_TOLERANT = 0,
    CCT_SIGIL_PARSE_MODE_STRICT = 1,
    CCT_SIGIL_PARSE_MODE_LEGACY_TOLERANT = 2,
    CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT = 3,
    CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT = 4
} cct_sigil_parse_mode_t;

typedef enum {
    CCT_SIGIL_DIAG_WARNING = 0,
    CCT_SIGIL_DIAG_ERROR = 1
} cct_sigil_diag_level_t;

typedef enum {
    CCT_SIGIL_PARSE_OK = 0,
    CCT_SIGIL_PARSE_SYNTAX,
    CCT_SIGIL_PARSE_TYPE,
    CCT_SIGIL_PARSE_MISSING_REQUIRED,
    CCT_SIGIL_PARSE_DUPLICATE_KEY,
    CCT_SIGIL_PARSE_UNKNOWN_FIELD,
    CCT_SIGIL_PARSE_DEPRECATED_FIELD,
    CCT_SIGIL_PARSE_SCHEMA_MISMATCH,
    CCT_SIGIL_PARSE_IO
} cct_sigil_diag_kind_t;

typedef struct {
    cct_sigil_diag_level_t level;
    cct_sigil_diag_kind_t kind;
    u32 line;
    u32 column;
    char *message;
} cct_sigil_diag_t;

typedef struct {
    cct_sigil_diag_t *items;
    size_t count;
    size_t capacity;
} cct_sigil_diag_list_t;

typedef struct {
    char *section;
    char *key;
    char *value;
    u32 line;
} cct_sigil_kv_t;

typedef struct {
    char *input_path;
    cct_sigil_parse_mode_t mode;

    char *format;
    char *sigilo_scope;
    char *visual_engine;
    char *semantic_hash;
    char *system_hash;
    bool has_analysis_summary;
    bool has_diff_fingerprint_context;
    bool has_module_structural_summary;
    bool has_compatibility_hints;

    cct_sigil_kv_t *entries;
    size_t entry_count;
    size_t entry_capacity;

    cct_sigil_diag_list_t diagnostics;
} cct_sigil_document_t;

bool cct_sigil_parse_file(
    const char *path,
    cct_sigil_parse_mode_t mode,
    cct_sigil_document_t *out_doc
);

bool cct_sigil_validate(
    cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode
);

void cct_sigil_document_dispose(cct_sigil_document_t *doc);

bool cct_sigil_document_has_errors(const cct_sigil_document_t *doc);

const char* cct_sigil_diag_kind_str(cct_sigil_diag_kind_t kind);

#endif /* CCT_SIGIL_PARSE_H */
