/*
 * CCT — Clavicula Turing
 * Runtime Helper Emission Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_RUNTIME_H
#define CCT_RUNTIME_H

#include "../common/types.h"

#include <stdio.h>

typedef struct {
    bool emit_scribe_helpers;
    bool emit_fail_helper;
    bool emit_memory_helpers;
    bool emit_fluxus_helpers;
    bool emit_io_helpers;
    bool emit_fs_helpers;
    bool emit_path_helpers;
    bool emit_random_helpers;
    bool emit_process_helpers;
    bool emit_hash_helpers;
    bool emit_verbum_helpers;
    bool emit_fmt_helpers;
    bool emit_db_helpers;
} cct_runtime_codegen_config_t;

void cct_runtime_codegen_config_defaults(cct_runtime_codegen_config_t *cfg);

/*
 * Emit C helper functions used by the generated C backend (`.cgen.c`).
 * Returns false only for invalid arguments.
 */
bool cct_runtime_emit_c_helpers(FILE *out, const cct_runtime_codegen_config_t *cfg);

#endif /* CCT_RUNTIME_H */
