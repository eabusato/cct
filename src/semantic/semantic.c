/*
 * CCT — Clavicula Turing
 * Semantic Analyzer Implementation
 *
 * FASE 3: Semantic analysis and type checking (subset-focused)
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "semantic.h"
#include "../common/fuzzy.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cct_sem_type_alloc {
    cct_sem_type_t *type;
    cct_sem_type_alloc_t *next;
};

struct cct_sem_scope_alloc {
    cct_sem_scope_t *scope;
    cct_sem_scope_alloc_t *next;
};

struct cct_sem_symbol_alloc {
    cct_sem_symbol_t *symbol;
    cct_sem_symbol_alloc_t *next;
};

struct cct_sem_generic_instance {
    char *name; /* Canonical: Base<Arg1,...> */
    const cct_ast_node_t *decl; /* Generic SIGILLUM declaration */
    cct_sem_type_t **type_args;
    size_t type_arg_count;
    cct_sem_generic_instance_t *next;
};

typedef struct {
    const cct_ast_param_list_t *params;
    cct_sem_type_t **types;
    size_t count;
} cct_sem_signature_tmp_t;

static void sem_reportf(cct_semantic_analyzer_t *sem, u32 line, u32 col, const char *fmt, ...);
static void sem_report_with_suggestion(cct_semantic_analyzer_t *sem, u32 line, u32 col, const char *message, const char *suggestion);
static cct_sem_type_t* sem_resolve_ast_type(cct_semantic_analyzer_t *sem, const cct_ast_type_t *ast_type, u32 line, u32 col);

/* ========================================================================
 * Allocation Helpers
 * ======================================================================== */

static void* sem_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return ptr;
}

static char* sem_strdup(const char *s) {
    if (!s) return NULL;
    char *copy = strdup(s);
    if (!copy) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return copy;
}

static bool sem_active_type_params_contains(const cct_semantic_analyzer_t *sem, const char *name) {
    if (!sem || !name) return false;
    for (size_t i = 0; i < sem->active_type_param_count; i++) {
        if (sem->active_type_param_names[i] && strcmp(sem->active_type_param_names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static bool sem_active_type_params_reserve(cct_semantic_analyzer_t *sem, size_t needed) {
    if (needed <= sem->active_type_param_capacity) return true;
    size_t cap = sem->active_type_param_capacity ? sem->active_type_param_capacity * 2 : 8;
    while (cap < needed) cap *= 2;
    char **next = (char**)realloc(sem->active_type_param_names, cap * sizeof(char*));
    if (!next) {
        sem_reportf(sem, 0, 0, "out of memory while tracking GENUS type parameters");
        return false;
    }
    sem->active_type_param_names = next;
    sem->active_type_param_capacity = cap;
    return true;
}

static void sem_active_type_params_pop_to(cct_semantic_analyzer_t *sem, size_t mark) {
    if (!sem) return;
    while (sem->active_type_param_count > mark) {
        sem->active_type_param_count--;
        free(sem->active_type_param_names[sem->active_type_param_count]);
        sem->active_type_param_names[sem->active_type_param_count] = NULL;
    }
}

static size_t sem_active_type_params_push_from_list(
    cct_semantic_analyzer_t *sem,
    const cct_ast_type_param_list_t *list,
    const cct_ast_node_t *owner
) {
    size_t mark = sem ? sem->active_type_param_count : 0;
    if (!sem || !list) return mark;

    for (size_t i = 0; i < list->count; i++) {
        cct_ast_type_param_t *p = list->params[i];
        if (!p || !p->name) continue;

        for (size_t j = i + 1; j < list->count; j++) {
            cct_ast_type_param_t *q = list->params[j];
            if (q && q->name && strcmp(p->name, q->name) == 0) {
                sem_reportf(sem, q->line, q->column,
                            "duplicate GENUS parameter '%s' in %s declaration (subset 10A)",
                            q->name,
                            owner && owner->type == AST_SIGILLUM ? "SIGILLUM" : "RITUALE");
            }
        }

        if (!sem_active_type_params_reserve(sem, sem->active_type_param_count + 1)) {
            return mark;
        }
        sem->active_type_param_names[sem->active_type_param_count++] = sem_strdup(p->name);
    }

    return mark;
}

static cct_sem_scope_t* sem_alloc_scope(cct_semantic_analyzer_t *sem, cct_sem_scope_kind_t kind) {
    cct_sem_scope_t *scope = (cct_sem_scope_t*)sem_calloc(1, sizeof(*scope));
    scope->kind = kind;

    cct_sem_scope_alloc_t *node = (cct_sem_scope_alloc_t*)sem_calloc(1, sizeof(*node));
    node->scope = scope;
    node->next = sem->scope_alloc_list;
    sem->scope_alloc_list = node;

    return scope;
}

static cct_sem_symbol_t* sem_alloc_symbol(cct_semantic_analyzer_t *sem) {
    cct_sem_symbol_t *sym = (cct_sem_symbol_t*)sem_calloc(1, sizeof(*sym));

    cct_sem_symbol_alloc_t *node = (cct_sem_symbol_alloc_t*)sem_calloc(1, sizeof(*node));
    node->symbol = sym;
    node->next = sem->symbol_alloc_list;
    sem->symbol_alloc_list = node;

    return sym;
}

static cct_sem_type_t* sem_alloc_type(cct_semantic_analyzer_t *sem) {
    cct_sem_type_t *type = (cct_sem_type_t*)sem_calloc(1, sizeof(*type));

    cct_sem_type_alloc_t *node = (cct_sem_type_alloc_t*)sem_calloc(1, sizeof(*node));
    node->type = type;
    node->next = sem->type_alloc_list;
    sem->type_alloc_list = node;

    return type;
}

/* ========================================================================
 * Error Reporting
 * ======================================================================== */

static void sem_report_with_suggestion(
    cct_semantic_analyzer_t *sem,
    u32 line,
    u32 col,
    const char *message,
    const char *suggestion
) {
    sem->had_error = true;
    sem->error_count++;
    cct_error_at_location_with_suggestion(CCT_ERROR_SEMANTIC, sem->filename, line, col, message, suggestion);
}

static void sem_report(cct_semantic_analyzer_t *sem, u32 line, u32 col, const char *message) {
    sem_report_with_suggestion(sem, line, col, message, NULL);
}

static void sem_reportf(cct_semantic_analyzer_t *sem, u32 line, u32 col, const char *fmt, ...) {
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    sem_report(sem, line, col, buffer);
}

static void sem_report_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node, const char *message) {
    u32 line = node ? node->line : 0;
    u32 col = node ? node->column : 0;
    sem_report(sem, line, col, message);
}

static void sem_report_nodef(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node, const char *fmt, ...) {
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    sem_report_node(sem, node, buffer);
}

/* ========================================================================
 * Builtin Types
 * ======================================================================== */

static void sem_init_builtin_types(cct_semantic_analyzer_t *sem) {
    sem->type_error.kind = CCT_SEM_TYPE_ERROR;
    sem->type_error.name = "<error>";

    sem->type_nihil.kind = CCT_SEM_TYPE_NIHIL;
    sem->type_nihil.name = "NIHIL";

    sem->type_verum.kind = CCT_SEM_TYPE_VERUM;
    sem->type_verum.name = "VERUM";

    sem->type_verbum.kind = CCT_SEM_TYPE_VERBUM;
    sem->type_verbum.name = "VERBUM";

    sem->type_fractum.kind = CCT_SEM_TYPE_FRACTUM;
    sem->type_fractum.name = "FRACTUM";

    sem->type_rex.kind = CCT_SEM_TYPE_REX;
    sem->type_rex.name = "REX";

    sem->type_dux.kind = CCT_SEM_TYPE_DUX;
    sem->type_dux.name = "DUX";

    sem->type_comes.kind = CCT_SEM_TYPE_COMES;
    sem->type_comes.name = "COMES";

    sem->type_miles.kind = CCT_SEM_TYPE_MILES;
    sem->type_miles.name = "MILES";

    sem->type_umbra.kind = CCT_SEM_TYPE_UMBRA;
    sem->type_umbra.name = "UMBRA";

    sem->type_flamma.kind = CCT_SEM_TYPE_FLAMMA;
    sem->type_flamma.name = "FLAMMA";
}

static cct_sem_type_t* sem_builtin_type_by_name(cct_semantic_analyzer_t *sem, const char *name) {
    if (!name) return &sem->type_error;
    if (strcmp(name, "NIHIL") == 0) return &sem->type_nihil;
    if (strcmp(name, "VERUM") == 0) return &sem->type_verum;
    if (strcmp(name, "VERBUM") == 0) return &sem->type_verbum;
    if (strcmp(name, "FRACTUM") == 0) return &sem->type_fractum;
    if (strcmp(name, "REX") == 0) return &sem->type_rex;
    if (strcmp(name, "DUX") == 0) return &sem->type_dux;
    if (strcmp(name, "COMES") == 0) return &sem->type_comes;
    if (strcmp(name, "MILES") == 0) return &sem->type_miles;
    if (strcmp(name, "UMBRA") == 0) return &sem->type_umbra;
    if (strcmp(name, "FLAMMA") == 0) return &sem->type_flamma;
    return NULL;
}

/* ========================================================================
 * Scope / Symbol Table
 * ======================================================================== */

static void sem_push_scope(cct_semantic_analyzer_t *sem, cct_sem_scope_kind_t kind) {
    cct_sem_scope_t *scope = sem_alloc_scope(sem, kind);
    scope->parent = sem->current_scope;
    sem->current_scope = scope;
    if (!sem->global_scope) {
        sem->global_scope = scope;
    }
}

static void sem_pop_scope(cct_semantic_analyzer_t *sem) {
    if (sem->current_scope) {
        sem->current_scope = sem->current_scope->parent;
    }
}

static cct_sem_symbol_t* sem_lookup_in_scope(const cct_sem_scope_t *scope, const char *name) {
    for (const cct_sem_symbol_t *sym = scope ? scope->symbols : NULL; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return (cct_sem_symbol_t*)sym;
        }
    }
    return NULL;
}

static cct_sem_symbol_t* sem_lookup(cct_semantic_analyzer_t *sem, const char *name) {
    for (cct_sem_scope_t *scope = sem->current_scope; scope; scope = scope->parent) {
        cct_sem_symbol_t *sym = sem_lookup_in_scope(scope, name);
        if (sym) return sym;
    }
    return NULL;
}

static cct_sem_symbol_t* sem_lookup_global(cct_semantic_analyzer_t *sem, const char *name) {
    return sem_lookup_in_scope(sem->global_scope, name);
}

static const char* sem_stdlib_import_hint(const char *name) {
    if (!name) return NULL;

    if (strcmp(name, "len") == 0 ||
        strcmp(name, "concat") == 0 ||
        strcmp(name, "compare") == 0 ||
        strcmp(name, "substring") == 0 ||
        strcmp(name, "trim") == 0 ||
        strcmp(name, "find") == 0 ||
        strcmp(name, "contains") == 0) {
        return "add ADVOCARE \"cct/verbum.cct\" in the module header";
    }
    if (strcmp(name, "print") == 0 ||
        strcmp(name, "println") == 0 ||
        strcmp(name, "print_int") == 0 ||
        strcmp(name, "read_line") == 0) {
        return "add ADVOCARE \"cct/io.cct\" in the module header";
    }
    if (strcmp(name, "join") == 0 ||
        strcmp(name, "basename") == 0 ||
        strcmp(name, "dirname") == 0 ||
        strcmp(name, "ext") == 0) {
        return "add ADVOCARE \"cct/path.cct\" in the module header";
    }
    if (strcmp(name, "Some") == 0 ||
        strcmp(name, "None") == 0 ||
        strcmp(name, "option_is_some") == 0 ||
        strcmp(name, "option_is_none") == 0 ||
        strcmp(name, "option_unwrap") == 0 ||
        strcmp(name, "option_unwrap_or") == 0 ||
        strcmp(name, "option_expect") == 0 ||
        strcmp(name, "option_free") == 0) {
        return "add ADVOCARE \"cct/option.cct\" in the module header";
    }
    if (strcmp(name, "Ok") == 0 ||
        strcmp(name, "Err") == 0 ||
        strcmp(name, "result_is_ok") == 0 ||
        strcmp(name, "result_is_err") == 0 ||
        strcmp(name, "result_unwrap") == 0 ||
        strcmp(name, "result_unwrap_or") == 0 ||
        strcmp(name, "result_unwrap_err") == 0 ||
        strcmp(name, "result_expect") == 0 ||
        strcmp(name, "result_free") == 0) {
        return "add ADVOCARE \"cct/result.cct\" in the module header";
    }
    if (strcmp(name, "map_init") == 0 ||
        strcmp(name, "map_free") == 0 ||
        strcmp(name, "map_insert") == 0 ||
        strcmp(name, "map_remove") == 0 ||
        strcmp(name, "map_get") == 0 ||
        strcmp(name, "map_contains") == 0 ||
        strcmp(name, "map_len") == 0 ||
        strcmp(name, "map_is_empty") == 0 ||
        strcmp(name, "map_capacity") == 0 ||
        strcmp(name, "map_clear") == 0 ||
        strcmp(name, "map_reserve") == 0) {
        return "add ADVOCARE \"cct/map.cct\" in the module header";
    }
    if (strcmp(name, "set_init") == 0 ||
        strcmp(name, "set_free") == 0 ||
        strcmp(name, "set_insert") == 0 ||
        strcmp(name, "set_remove") == 0 ||
        strcmp(name, "set_contains") == 0 ||
        strcmp(name, "set_len") == 0 ||
        strcmp(name, "set_is_empty") == 0 ||
        strcmp(name, "set_clear") == 0) {
        return "add ADVOCARE \"cct/set.cct\" in the module header";
    }
    if (strcmp(name, "fluxus_map") == 0 ||
        strcmp(name, "fluxus_filter") == 0 ||
        strcmp(name, "fluxus_fold") == 0 ||
        strcmp(name, "fluxus_find") == 0 ||
        strcmp(name, "fluxus_any") == 0 ||
        strcmp(name, "fluxus_all") == 0 ||
        strcmp(name, "series_map") == 0 ||
        strcmp(name, "series_filter") == 0 ||
        strcmp(name, "series_reduce") == 0 ||
        strcmp(name, "series_find") == 0 ||
        strcmp(name, "series_any") == 0 ||
        strcmp(name, "series_all") == 0) {
        return "add ADVOCARE \"cct/collection_ops.cct\" in the module header";
    }

    return NULL;
}

static const char* sem_find_symbol_name_suggestion(
    cct_semantic_analyzer_t *sem,
    const char *target,
    cct_sem_symbol_kind_t expected_kind,
    bool filter_by_kind
) {
    if (!sem || !target) return NULL;

    const char *candidates[256];
    size_t count = 0;

    for (cct_sem_scope_t *scope = sem->current_scope; scope && count < 256; scope = scope->parent) {
        for (cct_sem_symbol_t *sym = scope->symbols; sym && count < 256; sym = sym->next) {
            if (filter_by_kind && sym->kind != expected_kind) continue;
            candidates[count++] = sym->name;
        }
    }

    return cct_fuzzy_match(target, candidates, count);
}

static cct_sem_symbol_t* sem_define_symbol(
    cct_semantic_analyzer_t *sem,
    cct_sem_symbol_kind_t kind,
    const char *name,
    u32 line,
    u32 col
) {
    if (!sem->current_scope) {
        sem_reportf(sem, line, col, "internal error: no current scope when defining '%s'", name);
        return NULL;
    }

    cct_sem_symbol_t *existing = sem_lookup_in_scope(sem->current_scope, name);
    if (existing) {
        sem_reportf(sem, line, col,
                    "duplicate symbol '%s' in same scope (first declared at %u:%u)",
                    name, existing->line, existing->column);
        return NULL;
    }

    cct_sem_symbol_t *sym = sem_alloc_symbol(sem);
    sym->name = sem_strdup(name);
    sym->kind = kind;
    sym->line = line;
    sym->column = col;

    sym->next = sem->current_scope->symbols;
    sem->current_scope->symbols = sym;
    return sym;
}

static cct_sem_symbol_t* sem_find_function(cct_semantic_analyzer_t *sem, const char *name) {
    cct_sem_symbol_t *sym = sem_lookup_global(sem, name);
    if (!sym || sym->kind != CCT_SEM_SYMBOL_RITUALE) return NULL;
    return sym;
}

static cct_sem_symbol_t* sem_find_type_symbol(cct_semantic_analyzer_t *sem, const char *name) {
    cct_sem_symbol_t *sym = sem_lookup_global(sem, name);
    if (!sym || sym->kind != CCT_SEM_SYMBOL_TYPE) return NULL;
    return sym;
}

static cct_sem_symbol_t* sem_find_pactum_symbol(cct_semantic_analyzer_t *sem, const char *name) {
    cct_sem_symbol_t *sym = sem_lookup_global(sem, name);
    if (!sym || sym->kind != CCT_SEM_SYMBOL_PACTUM) return NULL;
    return sym;
}

static const char* sem_rituale_type_param_name(const cct_ast_node_t *rituale_decl, size_t index) {
    if (!rituale_decl || rituale_decl->type != AST_RITUALE ||
        !rituale_decl->as.rituale.type_params ||
        index >= rituale_decl->as.rituale.type_params->count) {
        return NULL;
    }
    cct_ast_type_param_t *tp = rituale_decl->as.rituale.type_params->params[index];
    return (tp && tp->name) ? tp->name : NULL;
}

static void sem_validate_rituale_genus_constraints_10d(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *rituale_decl
) {
    if (!sem || !rituale_decl || rituale_decl->type != AST_RITUALE || !rituale_decl->as.rituale.type_params) {
        return;
    }

    for (size_t i = 0; i < rituale_decl->as.rituale.type_params->count; i++) {
        cct_ast_type_param_t *tp = rituale_decl->as.rituale.type_params->params[i];
        if (!tp || !tp->constraint_pactum_name || !tp->constraint_pactum_name[0]) continue;
        cct_sem_symbol_t *pact = sem_find_pactum_symbol(sem, tp->constraint_pactum_name);
        if (!pact || !pact->pactum_decl || pact->pactum_decl->type != AST_PACTUM) {
            sem_reportf(
                sem,
                tp->line,
                tp->column,
                "GENUS constraint '%s PACTUM %s' references unknown contract (subset 10D; subset final da FASE 10)",
                tp->name ? tp->name : "<T>",
                tp->constraint_pactum_name
            );
        }
    }
}

static bool sem_validate_genus_constraint_instantiation_10d(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *call_node,
    const cct_sem_symbol_t *fn,
    size_t param_index,
    const cct_ast_type_t *ast_arg,
    cct_sem_type_t *resolved_arg
) {
    if (!sem || !fn || !fn->type_param_constraint_pactum_names || param_index >= fn->type_param_count) {
        return true;
    }
    const char *required_pactum = fn->type_param_constraint_pactum_names[param_index];
    if (!required_pactum || !required_pactum[0]) return true;

    const char *tp_name = sem_rituale_type_param_name(fn->rituale_decl, param_index);
    const char *shown_tp = tp_name ? tp_name : "T";
    const char *shown_type = (resolved_arg && resolved_arg->name) ? resolved_arg->name : cct_sem_type_string(resolved_arg);

    if (!ast_arg || !resolved_arg ||
        resolved_arg->kind != CCT_SEM_TYPE_NAMED ||
        ast_arg->is_pointer || ast_arg->is_array ||
        !ast_arg->name || (ast_arg->generic_args && ast_arg->generic_args->count > 0)) {
        sem_report_nodef(
            sem,
            call_node,
            "type argument '%s' does not satisfy constraint '%s PACTUM %s': subset 10D accepts only named SIGILLUM type arguments with explicit conformance (subset final da FASE 10)",
            shown_type ? shown_type : "<unknown>",
            shown_tp,
            required_pactum
        );
        return false;
    }

    cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, ast_arg->name);
    if (!type_sym || !type_sym->type_decl || type_sym->type_decl->type != AST_SIGILLUM) {
        sem_report_nodef(
            sem,
            call_node,
            "type argument '%s' does not satisfy constraint '%s PACTUM %s': expected named SIGILLUM in subset 10D (subset final da FASE 10)",
            shown_type ? shown_type : "<unknown>",
            shown_tp,
            required_pactum
        );
        return false;
    }

    const char *provided_pactum = type_sym->type_decl->as.sigillum.pactum_name;
    if (!provided_pactum || strcmp(provided_pactum, required_pactum) != 0) {
        sem_report_nodef(
            sem,
            call_node,
            "type argument '%s' does not satisfy constraint '%s PACTUM %s': SIGILLUM lacks explicit matching conformance (subset 10D; subset final da FASE 10)",
            ast_arg->name,
            shown_tp,
            required_pactum
        );
        return false;
    }

    return true;
}

/* ========================================================================
 * Semantic Type Helpers
 * ======================================================================== */

static bool sem_is_error_type(const cct_sem_type_t *type) {
    return type && type->kind == CCT_SEM_TYPE_ERROR;
}

static bool sem_is_integer_type(const cct_sem_type_t *type) {
    if (!type) return false;
    switch (type->kind) {
        case CCT_SEM_TYPE_REX:
        case CCT_SEM_TYPE_DUX:
        case CCT_SEM_TYPE_COMES:
        case CCT_SEM_TYPE_MILES:
            return true;
        default:
            return false;
    }
}

static bool sem_is_real_type(const cct_sem_type_t *type) {
    if (!type) return false;
    return type->kind == CCT_SEM_TYPE_UMBRA || type->kind == CCT_SEM_TYPE_FLAMMA;
}

static bool sem_is_numeric_type(const cct_sem_type_t *type) {
    return sem_is_integer_type(type) || sem_is_real_type(type);
}

static bool sem_is_bool_type(const cct_sem_type_t *type) {
    return type && type->kind == CCT_SEM_TYPE_VERUM;
}

static bool sem_is_type_param_type(const cct_sem_type_t *type) {
    return type && type->kind == CCT_SEM_TYPE_TYPE_PARAM;
}

static bool sem_type_equal(const cct_sem_type_t *a, const cct_sem_type_t *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case CCT_SEM_TYPE_POINTER:
            return sem_type_equal(a->element, b->element);
        case CCT_SEM_TYPE_ARRAY:
            return a->array_size == b->array_size && sem_type_equal(a->element, b->element);
        case CCT_SEM_TYPE_NAMED:
        case CCT_SEM_TYPE_TYPE_PARAM:
            return a->name && b->name && strcmp(a->name, b->name) == 0;
        default:
            return true;
    }
}

static cct_sem_type_t* sem_make_pointer_type(cct_semantic_analyzer_t *sem, cct_sem_type_t *element) {
    cct_sem_type_t *type = sem_alloc_type(sem);
    type->kind = CCT_SEM_TYPE_POINTER;
    type->name = "SPECULUM";
    type->element = element;
    return type;
}

static cct_sem_type_t* sem_make_array_type(cct_semantic_analyzer_t *sem, cct_sem_type_t *element, u32 size) {
    cct_sem_type_t *type = sem_alloc_type(sem);
    type->kind = CCT_SEM_TYPE_ARRAY;
    type->name = "SERIES";
    type->element = element;
    type->array_size = size;
    return type;
}

static cct_sem_type_t* sem_make_named_type(cct_semantic_analyzer_t *sem, const char *name) {
    cct_sem_type_t *type = sem_alloc_type(sem);
    type->kind = CCT_SEM_TYPE_NAMED;
    type->name = sem_strdup(name);
    return type;
}

static cct_sem_type_t* sem_make_type_param_type(cct_semantic_analyzer_t *sem, const char *name) {
    cct_sem_type_t *type = sem_alloc_type(sem);
    type->kind = CCT_SEM_TYPE_TYPE_PARAM;
    type->name = sem_strdup(name);
    return type;
}

static size_t sem_decl_type_param_arity(const cct_ast_node_t *decl) {
    if (!decl) return 0;
    if (decl->type == AST_SIGILLUM && decl->as.sigillum.type_params) {
        return decl->as.sigillum.type_params->count;
    }
    if (decl->type == AST_RITUALE && decl->as.rituale.type_params) {
        return decl->as.rituale.type_params->count;
    }
    return 0;
}

static bool sem_is_materializable_type_arg_10b(
    cct_semantic_analyzer_t *sem,
    const cct_ast_type_t *ast_type,
    cct_sem_type_t *resolved,
    u32 line,
    u32 col
) {
    if (!ast_type || !resolved) return false;

    if (ast_type->is_pointer || ast_type->is_array) {
        sem_reportf(sem, line, col,
                    "type argument '%s' is outside executable subset 10B (pointer/array type args are not materializable)",
                    cct_sem_type_string(resolved));
        return false;
    }
    if (ast_type->generic_args && ast_type->generic_args->count > 0) {
        sem_reportf(sem, line, col,
                    "type argument '%s' is outside executable subset 10B (nested generic application is not materializable yet)",
                    cct_sem_type_string(resolved));
        return false;
    }
    if (resolved->kind == CCT_SEM_TYPE_TYPE_PARAM) {
        sem_reportf(sem, line, col,
                    "type argument '%s' is still parametric and outside subset 10B materialization",
                    cct_sem_type_string(resolved));
        return false;
    }

    switch (resolved->kind) {
        case CCT_SEM_TYPE_REX:
        case CCT_SEM_TYPE_DUX:
        case CCT_SEM_TYPE_COMES:
        case CCT_SEM_TYPE_MILES:
        case CCT_SEM_TYPE_VERUM:
        case CCT_SEM_TYPE_VERBUM:
        case CCT_SEM_TYPE_UMBRA:
        case CCT_SEM_TYPE_FLAMMA:
            return true;
        case CCT_SEM_TYPE_NAMED: {
            cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, ast_type->name);
            if (!type_sym || !type_sym->type_decl) {
                sem_reportf(sem, line, col,
                            "type argument '%s' is not a resolvable executable named type in subset 10B",
                            cct_sem_type_string(resolved));
                return false;
            }
            if (type_sym->type_decl->type != AST_SIGILLUM && type_sym->type_decl->type != AST_ORDO) {
                sem_reportf(sem, line, col,
                            "type argument '%s' is outside executable subset 10B (named executable args must be SIGILLUM/ORDO)",
                            cct_sem_type_string(resolved));
                return false;
            }
            if (sem_decl_type_param_arity(type_sym->type_decl) > 0) {
                sem_reportf(sem, line, col,
                            "type argument '%s' is generic and still not materializable in subset 10B",
                            cct_sem_type_string(resolved));
                return false;
            }
            return true;
        }
        default:
            sem_reportf(sem, line, col,
                        "type argument '%s' is outside executable subset 10B",
                        cct_sem_type_string(resolved));
            return false;
    }
}

static cct_sem_type_t* sem_substitute_type_params(
    cct_semantic_analyzer_t *sem,
    cct_sem_type_t *type,
    char **param_names,
    cct_sem_type_t **param_types,
    size_t param_count
) {
    if (!type) return &sem->type_error;
    if (!param_names || !param_types || param_count == 0) return type;

    if (type->kind == CCT_SEM_TYPE_TYPE_PARAM && type->name) {
        for (size_t i = 0; i < param_count; i++) {
            if (param_names[i] && strcmp(param_names[i], type->name) == 0) {
                return param_types[i] ? param_types[i] : &sem->type_error;
            }
        }
        return type;
    }
    if (type->kind == CCT_SEM_TYPE_POINTER) {
        return sem_make_pointer_type(
            sem,
            sem_substitute_type_params(sem, type->element, param_names, param_types, param_count)
        );
    }
    if (type->kind == CCT_SEM_TYPE_ARRAY) {
        return sem_make_array_type(
            sem,
            sem_substitute_type_params(sem, type->element, param_names, param_types, param_count),
            type->array_size
        );
    }
    return type;
}

static cct_sem_generic_instance_t* sem_find_generic_instance(
    cct_semantic_analyzer_t *sem,
    const char *name
) {
    if (!sem || !name) return NULL;
    for (cct_sem_generic_instance_t *it = sem->generic_instances; it; it = it->next) {
        if (it->name && strcmp(it->name, name) == 0) return it;
    }
    return NULL;
}

static cct_sem_generic_instance_t* sem_register_generic_instance(
    cct_semantic_analyzer_t *sem,
    const char *name,
    const cct_ast_node_t *decl,
    cct_sem_type_t **type_args,
    size_t type_arg_count
) {
    if (!sem || !name) return NULL;
    cct_sem_generic_instance_t *existing = sem_find_generic_instance(sem, name);
    if (existing) return existing;

    cct_sem_generic_instance_t *inst = (cct_sem_generic_instance_t*)sem_calloc(1, sizeof(*inst));
    inst->name = sem_strdup(name);
    inst->decl = decl;
    inst->type_arg_count = type_arg_count;
    if (type_arg_count > 0) {
        inst->type_args = (cct_sem_type_t**)sem_calloc(type_arg_count, sizeof(*inst->type_args));
        for (size_t i = 0; i < type_arg_count; i++) {
            inst->type_args[i] = type_args[i];
        }
    }
    inst->next = sem->generic_instances;
    sem->generic_instances = inst;
    return inst;
}

static cct_sem_type_t* sem_resolve_ast_type_with_bindings(
    cct_semantic_analyzer_t *sem,
    const cct_ast_type_t *ast_type,
    char **param_names,
    cct_sem_type_t **param_types,
    size_t param_count,
    u32 line,
    u32 col
) {
    if (!ast_type) return &sem->type_error;

    if (ast_type->is_pointer) {
        cct_sem_type_t *elem = sem_resolve_ast_type_with_bindings(
            sem, ast_type->element_type, param_names, param_types, param_count, line, col
        );
        return sem_make_pointer_type(sem, elem);
    }

    if (ast_type->is_array) {
        cct_sem_type_t *elem = sem_resolve_ast_type_with_bindings(
            sem, ast_type->element_type, param_names, param_types, param_count, line, col
        );
        return sem_make_array_type(sem, elem, ast_type->array_size);
    }

    if (ast_type->name && param_names && param_types) {
        for (size_t i = 0; i < param_count; i++) {
            if (param_names[i] && strcmp(param_names[i], ast_type->name) == 0) {
                return param_types[i] ? param_types[i] : &sem->type_error;
            }
        }
    }

    return sem_resolve_ast_type(sem, ast_type, line, col);
}

static cct_sem_type_t* sem_arithmetic_result_type(cct_semantic_analyzer_t *sem, cct_sem_type_t *a, cct_sem_type_t *b) {
    if (sem_is_error_type(a) || sem_is_error_type(b)) return &sem->type_error;
    if (!sem_is_numeric_type(a) || !sem_is_numeric_type(b)) return &sem->type_error;
    if (sem_is_real_type(a) || sem_is_real_type(b)) return &sem->type_umbra;
    return &sem->type_rex;
}

static bool sem_types_compatible_assign(cct_semantic_analyzer_t *sem, cct_sem_type_t *target, cct_sem_type_t *value) {
    if (!target || !value) return false;
    if (sem_is_error_type(target) || sem_is_error_type(value)) return true;
    if (sem_type_equal(target, value)) return true;

    if (sem_is_type_param_type(target) || sem_is_type_param_type(value)) {
        return false;
    }

    if (target->kind == CCT_SEM_TYPE_POINTER && value->kind == CCT_SEM_TYPE_POINTER) {
        /* FASE 7A pragmatic rule: SPECULUM NIHIL behaves as generic alloc/free pointer. */
        if (target->element && target->element->kind == CCT_SEM_TYPE_NIHIL) return true;
        if (value->element && value->element->kind == CCT_SEM_TYPE_NIHIL) return true;
        return false;
    }

    if (sem_is_real_type(target) && sem_is_integer_type(value)) {
        return true; /* simple numeric promotion */
    }

    if (sem_is_real_type(target) && sem_is_real_type(value)) {
        return true; /* FASE 6B pragmatic real<->real assignment */
    }

    (void)sem;
    return false;
}

static bool sem_sigillum_ast_contains_speculum_field_recursive(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *sig_node,
    const char *root_name,
    u32 depth
) {
    if (!sig_node || sig_node->type != AST_SIGILLUM || !sig_node->as.sigillum.fields) return false;
    if (depth > 32) {
        sem_reportf(sem, sig_node->line, sig_node->column,
                    "SIGILLUM copy analysis exceeded recursion limit (subset final da FASE 7 treats this composition as restricted)");
        return true;
    }

    cct_ast_field_list_t *fields = sig_node->as.sigillum.fields;
    for (size_t i = 0; i < fields->count; i++) {
        cct_ast_field_t *f = fields->fields[i];
        if (!f || !f->type) continue;

        if (f->type->is_pointer) {
            return true;
        }

        if (f->type->is_array) {
            cct_ast_type_t *elem = f->type->element_type;
            if (elem && elem->is_pointer) return true;
            if (elem && !elem->is_pointer && !elem->is_array && elem->name) {
                cct_sem_symbol_t *nested_sym = sem_find_type_symbol(sem, elem->name);
                if (nested_sym && nested_sym->type_decl && nested_sym->type_decl->type == AST_SIGILLUM) {
                    if (root_name && strcmp(elem->name, root_name) == 0) {
                        return true; /* recursive composition remains restricted */
                    }
                    if (sem_sigillum_ast_contains_speculum_field_recursive(
                            sem, nested_sym->type_decl, root_name ? root_name : elem->name, depth + 1)) {
                        return true;
                    }
                }
            }
            continue;
        }

        if (f->type->name) {
            cct_sem_symbol_t *nested_sym = sem_find_type_symbol(sem, f->type->name);
            if (nested_sym && nested_sym->type_decl && nested_sym->type_decl->type == AST_SIGILLUM) {
                if (root_name && strcmp(f->type->name, root_name) == 0) {
                    return true; /* recursive by-value composition remains restricted */
                }
                if (sem_sigillum_ast_contains_speculum_field_recursive(
                        sem, nested_sym->type_decl, root_name ? root_name : f->type->name, depth + 1)) {
                    return true;
                }
            }
        }
    }

    return false;
}

static bool sem_sigillum_named_copy_is_restricted(cct_semantic_analyzer_t *sem, const cct_sem_type_t *named_type) {
    if (!named_type || named_type->kind != CCT_SEM_TYPE_NAMED || !named_type->name) return false;
    cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, named_type->name);
    if (!type_sym || !type_sym->type_decl || type_sym->type_decl->type != AST_SIGILLUM) return false;
    return sem_sigillum_ast_contains_speculum_field_recursive(sem, type_sym->type_decl, named_type->name, 0);
}

static bool sem_sigillum_fields_have_duplicates(cct_semantic_analyzer_t *sem, const cct_ast_node_t *sig_node) {
    if (!sig_node || sig_node->type != AST_SIGILLUM || !sig_node->as.sigillum.fields) return false;
    cct_ast_field_list_t *fields = sig_node->as.sigillum.fields;
    bool dup = false;
    for (size_t i = 0; i < fields->count; i++) {
        cct_ast_field_t *a = fields->fields[i];
        if (!a || !a->name) continue;
        for (size_t j = i + 1; j < fields->count; j++) {
            cct_ast_field_t *b = fields->fields[j];
            if (!b || !b->name) continue;
            if (strcmp(a->name, b->name) == 0) {
                sem_reportf(sem, b->line, b->column, "duplicate SIGILLUM field '%s'", b->name);
                dup = true;
            }
        }
    }
    return dup;
}

static void sem_validate_sigillum_field_subset(cct_semantic_analyzer_t *sem, const cct_ast_node_t *sig_node) {
    if (!sig_node || sig_node->type != AST_SIGILLUM || !sig_node->as.sigillum.fields) return;
    cct_ast_field_list_t *fields = sig_node->as.sigillum.fields;
    for (size_t i = 0; i < fields->count; i++) {
        cct_ast_field_t *f = fields->fields[i];
        if (!f || !f->type) continue;
        if (f->type->is_array && f->type->element_type && f->type->element_type->is_array) {
            sem_reportf(sem, f->line, f->column, "nested SERIES field '%s' is outside current executable subset", f->name);
        }
        /* Block direct by-value self recursion in FASE 7B subset. */
        if (!f->type->is_pointer && !f->type->is_array && f->type->name &&
            sig_node->as.sigillum.name && strcmp(f->type->name, sig_node->as.sigillum.name) == 0) {
            sem_reportf(sem, f->line, f->column,
                        "SIGILLUM '%s' field '%s' creates direct by-value recursive composition (outside FASE 7B subset)",
                        sig_node->as.sigillum.name, f->name);
        }
    }
}

/* ========================================================================
 * AST Type Resolution
 * ======================================================================== */

static cct_sem_type_t* sem_resolve_ast_type(cct_semantic_analyzer_t *sem, const cct_ast_type_t *ast_type, u32 line, u32 col) {
    if (!ast_type) {
        sem_reportf(sem, line, col, "missing type information");
        return &sem->type_error;
    }

    if (ast_type->is_pointer) {
        cct_sem_type_t *elem = sem_resolve_ast_type(sem, ast_type->element_type, line, col);
        return sem_make_pointer_type(sem, elem);
    }

    if (ast_type->is_array) {
        cct_sem_type_t *elem = sem_resolve_ast_type(sem, ast_type->element_type, line, col);
        return sem_make_array_type(sem, elem, ast_type->array_size);
    }

    bool has_generic_args = ast_type->generic_args && ast_type->generic_args->count > 0;

    cct_sem_type_t *builtin = sem_builtin_type_by_name(sem, ast_type->name);
    if (builtin) {
        if (has_generic_args) {
            sem_reportf(sem, line, col,
                        "GENUS(...) cannot be applied to non-generic builtin type '%s' (subset 10B)",
                        ast_type->name ? ast_type->name : "<builtin>");
        }
        return builtin;
    }

    if (ast_type->name) {
        if (sem_active_type_params_contains(sem, ast_type->name)) {
            if (has_generic_args) {
                sem_reportf(sem, line, col,
                            "GENUS(...) cannot be applied to generic type parameter '%s' (subset 10B)",
                            ast_type->name);
            }
            return sem_make_type_param_type(sem, ast_type->name);
        }

        cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, ast_type->name);
        size_t decl_arity = sem_decl_type_param_arity(type_sym ? type_sym->type_decl : NULL);

        if (has_generic_args) {
            if (!type_sym || !type_sym->type_decl || decl_arity == 0) {
                sem_reportf(sem, line, col,
                            "GENUS(...) applied to non-generic type '%s' (subset 10B)",
                            ast_type->name);
                return sem_make_named_type(sem, ast_type->name);
            }

            size_t inst_arity = ast_type->generic_args->count;
            if (inst_arity != decl_arity) {
                sem_reportf(sem, line, col,
                            "generic type '%s' expects %zu type argument(s), got %zu (subset 10B)",
                            ast_type->name, decl_arity, inst_arity);
            }

            size_t arg_count = inst_arity;
            cct_sem_type_t **resolved_args = NULL;
            if (arg_count > 0) {
                resolved_args = (cct_sem_type_t**)sem_calloc(arg_count, sizeof(*resolved_args));
            }

            for (size_t i = 0; i < arg_count; i++) {
                const cct_ast_type_t *arg_ast = ast_type->generic_args->types[i];
                cct_sem_type_t *arg_sem = sem_resolve_ast_type(
                    sem,
                    arg_ast,
                    line,
                    col
                );
                resolved_args[i] = arg_sem;
                if (arg_ast) {
                    (void)sem_is_materializable_type_arg_10b(sem, arg_ast, arg_sem, line, col);
                }
            }

            size_t needed = strlen(ast_type->name) + 3; /* "<>" + NUL */
            for (size_t i = 0; i < arg_count; i++) {
                needed += strlen(cct_sem_type_string(resolved_args[i])) + 2;
            }
            char *inst_name = (char*)sem_calloc(needed, 1);
            strcat(inst_name, ast_type->name);
            strcat(inst_name, "<");
            for (size_t i = 0; i < arg_count; i++) {
                if (i > 0) strcat(inst_name, ",");
                strcat(inst_name, cct_sem_type_string(resolved_args[i]));
            }
            strcat(inst_name, ">");

            (void)sem_register_generic_instance(sem, inst_name, type_sym ? type_sym->type_decl : NULL,
                                                resolved_args, arg_count);
            cct_sem_type_t *inst = sem_make_named_type(sem, inst_name);
            free(inst_name);
            free(resolved_args);
            return inst;
        }

        if (type_sym && decl_arity > 0) {
            sem_reportf(sem, line, col,
                        "generic type '%s' requires explicit GENUS(...) instantiation in subset 10B",
                        ast_type->name);
            return sem_make_named_type(sem, ast_type->name);
        }

        if (type_sym && type_sym->type) {
            return type_sym->type;
        }

        sem_reportf(sem, line, col, "unknown type '%s'", ast_type->name);
        return sem_make_named_type(sem, ast_type->name);
    }

    sem_reportf(sem, line, col, "invalid type syntax");
    return &sem->type_error;
}

/* ========================================================================
 * Builtins
 * ======================================================================== */

static const cct_sem_builtin_spec_t* sem_find_builtin(cct_semantic_analyzer_t *sem, const char *name) {
    static cct_sem_builtin_spec_t specs[112];
    static bool initialized = false;

    if (!initialized) {
        /* return_type pointers are assigned lazily per analyzer callsite */
        specs[0].name = "scribe"; specs[0].min_args = 1; specs[0].variadic = true;
        specs[1].name = "lege";   specs[1].min_args = 0; specs[1].variadic = true;
        specs[2].name = "pete";   specs[2].min_args = 1; specs[2].variadic = false;
        specs[3].name = "libera"; specs[3].min_args = 1; specs[3].variadic = false;
        specs[4].name = "aperi";  specs[4].min_args = 1; specs[4].variadic = true;
        specs[5].name = "claude"; specs[5].min_args = 1; specs[5].variadic = false;
        specs[6].name = "verbum_len"; specs[6].min_args = 1; specs[6].variadic = false;
        specs[7].name = "verbum_concat"; specs[7].min_args = 2; specs[7].variadic = false;
        specs[8].name = "verbum_compare"; specs[8].min_args = 2; specs[8].variadic = false;
        specs[9].name = "verbum_substring"; specs[9].min_args = 3; specs[9].variadic = false;
        specs[10].name = "verbum_trim"; specs[10].min_args = 1; specs[10].variadic = false;
        specs[11].name = "verbum_find"; specs[11].min_args = 2; specs[11].variadic = false;
        specs[12].name = "fmt_stringify_int"; specs[12].min_args = 1; specs[12].variadic = false;
        specs[13].name = "fmt_stringify_real"; specs[13].min_args = 1; specs[13].variadic = false;
        specs[14].name = "fmt_stringify_float"; specs[14].min_args = 1; specs[14].variadic = false;
        specs[15].name = "fmt_parse_int"; specs[15].min_args = 1; specs[15].variadic = false;
        specs[16].name = "fmt_parse_real"; specs[16].min_args = 1; specs[16].variadic = false;
        specs[17].name = "mem_alloc"; specs[17].min_args = 1; specs[17].variadic = false;
        specs[18].name = "mem_free"; specs[18].min_args = 1; specs[18].variadic = false;
        specs[19].name = "mem_realloc"; specs[19].min_args = 2; specs[19].variadic = false;
        specs[20].name = "mem_copy"; specs[20].min_args = 3; specs[20].variadic = false;
        specs[21].name = "mem_set"; specs[21].min_args = 3; specs[21].variadic = false;
        specs[22].name = "mem_zero"; specs[22].min_args = 2; specs[22].variadic = false;
        specs[23].name = "mem_compare"; specs[23].min_args = 3; specs[23].variadic = false;
        specs[24].name = "fluxus_init"; specs[24].min_args = 1; specs[24].variadic = false;
        specs[25].name = "fluxus_free"; specs[25].min_args = 1; specs[25].variadic = false;
        specs[26].name = "fluxus_push"; specs[26].min_args = 2; specs[26].variadic = false;
        specs[27].name = "fluxus_pop"; specs[27].min_args = 2; specs[27].variadic = false;
        specs[28].name = "fluxus_len"; specs[28].min_args = 1; specs[28].variadic = false;
        specs[29].name = "fluxus_get"; specs[29].min_args = 2; specs[29].variadic = false;
        specs[30].name = "fluxus_clear"; specs[30].min_args = 1; specs[30].variadic = false;
        specs[31].name = "fluxus_reserve"; specs[31].min_args = 2; specs[31].variadic = false;
        specs[32].name = "fluxus_capacity"; specs[32].min_args = 1; specs[32].variadic = false;
        specs[33].name = "io_print"; specs[33].min_args = 1; specs[33].variadic = false;
        specs[34].name = "io_println"; specs[34].min_args = 1; specs[34].variadic = false;
        specs[35].name = "io_print_int"; specs[35].min_args = 1; specs[35].variadic = false;
        specs[36].name = "io_read_line"; specs[36].min_args = 0; specs[36].variadic = false;
        specs[37].name = "fs_read_all"; specs[37].min_args = 1; specs[37].variadic = false;
        specs[38].name = "fs_write_all"; specs[38].min_args = 2; specs[38].variadic = false;
        specs[39].name = "fs_append_all"; specs[39].min_args = 2; specs[39].variadic = false;
        specs[40].name = "fs_exists"; specs[40].min_args = 1; specs[40].variadic = false;
        specs[41].name = "fs_size"; specs[41].min_args = 1; specs[41].variadic = false;
        specs[42].name = "path_join"; specs[42].min_args = 2; specs[42].variadic = false;
        specs[43].name = "path_basename"; specs[43].min_args = 1; specs[43].variadic = false;
        specs[44].name = "path_dirname"; specs[44].min_args = 1; specs[44].variadic = false;
        specs[45].name = "path_ext"; specs[45].min_args = 1; specs[45].variadic = false;
        specs[46].name = "random_seed"; specs[46].min_args = 1; specs[46].variadic = false;
        specs[47].name = "random_int"; specs[47].min_args = 2; specs[47].variadic = false;
        specs[48].name = "random_real"; specs[48].min_args = 0; specs[48].variadic = false;
        specs[49].name = "option_some"; specs[49].min_args = 2; specs[49].variadic = false;
        specs[50].name = "option_none"; specs[50].min_args = 1; specs[50].variadic = false;
        specs[51].name = "option_is_some"; specs[51].min_args = 1; specs[51].variadic = false;
        specs[52].name = "option_is_none"; specs[52].min_args = 1; specs[52].variadic = false;
        specs[53].name = "option_unwrap_ptr"; specs[53].min_args = 1; specs[53].variadic = false;
        specs[54].name = "option_expect_ptr"; specs[54].min_args = 2; specs[54].variadic = false;
        specs[55].name = "option_free"; specs[55].min_args = 1; specs[55].variadic = false;
        specs[56].name = "result_ok"; specs[56].min_args = 3; specs[56].variadic = false;
        specs[57].name = "result_err"; specs[57].min_args = 3; specs[57].variadic = false;
        specs[58].name = "result_is_ok"; specs[58].min_args = 1; specs[58].variadic = false;
        specs[59].name = "result_is_err"; specs[59].min_args = 1; specs[59].variadic = false;
        specs[60].name = "result_unwrap_ptr"; specs[60].min_args = 1; specs[60].variadic = false;
        specs[61].name = "result_unwrap_err_ptr"; specs[61].min_args = 1; specs[61].variadic = false;
        specs[62].name = "result_expect_ptr"; specs[62].min_args = 2; specs[62].variadic = false;
        specs[63].name = "result_free"; specs[63].min_args = 1; specs[63].variadic = false;
        specs[64].name = "map_init"; specs[64].min_args = 2; specs[64].variadic = false;
        specs[65].name = "map_free"; specs[65].min_args = 1; specs[65].variadic = false;
        specs[66].name = "map_insert"; specs[66].min_args = 3; specs[66].variadic = false;
        specs[67].name = "map_remove"; specs[67].min_args = 2; specs[67].variadic = false;
        specs[68].name = "map_get_ptr"; specs[68].min_args = 2; specs[68].variadic = false;
        specs[69].name = "map_contains"; specs[69].min_args = 2; specs[69].variadic = false;
        specs[70].name = "map_len"; specs[70].min_args = 1; specs[70].variadic = false;
        specs[71].name = "map_is_empty"; specs[71].min_args = 1; specs[71].variadic = false;
        specs[72].name = "map_capacity"; specs[72].min_args = 1; specs[72].variadic = false;
        specs[73].name = "map_clear"; specs[73].min_args = 1; specs[73].variadic = false;
        specs[74].name = "map_reserve"; specs[74].min_args = 2; specs[74].variadic = false;
        specs[75].name = "set_init"; specs[75].min_args = 1; specs[75].variadic = false;
        specs[76].name = "set_free"; specs[76].min_args = 1; specs[76].variadic = false;
        specs[77].name = "set_insert"; specs[77].min_args = 2; specs[77].variadic = false;
        specs[78].name = "set_remove"; specs[78].min_args = 2; specs[78].variadic = false;
        specs[79].name = "set_contains"; specs[79].min_args = 2; specs[79].variadic = false;
        specs[80].name = "set_len"; specs[80].min_args = 1; specs[80].variadic = false;
        specs[81].name = "set_is_empty"; specs[81].min_args = 1; specs[81].variadic = false;
        specs[82].name = "set_clear"; specs[82].min_args = 1; specs[82].variadic = false;
        specs[83].name = "collection_fluxus_map"; specs[83].min_args = 4; specs[83].variadic = false;
        specs[84].name = "collection_fluxus_filter"; specs[84].min_args = 3; specs[84].variadic = false;
        specs[85].name = "collection_fluxus_fold"; specs[85].min_args = 5; specs[85].variadic = false;
        specs[86].name = "collection_fluxus_find"; specs[86].min_args = 3; specs[86].variadic = false;
        specs[87].name = "collection_fluxus_any"; specs[87].min_args = 3; specs[87].variadic = false;
        specs[88].name = "collection_fluxus_all"; specs[88].min_args = 3; specs[88].variadic = false;
        specs[89].name = "collection_series_map"; specs[89].min_args = 5; specs[89].variadic = false;
        specs[90].name = "collection_series_filter"; specs[90].min_args = 4; specs[90].variadic = false;
        specs[91].name = "collection_series_reduce"; specs[91].min_args = 6; specs[91].variadic = false;
        specs[92].name = "collection_series_find"; specs[92].min_args = 4; specs[92].variadic = false;
        specs[93].name = "collection_series_any"; specs[93].min_args = 4; specs[93].variadic = false;
        specs[94].name = "collection_series_all"; specs[94].min_args = 4; specs[94].variadic = false;
        specs[95].name = "math_sqrt"; specs[95].min_args = 1; specs[95].variadic = false;
        specs[96].name = "math_cbrt"; specs[96].min_args = 1; specs[96].variadic = false;
        specs[97].name = "math_pow"; specs[97].min_args = 2; specs[97].variadic = false;
        specs[98].name = "math_hypot"; specs[98].min_args = 2; specs[98].variadic = false;
        specs[99].name = "math_sin"; specs[99].min_args = 1; specs[99].variadic = false;
        specs[100].name = "math_cos"; specs[100].min_args = 1; specs[100].variadic = false;
        specs[101].name = "math_tan"; specs[101].min_args = 1; specs[101].variadic = false;
        specs[102].name = "math_asin"; specs[102].min_args = 1; specs[102].variadic = false;
        specs[103].name = "math_acos"; specs[103].min_args = 1; specs[103].variadic = false;
        specs[104].name = "math_atan"; specs[104].min_args = 1; specs[104].variadic = false;
        specs[105].name = "math_atan2"; specs[105].min_args = 2; specs[105].variadic = false;
        specs[106].name = "math_deg_to_rad"; specs[106].min_args = 1; specs[106].variadic = false;
        specs[107].name = "math_rad_to_deg"; specs[107].min_args = 1; specs[107].variadic = false;
        specs[108].name = "math_exp"; specs[108].min_args = 1; specs[108].variadic = false;
        specs[109].name = "math_log"; specs[109].min_args = 1; specs[109].variadic = false;
        specs[110].name = "math_log10"; specs[110].min_args = 1; specs[110].variadic = false;
        specs[111].name = "math_log2"; specs[111].min_args = 1; specs[111].variadic = false;
        initialized = true;
    }

    specs[0].return_type = &sem->type_nihil;
    specs[1].return_type = &sem->type_rex;
    specs[2].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[3].return_type = &sem->type_nihil;
    specs[4].return_type = &sem->type_rex;
    specs[5].return_type = &sem->type_nihil;
    specs[6].return_type = &sem->type_rex;
    specs[7].return_type = &sem->type_verbum;
    specs[8].return_type = &sem->type_rex;
    specs[9].return_type = &sem->type_verbum;
    specs[10].return_type = &sem->type_verbum;
    specs[11].return_type = &sem->type_rex;
    specs[12].return_type = &sem->type_verbum;
    specs[13].return_type = &sem->type_verbum;
    specs[14].return_type = &sem->type_verbum;
    specs[15].return_type = &sem->type_rex;
    specs[16].return_type = &sem->type_umbra;
    specs[17].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[18].return_type = &sem->type_nihil;
    specs[19].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[20].return_type = &sem->type_nihil;
    specs[21].return_type = &sem->type_nihil;
    specs[22].return_type = &sem->type_nihil;
    specs[23].return_type = &sem->type_rex;
    specs[24].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[25].return_type = &sem->type_nihil;
    specs[26].return_type = &sem->type_nihil;
    specs[27].return_type = &sem->type_nihil;
    specs[28].return_type = &sem->type_rex;
    specs[29].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[30].return_type = &sem->type_nihil;
    specs[31].return_type = &sem->type_nihil;
    specs[32].return_type = &sem->type_rex;
    specs[33].return_type = &sem->type_nihil;
    specs[34].return_type = &sem->type_nihil;
    specs[35].return_type = &sem->type_nihil;
    specs[36].return_type = &sem->type_verbum;
    specs[37].return_type = &sem->type_verbum;
    specs[38].return_type = &sem->type_nihil;
    specs[39].return_type = &sem->type_nihil;
    specs[40].return_type = &sem->type_verum;
    specs[41].return_type = &sem->type_rex;
    specs[42].return_type = &sem->type_verbum;
    specs[43].return_type = &sem->type_verbum;
    specs[44].return_type = &sem->type_verbum;
    specs[45].return_type = &sem->type_verbum;
    specs[46].return_type = &sem->type_nihil;
    specs[47].return_type = &sem->type_rex;
    specs[48].return_type = &sem->type_flamma;
    specs[49].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[50].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[51].return_type = &sem->type_verum;
    specs[52].return_type = &sem->type_verum;
    specs[53].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[54].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[55].return_type = &sem->type_nihil;
    specs[56].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[57].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[58].return_type = &sem->type_verum;
    specs[59].return_type = &sem->type_verum;
    specs[60].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[61].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[62].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[63].return_type = &sem->type_nihil;
    specs[64].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[65].return_type = &sem->type_nihil;
    specs[66].return_type = &sem->type_nihil;
    specs[67].return_type = &sem->type_verum;
    specs[68].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[69].return_type = &sem->type_verum;
    specs[70].return_type = &sem->type_rex;
    specs[71].return_type = &sem->type_verum;
    specs[72].return_type = &sem->type_rex;
    specs[73].return_type = &sem->type_nihil;
    specs[74].return_type = &sem->type_nihil;
    specs[75].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[76].return_type = &sem->type_nihil;
    specs[77].return_type = &sem->type_verum;
    specs[78].return_type = &sem->type_verum;
    specs[79].return_type = &sem->type_verum;
    specs[80].return_type = &sem->type_rex;
    specs[81].return_type = &sem->type_verum;
    specs[82].return_type = &sem->type_nihil;
    specs[83].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[84].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[85].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[86].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[87].return_type = &sem->type_verum;
    specs[88].return_type = &sem->type_verum;
    specs[89].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[90].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[91].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[92].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[93].return_type = &sem->type_verum;
    specs[94].return_type = &sem->type_verum;
    specs[95].return_type = &sem->type_umbra;
    specs[96].return_type = &sem->type_umbra;
    specs[97].return_type = &sem->type_umbra;
    specs[98].return_type = &sem->type_umbra;
    specs[99].return_type = &sem->type_umbra;
    specs[100].return_type = &sem->type_umbra;
    specs[101].return_type = &sem->type_umbra;
    specs[102].return_type = &sem->type_umbra;
    specs[103].return_type = &sem->type_umbra;
    specs[104].return_type = &sem->type_umbra;
    specs[105].return_type = &sem->type_umbra;
    specs[106].return_type = &sem->type_umbra;
    specs[107].return_type = &sem->type_umbra;
    specs[108].return_type = &sem->type_umbra;
    specs[109].return_type = &sem->type_umbra;
    specs[110].return_type = &sem->type_umbra;
    specs[111].return_type = &sem->type_umbra;

    for (size_t i = 0; i < sizeof(specs) / sizeof(specs[0]); i++) {
        if (!specs[i].name) continue;
        if (strcmp(specs[i].name, name) == 0) {
            return &specs[i];
        }
    }
    return NULL;
}

/* ========================================================================
 * Forward Declarations (analysis)
 * ======================================================================== */

static cct_sem_type_t* sem_analyze_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr);
static void sem_analyze_stmt(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt);
static void sem_analyze_block(cct_semantic_analyzer_t *sem, const cct_ast_node_t *block, bool create_scope);

static bool sem_is_addressable_lvalue_node(const cct_ast_node_t *node) {
    if (!node) return false;
    switch (node->type) {
        case AST_IDENTIFIER:
        case AST_FIELD_ACCESS:
        case AST_INDEX_ACCESS:
            return true;
        case AST_UNARY_OP:
            return node->as.unary_op.operator == TOKEN_STAR; /* allow &*p in subset */
        default:
            return false;
    }
}

static bool sem_require_concrete_type_for_operation(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *node,
    const cct_sem_type_t *type
) {
    if (!type || sem_is_error_type(type) || !sem_is_type_param_type(type)) return true;
    sem_report_nodef(sem, node,
                     "operation requires concrete type but got generic type parameter '%s' (subset 10A)",
                     cct_sem_type_string(type));
    return false;
}

/* ========================================================================
 * Expression Analysis
 * ======================================================================== */

static cct_sem_type_t* sem_analyze_identifier_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    const char *name = expr->as.identifier.name;
    cct_sem_symbol_t *sym = sem_lookup(sem, name);
    if (!sym) {
        const char *closest = sem_find_symbol_name_suggestion(sem, name, CCT_SEM_SYMBOL_VARIABLE, false);
        char suggestion[256];
        const char *hint = NULL;
        if (closest) {
            snprintf(suggestion, sizeof(suggestion), "did you mean '%s'?", closest);
            hint = suggestion;
        } else {
            hint = sem_stdlib_import_hint(name);
        }

        char message[256];
        snprintf(message, sizeof(message), "undeclared symbol '%s'", name);
        sem_report_with_suggestion(sem, expr->line, expr->column, message, hint);
        return &sem->type_error;
    }

    if (sym->kind == CCT_SEM_SYMBOL_RITUALE) {
        sem_report_nodef(sem, expr, "rituale '%s' used as value; use CONIURA", name);
        return &sem->type_error;
    }

    return sym->type ? sym->type : &sem->type_error;
}

static cct_sem_type_t* sem_analyze_call_like_function(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *call_node,
    const char *name,
    const cct_ast_type_list_t *type_args,
    const cct_ast_node_list_t *args
) {
    if (name && strcmp(name, "__cast") == 0) {
        if (!type_args || type_args->count != 1) {
            sem_report_node(sem, call_node, "cast requires exactly one target type in GENUS(T)");
            if (args && args->count > 0) {
                (void)sem_analyze_expr(sem, args->nodes[0]);
            }
            return &sem->type_error;
        }
        if (!args || args->count != 1) {
            sem_report_node(sem, call_node, "cast requires exactly one source argument");
            return &sem->type_error;
        }

        cct_sem_type_t *target = sem_resolve_ast_type(
            sem, type_args->types[0], call_node->line, call_node->column
        );
        cct_sem_type_t *source = sem_analyze_expr(sem, args->nodes[0]);

        if (sem_is_error_type(target) || sem_is_error_type(source)) {
            return &sem->type_error;
        }

        if (!sem_is_numeric_type(target) || !sem_is_numeric_type(source)) {
            if (target == &sem->type_verbum || source == &sem->type_verbum) {
                sem_report_node(
                    sem, call_node,
                    "cast invalid: VERBUM conversions are not allowed (use fmt_stringify_* or parse_*)"
                );
            } else if (target == &sem->type_verum || source == &sem->type_verum) {
                sem_report_node(
                    sem, call_node,
                    "cast invalid: VERUM conversions are not allowed in subset 12B"
                );
            } else {
                sem_report_node(
                    sem, call_node,
                    "cast invalid: only numeric primitive conversions are allowed in subset 12B"
                );
            }
            return &sem->type_error;
        }

        return target;
    }

    cct_sem_symbol_t *fn = sem_find_function(sem, name);
    if (!fn) {
        const char *closest = sem_find_symbol_name_suggestion(sem, name, CCT_SEM_SYMBOL_RITUALE, true);
        char suggestion[256];
        const char *hint = NULL;
        if (closest) {
            snprintf(suggestion, sizeof(suggestion), "did you mean '%s'?", closest);
            hint = suggestion;
        } else {
            hint = sem_stdlib_import_hint(name);
        }

        char message[256];
        snprintf(message, sizeof(message), "rituale '%s' is not declared", name);
        sem_report_with_suggestion(sem, call_node->line, call_node->column, message, hint);
        for (size_t i = 0; args && i < args->count; i++) {
            (void)sem_analyze_expr(sem, args->nodes[i]);
        }
        return &sem->type_error;
    }

    bool has_type_args = type_args && type_args->count > 0;
    size_t fn_type_arity = fn->type_param_count;
    cct_sem_type_t **inst_type_args = NULL;

    if (fn_type_arity > 0) {
        if (!has_type_args) {
            sem_report_nodef(sem, call_node,
                             "generic rituale '%s' requires explicit GENUS(...) instantiation in subset 10B (subset final da FASE 10 has no type-arg inference)",
                             name);
        } else if (type_args->count != fn_type_arity) {
            sem_report_nodef(sem, call_node,
                             "generic rituale '%s' expects %zu type argument(s), got %zu (subset 10B)",
                             name, fn_type_arity, type_args->count);
        }

        if (has_type_args) {
            inst_type_args = (cct_sem_type_t**)sem_calloc(fn_type_arity, sizeof(*inst_type_args));
            size_t ninst = type_args->count < fn_type_arity ? type_args->count : fn_type_arity;
            for (size_t i = 0; i < ninst; i++) {
                cct_ast_type_t *ast_arg = type_args->types[i];
                cct_sem_type_t *resolved = sem_resolve_ast_type(sem, ast_arg, call_node->line, call_node->column);
                inst_type_args[i] = resolved;
                if (ast_arg) {
                    (void)sem_is_materializable_type_arg_10b(
                        sem, ast_arg, resolved, call_node->line, call_node->column
                    );
                }
                (void)sem_validate_genus_constraint_instantiation_10d(
                    sem, call_node, fn, i, ast_arg, resolved
                );
            }
        }
    } else if (has_type_args) {
        sem_report_nodef(sem, call_node,
                         "GENUS(...) applied to non-generic rituale '%s' (subset 10B)",
                         name);
    }

    size_t argc = args ? args->count : 0;
    if (argc != fn->param_count) {
        sem_report_nodef(sem, call_node,
                         "rituale '%s' expects %zu argument(s), got %zu",
                         name, fn->param_count, argc);
    }

    size_t n = argc < fn->param_count ? argc : fn->param_count;
    for (size_t i = 0; i < argc; i++) {
        cct_sem_type_t *expected_type = (i < n) ? fn->param_types[i] : NULL;
        if (expected_type && fn_type_arity > 0 && fn->type_param_names && inst_type_args) {
            expected_type = sem_substitute_type_params(
                sem, expected_type, fn->type_param_names, inst_type_args, fn_type_arity
            );
        }
        cct_sem_type_t *arg_type = NULL;
        if (i < n && expected_type &&
            expected_type->kind == CCT_SEM_TYPE_POINTER &&
            args->nodes[i] && args->nodes[i]->type == AST_IDENTIFIER) {
            cct_sem_symbol_t *arg_sym = sem_lookup(sem, args->nodes[i]->as.identifier.name);
            if (arg_sym && arg_sym->kind == CCT_SEM_SYMBOL_RITUALE) {
                /* subset 12D.2 callback bridge: ritual identifier can be used as function-pointer argument */
                arg_type = expected_type;
            }
        }
        if (!arg_type) {
            arg_type = sem_analyze_expr(sem, args->nodes[i]);
        }
        if (i < n && !sem_types_compatible_assign(sem, expected_type, arg_type)) {
            sem_report_nodef(sem, args->nodes[i],
                             "argument %zu to rituale '%s' has incompatible type (%s -> %s)",
                             i + 1, name, cct_sem_type_string(arg_type), cct_sem_type_string(expected_type));
        }
    }
    cct_sem_type_t *ret = fn->return_type ? fn->return_type : &sem->type_nihil;
    if (fn_type_arity > 0 && fn->type_param_names && inst_type_args) {
        ret = sem_substitute_type_params(sem, ret, fn->type_param_names, inst_type_args, fn_type_arity);
    }
    free(inst_type_args);
    return ret;
}

static cct_sem_type_t* sem_analyze_builtin_obsecro(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *expr
) {
    const char *name = expr->as.obsecro.name;
    const cct_sem_builtin_spec_t *spec = sem_find_builtin(sem, name);
    if (!spec) {
        sem_report_nodef(sem, expr, "unknown OBSECRO builtin '%s'", name);
        for (size_t i = 0; expr->as.obsecro.arguments && i < expr->as.obsecro.arguments->count; i++) {
            (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
        }
        return &sem->type_error;
    }

    size_t argc = expr->as.obsecro.arguments ? expr->as.obsecro.arguments->count : 0;
    if (argc < spec->min_args) {
        sem_report_nodef(sem, expr,
                         "OBSECRO %s expects at least %zu argument(s), got %zu",
                         name, spec->min_args, argc);
    }
    if (!spec->variadic && argc != spec->min_args) {
        sem_report_nodef(sem, expr,
                         "OBSECRO %s expects exactly %zu argument(s), got %zu",
                         name, spec->min_args, argc);
    }

    for (size_t i = 0; i < argc; i++) {
        const cct_ast_node_t *arg_node = expr->as.obsecro.arguments->nodes[i];
        bool callback_slot = false;
        if ((strcmp(name, "collection_fluxus_map") == 0 && i == 3) ||
            (strcmp(name, "collection_fluxus_filter") == 0 && i == 2) ||
            (strcmp(name, "collection_fluxus_fold") == 0 && i == 4) ||
            (strcmp(name, "collection_fluxus_find") == 0 && i == 2) ||
            (strcmp(name, "collection_fluxus_any") == 0 && i == 2) ||
            (strcmp(name, "collection_fluxus_all") == 0 && i == 2) ||
            (strcmp(name, "collection_series_map") == 0 && i == 4) ||
            (strcmp(name, "collection_series_filter") == 0 && i == 3) ||
            (strcmp(name, "collection_series_reduce") == 0 && i == 5) ||
            (strcmp(name, "collection_series_find") == 0 && i == 3) ||
            (strcmp(name, "collection_series_any") == 0 && i == 3) ||
            (strcmp(name, "collection_series_all") == 0 && i == 3)) {
            callback_slot = true;
        }

        cct_sem_type_t *arg_type = NULL;
        if (callback_slot && arg_node && arg_node->type == AST_IDENTIFIER) {
            cct_sem_symbol_t *sym = sem_lookup(sem, arg_node->as.identifier.name);
            if (sym && sym->kind == CCT_SEM_SYMBOL_RITUALE) {
                arg_type = sem_make_pointer_type(sem, &sem->type_nihil);
            }
        }
        if (!arg_type) {
            arg_type = sem_analyze_expr(sem, arg_node);
        }
        if (strcmp(name, "pete") == 0 && i == 0 && !sem_is_integer_type(arg_type)) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO pete expects integer size argument");
        }
        if (strcmp(name, "libera") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO libera expects pointer argument (manual explicit discard in subset final da FASE 7)");
        }
        if (strcmp(name, "verbum_len") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO verbum_len expects VERBUM argument");
        }
        if ((strcmp(name, "verbum_concat") == 0 || strcmp(name, "verbum_compare") == 0 || strcmp(name, "verbum_find") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "verbum_trim") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO verbum_trim expects VERBUM argument");
        }
        if (strcmp(name, "verbum_substring") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_substring expects first argument as VERBUM");
            }
            if ((i == 1 || i == 2) && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_substring expects integer indices");
            }
        }
        if (strcmp(name, "fmt_stringify_int") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fmt_stringify_int expects integer argument");
        }
        if (strcmp(name, "fmt_stringify_real") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_UMBRA || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fmt_stringify_real expects UMBRA argument");
        }
        if (strcmp(name, "fmt_stringify_float") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_FLAMMA || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fmt_stringify_float expects FLAMMA argument");
        }
        if ((strcmp(name, "fmt_parse_int") == 0 || strcmp(name, "fmt_parse_real") == 0) &&
            i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if (strcmp(name, "mem_alloc") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mem_alloc expects integer size argument");
        }
        if (strcmp(name, "mem_realloc") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO mem_realloc expects pointer as first argument");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO mem_realloc expects integer size as second argument");
            }
        }
        if ((strcmp(name, "mem_free") == 0 || strcmp(name, "mem_copy") == 0 || strcmp(name, "mem_set") == 0 || strcmp(name, "mem_zero") == 0 || strcmp(name, "mem_compare") == 0) &&
            (i == 0 || (i == 1 && (strcmp(name, "mem_copy") == 0 || strcmp(name, "mem_compare") == 0))) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "mem_copy") == 0 || strcmp(name, "mem_set") == 0 || strcmp(name, "mem_compare") == 0) &&
            i == 2 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer size argument", name);
        }
        if (strcmp(name, "mem_set") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mem_set expects integer byte value as second argument");
        }
        if (strcmp(name, "mem_zero") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mem_zero expects integer size argument");
        }
        if (strcmp(name, "fluxus_init") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fluxus_init expects integer element size argument");
        }
        if ((strcmp(name, "fluxus_free") == 0 || strcmp(name, "fluxus_push") == 0 ||
             strcmp(name, "fluxus_pop") == 0 || strcmp(name, "fluxus_len") == 0 ||
             strcmp(name, "fluxus_get") == 0 || strcmp(name, "fluxus_clear") == 0 ||
             strcmp(name, "fluxus_reserve") == 0 || strcmp(name, "fluxus_capacity") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects fluxus pointer as first argument", name);
        }
        if ((strcmp(name, "fluxus_push") == 0 || strcmp(name, "fluxus_pop") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer as second argument", name);
        }
        if ((strcmp(name, "fluxus_get") == 0 || strcmp(name, "fluxus_reserve") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer as second argument", name);
        }
        if ((strcmp(name, "io_print") == 0 || strcmp(name, "io_println") == 0 ||
             strcmp(name, "fs_read_all") == 0 || strcmp(name, "fs_exists") == 0 ||
             strcmp(name, "fs_size") == 0 || strcmp(name, "path_basename") == 0 ||
             strcmp(name, "path_dirname") == 0 || strcmp(name, "path_ext") == 0 ||
             strcmp(name, "path_join") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if (strcmp(name, "io_print_int") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO io_print_int expects integer argument");
        }
        if ((strcmp(name, "fs_write_all") == 0 || strcmp(name, "fs_append_all") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "path_join") == 0 && i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO path_join expects VERBUM arguments");
        }
        if (strcmp(name, "random_seed") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO random_seed expects integer seed argument");
        }
        if (strcmp(name, "random_int") == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO random_int expects integer lo/hi arguments");
        }
        if (strcmp(name, "option_some") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO option_some expects pointer value argument");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO option_some expects integer type size argument");
            }
        }
        if (strcmp(name, "option_none") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO option_none expects integer type size argument");
        }
        if ((strcmp(name, "option_is_some") == 0 || strcmp(name, "option_is_none") == 0 ||
             strcmp(name, "option_unwrap_ptr") == 0 || strcmp(name, "option_free") == 0 ||
             strcmp(name, "result_is_ok") == 0 || strcmp(name, "result_is_err") == 0 ||
             strcmp(name, "result_unwrap_ptr") == 0 || strcmp(name, "result_unwrap_err_ptr") == 0 ||
             strcmp(name, "result_free") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "option_expect_ptr") == 0 || strcmp(name, "result_expect_ptr") == 0)) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects pointer value as first argument", name);
            }
            if (i == 1 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects VERBUM message as second argument", name);
            }
        }
        if (strcmp(name, "result_ok") == 0 || strcmp(name, "result_err") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects pointer payload as first argument", name);
            }
            if ((i == 1 || i == 2) && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer type sizes as second/third arguments", name);
            }
        }
        if (strcmp(name, "map_init") == 0 && (i == 0 || i == 1) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO map_init expects integer key/value size arguments");
        }
        if ((strcmp(name, "map_free") == 0 || strcmp(name, "map_len") == 0 ||
             strcmp(name, "map_is_empty") == 0 || strcmp(name, "map_capacity") == 0 ||
             strcmp(name, "map_clear") == 0 || strcmp(name, "set_free") == 0 ||
             strcmp(name, "set_len") == 0 || strcmp(name, "set_is_empty") == 0 ||
             strcmp(name, "set_clear") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "map_insert") == 0 || strcmp(name, "map_remove") == 0 ||
             strcmp(name, "map_get_ptr") == 0 || strcmp(name, "map_contains") == 0 ||
             strcmp(name, "map_reserve") == 0 || strcmp(name, "set_insert") == 0 ||
             strcmp(name, "set_remove") == 0 || strcmp(name, "set_contains") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer container argument", name);
        }
        if ((strcmp(name, "map_insert") == 0 || strcmp(name, "map_remove") == 0 ||
             strcmp(name, "map_get_ptr") == 0 || strcmp(name, "map_contains") == 0 ||
             strcmp(name, "set_insert") == 0 || strcmp(name, "set_remove") == 0 ||
             strcmp(name, "set_contains") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer payload argument", name);
        }
        if (strcmp(name, "map_insert") == 0 && i == 2 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO map_insert expects pointer value argument");
        }
        if (strcmp(name, "map_reserve") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO map_reserve expects integer size argument");
        }
        if (strcmp(name, "set_init") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer size argument", name);
        }
        if ((strcmp(name, "collection_fluxus_map") == 0 ||
             strcmp(name, "collection_fluxus_filter") == 0 ||
             strcmp(name, "collection_fluxus_fold") == 0 ||
             strcmp(name, "collection_fluxus_find") == 0 ||
             strcmp(name, "collection_fluxus_any") == 0 ||
             strcmp(name, "collection_fluxus_all") == 0 ||
             strcmp(name, "collection_series_map") == 0 ||
             strcmp(name, "collection_series_filter") == 0 ||
             strcmp(name, "collection_series_reduce") == 0 ||
             strcmp(name, "collection_series_find") == 0 ||
             strcmp(name, "collection_series_any") == 0 ||
             strcmp(name, "collection_series_all") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer collection argument", name);
        }
        if ((strcmp(name, "collection_fluxus_map") == 0 ||
             strcmp(name, "collection_fluxus_filter") == 0 ||
             strcmp(name, "collection_fluxus_find") == 0 ||
             strcmp(name, "collection_fluxus_any") == 0 ||
             strcmp(name, "collection_fluxus_all") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer element-size argument", name);
        }
        if (strcmp(name, "collection_fluxus_map") == 0 && i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO collection_fluxus_map expects integer result-size argument");
        }
        if (strcmp(name, "collection_fluxus_fold") == 0) {
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO collection_fluxus_fold expects integer element-size argument");
            }
            if (i == 2 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO collection_fluxus_fold expects pointer initial-accumulator argument");
            }
            if (i == 3 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO collection_fluxus_fold expects integer accumulator-size argument");
            }
        }
        if ((strcmp(name, "collection_series_map") == 0 ||
             strcmp(name, "collection_series_filter") == 0 ||
             strcmp(name, "collection_series_reduce") == 0 ||
             strcmp(name, "collection_series_find") == 0 ||
             strcmp(name, "collection_series_any") == 0 ||
             strcmp(name, "collection_series_all") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer length argument", name);
        }
        if ((strcmp(name, "collection_series_map") == 0 ||
             strcmp(name, "collection_series_filter") == 0 ||
             strcmp(name, "collection_series_reduce") == 0 ||
             strcmp(name, "collection_series_find") == 0 ||
             strcmp(name, "collection_series_any") == 0 ||
             strcmp(name, "collection_series_all") == 0) &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer element-size argument", name);
        }
        if (strcmp(name, "collection_series_map") == 0 && i == 3 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO collection_series_map expects integer result-size argument");
        }
        if (strcmp(name, "collection_series_reduce") == 0) {
            if (i == 3 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO collection_series_reduce expects pointer initial-accumulator argument");
            }
            if (i == 4 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO collection_series_reduce expects integer accumulator-size argument");
            }
        }
        if (callback_slot &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects callback rituale pointer in this argument position", name);
        }
    }

    return spec->return_type ? spec->return_type : &sem->type_error;
}

static cct_sem_type_t* sem_analyze_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    if (!expr) return &sem->type_nihil;

    switch (expr->type) {
        case AST_LITERAL_INT:
            return &sem->type_rex;
        case AST_LITERAL_REAL:
            return &sem->type_umbra;
        case AST_LITERAL_STRING:
            return &sem->type_verbum;
        case AST_LITERAL_BOOL:
            return &sem->type_verum;
        case AST_LITERAL_NIHIL:
            return &sem->type_nihil;

        case AST_IDENTIFIER:
            return sem_analyze_identifier_expr(sem, expr);

        case AST_MENSURA:
            (void)sem_resolve_ast_type(sem, expr->as.mensura.type, expr->line, expr->column);
            return &sem->type_rex;

        case AST_CONIURA:
            return sem_analyze_call_like_function(
                sem,
                expr,
                expr->as.coniura.name,
                expr->as.coniura.type_args,
                expr->as.coniura.arguments
            );

        case AST_OBSECRO:
            return sem_analyze_builtin_obsecro(sem, expr);

        case AST_CALL: {
            if (!expr->as.call.callee || expr->as.call.callee->type != AST_IDENTIFIER) {
                sem_report_node(sem, expr, "only identifier callees are supported in direct calls");
                if (expr->as.call.arguments) {
                    for (size_t i = 0; i < expr->as.call.arguments->count; i++) {
                        (void)sem_analyze_expr(sem, expr->as.call.arguments->nodes[i]);
                    }
                }
                return &sem->type_error;
            }
            return sem_analyze_call_like_function(
                sem,
                expr,
                expr->as.call.callee->as.identifier.name,
                NULL,
                expr->as.call.arguments
            );
        }

        case AST_FIELD_ACCESS:
        {
            cct_sem_type_t *obj_type = sem_analyze_expr(sem, expr->as.field_access.object);
            if (sem_is_error_type(obj_type)) return &sem->type_error;

            if (!obj_type || obj_type->kind != CCT_SEM_TYPE_NAMED) {
                sem_report_node(sem, expr, "field access requires SIGILLUM-typed object");
                return &sem->type_error;
            }

            cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, obj_type->name);
            const cct_ast_node_t *sig_decl = type_sym ? type_sym->type_decl : NULL;
            cct_sem_generic_instance_t *inst = NULL;
            if (!sig_decl) {
                inst = sem_find_generic_instance(sem, obj_type->name);
                if (inst) sig_decl = inst->decl;
            }
            if (!sig_decl) {
                sem_report_nodef(sem, expr, "unknown SIGILLUM type '%s' for field access", obj_type->name);
                return &sem->type_error;
            }
            if (sig_decl->type != AST_SIGILLUM) {
                sem_report_nodef(sem, expr, "type '%s' does not support field access", obj_type->name);
                return &sem->type_error;
            }

            const cct_ast_field_t *field = NULL;
            if (sig_decl->as.sigillum.fields) {
                for (size_t i = 0; i < sig_decl->as.sigillum.fields->count; i++) {
                    cct_ast_field_t *f = sig_decl->as.sigillum.fields->fields[i];
                    if (f && f->name && strcmp(f->name, expr->as.field_access.field) == 0) {
                        field = f;
                        break;
                    }
                }
            }
            if (!field) {
                sem_report_nodef(sem, expr, "SIGILLUM '%s' has no field '%s'",
                                 obj_type->name, expr->as.field_access.field);
                return &sem->type_error;
            }

            if (inst && sig_decl->as.sigillum.type_params && sig_decl->as.sigillum.type_params->count > 0) {
                size_t bind_count = sig_decl->as.sigillum.type_params->count;
                if (inst->type_arg_count == bind_count) {
                    char **bind_names = (char**)sem_calloc(bind_count, sizeof(*bind_names));
                    for (size_t i = 0; i < bind_count; i++) {
                        cct_ast_type_param_t *tp = sig_decl->as.sigillum.type_params->params[i];
                        bind_names[i] = tp ? tp->name : NULL;
                    }
                    cct_sem_type_t *resolved = sem_resolve_ast_type_with_bindings(
                        sem, field->type, bind_names, inst->type_args, bind_count, field->line, field->column
                    );
                    free(bind_names);
                    return resolved;
                }
            }

            return sem_resolve_ast_type(sem, field->type, field->line, field->column);
        }

        case AST_INDEX_ACCESS: {
            cct_sem_type_t *base = sem_analyze_expr(sem, expr->as.index_access.array);
            cct_sem_type_t *idx = sem_analyze_expr(sem, expr->as.index_access.index);
            if (!sem_is_integer_type(idx) && !sem_is_error_type(idx)) {
                sem_report_node(sem, expr->as.index_access.index, "index expression must be integer");
            }
            if (base && base->kind == CCT_SEM_TYPE_ARRAY) return base->element ? base->element : &sem->type_error;
            if (base && base->kind == CCT_SEM_TYPE_POINTER) return base->element ? base->element : &sem->type_error;
            if (!sem_is_error_type(base)) {
                sem_report_node(sem, expr, "indexing requires array or pointer type");
            }
            return &sem->type_error;
        }

        case AST_UNARY_OP: {
            cct_sem_type_t *operand = sem_analyze_expr(sem, expr->as.unary_op.operand);
            switch (expr->as.unary_op.operator) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                    if (!sem_require_concrete_type_for_operation(sem, expr, operand)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_numeric_type(operand) && !sem_is_error_type(operand)) {
                        sem_report_node(sem, expr, "unary +/- require numeric operand");
                        return &sem->type_error;
                    }
                    return operand;
                case TOKEN_NON:
                    if (!sem_require_concrete_type_for_operation(sem, expr, operand)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_bool_type(operand) && !sem_is_error_type(operand)) {
                        sem_report_node(sem, expr, "NON requires boolean operand");
                        return &sem->type_error;
                    }
                    return &sem->type_verum;
                case TOKEN_NON_BIT:
                    if (!sem_require_concrete_type_for_operation(sem, expr, operand)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(operand) && !sem_is_error_type(operand)) {
                        sem_report_node(sem, expr, "NON_BIT requires integer operand");
                        return &sem->type_error;
                    }
                    return operand;
                case TOKEN_STAR:
                    if (operand && operand->kind == CCT_SEM_TYPE_POINTER) {
                        return operand->element ? operand->element : &sem->type_error;
                    }
                    if (!sem_is_error_type(operand)) {
                        sem_report_node(sem, expr, "dereference requires pointer operand");
                    }
                    return &sem->type_error;
                case TOKEN_SPECULUM:
                    if (!sem_is_addressable_lvalue_node(expr->as.unary_op.operand)) {
                        if (!sem_is_error_type(operand)) {
                            sem_report_node(sem, expr, "address-of (SPECULUM) requires addressable lvalue operand");
                        }
                        return &sem->type_error;
                    }
                    return sem_make_pointer_type(sem, operand);
                default:
                    sem_report_node(sem, expr, "unsupported unary operator in semantic analysis");
                    return &sem->type_error;
            }
        }

        case AST_BINARY_OP: {
            cct_sem_type_t *left = sem_analyze_expr(sem, expr->as.binary_op.left);
            cct_sem_type_t *right = sem_analyze_expr(sem, expr->as.binary_op.right);
            cct_token_type_t op = expr->as.binary_op.operator;

            switch (op) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_STAR:
                case TOKEN_SLASH:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_numeric_type(left) || !sem_is_numeric_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "arithmetic operators require numeric operands");
                        }
                        return &sem->type_error;
                    }
                    return sem_arithmetic_result_type(sem, left, right);

                case TOKEN_PERCENT:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "operator % requires integer operands");
                        }
                        return &sem->type_error;
                    }
                    return &sem->type_rex;

                case TOKEN_EQ_EQ:
                case TOKEN_BANG_EQ:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_type_equal(left, right) &&
                        !(sem_is_numeric_type(left) && sem_is_numeric_type(right)) &&
                        !sem_is_error_type(left) && !sem_is_error_type(right)) {
                        sem_report_node(sem, expr, "equality comparison uses incompatible operand types");
                    }
                    return &sem->type_verum;

                case TOKEN_LESS:
                case TOKEN_LESS_EQ:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQ:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_numeric_type(left) || !sem_is_numeric_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "ordered comparison requires numeric operands");
                        }
                    }
                    return &sem->type_verum;

                case TOKEN_ET:
                case TOKEN_VEL:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_bool_type(left) || !sem_is_bool_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "logical operators require boolean operands");
                        }
                    }
                    return &sem->type_verum;

                case TOKEN_ET_BIT:
                case TOKEN_VEL_BIT:
                case TOKEN_XOR:
                case TOKEN_SINISTER:
                case TOKEN_DEXTER:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "bitwise/shift operators require integer operands");
                        }
                    }
                    return &sem->type_rex;

                default:
                    sem_report_node(sem, expr, "unsupported binary operator in semantic analysis");
                    return &sem->type_error;
            }
        }

        default:
            sem_report_nodef(sem, expr, "unexpected expression node for semantic analysis: %s",
                             cct_ast_node_type_string(expr->type));
            return &sem->type_error;
    }
}

/* ========================================================================
 * Statement / Block Analysis
 * ======================================================================== */

static bool sem_is_lvalue_node_supported(const cct_ast_node_t *node) {
    if (!node) return false;
    if (node->type == AST_IDENTIFIER || node->type == AST_FIELD_ACCESS || node->type == AST_INDEX_ACCESS) return true;
    if (node->type == AST_UNARY_OP && node->as.unary_op.operator == TOKEN_STAR) return true;
    return false;
}

static cct_sem_type_t* sem_analyze_lvalue(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!sem_is_lvalue_node_supported(node)) {
        sem_report_node(sem, node, "assignment target is not a valid lvalue");
        return &sem->type_error;
    }
    return sem_analyze_expr(sem, node);
}

static void sem_analyze_evoca(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    cct_sem_type_t *decl_type = sem_resolve_ast_type(sem, stmt->as.evoca.var_type, stmt->line, stmt->column);

    if (stmt->as.evoca.initializer) {
        cct_sem_type_t *init_type = sem_analyze_expr(sem, stmt->as.evoca.initializer);
        if (!sem_types_compatible_assign(sem, decl_type, init_type)) {
            sem_report_nodef(sem, stmt,
                             "initializer type mismatch for '%s' (%s <- %s)",
                             stmt->as.evoca.name,
                             cct_sem_type_string(decl_type),
                             cct_sem_type_string(init_type));
        }
    }

    cct_sem_symbol_t *sym = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                              stmt->as.evoca.name, stmt->line, stmt->column);
    if (sym) {
        sym->type = decl_type;
    }
}

static void sem_analyze_vincire(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    cct_sem_type_t *target_type = sem_analyze_lvalue(sem, stmt->as.vincire.target);
    cct_sem_type_t *value_type = sem_analyze_expr(sem, stmt->as.vincire.value);

    if (!sem_types_compatible_assign(sem, target_type, value_type)) {
        sem_report_nodef(sem, stmt,
                         "assignment type mismatch (%s <- %s)",
                         cct_sem_type_string(target_type),
                         cct_sem_type_string(value_type));
        return;
    }

    /* FASE 7D final policy: shallow SIGILLUM copy assignment is part of the subset
     * only when the composed layout does not contain SPECULUM fields (directly or
     * through nested by-value SIGILLUM composition). */
    if (target_type && value_type &&
        target_type->kind == CCT_SEM_TYPE_NAMED &&
        value_type->kind == CCT_SEM_TYPE_NAMED &&
        target_type->name && value_type->name &&
        strcmp(target_type->name, value_type->name) == 0 &&
        sem_sigillum_named_copy_is_restricted(sem, target_type)) {
        sem_report_nodef(sem, stmt,
                         "shallow SIGILLUM assignment for '%s' with SPECULUM-containing fields is outside subset final da FASE 7 (use field-by-field mutation or fill-by-reference)",
                         target_type->name);
    }
}

static void sem_analyze_redde(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    cct_sem_type_t *expected = sem->current_rituale && sem->current_rituale->return_type
        ? sem->current_rituale->return_type
        : &sem->type_nihil;

    if (expected->kind == CCT_SEM_TYPE_NIHIL) {
        if (stmt->as.redde.value) {
            cct_sem_type_t *actual = sem_analyze_expr(sem, stmt->as.redde.value);
            if (!sem_is_error_type(actual)) {
                sem_report_node(sem, stmt, "REDDE with value is invalid in rituale returning NIHIL");
            }
        }
        return;
    }

    if (!stmt->as.redde.value) {
        sem_report_nodef(sem, stmt, "REDDE requires value of type %s", cct_sem_type_string(expected));
        return;
    }

    cct_sem_type_t *actual = sem_analyze_expr(sem, stmt->as.redde.value);
    if (!sem_types_compatible_assign(sem, expected, actual)) {
        sem_report_nodef(sem, stmt,
                         "return type mismatch (%s <- %s)",
                         cct_sem_type_string(expected),
                         cct_sem_type_string(actual));
    }
}

static void sem_analyze_dimitte(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    const cct_ast_node_t *target = stmt->as.dimitte.target;
    if (!target) {
        sem_report_node(sem, stmt, "DIMITTE requires pointer symbol");
        return;
    }
    if (target->type != AST_IDENTIFIER) {
        /* Keep 7A policy explicit and narrow. */
        (void)sem_analyze_expr(sem, target);
        sem_report_node(sem, target, "DIMITTE in subset final da FASE 7 requires identifier pointer symbol");
        return;
    }

    cct_sem_symbol_t *sym = sem_lookup(sem, target->as.identifier.name);
    if (!sym) {
        sem_report_nodef(sem, target, "undeclared symbol '%s'", target->as.identifier.name);
        return;
    }
    if (sym->kind != CCT_SEM_SYMBOL_VARIABLE && sym->kind != CCT_SEM_SYMBOL_PARAMETER) {
        sem_report_node(sem, target, "DIMITTE target must be variable/parameter pointer symbol");
        return;
    }
    if (!sym->type || sym->type->kind != CCT_SEM_TYPE_POINTER) {
        sem_report_nodef(sem, target, "DIMITTE requires pointer symbol (manually-liberable in subset final da FASE 7; got %s)",
                         cct_sem_type_string(sym->type));
        return;
    }
}

static void sem_analyze_condition(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    cct_sem_type_t *type = sem_analyze_expr(sem, expr);
    if (!sem_is_bool_type(type) && !sem_is_error_type(type)) {
        sem_report_node(sem, expr, "condition expression must be VERUM (boolean)");
    }
}

static void sem_analyze_iace(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    if (!stmt->as.iace.value) {
        sem_report_node(sem, stmt,
                        "IACE requires VERBUM or FRACTUM payload in subset 8A (subset final da FASE 8 preserves this restriction)");
        return;
    }

    cct_sem_type_t *payload = sem_analyze_expr(sem, stmt->as.iace.value);
    if (sem_is_error_type(payload)) return;

    if (payload != &sem->type_verbum && payload != &sem->type_fractum) {
        sem_report_nodef(sem, stmt,
                         "IACE payload type %s is outside subset 8A (allowed: VERBUM, FRACTUM) [subset final da FASE 8]",
                         cct_sem_type_string(payload));
    }
}

static void sem_analyze_tempta(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    if (!stmt->as.tempta.try_block) {
        sem_report_node(sem, stmt, "TEMPTA requires protected block");
        return;
    }
    if (!stmt->as.tempta.cape_block || !stmt->as.tempta.cape_name || !stmt->as.tempta.cape_type) {
        sem_report_node(sem, stmt,
                        "TEMPTA requires CAPE FRACTUM ident in subset 8A (subset final da FASE 8 keeps CAPE obrigatório)");
        /* Analyze what exists to surface secondary errors deterministically. */
    }

    if (stmt->as.tempta.cape_type) {
        cct_sem_type_t *cape_t = sem_resolve_ast_type(sem, stmt->as.tempta.cape_type, stmt->line, stmt->column);
        if (!sem_is_error_type(cape_t) && cape_t != &sem->type_fractum) {
            sem_report_node(sem, stmt,
                            "CAPE in subset 8A requires type FRACTUM (subset final da FASE 8 mantém CAPE FRACTUM)");
        }
    }

    sem_analyze_stmt(sem, stmt->as.tempta.try_block);

    if (stmt->as.tempta.cape_block) {
        sem_push_scope(sem, CCT_SEM_SCOPE_BLOCK);
        if (stmt->as.tempta.cape_name) {
            cct_sem_symbol_t *sym = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                                      stmt->as.tempta.cape_name, stmt->line, stmt->column);
            if (sym) sym->type = &sem->type_fractum;
        }
        sem_analyze_stmt(sem, stmt->as.tempta.cape_block);
        sem_pop_scope(sem);
    }

    if (stmt->as.tempta.semper_block) {
        sem_analyze_stmt(sem, stmt->as.tempta.semper_block);
    }
}

static void sem_analyze_stmt(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        case AST_BLOCK:
            sem_analyze_block(sem, stmt, true);
            return;

        case AST_EVOCA:
            sem_analyze_evoca(sem, stmt);
            return;

        case AST_VINCIRE:
            sem_analyze_vincire(sem, stmt);
            return;

        case AST_REDDE:
            sem_analyze_redde(sem, stmt);
            return;

        case AST_ANUR:
            if (stmt->as.anur.value) {
                cct_sem_type_t *type = sem_analyze_expr(sem, stmt->as.anur.value);
                if (!sem_is_integer_type(type) && !sem_is_error_type(type)) {
                    sem_report_node(sem, stmt, "ANUR argument must be integer");
                }
            }
            return;

        case AST_DIMITTE:
            sem_analyze_dimitte(sem, stmt);
            return;

        case AST_IACE:
            sem_analyze_iace(sem, stmt);
            return;

        case AST_TEMPTA:
            sem_analyze_tempta(sem, stmt);
            return;

        case AST_EXPR_STMT:
            (void)sem_analyze_expr(sem, stmt->as.expr_stmt.expression);
            return;

        case AST_SI:
            sem_analyze_condition(sem, stmt->as.si.condition);
            sem_analyze_stmt(sem, stmt->as.si.then_branch);
            if (stmt->as.si.else_branch) sem_analyze_stmt(sem, stmt->as.si.else_branch);
            return;

        case AST_DUM:
            sem_analyze_condition(sem, stmt->as.dum.condition);
            sem_analyze_stmt(sem, stmt->as.dum.body);
            return;

        case AST_DONEC:
            sem_analyze_stmt(sem, stmt->as.donec.body);
            if (stmt->as.donec.condition) sem_analyze_condition(sem, stmt->as.donec.condition);
            return;

        case AST_REPETE: {
            cct_sem_type_t *start_t = sem_analyze_expr(sem, stmt->as.repete.start);
            cct_sem_type_t *end_t = sem_analyze_expr(sem, stmt->as.repete.end);
            cct_sem_type_t *step_t = stmt->as.repete.step ? sem_analyze_expr(sem, stmt->as.repete.step) : &sem->type_rex;

            if (!sem_is_integer_type(start_t) && !sem_is_error_type(start_t)) {
                sem_report_node(sem, stmt->as.repete.start, "REPETE start value must be integer");
            }
            if (!sem_is_integer_type(end_t) && !sem_is_error_type(end_t)) {
                sem_report_node(sem, stmt->as.repete.end, "REPETE end value must be integer");
            }
            if (!sem_is_integer_type(step_t) && !sem_is_error_type(step_t)) {
                sem_report_node(sem, stmt->as.repete.step, "REPETE GRADUS value must be integer");
            }

            sem_push_scope(sem, CCT_SEM_SCOPE_LOOP);
            cct_sem_symbol_t *iter = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                                       stmt->as.repete.iterator, stmt->line, stmt->column);
            if (iter) iter->type = &sem->type_rex;
            sem_analyze_stmt(sem, stmt->as.repete.body);
            sem_pop_scope(sem);
            return;
        }

        case AST_ITERUM: {
            cct_sem_type_t *collection_t = sem_analyze_expr(sem, stmt->as.iterum.collection);
            cct_sem_type_t *item_t = &sem->type_error;

            if (collection_t && collection_t->kind == CCT_SEM_TYPE_ARRAY && collection_t->element) {
                item_t = collection_t->element;
            } else if (collection_t && collection_t->kind == CCT_SEM_TYPE_POINTER &&
                       collection_t->element && collection_t->element->kind == CCT_SEM_TYPE_NIHIL) {
                /* FASE 12D.3 subset: treat FLUXUS opaque pointer iteration item as REX by default. */
                item_t = &sem->type_rex;
            } else if (!sem_is_error_type(collection_t)) {
                sem_report_nodef(
                    sem,
                    stmt->as.iterum.collection,
                    "ITERUM requires FLUXUS or SERIES, found: %s",
                    cct_sem_type_string(collection_t)
                );
            }

            sem_push_scope(sem, CCT_SEM_SCOPE_LOOP);
            cct_sem_symbol_t *iter = sem_define_symbol(
                sem,
                CCT_SEM_SYMBOL_VARIABLE,
                stmt->as.iterum.item_name ? stmt->as.iterum.item_name : "_",
                stmt->line,
                stmt->column
            );
            if (iter) iter->type = item_t;
            sem_analyze_stmt(sem, stmt->as.iterum.body);
            sem_pop_scope(sem);
            return;
        }

        case AST_FRANGE:
        case AST_RECEDE:
        case AST_TRANSITUS:
            return;

        default:
            sem_report_nodef(sem, stmt, "unsupported statement node in semantic analysis: %s",
                             cct_ast_node_type_string(stmt->type));
            return;
    }
}

static void sem_analyze_block(cct_semantic_analyzer_t *sem, const cct_ast_node_t *block, bool create_scope) {
    if (!block || block->type != AST_BLOCK) {
        sem_report_node(sem, block, "internal error: expected AST_BLOCK");
        return;
    }

    if (create_scope) sem_push_scope(sem, CCT_SEM_SCOPE_BLOCK);

    cct_ast_node_list_t *stmts = block->as.block.statements;
    if (stmts) {
        for (size_t i = 0; i < stmts->count; i++) {
            sem_analyze_stmt(sem, stmts->nodes[i]);
        }
    }

    if (create_scope) sem_pop_scope(sem);
}

/* ========================================================================
 * Global Registration Passes
 * ======================================================================== */

static void sem_register_named_types_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);
static void sem_register_global_pacta_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);
static void sem_register_global_rituales_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);
static void sem_verify_sigillum_conformance_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);
static void sem_analyze_global_bodies_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);

static void sem_walk_node_list(const cct_ast_node_list_t *list,
                               void (*fn)(cct_semantic_analyzer_t*, const cct_ast_node_t*),
                               cct_semantic_analyzer_t *sem) {
    if (!list || !fn) return;
    for (size_t i = 0; i < list->count; i++) {
        fn(sem, list->nodes[i]);
    }
}

static cct_sem_signature_tmp_t sem_resolve_signature_tmp(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *rituale
) {
    cct_sem_signature_tmp_t sig;
    memset(&sig, 0, sizeof(sig));

    if (!rituale || rituale->type != AST_RITUALE) return sig;

    sig.params = rituale->as.rituale.params;
    sig.count = sig.params ? sig.params->count : 0;
    if (sig.count == 0) return sig;

    sig.types = (cct_sem_type_t**)sem_calloc(sig.count, sizeof(cct_sem_type_t*));
    for (size_t i = 0; i < sig.count; i++) {
        const cct_ast_param_t *param = sig.params->params[i];
        sig.types[i] = sem_resolve_ast_type(sem, param->type, param->line, param->column);
    }
    return sig;
}

static void sem_signature_tmp_dispose(cct_sem_signature_tmp_t *sig) {
    if (!sig) return;
    free(sig->types);
    memset(sig, 0, sizeof(*sig));
}

static bool sem_signature_types_equal(
    const cct_sem_signature_tmp_t *a,
    const cct_sem_signature_tmp_t *b
) {
    if (!a || !b) return false;
    if (a->count != b->count) return false;
    for (size_t i = 0; i < a->count; i++) {
        if (!sem_type_equal(a->types[i], b->types[i])) return false;
    }
    return true;
}

static void sem_validate_pactum_signature_duplicates(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *pactum
) {
    if (!sem || !pactum || pactum->type != AST_PACTUM || !pactum->as.pactum.signatures) return;

    size_t n = pactum->as.pactum.signatures->count;
    if (n < 2) return;

    cct_sem_signature_tmp_t *params = (cct_sem_signature_tmp_t*)sem_calloc(n, sizeof(*params));
    cct_sem_type_t **returns = (cct_sem_type_t**)sem_calloc(n, sizeof(*returns));

    for (size_t i = 0; i < n; i++) {
        const cct_ast_node_t *sig = pactum->as.pactum.signatures->nodes[i];
        if (!sig || sig->type != AST_RITUALE) continue;
        params[i] = sem_resolve_signature_tmp(sem, sig);
        returns[i] = sig->as.rituale.return_type
            ? sem_resolve_ast_type(sem, sig->as.rituale.return_type, sig->line, sig->column)
            : &sem->type_nihil;
    }

    for (size_t i = 0; i < n; i++) {
        const cct_ast_node_t *a = pactum->as.pactum.signatures->nodes[i];
        if (!a || a->type != AST_RITUALE) continue;
        for (size_t j = i + 1; j < n; j++) {
            const cct_ast_node_t *b = pactum->as.pactum.signatures->nodes[j];
            if (!b || b->type != AST_RITUALE) continue;
            if (!a->as.rituale.name || !b->as.rituale.name) continue;
            if (strcmp(a->as.rituale.name, b->as.rituale.name) != 0) continue;
            if (!sem_signature_types_equal(&params[i], &params[j])) continue;
            if (!sem_type_equal(returns[i], returns[j])) continue;

            sem_report_nodef(
                sem, b,
                "duplicate PACTUM signature '%s' in contract '%s' (subset 10C)",
                b->as.rituale.name,
                pactum->as.pactum.name ? pactum->as.pactum.name : "<anonymous>"
            );
        }
    }

    for (size_t i = 0; i < n; i++) {
        sem_signature_tmp_dispose(&params[i]);
    }
    free(params);
    free(returns);
}

static void sem_validate_sigillum_pactum_conformance(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *sigillum
) {
    if (!sem || !sigillum || sigillum->type != AST_SIGILLUM) return;

    const char *pactum_name = sigillum->as.sigillum.pactum_name;
    if (!pactum_name || !pactum_name[0]) return;

    if (sigillum->as.sigillum.type_params && sigillum->as.sigillum.type_params->count > 0) {
        sem_report_nodef(
            sem,
            sigillum,
            "SIGILLUM '%s' with GENUS(...) cannot conform to PACTUM in subset 10C (constraints/generic contracts start in 10D)",
            sigillum->as.sigillum.name ? sigillum->as.sigillum.name : "<anonymous>"
        );
        return;
    }

    cct_sem_symbol_t *pact = sem_find_pactum_symbol(sem, pactum_name);
    if (!pact || !pact->pactum_decl || pact->pactum_decl->type != AST_PACTUM) {
        sem_report_nodef(
            sem,
            sigillum,
            "SIGILLUM '%s' references unknown PACTUM '%s' (subset 10C)",
            sigillum->as.sigillum.name ? sigillum->as.sigillum.name : "<anonymous>",
            pactum_name
        );
        return;
    }

    const cct_ast_node_t *pactum_decl = pact->pactum_decl;
    if (!pactum_decl->as.pactum.signatures) return;

    for (size_t i = 0; i < pactum_decl->as.pactum.signatures->count; i++) {
        const cct_ast_node_t *contract_sig = pactum_decl->as.pactum.signatures->nodes[i];
        if (!contract_sig || contract_sig->type != AST_RITUALE) continue;

        const char *ritual_name = contract_sig->as.rituale.name;
        cct_sem_symbol_t *impl = sem_find_function(sem, ritual_name);
        if (!impl) {
            sem_report_nodef(
                sem,
                sigillum,
                "SIGILLUM '%s' does not implement PACTUM '%s': missing RITUALE '%s' (subset 10C)",
                sigillum->as.sigillum.name ? sigillum->as.sigillum.name : "<anonymous>",
                pactum_name,
                ritual_name ? ritual_name : "<anonymous>"
            );
            continue;
        }

        if (impl->type_param_count > 0) {
            sem_report_nodef(
                sem,
                impl->rituale_decl ? impl->rituale_decl : contract_sig,
                "RITUALE '%s' used for PACTUM '%s' conformance cannot be generic in subset 10C",
                ritual_name ? ritual_name : "<anonymous>",
                pactum_name
            );
            continue;
        }

        cct_sem_signature_tmp_t expected = sem_resolve_signature_tmp(sem, contract_sig);
        cct_sem_type_t *expected_ret = contract_sig->as.rituale.return_type
            ? sem_resolve_ast_type(sem, contract_sig->as.rituale.return_type, contract_sig->line, contract_sig->column)
            : &sem->type_nihil;

        size_t required_param_count = expected.count + 1; /* self + contract signature params */
        if (impl->param_count != required_param_count) {
            sem_report_nodef(
                sem,
                impl->rituale_decl ? impl->rituale_decl : contract_sig,
                "RITUALE '%s' for PACTUM '%s' must have %zu parameter(s): SPECULUM %s self + %zu contract parameter(s) (subset 10C)",
                ritual_name ? ritual_name : "<anonymous>",
                pactum_name,
                required_param_count,
                sigillum->as.sigillum.name ? sigillum->as.sigillum.name : "<sigillum>",
                expected.count
            );
            sem_signature_tmp_dispose(&expected);
            continue;
        }

        cct_sem_type_t *self_type = impl->param_types[0];
        bool valid_self = self_type &&
                          self_type->kind == CCT_SEM_TYPE_POINTER &&
                          self_type->element &&
                          self_type->element->kind == CCT_SEM_TYPE_NAMED &&
                          self_type->element->name &&
                          sigillum->as.sigillum.name &&
                          strcmp(self_type->element->name, sigillum->as.sigillum.name) == 0;
        if (!valid_self) {
            sem_report_nodef(
                sem,
                impl->rituale_decl ? impl->rituale_decl : contract_sig,
                "RITUALE '%s' for PACTUM '%s' must start with parameter 'SPECULUM %s self' (subset 10C)",
                ritual_name ? ritual_name : "<anonymous>",
                pactum_name,
                sigillum->as.sigillum.name ? sigillum->as.sigillum.name : "<sigillum>"
            );
            sem_signature_tmp_dispose(&expected);
            continue;
        }

        for (size_t p = 0; p < expected.count; p++) {
            if (!sem_type_equal(impl->param_types[p + 1], expected.types[p])) {
                sem_report_nodef(
                    sem,
                    impl->rituale_decl ? impl->rituale_decl : contract_sig,
                    "RITUALE '%s' for PACTUM '%s' has parameter mismatch at position %zu (subset 10C)",
                    ritual_name ? ritual_name : "<anonymous>",
                    pactum_name,
                    p + 1
                );
                break;
            }
        }

        if (!sem_type_equal(impl->return_type, expected_ret)) {
            sem_report_nodef(
                sem,
                impl->rituale_decl ? impl->rituale_decl : contract_sig,
                "RITUALE '%s' for PACTUM '%s' has incompatible return type (subset 10C)",
                ritual_name ? ritual_name : "<anonymous>",
                pactum_name
            );
        }

        sem_signature_tmp_dispose(&expected);
    }
}

static void sem_register_named_types_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_SIGILLUM:
        case AST_ORDO: {
            const char *name = (node->type == AST_SIGILLUM) ? node->as.sigillum.name : node->as.ordo.name;
            cct_sem_symbol_t *sym = sem_define_symbol(sem, CCT_SEM_SYMBOL_TYPE, name, node->line, node->column);
            if (sym) {
                sym->type = sem_make_named_type(sem, name);
                sym->type_decl = node;

                if (node->type == AST_SIGILLUM) {
                    size_t tp_mark = sem_active_type_params_push_from_list(sem, node->as.sigillum.type_params, node);
                    if (node->as.sigillum.fields) {
                        for (size_t fi = 0; fi < node->as.sigillum.fields->count; fi++) {
                            cct_ast_field_t *field = node->as.sigillum.fields->fields[fi];
                            if (!field) continue;
                            (void)sem_resolve_ast_type(sem, field->type, field->line, field->column);
                        }
                    }
                    (void)sem_sigillum_fields_have_duplicates(sem, node);
                    sem_validate_sigillum_field_subset(sem, node);
                    sem_active_type_params_pop_to(sem, tp_mark);
                }

                if (node->type == AST_ORDO && node->as.ordo.items) {
                    for (size_t i = 0; i < node->as.ordo.items->count; i++) {
                        cct_ast_enum_item_t *item = node->as.ordo.items->items[i];
                        if (!item) continue;
                        cct_sem_symbol_t *es = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                                                 item->name, item->line, item->column);
                        if (es) es->type = sym->type;
                    }
                }
            }
            return;
        }

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_register_named_types_in_node, sem);
            return;

        default:
            return;
    }
}

static void sem_register_global_pacta_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_PACTUM: {
            cct_sem_symbol_t *existing = sem_lookup_in_scope(sem->current_scope, node->as.pactum.name);
            if (existing) {
                sem_report_nodef(
                    sem,
                    node,
                    "duplicate PACTUM '%s' in same scope (subset 10C)",
                    node->as.pactum.name ? node->as.pactum.name : "<anonymous>"
                );
                return;
            }
            cct_sem_symbol_t *sym = sem_define_symbol(
                sem, CCT_SEM_SYMBOL_PACTUM, node->as.pactum.name, node->line, node->column
            );
            if (sym) {
                sym->pactum_decl = node;
            }
            sem_validate_pactum_signature_duplicates(sem, node);

            if (node->as.pactum.signatures) {
                for (size_t i = 0; i < node->as.pactum.signatures->count; i++) {
                    const cct_ast_node_t *sig = node->as.pactum.signatures->nodes[i];
                    if (!sig || sig->type != AST_RITUALE) continue;

                    cct_sem_signature_tmp_t tmp = sem_resolve_signature_tmp(sem, sig);
                    sem_signature_tmp_dispose(&tmp);
                    if (sig->as.rituale.return_type) {
                        (void)sem_resolve_ast_type(sem, sig->as.rituale.return_type, sig->line, sig->column);
                    }
                }
            }
            return;
        }

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_register_global_pacta_in_node, sem);
            return;

        default:
            return;
    }
}

static void sem_register_global_rituales_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_RITUALE: {
            cct_sem_symbol_t *sym = sem_define_symbol(sem, CCT_SEM_SYMBOL_RITUALE,
                                                      node->as.rituale.name, node->line, node->column);
            if (!sym) return;

            sem_validate_rituale_genus_constraints_10d(sem, node);

            size_t tp_mark = sem_active_type_params_push_from_list(sem, node->as.rituale.type_params, node);

            cct_sem_type_t *ret = &sem->type_nihil;
            if (node->as.rituale.return_type) {
                ret = sem_resolve_ast_type(sem, node->as.rituale.return_type, node->line, node->column);
            }

            cct_sem_signature_tmp_t tmp = sem_resolve_signature_tmp(sem, node);
            sem_active_type_params_pop_to(sem, tp_mark);
            sym->return_type = ret;
            sym->param_count = tmp.count;
            sym->param_types = tmp.types;
            sym->rituale_decl = node;
            sym->type_param_count = node->as.rituale.type_params ? node->as.rituale.type_params->count : 0;
            if (sym->type_param_count > 0) {
                sym->type_param_names = (char**)sem_calloc(sym->type_param_count, sizeof(char*));
                sym->type_param_constraint_pactum_names = (char**)sem_calloc(sym->type_param_count, sizeof(char*));
                for (size_t i = 0; i < sym->type_param_count; i++) {
                    cct_ast_type_param_t *tp = node->as.rituale.type_params->params[i];
                    sym->type_param_names[i] = sem_strdup((tp && tp->name) ? tp->name : "");
                    sym->type_param_constraint_pactum_names[i] = sem_strdup(
                        (tp && tp->constraint_pactum_name) ? tp->constraint_pactum_name : ""
                    );
                }
            }
            return;
        }

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_register_global_rituales_in_node, sem);
            return;

        default:
            return;
    }
}

static void sem_verify_sigillum_conformance_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_SIGILLUM:
            sem_validate_sigillum_pactum_conformance(sem, node);
            return;

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_verify_sigillum_conformance_in_node, sem);
            return;

        default:
            return;
    }
}

static void sem_analyze_rituale_body(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    cct_sem_symbol_t *fn = sem_find_function(sem, node->as.rituale.name);
    if (!fn) {
        sem_report_nodef(sem, node, "internal error: rituale '%s' missing from global table", node->as.rituale.name);
        return;
    }

    cct_sem_symbol_t *prev_fn = sem->current_rituale;
    sem->current_rituale = fn;

    size_t tp_mark = sem_active_type_params_push_from_list(sem, node->as.rituale.type_params, node);

    sem_push_scope(sem, CCT_SEM_SCOPE_RITUALE);

    cct_ast_param_list_t *params = node->as.rituale.params;
    if (params) {
        for (size_t i = 0; i < params->count; i++) {
            cct_ast_param_t *param = params->params[i];
            cct_sem_type_t *ptype = sem_resolve_ast_type(sem, param->type, param->line, param->column);
            cct_sem_symbol_t *psym = sem_define_symbol(sem, CCT_SEM_SYMBOL_PARAMETER,
                                                       param->name, param->line, param->column);
            if (psym) psym->type = ptype;
        }
    }

    if (node->as.rituale.body) {
        sem_analyze_block(sem, node->as.rituale.body, false);
    }

    sem_pop_scope(sem);
    sem_active_type_params_pop_to(sem, tp_mark);
    sem->current_rituale = prev_fn;
}

static void sem_analyze_global_bodies_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_RITUALE:
            sem_analyze_rituale_body(sem, node);
            return;

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_analyze_global_bodies_in_node, sem);
            return;

        default:
            return;
    }
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void cct_semantic_init(cct_semantic_analyzer_t *sem, const char *filename) {
    memset(sem, 0, sizeof(*sem));
    sem->filename = filename;
    sem_init_builtin_types(sem);

    sem_push_scope(sem, CCT_SEM_SCOPE_GLOBAL);
}

bool cct_semantic_analyze_program(cct_semantic_analyzer_t *sem, const cct_ast_program_t *program) {
    if (!sem || !program) return false;

    /* Pass 1A: register named types that may be referenced by ritual signatures. */
    sem_walk_node_list(program->declarations, sem_register_named_types_in_node, sem);

    /* Pass 1B: register contracts (PACTUM) globally. */
    sem_walk_node_list(program->declarations, sem_register_global_pacta_in_node, sem);

    /* Pass 1C: register rituals (functions) globally. */
    sem_walk_node_list(program->declarations, sem_register_global_rituales_in_node, sem);

    /* Pass 2: validate explicit SIGILLUM -> PACTUM conformance. */
    sem_walk_node_list(program->declarations, sem_verify_sigillum_conformance_in_node, sem);

    /* Pass 3: analyze ritual bodies/statements/expressions. */
    sem_walk_node_list(program->declarations, sem_analyze_global_bodies_in_node, sem);

    return !sem->had_error;
}

bool cct_semantic_had_error(const cct_semantic_analyzer_t *sem) {
    return sem ? sem->had_error : true;
}

u32 cct_semantic_error_count(const cct_semantic_analyzer_t *sem) {
    return sem ? sem->error_count : 0;
}

void cct_semantic_dispose(cct_semantic_analyzer_t *sem) {
    if (!sem) return;

    cct_sem_symbol_alloc_t *salloc = sem->symbol_alloc_list;
    while (salloc) {
        cct_sem_symbol_alloc_t *next = salloc->next;
        if (salloc->symbol) {
            free(salloc->symbol->name);
            free(salloc->symbol->param_types);
            if (salloc->symbol->type_param_names) {
                for (size_t i = 0; i < salloc->symbol->type_param_count; i++) {
                    free(salloc->symbol->type_param_names[i]);
                }
                free(salloc->symbol->type_param_names);
            }
            if (salloc->symbol->type_param_constraint_pactum_names) {
                for (size_t i = 0; i < salloc->symbol->type_param_count; i++) {
                    free(salloc->symbol->type_param_constraint_pactum_names[i]);
                }
                free(salloc->symbol->type_param_constraint_pactum_names);
            }
            free(salloc->symbol);
        }
        free(salloc);
        salloc = next;
    }

    cct_sem_scope_alloc_t *scalloc = sem->scope_alloc_list;
    while (scalloc) {
        cct_sem_scope_alloc_t *next = scalloc->next;
        free(scalloc->scope);
        free(scalloc);
        scalloc = next;
    }

    cct_sem_type_alloc_t *talloc = sem->type_alloc_list;
    while (talloc) {
        cct_sem_type_alloc_t *next = talloc->next;
        if (talloc->type) {
            if ((talloc->type->kind == CCT_SEM_TYPE_NAMED ||
                 talloc->type->kind == CCT_SEM_TYPE_TYPE_PARAM) &&
                talloc->type->name) {
                free((char*)talloc->type->name);
            }
            free(talloc->type);
        }
        free(talloc);
        talloc = next;
    }

    cct_sem_generic_instance_t *ginst = sem->generic_instances;
    while (ginst) {
        cct_sem_generic_instance_t *next = ginst->next;
        free(ginst->name);
        free(ginst->type_args);
        free(ginst);
        ginst = next;
    }
    sem->generic_instances = NULL;

    if (sem->active_type_param_names) {
        for (size_t i = 0; i < sem->active_type_param_count; i++) {
            free(sem->active_type_param_names[i]);
        }
        free(sem->active_type_param_names);
    }

    memset(sem, 0, sizeof(*sem));
}

const char* cct_sem_type_string(const cct_sem_type_t *type) {
    if (!type) return "(null)";
    if (type->name) return type->name;

    switch (type->kind) {
        case CCT_SEM_TYPE_ERROR: return "<error>";
        case CCT_SEM_TYPE_NIHIL: return "NIHIL";
        case CCT_SEM_TYPE_VERUM: return "VERUM";
        case CCT_SEM_TYPE_VERBUM: return "VERBUM";
        case CCT_SEM_TYPE_FRACTUM: return "FRACTUM";
        case CCT_SEM_TYPE_REX: return "REX";
        case CCT_SEM_TYPE_DUX: return "DUX";
        case CCT_SEM_TYPE_COMES: return "COMES";
        case CCT_SEM_TYPE_MILES: return "MILES";
        case CCT_SEM_TYPE_UMBRA: return "UMBRA";
        case CCT_SEM_TYPE_FLAMMA: return "FLAMMA";
        case CCT_SEM_TYPE_POINTER: return "SPECULUM";
        case CCT_SEM_TYPE_ARRAY: return "SERIES";
        case CCT_SEM_TYPE_NAMED: return "NAMED";
        case CCT_SEM_TYPE_TYPE_PARAM: return "TYPE_PARAM";
        default: return "UNKNOWN";
    }
}

const char* cct_sem_symbol_kind_string(cct_sem_symbol_kind_t kind) {
    switch (kind) {
        case CCT_SEM_SYMBOL_VARIABLE: return "variable";
        case CCT_SEM_SYMBOL_PARAMETER: return "parameter";
        case CCT_SEM_SYMBOL_RITUALE: return "rituale";
        case CCT_SEM_SYMBOL_TYPE: return "type";
        case CCT_SEM_SYMBOL_PACTUM: return "pactum";
        default: return "unknown";
    }
}
