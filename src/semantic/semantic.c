/*
 * CCT — Clavicula Turing
 * Semantic Analyzer Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "semantic.h"
#include "../common/fuzzy.h"
#include "../common/diagnostic.h"

#include <stdarg.h>
#include <ctype.h>
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
static bool sem_is_negative_shift_constant(const cct_ast_node_t *node);
static bool sem_profile_is_freestanding(const cct_semantic_analyzer_t *sem);
static void sem_warn_fpu_type_in_freestanding(
    cct_semantic_analyzer_t *sem,
    const cct_sem_type_t *type,
    u32 line,
    u32 col
);
static bool sem_is_lvalue_node_supported(const cct_ast_node_t *node);
static cct_sem_type_t* sem_analyze_lvalue(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node);

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

static void sem_warn_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node, const char *message) {
    if (!node || !message) return;
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_WARNING,
        .message = message,
        .file_path = sem ? sem->filename : NULL,
        .line = node->line,
        .column = node->column,
        .suggestion = "shift count should be non-negative; behavior is platform-defined for invalid counts",
        .code_label = "Semantic warning",
    };
    cct_diagnostic_emit(&diag);
}

static void sem_warn_at(
    cct_semantic_analyzer_t *sem,
    u32 line,
    u32 col,
    const char *message,
    const char *suggestion
) {
    if (!message) return;
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_WARNING,
        .message = message,
        .file_path = sem ? sem->filename : NULL,
        .line = line,
        .column = col,
        .suggestion = suggestion,
        .code_label = "Semantic warning",
    };
    cct_diagnostic_emit(&diag);
}

static bool sem_is_negative_shift_constant(const cct_ast_node_t *node) {
    if (!node) return false;
    if (node->type == AST_LITERAL_INT) {
        return node->as.literal_int.int_value < 0;
    }
    if (node->type == AST_UNARY_OP && node->as.unary_op.operator == TOKEN_MINUS) {
        const cct_ast_node_t *operand = node->as.unary_op.operand;
        return operand && operand->type == AST_LITERAL_INT && operand->as.literal_int.int_value > 0;
    }
    return false;
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
        strcmp(name, "char_at") == 0 ||
        strcmp(name, "from_char") == 0 ||
        strcmp(name, "contains") == 0) {
        return "add ADVOCARE \"cct/verbum.cct\" in the module header";
    }
    if (strcmp(name, "is_digit") == 0 ||
        strcmp(name, "is_alpha") == 0 ||
        strcmp(name, "is_whitespace") == 0) {
        return "add ADVOCARE \"cct/char.cct\" in the module header";
    }
    if (strcmp(name, "argc") == 0 ||
        strcmp(name, "arg") == 0) {
        return "add ADVOCARE \"cct/args.cct\" in the module header";
    }
    if (strcmp(name, "cursor_init") == 0 ||
        strcmp(name, "cursor_pos") == 0 ||
        strcmp(name, "cursor_eof") == 0 ||
        strcmp(name, "cursor_peek") == 0 ||
        strcmp(name, "cursor_next") == 0 ||
        strcmp(name, "cursor_free") == 0) {
        return "add ADVOCARE \"cct/verbum_scan.cct\" in the module header";
    }
    if (strcmp(name, "builder_init") == 0 ||
        strcmp(name, "builder_append") == 0 ||
        strcmp(name, "builder_append_char") == 0 ||
        strcmp(name, "builder_len") == 0 ||
        strcmp(name, "builder_to_verbum") == 0 ||
        strcmp(name, "builder_clear") == 0 ||
        strcmp(name, "builder_free") == 0) {
        return "add ADVOCARE \"cct/verbum_builder.cct\" in the module header";
    }
    if (strcmp(name, "writer_init") == 0 ||
        strcmp(name, "writer_indent") == 0 ||
        strcmp(name, "writer_dedent") == 0 ||
        strcmp(name, "writer_write") == 0 ||
        strcmp(name, "writer_writeln") == 0 ||
        strcmp(name, "writer_to_verbum") == 0 ||
        strcmp(name, "writer_free") == 0) {
        return "add ADVOCARE \"cct/code_writer.cct\" in the module header";
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
    if (strcmp(name, "run") == 0 ||
        strcmp(name, "run_capture") == 0 ||
        strcmp(name, "run_capture_err") == 0 ||
        strcmp(name, "run_with_input") == 0 ||
        strcmp(name, "run_env") == 0 ||
        strcmp(name, "run_timeout") == 0) {
        return "add ADVOCARE \"cct/process.cct\" in the module header";
    }
    if (strcmp(name, "djb2") == 0 ||
        strcmp(name, "fnv1a") == 0 ||
        strcmp(name, "fnv1a_bytes") == 0 ||
        strcmp(name, "crc32") == 0 ||
        strcmp(name, "murmur3") == 0 ||
        strcmp(name, "combine") == 0) {
        return "add ADVOCARE \"cct/hash.cct\" in the module header";
    }
    if (strcmp(name, "popcount") == 0 ||
        strcmp(name, "leading_zeros") == 0 ||
        strcmp(name, "trailing_zeros") == 0 ||
        strcmp(name, "rotate_left") == 0 ||
        strcmp(name, "rotate_right") == 0 ||
        strcmp(name, "next_power_of_2") == 0 ||
        strcmp(name, "is_power_of_2") == 0 ||
        strcmp(name, "byte_swap") == 0 ||
        strcmp(name, "bit_get") == 0 ||
        strcmp(name, "bit_set") == 0 ||
        strcmp(name, "bit_clear") == 0 ||
        strcmp(name, "bit_toggle") == 0 ||
        strcmp(name, "parity") == 0 ||
        strcmp(name, "bit_extract") == 0) {
        return "add ADVOCARE \"cct/bit.cct\" in the module header";
    }
    if (strcmp(name, "random_real_unit") == 0 ||
        strcmp(name, "random_bool") == 0 ||
        strcmp(name, "random_real_range") == 0 ||
        strcmp(name, "random_verbum") == 0 ||
        strcmp(name, "random_verbum_from") == 0 ||
        strcmp(name, "random_choice_int") == 0 ||
        strcmp(name, "shuffle_int") == 0 ||
        strcmp(name, "random_bytes") == 0) {
        return "add ADVOCARE \"cct/random.cct\" in the module header";
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
    if (strcmp(name, "callback_invoke0") == 0 ||
        strcmp(name, "callback_invoke1") == 0 ||
        strcmp(name, "callback_invoke2") == 0 ||
        strcmp(name, "callback_invoke3") == 0 ||
        strcmp(name, "callback_invoke4") == 0 ||
        strcmp(name, "callback_invoke0_void") == 0 ||
        strcmp(name, "callback_invoke1_void") == 0 ||
        strcmp(name, "callback_invoke2_void") == 0 ||
        strcmp(name, "callback_invoke3_void") == 0 ||
        strcmp(name, "callback_invoke4_void") == 0) {
        return "add ADVOCARE \"cct/callback.cct\" in the module header";
    }
    if (strcmp(name, "map_init") == 0 ||
        strcmp(name, "map_free") == 0 ||
        strcmp(name, "map_insert") == 0 ||
        strcmp(name, "map_remove") == 0 ||
        strcmp(name, "map_get") == 0 ||
        strcmp(name, "map_get_or_default") == 0 ||
        strcmp(name, "map_update_or_insert") == 0 ||
        strcmp(name, "map_contains") == 0 ||
        strcmp(name, "map_len") == 0 ||
        strcmp(name, "map_is_empty") == 0 ||
        strcmp(name, "map_capacity") == 0 ||
        strcmp(name, "map_clear") == 0 ||
        strcmp(name, "map_reserve") == 0 ||
        strcmp(name, "map_copy") == 0 ||
        strcmp(name, "map_keys") == 0 ||
        strcmp(name, "map_values") == 0 ||
        strcmp(name, "map_merge") == 0) {
        return "add ADVOCARE \"cct/map.cct\" in the module header";
    }
    if (strcmp(name, "set_init") == 0 ||
        strcmp(name, "set_free") == 0 ||
        strcmp(name, "set_insert") == 0 ||
        strcmp(name, "set_remove") == 0 ||
        strcmp(name, "set_contains") == 0 ||
        strcmp(name, "set_len") == 0 ||
        strcmp(name, "set_is_empty") == 0 ||
        strcmp(name, "set_clear") == 0 ||
        strcmp(name, "set_union") == 0 ||
        strcmp(name, "set_intersection") == 0 ||
        strcmp(name, "set_difference") == 0 ||
        strcmp(name, "set_symmetric_difference") == 0 ||
        strcmp(name, "set_is_subset") == 0 ||
        strcmp(name, "set_is_superset") == 0 ||
        strcmp(name, "set_equals") == 0 ||
        strcmp(name, "set_copy") == 0 ||
        strcmp(name, "set_to_fluxus") == 0 ||
        strcmp(name, "set_reserve") == 0 ||
        strcmp(name, "set_capacity") == 0) {
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

static bool sem_quando_types_compatible(const cct_sem_type_t *expr_type, const cct_sem_type_t *lit_type) {
    if (!expr_type || !lit_type) return false;
    if (sem_is_error_type(expr_type) || sem_is_error_type(lit_type)) return true;
    if (expr_type->kind == CCT_SEM_TYPE_VERBUM) return lit_type->kind == CCT_SEM_TYPE_VERBUM;
    if (sem_is_bool_type(expr_type)) return sem_is_bool_type(lit_type);
    if (sem_is_integer_type(expr_type)) return sem_is_integer_type(lit_type);
    return false;
}

static bool sem_is_molde_compatible_type(const cct_sem_type_t *type) {
    if (!type) return false;
    switch (type->kind) {
        case CCT_SEM_TYPE_REX:
        case CCT_SEM_TYPE_DUX:
        case CCT_SEM_TYPE_COMES:
        case CCT_SEM_TYPE_MILES:
        case CCT_SEM_TYPE_UMBRA:
        case CCT_SEM_TYPE_FLAMMA:
        case CCT_SEM_TYPE_VERBUM:
        case CCT_SEM_TYPE_VERUM:
            return true;
        default:
            return false;
    }
}

static bool sem_is_ordo_payload_supported_type(const cct_sem_type_t *type) {
    if (!type) return false;
    switch (type->kind) {
        case CCT_SEM_TYPE_REX:
        case CCT_SEM_TYPE_DUX:
        case CCT_SEM_TYPE_COMES:
        case CCT_SEM_TYPE_MILES:
        case CCT_SEM_TYPE_UMBRA:
        case CCT_SEM_TYPE_FLAMMA:
        case CCT_SEM_TYPE_VERBUM:
        case CCT_SEM_TYPE_VERUM:
            return true;
        default:
            return false;
    }
}

static bool sem_is_c_reserved_keyword(const char *name) {
    static const char *kwords[] = {
        "auto", "break", "case", "char", "const", "continue", "default",
        "do", "double", "else", "enum", "extern", "float", "for", "goto",
        "if", "inline", "int", "long", "register", "restrict", "return",
        "short", "signed", "sizeof", "static", "struct", "switch",
        "typedef", "union", "unsigned", "void", "volatile", "while",
        "_Bool", "_Complex", "_Imaginary"
    };
    if (!name || !name[0]) return false;
    for (size_t i = 0; i < sizeof(kwords) / sizeof(kwords[0]); i++) {
        if (strcmp(name, kwords[i]) == 0) return true;
    }
    return false;
}

typedef enum {
    CCT_SEM_MOLDE_FMT_INVALID = 0,
    CCT_SEM_MOLDE_FMT_INT,
    CCT_SEM_MOLDE_FMT_REAL,
    CCT_SEM_MOLDE_FMT_STR,
    CCT_SEM_MOLDE_FMT_ALIGN,
} cct_sem_molde_fmt_kind_t;

static cct_sem_molde_fmt_kind_t sem_classify_molde_fmt_spec(const char *spec) {
    if (!spec || !spec[0]) return CCT_SEM_MOLDE_FMT_INVALID;

    size_t len = strlen(spec);
    if (len >= 1 && (spec[0] == '<' || spec[0] == '>')) {
        if (len == 1) return CCT_SEM_MOLDE_FMT_INVALID;
        for (size_t i = 1; i < len; i++) {
            if (!isdigit((unsigned char)spec[i])) return CCT_SEM_MOLDE_FMT_INVALID;
        }
        return CCT_SEM_MOLDE_FMT_ALIGN;
    }

    size_t i = 0;
    while (i < len && isdigit((unsigned char)spec[i])) i++;

    bool saw_precision = false;
    if (i < len && spec[i] == '.') {
        saw_precision = true;
        i++;
        size_t prec_start = i;
        while (i < len && isdigit((unsigned char)spec[i])) i++;
        if (i == prec_start) return CCT_SEM_MOLDE_FMT_INVALID;
    }

    if (i + 1 != len) return CCT_SEM_MOLDE_FMT_INVALID;

    switch (spec[i]) {
        case 'd':
        case 'u':
        case 'x':
        case 'X':
            return saw_precision ? CCT_SEM_MOLDE_FMT_INVALID : CCT_SEM_MOLDE_FMT_INT;
        case 'f':
        case 'e':
        case 'g':
            return CCT_SEM_MOLDE_FMT_REAL;
        case 's':
            return saw_precision ? CCT_SEM_MOLDE_FMT_INVALID : CCT_SEM_MOLDE_FMT_STR;
        default:
            return CCT_SEM_MOLDE_FMT_INVALID;
    }
}

static void sem_validate_molde_fmt_spec(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *at,
    const cct_sem_type_t *type,
    const char *spec
) {
    if (!spec || !spec[0] || !at) return;

    cct_sem_molde_fmt_kind_t fmt_kind = sem_classify_molde_fmt_spec(spec);
    if (fmt_kind == CCT_SEM_MOLDE_FMT_INVALID) {
        sem_report_nodef(sem, at, "MOLDE: especificador de formato invalido: '%s'", spec);
        return;
    }

    bool is_int = sem_is_integer_type(type);
    bool is_real = sem_is_real_type(type);
    bool is_str = (type && type->kind == CCT_SEM_TYPE_VERBUM);

    if (fmt_kind == CCT_SEM_MOLDE_FMT_INT && !is_int) {
        sem_report_nodef(sem, at, "MOLDE: especificador '%s' requer tipo inteiro", spec);
        return;
    }
    if (fmt_kind == CCT_SEM_MOLDE_FMT_REAL && !is_real) {
        sem_report_nodef(sem, at, "MOLDE: especificador '%s' requer tipo real", spec);
        return;
    }
    if (fmt_kind == CCT_SEM_MOLDE_FMT_STR && !is_str) {
        sem_report_node(sem, at, "MOLDE: especificador 's' requer tipo VERBUM");
        return;
    }
    if (fmt_kind == CCT_SEM_MOLDE_FMT_ALIGN && !is_str) {
        sem_report_node(sem, at, "MOLDE: alinhamento requer tipo VERBUM");
    }
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
        if (sem_profile_is_freestanding(sem)) {
            sem_reportf(sem, line, col,
                        "GENUS/PACTUM dinâmico não suportado em perfil freestanding; use instanciação estática");
        } else {
            sem_reportf(sem, line, col,
                        "type argument '%s' is still parametric and outside subset 10B materialization",
                        cct_sem_type_string(resolved));
        }
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
        if (sem_profile_is_freestanding(sem) && ast_type->array_size == 0) {
            sem_reportf(sem, line, col,
                        "SERIES com tamanho dinâmico não suportado em perfil freestanding");
            return &sem->type_error;
        }
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

    if (target->kind == CCT_SEM_TYPE_POINTER && value->kind == CCT_SEM_TYPE_NIHIL) {
        return true;
    }

    if (target->kind == CCT_SEM_TYPE_POINTER && value->kind == CCT_SEM_TYPE_POINTER) {
        /* FASE 7A pragmatic rule: SPECULUM NIHIL behaves as generic alloc/free pointer. */
        if (target->element && target->element->kind == CCT_SEM_TYPE_NIHIL) return true;
        if (value->element && value->element->kind == CCT_SEM_TYPE_NIHIL) return true;
        return false;
    }

    if (target->kind == CCT_SEM_TYPE_MILES && sem_is_integer_type(value)) {
        return true;
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
        if (sem_profile_is_freestanding(sem) && ast_type->array_size == 0) {
            sem_reportf(sem, line, col,
                        "SERIES com tamanho dinâmico não suportado em perfil freestanding");
            return &sem->type_error;
        }
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
        sem_warn_fpu_type_in_freestanding(sem, builtin, line, col);
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
static cct_sem_builtin_spec_t specs[604];
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
        specs[112].name = "kernel_halt"; specs[112].min_args = 0; specs[112].variadic = false;
        specs[113].name = "kernel_outb"; specs[113].min_args = 2; specs[113].variadic = false;
        specs[114].name = "kernel_inb"; specs[114].min_args = 1; specs[114].variadic = false;
        specs[115].name = "kernel_memcpy"; specs[115].min_args = 3; specs[115].variadic = false;
        specs[116].name = "kernel_memset"; specs[116].min_args = 3; specs[116].variadic = false;
        specs[117].name = "verbum_char_at"; specs[117].min_args = 2; specs[117].variadic = false;
        specs[118].name = "verbum_from_char"; specs[118].min_args = 1; specs[118].variadic = false;
        specs[119].name = "char_is_digit"; specs[119].min_args = 1; specs[119].variadic = false;
        specs[120].name = "char_is_alpha"; specs[120].min_args = 1; specs[120].variadic = false;
        specs[121].name = "char_is_whitespace"; specs[121].min_args = 1; specs[121].variadic = false;
        specs[122].name = "args_argc"; specs[122].min_args = 0; specs[122].variadic = false;
        specs[123].name = "args_arg"; specs[123].min_args = 1; specs[123].variadic = false;
        specs[124].name = "scan_init"; specs[124].min_args = 1; specs[124].variadic = false;
        specs[125].name = "scan_pos"; specs[125].min_args = 1; specs[125].variadic = false;
        specs[126].name = "scan_eof"; specs[126].min_args = 1; specs[126].variadic = false;
        specs[127].name = "scan_peek"; specs[127].min_args = 1; specs[127].variadic = false;
        specs[128].name = "scan_next"; specs[128].min_args = 1; specs[128].variadic = false;
        specs[129].name = "scan_free"; specs[129].min_args = 1; specs[129].variadic = false;
        specs[130].name = "builder_init"; specs[130].min_args = 0; specs[130].variadic = false;
        specs[131].name = "builder_append"; specs[131].min_args = 2; specs[131].variadic = false;
        specs[132].name = "builder_append_char"; specs[132].min_args = 2; specs[132].variadic = false;
        specs[133].name = "builder_len"; specs[133].min_args = 1; specs[133].variadic = false;
        specs[134].name = "builder_to_verbum"; specs[134].min_args = 1; specs[134].variadic = false;
        specs[135].name = "builder_clear"; specs[135].min_args = 1; specs[135].variadic = false;
        specs[136].name = "builder_free"; specs[136].min_args = 1; specs[136].variadic = false;
        specs[137].name = "writer_init"; specs[137].min_args = 0; specs[137].variadic = false;
        specs[138].name = "writer_indent"; specs[138].min_args = 1; specs[138].variadic = false;
        specs[139].name = "writer_dedent"; specs[139].min_args = 1; specs[139].variadic = false;
        specs[140].name = "writer_write"; specs[140].min_args = 2; specs[140].variadic = false;
        specs[141].name = "writer_writeln"; specs[141].min_args = 2; specs[141].variadic = false;
        specs[142].name = "writer_to_verbum"; specs[142].min_args = 1; specs[142].variadic = false;
        specs[143].name = "writer_free"; specs[143].min_args = 1; specs[143].variadic = false;
        specs[144].name = "env_get"; specs[144].min_args = 1; specs[144].variadic = false;
        specs[145].name = "env_has"; specs[145].min_args = 1; specs[145].variadic = false;
        specs[146].name = "env_cwd"; specs[146].min_args = 0; specs[146].variadic = false;
        specs[147].name = "time_now_ms"; specs[147].min_args = 0; specs[147].variadic = false;
        specs[148].name = "time_now_ns"; specs[148].min_args = 0; specs[148].variadic = false;
        specs[149].name = "time_sleep_ms"; specs[149].min_args = 1; specs[149].variadic = false;
        specs[150].name = "bytes_new"; specs[150].min_args = 1; specs[150].variadic = false;
        specs[151].name = "bytes_len"; specs[151].min_args = 1; specs[151].variadic = false;
        specs[152].name = "bytes_get"; specs[152].min_args = 2; specs[152].variadic = false;
        specs[153].name = "bytes_set"; specs[153].min_args = 3; specs[153].variadic = false;
        specs[154].name = "bytes_free"; specs[154].min_args = 1; specs[154].variadic = false;
        specs[155].name = "verbum_starts_with"; specs[155].min_args = 2; specs[155].variadic = false;
        specs[156].name = "verbum_ends_with"; specs[156].min_args = 2; specs[156].variadic = false;
        specs[157].name = "verbum_strip_prefix"; specs[157].min_args = 2; specs[157].variadic = false;
        specs[158].name = "verbum_strip_suffix"; specs[158].min_args = 2; specs[158].variadic = false;
        specs[159].name = "verbum_replace"; specs[159].min_args = 3; specs[159].variadic = false;
        specs[160].name = "verbum_replace_all"; specs[160].min_args = 3; specs[160].variadic = false;
        specs[161].name = "verbum_to_upper"; specs[161].min_args = 1; specs[161].variadic = false;
        specs[162].name = "verbum_to_lower"; specs[162].min_args = 1; specs[162].variadic = false;
        specs[163].name = "verbum_trim_left"; specs[163].min_args = 1; specs[163].variadic = false;
        specs[164].name = "verbum_trim_right"; specs[164].min_args = 1; specs[164].variadic = false;
        specs[165].name = "verbum_trim_char"; specs[165].min_args = 2; specs[165].variadic = false;
        specs[166].name = "verbum_repeat"; specs[166].min_args = 2; specs[166].variadic = false;
        specs[167].name = "verbum_pad_left"; specs[167].min_args = 3; specs[167].variadic = false;
        specs[168].name = "verbum_pad_right"; specs[168].min_args = 3; specs[168].variadic = false;
        specs[169].name = "verbum_center"; specs[169].min_args = 3; specs[169].variadic = false;
        specs[170].name = "verbum_last_find"; specs[170].min_args = 2; specs[170].variadic = false;
        specs[171].name = "verbum_find_from"; specs[171].min_args = 3; specs[171].variadic = false;
        specs[172].name = "verbum_count_occurrences"; specs[172].min_args = 2; specs[172].variadic = false;
        specs[173].name = "verbum_reverse"; specs[173].min_args = 1; specs[173].variadic = false;
        specs[174].name = "verbum_equals_ignore_case"; specs[174].min_args = 2; specs[174].variadic = false;
        specs[175].name = "verbum_slice"; specs[175].min_args = 3; specs[175].variadic = false;
        specs[176].name = "verbum_is_ascii"; specs[176].min_args = 1; specs[176].variadic = false;
        specs[177].name = "fluxus_peek"; specs[177].min_args = 1; specs[177].variadic = false;
        specs[178].name = "fluxus_set"; specs[178].min_args = 3; specs[178].variadic = false;
        specs[179].name = "fluxus_remove"; specs[179].min_args = 2; specs[179].variadic = false;
        specs[180].name = "fluxus_insert"; specs[180].min_args = 3; specs[180].variadic = false;
        specs[181].name = "fluxus_contains"; specs[181].min_args = 2; specs[181].variadic = false;
        specs[182].name = "fluxus_reverse"; specs[182].min_args = 1; specs[182].variadic = false;
        specs[183].name = "fluxus_sort_int"; specs[183].min_args = 1; specs[183].variadic = false;
        specs[184].name = "fluxus_sort_verbum"; specs[184].min_args = 1; specs[184].variadic = false;
        specs[185].name = "fluxus_to_ptr"; specs[185].min_args = 1; specs[185].variadic = false;
        specs[186].name = "verbum_split"; specs[186].min_args = 2; specs[186].variadic = false;
        specs[187].name = "verbum_split_char"; specs[187].min_args = 2; specs[187].variadic = false;
        specs[188].name = "verbum_join"; specs[188].min_args = 2; specs[188].variadic = false;
        specs[189].name = "verbum_lines"; specs[189].min_args = 1; specs[189].variadic = false;
        specs[190].name = "verbum_words"; specs[190].min_args = 1; specs[190].variadic = false;
        specs[191].name = "fmt_stringify_int_hex"; specs[191].min_args = 1; specs[191].variadic = false;
        specs[192].name = "fmt_stringify_int_hex_upper"; specs[192].min_args = 1; specs[192].variadic = false;
        specs[193].name = "fmt_stringify_int_oct"; specs[193].min_args = 1; specs[193].variadic = false;
        specs[194].name = "fmt_stringify_int_bin"; specs[194].min_args = 1; specs[194].variadic = false;
        specs[195].name = "fmt_stringify_uint"; specs[195].min_args = 1; specs[195].variadic = false;
        specs[196].name = "fmt_stringify_int_padded"; specs[196].min_args = 3; specs[196].variadic = false;
        specs[197].name = "fmt_stringify_real_prec"; specs[197].min_args = 2; specs[197].variadic = false;
        specs[198].name = "fmt_stringify_real_sci"; specs[198].min_args = 1; specs[198].variadic = false;
        specs[199].name = "fmt_stringify_real_fixed"; specs[199].min_args = 2; specs[199].variadic = false;
        specs[200].name = "fmt_format_1"; specs[200].min_args = 2; specs[200].variadic = false;
        specs[201].name = "fmt_format_2"; specs[201].min_args = 3; specs[201].variadic = false;
        specs[202].name = "fmt_format_3"; specs[202].min_args = 4; specs[202].variadic = false;
        specs[203].name = "fmt_format_4"; specs[203].min_args = 5; specs[203].variadic = false;
        specs[204].name = "fmt_repeat_char"; specs[204].min_args = 2; specs[204].variadic = false;
        specs[205].name = "fmt_table_row"; specs[205].min_args = 3; specs[205].variadic = false;
        specs[206].name = "parse_try_int"; specs[206].min_args = 1; specs[206].variadic = false;
        specs[207].name = "parse_try_real"; specs[207].min_args = 1; specs[207].variadic = false;
        specs[208].name = "parse_try_bool"; specs[208].min_args = 1; specs[208].variadic = false;
        specs[209].name = "parse_int_hex"; specs[209].min_args = 1; specs[209].variadic = false;
        specs[210].name = "parse_try_int_hex"; specs[210].min_args = 1; specs[210].variadic = false;
        specs[211].name = "parse_int_radix"; specs[211].min_args = 2; specs[211].variadic = false;
        specs[212].name = "parse_try_int_radix"; specs[212].min_args = 2; specs[212].variadic = false;
        specs[213].name = "parse_is_int"; specs[213].min_args = 1; specs[213].variadic = false;
        specs[214].name = "parse_is_real"; specs[214].min_args = 1; specs[214].variadic = false;
        specs[215].name = "parse_csv_line"; specs[215].min_args = 2; specs[215].variadic = false;
        specs[216].name = "fs_mkdir"; specs[216].min_args = 1; specs[216].variadic = false;
        specs[217].name = "fs_mkdir_all"; specs[217].min_args = 1; specs[217].variadic = false;
        specs[218].name = "fs_delete_file"; specs[218].min_args = 1; specs[218].variadic = false;
        specs[219].name = "fs_delete_dir"; specs[219].min_args = 1; specs[219].variadic = false;
        specs[220].name = "fs_rename"; specs[220].min_args = 2; specs[220].variadic = false;
        specs[221].name = "fs_copy"; specs[221].min_args = 2; specs[221].variadic = false;
        specs[222].name = "fs_move"; specs[222].min_args = 2; specs[222].variadic = false;
        specs[223].name = "fs_is_file"; specs[223].min_args = 1; specs[223].variadic = false;
        specs[224].name = "fs_is_dir"; specs[224].min_args = 1; specs[224].variadic = false;
        specs[225].name = "fs_is_symlink"; specs[225].min_args = 1; specs[225].variadic = false;
        specs[226].name = "fs_is_readable"; specs[226].min_args = 1; specs[226].variadic = false;
        specs[227].name = "fs_is_writable"; specs[227].min_args = 1; specs[227].variadic = false;
        specs[228].name = "fs_modified_time"; specs[228].min_args = 1; specs[228].variadic = false;
        specs[229].name = "fs_chmod"; specs[229].min_args = 2; specs[229].variadic = false;
        specs[230].name = "fs_list_dir"; specs[230].min_args = 1; specs[230].variadic = false;
        specs[231].name = "fs_create_temp_file"; specs[231].min_args = 0; specs[231].variadic = false;
        specs[232].name = "fs_create_temp_dir"; specs[232].min_args = 0; specs[232].variadic = false;
        specs[233].name = "fs_truncate"; specs[233].min_args = 2; specs[233].variadic = false;
        specs[234].name = "fs_symlink"; specs[234].min_args = 2; specs[234].variadic = false;
        specs[235].name = "fs_same_file"; specs[235].min_args = 2; specs[235].variadic = false;
        specs[236].name = "io_print_real"; specs[236].min_args = 1; specs[236].variadic = false;
        specs[237].name = "io_print_char"; specs[237].min_args = 1; specs[237].variadic = false;
        specs[238].name = "io_eprint"; specs[238].min_args = 1; specs[238].variadic = false;
        specs[239].name = "io_eprintln"; specs[239].min_args = 1; specs[239].variadic = false;
        specs[240].name = "io_eprint_int"; specs[240].min_args = 1; specs[240].variadic = false;
        specs[241].name = "io_eprint_real"; specs[241].min_args = 1; specs[241].variadic = false;
        specs[242].name = "io_flush"; specs[242].min_args = 0; specs[242].variadic = false;
        specs[243].name = "io_flush_err"; specs[243].min_args = 0; specs[243].variadic = false;
        specs[244].name = "io_read_char"; specs[244].min_args = 0; specs[244].variadic = false;
        specs[245].name = "io_is_tty"; specs[245].min_args = 0; specs[245].variadic = false;
        specs[246].name = "path_stem"; specs[246].min_args = 1; specs[246].variadic = false;
        specs[247].name = "path_normalize"; specs[247].min_args = 1; specs[247].variadic = false;
        specs[248].name = "path_is_absolute"; specs[248].min_args = 1; specs[248].variadic = false;
        specs[249].name = "path_resolve"; specs[249].min_args = 1; specs[249].variadic = false;
        specs[250].name = "path_relative_to"; specs[250].min_args = 2; specs[250].variadic = false;
        specs[251].name = "path_with_ext"; specs[251].min_args = 2; specs[251].variadic = false;
        specs[252].name = "path_without_ext"; specs[252].min_args = 1; specs[252].variadic = false;
        specs[253].name = "path_home_dir"; specs[253].min_args = 0; specs[253].variadic = false;
        specs[254].name = "path_temp_dir"; specs[254].min_args = 0; specs[254].variadic = false;
        specs[255].name = "path_split"; specs[255].min_args = 1; specs[255].variadic = false;
        specs[256].name = "set_union"; specs[256].min_args = 3; specs[256].variadic = false;
        specs[257].name = "set_intersection"; specs[257].min_args = 3; specs[257].variadic = false;
        specs[258].name = "set_difference"; specs[258].min_args = 3; specs[258].variadic = false;
        specs[259].name = "set_symmetric_difference"; specs[259].min_args = 3; specs[259].variadic = false;
        specs[260].name = "set_is_subset"; specs[260].min_args = 3; specs[260].variadic = false;
        specs[261].name = "set_equals"; specs[261].min_args = 3; specs[261].variadic = false;
        specs[262].name = "set_copy"; specs[262].min_args = 2; specs[262].variadic = false;
        specs[263].name = "set_to_fluxus"; specs[263].min_args = 2; specs[263].variadic = false;
        specs[264].name = "set_reserve"; specs[264].min_args = 2; specs[264].variadic = false;
        specs[265].name = "set_capacity"; specs[265].min_args = 1; specs[265].variadic = false;
        specs[266].name = "map_copy"; specs[266].min_args = 1; specs[266].variadic = false;
        specs[267].name = "map_keys"; specs[267].min_args = 2; specs[267].variadic = false;
        specs[268].name = "map_values"; specs[268].min_args = 2; specs[268].variadic = false;
        specs[269].name = "map_merge"; specs[269].min_args = 2; specs[269].variadic = false;
        specs[270].name = "alg_sort_verbum"; specs[270].min_args = 2; specs[270].variadic = false;
        specs[271].name = "process_run"; specs[271].min_args = 1; specs[271].variadic = false;
        specs[272].name = "process_run_capture"; specs[272].min_args = 1; specs[272].variadic = false;
        specs[273].name = "process_run_with_input"; specs[273].min_args = 2; specs[273].variadic = false;
        specs[274].name = "process_run_env"; specs[274].min_args = 2; specs[274].variadic = false;
        specs[275].name = "process_run_timeout"; specs[275].min_args = 2; specs[275].variadic = false;
        specs[276].name = "hash_fnv1a_bytes"; specs[276].min_args = 2; specs[276].variadic = false;
        specs[277].name = "hash_crc32"; specs[277].min_args = 1; specs[277].variadic = false;
        specs[278].name = "hash_murmur3"; specs[278].min_args = 2; specs[278].variadic = false;
        specs[279].name = "random_verbum"; specs[279].min_args = 1; specs[279].variadic = false;
        specs[280].name = "random_verbum_from"; specs[280].min_args = 2; specs[280].variadic = false;
        specs[281].name = "random_shuffle_int"; specs[281].min_args = 2; specs[281].variadic = false;
        specs[282].name = "random_bytes"; specs[282].min_args = 1; specs[282].variadic = false;
        specs[283].name = "json_arr_handle_new"; specs[283].min_args = 1; specs[283].variadic = false;
        specs[284].name = "json_arr_handle_push"; specs[284].min_args = 2; specs[284].variadic = false;
        specs[285].name = "json_arr_handle_len"; specs[285].min_args = 1; specs[285].variadic = false;
        specs[286].name = "json_arr_handle_get"; specs[286].min_args = 2; specs[286].variadic = false;
        specs[287].name = "json_obj_handle_new"; specs[287].min_args = 1; specs[287].variadic = false;
        specs[288].name = "json_obj_handle_push"; specs[288].min_args = 2; specs[288].variadic = false;
        specs[289].name = "json_obj_handle_len"; specs[289].min_args = 1; specs[289].variadic = false;
        specs[290].name = "json_obj_handle_get"; specs[290].min_args = 2; specs[290].variadic = false;
        specs[291].name = "fractum_to_verbum"; specs[291].min_args = 1; specs[291].variadic = false;
        specs[292].name = "json_handle_ptr"; specs[292].min_args = 1; specs[292].variadic = false;
        specs[293].name = "socket_tcp"; specs[293].min_args = 0; specs[293].variadic = false;
        specs[294].name = "socket_udp"; specs[294].min_args = 0; specs[294].variadic = false;
        specs[295].name = "sock_connect"; specs[295].min_args = 3; specs[295].variadic = false;
        specs[296].name = "sock_bind"; specs[296].min_args = 3; specs[296].variadic = false;
        specs[297].name = "sock_listen"; specs[297].min_args = 2; specs[297].variadic = false;
        specs[298].name = "sock_accept"; specs[298].min_args = 1; specs[298].variadic = false;
        specs[299].name = "sock_send"; specs[299].min_args = 2; specs[299].variadic = false;
        specs[300].name = "sock_recv"; specs[300].min_args = 2; specs[300].variadic = false;
        specs[301].name = "sock_close"; specs[301].min_args = 1; specs[301].variadic = false;
        specs[302].name = "sock_set_timeout_ms"; specs[302].min_args = 2; specs[302].variadic = false;
        specs[303].name = "sock_peer_addr"; specs[303].min_args = 1; specs[303].variadic = false;
        specs[304].name = "sock_local_addr"; specs[304].min_args = 1; specs[304].variadic = false;
        specs[305].name = "sock_last_error"; specs[305].min_args = 0; specs[305].variadic = false;
        specs[306].name = "db_open"; specs[306].min_args = 1; specs[306].variadic = false;
        specs[307].name = "db_close"; specs[307].min_args = 1; specs[307].variadic = false;
        specs[308].name = "db_exec"; specs[308].min_args = 2; specs[308].variadic = false;
        specs[309].name = "db_last_error"; specs[309].min_args = 1; specs[309].variadic = false;
        specs[310].name = "db_query"; specs[310].min_args = 2; specs[310].variadic = false;
        specs[311].name = "rows_next"; specs[311].min_args = 1; specs[311].variadic = false;
        specs[312].name = "rows_get_text"; specs[312].min_args = 2; specs[312].variadic = false;
        specs[313].name = "rows_get_int"; specs[313].min_args = 2; specs[313].variadic = false;
        specs[314].name = "rows_get_real"; specs[314].min_args = 2; specs[314].variadic = false;
        specs[315].name = "rows_close"; specs[315].min_args = 1; specs[315].variadic = false;
        specs[316].name = "db_prepare"; specs[316].min_args = 2; specs[316].variadic = false;
        specs[317].name = "stmt_bind_text"; specs[317].min_args = 3; specs[317].variadic = false;
        specs[318].name = "stmt_bind_int"; specs[318].min_args = 3; specs[318].variadic = false;
        specs[319].name = "stmt_bind_real"; specs[319].min_args = 3; specs[319].variadic = false;
        specs[320].name = "stmt_step"; specs[320].min_args = 1; specs[320].variadic = false;
        specs[321].name = "stmt_reset"; specs[321].min_args = 1; specs[321].variadic = false;
        specs[322].name = "stmt_finalize"; specs[322].min_args = 1; specs[322].variadic = false;
        specs[323].name = "db_begin"; specs[323].min_args = 1; specs[323].variadic = false;
        specs[324].name = "db_commit"; specs[324].min_args = 1; specs[324].variadic = false;
        specs[325].name = "db_rollback"; specs[325].min_args = 1; specs[325].variadic = false;
        specs[326].name = "db_scalar_int"; specs[326].min_args = 2; specs[326].variadic = false;
        specs[327].name = "db_scalar_text"; specs[327].min_args = 2; specs[327].variadic = false;
        specs[328].name = "crypto_sha256_text"; specs[328].min_args = 1; specs[328].variadic = false;
        specs[329].name = "crypto_sha256_bytes"; specs[329].min_args = 2; specs[329].variadic = false;
        specs[330].name = "crypto_sha512_text"; specs[330].min_args = 1; specs[330].variadic = false;
        specs[331].name = "crypto_sha512_bytes"; specs[331].min_args = 2; specs[331].variadic = false;
        specs[332].name = "crypto_hmac_sha256"; specs[332].min_args = 2; specs[332].variadic = false;
        specs[333].name = "crypto_hmac_sha512"; specs[333].min_args = 2; specs[333].variadic = false;
        specs[334].name = "crypto_pbkdf2_sha256"; specs[334].min_args = 4; specs[334].variadic = false;
        specs[335].name = "crypto_csprng_bytes"; specs[335].min_args = 1; specs[335].variadic = false;
        specs[336].name = "crypto_constant_time_compare"; specs[336].min_args = 2; specs[336].variadic = false;
        specs[337].name = "regex_builtin_compile"; specs[337].min_args = 2; specs[337].variadic = false;
        specs[338].name = "regex_builtin_match"; specs[338].min_args = 2; specs[338].variadic = false;
        specs[339].name = "regex_builtin_search"; specs[339].min_args = 2; specs[339].variadic = false;
        specs[340].name = "regex_builtin_find_all"; specs[340].min_args = 2; specs[340].variadic = false;
        specs[341].name = "regex_builtin_replace"; specs[341].min_args = 4; specs[341].variadic = false;
        specs[342].name = "regex_builtin_split"; specs[342].min_args = 2; specs[342].variadic = false;
        specs[343].name = "regex_builtin_free"; specs[343].min_args = 1; specs[343].variadic = false;
        specs[344].name = "regex_builtin_last_error"; specs[344].min_args = 0; specs[344].variadic = false;
        specs[345].name = "date_now_unix"; specs[345].min_args = 0; specs[345].variadic = false;
        specs[346].name = "toml_builtin_parse"; specs[346].min_args = 1; specs[346].variadic = false;
        specs[347].name = "toml_builtin_parse_file"; specs[347].min_args = 1; specs[347].variadic = false;
        specs[348].name = "toml_builtin_last_error"; specs[348].min_args = 0; specs[348].variadic = false;
        specs[349].name = "toml_builtin_type"; specs[349].min_args = 2; specs[349].variadic = false;
        specs[350].name = "toml_builtin_get_string"; specs[350].min_args = 2; specs[350].variadic = false;
        specs[351].name = "toml_builtin_get_int"; specs[351].min_args = 2; specs[351].variadic = false;
        specs[352].name = "toml_builtin_get_real"; specs[352].min_args = 2; specs[352].variadic = false;
        specs[353].name = "toml_builtin_get_bool"; specs[353].min_args = 2; specs[353].variadic = false;
        specs[354].name = "toml_builtin_get_subdoc"; specs[354].min_args = 2; specs[354].variadic = false;
        specs[355].name = "toml_builtin_array_len"; specs[355].min_args = 2; specs[355].variadic = false;
        specs[356].name = "toml_builtin_array_item_string"; specs[356].min_args = 3; specs[356].variadic = false;
        specs[357].name = "toml_builtin_array_item_int"; specs[357].min_args = 3; specs[357].variadic = false;
        specs[358].name = "toml_builtin_array_item_real"; specs[358].min_args = 3; specs[358].variadic = false;
        specs[359].name = "toml_builtin_array_item_bool"; specs[359].min_args = 3; specs[359].variadic = false;
        specs[360].name = "toml_builtin_expand_env"; specs[360].min_args = 1; specs[360].variadic = false;
        specs[361].name = "toml_builtin_stringify"; specs[361].min_args = 1; specs[361].variadic = false;
        specs[362].name = "compress_builtin_gzip_compress_text"; specs[362].min_args = 1; specs[362].variadic = false;
        specs[363].name = "compress_builtin_gzip_compress_bytes"; specs[363].min_args = 2; specs[363].variadic = false;
        specs[364].name = "compress_builtin_gzip_decompress_text"; specs[364].min_args = 1; specs[364].variadic = false;
        specs[365].name = "compress_builtin_gzip_decompress_bytes"; specs[365].min_args = 1; specs[365].variadic = false;
        specs[366].name = "compress_builtin_last_error"; specs[366].min_args = 0; specs[366].variadic = false;
        specs[367].name = "filetype_builtin_detect_path"; specs[367].min_args = 1; specs[367].variadic = false;
        specs[368].name = "filetype_builtin_detect_bytes"; specs[368].min_args = 2; specs[368].variadic = false;
        specs[369].name = "image_builtin_load"; specs[369].min_args = 1; specs[369].variadic = false;
        specs[370].name = "image_builtin_free"; specs[370].min_args = 1; specs[370].variadic = false;
        specs[371].name = "image_builtin_save"; specs[371].min_args = 3; specs[371].variadic = false;
        specs[372].name = "image_builtin_resize"; specs[372].min_args = 4; specs[372].variadic = false;
        specs[373].name = "image_builtin_crop"; specs[373].min_args = 5; specs[373].variadic = false;
        specs[374].name = "image_builtin_rotate"; specs[374].min_args = 2; specs[374].variadic = false;
        specs[375].name = "image_builtin_convert"; specs[375].min_args = 2; specs[375].variadic = false;
        specs[376].name = "image_builtin_get_width"; specs[376].min_args = 1; specs[376].variadic = false;
        specs[377].name = "image_builtin_get_height"; specs[377].min_args = 1; specs[377].variadic = false;
        specs[378].name = "image_builtin_get_channels"; specs[378].min_args = 1; specs[378].variadic = false;
        specs[379].name = "image_builtin_get_format"; specs[379].min_args = 1; specs[379].variadic = false;
        specs[380].name = "image_builtin_last_error"; specs[380].min_args = 0; specs[380].variadic = false;
        specs[381].name = "fluxus_contains_verbum"; specs[381].min_args = 2; specs[381].variadic = false;
        specs[382].name = "gettext_builtin_catalog_new"; specs[382].min_args = 1; specs[382].variadic = false;
        specs[383].name = "gettext_builtin_catalog_add"; specs[383].min_args = 3; specs[383].variadic = false;
        specs[384].name = "gettext_builtin_catalog_add_plural"; specs[384].min_args = 5; specs[384].variadic = false;
        specs[385].name = "gettext_builtin_catalog_load"; specs[385].min_args = 2; specs[385].variadic = false;
        specs[386].name = "gettext_builtin_catalog_last_error"; specs[386].min_args = 0; specs[386].variadic = false;
        specs[387].name = "gettext_builtin_translate"; specs[387].min_args = 2; specs[387].variadic = false;
        specs[388].name = "gettext_builtin_translate_plural"; specs[388].min_args = 4; specs[388].variadic = false;
        specs[389].name = "gettext_builtin_default_set"; specs[389].min_args = 1; specs[389].variadic = false;
        specs[390].name = "gettext_builtin_default_translate"; specs[390].min_args = 1; specs[390].variadic = false;
        specs[391].name = "signal_builtin_is_supported"; specs[391].min_args = 0; specs[391].variadic = false;
        specs[392].name = "signal_builtin_install"; specs[392].min_args = 0; specs[392].variadic = false;
        specs[393].name = "signal_builtin_last_kind"; specs[393].min_args = 0; specs[393].variadic = false;
        specs[394].name = "signal_builtin_last_sequence"; specs[394].min_args = 0; specs[394].variadic = false;
        specs[395].name = "signal_builtin_last_unix_ms"; specs[395].min_args = 0; specs[395].variadic = false;
        specs[396].name = "signal_builtin_clear"; specs[396].min_args = 0; specs[396].variadic = false;
        specs[397].name = "signal_builtin_check_shutdown"; specs[397].min_args = 0; specs[397].variadic = false;
        specs[398].name = "signal_builtin_received_sigterm"; specs[398].min_args = 0; specs[398].variadic = false;
        specs[399].name = "signal_builtin_received_sigint"; specs[399].min_args = 0; specs[399].variadic = false;
        specs[400].name = "signal_builtin_received_sighup"; specs[400].min_args = 0; specs[400].variadic = false;
        specs[401].name = "signal_builtin_raise_self"; specs[401].min_args = 1; specs[401].variadic = false;
        specs[402].name = "pg_builtin_open"; specs[402].min_args = 1; specs[402].variadic = false;
        specs[403].name = "pg_builtin_close"; specs[403].min_args = 1; specs[403].variadic = false;
        specs[404].name = "pg_builtin_is_open"; specs[404].min_args = 1; specs[404].variadic = false;
        specs[405].name = "pg_builtin_last_error"; specs[405].min_args = 1; specs[405].variadic = false;
        specs[406].name = "pg_builtin_exec"; specs[406].min_args = 2; specs[406].variadic = false;
        specs[407].name = "pg_builtin_query"; specs[407].min_args = 2; specs[407].variadic = false;
        specs[408].name = "pg_builtin_rows_count"; specs[408].min_args = 1; specs[408].variadic = false;
        specs[409].name = "pg_builtin_rows_columns"; specs[409].min_args = 1; specs[409].variadic = false;
        specs[410].name = "pg_builtin_rows_next"; specs[410].min_args = 1; specs[410].variadic = false;
        specs[411].name = "pg_builtin_rows_get_text"; specs[411].min_args = 2; specs[411].variadic = false;
        specs[412].name = "pg_builtin_rows_is_null"; specs[412].min_args = 2; specs[412].variadic = false;
        specs[413].name = "pg_builtin_rows_close"; specs[413].min_args = 1; specs[413].variadic = false;
        specs[414].name = "pg_builtin_poll_channel"; specs[414].min_args = 2; specs[414].variadic = false;
        specs[415].name = "pg_builtin_poll_payload"; specs[415].min_args = 1; specs[415].variadic = false;
        specs[416].name = "mail_builtin_smtp_send"; specs[416].min_args = 10; specs[416].variadic = false;
        specs[417].name = "instr_builtin_enable"; specs[417].min_args = 1; specs[417].variadic = false;
        specs[418].name = "instr_builtin_disable"; specs[418].min_args = 0; specs[418].variadic = false;
        specs[419].name = "instr_builtin_is_enabled"; specs[419].min_args = 0; specs[419].variadic = false;
        specs[420].name = "instr_builtin_mode"; specs[420].min_args = 0; specs[420].variadic = false;
        specs[421].name = "instr_builtin_span_begin"; specs[421].min_args = 2; specs[421].variadic = false;
        specs[422].name = "instr_builtin_span_end"; specs[422].min_args = 1; specs[422].variadic = false;
        specs[423].name = "instr_builtin_span_attr"; specs[423].min_args = 3; specs[423].variadic = false;
        specs[424].name = "instr_builtin_event"; specs[424].min_args = 2; specs[424].variadic = false;
        specs[425].name = "instr_builtin_buffer_count"; specs[425].min_args = 0; specs[425].variadic = false;
        specs[426].name = "instr_builtin_buffer_clear"; specs[426].min_args = 0; specs[426].variadic = false;
        specs[427].name = "instr_builtin_buffer_span_id"; specs[427].min_args = 1; specs[427].variadic = false;
        specs[428].name = "instr_builtin_buffer_parent_id"; specs[428].min_args = 1; specs[428].variadic = false;
        specs[429].name = "instr_builtin_buffer_name"; specs[429].min_args = 1; specs[429].variadic = false;
        specs[430].name = "instr_builtin_buffer_category"; specs[430].min_args = 1; specs[430].variadic = false;
        specs[431].name = "instr_builtin_buffer_start_us"; specs[431].min_args = 1; specs[431].variadic = false;
        specs[432].name = "instr_builtin_buffer_end_us"; specs[432].min_args = 1; specs[432].variadic = false;
        specs[433].name = "instr_builtin_buffer_closed"; specs[433].min_args = 1; specs[433].variadic = false;
        specs[434].name = "instr_builtin_buffer_attr_count"; specs[434].min_args = 1; specs[434].variadic = false;
        specs[435].name = "instr_builtin_buffer_attr_key"; specs[435].min_args = 2; specs[435].variadic = false;
        specs[436].name = "instr_builtin_buffer_attr_value"; specs[436].min_args = 2; specs[436].variadic = false;
        specs[437].name = "instr_builtin_buffer_discard_closed"; specs[437].min_args = 0; specs[437].variadic = false;
        specs[438].name = "media_store_builtin_sha256_file"; specs[438].min_args = 1; specs[438].variadic = false;
        specs[439].name = "media_store_builtin_last_error"; specs[439].min_args = 0; specs[439].variadic = false;
        specs[440].name = "zip_builtin_last_error"; specs[440].min_args = 0; specs[440].variadic = false;
        specs[441].name = "zip_builtin_create"; specs[441].min_args = 1; specs[441].variadic = false;
        specs[442].name = "zip_builtin_open"; specs[442].min_args = 1; specs[442].variadic = false;
        specs[443].name = "zip_builtin_close"; specs[443].min_args = 1; specs[443].variadic = false;
        specs[444].name = "zip_builtin_add_file"; specs[444].min_args = 3; specs[444].variadic = false;
        specs[445].name = "zip_builtin_add_text"; specs[445].min_args = 3; specs[445].variadic = false;
        specs[446].name = "zip_builtin_entry_count"; specs[446].min_args = 1; specs[446].variadic = false;
        specs[447].name = "zip_builtin_entry_name"; specs[447].min_args = 2; specs[447].variadic = false;
        specs[448].name = "zip_builtin_entry_size"; specs[448].min_args = 2; specs[448].variadic = false;
        specs[449].name = "zip_builtin_entry_is_dir"; specs[449].min_args = 2; specs[449].variadic = false;
        specs[450].name = "zip_builtin_read_text"; specs[450].min_args = 2; specs[450].variadic = false;
        specs[451].name = "zip_builtin_extract_all"; specs[451].min_args = 3; specs[451].variadic = false;
        specs[452].name = "zip_builtin_extract_entry"; specs[452].min_args = 4; specs[452].variadic = false;
        specs[453].name = "obj_storage_builtin_last_error"; specs[453].min_args = 0; specs[453].variadic = false;
        specs[454].name = "obj_storage_builtin_last_content_type"; specs[454].min_args = 0; specs[454].variadic = false;
        specs[455].name = "obj_storage_builtin_last_etag"; specs[455].min_args = 0; specs[455].variadic = false;
        specs[456].name = "obj_storage_builtin_last_size"; specs[456].min_args = 0; specs[456].variadic = false;
        specs[457].name = "obj_storage_builtin_last_signed_expires_at"; specs[457].min_args = 0; specs[457].variadic = false;
        specs[458].name = "obj_storage_builtin_open"; specs[458].min_args = 6; specs[458].variadic = false;
        specs[459].name = "obj_storage_builtin_close"; specs[459].min_args = 1; specs[459].variadic = false;
        specs[460].name = "obj_storage_builtin_put_file"; specs[460].min_args = 4; specs[460].variadic = false;
        specs[461].name = "obj_storage_builtin_put_text"; specs[461].min_args = 4; specs[461].variadic = false;
        specs[462].name = "obj_storage_builtin_get_to_file"; specs[462].min_args = 3; specs[462].variadic = false;
        specs[463].name = "obj_storage_builtin_exists"; specs[463].min_args = 2; specs[463].variadic = false;
        specs[464].name = "obj_storage_builtin_head"; specs[464].min_args = 2; specs[464].variadic = false;
        specs[465].name = "obj_storage_builtin_delete"; specs[465].min_args = 2; specs[465].variadic = false;
        specs[466].name = "obj_storage_builtin_signed_url"; specs[466].min_args = 3; specs[466].variadic = false;
        specs[467].name = "callback_builtin_invoke0"; specs[467].min_args = 1; specs[467].variadic = false;
        specs[468].name = "callback_builtin_invoke1"; specs[468].min_args = 2; specs[468].variadic = false;
        specs[469].name = "callback_builtin_invoke2"; specs[469].min_args = 3; specs[469].variadic = false;
        specs[470].name = "callback_builtin_invoke3"; specs[470].min_args = 4; specs[470].variadic = false;
        specs[471].name = "callback_builtin_invoke4"; specs[471].min_args = 5; specs[471].variadic = false;
        specs[472].name = "callback_builtin_invoke0_void"; specs[472].min_args = 1; specs[472].variadic = false;
        specs[473].name = "callback_builtin_invoke1_void"; specs[473].min_args = 2; specs[473].variadic = false;
        specs[474].name = "callback_builtin_invoke2_void"; specs[474].min_args = 3; specs[474].variadic = false;
        specs[475].name = "callback_builtin_invoke3_void"; specs[475].min_args = 4; specs[475].variadic = false;
        specs[476].name = "callback_builtin_invoke4_void"; specs[476].min_args = 5; specs[476].variadic = false;
        specs[477].name = "result_unwrap_handle"; specs[477].min_args = 1; specs[477].variadic = false;
        specs[478].name = "stmt_has_row"; specs[478].min_args = 1; specs[478].variadic = false;
        specs[479].name = "stmt_get_text"; specs[479].min_args = 2; specs[479].variadic = false;
        specs[480].name = "stmt_get_int"; specs[480].min_args = 2; specs[480].variadic = false;
        specs[481].name = "stmt_get_real"; specs[481].min_args = 2; specs[481].variadic = false;
        specs[482].name = "console_init"; specs[482].min_args = 0; specs[482].variadic = false;
        specs[483].name = "console_clear"; specs[483].min_args = 0; specs[483].variadic = false;
        specs[484].name = "console_putc"; specs[484].min_args = 1; specs[484].variadic = false;
        specs[485].name = "console_write"; specs[485].min_args = 1; specs[485].variadic = false;
        specs[486].name = "console_write_centered"; specs[486].min_args = 1; specs[486].variadic = false;
        specs[487].name = "console_set_color"; specs[487].min_args = 2; specs[487].variadic = false;
        specs[488].name = "console_set_cursor"; specs[488].min_args = 2; specs[488].variadic = false;
        specs[489].name = "console_get_linha"; specs[489].min_args = 0; specs[489].variadic = false;
        specs[490].name = "console_get_coluna"; specs[490].min_args = 0; specs[490].variadic = false;
        specs[491].name = "cct_svc_alloc"; specs[491].min_args = 1; specs[491].variadic = false;
        specs[492].name = "cct_svc_realloc"; specs[492].min_args = 3; specs[492].variadic = false;
        specs[493].name = "cct_svc_free"; specs[493].min_args = 1; specs[493].variadic = false;
        specs[494].name = "cct_svc_memcpy"; specs[494].min_args = 3; specs[494].variadic = false;
        specs[495].name = "cct_svc_memset"; specs[495].min_args = 3; specs[495].variadic = false;
        specs[496].name = "cct_svc_heap_available"; specs[496].min_args = 0; specs[496].variadic = false;
        specs[497].name = "cct_svc_heap_allocated"; specs[497].min_args = 0; specs[497].variadic = false;
        specs[498].name = "cct_svc_heap_total"; specs[498].min_args = 0; specs[498].variadic = false;
        specs[499].name = "cct_svc_heap_base_addr"; specs[499].min_args = 0; specs[499].variadic = false;
        specs[500].name = "cct_svc_heap_alloc_count"; specs[500].min_args = 0; specs[500].variadic = false;
        specs[501].name = "cct_svc_byte_at"; specs[501].min_args = 2; specs[501].variadic = false;
        specs[502].name = "cct_svc_verbum_byte"; specs[502].min_args = 2; specs[502].variadic = false;
        specs[503].name = "cct_svc_verbum_len"; specs[503].min_args = 1; specs[503].variadic = false;
        specs[504].name = "cct_svc_verbum_copy_slice"; specs[504].min_args = 3; specs[504].variadic = false;
        specs[505].name = "cct_svc_builder_new"; specs[505].min_args = 0; specs[505].variadic = false;
        specs[506].name = "cct_svc_builder_append"; specs[506].min_args = 2; specs[506].variadic = false;
        specs[507].name = "cct_svc_builder_append_char"; specs[507].min_args = 2; specs[507].variadic = false;
        specs[508].name = "cct_svc_builder_build"; specs[508].min_args = 1; specs[508].variadic = false;
        specs[509].name = "cct_svc_builder_clear"; specs[509].min_args = 1; specs[509].variadic = false;
        specs[510].name = "cct_svc_builder_len"; specs[510].min_args = 1; specs[510].variadic = false;
        specs[511].name = "cct_svc_fluxus_new"; specs[511].min_args = 1; specs[511].variadic = false;
        specs[512].name = "cct_svc_fluxus_push"; specs[512].min_args = 2; specs[512].variadic = false;
        specs[513].name = "cct_svc_fluxus_pop"; specs[513].min_args = 2; specs[513].variadic = false;
        specs[514].name = "cct_svc_fluxus_get"; specs[514].min_args = 2; specs[514].variadic = false;
        specs[515].name = "cct_svc_fluxus_set"; specs[515].min_args = 3; specs[515].variadic = false;
        specs[516].name = "cct_svc_fluxus_clear"; specs[516].min_args = 1; specs[516].variadic = false;
        specs[517].name = "cct_svc_fluxus_reserve"; specs[517].min_args = 2; specs[517].variadic = false;
        specs[518].name = "cct_svc_fluxus_free"; specs[518].min_args = 1; specs[518].variadic = false;
        specs[519].name = "cct_svc_fluxus_len"; specs[519].min_args = 1; specs[519].variadic = false;
        specs[520].name = "cct_svc_fluxus_cap"; specs[520].min_args = 1; specs[520].variadic = false;
        specs[521].name = "cct_svc_fluxus_peek"; specs[521].min_args = 1; specs[521].variadic = false;
        specs[522].name = "cct_svc_irq_enable"; specs[522].min_args = 0; specs[522].variadic = false;
        specs[523].name = "cct_svc_irq_disable"; specs[523].min_args = 0; specs[523].variadic = false;
        specs[524].name = "cct_svc_irq_flags"; specs[524].min_args = 0; specs[524].variadic = false;
        specs[525].name = "cct_svc_irq_mask"; specs[525].min_args = 1; specs[525].variadic = false;
        specs[526].name = "cct_svc_irq_unmask"; specs[526].min_args = 1; specs[526].variadic = false;
        specs[527].name = "cct_svc_irq_register"; specs[527].min_args = 2; specs[527].variadic = false;
        specs[528].name = "cct_svc_irq_unregister"; specs[528].min_args = 1; specs[528].variadic = false;
        specs[529].name = "cct_svc_reboot"; specs[529].min_args = 0; specs[529].variadic = false;
        specs[530].name = "cct_svc_keyboard_getc"; specs[530].min_args = 0; specs[530].variadic = false;
        specs[531].name = "cct_svc_keyboard_poll"; specs[531].min_args = 0; specs[531].variadic = false;
        specs[532].name = "cct_svc_keyboard_available"; specs[532].min_args = 0; specs[532].variadic = false;
        specs[533].name = "cct_svc_keyboard_flush"; specs[533].min_args = 0; specs[533].variadic = false;
        specs[534].name = "cct_svc_keyboard_self_test"; specs[534].min_args = 0; specs[534].variadic = false;
        specs[535].name = "cct_svc_builder_new_raw"; specs[535].min_args = 0; specs[535].variadic = false;
        specs[536].name = "cct_svc_builder_backspace"; specs[536].min_args = 1; specs[536].variadic = false;
        specs[537].name = "cct_svc_timer_ms"; specs[537].min_args = 0; specs[537].variadic = false;
        specs[538].name = "cct_svc_timer_ticks"; specs[538].min_args = 0; specs[538].variadic = false;
        specs[539].name = "cct_svc_timer_sleep"; specs[539].min_args = 1; specs[539].variadic = false;
        specs[540].name = "cct_svc_fluxus_remove_first"; specs[540].min_args = 1; specs[540].variadic = false;
        specs[541].name = "cct_svc_outw"; specs[541].min_args = 2; specs[541].variadic = false;
        specs[542].name = "cct_svc_inw"; specs[542].min_args = 1; specs[542].variadic = false;
        specs[543].name = "cct_svc_outl"; specs[543].min_args = 2; specs[543].variadic = false;
        specs[544].name = "cct_svc_inl"; specs[544].min_args = 1; specs[544].variadic = false;
        specs[545].name = "cct_svc_pci_init"; specs[545].min_args = 0; specs[545].variadic = false;
        specs[546].name = "cct_svc_pci_count"; specs[546].min_args = 0; specs[546].variadic = false;
        specs[547].name = "cct_svc_pci_vendor"; specs[547].min_args = 1; specs[547].variadic = false;
        specs[548].name = "cct_svc_pci_device_id"; specs[548].min_args = 1; specs[548].variadic = false;
        specs[549].name = "cct_svc_pci_class"; specs[549].min_args = 1; specs[549].variadic = false;
        specs[550].name = "cct_svc_pci_bar0"; specs[550].min_args = 1; specs[550].variadic = false;
        specs[551].name = "cct_svc_pci_irq"; specs[551].min_args = 1; specs[551].variadic = false;
        specs[552].name = "cct_svc_pci_find"; specs[552].min_args = 2; specs[552].variadic = false;
        specs[553].name = "cct_svc_pci_enable_busmaster"; specs[553].min_args = 1; specs[553].variadic = false;
        specs[554].name = "cct_svc_net_init"; specs[554].min_args = 1; specs[554].variadic = false;
        specs[555].name = "cct_svc_net_send"; specs[555].min_args = 2; specs[555].variadic = false;
        specs[556].name = "cct_svc_net_recv"; specs[556].min_args = 2; specs[556].variadic = false;
        specs[557].name = "cct_svc_net_mac"; specs[557].min_args = 1; specs[557].variadic = false;
        specs[558].name = "cct_svc_net_poll"; specs[558].min_args = 0; specs[558].variadic = false;
        specs[559].name = "cct_svc_net_dispatch_init"; specs[559].min_args = 0; specs[559].variadic = false;
        specs[560].name = "cct_svc_tcp_init"; specs[560].min_args = 1; specs[560].variadic = false;
        specs[561].name = "cct_svc_tcp_accept"; specs[561].min_args = 0; specs[561].variadic = false;
        specs[562].name = "cct_svc_tcp_recv"; specs[562].min_args = 2; specs[562].variadic = false;
        specs[563].name = "cct_svc_tcp_send"; specs[563].min_args = 2; specs[563].variadic = false;
        specs[564].name = "cct_svc_tcp_close"; specs[564].min_args = 0; specs[564].variadic = false;
        specs[565].name = "cct_svc_tcp_state"; specs[565].min_args = 0; specs[565].variadic = false;
        specs[566].name = "cct_svc_http_server_init"; specs[566].min_args = 1; specs[566].variadic = false;
        specs[567].name = "cct_svc_http_server_accept"; specs[567].min_args = 1; specs[567].variadic = false;
        specs[568].name = "cct_svc_http_server_read"; specs[568].min_args = 2; specs[568].variadic = false;
        specs[569].name = "cct_svc_http_server_send"; specs[569].min_args = 2; specs[569].variadic = false;
        specs[570].name = "cct_svc_http_server_close"; specs[570].min_args = 0; specs[570].variadic = false;
        specs[571].name = "cct_svc_http_server_req_len"; specs[571].min_args = 0; specs[571].variadic = false;
        specs[572].name = "cct_svc_http_server_req_copy"; specs[572].min_args = 2; specs[572].variadic = false;
        specs[573].name = "cct_svc_http_server_req_count"; specs[573].min_args = 0; specs[573].variadic = false;
        specs[574].name = "cct_svc_http_parse"; specs[574].min_args = 2; specs[574].variadic = false;
        specs[575].name = "cct_svc_http_req_method"; specs[575].min_args = 2; specs[575].variadic = false;
        specs[576].name = "cct_svc_http_req_path"; specs[576].min_args = 2; specs[576].variadic = false;
        specs[577].name = "cct_svc_http_req_query"; specs[577].min_args = 2; specs[577].variadic = false;
        specs[578].name = "cct_svc_http_req_version"; specs[578].min_args = 2; specs[578].variadic = false;
        specs[579].name = "cct_svc_http_req_method_ptr"; specs[579].min_args = 0; specs[579].variadic = false;
        specs[580].name = "cct_svc_http_req_path_ptr"; specs[580].min_args = 0; specs[580].variadic = false;
        specs[581].name = "cct_svc_http_req_query_ptr"; specs[581].min_args = 0; specs[581].variadic = false;
        specs[582].name = "cct_svc_http_req_version_ptr"; specs[582].min_args = 0; specs[582].variadic = false;
        specs[583].name = "cct_svc_http_req_method_is"; specs[583].min_args = 1; specs[583].variadic = false;
        specs[584].name = "cct_svc_http_req_path_is"; specs[584].min_args = 1; specs[584].variadic = false;
        specs[585].name = "cct_svc_http_req_path_starts"; specs[585].min_args = 1; specs[585].variadic = false;
        specs[586].name = "cct_svc_http_req_header_count"; specs[586].min_args = 0; specs[586].variadic = false;
        specs[587].name = "cct_svc_http_req_header_name"; specs[587].min_args = 3; specs[587].variadic = false;
        specs[588].name = "cct_svc_http_req_header_value"; specs[588].min_args = 3; specs[588].variadic = false;
        specs[589].name = "cct_svc_http_req_header_name_ptr"; specs[589].min_args = 1; specs[589].variadic = false;
        specs[590].name = "cct_svc_http_req_header_value_ptr"; specs[590].min_args = 1; specs[590].variadic = false;
        specs[591].name = "cct_svc_http_req_body_len"; specs[591].min_args = 0; specs[591].variadic = false;
        specs[592].name = "cct_svc_http_req_body_copy"; specs[592].min_args = 2; specs[592].variadic = false;
        specs[593].name = "cct_svc_http_req_find_header"; specs[593].min_args = 1; specs[593].variadic = false;
        specs[594].name = "cct_svc_http_res_begin"; specs[594].min_args = 1; specs[594].variadic = false;
        specs[595].name = "cct_svc_http_res_header"; specs[595].min_args = 2; specs[595].variadic = false;
        specs[596].name = "cct_svc_http_res_finish"; specs[596].min_args = 3; specs[596].variadic = false;
        specs[597].name = "cct_svc_http_res_build"; specs[597].min_args = 4; specs[597].variadic = false;
        specs[598].name = "cct_svc_http_res_send"; specs[598].min_args = 0; specs[598].variadic = false;
        specs[599].name = "cct_svc_http_res_len"; specs[599].min_args = 0; specs[599].variadic = false;
        specs[600].name = "cct_svc_http_router_init"; specs[600].min_args = 0; specs[600].variadic = false;
        specs[601].name = "cct_svc_http_router_add"; specs[601].min_args = 4; specs[601].variadic = false;
        specs[602].name = "cct_svc_http_router_dispatch"; specs[602].min_args = 2; specs[602].variadic = false;
        specs[603].name = "cct_svc_http_router_set_404"; specs[603].min_args = 1; specs[603].variadic = false;
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
    specs[112].return_type = &sem->type_nihil;
    specs[113].return_type = &sem->type_nihil;
    specs[114].return_type = &sem->type_rex;
    specs[115].return_type = &sem->type_nihil;
    specs[116].return_type = &sem->type_nihil;
    specs[117].return_type = &sem->type_miles;
    specs[118].return_type = &sem->type_verbum;
    specs[119].return_type = &sem->type_verum;
    specs[120].return_type = &sem->type_verum;
    specs[121].return_type = &sem->type_verum;
    specs[122].return_type = &sem->type_rex;
    specs[123].return_type = &sem->type_verbum;
    specs[124].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[125].return_type = &sem->type_rex;
    specs[126].return_type = &sem->type_verum;
    specs[127].return_type = &sem->type_miles;
    specs[128].return_type = &sem->type_miles;
    specs[129].return_type = &sem->type_nihil;
    specs[130].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[131].return_type = &sem->type_nihil;
    specs[132].return_type = &sem->type_nihil;
    specs[133].return_type = &sem->type_rex;
    specs[134].return_type = &sem->type_verbum;
    specs[135].return_type = &sem->type_nihil;
    specs[136].return_type = &sem->type_nihil;
    specs[137].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[138].return_type = &sem->type_nihil;
    specs[139].return_type = &sem->type_nihil;
    specs[140].return_type = &sem->type_nihil;
    specs[141].return_type = &sem->type_nihil;
    specs[142].return_type = &sem->type_verbum;
    specs[143].return_type = &sem->type_nihil;
    specs[144].return_type = &sem->type_verbum;
    specs[145].return_type = &sem->type_verum;
    specs[146].return_type = &sem->type_verbum;
    specs[147].return_type = &sem->type_rex;
    specs[148].return_type = &sem->type_rex;
    specs[149].return_type = &sem->type_nihil;
    specs[150].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[151].return_type = &sem->type_rex;
    specs[152].return_type = &sem->type_miles;
    specs[153].return_type = &sem->type_nihil;
    specs[154].return_type = &sem->type_nihil;
    specs[155].return_type = &sem->type_verum;
    specs[156].return_type = &sem->type_verum;
    specs[157].return_type = &sem->type_verbum;
    specs[158].return_type = &sem->type_verbum;
    specs[159].return_type = &sem->type_verbum;
    specs[160].return_type = &sem->type_verbum;
    specs[161].return_type = &sem->type_verbum;
    specs[162].return_type = &sem->type_verbum;
    specs[163].return_type = &sem->type_verbum;
    specs[164].return_type = &sem->type_verbum;
    specs[165].return_type = &sem->type_verbum;
    specs[166].return_type = &sem->type_verbum;
    specs[167].return_type = &sem->type_verbum;
    specs[168].return_type = &sem->type_verbum;
    specs[169].return_type = &sem->type_verbum;
    specs[170].return_type = &sem->type_rex;
    specs[171].return_type = &sem->type_rex;
    specs[172].return_type = &sem->type_rex;
    specs[173].return_type = &sem->type_verbum;
    specs[174].return_type = &sem->type_verum;
    specs[175].return_type = &sem->type_verbum;
    specs[176].return_type = &sem->type_verum;
    specs[177].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[178].return_type = &sem->type_nihil;
    specs[179].return_type = &sem->type_nihil;
    specs[180].return_type = &sem->type_nihil;
    specs[181].return_type = &sem->type_verum;
    specs[182].return_type = &sem->type_nihil;
    specs[183].return_type = &sem->type_nihil;
    specs[184].return_type = &sem->type_nihil;
    specs[185].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[186].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[187].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[188].return_type = &sem->type_verbum;
    specs[189].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[190].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[191].return_type = &sem->type_verbum;
    specs[192].return_type = &sem->type_verbum;
    specs[193].return_type = &sem->type_verbum;
    specs[194].return_type = &sem->type_verbum;
    specs[195].return_type = &sem->type_verbum;
    specs[196].return_type = &sem->type_verbum;
    specs[197].return_type = &sem->type_verbum;
    specs[198].return_type = &sem->type_verbum;
    specs[199].return_type = &sem->type_verbum;
    specs[200].return_type = &sem->type_verbum;
    specs[201].return_type = &sem->type_verbum;
    specs[202].return_type = &sem->type_verbum;
    specs[203].return_type = &sem->type_verbum;
    specs[204].return_type = &sem->type_verbum;
    specs[205].return_type = &sem->type_verbum;
    specs[206].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[207].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[208].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[209].return_type = &sem->type_rex;
    specs[210].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[211].return_type = &sem->type_rex;
    specs[212].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[213].return_type = &sem->type_verum;
    specs[214].return_type = &sem->type_verum;
    specs[215].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[216].return_type = &sem->type_nihil;
    specs[217].return_type = &sem->type_nihil;
    specs[218].return_type = &sem->type_nihil;
    specs[219].return_type = &sem->type_nihil;
    specs[220].return_type = &sem->type_nihil;
    specs[221].return_type = &sem->type_nihil;
    specs[222].return_type = &sem->type_nihil;
    specs[223].return_type = &sem->type_verum;
    specs[224].return_type = &sem->type_verum;
    specs[225].return_type = &sem->type_verum;
    specs[226].return_type = &sem->type_verum;
    specs[227].return_type = &sem->type_verum;
    specs[228].return_type = &sem->type_rex;
    specs[229].return_type = &sem->type_nihil;
    specs[230].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[231].return_type = &sem->type_verbum;
    specs[232].return_type = &sem->type_verbum;
    specs[233].return_type = &sem->type_nihil;
    specs[234].return_type = &sem->type_nihil;
    specs[235].return_type = &sem->type_verum;
    specs[236].return_type = &sem->type_nihil;
    specs[237].return_type = &sem->type_nihil;
    specs[238].return_type = &sem->type_nihil;
    specs[239].return_type = &sem->type_nihil;
    specs[240].return_type = &sem->type_nihil;
    specs[241].return_type = &sem->type_nihil;
    specs[242].return_type = &sem->type_nihil;
    specs[243].return_type = &sem->type_nihil;
    specs[244].return_type = &sem->type_miles;
    specs[245].return_type = &sem->type_verum;
    specs[246].return_type = &sem->type_verbum;
    specs[247].return_type = &sem->type_verbum;
    specs[248].return_type = &sem->type_verum;
    specs[249].return_type = &sem->type_verbum;
    specs[250].return_type = &sem->type_verbum;
    specs[251].return_type = &sem->type_verbum;
    specs[252].return_type = &sem->type_verbum;
    specs[253].return_type = &sem->type_verbum;
    specs[254].return_type = &sem->type_verbum;
    specs[255].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[256].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[257].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[258].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[259].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[260].return_type = &sem->type_verum;
    specs[261].return_type = &sem->type_verum;
    specs[262].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[263].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[264].return_type = &sem->type_nihil;
    specs[265].return_type = &sem->type_rex;
    specs[266].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[267].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[268].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[269].return_type = &sem->type_nihil;
    specs[270].return_type = &sem->type_nihil;
    specs[271].return_type = &sem->type_rex;
    specs[272].return_type = &sem->type_verbum;
    specs[273].return_type = &sem->type_verbum;
    specs[274].return_type = &sem->type_rex;
    specs[275].return_type = &sem->type_rex;
    specs[276].return_type = &sem->type_rex;
    specs[277].return_type = &sem->type_rex;
    specs[278].return_type = &sem->type_rex;
    specs[279].return_type = &sem->type_verbum;
    specs[280].return_type = &sem->type_verbum;
    specs[281].return_type = &sem->type_nihil;
    specs[282].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[283].return_type = &sem->type_rex;
    specs[284].return_type = &sem->type_nihil;
    specs[285].return_type = &sem->type_rex;
    specs[286].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[287].return_type = &sem->type_rex;
    specs[288].return_type = &sem->type_nihil;
    specs[289].return_type = &sem->type_rex;
    specs[290].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[291].return_type = &sem->type_verbum;
    specs[292].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[293].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[294].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[295].return_type = &sem->type_nihil;
    specs[296].return_type = &sem->type_nihil;
    specs[297].return_type = &sem->type_nihil;
    specs[298].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[299].return_type = &sem->type_rex;
    specs[300].return_type = &sem->type_verbum;
    specs[301].return_type = &sem->type_nihil;
    specs[302].return_type = &sem->type_nihil;
    specs[303].return_type = &sem->type_verbum;
    specs[304].return_type = &sem->type_verbum;
    specs[305].return_type = &sem->type_verbum;
    specs[306].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[307].return_type = &sem->type_nihil;
    specs[308].return_type = &sem->type_nihil;
    specs[309].return_type = &sem->type_verbum;
    specs[310].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[311].return_type = &sem->type_verum;
    specs[312].return_type = &sem->type_verbum;
    specs[313].return_type = &sem->type_rex;
    specs[314].return_type = &sem->type_umbra;
    specs[315].return_type = &sem->type_nihil;
    specs[316].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[317].return_type = &sem->type_nihil;
    specs[318].return_type = &sem->type_nihil;
    specs[319].return_type = &sem->type_nihil;
    specs[320].return_type = &sem->type_verum;
    specs[321].return_type = &sem->type_nihil;
    specs[322].return_type = &sem->type_nihil;
    specs[323].return_type = &sem->type_nihil;
    specs[324].return_type = &sem->type_nihil;
    specs[325].return_type = &sem->type_nihil;
    specs[326].return_type = &sem->type_rex;
    specs[327].return_type = &sem->type_verbum;
    specs[328].return_type = &sem->type_verbum;
    specs[329].return_type = &sem->type_verbum;
    specs[330].return_type = &sem->type_verbum;
    specs[331].return_type = &sem->type_verbum;
    specs[332].return_type = &sem->type_verbum;
    specs[333].return_type = &sem->type_verbum;
    specs[334].return_type = &sem->type_verbum;
    specs[335].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[336].return_type = &sem->type_verum;
    specs[337].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[338].return_type = &sem->type_verum;
    specs[339].return_type = &sem->type_verbum;
    specs[340].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[341].return_type = &sem->type_verbum;
    specs[342].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[343].return_type = &sem->type_nihil;
    specs[344].return_type = &sem->type_verbum;
    specs[345].return_type = &sem->type_rex;
    specs[346].return_type = &sem->type_rex;
    specs[347].return_type = &sem->type_rex;
    specs[348].return_type = &sem->type_verbum;
    specs[349].return_type = &sem->type_rex;
    specs[350].return_type = &sem->type_verbum;
    specs[351].return_type = &sem->type_rex;
    specs[352].return_type = &sem->type_umbra;
    specs[353].return_type = &sem->type_verum;
    specs[354].return_type = &sem->type_rex;
    specs[355].return_type = &sem->type_rex;
    specs[356].return_type = &sem->type_verbum;
    specs[357].return_type = &sem->type_rex;
    specs[358].return_type = &sem->type_umbra;
    specs[359].return_type = &sem->type_verum;
    specs[360].return_type = &sem->type_rex;
    specs[361].return_type = &sem->type_verbum;
    specs[362].return_type = &sem->type_verbum;
    specs[363].return_type = &sem->type_verbum;
    specs[364].return_type = &sem->type_verbum;
    specs[365].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[366].return_type = &sem->type_verbum;
    specs[367].return_type = &sem->type_rex;
    specs[368].return_type = &sem->type_rex;
    specs[369].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[370].return_type = &sem->type_nihil;
    specs[371].return_type = &sem->type_rex;
    specs[372].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[373].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[374].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[375].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[376].return_type = &sem->type_rex;
    specs[377].return_type = &sem->type_rex;
    specs[378].return_type = &sem->type_rex;
    specs[379].return_type = &sem->type_rex;
    specs[380].return_type = &sem->type_verbum;
    specs[381].return_type = &sem->type_verum;
    specs[382].return_type = &sem->type_rex;
    specs[383].return_type = &sem->type_verum;
    specs[384].return_type = &sem->type_verum;
    specs[385].return_type = &sem->type_rex;
    specs[386].return_type = &sem->type_verbum;
    specs[387].return_type = &sem->type_verbum;
    specs[388].return_type = &sem->type_verbum;
    specs[389].return_type = &sem->type_verum;
    specs[390].return_type = &sem->type_verbum;
    specs[391].return_type = &sem->type_rex;
    specs[392].return_type = &sem->type_rex;
    specs[393].return_type = &sem->type_rex;
    specs[394].return_type = &sem->type_rex;
    specs[395].return_type = &sem->type_rex;
    specs[396].return_type = &sem->type_nihil;
    specs[397].return_type = &sem->type_rex;
    specs[398].return_type = &sem->type_rex;
    specs[399].return_type = &sem->type_rex;
    specs[400].return_type = &sem->type_rex;
    specs[401].return_type = &sem->type_rex;
    specs[402].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[403].return_type = &sem->type_nihil;
    specs[404].return_type = &sem->type_verum;
    specs[405].return_type = &sem->type_verbum;
    specs[406].return_type = &sem->type_nihil;
    specs[407].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[408].return_type = &sem->type_rex;
    specs[409].return_type = &sem->type_rex;
    specs[410].return_type = &sem->type_verum;
    specs[411].return_type = &sem->type_verbum;
    specs[412].return_type = &sem->type_verum;
    specs[413].return_type = &sem->type_nihil;
    specs[414].return_type = &sem->type_verbum;
    specs[415].return_type = &sem->type_verbum;
    specs[416].return_type = &sem->type_verbum;
    specs[417].return_type = &sem->type_nihil;
    specs[418].return_type = &sem->type_nihil;
    specs[419].return_type = &sem->type_rex;
    specs[420].return_type = &sem->type_rex;
    specs[421].return_type = &sem->type_rex;
    specs[422].return_type = &sem->type_nihil;
    specs[423].return_type = &sem->type_nihil;
    specs[424].return_type = &sem->type_nihil;
    specs[425].return_type = &sem->type_rex;
    specs[426].return_type = &sem->type_nihil;
    specs[427].return_type = &sem->type_rex;
    specs[428].return_type = &sem->type_rex;
    specs[429].return_type = &sem->type_verbum;
    specs[430].return_type = &sem->type_verbum;
    specs[431].return_type = &sem->type_rex;
    specs[432].return_type = &sem->type_rex;
    specs[433].return_type = &sem->type_rex;
    specs[434].return_type = &sem->type_rex;
    specs[435].return_type = &sem->type_verbum;
    specs[436].return_type = &sem->type_verbum;
    specs[437].return_type = &sem->type_nihil;
    specs[438].return_type = &sem->type_verbum;
    specs[439].return_type = &sem->type_verbum;
    specs[440].return_type = &sem->type_verbum;
    specs[441].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[442].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[443].return_type = &sem->type_nihil;
    specs[444].return_type = &sem->type_verum;
    specs[445].return_type = &sem->type_verum;
    specs[446].return_type = &sem->type_rex;
    specs[447].return_type = &sem->type_verbum;
    specs[448].return_type = &sem->type_rex;
    specs[449].return_type = &sem->type_verum;
    specs[450].return_type = &sem->type_verbum;
    specs[451].return_type = &sem->type_verum;
    specs[452].return_type = &sem->type_verum;
    specs[453].return_type = &sem->type_verbum;
    specs[454].return_type = &sem->type_verbum;
    specs[455].return_type = &sem->type_verbum;
    specs[456].return_type = &sem->type_rex;
    specs[457].return_type = &sem->type_verbum;
    specs[458].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[459].return_type = &sem->type_nihil;
    specs[460].return_type = &sem->type_verum;
    specs[461].return_type = &sem->type_verum;
    specs[462].return_type = &sem->type_verum;
    specs[463].return_type = &sem->type_rex;
    specs[464].return_type = &sem->type_verum;
    specs[465].return_type = &sem->type_verum;
    specs[466].return_type = &sem->type_verbum;
    specs[467].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[468].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[469].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[470].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[471].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[472].return_type = &sem->type_nihil;
    specs[473].return_type = &sem->type_nihil;
    specs[474].return_type = &sem->type_nihil;
    specs[475].return_type = &sem->type_nihil;
    specs[476].return_type = &sem->type_nihil;
    specs[477].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[478].return_type = &sem->type_verum;
    specs[479].return_type = &sem->type_verbum;
    specs[480].return_type = &sem->type_rex;
    specs[481].return_type = &sem->type_umbra;
    specs[482].return_type = &sem->type_nihil;
    specs[483].return_type = &sem->type_nihil;
    specs[484].return_type = &sem->type_nihil;
    specs[485].return_type = &sem->type_nihil;
    specs[486].return_type = &sem->type_nihil;
    specs[487].return_type = &sem->type_nihil;
    specs[488].return_type = &sem->type_nihil;
    specs[489].return_type = &sem->type_rex;
    specs[490].return_type = &sem->type_rex;
    specs[491].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[492].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[493].return_type = &sem->type_nihil;
    specs[494].return_type = &sem->type_nihil;
    specs[495].return_type = &sem->type_nihil;
    specs[496].return_type = &sem->type_rex;
    specs[497].return_type = &sem->type_rex;
    specs[498].return_type = &sem->type_rex;
    specs[499].return_type = &sem->type_rex;
    specs[500].return_type = &sem->type_rex;
    specs[501].return_type = &sem->type_rex;
    specs[502].return_type = &sem->type_rex;
    specs[503].return_type = &sem->type_rex;
    specs[504].return_type = &sem->type_verbum;
    specs[505].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[506].return_type = &sem->type_nihil;
    specs[507].return_type = &sem->type_nihil;
    specs[508].return_type = &sem->type_verbum;
    specs[509].return_type = &sem->type_nihil;
    specs[510].return_type = &sem->type_rex;
    specs[511].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[512].return_type = &sem->type_nihil;
    specs[513].return_type = &sem->type_nihil;
    specs[514].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[515].return_type = &sem->type_nihil;
    specs[516].return_type = &sem->type_nihil;
    specs[517].return_type = &sem->type_nihil;
    specs[518].return_type = &sem->type_nihil;
    specs[519].return_type = &sem->type_rex;
    specs[520].return_type = &sem->type_rex;
    specs[521].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[522].return_type = &sem->type_nihil;
    specs[523].return_type = &sem->type_nihil;
    specs[524].return_type = &sem->type_rex;
    specs[525].return_type = &sem->type_nihil;
    specs[526].return_type = &sem->type_nihil;
    specs[527].return_type = &sem->type_nihil;
    specs[528].return_type = &sem->type_nihil;
    specs[529].return_type = &sem->type_nihil;
    specs[530].return_type = &sem->type_rex;
    specs[531].return_type = &sem->type_rex;
    specs[532].return_type = &sem->type_rex;
    specs[533].return_type = &sem->type_nihil;
    specs[534].return_type = &sem->type_verum;
    specs[535].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[536].return_type = &sem->type_nihil;
    specs[537].return_type = &sem->type_rex;
    specs[538].return_type = &sem->type_rex;
    specs[539].return_type = &sem->type_nihil;
    specs[540].return_type = sem_make_pointer_type(sem, &sem->type_nihil);
    specs[541].return_type = &sem->type_nihil;
    specs[542].return_type = &sem->type_rex;
    specs[543].return_type = &sem->type_nihil;
    specs[544].return_type = &sem->type_rex;
    specs[545].return_type = &sem->type_nihil;
    specs[546].return_type = &sem->type_rex;
    specs[547].return_type = &sem->type_rex;
    specs[548].return_type = &sem->type_rex;
    specs[549].return_type = &sem->type_rex;
    specs[550].return_type = &sem->type_rex;
    specs[551].return_type = &sem->type_rex;
    specs[552].return_type = &sem->type_rex;
    specs[553].return_type = &sem->type_nihil;
    specs[554].return_type = &sem->type_rex;
    specs[555].return_type = &sem->type_nihil;
    specs[556].return_type = &sem->type_rex;
    specs[557].return_type = &sem->type_nihil;
    specs[558].return_type = &sem->type_nihil;
    specs[559].return_type = &sem->type_nihil;
    specs[560].return_type = &sem->type_nihil;
    specs[561].return_type = &sem->type_rex;
    specs[562].return_type = &sem->type_rex;
    specs[563].return_type = &sem->type_nihil;
    specs[564].return_type = &sem->type_nihil;
    specs[565].return_type = &sem->type_rex;
    specs[566].return_type = &sem->type_nihil;
    specs[567].return_type = &sem->type_rex;
    specs[568].return_type = &sem->type_rex;
    specs[569].return_type = &sem->type_nihil;
    specs[570].return_type = &sem->type_nihil;
    specs[571].return_type = &sem->type_rex;
    specs[572].return_type = &sem->type_nihil;
    specs[573].return_type = &sem->type_rex;
    specs[574].return_type = &sem->type_rex;
    specs[575].return_type = &sem->type_nihil;
    specs[576].return_type = &sem->type_nihil;
    specs[577].return_type = &sem->type_nihil;
    specs[578].return_type = &sem->type_nihil;
    specs[579].return_type = &sem->type_verbum;
    specs[580].return_type = &sem->type_verbum;
    specs[581].return_type = &sem->type_verbum;
    specs[582].return_type = &sem->type_verbum;
    specs[583].return_type = &sem->type_rex;
    specs[584].return_type = &sem->type_rex;
    specs[585].return_type = &sem->type_rex;
    specs[586].return_type = &sem->type_rex;
    specs[587].return_type = &sem->type_nihil;
    specs[588].return_type = &sem->type_nihil;
    specs[589].return_type = &sem->type_verbum;
    specs[590].return_type = &sem->type_verbum;
    specs[591].return_type = &sem->type_rex;
    specs[592].return_type = &sem->type_nihil;
    specs[593].return_type = &sem->type_rex;
    specs[594].return_type = &sem->type_nihil;
    specs[595].return_type = &sem->type_nihil;
    specs[596].return_type = &sem->type_nihil;
    specs[597].return_type = &sem->type_nihil;
    specs[598].return_type = &sem->type_nihil;
    specs[599].return_type = &sem->type_rex;
    specs[600].return_type = &sem->type_nihil;
    specs[601].return_type = &sem->type_nihil;
    specs[602].return_type = &sem->type_rex;
    specs[603].return_type = &sem->type_nihil;

    for (size_t i = 0; i < sizeof(specs) / sizeof(specs[0]); i++) {
        if (!specs[i].name) continue;
        if (strcmp(specs[i].name, name) == 0) {
            return &specs[i];
        }
    }
    return NULL;
}

static bool sem_profile_is_freestanding(const cct_semantic_analyzer_t *sem) {
    return sem && sem->profile == CCT_PROFILE_FREESTANDING;
}

static void sem_warn_fpu_type_in_freestanding(
    cct_semantic_analyzer_t *sem,
    const cct_sem_type_t *type,
    u32 line,
    u32 col
) {
    if (!sem_profile_is_freestanding(sem) || !type) return;

    if (type->kind == CCT_SEM_TYPE_MILES) {
        sem_warn_at(
            sem,
            line,
            col,
            "aviso: MILES usa FPU x87; confirmar disponibilidade no target freestanding",
            "considere validar suporte de ponto flutuante no target antes de promover o artefato"
        );
        return;
    }

    if (type->kind == CCT_SEM_TYPE_COMES) {
        sem_warn_at(
            sem,
            line,
            col,
            "aviso: COMES usa FPU x87; confirmar disponibilidade no target freestanding",
            "considere validar suporte de ponto flutuante no target antes de promover o artefato"
        );
    }
}

static bool sem_str_has_prefix(const char *s, const char *prefix) {
    if (!s || !prefix) return false;
    size_t plen = strlen(prefix);
    return strncmp(s, prefix, plen) == 0;
}

static bool sem_is_heap_obsecro_forbidden_in_freestanding(const char *name) {
    if (!name) return false;
    return strcmp(name, "pete") == 0 || strcmp(name, "libera") == 0;
}

static bool sem_is_kernel_obsecro(const char *name) {
    if (!name) return false;
    return strcmp(name, "kernel_halt") == 0 ||
           strcmp(name, "kernel_outb") == 0 ||
           strcmp(name, "kernel_inb") == 0 ||
           strcmp(name, "kernel_memcpy") == 0 ||
           strcmp(name, "kernel_memset") == 0;
}

static bool sem_is_console_obsecro(const char *name) {
    if (!name) return false;
    return strcmp(name, "console_init") == 0 ||
           strcmp(name, "console_clear") == 0 ||
           strcmp(name, "console_putc") == 0 ||
           strcmp(name, "console_write") == 0 ||
           strcmp(name, "console_write_centered") == 0 ||
           strcmp(name, "console_set_color") == 0 ||
           strcmp(name, "console_set_cursor") == 0 ||
           strcmp(name, "console_get_linha") == 0 ||
           strcmp(name, "console_get_coluna") == 0;
}

static bool sem_is_freestanding_svc_obsecro(const char *name) {
    return name && sem_str_has_prefix(name, "cct_svc_");
}

static const char* sem_forbidden_module_for_obsecro_in_freestanding(const char *name) {
    if (!name) return NULL;

    if (sem_str_has_prefix(name, "io_")) return "cct/io";
    if (sem_str_has_prefix(name, "fs_")) return "cct/fs";
    if (sem_str_has_prefix(name, "fluxus_") || sem_str_has_prefix(name, "collection_fluxus_")) {
        return "cct/fluxus";
    }
    if (sem_str_has_prefix(name, "map_")) return "cct/map";
    if (sem_str_has_prefix(name, "set_")) return "cct/set";
    if (sem_str_has_prefix(name, "random_")) return "cct/random";
    if (sem_str_has_prefix(name, "args_")) return "cct/args";
    if (sem_str_has_prefix(name, "scan_")) return "cct/verbum_scan";
    if (sem_str_has_prefix(name, "builder_")) return "cct/verbum_builder";
    if (sem_str_has_prefix(name, "writer_")) return "cct/code_writer";
    if (sem_str_has_prefix(name, "env_")) return "cct/env";
    if (sem_str_has_prefix(name, "time_")) return "cct/time";
    if (sem_str_has_prefix(name, "bytes_")) return "cct/bytes";
    if (sem_str_has_prefix(name, "process_")) return "cct/process";
    if (sem_str_has_prefix(name, "hash_")) return "cct/hash";
    if (sem_str_has_prefix(name, "json_")) return "cct/json";
    if (sem_str_has_prefix(name, "sock_") || sem_str_has_prefix(name, "socket_")) return "cct/socket";
    if (sem_str_has_prefix(name, "pg_builtin_")) return "cct/db_postgres";
    if (sem_str_has_prefix(name, "mail_builtin_")) return "cct/mail";
    if (sem_str_has_prefix(name, "instr_builtin_")) return "cct/instrument";
    if (sem_str_has_prefix(name, "media_store_builtin_")) return "cct/media_store";
    if (sem_str_has_prefix(name, "zip_builtin_")) return "cct/archive_zip";
    if (sem_str_has_prefix(name, "obj_storage_builtin_")) return "cct/object_storage";

    return NULL;
}

/* ========================================================================
 * Forward Declarations (analysis)
 * ======================================================================== */

static cct_sem_type_t* sem_analyze_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr);
static cct_sem_type_t* sem_analyze_expr_with_expected(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr, cct_sem_type_t *expected);
static void sem_analyze_stmt(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt);
static void sem_analyze_block(cct_semantic_analyzer_t *sem, const cct_ast_node_t *block, bool create_scope);
static cct_sem_type_t* sem_analyze_molde_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr);
static const cct_ast_node_t* sem_resolve_ordo_decl_from_type(cct_semantic_analyzer_t *sem, const cct_sem_type_t *type);
static cct_ast_ordo_variant_t* sem_find_ordo_variant(const cct_ast_node_t *ordo_decl, const char *variant_name, size_t *index_out);
static bool sem_identifier_is_any_ordo_variant(cct_semantic_analyzer_t *sem, const char *name);

static void sem_set_symbol_iter_collection_info(
    cct_sem_symbol_t *sym,
    cct_sem_iter_collection_kind_t kind,
    cct_sem_type_t *key_type,
    cct_sem_type_t *value_type
) {
    if (!sym) return;
    sym->iter_collection_kind = kind;
    sym->iter_key_type = key_type;
    sym->iter_value_type = value_type;
}

static bool sem_extract_iter_collection_from_coniura(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *expr,
    cct_sem_iter_collection_kind_t *kind_out,
    cct_sem_type_t **key_type_out,
    cct_sem_type_t **value_type_out
) {
    if (!expr || expr->type != AST_CONIURA || !expr->as.coniura.name) return false;

    const char *name = expr->as.coniura.name;
    const cct_ast_type_list_t *type_args = expr->as.coniura.type_args;
    if (strcmp(name, "map_init") == 0 || strcmp(name, "map_copy") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_MAP;
        if (key_type_out) {
            *key_type_out = (type_args && type_args->count >= 1)
                ? sem_resolve_ast_type(sem, type_args->types[0], expr->line, expr->column)
                : NULL;
        }
        if (value_type_out) {
            *value_type_out = (type_args && type_args->count >= 2)
                ? sem_resolve_ast_type(sem, type_args->types[1], expr->line, expr->column)
                : NULL;
        }
        return true;
    }

    if (strcmp(name, "set_init") == 0 || strcmp(name, "set_copy") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_SET;
        if (key_type_out) {
            *key_type_out = (type_args && type_args->count >= 1)
                ? sem_resolve_ast_type(sem, type_args->types[0], expr->line, expr->column)
                : NULL;
        }
        if (value_type_out) {
            *value_type_out = (type_args && type_args->count >= 1)
                ? sem_resolve_ast_type(sem, type_args->types[0], expr->line, expr->column)
                : NULL;
        }
        return true;
    }

    if (strcmp(name, "fluxus_init") == 0 ||
        strcmp(name, "fluxus_copy") == 0 ||
        strcmp(name, "fluxus_concat") == 0 ||
        strcmp(name, "fluxus_slice") == 0 ||
        strcmp(name, "fluxus_map") == 0 ||
        strcmp(name, "fluxus_filter") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_FLUXUS;
        if (key_type_out) *key_type_out = NULL;
        if (value_type_out) *value_type_out = NULL;
        return true;
    }

    if ((strcmp(name, "map_keys") == 0 || strcmp(name, "map_values") == 0) &&
        type_args && type_args->count >= 2) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_FLUXUS;
        if (key_type_out) *key_type_out = NULL;
        if (value_type_out) {
            const cct_ast_type_t *elem_ast =
                strcmp(name, "map_keys") == 0 ? type_args->types[0] : type_args->types[1];
            *value_type_out = sem_resolve_ast_type(sem, elem_ast, expr->line, expr->column);
        }
        return true;
    }

    return false;
}

static bool sem_extract_iter_collection_from_obsecro(
    const cct_ast_node_t *expr,
    cct_sem_iter_collection_kind_t *kind_out
) {
    if (!expr || expr->type != AST_OBSECRO || !expr->as.obsecro.name) return false;
    const char *name = expr->as.obsecro.name;
    if (strcmp(name, "map_init") == 0 || strcmp(name, "map_copy") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_MAP;
        return true;
    }
    if (strcmp(name, "set_init") == 0 || strcmp(name, "set_copy") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_SET;
        return true;
    }
    if (strcmp(name, "fluxus_init") == 0 || strcmp(name, "cct_svc_fluxus_new") == 0) {
        if (kind_out) *kind_out = CCT_SEM_ITER_COLLECTION_FLUXUS;
        return true;
    }
    return false;
}

static void sem_infer_iter_collection_from_expr(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *expr,
    cct_sem_iter_collection_kind_t *kind_out,
    cct_sem_type_t **key_type_out,
    cct_sem_type_t **value_type_out
) {
    cct_sem_iter_collection_kind_t kind = CCT_SEM_ITER_COLLECTION_NONE;
    cct_sem_type_t *key_type = NULL;
    cct_sem_type_t *value_type = NULL;

    if (!expr) {
        if (kind_out) *kind_out = kind;
        if (key_type_out) *key_type_out = key_type;
        if (value_type_out) *value_type_out = value_type;
        return;
    }

    if (expr->type == AST_IDENTIFIER && expr->as.identifier.name) {
        cct_sem_symbol_t *sym = sem_lookup(sem, expr->as.identifier.name);
        if (sym) {
            kind = sym->iter_collection_kind;
            key_type = sym->iter_key_type;
            value_type = sym->iter_value_type;
            if (kind == CCT_SEM_ITER_COLLECTION_NONE &&
                sym->type &&
                sym->type->kind == CCT_SEM_TYPE_POINTER &&
                sym->type->element &&
                sym->type->element->kind == CCT_SEM_TYPE_NIHIL) {
                kind = CCT_SEM_ITER_COLLECTION_FLUXUS;
            }
        }
    }

    if (kind == CCT_SEM_ITER_COLLECTION_NONE) {
        if (!sem_extract_iter_collection_from_coniura(sem, expr, &kind, &key_type, &value_type)) {
            (void)sem_extract_iter_collection_from_obsecro(expr, &kind);
        }
    }

    if (kind_out) *kind_out = kind;
    if (key_type_out) *key_type_out = key_type;
    if (value_type_out) *value_type_out = value_type;
}

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
                if (ast_arg && !ast_arg->is_pointer) {
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
            arg_type = sem_analyze_expr_with_expected(sem, args->nodes[i], expected_type);
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

    if (!sem_profile_is_freestanding(sem) && sem_is_kernel_obsecro(name)) {
        if (expr->as.obsecro.arguments) {
            for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
            }
        }
        sem_report_node(sem, expr, "cct/kernel disponível apenas em perfil freestanding");
        return &sem->type_error;
    }

    if (!sem_profile_is_freestanding(sem) && sem_is_console_obsecro(name)) {
        if (expr->as.obsecro.arguments) {
            for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
            }
        }
        sem_report_node(sem, expr, "cct/console disponível apenas em perfil freestanding");
        return &sem->type_error;
    }

    if (!sem_profile_is_freestanding(sem) && sem_is_freestanding_svc_obsecro(name)) {
        if (expr->as.obsecro.arguments) {
            for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
            }
        }
        sem_report_node(sem, expr, "serviços cct_svc_* disponíveis apenas em perfil freestanding");
        return &sem->type_error;
    }

    if (sem_profile_is_freestanding(sem)) {
        if (sem_is_heap_obsecro_forbidden_in_freestanding(name)) {
            if (expr->as.obsecro.arguments) {
                for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                    (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
                }
            }
            sem_report_node(sem, expr,
                            "pete()/libera() não disponíveis em perfil freestanding (heap ausente no target)");
            return &sem->type_error;
        }

        const char *forbidden_module = sem_forbidden_module_for_obsecro_in_freestanding(name);
        if (forbidden_module) {
            if (expr->as.obsecro.arguments) {
                for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                    (void)sem_analyze_expr(sem, expr->as.obsecro.arguments->nodes[i]);
                }
            }
            sem_report_nodef(sem, expr,
                             "módulo '%s' não disponível em perfil freestanding",
                             forbidden_module);
            return &sem->type_error;
        }
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
        if (strncmp(name, "callback_builtin_invoke", 23) == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects callback pointer in first argument", name);
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
        if (strcmp(name, "verbum_char_at") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_char_at expects first argument as VERBUM");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_char_at expects integer index");
            }
        }
        if (strcmp(name, "verbum_from_char") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO verbum_from_char expects integer byte argument");
        }
        if ((strcmp(name, "verbum_to_upper") == 0 ||
             strcmp(name, "verbum_to_lower") == 0 ||
             strcmp(name, "verbum_trim_left") == 0 ||
             strcmp(name, "verbum_trim_right") == 0 ||
             strcmp(name, "verbum_reverse") == 0 ||
             strcmp(name, "verbum_is_ascii") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if ((strcmp(name, "verbum_starts_with") == 0 ||
             strcmp(name, "verbum_ends_with") == 0 ||
             strcmp(name, "verbum_strip_prefix") == 0 ||
             strcmp(name, "verbum_strip_suffix") == 0 ||
             strcmp(name, "verbum_last_find") == 0 ||
             strcmp(name, "verbum_count_occurrences") == 0 ||
             strcmp(name, "verbum_equals_ignore_case") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if ((strcmp(name, "verbum_replace") == 0 ||
             strcmp(name, "verbum_replace_all") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "verbum_trim_char") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_trim_char expects first argument as VERBUM");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_trim_char expects integer fill byte");
            }
        }
        if (strcmp(name, "verbum_repeat") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_repeat expects first argument as VERBUM");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_repeat expects integer repeat count");
            }
        }
        if (strcmp(name, "verbum_pad_left") == 0 ||
            strcmp(name, "verbum_pad_right") == 0 ||
            strcmp(name, "verbum_center") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects first argument as VERBUM", name);
            }
            if ((i == 1 || i == 2) && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer width/fill arguments", name);
            }
        }
        if (strcmp(name, "verbum_find_from") == 0) {
            if ((i == 0 || i == 1) &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_find_from expects first two arguments as VERBUM");
            }
            if (i == 2 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_find_from expects integer offset");
            }
        }
        if (strcmp(name, "verbum_slice") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_slice expects first argument as VERBUM");
            }
            if ((i == 1 || i == 2) && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_slice expects integer start/length arguments");
            }
        }
        if (strcmp(name, "verbum_split") == 0) {
            if (!(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_split expects VERBUM arguments");
            }
        }
        if (strcmp(name, "verbum_split_char") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_split_char expects first argument as VERBUM");
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_split_char expects integer separator byte");
            }
        }
        if (strcmp(name, "verbum_join") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_join expects fluxus pointer as first argument");
            }
            if (i == 1 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO verbum_join expects VERBUM separator as second argument");
            }
        }
        if ((strcmp(name, "verbum_lines") == 0 || strcmp(name, "verbum_words") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if ((strcmp(name, "char_is_digit") == 0 ||
             strcmp(name, "char_is_alpha") == 0 ||
             strcmp(name, "char_is_whitespace") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer byte argument", name);
        }
        if (strcmp(name, "args_arg") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO args_arg expects integer index");
        }
        if (strcmp(name, "scan_init") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO scan_init expects VERBUM source argument");
        }
        if ((strcmp(name, "scan_pos") == 0 ||
             strcmp(name, "scan_eof") == 0 ||
             strcmp(name, "scan_peek") == 0 ||
             strcmp(name, "scan_next") == 0 ||
             strcmp(name, "scan_free") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects cursor pointer argument", name);
        }
        if ((strcmp(name, "builder_append") == 0 ||
             strcmp(name, "builder_append_char") == 0 ||
             strcmp(name, "builder_len") == 0 ||
             strcmp(name, "builder_to_verbum") == 0 ||
             strcmp(name, "builder_clear") == 0 ||
             strcmp(name, "builder_free") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects builder pointer argument", name);
        }
        if (strcmp(name, "builder_append") == 0 && i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO builder_append expects VERBUM argument");
        }
        if (strcmp(name, "builder_append_char") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO builder_append_char expects integer byte argument");
        }
        if ((strcmp(name, "writer_indent") == 0 ||
             strcmp(name, "writer_dedent") == 0 ||
             strcmp(name, "writer_write") == 0 ||
             strcmp(name, "writer_writeln") == 0 ||
             strcmp(name, "writer_to_verbum") == 0 ||
             strcmp(name, "writer_free") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects writer pointer argument", name);
        }
        if ((strcmp(name, "writer_write") == 0 || strcmp(name, "writer_writeln") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM text argument", name);
        }
        if ((strcmp(name, "env_get") == 0 || strcmp(name, "env_has") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if (strcmp(name, "time_sleep_ms") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO time_sleep_ms expects integer ms argument");
        }
        if (strcmp(name, "bytes_new") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO bytes_new expects integer size argument");
        }
        if ((strcmp(name, "bytes_len") == 0 ||
             strcmp(name, "bytes_get") == 0 ||
             strcmp(name, "bytes_set") == 0 ||
             strcmp(name, "bytes_free") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects bytes pointer argument", name);
        }
        if ((strcmp(name, "bytes_get") == 0 || strcmp(name, "bytes_set") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer index argument", name);
        }
        if (strcmp(name, "bytes_set") == 0 &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO bytes_set expects integer byte value");
        }
        if (strcmp(name, "fmt_stringify_int") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fmt_stringify_int expects integer argument");
        }
        if ((strcmp(name, "fmt_stringify_int_hex") == 0 ||
             strcmp(name, "fmt_stringify_int_hex_upper") == 0 ||
             strcmp(name, "fmt_stringify_int_oct") == 0 ||
             strcmp(name, "fmt_stringify_int_bin") == 0 ||
             strcmp(name, "fmt_stringify_uint") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
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
        if ((strcmp(name, "parse_try_int") == 0 ||
             strcmp(name, "parse_try_real") == 0 ||
             strcmp(name, "parse_try_bool") == 0 ||
             strcmp(name, "parse_int_hex") == 0 ||
             strcmp(name, "parse_try_int_hex") == 0 ||
             strcmp(name, "parse_is_int") == 0 ||
             strcmp(name, "parse_is_real") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if (strcmp(name, "fractum_to_verbum") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_FRACTUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fractum_to_verbum expects FRACTUM argument");
        }
        if ((strcmp(name, "parse_int_radix") == 0 ||
             strcmp(name, "parse_try_int_radix") == 0)) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects VERBUM as first argument", name);
            }
            if (i == 1 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer radix as second argument", name);
            }
        }
        if (strcmp(name, "parse_csv_line") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO parse_csv_line expects VERBUM as first argument");
            }
            if (i == 1 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO parse_csv_line expects integer separator byte as second argument");
            }
        }
        if (strcmp(name, "fmt_stringify_int_padded") == 0) {
            if ((i == 0 || i == 1 || i == 2) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO fmt_stringify_int_padded expects integer arguments");
            }
        }
        if (strcmp(name, "fmt_stringify_real_prec") == 0 ||
            strcmp(name, "fmt_stringify_real_fixed") == 0) {
            if (i == 0 && !(arg_type && (arg_type->kind == CCT_SEM_TYPE_UMBRA || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects UMBRA first argument", name);
            }
            if (i == 1 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer precision argument", name);
            }
        }
        if (strcmp(name, "fmt_stringify_real_sci") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_UMBRA || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fmt_stringify_real_sci expects UMBRA argument");
        }
        if ((strcmp(name, "fmt_format_1") == 0 ||
             strcmp(name, "fmt_format_2") == 0 ||
             strcmp(name, "fmt_format_3") == 0 ||
             strcmp(name, "fmt_format_4") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "fmt_repeat_char") == 0) {
            if ((i == 0 || i == 1) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO fmt_repeat_char expects integer arguments");
            }
        }
        if (strcmp(name, "fmt_table_row") == 0) {
            if ((i == 0 || i == 1) &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO fmt_table_row expects pointer arguments for parts/widths");
            }
            if (i == 2 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO fmt_table_row expects integer ncols argument");
            }
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
        if ((strcmp(name, "kernel_outb") == 0 || strcmp(name, "kernel_inb") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "kernel_outb") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO kernel_outb expects integer argument");
        }
        if ((strcmp(name, "cct_svc_outw") == 0 ||
             strcmp(name, "cct_svc_inw") == 0 ||
             strcmp(name, "cct_svc_outl") == 0 ||
             strcmp(name, "cct_svc_inl") == 0 ||
             strcmp(name, "cct_svc_pci_vendor") == 0 ||
             strcmp(name, "cct_svc_pci_device_id") == 0 ||
             strcmp(name, "cct_svc_pci_class") == 0 ||
             strcmp(name, "cct_svc_pci_bar0") == 0 ||
             strcmp(name, "cct_svc_pci_irq") == 0 ||
             strcmp(name, "cct_svc_pci_find") == 0 ||
             strcmp(name, "cct_svc_pci_enable_busmaster") == 0) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument in FS-4A", name);
        }
        if (strcmp(name, "kernel_memcpy") == 0 && (i == 0 || i == 1) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO kernel_memcpy expects pointer arguments");
        }
        if (strcmp(name, "kernel_memset") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO kernel_memset expects pointer destination argument");
        }
        if ((strcmp(name, "kernel_memcpy") == 0 || strcmp(name, "kernel_memset") == 0) &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer size argument", name);
        }
        if (strcmp(name, "kernel_memset") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO kernel_memset expects integer byte value as second argument");
        }
        if ((strcmp(name, "console_putc") == 0 ||
             strcmp(name, "console_set_cursor") == 0) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "console_set_color") == 0 &&
            !(sem_is_integer_type(arg_type) ||
              (arg_type && arg_type->kind == CCT_SEM_TYPE_NAMED) ||
              sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO console_set_color expects color enum/integer argument");
        }
        if ((strcmp(name, "console_write") == 0 ||
             strcmp(name, "console_write_centered") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if (strcmp(name, "cct_svc_alloc") == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_alloc expects integer size argument");
        }
        if (strcmp(name, "cct_svc_realloc") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO cct_svc_realloc expects pointer as first argument");
            }
            if ((i == 1 || i == 2) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO cct_svc_realloc expects integer size arguments");
            }
        }
        if ((strcmp(name, "cct_svc_free") == 0 || strcmp(name, "cct_svc_memcpy") == 0 ||
             strcmp(name, "cct_svc_memset") == 0 || strcmp(name, "cct_svc_byte_at") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "cct_svc_memcpy") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_memcpy expects pointer source argument");
        }
        if ((strcmp(name, "cct_svc_memcpy") == 0 || strcmp(name, "cct_svc_memset") == 0 ||
             strcmp(name, "cct_svc_byte_at") == 0) &&
            ((strcmp(name, "cct_svc_byte_at") == 0) ? i == 1 : i == 2) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "cct_svc_memset") == 0 &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_memset expects integer byte value");
        }
        if ((strcmp(name, "cct_svc_verbum_byte") == 0 || strcmp(name, "cct_svc_verbum_copy_slice") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if ((strcmp(name, "cct_svc_verbum_len") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_verbum_len expects VERBUM argument");
        }
        if ((strcmp(name, "cct_svc_verbum_byte") == 0 || strcmp(name, "cct_svc_verbum_copy_slice") == 0) &&
            i > 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer index/length argument", name);
        }
        if ((strcmp(name, "cct_svc_builder_append") == 0 ||
             strcmp(name, "cct_svc_builder_append_char") == 0 ||
             strcmp(name, "cct_svc_builder_build") == 0 ||
             strcmp(name, "cct_svc_builder_clear") == 0 ||
             strcmp(name, "cct_svc_builder_len") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects builder pointer as first argument", name);
        }
        if (strcmp(name, "cct_svc_builder_append") == 0 &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_builder_append expects VERBUM payload");
        }
        if (strcmp(name, "cct_svc_builder_append_char") == 0 &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_builder_append_char expects integer byte");
        }
        if (strcmp(name, "cct_svc_fluxus_new") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_fluxus_new expects integer element size");
        }
        if ((strcmp(name, "cct_svc_fluxus_push") == 0 || strcmp(name, "cct_svc_fluxus_pop") == 0 ||
             strcmp(name, "cct_svc_fluxus_get") == 0 || strcmp(name, "cct_svc_fluxus_set") == 0 ||
             strcmp(name, "cct_svc_fluxus_clear") == 0 || strcmp(name, "cct_svc_fluxus_reserve") == 0 ||
             strcmp(name, "cct_svc_fluxus_free") == 0 || strcmp(name, "cct_svc_fluxus_len") == 0 ||
             strcmp(name, "cct_svc_fluxus_cap") == 0 || strcmp(name, "cct_svc_fluxus_peek") == 0 ||
             strcmp(name, "cct_svc_fluxus_remove_first") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects fluxus pointer as first argument", name);
        }
        if ((strcmp(name, "cct_svc_fluxus_push") == 0 || strcmp(name, "cct_svc_fluxus_pop") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer payload as second argument", name);
        }
        if ((strcmp(name, "cct_svc_fluxus_get") == 0 || strcmp(name, "cct_svc_fluxus_reserve") == 0 ||
             strcmp(name, "cct_svc_fluxus_set") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "cct_svc_fluxus_set") == 0 &&
            i == 2 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_fluxus_set expects pointer payload as third argument");
        }
        if ((strcmp(name, "cct_svc_irq_mask") == 0 ||
             strcmp(name, "cct_svc_irq_unmask") == 0 ||
             strcmp(name, "cct_svc_irq_unregister") == 0 ||
             strcmp(name, "cct_svc_timer_sleep") == 0 ||
             strcmp(name, "cct_svc_net_init") == 0 ||
             strcmp(name, "cct_svc_tcp_init") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "cct_svc_irq_register") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_irq_register expects integer IRQ number");
        }
        if (strcmp(name, "cct_svc_irq_register") == 0 &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if (strcmp(name, "cct_svc_builder_backspace") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO cct_svc_builder_backspace expects builder pointer");
        }
        if ((strcmp(name, "cct_svc_net_mac") == 0 ||
             strcmp(name, "cct_svc_net_recv") == 0 ||
             strcmp(name, "cct_svc_tcp_recv") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "cct_svc_net_send") == 0 ||
             strcmp(name, "cct_svc_tcp_send") == 0) &&
            i == 0 &&
            !(arg_type && ((arg_type->kind == CCT_SEM_TYPE_POINTER) ||
                           (arg_type->kind == CCT_SEM_TYPE_VERBUM) ||
                           sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer or VERBUM payload", name);
        }
        if ((strcmp(name, "cct_svc_net_send") == 0 ||
             strcmp(name, "cct_svc_net_recv") == 0 ||
             strcmp(name, "cct_svc_tcp_recv") == 0 ||
             strcmp(name, "cct_svc_tcp_send") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer length argument", name);
        }
        if ((strcmp(name, "cct_svc_http_server_init") == 0 ||
             strcmp(name, "cct_svc_http_server_accept") == 0 ||
             strcmp(name, "cct_svc_http_server_read") == 0 ||
             strcmp(name, "cct_svc_http_server_req_len") == 0 ||
             strcmp(name, "cct_svc_http_server_req_count") == 0 ||
             strcmp(name, "cct_svc_http_parse") == 0 ||
             strcmp(name, "cct_svc_http_req_method") == 0 ||
             strcmp(name, "cct_svc_http_req_path") == 0 ||
             strcmp(name, "cct_svc_http_req_query") == 0 ||
             strcmp(name, "cct_svc_http_req_version") == 0 ||
             strcmp(name, "cct_svc_http_req_method_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_path_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_query_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_version_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_method_is") == 0 ||
             strcmp(name, "cct_svc_http_req_path_is") == 0 ||
             strcmp(name, "cct_svc_http_req_path_starts") == 0 ||
             strcmp(name, "cct_svc_http_req_header_count") == 0 ||
             strcmp(name, "cct_svc_http_req_header_name") == 0 ||
             strcmp(name, "cct_svc_http_req_header_value") == 0 ||
             strcmp(name, "cct_svc_http_req_header_name_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_header_value_ptr") == 0 ||
             strcmp(name, "cct_svc_http_req_body_len") == 0 ||
             strcmp(name, "cct_svc_http_req_body_copy") == 0 ||
             strcmp(name, "cct_svc_http_req_find_header") == 0 ||
             strcmp(name, "cct_svc_http_res_begin") == 0 ||
             strcmp(name, "cct_svc_http_res_header") == 0 ||
             strcmp(name, "cct_svc_http_res_finish") == 0 ||
             strcmp(name, "cct_svc_http_res_build") == 0 ||
             strcmp(name, "cct_svc_http_res_send") == 0 ||
             strcmp(name, "cct_svc_http_res_len") == 0 ||
             strcmp(name, "cct_svc_http_router_init") == 0 ||
             strcmp(name, "cct_svc_http_router_add") == 0 ||
             strcmp(name, "cct_svc_http_router_dispatch") == 0 ||
             strcmp(name, "cct_svc_http_router_set_404") == 0) &&
            sem_profile_is_freestanding(sem)) {
            if ((strcmp(name, "cct_svc_http_server_init") == 0 ||
                 strcmp(name, "cct_svc_http_server_accept") == 0 ||
                 strcmp(name, "cct_svc_http_server_read") == 0 ||
                 strcmp(name, "cct_svc_http_server_req_len") == 0 ||
                 strcmp(name, "cct_svc_http_server_req_count") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_count") == 0 ||
                 strcmp(name, "cct_svc_http_req_body_len") == 0 ||
                 strcmp(name, "cct_svc_http_res_begin") == 0 ||
                 strcmp(name, "cct_svc_http_res_len") == 0 ||
                 strcmp(name, "cct_svc_http_router_set_404") == 0) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer argument in FS-5", name);
            }
            if ((strcmp(name, "cct_svc_http_server_send") == 0 ||
                 strcmp(name, "cct_svc_http_res_finish") == 0 ||
                 strcmp(name, "cct_svc_http_res_build") == 0) &&
                ((strcmp(name, "cct_svc_http_server_send") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_res_finish") == 0 && i == 1) ||
                 (strcmp(name, "cct_svc_http_res_build") == 0 && i == 2)) &&
                !(arg_type && ((arg_type->kind == CCT_SEM_TYPE_POINTER) ||
                               (arg_type->kind == CCT_SEM_TYPE_VERBUM) ||
                               sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects pointer or VERBUM payload in FS-5", name);
            }
            if ((strcmp(name, "cct_svc_http_server_req_copy") == 0 ||
                 strcmp(name, "cct_svc_http_req_body_copy") == 0 ||
                 strcmp(name, "cct_svc_http_req_method") == 0 ||
                 strcmp(name, "cct_svc_http_req_path") == 0 ||
                 strcmp(name, "cct_svc_http_req_query") == 0 ||
                 strcmp(name, "cct_svc_http_req_version") == 0) &&
                i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects pointer destination in FS-5", name);
            }
            if ((strcmp(name, "cct_svc_http_req_header_name") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_value") == 0) &&
                i == 1 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects pointer destination in FS-5", name);
            }
            if ((strcmp(name, "cct_svc_http_server_send") == 0 ||
                 strcmp(name, "cct_svc_http_server_req_copy") == 0 ||
                 strcmp(name, "cct_svc_http_req_body_copy") == 0 ||
                 strcmp(name, "cct_svc_http_req_method") == 0 ||
                 strcmp(name, "cct_svc_http_req_path") == 0 ||
                 strcmp(name, "cct_svc_http_req_query") == 0 ||
                 strcmp(name, "cct_svc_http_req_version") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_name") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_value") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_name_ptr") == 0 ||
                 strcmp(name, "cct_svc_http_req_header_value_ptr") == 0 ||
                 strcmp(name, "cct_svc_http_res_finish") == 0 ||
                 strcmp(name, "cct_svc_http_res_build") == 0 ||
                 strcmp(name, "cct_svc_http_router_add") == 0) &&
                ((strcmp(name, "cct_svc_http_server_send") == 0 && i == 1) ||
                 (strcmp(name, "cct_svc_http_server_req_copy") == 0 && i == 1) ||
                 (strcmp(name, "cct_svc_http_req_body_copy") == 0 && i == 1) ||
                 ((strcmp(name, "cct_svc_http_req_method") == 0 ||
                   strcmp(name, "cct_svc_http_req_path") == 0 ||
                   strcmp(name, "cct_svc_http_req_query") == 0 ||
                   strcmp(name, "cct_svc_http_req_version") == 0) && i == 1) ||
                 ((strcmp(name, "cct_svc_http_req_header_name") == 0 ||
                   strcmp(name, "cct_svc_http_req_header_value") == 0) && (i == 0 || i == 2)) ||
                 ((strcmp(name, "cct_svc_http_req_header_name_ptr") == 0 ||
                   strcmp(name, "cct_svc_http_req_header_value_ptr") == 0) && i == 0) ||
                 (strcmp(name, "cct_svc_http_res_finish") == 0 && i == 2) ||
                 (strcmp(name, "cct_svc_http_res_build") == 0 && (i == 0 || i == 3)) ||
                 (strcmp(name, "cct_svc_http_router_add") == 0 && (i == 2 || i == 3))) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer argument in FS-5", name);
            }
            if ((strcmp(name, "cct_svc_http_req_method_is") == 0 ||
                 strcmp(name, "cct_svc_http_req_path_is") == 0 ||
                 strcmp(name, "cct_svc_http_req_path_starts") == 0 ||
                 strcmp(name, "cct_svc_http_req_find_header") == 0 ||
                 strcmp(name, "cct_svc_http_res_header") == 0 ||
                 strcmp(name, "cct_svc_http_router_dispatch") == 0 ||
                 strcmp(name, "cct_svc_http_router_add") == 0 ||
                 strcmp(name, "cct_svc_http_res_finish") == 0 ||
                 strcmp(name, "cct_svc_http_res_build") == 0) &&
                ((strcmp(name, "cct_svc_http_req_method_is") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_req_path_is") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_req_path_starts") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_req_find_header") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_res_header") == 0 && (i == 0 || i == 1)) ||
                 (strcmp(name, "cct_svc_http_router_dispatch") == 0 && (i == 0 || i == 1)) ||
                 (strcmp(name, "cct_svc_http_router_add") == 0 && (i == 0 || i == 1)) ||
                 (strcmp(name, "cct_svc_http_res_finish") == 0 && i == 0) ||
                 (strcmp(name, "cct_svc_http_res_build") == 0 && i == 1)) &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects VERBUM argument in FS-5", name);
            }
        }
        if ((strcmp(name, "json_arr_handle_new") == 0 ||
             strcmp(name, "json_obj_handle_new") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer element size argument", name);
        }
        if ((strcmp(name, "json_arr_handle_push") == 0 ||
             strcmp(name, "json_obj_handle_push") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer handle as first argument", name);
        }
        if ((strcmp(name, "json_arr_handle_push") == 0 ||
             strcmp(name, "json_obj_handle_push") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer payload as second argument", name);
        }
        if ((strcmp(name, "json_arr_handle_len") == 0 ||
             strcmp(name, "json_obj_handle_len") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer handle argument", name);
        }
        if ((strcmp(name, "json_arr_handle_get") == 0 ||
             strcmp(name, "json_obj_handle_get") == 0)) {
            if (i == 0 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer handle as first argument", name);
            }
            if (i == 1 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects integer index as second argument", name);
            }
        }
        if (strcmp(name, "json_handle_ptr") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO json_handle_ptr expects integer handle argument");
        }
        if ((strcmp(name, "sock_connect") == 0 ||
             strcmp(name, "sock_bind") == 0 ||
             strcmp(name, "sock_listen") == 0 ||
             strcmp(name, "sock_accept") == 0 ||
             strcmp(name, "sock_send") == 0 ||
             strcmp(name, "sock_recv") == 0 ||
             strcmp(name, "sock_close") == 0 ||
             strcmp(name, "sock_set_timeout_ms") == 0 ||
             strcmp(name, "sock_peer_addr") == 0 ||
             strcmp(name, "sock_local_addr") == 0 ||
             strcmp(name, "db_close") == 0 ||
             strcmp(name, "db_exec") == 0 ||
             strcmp(name, "db_last_error") == 0 ||
             strcmp(name, "db_query") == 0 ||
             strcmp(name, "db_prepare") == 0 ||
             strcmp(name, "rows_next") == 0 ||
             strcmp(name, "rows_get_text") == 0 ||
             strcmp(name, "rows_get_int") == 0 ||
             strcmp(name, "rows_get_real") == 0 ||
             strcmp(name, "rows_close") == 0 ||
             strcmp(name, "stmt_bind_text") == 0 ||
             strcmp(name, "stmt_bind_int") == 0 ||
             strcmp(name, "stmt_bind_real") == 0 ||
             strcmp(name, "stmt_step") == 0 ||
             strcmp(name, "stmt_has_row") == 0 ||
             strcmp(name, "stmt_get_text") == 0 ||
             strcmp(name, "stmt_get_int") == 0 ||
             strcmp(name, "stmt_get_real") == 0 ||
             strcmp(name, "stmt_reset") == 0 ||
             strcmp(name, "stmt_finalize") == 0 ||
             strcmp(name, "db_begin") == 0 ||
             strcmp(name, "db_commit") == 0 ||
             strcmp(name, "db_rollback") == 0 ||
             strcmp(name, "db_scalar_int") == 0 ||
             strcmp(name, "db_scalar_text") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             (strcmp(name, "db_close") == 0 || strcmp(name, "db_exec") == 0 ||
                              strcmp(name, "db_last_error") == 0 || strcmp(name, "db_query") == 0 ||
                              strcmp(name, "db_prepare") == 0 || strcmp(name, "db_begin") == 0 ||
                              strcmp(name, "db_commit") == 0 || strcmp(name, "db_rollback") == 0 ||
                              strcmp(name, "db_scalar_int") == 0 || strcmp(name, "db_scalar_text") == 0)
                                 ? "OBSECRO %s expects db pointer as first argument"
                                 : ((strcmp(name, "rows_next") == 0 || strcmp(name, "rows_get_text") == 0 ||
                                     strcmp(name, "rows_get_int") == 0 || strcmp(name, "rows_get_real") == 0 ||
                                     strcmp(name, "rows_close") == 0)
                                        ? "OBSECRO %s expects rows pointer as first argument"
                                        : ((strcmp(name, "stmt_bind_text") == 0 || strcmp(name, "stmt_bind_int") == 0 ||
                                            strcmp(name, "stmt_bind_real") == 0 || strcmp(name, "stmt_step") == 0 ||
                                            strcmp(name, "stmt_has_row") == 0 || strcmp(name, "stmt_get_text") == 0 ||
                                            strcmp(name, "stmt_get_int") == 0 || strcmp(name, "stmt_get_real") == 0 ||
                                            strcmp(name, "stmt_reset") == 0 || strcmp(name, "stmt_finalize") == 0)
                                               ? "OBSECRO %s expects stmt pointer as first argument"
                                               : "OBSECRO %s expects socket pointer as first argument")),
                             name);
        }
        if ((strcmp(name, "db_open") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO db_open expects VERBUM path argument");
        }
        if ((strcmp(name, "db_exec") == 0 || strcmp(name, "db_query") == 0 || strcmp(name, "db_prepare") == 0 ||
             strcmp(name, "db_scalar_int") == 0 || strcmp(name, "db_scalar_text") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM sql as second argument", name);
        }
        if ((strcmp(name, "pg_builtin_open") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO pg_builtin_open expects VERBUM conninfo argument");
        }
        if ((strcmp(name, "pg_builtin_close") == 0 || strcmp(name, "pg_builtin_is_open") == 0 ||
             strcmp(name, "pg_builtin_last_error") == 0 || strcmp(name, "pg_builtin_exec") == 0 ||
             strcmp(name, "pg_builtin_query") == 0 || strcmp(name, "pg_builtin_poll_channel") == 0 ||
             strcmp(name, "pg_builtin_poll_payload") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects db pointer as first argument", name);
        }
        if ((strcmp(name, "pg_builtin_rows_count") == 0 || strcmp(name, "pg_builtin_rows_columns") == 0 ||
             strcmp(name, "pg_builtin_rows_next") == 0 || strcmp(name, "pg_builtin_rows_get_text") == 0 ||
             strcmp(name, "pg_builtin_rows_is_null") == 0 || strcmp(name, "pg_builtin_rows_close") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects rows pointer as first argument", name);
        }
        if ((strcmp(name, "pg_builtin_exec") == 0 || strcmp(name, "pg_builtin_query") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM sql as second argument", name);
        }
        if ((strcmp(name, "pg_builtin_rows_get_text") == 0 || strcmp(name, "pg_builtin_rows_is_null") == 0 ||
             strcmp(name, "pg_builtin_poll_channel") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if ((strcmp(name, "mail_builtin_smtp_send") == 0) &&
            (i == 0 || i == 2 || i == 3 || i == 7 || i == 8 || i == 9) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mail_builtin_smtp_send expects VERBUM argument at position %zu", i + 1);
        }
        if ((strcmp(name, "mail_builtin_smtp_send") == 0) &&
            (i == 1 || i == 4) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mail_builtin_smtp_send expects integer argument at position %zu", i + 1);
        }
        if ((strcmp(name, "mail_builtin_smtp_send") == 0) &&
            (i == 5 || i == 6) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO mail_builtin_smtp_send expects VERUM argument at position %zu", i + 1);
        }
        if ((strcmp(name, "instr_builtin_enable") == 0 ||
             strcmp(name, "instr_builtin_span_end") == 0 ||
             strcmp(name, "instr_builtin_buffer_span_id") == 0 ||
             strcmp(name, "instr_builtin_buffer_parent_id") == 0 ||
             strcmp(name, "instr_builtin_buffer_name") == 0 ||
             strcmp(name, "instr_builtin_buffer_category") == 0 ||
             strcmp(name, "instr_builtin_buffer_start_us") == 0 ||
             strcmp(name, "instr_builtin_buffer_end_us") == 0 ||
             strcmp(name, "instr_builtin_buffer_closed") == 0 ||
             strcmp(name, "instr_builtin_buffer_attr_count") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if ((strcmp(name, "instr_builtin_span_begin") == 0 ||
             strcmp(name, "instr_builtin_event") == 0) &&
            (i == 0 || i == 1) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument at position %zu", name, i + 1);
        }
        if ((strcmp(name, "instr_builtin_span_attr") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO instr_builtin_span_attr expects integer span_id argument");
        }
        if ((strcmp(name, "instr_builtin_span_attr") == 0) &&
            (i == 1 || i == 2) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO instr_builtin_span_attr expects VERBUM argument at position %zu", i + 1);
        }
        if ((strcmp(name, "instr_builtin_buffer_attr_key") == 0 ||
             strcmp(name, "instr_builtin_buffer_attr_value") == 0) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer arguments", name);
        }
        if ((strcmp(name, "rows_get_text") == 0 ||
             strcmp(name, "rows_get_int") == 0 ||
             strcmp(name, "rows_get_real") == 0 ||
             strcmp(name, "stmt_get_text") == 0 ||
             strcmp(name, "stmt_get_int") == 0 ||
             strcmp(name, "stmt_get_real") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer column as second argument", name);
        }
        if ((strcmp(name, "stmt_bind_text") == 0 ||
             strcmp(name, "stmt_bind_int") == 0 ||
             strcmp(name, "stmt_bind_real") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer idx as second argument", name);
        }
        if (strcmp(name, "stmt_bind_text") == 0 &&
            i == 2 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO stmt_bind_text expects VERBUM value as third argument");
        }
        if (strcmp(name, "stmt_bind_int") == 0 &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO stmt_bind_int expects integer value as third argument");
        }
        if (strcmp(name, "stmt_bind_real") == 0 &&
            i == 2 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_UMBRA || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO stmt_bind_real expects UMBRA value as third argument");
        }
        if ((strcmp(name, "sock_connect") == 0 || strcmp(name, "sock_bind") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM host as second argument", name);
        }
        if ((strcmp(name, "sock_connect") == 0 || strcmp(name, "sock_bind") == 0 ||
             strcmp(name, "sock_listen") == 0 || strcmp(name, "sock_recv") == 0 ||
             strcmp(name, "sock_set_timeout_ms") == 0) &&
            ((strcmp(name, "sock_connect") == 0 || strcmp(name, "sock_bind") == 0) ? i == 2 : i == 1) &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "sock_send") == 0 && i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO sock_send expects VERBUM payload as second argument");
        }
        if (strcmp(name, "fluxus_init") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fluxus_init expects integer element size argument");
        }
        if ((strcmp(name, "fluxus_free") == 0 || strcmp(name, "fluxus_push") == 0 ||
             strcmp(name, "fluxus_pop") == 0 || strcmp(name, "fluxus_len") == 0 ||
             strcmp(name, "fluxus_get") == 0 || strcmp(name, "fluxus_clear") == 0 ||
             strcmp(name, "fluxus_reserve") == 0 || strcmp(name, "fluxus_capacity") == 0 ||
             strcmp(name, "fluxus_peek") == 0 || strcmp(name, "fluxus_set") == 0 ||
             strcmp(name, "fluxus_remove") == 0 || strcmp(name, "fluxus_insert") == 0 ||
             strcmp(name, "fluxus_contains") == 0 || strcmp(name, "fluxus_reverse") == 0 ||
             strcmp(name, "fluxus_sort_int") == 0 || strcmp(name, "fluxus_sort_verbum") == 0 ||
             strcmp(name, "fluxus_to_ptr") == 0) &&
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
        if ((strcmp(name, "fluxus_get") == 0 || strcmp(name, "fluxus_reserve") == 0 ||
             strcmp(name, "fluxus_remove") == 0 || strcmp(name, "fluxus_set") == 0 ||
             strcmp(name, "fluxus_insert") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer as second argument", name);
        }
        if ((strcmp(name, "fluxus_contains") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fluxus_contains expects pointer as second argument");
        }
        if ((strcmp(name, "fluxus_set") == 0 || strcmp(name, "fluxus_insert") == 0) &&
            i == 2 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer as third argument", name);
        }
        if ((strcmp(name, "io_print") == 0 || strcmp(name, "io_println") == 0 ||
             strcmp(name, "io_eprint") == 0 || strcmp(name, "io_eprintln") == 0 ||
             strcmp(name, "fs_read_all") == 0 || strcmp(name, "fs_exists") == 0 ||
             strcmp(name, "fs_size") == 0 || strcmp(name, "fs_mkdir") == 0 ||
             strcmp(name, "fs_mkdir_all") == 0 || strcmp(name, "fs_delete_file") == 0 ||
             strcmp(name, "fs_delete_dir") == 0 || strcmp(name, "fs_is_file") == 0 ||
             strcmp(name, "fs_is_dir") == 0 || strcmp(name, "fs_is_symlink") == 0 ||
             strcmp(name, "fs_is_readable") == 0 || strcmp(name, "fs_is_writable") == 0 ||
             strcmp(name, "fs_modified_time") == 0 || strcmp(name, "fs_list_dir") == 0 ||
             strcmp(name, "path_basename") == 0 || strcmp(name, "path_dirname") == 0 ||
             strcmp(name, "path_ext") == 0 || strcmp(name, "path_join") == 0 ||
             strcmp(name, "path_stem") == 0 || strcmp(name, "path_normalize") == 0 ||
             strcmp(name, "path_is_absolute") == 0 || strcmp(name, "path_resolve") == 0 ||
             strcmp(name, "path_relative_to") == 0 || strcmp(name, "path_with_ext") == 0 ||
             strcmp(name, "path_without_ext") == 0 || strcmp(name, "path_split") == 0 ||
             strcmp(name, "process_run") == 0 || strcmp(name, "process_run_capture") == 0 ||
             strcmp(name, "process_run_with_input") == 0 || strcmp(name, "process_run_env") == 0 ||
             strcmp(name, "process_run_timeout") == 0 ||
             strcmp(name, "hash_crc32") == 0 || strcmp(name, "hash_murmur3") == 0 ||
             strcmp(name, "crypto_sha256_text") == 0 || strcmp(name, "crypto_sha512_text") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if ((strcmp(name, "crypto_sha256_bytes") == 0 || strcmp(name, "crypto_sha512_bytes") == 0) && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects bytes pointer as first argument", name);
        }
        if ((strcmp(name, "crypto_sha256_bytes") == 0 || strcmp(name, "crypto_sha512_bytes") == 0) && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer length as second argument", name);
        }
        if ((strcmp(name, "crypto_hmac_sha256") == 0 || strcmp(name, "crypto_hmac_sha512") == 0 ||
             strcmp(name, "crypto_constant_time_compare") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "crypto_pbkdf2_sha256") == 0) {
            if ((i == 0 || i == 1) &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO crypto_pbkdf2_sha256 expects VERBUM password/salt arguments");
            }
            if ((i == 2 || i == 3) &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO crypto_pbkdf2_sha256 expects integer iterations/key_length");
            }
        }
        if (strcmp(name, "crypto_csprng_bytes") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO crypto_csprng_bytes expects integer count argument");
        }
        if (strcmp(name, "regex_builtin_compile") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO regex_builtin_compile expects VERBUM pattern as first argument");
            }
            if (i == 1 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO regex_builtin_compile expects integer flags as second argument");
            }
        }
        if ((strcmp(name, "regex_builtin_match") == 0 ||
             strcmp(name, "regex_builtin_search") == 0 ||
             strcmp(name, "regex_builtin_find_all") == 0 ||
             strcmp(name, "regex_builtin_split") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects regex handle pointer as first argument", name);
        }
        if ((strcmp(name, "regex_builtin_match") == 0 ||
             strcmp(name, "regex_builtin_search") == 0 ||
             strcmp(name, "regex_builtin_find_all") == 0 ||
             strcmp(name, "regex_builtin_split") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM text as second argument", name);
        }
        if (strcmp(name, "regex_builtin_replace") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO regex_builtin_replace expects regex handle pointer as first argument");
            }
            if ((i == 1 || i == 2) &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO regex_builtin_replace expects VERBUM text/replacement arguments");
            }
            if (i == 3 &&
                !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO regex_builtin_replace expects integer all flag as fourth argument");
            }
        }
        if (strcmp(name, "regex_builtin_free") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO regex_builtin_free expects regex handle pointer argument");
        }
        if ((strcmp(name, "toml_builtin_parse") == 0 ||
             strcmp(name, "toml_builtin_parse_file") == 0 ||
             strcmp(name, "compress_builtin_gzip_compress_text") == 0 ||
             strcmp(name, "compress_builtin_gzip_decompress_text") == 0 ||
             strcmp(name, "compress_builtin_gzip_decompress_bytes") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM argument", name);
        }
        if ((strcmp(name, "toml_builtin_type") == 0 ||
             strcmp(name, "toml_builtin_get_string") == 0 ||
             strcmp(name, "toml_builtin_get_int") == 0 ||
             strcmp(name, "toml_builtin_get_real") == 0 ||
             strcmp(name, "toml_builtin_get_bool") == 0 ||
             strcmp(name, "toml_builtin_get_subdoc") == 0 ||
             strcmp(name, "toml_builtin_array_len") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects document handle integer as first argument", name);
        }
        if ((strcmp(name, "toml_builtin_type") == 0 ||
             strcmp(name, "toml_builtin_get_string") == 0 ||
             strcmp(name, "toml_builtin_get_int") == 0 ||
             strcmp(name, "toml_builtin_get_real") == 0 ||
             strcmp(name, "toml_builtin_get_bool") == 0 ||
             strcmp(name, "toml_builtin_get_subdoc") == 0 ||
             strcmp(name, "toml_builtin_array_len") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM key as second argument", name);
        }
        if ((strcmp(name, "toml_builtin_array_item_string") == 0 ||
             strcmp(name, "toml_builtin_array_item_int") == 0 ||
             strcmp(name, "toml_builtin_array_item_real") == 0 ||
             strcmp(name, "toml_builtin_array_item_bool") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects document handle integer as first argument", name);
        }
        if ((strcmp(name, "toml_builtin_array_item_string") == 0 ||
             strcmp(name, "toml_builtin_array_item_int") == 0 ||
             strcmp(name, "toml_builtin_array_item_real") == 0 ||
             strcmp(name, "toml_builtin_array_item_bool") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM key as second argument", name);
        }
        if (strcmp(name, "compress_builtin_gzip_compress_bytes") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO compress_builtin_gzip_compress_bytes expects bytes pointer as first argument");
        }
        if ((strcmp(name, "toml_builtin_array_item_string") == 0 ||
             strcmp(name, "toml_builtin_array_item_int") == 0 ||
             strcmp(name, "toml_builtin_array_item_real") == 0 ||
             strcmp(name, "toml_builtin_array_item_bool") == 0) &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer index as third argument", name);
        }
        if (strcmp(name, "toml_builtin_expand_env") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO toml_builtin_expand_env expects document handle integer argument");
        }
        if (strcmp(name, "toml_builtin_stringify") == 0 &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO toml_builtin_stringify expects document handle integer argument");
        }
        if (strcmp(name, "compress_builtin_gzip_compress_bytes") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO compress_builtin_gzip_compress_bytes expects integer length as second argument");
        }
        if (strcmp(name, "filetype_builtin_detect_path") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO filetype_builtin_detect_path expects VERBUM path argument");
        }
        if (strcmp(name, "filetype_builtin_detect_bytes") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO filetype_builtin_detect_bytes expects bytes pointer as first argument");
        }
        if (strcmp(name, "filetype_builtin_detect_bytes") == 0 &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO filetype_builtin_detect_bytes expects integer length as second argument");
        }
        if (strcmp(name, "fluxus_contains_verbum") == 0 &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fluxus_contains_verbum expects fluxus handle pointer as first argument");
        }
        if (strcmp(name, "fluxus_contains_verbum") == 0 &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO fluxus_contains_verbum expects VERBUM candidate as second argument");
        }
        if ((strcmp(name, "gettext_builtin_default_translate") == 0 ||
             strcmp(name, "gettext_builtin_catalog_new") == 0 ||
             strcmp(name, "gettext_builtin_catalog_load") == 0 ||
             strcmp(name, "gettext_builtin_catalog_last_error") == 0) &&
            (strcmp(name, "gettext_builtin_catalog_last_error") != 0) &&
            (i == 0 || (strcmp(name, "gettext_builtin_catalog_load") == 0 && i == 1)) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if ((strcmp(name, "gettext_builtin_catalog_add") == 0 ||
             strcmp(name, "gettext_builtin_catalog_add_plural") == 0 ||
             strcmp(name, "gettext_builtin_translate") == 0 ||
             strcmp(name, "gettext_builtin_translate_plural") == 0 ||
             strcmp(name, "gettext_builtin_default_set") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer handle as first argument", name);
        }
        if ((strcmp(name, "gettext_builtin_catalog_add") == 0 && (i == 1 || i == 2)) ||
            (strcmp(name, "gettext_builtin_catalog_add_plural") == 0 && i >= 1 && i <= 4) ||
            (strcmp(name, "gettext_builtin_translate") == 0 && i == 1) ||
            (strcmp(name, "gettext_builtin_translate_plural") == 0 && (i == 1 || i == 2)) ||
            (strcmp(name, "gettext_builtin_default_translate") == 0 && i == 0)) {
            if (!(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO %s expects VERBUM arguments", name);
            }
        }
        if (strcmp(name, "gettext_builtin_translate_plural") == 0 &&
            i == 3 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO gettext_builtin_translate_plural expects integer count argument");
        }
        if (strcmp(name, "gettext_builtin_catalog_load") == 0 &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO gettext_builtin_catalog_load expects VERBUM locale argument");
        }
        if ((strcmp(name, "image_builtin_load") == 0 || strcmp(name, "image_builtin_last_error") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM path argument", name);
        }
        if ((strcmp(name, "image_builtin_free") == 0 ||
             strcmp(name, "image_builtin_get_width") == 0 ||
             strcmp(name, "image_builtin_get_height") == 0 ||
             strcmp(name, "image_builtin_get_channels") == 0 ||
             strcmp(name, "image_builtin_get_format") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects image handle pointer argument", name);
        }
        if (strcmp(name, "image_builtin_save") == 0) {
            if (i == 0 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO image_builtin_save expects image handle pointer as first argument");
            }
            if (i == 1 &&
                !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO image_builtin_save expects VERBUM path as second argument");
            }
            if (i == 2 && !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
                sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                                 "OBSECRO image_builtin_save expects integer quality as third argument");
            }
        }
        if ((strcmp(name, "image_builtin_resize") == 0 ||
             strcmp(name, "image_builtin_crop") == 0 ||
             strcmp(name, "image_builtin_rotate") == 0 ||
             strcmp(name, "image_builtin_convert") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects image handle pointer as first argument", name);
        }
        if ((strcmp(name, "image_builtin_resize") == 0 ||
             strcmp(name, "image_builtin_crop") == 0 ||
             strcmp(name, "image_builtin_rotate") == 0 ||
             strcmp(name, "image_builtin_convert") == 0) &&
            i > 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer scalar arguments after the image handle", name);
        }
        if ((strcmp(name, "io_print_int") == 0 || strcmp(name, "io_eprint_int") == 0 ||
             strcmp(name, "io_print_char") == 0) &&
            i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if ((strcmp(name, "io_print_real") == 0 || strcmp(name, "io_eprint_real") == 0) &&
            i == 0 &&
            !(sem_is_numeric_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects numeric argument", name);
        }
        if ((strcmp(name, "fs_write_all") == 0 || strcmp(name, "fs_append_all") == 0 ||
             strcmp(name, "fs_rename") == 0 || strcmp(name, "fs_copy") == 0 ||
             strcmp(name, "fs_move") == 0 || strcmp(name, "fs_symlink") == 0 ||
             strcmp(name, "fs_same_file") == 0) &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if ((strcmp(name, "fs_chmod") == 0 || strcmp(name, "fs_truncate") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM path argument", name);
        }
        if ((strcmp(name, "fs_chmod") == 0 || strcmp(name, "fs_truncate") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer second argument", name);
        }
        if ((strcmp(name, "path_join") == 0 ||
             strcmp(name, "path_relative_to") == 0 ||
             strcmp(name, "path_with_ext") == 0 ||
             strcmp(name, "process_run_with_input") == 0 ||
             strcmp(name, "random_verbum_from") == 0) &&
            i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_VERBUM || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects VERBUM arguments", name);
        }
        if (strcmp(name, "process_run_env") == 0 && i == 1 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO process_run_env expects pointer env_pairs argument");
        }
        if (strcmp(name, "process_run_timeout") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO process_run_timeout expects integer timeout_ms argument");
        }
        if (strcmp(name, "hash_fnv1a_bytes") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO hash_fnv1a_bytes expects pointer data argument");
        }
        if (strcmp(name, "random_shuffle_int") == 0 && i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO random_shuffle_int expects pointer array argument");
        }
        if ((strcmp(name, "hash_fnv1a_bytes") == 0 || strcmp(name, "hash_murmur3") == 0) && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer second argument", name);
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
        if ((strcmp(name, "random_verbum") == 0 || strcmp(name, "random_bytes") == 0) && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer argument", name);
        }
        if (strcmp(name, "random_verbum_from") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO random_verbum_from expects integer length argument");
        }
        if (strcmp(name, "random_shuffle_int") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO random_shuffle_int expects integer length argument");
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
             strcmp(name, "result_unwrap_ptr") == 0 || strcmp(name, "result_unwrap_handle") == 0 ||
             strcmp(name, "result_unwrap_err_ptr") == 0 ||
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
             strcmp(name, "map_clear") == 0 || strcmp(name, "map_copy") == 0 ||
             strcmp(name, "map_keys") == 0 || strcmp(name, "map_values") == 0 ||
             strcmp(name, "set_free") == 0 || strcmp(name, "set_len") == 0 ||
             strcmp(name, "set_is_empty") == 0 || strcmp(name, "set_clear") == 0 ||
             strcmp(name, "set_copy") == 0 || strcmp(name, "set_to_fluxus") == 0 ||
             strcmp(name, "set_capacity") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer argument", name);
        }
        if ((strcmp(name, "map_insert") == 0 || strcmp(name, "map_remove") == 0 ||
             strcmp(name, "map_get_ptr") == 0 || strcmp(name, "map_contains") == 0 ||
             strcmp(name, "map_reserve") == 0 || strcmp(name, "map_merge") == 0 ||
             strcmp(name, "alg_sort_verbum") == 0 ||
             strcmp(name, "set_insert") == 0 || strcmp(name, "set_remove") == 0 ||
             strcmp(name, "set_contains") == 0 || strcmp(name, "set_reserve") == 0 ||
             strcmp(name, "set_union") == 0 || strcmp(name, "set_intersection") == 0 ||
             strcmp(name, "set_difference") == 0 || strcmp(name, "set_symmetric_difference") == 0 ||
             strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) &&
            i == 0 &&
            !(arg_type && (arg_type->kind == CCT_SEM_TYPE_POINTER || sem_is_error_type(arg_type)))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects pointer container argument", name);
        }
        if ((strcmp(name, "map_insert") == 0 || strcmp(name, "map_remove") == 0 ||
             strcmp(name, "map_get_ptr") == 0 || strcmp(name, "map_contains") == 0 ||
             strcmp(name, "map_merge") == 0 || strcmp(name, "set_insert") == 0 ||
             strcmp(name, "set_remove") == 0 || strcmp(name, "set_contains") == 0 ||
             strcmp(name, "set_union") == 0 || strcmp(name, "set_intersection") == 0 ||
             strcmp(name, "set_difference") == 0 || strcmp(name, "set_symmetric_difference") == 0 ||
             strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) &&
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
        if ((strcmp(name, "map_keys") == 0 || strcmp(name, "map_values") == 0) && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer element-size argument", name);
        }
        if (strcmp(name, "alg_sort_verbum") == 0 && i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO alg_sort_verbum expects integer length argument");
        }
        if (strcmp(name, "set_init") == 0 && i == 0 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer size argument", name);
        }
        if ((strcmp(name, "set_copy") == 0 || strcmp(name, "set_to_fluxus") == 0 ||
             strcmp(name, "set_reserve") == 0) &&
            i == 1 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer size argument", name);
        }
        if ((strcmp(name, "set_union") == 0 || strcmp(name, "set_intersection") == 0 ||
             strcmp(name, "set_difference") == 0 || strcmp(name, "set_symmetric_difference") == 0 ||
             strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) &&
            i == 2 &&
            !(sem_is_integer_type(arg_type) || sem_is_error_type(arg_type))) {
            sem_report_nodef(sem, expr->as.obsecro.arguments->nodes[i],
                             "OBSECRO %s expects integer item-size argument", name);
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

static cct_sem_type_t* sem_analyze_molde_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    if (!expr || expr->type != AST_MOLDE) return &sem->type_error;

    if (sem_profile_is_freestanding(sem)) {
        sem_report_node(sem, expr, "MOLDE nao disponivel no perfil freestanding");
    }

    if (expr->as.molde.parts) {
        for (size_t i = 0; i < expr->as.molde.part_count; i++) {
            cct_ast_molde_part_t *part = &expr->as.molde.parts[i];
            if (!part || part->kind != CCT_AST_MOLDE_PART_EXPR) continue;
            if (part->expr && part->expr->type == AST_OBSECRO) {
                sem_report_node(
                    sem,
                    part->expr,
                    "MOLDE: OBSECRO nao pode ser usado como expressao em interpolacao"
                );
                continue;
            }

            cct_sem_type_t *part_type = sem_analyze_expr(sem, part->expr);
            if (sem_is_error_type(part_type)) continue;

            if (!sem_is_molde_compatible_type(part_type)) {
                sem_report_nodef(
                    sem,
                    part->expr,
                    "MOLDE: expressao em {} nao suporta interpolacao de tipo %s",
                    cct_sem_type_string(part_type)
                );
                continue;
            }

            sem_validate_molde_fmt_spec(sem, part->expr, part_type, part->fmt_spec);
        }
    }

    return &sem->type_verbum;
}

static cct_sem_type_t* sem_analyze_expr_with_expected(
    cct_semantic_analyzer_t *sem,
    const cct_ast_node_t *expr,
    cct_sem_type_t *expected
) {
    if (!expr) return &sem->type_nihil;

    switch (expr->type) {
        case AST_LITERAL_INT:
            return &sem->type_rex;
        case AST_LITERAL_REAL:
            return &sem->type_umbra;
        case AST_LITERAL_STRING:
            return &sem->type_verbum;
        case AST_MOLDE:
            return sem_analyze_molde_expr(sem, expr);
        case AST_LITERAL_BOOL:
            return &sem->type_verum;
        case AST_LITERAL_NIHIL:
            return &sem->type_nihil;

        case AST_IDENTIFIER: {
            if (expected && expected->kind == CCT_SEM_TYPE_POINTER && expr->as.identifier.name) {
                cct_sem_symbol_t *sym = sem_lookup(sem, expr->as.identifier.name);
                if (sym && sym->kind == CCT_SEM_SYMBOL_RITUALE) {
                    return expected;
                }
            }
            cct_sem_type_t *id_type = sem_analyze_identifier_expr(sem, expr);
            const cct_ast_node_t *expected_ordo = sem_resolve_ordo_decl_from_type(sem, expected);
            if (expected_ordo && expected_ordo->as.ordo.has_payload && expr->as.identifier.name) {
                cct_ast_ordo_variant_t *variant = sem_find_ordo_variant(expected_ordo, expr->as.identifier.name, NULL);
                if (variant) {
                    if (variant->field_count != 0) {
                        sem_report_nodef(
                            sem,
                            expr,
                            "ORDO payload: construtor %s() chamado com 0 args, esperava %zu",
                            variant->name,
                            variant->field_count
                        );
                        return &sem->type_error;
                    }
                    cct_ast_node_t *mutable_expr = (cct_ast_node_t*)expr;
                    mutable_expr->as.identifier.is_ordo_construct = true;
                    free(mutable_expr->as.identifier.ordo_name);
                    free(mutable_expr->as.identifier.variant_name);
                    mutable_expr->as.identifier.ordo_name = sem_strdup(expected_ordo->as.ordo.name);
                    mutable_expr->as.identifier.variant_name = sem_strdup(variant->name);
                    mutable_expr->as.identifier.ordo_tag_value = variant->tag_value;
                    return expected;
                }
            }
            return id_type;
        }

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

            const char *callee_name = expr->as.call.callee->as.identifier.name;
            const cct_ast_node_t *expected_ordo = sem_resolve_ordo_decl_from_type(sem, expected);
            if (expected_ordo && expected_ordo->as.ordo.has_payload && callee_name) {
                cct_ast_ordo_variant_t *variant = sem_find_ordo_variant(expected_ordo, callee_name, NULL);
                if (variant) {
                    size_t argc = expr->as.call.arguments ? expr->as.call.arguments->count : 0;

                    if (variant->field_count == 0) {
                        sem_report_nodef(
                            sem,
                            expr,
                            "ORDO payload: variante sem payload deve ser usada sem parenteses: %s",
                            variant->name
                        );
                        return &sem->type_error;
                    }

                    if (argc != variant->field_count) {
                        sem_report_nodef(
                            sem,
                            expr,
                            "ORDO payload: construtor %s() chamado com %zu args, esperava %zu",
                            variant->name,
                            argc,
                            variant->field_count
                        );
                        for (size_t i = 0; i < argc; i++) {
                            (void)sem_analyze_expr(sem, expr->as.call.arguments->nodes[i]);
                        }
                        return &sem->type_error;
                    }

                    for (size_t i = 0; i < argc; i++) {
                        cct_ast_ordo_field_t *field = variant->fields[i];
                        cct_sem_type_t *field_type = field
                            ? sem_resolve_ast_type(sem, field->type, field->line, field->column)
                            : &sem->type_error;
                        cct_sem_type_t *arg_type = sem_analyze_expr_with_expected(
                            sem,
                            expr->as.call.arguments->nodes[i],
                            field_type
                        );
                        if (!sem_types_compatible_assign(sem, field_type, arg_type)) {
                            sem_report_nodef(
                                sem,
                                expr->as.call.arguments->nodes[i],
                                "ORDO payload: construtor %s() arg %zu tipo %s incompativel com campo %s:%s",
                                variant->name,
                                i + 1,
                                cct_sem_type_string(arg_type),
                                field && field->name ? field->name : "<campo>",
                                cct_sem_type_string(field_type)
                            );
                        }
                    }

                    cct_ast_node_t *mutable_expr = (cct_ast_node_t*)expr;
                    mutable_expr->as.call.is_ordo_construct = true;
                    free(mutable_expr->as.call.ordo_name);
                    free(mutable_expr->as.call.variant_name);
                    mutable_expr->as.call.ordo_name = sem_strdup(expected_ordo->as.ordo.name);
                    mutable_expr->as.call.variant_name = sem_strdup(variant->name);
                    mutable_expr->as.call.ordo_tag_value = variant->tag_value;
                    return expected;
                }

                if (sem_identifier_is_any_ordo_variant(sem, callee_name)) {
                    sem_report_nodef(
                        sem,
                        expr,
                        "ORDO payload: construtor '%s' nao existe no tipo '%s'",
                        callee_name,
                        expected_ordo->as.ordo.name ? expected_ordo->as.ordo.name : "<ORDO>"
                    );
                    if (expr->as.call.arguments) {
                        for (size_t i = 0; i < expr->as.call.arguments->count; i++) {
                            (void)sem_analyze_expr(sem, expr->as.call.arguments->nodes[i]);
                        }
                    }
                    return &sem->type_error;
                }
            }

            if (callee_name && sem_identifier_is_any_ordo_variant(sem, callee_name) && !expected_ordo) {
                sem_report_nodef(
                    sem,
                    expr,
                    "ORDO payload: construtor '%s' sem contexto de tipo ORDO",
                    callee_name
                );
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
                callee_name,
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
                    if (!(sem_is_bool_type(operand) || sem_is_integer_type(operand)) && !sem_is_error_type(operand)) {
                        sem_report_node(sem, expr, "NON requires boolean or integer operand");
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
                case TOKEN_STAR_STAR:
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

                case TOKEN_SLASH_SLASH:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "operator // requires integer operands");
                        }
                        return &sem->type_error;
                    }
                    return &sem->type_rex;

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

                case TOKEN_PERCENT_PERCENT:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "operator %% requires integer operands");
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
                    if ((left->kind == CCT_SEM_TYPE_POINTER && right->kind == CCT_SEM_TYPE_NIHIL) ||
                        (left->kind == CCT_SEM_TYPE_NIHIL && right->kind == CCT_SEM_TYPE_POINTER) ||
                        (left->kind == CCT_SEM_TYPE_POINTER && right->kind == CCT_SEM_TYPE_POINTER)) {
                        return &sem->type_verum;
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
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!(sem_is_bool_type(left) || sem_is_integer_type(left)) ||
                        !(sem_is_bool_type(right) || sem_is_integer_type(right))) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "operator ET requires boolean or integer operands");
                        }
                    }
                    return &sem->type_verum;

                case TOKEN_VEL:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!(sem_is_bool_type(left) || sem_is_integer_type(left)) ||
                        !(sem_is_bool_type(right) || sem_is_integer_type(right))) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "operator VEL requires boolean or integer operands");
                        }
                    }
                    return &sem->type_verum;

                case TOKEN_ET_BIT:
                case TOKEN_VEL_BIT:
                case TOKEN_XOR:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "bitwise operators require integer operands");
                        }
                    }
                    return &sem->type_rex;

                case TOKEN_SINISTER:
                case TOKEN_DEXTER:
                    if (!sem_require_concrete_type_for_operation(sem, expr->as.binary_op.left, left) ||
                        !sem_require_concrete_type_for_operation(sem, expr->as.binary_op.right, right)) {
                        return &sem->type_error;
                    }
                    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
                        if (!sem_is_error_type(left) && !sem_is_error_type(right)) {
                            sem_report_node(sem, expr, "shift operators require integer operands");
                        }
                        return &sem->type_error;
                    }
                    if (sem_is_negative_shift_constant(expr->as.binary_op.right)) {
                        sem_warn_node(sem, expr->as.binary_op.right,
                                      "shift count is a negative constant; behavior is platform-defined");
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

static cct_sem_type_t* sem_analyze_expr(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    return sem_analyze_expr_with_expected(sem, expr, NULL);
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
        cct_sem_type_t *init_type = sem_analyze_expr_with_expected(sem, stmt->as.evoca.initializer, decl_type);
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
        sym->is_constans = stmt->as.evoca.is_constans;
        sem_set_symbol_iter_collection_info(sym, CCT_SEM_ITER_COLLECTION_NONE, NULL, NULL);
        if (stmt->as.evoca.initializer) {
            cct_sem_iter_collection_kind_t coll_kind = CCT_SEM_ITER_COLLECTION_NONE;
            cct_sem_type_t *key_type = NULL;
            cct_sem_type_t *value_type = NULL;
            sem_infer_iter_collection_from_expr(
                sem,
                stmt->as.evoca.initializer,
                &coll_kind,
                &key_type,
                &value_type
            );
            if (coll_kind != CCT_SEM_ITER_COLLECTION_NONE) {
                sem_set_symbol_iter_collection_info(sym, coll_kind, key_type, value_type);
            }
        }
    }
}

static void sem_analyze_vincire(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    cct_sem_type_t *target_type = sem_analyze_lvalue(sem, stmt->as.vincire.target);
    cct_sem_type_t *value_type = sem_analyze_expr_with_expected(sem, stmt->as.vincire.value, target_type);

    if (stmt->as.vincire.target && stmt->as.vincire.target->type == AST_IDENTIFIER) {
        const char *name = stmt->as.vincire.target->as.identifier.name;
        cct_sem_symbol_t *target_sym = sem_lookup(sem, name);
        if (target_sym &&
            (target_sym->kind == CCT_SEM_SYMBOL_VARIABLE || target_sym->kind == CCT_SEM_SYMBOL_PARAMETER) &&
            target_sym->is_constans) {
            if (target_sym->kind == CCT_SEM_SYMBOL_PARAMETER) {
                sem_reportf(sem,
                            stmt->as.vincire.target->line,
                            stmt->as.vincire.target->column,
                            "cannot reassign CONSTANS parameter '%s'",
                            name);
            } else {
                sem_reportf(sem,
                            stmt->as.vincire.target->line,
                            stmt->as.vincire.target->column,
                            "cannot reassign CONSTANS variable '%s'",
                            name);
            }
            return;
        }
    }

    if (!sem_types_compatible_assign(sem, target_type, value_type)) {
        sem_report_nodef(sem, stmt,
                         "assignment type mismatch (%s <- %s)",
                         cct_sem_type_string(target_type),
                         cct_sem_type_string(value_type));
        return;
    }

    if (stmt->as.vincire.target && stmt->as.vincire.target->type == AST_IDENTIFIER) {
        const char *target_name = stmt->as.vincire.target->as.identifier.name;
        cct_sem_symbol_t *target_sym = sem_lookup(sem, target_name);
        if (target_sym && (target_sym->kind == CCT_SEM_SYMBOL_VARIABLE || target_sym->kind == CCT_SEM_SYMBOL_PARAMETER)) {
            cct_sem_iter_collection_kind_t coll_kind = CCT_SEM_ITER_COLLECTION_NONE;
            cct_sem_type_t *key_type = NULL;
            cct_sem_type_t *value_type_hint = NULL;
            sem_infer_iter_collection_from_expr(sem, stmt->as.vincire.value, &coll_kind, &key_type, &value_type_hint);
            if (coll_kind != CCT_SEM_ITER_COLLECTION_NONE) {
                sem_set_symbol_iter_collection_info(target_sym, coll_kind, key_type, value_type_hint);
            }
        }
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

    cct_sem_type_t *actual = sem_analyze_expr_with_expected(sem, stmt->as.redde.value, expected);
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
        sem_report_node(sem, stmt, "DIMITTE requires pointer/VERBUM lvalue");
        return;
    }
    if (!sem_is_lvalue_node_supported(target)) {
        (void)sem_analyze_expr(sem, target);
        sem_report_node(sem, target, "DIMITTE requires addressable pointer/VERBUM lvalue");
        return;
    }

    cct_sem_type_t *target_type = sem_analyze_lvalue(sem, target);
    if (sem_is_error_type(target_type)) return;

    if (target_type->kind != CCT_SEM_TYPE_POINTER && target_type->kind != CCT_SEM_TYPE_VERBUM) {
        sem_report_nodef(sem, target,
                         "DIMITTE requires pointer/VERBUM lvalue (manually-liberable in subset final da FASE 7; got %s)",
                         cct_sem_type_string(target_type));
        return;
    }
}

static void sem_analyze_condition(cct_semantic_analyzer_t *sem, const cct_ast_node_t *expr) {
    cct_sem_type_t *type = sem_analyze_expr(sem, expr);
    if (!(sem_is_bool_type(type) || sem_is_integer_type(type)) && !sem_is_error_type(type)) {
        sem_report_node(sem, expr, "condition expression must be VERUM (boolean) or integer");
    }
}

static void sem_analyze_iace(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    if (sem_profile_is_freestanding(sem)) {
        if (stmt->as.iace.value) (void)sem_analyze_expr(sem, stmt->as.iace.value);
        sem_report_node(sem, stmt, "IACE não suportado em perfil freestanding");
        return;
    }

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
    if (sem_profile_is_freestanding(sem)) {
        sem_report_node(sem, stmt, "TEMPTA não suportado em perfil freestanding");
        if (stmt->as.tempta.cape_block || stmt->as.tempta.cape_name || stmt->as.tempta.cape_type) {
            sem_report_node(sem, stmt, "CAPE não suportado em perfil freestanding");
        }
        if (stmt->as.tempta.semper_block) {
            sem_report_node(sem, stmt->as.tempta.semper_block, "SEMPER não suportado em perfil freestanding");
        }
        if (stmt->as.tempta.try_block) sem_analyze_stmt(sem, stmt->as.tempta.try_block);
        if (stmt->as.tempta.cape_block) sem_analyze_stmt(sem, stmt->as.tempta.cape_block);
        return;
    }

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

static bool sem_quando_eval_int_literal(const cct_ast_node_t *literal, long long *value_out) {
    if (!literal || !value_out) return false;

    switch (literal->type) {
        case AST_LITERAL_INT:
            *value_out = (long long)literal->as.literal_int.int_value;
            return true;
        case AST_LITERAL_BOOL:
            *value_out = literal->as.literal_bool.bool_value ? 1LL : 0LL;
            return true;
        case AST_UNARY_OP:
            if (!literal->as.unary_op.operand || literal->as.unary_op.operand->type != AST_LITERAL_INT) {
                return false;
            }
            if (literal->as.unary_op.operator == TOKEN_MINUS) {
                *value_out = -(long long)literal->as.unary_op.operand->as.literal_int.int_value;
                return true;
            }
            if (literal->as.unary_op.operator == TOKEN_PLUS) {
                *value_out = (long long)literal->as.unary_op.operand->as.literal_int.int_value;
                return true;
            }
            return false;
        default:
            return false;
    }
}

static void sem_analyze_quando(cct_semantic_analyzer_t *sem, const cct_ast_node_t *stmt) {
    cct_sem_type_t *expr_type = sem_analyze_expr(sem, stmt->as.quando.expression);
    bool expr_is_string = expr_type && expr_type->kind == CCT_SEM_TYPE_VERBUM;
    bool expr_is_ordo = false;
    const cct_ast_node_t *ordo_decl = NULL;
    if (expr_type && expr_type->kind == CCT_SEM_TYPE_NAMED && expr_type->name) {
        cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, expr_type->name);
        if (type_sym && type_sym->type_decl && type_sym->type_decl->type == AST_ORDO) {
            expr_is_ordo = true;
            ordo_decl = type_sym->type_decl;
        }
    }
    bool expr_supported = sem_is_integer_type(expr_type) || sem_is_bool_type(expr_type) ||
                          expr_is_string || expr_is_ordo || sem_is_error_type(expr_type);
    if (!expr_supported) {
        sem_report_node(sem, stmt->as.quando.expression, "QUANDO: expressao deve ser de tipo inteiro, VERUM, VERBUM ou ORDO");
    }

    if (stmt->as.quando.case_count == 0) {
        sem_warn_at(
            sem,
            stmt->line,
            stmt->column,
            "QUANDO sem CASO: bloco nao cobre nenhum valor explicitamente",
            NULL
        );
    }

    long long *seen_values = NULL;
    size_t seen_count = 0;
    size_t seen_capacity = 0;
    const char **seen_strings = NULL;
    size_t seen_strings_count = 0;
    size_t seen_strings_capacity = 0;

    size_t ordo_variant_count = 0;
    bool *covered_ordo = NULL;
    if (expr_is_ordo && ordo_decl) {
        if (ordo_decl->as.ordo.variants && ordo_decl->as.ordo.variants->count > 0) {
            ordo_variant_count = ordo_decl->as.ordo.variants->count;
        } else if (ordo_decl->as.ordo.items) {
            ordo_variant_count = ordo_decl->as.ordo.items->count;
        }
        if (ordo_variant_count > 0) {
            covered_ordo = (bool*)calloc(ordo_variant_count, sizeof(bool));
            if (!covered_ordo) {
                fprintf(stderr, "cct: fatal: out of memory\n");
                exit(CCT_ERROR_OUT_OF_MEMORY);
            }
        }
    }

    for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
        cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
        bool case_body_checked = false;

        for (size_t j = 0; j < case_node->literal_count; j++) {
            const cct_ast_node_t *literal = case_node->literals[j];

            if (expr_is_ordo) {
                if (!literal || literal->type != AST_IDENTIFIER) {
                    sem_report_node(sem, literal, "QUANDO ORDO: CASO deve ser nome de variante");
                    continue;
                }

                const char *variant = literal->as.identifier.name ? literal->as.identifier.name : "";
                size_t variant_index = 0;
                cct_ast_ordo_variant_t *resolved_variant = sem_find_ordo_variant(ordo_decl, variant, &variant_index);
                bool found_in_items = false;
                if (!resolved_variant && ordo_decl && ordo_decl->as.ordo.items) {
                    for (size_t v = 0; v < ordo_decl->as.ordo.items->count; v++) {
                        cct_ast_enum_item_t *item = ordo_decl->as.ordo.items->items[v];
                        if (item && item->name && strcmp(item->name, variant) == 0) {
                            variant_index = v;
                            found_in_items = true;
                            break;
                        }
                    }
                }

                if (!resolved_variant && !found_in_items) {
                    sem_report_nodef(sem, literal, "QUANDO ORDO: variante '%s' nao existe no tipo", variant);
                    continue;
                }

                if (covered_ordo && variant_index < ordo_variant_count && covered_ordo[variant_index]) {
                    sem_report_nodef(sem, literal, "QUANDO: CASO duplicado (variante '%s')", variant);
                    continue;
                }

                if (covered_ordo && variant_index < ordo_variant_count) covered_ordo[variant_index] = true;
                if (j == 0 && case_node->literal_count == 1) {
                    case_node->resolved_ordo_variant = resolved_variant;
                }
                continue;
            }

            cct_sem_type_t *lit_type = sem_analyze_expr(sem, literal);

            if (!sem_quando_types_compatible(expr_type, lit_type)) {
                if (!sem_is_error_type(expr_type) && !sem_is_error_type(lit_type)) {
                    sem_report_node(sem, literal, "QUANDO: CASO tipo incompativel com expressao");
                }
                continue;
            }

            if (expr_is_string) {
                if (literal->type != AST_LITERAL_STRING) {
                    sem_report_node(sem, literal, "QUANDO: CASO sobre VERBUM requer literal de string");
                    continue;
                }
                const char *literal_text = literal->as.literal_string.string_value ? literal->as.literal_string.string_value : "";
                bool duplicate = false;
                for (size_t k = 0; k < seen_strings_count; k++) {
                    if (strcmp(seen_strings[k], literal_text) == 0) {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate) {
                    sem_report_node(sem, literal, "QUANDO: CASO duplicado");
                    continue;
                }
                if (seen_strings_count >= seen_strings_capacity) {
                    size_t next_cap = (seen_strings_capacity == 0) ? 8 : (seen_strings_capacity * 2);
                    const char **next = (const char**)realloc(seen_strings, next_cap * sizeof(const char*));
                    if (!next) {
                        fprintf(stderr, "cct: fatal: out of memory\n");
                        exit(CCT_ERROR_OUT_OF_MEMORY);
                    }
                    seen_strings = next;
                    seen_strings_capacity = next_cap;
                }
                seen_strings[seen_strings_count++] = literal_text;
                continue;
            }

            long long literal_value = 0;
            if (!sem_quando_eval_int_literal(literal, &literal_value)) {
                sem_report_node(sem, literal, "QUANDO: CASO requer literal inteiro ou booleano");
                continue;
            }

            bool duplicate = false;
            for (size_t k = 0; k < seen_count; k++) {
                if (seen_values[k] == literal_value) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                sem_report_node(sem, literal, "QUANDO: CASO duplicado");
                continue;
            }

            if (seen_count >= seen_capacity) {
                size_t next_cap = (seen_capacity == 0) ? 8 : (seen_capacity * 2);
                long long *next = (long long*)realloc(seen_values, next_cap * sizeof(long long));
                if (!next) {
                    fprintf(stderr, "cct: fatal: out of memory\n");
                    exit(CCT_ERROR_OUT_OF_MEMORY);
                }
                seen_values = next;
                seen_capacity = next_cap;
            }
            seen_values[seen_count++] = literal_value;
        }

        if (expr_is_ordo && case_node->literal_count == 1) {
            const cct_ast_node_t *first = case_node->literals[0];
            const char *variant_name = (first && first->type == AST_IDENTIFIER && first->as.identifier.name)
                ? first->as.identifier.name
                : "<variante>";
            cct_ast_ordo_variant_t *resolved_variant = case_node->resolved_ordo_variant
                ? case_node->resolved_ordo_variant
                : sem_find_ordo_variant(ordo_decl, variant_name, NULL);

            if (!resolved_variant) {
                /* variant-exists error already reported above */
            } else if (resolved_variant->field_count == 0 && case_node->binding_count > 0) {
                sem_report_nodef(
                    sem,
                    first,
                    "QUANDO ORDO: variante '%s' nao tem payload; remova os bindings",
                    variant_name
                );
            } else if (case_node->binding_count != resolved_variant->field_count) {
                sem_report_nodef(
                    sem,
                    first,
                    "QUANDO ORDO: CASO %s com %zu bindings, variante tem %zu campos",
                    variant_name,
                    case_node->binding_count,
                    resolved_variant->field_count
                );
            } else if (case_node->binding_count > 0) {
                sem_push_scope(sem, CCT_SEM_SCOPE_BLOCK);
                for (size_t b = 0; b < case_node->binding_count; b++) {
                    cct_ast_ordo_field_t *field = resolved_variant->fields[b];
                    cct_sem_type_t *field_type = field
                        ? sem_resolve_ast_type(sem, field->type, field->line, field->column)
                        : &sem->type_error;
                    const char *binding_name = case_node->binding_names[b];
                    cct_sem_symbol_t *sym = sem_define_symbol(
                        sem,
                        CCT_SEM_SYMBOL_VARIABLE,
                        binding_name,
                        first ? first->line : stmt->line,
                        first ? first->column : stmt->column
                    );
                    if (sym) sym->type = field_type;
                }
                if (case_node->body) sem_analyze_stmt(sem, case_node->body);
                sem_pop_scope(sem);
                case_body_checked = true;
            }
        } else if (expr_is_ordo && case_node->binding_count > 0) {
            const cct_ast_node_t *first = (case_node->literal_count > 0) ? case_node->literals[0] : stmt;
            if (case_node->literal_count != 1) {
                sem_report_node(sem, first ? first : stmt, "QUANDO ORDO: OR-cases com payload nao sao suportados nesta fase");
            }
        } else if (!expr_is_ordo && case_node->binding_count > 0) {
            sem_report_node(sem, stmt, "QUANDO: bindings em CASO so sao validos para ORDO");
        }

        if (case_node->body && !case_body_checked) {
            sem_analyze_stmt(sem, case_node->body);
        }
    }

    free(seen_values);
    free(seen_strings);

    if (expr_is_ordo && !stmt->as.quando.else_body && ordo_decl) {
        char missing[512];
        missing[0] = '\0';

        for (size_t i = 0; i < ordo_variant_count; i++) {
            if (covered_ordo && covered_ordo[i]) continue;
            const char *missing_name = NULL;
            if (ordo_decl->as.ordo.variants && i < ordo_decl->as.ordo.variants->count) {
                cct_ast_ordo_variant_t *variant = ordo_decl->as.ordo.variants->variants[i];
                if (variant && variant->name) missing_name = variant->name;
            } else if (ordo_decl->as.ordo.items && i < ordo_decl->as.ordo.items->count) {
                cct_ast_enum_item_t *item = ordo_decl->as.ordo.items->items[i];
                if (item && item->name) missing_name = item->name;
            }
            if (!missing_name) continue;

            if (missing[0] != '\0') {
                strncat(missing, " ", sizeof(missing) - strlen(missing) - 1);
            }
            strncat(missing, missing_name, sizeof(missing) - strlen(missing) - 1);
        }

        if (missing[0] != '\0') {
            sem_report_nodef(sem, stmt, "QUANDO ORDO: variantes nao-exaustivas - falta %s", missing);
        }
    }
    free(covered_ordo);

    if (stmt->as.quando.else_body) {
        sem_analyze_stmt(sem, stmt->as.quando.else_body);
    } else if (expr_supported && !expr_is_ordo) {
        sem_warn_at(
            sem,
            stmt->line,
            stmt->column,
            "QUANDO sem SENAO: comportamento indefinido para valores nao cobertos",
            NULL
        );
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

        case AST_QUANDO:
            sem_analyze_quando(sem, stmt);
            return;

        case AST_DUM:
            sem_analyze_condition(sem, stmt->as.dum.condition);
            sem->loop_depth++;
            sem_analyze_stmt(sem, stmt->as.dum.body);
            if (sem->loop_depth > 0) sem->loop_depth--;
            return;

        case AST_DONEC:
            sem->loop_depth++;
            sem_analyze_stmt(sem, stmt->as.donec.body);
            if (sem->loop_depth > 0) sem->loop_depth--;
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
            sem->loop_depth++;
            sem_analyze_stmt(sem, stmt->as.repete.body);
            if (sem->loop_depth > 0) sem->loop_depth--;
            sem_pop_scope(sem);
            return;
        }

        case AST_ITERUM: {
            cct_sem_type_t *collection_t = sem_analyze_expr(sem, stmt->as.iterum.collection);
            cct_sem_type_t *item_t = &sem->type_error;
            cct_sem_type_t *value_t = &sem->type_error;

            cct_sem_iter_collection_kind_t inferred_kind = CCT_SEM_ITER_COLLECTION_NONE;
            cct_sem_type_t *inferred_key_t = NULL;
            cct_sem_type_t *inferred_value_t = NULL;
            sem_infer_iter_collection_from_expr(
                sem,
                stmt->as.iterum.collection,
                &inferred_kind,
                &inferred_key_t,
                &inferred_value_t
            );

            bool is_map_iter = false;
            bool is_set_iter = false;
            bool is_series_iter = false;
            bool is_fluxus_iter = false;

            if (collection_t && collection_t->kind == CCT_SEM_TYPE_ARRAY && collection_t->element) {
                is_series_iter = true;
                item_t = collection_t->element;
            } else if (inferred_kind == CCT_SEM_ITER_COLLECTION_MAP) {
                is_map_iter = true;
                item_t = inferred_key_t ? inferred_key_t : &sem->type_rex;
                value_t = inferred_value_t ? inferred_value_t : &sem->type_rex;
            } else if (inferred_kind == CCT_SEM_ITER_COLLECTION_SET) {
                is_set_iter = true;
                item_t = inferred_key_t ? inferred_key_t : &sem->type_rex;
            } else if (collection_t && collection_t->kind == CCT_SEM_TYPE_POINTER &&
                       collection_t->element && collection_t->element->kind == CCT_SEM_TYPE_NIHIL) {
                is_fluxus_iter = true;
                if (sem_profile_is_freestanding(sem)) {
                    sem_report_node(sem, stmt->as.iterum.collection,
                                    "FLUXUS não suportado em perfil freestanding (heap indisponível)");
                }
                if (inferred_kind == CCT_SEM_ITER_COLLECTION_FLUXUS && inferred_value_t) {
                    item_t = inferred_value_t;
                } else {
                    /* FASE 12D.3 baseline: opaque FLUXUS iteration item defaults to REX. */
                    item_t = &sem->type_rex;
                }
            } else if (!sem_is_error_type(collection_t)) {
                sem_report_nodef(
                    sem,
                    stmt->as.iterum.collection,
                    "ITERUM requires FLUXUS or SERIES (and now map/set), found: %s",
                    cct_sem_type_string(collection_t)
                );
            }

            if (is_map_iter) {
                if (!stmt->as.iterum.value_name || !stmt->as.iterum.value_name[0]) {
                    sem_report_node(sem, stmt, "ITERUM sobre map requer 2 variaveis (chave, valor)");
                    return;
                }
            } else if (stmt->as.iterum.value_name && stmt->as.iterum.value_name[0]) {
                if (is_set_iter) {
                    sem_report_node(sem, stmt, "ITERUM sobre set requer 1 variavel (elemento)");
                } else if (is_fluxus_iter || is_series_iter) {
                    sem_report_node(sem, stmt, "ITERUM sobre FLUXUS/SERIES requer 1 variavel");
                } else {
                    sem_report_node(sem, stmt, "ITERUM com 2 variaveis so e suportado para map");
                }
                return;
            }

            sem_push_scope(sem, CCT_SEM_SCOPE_LOOP);
            cct_sem_symbol_t *iter_item = sem_define_symbol(
                sem,
                CCT_SEM_SYMBOL_VARIABLE,
                stmt->as.iterum.item_name ? stmt->as.iterum.item_name : "_",
                stmt->line,
                stmt->column
            );
            if (iter_item) iter_item->type = item_t;

            if (is_map_iter) {
                cct_sem_symbol_t *iter_value = sem_define_symbol(
                    sem,
                    CCT_SEM_SYMBOL_VARIABLE,
                    stmt->as.iterum.value_name,
                    stmt->line,
                    stmt->column
                );
                if (iter_value) iter_value->type = value_t;
            }

            sem->loop_depth++;
            sem_analyze_stmt(sem, stmt->as.iterum.body);
            if (sem->loop_depth > 0) sem->loop_depth--;
            sem_pop_scope(sem);
            return;
        }

        case AST_FRANGE:
            if (sem->loop_depth == 0) {
                sem_report_node(sem, stmt, "FRANGE outside loop context");
            }
            return;

        case AST_RECEDE:
            if (sem->loop_depth == 0) {
                sem_report_node(sem, stmt, "RECEDE outside loop context");
            }
            return;

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

static const cct_ast_node_t* sem_resolve_ordo_decl_from_type(
    cct_semantic_analyzer_t *sem,
    const cct_sem_type_t *type
) {
    if (!sem || !type || type->kind != CCT_SEM_TYPE_NAMED || !type->name) return NULL;
    cct_sem_symbol_t *type_sym = sem_find_type_symbol(sem, type->name);
    if (!type_sym || !type_sym->type_decl || type_sym->type_decl->type != AST_ORDO) return NULL;
    return type_sym->type_decl;
}

static cct_ast_ordo_variant_t* sem_find_ordo_variant(
    const cct_ast_node_t *ordo_decl,
    const char *variant_name,
    size_t *index_out
) {
    if (!ordo_decl || ordo_decl->type != AST_ORDO || !variant_name) return NULL;
    if (!ordo_decl->as.ordo.variants) return NULL;
    for (size_t i = 0; i < ordo_decl->as.ordo.variants->count; i++) {
        cct_ast_ordo_variant_t *variant = ordo_decl->as.ordo.variants->variants[i];
        if (!variant || !variant->name) continue;
        if (strcmp(variant->name, variant_name) == 0) {
            if (index_out) *index_out = i;
            return variant;
        }
    }
    return NULL;
}

static bool sem_ordo_decl_has_payload_fields(const cct_ast_node_t *ordo_decl) {
    if (!ordo_decl || ordo_decl->type != AST_ORDO || !ordo_decl->as.ordo.variants) return false;
    for (size_t i = 0; i < ordo_decl->as.ordo.variants->count; i++) {
        cct_ast_ordo_variant_t *variant = ordo_decl->as.ordo.variants->variants[i];
        if (variant && variant->field_count > 0) return true;
    }
    return false;
}

static bool sem_identifier_is_any_ordo_variant(cct_semantic_analyzer_t *sem, const char *name) {
    if (!sem || !name || !sem->global_scope) return false;
    for (cct_sem_symbol_t *sym = sem->global_scope->symbols; sym; sym = sym->next) {
        if (!sym->type_decl || sym->type_decl->type != AST_ORDO) continue;
        if (!sem_ordo_decl_has_payload_fields(sym->type_decl)) continue;
        if (sem_find_ordo_variant(sym->type_decl, name, NULL)) return true;
    }
    return false;
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

                if (node->type == AST_ORDO) {
                    cct_ast_node_t *mutable = (cct_ast_node_t*)node;
                    bool has_payload = false;

                    if (node->as.ordo.variants && node->as.ordo.variants->count > 0) {
                        for (size_t i = 0; i < node->as.ordo.variants->count; i++) {
                            cct_ast_ordo_variant_t *variant = node->as.ordo.variants->variants[i];
                            if (!variant) continue;

                            variant->tag_value = (i64)i;

                            if (variant->name && variant->name[0] &&
                                (variant->name[0] < 'A' || variant->name[0] > 'Z')) {
                                char warn[256];
                                snprintf(warn, sizeof(warn),
                                         "ORDO: variante '%s' deve comecar com letra maiuscula",
                                         variant->name);
                                sem_warn_at(sem, variant->line, variant->column, warn, NULL);
                            }

                            cct_sem_symbol_t *es = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                                                     variant->name, variant->line, variant->column);
                            if (es) es->type = sym->type;

                            for (size_t f = 0; f < variant->field_count; f++) {
                                cct_ast_ordo_field_t *field = variant->fields[f];
                                if (!field) continue;
                                if (sem_is_c_reserved_keyword(field->name)) {
                                    char warn[256];
                                    snprintf(warn, sizeof(warn),
                                             "ORDO payload: campo '%s' coincide com keyword C reservada",
                                             field->name ? field->name : "<campo>");
                                    sem_warn_at(sem, field->line, field->column, warn, NULL);
                                }
                            }

                            if (variant->field_count > 0) has_payload = true;
                        }
                    } else if (node->as.ordo.items) {
                        for (size_t i = 0; i < node->as.ordo.items->count; i++) {
                            cct_ast_enum_item_t *item = node->as.ordo.items->items[i];
                            if (!item) continue;
                            cct_sem_symbol_t *es = sem_define_symbol(sem, CCT_SEM_SYMBOL_VARIABLE,
                                                                     item->name, item->line, item->column);
                            if (es) es->type = sym->type;
                        }
                    }

                    mutable->as.ordo.has_payload = has_payload;
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

static void sem_validate_named_type_definitions_in_node(cct_semantic_analyzer_t *sem, const cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_SIGILLUM: {
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
            return;
        }

        case AST_ORDO: {
            if (node->as.ordo.variants) {
                for (size_t i = 0; i < node->as.ordo.variants->count; i++) {
                    cct_ast_ordo_variant_t *variant = node->as.ordo.variants->variants[i];
                    if (!variant) continue;
                    for (size_t f = 0; f < variant->field_count; f++) {
                        cct_ast_ordo_field_t *field = variant->fields[f];
                        if (!field) continue;
                        cct_sem_type_t *ft = sem_resolve_ast_type(sem, field->type, field->line, field->column);
                        if (!sem_is_error_type(ft) && !sem_is_ordo_payload_supported_type(ft)) {
                            sem_reportf(
                                sem,
                                field->line,
                                field->column,
                                "ORDO payload: tipo %s nao suportado em payload nesta versao",
                                cct_sem_type_string(ft)
                            );
                        }
                    }
                }
            }
            return;
        }

        case AST_CODEX:
            sem_walk_node_list(node->as.codex.declarations, sem_validate_named_type_definitions_in_node, sem);
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
            if (psym) {
                psym->type = ptype;
                psym->is_constans = param->is_constans;
            }
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

void cct_semantic_init(cct_semantic_analyzer_t *sem, const char *filename, cct_profile_t profile) {
    memset(sem, 0, sizeof(*sem));
    sem->filename = filename;
    sem->profile = profile;
    sem_init_builtin_types(sem);

    sem_push_scope(sem, CCT_SEM_SCOPE_GLOBAL);
}

bool cct_semantic_analyze_program(cct_semantic_analyzer_t *sem, const cct_ast_program_t *program) {
    if (!sem || !program) return false;

    /* Pass 1A: register named types that may be referenced by ritual signatures. */
    sem_walk_node_list(program->declarations, sem_register_named_types_in_node, sem);

    /* Pass 1B: validate SIGILLUM/ORDO internals once all named types are known. */
    sem_walk_node_list(program->declarations, sem_validate_named_type_definitions_in_node, sem);

    /* Pass 1C: register contracts (PACTUM) globally. */
    sem_walk_node_list(program->declarations, sem_register_global_pacta_in_node, sem);

    /* Pass 1D: register rituals (functions) globally. */
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
