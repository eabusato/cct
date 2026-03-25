/*
 * CCT — Clavicula Turing
 * Sigilo Generation Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_SIGILO_H
#define CCT_SIGILO_H

#include "../common/types.h"
#include "../common/errors.h"
#include "../parser/ast.h"

typedef struct {
    const char *filename;
    bool had_error;
    u32 error_count;

    char *input_path;
    char *svg_path;
    char *sigil_meta_path;

    /* Deterministic structural signature produced for the last program */
    u64 semantic_hash;
    char semantic_hash_hex[17];

    /* Small set of exported metrics for callers/debug */
    u32 ritual_count;
    u32 total_statements;
    u32 max_nesting;

    /* FASE 9A modular metadata context */
    u32 module_count;
    u32 import_edge_count;
    char *entry_module_path;
    u32 cross_module_call_count;      /* FASE 9B */
    u32 cross_module_type_ref_count;  /* FASE 9B */
    bool module_resolution_ok;        /* FASE 9B */
    u32 public_symbol_count;          /* FASE 9C */
    u32 internal_symbol_count;        /* FASE 9C */

    /* FASE 6A sigilo generation controls */
    bool emit_svg;
    bool emit_meta;
    bool emit_titles;
    bool emit_data_attrs;
    const char *style_name; /* "network" (default), "seal", "scriptum", "routes" */
    const char *manifest_path; /* FASE 35D: optional external manifest path */
} cct_sigilo_t;

void cct_sigilo_init(cct_sigilo_t *sg, const char *filename);
void cct_sigilo_dispose(cct_sigilo_t *sg);

bool cct_sigilo_had_error(const cct_sigilo_t *sg);
u32 cct_sigilo_error_count(const cct_sigilo_t *sg);

bool cct_sigilo_set_style(cct_sigilo_t *sg, const char *style_name);
const char* cct_sigilo_get_style(const cct_sigilo_t *sg);
void cct_sigilo_set_module_context(
    cct_sigilo_t *sg,
    u32 module_count,
    u32 import_edge_count,
    const char *entry_module_path,
    u32 cross_module_call_count,
    u32 cross_module_type_ref_count,
    bool module_resolution_ok,
    u32 public_symbol_count,
    u32 internal_symbol_count
);

/*
 * Generate sigil artifacts from a semantic-valid AST.
 *
 * If output_base_path is NULL, derive base path by stripping `.cct` from
 * input_path. Files generated depend on `sg->emit_svg` / `sg->emit_meta`:
 *   <base>.svg
 *   <base>.sigil
 */
bool cct_sigilo_generate_artifacts(
    cct_sigilo_t *sg,
    const cct_ast_program_t *program,
    const char *input_path,
    const char *output_base_path
);

/*
 * Generate composed multi-module system sigilo artifacts (.system.svg/.system.sigil).
 *
 * `module_paths` and import edge arrays must follow deterministic closure order.
 * Edge arrays use indices into `module_paths`.
 */
bool cct_sigilo_generate_system_artifacts(
    cct_sigilo_t *sg,
    const char *entry_input_path,
    const char *output_base_path,
    const cct_ast_program_t *const *module_programs,
    const char *const *module_paths,
    u32 module_count,
    const u32 *import_from_indices,
    const u32 *import_to_indices,
    u32 import_edge_count,
    u32 cross_module_call_count,
    u32 cross_module_type_ref_count,
    u32 public_symbol_count,
    u32 internal_symbol_count,
    bool module_resolution_ok
);

#endif /* CCT_SIGILO_H */
