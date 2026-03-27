/*
 * CCT — Clavicula Turing
 * Code Generator Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "codegen.h"
#include "codegen_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifndef CCT_FREESTANDING_RT_SOURCE
#define CCT_FREESTANDING_RT_SOURCE "src/runtime/cct_freestanding_rt.c"
#endif

struct cct_codegen_string {
    char *value;                    /* decoded string contents from AST */
    char *label;                    /* cct_str_N */
    cct_codegen_string_t *next;
};

typedef enum {
    CCT_CODEGEN_ITER_COLLECTION_NONE = 0,
    CCT_CODEGEN_ITER_COLLECTION_FLUXUS,
    CCT_CODEGEN_ITER_COLLECTION_MAP,
    CCT_CODEGEN_ITER_COLLECTION_SET,
} cct_codegen_iter_collection_kind_t;

struct cct_codegen_local {
    char *name;
    u32 scope_depth;
    cct_codegen_local_kind_t kind;
    const cct_ast_type_t *ast_type;
    cct_codegen_iter_collection_kind_t iter_collection_kind;
    const cct_ast_type_t *iter_key_ast_type;
    const cct_ast_type_t *iter_value_ast_type;
    cct_codegen_local_t *next;
};

struct cct_codegen_rituale {
    const cct_ast_node_t *node; /* AST_RITUALE */
    char *name;                 /* source ritual name */
    char *c_name;               /* emitted C function name */
    cct_codegen_value_kind_t return_kind;
    cct_codegen_local_kind_t *param_kinds;
    size_t param_count;
    bool returns_nihil;
    cct_codegen_rituale_t *next;
};

typedef struct cct_codegen_sigillum {
    const cct_ast_node_t *node; /* AST_SIGILLUM */
    char *name;
    struct cct_codegen_sigillum *next;
} cct_codegen_sigillum_t;

typedef struct cct_codegen_ordo {
    const cct_ast_node_t *node; /* AST_ORDO */
    char *name;
    struct cct_codegen_ordo *next;
} cct_codegen_ordo_t;

typedef struct cct_codegen_fail_handler {
    char *catch_label;
    struct cct_codegen_fail_handler *next;
} cct_codegen_fail_handler_t;

typedef struct cct_codegen_genus_inst {
    bool is_rituale;
    char *key;
    char *special_name;
    cct_ast_node_t *materialized_node; /* Owned by codegen runtime */
    bool scanned;
    struct cct_codegen_genus_inst *next;
} cct_codegen_genus_inst_t;

/* ========================================================================
 * Helpers: allocation, strings, errors
 * ======================================================================== */

static void* cg_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return ptr;
}

static char* cg_strdup(const char *s) {
    if (!s) return NULL;
    char *copy = strdup(s);
    if (!copy) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return copy;
}

static char* cg_strdup_printf(const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return cg_strdup(buffer);
}

static void cg_report(cct_codegen_t *cg, u32 line, u32 col, const char *msg) {
    cg->had_error = true;
    cg->error_count++;
    cct_error_at_location(CCT_ERROR_CODEGEN, cg->filename ? cg->filename : "<unknown>", line, col, msg);
}

static void cg_reportf(cct_codegen_t *cg, u32 line, u32 col, const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    cg_report(cg, line, col, buffer);
}

static void cg_report_node(cct_codegen_t *cg, const cct_ast_node_t *node, const char *msg) {
    cg_report(cg, node ? node->line : 0, node ? node->column : 0, msg);
}

static void cg_report_nodef(cct_codegen_t *cg, const cct_ast_node_t *node, const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    cg_report_node(cg, node, buffer);
}

static bool cg_push_fail_handler(cct_codegen_t *cg, const char *catch_label) {
    cct_codegen_fail_handler_t *h = (cct_codegen_fail_handler_t*)cg_calloc(1, sizeof(*h));
    h->catch_label = cg_strdup(catch_label);
    h->next = cg->fail_handlers;
    cg->fail_handlers = h;
    return true;
}

static void cg_pop_fail_handler(cct_codegen_t *cg) {
    if (!cg || !cg->fail_handlers) return;
    cct_codegen_fail_handler_t *h = cg->fail_handlers;
    cg->fail_handlers = h->next;
    free(h->catch_label);
    free(h);
}

static const char* cg_current_fail_handler_label(const cct_codegen_t *cg) {
    return (cg && cg->fail_handlers) ? cg->fail_handlers->catch_label : NULL;
}

/* ========================================================================
 * Helpers: path derivation
 * ======================================================================== */

static bool ends_with(const char *s, const char *suffix) {
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    return n >= m && strcmp(s + n - m, suffix) == 0;
}

static char* derive_default_output_executable(const char *input_path) {
    size_t len = strlen(input_path);
    if (ends_with(input_path, ".cct")) {
        char *out = (char*)cg_calloc(1, len - 4 + 1);
        memcpy(out, input_path, len - 4);
        out[len - 4] = '\0';
        return out;
    }
    return cg_strdup(input_path);
}

static char* derive_intermediate_c_path(const char *output_exe_path) {
    size_t len = strlen(output_exe_path);
    const char *suffix = ".cgen.c";
    size_t sfx_len = strlen(suffix);
    char *path = (char*)cg_calloc(1, len + sfx_len + 1);
    memcpy(path, output_exe_path, len);
    memcpy(path + len, suffix, sfx_len);
    path[len + sfx_len] = '\0';
    return path;
}

/* ========================================================================
 * Helpers: string pool and escaping
 * ======================================================================== */

static void cg_emit_c_escaped_string(FILE *out, const char *s) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        switch (*p) {
            case '\\': fputs("\\\\", out); break;
            case '"':  fputs("\\\"", out); break;
            case '\n': fputs("\\n", out); break;
            case '\t': fputs("\\t", out); break;
            case '\r': fputs("\\r", out); break;
            default:
                if (*p < 32 || *p > 126) {
                    fprintf(out, "\\x%02x", *p);
                } else {
                    fputc(*p, out);
                }
                break;
        }
    }
    fputc('"', out);
}

/*
 * Parser/AST currently stores string contents with escape sequences preserved.
 * Decode once into runtime bytes before pooling, then re-escape for generated C.
 */
static char* cg_decode_cct_string(const char *raw) {
    if (!raw) return cg_strdup("");

    size_t len = strlen(raw);
    char *decoded = (char*)cg_calloc(len + 1, 1);
    size_t j = 0;

    for (size_t i = 0; i < len; i++) {
        char c = raw[i];
        if (c == '\\' && (i + 1) < len) {
            char n = raw[++i];
            switch (n) {
                case 'n': decoded[j++] = '\n'; break;
                case 't': decoded[j++] = '\t'; break;
                case 'r': decoded[j++] = '\r'; break;
                case '\\': decoded[j++] = '\\'; break;
                case '"': decoded[j++] = '"'; break;
                default:
                    decoded[j++] = n;
                    break;
            }
        } else {
            decoded[j++] = c;
        }
    }

    decoded[j] = '\0';
    return decoded;
}

static const char* cg_string_label_for_value(cct_codegen_t *cg, const char *value) {
    char *decoded = cg_decode_cct_string(value);

    for (cct_codegen_string_t *s = cg->strings; s; s = s->next) {
        if (strcmp(s->value, decoded) == 0) {
            free(decoded);
            return s->label;
        }
    }

    cct_codegen_string_t *node = (cct_codegen_string_t*)cg_calloc(1, sizeof(*node));
    node->value = decoded;

    char label[64];
    snprintf(label, sizeof(label), "cct_str_%u", cg->next_string_id++);
    node->label = cg_strdup(label);

    node->next = cg->strings;
    cg->strings = node;
    return node->label;
}

/* ========================================================================
 * Helpers: locals (FASE 4A subset)
 * ======================================================================== */

static void cg_reset_locals(cct_codegen_t *cg) {
    cct_codegen_local_t *it = cg->locals;
    while (it) {
        cct_codegen_local_t *next = it->next;
        free(it->name);
        free(it);
        it = next;
    }
    cg->locals = NULL;
    cg->scope_depth = 0;
}

static cct_codegen_local_t* cg_find_local(cct_codegen_t *cg, const char *name) {
    for (cct_codegen_local_t *l = cg->locals; l; l = l->next) {
        if (strcmp(l->name, name) == 0) return l;
    }
    return NULL;
}

static void cg_push_scope(cct_codegen_t *cg) {
    cg->scope_depth++;
}

static void cg_pop_scope(cct_codegen_t *cg) {
    cct_codegen_local_t *it = cg->locals;
    while (it && it->scope_depth == cg->scope_depth) {
        cct_codegen_local_t *next = it->next;
        free(it->name);
        free(it);
        it = next;
    }
    cg->locals = it;
    if (cg->scope_depth > 0) cg->scope_depth--;
}

static bool cg_define_local(
    cct_codegen_t *cg,
    const char *name,
    cct_codegen_local_kind_t kind,
    const cct_ast_type_t *ast_type,
    const cct_ast_node_t *node
) {
    for (cct_codegen_local_t *l = cg->locals; l; l = l->next) {
        if (l->scope_depth != cg->scope_depth) break;
        if (strcmp(l->name, name) == 0) {
            cg_report_nodef(cg, node, "local variable '%s' already defined in current scope (FASE 4C backend)", name);
            return false;
        }
    }

    cct_codegen_local_t *l = (cct_codegen_local_t*)cg_calloc(1, sizeof(*l));
    l->name = cg_strdup(name);
    l->scope_depth = cg->scope_depth;
    l->kind = kind;
    l->ast_type = ast_type;
    l->next = cg->locals;
    cg->locals = l;
    return true;
}

static void cg_set_local_iter_collection_info(
    cct_codegen_t *cg,
    const char *name,
    cct_codegen_iter_collection_kind_t kind,
    const cct_ast_type_t *key_ast_type,
    const cct_ast_type_t *value_ast_type
) {
    cct_codegen_local_t *local = cg_find_local(cg, name);
    if (!local) return;
    local->iter_collection_kind = kind;
    local->iter_key_ast_type = key_ast_type;
    local->iter_value_ast_type = value_ast_type;
}

static bool cg_extract_iter_collection_from_coniura(
    const cct_ast_node_t *expr,
    cct_codegen_iter_collection_kind_t *kind_out,
    const cct_ast_type_t **key_ast_type_out,
    const cct_ast_type_t **value_ast_type_out
) {
    if (!expr || expr->type != AST_CONIURA || !expr->as.coniura.name) return false;

    const char *name = expr->as.coniura.name;
    const cct_ast_type_list_t *type_args = expr->as.coniura.type_args;
    if (strcmp(name, "map_init") == 0 || strcmp(name, "map_copy") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_MAP;
        if (key_ast_type_out) {
            *key_ast_type_out = (type_args && type_args->count >= 1) ? type_args->types[0] : NULL;
        }
        if (value_ast_type_out) {
            *value_ast_type_out = (type_args && type_args->count >= 2) ? type_args->types[1] : NULL;
        }
        return true;
    }

    if (strcmp(name, "set_init") == 0 || strcmp(name, "set_copy") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_SET;
        if (key_ast_type_out) {
            *key_ast_type_out = (type_args && type_args->count >= 1) ? type_args->types[0] : NULL;
        }
        if (value_ast_type_out) {
            *value_ast_type_out = (type_args && type_args->count >= 1) ? type_args->types[0] : NULL;
        }
        return true;
    }

    if (strcmp(name, "fluxus_init") == 0 ||
        strcmp(name, "fluxus_copy") == 0 ||
        strcmp(name, "fluxus_concat") == 0 ||
        strcmp(name, "fluxus_slice") == 0 ||
        strcmp(name, "fluxus_map") == 0 ||
        strcmp(name, "fluxus_filter") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_FLUXUS;
        if (key_ast_type_out) *key_ast_type_out = NULL;
        if (value_ast_type_out) *value_ast_type_out = NULL;
        return true;
    }

    if ((strcmp(name, "map_keys") == 0 || strcmp(name, "map_values") == 0) &&
        type_args && type_args->count >= 2) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_FLUXUS;
        if (key_ast_type_out) *key_ast_type_out = NULL;
        if (value_ast_type_out) {
            *value_ast_type_out = strcmp(name, "map_keys") == 0 ? type_args->types[0] : type_args->types[1];
        }
        return true;
    }

    return false;
}

static bool cg_extract_iter_collection_from_obsecro(
    const cct_ast_node_t *expr,
    cct_codegen_iter_collection_kind_t *kind_out
) {
    if (!expr || expr->type != AST_OBSECRO || !expr->as.obsecro.name) return false;

    const char *name = expr->as.obsecro.name;
    if (strcmp(name, "map_init") == 0 || strcmp(name, "map_copy") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_MAP;
        return true;
    }
    if (strcmp(name, "set_init") == 0 || strcmp(name, "set_copy") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_SET;
        return true;
    }
    if (strcmp(name, "fluxus_init") == 0) {
        if (kind_out) *kind_out = CCT_CODEGEN_ITER_COLLECTION_FLUXUS;
        return true;
    }
    return false;
}

static void cg_infer_iter_collection_from_expr(
    cct_codegen_t *cg,
    const cct_ast_node_t *expr,
    cct_codegen_iter_collection_kind_t *kind_out,
    const cct_ast_type_t **key_ast_type_out,
    const cct_ast_type_t **value_ast_type_out
) {
    cct_codegen_iter_collection_kind_t kind = CCT_CODEGEN_ITER_COLLECTION_NONE;
    const cct_ast_type_t *key_type = NULL;
    const cct_ast_type_t *value_type = NULL;

    if (!expr) {
        if (kind_out) *kind_out = kind;
        if (key_ast_type_out) *key_ast_type_out = key_type;
        if (value_ast_type_out) *value_ast_type_out = value_type;
        return;
    }

    if (expr->type == AST_IDENTIFIER && expr->as.identifier.name) {
        cct_codegen_local_t *local = cg_find_local(cg, expr->as.identifier.name);
        if (local) {
            kind = local->iter_collection_kind;
            key_type = local->iter_key_ast_type;
            value_type = local->iter_value_ast_type;
            if (kind == CCT_CODEGEN_ITER_COLLECTION_NONE &&
                local->ast_type &&
                local->ast_type->is_pointer &&
                local->ast_type->element_type &&
                local->ast_type->element_type->name &&
                strcmp(local->ast_type->element_type->name, "NIHIL") == 0) {
                kind = CCT_CODEGEN_ITER_COLLECTION_FLUXUS;
            }
        }
    }

    if (kind == CCT_CODEGEN_ITER_COLLECTION_NONE) {
        if (!cg_extract_iter_collection_from_coniura(expr, &kind, &key_type, &value_type)) {
            (void)cg_extract_iter_collection_from_obsecro(expr, &kind);
        }
    }

    if (kind_out) *kind_out = kind;
    if (key_ast_type_out) *key_ast_type_out = key_type;
    if (value_ast_type_out) *value_ast_type_out = value_type;
}

/* ========================================================================
 * AST subset discovery / validation helpers
 * ======================================================================== */

static cct_codegen_local_kind_t cg_local_kind_from_ast_type(const cct_ast_type_t *type) {
    if (!type) return CCT_CODEGEN_LOCAL_UNSUPPORTED;
    if (type->is_pointer) {
        const cct_ast_type_t *elem = type->element_type;
        if (!elem || elem->is_pointer || elem->is_array || !elem->name) return CCT_CODEGEN_LOCAL_UNSUPPORTED;
        if (strcmp(elem->name, "VERBUM") == 0) {
            return CCT_CODEGEN_LOCAL_UNSUPPORTED; /* keep outside subset for now */
        }
        if (strcmp(elem->name, "REX") == 0 ||
            strcmp(elem->name, "DUX") == 0 ||
            strcmp(elem->name, "COMES") == 0 ||
            strcmp(elem->name, "MILES") == 0 ||
            strcmp(elem->name, "VERUM") == 0 ||
            strcmp(elem->name, "UMBRA") == 0 ||
            strcmp(elem->name, "FLAMMA") == 0 ||
            strcmp(elem->name, "NIHIL") == 0 ||
            /* named types (e.g. SIGILLUM) validated later against registry */
            (strcmp(elem->name, "REX") != 0 &&
             strcmp(elem->name, "DUX") != 0 &&
             strcmp(elem->name, "COMES") != 0 &&
             strcmp(elem->name, "MILES") != 0 &&
             strcmp(elem->name, "VERUM") != 0 &&
             strcmp(elem->name, "UMBRA") != 0 &&
             strcmp(elem->name, "FLAMMA") != 0 &&
             strcmp(elem->name, "NIHIL") != 0)) {
            return CCT_CODEGEN_LOCAL_POINTER;
        }
        return CCT_CODEGEN_LOCAL_UNSUPPORTED;
    }

    const char *name = type->name;
    if (type->is_array) {
        if (!type->element_type || !type->element_type->name) return CCT_CODEGEN_LOCAL_UNSUPPORTED;
        name = type->element_type->name;
    }
    if (!name) return CCT_CODEGEN_LOCAL_UNSUPPORTED;

    bool is_int =
        strcmp(name, "REX") == 0 ||
        strcmp(name, "DUX") == 0 ||
        strcmp(name, "COMES") == 0 ||
        strcmp(name, "MILES") == 0;
    bool is_bool = strcmp(name, "VERUM") == 0;
    bool is_string = strcmp(name, "VERBUM") == 0;
    bool is_fractum = strcmp(name, "FRACTUM") == 0;
    bool is_umbra = strcmp(name, "UMBRA") == 0;
    bool is_flamma = strcmp(name, "FLAMMA") == 0;

    if (type->is_array) {
        if (is_int) return CCT_CODEGEN_LOCAL_ARRAY_INT;
        if (is_bool) return CCT_CODEGEN_LOCAL_ARRAY_BOOL;
        if (is_string) return CCT_CODEGEN_LOCAL_ARRAY_STRING;
        if (is_umbra) return CCT_CODEGEN_LOCAL_ARRAY_UMBRA;
        if (is_flamma) return CCT_CODEGEN_LOCAL_ARRAY_FLAMMA;
        return CCT_CODEGEN_LOCAL_UNSUPPORTED;
    }

    if (is_int) return CCT_CODEGEN_LOCAL_INT;
    if (is_bool) return CCT_CODEGEN_LOCAL_BOOL;
    if (is_string || is_fractum) return CCT_CODEGEN_LOCAL_STRING;
    if (is_umbra) return CCT_CODEGEN_LOCAL_UMBRA;
    if (is_flamma) return CCT_CODEGEN_LOCAL_FLAMMA;

    /* Named types are handled later (SIGILLUM / ORDO) after registry exists. */
    return CCT_CODEGEN_LOCAL_STRUCT;
}

static bool cg_is_entry_rituale_name(const char *name) {
    return name && (strcmp(name, "principium") == 0 || strcmp(name, "main") == 0);
}

static cct_codegen_value_kind_t cg_value_kind_from_local_kind(cct_codegen_local_kind_t kind) {
    switch (kind) {
        case CCT_CODEGEN_LOCAL_INT: return CCT_CODEGEN_VALUE_INT;
        case CCT_CODEGEN_LOCAL_BOOL: return CCT_CODEGEN_VALUE_BOOL;
        case CCT_CODEGEN_LOCAL_STRING: return CCT_CODEGEN_VALUE_STRING;
        case CCT_CODEGEN_LOCAL_UMBRA:
        case CCT_CODEGEN_LOCAL_FLAMMA: return CCT_CODEGEN_VALUE_REAL;
        case CCT_CODEGEN_LOCAL_POINTER: return CCT_CODEGEN_VALUE_POINTER;
        case CCT_CODEGEN_LOCAL_ARRAY_INT:
        case CCT_CODEGEN_LOCAL_ARRAY_BOOL:
        case CCT_CODEGEN_LOCAL_ARRAY_STRING:
        case CCT_CODEGEN_LOCAL_ARRAY_UMBRA:
        case CCT_CODEGEN_LOCAL_ARRAY_FLAMMA: return CCT_CODEGEN_VALUE_ARRAY;
        case CCT_CODEGEN_LOCAL_STRUCT: return CCT_CODEGEN_VALUE_STRUCT;
        case CCT_CODEGEN_LOCAL_ORDO: return CCT_CODEGEN_VALUE_INT;
        default: return CCT_CODEGEN_VALUE_UNKNOWN;
    }
}

static cct_codegen_value_kind_t cg_value_kind_from_ast_type(const cct_ast_type_t *type) {
    if (!type) return CCT_CODEGEN_VALUE_NIHIL;
    if (type->is_pointer) return CCT_CODEGEN_VALUE_POINTER;
    if (!type->name) return CCT_CODEGEN_VALUE_UNKNOWN;

    if (strcmp(type->name, "NIHIL") == 0) return CCT_CODEGEN_VALUE_NIHIL;
    if (strcmp(type->name, "VERBUM") == 0) return CCT_CODEGEN_VALUE_STRING;
    if (strcmp(type->name, "FRACTUM") == 0) return CCT_CODEGEN_VALUE_STRING;
    if (strcmp(type->name, "VERUM") == 0) return CCT_CODEGEN_VALUE_BOOL;
    if (strcmp(type->name, "UMBRA") == 0 || strcmp(type->name, "FLAMMA") == 0) return CCT_CODEGEN_VALUE_REAL;
    if (strcmp(type->name, "REX") == 0 ||
        strcmp(type->name, "DUX") == 0 ||
        strcmp(type->name, "COMES") == 0 ||
        strcmp(type->name, "MILES") == 0) {
        return CCT_CODEGEN_VALUE_INT;
    }
    if (type->is_array) return CCT_CODEGEN_VALUE_ARRAY;

    return CCT_CODEGEN_VALUE_STRUCT; /* named types/ORDO treated downstream */
}

static cct_codegen_sigillum_t* cg_find_sigillum(cct_codegen_t *cg, const char *name) {
    for (cct_codegen_sigillum_t *s = cg->sigilla; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

static cct_codegen_ordo_t* cg_find_ordo(cct_codegen_t *cg, const char *name) {
    for (cct_codegen_ordo_t *o = cg->ordines; o; o = o->next) {
        if (strcmp(o->name, name) == 0) return o;
    }
    return NULL;
}

static bool cg_ordo_decl_has_payload(const cct_ast_node_t *ordo) {
    if (!ordo || ordo->type != AST_ORDO) return false;
    if (ordo->as.ordo.has_payload) return true;
    if (!ordo->as.ordo.variants) return false;
    for (size_t i = 0; i < ordo->as.ordo.variants->count; i++) {
        cct_ast_ordo_variant_t *variant = ordo->as.ordo.variants->variants[i];
        if (variant && variant->field_count > 0) return true;
    }
    return false;
}

static bool cg_is_payload_ordo_type(cct_codegen_t *cg, const cct_ast_type_t *type) {
    if (!cg || !type || !type->name || type->is_pointer || type->is_array ||
        (type->generic_args && type->generic_args->count > 0)) {
        return false;
    }
    cct_codegen_ordo_t *ordo = cg_find_ordo(cg, type->name);
    return ordo && cg_ordo_decl_has_payload(ordo->node);
}

static cct_ast_ordo_variant_t* cg_find_ordo_variant_decl(
    const cct_ast_node_t *ordo,
    const char *variant_name
) {
    if (!ordo || ordo->type != AST_ORDO || !variant_name) return NULL;
    if (!ordo->as.ordo.variants) return NULL;
    for (size_t i = 0; i < ordo->as.ordo.variants->count; i++) {
        cct_ast_ordo_variant_t *variant = ordo->as.ordo.variants->variants[i];
        if (!variant || !variant->name) continue;
        if (strcmp(variant->name, variant_name) == 0) return variant;
    }
    return NULL;
}

static bool cg_find_ordo_variant_tag(
    const cct_ast_node_t *ordo,
    const char *variant_name,
    i64 *tag_value_out
) {
    if (!ordo || ordo->type != AST_ORDO || !variant_name) return false;

    if (ordo->as.ordo.variants && ordo->as.ordo.variants->count > 0) {
        i64 next_value = 0;
        for (size_t i = 0; i < ordo->as.ordo.variants->count; i++) {
            cct_ast_ordo_variant_t *variant = ordo->as.ordo.variants->variants[i];
            if (!variant || !variant->name) continue;
            i64 tag = variant->has_value ? variant->value : next_value;
            if (strcmp(variant->name, variant_name) == 0) {
                if (tag_value_out) *tag_value_out = tag;
                return true;
            }
            next_value = tag + 1;
        }
        return false;
    }

    cct_ast_enum_item_list_t *items = ordo->as.ordo.items;
    if (!items) return false;
    i64 next_value = 0;
    for (size_t i = 0; i < items->count; i++) {
        cct_ast_enum_item_t *it = items->items[i];
        if (!it || !it->name) continue;
        if (it->has_value) next_value = it->value;
        if (strcmp(it->name, variant_name) == 0) {
            if (tag_value_out) *tag_value_out = next_value;
            return true;
        }
        next_value++;
    }
    return false;
}

static const cct_ast_field_t* cg_find_sigillum_field(const cct_codegen_sigillum_t *sig, const char *field_name) {
    if (!sig || !sig->node || sig->node->type != AST_SIGILLUM || !field_name) return NULL;
    cct_ast_field_list_t *fields = sig->node->as.sigillum.fields;
    if (!fields) return NULL;
    for (size_t i = 0; i < fields->count; i++) {
        cct_ast_field_t *f = fields->fields[i];
        if (f && f->name && strcmp(f->name, field_name) == 0) return f;
    }
    return NULL;
}

static const char* cg_materialize_generic_sigillum_name(
    cct_codegen_t *cg,
    const cct_ast_type_t *inst_type,
    const cct_ast_node_t *at
);

static bool cg_is_known_sigillum_type(cct_codegen_t *cg, const cct_ast_type_t *type) {
    if (!type || !type->name || type->is_array || type->is_pointer) return false;
    if (type->generic_args && type->generic_args->count > 0) {
        const char *special = cg_materialize_generic_sigillum_name(cg, type, NULL);
        return special && cg_find_sigillum(cg, special) != NULL;
    }
    return cg_find_sigillum(cg, type->name) != NULL;
}

static bool cg_is_known_ordo_type(cct_codegen_t *cg, const cct_ast_type_t *type) {
    return type && type->name && !type->is_array && !type->is_pointer &&
           (!type->generic_args || type->generic_args->count == 0) &&
           cg_find_ordo(cg, type->name) != NULL;
}

static const char* cg_c_scalar_type_for_name(const char *name) {
    if (!name) return NULL;
    if (strcmp(name, "REX") == 0 ||
        strcmp(name, "DUX") == 0 ||
        strcmp(name, "COMES") == 0 ||
        strcmp(name, "MILES") == 0) return "long long";
    if (strcmp(name, "VERUM") == 0) return "long long";
    if (strcmp(name, "VERBUM") == 0) return "const char *";
    if (strcmp(name, "FRACTUM") == 0) return "const char *";
    if (strcmp(name, "UMBRA") == 0) return "double";
    if (strcmp(name, "FLAMMA") == 0) return "float";
    return NULL;
}

static cct_codegen_rituale_t* cg_find_rituale(cct_codegen_t *cg, const char *name) {
    for (cct_codegen_rituale_t *r = cg->rituales; r; r = r->next) {
        if (strcmp(r->name, name) == 0) return r;
    }
    return NULL;
}

static char* cg_make_rituale_c_name(const char *name) {
    /* Parser identifiers are already C-compatible for current subset. */
    size_t len = strlen(name);
    const char *prefix = "cct_fn_";
    size_t pfx_len = strlen(prefix);
    char *out = (char*)cg_calloc(1, pfx_len + len + 1);
    memcpy(out, prefix, pfx_len);
    memcpy(out + pfx_len, name, len);
    out[pfx_len + len] = '\0';
    return out;
}

static bool cg_has_explicit_freestanding_entry(const cct_codegen_t *cg) {
    return cg &&
           cg->profile == CCT_PROFILE_FREESTANDING &&
           cg->entry_rituale_name &&
           cg->entry_rituale_name[0] != '\0';
}

static bool cg_is_explicit_entry_name(const cct_codegen_t *cg, const char *rituale_name) {
    return cg_has_explicit_freestanding_entry(cg) &&
           rituale_name &&
           strcmp(rituale_name, cg->entry_rituale_name) == 0;
}

static char* cg_make_sanitized_ident_fragment(const char *raw) {
    const char *src = (raw && raw[0]) ? raw : "module";
    size_t len = strlen(src);
    char *out = (char*)cg_calloc(1, (len * 2) + 2);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)src[i];
        char mapped = (isalnum(ch) || ch == '_') ? (char)ch : '_';
        if (j == 0 && isdigit((unsigned char)mapped)) {
            out[j++] = '_';
        }
        out[j++] = mapped;
    }
    if (j == 0) {
        memcpy(out, "module", 6);
        out[6] = '\0';
        return out;
    }
    out[j] = '\0';
    return out;
}

static char* cg_make_freestanding_entry_c_name(cct_codegen_t *cg, const char *entry_name) {
    const char *program_name = (cg && cg->source_program && cg->source_program->name)
        ? cg->source_program->name
        : "module";
    char *module_part = cg_make_sanitized_ident_fragment(program_name);
    size_t module_len = strlen(module_part);
    size_t entry_len = strlen(entry_name);
    const char *prefix = "cct_fn_";
    size_t pfx_len = strlen(prefix);
    char *out = (char*)cg_calloc(1, pfx_len + module_len + 1 + entry_len + 1);
    memcpy(out, prefix, pfx_len);
    memcpy(out + pfx_len, module_part, module_len);
    out[pfx_len + module_len] = '_';
    memcpy(out + pfx_len + module_len + 1, entry_name, entry_len);
    out[pfx_len + module_len + 1 + entry_len] = '\0';
    free(module_part);
    return out;
}

static const char* cg_c_type_for_ast_type(cct_codegen_t *cg, const cct_ast_type_t *type);

static bool cg_validate_rituale_signature_f6b(cct_codegen_t *cg, const cct_ast_node_t *rituale) {
    if (!rituale || rituale->type != AST_RITUALE) {
        cg_report_node(cg, rituale, "internal codegen error: expected rituale");
        return false;
    }

    cct_codegen_value_kind_t ret_kind = cg_value_kind_from_ast_type(rituale->as.rituale.return_type);
    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->is_array) {
        cg_report_nodef(cg, rituale, "rituale '%s' return array type is not supported in FASE 7A codegen",
                        rituale->as.rituale.name);
        return false;
    }
    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->is_pointer) {
        if (!cg_c_type_for_ast_type(cg, rituale->as.rituale.return_type)) {
            cg_report_nodef(cg, rituale, "rituale '%s' return pointer type is outside FASE 7A SPECULUM subset",
                            rituale->as.rituale.name);
            return false;
        }
    }

    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->name &&
        ret_kind == CCT_CODEGEN_VALUE_STRUCT) {
        if (!cg_c_type_for_ast_type(cg, rituale->as.rituale.return_type)) {
            cg_report_nodef(cg, rituale, "rituale '%s' return type '%s' is not supported in FASE 6B codegen",
                            rituale->as.rituale.name, rituale->as.rituale.return_type->name);
            return false;
        }
    }

    if (rituale->as.rituale.params) {
        for (size_t i = 0; i < rituale->as.rituale.params->count; i++) {
            cct_ast_param_t *param = rituale->as.rituale.params->params[i];
            cct_codegen_local_kind_t pk = cg_local_kind_from_ast_type(param ? param->type : NULL);
            if (pk == CCT_CODEGEN_LOCAL_POINTER && param && param->type &&
                !cg_c_type_for_ast_type(cg, param->type)) {
                cg_report(cg, param->line, param->column,
                          "FASE 7B SPECULUM parameter type is outside executable pointer subset");
                return false;
            }
            bool ok_param =
                pk == CCT_CODEGEN_LOCAL_INT ||
                pk == CCT_CODEGEN_LOCAL_BOOL ||
                pk == CCT_CODEGEN_LOCAL_STRING ||
                pk == CCT_CODEGEN_LOCAL_UMBRA ||
                pk == CCT_CODEGEN_LOCAL_FLAMMA ||
                pk == CCT_CODEGEN_LOCAL_POINTER ||
                pk == CCT_CODEGEN_LOCAL_ORDO;

            if (pk == CCT_CODEGEN_LOCAL_STRUCT && param && param->type) {
                ok_param = cg_c_type_for_ast_type(cg, param->type) != NULL;
            }
            if (pk == CCT_CODEGEN_LOCAL_STRUCT && param && param->type && cg_is_known_ordo_type(cg, param->type)) {
                ok_param = true;
            }

            if (!ok_param) {
                cg_report(cg, param ? param->line : rituale->line, param ? param->column : rituale->column,
                          "FASE 7A supports ritual parameters only for executable scalar/SPECULUM subset types");
                return false;
            }
        }
    }

    return true;
}

static bool cg_register_rituale(cct_codegen_t *cg, const cct_ast_node_t *rituale) {
    if (!cg_validate_rituale_signature_f6b(cg, rituale)) return false;

    if (cg_find_rituale(cg, rituale->as.rituale.name)) {
        cg_report_nodef(cg, rituale, "duplicate rituale '%s' in codegen registry", rituale->as.rituale.name);
        return false;
    }

    cct_codegen_rituale_t *r = (cct_codegen_rituale_t*)cg_calloc(1, sizeof(*r));
    r->node = rituale;
    r->name = cg_strdup(rituale->as.rituale.name);
    if (cg_is_explicit_entry_name(cg, rituale->as.rituale.name)) {
        r->c_name = cg_make_freestanding_entry_c_name(cg, rituale->as.rituale.name);
    } else {
        r->c_name = cg_make_rituale_c_name(rituale->as.rituale.name);
    }
    r->return_kind = cg_value_kind_from_ast_type(rituale->as.rituale.return_type);
    if (rituale->as.rituale.return_type &&
        cg_is_known_ordo_type(cg, rituale->as.rituale.return_type) &&
        !cg_is_payload_ordo_type(cg, rituale->as.rituale.return_type)) {
        r->return_kind = CCT_CODEGEN_VALUE_INT;
    }
    r->returns_nihil = (r->return_kind == CCT_CODEGEN_VALUE_NIHIL);

    if (rituale->as.rituale.params && rituale->as.rituale.params->count > 0) {
        r->param_count = rituale->as.rituale.params->count;
        r->param_kinds = (cct_codegen_local_kind_t*)cg_calloc(r->param_count, sizeof(*r->param_kinds));
        for (size_t i = 0; i < r->param_count; i++) {
            cct_ast_type_t *ptype = rituale->as.rituale.params->params[i]->type;
            r->param_kinds[i] = cg_local_kind_from_ast_type(ptype);
            if (r->param_kinds[i] == CCT_CODEGEN_LOCAL_STRUCT && ptype &&
                cg_is_known_ordo_type(cg, ptype) && !cg_is_payload_ordo_type(cg, ptype)) {
                r->param_kinds[i] = CCT_CODEGEN_LOCAL_ORDO;
            }
        }
    }

    r->next = cg->rituales;
    cg->rituales = r;
    return true;
}

static bool cg_register_sigillum(cct_codegen_t *cg, const cct_ast_node_t *sig) {
    if (!sig || sig->type != AST_SIGILLUM) return true;
    if (cg_find_sigillum(cg, sig->as.sigillum.name)) {
        cg_report_nodef(cg, sig, "duplicate SIGILLUM '%s' in codegen registry", sig->as.sigillum.name);
        return false;
    }
    if (sig->as.sigillum.methods && sig->as.sigillum.methods->count > 0) {
        cg_report_nodef(cg, sig, "SIGILLUM '%s' methods are not executable in FASE 6B codegen yet", sig->as.sigillum.name);
        return false;
    }
    cct_codegen_sigillum_t *n = (cct_codegen_sigillum_t*)cg_calloc(1, sizeof(*n));
    n->node = sig;
    n->name = cg_strdup(sig->as.sigillum.name);
    if (!cg->sigilla) {
        cg->sigilla = n;
    } else {
        cct_codegen_sigillum_t *tail = cg->sigilla;
        while (tail->next) tail = tail->next;
        tail->next = n;
    }
    return true;
}

static bool cg_register_ordo(cct_codegen_t *cg, const cct_ast_node_t *ordo) {
    if (!ordo || ordo->type != AST_ORDO) return true;
    if (cg_find_ordo(cg, ordo->as.ordo.name)) {
        cg_report_nodef(cg, ordo, "duplicate ORDO '%s' in codegen registry", ordo->as.ordo.name);
        return false;
    }
    cct_codegen_ordo_t *n = (cct_codegen_ordo_t*)cg_calloc(1, sizeof(*n));
    n->node = ordo;
    n->name = cg_strdup(ordo->as.ordo.name);
    n->next = cg->ordines;
    cg->ordines = n;
    return true;
}

static size_t cg_decl_genus_arity(const cct_ast_node_t *decl) {
    if (!decl) return 0;
    if (decl->type == AST_RITUALE && decl->as.rituale.type_params) return decl->as.rituale.type_params->count;
    if (decl->type == AST_SIGILLUM && decl->as.sigillum.type_params) return decl->as.sigillum.type_params->count;
    return 0;
}

static const cct_ast_node_t* cg_find_generic_rituale_template(cct_codegen_t *cg, const char *name) {
    if (!cg || !cg->source_program || !cg->source_program->declarations || !name) return NULL;
    for (size_t i = 0; i < cg->source_program->declarations->count; i++) {
        const cct_ast_node_t *decl = cg->source_program->declarations->nodes[i];
        if (!decl || decl->type != AST_RITUALE) continue;
        if (decl->as.rituale.name && strcmp(decl->as.rituale.name, name) == 0 &&
            cg_decl_genus_arity(decl) > 0) {
            return decl;
        }
    }
    return NULL;
}

static const cct_ast_node_t* cg_find_generic_sigillum_template(cct_codegen_t *cg, const char *name) {
    if (!cg || !cg->source_program || !cg->source_program->declarations || !name) return NULL;
    for (size_t i = 0; i < cg->source_program->declarations->count; i++) {
        const cct_ast_node_t *decl = cg->source_program->declarations->nodes[i];
        if (!decl || decl->type != AST_SIGILLUM) continue;
        if (decl->as.sigillum.name && strcmp(decl->as.sigillum.name, name) == 0 &&
            cg_decl_genus_arity(decl) > 0) {
            return decl;
        }
    }
    return NULL;
}

static char* cg_type_key_string(const cct_ast_type_t *type);

static char* cg_type_list_key_string(const cct_ast_type_list_t *types) {
    if (!types || types->count == 0) return cg_strdup("");
    char *acc = cg_strdup("");
    for (size_t i = 0; i < types->count; i++) {
        char *piece = cg_type_key_string(types->types[i]);
        char *next = cg_strdup_printf("%s%s%s", acc, (i > 0) ? "," : "", piece);
        free(piece);
        free(acc);
        acc = next;
    }
    return acc;
}

static char* cg_type_key_string(const cct_ast_type_t *type) {
    if (!type) return cg_strdup("?");
    if (type->is_pointer) {
        char *inner = cg_type_key_string(type->element_type);
        char *out = cg_strdup_printf("P(%s)", inner);
        free(inner);
        return out;
    }
    if (type->is_array) {
        char *inner = cg_type_key_string(type->element_type);
        char *out = cg_strdup_printf("A[%u](%s)", type->array_size, inner);
        free(inner);
        return out;
    }
    if (type->generic_args && type->generic_args->count > 0) {
        char *args = cg_type_list_key_string(type->generic_args);
        char *out = cg_strdup_printf("%s<%s>", type->name ? type->name : "?", args);
        free(args);
        return out;
    }
    return cg_strdup(type->name ? type->name : "?");
}

static char* cg_type_param_constraints_key_string(const cct_ast_type_param_list_t *params) {
    if (!params || params->count == 0) return cg_strdup("");
    char *acc = cg_strdup("");
    for (size_t i = 0; i < params->count; i++) {
        const cct_ast_type_param_t *tp = params->params[i];
        const char *name = (tp && tp->name) ? tp->name : "";
        const char *constraint = (tp && tp->constraint_pactum_name) ? tp->constraint_pactum_name : "";
        char *next = cg_strdup_printf("%s%s%s:%s", acc, (i > 0) ? "," : "", name, constraint);
        free(acc);
        acc = next;
    }
    return acc;
}

static u64 cg_fnv1a64(const char *s) {
    u64 h = 1469598103934665603ULL;
    if (!s) return h;
    while (*s) {
        h ^= (u8)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

static char* cg_sanitize_for_symbol(const char *s) {
    if (!s) return cg_strdup("x");
    size_t n = strlen(s);
    char *out = (char*)cg_calloc(n + 1, 1);
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9')) {
            out[j++] = c;
        } else {
            out[j++] = '_';
        }
    }
    out[j] = '\0';
    return out;
}

static char* cg_make_inst_key(
    bool is_rituale,
    const char *name,
    const cct_ast_type_list_t *type_args,
    const cct_ast_node_t *templ_decl
) {
    char *args = cg_type_list_key_string(type_args);
    char *constraints = (is_rituale && templ_decl && templ_decl->type == AST_RITUALE)
        ? cg_type_param_constraints_key_string(templ_decl->as.rituale.type_params)
        : cg_strdup("");
    char *key = cg_strdup_printf("%c|%s|%s|%s", is_rituale ? 'R' : 'S', name ? name : "", args, constraints);
    free(args);
    free(constraints);
    return key;
}

static char* cg_make_specialized_decl_name(bool is_rituale, const char *decl_name, const cct_ast_type_list_t *type_args) {
    char *args = cg_type_list_key_string(type_args);
    char *safe = cg_sanitize_for_symbol(args);
    u64 h = cg_fnv1a64(args);
    char *name = cg_strdup_printf("cctg_%s_%s__%s__%04llx",
                                  is_rituale ? "fn" : "ty",
                                  decl_name ? decl_name : "anon",
                                  safe,
                                  (unsigned long long)(h & 0xFFFFULL));
    free(args);
    free(safe);
    return name;
}

static cct_codegen_genus_inst_t* cg_find_genus_instance(cct_codegen_t *cg, bool is_rituale, const char *key) {
    for (cct_codegen_genus_inst_t *it = cg ? cg->genus_instances : NULL; it; it = it->next) {
        if (it->is_rituale == is_rituale && it->key && key && strcmp(it->key, key) == 0) return it;
    }
    return NULL;
}

static cct_codegen_genus_inst_t* cg_add_genus_instance(
    cct_codegen_t *cg,
    bool is_rituale,
    const char *key,
    const char *special_name,
    cct_ast_node_t *node
) {
    cct_codegen_genus_inst_t *it = (cct_codegen_genus_inst_t*)cg_calloc(1, sizeof(*it));
    it->is_rituale = is_rituale;
    it->key = cg_strdup(key);
    it->special_name = cg_strdup(special_name);
    it->materialized_node = node;
    it->next = cg->genus_instances;
    cg->genus_instances = it;
    return it;
}

static cct_ast_type_t* cg_clone_type_raw(const cct_ast_type_t *src);
static cct_ast_type_t* cg_clone_type_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_type_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
);
static cct_ast_node_t* cg_clone_node_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_node_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
);
static cct_ast_type_list_t* cg_clone_type_list_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_type_list_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
);
static cct_codegen_rituale_t* cg_materialize_generic_rituale_instance(
    cct_codegen_t *cg,
    const char *name,
    const cct_ast_type_list_t *type_args,
    const cct_ast_node_t *at
);

static const cct_ast_type_t* cg_lookup_binding_type(
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args,
    const char *name
) {
    if (!params || !args || !name) return NULL;
    size_t n = params->count < args->count ? params->count : args->count;
    for (size_t i = 0; i < n; i++) {
        cct_ast_type_param_t *p = params->params[i];
        if (p && p->name && strcmp(p->name, name) == 0) {
            return args->types[i];
        }
    }
    return NULL;
}

static cct_ast_type_t* cg_clone_type_raw(const cct_ast_type_t *src) {
    if (!src) return NULL;
    if (src->is_pointer) {
        return cct_ast_create_pointer_type(cg_clone_type_raw(src->element_type));
    }
    if (src->is_array) {
        return cct_ast_create_array_type(cg_clone_type_raw(src->element_type), src->array_size);
    }
    cct_ast_type_t *dst = cct_ast_create_type(src->name ? src->name : "");
    if (src->generic_args && src->generic_args->count > 0) {
        dst->generic_args = cct_ast_create_type_list();
        for (size_t i = 0; i < src->generic_args->count; i++) {
            cct_ast_type_list_append(dst->generic_args, cg_clone_type_raw(src->generic_args->types[i]));
        }
    }
    return dst;
}

static cct_ast_type_list_t* cg_clone_type_list_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_type_list_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
) {
    if (!src) return NULL;
    cct_ast_type_list_t *dst = cct_ast_create_type_list();
    for (size_t i = 0; i < src->count; i++) {
        cct_ast_type_t *item = cg_clone_type_with_bindings(cg, src->types[i], params, args);
        cct_ast_type_list_append(dst, item);
    }
    return dst;
}

static const char* cg_materialize_generic_sigillum_name(
    cct_codegen_t *cg,
    const cct_ast_type_t *inst_type,
    const cct_ast_node_t *at
) {
    if (!inst_type || !inst_type->name || !inst_type->generic_args || inst_type->generic_args->count == 0) {
        return inst_type ? inst_type->name : NULL;
    }
    const cct_ast_node_t *templ = cg_find_generic_sigillum_template(cg, inst_type->name);
    if (!templ) {
        cg_report_nodef(cg, at, "GENUS(...) applied to non-generic type '%s' (subset 10B)", inst_type->name);
        return NULL;
    }
    size_t arity = cg_decl_genus_arity(templ);
    if (inst_type->generic_args->count != arity) {
        cg_report_nodef(cg, at,
                        "generic type '%s' expects %zu type argument(s), got %zu (subset 10B)",
                        inst_type->name, arity, inst_type->generic_args->count);
        return NULL;
    }

    char *key = cg_make_inst_key(false, inst_type->name, inst_type->generic_args, templ);
    cct_codegen_genus_inst_t *existing = cg_find_genus_instance(cg, false, key);
    if (existing) {
        free(key);
        return existing->special_name;
    }

    if (templ->as.sigillum.methods && templ->as.sigillum.methods->count > 0) {
        cg_report_nodef(cg, at, "generic SIGILLUM '%s' with methods is outside subset 10B materialization", inst_type->name);
        free(key);
        return NULL;
    }

    char *special_name = cg_make_specialized_decl_name(false, inst_type->name, inst_type->generic_args);
    cct_ast_field_list_t *fields = cct_ast_create_field_list();
    cct_ast_field_list_t *src_fields = templ->as.sigillum.fields;
    if (src_fields) {
        for (size_t i = 0; i < src_fields->count; i++) {
            cct_ast_field_t *f = src_fields->fields[i];
            if (!f) continue;
            cct_ast_type_t *ft = cg_clone_type_with_bindings(
                cg, f->type, templ->as.sigillum.type_params, inst_type->generic_args
            );
            cct_ast_field_t *nf = cct_ast_create_field(f->name ? f->name : "", ft, f->line, f->column);
            cct_ast_field_list_append(fields, nf);
        }
    }
    cct_ast_node_t *node = cct_ast_create_sigillum(
        special_name, cct_ast_create_type_param_list(), templ->as.sigillum.pactum_name,
        fields, cct_ast_create_node_list(), templ->line, templ->column
    );
    if (!cg_register_sigillum(cg, node)) {
        cct_ast_free_node(node);
        free(special_name);
        free(key);
        return NULL;
    }
    cct_codegen_genus_inst_t *created = cg_add_genus_instance(cg, false, key, special_name, node);
    free(special_name);
    free(key);
    return created ? created->special_name : NULL;
}

static cct_ast_type_t* cg_clone_type_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_type_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
) {
    if (!src) return NULL;
    if (src->is_pointer) {
        return cct_ast_create_pointer_type(cg_clone_type_with_bindings(cg, src->element_type, params, args));
    }
    if (src->is_array) {
        return cct_ast_create_array_type(
            cg_clone_type_with_bindings(cg, src->element_type, params, args),
            src->array_size
        );
    }

    const cct_ast_type_t *bound = cg_lookup_binding_type(params, args, src->name);
    if (bound) {
        return cg_clone_type_raw(bound);
    }

    cct_ast_type_t *dst = cct_ast_create_type(src->name ? src->name : "");
    if (src->generic_args && src->generic_args->count > 0) {
        dst->generic_args = cg_clone_type_list_with_bindings(cg, src->generic_args, params, args);
        const char *special = cg_materialize_generic_sigillum_name(cg, dst, NULL);
        if (special) {
            free(dst->name);
            dst->name = cg_strdup(special);
            cct_ast_free_type_list(dst->generic_args);
            dst->generic_args = NULL;
        }
    }
    return dst;
}

static cct_ast_node_list_t* cg_clone_node_list_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_node_list_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
) {
    cct_ast_node_list_t *dst = cct_ast_create_node_list();
    if (!src) return dst;
    for (size_t i = 0; i < src->count; i++) {
        cct_ast_node_t *n = cg_clone_node_with_bindings(cg, src->nodes[i], params, args);
        cct_ast_node_list_append(dst, n);
    }
    return dst;
}

static cct_ast_param_list_t* cg_clone_param_list_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_param_list_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
) {
    cct_ast_param_list_t *dst = cct_ast_create_param_list();
    if (!src) return dst;
    for (size_t i = 0; i < src->count; i++) {
        cct_ast_param_t *p = src->params[i];
        cct_ast_param_t *np = cct_ast_create_param(
            p ? p->name : "",
            cg_clone_type_with_bindings(cg, p ? p->type : NULL, params, args),
            p ? p->is_constans : false,
            p ? p->line : 0,
            p ? p->column : 0
        );
        cct_ast_param_list_append(dst, np);
    }
    return dst;
}

static cct_ast_node_t* cg_clone_node_with_bindings(
    cct_codegen_t *cg,
    const cct_ast_node_t *src,
    const cct_ast_type_param_list_t *params,
    const cct_ast_type_list_t *args
) {
    if (!src) return NULL;
    switch (src->type) {
        case AST_BLOCK:
            return cct_ast_create_block(
                cg_clone_node_list_with_bindings(cg, src->as.block.statements, params, args),
                src->line, src->column
            );
        case AST_EVOCA:
            return cct_ast_create_evoca(
                cg_clone_type_with_bindings(cg, src->as.evoca.var_type, params, args),
                src->as.evoca.name,
                cg_clone_node_with_bindings(cg, src->as.evoca.initializer, params, args),
                src->as.evoca.is_constans,
                src->line, src->column
            );
        case AST_VINCIRE:
            return cct_ast_create_vincire(
                cg_clone_node_with_bindings(cg, src->as.vincire.target, params, args),
                cg_clone_node_with_bindings(cg, src->as.vincire.value, params, args),
                src->line, src->column
            );
        case AST_REDDE:
            return cct_ast_create_redde(
                cg_clone_node_with_bindings(cg, src->as.redde.value, params, args),
                src->line, src->column
            );
        case AST_EXPR_STMT:
            return cct_ast_create_expr_stmt(
                cg_clone_node_with_bindings(cg, src->as.expr_stmt.expression, params, args),
                src->line, src->column
            );
        case AST_IDENTIFIER:
            return cct_ast_create_identifier(src->as.identifier.name, src->line, src->column);
        case AST_LITERAL_INT:
            return cct_ast_create_literal_int(src->as.literal_int.int_value, src->line, src->column);
        case AST_LITERAL_REAL:
            return cct_ast_create_literal_real(src->as.literal_real.real_value, src->line, src->column);
        case AST_LITERAL_STRING:
            return cct_ast_create_literal_string(src->as.literal_string.string_value, src->line, src->column);
        case AST_LITERAL_BOOL:
            return cct_ast_create_literal_bool(src->as.literal_bool.bool_value, src->line, src->column);
        case AST_LITERAL_NIHIL:
            return cct_ast_create_literal_nihil(src->line, src->column);
        case AST_MOLDE: {
            cct_ast_molde_part_t *parts = NULL;
            size_t part_count = src->as.molde.part_count;
            if (part_count > 0 && src->as.molde.parts) {
                parts = (cct_ast_molde_part_t*)cg_calloc(part_count, sizeof(*parts));
                for (size_t i = 0; i < part_count; i++) {
                    parts[i].kind = src->as.molde.parts[i].kind;
                    if (parts[i].kind == CCT_AST_MOLDE_PART_LITERAL) {
                        parts[i].literal_text = cg_strdup(src->as.molde.parts[i].literal_text);
                        parts[i].expr = NULL;
                        parts[i].fmt_spec = NULL;
                    } else {
                        parts[i].literal_text = NULL;
                        parts[i].expr = cg_clone_node_with_bindings(
                            cg, src->as.molde.parts[i].expr, params, args
                        );
                        parts[i].fmt_spec = cg_strdup(src->as.molde.parts[i].fmt_spec);
                    }
                }
            }
            return cct_ast_create_molde(parts, part_count, src->line, src->column);
        }
        case AST_BINARY_OP:
            return cct_ast_create_binary_op(
                src->as.binary_op.operator,
                cg_clone_node_with_bindings(cg, src->as.binary_op.left, params, args),
                cg_clone_node_with_bindings(cg, src->as.binary_op.right, params, args),
                src->line, src->column
            );
        case AST_UNARY_OP:
            return cct_ast_create_unary_op(
                src->as.unary_op.operator,
                cg_clone_node_with_bindings(cg, src->as.unary_op.operand, params, args),
                src->line, src->column
            );
        case AST_CONIURA: {
            cct_ast_type_list_t *targs = cg_clone_type_list_with_bindings(cg, src->as.coniura.type_args, params, args);
            return cct_ast_create_coniura(
                src->as.coniura.name,
                targs,
                cg_clone_node_list_with_bindings(cg, src->as.coniura.arguments, params, args),
                src->line, src->column
            );
        }
        case AST_OBSECRO:
            return cct_ast_create_obsecro(
                src->as.obsecro.name,
                cg_clone_node_list_with_bindings(cg, src->as.obsecro.arguments, params, args),
                src->line, src->column
            );
        case AST_FIELD_ACCESS:
            return cct_ast_create_field_access(
                cg_clone_node_with_bindings(cg, src->as.field_access.object, params, args),
                src->as.field_access.field,
                src->line, src->column
            );
        case AST_INDEX_ACCESS:
            return cct_ast_create_index_access(
                cg_clone_node_with_bindings(cg, src->as.index_access.array, params, args),
                cg_clone_node_with_bindings(cg, src->as.index_access.index, params, args),
                src->line, src->column
            );
        case AST_MENSURA:
            return cct_ast_create_mensura(
                cg_clone_type_with_bindings(cg, src->as.mensura.type, params, args),
                src->line, src->column
            );
        case AST_SI:
            return cct_ast_create_si(
                cg_clone_node_with_bindings(cg, src->as.si.condition, params, args),
                cg_clone_node_with_bindings(cg, src->as.si.then_branch, params, args),
                cg_clone_node_with_bindings(cg, src->as.si.else_branch, params, args),
                src->line, src->column
            );
        case AST_QUANDO: {
            cct_ast_case_node_t *cases = NULL;
            size_t case_count = src->as.quando.case_count;
            if (case_count > 0) {
                cases = (cct_ast_case_node_t*)cg_calloc(case_count, sizeof(cct_ast_case_node_t));
                for (size_t i = 0; i < case_count; i++) {
                    cct_ast_case_node_t *dst_case = &cases[i];
                    const cct_ast_case_node_t *src_case = &src->as.quando.cases[i];
                    dst_case->literal_count = src_case->literal_count;
                    dst_case->binding_count = src_case->binding_count;
                    if (dst_case->literal_count > 0) {
                        dst_case->literals = (cct_ast_node_t**)cg_calloc(dst_case->literal_count, sizeof(cct_ast_node_t*));
                        for (size_t j = 0; j < dst_case->literal_count; j++) {
                            dst_case->literals[j] = cg_clone_node_with_bindings(
                                cg, src_case->literals[j], params, args
                            );
                        }
                    }
                    if (dst_case->binding_count > 0) {
                        dst_case->binding_names = (char**)cg_calloc(dst_case->binding_count, sizeof(char*));
                        for (size_t j = 0; j < dst_case->binding_count; j++) {
                            dst_case->binding_names[j] = cg_strdup(src_case->binding_names[j]);
                        }
                    }
                    dst_case->resolved_ordo_variant = NULL;
                    dst_case->body = cg_clone_node_with_bindings(cg, src_case->body, params, args);
                }
            }
            return cct_ast_create_quando(
                cg_clone_node_with_bindings(cg, src->as.quando.expression, params, args),
                cases,
                case_count,
                cg_clone_node_with_bindings(cg, src->as.quando.else_body, params, args),
                src->line,
                src->column
            );
        }
        case AST_DUM:
            return cct_ast_create_dum(
                cg_clone_node_with_bindings(cg, src->as.dum.condition, params, args),
                cg_clone_node_with_bindings(cg, src->as.dum.body, params, args),
                src->line, src->column
            );
        case AST_DONEC:
            return cct_ast_create_donec(
                cg_clone_node_with_bindings(cg, src->as.donec.body, params, args),
                cg_clone_node_with_bindings(cg, src->as.donec.condition, params, args),
                src->line, src->column
            );
        case AST_REPETE:
            return cct_ast_create_repete(
                src->as.repete.iterator,
                cg_clone_node_with_bindings(cg, src->as.repete.start, params, args),
                cg_clone_node_with_bindings(cg, src->as.repete.end, params, args),
                cg_clone_node_with_bindings(cg, src->as.repete.step, params, args),
                cg_clone_node_with_bindings(cg, src->as.repete.body, params, args),
                src->line, src->column
            );
        case AST_ITERUM:
            return cct_ast_create_iterum(
                src->as.iterum.item_name,
                src->as.iterum.value_name,
                cg_clone_node_with_bindings(cg, src->as.iterum.collection, params, args),
                cg_clone_node_with_bindings(cg, src->as.iterum.body, params, args),
                src->line, src->column
            );
        case AST_TEMPTA:
            return cct_ast_create_tempta(
                cg_clone_node_with_bindings(cg, src->as.tempta.try_block, params, args),
                cg_clone_type_with_bindings(cg, src->as.tempta.cape_type, params, args),
                src->as.tempta.cape_name,
                cg_clone_node_with_bindings(cg, src->as.tempta.cape_block, params, args),
                cg_clone_node_with_bindings(cg, src->as.tempta.semper_block, params, args),
                src->line, src->column
            );
        case AST_IACE:
            return cct_ast_create_iace(
                cg_clone_node_with_bindings(cg, src->as.iace.value, params, args),
                src->line, src->column
            );
        case AST_ANUR:
            return cct_ast_create_anur(
                cg_clone_node_with_bindings(cg, src->as.anur.value, params, args),
                src->line, src->column
            );
        case AST_DIMITTE:
            return cct_ast_create_dimitte(
                cg_clone_node_with_bindings(cg, src->as.dimitte.target, params, args),
                src->line, src->column
            );
        case AST_FRANGE:
            return cct_ast_create_frange(src->line, src->column);
        case AST_RECEDE:
            return cct_ast_create_recede(src->line, src->column);
        case AST_TRANSITUS:
            return cct_ast_create_transitus(src->as.transitus.label, src->line, src->column);
        default:
            cg_report_nodef(cg, src, "generic materialization clone does not support AST node %s in subset 10B",
                            cct_ast_node_type_string(src->type));
            return cct_ast_create_literal_nihil(src->line, src->column);
    }
}

static cct_codegen_rituale_t* cg_materialize_generic_rituale_instance(
    cct_codegen_t *cg,
    const char *name,
    const cct_ast_type_list_t *type_args,
    const cct_ast_node_t *at
) {
    const cct_ast_node_t *templ = cg_find_generic_rituale_template(cg, name);
    if (!templ) {
        cg_report_nodef(cg, at, "GENUS(...) applied to non-generic rituale '%s' (subset 10B)", name ? name : "");
        return NULL;
    }
    size_t arity = cg_decl_genus_arity(templ);
    size_t got = type_args ? type_args->count : 0;
    if (got != arity) {
        cg_report_nodef(cg, at,
                        "generic rituale '%s' expects %zu type argument(s), got %zu (subset 10B)",
                        name ? name : "", arity, got);
        return NULL;
    }

    char *key = cg_make_inst_key(true, name, type_args, templ);
    cct_codegen_genus_inst_t *existing = cg_find_genus_instance(cg, true, key);
    if (existing) {
        cct_codegen_rituale_t *r = cg_find_rituale(cg, existing->special_name);
        free(key);
        return r;
    }

    char *special_name = cg_make_specialized_decl_name(true, name, type_args);
    cct_ast_param_list_t *params = cg_clone_param_list_with_bindings(
        cg, templ->as.rituale.params, templ->as.rituale.type_params, type_args
    );
    cct_ast_type_t *ret = cg_clone_type_with_bindings(
        cg, templ->as.rituale.return_type, templ->as.rituale.type_params, type_args
    );
    cct_ast_node_t *body = cg_clone_node_with_bindings(
        cg, templ->as.rituale.body, templ->as.rituale.type_params, type_args
    );

    cct_ast_node_t *node = cct_ast_create_rituale(
        special_name,
        cct_ast_create_type_param_list(),
        params,
        ret,
        body,
        templ->line,
        templ->column
    );
    if (!cg_register_rituale(cg, node)) {
        cct_ast_free_node(node);
        free(special_name);
        free(key);
        return NULL;
    }
    cg_add_genus_instance(cg, true, key, special_name, node);
    cct_codegen_rituale_t *r = cg_find_rituale(cg, special_name);
    free(special_name);
    free(key);
    return r;
}

static bool cg_scan_type_for_genus_uses(cct_codegen_t *cg, const cct_ast_type_t *type, const cct_ast_node_t *at);
static bool cg_scan_expr_for_genus_uses(cct_codegen_t *cg, const cct_ast_node_t *expr);
static bool cg_scan_stmt_for_genus_uses(cct_codegen_t *cg, const cct_ast_node_t *stmt);

static bool cg_scan_type_for_genus_uses(cct_codegen_t *cg, const cct_ast_type_t *type, const cct_ast_node_t *at) {
    if (!type || !cg) return true;
    if (type->is_pointer || type->is_array) {
        return cg_scan_type_for_genus_uses(cg, type->element_type, at);
    }
    if (type->generic_args && type->generic_args->count > 0) {
        for (size_t i = 0; i < type->generic_args->count; i++) {
            if (!cg_scan_type_for_genus_uses(cg, type->generic_args->types[i], at)) return false;
        }
        if (!cg_materialize_generic_sigillum_name(cg, type, at)) return false;
    }
    return true;
}

static bool cg_scan_expr_for_genus_uses(cct_codegen_t *cg, const cct_ast_node_t *expr) {
    if (!expr) return true;
    switch (expr->type) {
        case AST_CONIURA:
            if (expr->as.coniura.name && strcmp(expr->as.coniura.name, "__cast") == 0) {
                if (expr->as.coniura.type_args && expr->as.coniura.type_args->count > 0) {
                    for (size_t i = 0; i < expr->as.coniura.type_args->count; i++) {
                        if (!cg_scan_type_for_genus_uses(cg, expr->as.coniura.type_args->types[i], expr)) return false;
                    }
                }
                if (expr->as.coniura.arguments) {
                    for (size_t i = 0; i < expr->as.coniura.arguments->count; i++) {
                        if (!cg_scan_expr_for_genus_uses(cg, expr->as.coniura.arguments->nodes[i])) return false;
                    }
                }
                return true;
            }
            if (expr->as.coniura.type_args && expr->as.coniura.type_args->count > 0) {
                if (!cg_materialize_generic_rituale_instance(cg, expr->as.coniura.name, expr->as.coniura.type_args, expr)) {
                    return false;
                }
            } else if (cg_find_generic_rituale_template(cg, expr->as.coniura.name)) {
                    cg_report_nodef(cg, expr,
                                "generic rituale '%s' requires explicit GENUS(...) instantiation in subset 10B (subset final da FASE 10 has no type-arg inference)",
                                expr->as.coniura.name ? expr->as.coniura.name : "");
                    return false;
            }
            if (expr->as.coniura.arguments) {
                for (size_t i = 0; i < expr->as.coniura.arguments->count; i++) {
                    if (!cg_scan_expr_for_genus_uses(cg, expr->as.coniura.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_OBSECRO:
            if (expr->as.obsecro.arguments) {
                for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                    if (!cg_scan_expr_for_genus_uses(cg, expr->as.obsecro.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_MOLDE:
            if (expr->as.molde.parts) {
                for (size_t i = 0; i < expr->as.molde.part_count; i++) {
                    cct_ast_molde_part_t *part = &expr->as.molde.parts[i];
                    if (part && part->kind == CCT_AST_MOLDE_PART_EXPR) {
                        if (!cg_scan_expr_for_genus_uses(cg, part->expr)) return false;
                    }
                }
            }
            return true;
        case AST_BINARY_OP:
            return cg_scan_expr_for_genus_uses(cg, expr->as.binary_op.left) &&
                   cg_scan_expr_for_genus_uses(cg, expr->as.binary_op.right);
        case AST_UNARY_OP:
            return cg_scan_expr_for_genus_uses(cg, expr->as.unary_op.operand);
        case AST_FIELD_ACCESS:
            return cg_scan_expr_for_genus_uses(cg, expr->as.field_access.object);
        case AST_INDEX_ACCESS:
            return cg_scan_expr_for_genus_uses(cg, expr->as.index_access.array) &&
                   cg_scan_expr_for_genus_uses(cg, expr->as.index_access.index);
        case AST_CALL:
            if (!cg_scan_expr_for_genus_uses(cg, expr->as.call.callee)) return false;
            if (expr->as.call.arguments) {
                for (size_t i = 0; i < expr->as.call.arguments->count; i++) {
                    if (!cg_scan_expr_for_genus_uses(cg, expr->as.call.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_MENSURA:
            return cg_scan_type_for_genus_uses(cg, expr->as.mensura.type, expr);
        default:
            return true;
    }
}

static bool cg_scan_stmt_for_genus_uses(cct_codegen_t *cg, const cct_ast_node_t *stmt) {
    if (!stmt) return true;
    switch (stmt->type) {
        case AST_BLOCK:
            if (!stmt->as.block.statements) return true;
            for (size_t i = 0; i < stmt->as.block.statements->count; i++) {
                if (!cg_scan_stmt_for_genus_uses(cg, stmt->as.block.statements->nodes[i])) return false;
            }
            return true;
        case AST_EVOCA:
            return cg_scan_type_for_genus_uses(cg, stmt->as.evoca.var_type, stmt) &&
                   cg_scan_expr_for_genus_uses(cg, stmt->as.evoca.initializer);
        case AST_VINCIRE:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.vincire.target) &&
                   cg_scan_expr_for_genus_uses(cg, stmt->as.vincire.value);
        case AST_REDDE:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.redde.value);
        case AST_SI:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.si.condition) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.si.then_branch) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.si.else_branch);
        case AST_QUANDO:
            if (!cg_scan_expr_for_genus_uses(cg, stmt->as.quando.expression)) return false;
            for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
                cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
                for (size_t j = 0; j < case_node->literal_count; j++) {
                    if (!cg_scan_expr_for_genus_uses(cg, case_node->literals[j])) return false;
                }
                if (!cg_scan_stmt_for_genus_uses(cg, case_node->body)) return false;
            }
            return cg_scan_stmt_for_genus_uses(cg, stmt->as.quando.else_body);
        case AST_DUM:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.dum.condition) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.dum.body);
        case AST_DONEC:
            return cg_scan_stmt_for_genus_uses(cg, stmt->as.donec.body) &&
                   cg_scan_expr_for_genus_uses(cg, stmt->as.donec.condition);
        case AST_REPETE:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.repete.start) &&
                   cg_scan_expr_for_genus_uses(cg, stmt->as.repete.end) &&
                   cg_scan_expr_for_genus_uses(cg, stmt->as.repete.step) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.repete.body);
        case AST_ITERUM:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.iterum.collection) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.iterum.body);
        case AST_TEMPTA:
            return cg_scan_stmt_for_genus_uses(cg, stmt->as.tempta.try_block) &&
                   cg_scan_type_for_genus_uses(cg, stmt->as.tempta.cape_type, stmt) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.tempta.cape_block) &&
                   cg_scan_stmt_for_genus_uses(cg, stmt->as.tempta.semper_block);
        case AST_IACE:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.iace.value);
        case AST_DIMITTE:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.dimitte.target);
        case AST_EXPR_STMT:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.expr_stmt.expression);
        case AST_ANUR:
            return cg_scan_expr_for_genus_uses(cg, stmt->as.anur.value);
        default:
            return true;
    }
}

static bool cg_scan_rituale_decl_for_genus_uses(cct_codegen_t *cg, const cct_ast_node_t *decl) {
    if (!decl || decl->type != AST_RITUALE) return true;
    if (!cg_scan_type_for_genus_uses(cg, decl->as.rituale.return_type, decl)) return false;
    if (decl->as.rituale.params) {
        for (size_t i = 0; i < decl->as.rituale.params->count; i++) {
            cct_ast_param_t *p = decl->as.rituale.params->params[i];
            if (!cg_scan_type_for_genus_uses(cg, p ? p->type : NULL, decl)) return false;
        }
    }
    return cg_scan_stmt_for_genus_uses(cg, decl->as.rituale.body);
}

static bool cg_collect_generic_instantiations(cct_codegen_t *cg, const cct_ast_program_t *program) {
    if (!cg || !program || !program->declarations) return true;

    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl) continue;
        if (decl->type == AST_RITUALE && cg_decl_genus_arity(decl) == 0) {
            if (!cg_scan_rituale_decl_for_genus_uses(cg, decl)) return false;
        }
        if (decl->type == AST_SIGILLUM && cg_decl_genus_arity(decl) == 0 && decl->as.sigillum.fields) {
            for (size_t f = 0; f < decl->as.sigillum.fields->count; f++) {
                cct_ast_field_t *field = decl->as.sigillum.fields->fields[f];
                if (!cg_scan_type_for_genus_uses(cg, field ? field->type : NULL, decl)) return false;
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (cct_codegen_genus_inst_t *it = cg->genus_instances; it; it = it->next) {
            if (!it->is_rituale || it->scanned || !it->materialized_node) continue;
            it->scanned = true;
            changed = true;
            if (!cg_scan_rituale_decl_for_genus_uses(cg, it->materialized_node)) return false;
        }
    }

    return !cg->had_error;
}

static bool cg_collect_top_level_rituales(cct_codegen_t *cg, const cct_ast_program_t *program) {
    const cct_ast_node_t *found_entry = NULL;
    const bool explicit_entry = cg_has_explicit_freestanding_entry(cg);
    const char *requested_entry = explicit_entry ? cg->entry_rituale_name : NULL;

    if (!program || !program->declarations) return true;

    /* Pass A: register executable type declarations (SIGILLUM / ORDO). */
    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl) continue;
        if (decl->type == AST_SIGILLUM && cg_decl_genus_arity(decl) == 0) (void)cg_register_sigillum(cg, decl);
        if (decl->type == AST_ORDO) (void)cg_register_ordo(cg, decl);
    }
    if (cg->had_error) return false;

    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl) continue;

        if (decl->type != AST_RITUALE) {
            if (decl->type == AST_SIGILLUM || decl->type == AST_ORDO || decl->type == AST_PACTUM) continue;
            if (decl->type == AST_IMPORT) {
                cg_report_node(cg, decl, "ADVOCARE is not yet supported by FASE 4C codegen");
            } else {
                cg_report_nodef(cg, decl, "top-level %s is not supported by FASE 4C codegen",
                                cct_ast_node_type_string(decl->type));
            }
            continue;
        }

        if (cg_decl_genus_arity(decl) > 0) continue; /* generic templates are materialized on demand */
        if (!cg_register_rituale(cg, decl)) continue;

        if (explicit_entry) {
            if (requested_entry && strcmp(decl->as.rituale.name, requested_entry) == 0) {
                found_entry = decl;
            }
        } else if (cg_is_entry_rituale_name(decl->as.rituale.name)) {
            if (!found_entry) {
                found_entry = decl;
            } else {
                bool found_is_main = strcmp(found_entry->as.rituale.name, "main") == 0;
                bool this_is_principium = strcmp(decl->as.rituale.name, "principium") == 0;
                if (found_is_main && this_is_principium) {
                    found_entry = decl;
                } else {
                    cg_report_node(cg, decl, "multiple entry rituales found (FASE 4C accepts only one)");
                }
            }
        }
    }

    if (!cg_collect_generic_instantiations(cg, program)) return false;
    if (cg->had_error) return false;
    if (explicit_entry && !found_entry) {
        cg_reportf(cg, 0, 0, "rituale de entry '%s' não encontrado no módulo", requested_entry);
        return false;
    }

    cg->entry_rituale = found_entry;
    return !cg->had_error;
}

/* ========================================================================
 * Emit helpers: expressions and statements
 * ======================================================================== */

static bool cg_emit_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind);
static bool cg_emit_lvalue(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *target, cct_codegen_value_kind_t *out_kind);
static bool cg_probe_expr_kind(cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind);
static const cct_ast_type_t* cg_expr_struct_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const cct_ast_type_t* cg_expr_array_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const cct_ast_type_t* cg_expr_ast_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const char* cg_c_type_for_ast_type(cct_codegen_t *cg, const cct_ast_type_t *type);
static const char* cg_c_type_for_return_kind(cct_codegen_value_kind_t kind);

static bool cg_emit_logical_chain(FILE *out,
                                  cct_codegen_t *cg,
                                  const cct_ast_node_t *expr,
                                  cct_token_type_t op,
                                  cct_codegen_value_kind_t *out_kind) {
    if (!out || !cg || !expr) return false;

    if (expr->type == AST_BINARY_OP && expr->as.binary_op.operator == op) {
        if (!cg_emit_logical_chain(out, cg, expr->as.binary_op.left, op, NULL)) return false;
        fputs(op == TOKEN_ET ? " && " : " || ", out);
        if (!cg_emit_logical_chain(out, cg, expr->as.binary_op.right, op, NULL)) return false;
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    cct_codegen_value_kind_t term_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr, &term_kind)) return false;
    if (!(term_kind == CCT_CODEGEN_VALUE_BOOL || term_kind == CCT_CODEGEN_VALUE_INT)) {
        cg_report_nodef(cg, expr, "operator %s requires boolean or integer operands",
                        op == TOKEN_ET ? "ET" : "VEL");
        return false;
    }
    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
    return true;
}

static cct_codegen_value_kind_t cg_value_kind_from_ast_type_codegen(cct_codegen_t *cg, const cct_ast_type_t *type) {
    if (!type) return CCT_CODEGEN_VALUE_UNKNOWN;
    if (type->is_pointer) return CCT_CODEGEN_VALUE_POINTER;
    if (type->is_array) return CCT_CODEGEN_VALUE_ARRAY;
    if (!type->name) return CCT_CODEGEN_VALUE_UNKNOWN;

    if (strcmp(type->name, "NIHIL") == 0) return CCT_CODEGEN_VALUE_NIHIL;
    if (strcmp(type->name, "VERBUM") == 0) return CCT_CODEGEN_VALUE_STRING;
    if (strcmp(type->name, "FRACTUM") == 0) return CCT_CODEGEN_VALUE_STRING;
    if (strcmp(type->name, "VERUM") == 0) return CCT_CODEGEN_VALUE_BOOL;
    if (strcmp(type->name, "UMBRA") == 0 || strcmp(type->name, "FLAMMA") == 0) return CCT_CODEGEN_VALUE_REAL;
    if (strcmp(type->name, "REX") == 0 || strcmp(type->name, "DUX") == 0 ||
        strcmp(type->name, "COMES") == 0 || strcmp(type->name, "MILES") == 0) return CCT_CODEGEN_VALUE_INT;
    if (cg_is_known_ordo_type(cg, type)) {
        return cg_is_payload_ordo_type(cg, type) ? CCT_CODEGEN_VALUE_STRUCT : CCT_CODEGEN_VALUE_INT;
    }
    if (cg_is_known_sigillum_type(cg, type)) return CCT_CODEGEN_VALUE_STRUCT;
    return CCT_CODEGEN_VALUE_UNKNOWN;
}

static const char* cg_effective_named_type_name(cct_codegen_t *cg, const cct_ast_type_t *type) {
    if (!type || !type->name || type->is_pointer || type->is_array) return NULL;
    if (type->generic_args && type->generic_args->count > 0) {
        return cg_materialize_generic_sigillum_name(cg, type, NULL);
    }
    return type->name;
}

static bool cg_is_addressable_lvalue_expr(const cct_ast_node_t *expr) {
    if (!expr) return false;
    if (expr->type == AST_IDENTIFIER || expr->type == AST_FIELD_ACCESS || expr->type == AST_INDEX_ACCESS) return true;
    if (expr->type == AST_UNARY_OP && expr->as.unary_op.operator == TOKEN_STAR) return true;
    return false;
}

static const cct_ast_type_t* cg_expr_struct_type(cct_codegen_t *cg, const cct_ast_node_t *expr) {
    if (!expr) return NULL;
    if (expr->type == AST_IDENTIFIER) {
        cct_codegen_local_t *l = cg_find_local(cg, expr->as.identifier.name);
        if (l && l->ast_type && cg_is_known_sigillum_type(cg, l->ast_type)) return l->ast_type;
        return NULL;
    }
    if (expr->type == AST_UNARY_OP && expr->as.unary_op.operator == TOKEN_STAR) {
        const cct_ast_type_t *ptr_t = cg_expr_ast_type(cg, expr->as.unary_op.operand);
        if (ptr_t && ptr_t->is_pointer && ptr_t->element_type && cg_is_known_sigillum_type(cg, ptr_t->element_type)) {
            return ptr_t->element_type;
        }
        return NULL;
    }
    if (expr->type == AST_FIELD_ACCESS) {
        const cct_ast_type_t *obj_t = cg_expr_struct_type(cg, expr->as.field_access.object);
        if (!obj_t) return NULL;
        const char *sig_name = cg_effective_named_type_name(cg, obj_t);
        cct_codegen_sigillum_t *sig = sig_name ? cg_find_sigillum(cg, sig_name) : NULL;
        const cct_ast_field_t *field = cg_find_sigillum_field(sig, expr->as.field_access.field);
        if (!field || !field->type) return NULL;
        if (cg_is_known_sigillum_type(cg, field->type)) return field->type;
    }
    if (expr->type == AST_INDEX_ACCESS) {
        const cct_ast_type_t *elem_t = cg_expr_ast_type(cg, expr);
        if (elem_t && cg_is_known_sigillum_type(cg, elem_t)) return elem_t;
        return NULL;
    }
    return NULL;
}

static const cct_ast_type_t* cg_expr_array_type(cct_codegen_t *cg, const cct_ast_node_t *expr) {
    if (!expr) return NULL;
    if (expr->type == AST_IDENTIFIER) {
        cct_codegen_local_t *l = cg_find_local(cg, expr->as.identifier.name);
        if (l && l->ast_type && l->ast_type->is_array) return l->ast_type;
        return NULL;
    }
    if (expr->type == AST_FIELD_ACCESS) {
        const cct_ast_type_t *obj_t = cg_expr_struct_type(cg, expr->as.field_access.object);
        if (!obj_t) return NULL;
        const char *sig_name = cg_effective_named_type_name(cg, obj_t);
        if (!sig_name) return NULL;
        cct_codegen_sigillum_t *sig = cg_find_sigillum(cg, sig_name);
        const cct_ast_field_t *field = cg_find_sigillum_field(sig, expr->as.field_access.field);
        if (field && field->type && field->type->is_array) return field->type;
    }
    return NULL;
}

static const cct_ast_type_t* cg_expr_ast_type(cct_codegen_t *cg, const cct_ast_node_t *expr) {
    if (!expr) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            cct_codegen_local_t *l = cg_find_local(cg, expr->as.identifier.name);
            return l ? l->ast_type : NULL;
        }
        case AST_FIELD_ACCESS: {
            const cct_ast_type_t *obj_struct_t = cg_expr_struct_type(cg, expr->as.field_access.object);
            const char *sig_name = cg_effective_named_type_name(cg, obj_struct_t);
            if (!sig_name) return NULL;
            cct_codegen_sigillum_t *sig = cg_find_sigillum(cg, sig_name);
            const cct_ast_field_t *field = cg_find_sigillum_field(sig, expr->as.field_access.field);
            return field ? field->type : NULL;
        }
        case AST_INDEX_ACCESS: {
            const cct_ast_type_t *arr_t = cg_expr_array_type(cg, expr->as.index_access.array);
            if (arr_t && arr_t->element_type) return arr_t->element_type;
            const cct_ast_type_t *base_t = cg_expr_ast_type(cg, expr->as.index_access.array);
            if (base_t && base_t->is_pointer) return base_t->element_type;
            return NULL;
        }
        case AST_UNARY_OP:
            if (expr->as.unary_op.operator == TOKEN_STAR) {
                const cct_ast_type_t *operand_t = cg_expr_ast_type(cg, expr->as.unary_op.operand);
                if (operand_t && operand_t->is_pointer) return operand_t->element_type;
            }
            return NULL;
        default:
            return NULL;
    }
}

static bool cg_emit_binary_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    cct_token_type_t op = expr->as.binary_op.operator;
    const char *op_text = NULL;
    bool is_comparison = false;
    bool is_logical_and = false;
    bool is_logical_or = false;
    bool is_bitwise_and = false;
    bool is_bitwise_or = false;
    bool is_bitwise_xor = false;
    bool is_shift_left = false;
    bool is_shift_right = false;

    switch (op) {
        case TOKEN_PLUS: op_text = "+"; break;
        case TOKEN_MINUS: op_text = "-"; break;
        case TOKEN_STAR: op_text = "*"; break;
        case TOKEN_SLASH: op_text = "/"; break;
        case TOKEN_PERCENT: op_text = "%"; break;
        case TOKEN_STAR_STAR:
        case TOKEN_SLASH_SLASH:
        case TOKEN_PERCENT_PERCENT:
            break;
        case TOKEN_EQ_EQ: op_text = "=="; is_comparison = true; break;
        case TOKEN_BANG_EQ: op_text = "!="; is_comparison = true; break;
        case TOKEN_LESS: op_text = "<"; is_comparison = true; break;
        case TOKEN_LESS_EQ: op_text = "<="; is_comparison = true; break;
        case TOKEN_GREATER: op_text = ">"; is_comparison = true; break;
        case TOKEN_GREATER_EQ: op_text = ">="; is_comparison = true; break;
        case TOKEN_ET:
            is_logical_and = true;
            break;
        case TOKEN_VEL:
            is_logical_or = true;
            break;
        case TOKEN_ET_BIT:
            is_bitwise_and = true;
            break;
        case TOKEN_VEL_BIT:
            is_bitwise_or = true;
            break;
        case TOKEN_XOR:
            is_bitwise_xor = true;
            break;
        case TOKEN_SINISTER:
            is_shift_left = true;
            break;
        case TOKEN_DEXTER:
            is_shift_right = true;
            break;
        default:
            cg_report_nodef(cg, expr, "operator %s not supported in FASE 4C codegen",
                            cct_token_type_string(op));
            return false;
    }

    cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;

    if (is_logical_and) {
        fputc('(', out);
        if (!cg_emit_logical_chain(out, cg, expr, TOKEN_ET, &lhs_kind)) return false;
        fputc(')', out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (is_logical_or) {
        fputc('(', out);
        if (!cg_emit_logical_chain(out, cg, expr, TOKEN_VEL, &lhs_kind)) return false;
        fputc(')', out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (is_bitwise_and) {
        fputc('(', out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs(" & ", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputc(')', out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator ET_BIT requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (is_bitwise_or) {
        fputc('(', out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs(" | ", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputc(')', out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator VEL_BIT requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (is_bitwise_xor) {
        fputc('(', out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs(" ^ ", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputc(')', out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator XOR requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (is_shift_left) {
        fputs("cct_rt_shl_ll((long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs("))", out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator SINISTER requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (is_shift_right) {
        fputs("cct_rt_shr_ll((long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs("))", out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator DEXTER requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (op == TOKEN_STAR_STAR) {
        fputs("(cct_pow((double)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs("), (double)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs(")))", out);
        if (!cct_cg_value_kind_is_numeric(lhs_kind) || !cct_cg_value_kind_is_numeric(rhs_kind)) {
            cg_report_node(cg, expr, "operator ** requires numeric operands");
            return false;
        }

        if (out_kind) {
            *out_kind = (lhs_kind == CCT_CODEGEN_VALUE_REAL || rhs_kind == CCT_CODEGEN_VALUE_REAL)
                ? CCT_CODEGEN_VALUE_REAL
                : CCT_CODEGEN_VALUE_INT;
        }
        return true;
    }

    if (op == TOKEN_SLASH_SLASH) {
        fputs("(cct_floor_div_ll((long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs(")))", out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator // requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (op == TOKEN_PERCENT_PERCENT) {
        fputs("(cct_euclid_mod_ll((long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs(")))", out);
        if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator %% requires integer operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (is_comparison) {
        fputs("(", out);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
        fprintf(out, " %s ", op_text);
        if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
        fputs(" ? 1 : 0)", out);

        bool lhs_ok = cct_cg_value_kind_is_numeric(lhs_kind);
        bool rhs_ok = cct_cg_value_kind_is_numeric(rhs_kind);
        if (!lhs_ok || !rhs_ok) {
            cg_report_node(cg, expr, "FASE 6B comparison codegen supports numeric operands");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    fputc('(', out);
    if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
    fprintf(out, " %s ", op_text);
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
    fputc(')', out);

    if (!cct_cg_value_kind_is_numeric(lhs_kind) || !cct_cg_value_kind_is_numeric(rhs_kind)) {
        if (lhs_kind == CCT_CODEGEN_VALUE_POINTER || rhs_kind == CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, expr, "FASE 7A does not support pointer arithmetic");
        } else {
            cg_report_node(cg, expr, "FASE 6B arithmetic codegen requires numeric operands");
        }
        return false;
    }
    if (op == TOKEN_PERCENT &&
        (lhs_kind == CCT_CODEGEN_VALUE_REAL || rhs_kind == CCT_CODEGEN_VALUE_REAL)) {
        cg_report_node(cg, expr, "operator % is not supported for real operands in FASE 6B codegen");
        return false;
    }

    if (out_kind) {
        *out_kind = (lhs_kind == CCT_CODEGEN_VALUE_REAL || rhs_kind == CCT_CODEGEN_VALUE_REAL)
            ? CCT_CODEGEN_VALUE_REAL
            : CCT_CODEGEN_VALUE_INT;
    }
    return true;
}

static bool cg_emit_unary_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    cct_token_type_t op = expr->as.unary_op.operator;
    const char *op_text = NULL;

    switch (op) {
        case TOKEN_PLUS: op_text = "+"; break;
        case TOKEN_MINUS: op_text = "-"; break;
        case TOKEN_NON: op_text = "!"; break;
        case TOKEN_NON_BIT:
            break;
        case TOKEN_STAR:
        case TOKEN_SPECULUM:
            break;
        default:
            cg_report_nodef(cg, expr, "unary operator %s not supported in FASE 4C codegen",
                            cct_token_type_string(op));
            return false;
    }

    if (op == TOKEN_SPECULUM) {
        if (!cg_is_addressable_lvalue_expr(expr->as.unary_op.operand)) {
            cg_report_node(cg, expr, "FASE 7A address-of (SPECULUM) requires addressable lvalue");
            return false;
        }
        fputs("(&(", out);
        if (!cg_emit_lvalue(out, cg, expr->as.unary_op.operand, NULL)) return false;
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (op == TOKEN_STAR) {
        cct_codegen_value_kind_t ptr_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        const cct_ast_type_t *ptr_type = cg_expr_ast_type(cg, expr->as.unary_op.operand);
        if (!ptr_type) {
            cg_report_node(cg, expr, "FASE 7A dereference requires resolvable SPECULUM operand in codegen");
            return false;
        }
        if (!ptr_type->is_pointer || !ptr_type->element_type) {
            cg_report_node(cg, expr, "FASE 7A dereference requires SPECULUM operand type");
            return false;
        }
        const cct_ast_type_t *pointee = ptr_type->element_type;
        if (pointee->is_array || pointee->is_pointer) {
            cg_report_node(cg, expr, "FASE 7A dereference does not support nested/array pointee types");
            return false;
        }
        const char *pointee_c = cg_c_type_for_ast_type(cg, pointee);
        if (!pointee_c) {
            cg_report_node(cg, expr, "FASE 7A dereference pointee type is outside executable SPECULUM subset");
            return false;
        }
        fputs("(*((", out);
        fputs(pointee_c, out);
        fputs("*)", out);
        fputs(cg_current_fail_handler_label(cg)
                  ? cct_cg_runtime_check_not_null_fractum_helper_name()
                  : cct_cg_runtime_check_not_null_helper_name(),
              out);
        fputs("((void*)(", out);
        if (!cg_emit_expr(out, cg, expr->as.unary_op.operand, &ptr_kind)) return false;
        if (ptr_kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, expr, "FASE 7A dereference requires pointer executable operand");
            return false;
        }
        fputs("), ", out);
        cg_emit_c_escaped_string(out, "runtime-fail (bridged): null pointer dereference");
        fputs(")))", out);
        if (out_kind) *out_kind = cg_value_kind_from_ast_type_codegen(cg, pointee);
        return true;
    }

    if (op == TOKEN_NON_BIT) {
        cct_codegen_value_kind_t operand_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("(~(", out);
        if (!cg_emit_expr(out, cg, expr->as.unary_op.operand, &operand_kind)) return false;
        fputs("))", out);
        if (operand_kind != CCT_CODEGEN_VALUE_INT) {
            cg_report_node(cg, expr, "operator NON_BIT requires integer operand");
            return false;
        }
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    cct_codegen_value_kind_t inner_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    fprintf(out, "(%s", op_text);
    if (!cg_emit_expr(out, cg, expr->as.unary_op.operand, &inner_kind)) return false;
    fputc(')', out);

    if ((op == TOKEN_PLUS || op == TOKEN_MINUS) && !cct_cg_value_kind_is_numeric(inner_kind)) {
        cg_report_node(cg, expr, "FASE 6B unary +/- supports only numeric operands");
        return false;
    }
    if (op == TOKEN_NON && inner_kind != CCT_CODEGEN_VALUE_BOOL && inner_kind != CCT_CODEGEN_VALUE_INT) {
        cg_report_node(cg, expr, "FASE 4C unary NON supports integer/boolean operands");
        return false;
    }

    if (out_kind) *out_kind = (op == TOKEN_NON) ? CCT_CODEGEN_VALUE_BOOL : inner_kind;
    return true;
}

static bool cg_emit_obsecro_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    const char *name = expr->as.obsecro.name;
    cct_ast_node_list_t *args = expr->as.obsecro.arguments;
    size_t argc = args ? args->count : 0;

    if (strcmp(name, "verbum_len") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO verbum_len expects exactly one VERBUM argument in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_len(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_len requires VERBUM executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "verbum_concat") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_concat expects exactly two VERBUM arguments in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_concat(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO verbum_concat requires VERBUM executable arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_compare") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_compare expects exactly two VERBUM arguments in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_compare(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO verbum_compare requires VERBUM executable arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "verbum_substring") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO verbum_substring expects exactly three arguments in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_substring(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING ||
            !(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL) ||
            !(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, expr, "OBSECRO verbum_substring requires (VERBUM, integer, integer)");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_trim") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO verbum_trim expects exactly one VERBUM argument in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_trim(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_trim requires VERBUM executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_find") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_find expects exactly two VERBUM arguments in FASE 11B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_find(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO verbum_find requires VERBUM executable arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "verbum_char_at") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_char_at expects exactly two arguments in FASE 17A.1");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ki = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_char_at(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ki)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_char_at requires VERBUM first argument");
            return false;
        }
        if (!(ki == CCT_CODEGEN_VALUE_INT || ki == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO verbum_char_at requires integer index");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "verbum_from_char") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO verbum_from_char expects exactly one integer argument in FASE 17A.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_from_char(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_from_char requires integer byte argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_to_upper") == 0 ||
        strcmp(name, "verbum_to_lower") == 0 ||
        strcmp(name, "verbum_trim_left") == 0 ||
        strcmp(name, "verbum_trim_right") == 0 ||
        strcmp(name, "verbum_reverse") == 0 ||
        strcmp(name, "verbum_is_ascii") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 18A.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs(strcmp(name, "verbum_to_upper") == 0 ? "cct_rt_verbum_to_upper(" :
              strcmp(name, "verbum_to_lower") == 0 ? "cct_rt_verbum_to_lower(" :
              strcmp(name, "verbum_trim_left") == 0 ? "cct_rt_verbum_trim_left(" :
              strcmp(name, "verbum_trim_right") == 0 ? "cct_rt_verbum_trim_right(" :
              strcmp(name, "verbum_reverse") == 0 ? "cct_rt_verbum_reverse(" :
              "cct_rt_verbum_is_ascii(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            if (strcmp(name, "verbum_is_ascii") == 0) {
                *out_kind = CCT_CODEGEN_VALUE_BOOL;
            } else {
                *out_kind = CCT_CODEGEN_VALUE_STRING;
            }
        }
        return true;
    }

    if (strcmp(name, "verbum_starts_with") == 0 ||
        strcmp(name, "verbum_ends_with") == 0 ||
        strcmp(name, "verbum_strip_prefix") == 0 ||
        strcmp(name, "verbum_strip_suffix") == 0 ||
        strcmp(name, "verbum_last_find") == 0 ||
        strcmp(name, "verbum_count_occurrences") == 0 ||
        strcmp(name, "verbum_equals_ignore_case") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two VERBUM arguments in FASE 18A.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, expr, "OBSECRO %s requires VERBUM executable arguments", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            if (strcmp(name, "verbum_starts_with") == 0 ||
                strcmp(name, "verbum_ends_with") == 0 ||
                strcmp(name, "verbum_equals_ignore_case") == 0) {
                *out_kind = CCT_CODEGEN_VALUE_BOOL;
            } else if (strcmp(name, "verbum_last_find") == 0 ||
                       strcmp(name, "verbum_count_occurrences") == 0) {
                *out_kind = CCT_CODEGEN_VALUE_INT;
            } else {
                *out_kind = CCT_CODEGEN_VALUE_STRING;
            }
        }
        return true;
    }

    if (strcmp(name, "verbum_replace") == 0 || strcmp(name, "verbum_replace_all") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly three VERBUM arguments in FASE 18A.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING || k2 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, expr, "OBSECRO %s requires VERBUM executable arguments", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_trim_char") == 0 ||
        strcmp(name, "verbum_repeat") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two arguments in FASE 18A.1", name);
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ki = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ki)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument", name);
            return false;
        }
        if (!(ki == CCT_CODEGEN_VALUE_INT || ki == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer second argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_pad_left") == 0 ||
        strcmp(name, "verbum_pad_right") == 0 ||
        strcmp(name, "verbum_center") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly three arguments in FASE 18A.1", name);
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kw = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kf = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kw)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &kf)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument", name);
            return false;
        }
        if (!(kw == CCT_CODEGEN_VALUE_INT || kw == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer width argument", name);
            return false;
        }
        if (!(kf == CCT_CODEGEN_VALUE_INT || kf == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer fill-byte argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_find_from") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO verbum_find_from expects exactly (VERBUM, VERBUM, integer) in FASE 18A.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_find_from(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING || k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO verbum_find_from requires VERBUM in first two arguments");
            return false;
        }
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO verbum_find_from requires integer offset argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "verbum_slice") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO verbum_slice expects exactly (VERBUM, integer, integer) in FASE 18A.1");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kstart = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t klen = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_slice(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kstart)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &klen)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_slice requires VERBUM first argument");
            return false;
        }
        if (!(kstart == CCT_CODEGEN_VALUE_INT || kstart == CCT_CODEGEN_VALUE_BOOL) ||
            !(klen == CCT_CODEGEN_VALUE_INT || klen == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, expr, "OBSECRO verbum_slice requires integer start/length arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_split") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_split expects exactly (VERBUM, VERBUM) in FASE 18A.2");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksep = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_verbum_split(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ksep)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING || ksep != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO verbum_split requires VERBUM arguments");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "verbum_split_char") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_split_char expects exactly (VERBUM, integer) in FASE 18A.2");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kc = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_verbum_split_char(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kc)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_split_char requires VERBUM first argument");
            return false;
        }
        if (!(kc == CCT_CODEGEN_VALUE_INT || kc == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO verbum_split_char requires integer separator byte");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "verbum_join") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO verbum_join expects exactly (parts, VERBUM) in FASE 18A.2");
            return false;
        }
        cct_codegen_value_kind_t kp = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksep = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_verbum_join((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kp)) return false;
        if (kp != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO verbum_join requires fluxus pointer as first argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ksep)) return false;
        if (ksep != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO verbum_join requires VERBUM separator as second argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "verbum_lines") == 0 || strcmp(name, "verbum_words") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 18A.2", name);
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "char_is_digit") == 0 ||
        strcmp(name, "char_is_alpha") == 0 ||
        strcmp(name, "char_is_whitespace") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer argument in FASE 17A.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer byte argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "args_argc") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO args_argc expects exactly zero arguments in FASE 17A.2");
            return false;
        }
        fputs("cct_rt_args_argc()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "args_arg") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO args_arg expects exactly one integer argument in FASE 17A.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_args_arg(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO args_arg requires integer index");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "env_get") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO env_get expects exactly one VERBUM argument in FASE 17D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_env_get(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO env_get requires VERBUM argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "env_has") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO env_has expects exactly one VERBUM argument in FASE 17D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_env_has(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO env_has requires VERBUM argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "env_cwd") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO env_cwd expects exactly zero arguments in FASE 17D.1");
            return false;
        }
        fputs("cct_rt_env_cwd()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "time_now_ms") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO time_now_ms expects exactly zero arguments in FASE 17D.2");
            return false;
        }
        fputs("cct_rt_time_now_ms()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "time_now_ns") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO time_now_ns expects exactly zero arguments in FASE 17D.2");
            return false;
        }
        fputs("cct_rt_time_now_ns()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "date_now_unix") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO date_now_unix expects exactly zero arguments in FASE 32D");
            return false;
        }
        fputs("cct_rt_date_now_unix()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "bytes_new") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO bytes_new expects exactly one integer size argument in FASE 17D.3");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_bytes_new(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        if (!(ks == CCT_CODEGEN_VALUE_INT || ks == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO bytes_new requires integer size argument");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "bytes_len") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO bytes_len expects exactly one bytes pointer argument in FASE 17D.3");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_bytes_len((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO bytes_len requires bytes pointer argument");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "bytes_get") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO bytes_get expects exactly (bytes, index) in FASE 17D.3");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ki = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_bytes_get((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO bytes_get requires bytes pointer argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ki)) return false;
        if (!(ki == CCT_CODEGEN_VALUE_INT || ki == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO bytes_get requires integer index argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "scan_init") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO scan_init expects exactly one VERBUM argument in FASE 17A.3");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_scan_init(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO scan_init requires VERBUM source argument");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "scan_pos") == 0 ||
        strcmp(name, "scan_eof") == 0 ||
        strcmp(name, "scan_peek") == 0 ||
        strcmp(name, "scan_next") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one cursor pointer argument in FASE 17A.3", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires cursor pointer argument", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) {
            if (strcmp(name, "scan_eof") == 0) {
                *out_kind = CCT_CODEGEN_VALUE_BOOL;
            } else {
                *out_kind = CCT_CODEGEN_VALUE_INT;
            }
        }
        return true;
    }

    if (strcmp(name, "fractum_to_verbum") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fractum_to_verbum expects exactly one FRACTUM argument in FASE 20A.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fractum_to_verbum requires FRACTUM argument in FASE 20A.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "json_handle_ptr") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO json_handle_ptr expects exactly one integer handle argument in FASE 20A.5");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)(intptr_t)(long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO json_handle_ptr requires integer handle argument in FASE 20A.5");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "crypto_sha256_text") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO crypto_sha256_text expects exactly one VERBUM argument in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_sha256_text(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_sha256_text requires VERBUM argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_sha256_bytes") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO crypto_sha256_bytes expects exactly (bytes, len) in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_sha256_bytes((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_sha256_bytes requires bytes pointer as first argument in FASE 32A");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO crypto_sha256_bytes requires integer len as second argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_sha512_text") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO crypto_sha512_text expects exactly one VERBUM argument in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_sha512_text(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_sha512_text requires VERBUM argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_sha512_bytes") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO crypto_sha512_bytes expects exactly (bytes, len) in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_sha512_bytes((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_sha512_bytes requires bytes pointer as first argument in FASE 32A");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO crypto_sha512_bytes requires integer len as second argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_hmac_sha256") == 0 || strcmp(name, "crypto_hmac_sha512") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (key, message) in FASE 32A", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM key as first argument in FASE 32A", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM message as second argument in FASE 32A", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_pbkdf2_sha256") == 0) {
        if (argc != 4) {
            cg_report_node(cg, expr, "OBSECRO crypto_pbkdf2_sha256 expects exactly (password, salt, iterations, key_length) in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_pbkdf2_sha256(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_pbkdf2_sha256 requires VERBUM password as first argument in FASE 32A");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO crypto_pbkdf2_sha256 requires VERBUM salt as second argument in FASE 32A");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO crypto_pbkdf2_sha256 requires integer iterations as third argument in FASE 32A");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (!(k3 == CCT_CODEGEN_VALUE_INT || k3 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[3], "OBSECRO crypto_pbkdf2_sha256 requires integer key_length as fourth argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "crypto_csprng_bytes") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO crypto_csprng_bytes expects exactly one integer count argument in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("((void*)cct_rt_crypto_csprng_bytes(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_csprng_bytes requires integer count argument in FASE 32A");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "crypto_constant_time_compare") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO crypto_constant_time_compare expects exactly (a, b) in FASE 32A");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_crypto = true;
        fputs("cct_rt_crypto_constant_time_compare(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO crypto_constant_time_compare requires VERBUM a as first argument in FASE 32A");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO crypto_constant_time_compare requires VERBUM b as second argument in FASE 32A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "regex_builtin_compile") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_compile expects exactly (pattern, flags) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("((void*)cct_rt_regex_compile(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_compile requires VERBUM pattern as first argument in FASE 32C");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_compile requires integer flags as second argument in FASE 32C");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "regex_builtin_match") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_match expects exactly (handle, text) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("cct_rt_regex_match((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_match requires regex handle pointer as first argument in FASE 32C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_match requires VERBUM text as second argument in FASE 32C");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "regex_builtin_search") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_search expects exactly (handle, text) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("cct_rt_regex_search((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_search requires regex handle pointer as first argument in FASE 32C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_search requires VERBUM text as second argument in FASE 32C");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "regex_builtin_find_all") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_find_all expects exactly (handle, text) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("((void*)cct_rt_regex_find_all((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_find_all requires regex handle pointer as first argument in FASE 32C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_find_all requires VERBUM text as second argument in FASE 32C");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "regex_builtin_replace") == 0) {
        if (argc != 4) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_replace expects exactly (handle, text, replacement, all) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("cct_rt_regex_replace((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_replace requires regex handle pointer as first argument in FASE 32C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_replace requires VERBUM text as second argument in FASE 32C");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[2], "OBSECRO regex_builtin_replace requires VERBUM replacement as third argument in FASE 32C");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (!(k3 == CCT_CODEGEN_VALUE_INT || k3 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[3], "OBSECRO regex_builtin_replace requires integer all flag as fourth argument in FASE 32C");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "regex_builtin_split") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_split expects exactly (handle, text) in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        fputs("((void*)cct_rt_regex_split((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_split requires regex handle pointer as first argument in FASE 32C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO regex_builtin_split requires VERBUM text as second argument in FASE 32C");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "regex_builtin_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO regex_builtin_last_error expects no arguments in FASE 32C");
            return false;
        }
        cg->uses_regex = true;
        fputs("cct_rt_regex_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "toml_builtin_parse") == 0 || strcmp(name, "toml_builtin_parse_file") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 32E", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_toml = true;
        fputs(strcmp(name, "toml_builtin_parse") == 0 ? "cct_rt_toml_parse(" : "cct_rt_toml_parse_file(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument in FASE 32E", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "toml_builtin_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO toml_builtin_last_error expects no arguments in FASE 32E");
            return false;
        }
        cg->uses_toml = true;
        fputs("cct_rt_toml_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "toml_builtin_type") == 0 ||
        strcmp(name, "toml_builtin_get_string") == 0 ||
        strcmp(name, "toml_builtin_get_int") == 0 ||
        strcmp(name, "toml_builtin_get_real") == 0 ||
        strcmp(name, "toml_builtin_get_bool") == 0 ||
        strcmp(name, "toml_builtin_get_subdoc") == 0 ||
        strcmp(name, "toml_builtin_array_len") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (doc, key) in FASE 32E", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_toml = true;
        fprintf(out, "%s(", strcmp(name, "toml_builtin_get_subdoc") == 0 ? "cct_rt_toml_get_subdoc" :
                                           strcmp(name, "toml_builtin_type") == 0 ? "cct_rt_toml_type" :
                                           strcmp(name, "toml_builtin_get_string") == 0 ? "cct_rt_toml_get_string" :
                                           strcmp(name, "toml_builtin_get_int") == 0 ? "cct_rt_toml_get_int" :
                                           strcmp(name, "toml_builtin_get_real") == 0 ? "cct_rt_toml_get_real" :
                                           strcmp(name, "toml_builtin_get_bool") == 0 ? "cct_rt_toml_get_bool" :
                                           "cct_rt_toml_array_len");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires document handle integer as first argument in FASE 32E", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM key as second argument in FASE 32E", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            if (strcmp(name, "toml_builtin_get_subdoc") == 0) *out_kind = CCT_CODEGEN_VALUE_INT;
            else if (strcmp(name, "toml_builtin_get_real") == 0) *out_kind = CCT_CODEGEN_VALUE_REAL;
            else if (strcmp(name, "toml_builtin_get_bool") == 0) *out_kind = CCT_CODEGEN_VALUE_BOOL;
            else if (strcmp(name, "toml_builtin_get_string") == 0) *out_kind = CCT_CODEGEN_VALUE_STRING;
            else *out_kind = CCT_CODEGEN_VALUE_INT;
        }
        return true;
    }

    if (strcmp(name, "toml_builtin_array_item_string") == 0 ||
        strcmp(name, "toml_builtin_array_item_int") == 0 ||
        strcmp(name, "toml_builtin_array_item_real") == 0 ||
        strcmp(name, "toml_builtin_array_item_bool") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (doc, key, index) in FASE 32E", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_toml = true;
        fprintf(out, "%s(", strcmp(name, "toml_builtin_array_item_string") == 0 ? "cct_rt_toml_array_item_string" :
                                           strcmp(name, "toml_builtin_array_item_int") == 0 ? "cct_rt_toml_array_item_int" :
                                           strcmp(name, "toml_builtin_array_item_real") == 0 ? "cct_rt_toml_array_item_real" :
                                           "cct_rt_toml_array_item_bool");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires document handle integer as first argument in FASE 32E", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM key as second argument in FASE 32E", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer index as third argument in FASE 32E", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            if (strcmp(name, "toml_builtin_array_item_string") == 0) *out_kind = CCT_CODEGEN_VALUE_STRING;
            else if (strcmp(name, "toml_builtin_array_item_real") == 0) *out_kind = CCT_CODEGEN_VALUE_REAL;
            else if (strcmp(name, "toml_builtin_array_item_bool") == 0) *out_kind = CCT_CODEGEN_VALUE_BOOL;
            else *out_kind = CCT_CODEGEN_VALUE_INT;
        }
        return true;
    }

    if (strcmp(name, "toml_builtin_expand_env") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO toml_builtin_expand_env expects exactly one doc handle in FASE 32E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_toml = true;
        fputs("cct_rt_toml_expand_env(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO toml_builtin_expand_env requires doc handle integer argument in FASE 32E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "toml_builtin_stringify") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO toml_builtin_stringify expects exactly one doc handle in FASE 32E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_toml = true;
        fputs("cct_rt_toml_stringify(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO toml_builtin_stringify requires doc handle integer argument in FASE 32E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "compress_builtin_gzip_compress_text") == 0 ||
        strcmp(name, "compress_builtin_gzip_decompress_text") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 32F", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_compress = true;
        fprintf(out, "%s(", strcmp(name, "compress_builtin_gzip_compress_text") == 0 ? "cct_rt_gzip_compress_text" : "cct_rt_gzip_decompress_text");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument in FASE 32F", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "compress_builtin_gzip_compress_bytes") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO compress_builtin_gzip_compress_bytes expects exactly (bytes, len) in FASE 32F");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_compress = true;
        fputs("cct_rt_gzip_compress_bytes((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO compress_builtin_gzip_compress_bytes requires bytes pointer as first argument in FASE 32F");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO compress_builtin_gzip_compress_bytes requires integer length as second argument in FASE 32F");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "compress_builtin_gzip_decompress_bytes") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO compress_builtin_gzip_decompress_bytes expects exactly one VERBUM argument in FASE 32F");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_compress = true;
        fputs("((void*)cct_rt_gzip_decompress_bytes(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO compress_builtin_gzip_decompress_bytes requires VERBUM argument in FASE 32F");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "compress_builtin_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO compress_builtin_last_error expects no arguments in FASE 32F");
            return false;
        }
        cg->uses_compress = true;
        fputs("cct_rt_compress_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "filetype_builtin_detect_path") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO filetype_builtin_detect_path expects exactly one VERBUM argument in FASE 32G");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_filetype = true;
        fputs("cct_rt_filetype_detect_path(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO filetype_builtin_detect_path requires VERBUM argument in FASE 32G");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "filetype_builtin_detect_bytes") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO filetype_builtin_detect_bytes expects exactly (bytes, len) in FASE 32G");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_filetype = true;
        fputs("cct_rt_filetype_detect_bytes((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO filetype_builtin_detect_bytes requires bytes pointer as first argument in FASE 32G");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO filetype_builtin_detect_bytes requires integer length as second argument in FASE 32G");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fluxus_contains_verbum") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO fluxus_contains_verbum expects exactly (flux, value) in FASE 33D");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fluxus_contains_verbum((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_contains_verbum requires fluxus pointer as first argument in FASE 33D");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fluxus_contains_verbum requires VERBUM value as second argument in FASE 33D");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "signal_builtin_is_supported") == 0 ||
        strcmp(name, "signal_builtin_install") == 0 ||
        strcmp(name, "signal_builtin_last_kind") == 0 ||
        strcmp(name, "signal_builtin_last_sequence") == 0 ||
        strcmp(name, "signal_builtin_last_unix_ms") == 0 ||
        strcmp(name, "signal_builtin_check_shutdown") == 0 ||
        strcmp(name, "signal_builtin_received_sigterm") == 0 ||
        strcmp(name, "signal_builtin_received_sigint") == 0 ||
        strcmp(name, "signal_builtin_received_sighup") == 0) {
        if (argc != 0) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects no arguments in FASE 34D", name);
            return false;
        }
        cg->uses_signal = true;
        if (strcmp(name, "signal_builtin_is_supported") == 0) fputs("cct_rt_signal_is_supported()", out);
        else if (strcmp(name, "signal_builtin_install") == 0) fputs("cct_rt_signal_install()", out);
        else if (strcmp(name, "signal_builtin_last_kind") == 0) fputs("cct_rt_signal_last_kind()", out);
        else if (strcmp(name, "signal_builtin_last_sequence") == 0) fputs("cct_rt_signal_last_sequence()", out);
        else if (strcmp(name, "signal_builtin_last_unix_ms") == 0) fputs("cct_rt_signal_last_unix_ms()", out);
        else if (strcmp(name, "signal_builtin_check_shutdown") == 0) fputs("cct_rt_signal_check_shutdown()", out);
        else if (strcmp(name, "signal_builtin_received_sigterm") == 0) fputs("cct_rt_signal_received_sigterm()", out);
        else if (strcmp(name, "signal_builtin_received_sigint") == 0) fputs("cct_rt_signal_received_sigint()", out);
        else fputs("cct_rt_signal_received_sighup()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "signal_builtin_clear") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO signal_builtin_clear expects no arguments in FASE 34D");
            return false;
        }
        cg->uses_signal = true;
        fputs("cct_rt_signal_clear()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
        return true;
    }

    if (strcmp(name, "signal_builtin_raise_self") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO signal_builtin_raise_self expects exactly one integer code in FASE 34D");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_signal = true;
        fputs("cct_rt_signal_raise_self(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO signal_builtin_raise_self requires integer code argument in FASE 34D");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "gettext_builtin_catalog_new") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_catalog_new expects exactly (locale) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_catalog_new(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_catalog_new requires VERBUM locale argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "gettext_builtin_catalog_add") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_catalog_add expects exactly (catalog, key, value) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_catalog_add((long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_catalog_add requires catalog handle as first argument in FASE 33E");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO gettext_builtin_catalog_add requires VERBUM key as second argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[2], "OBSECRO gettext_builtin_catalog_add requires VERBUM value as third argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "gettext_builtin_catalog_add_plural") == 0) {
        if (argc != 5) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_catalog_add_plural expects exactly (catalog, singular, plural, translated_singular, translated_plural) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k4 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_catalog_add_plural((long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_catalog_add_plural requires catalog handle as first argument in FASE 33E");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO gettext_builtin_catalog_add_plural requires VERBUM singular key as second argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[2], "OBSECRO gettext_builtin_catalog_add_plural requires VERBUM plural key as third argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (k3 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[3], "OBSECRO gettext_builtin_catalog_add_plural requires VERBUM translated singular as fourth argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[4], &k4)) return false;
        if (k4 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[4], "OBSECRO gettext_builtin_catalog_add_plural requires VERBUM translated plural as fifth argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "gettext_builtin_catalog_load") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_catalog_load expects exactly (path, locale) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_catalog_load(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_catalog_load requires VERBUM path as first argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO gettext_builtin_catalog_load requires VERBUM locale as second argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "gettext_builtin_catalog_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_catalog_last_error expects no arguments in FASE 33E");
            return false;
        }
        fputs("cct_rt_gettext_catalog_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "gettext_builtin_translate") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_translate expects exactly (catalog, key) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_translate((long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_translate requires catalog handle as first argument in FASE 33E");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO gettext_builtin_translate requires VERBUM key as second argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "gettext_builtin_translate_plural") == 0) {
        if (argc != 4) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_translate_plural expects exactly (catalog, singular, plural, count) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_translate_plural((long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_translate_plural requires catalog handle as first argument in FASE 33E");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO gettext_builtin_translate_plural requires VERBUM singular as second argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[2], "OBSECRO gettext_builtin_translate_plural requires VERBUM plural as third argument in FASE 33E");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (!(k3 == CCT_CODEGEN_VALUE_INT || k3 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[3], "OBSECRO gettext_builtin_translate_plural requires integer count as fourth argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "gettext_builtin_default_set") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_default_set expects exactly (catalog) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_default_set((long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_default_set requires catalog handle as first argument in FASE 33E");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "gettext_builtin_default_translate") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO gettext_builtin_default_translate expects exactly (key) in FASE 33E");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_gettext_default_translate(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO gettext_builtin_default_translate requires VERBUM key argument in FASE 33E");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "image_builtin_load") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO image_builtin_load expects exactly one VERBUM path argument in FASE 32I");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_image_ops = true;
        fputs("((void*)cct_rt_image_load(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO image_builtin_load requires VERBUM path argument in FASE 32I");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "image_builtin_free") == 0 ||
        strcmp(name, "image_builtin_get_width") == 0 ||
        strcmp(name, "image_builtin_get_height") == 0 ||
        strcmp(name, "image_builtin_get_channels") == 0 ||
        strcmp(name, "image_builtin_get_format") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO image builtin expects exactly one handle argument in FASE 32I");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_image_ops = true;
        fprintf(out, "%s(", strcmp(name, "image_builtin_free") == 0 ? "cct_rt_image_free" :
                         strcmp(name, "image_builtin_get_width") == 0 ? "cct_rt_image_get_width" :
                         strcmp(name, "image_builtin_get_height") == 0 ? "cct_rt_image_get_height" :
                         strcmp(name, "image_builtin_get_channels") == 0 ? "cct_rt_image_get_channels" :
                         "cct_rt_image_get_format");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO image builtin requires image handle pointer argument in FASE 32I");
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            *out_kind = strcmp(name, "image_builtin_free") == 0 ? CCT_CODEGEN_VALUE_UNKNOWN : CCT_CODEGEN_VALUE_INT;
        }
        return true;
    }

    if (strcmp(name, "image_builtin_save") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO image_builtin_save expects exactly (handle, path, quality) in FASE 32I");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_image_ops = true;
        fputs("cct_rt_image_save(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO image_builtin_save requires image handle pointer as first argument in FASE 32I");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO image_builtin_save requires VERBUM path as second argument in FASE 32I");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO image_builtin_save requires integer quality as third argument in FASE 32I");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "image_builtin_resize") == 0 ||
        strcmp(name, "image_builtin_crop") == 0 ||
        strcmp(name, "image_builtin_rotate") == 0 ||
        strcmp(name, "image_builtin_convert") == 0) {
        size_t expected = strcmp(name, "image_builtin_crop") == 0 ? 5u :
                          strcmp(name, "image_builtin_resize") == 0 ? 4u : 2u;
        if (argc != expected) {
            cg_report_node(cg, expr, "OBSECRO image transform builtin received unexpected arity in FASE 32I");
            return false;
        }
        cg->uses_image_ops = true;
        fprintf(out, "((void*)%s(", strcmp(name, "image_builtin_resize") == 0 ? "cct_rt_image_resize" :
                                   strcmp(name, "image_builtin_crop") == 0 ? "cct_rt_image_crop" :
                                   strcmp(name, "image_builtin_rotate") == 0 ? "cct_rt_image_rotate" :
                                   "cct_rt_image_convert");
        for (u32 i = 0; i < argc; i++) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            if (i > 0) fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[i], &k)) return false;
            if (i == 0 && k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[i], "OBSECRO image transform builtin requires image handle pointer as first argument in FASE 32I");
                return false;
            }
            if (i > 0 && !(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, args->nodes[i], "OBSECRO image transform builtin requires integer scalar arguments after the handle in FASE 32I");
                return false;
            }
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "image_builtin_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO image_builtin_last_error expects no arguments in FASE 32I");
            return false;
        }
        cg->uses_image_ops = true;
        fputs("cct_rt_image_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "mail_builtin_smtp_send") == 0) {
        cct_codegen_value_kind_t kinds[10];
        if (argc != 10) {
            cg_report_node(cg, expr, "OBSECRO mail_builtin_smtp_send expects exactly 10 arguments in FASE 37A");
            return false;
        }
        memset(kinds, 0, sizeof(kinds));
        cg->uses_mail = true;
        cg->uses_crypto = true;
        fputs("cct_rt_mail_smtp_send(", out);
        for (size_t i = 0; i < 10; i++) {
            if (i > 0) fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[i], &kinds[i])) return false;
        }
        if (kinds[0] != CCT_CODEGEN_VALUE_STRING || kinds[1] != CCT_CODEGEN_VALUE_INT ||
            kinds[2] != CCT_CODEGEN_VALUE_STRING || kinds[3] != CCT_CODEGEN_VALUE_STRING ||
            !(kinds[4] == CCT_CODEGEN_VALUE_INT || kinds[4] == CCT_CODEGEN_VALUE_BOOL) ||
            kinds[5] != CCT_CODEGEN_VALUE_BOOL || kinds[6] != CCT_CODEGEN_VALUE_BOOL ||
            kinds[7] != CCT_CODEGEN_VALUE_STRING || kinds[8] != CCT_CODEGEN_VALUE_STRING ||
            kinds[9] != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, expr, "OBSECRO mail_builtin_smtp_send recebeu tipos invalidos em FASE 37A");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strncmp(name, "instr_builtin_", 14) == 0) {
        cg->uses_instrument = true;

        if (strcmp(name, "instr_builtin_enable") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            if (argc != 1) {
                cg_report_node(cg, expr, "OBSECRO instr_builtin_enable expects exactly one integer mode argument in FASE 38A");
                return false;
            }
            fputs("cct_rt_instr_enable(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, args->nodes[0], "OBSECRO instr_builtin_enable requires integer mode argument in FASE 38A");
                return false;
            }
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
            return true;
        }

        if (strcmp(name, "instr_builtin_disable") == 0 ||
            strcmp(name, "instr_builtin_is_enabled") == 0 ||
            strcmp(name, "instr_builtin_mode") == 0 ||
            strcmp(name, "instr_builtin_buffer_count") == 0 ||
            strcmp(name, "instr_builtin_buffer_clear") == 0) {
            if (argc != 0) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects no arguments in FASE 38A", name);
                return false;
            }
            if (strcmp(name, "instr_builtin_disable") == 0) {
                fputs("cct_rt_instr_disable()", out);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
            } else if (strcmp(name, "instr_builtin_is_enabled") == 0) {
                fputs("cct_rt_instr_is_enabled()", out);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
            } else if (strcmp(name, "instr_builtin_mode") == 0) {
                fputs("cct_rt_instr_mode()", out);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
            } else if (strcmp(name, "instr_builtin_buffer_count") == 0) {
                fputs("cct_rt_instr_buffer_count()", out);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
            } else {
                fputs("cct_rt_instr_buffer_clear()", out);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
            }
            return true;
        }

        if (strcmp(name, "instr_builtin_span_begin") == 0 ||
            strcmp(name, "instr_builtin_event") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            if (argc != 2) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two VERBUM arguments in FASE 38A", name);
                return false;
            }
            fputs(strcmp(name, "instr_builtin_span_begin") == 0 ? "cct_rt_instr_span_begin(" : "cct_rt_instr_event(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (k0 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM as first argument in FASE 38A", name);
                return false;
            }
            fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (k1 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM as second argument in FASE 38A", name);
                return false;
            }
            fputs(")", out);
            if (out_kind) {
                *out_kind = (strcmp(name, "instr_builtin_span_begin") == 0)
                    ? CCT_CODEGEN_VALUE_INT
                    : CCT_CODEGEN_VALUE_NIHIL;
            }
            return true;
        }

        if (strcmp(name, "instr_builtin_span_end") == 0 ||
            strcmp(name, "instr_builtin_buffer_span_id") == 0 ||
            strcmp(name, "instr_builtin_buffer_parent_id") == 0 ||
            strcmp(name, "instr_builtin_buffer_name") == 0 ||
            strcmp(name, "instr_builtin_buffer_category") == 0 ||
            strcmp(name, "instr_builtin_buffer_start_us") == 0 ||
            strcmp(name, "instr_builtin_buffer_end_us") == 0 ||
            strcmp(name, "instr_builtin_buffer_closed") == 0 ||
            strcmp(name, "instr_builtin_buffer_attr_count") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            if (argc != 1) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer argument in FASE 38A", name);
                return false;
            }
            if (strcmp(name, "instr_builtin_span_end") == 0) fputs("cct_rt_instr_span_end(", out);
            else if (strcmp(name, "instr_builtin_buffer_span_id") == 0) fputs("cct_rt_instr_buffer_span_id(", out);
            else if (strcmp(name, "instr_builtin_buffer_parent_id") == 0) fputs("cct_rt_instr_buffer_parent_id(", out);
            else if (strcmp(name, "instr_builtin_buffer_name") == 0) fputs("cct_rt_instr_buffer_name(", out);
            else if (strcmp(name, "instr_builtin_buffer_category") == 0) fputs("cct_rt_instr_buffer_category(", out);
            else if (strcmp(name, "instr_builtin_buffer_start_us") == 0) fputs("cct_rt_instr_buffer_start_us(", out);
            else if (strcmp(name, "instr_builtin_buffer_end_us") == 0) fputs("cct_rt_instr_buffer_end_us(", out);
            else if (strcmp(name, "instr_builtin_buffer_closed") == 0) fputs("cct_rt_instr_buffer_closed(", out);
            else fputs("cct_rt_instr_buffer_attr_count(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer argument in FASE 38A", name);
                return false;
            }
            fputs(")", out);
            if (out_kind) {
                if (strcmp(name, "instr_builtin_span_end") == 0) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
                else if (strcmp(name, "instr_builtin_buffer_name") == 0 ||
                         strcmp(name, "instr_builtin_buffer_category") == 0) *out_kind = CCT_CODEGEN_VALUE_STRING;
                else *out_kind = CCT_CODEGEN_VALUE_INT;
            }
            return true;
        }

        if (strcmp(name, "instr_builtin_span_attr") == 0) {
            cct_codegen_value_kind_t kinds[3];
            if (argc != 3) {
                cg_report_node(cg, expr, "OBSECRO instr_builtin_span_attr expects exactly (span_id, key, value) in FASE 38A");
                return false;
            }
            memset(kinds, 0, sizeof(kinds));
            fputs("cct_rt_instr_span_attr(", out);
            for (size_t i = 0; i < 3; i++) {
                if (i > 0) fputs(", ", out);
                if (!cg_emit_expr(out, cg, args->nodes[i], &kinds[i])) return false;
            }
            if (!(kinds[0] == CCT_CODEGEN_VALUE_INT || kinds[0] == CCT_CODEGEN_VALUE_BOOL) ||
                kinds[1] != CCT_CODEGEN_VALUE_STRING ||
                kinds[2] != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, expr, "OBSECRO instr_builtin_span_attr recebeu tipos invalidos em FASE 38A");
                return false;
            }
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
            return true;
        }

        if (strcmp(name, "instr_builtin_buffer_attr_key") == 0 ||
            strcmp(name, "instr_builtin_buffer_attr_value") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            if (argc != 2) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two integer arguments in FASE 38A", name);
                return false;
            }
            fputs(strcmp(name, "instr_builtin_buffer_attr_key") == 0
                ? "cct_rt_instr_buffer_attr_key("
                : "cct_rt_instr_buffer_attr_value(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer buffer index in FASE 38A", name);
                return false;
            }
            fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer attr index in FASE 38A", name);
                return false;
            }
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
            return true;
        }
    }

    if (strcmp(name, "pg_builtin_open") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO pg_builtin_open expects exactly one VERBUM conninfo argument in FASE 36A");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_postgres = true;
        fputs("((void*)cct_rt_pg_db_open(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_open requires VERBUM conninfo argument in FASE 36A");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "pg_builtin_is_open") == 0 || strcmp(name, "pg_builtin_last_error") == 0 ||
        strcmp(name, "pg_builtin_query") == 0 || strcmp(name, "pg_builtin_rows_count") == 0 ||
        strcmp(name, "pg_builtin_rows_columns") == 0 || strcmp(name, "pg_builtin_rows_next") == 0 ||
        strcmp(name, "pg_builtin_rows_get_text") == 0 || strcmp(name, "pg_builtin_rows_is_null") == 0 ||
        strcmp(name, "pg_builtin_poll_channel") == 0 || strcmp(name, "pg_builtin_poll_payload") == 0) {
        cg->uses_postgres = true;
        if (strcmp(name, "pg_builtin_is_open") == 0 || strcmp(name, "pg_builtin_last_error") == 0 ||
            strcmp(name, "pg_builtin_rows_count") == 0 || strcmp(name, "pg_builtin_rows_columns") == 0 ||
            strcmp(name, "pg_builtin_rows_next") == 0 || strcmp(name, "pg_builtin_poll_payload") == 0) {
            if (argc != 1) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one pointer argument in FASE 36A", name);
                return false;
            }
        }
        if (strcmp(name, "pg_builtin_query") == 0 || strcmp(name, "pg_builtin_poll_channel") == 0 ||
            strcmp(name, "pg_builtin_rows_get_text") == 0 || strcmp(name, "pg_builtin_rows_is_null") == 0) {
            if (argc != 2) {
                cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two arguments in FASE 36A", name);
                return false;
            }
        }
        if (strcmp(name, "pg_builtin_is_open") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("(cct_rt_pg_db_is_open((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_is_open requires db pointer in FASE 36A");
                return false;
            }
            fputs(")))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
            return true;
        }
        if (strcmp(name, "pg_builtin_last_error") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("cct_rt_pg_db_last_error((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_last_error requires db pointer in FASE 36A");
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
            return true;
        }
        if (strcmp(name, "pg_builtin_query") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("((void*)cct_rt_pg_db_query((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (k0 != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_query requires db pointer in FASE 36A");
                return false;
            }
            fputs("), ", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (k1 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[1], "OBSECRO pg_builtin_query requires VERBUM sql in FASE 36A");
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
            return true;
        }
        if (strcmp(name, "pg_builtin_rows_count") == 0 || strcmp(name, "pg_builtin_rows_columns") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            fprintf(out, "%s((void*)(", strcmp(name, "pg_builtin_rows_count") == 0 ? "cct_rt_pg_rows_count" : "cct_rt_pg_rows_columns");
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires rows pointer in FASE 36A", name);
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
            return true;
        }
        if (strcmp(name, "pg_builtin_rows_next") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("(cct_rt_pg_rows_next((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_rows_next requires rows pointer in FASE 36A");
                return false;
            }
            fputs(")))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
            return true;
        }
        if (strcmp(name, "pg_builtin_rows_get_text") == 0 || strcmp(name, "pg_builtin_rows_is_null") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            fprintf(out, "%s((void*)(", strcmp(name, "pg_builtin_rows_get_text") == 0 ? "cct_rt_pg_rows_get_text" : "cct_rt_pg_rows_is_null");
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (k0 != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires rows pointer in FASE 36A", name);
                return false;
            }
            fputs("), (long long)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer column in FASE 36A", name);
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = strcmp(name, "pg_builtin_rows_get_text") == 0 ? CCT_CODEGEN_VALUE_STRING : CCT_CODEGEN_VALUE_BOOL;
            return true;
        }
        if (strcmp(name, "pg_builtin_poll_channel") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("cct_rt_pg_db_poll_channel((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (k0 != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_poll_channel requires db pointer in FASE 36A");
                return false;
            }
            fputs("), (long long)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, args->nodes[1], "OBSECRO pg_builtin_poll_channel requires integer timeout in FASE 36A");
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
            return true;
        }
        if (strcmp(name, "pg_builtin_poll_payload") == 0) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("cct_rt_pg_db_poll_payload((void*)(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_POINTER) {
                cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_poll_payload requires db pointer in FASE 36A");
                return false;
            }
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
            return true;
        }
    }

    if (strcmp(name, "db_open") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO db_open expects exactly one VERBUM path argument in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("((void*)cct_rt_db_open(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_open requires VERBUM path argument in FASE 20E.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "db_close") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO db_close expects exactly one db pointer argument in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("cct_rt_db_close((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_close requires db pointer argument in FASE 20E.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
        return true;
    }

    if (strcmp(name, "db_exec") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO db_exec expects exactly (db, sql) in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("cct_rt_db_exec((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_exec requires db pointer as first argument in FASE 20E.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO db_exec requires VERBUM sql as second argument in FASE 20E.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
        return true;
    }

    if (strcmp(name, "db_last_error") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO db_last_error expects exactly one db pointer argument in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("cct_rt_db_last_error((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_last_error requires db pointer argument in FASE 20E.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "db_query") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO db_query expects exactly (db, sql) in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("((void*)cct_rt_db_query((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_query requires db pointer as first argument in FASE 20E.2");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO db_query requires VERBUM sql as second argument in FASE 20E.2");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "rows_next") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO rows_next expects exactly one rows pointer argument in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("(cct_rt_rows_next((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_next requires rows pointer argument in FASE 20E.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "rows_get_text") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO rows_get_text expects exactly (rows, col) in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("cct_rt_rows_get_text((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_get_text requires rows pointer as first argument in FASE 20E.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO rows_get_text requires integer col as second argument in FASE 20E.2");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "rows_get_int") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO rows_get_int expects exactly (rows, col) in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("(cct_rt_rows_get_int((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_get_int requires rows pointer as first argument in FASE 20E.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO rows_get_int requires integer col as second argument in FASE 20E.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "rows_get_real") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO rows_get_real expects exactly (rows, col) in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("(cct_rt_rows_get_real((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_get_real requires rows pointer as first argument in FASE 20E.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO rows_get_real requires integer col as second argument in FASE 20E.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "rows_close") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO rows_close expects exactly one rows pointer argument in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("cct_rt_rows_close((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_close requires rows pointer argument in FASE 20E.2");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
        return true;
    }

    if (strcmp(name, "db_prepare") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO db_prepare expects exactly (db, sql) in FASE 20E.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("((void*)cct_rt_db_prepare((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_prepare requires db pointer as first argument in FASE 20E.3");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO db_prepare requires VERBUM sql as second argument in FASE 20E.3");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "stmt_step") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO stmt_step expects exactly one stmt pointer argument in FASE 20E.3");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fputs("(cct_rt_stmt_step((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO stmt_step requires stmt pointer argument in FASE 20E.3");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "db_scalar_int") == 0 || strcmp(name, "db_scalar_text") == 0) {
        const char *rt_name = strcmp(name, "db_scalar_int") == 0 ? "cct_rt_db_scalar_int" : "cct_rt_db_scalar_text";
        cct_codegen_value_kind_t ret_kind = strcmp(name, "db_scalar_int") == 0 ? CCT_CODEGEN_VALUE_INT : CCT_CODEGEN_VALUE_STRING;
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (db, sql) in FASE 20E.4", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        fprintf(out, "%s((void*)(", rt_name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires db pointer as first argument in FASE 20E.4", name);
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM sql as second argument in FASE 20E.4", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = ret_kind;
        return true;
    }

    if (strcmp(name, "socket_tcp") == 0 || strcmp(name, "socket_udp") == 0) {
        if (argc != 0) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly zero arguments in FASE 20B.1", name);
            return false;
        }
        fprintf(out, "((void*)cct_rt_%s())", name);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "sock_accept") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO sock_accept expects exactly one socket pointer argument in FASE 20B.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_sock_accept((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO sock_accept requires socket pointer argument in FASE 20B.1");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "sock_send") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO sock_send expects exactly (sock, data) in FASE 20B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_sock_send((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO sock_send requires socket pointer argument in FASE 20B.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO sock_send requires VERBUM payload in FASE 20B.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "sock_recv") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO sock_recv expects exactly (sock, max_bytes) in FASE 20B.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_sock_recv((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO sock_recv requires socket pointer argument in FASE 20B.1");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO sock_recv requires integer max_bytes in FASE 20B.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "sock_peer_addr") == 0 || strcmp(name, "sock_local_addr") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one socket pointer argument in FASE 20B.4", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires socket pointer argument in FASE 20B.4", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "sock_last_error") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO sock_last_error expects zero arguments in FASE 20B.4");
            return false;
        }
        fputs("cct_rt_sock_last_error()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "json_arr_handle_new") == 0 ||
        strcmp(name, "json_obj_handle_new") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer size argument in FASE 20A.2", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((long long)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer element size argument in FASE 20A.2", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "json_arr_handle_len") == 0 ||
        strcmp(name, "json_obj_handle_len") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer handle argument in FASE 20A.2", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((long long)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer handle argument in FASE 20A.2", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "json_arr_handle_get") == 0 ||
        strcmp(name, "json_obj_handle_get") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (handle, idx) in FASE 20A.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((long long)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer handle argument in FASE 20A.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer index argument in FASE 20A.2", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "builder_init") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO builder_init expects exactly zero arguments in FASE 17B.1");
            return false;
        }
        fputs("((void*)cct_rt_builder_init())", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "builder_len") == 0 || strcmp(name, "builder_to_verbum") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one builder pointer argument in FASE 17B.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires builder pointer argument", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) {
            if (strcmp(name, "builder_len") == 0) {
                *out_kind = CCT_CODEGEN_VALUE_INT;
            } else {
                *out_kind = CCT_CODEGEN_VALUE_STRING;
            }
        }
        return true;
    }

    if (strcmp(name, "writer_init") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO writer_init expects exactly zero arguments in FASE 17B.2");
            return false;
        }
        fputs("((void*)cct_rt_writer_init())", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "writer_to_verbum") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO writer_to_verbum expects exactly one writer pointer argument in FASE 17B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_writer_to_verbum((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO writer_to_verbum requires writer pointer argument");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_int") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_int expects exactly one integer argument in FASE 11B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_stringify_int(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_stringify_int requires integer executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_real") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_real expects exactly one real argument in FASE 11B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_stringify_real(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_REAL) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_stringify_real requires real executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_float") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_float expects exactly one real argument in FASE 11B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_stringify_float(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_REAL) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_stringify_float requires real executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_parse_int") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_parse_int expects exactly one VERBUM argument in FASE 11B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_parse_int_or_fail(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_parse_int requires VERBUM executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fmt_parse_real") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_parse_real expects exactly one VERBUM argument in FASE 11B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_parse_real_or_fail(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_parse_real requires VERBUM executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "parse_try_int") == 0 ||
        strcmp(name, "parse_try_real") == 0 ||
        strcmp(name, "parse_try_bool") == 0 ||
        strcmp(name, "parse_try_int_hex") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 18A.4", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM executable argument", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "parse_int_hex") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO parse_int_hex expects exactly one VERBUM argument in FASE 18A.4");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_parse_int_hex(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO parse_int_hex requires VERBUM executable argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "parse_is_int") == 0 ||
        strcmp(name, "parse_is_real") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 18A.4", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM executable argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "parse_int_radix") == 0 ||
        strcmp(name, "parse_try_int_radix") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (VERBUM, radix) in FASE 18A.4", name);
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kr = CCT_CODEGEN_VALUE_UNKNOWN;
        if (strcmp(name, "parse_try_int_radix") == 0) {
            fputs("((void*)", out);
        }
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kr)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument", name);
            return false;
        }
        if (!(kr == CCT_CODEGEN_VALUE_INT || kr == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer radix argument", name);
            return false;
        }
        if (strcmp(name, "parse_try_int_radix") == 0) {
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        } else {
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        }
        return true;
    }

    if (strcmp(name, "parse_csv_line") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO parse_csv_line expects exactly (VERBUM, sep) in FASE 18A.4");
            return false;
        }
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kc = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_parse_csv_line(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ks)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kc)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO parse_csv_line requires VERBUM first argument");
            return false;
        }
        if (!(kc == CCT_CODEGEN_VALUE_INT || kc == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO parse_csv_line requires integer separator byte");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "fmt_stringify_int_hex") == 0 ||
        strcmp(name, "fmt_stringify_int_hex_upper") == 0 ||
        strcmp(name, "fmt_stringify_int_oct") == 0 ||
        strcmp(name, "fmt_stringify_int_bin") == 0 ||
        strcmp(name, "fmt_stringify_uint") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer argument in FASE 18A.3", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer executable argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_int_padded") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_int_padded expects exactly (int, width, fill) in FASE 18A.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_stringify_int_padded(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL) ||
            !(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL) ||
            !(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_int_padded requires integer arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_real_prec") == 0 ||
        strcmp(name, "fmt_stringify_real_fixed") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (real, prec) in FASE 18A.3", name);
            return false;
        }
        cct_codegen_value_kind_t kr = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kp = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kr)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kp)) return false;
        if (kr != CCT_CODEGEN_VALUE_REAL) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires real first argument", name);
            return false;
        }
        if (!(kp == CCT_CODEGEN_VALUE_INT || kp == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer precision argument", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_stringify_real_sci") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fmt_stringify_real_sci expects exactly one real argument in FASE 18A.3");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_stringify_real_sci(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_REAL) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_stringify_real_sci requires real argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_format_1") == 0 ||
        strcmp(name, "fmt_format_2") == 0 ||
        strcmp(name, "fmt_format_3") == 0 ||
        strcmp(name, "fmt_format_4") == 0) {
        size_t expected = (strcmp(name, "fmt_format_1") == 0) ? 2 :
                          (strcmp(name, "fmt_format_2") == 0) ? 3 :
                          (strcmp(name, "fmt_format_3") == 0) ? 4 : 5;
        if (argc != expected) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly %zu VERBUM arguments in FASE 18A.3", name, expected);
            return false;
        }
        fprintf(out, "cct_rt_%s(", name);
        for (size_t i = 0; i < argc; i++) {
            cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
            if (i > 0) fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[i], &k)) return false;
            if (k != CCT_CODEGEN_VALUE_STRING) {
                cg_report_nodef(cg, args->nodes[i], "OBSECRO %s requires VERBUM arguments", name);
                return false;
            }
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_repeat_char") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO fmt_repeat_char expects exactly (byte, n) in FASE 18A.3");
            return false;
        }
        cct_codegen_value_kind_t kc = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kn = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_repeat_char(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kc)) return false;
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kn)) return false;
        if (!(kc == CCT_CODEGEN_VALUE_INT || kc == CCT_CODEGEN_VALUE_BOOL) ||
            !(kn == CCT_CODEGEN_VALUE_INT || kn == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, expr, "OBSECRO fmt_repeat_char requires integer arguments");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fmt_table_row") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO fmt_table_row expects exactly (parts, widths, ncols) in FASE 18A.3");
            return false;
        }
        cct_codegen_value_kind_t kp = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kw = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kn = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fmt_table_row((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kp)) return false;
        if (kp != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fmt_table_row requires pointer parts argument");
            return false;
        }
        fputs("), (const long long*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kw)) return false;
        if (kw != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fmt_table_row requires pointer widths argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &kn)) return false;
        if (!(kn == CCT_CODEGEN_VALUE_INT || kn == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO fmt_table_row requires integer ncols argument");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "mem_alloc") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO mem_alloc expects exactly one size argument in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs("void*", out);
        fputs(")", out);
        fputs("cct_rt_mem_alloc(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_alloc requires integer executable size argument in FASE 11D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "mem_realloc") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO mem_realloc expects exactly two arguments in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t kptr = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksize = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs("void*", out);
        fputs(")", out);
        fputs("cct_rt_mem_realloc((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kptr)) return false;
        if (kptr != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_realloc requires pointer as first argument in FASE 11D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ksize)) return false;
        if (!(ksize == CCT_CODEGEN_VALUE_INT || ksize == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO mem_realloc requires integer size as second argument in FASE 11D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "mem_compare") == 0) {
        if (argc != 3) {
            cg_report_node(cg, expr, "OBSECRO mem_compare expects exactly three arguments in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t ka = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_mem_compare((const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ka)) return false;
        if (ka != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_compare requires pointer as first argument in FASE 11D.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO mem_compare requires pointer as second argument in FASE 11D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &ks)) return false;
        if (!(ks == CCT_CODEGEN_VALUE_INT || ks == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO mem_compare requires integer size as third argument in FASE 11D.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "kernel_inb") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO kernel_inb expects exactly one integer port argument in FASE 16B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_svc_inb(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO kernel_inb requires integer port argument in FASE 16B.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fluxus_init") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fluxus_init expects exactly one element-size argument in FASE 11D.3");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs("void*", out);
        fputs(")", out);
        fputs("cct_rt_fluxus_create(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_init requires integer element-size argument in FASE 11D.3");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "fluxus_len") == 0 || strcmp(name, "fluxus_capacity") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one pointer argument in FASE 11D.3", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer executable argument in FASE 11D.3", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fluxus_get") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO fluxus_get expects exactly two arguments in FASE 11D.3");
            return false;
        }
        cct_codegen_value_kind_t kptr = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kidx = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs("void*", out);
        fputs(")", out);
        fputs("cct_rt_fluxus_get((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kptr)) return false;
        if (kptr != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_get requires pointer as first argument in FASE 11D.3");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kidx)) return false;
        if (!(kidx == CCT_CODEGEN_VALUE_INT || kidx == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fluxus_get requires integer index as second argument in FASE 11D.3");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "fluxus_peek") == 0 || strcmp(name, "fluxus_to_ptr") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one fluxus pointer argument in FASE 18C.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer fluxus argument in FASE 18C.1", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "fluxus_contains") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO fluxus_contains expects exactly (flux, elem_ptr) in FASE 18C.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fluxus_contains((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_contains requires pointer fluxus argument in FASE 18C.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fluxus_contains requires pointer elem argument in FASE 18C.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "io_read_line") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO io_read_line expects no arguments in FASE 11E.1");
            return false;
        }
        fputs("cct_rt_io_read_line()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "io_read_char") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO io_read_char expects no arguments in FASE 18B.3");
            return false;
        }
        fputs("cct_rt_io_read_char()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "io_is_tty") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO io_is_tty expects no arguments in FASE 18B.3");
            return false;
        }
        fputs("cct_rt_io_is_tty()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "fs_read_all") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fs_read_all expects exactly one VERBUM path argument in FASE 11E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fs_read_all(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_read_all requires VERBUM path argument in FASE 11E.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fs_exists") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fs_exists expects exactly one VERBUM path argument in FASE 11E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fs_exists(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_exists requires VERBUM path argument in FASE 11E.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "fs_size") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fs_size expects exactly one VERBUM path argument in FASE 11E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fs_size(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_size requires VERBUM path argument in FASE 11E.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fs_is_file") == 0 || strcmp(name, "fs_is_dir") == 0 ||
        strcmp(name, "fs_is_symlink") == 0 || strcmp(name, "fs_is_readable") == 0 ||
        strcmp(name, "fs_is_writable") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM path argument in FASE 18B.2", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM path argument in FASE 18B.2", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "fs_modified_time") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fs_modified_time expects exactly one VERBUM path argument in FASE 18B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fs_modified_time(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_modified_time requires VERBUM path argument in FASE 18B.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "fs_list_dir") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO fs_list_dir expects exactly one VERBUM path argument in FASE 18B.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_fs_list_dir(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_list_dir requires VERBUM path argument in FASE 18B.2");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "fs_create_temp_file") == 0 || strcmp(name, "fs_create_temp_dir") == 0) {
        if (argc != 0) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects zero arguments in FASE 18B.2", name);
            return false;
        }
        fprintf(out, "cct_rt_%s()", name);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "fs_same_file") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO fs_same_file expects exactly two VERBUM path arguments in FASE 18B.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_fs_same_file(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fs_same_file requires VERBUM first argument in FASE 18B.2");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fs_same_file requires VERBUM second argument in FASE 18B.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "path_join") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO path_join expects exactly two VERBUM arguments in FASE 11E.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_path_join(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO path_join requires VERBUM first argument in FASE 11E.2");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO path_join requires VERBUM second argument in FASE 11E.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "path_basename") == 0 ||
        strcmp(name, "path_dirname") == 0 ||
        strcmp(name, "path_ext") == 0 ||
        strcmp(name, "path_stem") == 0 ||
        strcmp(name, "path_normalize") == 0 ||
        strcmp(name, "path_resolve") == 0 ||
        strcmp(name, "path_without_ext") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 18B.4", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument in FASE 18B.4", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "path_is_absolute") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO path_is_absolute expects exactly one VERBUM argument in FASE 18B.4");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_path_is_absolute(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO path_is_absolute requires VERBUM argument in FASE 18B.4");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "path_relative_to") == 0 || strcmp(name, "path_with_ext") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly two VERBUM arguments in FASE 18B.4", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument in FASE 18B.4", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM second argument in FASE 18B.4", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "path_home_dir") == 0 || strcmp(name, "path_temp_dir") == 0) {
        if (argc != 0) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects zero arguments in FASE 18B.4", name);
            return false;
        }
        fprintf(out, "cct_rt_%s()", name);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "path_split") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO path_split expects exactly one VERBUM argument in FASE 18B.4");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_path_split(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO path_split requires VERBUM argument in FASE 18B.4");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "process_run") == 0 || strcmp(name, "process_run_capture") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM command argument in FASE 18D.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM command argument in FASE 18D.1", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            *out_kind = (strcmp(name, "process_run") == 0)
                ? CCT_CODEGEN_VALUE_INT
                : CCT_CODEGEN_VALUE_STRING;
        }
        return true;
    }

    if (strcmp(name, "process_run_with_input") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO process_run_with_input expects exactly (cmd, input) in FASE 18D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_process_run_with_input(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO process_run_with_input requires VERBUM cmd argument in FASE 18D.1");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO process_run_with_input requires VERBUM input argument in FASE 18D.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "process_run_env") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO process_run_env expects exactly (cmd, env_pairs_ptr) in FASE 18D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_process_run_env(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO process_run_env requires VERBUM cmd argument in FASE 18D.1");
            return false;
        }
        fputs(", (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO process_run_env requires pointer env_pairs argument in FASE 18D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "process_run_timeout") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO process_run_timeout expects exactly (cmd, timeout_ms) in FASE 18D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_process_run_timeout(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO process_run_timeout requires VERBUM cmd argument in FASE 18D.1");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO process_run_timeout requires integer timeout_ms argument in FASE 18D.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "hash_crc32") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO hash_crc32 expects exactly one VERBUM argument in FASE 18D.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_hash_crc32(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO hash_crc32 requires VERBUM argument in FASE 18D.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "hash_fnv1a_bytes") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO hash_fnv1a_bytes expects exactly (data_ptr, len) in FASE 18D.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_hash_fnv1a_bytes((const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO hash_fnv1a_bytes requires pointer first argument in FASE 18D.2");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO hash_fnv1a_bytes requires integer len argument in FASE 18D.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "hash_murmur3") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO hash_murmur3 expects exactly (text, seed) in FASE 18D.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_hash_murmur3(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[0], "OBSECRO hash_murmur3 requires VERBUM text argument in FASE 18D.2");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO hash_murmur3 requires integer seed argument in FASE 18D.2");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "random_int") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO random_int expects exactly two integer arguments in FASE 11F.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_random_int(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO random_int requires integer lo argument in FASE 11F.1");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO random_int requires integer hi argument in FASE 11F.1");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "random_real") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO random_real expects no arguments in FASE 11F.1");
            return false;
        }
        fputs("cct_rt_random_real()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "random_verbum") == 0 || strcmp(name, "random_bytes") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one integer argument in FASE 18D.3", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer argument in FASE 18D.3", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) {
            *out_kind = (strcmp(name, "random_verbum") == 0)
                ? CCT_CODEGEN_VALUE_STRING
                : CCT_CODEGEN_VALUE_POINTER;
        }
        return true;
    }

    if (strcmp(name, "random_verbum_from") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO random_verbum_from expects exactly (len, alphabet) in FASE 18D.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_random_verbum_from(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO random_verbum_from requires integer len argument in FASE 18D.3");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO random_verbum_from requires VERBUM alphabet argument in FASE 18D.3");
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
        return true;
    }

    if (strcmp(name, "math_sqrt") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO math_sqrt expects exactly one argument");
            return false;
        }
        fputs("cct_sqrt(", out);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "math_cbrt") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO math_cbrt expects exactly one argument");
            return false;
        }
        fputs("cct_cbrt(", out);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "math_pow") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO math_pow expects exactly two arguments");
            return false;
        }
        fputs("cct_pow(", out);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "math_hypot") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO math_hypot expects exactly two arguments");
            return false;
        }
        fputs("cct_hypot(", out);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "math_sin") == 0 || strcmp(name, "math_cos") == 0 ||
        strcmp(name, "math_tan") == 0 || strcmp(name, "math_asin") == 0 ||
        strcmp(name, "math_acos") == 0 || strcmp(name, "math_atan") == 0 ||
        strcmp(name, "math_deg_to_rad") == 0 || strcmp(name, "math_rad_to_deg") == 0 ||
        strcmp(name, "math_exp") == 0 || strcmp(name, "math_log") == 0 ||
        strcmp(name, "math_log10") == 0 || strcmp(name, "math_log2") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO math function expects exactly one argument");
            return false;
        }
        const char* c_name = name + 5;
        fprintf(out, "cct_%s(", c_name);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "math_atan2") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO math_atan2 expects exactly two arguments");
            return false;
        }
        fputs("cct_atan2(", out);
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        fputs(", ", out);
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
        return true;
    }

    if (strcmp(name, "collection_fluxus_map") == 0) {
        if (argc != 4) {
            cg_report_node(cg, expr, "OBSECRO collection_fluxus_map expects (flux, item_size, result_size, fn) in FASE 12D.2");
            return false;
        }
        cct_codegen_value_kind_t kflux = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kis = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kos = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kfn = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_collection_fluxus_map((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kflux)) return false;
        if (kflux != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO collection_fluxus_map requires pointer flux argument in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kis)) return false;
        if (!(kis == CCT_CODEGEN_VALUE_INT || kis == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO collection_fluxus_map requires integer item_size in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &kos)) return false;
        if (!(kos == CCT_CODEGEN_VALUE_INT || kos == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO collection_fluxus_map requires integer result_size in FASE 12D.2");
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &kfn)) return false;
        if (kfn != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[3], "OBSECRO collection_fluxus_map requires callback pointer in FASE 12D.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "collection_fluxus_filter") == 0 ||
        strcmp(name, "collection_fluxus_find") == 0 ||
        strcmp(name, "collection_fluxus_any") == 0 ||
        strcmp(name, "collection_fluxus_all") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects (flux, item_size, predicate) in FASE 12D.2", name);
            return false;
        }
        cct_codegen_value_kind_t kflux = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kis = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kfn = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "%s((void*)(", strcmp(name, "collection_fluxus_filter") == 0
                                ? "((void*)cct_rt_collection_fluxus_filter"
                                : strcmp(name, "collection_fluxus_find") == 0
                                    ? "((void*)cct_rt_collection_fluxus_find"
                                    : strcmp(name, "collection_fluxus_any") == 0
                                        ? "cct_rt_collection_fluxus_any"
                                        : "cct_rt_collection_fluxus_all");
        if (!cg_emit_expr(out, cg, args->nodes[0], &kflux)) return false;
        if (kflux != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer flux argument in FASE 12D.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kis)) return false;
        if (!(kis == CCT_CODEGEN_VALUE_INT || kis == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer item_size in FASE 12D.2", name);
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &kfn)) return false;
        if (kfn != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires callback pointer in FASE 12D.2", name);
            return false;
        }
        fputs("))", out);
        if (strcmp(name, "collection_fluxus_filter") == 0 || strcmp(name, "collection_fluxus_find") == 0) {
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        } else {
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        }
        return true;
    }

    if (strcmp(name, "collection_fluxus_fold") == 0) {
        if (argc != 5) {
            cg_report_node(cg, expr, "OBSECRO collection_fluxus_fold expects (flux, item_size, initial_ptr, acc_size, fn) in FASE 12D.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k4 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_collection_fluxus_fold((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO collection_fluxus_fold requires pointer flux argument in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO collection_fluxus_fold requires integer item_size in FASE 12D.2");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[2], "OBSECRO collection_fluxus_fold requires pointer initial accumulator in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (!(k3 == CCT_CODEGEN_VALUE_INT || k3 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[3], "OBSECRO collection_fluxus_fold requires integer acc_size in FASE 12D.2");
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[4], &k4)) return false;
        if (k4 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[4], "OBSECRO collection_fluxus_fold requires callback pointer in FASE 12D.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "collection_series_map") == 0) {
        if (argc != 5) {
            cg_report_node(cg, expr, "OBSECRO collection_series_map expects (arr, len, item_size, result_size, fn) in FASE 12D.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k4 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_collection_series_map((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO collection_series_map requires pointer array argument in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO collection_series_map requires integer len in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO collection_series_map requires integer item_size in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (!(k3 == CCT_CODEGEN_VALUE_INT || k3 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[3], "OBSECRO collection_series_map requires integer result_size in FASE 12D.2");
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[4], &k4)) return false;
        if (k4 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[4], "OBSECRO collection_series_map requires callback pointer in FASE 12D.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "collection_series_filter") == 0 ||
        strcmp(name, "collection_series_find") == 0 ||
        strcmp(name, "collection_series_any") == 0 ||
        strcmp(name, "collection_series_all") == 0) {
        if (argc != 4) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects (arr, len, item_size, predicate) in FASE 12D.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "%s((void*)(", strcmp(name, "collection_series_filter") == 0
                                ? "((void*)cct_rt_collection_series_filter"
                                : strcmp(name, "collection_series_find") == 0
                                    ? "((void*)cct_rt_collection_series_find"
                                    : strcmp(name, "collection_series_any") == 0
                                        ? "cct_rt_collection_series_any"
                                        : "cct_rt_collection_series_all");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer array argument in FASE 12D.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer len in FASE 12D.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer item_size in FASE 12D.2", name);
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (k3 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[3], "OBSECRO %s requires callback pointer in FASE 12D.2", name);
            return false;
        }
        fputs("))", out);
        if (strcmp(name, "collection_series_filter") == 0 || strcmp(name, "collection_series_find") == 0) {
            fputs(")", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        } else {
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        }
        return true;
    }

    if (strcmp(name, "collection_series_reduce") == 0) {
        if (argc != 6) {
            cg_report_node(cg, expr, "OBSECRO collection_series_reduce expects (arr, len, item_size, initial_ptr, acc_size, fn) in FASE 12D.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k3 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k4 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k5 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_collection_series_reduce((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO collection_series_reduce requires pointer array argument in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO collection_series_reduce requires integer len in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO collection_series_reduce requires integer item_size in FASE 12D.2");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[3], &k3)) return false;
        if (k3 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[3], "OBSECRO collection_series_reduce requires pointer initial accumulator in FASE 12D.2");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[4], &k4)) return false;
        if (!(k4 == CCT_CODEGEN_VALUE_INT || k4 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[4], "OBSECRO collection_series_reduce requires integer acc_size in FASE 12D.2");
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[5], &k5)) return false;
        if (k5 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[5], "OBSECRO collection_series_reduce requires callback pointer in FASE 12D.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "map_init") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO map_init expects exactly (key_size, value_size) in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_map_init(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_init requires integer key-size argument in FASE 12D.1");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO map_init requires integer value-size argument in FASE 12D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "map_remove") == 0 || strcmp(name, "map_contains") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (map, key_ptr) in FASE 12D.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer map argument in FASE 12D.1", name);
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires pointer key argument in FASE 12D.1", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "map_get_ptr") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO map_get_ptr expects exactly (map, key_ptr) in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_map_get_ptr((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_get_ptr requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO map_get_ptr requires pointer key argument in FASE 12D.1");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "map_len") == 0 || strcmp(name, "map_capacity") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one map pointer argument in FASE 12D.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer map argument in FASE 12D.1", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "map_is_empty") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO map_is_empty expects exactly one map pointer argument in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_map_is_empty((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_is_empty requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "map_copy") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO map_copy expects exactly one map pointer argument in FASE 18C.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_map_copy((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_copy requires pointer map argument in FASE 18C.2");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "map_keys") == 0 || strcmp(name, "map_values") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (map, elem_size) in FASE 18C.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer map argument in FASE 18C.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer elem_size argument in FASE 18C.2", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "set_init") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO set_init expects exactly (item_size) in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_set_init(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO set_init requires integer item-size argument in FASE 12D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "set_insert") == 0 || strcmp(name, "set_remove") == 0 || strcmp(name, "set_contains") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (set, item_ptr) in FASE 12D.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer set argument in FASE 12D.1", name);
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires pointer item argument in FASE 12D.1", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "set_len") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO set_len expects exactly one set pointer argument in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_set_len((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO set_len requires pointer set argument in FASE 12D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "set_is_empty") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO set_is_empty expects exactly one set pointer argument in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_set_is_empty((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO set_is_empty requires pointer set argument in FASE 12D.1");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "set_capacity") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO set_capacity expects exactly one set pointer argument in FASE 18C.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("cct_rt_set_capacity((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO set_capacity requires pointer set argument in FASE 18C.2");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
        return true;
    }

    if (strcmp(name, "set_union") == 0 || strcmp(name, "set_intersection") == 0 ||
        strcmp(name, "set_difference") == 0 || strcmp(name, "set_symmetric_difference") == 0 ||
        strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (a, b, item_size) in FASE 18C.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        if (strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) {
            fprintf(out, "cct_rt_%s((void*)(", name);
        } else {
            fprintf(out, "((void*)cct_rt_%s((void*)(", name);
        }
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer set argument (a) in FASE 18C.2", name);
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires pointer set argument (b) in FASE 18C.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer item_size argument in FASE 18C.2", name);
            return false;
        }
        if (strcmp(name, "set_is_subset") == 0 || strcmp(name, "set_equals") == 0) {
            fputs("))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        } else {
            fputs(")))", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        }
        return true;
    }

    if (strcmp(name, "set_copy") == 0 || strcmp(name, "set_to_fluxus") == 0) {
        if (argc != 2) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (set, item_size) in FASE 18C.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer set argument in FASE 18C.2", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer item_size argument in FASE 18C.2", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "option_some") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO option_some expects exactly (value_ptr, value_size) in FASE 12C");
            return false;
        }
        cct_codegen_value_kind_t kptr = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksize = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_option_some((const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kptr)) return false;
        if (kptr != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO option_some requires pointer payload argument in FASE 12C");
            return false;
        }
        fputs("), (size_t)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ksize)) return false;
        if (!(ksize == CCT_CODEGEN_VALUE_INT || ksize == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO option_some requires integer payload size in FASE 12C");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "option_none") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO option_none expects exactly (value_size) in FASE 12C");
            return false;
        }
        cct_codegen_value_kind_t ksize = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_option_none((size_t)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &ksize)) return false;
        if (!(ksize == CCT_CODEGEN_VALUE_INT || ksize == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO option_none requires integer payload size in FASE 12C");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "option_is_some") == 0 || strcmp(name, "option_is_none") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one pointer argument in FASE 12C", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((const void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer argument in FASE 12C", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "option_unwrap_ptr") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO option_unwrap_ptr expects exactly one pointer argument in FASE 12C");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_option_unwrap_ptr((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO option_unwrap_ptr requires pointer argument in FASE 12C");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "option_expect_ptr") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO option_expect_ptr expects exactly (opt, message) in FASE 12C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_option_expect_ptr((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO option_expect_ptr requires pointer as first argument in FASE 12C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO option_expect_ptr requires VERBUM message in FASE 12C");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "result_ok") == 0 || strcmp(name, "result_err") == 0) {
        if (argc != 3) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly (payload_ptr, ok_size, err_size) in FASE 12C", name);
            return false;
        }
        cct_codegen_value_kind_t kptr = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksz0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ksz1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((const void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kptr)) return false;
        if (kptr != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer payload as first argument in FASE 12C", name);
            return false;
        }
        fputs("), (size_t)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ksz0)) return false;
        if (!(ksz0 == CCT_CODEGEN_VALUE_INT || ksz0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer ok-size as second argument in FASE 12C", name);
            return false;
        }
        fputs("), (size_t)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &ksz1)) return false;
        if (!(ksz1 == CCT_CODEGEN_VALUE_INT || ksz1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer err-size as third argument in FASE 12C", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "result_is_ok") == 0 || strcmp(name, "result_is_err") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one pointer argument in FASE 12C", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s((const void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer argument in FASE 12C", name);
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
        return true;
    }

    if (strcmp(name, "result_unwrap_ptr") == 0 || strcmp(name, "result_unwrap_err_ptr") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one pointer argument in FASE 12C", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "((void*)cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer argument in FASE 12C", name);
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "result_expect_ptr") == 0) {
        if (argc != 2) {
            cg_report_node(cg, expr, "OBSECRO result_expect_ptr expects exactly (res, message) in FASE 12C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((void*)cct_rt_result_expect_ptr((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO result_expect_ptr requires pointer as first argument in FASE 12C");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO result_expect_ptr requires VERBUM message in FASE 12C");
            return false;
        }
        fputs("))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "mem_free") == 0 || strcmp(name, "mem_copy") == 0 ||
        strcmp(name, "mem_set") == 0 || strcmp(name, "mem_zero") == 0 ||
        strcmp(name, "kernel_halt") == 0 || strcmp(name, "kernel_outb") == 0 ||
        strcmp(name, "kernel_memcpy") == 0 || strcmp(name, "kernel_memset") == 0 ||
        strcmp(name, "fluxus_free") == 0 || strcmp(name, "fluxus_push") == 0 ||
        strcmp(name, "fluxus_pop") == 0 || strcmp(name, "fluxus_clear") == 0 ||
        strcmp(name, "fluxus_reserve") == 0 || strcmp(name, "fluxus_set") == 0 ||
        strcmp(name, "fluxus_remove") == 0 || strcmp(name, "fluxus_insert") == 0 ||
        strcmp(name, "fluxus_reverse") == 0 || strcmp(name, "fluxus_sort_int") == 0 ||
        strcmp(name, "fluxus_sort_verbum") == 0 || strcmp(name, "alg_sort_verbum") == 0 ||
        strcmp(name, "json_arr_handle_push") == 0 || strcmp(name, "json_obj_handle_push") == 0 ||
        strcmp(name, "sock_connect") == 0 || strcmp(name, "sock_bind") == 0 ||
        strcmp(name, "sock_listen") == 0 || strcmp(name, "sock_close") == 0 ||
        strcmp(name, "sock_set_timeout_ms") == 0 ||
        strcmp(name, "map_free") == 0 || strcmp(name, "map_insert") == 0 ||
        strcmp(name, "map_clear") == 0 || strcmp(name, "map_reserve") == 0 ||
        strcmp(name, "map_merge") == 0 ||
        strcmp(name, "set_free") == 0 || strcmp(name, "set_clear") == 0 ||
        strcmp(name, "set_reserve") == 0 ||
        strcmp(name, "io_print") == 0 || strcmp(name, "io_println") == 0 ||
        strcmp(name, "io_print_int") == 0 || strcmp(name, "io_print_real") == 0 ||
        strcmp(name, "io_print_char") == 0 || strcmp(name, "io_eprint") == 0 ||
        strcmp(name, "io_eprintln") == 0 || strcmp(name, "io_eprint_int") == 0 ||
        strcmp(name, "io_eprint_real") == 0 || strcmp(name, "io_flush") == 0 ||
        strcmp(name, "io_flush_err") == 0 ||
        strcmp(name, "fs_write_all") == 0 || strcmp(name, "fs_append_all") == 0 ||
        strcmp(name, "fs_mkdir") == 0 || strcmp(name, "fs_mkdir_all") == 0 ||
        strcmp(name, "fs_delete_file") == 0 || strcmp(name, "fs_delete_dir") == 0 ||
        strcmp(name, "fs_rename") == 0 || strcmp(name, "fs_copy") == 0 ||
        strcmp(name, "fs_move") == 0 || strcmp(name, "fs_chmod") == 0 ||
        strcmp(name, "fs_truncate") == 0 || strcmp(name, "fs_symlink") == 0 ||
        strcmp(name, "random_seed") == 0 || strcmp(name, "random_shuffle_int") == 0 ||
        strcmp(name, "option_free") == 0 || strcmp(name, "result_free") == 0) {
        cg_report_nodef(cg, expr, "OBSECRO %s is statement-only in current executable subset (11D/11E)", name);
        return false;
    }

    if (strcmp(name, "pete") == 0) {
        if (argc != 1) {
            cg_report_node(cg, expr, "OBSECRO pete expects exactly one size argument in FASE 7A");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs("void*", out);
        fputs(")", out);
        fputs(cct_cg_runtime_alloc_helper_name(), out);
        fputs("((size_t)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO pete size argument must be integer/boolean executable in FASE 7A");
            return false;
        }
        fputs(")))", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
        return true;
    }

    if (strcmp(name, "libera") == 0) {
            cg_report_node(cg, expr, "OBSECRO libera is statement-only in subset final da FASE 7 (use as standalone statement or DIMITTE)");
            return false;
    }

    cg_report_nodef(cg, expr, "OBSECRO %s expression codegen is not supported in FASE 7A", name ? name : "(builtin)");
    return false;
}

static bool cg_emit_coniura_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    if (expr->as.coniura.name && strcmp(expr->as.coniura.name, "__cast") == 0) {
        cct_ast_type_list_t *type_args = expr->as.coniura.type_args;
        cct_ast_node_list_t *args = expr->as.coniura.arguments;
        if (!type_args || type_args->count != 1) {
            cg_report_node(cg, expr, "cast requires exactly one target type in GENUS(T) (subset 12B)");
            return false;
        }
        if (!args || args->count != 1) {
            cg_report_node(cg, expr, "cast requires exactly one source expression (subset 12B)");
            return false;
        }

        const cct_ast_type_t *target_t = type_args->types[0];
        const char *c_ty = cg_c_type_for_ast_type(cg, target_t);
        if (!c_ty) {
            cg_report_node(cg, expr, "cast target type is outside executable subset 12B");
            return false;
        }

        cct_codegen_value_kind_t src_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        fputs("((", out);
        fputs(c_ty, out);
        fputs(")(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &src_kind)) return false;
        fputs("))", out);

        if (!(src_kind == CCT_CODEGEN_VALUE_INT ||
              src_kind == CCT_CODEGEN_VALUE_BOOL ||
              src_kind == CCT_CODEGEN_VALUE_REAL)) {
            cg_report_node(cg, args->nodes[0], "cast source type is outside numeric executable subset 12B");
            return false;
        }

        if (out_kind) *out_kind = cg_value_kind_from_ast_type_codegen(cg, target_t);
        return true;
    }

    cct_codegen_rituale_t *rit = NULL;
    if (expr->as.coniura.type_args && expr->as.coniura.type_args->count > 0) {
        rit = cg_materialize_generic_rituale_instance(
            cg, expr->as.coniura.name, expr->as.coniura.type_args, expr
        );
    } else {
        const cct_ast_node_t *templ = cg_find_generic_rituale_template(cg, expr->as.coniura.name);
        if (templ) {
                cg_report_nodef(cg, expr,
                            "generic rituale '%s' requires explicit GENUS(...) instantiation in subset 10B (subset final da FASE 10 has no type-arg inference)",
                            expr->as.coniura.name ? expr->as.coniura.name : "");
                return false;
        }
        rit = cg_find_rituale(cg, expr->as.coniura.name);
    }
    if (!rit) {
        cg_report_nodef(cg, expr, "rituale '%s' not found in FASE 4C codegen registry", expr->as.coniura.name);
        return false;
    }

    cct_ast_node_list_t *args = expr->as.coniura.arguments;
    size_t argc = args ? args->count : 0;
    if (argc != rit->param_count) {
        cg_report_nodef(cg, expr, "CONIURA '%s' argument count mismatch in FASE 4C codegen", expr->as.coniura.name);
        return false;
    }

    fprintf(out, "%s(", rit->c_name);
    for (size_t i = 0; i < argc; i++) {
        cct_codegen_value_kind_t arg_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        if (i > 0) fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[i], &arg_kind)) return false;

        cct_codegen_value_kind_t expected = cg_value_kind_from_local_kind(rit->param_kinds[i]);
        bool ok = false;
        if (arg_kind == expected) ok = true;
        if (expected == CCT_CODEGEN_VALUE_POINTER && arg_kind == CCT_CODEGEN_VALUE_POINTER) ok = true;
        if (expected == CCT_CODEGEN_VALUE_INT && arg_kind == CCT_CODEGEN_VALUE_BOOL) ok = true;
        if (expected == CCT_CODEGEN_VALUE_REAL &&
            (arg_kind == CCT_CODEGEN_VALUE_INT || arg_kind == CCT_CODEGEN_VALUE_BOOL)) ok = true;
        if (!ok) {
            cg_report_nodef(cg, args->nodes[i], "CONIURA '%s' argument %zu type not supported/mismatched in FASE 4C codegen",
                            expr->as.coniura.name, i + 1);
            return false;
        }
    }
    fputc(')', out);

    if (out_kind) *out_kind = rit->return_kind;
    return true;
}

static bool cg_emit_molde_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    if (!expr || expr->type != AST_MOLDE) {
        cg_report_node(cg, expr, "internal codegen error: expected AST_MOLDE");
        return false;
    }

    u32 id = cg->next_temp_id++;
    fprintf(out, "({ cct_rt_molde_ctx_t *__cct_molde_%u = cct_rt_molde_begin(); ", id);

    if (expr->as.molde.parts) {
        for (size_t i = 0; i < expr->as.molde.part_count; i++) {
            cct_ast_molde_part_t *part = &expr->as.molde.parts[i];
            if (!part) continue;

            if (part->kind == CCT_AST_MOLDE_PART_LITERAL) {
                const char *label = cg_string_label_for_value(cg, part->literal_text ? part->literal_text : "");
                fprintf(out, "cct_rt_molde_str(__cct_molde_%u, %s); ", id, label);
                continue;
            }

            cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!cg_probe_expr_kind(cg, part->expr, &kind)) return false;
            bool has_fmt = (part->fmt_spec && part->fmt_spec[0] != '\0');

            if (has_fmt) {
                size_t spec_len = strlen(part->fmt_spec);
                char spec_type = spec_len > 0 ? part->fmt_spec[spec_len - 1] : '\0';

                switch (kind) {
                    case CCT_CODEGEN_VALUE_INT:
                        if (spec_type == 'u' || spec_type == 'x' || spec_type == 'X') {
                            fprintf(out, "cct_rt_molde_dux_fmt(__cct_molde_%u, (unsigned long long)(", id);
                        } else {
                            fprintf(out, "cct_rt_molde_rex_fmt(__cct_molde_%u, (long long)(", id);
                        }
                        if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                        fputs("), ", out);
                        cg_emit_c_escaped_string(out, part->fmt_spec);
                        fputs("); ", out);
                        break;
                    case CCT_CODEGEN_VALUE_REAL:
                        fprintf(out, "cct_rt_molde_umbra_fmt(__cct_molde_%u, (double)(", id);
                        if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                        fputs("), ", out);
                        cg_emit_c_escaped_string(out, part->fmt_spec);
                        fputs("); ", out);
                        break;
                    case CCT_CODEGEN_VALUE_STRING:
                        fprintf(out, "cct_rt_molde_str_fmt(__cct_molde_%u, (const char*)(", id);
                        if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                        fputs("), ", out);
                        cg_emit_c_escaped_string(out, part->fmt_spec);
                        fputs("); ", out);
                        break;
                    default:
                        cg_report_nodef(
                            cg,
                            part->expr,
                            "MOLDE codegen nao suporta especificador para esta expressao (%s)",
                            cct_ast_node_type_string(part->expr ? part->expr->type : AST_LITERAL_NIHIL)
                        );
                        return false;
                }
                continue;
            }

            switch (kind) {
                case CCT_CODEGEN_VALUE_INT:
                    fprintf(out, "cct_rt_molde_rex(__cct_molde_%u, (long long)(", id);
                    if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                    fputs(")); ", out);
                    break;
                case CCT_CODEGEN_VALUE_BOOL:
                    fprintf(out, "cct_rt_molde_verum(__cct_molde_%u, (long long)(", id);
                    if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                    fputs(")); ", out);
                    break;
                case CCT_CODEGEN_VALUE_REAL:
                    fprintf(out, "cct_rt_molde_umbra(__cct_molde_%u, (double)(", id);
                    if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                    fputs(")); ", out);
                    break;
                case CCT_CODEGEN_VALUE_STRING:
                    fprintf(out, "cct_rt_molde_str(__cct_molde_%u, (const char*)(", id);
                    if (!cg_emit_expr(out, cg, part->expr, NULL)) return false;
                    fputs(")); ", out);
                    break;
                default:
                    cg_report_nodef(
                        cg,
                        part->expr,
                        "MOLDE codegen nao suporta interpolacao desta expressao (%s)",
                        cct_ast_node_type_string(part->expr ? part->expr->type : AST_LITERAL_NIHIL)
                    );
                    return false;
            }
        }
    }

    fprintf(out, "cct_rt_molde_end(__cct_molde_%u); })", id);
    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
    return true;
}

static bool cg_emit_ordo_construct_expr(
    FILE *out,
    cct_codegen_t *cg,
    const cct_ast_node_t *at,
    const char *ordo_name,
    const char *variant_name,
    const cct_ast_node_list_t *args,
    cct_codegen_value_kind_t *out_kind
) {
    if (!ordo_name || !variant_name) {
        cg_report_node(cg, at, "internal error: ORDO constructor missing resolved metadata");
        return false;
    }

    cct_codegen_ordo_t *ordo = cg_find_ordo(cg, ordo_name);
    if (!ordo || !ordo->node) {
        cg_report_nodef(cg, at, "internal error: ORDO constructor references unknown type '%s'", ordo_name);
        return false;
    }

    cct_ast_ordo_variant_t *variant = cg_find_ordo_variant_decl(ordo->node, variant_name);
    if (!variant) {
        cg_report_nodef(cg, at, "internal error: ORDO constructor variant '%s' not found in '%s'",
                        variant_name, ordo_name);
        return false;
    }

    size_t argc = args ? args->count : 0;
    if (argc != variant->field_count) {
        cg_report_nodef(cg, at, "internal error: ORDO constructor %s expects %zu arg(s), got %zu",
                        variant_name, variant->field_count, argc);
        return false;
    }

    fprintf(out, "((cct_%s){ .__tag = CCT_%s_%s", ordo_name, ordo_name, variant_name);
    if (variant->field_count > 0) {
        fprintf(out, ", .__payload.%s = {", variant_name);
        for (size_t i = 0; i < variant->field_count; i++) {
            cct_ast_ordo_field_t *field = variant->fields[i];
            if (i > 0) fputs(", ", out);
            fprintf(out, ".%s = ", field && field->name ? field->name : "_");
            if (!cg_emit_expr(out, cg, args->nodes[i], NULL)) return false;
        }
        fputs("}", out);
    }
    fputs(" })", out);
    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRUCT;
    return true;
}

static bool cg_emit_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!expr) {
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_NIHIL;
        fputs("0", out);
        return true;
    }

    switch (expr->type) {
        case AST_LITERAL_INT:
            fprintf(out, "%lld", (long long)expr->as.literal_int.int_value);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
            return true;

        case AST_LITERAL_REAL:
            fprintf(out, "%.17g", expr->as.literal_real.real_value);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_REAL;
            return true;

        case AST_LITERAL_BOOL:
            fputs(expr->as.literal_bool.bool_value ? "1" : "0", out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_BOOL;
            return true;

        case AST_LITERAL_STRING: {
            const char *label = cg_string_label_for_value(cg, expr->as.literal_string.string_value);
            fputs(label, out);
            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
            return true;
        }

        case AST_MOLDE:
            return cg_emit_molde_expr(out, cg, expr, out_kind);

        case AST_IDENTIFIER: {
            if (expr->as.identifier.is_ordo_construct) {
                return cg_emit_ordo_construct_expr(
                    out,
                    cg,
                    expr,
                    expr->as.identifier.ordo_name,
                    expr->as.identifier.variant_name ? expr->as.identifier.variant_name : expr->as.identifier.name,
                    NULL,
                    out_kind
                );
            }

            cct_codegen_local_t *local = cg_find_local(cg, expr->as.identifier.name);
            if (!local) {
                /* Try enum constant lookup (ORDO item). */
                for (cct_codegen_ordo_t *o = cg->ordines; o; o = o->next) {
                    i64 tag_value = 0;
                    if (cg_find_ordo_variant_tag(o->node, expr->as.identifier.name, &tag_value)) {
                        fprintf(out, "%lld", (long long)tag_value);
                        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
                        return true;
                    }
                }
                cct_codegen_rituale_t *rit = cg_find_rituale(cg, expr->as.identifier.name);
                if (rit) {
                    fprintf(out, "((void*)%s)", rit->c_name);
                    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_POINTER;
                    return true;
                }
                cg_report_nodef(cg, expr, "identifier '%s' not supported in FASE 6B codegen (only locals/params/ORDO items)", expr->as.identifier.name);
                return false;
            }
            fputs(expr->as.identifier.name, out);
            if (out_kind) {
                *out_kind = cg_value_kind_from_local_kind(local->kind);
            }
            return true;
        }

        case AST_FIELD_ACCESS: {
            const cct_ast_type_t *obj_t = cg_expr_struct_type(cg, expr->as.field_access.object);
            const char *sig_name = cg_effective_named_type_name(cg, obj_t);
            if (!obj_t || !sig_name) {
                cg_report_node(cg, expr, "field access object is not an executable SIGILLUM value in FASE 6B");
                return false;
            }
            cct_codegen_sigillum_t *sig = cg_find_sigillum(cg, sig_name);
            const cct_ast_field_t *field = cg_find_sigillum_field(sig, expr->as.field_access.field);
            if (!field || !field->type) {
                cg_report_nodef(cg, expr, "SIGILLUM '%s' field '%s' is not available in FASE 6B codegen",
                                sig_name, expr->as.field_access.field);
                return false;
            }
            fputc('(', out);
            if (!cg_emit_expr(out, cg, expr->as.field_access.object, NULL)) return false;
            fprintf(out, ").%s", expr->as.field_access.field);
            if (out_kind) *out_kind = cg_value_kind_from_ast_type_codegen(cg, field->type);
            return true;
        }

        case AST_INDEX_ACCESS: {
            const cct_ast_type_t *arr_t = cg_expr_array_type(cg, expr->as.index_access.array);
            const cct_ast_type_t *ptr_t = NULL;
            if ((!arr_t || !arr_t->element_type) && expr->as.index_access.array) {
                ptr_t = cg_expr_ast_type(cg, expr->as.index_access.array);
                if (!(ptr_t && ptr_t->is_pointer && ptr_t->element_type)) ptr_t = NULL;
            }
            if ((!arr_t || !arr_t->element_type) && !ptr_t) {
                cg_report_node(cg, expr, "index access requires executable SERIES or SPECULUM value in subset final da FASE 7");
                return false;
            }
            cct_codegen_value_kind_t idx_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (ptr_t) {
                const char *elem_c = cg_c_type_for_ast_type(cg, ptr_t->element_type);
                if (!elem_c) {
                    cg_report_node(cg, expr, "pointer index element type is outside subset final da FASE 7");
                    return false;
                }
                fputs("(((", out);
                fputs(elem_c, out);
                fputs("*)", out);
                fputs(cg_current_fail_handler_label(cg)
                          ? cct_cg_runtime_check_not_null_fractum_helper_name()
                          : cct_cg_runtime_check_not_null_helper_name(),
                      out);
                fputs("((void*)(", out);
                if (!cg_emit_expr(out, cg, expr->as.index_access.array, NULL)) return false;
                fputs("), ", out);
                cg_emit_c_escaped_string(out, "runtime-fail (bridged): null pointer index base");
                fputs("))[", out);
            } else {
                fputc('(', out);
                if (!cg_emit_expr(out, cg, expr->as.index_access.array, NULL)) return false;
                fputs("[", out);
            }
            if (!cg_emit_expr(out, cg, expr->as.index_access.index, &idx_kind)) return false;
            fputs("])", out);
            if (!(idx_kind == CCT_CODEGEN_VALUE_INT || idx_kind == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, expr, "index expression must be integer/boolean executable in subset final da FASE 7");
                return false;
            }
            if (out_kind) *out_kind = cg_value_kind_from_ast_type_codegen(cg, arr_t ? arr_t->element_type : ptr_t->element_type);
            return true;
        }

        case AST_BINARY_OP:
            return cg_emit_binary_expr(out, cg, expr, out_kind);

        case AST_UNARY_OP:
            return cg_emit_unary_expr(out, cg, expr, out_kind);

        case AST_CONIURA:
            return cg_emit_coniura_expr(out, cg, expr, out_kind);

        case AST_OBSECRO:
            return cg_emit_obsecro_expr(out, cg, expr, out_kind);

        case AST_CALL:
            if (expr->as.call.is_ordo_construct) {
                return cg_emit_ordo_construct_expr(
                    out,
                    cg,
                    expr,
                    expr->as.call.ordo_name,
                    expr->as.call.variant_name,
                    expr->as.call.arguments,
                    out_kind
                );
            }
            cg_report_node(cg, expr, "direct call expression codegen is not supported in FASE 4C (use CONIURA)");
            return false;

        case AST_MENSURA:
            if (!expr->as.mensura.type) {
                cg_report_node(cg, expr, "MENSURA requires type argument");
                return false;
            }
            {
                const char *c_ty = cg_c_type_for_ast_type(cg, expr->as.mensura.type);
                if (!c_ty) {
                    cg_report_node(cg, expr, "MENSURA type is outside FASE 7A executable subset");
                    return false;
                }
                fprintf(out, "((long long)sizeof(%s))", c_ty);
                if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
                return true;
            }

        default:
            cg_report_nodef(cg, expr, "expression %s not supported in FASE 4C codegen",
                            cct_ast_node_type_string(expr->type));
            return false;
    }
}

static void cg_emit_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) fputs("    ", out);
}

static void cg_emit_failure_terminal_after_uncaught(FILE *out, cct_codegen_t *cg, int indent);

static bool cg_probe_expr_kind(cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind) {
    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_UNKNOWN;
#ifdef _WIN32
    FILE *null_out = fopen("NUL", "wb");
#else
    FILE *null_out = fopen("/dev/null", "wb");
#endif
    if (!null_out) {
        cg_report_node(cg, expr, "internal codegen error: could not open null device for expression type probe");
        return false;
    }
    bool ok = cg_emit_expr(null_out, cg, expr, out_kind);
    fclose(null_out);
    return ok;
}

static bool cg_emit_fail_propagation_check(FILE *out, cct_codegen_t *cg, int indent) {
    const char *catch_label = cg_current_fail_handler_label(cg);

    cg_emit_indent(out, indent);
    fputs("if (cct_rt_fractum_is_active()) {\n", out);
    if (catch_label) {
        cg_emit_indent(out, indent + 1);
        fprintf(out, "goto %s;\n", catch_label);
    } else {
        cg_emit_failure_terminal_after_uncaught(out, cg, indent + 1);
    }
    cg_emit_indent(out, indent);
    fputs("}\n", out);
    return true;
}

static bool cg_emit_coniura_expr_to_temp_with_failcheck(
    FILE *out,
    cct_codegen_t *cg,
    const cct_ast_node_t *expr,
    int indent,
    char *temp_name,
    size_t temp_name_sz,
    cct_codegen_value_kind_t *out_kind
) {
    if (!out || !cg || !expr || expr->type != AST_CONIURA || !temp_name || temp_name_sz == 0) {
        cg_report_node(cg, expr, "internal codegen error: invalid CONIURA temp emission request");
        return false;
    }

    cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_probe_expr_kind(cg, expr, &kind)) return false;

    const char *c_ty = cg_c_type_for_return_kind(kind);
    if (!c_ty && kind == CCT_CODEGEN_VALUE_STRUCT) {
        cct_codegen_rituale_t *rit = cg_find_rituale(cg, expr->as.coniura.name);
        if (rit && rit->node && rit->node->as.rituale.return_type) {
            c_ty = cg_c_type_for_ast_type(cg, rit->node->as.rituale.return_type);
        }
    }
    if (!c_ty) {
        cg_report_node(cg, expr,
                       "CONIURA expression result kind is outside subset 8B propagation handling (subset final da FASE 8 keeps this limitation)");
        return false;
    }

    snprintf(temp_name, temp_name_sz, "__cct_failtmp_%u", cg->next_temp_id++);

    cg_emit_indent(out, indent);
    fprintf(out, "%s %s = ", c_ty, temp_name);
    if (!cg_emit_expr(out, cg, expr, NULL)) return false;
    fputs(";\n", out);

    return cg_emit_fail_propagation_check(out, cg, indent) &&
           (!(out_kind) || ((*out_kind = kind), true));
}

static bool cg_emit_scribe_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *obsecro_node, int indent) {
    if (strcmp(obsecro_node->as.obsecro.name, "libera") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO libera requires exactly one pointer argument in subset final da FASE 7");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs(cct_cg_runtime_free_helper_name(), out);
        fputs("((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO libera requires pointer executable argument in subset final da FASE 7");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "mem_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO mem_free requires exactly one pointer argument in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_mem_free((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_free requires pointer executable argument in FASE 11D.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "mem_copy") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO mem_copy requires exactly (dest, src, size) in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_mem_copy((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_copy requires pointer destination in FASE 11D.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO mem_copy requires pointer source in FASE 11D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO mem_copy requires integer size in FASE 11D.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "mem_set") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO mem_set requires exactly (ptr, value, size) in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_mem_set((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_set requires pointer argument in FASE 11D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO mem_set requires integer byte value in FASE 11D.1");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO mem_set requires integer size in FASE 11D.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "mem_zero") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO mem_zero requires exactly (ptr, size) in FASE 11D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_mem_zero((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO mem_zero requires pointer argument in FASE 11D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO mem_zero requires integer size in FASE 11D.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "kernel_halt") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (args && args->count != 0) {
            cg_report_node(cg, obsecro_node, "OBSECRO kernel_halt requires no arguments in FASE 16B.2");
            return false;
        }
        cg_emit_indent(out, indent);
        fputs("cct_svc_halt();\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "kernel_outb") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO kernel_outb requires exactly (porta, valor) in FASE 16B.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_svc_outb(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO kernel_outb requires integer port argument in FASE 16B.2");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO kernel_outb requires integer value argument in FASE 16B.2");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "kernel_memcpy") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO kernel_memcpy requires exactly (dst, src, n) in FASE 16B.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_svc_memcpy((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO kernel_memcpy requires pointer destination in FASE 16B.2");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO kernel_memcpy requires pointer source in FASE 16B.2");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO kernel_memcpy requires integer size in FASE 16B.2");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "kernel_memset") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO kernel_memset requires exactly (dst, valor, n) in FASE 16B.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_svc_memset((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO kernel_memset requires pointer destination in FASE 16B.2");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO kernel_memset requires integer byte value in FASE 16B.2");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO kernel_memset requires integer size in FASE 16B.2");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO fluxus_free requires exactly one pointer argument in FASE 11D.3");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_fluxus_destroy((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_free requires pointer executable argument in FASE 11D.3");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_push") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fluxus_pop") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (flux, ptr) in FASE 11D.3", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer fluxus argument in FASE 11D.3", name);
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires pointer payload argument in FASE 11D.3", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "json_arr_handle_push") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "json_obj_handle_push") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (handle, ptr) in FASE 20A.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((long long)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer handle argument in FASE 20A.2", name);
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires pointer payload argument in FASE 20A.2", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "sock_connect") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "sock_bind") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (sock, host, port) in FASE 20B.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires socket pointer argument in FASE 20B.1", name);
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM host argument in FASE 20B.1", name);
            return false;
        }
        fputs(", (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires integer port argument in FASE 20B.1", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "sock_listen") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "sock_set_timeout_ms") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (sock, value) in FASE 20B.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires socket pointer argument in FASE 20B.1", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer argument in FASE 20B.1", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "sock_close") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO sock_close requires exactly one socket pointer argument in FASE 20B.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_sock_close((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO sock_close requires socket pointer argument in FASE 20B.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "pg_builtin_close") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "pg_builtin_rows_close") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one pointer argument in FASE 36A", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_postgres = true;
        cg_emit_indent(out, indent);
        fprintf(out, "%s((void*)(", strcmp(name, "pg_builtin_close") == 0 ? "cct_rt_pg_db_close" : "cct_rt_pg_rows_close");
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer argument in FASE 36A", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "pg_builtin_exec") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO pg_builtin_exec requires exactly (db, sql) in FASE 36A");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_postgres = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_pg_db_exec((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO pg_builtin_exec requires db pointer in FASE 36A");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO pg_builtin_exec requires VERBUM sql in FASE 36A");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "db_close") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO db_close requires exactly one db pointer argument in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_db_close((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_close requires db pointer argument in FASE 20E.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "db_exec") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO db_exec requires exactly (db, sql) in FASE 20E.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_db_exec((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO db_exec requires db pointer as first argument in FASE 20E.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO db_exec requires VERBUM sql as second argument in FASE 20E.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "rows_close") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO rows_close requires exactly one rows pointer argument in FASE 20E.2");
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_rows_close((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO rows_close requires rows pointer argument in FASE 20E.2");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "stmt_bind_text") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "stmt_bind_int") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "stmt_bind_real") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (stmt, idx, value) in FASE 20E.3", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires stmt pointer as first argument in FASE 20E.3", name);
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer idx as second argument in FASE 20E.3", name);
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (strcmp(name, "stmt_bind_text") == 0) {
            if (k2 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[2], "OBSECRO stmt_bind_text requires VERBUM value as third argument in FASE 20E.3");
                return false;
            }
        } else if (strcmp(name, "stmt_bind_int") == 0) {
            if (!(k2 == CCT_CODEGEN_VALUE_INT || k2 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, args->nodes[2], "OBSECRO stmt_bind_int requires integer value as third argument in FASE 20E.3");
                return false;
            }
        } else {
            if (k2 != CCT_CODEGEN_VALUE_REAL) {
                cg_report_node(cg, args->nodes[2], "OBSECRO stmt_bind_real requires UMBRA value as third argument in FASE 20E.3");
                return false;
            }
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "stmt_reset") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "stmt_finalize") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one stmt pointer argument in FASE 20E.3", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires stmt pointer argument in FASE 20E.3", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "db_begin") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "db_commit") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "db_rollback") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one db pointer argument in FASE 20E.4", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_sqlite = true;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires db pointer argument in FASE 20E.4", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_clear") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO fluxus_clear requires exactly one pointer argument in FASE 11D.3");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_fluxus_clear((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_clear requires pointer executable argument in FASE 11D.3");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_reserve") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO fluxus_reserve requires exactly (flux, capacity) in FASE 11D.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_fluxus_reserve((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_reserve requires pointer argument in FASE 11D.3");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fluxus_reserve requires integer capacity in FASE 11D.3");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_set") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fluxus_insert") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (flux, idx, elem_ptr) in FASE 18C.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer fluxus argument in FASE 18C.1", name);
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer index argument in FASE 18C.1", name);
            return false;
        }
        fputs(", (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[2], "OBSECRO %s requires pointer elem argument in FASE 18C.1", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_remove") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO fluxus_remove requires exactly (flux, idx) in FASE 18C.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_fluxus_remove((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO fluxus_remove requires pointer fluxus argument in FASE 18C.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO fluxus_remove requires integer index argument in FASE 18C.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fluxus_reverse") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fluxus_sort_int") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fluxus_sort_verbum") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one fluxus pointer argument in FASE 18C.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer fluxus argument in FASE 18C.1", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "alg_sort_verbum") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO alg_sort_verbum requires exactly (arr_ptr, len) in FASE 18C.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_alg_sort_verbum((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO alg_sort_verbum requires pointer array argument in FASE 18C.3");
            return false;
        }
        fputs("), (long long)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO alg_sort_verbum requires integer length argument in FASE 18C.3");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "map_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO map_free requires exactly one map pointer argument in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_map_free((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_free requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "map_insert") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO map_insert requires exactly (map, key_ptr, value_ptr) in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_map_insert((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_insert requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO map_insert requires pointer key argument in FASE 12D.1");
            return false;
        }
        fputs("), (const void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
        if (k2 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[2], "OBSECRO map_insert requires pointer value argument in FASE 12D.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "map_clear") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO map_clear requires exactly one map pointer argument in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_map_clear((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_clear requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "map_reserve") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO map_reserve requires exactly (map, additional) in FASE 12D.1");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_map_reserve((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_reserve requires pointer map argument in FASE 12D.1");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO map_reserve requires integer additional argument in FASE 12D.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "map_merge") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO map_merge requires exactly (dest, src) in FASE 18C.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_map_merge((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO map_merge requires pointer destination map argument in FASE 18C.2");
            return false;
        }
        fputs("), (void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[1], "OBSECRO map_merge requires pointer source map argument in FASE 18C.2");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "set_free") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "set_clear") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one set pointer argument in FASE 12D.1", name);
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer set argument in FASE 12D.1", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "set_reserve") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO set_reserve requires exactly (set, additional) in FASE 18C.2");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_set_reserve((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO set_reserve requires pointer set argument in FASE 18C.2");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO set_reserve requires integer additional argument in FASE 18C.2");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "io_print") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_println") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_eprint") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_eprintln") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one VERBUM argument in FASE 18B.3", name);
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM executable argument in FASE 11E.1", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "io_print_int") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_eprint_int") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_print_char") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one integer argument in FASE 18B.3", name);
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer executable argument in FASE 18B.3", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "io_print_real") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_eprint_real") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one numeric argument in FASE 18B.3", name);
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_REAL || kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires numeric executable argument in FASE 18B.3", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "io_flush") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_flush_err") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (args && args->count != 0) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires zero arguments in FASE 18B.3", name);
            return false;
        }
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s();\n", name);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fs_write_all") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_append_all") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (path, content) in FASE 11E.1", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM path argument in FASE 11E.1", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM content argument in FASE 11E.1", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fs_mkdir") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_mkdir_all") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_delete_file") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_delete_dir") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one VERBUM path argument in FASE 18B.1", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM path argument in FASE 18B.1", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fs_rename") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_copy") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_move") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_symlink") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly two VERBUM path arguments in FASE 18B.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument in FASE 18B.1", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (k1 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM second argument in FASE 18B.1", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "fs_chmod") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "fs_truncate") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (path, integer) arguments in FASE 18B.2", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM first argument in FASE 18B.2", name);
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires integer second argument in FASE 18B.2", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "random_seed") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO random_seed requires exactly one integer argument in FASE 11F.1");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_random_seed(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO random_seed requires integer executable argument in FASE 11F.1");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "random_shuffle_int") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO random_shuffle_int requires exactly (arr, n) in FASE 18D.3");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_random_shuffle_int((long long*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO random_shuffle_int requires pointer array argument in FASE 18D.3");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
        if (!(k1 == CCT_CODEGEN_VALUE_INT || k1 == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO random_shuffle_int requires integer n argument in FASE 18D.3");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "time_sleep_ms") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO time_sleep_ms requires exactly one integer argument in FASE 17D.2");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_time_sleep_ms(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO time_sleep_ms requires integer ms argument");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "signal_builtin_clear") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (args && args->count != 0) {
            cg_report_node(cg, obsecro_node, "OBSECRO signal_builtin_clear expects no arguments in FASE 34D");
            return false;
        }
        cg->uses_signal = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_signal_clear();\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "signal_builtin_raise_self") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO signal_builtin_raise_self expects exactly one integer code in FASE 34D");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_signal = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_signal_raise_self(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO signal_builtin_raise_self requires integer code argument");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "bytes_set") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 3) {
            cg_report_node(cg, obsecro_node, "OBSECRO bytes_set requires exactly (bytes, index, value) in FASE 17D.3");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ki = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kv = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_bytes_set((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO bytes_set requires bytes pointer argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ki)) return false;
        if (!(ki == CCT_CODEGEN_VALUE_INT || ki == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO bytes_set requires integer index argument");
            return false;
        }
        fputs(", ", out);
        if (!cg_emit_expr(out, cg, args->nodes[2], &kv)) return false;
        if (!(kv == CCT_CODEGEN_VALUE_INT || kv == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[2], "OBSECRO bytes_set requires integer byte value");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "bytes_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO bytes_free requires exactly one bytes pointer argument in FASE 17D.3");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_bytes_free((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO bytes_free requires bytes pointer argument");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "option_free") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "result_free") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one pointer argument in FASE 12C", name);
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires pointer argument in FASE 12C", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "regex_builtin_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO regex_builtin_free requires exactly one regex handle pointer in FASE 32C");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_regex = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_regex_free((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO regex_builtin_free requires regex handle pointer argument in FASE 32C");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "image_builtin_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO image_builtin_free requires exactly one image handle pointer in FASE 32I");
            return false;
        }
        cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
        cg->uses_image_ops = true;
        cg_emit_indent(out, indent);
        fputs("cct_rt_image_free(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
        if (k0 != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO image_builtin_free requires image handle pointer argument in FASE 32I");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "scan_free") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO scan_free requires exactly one cursor pointer argument in FASE 17A.3");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_scan_free((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO scan_free requires cursor pointer argument");
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "builder_append") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO builder_append requires exactly (builder, text) in FASE 17B.1");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_builder_append((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO builder_append requires builder pointer argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ks)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_node(cg, args->nodes[1], "OBSECRO builder_append requires VERBUM argument");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "builder_append_char") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_node(cg, obsecro_node, "OBSECRO builder_append_char requires exactly (builder, byte) in FASE 17B.1");
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t kc = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_builder_append_char((void*)(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, args->nodes[0], "OBSECRO builder_append_char requires builder pointer argument");
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &kc)) return false;
        if (!(kc == CCT_CODEGEN_VALUE_INT || kc == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[1], "OBSECRO builder_append_char requires integer byte argument");
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "builder_clear") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "builder_free") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one builder pointer argument in FASE 17B.1", name);
            return false;
        }
        cct_codegen_value_kind_t kb = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kb)) return false;
        if (kb != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires builder pointer argument", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "writer_indent") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "writer_dedent") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "writer_free") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one writer pointer argument in FASE 17B.2", name);
            return false;
        }
        cct_codegen_value_kind_t kw = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kw)) return false;
        if (kw != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires writer pointer argument", name);
            return false;
        }
        fputs("));\n", out);
        return true;
    }

    if (strcmp(obsecro_node->as.obsecro.name, "writer_write") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "writer_writeln") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 2) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly (writer, text) in FASE 17B.2", name);
            return false;
        }
        cct_codegen_value_kind_t kw = CCT_CODEGEN_VALUE_UNKNOWN;
        cct_codegen_value_kind_t ks = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fprintf(out, "cct_rt_%s((void*)(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kw)) return false;
        if (kw != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires writer pointer argument", name);
            return false;
        }
        fputs("), ", out);
        if (!cg_emit_expr(out, cg, args->nodes[1], &ks)) return false;
        if (ks != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[1], "OBSECRO %s requires VERBUM text argument", name);
            return false;
        }
        fputs(");\n", out);
        return true;
    }

    if (strncmp(obsecro_node->as.obsecro.name, "instr_builtin_", 14) == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        cg->uses_instrument = true;

        if (strcmp(name, "instr_builtin_disable") == 0 ||
            strcmp(name, "instr_builtin_buffer_clear") == 0) {
            if (args && args->count != 0) {
                cg_report_nodef(cg, obsecro_node, "OBSECRO %s expects no arguments in FASE 38A", name);
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s();\n",
                    strcmp(name, "instr_builtin_disable") == 0
                        ? "cct_rt_instr_disable"
                        : "cct_rt_instr_buffer_clear");
            return true;
        }

        if (strcmp(name, "instr_builtin_enable") == 0 ||
            strcmp(name, "instr_builtin_span_end") == 0) {
            cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!args || args->count != 1) {
                cg_report_nodef(cg, obsecro_node, "OBSECRO %s expects exactly one integer argument in FASE 38A", name);
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s(",
                    strcmp(name, "instr_builtin_enable") == 0
                        ? "cct_rt_instr_enable"
                        : "cct_rt_instr_span_end");
            if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
            if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires integer argument in FASE 38A", name);
                return false;
            }
            fputs(");\n", out);
            return true;
        }

        if (strcmp(name, "instr_builtin_span_attr") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k2 = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!args || args->count != 3) {
                cg_report_node(cg, obsecro_node, "OBSECRO instr_builtin_span_attr expects exactly (span_id, key, value) in FASE 38A");
                return false;
            }
            cg_emit_indent(out, indent);
            fputs("cct_rt_instr_span_attr(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (!(k0 == CCT_CODEGEN_VALUE_INT || k0 == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, args->nodes[0], "OBSECRO instr_builtin_span_attr requires integer span_id argument in FASE 38A");
                return false;
            }
            fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (k1 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[1], "OBSECRO instr_builtin_span_attr requires VERBUM key argument in FASE 38A");
                return false;
            }
            fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[2], &k2)) return false;
            if (k2 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[2], "OBSECRO instr_builtin_span_attr requires VERBUM value argument in FASE 38A");
                return false;
            }
            fputs(");\n", out);
            return true;
        }

        if (strcmp(name, "instr_builtin_event") == 0) {
            cct_codegen_value_kind_t k0 = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t k1 = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!args || args->count != 2) {
                cg_report_node(cg, obsecro_node, "OBSECRO instr_builtin_event expects exactly two VERBUM arguments in FASE 38A");
                return false;
            }
            cg_emit_indent(out, indent);
            fputs("cct_rt_instr_event(", out);
            if (!cg_emit_expr(out, cg, args->nodes[0], &k0)) return false;
            if (k0 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[0], "OBSECRO instr_builtin_event requires VERBUM kind argument in FASE 38A");
                return false;
            }
            fputs(", ", out);
            if (!cg_emit_expr(out, cg, args->nodes[1], &k1)) return false;
            if (k1 != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, args->nodes[1], "OBSECRO instr_builtin_event requires VERBUM name argument in FASE 38A");
                return false;
            }
            fputs(");\n", out);
            return true;
        }
    }

    if (strcmp(obsecro_node->as.obsecro.name, "scribe") != 0) {
        cg_report_nodef(cg, obsecro_node, "OBSECRO %s codegen is not supported in current executable subset (supported stmt builtins: scribe, libera, mem_free, mem_copy, mem_set, mem_zero, kernel_halt, kernel_outb, kernel_memcpy, kernel_memset, fluxus_free, fluxus_push, fluxus_pop, fluxus_clear, fluxus_reserve, fluxus_set, fluxus_remove, fluxus_insert, fluxus_reverse, fluxus_sort_int, fluxus_sort_verbum, alg_sort_verbum, json_arr_handle_push, json_obj_handle_push, sock_connect, sock_bind, sock_listen, sock_close, sock_set_timeout_ms, db_exec, db_close, rows_close, stmt_bind_text, stmt_bind_int, stmt_bind_real, stmt_reset, stmt_finalize, db_begin, db_commit, db_rollback, map_free, map_insert, map_clear, map_reserve, map_merge, set_free, set_clear, set_reserve, io_print, io_println, io_print_int, io_print_real, io_print_char, io_eprint, io_eprintln, io_eprint_int, io_eprint_real, io_flush, io_flush_err, fs_write_all, fs_append_all, fs_mkdir, fs_mkdir_all, fs_delete_file, fs_delete_dir, fs_rename, fs_copy, fs_move, fs_chmod, fs_truncate, fs_symlink, random_seed, time_sleep_ms, bytes_set, bytes_free, option_free, result_free, regex_builtin_free, image_builtin_free, scan_free, builder_append, builder_append_char, builder_clear, builder_free, writer_indent, writer_dedent, writer_write, writer_writeln, writer_free, instr_builtin_enable, instr_builtin_disable, instr_builtin_span_end, instr_builtin_span_attr, instr_builtin_event, instr_builtin_buffer_clear)",
                        obsecro_node->as.obsecro.name);
        return false;
    }

    cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
    if (!args || args->count == 0) {
        cg_report_node(cg, obsecro_node, "OBSECRO scribe requires at least one argument in FASE 4C");
        return false;
    }

    for (size_t i = 0; i < args->count; i++) {
        cct_ast_node_t *arg = args->nodes[i];
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        if (!cg_probe_expr_kind(cg, arg, &kind)) return false;

        bool arg_is_coniura = (arg->type == AST_CONIURA);
        char coniura_tmp[64];
        const char *arg_override = NULL;
        if (arg_is_coniura) {
            if (!cg_emit_coniura_expr_to_temp_with_failcheck(out, cg, arg, indent, coniura_tmp, sizeof(coniura_tmp), &kind)) {
                return false;
            }
            arg_override = coniura_tmp;
        }

        if (kind == CCT_CODEGEN_VALUE_STRING) {
            const char *rt_fn = cct_cg_runtime_scribe_helper_name(kind);
            if (!rt_fn) {
                cg_report_node(cg, arg, "internal codegen error: missing runtime scribe helper for string");
                return false;
            }
            if (!arg_override && arg->type == AST_MOLDE) {
                char molde_tmp[64];
                snprintf(molde_tmp, sizeof(molde_tmp), "__cct_molde_scribe_%u", cg->next_temp_id++);

                cg_emit_indent(out, indent);
                fprintf(out, "char *%s = (char*)(", molde_tmp);
                if (!cg_emit_expr(out, cg, arg, NULL)) return false;
                fputs(");\n", out);

                cg_emit_indent(out, indent);
                fprintf(out, "%s((%s));\n", rt_fn, molde_tmp);

                cg_emit_indent(out, indent);
                fprintf(out, "%s((void*)%s);\n", cct_cg_runtime_free_helper_name(), molde_tmp);
                continue;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s((", rt_fn);
            if (arg_override) {
                fputs(arg_override, out);
            } else if (!cg_emit_expr(out, cg, arg, NULL)) {
                return false;
            }
            fputs("));\n", out);
            continue;
        }

        if (kind == CCT_CODEGEN_VALUE_REAL) {
            const char *rt_fn = cct_cg_runtime_scribe_helper_name(kind);
            if (!rt_fn) {
                cg_report_node(cg, arg, "internal codegen error: missing runtime scribe helper for real");
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s((double)(", rt_fn);
            if (arg_override) {
                fputs(arg_override, out);
            } else if (!cg_emit_expr(out, cg, arg, NULL)) {
                return false;
            }
            fputs("));\n", out);
            continue;
        }

        if (kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL) {
            const char *rt_fn = cct_cg_runtime_scribe_helper_name(kind);
            if (!rt_fn) {
                cg_report_node(cg, arg, "internal codegen error: missing runtime scribe helper for scalar");
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s((long long)(", rt_fn);
            if (arg_override) {
                fputs(arg_override, out);
            } else if (!cg_emit_expr(out, cg, arg, NULL)) {
                return false;
            }
            fputs("));\n", out);
            continue;
        }

        cg_report_node(cg, arg, "FASE 6B scribe supports string, integer, boolean, and real expressions");
        return false;
    }

    return true;
}

static bool cg_emit_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent);

static bool cg_emit_lvalue(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *target, cct_codegen_value_kind_t *out_kind) {
    if (!target) {
        cg_report_node(cg, target, "invalid empty lvalue");
        return false;
    }
    if (target->type == AST_IDENTIFIER || target->type == AST_FIELD_ACCESS || target->type == AST_INDEX_ACCESS) {
        return cg_emit_expr(out, cg, target, out_kind);
    }
    if (target->type == AST_UNARY_OP && target->as.unary_op.operator == TOKEN_STAR) {
        return cg_emit_expr(out, cg, target, out_kind);
    }
    cg_report_node(cg, target, "assignment target is not supported in FASE 6B codegen");
    return false;
}

static bool cg_emit_block_statements(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *block, int indent) {
    if (!block || block->type != AST_BLOCK) {
        cg_report_node(cg, block, "internal codegen error: expected AST_BLOCK");
        return false;
    }

    cct_ast_node_list_t *stmts = block->as.block.statements;
    if (!stmts) return true;

    for (size_t i = 0; i < stmts->count; i++) {
        if (!cg_emit_stmt(out, cg, stmts->nodes[i], indent)) return false;
        if (cg->had_error) return false;
    }
    return true;
}

static bool cg_emit_compound_block(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *block, int indent) {
    cg_emit_indent(out, indent);
    fputs("{\n", out);
    cg_push_scope(cg);
    bool ok = cg_emit_block_statements(out, cg, block, indent + 1);
    cg_pop_scope(cg);
    if (!ok) return false;
    cg_emit_indent(out, indent);
    fputs("}\n", out);
    return true;
}

static bool cg_emit_donec_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent) {
    cct_codegen_value_kind_t cond_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    bool in_loop = false;

    u32 brk_id = cg->next_temp_id++;
    if (cg->loop_depth < 32) cg->loop_break_ids[cg->loop_depth] = brk_id;

    cg_emit_indent(out, indent);
    fputs("do\n", out);
    cg->loop_depth++;
    in_loop = true;
    if (!cg_emit_compound_block(out, cg, stmt->as.donec.body, indent)) {
        if (in_loop && cg->loop_depth > 0) cg->loop_depth--;
        return false;
    }
    if (in_loop && cg->loop_depth > 0) cg->loop_depth--;

    cg_emit_indent(out, indent);
    fputs("while (", out);
    if (!cg_emit_expr(out, cg, stmt->as.donec.condition, &cond_kind)) return false;
    if (!(cond_kind == CCT_CODEGEN_VALUE_BOOL || cond_kind == CCT_CODEGEN_VALUE_INT)) {
        cg_report_node(cg, stmt->as.donec.condition, "DONEC condition is not executable in FASE 4C codegen");
        return false;
    }
    fputs(");\n", out);
    cg_emit_indent(out, indent);
    fprintf(out, "__cct_loop_brk_%u:;\n", brk_id);
    return true;
}

static bool cg_emit_repete_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent) {
    cct_codegen_value_kind_t start_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    cct_codegen_value_kind_t end_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    cct_codegen_value_kind_t step_kind = CCT_CODEGEN_VALUE_UNKNOWN;

    u32 id = cg->next_temp_id++;
    char *start_name = cg_strdup_printf("__cct_repete_start_%u", id);
    char *end_name = cg_strdup_printf("__cct_repete_end_%u", id);
    char *step_name = cg_strdup_printf("__cct_repete_step_%u", id);
    bool in_loop = false;

    cg_emit_indent(out, indent);
    fputs("{\n", out);
    cg_push_scope(cg); /* loop scope (contains iterator and loop temps conceptually) */

    cg_emit_indent(out, indent + 1);
    fprintf(out, "long long %s = ", start_name);
    if (!cg_emit_expr(out, cg, stmt->as.repete.start, &start_kind)) goto fail;
    if (!(start_kind == CCT_CODEGEN_VALUE_INT || start_kind == CCT_CODEGEN_VALUE_BOOL)) {
        cg_report_node(cg, stmt->as.repete.start, "REPETE start must be integer/boolean executable in FASE 4C");
        goto fail;
    }
    fputs(";\n", out);

    cg_emit_indent(out, indent + 1);
    fprintf(out, "long long %s = ", end_name);
    if (!cg_emit_expr(out, cg, stmt->as.repete.end, &end_kind)) goto fail;
    if (!(end_kind == CCT_CODEGEN_VALUE_INT || end_kind == CCT_CODEGEN_VALUE_BOOL)) {
        cg_report_node(cg, stmt->as.repete.end, "REPETE end must be integer/boolean executable in FASE 4C");
        goto fail;
    }
    fputs(";\n", out);

    cg_emit_indent(out, indent + 1);
    fprintf(out, "long long %s = ", step_name);
    if (stmt->as.repete.step) {
        if (!cg_emit_expr(out, cg, stmt->as.repete.step, &step_kind)) goto fail;
        if (!(step_kind == CCT_CODEGEN_VALUE_INT || step_kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, stmt->as.repete.step, "REPETE GRADUS must be integer/boolean executable in FASE 4C");
            goto fail;
        }
    } else {
        fputs("1", out);
    }
    fputs(";\n", out);

    cg_emit_indent(out, indent + 1);
    fprintf(out, "if (%s == 0) { ", step_name);
    if (!cct_cg_emit_runtime_fail_call(out, "REPETE GRADUS cannot be 0")) goto fail;
    fputs("; }\n", out);

    /* Iterator lives in the loop scope for the duration of REPETE body generation. */
    if (!cg_define_local(cg, stmt->as.repete.iterator, CCT_CODEGEN_LOCAL_INT, NULL, stmt)) goto fail;

    cg_emit_indent(out, indent + 1);
    fprintf(out,
            "for (long long %s = %s; ((%s > 0) ? (%s <= %s) : (%s >= %s)); %s += %s)\n",
            stmt->as.repete.iterator,
            start_name,
            step_name,
            stmt->as.repete.iterator, end_name,
            stmt->as.repete.iterator, end_name,
            stmt->as.repete.iterator, step_name);
    if (cg->loop_depth < 32) cg->loop_break_ids[cg->loop_depth] = id;
    cg->loop_depth++;
    in_loop = true;
    if (!cg_emit_compound_block(out, cg, stmt->as.repete.body, indent + 1)) goto fail;
    if (in_loop && cg->loop_depth > 0) cg->loop_depth--;

    cg_pop_scope(cg);
    cg_emit_indent(out, indent + 1);
    fprintf(out, "__cct_loop_brk_%u:;\n", id);
    cg_emit_indent(out, indent);
    fputs("}\n", out);

    free(start_name);
    free(end_name);
    free(step_name);
    return true;

fail:
    if (in_loop && cg->loop_depth > 0) cg->loop_depth--;
    cg_pop_scope(cg);
    free(start_name);
    free(end_name);
    free(step_name);
    return false;
}

static bool cg_iterum_binding_kind_from_ast_type(
    cct_codegen_t *cg,
    const cct_ast_type_t *ast_type,
    cct_codegen_local_kind_t *kind_out
) {
    cct_codegen_local_kind_t kind = CCT_CODEGEN_LOCAL_INT;
    if (ast_type) {
        kind = cg_local_kind_from_ast_type(ast_type);
        if (kind == CCT_CODEGEN_LOCAL_STRUCT) {
            if (cg_is_known_ordo_type(cg, ast_type) && !cg_is_payload_ordo_type(cg, ast_type)) {
                kind = CCT_CODEGEN_LOCAL_ORDO;
            } else if (!cg_is_known_sigillum_type(cg, ast_type) &&
                       !cg_is_payload_ordo_type(cg, ast_type)) {
                kind = CCT_CODEGEN_LOCAL_UNSUPPORTED;
            }
        }
        if (kind == CCT_CODEGEN_LOCAL_ARRAY_INT ||
            kind == CCT_CODEGEN_LOCAL_ARRAY_BOOL ||
            kind == CCT_CODEGEN_LOCAL_ARRAY_STRING ||
            kind == CCT_CODEGEN_LOCAL_ARRAY_UMBRA ||
            kind == CCT_CODEGEN_LOCAL_ARRAY_FLAMMA) {
            kind = CCT_CODEGEN_LOCAL_UNSUPPORTED;
        }
    }

    if (kind_out) *kind_out = kind;
    return kind != CCT_CODEGEN_LOCAL_UNSUPPORTED;
}

static bool cg_emit_iterum_binding_decl(
    FILE *out,
    cct_codegen_t *cg,
    const cct_ast_node_t *stmt,
    int indent,
    const char *binding_name,
    cct_codegen_local_kind_t binding_kind,
    const cct_ast_type_t *binding_ast_type
) {
    if (!binding_name || !binding_name[0]) {
        cg_report_node(cg, stmt, "ITERUM binding name is empty");
        return false;
    }

    cg_emit_indent(out, indent);
    if (binding_kind == CCT_CODEGEN_LOCAL_INT ||
        binding_kind == CCT_CODEGEN_LOCAL_BOOL ||
        binding_kind == CCT_CODEGEN_LOCAL_ORDO) {
        fprintf(out, "long long %s = 0;\n", binding_name);
        return true;
    }
    if (binding_kind == CCT_CODEGEN_LOCAL_STRING) {
        fprintf(out, "const char *%s = NULL;\n", binding_name);
        return true;
    }
    if (binding_kind == CCT_CODEGEN_LOCAL_UMBRA) {
        fprintf(out, "double %s = 0.0;\n", binding_name);
        return true;
    }
    if (binding_kind == CCT_CODEGEN_LOCAL_FLAMMA) {
        fprintf(out, "float %s = 0.0f;\n", binding_name);
        return true;
    }
    if (binding_kind == CCT_CODEGEN_LOCAL_POINTER || binding_kind == CCT_CODEGEN_LOCAL_STRUCT) {
        const char *c_ty = cg_c_type_for_ast_type(cg, binding_ast_type);
        if (!c_ty) {
            cg_report_node(cg, stmt, "ITERUM binding type is outside executable subset in FASE 19D.1");
            return false;
        }
        if (binding_kind == CCT_CODEGEN_LOCAL_STRUCT) {
            fprintf(out, "%s %s = {0};\n", c_ty, binding_name);
        } else {
            fprintf(out, "%s %s = NULL;\n", c_ty, binding_name);
        }
        return true;
    }

    cg_report_node(cg, stmt, "ITERUM binding local kind is unsupported in FASE 19D.1");
    return false;
}

static bool cg_emit_iterum_binding_assign_from_ptr(
    FILE *out,
    cct_codegen_t *cg,
    const cct_ast_node_t *stmt,
    int indent,
    const char *binding_name,
    cct_codegen_local_kind_t binding_kind,
    const cct_ast_type_t *binding_ast_type,
    const char *src_ptr_name
) {
    if (!binding_name || !src_ptr_name) {
        cg_report_node(cg, stmt, "internal ITERUM binding assignment is incomplete");
        return false;
    }

    const char *ptr_cast_type = NULL;
    switch (binding_kind) {
        case CCT_CODEGEN_LOCAL_INT:
        case CCT_CODEGEN_LOCAL_BOOL:
        case CCT_CODEGEN_LOCAL_ORDO:
            ptr_cast_type = "long long";
            break;
        case CCT_CODEGEN_LOCAL_STRING:
            ptr_cast_type = "const char *";
            break;
        case CCT_CODEGEN_LOCAL_UMBRA:
            ptr_cast_type = "double";
            break;
        case CCT_CODEGEN_LOCAL_FLAMMA:
            ptr_cast_type = "float";
            break;
        case CCT_CODEGEN_LOCAL_POINTER:
        case CCT_CODEGEN_LOCAL_STRUCT:
            ptr_cast_type = cg_c_type_for_ast_type(cg, binding_ast_type);
            if (!ptr_cast_type) {
                cg_report_node(cg, stmt, "ITERUM binding pointer cast type unavailable in FASE 19D.1");
                return false;
            }
            break;
        default:
            cg_report_node(cg, stmt, "ITERUM binding assignment kind unsupported in FASE 19D.1");
            return false;
    }

    cg_emit_indent(out, indent);
    fprintf(out, "%s = *((%s*)%s);\n", binding_name, ptr_cast_type, src_ptr_name);
    return true;
}

static bool cg_emit_iterum_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent) {
    if (!stmt || !stmt->as.iterum.item_name || !stmt->as.iterum.collection || !stmt->as.iterum.body) {
        cg_report_node(cg, stmt, "internal ITERUM node is incomplete");
        return false;
    }

    enum {
        CCT_ITERUM_BRANCH_SERIES = 0,
        CCT_ITERUM_BRANCH_FLUXUS,
        CCT_ITERUM_BRANCH_MAP,
        CCT_ITERUM_BRANCH_SET
    } branch = CCT_ITERUM_BRANCH_FLUXUS;

    const cct_ast_type_t *collection_ast_type = cg_expr_ast_type(cg, stmt->as.iterum.collection);
    const cct_ast_type_t *array_ast_type = cg_expr_array_type(cg, stmt->as.iterum.collection);
    bool is_fluxus_like = collection_ast_type &&
                          collection_ast_type->is_pointer &&
                          collection_ast_type->element_type &&
                          collection_ast_type->element_type->name &&
                          strcmp(collection_ast_type->element_type->name, "NIHIL") == 0;

    cct_codegen_iter_collection_kind_t inferred_kind = CCT_CODEGEN_ITER_COLLECTION_NONE;
    const cct_ast_type_t *map_key_ast_type = NULL;
    const cct_ast_type_t *map_value_ast_type = NULL;
    cg_infer_iter_collection_from_expr(
        cg,
        stmt->as.iterum.collection,
        &inferred_kind,
        &map_key_ast_type,
        &map_value_ast_type
    );

    if (array_ast_type && array_ast_type->element_type) {
        branch = CCT_ITERUM_BRANCH_SERIES;
    } else if (inferred_kind == CCT_CODEGEN_ITER_COLLECTION_MAP) {
        branch = CCT_ITERUM_BRANCH_MAP;
    } else if (inferred_kind == CCT_CODEGEN_ITER_COLLECTION_SET) {
        branch = CCT_ITERUM_BRANCH_SET;
    } else if (is_fluxus_like || inferred_kind == CCT_CODEGEN_ITER_COLLECTION_FLUXUS) {
        branch = CCT_ITERUM_BRANCH_FLUXUS;
    } else {
        cg_report_node(cg, stmt->as.iterum.collection,
                       "ITERUM requires executable FLUXUS (SPECULUM NIHIL) or SERIES (and now map/set) in FASE 19D.1");
        return false;
    }

    if (branch == CCT_ITERUM_BRANCH_MAP) {
        if (!stmt->as.iterum.value_name || !stmt->as.iterum.value_name[0]) {
            cg_report_node(cg, stmt, "ITERUM over map requires 2 bindings: chave, valor");
            return false;
        }
    } else {
        if (stmt->as.iterum.value_name && stmt->as.iterum.value_name[0]) {
            if (branch == CCT_ITERUM_BRANCH_SET) {
                cg_report_node(cg, stmt, "ITERUM over set requires exactly 1 binding: elemento");
            } else {
                cg_report_node(cg, stmt, "ITERUM over FLUXUS/SERIES requires exactly 1 binding");
            }
            return false;
        }
    }

    u32 id = cg->next_temp_id++;
    char *idx_name = cg_strdup_printf("__cct_iterum_i_%u", id);
    char *len_name = cg_strdup_printf("__cct_iterum_len_%u", id);
    char *flux_name = cg_strdup_printf("__cct_iterum_flux_%u", id);
    char *ptr_name = cg_strdup_printf("__cct_iterum_ptr_%u", id);
    char *iter_name = cg_strdup_printf("__cct_iterum_it_%u", id);
    char *map_key_ptr_name = cg_strdup_printf("__cct_iterum_kptr_%u", id);
    char *map_value_ptr_name = cg_strdup_printf("__cct_iterum_vptr_%u", id);
    bool in_loop = false;

    cg_emit_indent(out, indent);
    fputs("{\n", out);
    cg_push_scope(cg);

    cct_codegen_local_kind_t item_kind = CCT_CODEGEN_LOCAL_INT;
    cct_codegen_local_kind_t value_kind = CCT_CODEGEN_LOCAL_INT;
    const cct_ast_type_t *item_ast_type = NULL;
    const cct_ast_type_t *value_ast_type = NULL;

    if (branch == CCT_ITERUM_BRANCH_SERIES) {
        item_ast_type = array_ast_type->element_type;
        if (!cg_iterum_binding_kind_from_ast_type(cg, item_ast_type, &item_kind)) {
            cg_report_node(cg, stmt, "ITERUM SERIES element type is outside executable subset in FASE 19D.1");
            goto fail;
        }
    } else if (branch == CCT_ITERUM_BRANCH_MAP) {
        item_ast_type = map_key_ast_type;
        value_ast_type = map_value_ast_type;
        if (!cg_iterum_binding_kind_from_ast_type(cg, item_ast_type, &item_kind)) {
            cg_report_node(cg, stmt, "ITERUM map key type is outside executable subset in FASE 19D.1");
            goto fail;
        }
        if (!cg_iterum_binding_kind_from_ast_type(cg, value_ast_type, &value_kind)) {
            cg_report_node(cg, stmt, "ITERUM map value type is outside executable subset in FASE 19D.1");
            goto fail;
        }
    } else if (branch == CCT_ITERUM_BRANCH_SET) {
        item_ast_type = map_key_ast_type;
        if (!cg_iterum_binding_kind_from_ast_type(cg, item_ast_type, &item_kind)) {
            cg_report_node(cg, stmt, "ITERUM set element type is outside executable subset in FASE 19D.1");
            goto fail;
        }
    } else {
        item_ast_type = map_value_ast_type;
        if (item_ast_type) {
            if (!cg_iterum_binding_kind_from_ast_type(cg, item_ast_type, &item_kind)) {
                cg_report_node(cg, stmt, "ITERUM FLUXUS element type is outside executable subset in FASE 19D.1");
                goto fail;
            }
        } else {
            item_kind = CCT_CODEGEN_LOCAL_INT;
        }
    }

    if (!cg_define_local(cg, stmt->as.iterum.item_name, item_kind, item_ast_type, stmt)) goto fail;
    if (!cg_emit_iterum_binding_decl(out, cg, stmt, indent + 1, stmt->as.iterum.item_name, item_kind, item_ast_type)) goto fail;
    if (branch == CCT_ITERUM_BRANCH_MAP) {
        if (!cg_define_local(cg, stmt->as.iterum.value_name, value_kind, value_ast_type, stmt)) goto fail;
        if (!cg_emit_iterum_binding_decl(out, cg, stmt, indent + 1, stmt->as.iterum.value_name, value_kind, value_ast_type)) goto fail;
    }

    if (cg->loop_depth < 32) cg->loop_break_ids[cg->loop_depth] = id;
    cg->loop_depth++;
    in_loop = true;

    if (branch == CCT_ITERUM_BRANCH_SERIES) {
        cct_codegen_value_kind_t collection_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent + 1);
        fprintf(out, "for (long long %s = 0; %s < %u; %s++) {\n",
                idx_name, idx_name, array_ast_type->array_size, idx_name);
        cg_emit_indent(out, indent + 2);
        fprintf(out, "%s = (", stmt->as.iterum.item_name);
        if (!cg_emit_expr(out, cg, stmt->as.iterum.collection, &collection_kind)) goto fail;
        if (collection_kind != CCT_CODEGEN_VALUE_ARRAY) {
            cg_report_node(cg, stmt->as.iterum.collection, "ITERUM SERIES branch expects array value");
            goto fail;
        }
        fprintf(out, ")[%s];\n", idx_name);
        if (!cg_emit_stmt(out, cg, stmt->as.iterum.body, indent + 2)) goto fail;
        cg_emit_indent(out, indent + 1);
        fputs("}\n", out);
    } else if (branch == CCT_ITERUM_BRANCH_FLUXUS) {
        cct_codegen_value_kind_t collection_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = ", flux_name);
        if (!cg_emit_expr(out, cg, stmt->as.iterum.collection, &collection_kind)) goto fail;
        if (collection_kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, stmt->as.iterum.collection, "ITERUM FLUXUS branch expects pointer value");
            goto fail;
        }
        fputs(";\n", out);

        cg_emit_indent(out, indent + 1);
        fprintf(out, "long long %s = cct_rt_fluxus_len(%s);\n", len_name, flux_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "for (long long %s = 0; %s < %s; %s++) {\n", idx_name, idx_name, len_name, idx_name);
        cg_emit_indent(out, indent + 2);
        fprintf(out, "void *%s = cct_rt_fluxus_get(%s, %s);\n", ptr_name, flux_name, idx_name);
        if (!cg_emit_iterum_binding_assign_from_ptr(
                out, cg, stmt, indent + 2,
                stmt->as.iterum.item_name, item_kind, item_ast_type, ptr_name)) {
            goto fail;
        }
        if (!cg_emit_stmt(out, cg, stmt->as.iterum.body, indent + 2)) goto fail;
        cg_emit_indent(out, indent + 1);
        fputs("}\n", out);
    } else if (branch == CCT_ITERUM_BRANCH_MAP) {
        cct_codegen_value_kind_t collection_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = ", flux_name);
        if (!cg_emit_expr(out, cg, stmt->as.iterum.collection, &collection_kind)) goto fail;
        if (collection_kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, stmt->as.iterum.collection, "ITERUM map branch expects pointer value");
            goto fail;
        }
        fputs(";\n", out);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = cct_rt_map_iter_begin(%s);\n", iter_name, flux_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = NULL;\n", map_key_ptr_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = NULL;\n", map_value_ptr_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "while (cct_rt_map_iter_next(%s, &%s, &%s)) {\n",
                iter_name, map_key_ptr_name, map_value_ptr_name);
        if (!cg_emit_iterum_binding_assign_from_ptr(
                out, cg, stmt, indent + 2,
                stmt->as.iterum.item_name, item_kind, item_ast_type, map_key_ptr_name)) {
            goto fail;
        }
        if (!cg_emit_iterum_binding_assign_from_ptr(
                out, cg, stmt, indent + 2,
                stmt->as.iterum.value_name, value_kind, value_ast_type, map_value_ptr_name)) {
            goto fail;
        }
        if (!cg_emit_stmt(out, cg, stmt->as.iterum.body, indent + 2)) goto fail;
        cg_emit_indent(out, indent + 1);
        fputs("}\n", out);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "cct_rt_map_iter_end(%s);\n", iter_name);
    } else { /* set */
        cct_codegen_value_kind_t collection_kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = ", flux_name);
        if (!cg_emit_expr(out, cg, stmt->as.iterum.collection, &collection_kind)) goto fail;
        if (collection_kind != CCT_CODEGEN_VALUE_POINTER) {
            cg_report_node(cg, stmt->as.iterum.collection, "ITERUM set branch expects pointer value");
            goto fail;
        }
        fputs(";\n", out);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = cct_rt_set_iter_begin(%s);\n", iter_name, flux_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "void *%s = NULL;\n", map_key_ptr_name);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "while (cct_rt_set_iter_next(%s, &%s)) {\n", iter_name, map_key_ptr_name);
        if (!cg_emit_iterum_binding_assign_from_ptr(
                out, cg, stmt, indent + 2,
                stmt->as.iterum.item_name, item_kind, item_ast_type, map_key_ptr_name)) {
            goto fail;
        }
        if (!cg_emit_stmt(out, cg, stmt->as.iterum.body, indent + 2)) goto fail;
        cg_emit_indent(out, indent + 1);
        fputs("}\n", out);
        cg_emit_indent(out, indent + 1);
        fprintf(out, "cct_rt_set_iter_end(%s);\n", iter_name);
    }

    if (in_loop && cg->loop_depth > 0) cg->loop_depth--;
    cg_pop_scope(cg);
    cg_emit_indent(out, indent + 1);
    fprintf(out, "__cct_loop_brk_%u:;\n", id);
    cg_emit_indent(out, indent);
    fputs("}\n", out);

    free(idx_name);
    free(len_name);
    free(flux_name);
    free(ptr_name);
    free(iter_name);
    free(map_key_ptr_name);
    free(map_value_ptr_name);
    return true;

fail:
    if (in_loop && cg->loop_depth > 0) cg->loop_depth--;
    cg_pop_scope(cg);
    free(idx_name);
    free(len_name);
    free(flux_name);
    free(ptr_name);
    free(iter_name);
    free(map_key_ptr_name);
    free(map_value_ptr_name);
    return false;
}

static void cg_emit_failure_terminal_after_uncaught(FILE *out, cct_codegen_t *cg, int indent) {
    cg_emit_indent(out, indent);
    if (cg->current_function_returns_nihil) {
        fputs("return 0;\n", out);
    } else {
        const char *ret_c = cg_c_type_for_ast_type(cg, cg->current_function_return_type);
        if (cg->current_function_return_type &&
            cg_value_kind_from_ast_type(cg->current_function_return_type) == CCT_CODEGEN_VALUE_STRUCT &&
            ret_c) {
            fprintf(out, "return (%s){0};\n", ret_c);
            return;
        }
        fputs("return 0;\n", out);
    }
}

static bool cg_emit_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent) {
    if (!stmt) return true;

    switch (stmt->type) {
        case AST_BLOCK:
            return cg_emit_compound_block(out, cg, stmt, indent);

        case AST_EVOCA: {
            cct_codegen_local_kind_t kind = cg_local_kind_from_ast_type(stmt->as.evoca.var_type);
            const bool is_constans = stmt->as.evoca.is_constans;
            const char *const_prefix = is_constans ? "const " : "";
            if (kind == CCT_CODEGEN_LOCAL_UNSUPPORTED) {
                if (stmt->as.evoca.var_type && stmt->as.evoca.var_type->is_pointer) {
                    cg_report_node(cg, stmt, "SPECULUM pointee type is outside FASE 7A executable subset");
                } else {
                    cg_report_node(cg, stmt, "EVOCA type not supported by FASE 7A codegen");
                }
                return false;
            }
            if (kind == CCT_CODEGEN_LOCAL_STRUCT && stmt->as.evoca.var_type) {
                if (cg_is_known_ordo_type(cg, stmt->as.evoca.var_type) &&
                    !cg_is_payload_ordo_type(cg, stmt->as.evoca.var_type)) {
                    kind = CCT_CODEGEN_LOCAL_ORDO;
                } else if (!cg_is_known_sigillum_type(cg, stmt->as.evoca.var_type) &&
                           !cg_is_payload_ordo_type(cg, stmt->as.evoca.var_type)) {
                    cg_report_nodef(cg, stmt, "named type '%s' is not executable in FASE 6B codegen",
                                    stmt->as.evoca.var_type->name ? stmt->as.evoca.var_type->name : "(anonymous)");
                    return false;
                }
            }

            if (!cg_define_local(cg, stmt->as.evoca.name, kind, stmt->as.evoca.var_type, stmt)) return false;
            if (stmt->as.evoca.initializer) {
                cct_codegen_iter_collection_kind_t coll_kind = CCT_CODEGEN_ITER_COLLECTION_NONE;
                const cct_ast_type_t *key_type = NULL;
                const cct_ast_type_t *value_type = NULL;
                cg_infer_iter_collection_from_expr(
                    cg,
                    stmt->as.evoca.initializer,
                    &coll_kind,
                    &key_type,
                    &value_type
                );
                if (coll_kind != CCT_CODEGEN_ITER_COLLECTION_NONE) {
                    cg_set_local_iter_collection_info(cg, stmt->as.evoca.name, coll_kind, key_type, value_type);
                }
            }

            cg_emit_indent(out, indent);
            if (kind == CCT_CODEGEN_LOCAL_ARRAY_INT || kind == CCT_CODEGEN_LOCAL_ARRAY_BOOL ||
                kind == CCT_CODEGEN_LOCAL_ARRAY_STRING ||
                kind == CCT_CODEGEN_LOCAL_ARRAY_UMBRA || kind == CCT_CODEGEN_LOCAL_ARRAY_FLAMMA) {
                const cct_ast_type_t *t = stmt->as.evoca.var_type;
                if (!t || !t->is_array || !t->element_type || t->array_size == 0) {
                    cg_report_node(cg, stmt, "FASE 6B requires static SERIES with fixed size");
                    return false;
                }
                const char *elem_c = cg_c_type_for_ast_type(cg, t->element_type);
                if (!elem_c) {
                    cg_report_node(cg, stmt, "SERIES element type not executable in FASE 6B codegen");
                    return false;
                }
                if (stmt->as.evoca.initializer) {
                    cg_report_node(cg, stmt, "SERIES inline initializer is not supported in FASE 6B codegen");
                    return false;
                }
                fprintf(out, "%s%s %s[%u] = {0};\n", const_prefix, elem_c, stmt->as.evoca.name, t->array_size);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_INT || kind == CCT_CODEGEN_LOCAL_ORDO) {
                fprintf(out, "%slong long %s = ", const_prefix, stmt->as.evoca.name);
                if (stmt->as.evoca.initializer) {
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (!(k == CCT_CODEGEN_VALUE_INT || k == CCT_CODEGEN_VALUE_BOOL)) {
                        cg_report_node(cg, stmt, "EVOCA integer/ORDO local requires integer/boolean initializer in FASE 6B codegen");
                        return false;
                    }
                } else {
                    fputs("0", out);
                }
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_BOOL) {
                fprintf(out, "%slong long %s = ", const_prefix, stmt->as.evoca.name);
                if (stmt->as.evoca.initializer) {
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (!(k == CCT_CODEGEN_VALUE_BOOL || k == CCT_CODEGEN_VALUE_INT)) {
                        cg_report_node(cg, stmt, "EVOCA VERUM local requires boolean/integer initializer in FASE 6B codegen");
                        return false;
                    }
                } else {
                    fputs("0", out);
                }
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_STRING) {
                fprintf(out, "%s", is_constans ? "const char *const " : "const char *");
                fprintf(out, "%s = ", stmt->as.evoca.name);
                if (stmt->as.evoca.initializer) {
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (k != CCT_CODEGEN_VALUE_STRING) {
                        cg_report_node(cg, stmt, "EVOCA VERBUM local requires string initializer in FASE 6B codegen");
                        return false;
                    }
                } else {
                    fputs("NULL", out);
                }
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_POINTER) {
                const char *c_ty = cg_c_type_for_ast_type(cg, stmt->as.evoca.var_type);
                if (!c_ty) {
                    cg_report_node(cg, stmt, "SPECULUM pointee type is outside FASE 7A executable subset");
                    return false;
                }
                fprintf(out, "%s %s%s = ", c_ty, is_constans ? "const " : "", stmt->as.evoca.name);
                if (stmt->as.evoca.initializer) {
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (k != CCT_CODEGEN_VALUE_POINTER) {
                        cg_report_node(cg, stmt, "EVOCA SPECULUM local requires pointer initializer in FASE 7A codegen");
                        return false;
                    }
                } else {
                    fputs("NULL", out);
                }
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_UMBRA || kind == CCT_CODEGEN_LOCAL_FLAMMA) {
                fprintf(out, "%s%s %s = ",
                        const_prefix,
                        (kind == CCT_CODEGEN_LOCAL_UMBRA) ? "double" : "float",
                        stmt->as.evoca.name);
                if (stmt->as.evoca.initializer) {
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (!cct_cg_value_kind_is_numeric(k)) {
                        cg_report_node(cg, stmt, "EVOCA real local requires numeric initializer in FASE 6B codegen");
                        return false;
                    }
                } else {
                    fputs("0.0", out);
                }
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_STRUCT) {
                const char *c_ty = cg_c_type_for_ast_type(cg, stmt->as.evoca.var_type);
                if (!c_ty) {
                    cg_report_node(cg, stmt, "SIGILLUM type is not executable in FASE 6B codegen");
                    return false;
                }
                if (stmt->as.evoca.initializer) {
                    fprintf(out, "%s%s %s = ", const_prefix, c_ty, stmt->as.evoca.name);
                    cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (!cg_emit_expr(out, cg, stmt->as.evoca.initializer, &k)) return false;
                    if (k != CCT_CODEGEN_VALUE_STRUCT) {
                        cg_report_node(cg, stmt, "SIGILLUM initializer requires struct-compatible expression");
                        return false;
                    }
                    fputs(";\n", out);
                    if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                    return true;
                }
                fprintf(out, "%s%s %s = {0};\n", const_prefix, c_ty, stmt->as.evoca.name);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            cg_report_node(cg, stmt, "unsupported EVOCA local type");
            return false;
        }

        case AST_VINCIRE: {
            cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (stmt->as.vincire.value && stmt->as.vincire.value->type == AST_CONIURA) {
                if (!cg_probe_expr_kind(cg, stmt->as.vincire.target, &lhs_kind)) return false;
                char tmp_name[64];
                if (!cg_emit_coniura_expr_to_temp_with_failcheck(out, cg, stmt->as.vincire.value, indent, tmp_name, sizeof(tmp_name), &rhs_kind)) {
                    return false;
                }
                if (!cct_cg_assignment_kind_compatible(lhs_kind, rhs_kind)) {
                    cg_report_node(cg, stmt, "VINCIRE type unsupported/mismatched in subset 8B codegen");
                    return false;
                }
                cg_emit_indent(out, indent);
                if (!cg_emit_lvalue(out, cg, stmt->as.vincire.target, &lhs_kind)) return false;
                fprintf(out, " = %s;\n", tmp_name);
                if (stmt->as.vincire.target && stmt->as.vincire.target->type == AST_IDENTIFIER) {
                    cct_codegen_iter_collection_kind_t coll_kind = CCT_CODEGEN_ITER_COLLECTION_NONE;
                    const cct_ast_type_t *key_type = NULL;
                    const cct_ast_type_t *value_type = NULL;
                    cg_infer_iter_collection_from_expr(cg, stmt->as.vincire.value, &coll_kind, &key_type, &value_type);
                    if (coll_kind != CCT_CODEGEN_ITER_COLLECTION_NONE) {
                        cg_set_local_iter_collection_info(
                            cg,
                            stmt->as.vincire.target->as.identifier.name,
                            coll_kind,
                            key_type,
                            value_type
                        );
                    }
                }
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }
            cg_emit_indent(out, indent);
            if (!cg_emit_lvalue(out, cg, stmt->as.vincire.target, &lhs_kind)) return false;
            fputs(" = ", out);
            if (!cg_emit_expr(out, cg, stmt->as.vincire.value, &rhs_kind)) return false;
            if (!cct_cg_assignment_kind_compatible(lhs_kind, rhs_kind)) {
                cg_report_node(cg, stmt, "VINCIRE type unsupported/mismatched in FASE 6B codegen");
                return false;
            }

            fputs(";\n", out);
            if (stmt->as.vincire.target && stmt->as.vincire.target->type == AST_IDENTIFIER) {
                cct_codegen_iter_collection_kind_t coll_kind = CCT_CODEGEN_ITER_COLLECTION_NONE;
                const cct_ast_type_t *key_type = NULL;
                const cct_ast_type_t *value_type = NULL;
                cg_infer_iter_collection_from_expr(cg, stmt->as.vincire.value, &coll_kind, &key_type, &value_type);
                if (coll_kind != CCT_CODEGEN_ITER_COLLECTION_NONE) {
                    cg_set_local_iter_collection_info(
                        cg,
                        stmt->as.vincire.target->as.identifier.name,
                        coll_kind,
                        key_type,
                        value_type
                    );
                }
            }
            if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
            return true;
        }

        case AST_REDDE: {
            if (!stmt->as.redde.value) {
                cg_emit_indent(out, indent);
                if (cg->current_function_returns_nihil) {
                    fputs("return 0;\n", out);
                } else {
                    cg_report_node(cg, stmt, "REDDE without value is unsupported for non-NIHIL ritual in FASE 6B");
                    return false;
                }
                return true;
            }

            if (stmt->as.redde.value->type == AST_CONIURA) {
                cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
                char tmp_name[64];
                if (!cg_emit_coniura_expr_to_temp_with_failcheck(out, cg, stmt->as.redde.value, indent, tmp_name, sizeof(tmp_name), &kind)) {
                    return false;
                }
                bool allow_struct =
                    (kind == CCT_CODEGEN_VALUE_STRUCT) &&
                    cg_c_type_for_ast_type(cg, cg->current_function_return_type);
                if (kind == CCT_CODEGEN_VALUE_UNKNOWN || kind == CCT_CODEGEN_VALUE_ARRAY ||
                    (kind == CCT_CODEGEN_VALUE_STRUCT && !allow_struct)) {
                    cg_report_node(cg, stmt, "subset 8B return codegen does not support returning this CONIURA expression kind");
                    return false;
                }
                cg_emit_indent(out, indent);
                fprintf(out, "return (%s);\n", tmp_name);
                return true;
            }

            cg_emit_indent(out, indent);
            cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("return (", out);
            if (!cg_emit_expr(out, cg, stmt->as.redde.value, &kind)) return false;
            bool allow_struct =
                (kind == CCT_CODEGEN_VALUE_STRUCT) &&
                cg_c_type_for_ast_type(cg, cg->current_function_return_type);
            if (kind == CCT_CODEGEN_VALUE_UNKNOWN || kind == CCT_CODEGEN_VALUE_ARRAY ||
                (kind == CCT_CODEGEN_VALUE_STRUCT && !allow_struct)) {
                cg_report_node(cg, stmt, "FASE 6B return codegen does not support returning this expression kind");
                return false;
            }
            fputs(");\n", out);
            return true;
        }

        case AST_ANUR: {
            cg_emit_indent(out, indent);
            if (!stmt->as.anur.value) {
                fputs("exit(0);\n", out);
                return true;
            }
            cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
            fputs("exit((int)(", out);
            if (!cg_emit_expr(out, cg, stmt->as.anur.value, &kind)) return false;
            if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, stmt, "FASE 6B ANUR supports only integer/boolean exit code expressions");
                return false;
            }
            fputs("));\n", out);
            return true;
        }

        case AST_EXPR_STMT: {
            if (!stmt->as.expr_stmt.expression) {
                cg_emit_indent(out, indent);
                fputs("/* empty expr stmt */\n", out);
                return true;
            }
            cct_ast_node_t *expr = stmt->as.expr_stmt.expression;
            if (expr->type == AST_OBSECRO) {
                return cg_emit_scribe_stmt(out, cg, expr, indent);
            }
            if (expr->type == AST_CONIURA) {
                cct_codegen_value_kind_t call_kind = CCT_CODEGEN_VALUE_UNKNOWN;
                cg_emit_indent(out, indent);
                if (!cg_emit_expr(out, cg, expr, &call_kind)) return false;
                fputs(";\n", out);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }
            cg_report_nodef(cg, stmt, "expression statement %s not supported in FASE 6B codegen",
                            cct_ast_node_type_string(expr->type));
            return false;
        }

        case AST_DIMITTE: {
            const cct_ast_node_t *target = stmt->as.dimitte.target;
            const cct_ast_type_t *target_type = NULL;
            cct_codegen_value_kind_t target_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!target || !cg_is_addressable_lvalue_expr(target)) {
                cg_report_node(cg, stmt, "DIMITTE requires addressable pointer/VERBUM lvalue");
                return false;
            }
            target_type = cg_expr_ast_type(cg, target);
            target_kind = cg_value_kind_from_ast_type_codegen(cg, target_type);
            if (target_kind != CCT_CODEGEN_VALUE_POINTER && target_kind != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, target, "DIMITTE requires executable pointer/VERBUM lvalue");
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s((void*)(", cct_cg_runtime_free_helper_name());
            if (!cg_emit_lvalue(out, cg, target, NULL)) return false;
            fputs("));\n", out);
            cg_emit_indent(out, indent);
            if (!cg_emit_lvalue(out, cg, target, NULL)) return false;
            fputs(" = NULL;\n", out);
            return true;
        }

        case AST_IACE: {
            cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
            cg_emit_indent(out, indent);
            fputs("cct_rt_fractum_throw_str((const char*)(", out);
            if (!cg_emit_expr(out, cg, stmt->as.iace.value, &kind)) return false;
            if (kind != CCT_CODEGEN_VALUE_STRING) {
                cg_report_node(cg, stmt,
                               "IACE payload codegen in subset 8A supports only VERBUM/FRACTUM expressions (subset final da FASE 8)");
                return false;
            }
            fputs("));\n", out);

            const char *catch_label = cg_current_fail_handler_label(cg);
            cg_emit_indent(out, indent);
            if (catch_label) {
                fprintf(out, "goto %s;\n", catch_label);
            } else {
                /* FASE 8B propagation: leave FRACTUM active and return to caller. */
                cg_emit_failure_terminal_after_uncaught(out, cg, indent);
            }
            return true;
        }

        case AST_TEMPTA: {
            if (!stmt->as.tempta.try_block || !stmt->as.tempta.cape_block ||
                !stmt->as.tempta.cape_name || !stmt->as.tempta.cape_type) {
                cg_report_node(cg, stmt,
                               "TEMPTA/CAPE subset 8A node is incomplete for codegen (subset final da FASE 8 expects CAPE FRACTUM ident)");
                return false;
            }
            if (!stmt->as.tempta.cape_type->name || strcmp(stmt->as.tempta.cape_type->name, "FRACTUM") != 0 ||
                stmt->as.tempta.cape_type->is_pointer || stmt->as.tempta.cape_type->is_array) {
                cg_report_node(cg, stmt,
                               "CAPE in subset 8A requires FRACTUM binding type (subset final da FASE 8 mantém CAPE FRACTUM)");
                return false;
            }

            u32 id = cg->next_temp_id++;
            char *catch_label = cg_strdup_printf("__cct_tempta_catch_%u", id);
            char *dispatch_label = cg_strdup_printf("__cct_tempta_dispatch_%u", id);
            char *semper_label = cg_strdup_printf("__cct_tempta_semper_%u", id);
            char *end_label = cg_strdup_printf("__cct_tempta_end_%u", id);
            char *stage_name = cg_strdup_printf("__cct_tempta_stage_%u", id);
            char *prop_name = cg_strdup_printf("__cct_tempta_propagate_%u", id);
            char *saved_msg_name = cg_strdup_printf("__cct_tempta_saved_msg_%u", id);
            bool has_semper = (stmt->as.tempta.semper_block != NULL);
            const char *outer_fail_label = cg_current_fail_handler_label(cg);

            cg_emit_indent(out, indent);
            fputs("{\n", out);
            cg_push_scope(cg);

            cg_emit_indent(out, indent + 1);
            fprintf(out, "int %s = 1;\n", stage_name); /* 1=try, 2=cape, 3=semper */
            cg_emit_indent(out, indent + 1);
            fprintf(out, "int %s = 0;\n", prop_name);
            cg_emit_indent(out, indent + 1);
            fprintf(out, "const char *%s = NULL;\n", saved_msg_name);

            cg_emit_indent(out, indent + 1);
            fputs("{\n", out);
            cg_push_scope(cg);

            if (!cg_push_fail_handler(cg, dispatch_label)) goto tempta_fail;

            if (!cg_emit_block_statements(out, cg, stmt->as.tempta.try_block, indent + 2)) goto tempta_fail;

            cg_pop_fail_handler(cg);
            cg_pop_scope(cg);
            cg_emit_indent(out, indent + 1);
            fputs("}\n", out);

            cg_emit_indent(out, indent + 1);
            fprintf(out, "goto %s;\n", has_semper ? semper_label : end_label);

            cg_emit_indent(out, indent);
            fprintf(out, "%s:\n", dispatch_label);
            cg_emit_indent(out, indent + 1);
            fprintf(out, "if (%s == 1) goto %s;\n", stage_name, catch_label);
            cg_emit_indent(out, indent + 1);
            fprintf(out, "%s = 1;\n", prop_name);
            cg_emit_indent(out, indent + 1);
            fprintf(out, "goto %s;\n", has_semper ? semper_label : end_label);

            cg_emit_indent(out, indent);
            fprintf(out, "%s:\n", catch_label);

            cg_emit_indent(out, indent + 1);
            fprintf(out, "%s = 2;\n", stage_name);
            cg_emit_indent(out, indent + 1);
            fputs("{\n", out);
            cg_push_scope(cg);
            if (!cg_define_local(cg, stmt->as.tempta.cape_name, CCT_CODEGEN_LOCAL_STRING, stmt->as.tempta.cape_type, stmt)) {
                goto tempta_fail;
            }
            cg_emit_indent(out, indent + 2);
            fprintf(out, "const char *%s = cct_rt_fractum_peek();\n", stmt->as.tempta.cape_name);
            cg_emit_indent(out, indent + 2);
            fputs("cct_rt_fractum_clear();\n", out);
            if (!cg_push_fail_handler(cg, dispatch_label)) goto tempta_fail;
            if (!cg_emit_block_statements(out, cg, stmt->as.tempta.cape_block, indent + 2)) goto tempta_fail;
            cg_pop_fail_handler(cg);
            cg_pop_scope(cg);
            cg_emit_indent(out, indent + 1);
            fputs("}\n", out);

            cg_emit_indent(out, indent);
            if (has_semper) {
                fprintf(out, "%s:\n", semper_label);
                cg_emit_indent(out, indent + 1);
                fprintf(out, "%s = 3;\n", stage_name);
                /* Preserve incoming failure while SEMPER runs, so normal SEMPER statements don't short-circuit. */
                cg_emit_indent(out, indent + 1);
                fputs("if (cct_rt_fractum_is_active()) {\n", out);
                cg_emit_indent(out, indent + 2);
                fprintf(out, "%s = cct_rt_fractum_peek();\n", saved_msg_name);
                cg_emit_indent(out, indent + 2);
                fputs("cct_rt_fractum_clear();\n", out);
                cg_emit_indent(out, indent + 1);
                fputs("}\n", out);
                cg_emit_indent(out, indent + 1);
                fputs("{\n", out);
                cg_push_scope(cg);
                if (!cg_push_fail_handler(cg, dispatch_label)) goto tempta_fail;
                if (!cg_emit_block_statements(out, cg, stmt->as.tempta.semper_block, indent + 2)) goto tempta_fail;
                cg_pop_fail_handler(cg);
                cg_pop_scope(cg);
                cg_emit_indent(out, indent + 1);
                fputs("}\n", out);
            }

            cg_emit_indent(out, indent);
            fprintf(out, "%s:\n", end_label);
            if (has_semper) {
                cg_emit_indent(out, indent + 1);
                fprintf(out, "if (%s && !cct_rt_fractum_is_active() && %s) {\n", prop_name, saved_msg_name);
                cg_emit_indent(out, indent + 2);
                fprintf(out, "cct_rt_fractum_throw_str(%s);\n", saved_msg_name);
                cg_emit_indent(out, indent + 1);
                fputs("}\n", out);
            }
            cg_emit_indent(out, indent + 1);
            fprintf(out, "if (%s || cct_rt_fractum_is_active()) {\n", prop_name);
            if (outer_fail_label) {
                cg_emit_indent(out, indent + 2);
                fprintf(out, "goto %s;\n", outer_fail_label);
            } else {
                cg_emit_failure_terminal_after_uncaught(out, cg, indent + 2);
            }
            cg_emit_indent(out, indent + 1);
            fputs("}\n", out);
            cg_emit_indent(out, indent + 1);
            fputs("; /* label anchor (avoids end-of-compound label warning on older C modes) */\n", out);

            cg_pop_scope(cg);
            cg_emit_indent(out, indent);
            fputs("}\n", out);

            free(catch_label);
            free(dispatch_label);
            free(semper_label);
            free(end_label);
            free(stage_name);
            free(prop_name);
            free(saved_msg_name);
            return true;

tempta_fail:
            while (cg->fail_handlers && strcmp(cg->fail_handlers->catch_label, dispatch_label) == 0) {
                cg_pop_fail_handler(cg);
            }
            free(catch_label);
            free(dispatch_label);
            free(semper_label);
            free(end_label);
            free(stage_name);
            free(prop_name);
            free(saved_msg_name);
            return false;
        }

        case AST_SI: {
            cct_codegen_value_kind_t cond_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            cg_emit_indent(out, indent);
            fputs("if (", out);
            if (!cg_emit_expr(out, cg, stmt->as.si.condition, &cond_kind)) return false;
            if (!(cond_kind == CCT_CODEGEN_VALUE_BOOL || cond_kind == CCT_CODEGEN_VALUE_INT)) {
                cg_report_node(cg, stmt->as.si.condition, "SI condition is not executable in FASE 4C codegen");
                return false;
            }
            fputs(")\n", out);
            if (!cg_emit_compound_block(out, cg, stmt->as.si.then_branch, indent)) return false;
            if (stmt->as.si.else_branch) {
                cg_emit_indent(out, indent);
                fputs("else\n", out);
                if (!cg_emit_compound_block(out, cg, stmt->as.si.else_branch, indent)) return false;
            }
            return true;
        }

        case AST_QUANDO: {
            cct_codegen_value_kind_t expr_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            if (!cg_probe_expr_kind(cg, stmt->as.quando.expression, &expr_kind)) return false;

            const cct_ast_type_t *quando_expr_type = cg_expr_ast_type(cg, stmt->as.quando.expression);
            if (quando_expr_type && cg_is_payload_ordo_type(cg, quando_expr_type)) {
                cct_codegen_ordo_t *ordo = cg_find_ordo(cg, quando_expr_type->name);
                const char *expr_c_type = cg_c_type_for_ast_type(cg, quando_expr_type);
                if (!ordo || !ordo->node || !expr_c_type) {
                    cg_report_node(cg, stmt->as.quando.expression, "QUANDO ORDO payload: tipo nao suportado em codegen");
                    return false;
                }

                char value_name[64];
                snprintf(value_name, sizeof(value_name), "__cct_quando_ordo_%u", cg->next_temp_id++);

                cg_emit_indent(out, indent);
                fputs("{\n", out);
                cg_emit_indent(out, indent + 1);
                fprintf(out, "%s %s = ", expr_c_type, value_name);
                if (!cg_emit_expr(out, cg, stmt->as.quando.expression, NULL)) return false;
                fputs(";\n", out);

                bool quando_entered_switch_depth = cg->loop_depth > 0;
                if (quando_entered_switch_depth) cg->quando_switch_depth++;

                cg_emit_indent(out, indent + 1);
                fprintf(out, "switch (%s.__tag) {\n", value_name);

                for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
                    cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
                    if (case_node->literal_count == 0) continue;
                    if (case_node->literal_count != 1) {
                        cg_report_node(cg, case_node->literals[0], "QUANDO ORDO: OR-cases com payload nao sao suportados nesta fase");
                        return false;
                    }
                    const cct_ast_node_t *literal = case_node->literals[0];
                    if (!literal || literal->type != AST_IDENTIFIER || !literal->as.identifier.name) {
                        cg_report_node(cg, literal ? literal : stmt, "QUANDO ORDO: CASO deve ser nome de variante");
                        return false;
                    }

                    const char *variant_name = literal->as.identifier.name;
                    cct_ast_ordo_variant_t *variant = cg_find_ordo_variant_decl(ordo->node, variant_name);
                    if (!variant) {
                        cg_report_nodef(cg, literal, "QUANDO ORDO: variante '%s' nao existe no tipo", variant_name);
                        return false;
                    }
                    if (case_node->binding_count != variant->field_count) {
                        cg_report_nodef(
                            cg,
                            literal,
                            "QUANDO ORDO: CASO %s com %zu bindings, variante tem %zu campos",
                            variant_name,
                            case_node->binding_count,
                            variant->field_count
                        );
                        return false;
                    }

                    cg_emit_indent(out, indent + 2);
                    fprintf(out, "case CCT_%s_%s: {\n", ordo->name, variant_name);

                    cg_push_scope(cg);
                    for (size_t j = 0; j < case_node->binding_count; j++) {
                        cct_ast_ordo_field_t *field = variant->fields[j];
                        const cct_ast_type_t *field_type = field ? field->type : NULL;
                        const char *binding_name = case_node->binding_names[j];
                        const char *field_c_type = cg_c_type_for_ast_type(cg, field_type);
                        if (!field || !field_type || !binding_name || !field_c_type) {
                            cg_report_node(cg, literal, "QUANDO ORDO payload: binding invalido em codegen");
                            return false;
                        }
                        cct_codegen_local_kind_t local_kind = cg_local_kind_from_ast_type(field_type);
                        if (local_kind == CCT_CODEGEN_LOCAL_UNSUPPORTED) {
                            cg_report_nodef(cg, literal, "QUANDO ORDO payload: binding '%s' tem tipo nao suportado",
                                            binding_name);
                            return false;
                        }
                        if (!cg_define_local(cg, binding_name, local_kind, field_type, literal)) {
                            return false;
                        }

                        cg_emit_indent(out, indent + 3);
                        fprintf(
                            out,
                            "%s %s = %s.__payload.%s.%s;\n",
                            field_c_type,
                            binding_name,
                            value_name,
                            variant_name,
                            field->name ? field->name : "_"
                        );
                    }

                    if (!cg_emit_compound_block(out, cg, case_node->body, indent + 3)) return false;
                    cg_pop_scope(cg);
                    cg_emit_indent(out, indent + 3);
                    fputs("break;\n", out);
                    cg_emit_indent(out, indent + 2);
                    fputs("}\n", out);
                }

                if (stmt->as.quando.else_body) {
                    cg_emit_indent(out, indent + 2);
                    fputs("default: {\n", out);
                    if (!cg_emit_compound_block(out, cg, stmt->as.quando.else_body, indent + 3)) return false;
                    cg_emit_indent(out, indent + 3);
                    fputs("break;\n", out);
                    cg_emit_indent(out, indent + 2);
                    fputs("}\n", out);
                }

                cg_emit_indent(out, indent + 1);
                fputs("}\n", out);
                if (quando_entered_switch_depth && cg->quando_switch_depth > 0) cg->quando_switch_depth--;
                cg_emit_indent(out, indent);
                fputs("}\n", out);
                return true;
            }

            if (expr_kind == CCT_CODEGEN_VALUE_STRING) {
                char value_name[64];
                snprintf(value_name, sizeof(value_name), "__cct_quando_str_%u", cg->next_temp_id++);

                cg_emit_indent(out, indent);
                fputs("{\n", out);
                cg_emit_indent(out, indent + 1);
                fprintf(out, "const char *%s = ", value_name);
                cct_codegen_value_kind_t emitted_kind = CCT_CODEGEN_VALUE_UNKNOWN;
                if (!cg_emit_expr(out, cg, stmt->as.quando.expression, &emitted_kind)) return false;
                if (emitted_kind != CCT_CODEGEN_VALUE_STRING) {
                    cg_report_node(cg, stmt->as.quando.expression, "QUANDO VERBUM expression must be string in codegen");
                    return false;
                }
                fputs(";\n", out);

                bool first_branch = true;
                for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
                    cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
                    if (case_node->literal_count == 0) continue;
                    cg_emit_indent(out, indent + 1);
                    fputs(first_branch ? "if (" : "else if (", out);
                    for (size_t j = 0; j < case_node->literal_count; j++) {
                        cct_codegen_value_kind_t literal_kind = CCT_CODEGEN_VALUE_UNKNOWN;
                        if (j > 0) fputs(" || ", out);
                        fputs("(strcmp(", out);
                        fputs(value_name, out);
                        fputs(", ", out);
                        if (!cg_emit_expr(out, cg, case_node->literals[j], &literal_kind)) return false;
                        if (literal_kind != CCT_CODEGEN_VALUE_STRING) {
                            cg_report_node(cg, case_node->literals[j], "QUANDO CASO over VERBUM requires string literal in codegen");
                            return false;
                        }
                        fputs(") == 0)", out);
                    }
                    fputs(")\n", out);
                    if (!cg_emit_compound_block(out, cg, case_node->body, indent + 1)) return false;
                    first_branch = false;
                }

                if (stmt->as.quando.else_body) {
                    if (first_branch) {
                        if (!cg_emit_compound_block(out, cg, stmt->as.quando.else_body, indent + 1)) return false;
                    } else {
                        cg_emit_indent(out, indent + 1);
                        fputs("else\n", out);
                        if (!cg_emit_compound_block(out, cg, stmt->as.quando.else_body, indent + 1)) return false;
                    }
                }

                cg_emit_indent(out, indent);
                fputs("}\n", out);
                return true;
            }

            char value_name[64];
            snprintf(value_name, sizeof(value_name), "__cct_quando_val_%u", cg->next_temp_id++);

            cg_emit_indent(out, indent);
            fputs("{\n", out);
            cg_emit_indent(out, indent + 1);
            fprintf(out, "long long %s = (long long)(", value_name);
            if (!cg_emit_expr(out, cg, stmt->as.quando.expression, &expr_kind)) return false;
            if (!(expr_kind == CCT_CODEGEN_VALUE_INT || expr_kind == CCT_CODEGEN_VALUE_BOOL)) {
                cg_report_node(cg, stmt->as.quando.expression, "QUANDO expression must be integer/boolean/string in codegen");
                return false;
            }
            fputs(");\n", out);

            bool first_branch = true;
            for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
                cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
                if (case_node->literal_count == 0) continue;

                cg_emit_indent(out, indent + 1);
                fputs(first_branch ? "if (" : "else if (", out);
                for (size_t j = 0; j < case_node->literal_count; j++) {
                    cct_codegen_value_kind_t literal_kind = CCT_CODEGEN_VALUE_UNKNOWN;
                    if (j > 0) fputs(" || ", out);
                    fputs(value_name, out);
                    fputs(" == (long long)(", out);
                    if (!cg_emit_expr(out, cg, case_node->literals[j], &literal_kind)) return false;
                    if (!(literal_kind == CCT_CODEGEN_VALUE_INT || literal_kind == CCT_CODEGEN_VALUE_BOOL)) {
                        cg_report_node(cg, case_node->literals[j], "QUANDO CASO literal must be integer/boolean in codegen");
                        return false;
                    }
                    fputs(")", out);
                }
                fputs(")\n", out);
                if (!cg_emit_compound_block(out, cg, case_node->body, indent + 1)) return false;
                first_branch = false;
            }

            if (stmt->as.quando.else_body) {
                if (first_branch) {
                    if (!cg_emit_compound_block(out, cg, stmt->as.quando.else_body, indent + 1)) return false;
                } else {
                    cg_emit_indent(out, indent + 1);
                    fputs("else\n", out);
                    if (!cg_emit_compound_block(out, cg, stmt->as.quando.else_body, indent + 1)) return false;
                }
            }

            cg_emit_indent(out, indent);
            fputs("}\n", out);
            return true;
        }

        case AST_DUM: {
            cct_codegen_value_kind_t cond_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            u32 dum_brk_id = cg->next_temp_id++;
            if (cg->loop_depth < 32) cg->loop_break_ids[cg->loop_depth] = dum_brk_id;
            cg_emit_indent(out, indent);
            fputs("while (", out);
            if (!cg_emit_expr(out, cg, stmt->as.dum.condition, &cond_kind)) return false;
            if (!(cond_kind == CCT_CODEGEN_VALUE_BOOL || cond_kind == CCT_CODEGEN_VALUE_INT)) {
                cg_report_node(cg, stmt->as.dum.condition, "DUM condition is not executable in FASE 4C codegen");
                return false;
            }
            fputs(")\n", out);
            cg->loop_depth++;
            bool ok = cg_emit_compound_block(out, cg, stmt->as.dum.body, indent);
            if (cg->loop_depth > 0) cg->loop_depth--;
            cg_emit_indent(out, indent);
            fprintf(out, "__cct_loop_brk_%u:;\n", dum_brk_id);
            return ok;
        }

        case AST_DONEC:
            return cg_emit_donec_stmt(out, cg, stmt, indent);

        case AST_REPETE:
            return cg_emit_repete_stmt(out, cg, stmt, indent);

        case AST_ITERUM:
            return cg_emit_iterum_stmt(out, cg, stmt, indent);

        case AST_FRANGE:
            if (cg->loop_depth == 0) {
                cg_report_node(cg, stmt, "FRANGE outside loop context");
                return false;
            }
            cg_emit_indent(out, indent);
            if (cg->quando_switch_depth > 0 && cg->loop_depth <= 32) {
                fprintf(out, "goto __cct_loop_brk_%u;\n", cg->loop_break_ids[cg->loop_depth - 1]);
            } else {
                fputs("break;\n", out);
            }
            return true;

        case AST_RECEDE:
            if (cg->loop_depth == 0) {
                cg_report_node(cg, stmt, "RECEDE outside loop context");
                return false;
            }
            cg_emit_indent(out, indent);
            fputs("continue;\n", out);
            return true;

        case AST_TRANSITUS:
            cg_report_nodef(cg, stmt, "%s codegen is not supported in FASE 4C",
                            cct_ast_node_type_string(stmt->type));
            return false;

        default:
            cg_report_nodef(cg, stmt, "statement %s not supported in FASE 4C codegen",
                            cct_ast_node_type_string(stmt->type));
            return false;
    }
}

/* ========================================================================
 * Emit helpers: program / C file
 * ======================================================================== */

static bool cg_validate_entry_signature(cct_codegen_t *cg, const cct_ast_node_t *rituale) {
    if (!rituale) return true; /* minimal program fallback */

    if (rituale->type != AST_RITUALE) {
        cg_report_node(cg, rituale, "internal codegen error: entry ritual is not AST_RITUALE");
        return false;
    }

    if (rituale->as.rituale.params && rituale->as.rituale.params->count != 0) {
        cg_report_node(cg, rituale, "FASE 4C only supports entry ritual with zero parameters");
        return false;
    }

    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->is_pointer) {
        cg_report_node(cg, rituale, "FASE 4C does not support pointer return type for entry ritual");
        return false;
    }

    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->is_array) {
        cg_report_node(cg, rituale, "FASE 4C does not support array return type for entry ritual");
        return false;
    }

    if (rituale->as.rituale.return_type && rituale->as.rituale.return_type->name) {
        const char *name = rituale->as.rituale.return_type->name;
        if (!(strcmp(name, "NIHIL") == 0 ||
              strcmp(name, "VERUM") == 0 ||
              strcmp(name, "REX") == 0 ||
              strcmp(name, "DUX") == 0 ||
              strcmp(name, "COMES") == 0 ||
              strcmp(name, "MILES") == 0)) {
            cg_report_nodef(cg, rituale, "FASE 4C entry ritual return type '%s' is not supported", name);
            return false;
        }
    }

    return true;
}

static bool cg_emit_string_pool(FILE *out, cct_codegen_t *cg) {
    if (!cg->strings) return true;

    fputs("/* CCT string pool (FASE 4A/4B/4C) */\n", out);
    for (cct_codegen_string_t *s = cg->strings; s; s = s->next) {
        fprintf(out, "static const char %s[] = ", s->label);
        cg_emit_c_escaped_string(out, s->value);
        fputs(";\n", out);
    }
    fputc('\n', out);
    return true;
}

static bool cg_precollect_strings_from_expr(cct_codegen_t *cg, const cct_ast_node_t *expr) {
    if (!expr) return true;
    switch (expr->type) {
        case AST_LITERAL_STRING:
            (void)cg_string_label_for_value(cg, expr->as.literal_string.string_value);
            return true;
        case AST_MOLDE:
            if (expr->as.molde.parts) {
                for (size_t i = 0; i < expr->as.molde.part_count; i++) {
                    cct_ast_molde_part_t *part = &expr->as.molde.parts[i];
                    if (!part) continue;
                    if (part->kind == CCT_AST_MOLDE_PART_LITERAL) {
                        (void)cg_string_label_for_value(cg, part->literal_text ? part->literal_text : "");
                    } else if (!cg_precollect_strings_from_expr(cg, part->expr)) {
                        return false;
                    }
                }
            }
            return true;
        case AST_BINARY_OP:
            return cg_precollect_strings_from_expr(cg, expr->as.binary_op.left) &&
                   cg_precollect_strings_from_expr(cg, expr->as.binary_op.right);
        case AST_UNARY_OP:
            return cg_precollect_strings_from_expr(cg, expr->as.unary_op.operand);
        case AST_CONIURA:
            if (expr->as.coniura.arguments) {
                for (size_t i = 0; i < expr->as.coniura.arguments->count; i++) {
                    if (!cg_precollect_strings_from_expr(cg, expr->as.coniura.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_OBSECRO:
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "crypto_sha256_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_sha256_bytes") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_sha512_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_sha512_bytes") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_hmac_sha256") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_hmac_sha512") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_pbkdf2_sha256") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_csprng_bytes") == 0 ||
                 strcmp(expr->as.obsecro.name, "crypto_constant_time_compare") == 0)) {
                cg->uses_crypto = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "regex_builtin_compile") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_match") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_search") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_find_all") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_replace") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_split") == 0 ||
                 strcmp(expr->as.obsecro.name, "regex_builtin_last_error") == 0)) {
                cg->uses_regex = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "toml_builtin_parse") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_parse_file") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_last_error") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_type") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_get_string") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_get_int") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_get_real") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_get_bool") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_get_subdoc") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_array_len") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_array_item_string") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_array_item_int") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_array_item_real") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_array_item_bool") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_expand_env") == 0 ||
                 strcmp(expr->as.obsecro.name, "toml_builtin_stringify") == 0)) {
                cg->uses_toml = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "compress_builtin_gzip_compress_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "compress_builtin_gzip_compress_bytes") == 0 ||
                 strcmp(expr->as.obsecro.name, "compress_builtin_gzip_decompress_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "compress_builtin_gzip_decompress_bytes") == 0 ||
                 strcmp(expr->as.obsecro.name, "compress_builtin_last_error") == 0)) {
                cg->uses_compress = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "filetype_builtin_detect_path") == 0 ||
                 strcmp(expr->as.obsecro.name, "filetype_builtin_detect_bytes") == 0)) {
                cg->uses_filetype = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "image_builtin_load") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_free") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_save") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_resize") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_crop") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_rotate") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_convert") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_get_width") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_get_height") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_get_channels") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_get_format") == 0 ||
                 strcmp(expr->as.obsecro.name, "image_builtin_last_error") == 0)) {
                cg->uses_image_ops = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "signal_builtin_is_supported") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_install") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_last_kind") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_last_sequence") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_last_unix_ms") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_clear") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_check_shutdown") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_received_sigterm") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_received_sigint") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_received_sighup") == 0 ||
                 strcmp(expr->as.obsecro.name, "signal_builtin_raise_self") == 0)) {
                cg->uses_signal = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "db_open") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_close") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_exec") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_last_error") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_query") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_prepare") == 0 ||
                 strcmp(expr->as.obsecro.name, "rows_next") == 0 ||
                 strcmp(expr->as.obsecro.name, "rows_get_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "rows_get_int") == 0 ||
                 strcmp(expr->as.obsecro.name, "rows_get_real") == 0 ||
                 strcmp(expr->as.obsecro.name, "rows_close") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_bind_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_bind_int") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_bind_real") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_step") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_reset") == 0 ||
                 strcmp(expr->as.obsecro.name, "stmt_finalize") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_begin") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_commit") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_rollback") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_scalar_int") == 0 ||
                 strcmp(expr->as.obsecro.name, "db_scalar_text") == 0)) {
                cg->uses_sqlite = true;
            }
            if (expr->as.obsecro.name &&
                strcmp(expr->as.obsecro.name, "mail_builtin_smtp_send") == 0) {
                cg->uses_mail = true;
                cg->uses_crypto = true;
            }
            if (expr->as.obsecro.name &&
                strncmp(expr->as.obsecro.name, "instr_builtin_", 14) == 0) {
                cg->uses_instrument = true;
            }
            if (expr->as.obsecro.name &&
                (strcmp(expr->as.obsecro.name, "pg_builtin_open") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_close") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_is_open") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_last_error") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_exec") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_query") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_count") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_columns") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_next") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_get_text") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_is_null") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_rows_close") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_poll_channel") == 0 ||
                 strcmp(expr->as.obsecro.name, "pg_builtin_poll_payload") == 0)) {
                cg->uses_postgres = true;
            }
            if (expr->as.obsecro.arguments) {
                for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                    if (!cg_precollect_strings_from_expr(cg, expr->as.obsecro.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_CALL:
            if (expr->as.call.callee && !cg_precollect_strings_from_expr(cg, expr->as.call.callee)) return false;
            if (expr->as.call.arguments) {
                for (size_t i = 0; i < expr->as.call.arguments->count; i++) {
                    if (!cg_precollect_strings_from_expr(cg, expr->as.call.arguments->nodes[i])) return false;
                }
            }
            return true;
        case AST_FIELD_ACCESS:
            return cg_precollect_strings_from_expr(cg, expr->as.field_access.object);
        case AST_INDEX_ACCESS:
            return cg_precollect_strings_from_expr(cg, expr->as.index_access.array) &&
                   cg_precollect_strings_from_expr(cg, expr->as.index_access.index);
        default:
            return true;
    }
}

static bool cg_precollect_strings_from_stmt(cct_codegen_t *cg, const cct_ast_node_t *stmt) {
    if (!stmt) return true;
    switch (stmt->type) {
        case AST_BLOCK:
            if (stmt->as.block.statements) {
                for (size_t i = 0; i < stmt->as.block.statements->count; i++) {
                    if (!cg_precollect_strings_from_stmt(cg, stmt->as.block.statements->nodes[i])) return false;
                }
            }
            return true;
        case AST_EVOCA:
            return cg_precollect_strings_from_expr(cg, stmt->as.evoca.initializer);
        case AST_VINCIRE:
            return cg_precollect_strings_from_expr(cg, stmt->as.vincire.target) &&
                   cg_precollect_strings_from_expr(cg, stmt->as.vincire.value);
        case AST_REDDE:
            return cg_precollect_strings_from_expr(cg, stmt->as.redde.value);
        case AST_ANUR:
            return cg_precollect_strings_from_expr(cg, stmt->as.anur.value);
        case AST_EXPR_STMT:
            return cg_precollect_strings_from_expr(cg, stmt->as.expr_stmt.expression);
        case AST_DIMITTE:
            return cg_precollect_strings_from_expr(cg, stmt->as.dimitte.target);
        case AST_SI:
            return cg_precollect_strings_from_expr(cg, stmt->as.si.condition) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.si.then_branch) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.si.else_branch);
        case AST_QUANDO:
            if (!cg_precollect_strings_from_expr(cg, stmt->as.quando.expression)) return false;
            for (size_t i = 0; i < stmt->as.quando.case_count; i++) {
                cct_ast_case_node_t *case_node = &stmt->as.quando.cases[i];
                for (size_t j = 0; j < case_node->literal_count; j++) {
                    if (!cg_precollect_strings_from_expr(cg, case_node->literals[j])) return false;
                }
                if (!cg_precollect_strings_from_stmt(cg, case_node->body)) return false;
            }
            return cg_precollect_strings_from_stmt(cg, stmt->as.quando.else_body);
        case AST_DUM:
            return cg_precollect_strings_from_expr(cg, stmt->as.dum.condition) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.dum.body);
        case AST_DONEC:
            return cg_precollect_strings_from_stmt(cg, stmt->as.donec.body) &&
                   cg_precollect_strings_from_expr(cg, stmt->as.donec.condition);
        case AST_REPETE:
            return cg_precollect_strings_from_expr(cg, stmt->as.repete.start) &&
                   cg_precollect_strings_from_expr(cg, stmt->as.repete.end) &&
                   cg_precollect_strings_from_expr(cg, stmt->as.repete.step) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.repete.body);
        case AST_ITERUM:
            return cg_precollect_strings_from_expr(cg, stmt->as.iterum.collection) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.iterum.body);
        case AST_TEMPTA:
            return cg_precollect_strings_from_stmt(cg, stmt->as.tempta.try_block) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.tempta.cape_block) &&
                   cg_precollect_strings_from_stmt(cg, stmt->as.tempta.semper_block);
        case AST_IACE:
            return cg_precollect_strings_from_expr(cg, stmt->as.iace.value);
        default:
            return true;
    }
}

static const char* cg_c_type_for_local_kind(cct_codegen_local_kind_t kind) {
    switch (kind) {
        case CCT_CODEGEN_LOCAL_INT: return "long long";
        case CCT_CODEGEN_LOCAL_BOOL: return "long long";
        case CCT_CODEGEN_LOCAL_STRING: return "const char *";
        case CCT_CODEGEN_LOCAL_UMBRA: return "double";
        case CCT_CODEGEN_LOCAL_FLAMMA: return "float";
        case CCT_CODEGEN_LOCAL_POINTER: return "void *";
        case CCT_CODEGEN_LOCAL_ORDO: return "long long";
        default: return NULL;
    }
}

static const char* cg_c_type_for_return_kind(cct_codegen_value_kind_t kind) {
    switch (kind) {
        case CCT_CODEGEN_VALUE_NIHIL:
        case CCT_CODEGEN_VALUE_INT:
        case CCT_CODEGEN_VALUE_BOOL:
            return "long long";
        case CCT_CODEGEN_VALUE_REAL:
            return "double";
        case CCT_CODEGEN_VALUE_STRING:
            return "const char *";
        case CCT_CODEGEN_VALUE_POINTER:
            return "void *";
        default:
            return NULL;
    }
}

static const char* cg_c_type_for_ast_type(cct_codegen_t *cg, const cct_ast_type_t *type) {
    if (!type) return NULL;
    if (type->is_pointer) {
        const cct_ast_type_t *elem = type->element_type;
        if (!elem || elem->is_pointer || elem->is_array || !elem->name) return NULL;
        const char *elem_name = elem->name;
        if (elem->generic_args && elem->generic_args->count > 0) {
            elem_name = cg_materialize_generic_sigillum_name(cg, elem, NULL);
            if (!elem_name) return NULL;
        }
        if (strcmp(elem_name, "NIHIL") == 0) {
            return "void *";
        }
        const char *scalar = NULL;
        if (strcmp(elem_name, "VERBUM") != 0) {
            scalar = cg_c_scalar_type_for_name(elem_name);
        }
        if (!scalar) {
            cct_ast_type_t tmp = *elem;
            tmp.name = (char*)elem_name;
            tmp.generic_args = NULL;
            if (cg_is_known_sigillum_type(cg, &tmp)) {
                scalar = elem_name; /* pointer to SIGILLUM in FASE 10B subset */
            } else if (cg_is_payload_ordo_type(cg, &tmp)) {
                scalar = cg_strdup_printf("cct_%s", elem_name);
            }
        }
        if (!scalar) return NULL; /* FASE 7A/7B pointer subset */
        size_t len = strlen(scalar);
        char *buf = (char*)cg_calloc(1, len + 3);
        memcpy(buf, scalar, len);
        buf[len] = ' ';
        buf[len + 1] = '*';
        buf[len + 2] = '\0';
        return buf; /* process-lifetime leak is acceptable for one-shot codegen */
    }
    if (!type->name) return NULL;
    const char *base_name = type->name;
    if (type->generic_args && type->generic_args->count > 0) {
        base_name = cg_materialize_generic_sigillum_name(cg, type, NULL);
        if (!base_name) return NULL;
    }
    const char *scalar = cg_c_scalar_type_for_name(base_name);
    if (scalar && !type->is_array) return scalar;
    if (!type->is_array) {
        cct_ast_type_t tmp = *type;
        tmp.name = (char*)base_name;
        tmp.generic_args = NULL;
        if (cg_is_known_sigillum_type(cg, &tmp)) return base_name;
        if (cg_is_known_ordo_type(cg, &tmp)) {
            cct_codegen_ordo_t *ordo = cg_find_ordo(cg, base_name);
            if (ordo && cg_ordo_decl_has_payload(ordo->node)) {
                return cg_strdup_printf("cct_%s", base_name);
            }
            return "long long";
        }
    }
    return NULL;
}

static bool cg_precollect_strings_from_program(cct_codegen_t *cg, const cct_ast_program_t *program) {
    if (!program || !program->declarations) return true;
    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl) continue;
        if (decl->type == AST_RITUALE && decl->as.rituale.body) {
            if (!cg_precollect_strings_from_stmt(cg, decl->as.rituale.body)) return false;
        }
    }
    for (cct_codegen_rituale_t *r = cg->rituales; r; r = r->next) {
        if (r->node && r->node->type == AST_RITUALE && r->node->as.rituale.body) {
            if (!cg_precollect_strings_from_stmt(cg, r->node->as.rituale.body)) return false;
        }
    }
    return true;
}

static bool cg_emit_ordo_decls(FILE *out, cct_codegen_t *cg) {
    for (cct_codegen_ordo_t *o = cg->ordines; o; o = o->next) {
        if (!o->node || o->node->type != AST_ORDO) continue;
        if (cg_ordo_decl_has_payload(o->node)) {
            fprintf(out, "#ifndef CCT_ORDO_%s_DEFINED\n", o->name);
            fprintf(out, "#define CCT_ORDO_%s_DEFINED\n", o->name);

            if (o->node->as.ordo.variants) {
                i64 next_tag = 0;
                for (size_t i = 0; i < o->node->as.ordo.variants->count; i++) {
                    cct_ast_ordo_variant_t *variant = o->node->as.ordo.variants->variants[i];
                    if (!variant || !variant->name) continue;
                    i64 tag = variant->has_value ? variant->value : next_tag;
                    fprintf(out, "#define CCT_%s_%s %lld\n", o->name, variant->name, (long long)tag);
                    next_tag = tag + 1;
                }
            }

            fprintf(out, "typedef struct {\n");
            fprintf(out, "  int __tag;\n");
            fprintf(out, "  union {\n");
            if (o->node->as.ordo.variants) {
                for (size_t i = 0; i < o->node->as.ordo.variants->count; i++) {
                    cct_ast_ordo_variant_t *variant = o->node->as.ordo.variants->variants[i];
                    if (!variant || !variant->name || variant->field_count == 0) continue;
                    fprintf(out, "    struct {\n");
                    for (size_t j = 0; j < variant->field_count; j++) {
                        cct_ast_ordo_field_t *field = variant->fields[j];
                        if (!field || !field->type) continue;
                        const char *c_ty = cg_c_type_for_ast_type(cg, field->type);
                        if (!c_ty) {
                            cg_report_nodef(cg, o->node,
                                            "ORDO payload: campo %s tem tipo nao-suportado em payload",
                                            field->name ? field->name : "<campo>");
                            return false;
                        }
                        fprintf(out, "      %s %s;\n", c_ty, field->name ? field->name : "_");
                    }
                    fprintf(out, "    } %s;\n", variant->name);
                }
            }
            fprintf(out, "  } __payload;\n");
            fprintf(out, "} cct_%s;\n", o->name);
            fprintf(out, "#endif /* CCT_ORDO_%s_DEFINED */\n\n", o->name);
            continue;
        }

        fprintf(out, "typedef long long %s;\n", o->name);
        if (o->node->as.ordo.variants && o->node->as.ordo.variants->count > 0) {
            i64 next_value = 0;
            for (size_t i = 0; i < o->node->as.ordo.variants->count; i++) {
                cct_ast_ordo_variant_t *variant = o->node->as.ordo.variants->variants[i];
                if (!variant || !variant->name) continue;
                if (variant->has_value) next_value = variant->value;
                fprintf(out, "static const long long %s = %lld;\n", variant->name, (long long)next_value);
                next_value++;
            }
        } else {
            cct_ast_enum_item_list_t *items = o->node->as.ordo.items;
            i64 next_value = 0;
            if (items) {
                for (size_t i = 0; i < items->count; i++) {
                    cct_ast_enum_item_t *it = items->items[i];
                    if (!it) continue;
                    if (it->has_value) next_value = it->value;
                    fprintf(out, "static const long long %s = %lld;\n", it->name, (long long)next_value);
                    next_value++;
                }
            }
        }
        fputc('\n', out);
    }
    return true;
}

static bool cg_emit_sigillum_decls(FILE *out, cct_codegen_t *cg) {
    /* Emit forward typedefs first so nested/composed SIGILLUM fields can refer to
     * types declared later in source (registry order is not guaranteed). */
    for (cct_codegen_sigillum_t *s = cg->sigilla; s; s = s->next) {
        if (!s->node || s->node->type != AST_SIGILLUM) continue;
        fprintf(out, "typedef struct %s %s;\n", s->name, s->name);
    }
    if (cg->sigilla) fputc('\n', out);

    for (cct_codegen_sigillum_t *s = cg->sigilla; s; s = s->next) {
        if (!s->node || s->node->type != AST_SIGILLUM) continue;
        fprintf(out, "struct %s {\n", s->name);
        cct_ast_field_list_t *fields = s->node->as.sigillum.fields;
        if (fields) {
            for (size_t i = 0; i < fields->count; i++) {
                cct_ast_field_t *f = fields->fields[i];
                if (!f || !f->type) continue;
                if (f->type->is_array) {
                    const char *elem_c = cg_c_type_for_ast_type(cg, f->type->element_type);
                    if (!elem_c || f->type->array_size == 0) {
                        cg_report_nodef(cg, s->node, "SIGILLUM '%s' field '%s' uses unsupported SERIES type in FASE 6B codegen",
                                        s->name, f->name ? f->name : "(field)");
                        return false;
                    }
                    fprintf(out, "    %s %s[%u];\n", elem_c, f->name, f->type->array_size);
                    continue;
                }
                const char *c_ty = cg_c_type_for_ast_type(cg, f->type);
                if (!c_ty) {
                    cg_report_nodef(cg, s->node, "SIGILLUM '%s' field '%s' type is not executable in FASE 6B codegen",
                                    s->name, f->name ? f->name : "(field)");
                    return false;
                }
                fprintf(out, "    %s %s;\n", c_ty, f->name);
            }
        }
        fprintf(out, "};\n\n");
    }
    return true;
}

static bool cg_emit_rituale_prototype(FILE *out, cct_codegen_t *cg, const cct_codegen_rituale_t *rit) {
    const char *ret_c = cg_c_type_for_return_kind(rit->return_kind);
    if (rit->node->as.rituale.return_type) {
        const char *ast_ret = cg_c_type_for_ast_type(cg, rit->node->as.rituale.return_type);
        if (ast_ret) ret_c = ast_ret;
    }
    if (!ret_c) {
        cg_report_node((cct_codegen_t*)cg, rit->node, "unsupported ritual return type in FASE 6B prototype emission");
        return false;
    }

    bool export_entry = cg_is_explicit_entry_name(cg, rit->name);
    const char *storage_prefix = "";
    if (!export_entry) {
        storage_prefix = (cg->profile == CCT_PROFILE_FREESTANDING)
            ? "__attribute__((used)) static "
            : "static ";
    }
    fprintf(out, "%s%s %s(", storage_prefix, ret_c, rit->c_name);
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        const char *c_ty = cg_c_type_for_ast_type(cg, params->params[i]->type);
        if (!c_ty) c_ty = cg_c_type_for_local_kind(rit->param_kinds[i]);
        if (!c_ty) {
            cg_report_nodef((cct_codegen_t*)cg, rit->node, "unsupported parameter type for ritual '%s' in FASE 6B", rit->name);
            return false;
        }
        if (i > 0) fputs(", ", out);
        if (params->params[i]->is_constans) {
            if (strchr(c_ty, '*')) {
                fprintf(out, "%s const %s", c_ty, params->params[i]->name);
            } else {
                fprintf(out, "const %s %s", c_ty, params->params[i]->name);
            }
        } else {
            fprintf(out, "%s %s", c_ty, params->params[i]->name);
        }
    }
    fputs(");\n", out);
    return true;
}

static bool cg_register_rituale_params_as_locals(cct_codegen_t *cg, const cct_codegen_rituale_t *rit) {
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        cct_ast_type_t *ptype = params->params[i]->type;
        cct_codegen_local_kind_t kind = rit->param_kinds[i];
        if (kind == CCT_CODEGEN_LOCAL_STRUCT && ptype && cg_is_known_ordo_type(cg, ptype) &&
            !cg_is_payload_ordo_type(cg, ptype)) {
            kind = CCT_CODEGEN_LOCAL_ORDO;
        }
        if (!cg_define_local(cg, params->params[i]->name, kind, ptype, rit->node)) {
            return false;
        }
    }
    return true;
}

static bool cg_emit_rituale_function(FILE *out, cct_codegen_t *cg, const cct_codegen_rituale_t *rit) {
    const char *ret_c = cg_c_type_for_return_kind(rit->return_kind);
    if (rit->node->as.rituale.return_type) {
        const char *ast_ret = cg_c_type_for_ast_type(cg, rit->node->as.rituale.return_type);
        if (ast_ret) ret_c = ast_ret;
    }
    if (!ret_c) {
        cg_report_node(cg, rit->node, "unsupported ritual return type in FASE 6B");
        return false;
    }
    if (!rit->node->as.rituale.body || rit->node->as.rituale.body->type != AST_BLOCK) {
        cg_report_node(cg, rit->node, "FASE 4C codegen requires ritual body block");
        return false;
    }

    bool export_entry = cg_is_explicit_entry_name(cg, rit->name);
    const char *storage_prefix = "";
    if (!export_entry) {
        storage_prefix = (cg->profile == CCT_PROFILE_FREESTANDING)
            ? "__attribute__((used)) static "
            : "static ";
    }
    fprintf(out, "%s%s %s(", storage_prefix, ret_c, rit->c_name);
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        const char *c_ty = cg_c_type_for_ast_type(cg, params->params[i]->type);
        if (!c_ty) c_ty = cg_c_type_for_local_kind(rit->param_kinds[i]);
        if (!c_ty) {
            cg_report_nodef(cg, rit->node, "unsupported parameter type in ritual '%s' for FASE 6B", rit->name);
            return false;
        }
        if (i > 0) fputs(", ", out);
        if (params->params[i]->is_constans) {
            if (strchr(c_ty, '*')) {
                fprintf(out, "%s const %s", c_ty, params->params[i]->name);
            } else {
                fprintf(out, "const %s %s", c_ty, params->params[i]->name);
            }
        } else {
            fprintf(out, "%s %s", c_ty, params->params[i]->name);
        }
    }
    fputs(") {\n", out);

    cg_reset_locals(cg);
    cg_push_scope(cg); /* function scope */
    const cct_ast_type_t *prev_return_type = cg->current_function_return_type;
    bool prev_returns_nihil = cg->current_function_returns_nihil;
    cg->current_function_return_type = rit->node->as.rituale.return_type;
    cg->current_function_returns_nihil = rit->returns_nihil;
    if (!cg_register_rituale_params_as_locals(cg, rit)) {
        cg_pop_scope(cg);
        cg->current_function_return_type = prev_return_type;
        cg->current_function_returns_nihil = prev_returns_nihil;
        return false;
    }

    if (!cg_emit_block_statements(out, cg, rit->node->as.rituale.body, 1)) {
        cg_pop_scope(cg);
        cg->current_function_return_type = prev_return_type;
        cg->current_function_returns_nihil = prev_returns_nihil;
        return false;
    }
    cg_pop_scope(cg);
    cg->current_function_return_type = prev_return_type;
    cg->current_function_returns_nihil = prev_returns_nihil;

    if (rit->return_kind == CCT_CODEGEN_VALUE_STRUCT &&
        cg_c_type_for_ast_type(cg, rit->node->as.rituale.return_type)) {
        fprintf(out, "    return (%s){0};\n", ret_c);
    } else {
        fputs("    return 0;\n", out);
    }
    fputs("}\n\n", out);
    return true;
}

static bool cg_emit_entry_wrapper_main(FILE *out, cct_codegen_t *cg) {
    fputs("int main(int argc, char **argv) {\n", out);
    if (cg->profile == CCT_PROFILE_FREESTANDING) {
        fputs("    (void)argc;\n", out);
        fputs("    (void)argv;\n", out);
    } else {
        fputs("    cct_rt_args_init(argc, argv);\n", out);
    }

    if (!cg->entry_rituale) {
        fputs("    return 0;\n", out);
        fputs("}\n", out);
        return true;
    }

    cct_codegen_rituale_t *entry = cg_find_rituale(cg, cg->entry_rituale->as.rituale.name);
    if (!entry) {
        cg_report_node(cg, cg->entry_rituale, "internal codegen error: entry ritual missing from registry");
        return false;
    }

    if (entry->returns_nihil) {
        fprintf(out, "    %s();\n", entry->c_name);
        fputs("    if (cct_rt_fractum_is_active()) {\n", out);
        fputs("        cct_rt_fractum_uncaught_abort();\n", out);
        fputs("        return 1;\n", out);
        fputs("    }\n", out);
        fputs("    return 0;\n", out);
    } else {
        fprintf(out, "    long long __cct_entry_rc = (long long)%s();\n", entry->c_name);
        fputs("    if (cct_rt_fractum_is_active()) {\n", out);
        fputs("        cct_rt_fractum_uncaught_abort();\n", out);
        fputs("        return 1;\n", out);
        fputs("    }\n", out);
        fputs("    return (int)__cct_entry_rc;\n", out);
    }
    fputs("}\n", out);
    return true;
}

static bool cg_emit_generated_c(cct_codegen_t *cg, const cct_ast_program_t *program, FILE *out) {
    cg->source_program = program;
    if (!cg_collect_top_level_rituales(cg, program)) return false;
    if (cg->had_error) return false;
    if (!cg_validate_entry_signature(cg, cg->entry_rituale)) return false;
    if (!cg_precollect_strings_from_program(cg, program)) return false;

    if (!cct_cg_emit_generated_c_prelude(out, cg)) {
        cg_reportf(cg, 0, 0, "failed to emit runtime helpers into generated C");
        return false;
    }

    fputs("/* ===== String Pool ===== */\n", out);
    if (!cg_emit_string_pool(out, cg)) return false;
    fputs("/* ===== Type Declarations ===== */\n", out);
    if (!cg_emit_ordo_decls(out, cg)) return false;
    if (!cg_emit_sigillum_decls(out, cg)) return false;

    fputs("/* ===== Rituale Prototypes ===== */\n", out);
    for (cct_codegen_rituale_t *r = cg->rituales; r; r = r->next) {
        if (!cg_emit_rituale_prototype(out, cg, r)) return false;
    }
    if (cg->rituales) fputc('\n', out);

    fputs("/* ===== Generated Rituales ===== */\n", out);
    for (cct_codegen_rituale_t *r = cg->rituales; r; r = r->next) {
        if (!cg_emit_rituale_function(out, cg, r)) return false;
    }

    if (cg_has_explicit_freestanding_entry(cg)) {
        fputs("/* ===== Freestanding Entry (no host wrapper) ===== */\n", out);
    } else {
        fputs("/* ===== Host Entry Wrapper ===== */\n", out);
        if (!cg_emit_entry_wrapper_main(out, cg)) return false;
    }
    return !cg->had_error;
}

/* ========================================================================
 * Host compiler invocation
 * ======================================================================== */

#ifdef _WIN32
static void cg_win32_prepend_cc_dir_to_path(const char *cc) {
    /* Extract the directory part of the CC path so that gcc's own DLLs
     * (libgmp, libisl, libmpc, libgcc_s_seh, libwinpthread, …) can be
     * found by Windows even when the MSYS2 bin dir is not in the system PATH. */
    char dir[4096];
    strncpy(dir, cc, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *sep = NULL;
    for (char *p = dir; *p; p++) {
        if (*p == '\\' || *p == '/') sep = p;
    }
    if (!sep) return; /* cc is just "gcc" with no directory — nothing to add */
    *sep = '\0';

    const char *old = getenv("PATH");
    char buf[32768];
    if (old && old[0]) {
        snprintf(buf, sizeof(buf), "PATH=%s;%s", dir, old);
    } else {
        snprintf(buf, sizeof(buf), "PATH=%s", dir);
    }
    _putenv(buf);
}
#endif

static void cg_host_link_flags(const cct_codegen_t *cg, char *buffer, size_t buffer_size) {
    const char *extra_ldflags = getenv("CCT_HOST_LDFLAGS");
    const char *crypto_env_ldflags = getenv("CCT_CRYPTO_LDFLAGS");
    const char *compress_env_ldflags = getenv("CCT_ZLIB_LDFLAGS");
#ifdef _WIN32
    const char *default_ldflags = "";
    const char *crypto_pkg_config = "";
    const char *compress_pkg_config = "";
#else
    /*
     * Generated host executables embed runtime helpers that call libm
     * functions directly (pow/cbrt/sqrt/log/trig/etc). Link it explicitly on
     * Unix so Linux and macOS behave consistently instead of relying on
     * toolchain-specific defaults.
     */
    const char *default_ldflags = "-lm";
    const char *crypto_pkg_config =
        " $(pkg-config --libs openssl 2>/dev/null || printf '%s' '-lssl -lcrypto')";
    const char *compress_pkg_config =
        " $(pkg-config --libs zlib 2>/dev/null || printf '%s' '-lz')";
#ifdef __APPLE__
    const char *postgres_ldflags_default = "";
#else
    const char *postgres_ldflags_default = "-ldl";
#endif
#endif
    const char *sqlite_ldflags = (cg && cg->uses_sqlite) ? "-lsqlite3" : "";
    const char *crypto_ldflags = "";
    const char *compress_ldflags = "";
    const char *postgres_ldflags = (cg && cg->uses_postgres) ? postgres_ldflags_default : "";
    if (cg && cg->uses_crypto) {
        crypto_ldflags = (crypto_env_ldflags && crypto_env_ldflags[0]) ? crypto_env_ldflags : crypto_pkg_config;
    }
    if (cg && cg->uses_compress) {
        compress_ldflags = (compress_env_ldflags && compress_env_ldflags[0]) ? compress_env_ldflags : compress_pkg_config;
    }

    if (!buffer || buffer_size == 0) return;

    buffer[0] = '\0';
    if (default_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", default_ldflags);
    if (sqlite_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", sqlite_ldflags);
    if (crypto_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", crypto_ldflags);
    if (compress_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", compress_ldflags);
    if (postgres_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", postgres_ldflags);
    if (extra_ldflags && extra_ldflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", extra_ldflags);
}

static void cg_host_compile_flags(const cct_codegen_t *cg, char *buffer, size_t buffer_size) {
    const char *extra_cflags = getenv("CCT_HOST_CFLAGS");
    const char *crypto_env_cflags = getenv("CCT_CRYPTO_CFLAGS");
    const char *compress_env_cflags = getenv("CCT_ZLIB_CFLAGS");
#ifdef _WIN32
    const char *crypto_pkg_config = "";
    const char *compress_pkg_config = "";
#else
    const char *crypto_pkg_config = " $(pkg-config --cflags openssl 2>/dev/null)";
    const char *compress_pkg_config = " $(pkg-config --cflags zlib 2>/dev/null)";
#endif
    const char *crypto_cflags = "";
    const char *compress_cflags = "";
    if (cg && cg->uses_crypto) {
        crypto_cflags = (crypto_env_cflags && crypto_env_cflags[0]) ? crypto_env_cflags : crypto_pkg_config;
    }
    if (cg && cg->uses_compress) {
        compress_cflags = (compress_env_cflags && compress_env_cflags[0]) ? compress_env_cflags : compress_pkg_config;
    }

    if (!buffer || buffer_size == 0) return;
    buffer[0] = '\0';
    if (crypto_cflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", crypto_cflags);
    if (compress_cflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", compress_cflags);
    if (extra_cflags && extra_cflags[0]) snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " %s", extra_cflags);
}

static bool cg_run_host_compiler(cct_codegen_t *cg) {
    if (!cg->intermediate_c_path || !cg->output_executable_path) {
        cg_reportf(cg, 0, 0, "internal codegen error: missing output paths");
        return false;
    }

    const char *cc_env = getenv("CC");
    const char *cc = cc_env ? cc_env : (cg->host_cc ? cg->host_cc : "cc");
#ifdef _WIN32
    cg_win32_prepend_cc_dir_to_path(cc);
#endif
    char command[4096];
    char host_compile_flags[1024];
    char host_link_flags[1024];
    cg_host_compile_flags(cg, host_compile_flags, sizeof(host_compile_flags));
    cg_host_link_flags(cg, host_link_flags, sizeof(host_link_flags));
    if (cg->profile == CCT_PROFILE_FREESTANDING && cg_has_explicit_freestanding_entry(cg)) {
        snprintf(command, sizeof(command),
#ifdef _WIN32
                 "%s -std=c11 -O2%s -c -o \"%s\" \"%s\"",
#else
                 "%s -std=c11 -O2 -fwrapv%s -c -o \"%s\" \"%s\"",
#endif
                 cc, host_compile_flags, cg->output_executable_path, cg->intermediate_c_path);
    } else if (cg->profile == CCT_PROFILE_FREESTANDING) {
        snprintf(command, sizeof(command),
#ifdef _WIN32
                 "%s -std=c11 -O2%s -static -o \"%s\" \"%s\" \"%s\"%s",
#else
                 "%s -std=c11 -O2 -fwrapv%s -o \"%s\" \"%s\" \"%s\"%s",
#endif
                 cc, host_compile_flags, cg->output_executable_path, cg->intermediate_c_path,
                 CCT_FREESTANDING_RT_SOURCE, host_link_flags);
    } else {
        snprintf(command, sizeof(command),
#ifdef _WIN32
                 "%s -std=c11 -O2%s -static -o \"%s\" \"%s\"%s",
#else
                 "%s -std=c11 -O2 -fwrapv%s -o \"%s\" \"%s\"%s",
#endif
                 cc, host_compile_flags, cg->output_executable_path, cg->intermediate_c_path, host_link_flags);
    }

    int rc = system(command);
    if (rc != 0) {
        cct_error_printf(CCT_ERROR_CODEGEN,
                         "host compiler failed while building executable (command: %s)"
#ifdef _WIN32
                         "\n  hint: set the CC environment variable to the full path of gcc.exe"
                         "\n  example: set CC=C:\\msys64\\ucrt64\\bin\\gcc.exe"
#endif
                         , command);
        cg->had_error = true;
        cg->error_count++;
        return false;
    }

    return true;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void cct_codegen_init(cct_codegen_t *cg, const char *filename) {
    memset(cg, 0, sizeof(*cg));
    cg->filename = filename;
    cg->backend_kind = CCT_CODEGEN_BACKEND_C_HOST;
    cg->uses_sqlite = false;
    cg->uses_crypto = false;
    cg->uses_regex = false;
    cg->uses_toml = false;
    cg->uses_compress = false;
    cg->uses_filetype = false;
    cg->uses_image_ops = false;
    cg->uses_signal = false;
    cg->uses_postgres = false;
    cg->uses_mail = false;
    cg->uses_instrument = false;
#ifdef _WIN32
    cg->host_cc = "gcc";
#else
    cg->host_cc = "cc";
#endif
    cg->keep_intermediate = true;
    cg->profile = CCT_PROFILE_HOST;
    cg->entry_rituale_name = NULL;
}

void cct_codegen_set_backend(cct_codegen_t *cg, cct_codegen_backend_kind_t backend_kind) {
    if (!cg) return;
    cg->backend_kind = backend_kind;
}

void cct_codegen_dispose(cct_codegen_t *cg) {
    if (!cg) return;

    cct_codegen_string_t *s = cg->strings;
    while (s) {
        cct_codegen_string_t *next = s->next;
        free(s->value);
        free(s->label);
        free(s);
        s = next;
    }
    cg->strings = NULL;

    cct_codegen_rituale_t *r = cg->rituales;
    while (r) {
        cct_codegen_rituale_t *next = r->next;
        free(r->name);
        free(r->c_name);
        free(r->param_kinds);
        free(r);
        r = next;
    }
    cg->rituales = NULL;

    cct_codegen_sigillum_t *sig = cg->sigilla;
    while (sig) {
        cct_codegen_sigillum_t *next = sig->next;
        free(sig->name);
        free(sig);
        sig = next;
    }
    cg->sigilla = NULL;

    cct_codegen_ordo_t *ord = cg->ordines;
    while (ord) {
        cct_codegen_ordo_t *next = ord->next;
        free(ord->name);
        free(ord);
        ord = next;
    }
    cg->ordines = NULL;

    cg_reset_locals(cg);

    while (cg->fail_handlers) {
        cg_pop_fail_handler(cg);
    }

    cct_codegen_genus_inst_t *gi = cg->genus_instances;
    while (gi) {
        cct_codegen_genus_inst_t *next = gi->next;
        free(gi->key);
        free(gi->special_name);
        if (gi->materialized_node) {
            cct_ast_free_node(gi->materialized_node);
        }
        free(gi);
        gi = next;
    }
    cg->genus_instances = NULL;
    cg->source_program = NULL;

    free(cg->input_path);
    free(cg->output_executable_path);
    free(cg->intermediate_c_path);

    memset(cg, 0, sizeof(*cg));
}

bool cct_codegen_had_error(const cct_codegen_t *cg) {
    return cg ? cg->had_error : true;
}

u32 cct_codegen_error_count(const cct_codegen_t *cg) {
    return cg ? cg->error_count : 0;
}

static bool cg_compile_program_backend_c_host(
    cct_codegen_t *cg,
    const cct_ast_program_t *program,
    const char *input_path,
    const char *output_executable_path
) {
    if (!cg || !program || !input_path) return false;

    cg->input_path = cg_strdup(input_path);
    cg->output_executable_path = output_executable_path
        ? cg_strdup(output_executable_path)
        : derive_default_output_executable(input_path);
    cg->intermediate_c_path = derive_intermediate_c_path(cg->output_executable_path);

    FILE *out = fopen(cg->intermediate_c_path, "wb");
    if (!out) {
        cct_error_printf(CCT_ERROR_FILE_WRITE,
                         "could not open intermediate file for writing: %s",
                         cg->intermediate_c_path);
        cg->had_error = true;
        cg->error_count++;
        return false;
    }

    bool ok = cg_emit_generated_c(cg, program, out);
    fclose(out);

    if (!ok || cg->had_error) {
        return false;
    }

    if (!cg_run_host_compiler(cg)) {
        return false;
    }

    if (!cg->keep_intermediate && cg->intermediate_c_path) {
        (void)remove(cg->intermediate_c_path);
    }

    printf("Compiled: %s -> %s\n", input_path, cg->output_executable_path);
    printf("Intermediate C: %s\n", cg->intermediate_c_path);

    return true;
}

bool cct_codegen_compile_program(
    cct_codegen_t *cg,
    const cct_ast_program_t *program,
    const char *input_path,
    const char *output_executable_path
) {
    if (!cg) return false;

    switch (cg->backend_kind) {
        case CCT_CODEGEN_BACKEND_C_HOST:
            return cg_compile_program_backend_c_host(cg, program, input_path, output_executable_path);
        default:
            cct_error_printf(CCT_ERROR_CODEGEN, "unsupported codegen backend selected");
            return false;
    }
}
