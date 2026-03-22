/*
 * CCT — Clavicula Turing
 * Code Generator Internal Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_CODEGEN_INTERNAL_H
#define CCT_CODEGEN_INTERNAL_H

#include "codegen.h"

#include <stdio.h>

typedef enum {
    CCT_CODEGEN_VALUE_UNKNOWN = 0,
    CCT_CODEGEN_VALUE_INT,
    CCT_CODEGEN_VALUE_BOOL,
    CCT_CODEGEN_VALUE_REAL,
    CCT_CODEGEN_VALUE_STRING,
    CCT_CODEGEN_VALUE_POINTER,
    CCT_CODEGEN_VALUE_STRUCT,
    CCT_CODEGEN_VALUE_ARRAY,
    CCT_CODEGEN_VALUE_NIHIL,
} cct_codegen_value_kind_t;

typedef enum {
    CCT_CODEGEN_LOCAL_UNSUPPORTED = 0,
    CCT_CODEGEN_LOCAL_INT,
    CCT_CODEGEN_LOCAL_BOOL,
    CCT_CODEGEN_LOCAL_STRING,
    CCT_CODEGEN_LOCAL_UMBRA,
    CCT_CODEGEN_LOCAL_FLAMMA,
    CCT_CODEGEN_LOCAL_POINTER,
    CCT_CODEGEN_LOCAL_ARRAY_INT,
    CCT_CODEGEN_LOCAL_ARRAY_BOOL,
    CCT_CODEGEN_LOCAL_ARRAY_STRING,
    CCT_CODEGEN_LOCAL_ARRAY_UMBRA,
    CCT_CODEGEN_LOCAL_ARRAY_FLAMMA,
    CCT_CODEGEN_LOCAL_STRUCT,
    CCT_CODEGEN_LOCAL_ORDO,
} cct_codegen_local_kind_t;

bool cct_cg_value_kind_is_numeric(cct_codegen_value_kind_t kind);
bool cct_cg_assignment_kind_compatible(cct_codegen_value_kind_t lhs, cct_codegen_value_kind_t rhs);

bool cct_cg_emit_generated_c_prelude(FILE *out, const cct_codegen_t *cg);
bool cct_cg_emit_runtime_fail_call(FILE *out, const char *message);
const char* cct_cg_runtime_scribe_helper_name(cct_codegen_value_kind_t kind);
const char* cct_cg_runtime_alloc_helper_name(void);
const char* cct_cg_runtime_free_helper_name(void);
const char* cct_cg_runtime_check_not_null_helper_name(void);
const char* cct_cg_runtime_check_not_null_fractum_helper_name(void);

#endif /* CCT_CODEGEN_INTERNAL_H */
