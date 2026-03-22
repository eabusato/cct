/*
 * CCT — Clavicula Turing
 * Module System Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_MODULE_H
#define CCT_MODULE_H

#include "../common/types.h"
#include "../common/errors.h"
#include "../parser/ast.h"

typedef enum {
    CCT_MODULE_ORIGIN_USER = 0,
    CCT_MODULE_ORIGIN_STDLIB = 1
} cct_module_origin_t;

typedef struct {
    cct_ast_program_t *program;   /* Composed program (entry + imported declarations) */
    u32 module_count;             /* Number of unique loaded modules */
    u32 import_edge_count;        /* Number of ADVOCARE edges discovered */
    char *entry_module_path;      /* Entry module path as requested by CLI */
    char **module_paths;          /* Canonical module paths in deterministic closure order */
    u32 module_path_count;        /* Count for module_paths */
    u32 *import_from_indices;     /* Deterministic module graph edges (from index) */
    u32 *import_to_indices;       /* Deterministic module graph edges (to index) */
    u32 import_graph_edge_count;  /* Count for import_from/to arrays */
    cct_module_origin_t *module_origins; /* Origin per module (user vs stdlib) */
    u32 stdlib_module_count;      /* Number of stdlib modules in closure */
    u32 cross_module_call_count;  /* FASE 9B metrics */
    u32 cross_module_type_ref_count;
    bool module_resolution_ok;    /* true when inter-module resolution pass succeeds */
    u32 public_symbol_count;      /* FASE 9C: top-level visibility metrics */
    u32 internal_symbol_count;
} cct_module_bundle_t;

/*
 * Build deterministic module closure from entry file.
 *
 * On success:
 *   - returns true
 *   - out_bundle is initialized and owns allocated memory
 *
 * On failure:
 *   - returns false
 *   - diagnostics are already emitted
 *   - out_error receives best-effort error code
 */
bool cct_module_bundle_build(
    const char *entry_input_path,
    cct_profile_t profile,
    cct_module_bundle_t *out_bundle,
    cct_error_code_t *out_error
);

void cct_module_bundle_dispose(cct_module_bundle_t *bundle);

#endif /* CCT_MODULE_H */
