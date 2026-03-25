/*
 * CCT — Clavicula Turing
 * Code Generator Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_CODEGEN_H
#define CCT_CODEGEN_H

#include "../common/types.h"
#include "../common/errors.h"
#include "../parser/ast.h"

typedef struct cct_codegen_string cct_codegen_string_t;
typedef struct cct_codegen_local cct_codegen_local_t;
typedef struct cct_codegen_rituale cct_codegen_rituale_t;
typedef struct cct_codegen_sigillum cct_codegen_sigillum_t;
typedef struct cct_codegen_ordo cct_codegen_ordo_t;
typedef struct cct_codegen_fail_handler cct_codegen_fail_handler_t;
typedef struct cct_codegen_genus_inst cct_codegen_genus_inst_t;

typedef enum {
    CCT_CODEGEN_BACKEND_C_HOST = 0, /* Official backend in FASE 6C */
} cct_codegen_backend_kind_t;

typedef struct {
    const char *filename;           /* source .cct filename for diagnostics */
    bool had_error;
    u32 error_count;

    /* Output naming chosen for current compilation */
    char *input_path;
    char *output_executable_path;
    char *intermediate_c_path;

    /* Backend config */
    cct_codegen_backend_kind_t backend_kind; /* backend selection (future-proofing) */
    const char *host_cc;            /* defaults to "cc" */
    bool keep_intermediate;         /* preserve generated .c file */
    bool uses_sqlite;               /* link sqlite bridge only when generated code needs it */
    bool uses_crypto;               /* link crypto bridge only when generated code needs it */
    bool uses_regex;                /* link regex bridge only when generated code needs it */
    bool uses_toml;                 /* link TOML bridge only when generated code needs it */
    bool uses_compress;             /* link gzip bridge only when generated code needs it */
    bool uses_filetype;             /* link filetype bridge only when generated code needs it */
    bool uses_image_ops;            /* link image bridge only when generated code needs it */
    cct_profile_t profile;          /* FASE 16A.2: compilation profile */
    const char *entry_rituale_name; /* FASE 16B.4: explicit freestanding entry ritual */

    /* Internal state (opaque to callers) */
    cct_codegen_string_t *strings;
    u32 next_string_id;
    u32 next_temp_id;
    cct_codegen_rituale_t *rituales;
    cct_codegen_sigillum_t *sigilla;
    cct_codegen_ordo_t *ordines;
    cct_codegen_local_t *locals;
    u32 scope_depth;
    u32 loop_depth;
    u32 quando_switch_depth;          /* nesting depth of QUANDO ORDO payload switch blocks */
    u32 loop_break_ids[32];           /* break label IDs per loop nesting level */
    const cct_ast_node_t *entry_rituale;
    bool current_function_returns_nihil;
    const cct_ast_type_t *current_function_return_type;
    cct_codegen_fail_handler_t *fail_handlers;
    const cct_ast_program_t *source_program; /* Program being materialized/emitted */
    cct_codegen_genus_inst_t *genus_instances; /* Materialized generic instances (FASE 10B) */
} cct_codegen_t;

void cct_codegen_init(cct_codegen_t *cg, const char *filename);
void cct_codegen_dispose(cct_codegen_t *cg);
void cct_codegen_set_backend(cct_codegen_t *cg, cct_codegen_backend_kind_t backend_kind);

bool cct_codegen_had_error(const cct_codegen_t *cg);
u32 cct_codegen_error_count(const cct_codegen_t *cg);

/*
 * Compile a semantic-valid AST program to a native executable using the
 * official C-hosted backend (generated C + host compiler).
 *
 * If output_executable_path is NULL, the executable path is derived from the
 * input source path by removing the `.cct` extension.
 */
bool cct_codegen_compile_program(
    cct_codegen_t *cg,
    const cct_ast_program_t *program,
    const char *input_path,
    const char *output_executable_path
);

#endif /* CCT_CODEGEN_H */
