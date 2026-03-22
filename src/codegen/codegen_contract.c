/*
 * CCT — Clavicula Turing
 * Code Generator Contract Helpers Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "codegen_internal.h"

bool cct_cg_value_kind_is_numeric(cct_codegen_value_kind_t kind) {
    return kind == CCT_CODEGEN_VALUE_INT ||
           kind == CCT_CODEGEN_VALUE_BOOL ||
           kind == CCT_CODEGEN_VALUE_REAL;
}

bool cct_cg_assignment_kind_compatible(cct_codegen_value_kind_t lhs, cct_codegen_value_kind_t rhs) {
    if (lhs == rhs) return true;
    if (lhs == CCT_CODEGEN_VALUE_POINTER && rhs == CCT_CODEGEN_VALUE_POINTER) return true;
    if (lhs == CCT_CODEGEN_VALUE_INT && rhs == CCT_CODEGEN_VALUE_BOOL) return true;
    if (lhs == CCT_CODEGEN_VALUE_BOOL && rhs == CCT_CODEGEN_VALUE_INT) return true;
    if (lhs == CCT_CODEGEN_VALUE_REAL &&
        (rhs == CCT_CODEGEN_VALUE_INT || rhs == CCT_CODEGEN_VALUE_BOOL || rhs == CCT_CODEGEN_VALUE_REAL)) {
        return true;
    }
    if (lhs == CCT_CODEGEN_VALUE_INT && rhs == CCT_CODEGEN_VALUE_REAL) {
        /* Semantic analysis should typically reject narrowing; codegen keeps explicit rule to fail less obscurely. */
        return true;
    }
    return false;
}
