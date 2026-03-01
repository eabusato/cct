/*
 * CCT — Clavicula Turing
 * Sigillum Generatio v2.1 (FASE 6A)
 *
 * Structural-first deterministic network sigil generation.
 * The AST defines the anatomy; a secondary semantic hash only modulates details.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "sigilo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

#define CCT_SIGILO_CANVAS_SIZE 512.0
#define CCT_SIGILO_CENTER_X 256.0
#define CCT_SIGILO_CENTER_Y 256.0
#define CCT_SIGILO_PI 3.14159265358979323846

#define CCT_SIGILO_MAX_RITUAIS 128
#define CCT_SIGILO_MAX_EDGES 512

typedef struct {
    char *name;
    bool is_entry;
    u32 total_statements;
    u32 direct_statements;
    u32 max_nesting;
    u32 evoca_count;
    u32 vincire_count;
    u32 si_count;
    u32 aliter_count;
    u32 dum_count;
    u32 donec_count;
    u32 repete_count;
    u32 iterum_count;
    u32 coniura_count;
    u32 obsecro_count;
    u32 redde_count;
    u32 anur_count;
} cct_sigilo_ritual_metric_t;

typedef struct {
    u32 from_idx;
    u32 to_idx;
    u32 count;
} cct_sigilo_call_edge_t;

typedef struct {
    const cct_ast_program_t *program;
    const char *program_name;
    cct_sigilo_ritual_metric_t *rituals;
    u32 ritual_count;
    i32 entry_idx;

    cct_sigilo_call_edge_t *edges;
    u32 edge_count;

    u32 total_statements;
    u32 total_exprs;
    u32 total_calls;
    u32 total_obsecro;
    u32 total_loops;
    u32 total_conditionals;
    u32 total_returns;
    u32 total_anur;
    u32 max_nesting;
    u32 depth_accum;
    u32 depth_samples;

    u32 count_si;
    u32 count_aliter;
    u32 count_dum;
    u32 count_donec;
    u32 count_repete;
    u32 count_iterum;
    u32 count_coniura;
    u32 count_obsecro;
    u32 count_redde;
    u32 count_anur;
    u32 count_tempta;
    u32 count_cape;
    u32 count_semper;
    u32 count_iace;
    u32 count_evoca;
    u32 count_vincire;
    u32 count_field_access;
    u32 count_index_access;
    u32 count_sigillum_decl;
    u32 count_sigillum_composite_field;
    u32 count_ordo_decl;
    u32 count_generic_decl;
    u32 count_generic_params;
    u32 count_generic_rituale_decl;
    u32 count_generic_sigillum_decl;
    u32 count_generic_constraint;
    u32 count_generic_constrained_param;
    u32 count_generic_constraint_violation;
    bool pactum_constraint_resolution_ok;
    u32 count_generic_instantiation;
    u32 count_generic_rituale_instantiation;
    u32 count_generic_sigillum_instantiation;
    u32 count_generic_instantiation_dedup;
    u32 count_series_type_use;
    u32 count_speculum_type_use;
    u32 count_type_umbra;
    u32 count_type_flamma;
    u32 count_addr_of;
    u32 count_deref;
    u32 count_alloc;
    u32 count_free;
    u32 count_dimitte;
    u32 count_fluxus_ops;
    u32 count_verbum_ops;
    u32 count_fmt_ops;
    u32 count_series_ops;
    u32 count_mem_ops;
    u32 count_io_ops;
    u32 count_fs_ops;
    u32 count_fluxus_init;
    u32 count_fluxus_push;
    u32 count_fluxus_pop;
    u32 count_fluxus_instances;
    u32 count_path_ops;
    u32 count_math_ops;
    u32 count_random_ops;
    u32 count_parse_ops;
    u32 count_cmp_ops;
    u32 count_alg_ops;
    u32 count_option_ops;
    u32 count_result_ops;
    u32 count_map_ops;
    u32 count_set_ops;
    u32 count_collection_ops;
    u32 count_pactum_decl;
    u32 count_pactum_signature;
    u32 count_sigillum_pactum_conformance;
    bool pactum_conformance_ok;

    char **generic_inst_keys;
    size_t generic_inst_key_count;
    size_t generic_inst_key_capacity;

    u64 hash;
} cct_sigilo_model_t;

typedef enum {
    SG_STYLE_NETWORK = 0,
    SG_STYLE_SEAL,
    SG_STYLE_SCRIPTUM
} sg_visual_style_t;

/* ============================== helpers ============================== */

static void* sg_calloc(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return p;
}

static char* sg_strdup(const char *s) {
    if (!s) return NULL;
    char *p = strdup(s);
    if (!p) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return p;
}

static char* sg_strdup_printf(const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return sg_strdup(buf);
}

static sg_visual_style_t sg_style_from_name(const char *name) {
    if (!name || strcmp(name, "network") == 0) return SG_STYLE_NETWORK;
    if (strcmp(name, "seal") == 0) return SG_STYLE_SEAL;
    if (strcmp(name, "scriptum") == 0) return SG_STYLE_SCRIPTUM;
    return SG_STYLE_NETWORK;
}

static const char* sg_style_name(sg_visual_style_t style) {
    switch (style) {
        case SG_STYLE_SEAL: return "seal";
        case SG_STYLE_SCRIPTUM: return "scriptum";
        case SG_STYLE_NETWORK:
        default: return "network";
    }
}

static void sg_report(cct_sigilo_t *sg, u32 line, u32 col, const char *msg) {
    sg->had_error = true;
    sg->error_count++;
    cct_error_at_location(CCT_ERROR_CODEGEN, sg->filename ? sg->filename : "<unknown>", line, col, msg);
}

static void sg_reportf(cct_sigilo_t *sg, u32 line, u32 col, const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    sg_report(sg, line, col, buf);
}

static bool sg_ends_with(const char *s, const char *suffix) {
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    return n >= m && strcmp(s + n - m, suffix) == 0;
}

static char* sg_derive_output_base(const char *input_path) {
    size_t len = strlen(input_path);
    if (sg_ends_with(input_path, ".cct")) {
        char *out = (char*)sg_calloc(1, len - 4 + 1);
        memcpy(out, input_path, len - 4);
        out[len - 4] = '\0';
        return out;
    }
    return sg_strdup(input_path);
}

static void sg_fnv_init(cct_sigilo_model_t *m) {
    m->hash = 1469598103934665603ULL;
}

static void sg_fnv_u8(cct_sigilo_model_t *m, u8 v) {
    m->hash ^= (u64)v;
    m->hash *= 1099511628211ULL;
}

static void sg_fnv_u32(cct_sigilo_model_t *m, u32 v) {
    for (int i = 0; i < 4; i++) sg_fnv_u8(m, (u8)((v >> (i * 8)) & 0xFF));
}

static void sg_fnv_u64(cct_sigilo_model_t *m, u64 v) {
    for (int i = 0; i < 8; i++) sg_fnv_u8(m, (u8)((v >> (i * 8)) & 0xFF));
}

static void sg_fnv_str(cct_sigilo_model_t *m, const char *s) {
    if (!s) {
        sg_fnv_u8(m, 0xFF);
        return;
    }
    while (*s) sg_fnv_u8(m, (u8)*s++);
    sg_fnv_u8(m, 0);
}

static void sg_hash_node_tag(cct_sigilo_model_t *m, cct_ast_node_type_t t) {
    sg_fnv_u8(m, 0xA5);
    sg_fnv_u32(m, (u32)t);
}

static int sg_find_ritual_index(const cct_sigilo_model_t *m, const char *name) {
    if (!name) return -1;
    for (u32 i = 0; i < m->ritual_count; i++) {
        if (m->rituals[i].name && strcmp(m->rituals[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static bool sg_program_has_sigillum_named(const cct_ast_program_t *program, const char *name) {
    if (!program || !program->declarations || !name) return false;
    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (decl && decl->type == AST_SIGILLUM && decl->as.sigillum.name &&
            strcmp(decl->as.sigillum.name, name) == 0) {
            return true;
        }
    }
    return false;
}

static void sg_record_call_edge(cct_sigilo_model_t *m, u32 from_idx, const char *target) {
    int to_idx = sg_find_ritual_index(m, target);
    if (to_idx < 0) return;

    for (u32 i = 0; i < m->edge_count; i++) {
        if (m->edges[i].from_idx == from_idx && m->edges[i].to_idx == (u32)to_idx) {
            m->edges[i].count++;
            return;
        }
    }
    if (m->edge_count >= CCT_SIGILO_MAX_EDGES) return;
    m->edges[m->edge_count].from_idx = from_idx;
    m->edges[m->edge_count].to_idx = (u32)to_idx;
    m->edges[m->edge_count].count = 1;
    m->edge_count++;
}

static void sg_hex_u64(u64 v, char out[17]) {
    static const char *hex = "0123456789abcdef";
    for (int i = 15; i >= 0; i--) {
        out[i] = hex[v & 0xF];
        v >>= 4;
    }
    out[16] = '\0';
}

static double sg_hash_unit(const cct_sigilo_model_t *m, u32 salt) {
    u64 x = m->hash ^ (0x9e3779b97f4a7c15ULL * (u64)(salt + 1));
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return (double)(x & 0xFFFFFFULL) / (double)0x1000000ULL;
}

static void sg_point_on_circle(double cx, double cy, double r, double angle, double *x, double *y) {
    *x = cx + cos(angle) * r;
    *y = cy + sin(angle) * r;
}

/* =========================== model extraction ======================== */

static void sg_walk_expr(cct_sigilo_model_t *m, const cct_ast_node_t *expr, u32 ritual_idx, u32 depth);
static void sg_walk_stmt(cct_sigilo_model_t *m, const cct_ast_node_t *stmt, u32 ritual_idx, u32 depth);
static void sg_tally_type(cct_sigilo_model_t *m, const cct_ast_type_t *type);
static char* sg_type_key_string(const cct_ast_type_t *type);

static void sg_touch_depth(cct_sigilo_model_t *m, u32 ritual_idx, u32 depth) {
    if (depth > m->max_nesting) m->max_nesting = depth;
    if (depth > m->rituals[ritual_idx].max_nesting) m->rituals[ritual_idx].max_nesting = depth;
    m->depth_accum += depth;
    m->depth_samples++;
}

static bool sg_generic_key_seen(const cct_sigilo_model_t *m, const char *key) {
    if (!m || !key) return false;
    for (size_t i = 0; i < m->generic_inst_key_count; i++) {
        if (m->generic_inst_keys[i] && strcmp(m->generic_inst_keys[i], key) == 0) return true;
    }
    return false;
}

static void sg_register_generic_instantiation(cct_sigilo_model_t *m, bool is_rituale, const char *key) {
    if (!m || !key) return;
    m->count_generic_instantiation++;
    if (is_rituale) m->count_generic_rituale_instantiation++;
    else m->count_generic_sigillum_instantiation++;
    sg_fnv_u8(m, is_rituale ? 0xC1 : 0xC2);
    sg_fnv_str(m, key);

    if (sg_generic_key_seen(m, key)) return;

    if (m->generic_inst_key_count >= m->generic_inst_key_capacity) {
        size_t new_cap = m->generic_inst_key_capacity ? m->generic_inst_key_capacity * 2 : 16;
        char **next = (char**)realloc(m->generic_inst_keys, new_cap * sizeof(*next));
        if (!next) return;
        m->generic_inst_keys = next;
        m->generic_inst_key_capacity = new_cap;
    }

    m->generic_inst_keys[m->generic_inst_key_count++] = sg_strdup(key);
    m->count_generic_instantiation_dedup++;
}

static char* sg_type_list_key_string(const cct_ast_type_list_t *list) {
    if (!list || list->count == 0) return sg_strdup("");
    char *acc = sg_strdup("");
    for (size_t i = 0; i < list->count; i++) {
        char *piece = sg_type_key_string(list->types[i]);
        char *next = sg_strdup_printf("%s%s%s", acc, (i > 0) ? "," : "", piece);
        free(acc);
        free(piece);
        acc = next;
    }
    return acc;
}

static char* sg_type_key_string(const cct_ast_type_t *type) {
    if (!type) return sg_strdup("?");
    if (type->is_pointer) {
        char *inner = sg_type_key_string(type->element_type);
        char *out = sg_strdup_printf("P(%s)", inner);
        free(inner);
        return out;
    }
    if (type->is_array) {
        char *inner = sg_type_key_string(type->element_type);
        char *out = sg_strdup_printf("A[%u](%s)", type->array_size, inner);
        free(inner);
        return out;
    }
    if (type->generic_args && type->generic_args->count > 0) {
        char *args = sg_type_list_key_string(type->generic_args);
        char *out = sg_strdup_printf("%s<%s>", type->name ? type->name : "?", args);
        free(args);
        return out;
    }
    return sg_strdup(type->name ? type->name : "?");
}

static void sg_tally_type(cct_sigilo_model_t *m, const cct_ast_type_t *type) {
    if (!m || !type) return;
    if (type->is_array) {
        m->count_series_type_use++;
        sg_fnv_u8(m, 0x5A);
        sg_fnv_u32(m, type->array_size);
        sg_tally_type(m, type->element_type);
        return;
    }
    if (type->is_pointer) {
        m->count_speculum_type_use++;
        sg_fnv_u8(m, 0x7E);
        sg_tally_type(m, type->element_type);
        return;
    }
    if (!type->name) return;
    sg_fnv_str(m, type->name);
    if (type->generic_args && type->generic_args->count > 0) {
        char *args = sg_type_list_key_string(type->generic_args);
        char *key = sg_strdup_printf("S|%s|%s", type->name, args);
        sg_register_generic_instantiation(m, false, key);
        free(key);
        free(args);
        for (size_t i = 0; i < type->generic_args->count; i++) {
            sg_tally_type(m, type->generic_args->types[i]);
        }
    }
    if (strcmp(type->name, "UMBRA") == 0) m->count_type_umbra++;
    if (strcmp(type->name, "FLAMMA") == 0) m->count_type_flamma++;
}

static void sg_walk_expr(cct_sigilo_model_t *m, const cct_ast_node_t *expr, u32 ritual_idx, u32 depth) {
    if (!expr) return;
    m->total_exprs++;
    sg_hash_node_tag(m, expr->type);
    sg_touch_depth(m, ritual_idx, depth);

    switch (expr->type) {
        case AST_LITERAL_INT:
            sg_fnv_u64(m, (u64)expr->as.literal_int.int_value);
            return;
        case AST_LITERAL_BOOL:
            sg_fnv_u8(m, expr->as.literal_bool.bool_value ? 1 : 0);
            return;
        case AST_LITERAL_STRING:
            sg_fnv_str(m, expr->as.literal_string.string_value);
            return;
        case AST_LITERAL_REAL:
            sg_fnv_u64(m, (u64)(expr->as.literal_real.real_value * 1000003.0));
            m->count_type_umbra++;
            return;
        case AST_IDENTIFIER:
            sg_fnv_str(m, expr->as.identifier.name);
            return;
        case AST_BINARY_OP:
            sg_fnv_u32(m, (u32)expr->as.binary_op.operator);
            sg_walk_expr(m, expr->as.binary_op.left, ritual_idx, depth + 1);
            sg_walk_expr(m, expr->as.binary_op.right, ritual_idx, depth + 1);
            return;
        case AST_UNARY_OP:
            sg_fnv_u32(m, (u32)expr->as.unary_op.operator);
            if (expr->as.unary_op.operator == TOKEN_SPECULUM) m->count_addr_of++;
            if (expr->as.unary_op.operator == TOKEN_STAR) m->count_deref++;
            sg_walk_expr(m, expr->as.unary_op.operand, ritual_idx, depth + 1);
            return;
        case AST_CONIURA:
            m->count_coniura++;
            m->total_calls++;
            m->rituals[ritual_idx].coniura_count++;
            sg_fnv_str(m, expr->as.coniura.name);
            if (expr->as.coniura.type_args && expr->as.coniura.type_args->count > 0) {
                char *args = sg_type_list_key_string(expr->as.coniura.type_args);
                char *key = sg_strdup_printf("R|%s|%s",
                                             expr->as.coniura.name ? expr->as.coniura.name : "",
                                             args);
                sg_register_generic_instantiation(m, true, key);
                free(key);
                free(args);
                for (size_t i = 0; i < expr->as.coniura.type_args->count; i++) {
                    sg_tally_type(m, expr->as.coniura.type_args->types[i]);
                }
            }
            sg_record_call_edge(m, ritual_idx, expr->as.coniura.name);
            if (expr->as.coniura.name) {
                if (strcmp(expr->as.coniura.name, "len") == 0 ||
                    strcmp(expr->as.coniura.name, "concat") == 0 ||
                    strcmp(expr->as.coniura.name, "compare") == 0 ||
                    strcmp(expr->as.coniura.name, "substring") == 0 ||
                    strcmp(expr->as.coniura.name, "trim") == 0 ||
                    strcmp(expr->as.coniura.name, "find") == 0 ||
                    strcmp(expr->as.coniura.name, "contains") == 0) {
                    m->count_verbum_ops++;
                }
                if (strcmp(expr->as.coniura.name, "stringify_int") == 0 ||
                    strcmp(expr->as.coniura.name, "stringify_real") == 0 ||
                    strcmp(expr->as.coniura.name, "stringify_float") == 0 ||
                    strcmp(expr->as.coniura.name, "stringify_bool") == 0 ||
                    strcmp(expr->as.coniura.name, "fmt_parse_int") == 0 ||
                    strcmp(expr->as.coniura.name, "fmt_parse_real") == 0 ||
                    strcmp(expr->as.coniura.name, "fmt_parse_bool") == 0 ||
                    strcmp(expr->as.coniura.name, "format_pair") == 0) {
                    m->count_fmt_ops++;
                }
                if (strncmp(expr->as.coniura.name, "series_", 7) == 0) {
                    m->count_series_ops++;
                }
                if (strcmp(expr->as.coniura.name, "alloc") == 0 ||
                    strcmp(expr->as.coniura.name, "free") == 0 ||
                    strcmp(expr->as.coniura.name, "realloc") == 0 ||
                    strcmp(expr->as.coniura.name, "copy") == 0 ||
                    strcmp(expr->as.coniura.name, "set") == 0 ||
                    strcmp(expr->as.coniura.name, "zero") == 0 ||
                    strcmp(expr->as.coniura.name, "mem_compare") == 0) {
                    m->count_mem_ops++;
                }
                if (strcmp(expr->as.coniura.name, "read_line") == 0) {
                    m->count_io_ops++;
                }
                if (strcmp(expr->as.coniura.name, "read_all") == 0 ||
                    strcmp(expr->as.coniura.name, "write_all") == 0 ||
                    strcmp(expr->as.coniura.name, "append_all") == 0 ||
                    strcmp(expr->as.coniura.name, "exists") == 0 ||
                    strcmp(expr->as.coniura.name, "size") == 0) {
                    m->count_fs_ops++;
                }
                if (strncmp(expr->as.coniura.name, "fluxus_", 7) == 0) {
                    m->count_fluxus_ops++;
                }
                if (strncmp(expr->as.coniura.name, "path_", 5) == 0) {
                    m->count_path_ops++;
                }
                if (strcmp(expr->as.coniura.name, "abs") == 0 ||
                    strcmp(expr->as.coniura.name, "min") == 0 ||
                    strcmp(expr->as.coniura.name, "max") == 0 ||
                    strcmp(expr->as.coniura.name, "clamp") == 0) {
                    m->count_math_ops++;
                }
                if (strncmp(expr->as.coniura.name, "random_", 7) == 0 ||
                    strcmp(expr->as.coniura.name, "seed") == 0) {
                    m->count_random_ops++;
                }
                if (strncmp(expr->as.coniura.name, "parse_", 6) == 0) {
                    m->count_parse_ops++;
                }
                if (strncmp(expr->as.coniura.name, "cmp_", 4) == 0) {
                    m->count_cmp_ops++;
                }
                if (strncmp(expr->as.coniura.name, "alg_", 4) == 0) {
                    m->count_alg_ops++;
                }
                if (strcmp(expr->as.coniura.name, "Some") == 0 ||
                    strcmp(expr->as.coniura.name, "None") == 0 ||
                    strncmp(expr->as.coniura.name, "option_", 7) == 0) {
                    m->count_option_ops++;
                }
                if (strcmp(expr->as.coniura.name, "Ok") == 0 ||
                    strcmp(expr->as.coniura.name, "Err") == 0 ||
                    strncmp(expr->as.coniura.name, "result_", 7) == 0) {
                    m->count_result_ops++;
                }
                if (strncmp(expr->as.coniura.name, "map_", 4) == 0) {
                    m->count_map_ops++;
                }
                if (strncmp(expr->as.coniura.name, "set_", 4) == 0) {
                    m->count_set_ops++;
                }
                if (strcmp(expr->as.coniura.name, "fluxus_map") == 0 ||
                    strcmp(expr->as.coniura.name, "fluxus_filter") == 0 ||
                    strcmp(expr->as.coniura.name, "fluxus_fold") == 0 ||
                    strcmp(expr->as.coniura.name, "fluxus_find") == 0 ||
                    strcmp(expr->as.coniura.name, "fluxus_any") == 0 ||
                    strcmp(expr->as.coniura.name, "fluxus_all") == 0 ||
                    strcmp(expr->as.coniura.name, "series_map") == 0 ||
                    strcmp(expr->as.coniura.name, "series_filter") == 0 ||
                    strcmp(expr->as.coniura.name, "series_reduce") == 0 ||
                    strcmp(expr->as.coniura.name, "series_find") == 0 ||
                    strcmp(expr->as.coniura.name, "series_any") == 0 ||
                    strcmp(expr->as.coniura.name, "series_all") == 0) {
                    m->count_collection_ops++;
                }
                if (strcmp(expr->as.coniura.name, "fluxus_init") == 0) {
                    m->count_fluxus_init++;
                    m->count_fluxus_instances++;
                } else if (strcmp(expr->as.coniura.name, "fluxus_push") == 0) {
                    m->count_fluxus_push++;
                } else if (strcmp(expr->as.coniura.name, "fluxus_pop") == 0) {
                    m->count_fluxus_pop++;
                }
            }
            if (expr->as.coniura.arguments) {
                for (size_t i = 0; i < expr->as.coniura.arguments->count; i++) {
                    sg_walk_expr(m, expr->as.coniura.arguments->nodes[i], ritual_idx, depth + 1);
                }
            }
            return;
        case AST_OBSECRO:
            m->count_obsecro++;
            m->total_obsecro++;
            m->rituals[ritual_idx].obsecro_count++;
            sg_fnv_str(m, expr->as.obsecro.name);
            if (expr->as.obsecro.name) {
                if (strcmp(expr->as.obsecro.name, "pete") == 0) m->count_alloc++;
                if (strcmp(expr->as.obsecro.name, "libera") == 0) m->count_free++;
                if (strncmp(expr->as.obsecro.name, "mem_", 4) == 0) m->count_mem_ops++;
                if (strncmp(expr->as.obsecro.name, "io_", 3) == 0) m->count_io_ops++;
                if (strncmp(expr->as.obsecro.name, "fs_", 3) == 0) m->count_fs_ops++;
                if (strncmp(expr->as.obsecro.name, "fmt_", 4) == 0) m->count_fmt_ops++;
                if (strncmp(expr->as.obsecro.name, "option_", 7) == 0) m->count_option_ops++;
                if (strncmp(expr->as.obsecro.name, "result_", 7) == 0) m->count_result_ops++;
                if (strncmp(expr->as.obsecro.name, "collection_", 11) == 0) m->count_collection_ops++;
            }
            if (expr->as.obsecro.arguments) {
                for (size_t i = 0; i < expr->as.obsecro.arguments->count; i++) {
                    sg_walk_expr(m, expr->as.obsecro.arguments->nodes[i], ritual_idx, depth + 1);
                }
            }
            return;
        case AST_CALL:
            if (expr->as.call.callee) sg_walk_expr(m, expr->as.call.callee, ritual_idx, depth + 1);
            if (expr->as.call.arguments) {
                for (size_t i = 0; i < expr->as.call.arguments->count; i++) {
                    sg_walk_expr(m, expr->as.call.arguments->nodes[i], ritual_idx, depth + 1);
                }
            }
            return;
        case AST_FIELD_ACCESS:
            m->count_field_access++;
            sg_walk_expr(m, expr->as.field_access.object, ritual_idx, depth + 1);
            sg_fnv_str(m, expr->as.field_access.field);
            return;
        case AST_INDEX_ACCESS:
            m->count_index_access++;
            sg_walk_expr(m, expr->as.index_access.array, ritual_idx, depth + 1);
            sg_walk_expr(m, expr->as.index_access.index, ritual_idx, depth + 1);
            return;
        case AST_MENSURA:
            if (expr->as.mensura.type && expr->as.mensura.type->name) sg_fnv_str(m, expr->as.mensura.type->name);
            return;
        default:
            return;
    }
}

static void sg_walk_block(cct_sigilo_model_t *m, const cct_ast_node_t *block, u32 ritual_idx, u32 depth) {
    if (!block || block->type != AST_BLOCK || !block->as.block.statements) return;
    for (size_t i = 0; i < block->as.block.statements->count; i++) {
        sg_walk_stmt(m, block->as.block.statements->nodes[i], ritual_idx, depth);
    }
}

static void sg_walk_stmt(cct_sigilo_model_t *m, const cct_ast_node_t *stmt, u32 ritual_idx, u32 depth) {
    if (!stmt) return;
    sg_touch_depth(m, ritual_idx, depth);

    if (stmt->type != AST_BLOCK) {
        m->total_statements++;
        m->rituals[ritual_idx].total_statements++;
    }
    sg_hash_node_tag(m, stmt->type);

    switch (stmt->type) {
        case AST_BLOCK:
            sg_walk_block(m, stmt, ritual_idx, depth + 1);
            return;
        case AST_EVOCA:
            m->count_evoca++;
            m->rituals[ritual_idx].evoca_count++;
            sg_fnv_str(m, stmt->as.evoca.name);
            sg_tally_type(m, stmt->as.evoca.var_type);
            sg_walk_expr(m, stmt->as.evoca.initializer, ritual_idx, depth + 1);
            return;
        case AST_VINCIRE:
            m->count_vincire++;
            m->rituals[ritual_idx].vincire_count++;
            sg_walk_expr(m, stmt->as.vincire.target, ritual_idx, depth + 1);
            sg_walk_expr(m, stmt->as.vincire.value, ritual_idx, depth + 1);
            return;
        case AST_REDDE:
            m->count_redde++;
            m->total_returns++;
            m->rituals[ritual_idx].redde_count++;
            sg_walk_expr(m, stmt->as.redde.value, ritual_idx, depth + 1);
            return;
        case AST_ANUR:
            m->count_anur++;
            m->total_anur++;
            m->rituals[ritual_idx].anur_count++;
            sg_walk_expr(m, stmt->as.anur.value, ritual_idx, depth + 1);
            return;
        case AST_DIMITTE:
            m->count_dimitte++;
            sg_walk_expr(m, stmt->as.dimitte.target, ritual_idx, depth + 1);
            return;
        case AST_EXPR_STMT:
            sg_walk_expr(m, stmt->as.expr_stmt.expression, ritual_idx, depth + 1);
            return;
        case AST_TEMPTA:
            m->count_tempta++;
            m->count_cape++;
            if (stmt->as.tempta.semper_block) m->count_semper++;
            sg_walk_stmt(m, stmt->as.tempta.try_block, ritual_idx, depth + 1);
            sg_tally_type(m, stmt->as.tempta.cape_type);
            sg_fnv_str(m, stmt->as.tempta.cape_name);
            sg_walk_stmt(m, stmt->as.tempta.cape_block, ritual_idx, depth + 1);
            if (stmt->as.tempta.semper_block) sg_walk_stmt(m, stmt->as.tempta.semper_block, ritual_idx, depth + 1);
            return;
        case AST_IACE:
            m->count_iace++;
            sg_walk_expr(m, stmt->as.iace.value, ritual_idx, depth + 1);
            return;
        case AST_SI:
            m->count_si++;
            m->total_conditionals++;
            m->rituals[ritual_idx].si_count++;
            sg_walk_expr(m, stmt->as.si.condition, ritual_idx, depth + 1);
            sg_walk_stmt(m, stmt->as.si.then_branch, ritual_idx, depth + 1);
            if (stmt->as.si.else_branch) {
                m->count_aliter++;
                m->rituals[ritual_idx].aliter_count++;
                sg_walk_stmt(m, stmt->as.si.else_branch, ritual_idx, depth + 1);
            }
            return;
        case AST_DUM:
            m->count_dum++;
            m->total_loops++;
            m->rituals[ritual_idx].dum_count++;
            sg_walk_expr(m, stmt->as.dum.condition, ritual_idx, depth + 1);
            sg_walk_stmt(m, stmt->as.dum.body, ritual_idx, depth + 1);
            return;
        case AST_DONEC:
            m->count_donec++;
            m->total_loops++;
            m->rituals[ritual_idx].donec_count++;
            sg_walk_stmt(m, stmt->as.donec.body, ritual_idx, depth + 1);
            sg_walk_expr(m, stmt->as.donec.condition, ritual_idx, depth + 1);
            return;
        case AST_REPETE:
            m->count_repete++;
            m->total_loops++;
            m->rituals[ritual_idx].repete_count++;
            sg_fnv_str(m, stmt->as.repete.iterator);
            sg_walk_expr(m, stmt->as.repete.start, ritual_idx, depth + 1);
            sg_walk_expr(m, stmt->as.repete.end, ritual_idx, depth + 1);
            sg_walk_expr(m, stmt->as.repete.step, ritual_idx, depth + 1);
            sg_walk_stmt(m, stmt->as.repete.body, ritual_idx, depth + 1);
            return;
        case AST_ITERUM:
            m->count_iterum++;
            m->total_loops++;
            m->rituals[ritual_idx].iterum_count++;
            sg_fnv_str(m, stmt->as.iterum.item_name);
            sg_walk_expr(m, stmt->as.iterum.collection, ritual_idx, depth + 1);
            sg_walk_stmt(m, stmt->as.iterum.body, ritual_idx, depth + 1);
            return;
        case AST_FRANGE:
        case AST_RECEDE:
        case AST_TRANSITUS:
            return;
        default:
            return;
    }
}

static bool sg_extract_model(cct_sigilo_t *sg, const cct_ast_program_t *program, cct_sigilo_model_t *m) {
    memset(m, 0, sizeof(*m));
    m->pactum_conformance_ok = true;
    m->pactum_constraint_resolution_ok = true;
    m->program = program;
    m->program_name = (program && program->name) ? program->name : "anonymous";
    m->entry_idx = -1;
    sg_fnv_init(m);
    sg_fnv_str(m, "CCT_SIGILLO_V1");
    sg_fnv_str(m, m->program_name);

    if (!program || !program->declarations) {
        sg_reportf(sg, 0, 0, "invalid AST program for sigilo generation");
        return false;
    }

    for (size_t i = 0; i < program->declarations->count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (decl && decl->type == AST_RITUALE) m->ritual_count++;
    }
    if (m->ritual_count > CCT_SIGILO_MAX_RITUAIS) m->ritual_count = CCT_SIGILO_MAX_RITUAIS;

    m->rituals = (cct_sigilo_ritual_metric_t*)sg_calloc(m->ritual_count ? m->ritual_count : 1, sizeof(*m->rituals));
    m->edges = (cct_sigilo_call_edge_t*)sg_calloc(CCT_SIGILO_MAX_EDGES, sizeof(*m->edges));

    u32 r = 0;
    for (size_t i = 0; i < program->declarations->count && r < m->ritual_count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl) continue;
        sg_hash_node_tag(m, decl->type);
        if (decl->type != AST_RITUALE) {
            if (decl->type == AST_SIGILLUM) {
                m->count_sigillum_decl++;
                sg_fnv_str(m, decl->as.sigillum.name);
                if (decl->as.sigillum.pactum_name && decl->as.sigillum.pactum_name[0]) {
                    m->count_sigillum_pactum_conformance++;
                    sg_fnv_u8(m, 0xE1);
                    sg_fnv_str(m, decl->as.sigillum.pactum_name);
                }
                if (decl->as.sigillum.type_params && decl->as.sigillum.type_params->count > 0) {
                    m->count_generic_decl++;
                    m->count_generic_sigillum_decl++;
                    m->count_generic_params += (u32)decl->as.sigillum.type_params->count;
                    sg_fnv_u8(m, 0xDA);
                    sg_fnv_u32(m, (u32)decl->as.sigillum.type_params->count);
                    for (size_t gi = 0; gi < decl->as.sigillum.type_params->count; gi++) {
                        cct_ast_type_param_t *tp = decl->as.sigillum.type_params->params[gi];
                        if (tp && tp->name) sg_fnv_str(m, tp->name);
                    }
                }
                if (decl->as.sigillum.fields) {
                    for (size_t fi = 0; fi < decl->as.sigillum.fields->count; fi++) {
                        cct_ast_field_t *f = decl->as.sigillum.fields->fields[fi];
                        if (!f) continue;
                        sg_fnv_str(m, f->name);
                        sg_tally_type(m, f->type);
                        if (f->type && !f->type->is_pointer && !f->type->is_array && f->type->name &&
                            sg_program_has_sigillum_named(program, f->type->name)) {
                            m->count_sigillum_composite_field++;
                        }
                    }
                }
            } else if (decl->type == AST_ORDO) {
                m->count_ordo_decl++;
                sg_fnv_str(m, decl->as.ordo.name);
                if (decl->as.ordo.items) {
                    for (size_t ei = 0; ei < decl->as.ordo.items->count; ei++) {
                        cct_ast_enum_item_t *it = decl->as.ordo.items->items[ei];
                        if (!it) continue;
                        sg_fnv_str(m, it->name);
                        sg_fnv_u64(m, (u64)it->value);
                    }
                }
            } else if (decl->type == AST_PACTUM) {
                m->count_pactum_decl++;
                sg_fnv_str(m, decl->as.pactum.name);
                if (decl->as.pactum.signatures) {
                    m->count_pactum_signature += (u32)decl->as.pactum.signatures->count;
                    for (size_t si = 0; si < decl->as.pactum.signatures->count; si++) {
                        const cct_ast_node_t *sig = decl->as.pactum.signatures->nodes[si];
                        if (!sig || sig->type != AST_RITUALE) continue;
                        sg_fnv_str(m, sig->as.rituale.name);
                        if (sig->as.rituale.params) {
                            sg_fnv_u32(m, (u32)sig->as.rituale.params->count);
                            for (size_t pi = 0; pi < sig->as.rituale.params->count; pi++) {
                                cct_ast_param_t *p = sig->as.rituale.params->params[pi];
                                if (!p) continue;
                                sg_fnv_str(m, p->name);
                                sg_tally_type(m, p->type);
                            }
                        } else {
                            sg_fnv_u32(m, 0);
                        }
                        sg_tally_type(m, sig->as.rituale.return_type);
                    }
                }
            } else if (decl->type == AST_CODEX) {
                /* Non-executable structure still contributes a tag to the hash only. */
            }
            continue;
        }

        m->rituals[r].name = sg_strdup(decl->as.rituale.name ? decl->as.rituale.name : "rituale");
        m->rituals[r].is_entry = (decl->as.rituale.name &&
                                (strcmp(decl->as.rituale.name, "principium") == 0 || strcmp(decl->as.rituale.name, "main") == 0));
        if (m->rituals[r].is_entry) {
            if (m->entry_idx < 0 || strcmp(m->rituals[r].name, "principium") == 0) m->entry_idx = (i32)r;
        }
        sg_fnv_str(m, m->rituals[r].name);
        if (decl->as.rituale.type_params && decl->as.rituale.type_params->count > 0) {
            m->count_generic_decl++;
            m->count_generic_rituale_decl++;
            m->count_generic_params += (u32)decl->as.rituale.type_params->count;
            sg_fnv_u8(m, 0xDB);
            sg_fnv_u32(m, (u32)decl->as.rituale.type_params->count);
            for (size_t gi = 0; gi < decl->as.rituale.type_params->count; gi++) {
                cct_ast_type_param_t *tp = decl->as.rituale.type_params->params[gi];
                if (tp && tp->name) sg_fnv_str(m, tp->name);
                if (tp && tp->constraint_pactum_name && tp->constraint_pactum_name[0]) {
                    m->count_generic_constraint++;
                    m->count_generic_constrained_param++;
                    sg_fnv_u8(m, 0xDC);
                    sg_fnv_str(m, tp->constraint_pactum_name);
                }
            }
        }
        if (decl->as.rituale.params) sg_fnv_u32(m, (u32)decl->as.rituale.params->count);

        if (decl->as.rituale.body && decl->as.rituale.body->type == AST_BLOCK && decl->as.rituale.body->as.block.statements) {
            m->rituals[r].direct_statements = (u32)decl->as.rituale.body->as.block.statements->count;
        }
        r++;
    }

    r = 0;
    for (size_t i = 0; i < program->declarations->count && r < m->ritual_count; i++) {
        const cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!decl || decl->type != AST_RITUALE) continue;
        if (decl->as.rituale.body) sg_walk_stmt(m, decl->as.rituale.body, r, 0);
        r++;
    }

    if (m->entry_idx < 0 && m->ritual_count > 0) m->entry_idx = 0;
    sg_hex_u64(m->hash, sg->semantic_hash_hex);
    sg->semantic_hash = m->hash;
    sg->ritual_count = m->ritual_count;
    sg->total_statements = m->total_statements;
    sg->max_nesting = m->max_nesting;
    return true;
}

static void sg_free_model(cct_sigilo_model_t *m) {
    if (!m) return;
    if (m->rituals) {
        for (u32 i = 0; i < m->ritual_count; i++) free(m->rituals[i].name);
    }
    if (m->generic_inst_keys) {
        for (size_t i = 0; i < m->generic_inst_key_count; i++) {
            free(m->generic_inst_keys[i]);
        }
    }
    free(m->rituals);
    free(m->edges);
    free(m->generic_inst_keys);
    memset(m, 0, sizeof(*m));
}

/* ============================== SVG (FASE 5B / v2) ================== */

typedef enum {
    SGN_RITUAL = 1,
    SGN_DECISION,
    SGN_ALITER,
    SGN_DUM,
    SGN_DONEC,
    SGN_REPETE,
    SGN_BINDING,
    SGN_RETURN,
    SGN_ANUR
} sg_geom_node_kind_t;

typedef enum {
    SGL_PRIMARY = 1,
    SGL_CONIURA,
    SGL_BRANCH,
    SGL_BINDING,
    SGL_RETURN,
    SGL_LOOP,
    SGL_TERMINAL
} sg_geom_link_kind_t;

typedef struct {
    double x;
    double y;
    double r;
    u32 ritual_idx;
    u32 kind;
    u32 strength;
    bool is_entry;
} cct_sigilo_node_t;

typedef struct {
    u32 from_idx;
    u32 to_idx;
    u32 kind;
    u32 weight;
    bool self_loop;
} cct_sigilo_link_t;

typedef struct {
    cct_sigilo_node_t *nodes;
    u32 node_count;
    u32 node_cap;

    cct_sigilo_link_t *links;
    u32 link_count;
    u32 link_cap;

    i32 *ritual_nodes;
    i32 *decision_nodes;
    i32 *aliter_nodes;
    i32 *dum_nodes;
    i32 *donec_nodes;
    i32 *repete_nodes;
    i32 *binding_nodes;
    i32 *return_nodes;
    i32 *anur_nodes;
    i32 hub_node;

    u32 *primary_order;
    u32 primary_count;
} cct_sigilo_geom_t;

static double sg_clamp(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static double sg_ritual_complexity(const cct_sigilo_ritual_metric_t *r) {
    if (!r) return 1.0;
    return 1.0
        + (double)r->total_statements * 0.09
        + (double)r->coniura_count * 0.45
        + (double)(r->dum_count + r->donec_count + r->repete_count + r->iterum_count) * 0.55
        + (double)r->si_count * 0.35
        + (double)r->vincire_count * 0.12
        + (double)r->max_nesting * 0.22;
}

static void sg_norm(double *x, double *y) {
    double d = sqrt((*x) * (*x) + (*y) * (*y));
    if (d < 1e-6) {
        *x = 1.0;
        *y = 0.0;
        return;
    }
    *x /= d;
    *y /= d;
}

static void sg_geom_init(cct_sigilo_geom_t *g, u32 ritual_count) {
    memset(g, 0, sizeof(*g));
    g->node_cap = 64 + ritual_count * 12;
    g->link_cap = 96 + ritual_count * 20;
    g->nodes = (cct_sigilo_node_t*)sg_calloc(g->node_cap, sizeof(*g->nodes));
    g->links = (cct_sigilo_link_t*)sg_calloc(g->link_cap, sizeof(*g->links));
    g->primary_order = (u32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(*g->primary_order));

    g->ritual_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->decision_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->aliter_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->dum_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->donec_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->repete_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->binding_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->return_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));
    g->anur_nodes = (i32*)sg_calloc((ritual_count ? ritual_count : 1), sizeof(i32));

    for (u32 i = 0; i < (ritual_count ? ritual_count : 1); i++) {
        g->ritual_nodes[i] = -1;
        g->decision_nodes[i] = -1;
        g->aliter_nodes[i] = -1;
        g->dum_nodes[i] = -1;
        g->donec_nodes[i] = -1;
        g->repete_nodes[i] = -1;
        g->binding_nodes[i] = -1;
        g->return_nodes[i] = -1;
        g->anur_nodes[i] = -1;
    }
    g->hub_node = -1;
}

static void sg_geom_dispose(cct_sigilo_geom_t *g) {
    if (!g) return;
    free(g->nodes);
    free(g->links);
    free(g->ritual_nodes);
    free(g->decision_nodes);
    free(g->aliter_nodes);
    free(g->dum_nodes);
    free(g->donec_nodes);
    free(g->repete_nodes);
    free(g->binding_nodes);
    free(g->return_nodes);
    free(g->anur_nodes);
    free(g->primary_order);
    memset(g, 0, sizeof(*g));
}

static u32 sg_geom_add_node(cct_sigilo_geom_t *g, const cct_sigilo_node_t *n) {
    if (g->node_count >= g->node_cap) return 0;
    g->nodes[g->node_count] = *n;
    return g->node_count++;
}

static void sg_geom_add_link(cct_sigilo_geom_t *g, u32 from_idx, u32 to_idx, u32 kind, u32 weight, bool self_loop) {
    if (g->link_count >= g->link_cap) return;
    g->links[g->link_count].from_idx = from_idx;
    g->links[g->link_count].to_idx = to_idx;
    g->links[g->link_count].kind = kind;
    g->links[g->link_count].weight = weight ? weight : 1;
    g->links[g->link_count].self_loop = self_loop;
    g->link_count++;
}

static double sg_ritual_angle(const cct_sigilo_model_t *m, u32 idx) {
    if (m->ritual_count == 0) return -CCT_SIGILO_PI / 2.0;

    double total_weight = 0.0;
    for (u32 i = 0; i < m->ritual_count; i++) {
        total_weight += sg_ritual_complexity(&m->rituals[i]);
    }
    if (total_weight < 1e-6) total_weight = (double)m->ritual_count;

    double prior = 0.0;
    for (u32 i = 0; i < idx; i++) prior += sg_ritual_complexity(&m->rituals[i]);
    double own = sg_ritual_complexity(&m->rituals[idx]);
    double frac = (prior + own * 0.5) / total_weight;

    /* Entry ritual tends toward the upper-left / top region. */
    double anchor = (m->entry_idx >= 0) ? (double)m->entry_idx / (double)m->ritual_count : 0.0;
    double base = -CCT_SIGILO_PI / 2.2;
    double slot = (2.0 * CCT_SIGILO_PI) * (frac - anchor);
    double jitter = (sg_hash_unit(m, 70 + idx) - 0.5) * (CCT_SIGILO_PI / 18.0);
    return base + slot + jitter;
}

static void sg_geom_add_aux_node(
    cct_sigilo_geom_t *g,
    i32 *slot,
    u32 ritual_idx,
    u32 kind,
    u32 strength,
    bool is_entry,
    double bx,
    double by,
    double ux,
    double uy,
    double tx,
    double ty,
    double off_r,
    double off_t,
    double radius
) {
    cct_sigilo_node_t n;
    n.x = bx + ux * off_r + tx * off_t;
    n.y = by + uy * off_r + ty * off_t;
    n.r = radius;
    n.ritual_idx = ritual_idx;
    n.kind = kind;
    n.strength = strength;
    n.is_entry = is_entry;
    *slot = (i32)sg_geom_add_node(g, &n);
}

static void sg_build_geom(const cct_sigilo_model_t *m, cct_sigilo_geom_t *g) {
    sg_geom_init(g, m->ritual_count);

    if (m->ritual_count == 0) {
        cct_sigilo_node_t n;
        n.x = CCT_SIGILO_CENTER_X;
        n.y = CCT_SIGILO_CENTER_Y;
        n.r = 8.0;
        n.ritual_idx = 0;
        n.kind = SGN_RITUAL;
        n.strength = 1;
        n.is_entry = true;
        sg_geom_add_node(g, &n);
        g->primary_order[g->primary_count++] = 0;
        return;
    }

    double y_scale = sg_clamp(0.78 + ((double)m->total_loops * 0.01), 0.78, 1.02);
    double base_rx = 150.0 + sg_clamp((double)m->total_calls * 2.2, 0.0, 24.0);
    double base_ry = base_rx * y_scale;

    for (u32 i = 0; i < m->ritual_count; i++) {
        const cct_sigilo_ritual_metric_t *r = &m->rituals[i];
        double a = sg_ritual_angle(m, i);
        double complexity = sg_ritual_complexity(r);
        double radial_jitter = (sg_hash_unit(m, 90 + i) - 0.5) * 20.0;
        double rx = base_rx + sg_clamp((complexity - 1.0) * 2.6, -8.0, 22.0) + radial_jitter;
        double ry = base_ry + sg_clamp((double)r->max_nesting * 3.5, 0.0, 18.0) - radial_jitter * 0.15;
        double x = CCT_SIGILO_CENTER_X + cos(a) * rx;
        double y = CCT_SIGILO_CENTER_Y + sin(a) * ry;
        y += (sg_hash_unit(m, 130 + i) - 0.5) * 12.0;

        cct_sigilo_node_t ritual_node;
        ritual_node.x = x;
        ritual_node.y = y;
        ritual_node.r = 5.8 + sg_clamp((double)r->total_statements * 0.15, 0.0, 5.0)
            + (r->is_entry ? 2.0 : 0.0);
        ritual_node.ritual_idx = i;
        ritual_node.kind = SGN_RITUAL;
        ritual_node.strength = 1 + r->total_statements;
        ritual_node.is_entry = r->is_entry;
        g->ritual_nodes[i] = (i32)sg_geom_add_node(g, &ritual_node);

        double ux = x - CCT_SIGILO_CENTER_X;
        double uy = y - CCT_SIGILO_CENTER_Y;
        sg_norm(&ux, &uy);
        double tx = -uy;
        double ty = ux;
        double sign = (sg_hash_unit(m, 160 + i) > 0.5) ? 1.0 : -1.0;

        if (r->si_count > 0) {
            sg_geom_add_aux_node(
                g, &g->decision_nodes[i], i, SGN_DECISION, r->si_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                -34.0 - (double)r->si_count * 1.5, 24.0 * sign, 3.8 + sg_clamp((double)r->si_count, 0.0, 2.0));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->decision_nodes[i], SGL_BRANCH, 1 + r->si_count, false);
            if (r->aliter_count > 0) {
                sg_geom_add_aux_node(
                    g, &g->aliter_nodes[i], i, SGN_ALITER, r->aliter_count, r->is_entry,
                    x, y, ux, uy, tx, ty,
                    -18.0, -34.0 * sign, 3.2 + sg_clamp((double)r->aliter_count, 0.0, 2.0));
                sg_geom_add_link(g, (u32)g->decision_nodes[i], (u32)g->aliter_nodes[i], SGL_BRANCH, 1 + r->aliter_count, false);
                sg_geom_add_link(g, (u32)g->aliter_nodes[i], (u32)g->ritual_nodes[i], SGL_BRANCH, 1, false);
            }
        }

        if (r->dum_count > 0) {
            sg_geom_add_aux_node(
                g, &g->dum_nodes[i], i, SGN_DUM, r->dum_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                24.0, 28.0 * sign, 4.0 + sg_clamp((double)r->dum_count * 0.7, 0.0, 2.0));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->dum_nodes[i], SGL_LOOP, 1 + r->dum_count, false);
            sg_geom_add_link(g, (u32)g->dum_nodes[i], (u32)g->dum_nodes[i], SGL_LOOP, 1 + r->dum_count, true);
        }

        if (r->donec_count > 0) {
            sg_geom_add_aux_node(
                g, &g->donec_nodes[i], i, SGN_DONEC, r->donec_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                28.0, -26.0 * sign, 4.0 + sg_clamp((double)r->donec_count * 0.6, 0.0, 1.8));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->donec_nodes[i], SGL_LOOP, 1 + r->donec_count, false);
            sg_geom_add_link(g, (u32)g->donec_nodes[i], (u32)g->donec_nodes[i], SGL_LOOP, 1 + r->donec_count, true);
        }

        if (r->repete_count > 0) {
            sg_geom_add_aux_node(
                g, &g->repete_nodes[i], i, SGN_REPETE, r->repete_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                42.0 + (double)(r->repete_count > 4 ? 4 : r->repete_count) * 2.0, 0.0,
                3.3 + sg_clamp((double)r->repete_count * 0.3, 0.0, 1.2));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->repete_nodes[i], SGL_LOOP, 1 + r->repete_count, false);
        }

        if (r->evoca_count > 0 || r->vincire_count > 0 || r->obsecro_count > 0) {
            u32 strength = r->evoca_count + r->vincire_count + r->obsecro_count;
            sg_geom_add_aux_node(
                g, &g->binding_nodes[i], i, SGN_BINDING, strength, r->is_entry,
                x, y, ux, uy, tx, ty,
                -26.0, 0.0,
                2.8 + sg_clamp((double)strength * 0.15, 0.0, 2.0));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->binding_nodes[i], SGL_BINDING, strength, false);
        }

        if (r->redde_count > 0) {
            sg_geom_add_aux_node(
                g, &g->return_nodes[i], i, SGN_RETURN, r->redde_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                -56.0, -8.0 * sign, 3.2 + sg_clamp((double)r->redde_count * 0.4, 0.0, 1.4));
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->return_nodes[i], SGL_RETURN, 1 + r->redde_count, false);
        }

        if (r->anur_count > 0) {
            sg_geom_add_aux_node(
                g, &g->anur_nodes[i], i, SGN_ANUR, r->anur_count, r->is_entry,
                x, y, ux, uy, tx, ty,
                56.0, 12.0 * sign, 2.9);
            sg_geom_add_link(g, (u32)g->ritual_nodes[i], (u32)g->anur_nodes[i], SGL_TERMINAL, 1 + r->anur_count, false);
        }
    }

    /* Primary path order starts at entry ritual and walks declaration order. */
    u32 start = (m->entry_idx >= 0 && (u32)m->entry_idx < m->ritual_count) ? (u32)m->entry_idx : 0;
    for (u32 k = 0; k < m->ritual_count; k++) {
        u32 idx = (start + k) % m->ritual_count;
        if (g->ritual_nodes[idx] >= 0) {
            g->primary_order[g->primary_count++] = (u32)g->ritual_nodes[idx];
        }
    }
    for (u32 i = 1; i < g->primary_count; i++) {
        sg_geom_add_link(g, g->primary_order[i - 1], g->primary_order[i], SGL_PRIMARY, 1, false);
    }

    /* FASE 6A refinement: structural hub anchor for multi-ritual/call-dense grimoires. */
    if ((m->ritual_count >= 2 && (m->total_calls > 0 || m->total_conditionals > 0 || m->total_loops > 0)) &&
        g->node_count + 1 < g->node_cap) {
        cct_sigilo_node_t hub;
        hub.x = CCT_SIGILO_CENTER_X + (sg_hash_unit(m, 1710) - 0.5) * 32.0;
        hub.y = CCT_SIGILO_CENTER_Y + (sg_hash_unit(m, 1711) - 0.5) * 28.0;
        hub.r = 3.2 + sg_clamp((double)m->total_calls * 0.12, 0.0, 1.8);
        hub.ritual_idx = (m->entry_idx >= 0) ? (u32)m->entry_idx : 0;
        hub.kind = SGN_BINDING;
        hub.strength = 1 + m->total_calls + m->total_conditionals + m->total_loops;
        hub.is_entry = false;
        g->hub_node = (i32)sg_geom_add_node(g, &hub);

        if (g->hub_node >= 0) {
            for (u32 i = 0; i < m->ritual_count; i++) {
                if (g->ritual_nodes[i] < 0) continue;
                const cct_sigilo_ritual_metric_t *r = &m->rituals[i];
                u32 w = 1 + r->coniura_count + r->si_count + r->dum_count + r->donec_count + r->repete_count + r->iterum_count;
                if (r->is_entry) w += 1;
                if (w > 1 || i == (u32)((m->entry_idx >= 0) ? m->entry_idx : 0)) {
                    sg_geom_add_link(g, (u32)g->hub_node, (u32)g->ritual_nodes[i], SGL_BRANCH, w, false);
                }
            }
        }
    }

    /* Strong CONIURA links between ritual poles. */
    for (u32 i = 0; i < m->edge_count; i++) {
        const cct_sigilo_call_edge_t *e = &m->edges[i];
        if (e->from_idx >= m->ritual_count || e->to_idx >= m->ritual_count) continue;
        if (g->ritual_nodes[e->from_idx] < 0 || g->ritual_nodes[e->to_idx] < 0) continue;
        sg_geom_add_link(
            g,
            (u32)g->ritual_nodes[e->from_idx],
            (u32)g->ritual_nodes[e->to_idx],
            SGL_CONIURA,
            e->count,
            e->from_idx == e->to_idx
        );
    }
}

static void sg_svg_header(FILE *f, sg_visual_style_t style) {
    const char *bg = "#f5f0e4";
    const char *base = "#2a221c";
    const char *primary = "#241913";
    const char *call = "#5b2a22";
    const char *loop = "#2d5f5a";
    const char *orn = "#355d58";
    double base_w = 1.6;
    double primary_w = 2.8;
    if (style == SG_STYLE_SEAL) {
        bg = "#f2ecdf";
        base = "#221b16";
        primary = "#1f1712";
        call = "#4c231d";
        loop = "#274b47";
        orn = "#5a5f34";
        base_w = 1.9;
        primary_w = 3.0;
    } else if (style == SG_STYLE_SCRIPTUM) {
        bg = "#f7f2e9";
        base = "#40352d";
        primary = "#2d1f18";
        call = "#643126";
        loop = "#336862";
        orn = "#406f69";
        base_w = 1.35;
        primary_w = 2.45;
    }
    fprintf(f,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\" width=\"512\" height=\"512\">\n"
            "  <defs>\n"
            "    <style><![CDATA[\n"
            "      .bg { fill: %s; }\n"
            "      .base { stroke: %s; stroke-width: %.2f; fill: none; opacity: 0.78; }\n"
            "      .primary { stroke: %s; stroke-width: %.2f; fill: none; stroke-linecap: round; stroke-linejoin: round; }\n"
            "      .call { stroke: %s; stroke-width: 2.0; fill: none; stroke-linecap: round; opacity: 0.9; }\n"
            "      .branch { stroke: #6b342d; stroke-width: 1.6; fill: none; stroke-linecap: round; }\n"
            "      .loop { stroke: %s; stroke-width: 1.6; fill: none; stroke-linecap: round; }\n"
            "      .bind { stroke: #73493d; stroke-width: 1.4; fill: none; stroke-linecap: round; }\n"
            "      .term { stroke: #201814; stroke-width: 1.7; fill: none; stroke-linecap: round; }\n"
            "      .node-main { fill: #fffaf1; stroke: #201814; stroke-width: 2.0; }\n"
            "      .node-entry { fill: #efe6d4; stroke: #201814; stroke-width: 2.4; }\n"
            "      .node-aux { fill: #f6efe0; stroke: #4f4339; stroke-width: 1.3; }\n"
            "      .node-loop { fill: #edf3f1; stroke: %s; stroke-width: 1.4; }\n"
            "      .node-term { fill: #f3e7e3; stroke: #5a2a23; stroke-width: 1.4; }\n"
            "      .orn { stroke: %s; stroke-width: 1.0; fill: none; opacity: 0.55; }\n"
            "      .hash { fill: #1f1a17; font: 10px monospace; letter-spacing: 0.8px; }\n"
            "      .label { fill: #1f1a17; font: 8px monospace; opacity: 0.7; }\n"
            "    ]]></style>\n"
            "  </defs>\n"
            "  <rect class=\"bg\" x=\"0\" y=\"0\" width=\"512\" height=\"512\"/>\n",
            bg, base, base_w, primary, primary_w, call, loop, loop, orn);
}

static void sg_svg_layer_foundation(FILE *f, const cct_sigilo_model_t *m, sg_visual_style_t style) {
    bool second_ring = (m->ritual_count >= 5) || (m->total_statements >= 28) || (m->total_calls >= 6);
    double off = (sg_hash_unit(m, 610) - 0.5) * 7.0;
    fprintf(f, "  <g id=\"foundation\">\n");
    fprintf(f, "    <circle class=\"base\" cx=\"256\" cy=\"256\" r=\"226\"/>\n");
    if (second_ring) {
        double ry = 184.0 + off;
        if (style == SG_STYLE_SEAL) ry = 190.0 + off * 0.6;
        if (style == SG_STYLE_SCRIPTUM) ry = 178.0 + off * 1.2;
        fprintf(f, "    <ellipse class=\"base\" cx=\"256\" cy=\"256\" rx=\"198\" ry=\"%.2f\" opacity=\"0.32\"/>\n", ry);
    }
    if (style == SG_STYLE_SEAL) {
        fprintf(f, "    <path class=\"base\" d=\"M 96 256 Q 256 104 416 256 Q 256 408 96 256\" opacity=\"0.20\"/>\n");
    } else if (style == SG_STYLE_SCRIPTUM) {
        fprintf(f, "    <path class=\"orn\" d=\"M 108 286 Q 196 198 256 254 Q 314 306 408 224\" opacity=\"0.22\"/>\n");
    }
    if (m->ritual_count == 0) {
        fprintf(f, "    <circle class=\"orn\" cx=\"256\" cy=\"256\" r=\"18\" opacity=\"0.45\"/>\n");
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_emit_curved_link(
    FILE *f,
    const cct_sigilo_model_t *m,
    sg_visual_style_t style,
    const cct_sigilo_node_t *a,
    const cct_sigilo_node_t *b,
    const char *klass,
    double bend,
    double width,
    double opacity
) {
    double mx = (a->x + b->x) * 0.5;
    double my = (a->y + b->y) * 0.5;
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    double nx = -dy, ny = dx;
    sg_norm(&nx, &ny);

    double jitter = (sg_hash_unit(m, (u32)(1000 + (u32)a->ritual_idx * 17 + (u32)b->ritual_idx * 13)) - 0.5) * 8.0;
    if (style == SG_STYLE_SCRIPTUM) {
        bend *= 1.35;
        jitter *= 1.45;
        width *= 0.95;
    } else if (style == SG_STYLE_SEAL) {
        bend *= 0.75;
        jitter *= 0.55;
        width *= 1.08;
        opacity = sg_clamp(opacity + 0.04, 0.0, 1.0);
    }
    double cx = mx + nx * bend + nx * jitter;
    double cy = my + ny * bend + ny * jitter;
    fprintf(f,
            "    <path class=\"%s\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\" stroke-width=\"%.2f\" opacity=\"%.2f\"/>\n",
            klass, a->x, a->y, cx, cy, b->x, b->y, width, opacity);
}

static void sg_svg_emit_self_loop(
    FILE *f,
    const cct_sigilo_model_t *m,
    sg_visual_style_t style,
    const cct_sigilo_node_t *n,
    const char *klass,
    double scale,
    bool with_tail
) {
    double a = (sg_hash_unit(m, 1200 + n->ritual_idx * 3 + n->kind) - 0.5) * 1.2;
    if (style == SG_STYLE_SEAL) scale *= 0.92;
    if (style == SG_STYLE_SCRIPTUM) scale *= 1.12;
    double rx = (12.0 + n->r * 1.6) * scale;
    double ry = (8.0 + n->r * 1.2) * scale;
    double cx1 = n->x + cos(a) * (rx + n->r + 4.0);
    double cy1 = n->y + sin(a) * (ry + n->r + 4.0);
    fprintf(f,
            "    <path class=\"%s\" d=\"M %.2f %.2f C %.2f %.2f %.2f %.2f %.2f %.2f C %.2f %.2f %.2f %.2f %.2f %.2f\"/>\n",
            klass,
            n->x + n->r * 0.8, n->y,
            cx1 + rx * 0.6, cy1 - ry * 0.8,
            cx1 + rx * 0.8, cy1 + ry * 0.8,
            cx1, cy1,
            cx1 - rx * 0.8, cy1 + ry * 0.8,
            cx1 - rx * 0.6, cy1 - ry * 0.8,
            n->x - n->r * 0.6, n->y + 1.5);
    if (with_tail) {
        fprintf(f,
                "    <path class=\"%s\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\" opacity=\"0.85\"/>\n",
                klass,
                cx1 + rx * 0.2, cy1 + ry * 0.4,
                cx1 + rx * 0.95, cy1 + ry * 0.9,
                cx1 + rx * 1.45, cy1 + ry * 0.25);
    }
}

static void sg_svg_layer_nodes(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    (void)m;
    fprintf(f, "  <g id=\"nodes\">\n");
    for (u32 i = 0; i < g->node_count; i++) {
        const cct_sigilo_node_t *n = &g->nodes[i];
        const char *klass = "node-aux";
        if (n->kind == SGN_RITUAL) klass = n->is_entry ? "node-entry" : "node-main";
        else if (n->kind == SGN_DUM || n->kind == SGN_DONEC || n->kind == SGN_REPETE) klass = "node-loop";
        else if (n->kind == SGN_RETURN || n->kind == SGN_ANUR) klass = "node-term";
        double nr = n->r;
        if (style == SG_STYLE_SEAL && n->kind == SGN_RITUAL) nr += 0.5;
        if (style == SG_STYLE_SCRIPTUM && n->kind != SGN_RITUAL) nr *= 0.92;
        fprintf(f, "    <circle class=\"%s\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", klass, n->x, n->y, nr);

        if (n->kind == SGN_REPETE) {
            /* REPETE: serial pulse chain clearly legible */
            u32 pulses = n->strength > 8 ? 8 : (n->strength * 2 + 1);
            double origin = -((double)(pulses ? (pulses - 1) : 0) * 0.5) * 5.6;
            for (u32 p = 0; p < pulses; p++) {
                double dx = origin + (double)p * 5.6;
                double px = n->x + dx;
                double py = n->y + ((p % 2) ? -2.1 : 2.1);
                fprintf(f, "    <circle class=\"node-loop\" cx=\"%.2f\" cy=\"%.2f\" r=\"1.25\" opacity=\"0.9\"/>\n", px, py);
                if (p > 0) {
                    fprintf(f, "    <line class=\"loop\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" opacity=\"0.75\"/>\n",
                            px - 5.6, n->y + (((p - 1) % 2) ? -2.1 : 2.1), px, py);
                }
            }
        }
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_primary_path(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"primary_path\">\n");
    if (g->primary_count == 0) {
        fprintf(f, "  </g>\n");
        return;
    }

    if (g->primary_count == 1) {
        const cct_sigilo_node_t *n = &g->nodes[g->primary_order[0]];
        if (style == SG_STYLE_SCRIPTUM) {
            fprintf(f, "    <path class=\"primary\" d=\"M %.2f %.2f q 20 -26 38 -4 q -8 18 -28 20\" opacity=\"0.92\"/>\n", n->x - 18.0, n->y + 2.0);
        } else {
            fprintf(f, "    <path class=\"primary\" d=\"M %.2f %.2f q 18 -22 34 0 q -18 22 -34 0\" opacity=\"0.9\"/>\n", n->x - 16.0, n->y);
        }
        fprintf(f, "  </g>\n");
        return;
    }

    for (u32 i = 1; i < g->primary_count; i++) {
        const cct_sigilo_node_t *a = &g->nodes[g->primary_order[i - 1]];
        const cct_sigilo_node_t *b = &g->nodes[g->primary_order[i]];
        double complexity_bend = 14.0 + (double)((m->ritual_count > 6 ? 6 : m->ritual_count) * 2);
        if (m->ritual_count == 1) complexity_bend += 8.0;
        sg_svg_emit_curved_link(f, m, style, a, b, "primary", complexity_bend, 2.6 + (a->is_entry ? 0.6 : 0.0), 0.96);
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_call_links(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"call_links\">\n");
    for (u32 i = 0; i < g->link_count; i++) {
        const cct_sigilo_link_t *lk = &g->links[i];
        if (lk->kind != SGL_CONIURA) continue;
        if (lk->from_idx >= g->node_count || lk->to_idx >= g->node_count) continue;
        const cct_sigilo_node_t *a = &g->nodes[lk->from_idx];
        const cct_sigilo_node_t *b = &g->nodes[lk->to_idx];
        if (lk->self_loop) {
            sg_svg_emit_self_loop(f, m, style, a, "call", 1.15 + sg_clamp((double)lk->weight * 0.1, 0.0, 0.35), false);
            continue;
        }
        sg_svg_emit_curved_link(
            f, m, style, a, b, "call",
            26.0 + sg_clamp((double)lk->weight * 5.0, 0.0, 22.0),
            1.8 + sg_clamp((double)lk->weight * 0.35, 0.0, 1.2),
            0.68 + sg_clamp((double)lk->weight * 0.06, 0.0, 0.22));
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_branches(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"branches\">\n");
    for (u32 i = 0; i < g->link_count; i++) {
        const cct_sigilo_link_t *lk = &g->links[i];
        if (lk->kind != SGL_BRANCH || lk->from_idx >= g->node_count || lk->to_idx >= g->node_count) continue;
        const cct_sigilo_node_t *a = &g->nodes[lk->from_idx];
        const cct_sigilo_node_t *b = &g->nodes[lk->to_idx];
        sg_svg_emit_curved_link(f, m, style, a, b, "branch", 10.0, 1.2 + sg_clamp((double)lk->weight * 0.25, 0.0, 0.9), 0.85);
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_loops(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"loops\">\n");
    for (u32 i = 0; i < g->link_count; i++) {
        const cct_sigilo_link_t *lk = &g->links[i];
        if (lk->kind != SGL_LOOP || lk->from_idx >= g->node_count || lk->to_idx >= g->node_count) continue;
        const cct_sigilo_node_t *a = &g->nodes[lk->from_idx];
        const cct_sigilo_node_t *b = &g->nodes[lk->to_idx];
        if (lk->self_loop) {
            bool tail = (b->kind == SGN_DONEC);
            sg_svg_emit_self_loop(f, m, style, b, "loop", (b->kind == SGN_DUM) ? 1.0 : 1.12, tail);
        } else {
            sg_svg_emit_curved_link(f, m, style, a, b, "loop", 8.0, 1.25 + sg_clamp((double)lk->weight * 0.15, 0.0, 0.55), 0.88);
        }
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_bindings(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"bindings\">\n");
    for (u32 i = 0; i < g->link_count; i++) {
        const cct_sigilo_link_t *lk = &g->links[i];
        if (lk->kind != SGL_BINDING || lk->from_idx >= g->node_count || lk->to_idx >= g->node_count) continue;
        const cct_sigilo_node_t *a = &g->nodes[lk->from_idx];
        const cct_sigilo_node_t *b = &g->nodes[lk->to_idx];
        sg_svg_emit_curved_link(f, m, style, a, b, "bind", 5.0, 1.2 + sg_clamp((double)lk->weight * 0.08, 0.0, 0.6), 0.85);

        /* VINCIRE / binding bars */
        u32 bars = lk->weight > 6 ? 6 : lk->weight;
        double dx = b->x - a->x;
        double dy = b->y - a->y;
        sg_norm(&dx, &dy);
        double nx = -dy, ny = dx;
        for (u32 k = 0; k < bars; k++) {
            double t = 0.20 + 0.10 * (double)k;
            if (t > 0.85) break;
            double px = a->x + (b->x - a->x) * t;
            double py = a->y + (b->y - a->y) * t;
            double len = 4.0 + (double)(k % 2);
            if (style == SG_STYLE_SEAL) len *= 0.85;
            if (style == SG_STYLE_SCRIPTUM) len *= 1.20;
            fprintf(f, "    <line class=\"bind\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" opacity=\"0.75\"/>\n",
                    px - nx * len, py - ny * len, px + nx * len, py + ny * len);
        }

        /* OBSECRO/EVOCA small emissions around binding node using node strength */
        const cct_sigilo_node_t *bn = b;
        u32 rays = bn->strength > 5 ? 5 : bn->strength;
        for (u32 r = 0; r < rays; r++) {
            double ang = (2.0 * CCT_SIGILO_PI) * (double)r / (double)(rays ? rays : 1);
            ang += (sg_hash_unit(m, 1400 + b->ritual_idx * 19 + r) - 0.5) * 0.6;
            double x1 = bn->x + cos(ang) * (bn->r + 2.0);
            double y1 = bn->y + sin(ang) * (bn->r + 2.0);
            double x2 = bn->x + cos(ang) * (bn->r + 8.0 + (double)(r % 2) * 3.0);
            double y2 = bn->y + sin(ang) * (bn->r + 8.0 + (double)(r % 2) * 3.0);
            fprintf(f, "    <line class=\"bind\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" opacity=\"0.58\"/>\n", x1, y1, x2, y2);
        }
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_terminals(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    fprintf(f, "  <g id=\"terminals\">\n");
    for (u32 i = 0; i < g->link_count; i++) {
        const cct_sigilo_link_t *lk = &g->links[i];
        if ((lk->kind != SGL_RETURN && lk->kind != SGL_TERMINAL) || lk->from_idx >= g->node_count || lk->to_idx >= g->node_count) continue;
        const cct_sigilo_node_t *a = &g->nodes[lk->from_idx];
        const cct_sigilo_node_t *b = &g->nodes[lk->to_idx];
        sg_svg_emit_curved_link(f, m, style, a, b, "term", 6.0, 1.35 + sg_clamp((double)lk->weight * 0.15, 0.0, 0.55), 0.9);
        if (b->kind == SGN_ANUR) {
            double dx = b->x - a->x;
            double dy = b->y - a->y;
            sg_norm(&dx, &dy);
            double nx = -dy, ny = dx;
            double tipx = b->x + dx * (b->r + 6.0);
            double tipy = b->y + dy * (b->r + 6.0);
            fprintf(f, "    <line class=\"term\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\"/>\n",
                    tipx - nx * 4.2, tipy - ny * 4.2, tipx + nx * 4.2, tipy + ny * 4.2);
            fprintf(f, "    <line class=\"term\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" opacity=\"0.8\"/>\n",
                    tipx - nx * 2.4 + dx * 3.0, tipy - ny * 2.4 + dy * 3.0,
                    tipx + nx * 2.4 + dx * 3.0, tipy + ny * 2.4 + dy * 3.0);
        }
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_ornaments(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_geom_t *g, sg_visual_style_t style) {
    (void)g;
    u32 marks = 4 + (m->total_statements > 40 ? 10 : (m->total_statements / 4));
    if (style == SG_STYLE_SEAL) marks += 4;
    if (style == SG_STYLE_SCRIPTUM) marks = (marks > 2) ? (marks - 2) : marks;
    fprintf(f, "  <g id=\"ornaments\">\n");
    for (u32 i = 0; i < marks; i++) {
        double a = (2.0 * CCT_SIGILO_PI) * (double)i / (double)(marks ? marks : 1);
        a += (sg_hash_unit(m, 1500 + i) - 0.5) * 0.45;
        double r = 205.0 + (sg_hash_unit(m, 1510 + i) - 0.5) * 14.0;
        if (style == SG_STYLE_SEAL) r -= 8.0;
        if (style == SG_STYLE_SCRIPTUM) r += (i % 2) ? -10.0 : 6.0;
        double x, y;
        sg_point_on_circle(CCT_SIGILO_CENTER_X, CCT_SIGILO_CENTER_Y, r, a, &x, &y);
        fprintf(f, "    <circle class=\"orn\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", x, y, 0.9 + (double)(i % 3));
    }
    fprintf(f, "  </g>\n");
}

static void sg_svg_layer_hash_signature(FILE *f, const cct_sigilo_model_t *m, const cct_sigilo_t *sg) {
    (void)m;
    fprintf(f, "  <g id=\"signature\">\n");
    fprintf(f, "    <text class=\"hash\" x=\"256\" y=\"496\" text-anchor=\"middle\">%s</text>\n", sg->semantic_hash_hex);
    if (sg->ritual_count > 0) {
        fprintf(f, "    <text class=\"label\" x=\"256\" y=\"18\" text-anchor=\"middle\">%s • sigillum v2.1 • %u rituale(s) • %u stmt • depth %u</text>\n",
                sg->style_name ? sg->style_name : "network",
                sg->ritual_count, sg->total_statements, sg->max_nesting);
    }
    fprintf(f, "  </g>\n");
}

static bool sg_write_svg(cct_sigilo_t *sg, const cct_sigilo_model_t *m) {
    FILE *f = fopen(sg->svg_path, "wb");
    if (!f) {
        sg_reportf(sg, 0, 0, "could not write sigilo SVG: %s", sg->svg_path);
        return false;
    }

    sg_visual_style_t style = sg_style_from_name(sg->style_name);
    cct_sigilo_geom_t geom;
    sg_build_geom(m, &geom);

    sg_svg_header(f, style);
    sg_svg_layer_foundation(f, m, style);
    sg_svg_layer_primary_path(f, m, &geom, style);
    sg_svg_layer_call_links(f, m, &geom, style);
    sg_svg_layer_branches(f, m, &geom, style);
    sg_svg_layer_loops(f, m, &geom, style);
    sg_svg_layer_bindings(f, m, &geom, style);
    sg_svg_layer_terminals(f, m, &geom, style);
    sg_svg_layer_nodes(f, m, &geom, style);
    sg_svg_layer_ornaments(f, m, &geom, style);
    sg_svg_layer_hash_signature(f, m, sg);
    fputs("</svg>\n", f);
    fclose(f);

    sg_geom_dispose(&geom);
    return true;
}

static void sg_svg_emit_core_layers(
    FILE *f,
    const cct_sigilo_model_t *m,
    const cct_sigilo_t *sg,
    bool include_signature
) {
    sg_visual_style_t style = sg_style_from_name(sg->style_name);
    cct_sigilo_geom_t geom;
    sg_build_geom(m, &geom);

    sg_svg_layer_foundation(f, m, style);
    sg_svg_layer_primary_path(f, m, &geom, style);
    sg_svg_layer_call_links(f, m, &geom, style);
    sg_svg_layer_branches(f, m, &geom, style);
    sg_svg_layer_loops(f, m, &geom, style);
    sg_svg_layer_bindings(f, m, &geom, style);
    sg_svg_layer_terminals(f, m, &geom, style);
    sg_svg_layer_nodes(f, m, &geom, style);
    sg_svg_layer_ornaments(f, m, &geom, style);
    if (include_signature) {
        sg_svg_layer_hash_signature(f, m, sg);
    }

    sg_geom_dispose(&geom);
}

/* ============================== .sigil ============================== */

static bool sg_write_meta(cct_sigilo_t *sg, const cct_sigilo_model_t *m) {
    FILE *f = fopen(sg->sigil_meta_path, "wb");
    if (!f) {
        sg_reportf(sg, 0, 0, "could not write sigilo metadata: %s", sg->sigil_meta_path);
        return false;
    }

    cct_sigilo_geom_t geom;
    sg_build_geom(m, &geom);

    u32 self_call_count = 0;
    for (u32 i = 0; i < m->edge_count; i++) {
        if (m->edges[i].from_idx == m->edges[i].to_idx) self_call_count += m->edges[i].count;
    }
    u32 loop_family = m->count_dum + m->count_donec + m->count_repete + m->count_iterum;
    bool multi_ritual = m->ritual_count > 1;
    const char *complexity_class = "simple";
    if (self_call_count > 0) complexity_class = "recursive";
    else if (loop_family > 0) complexity_class = "looped";
    else if (m->total_statements > 36 || geom.node_count > 20) complexity_class = "dense";
    else if (m->total_statements > 12 || m->total_calls > 2 || m->total_conditionals > 1) complexity_class = "compound";

    double primary_axis_deg = 0.0;
    if (geom.primary_count >= 2) {
        const cct_sigilo_node_t *a = &geom.nodes[geom.primary_order[0]];
        const cct_sigilo_node_t *b = &geom.nodes[geom.primary_order[1]];
        primary_axis_deg = atan2(b->y - a->y, b->x - a->x) * 180.0 / CCT_SIGILO_PI;
    } else if (geom.primary_count == 1) {
        const cct_sigilo_node_t *a = &geom.nodes[geom.primary_order[0]];
        primary_axis_deg = atan2(a->y - CCT_SIGILO_CENTER_Y, a->x - CCT_SIGILO_CENTER_X) * 180.0 / CCT_SIGILO_PI;
    }

    const char *stdlib_modules[17];
    u32 stdlib_module_count = 0;
    if (m->count_verbum_ops) stdlib_modules[stdlib_module_count++] = "verbum";
    if (m->count_fmt_ops) stdlib_modules[stdlib_module_count++] = "fmt";
    if (m->count_series_ops) stdlib_modules[stdlib_module_count++] = "series";
    if (m->count_fluxus_ops) stdlib_modules[stdlib_module_count++] = "fluxus";
    if (m->count_mem_ops) stdlib_modules[stdlib_module_count++] = "mem";
    if (m->count_io_ops) stdlib_modules[stdlib_module_count++] = "io";
    if (m->count_fs_ops) stdlib_modules[stdlib_module_count++] = "fs";
    if (m->count_path_ops) stdlib_modules[stdlib_module_count++] = "path";
    if (m->count_math_ops) stdlib_modules[stdlib_module_count++] = "math";
    if (m->count_random_ops) stdlib_modules[stdlib_module_count++] = "random";
    if (m->count_parse_ops) stdlib_modules[stdlib_module_count++] = "parse";
    if (m->count_cmp_ops) stdlib_modules[stdlib_module_count++] = "cmp";
    if (m->count_alg_ops) stdlib_modules[stdlib_module_count++] = "alg";
    if (m->count_option_ops) stdlib_modules[stdlib_module_count++] = "option";
    if (m->count_result_ops) stdlib_modules[stdlib_module_count++] = "result";
    if (m->count_map_ops) stdlib_modules[stdlib_module_count++] = "map";
    if (m->count_set_ops) stdlib_modules[stdlib_module_count++] = "set";
    if (m->count_collection_ops) stdlib_modules[stdlib_module_count++] = "collection_ops";

    char stdlib_modules_used[256];
    stdlib_modules_used[0] = '\0';
    for (u32 i = 0; i < stdlib_module_count; i++) {
        if (i > 0) {
            strncat(stdlib_modules_used, ",", sizeof(stdlib_modules_used) - strlen(stdlib_modules_used) - 1);
        }
        strncat(stdlib_modules_used, stdlib_modules[i], sizeof(stdlib_modules_used) - strlen(stdlib_modules_used) - 1);
    }
    if (stdlib_module_count == 0) {
        strncpy(stdlib_modules_used, "none", sizeof(stdlib_modules_used) - 1);
        stdlib_modules_used[sizeof(stdlib_modules_used) - 1] = '\0';
    }

    fprintf(f, "format = cct.sigil.v1\n");
    fprintf(f, "visual_engine = sigillum_v2_1\n");
    fprintf(f, "visual_style = %s\n", sg->style_name ? sg->style_name : "network");
    fprintf(f, "compiler_version = %s\n", CCT_VERSION_STRING);
    fprintf(f, "program_name = %s\n", m->program_name ? m->program_name : "anonymous");
    fprintf(f, "semantic_hash = %s\n", sg->semantic_hash_hex);
    fprintf(f, "layout_seed = %s:%s\n", sg->semantic_hash_hex, sg->style_name ? sg->style_name : "network");
    fprintf(f, "module_mode = %s\n", (sg->module_count > 1) ? "composed" : "single");
    fprintf(f, "module_count = %u\n", sg->module_count ? sg->module_count : 1);
    fprintf(f, "import_edge_count = %u\n", sg->import_edge_count);
    fprintf(f, "entry_module = %s\n", sg->entry_module_path ? sg->entry_module_path : (sg->input_path ? sg->input_path : "unknown"));
    fprintf(f, "cross_module_call_count = %u\n", sg->cross_module_call_count);
    fprintf(f, "cross_module_type_ref_count = %u\n", sg->cross_module_type_ref_count);
    fprintf(f, "module_resolution_status = %s\n", sg->module_resolution_ok ? "ok" : "error");
    fprintf(f, "stdlib_module_count = %u\n", stdlib_module_count);
    fprintf(f, "stdlib_modules_used = %s\n", stdlib_modules_used);
    fprintf(f, "verbum_module_used = %s\n", m->count_verbum_ops ? "true" : "false");
    fprintf(f, "fmt_module_used = %s\n", m->count_fmt_ops ? "true" : "false");
    fprintf(f, "series_module_used = %s\n", m->count_series_ops ? "true" : "false");
    fprintf(f, "fluxus_module_used = %s\n", m->count_fluxus_ops ? "true" : "false");
    fprintf(f, "mem_module_used = %s\n", m->count_mem_ops ? "true" : "false");
    fprintf(f, "io_module_used = %s\n", m->count_io_ops ? "true" : "false");
    fprintf(f, "fs_module_used = %s\n", m->count_fs_ops ? "true" : "false");
    fprintf(f, "path_module_used = %s\n", m->count_path_ops ? "true" : "false");
    fprintf(f, "math_module_used = %s\n", m->count_math_ops ? "true" : "false");
    fprintf(f, "random_module_used = %s\n", m->count_random_ops ? "true" : "false");
    fprintf(f, "parse_module_used = %s\n", m->count_parse_ops ? "true" : "false");
    fprintf(f, "cmp_module_used = %s\n", m->count_cmp_ops ? "true" : "false");
    fprintf(f, "alg_module_used = %s\n", m->count_alg_ops ? "true" : "false");
    fprintf(f, "option_module_used = %s\n", m->count_option_ops ? "true" : "false");
    fprintf(f, "result_module_used = %s\n", m->count_result_ops ? "true" : "false");
    fprintf(f, "map_module_used = %s\n", m->count_map_ops ? "true" : "false");
    fprintf(f, "set_module_used = %s\n", m->count_set_ops ? "true" : "false");
    fprintf(f, "collection_ops_module_used = %s\n", m->count_collection_ops ? "true" : "false");
    fprintf(f, "visibility_model = public_default_arcanum_internal\n");
    fprintf(f, "public_symbol_count = %u\n", sg->public_symbol_count);
    fprintf(f, "internal_symbol_count = %u\n", sg->internal_symbol_count);
    fprintf(f, "\n[metrics]\n");
    fprintf(f, "ritual_count = %u\n", m->ritual_count);
    fprintf(f, "entry_ritual_index = %d\n", m->entry_idx);
    if (m->entry_idx >= 0 && (u32)m->entry_idx < m->ritual_count) {
        fprintf(f, "entry_ritual_name = %s\n", m->rituals[m->entry_idx].name);
    }
    fprintf(f, "total_statements = %u\n", m->total_statements);
    fprintf(f, "total_expressions = %u\n", m->total_exprs);
    fprintf(f, "max_nesting = %u\n", m->max_nesting);
    fprintf(f, "avg_nesting = %.3f\n", (m->depth_samples ? ((double)m->depth_accum / (double)m->depth_samples) : 0.0));
    fprintf(f, "loops_total = %u\n", m->total_loops);
    fprintf(f, "conditionals_total = %u\n", m->total_conditionals);
    fprintf(f, "calls_total = %u\n", m->total_calls);
    fprintf(f, "obsecro_total = %u\n", m->total_obsecro);
    fprintf(f, "returns_total = %u\n", m->total_returns);
    fprintf(f, "anur_total = %u\n", m->total_anur);
    fprintf(f, "sigillum_decl_total = %u\n", m->count_sigillum_decl);
    fprintf(f, "ordo_decl_total = %u\n", m->count_ordo_decl);
    fprintf(f, "generic_decl_count = %u\n", m->count_generic_decl);
    fprintf(f, "generic_param_count = %u\n", m->count_generic_params);
    fprintf(f, "generic_rituale_count = %u\n", m->count_generic_rituale_decl);
    fprintf(f, "generic_sigillum_count = %u\n", m->count_generic_sigillum_decl);
    fprintf(f, "generic_instantiation_count = %u\n", m->count_generic_instantiation);
    fprintf(f, "generic_rituale_instantiation_count = %u\n", m->count_generic_rituale_instantiation);
    fprintf(f, "generic_sigillum_instantiation_count = %u\n", m->count_generic_sigillum_instantiation);
    fprintf(f, "generic_instantiation_dedup_count = %u\n", m->count_generic_instantiation_dedup);
    fprintf(f, "generic_constraint_count = %u\n", m->count_generic_constraint);
    fprintf(f, "generic_constrained_param_count = %u\n", m->count_generic_constrained_param);
    fprintf(f, "generic_constraint_violation_count = %u\n", m->count_generic_constraint_violation);
    fprintf(f, "pactum_constraint_resolution_status = %s\n", m->pactum_constraint_resolution_ok ? "ok" : "error");
    fprintf(f, "generic_monomorphization_status = ok\n");
    fprintf(f, "pactum_decl_count = %u\n", m->count_pactum_decl);
    fprintf(f, "pactum_signature_count = %u\n", m->count_pactum_signature);
    fprintf(f, "sigillum_pactum_conformance_count = %u\n", m->count_sigillum_pactum_conformance);
    fprintf(f, "pactum_conformance_status = %s\n", m->pactum_conformance_ok ? "ok" : "error");
    fprintf(f, "multi_ritual = %s\n", multi_ritual ? "true" : "false");
    fprintf(f, "complexity_class = %s\n", complexity_class);
    fprintf(f, "\n[counts]\n");
    fprintf(f, "evoca = %u\n", m->count_evoca);
    fprintf(f, "vincire = %u\n", m->count_vincire);
    fprintf(f, "si = %u\n", m->count_si);
    fprintf(f, "aliter = %u\n", m->count_aliter);
    fprintf(f, "dum = %u\n", m->count_dum);
    fprintf(f, "donec = %u\n", m->count_donec);
    fprintf(f, "repete = %u\n", m->count_repete);
    fprintf(f, "iterum = %u\n", m->count_iterum);
    fprintf(f, "iterum_count = %u\n", m->count_iterum);
    fprintf(f, "coniura = %u\n", m->count_coniura);
    fprintf(f, "obsecro = %u\n", m->count_obsecro);
    fprintf(f, "redde = %u\n", m->count_redde);
    fprintf(f, "anur = %u\n", m->count_anur);
    fprintf(f, "field_access = %u\n", m->count_field_access);
    fprintf(f, "index_access = %u\n", m->count_index_access);
    fprintf(f, "series_type_use = %u\n", m->count_series_type_use);
    fprintf(f, "speculum_type_use = %u\n", m->count_speculum_type_use);
    fprintf(f, "sigillum_decl = %u\n", m->count_sigillum_decl);
    fprintf(f, "sigillum_composite_field = %u\n", m->count_sigillum_composite_field);
    fprintf(f, "ordo_decl = %u\n", m->count_ordo_decl);
    fprintf(f, "type_umbra = %u\n", m->count_type_umbra);
    fprintf(f, "type_flamma = %u\n", m->count_type_flamma);
    fprintf(f, "address_of = %u\n", m->count_addr_of);
    fprintf(f, "deref = %u\n", m->count_deref);
    fprintf(f, "alloc = %u\n", m->count_alloc);
    fprintf(f, "free = %u\n", m->count_free);
    fprintf(f, "dimitte = %u\n", m->count_dimitte);
    fprintf(f, "fluxus_ops_count = %u\n", m->count_fluxus_ops);
    fprintf(f, "verbum_ops_count = %u\n", m->count_verbum_ops);
    fprintf(f, "fmt_ops_count = %u\n", m->count_fmt_ops);
    fprintf(f, "series_ops_count = %u\n", m->count_series_ops);
    fprintf(f, "mem_ops_count = %u\n", m->count_mem_ops);
    fprintf(f, "io_ops_count = %u\n", m->count_io_ops);
    fprintf(f, "fs_ops_count = %u\n", m->count_fs_ops);
    fprintf(f, "fluxus_init_count = %u\n", m->count_fluxus_init);
    fprintf(f, "fluxus_push_count = %u\n", m->count_fluxus_push);
    fprintf(f, "fluxus_pop_count = %u\n", m->count_fluxus_pop);
    fprintf(f, "fluxus_instances_count = %u\n", m->count_fluxus_instances);
    fprintf(f, "path_ops_count = %u\n", m->count_path_ops);
    fprintf(f, "math_ops_count = %u\n", m->count_math_ops);
    fprintf(f, "random_ops_count = %u\n", m->count_random_ops);
    fprintf(f, "parse_ops_count = %u\n", m->count_parse_ops);
    fprintf(f, "cmp_ops_count = %u\n", m->count_cmp_ops);
    fprintf(f, "alg_ops_count = %u\n", m->count_alg_ops);
    fprintf(f, "option_ops_count = %u\n", m->count_option_ops);
    fprintf(f, "result_ops_count = %u\n", m->count_result_ops);
    fprintf(f, "map_ops_count = %u\n", m->count_map_ops);
    fprintf(f, "set_ops_count = %u\n", m->count_set_ops);
    fprintf(f, "collection_ops_count = %u\n", m->count_collection_ops);
    fprintf(f, "pactum_decl = %u\n", m->count_pactum_decl);
    fprintf(f, "pactum_signature = %u\n", m->count_pactum_signature);
    fprintf(f, "sigillum_pactum_conformance = %u\n", m->count_sigillum_pactum_conformance);
    fprintf(f, "generic_constraint = %u\n", m->count_generic_constraint);
    fprintf(f, "generic_constrained_param = %u\n", m->count_generic_constrained_param);
    fprintf(f, "generic_constraint_violation = %u\n", m->count_generic_constraint_violation);
    fprintf(f, "tempta = %u\n", m->count_tempta);
    fprintf(f, "cape = %u\n", m->count_cape);
    fprintf(f, "semper = %u\n", m->count_semper);
    fprintf(f, "iace = %u\n", m->count_iace);

    fprintf(f, "\n[topology]\n");
    fprintf(f, "node_count = %u\n", geom.node_count);
    fprintf(f, "edge_count = %u\n", geom.link_count);
    fprintf(f, "primary_path_nodes = %u\n", geom.primary_count);
    fprintf(f, "ritual_poles = %u\n", m->ritual_count);
    fprintf(f, "aux_nodes = %u\n", (geom.node_count >= m->ritual_count ? geom.node_count - m->ritual_count : 0));
    fprintf(f, "self_call_count = %u\n", self_call_count);
    fprintf(f, "loop_presence = %s\n", loop_family > 0 ? "true" : "false");
    fprintf(f, "conditional_presence = %s\n", m->total_conditionals > 0 ? "true" : "false");
    fprintf(f, "call_presence = %s\n", m->total_calls > 0 ? "true" : "false");
    fprintf(f, "density_ratio = %.3f\n", (geom.node_count ? ((double)m->total_statements / (double)geom.node_count) : 0.0));
    fprintf(f, "field_access_presence = %s\n", m->count_field_access ? "true" : "false");
    fprintf(f, "index_access_presence = %s\n", m->count_index_access ? "true" : "false");
    fprintf(f, "memory_presence = %s\n", (m->count_alloc || m->count_free || m->count_dimitte) ? "true" : "false");
    fprintf(f, "speculum_presence = %s\n", (m->count_speculum_type_use || m->count_addr_of || m->count_deref) ? "true" : "false");
    fprintf(f, "dynamic_indexable_memory_presence = %s\n",
            ((m->count_alloc || m->count_dimitte || m->count_free) && m->count_index_access &&
             (m->count_speculum_type_use || m->count_addr_of || m->count_deref)) ? "true" : "false");
    fprintf(f, "indirect_structural_flow_presence = %s\n",
            ((m->count_field_access > 0) &&
             (m->count_deref > 0 || m->count_speculum_type_use > 0)) ? "true" : "false");
    fprintf(f, "failure_construct_presence = %s\n", (m->count_tempta || m->count_iace) ? "true" : "false");
    fprintf(f, "tempta_presence = %s\n", m->count_tempta ? "true" : "false");
    fprintf(f, "cape_presence = %s\n", m->count_cape ? "true" : "false");
    fprintf(f, "semper_presence = %s\n", m->count_semper ? "true" : "false");
    fprintf(f, "iace_presence = %s\n", m->count_iace ? "true" : "false");
    fprintf(f, "failure_call_propagation_presence = %s\n",
            (m->count_tempta > 0 && m->count_iace > 0 && m->count_coniura > 0) ? "true" : "false");
    fprintf(f, "failure_call_propagation_count = %u\n",
            (m->count_tempta > 0 && m->count_iace > 0) ? m->count_coniura : 0);
    fprintf(f, "iace_across_coniura_presence = %s\n",
            (m->count_iace > 0 && m->count_coniura > 0) ? "true" : "false");
    fprintf(f, "tempta_call_capture_presence = %s\n",
            (m->count_tempta > 0 && m->count_coniura > 0) ? "true" : "false");
    fprintf(f, "failure_cleanup_presence = %s\n",
            (m->count_semper > 0 && (m->count_tempta > 0 || m->count_iace > 0)) ? "true" : "false");
    fprintf(f, "failure_cleanup_memory_presence = %s\n",
            (m->count_semper > 0 && (m->count_alloc || m->count_free || m->count_dimitte)) ? "true" : "false");
    fprintf(f, "runtime_failure_bridge_presence = %s\n",
            (m->count_semper > 0 && (m->count_deref || m->count_index_access)) ? "true" : "false");
    fprintf(f, "phase7_subset_final = true\n");
    fprintf(f, "phase7_discard_policy = manual_explicit(obsecro_libera,dimitte)\n");
    fprintf(f, "phase8_subset_final = true\n");
    fprintf(f, "phase8_failure_policy = fractum_simple_opaco(payload=verbum);tempta_cape_semper_subset\n");
    fprintf(f, "phase8_runtime_fail_policy = partial_bridge_documented(integrated=null_pointer_checks_in_instrumented_failure_paths;others=direct_abort)\n");
    fprintf(f, "phase10_subset_final = true\n");
    fprintf(f, "phase10_final_status = consolidated\n");
    fprintf(f, "phase10_typing_contract = genus_explicit_monomorphization+pactum_explicit_conformance+constraint_single_contract_rituale_only\n");

    fprintf(f, "\n[layout]\n");
    fprintf(f, "primary_axis_deg = %.3f\n", primary_axis_deg);
    fprintf(f, "canvas_center = %.1f,%.1f\n", CCT_SIGILO_CENTER_X, CCT_SIGILO_CENTER_Y);
    fprintf(f, "containment_radius = 226.0\n");
    fprintf(f, "style_influence = stroke/curvature/distribution/ornamentation\n");

    fprintf(f, "\n[visual_grammar]\n");
    fprintf(f, "style = network inscription / ritual seal (FASE 6A)\n");
    fprintf(f, "style_selected = %s\n", sg->style_name ? sg->style_name : "network");
    fprintf(f, "foundation = minimal containment circle (+ optional light secondary ring)\n");
    fprintf(f, "layers = foundation,primary_path,call_links,branches,loops,bindings,terminals,nodes,ornaments,signature\n");
    fprintf(f, "macro_layout = ritual nodes + structural links + primary path\n");
    fprintf(f, "rituale = primary node / pole in network\n");
    fprintf(f, "coniura = dominant inter-ritual link (self-call => local loop)\n");
    fprintf(f, "si = real bifurcation node + branch split\n");
    fprintf(f, "aliter = alternate branch / compensatory reconnection\n");
    fprintf(f, "dum = recurrence loop glyph (pre-test)\n");
    fprintf(f, "donec = post_test loop glyph (loop + tail)\n");
    fprintf(f, "repete = serial pulse chain / cadence marks\n");
    fprintf(f, "series = indexed sequence pattern (metadata/hash influence, v2.1 visual modulation)\n");
    fprintf(f, "speculum = pointer/reference construct (metadata/hash/topology influence, visual refinement deferred)\n");
    fprintf(f, "address_of = reference acquisition mark (structural/hash influence)\n");
    fprintf(f, "deref = bound access incision mark (structural/hash influence)\n");
    fprintf(f, "memory_alloc_free = pete/libera/dimitte contribute memory-presence metrics/hash\n");
    fprintf(f, "failure_propagation = TEMPTA/CAPE/IACE + CONIURA propagation tracked primarily via metadata/topology counts (FASE 8B)\n");
    fprintf(f, "semper = guaranteed cleanup/finally block tracked in metadata/hash (FASE 8C; visual refinement deferred)\n");
    fprintf(f, "failure_phase8_final = metadata reflects consolidated subset semantics through FASE 8D (IACE/TEMPTA/CAPE/SEMPER + propagation + partial runtime bridge)\n");
    fprintf(f, "dynamic_indexable_memory = alloc/free + index_access + speculum/deref combination contributes topology metadata/hash (FASE 7C)\n");
    fprintf(f, "field_access = bound branch tap / field rune mark (metadata/hash influence)\n");
    fprintf(f, "indirect_structural_flow = field_access + speculum/deref combination contributes structural metadata/hash (FASE 7C)\n");
    fprintf(f, "phase7_final = metadata reflects consolidated subset semantics through FASE 7D (no visual engine redesign)\n");
    fprintf(f, "tempta_cape_iace = failure-control constructs contribute metadata/hash in FASE 8A (visual refinement deferred)\n");
    fprintf(f, "sigillum = named vessel/struct declaration contributes topology metadata\n");
    fprintf(f, "sigillum_composition = nested SIGILLUM field composition contributes structural metadata/hash\n");
    fprintf(f, "ordo = ordinal taxonomy declaration contributes topology metadata\n");
    fprintf(f, "evoca = emergence from binding node\n");
    fprintf(f, "vincire = visible binding bars / lock marks\n");
    fprintf(f, "obsecro = emissive rays (secondary)\n");
    fprintf(f, "redde = convergent terminal path\n");
    fprintf(f, "anur = terminal cut/notch\n");
    fprintf(f, "hash_role = secondary modulation (jitter/curvature/thickness/signature)\n");
    fprintf(f, "style_role = visual weighting/curvature/distribution without changing structural semantics\n");

    fprintf(f, "\n[rituales]\n");
    for (u32 i = 0; i < m->ritual_count; i++) {
        const cct_sigilo_ritual_metric_t *r = &m->rituals[i];
        fprintf(f, "%u = %s | entry=%s | stmt=%u | depth=%u | calls=%u | loops=%u\n",
                i,
                r->name ? r->name : "rituale",
                r->is_entry ? "true" : "false",
                r->total_statements,
                r->max_nesting,
                r->coniura_count,
                r->dum_count + r->donec_count + r->repete_count + r->iterum_count);
    }

    fprintf(f, "\n[call_edges]\n");
    for (u32 i = 0; i < m->edge_count; i++) {
        const cct_sigilo_call_edge_t *e = &m->edges[i];
        if (e->from_idx < m->ritual_count && e->to_idx < m->ritual_count) {
            fprintf(f, "%s -> %s : %u\n",
                    m->rituals[e->from_idx].name,
                    m->rituals[e->to_idx].name,
                    e->count);
        }
    }

    fclose(f);
    sg_geom_dispose(&geom);
    return true;
}

/* ============================== public API =========================== */

void cct_sigilo_init(cct_sigilo_t *sg, const char *filename) {
    memset(sg, 0, sizeof(*sg));
    sg->filename = filename;
    sg->emit_svg = true;
    sg->emit_meta = true;
    sg->style_name = "network";
    sg->module_count = 1;
    sg->import_edge_count = 0;
    sg->cross_module_call_count = 0;
    sg->cross_module_type_ref_count = 0;
    sg->module_resolution_ok = true;
    sg->public_symbol_count = 0;
    sg->internal_symbol_count = 0;
    sg->semantic_hash_hex[0] = '\0';
}

void cct_sigilo_dispose(cct_sigilo_t *sg) {
    if (!sg) return;
    free(sg->input_path);
    free(sg->svg_path);
    free(sg->sigil_meta_path);
    free(sg->entry_module_path);
    memset(sg, 0, sizeof(*sg));
}

bool cct_sigilo_had_error(const cct_sigilo_t *sg) {
    return sg ? sg->had_error : true;
}

u32 cct_sigilo_error_count(const cct_sigilo_t *sg) {
    return sg ? sg->error_count : 0;
}

bool cct_sigilo_set_style(cct_sigilo_t *sg, const char *style_name) {
    if (!sg || !style_name) return false;
    if (strcmp(style_name, "network") != 0 &&
        strcmp(style_name, "seal") != 0 &&
        strcmp(style_name, "scriptum") != 0) {
        return false;
    }
    sg->style_name = sg_style_name(sg_style_from_name(style_name));
    return true;
}

const char* cct_sigilo_get_style(const cct_sigilo_t *sg) {
    return (sg && sg->style_name) ? sg->style_name : "network";
}

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
) {
    if (!sg) return;

    sg->module_count = (module_count == 0) ? 1 : module_count;
    sg->import_edge_count = import_edge_count;
    sg->cross_module_call_count = cross_module_call_count;
    sg->cross_module_type_ref_count = cross_module_type_ref_count;
    sg->module_resolution_ok = module_resolution_ok;
    sg->public_symbol_count = public_symbol_count;
    sg->internal_symbol_count = internal_symbol_count;
    free(sg->entry_module_path);
    sg->entry_module_path = entry_module_path ? sg_strdup(entry_module_path) : NULL;
}

bool cct_sigilo_generate_artifacts(
    cct_sigilo_t *sg,
    const cct_ast_program_t *program,
    const char *input_path,
    const char *output_base_path
) {
    if (!sg || !program || !input_path) return false;
    if (!sg->emit_svg && !sg->emit_meta) {
        sg_reportf(sg, 0, 0, "sigilo generation requested with both SVG and metadata disabled");
        return false;
    }

    free(sg->input_path); sg->input_path = NULL;
    free(sg->svg_path); sg->svg_path = NULL;
    free(sg->sigil_meta_path); sg->sigil_meta_path = NULL;

    sg->input_path = sg_strdup(input_path);
    char *base = output_base_path ? sg_strdup(output_base_path) : sg_derive_output_base(input_path);
    if (sg->emit_svg) sg->svg_path = sg_strdup_printf("%s.svg", base);
    if (sg->emit_meta) sg->sigil_meta_path = sg_strdup_printf("%s.sigil", base);
    free(base);

    cct_sigilo_model_t model;
    if (!sg_extract_model(sg, program, &model)) {
        return false;
    }
    sg->semantic_hash = model.hash;
    sg_hex_u64(model.hash, sg->semantic_hash_hex);

    bool ok = true;
    if (sg->emit_svg) ok = ok && sg_write_svg(sg, &model);
    if (sg->emit_meta) ok = ok && sg_write_meta(sg, &model);
    if (ok) {
        printf("Sigilo style: %s\n", cct_sigilo_get_style(sg));
        printf("Sigilo hash: %s\n", sg->semantic_hash_hex);
        if (sg->emit_svg && sg->svg_path) printf("Sigil SVG: %s\n", sg->svg_path);
        if (sg->emit_meta && sg->sigil_meta_path) printf("Sigil Meta: %s\n", sg->sigil_meta_path);
    }

    sg_free_model(&model);
    return ok && !sg->had_error;
}

typedef struct {
    double x;
    double y;
    double r;
    const char *path;
    u32 depth;
} cct_sigilo_system_node_t;

static const char* sg_system_topology_class(u32 module_count, u32 import_edge_count) {
    if (module_count <= 1) return "single";
    if (import_edge_count == 0) return "disconnected";
    if (import_edge_count == module_count - 1) return "tree";
    if (import_edge_count <= module_count + 1) return "mesh_light";
    return "mesh_dense";
}

static u64 sg_system_hash(
    const char *const *module_paths,
    const u64 *module_hashes,
    u32 module_count,
    const u32 *import_from_indices,
    const u32 *import_to_indices,
    u32 import_edge_count,
    u32 cross_module_call_count,
    u32 cross_module_type_ref_count,
    u32 public_symbol_count,
    u32 internal_symbol_count,
    bool module_resolution_ok
) {
    u64 h = 1469598103934665603ULL;
#define SG_SYS_FNV_U8(v) do { h ^= (u64)(u8)(v); h *= 1099511628211ULL; } while (0)
#define SG_SYS_FNV_U32(v) do { \
    u32 _x = (u32)(v); \
    SG_SYS_FNV_U8((_x >> 0) & 0xFF); \
    SG_SYS_FNV_U8((_x >> 8) & 0xFF); \
    SG_SYS_FNV_U8((_x >> 16) & 0xFF); \
    SG_SYS_FNV_U8((_x >> 24) & 0xFF); \
} while (0)

    SG_SYS_FNV_U32(module_count);
    SG_SYS_FNV_U32(import_edge_count);
    SG_SYS_FNV_U32(cross_module_call_count);
    SG_SYS_FNV_U32(cross_module_type_ref_count);
    SG_SYS_FNV_U32(public_symbol_count);
    SG_SYS_FNV_U32(internal_symbol_count);
    SG_SYS_FNV_U8(module_resolution_ok ? 1 : 0);

    for (u32 i = 0; i < module_count; i++) {
        const char *s = (module_paths && module_paths[i]) ? module_paths[i] : "";
        while (*s) SG_SYS_FNV_U8((u8)*s++);
        SG_SYS_FNV_U8(0);
        u64 mh = module_hashes ? module_hashes[i] : 0;
        for (u32 b = 0; b < 8; b++) {
            SG_SYS_FNV_U8((mh >> (b * 8)) & 0xFF);
        }
    }
    for (u32 i = 0; i < import_edge_count; i++) {
        SG_SYS_FNV_U32(import_from_indices ? import_from_indices[i] : 0);
        SG_SYS_FNV_U32(import_to_indices ? import_to_indices[i] : 0);
    }

#undef SG_SYS_FNV_U32
#undef SG_SYS_FNV_U8
    return h;
}

static const char* sg_system_module_label(const char *path) {
    if (!path || !path[0]) return "module";
    const char *slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static void sg_system_compute_depths(
    u32 module_count,
    const u32 *import_from_indices,
    const u32 *import_to_indices,
    u32 import_edge_count,
    u32 *out_depths,
    u32 *out_max_depth
) {
    if (!out_depths || module_count == 0) {
        if (out_max_depth) *out_max_depth = 0;
        return;
    }

    for (u32 i = 0; i < module_count; i++) out_depths[i] = UINT32_MAX;
    out_depths[0] = 0;

    bool changed = true;
    while (changed) {
        changed = false;
        for (u32 e = 0; e < import_edge_count; e++) {
            u32 from = import_from_indices ? import_from_indices[e] : 0;
            u32 to = import_to_indices ? import_to_indices[e] : 0;
            if (from >= module_count || to >= module_count) continue;
            if (out_depths[from] == UINT32_MAX) continue;
            u32 next_depth = out_depths[from] + 1;
            if (out_depths[to] == UINT32_MAX || next_depth < out_depths[to]) {
                out_depths[to] = next_depth;
                changed = true;
            }
        }
    }

    u32 max_depth = 0;
    for (u32 i = 0; i < module_count; i++) {
        if (out_depths[i] == UINT32_MAX) out_depths[i] = 1;
        if (out_depths[i] > max_depth) max_depth = out_depths[i];
    }
    if (out_max_depth) *out_max_depth = max_depth;
}

static void sg_system_layout_nodes(
    cct_sigilo_system_node_t *nodes,
    u32 module_count,
    const u32 *depths,
    u32 max_depth
) {
    if (!nodes || module_count == 0) return;

    const double cx = 600.0;
    const double cy = 600.0;
    const double base_ring = 220.0;
    const double ring_step = 170.0;

    u32 *depth_counts = (u32*)calloc(max_depth + 1, sizeof(u32));
    u32 *depth_seen = (u32*)calloc(max_depth + 1, sizeof(u32));
    if (!depth_counts || !depth_seen) {
        free(depth_counts);
        free(depth_seen);
        for (u32 i = 0; i < module_count; i++) {
            nodes[i].x = cx;
            nodes[i].y = cy;
            nodes[i].r = (i == 0) ? 138.0 : 88.0;
            nodes[i].depth = depths ? depths[i] : 0;
        }
        return;
    }

    for (u32 i = 0; i < module_count; i++) {
        u32 d = depths ? depths[i] : 0;
        if (d > max_depth) d = max_depth;
        depth_counts[d]++;
    }

    for (u32 i = 0; i < module_count; i++) {
        u32 d = depths ? depths[i] : 0;
        if (d > max_depth) d = max_depth;
        nodes[i].depth = d;

        if (d == 0) {
            nodes[i].x = cx;
            nodes[i].y = cy;
            nodes[i].r = 138.0;
            continue;
        }

        u32 count = depth_counts[d] ? depth_counts[d] : 1;
        u32 pos = depth_seen[d]++;
        double ring_r = base_ring + (double)(d - 1) * ring_step;
        double angle = -CCT_SIGILO_PI / 2.0 + (2.0 * CCT_SIGILO_PI * (double)pos / (double)count);
        double max_slot_by_density = (2.0 * CCT_SIGILO_PI * ring_r) / ((double)count * 2.8);
        double slot_r = 104.0 - (double)(d - 1) * 10.0;
        if (slot_r > max_slot_by_density) slot_r = max_slot_by_density;
        slot_r = sg_clamp(slot_r, 42.0, 120.0);
        nodes[i].x = cx + cos(angle) * ring_r;
        nodes[i].y = cy + sin(angle) * ring_r;
        nodes[i].r = slot_r;
    }

    free(depth_counts);
    free(depth_seen);
}

static bool sg_write_system_svg(
    cct_sigilo_t *sg,
    const char *svg_path,
    const char *entry_path,
    const cct_sigilo_model_t *module_models,
    const char *const *module_paths,
    u32 module_count,
    const u32 *import_from_indices,
    const u32 *import_to_indices,
    u32 import_edge_count,
    u32 cross_module_call_count,
    u32 cross_module_type_ref_count
) {
    FILE *f = fopen(svg_path, "wb");
    if (!f) {
        sg_reportf(sg, 0, 0, "could not write system sigilo SVG: %s", svg_path);
        return false;
    }

    cct_sigilo_system_node_t *nodes = NULL;
    u32 *depths = NULL;
    u32 max_depth = 0;
    if (module_count > 0) {
        nodes = (cct_sigilo_system_node_t*)calloc(module_count, sizeof(*nodes));
        depths = (u32*)calloc(module_count, sizeof(*depths));
        if (!nodes || !depths) {
            fclose(f);
            free(nodes);
            free(depths);
            sg_reportf(sg, 0, 0, "out of memory while generating system sigilo SVG");
            return false;
        }
    }
    sg_system_compute_depths(module_count, import_from_indices, import_to_indices, import_edge_count, depths, &max_depth);
    sg_system_layout_nodes(nodes, module_count, depths, max_depth);
    for (u32 i = 0; i < module_count; i++) {
        nodes[i].path = (module_paths && module_paths[i]) ? module_paths[i] : "";
    }

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"1200\" viewBox=\"0 0 1200 1200\" role=\"img\" aria-label=\"CCT system sigilo\">\n");
    fprintf(f, "  <defs>\n");
    fprintf(f, "    <style><![CDATA[\n");
    fprintf(f, "      .bg { fill: #f7f1e5; }\n");
    fprintf(f, "      .macro-ring { fill: none; stroke: #1f2937; stroke-width: 2.0; opacity: 0.32; }\n");
    fprintf(f, "      .macro-import { stroke: #334155; stroke-width: 1.8; stroke-linecap: round; fill: none; opacity: 0.60; }\n");
    fprintf(f, "      .macro-sem { stroke: #9a3412; stroke-width: 1.3; stroke-dasharray: 5 4; fill: none; opacity: 0.62; }\n");
    fprintf(f, "      .module-frame { fill: none; stroke: #111827; stroke-width: 2.1; }\n");
    fprintf(f, "      .module-entry-frame { fill: none; stroke: #0f766e; stroke-width: 3.0; }\n");
    fprintf(f, "      .module-label { fill: #0f172a; font: 11px monospace; }\n");
    fprintf(f, "      .module-hash { fill: #334155; font: 9px monospace; opacity: 0.8; }\n");
    fprintf(f, "      .base { stroke: #1f1a17; stroke-width: 1.6; fill: none; opacity: 0.7; }\n");
    fprintf(f, "      .primary { stroke: #111827; stroke-width: 2.4; fill: none; stroke-linecap: round; }\n");
    fprintf(f, "      .call { stroke: #4b5563; stroke-width: 1.5; fill: none; stroke-linecap: round; opacity: 0.82; }\n");
    fprintf(f, "      .branch { stroke: #6b7280; stroke-width: 1.2; fill: none; opacity: 0.8; }\n");
    fprintf(f, "      .loop { stroke: #0f766e; stroke-width: 1.3; fill: none; opacity: 0.84; }\n");
    fprintf(f, "      .bind { stroke: #6b7280; stroke-width: 1.05; fill: none; opacity: 0.84; }\n");
    fprintf(f, "      .term { stroke: #7f1d1d; stroke-width: 1.45; fill: none; stroke-linecap: round; }\n");
    fprintf(f, "      .node-main { fill: #f8fafc; stroke: #1f2937; stroke-width: 1.65; }\n");
    fprintf(f, "      .node-entry { fill: #d1fae5; stroke: #0f766e; stroke-width: 1.95; }\n");
    fprintf(f, "      .node-aux { fill: #f1f5f9; stroke: #475569; stroke-width: 1.15; }\n");
    fprintf(f, "      .node-loop { fill: #ecfeff; stroke: #0f766e; stroke-width: 1.2; }\n");
    fprintf(f, "      .node-term { fill: #fef2f2; stroke: #991b1b; stroke-width: 1.2; }\n");
    fprintf(f, "      .orn { stroke: #64748b; stroke-width: 0.9; fill: none; opacity: 0.55; }\n");
    fprintf(f, "      .hash { fill: #111827; font: 9px monospace; letter-spacing: 0.5px; }\n");
    fprintf(f, "      .label { fill: #0f172a; font: 8px monospace; opacity: 0.68; }\n");
    fprintf(f, "    ]]></style>\n");
    for (u32 i = 0; i < module_count; i++) {
        fprintf(f, "    <clipPath id=\"module_clip_%03u\"><circle cx=\"256\" cy=\"256\" r=\"246\"/></clipPath>\n", i);
    }
    fprintf(f, "  </defs>\n");
    fprintf(f, "  <rect class=\"bg\" width=\"1200\" height=\"1200\"/>\n");
    fprintf(f, "  <g id=\"macro_foundation\">\n");
    fprintf(f, "    <circle class=\"macro-ring\" cx=\"600\" cy=\"600\" r=\"558\"/>\n");
    for (u32 d = 1; d <= max_depth; d++) {
        fprintf(f, "    <circle class=\"macro-ring\" cx=\"600\" cy=\"600\" r=\"%.2f\" opacity=\"%.2f\"/>\n",
                220.0 + (double)(d - 1) * 170.0,
                sg_clamp(0.24 - (double)d * 0.02, 0.10, 0.24));
    }
    fprintf(f, "  </g>\n");

    fprintf(f, "  <g id=\"imports\" class=\"macro-import\">\n");
    for (u32 i = 0; i < import_edge_count; i++) {
        u32 from = import_from_indices ? import_from_indices[i] : 0;
        u32 to = import_to_indices ? import_to_indices[i] : 0;
        if (from >= module_count || to >= module_count) continue;
        fprintf(f, "    <line x1=\"%.3f\" y1=\"%.3f\" x2=\"%.3f\" y2=\"%.3f\" />\n",
                nodes[from].x, nodes[from].y, nodes[to].x, nodes[to].y);
    }
    fprintf(f, "  </g>\n");

    fprintf(f, "  <g id=\"semantic_cross\" class=\"macro-sem\">\n");
    if (module_count > 1 && (cross_module_call_count > 0 || cross_module_type_ref_count > 0)) {
        u32 cross_draw_count = module_count - 1;
        for (u32 i = 1; i <= cross_draw_count; i++) {
            u32 idx = (i % module_count);
            fprintf(f, "    <line x1=\"%.3f\" y1=\"%.3f\" x2=\"%.3f\" y2=\"%.3f\" />\n",
                    nodes[0].x, nodes[0].y, nodes[idx].x, nodes[idx].y);
        }
    }
    fprintf(f, "  </g>\n");

    fprintf(f, "  <g id=\"module_sigils\">\n");
    for (u32 i = 0; i < module_count; i++) {
        double scale = (nodes[i].r * 2.0) / CCT_SIGILO_CANVAS_SIZE;
        double tx = nodes[i].x - nodes[i].r;
        double ty = nodes[i].y - nodes[i].r;
        fprintf(f, "    <g id=\"module_sigil_%03u\" transform=\"translate(%.3f %.3f) scale(%.8f)\">\n", i, tx, ty, scale);
        fprintf(f, "      <g clip-path=\"url(#module_clip_%03u)\">\n", i);
        if (module_models) {
            sg_svg_emit_core_layers(f, &module_models[i], sg, false);
        }
        fprintf(f, "      </g>\n");
        fprintf(f, "    </g>\n");
        fprintf(f, "    <circle class=\"%s\" cx=\"%.3f\" cy=\"%.3f\" r=\"%.3f\"/>\n",
                (i == 0) ? "module-entry-frame" : "module-frame",
                nodes[i].x, nodes[i].y, nodes[i].r);
        fprintf(f, "    <text class=\"module-label\" x=\"%.3f\" y=\"%.3f\" text-anchor=\"middle\">%s</text>\n",
                nodes[i].x, nodes[i].y + nodes[i].r + 16.0, sg_system_module_label(nodes[i].path));
        if (module_models) {
            char local_hex[17];
            sg_hex_u64(module_models[i].hash, local_hex);
            fprintf(f, "    <text class=\"module-hash\" x=\"%.3f\" y=\"%.3f\" text-anchor=\"middle\">%s</text>\n",
                    nodes[i].x, nodes[i].y + nodes[i].r + 28.0, local_hex);
        }
    }
    fprintf(f, "  </g>\n");

    fprintf(f, "  <text class=\"module-label\" x=\"24\" y=\"30\" font-size=\"14\">CCT system sigilo (sigil-of-sigils) • %s</text>\n", sg->semantic_hash_hex);
    fprintf(f, "  <text class=\"module-hash\" x=\"24\" y=\"50\" font-size=\"10\">entry=%s • modules=%u • imports=%u • calls=%u • type_refs=%u</text>\n",
            entry_path ? entry_path : "unknown",
            module_count,
            import_edge_count,
            cross_module_call_count,
            cross_module_type_ref_count);
    fprintf(f, "  <text class=\"module-hash\" x=\"24\" y=\"66\" font-size=\"10\">zoom: sub-sigilos vetoriais inline por módulo</text>\n");
    fprintf(f, "</svg>\n");
    fclose(f);
    free(nodes);
    free(depths);
    return true;
}

static bool sg_write_system_meta(
    cct_sigilo_t *sg,
    const char *meta_path,
    const char *entry_path,
    const cct_sigilo_model_t *module_models,
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
) {
    FILE *f = fopen(meta_path, "wb");
    if (!f) {
        sg_reportf(sg, 0, 0, "could not write system sigilo metadata: %s", meta_path);
        return false;
    }

    double module_density = (module_count > 0)
        ? ((double)import_edge_count / (double)module_count)
        : 0.0;
    u32 generic_inst_total = 0;
    u32 generic_inst_rituale_total = 0;
    u32 generic_inst_sigillum_total = 0;
    u32 generic_inst_dedup_total = 0;
    u32 generic_constraint_total = 0;
    u32 generic_constrained_param_total = 0;
    u32 generic_constraint_violation_total = 0;
    bool pactum_constraint_resolution_ok = true;
    u32 pactum_decl_total = 0;
    u32 pactum_signature_total = 0;
    u32 sigillum_pactum_conformance_total = 0;
    bool pactum_conformance_ok = true;
    u32 option_ops_total = 0;
    u32 result_ops_total = 0;
    u32 map_ops_total = 0;
    u32 set_ops_total = 0;
    u32 collection_ops_total = 0;
    u32 iterum_total = 0;
    if (module_models) {
        for (u32 i = 0; i < module_count; i++) {
            generic_inst_total += module_models[i].count_generic_instantiation;
            generic_inst_rituale_total += module_models[i].count_generic_rituale_instantiation;
            generic_inst_sigillum_total += module_models[i].count_generic_sigillum_instantiation;
            generic_inst_dedup_total += module_models[i].count_generic_instantiation_dedup;
            generic_constraint_total += module_models[i].count_generic_constraint;
            generic_constrained_param_total += module_models[i].count_generic_constrained_param;
            generic_constraint_violation_total += module_models[i].count_generic_constraint_violation;
            pactum_decl_total += module_models[i].count_pactum_decl;
            pactum_signature_total += module_models[i].count_pactum_signature;
            sigillum_pactum_conformance_total += module_models[i].count_sigillum_pactum_conformance;
            option_ops_total += module_models[i].count_option_ops;
            result_ops_total += module_models[i].count_result_ops;
            map_ops_total += module_models[i].count_map_ops;
            set_ops_total += module_models[i].count_set_ops;
            collection_ops_total += module_models[i].count_collection_ops;
            iterum_total += module_models[i].count_iterum;
            if (!module_models[i].pactum_conformance_ok) {
                pactum_conformance_ok = false;
            }
            if (!module_models[i].pactum_constraint_resolution_ok) {
                pactum_constraint_resolution_ok = false;
            }
        }
    }

    fprintf(f, "format = cct.sigil.v1\n");
    fprintf(f, "sigilo_scope = system\n");
    fprintf(f, "visual_style = %s\n", sg->style_name ? sg->style_name : "network");
    fprintf(f, "system_hash = %s\n", sg->semantic_hash_hex);
    fprintf(f, "module_count = %u\n", module_count);
    fprintf(f, "entry_module = %s\n", entry_path ? entry_path : "unknown");
    fprintf(f, "import_edges = %u\n", import_edge_count);
    fprintf(f, "cross_module_calls = %u\n", cross_module_call_count);
    fprintf(f, "cross_module_type_refs = %u\n", cross_module_type_ref_count);
    fprintf(f, "exported_symbol_count = %u\n", public_symbol_count);
    fprintf(f, "internal_symbol_count = %u\n", internal_symbol_count);
    fprintf(f, "module_density = %.6f\n", module_density);
    fprintf(f, "system_topology_class = %s\n", sg_system_topology_class(module_count, import_edge_count));
    fprintf(f, "module_visibility_model = public_default_arcanum_internal\n");
    fprintf(f, "module_resolution_status = %s\n", module_resolution_ok ? "ok" : "error");
    fprintf(f, "composition_mode = sigil_of_sigils_inline\n");
    fprintf(f, "generic_instantiation_count = %u\n", generic_inst_total);
    fprintf(f, "generic_rituale_instantiation_count = %u\n", generic_inst_rituale_total);
    fprintf(f, "generic_sigillum_instantiation_count = %u\n", generic_inst_sigillum_total);
    fprintf(f, "generic_instantiation_dedup_count = %u\n", generic_inst_dedup_total);
    fprintf(f, "generic_constraint_count = %u\n", generic_constraint_total);
    fprintf(f, "generic_constrained_param_count = %u\n", generic_constrained_param_total);
    fprintf(f, "generic_constraint_violation_count = %u\n", generic_constraint_violation_total);
    fprintf(f, "pactum_constraint_resolution_status = %s\n", pactum_constraint_resolution_ok ? "ok" : "error");
    fprintf(f, "generic_monomorphization_status = ok\n");
    fprintf(f, "pactum_decl_count = %u\n", pactum_decl_total);
    fprintf(f, "pactum_signature_count = %u\n", pactum_signature_total);
    fprintf(f, "sigillum_pactum_conformance_count = %u\n", sigillum_pactum_conformance_total);
    fprintf(f, "pactum_conformance_status = %s\n", pactum_conformance_ok ? "ok" : "error");
    fprintf(f, "option_ops_count = %u\n", option_ops_total);
    fprintf(f, "result_ops_count = %u\n", result_ops_total);
    fprintf(f, "map_ops_count = %u\n", map_ops_total);
    fprintf(f, "set_ops_count = %u\n", set_ops_total);
    fprintf(f, "collection_ops_count = %u\n", collection_ops_total);
    fprintf(f, "iterum_count = %u\n", iterum_total);
    fprintf(f, "option_module_used = %s\n", option_ops_total ? "true" : "false");
    fprintf(f, "result_module_used = %s\n", result_ops_total ? "true" : "false");
    fprintf(f, "map_module_used = %s\n", map_ops_total ? "true" : "false");
    fprintf(f, "set_module_used = %s\n", set_ops_total ? "true" : "false");
    fprintf(f, "collection_ops_module_used = %s\n", collection_ops_total ? "true" : "false");
    fprintf(f, "phase10_subset_final = true\n");
    fprintf(f, "phase10_final_status = consolidated\n");
    fprintf(f, "phase10_typing_contract = genus_explicit_monomorphization+pactum_explicit_conformance+constraint_single_contract_rituale_only\n");
    fprintf(f, "\n[modules]\n");
    for (u32 i = 0; i < module_count; i++) {
        fprintf(f, "%u = %s\n", i, (module_paths && module_paths[i]) ? module_paths[i] : "unknown");
    }
    fprintf(f, "\n[module_sigils]\n");
    for (u32 i = 0; i < module_count; i++) {
        char local_hex[17] = {0};
        if (module_models) sg_hex_u64(module_models[i].hash, local_hex);
        fprintf(f, "%u = %s | %s\n",
                i,
                (module_paths && module_paths[i]) ? module_paths[i] : "unknown",
                local_hex[0] ? local_hex : "0000000000000000");
    }
    fprintf(f, "\n[import_edges]\n");
    for (u32 i = 0; i < import_edge_count; i++) {
        u32 from = import_from_indices ? import_from_indices[i] : 0;
        u32 to = import_to_indices ? import_to_indices[i] : 0;
        const char *from_path = (module_paths && from < module_count) ? module_paths[from] : "unknown";
        const char *to_path = (module_paths && to < module_count) ? module_paths[to] : "unknown";
        fprintf(f, "%u = %s -> %s\n", i, from_path, to_path);
    }

    fclose(f);
    return true;
}

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
) {
    if (!sg || !entry_input_path || !module_programs) return false;
    if (!sg->emit_svg && !sg->emit_meta) {
        sg_reportf(sg, 0, 0, "sigilo generation requested with both SVG and metadata disabled");
        return false;
    }
    if (module_count == 0) module_count = 1;

    free(sg->input_path); sg->input_path = NULL;
    free(sg->svg_path); sg->svg_path = NULL;
    free(sg->sigil_meta_path); sg->sigil_meta_path = NULL;

    sg->input_path = sg_strdup(entry_input_path);
    sg->module_count = module_count;
    sg->import_edge_count = import_edge_count;
    sg->cross_module_call_count = cross_module_call_count;
    sg->cross_module_type_ref_count = cross_module_type_ref_count;
    sg->public_symbol_count = public_symbol_count;
    sg->internal_symbol_count = internal_symbol_count;
    sg->module_resolution_ok = module_resolution_ok;

    char *base = output_base_path ? sg_strdup(output_base_path) : sg_derive_output_base(entry_input_path);
    char *system_base = sg_strdup_printf("%s.system", base);
    free(base);

    if (sg->emit_svg) sg->svg_path = sg_strdup_printf("%s.svg", system_base);
    if (sg->emit_meta) sg->sigil_meta_path = sg_strdup_printf("%s.sigil", system_base);
    free(system_base);

    cct_sigilo_model_t *module_models = (cct_sigilo_model_t*)calloc(module_count, sizeof(*module_models));
    u64 *module_hashes = (u64*)calloc(module_count, sizeof(*module_hashes));
    if (!module_models || !module_hashes) {
        free(module_models);
        free(module_hashes);
        sg_reportf(sg, 0, 0, "out of memory while extracting module sigilo models");
        return false;
    }
    for (u32 i = 0; i < module_count; i++) {
        if (!module_programs[i]) {
            free(module_models);
            free(module_hashes);
            sg_reportf(sg, 0, 0, "missing module AST for system sigilo composition");
            return false;
        }
        if (!sg_extract_model(sg, module_programs[i], &module_models[i])) {
            for (u32 j = 0; j <= i; j++) {
                sg_free_model(&module_models[j]);
            }
            free(module_models);
            free(module_hashes);
            return false;
        }
        module_hashes[i] = module_models[i].hash;
    }

    sg->semantic_hash = sg_system_hash(
        module_paths,
        module_hashes,
        module_count,
        import_from_indices,
        import_to_indices,
        import_edge_count,
        cross_module_call_count,
        cross_module_type_ref_count,
        public_symbol_count,
        internal_symbol_count,
        module_resolution_ok
    );
    sg_hex_u64(sg->semantic_hash, sg->semantic_hash_hex);

    bool ok = true;
    if (sg->emit_svg) {
        ok = ok && sg_write_system_svg(
            sg,
            sg->svg_path,
            entry_input_path,
            module_models,
            module_paths,
            module_count,
            import_from_indices,
            import_to_indices,
            import_edge_count,
            cross_module_call_count,
            cross_module_type_ref_count
        );
    }
    if (sg->emit_meta) {
        ok = ok && sg_write_system_meta(
            sg,
            sg->sigil_meta_path,
            entry_input_path,
            module_models,
            module_paths,
            module_count,
            import_from_indices,
            import_to_indices,
            import_edge_count,
            cross_module_call_count,
            cross_module_type_ref_count,
            public_symbol_count,
            internal_symbol_count,
            module_resolution_ok
        );
    }

    if (ok) {
        printf("Sigilo style: %s\n", cct_sigilo_get_style(sg));
        printf("Sigilo hash: %s\n", sg->semantic_hash_hex);
        if (sg->emit_svg && sg->svg_path) printf("Sigil SVG: %s\n", sg->svg_path);
        if (sg->emit_meta && sg->sigil_meta_path) printf("Sigil Meta: %s\n", sg->sigil_meta_path);
    }

    for (u32 i = 0; i < module_count; i++) {
        sg_free_model(&module_models[i]);
    }
    free(module_models);
    free(module_hashes);

    return ok && !sg->had_error;
}
