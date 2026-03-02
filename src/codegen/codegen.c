/*
 * CCT — Clavicula Turing
 * Code Generator (FASE 4A..6D)
 *
 * Backend strategy:
 * - Generate C textual code for an executable subset
 * - Invoke host compiler (cc) to produce a native executable
 *
 * This remains the official C-hosted backend through FASE 6D.
 * Future phases may add/replace backends behind the same public entrypoints.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "codegen.h"
#include "codegen_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct cct_codegen_string {
    char *value;                    /* decoded string contents from AST */
    char *label;                    /* cct_str_N */
    cct_codegen_string_t *next;
};

struct cct_codegen_local {
    char *name;
    u32 scope_depth;
    cct_codegen_local_kind_t kind;
    const cct_ast_type_t *ast_type;
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
        if (cg_is_known_sigillum_type(cg, rituale->as.rituale.return_type)) {
            cg_report_nodef(cg, rituale, "rituale '%s' SIGILLUM return type is not supported in FASE 6B codegen",
                            rituale->as.rituale.name);
            return false;
        }
        if (!cg_is_known_ordo_type(cg, rituale->as.rituale.return_type)) {
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
                ok_param = false; /* by-value SIGILLUM params remain restricted in 7B */
            }
            if (pk == CCT_CODEGEN_LOCAL_STRUCT && param && param->type && cg_is_known_ordo_type(cg, param->type)) {
                ok_param = true;
            }

            if (!ok_param) {
                if (pk == CCT_CODEGEN_LOCAL_STRUCT && param && param->type && cg_is_known_sigillum_type(cg, param->type)) {
                    cg_report(cg, param->line, param->column,
                              "FASE 7B keeps SIGILLUM by-value ritual parameter outside executable subset (use SPECULUM SIGILLUM)");
                    return false;
                }
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
    r->c_name = cg_make_rituale_c_name(rituale->as.rituale.name);
    r->return_kind = cg_value_kind_from_ast_type(rituale->as.rituale.return_type);
    if (rituale->as.rituale.return_type && cg_is_known_ordo_type(cg, rituale->as.rituale.return_type)) {
        r->return_kind = CCT_CODEGEN_VALUE_INT;
    }
    r->returns_nihil = (r->return_kind == CCT_CODEGEN_VALUE_NIHIL);

    if (rituale->as.rituale.params && rituale->as.rituale.params->count > 0) {
        r->param_count = rituale->as.rituale.params->count;
        r->param_kinds = (cct_codegen_local_kind_t*)cg_calloc(r->param_count, sizeof(*r->param_kinds));
        for (size_t i = 0; i < r->param_count; i++) {
            cct_ast_type_t *ptype = rituale->as.rituale.params->params[i]->type;
            r->param_kinds[i] = cg_local_kind_from_ast_type(ptype);
            if (r->param_kinds[i] == CCT_CODEGEN_LOCAL_STRUCT && ptype && cg_is_known_ordo_type(cg, ptype)) {
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

        if (cg_is_entry_rituale_name(decl->as.rituale.name)) {
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

    cg->entry_rituale = found_entry;
    return !cg->had_error;
}

/* ========================================================================
 * Emit helpers: expressions and statements
 * ======================================================================== */

static bool cg_emit_expr(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *expr, cct_codegen_value_kind_t *out_kind);
static bool cg_emit_lvalue(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *target, cct_codegen_value_kind_t *out_kind);
static const cct_ast_type_t* cg_expr_struct_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const cct_ast_type_t* cg_expr_array_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const cct_ast_type_t* cg_expr_ast_type(cct_codegen_t *cg, const cct_ast_node_t *expr);
static const char* cg_c_type_for_ast_type(cct_codegen_t *cg, const cct_ast_type_t *type);
static const char* cg_c_type_for_return_kind(cct_codegen_value_kind_t kind);

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
    if (cg_is_known_ordo_type(cg, type)) return CCT_CODEGEN_VALUE_INT;
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

    switch (op) {
        case TOKEN_PLUS: op_text = "+"; break;
        case TOKEN_MINUS: op_text = "-"; break;
        case TOKEN_STAR: op_text = "*"; break;
        case TOKEN_SLASH: op_text = "/"; break;
        case TOKEN_PERCENT: op_text = "%"; break;
        case TOKEN_EQ_EQ: op_text = "=="; is_comparison = true; break;
        case TOKEN_BANG_EQ: op_text = "!="; is_comparison = true; break;
        case TOKEN_LESS: op_text = "<"; is_comparison = true; break;
        case TOKEN_LESS_EQ: op_text = "<="; is_comparison = true; break;
        case TOKEN_GREATER: op_text = ">"; is_comparison = true; break;
        case TOKEN_GREATER_EQ: op_text = ">="; is_comparison = true; break;
        default:
            cg_report_nodef(cg, expr, "operator %s not supported in FASE 4C codegen",
                            cct_token_type_string(op));
            return false;
    }

    cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;

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

    if (strcmp(name, "io_read_line") == 0) {
        if (argc != 0) {
            cg_report_node(cg, expr, "OBSECRO io_read_line expects no arguments in FASE 11E.1");
            return false;
        }
        fputs("cct_rt_io_read_line()", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
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
        strcmp(name, "path_ext") == 0) {
        if (argc != 1) {
            cg_report_nodef(cg, expr, "OBSECRO %s expects exactly one VERBUM argument in FASE 11E.2", name);
            return false;
        }
        cct_codegen_value_kind_t k = CCT_CODEGEN_VALUE_UNKNOWN;
        fprintf(out, "cct_rt_%s(", name);
        if (!cg_emit_expr(out, cg, args->nodes[0], &k)) return false;
        if (k != CCT_CODEGEN_VALUE_STRING) {
            cg_report_nodef(cg, args->nodes[0], "OBSECRO %s requires VERBUM argument in FASE 11E.2", name);
            return false;
        }
        fputs(")", out);
        if (out_kind) *out_kind = CCT_CODEGEN_VALUE_STRING;
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
        strcmp(name, "fluxus_free") == 0 || strcmp(name, "fluxus_push") == 0 ||
        strcmp(name, "fluxus_pop") == 0 || strcmp(name, "fluxus_clear") == 0 ||
        strcmp(name, "fluxus_reserve") == 0 ||
        strcmp(name, "map_free") == 0 || strcmp(name, "map_insert") == 0 ||
        strcmp(name, "map_clear") == 0 || strcmp(name, "map_reserve") == 0 ||
        strcmp(name, "set_free") == 0 || strcmp(name, "set_clear") == 0 ||
        strcmp(name, "io_print") == 0 || strcmp(name, "io_println") == 0 ||
        strcmp(name, "io_print_int") == 0 ||
        strcmp(name, "fs_write_all") == 0 || strcmp(name, "fs_append_all") == 0 ||
        strcmp(name, "random_seed") == 0 ||
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

        case AST_IDENTIFIER: {
            cct_codegen_local_t *local = cg_find_local(cg, expr->as.identifier.name);
            if (!local) {
                /* Try enum constant lookup (ORDO item). */
                for (cct_codegen_ordo_t *o = cg->ordines; o; o = o->next) {
                    cct_ast_enum_item_list_t *items = o->node->as.ordo.items;
                    if (!items) continue;
                    i64 next_value = 0;
                    for (size_t i = 0; i < items->count; i++) {
                        cct_ast_enum_item_t *it = items->items[i];
                        if (!it) continue;
                        if (it->has_value) next_value = it->value;
                        if (strcmp(it->name, expr->as.identifier.name) == 0) {
                            fprintf(out, "%lld", (long long)next_value);
                            if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
                            return true;
                        }
                        next_value++;
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

    if (strcmp(obsecro_node->as.obsecro.name, "io_print") == 0 ||
        strcmp(obsecro_node->as.obsecro.name, "io_println") == 0) {
        const char *name = obsecro_node->as.obsecro.name;
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_nodef(cg, obsecro_node, "OBSECRO %s requires exactly one VERBUM argument in FASE 11E.1", name);
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

    if (strcmp(obsecro_node->as.obsecro.name, "io_print_int") == 0) {
        cct_ast_node_list_t *args = obsecro_node->as.obsecro.arguments;
        if (!args || args->count != 1) {
            cg_report_node(cg, obsecro_node, "OBSECRO io_print_int requires exactly one integer argument in FASE 11E.1");
            return false;
        }
        cct_codegen_value_kind_t kind = CCT_CODEGEN_VALUE_UNKNOWN;
        cg_emit_indent(out, indent);
        fputs("cct_rt_io_print_int(", out);
        if (!cg_emit_expr(out, cg, args->nodes[0], &kind)) return false;
        if (!(kind == CCT_CODEGEN_VALUE_INT || kind == CCT_CODEGEN_VALUE_BOOL)) {
            cg_report_node(cg, args->nodes[0], "OBSECRO io_print_int requires integer executable argument in FASE 11E.1");
            return false;
        }
        fputs(");\n", out);
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

    if (strcmp(obsecro_node->as.obsecro.name, "scribe") != 0) {
        cg_report_nodef(cg, obsecro_node, "OBSECRO %s codegen is not supported in current executable subset (supported stmt builtins: scribe, libera, mem_free, mem_copy, mem_set, mem_zero, fluxus_free, fluxus_push, fluxus_pop, fluxus_clear, fluxus_reserve, map_free, map_insert, map_clear, map_reserve, set_free, set_clear, io_print, io_println, io_print_int, fs_write_all, fs_append_all, random_seed, option_free, result_free)",
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

    cg_emit_indent(out, indent);
    fputs("do\n", out);
    if (!cg_emit_compound_block(out, cg, stmt->as.donec.body, indent)) return false;

    cg_emit_indent(out, indent);
    fputs("while (", out);
    if (!cg_emit_expr(out, cg, stmt->as.donec.condition, &cond_kind)) return false;
    if (!(cond_kind == CCT_CODEGEN_VALUE_BOOL || cond_kind == CCT_CODEGEN_VALUE_INT)) {
        cg_report_node(cg, stmt->as.donec.condition, "DONEC condition is not executable in FASE 4C codegen");
        return false;
    }
    fputs(");\n", out);
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
    if (!cg_emit_compound_block(out, cg, stmt->as.repete.body, indent + 1)) goto fail;

    cg_pop_scope(cg);
    cg_emit_indent(out, indent);
    fputs("}\n", out);

    free(start_name);
    free(end_name);
    free(step_name);
    return true;

fail:
    cg_pop_scope(cg);
    free(start_name);
    free(end_name);
    free(step_name);
    return false;
}

static bool cg_emit_iterum_stmt(FILE *out, cct_codegen_t *cg, const cct_ast_node_t *stmt, int indent) {
    if (!stmt || !stmt->as.iterum.item_name || !stmt->as.iterum.collection || !stmt->as.iterum.body) {
        cg_report_node(cg, stmt, "internal ITERUM node is incomplete");
        return false;
    }

    const cct_ast_type_t *collection_ast_type = cg_expr_ast_type(cg, stmt->as.iterum.collection);
    const cct_ast_type_t *array_ast_type = cg_expr_array_type(cg, stmt->as.iterum.collection);
    bool is_fluxus_like = collection_ast_type &&
                          collection_ast_type->is_pointer &&
                          collection_ast_type->element_type &&
                          collection_ast_type->element_type->name &&
                          strcmp(collection_ast_type->element_type->name, "NIHIL") == 0;

    if (!array_ast_type && !is_fluxus_like) {
        cg_report_node(cg, stmt->as.iterum.collection,
                       "ITERUM requires executable FLUXUS (SPECULUM NIHIL) or SERIES value in FASE 12D.3");
        return false;
    }

    u32 id = cg->next_temp_id++;
    char *idx_name = cg_strdup_printf("__cct_iterum_i_%u", id);
    char *len_name = cg_strdup_printf("__cct_iterum_len_%u", id);
    char *flux_name = cg_strdup_printf("__cct_iterum_flux_%u", id);
    char *ptr_name = cg_strdup_printf("__cct_iterum_ptr_%u", id);

    cg_emit_indent(out, indent);
    fputs("{\n", out);
    cg_push_scope(cg);

    cct_codegen_local_kind_t item_kind = CCT_CODEGEN_LOCAL_INT;
    const cct_ast_type_t *item_ast_type = NULL;

    if (array_ast_type && array_ast_type->element_type) {
        item_ast_type = array_ast_type->element_type;
        item_kind = cg_local_kind_from_ast_type(item_ast_type);
        if (item_kind == CCT_CODEGEN_LOCAL_STRUCT && item_ast_type) {
            if (cg_is_known_ordo_type(cg, item_ast_type)) item_kind = CCT_CODEGEN_LOCAL_ORDO;
            else if (!cg_is_known_sigillum_type(cg, item_ast_type)) item_kind = CCT_CODEGEN_LOCAL_UNSUPPORTED;
        }
        if (item_kind == CCT_CODEGEN_LOCAL_UNSUPPORTED ||
            item_kind == CCT_CODEGEN_LOCAL_ARRAY_INT ||
            item_kind == CCT_CODEGEN_LOCAL_ARRAY_BOOL ||
            item_kind == CCT_CODEGEN_LOCAL_ARRAY_UMBRA ||
            item_kind == CCT_CODEGEN_LOCAL_ARRAY_FLAMMA) {
            cg_report_node(cg, stmt, "ITERUM SERIES element type is outside executable subset in FASE 12D.3");
            goto fail;
        }
    } else {
        /* FASE 12D.3 baseline: iterate opaque FLUXUS payload as long long items. */
        item_kind = CCT_CODEGEN_LOCAL_INT;
    }

    if (!cg_define_local(cg, stmt->as.iterum.item_name, item_kind, item_ast_type, stmt)) goto fail;

    cg_emit_indent(out, indent + 1);
    if (item_kind == CCT_CODEGEN_LOCAL_INT || item_kind == CCT_CODEGEN_LOCAL_BOOL || item_kind == CCT_CODEGEN_LOCAL_ORDO) {
        fprintf(out, "long long %s = 0;\n", stmt->as.iterum.item_name);
    } else if (item_kind == CCT_CODEGEN_LOCAL_STRING) {
        fprintf(out, "const char *%s = NULL;\n", stmt->as.iterum.item_name);
    } else if (item_kind == CCT_CODEGEN_LOCAL_UMBRA) {
        fprintf(out, "double %s = 0.0;\n", stmt->as.iterum.item_name);
    } else if (item_kind == CCT_CODEGEN_LOCAL_FLAMMA) {
        fprintf(out, "float %s = 0.0f;\n", stmt->as.iterum.item_name);
    } else if (item_kind == CCT_CODEGEN_LOCAL_POINTER || item_kind == CCT_CODEGEN_LOCAL_STRUCT) {
        const char *c_ty = cg_c_type_for_ast_type(cg, item_ast_type);
        if (!c_ty) {
            cg_report_node(cg, stmt, "ITERUM item type is outside executable subset in FASE 12D.3");
            goto fail;
        }
        if (item_kind == CCT_CODEGEN_LOCAL_STRUCT) {
            fprintf(out, "%s %s = {0};\n", c_ty, stmt->as.iterum.item_name);
        } else {
            fprintf(out, "%s %s = NULL;\n", c_ty, stmt->as.iterum.item_name);
        }
    } else {
        cg_report_node(cg, stmt, "ITERUM item local kind is unsupported in FASE 12D.3");
        goto fail;
    }

    if (array_ast_type && array_ast_type->element_type) {
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
    } else {
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
        cg_emit_indent(out, indent + 2);
        fprintf(out, "%s = *((long long*)%s);\n", stmt->as.iterum.item_name, ptr_name);
        if (!cg_emit_stmt(out, cg, stmt->as.iterum.body, indent + 2)) goto fail;
        cg_emit_indent(out, indent + 1);
        fputs("}\n", out);
    }

    cg_pop_scope(cg);
    cg_emit_indent(out, indent);
    fputs("}\n", out);

    free(idx_name);
    free(len_name);
    free(flux_name);
    free(ptr_name);
    return true;

fail:
    cg_pop_scope(cg);
    free(idx_name);
    free(len_name);
    free(flux_name);
    free(ptr_name);
    return false;
}

static void cg_emit_failure_terminal_after_uncaught(FILE *out, cct_codegen_t *cg, int indent) {
    cg_emit_indent(out, indent);
    if (cg->current_function_returns_nihil) {
        fputs("return 0;\n", out);
    } else {
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
            if (kind == CCT_CODEGEN_LOCAL_UNSUPPORTED) {
                if (stmt->as.evoca.var_type && stmt->as.evoca.var_type->is_pointer) {
                    cg_report_node(cg, stmt, "SPECULUM pointee type is outside FASE 7A executable subset");
                } else {
                    cg_report_node(cg, stmt, "EVOCA type not supported by FASE 7A codegen");
                }
                return false;
            }
            if (kind == CCT_CODEGEN_LOCAL_STRUCT && stmt->as.evoca.var_type) {
                if (cg_is_known_ordo_type(cg, stmt->as.evoca.var_type)) {
                    kind = CCT_CODEGEN_LOCAL_ORDO;
                } else if (!cg_is_known_sigillum_type(cg, stmt->as.evoca.var_type)) {
                    cg_report_nodef(cg, stmt, "named type '%s' is not executable in FASE 6B codegen",
                                    stmt->as.evoca.var_type->name ? stmt->as.evoca.var_type->name : "(anonymous)");
                    return false;
                }
            }

            if (!cg_define_local(cg, stmt->as.evoca.name, kind, stmt->as.evoca.var_type, stmt)) return false;

            cg_emit_indent(out, indent);
            if (kind == CCT_CODEGEN_LOCAL_ARRAY_INT || kind == CCT_CODEGEN_LOCAL_ARRAY_BOOL ||
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
                fprintf(out, "%s %s[%u] = {0};\n", elem_c, stmt->as.evoca.name, t->array_size);
                if (!cg_emit_fail_propagation_check(out, cg, indent)) return false;
                return true;
            }

            if (kind == CCT_CODEGEN_LOCAL_INT || kind == CCT_CODEGEN_LOCAL_ORDO) {
                fprintf(out, "long long %s = ", stmt->as.evoca.name);
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
                fprintf(out, "long long %s = ", stmt->as.evoca.name);
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
                fprintf(out, "const char *%s = ", stmt->as.evoca.name);
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
                fprintf(out, "%s %s = ", c_ty, stmt->as.evoca.name);
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
                fprintf(out, "%s %s = ", (kind == CCT_CODEGEN_LOCAL_UMBRA) ? "double" : "float", stmt->as.evoca.name);
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
                    cg_report_node(cg, stmt, "SIGILLUM inline initializer is not supported in FASE 6B codegen");
                    return false;
                }
                fprintf(out, "%s %s = {0};\n", c_ty, stmt->as.evoca.name);
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
                if (kind == CCT_CODEGEN_VALUE_UNKNOWN || kind == CCT_CODEGEN_VALUE_ARRAY || kind == CCT_CODEGEN_VALUE_STRUCT) {
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
            if (kind == CCT_CODEGEN_VALUE_UNKNOWN || kind == CCT_CODEGEN_VALUE_ARRAY || kind == CCT_CODEGEN_VALUE_STRUCT) {
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
            if (!target || target->type != AST_IDENTIFIER) {
                cg_report_node(cg, stmt, "DIMITTE in subset final da FASE 7 requires identifier pointer symbol");
                return false;
            }
            cct_codegen_local_t *local = cg_find_local(cg, target->as.identifier.name);
            if (!local || local->kind != CCT_CODEGEN_LOCAL_POINTER) {
                cg_report_nodef(cg, target, "DIMITTE target '%s' is not executable SPECULUM local/parameter in subset final da FASE 7",
                                target->as.identifier.name);
                return false;
            }
            cg_emit_indent(out, indent);
            fprintf(out, "%s((void*)%s);\n", cct_cg_runtime_free_helper_name(), target->as.identifier.name);
            cg_emit_indent(out, indent);
            fprintf(out, "%s = NULL;\n", target->as.identifier.name);
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

        case AST_DUM: {
            cct_codegen_value_kind_t cond_kind = CCT_CODEGEN_VALUE_UNKNOWN;
            cg_emit_indent(out, indent);
            fputs("while (", out);
            if (!cg_emit_expr(out, cg, stmt->as.dum.condition, &cond_kind)) return false;
            if (!(cond_kind == CCT_CODEGEN_VALUE_BOOL || cond_kind == CCT_CODEGEN_VALUE_INT)) {
                cg_report_node(cg, stmt->as.dum.condition, "DUM condition is not executable in FASE 4C codegen");
                return false;
            }
            fputs(")\n", out);
            return cg_emit_compound_block(out, cg, stmt->as.dum.body, indent);
        }

        case AST_DONEC:
            return cg_emit_donec_stmt(out, cg, stmt, indent);

        case AST_REPETE:
            return cg_emit_repete_stmt(out, cg, stmt, indent);

        case AST_ITERUM:
            return cg_emit_iterum_stmt(out, cg, stmt, indent);

        case AST_FRANGE:
        case AST_RECEDE:
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
        if (cg_is_known_ordo_type(cg, &tmp)) return "long long";
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
        fprintf(out, "typedef long long %s;\n", o->name);
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

    fprintf(out, "static %s %s(", ret_c, rit->c_name);
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        const char *c_ty = cg_c_type_for_ast_type(cg, params->params[i]->type);
        if (!c_ty) c_ty = cg_c_type_for_local_kind(rit->param_kinds[i]);
        if (!c_ty) {
            cg_report_nodef((cct_codegen_t*)cg, rit->node, "unsupported parameter type for ritual '%s' in FASE 6B", rit->name);
            return false;
        }
        if (i > 0) fputs(", ", out);
        fprintf(out, "%s %s", c_ty, params->params[i]->name);
    }
    fputs(");\n", out);
    return true;
}

static bool cg_register_rituale_params_as_locals(cct_codegen_t *cg, const cct_codegen_rituale_t *rit) {
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        cct_ast_type_t *ptype = params->params[i]->type;
        cct_codegen_local_kind_t kind = rit->param_kinds[i];
        if (kind == CCT_CODEGEN_LOCAL_STRUCT && ptype && cg_is_known_ordo_type(cg, ptype)) kind = CCT_CODEGEN_LOCAL_ORDO;
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

    fprintf(out, "static %s %s(", ret_c, rit->c_name);
    cct_ast_param_list_t *params = rit->node->as.rituale.params;
    for (size_t i = 0; i < rit->param_count; i++) {
        const char *c_ty = cg_c_type_for_ast_type(cg, params->params[i]->type);
        if (!c_ty) c_ty = cg_c_type_for_local_kind(rit->param_kinds[i]);
        if (!c_ty) {
            cg_report_nodef(cg, rit->node, "unsupported parameter type in ritual '%s' for FASE 6B", rit->name);
            return false;
        }
        if (i > 0) fputs(", ", out);
        fprintf(out, "%s %s", c_ty, params->params[i]->name);
    }
    fputs(") {\n", out);

    cg_reset_locals(cg);
    cg_push_scope(cg); /* function scope */
    cg->current_function_returns_nihil = rit->returns_nihil;
    if (!cg_register_rituale_params_as_locals(cg, rit)) {
        cg_pop_scope(cg);
        return false;
    }

    if (!cg_emit_block_statements(out, cg, rit->node->as.rituale.body, 1)) {
        cg_pop_scope(cg);
        return false;
    }
    cg_pop_scope(cg);

    fputs("    return 0;\n", out);
    fputs("}\n\n", out);
    return true;
}

static bool cg_emit_entry_wrapper_main(FILE *out, cct_codegen_t *cg) {
    fputs("int main(void) {\n", out);

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

    fputs("/* ===== Host Entry Wrapper ===== */\n", out);
    if (!cg_emit_entry_wrapper_main(out, cg)) return false;
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
    snprintf(command, sizeof(command),
#ifdef _WIN32
             "%s -std=c11 -O2 -static -o \"%s\" \"%s\"",
#else
             "%s -std=c11 -O2 -o \"%s\" \"%s\"",
#endif
             cc, cg->output_executable_path, cg->intermediate_c_path);

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
#ifdef _WIN32
    cg->host_cc = "gcc";
#else
    cg->host_cc = "cc";
#endif
    cg->keep_intermediate = true;
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
