/*
 * CCT — Clavicula Turing
 * AST Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "ast.h"
#include "../lexer/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * PART 1: Construction and Memory Management
 * ======================================================================== */

/* Helper: safe strdup */
static char* safe_strdup(const char *str) {
    if (!str) return NULL;
    char *copy = strdup(str);
    if (!copy) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    return copy;
}

/* Helper: allocate node */
static cct_ast_node_t* alloc_node(cct_ast_node_type_t type, u32 line, u32 col) {
    cct_ast_node_t *node = calloc(1, sizeof(cct_ast_node_t));
    if (!node) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    node->type = type;
    node->line = line;
    node->column = col;
    return node;
}

/* Create program */
cct_ast_program_t* cct_ast_create_program(const char *name) {
    cct_ast_program_t *prog = calloc(1, sizeof(cct_ast_program_t));
    if (!prog) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    prog->name = safe_strdup(name);
    prog->declarations = cct_ast_create_node_list();
    return prog;
}

/* Create lists */
cct_ast_node_list_t* cct_ast_create_node_list(void) {
    cct_ast_node_list_t *list = calloc(1, sizeof(cct_ast_node_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 8;
    list->nodes = calloc(list->capacity, sizeof(cct_ast_node_t*));
    if (!list->nodes) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_node_list_append(cct_ast_node_list_t *list, cct_ast_node_t *node) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->nodes = realloc(list->nodes, list->capacity * sizeof(cct_ast_node_t*));
        if (!list->nodes) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->nodes[list->count++] = node;
}

cct_ast_param_list_t* cct_ast_create_param_list(void) {
    cct_ast_param_list_t *list = calloc(1, sizeof(cct_ast_param_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 4;
    list->params = calloc(list->capacity, sizeof(cct_ast_param_t*));
    if (!list->params) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_param_list_append(cct_ast_param_list_t *list, cct_ast_param_t *param) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->params = realloc(list->params, list->capacity * sizeof(cct_ast_param_t*));
        if (!list->params) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->params[list->count++] = param;
}

cct_ast_field_list_t* cct_ast_create_field_list(void) {
    cct_ast_field_list_t *list = calloc(1, sizeof(cct_ast_field_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 4;
    list->fields = calloc(list->capacity, sizeof(cct_ast_field_t*));
    if (!list->fields) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_field_list_append(cct_ast_field_list_t *list, cct_ast_field_t *field) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->fields = realloc(list->fields, list->capacity * sizeof(cct_ast_field_t*));
        if (!list->fields) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->fields[list->count++] = field;
}

cct_ast_enum_item_list_t* cct_ast_create_enum_item_list(void) {
    cct_ast_enum_item_list_t *list = calloc(1, sizeof(cct_ast_enum_item_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 8;
    list->items = calloc(list->capacity, sizeof(cct_ast_enum_item_t*));
    if (!list->items) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_enum_item_list_append(cct_ast_enum_item_list_t *list, cct_ast_enum_item_t *item) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(cct_ast_enum_item_t*));
        if (!list->items) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->items[list->count++] = item;
}

cct_ast_ordo_variant_list_t* cct_ast_create_ordo_variant_list(void) {
    cct_ast_ordo_variant_list_t *list = calloc(1, sizeof(cct_ast_ordo_variant_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 8;
    list->variants = calloc(list->capacity, sizeof(cct_ast_ordo_variant_t*));
    if (!list->variants) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_ordo_variant_list_append(cct_ast_ordo_variant_list_t *list, cct_ast_ordo_variant_t *variant) {
    if (!list) return;
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->variants = realloc(list->variants, list->capacity * sizeof(cct_ast_ordo_variant_t*));
        if (!list->variants) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->variants[list->count++] = variant;
}

void cct_ast_ordo_variant_add_field(cct_ast_ordo_variant_t *variant, cct_ast_ordo_field_t *field) {
    if (!variant || !field) return;
    if (variant->field_count >= variant->field_capacity) {
        size_t next_capacity = (variant->field_capacity == 0) ? 4 : (variant->field_capacity * 2);
        variant->fields = realloc(variant->fields, next_capacity * sizeof(cct_ast_ordo_field_t*));
        if (!variant->fields) exit(CCT_ERROR_OUT_OF_MEMORY);
        variant->field_capacity = next_capacity;
    }
    variant->fields[variant->field_count++] = field;
}

cct_ast_type_param_list_t* cct_ast_create_type_param_list(void) {
    cct_ast_type_param_list_t *list = calloc(1, sizeof(cct_ast_type_param_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 4;
    list->params = calloc(list->capacity, sizeof(cct_ast_type_param_t*));
    if (!list->params) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_type_param_list_append(cct_ast_type_param_list_t *list, cct_ast_type_param_t *param) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->params = realloc(list->params, list->capacity * sizeof(cct_ast_type_param_t*));
        if (!list->params) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->params[list->count++] = param;
}

cct_ast_type_list_t* cct_ast_create_type_list(void) {
    cct_ast_type_list_t *list = calloc(1, sizeof(cct_ast_type_list_t));
    if (!list) exit(CCT_ERROR_OUT_OF_MEMORY);
    list->capacity = 4;
    list->types = calloc(list->capacity, sizeof(cct_ast_type_t*));
    if (!list->types) exit(CCT_ERROR_OUT_OF_MEMORY);
    return list;
}

void cct_ast_type_list_append(cct_ast_type_list_t *list, cct_ast_type_t *type) {
    if (!list) return;
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->types = realloc(list->types, list->capacity * sizeof(cct_ast_type_t*));
        if (!list->types) exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    list->types[list->count++] = type;
}

/* Create types */
cct_ast_type_t* cct_ast_create_type(const char *name) {
    cct_ast_type_t *type = calloc(1, sizeof(cct_ast_type_t));
    if (!type) exit(CCT_ERROR_OUT_OF_MEMORY);
    type->name = safe_strdup(name);
    return type;
}

cct_ast_type_t* cct_ast_create_pointer_type(cct_ast_type_t *base) {
    cct_ast_type_t *type = calloc(1, sizeof(cct_ast_type_t));
    if (!type) exit(CCT_ERROR_OUT_OF_MEMORY);
    type->is_pointer = true;
    type->element_type = base;
    return type;
}

cct_ast_type_t* cct_ast_create_array_type(cct_ast_type_t *base, u32 size) {
    cct_ast_type_t *type = calloc(1, sizeof(cct_ast_type_t));
    if (!type) exit(CCT_ERROR_OUT_OF_MEMORY);
    type->is_array = true;
    type->array_size = size;
    type->element_type = base;
    return type;
}

/* Create parameter, field, enum item */
cct_ast_param_t* cct_ast_create_param(const char *name, cct_ast_type_t *type, bool is_constans, u32 line, u32 col) {
    cct_ast_param_t *param = calloc(1, sizeof(cct_ast_param_t));
    if (!param) exit(CCT_ERROR_OUT_OF_MEMORY);
    param->name = safe_strdup(name);
    param->type = type;
    param->is_constans = is_constans;
    param->line = line;
    param->column = col;
    return param;
}

cct_ast_field_t* cct_ast_create_field(const char *name, cct_ast_type_t *type, u32 line, u32 col) {
    cct_ast_field_t *field = calloc(1, sizeof(cct_ast_field_t));
    if (!field) exit(CCT_ERROR_OUT_OF_MEMORY);
    field->name = safe_strdup(name);
    field->type = type;
    field->line = line;
    field->column = col;
    return field;
}

cct_ast_enum_item_t* cct_ast_create_enum_item(const char *name, i64 value, bool has_value, u32 line, u32 col) {
    cct_ast_enum_item_t *item = calloc(1, sizeof(cct_ast_enum_item_t));
    if (!item) exit(CCT_ERROR_OUT_OF_MEMORY);
    item->name = safe_strdup(name);
    item->value = value;
    item->has_value = has_value;
    item->line = line;
    item->column = col;
    return item;
}

cct_ast_ordo_field_t* cct_ast_create_ordo_field(const char *name, cct_ast_type_t *type, u32 line, u32 col) {
    cct_ast_ordo_field_t *field = calloc(1, sizeof(cct_ast_ordo_field_t));
    if (!field) exit(CCT_ERROR_OUT_OF_MEMORY);
    field->name = safe_strdup(name);
    field->type = type;
    field->line = line;
    field->column = col;
    return field;
}

cct_ast_ordo_variant_t* cct_ast_create_ordo_variant(const char *name, bool has_value, i64 value, u32 line, u32 col) {
    cct_ast_ordo_variant_t *variant = calloc(1, sizeof(cct_ast_ordo_variant_t));
    if (!variant) exit(CCT_ERROR_OUT_OF_MEMORY);
    variant->name = safe_strdup(name);
    variant->has_value = has_value;
    variant->value = value;
    variant->line = line;
    variant->column = col;
    return variant;
}

cct_ast_type_param_t* cct_ast_create_type_param(const char *name, const char *constraint_pactum_name, u32 line, u32 col) {
    cct_ast_type_param_t *param = calloc(1, sizeof(cct_ast_type_param_t));
    if (!param) exit(CCT_ERROR_OUT_OF_MEMORY);
    param->name = safe_strdup(name);
    param->constraint_pactum_name = safe_strdup(constraint_pactum_name);
    param->line = line;
    param->column = col;
    return param;
}

/* ========================================================================
 * PART 2: Node Creation Functions
 * ======================================================================== */

/* Top-level declarations */
cct_ast_node_t* cct_ast_create_import(const char *filename, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_IMPORT, line, col);
    node->as.import.filename = safe_strdup(filename);
    return node;
}

cct_ast_node_t* cct_ast_create_rituale(const char *name, cct_ast_type_param_list_t *type_params,
                                       cct_ast_param_list_t *params, cct_ast_type_t *return_type,
                                       cct_ast_node_t *body, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_RITUALE, line, col);
    node->as.rituale.name = safe_strdup(name);
    node->as.rituale.type_params = type_params;
    node->as.rituale.params = params;
    node->as.rituale.return_type = return_type;
    node->as.rituale.body = body;
    return node;
}

cct_ast_node_t* cct_ast_create_codex(const char *name, cct_ast_node_list_t *decls, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_CODEX, line, col);
    node->as.codex.name = safe_strdup(name);
    node->as.codex.declarations = decls;
    return node;
}

cct_ast_node_t* cct_ast_create_sigillum(const char *name, cct_ast_type_param_list_t *type_params,
                                        const char *pactum_name,
                                        cct_ast_field_list_t *fields, cct_ast_node_list_t *methods, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_SIGILLUM, line, col);
    node->as.sigillum.name = safe_strdup(name);
    node->as.sigillum.type_params = type_params;
    node->as.sigillum.pactum_name = safe_strdup(pactum_name);
    node->as.sigillum.fields = fields;
    node->as.sigillum.methods = methods;
    return node;
}

cct_ast_node_t* cct_ast_create_ordo(const char *name, cct_ast_enum_item_list_t *items, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_ORDO, line, col);
    node->as.ordo.name = safe_strdup(name);
    node->as.ordo.variants = NULL;
    node->as.ordo.items = items;
    node->as.ordo.has_payload = false;
    return node;
}

cct_ast_node_t* cct_ast_create_ordo_with_variants(const char *name, cct_ast_ordo_variant_list_t *variants,
                                                  cct_ast_enum_item_list_t *items, bool has_payload,
                                                  u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_ORDO, line, col);
    node->as.ordo.name = safe_strdup(name);
    node->as.ordo.variants = variants;
    node->as.ordo.items = items;
    node->as.ordo.has_payload = has_payload;
    return node;
}

cct_ast_node_t* cct_ast_create_pactum(const char *name, cct_ast_node_list_t *sigs, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_PACTUM, line, col);
    node->as.pactum.name = safe_strdup(name);
    node->as.pactum.signatures = sigs;
    return node;
}

/* Statements */
cct_ast_node_t* cct_ast_create_block(cct_ast_node_list_t *stmts, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_BLOCK, line, col);
    node->as.block.statements = stmts;
    return node;
}

cct_ast_node_t* cct_ast_create_evoca(cct_ast_type_t *type, const char *name, cct_ast_node_t *init, bool is_constans, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_EVOCA, line, col);
    node->as.evoca.var_type = type;
    node->as.evoca.name = safe_strdup(name);
    node->as.evoca.initializer = init;
    node->as.evoca.is_constans = is_constans;
    return node;
}

cct_ast_node_t* cct_ast_create_vincire(cct_ast_node_t *target, cct_ast_node_t *value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_VINCIRE, line, col);
    node->as.vincire.target = target;
    node->as.vincire.value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_redde(cct_ast_node_t *value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_REDDE, line, col);
    node->as.redde.value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_si(cct_ast_node_t *cond, cct_ast_node_t *then_br, cct_ast_node_t *else_br, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_SI, line, col);
    node->as.si.condition = cond;
    node->as.si.then_branch = then_br;
    node->as.si.else_branch = else_br;
    return node;
}

cct_ast_node_t* cct_ast_create_quando(cct_ast_node_t *expr, cct_ast_case_node_t *cases, size_t case_count,
                                      cct_ast_node_t *else_body, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_QUANDO, line, col);
    node->as.quando.expression = expr;
    node->as.quando.cases = cases;
    node->as.quando.case_count = case_count;
    node->as.quando.else_body = else_body;
    return node;
}

cct_ast_node_t* cct_ast_create_dum(cct_ast_node_t *cond, cct_ast_node_t *body, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_DUM, line, col);
    node->as.dum.condition = cond;
    node->as.dum.body = body;
    return node;
}

cct_ast_node_t* cct_ast_create_donec(cct_ast_node_t *body, cct_ast_node_t *cond, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_DONEC, line, col);
    node->as.donec.body = body;
    node->as.donec.condition = cond;
    return node;
}

cct_ast_node_t* cct_ast_create_repete(const char *iter, cct_ast_node_t *start, cct_ast_node_t *end,
                                      cct_ast_node_t *step, cct_ast_node_t *body, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_REPETE, line, col);
    node->as.repete.iterator = safe_strdup(iter);
    node->as.repete.start = start;
    node->as.repete.end = end;
    node->as.repete.step = step;
    node->as.repete.body = body;
    return node;
}

cct_ast_node_t* cct_ast_create_iterum(const char *item_name, const char *value_name,
                                      cct_ast_node_t *collection,
                                      cct_ast_node_t *body, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_ITERUM, line, col);
    node->as.iterum.item_name = safe_strdup(item_name);
    node->as.iterum.value_name = safe_strdup(value_name);
    node->as.iterum.collection = collection;
    node->as.iterum.body = body;
    return node;
}

cct_ast_node_t* cct_ast_create_tempta(cct_ast_node_t *try_block, cct_ast_type_t *cape_type,
                                      const char *cape_name, cct_ast_node_t *cape_block,
                                      cct_ast_node_t *semper_block, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_TEMPTA, line, col);
    node->as.tempta.try_block = try_block;
    node->as.tempta.cape_type = cape_type;
    node->as.tempta.cape_name = safe_strdup(cape_name);
    node->as.tempta.cape_block = cape_block;
    node->as.tempta.semper_block = semper_block;
    return node;
}

cct_ast_node_t* cct_ast_create_iace(cct_ast_node_t *value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_IACE, line, col);
    node->as.iace.value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_frange(u32 line, u32 col) {
    return alloc_node(AST_FRANGE, line, col);
}

cct_ast_node_t* cct_ast_create_recede(u32 line, u32 col) {
    return alloc_node(AST_RECEDE, line, col);
}

cct_ast_node_t* cct_ast_create_transitus(const char *label, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_TRANSITUS, line, col);
    node->as.transitus.label = safe_strdup(label);
    return node;
}

cct_ast_node_t* cct_ast_create_anur(cct_ast_node_t *value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_ANUR, line, col);
    node->as.anur.value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_dimitte(cct_ast_node_t *target, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_DIMITTE, line, col);
    node->as.dimitte.target = target;
    return node;
}

cct_ast_node_t* cct_ast_create_expr_stmt(cct_ast_node_t *expr, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_EXPR_STMT, line, col);
    node->as.expr_stmt.expression = expr;
    return node;
}

/* Literals and identifiers */
cct_ast_node_t* cct_ast_create_literal_int(i64 value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_LITERAL_INT, line, col);
    node->as.literal_int.int_value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_literal_real(f64 value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_LITERAL_REAL, line, col);
    node->as.literal_real.real_value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_literal_string(const char *value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_LITERAL_STRING, line, col);
    node->as.literal_string.string_value = safe_strdup(value);
    return node;
}

cct_ast_node_t* cct_ast_create_literal_bool(bool value, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_LITERAL_BOOL, line, col);
    node->as.literal_bool.bool_value = value;
    return node;
}

cct_ast_node_t* cct_ast_create_literal_nihil(u32 line, u32 col) {
    return alloc_node(AST_LITERAL_NIHIL, line, col);
}

cct_ast_node_t* cct_ast_create_molde(cct_ast_molde_part_t *parts, size_t part_count, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_MOLDE, line, col);
    node->as.molde.parts = parts;
    node->as.molde.part_count = part_count;
    return node;
}

cct_ast_node_t* cct_ast_create_identifier(const char *name, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_IDENTIFIER, line, col);
    node->as.identifier.name = safe_strdup(name);
    return node;
}

/* Expressions */
cct_ast_node_t* cct_ast_create_binary_op(cct_token_type_t op, cct_ast_node_t *left, cct_ast_node_t *right, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_BINARY_OP, line, col);
    node->as.binary_op.operator = op;
    node->as.binary_op.left = left;
    node->as.binary_op.right = right;
    return node;
}

cct_ast_node_t* cct_ast_create_unary_op(cct_token_type_t op, cct_ast_node_t *operand, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_UNARY_OP, line, col);
    node->as.unary_op.operator = op;
    node->as.unary_op.operand = operand;
    return node;
}

cct_ast_node_t* cct_ast_create_call(cct_ast_node_t *callee, cct_ast_node_list_t *args, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_CALL, line, col);
    node->as.call.callee = callee;
    node->as.call.arguments = args;
    return node;
}

cct_ast_node_t* cct_ast_create_coniura(const char *name, cct_ast_type_list_t *type_args,
                                       cct_ast_node_list_t *args, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_CONIURA, line, col);
    node->as.coniura.name = safe_strdup(name);
    node->as.coniura.type_args = type_args;
    node->as.coniura.arguments = args;
    return node;
}

cct_ast_node_t* cct_ast_create_obsecro(const char *name, cct_ast_node_list_t *args, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_OBSECRO, line, col);
    node->as.obsecro.name = safe_strdup(name);
    node->as.obsecro.arguments = args;
    return node;
}

cct_ast_node_t* cct_ast_create_field_access(cct_ast_node_t *obj, const char *field, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_FIELD_ACCESS, line, col);
    node->as.field_access.object = obj;
    node->as.field_access.field = safe_strdup(field);
    return node;
}

cct_ast_node_t* cct_ast_create_index_access(cct_ast_node_t *arr, cct_ast_node_t *index, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_INDEX_ACCESS, line, col);
    node->as.index_access.array = arr;
    node->as.index_access.index = index;
    return node;
}

cct_ast_node_t* cct_ast_create_mensura(cct_ast_type_t *type, u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_MENSURA, line, col);
    node->as.mensura.type = type;
    return node;
}

/* ========================================================================
 * PART 3: Destruction/Free Functions
 * ======================================================================== */

/* Free type */
void cct_ast_free_type(cct_ast_type_t *type) {
    if (!type) return;
    free(type->name);
    if (type->generic_args) {
        cct_ast_free_type_list(type->generic_args);
    }
    if (type->element_type) {
        cct_ast_free_type(type->element_type);
    }
    free(type);
}

/* Free lists */
void cct_ast_free_node_list(cct_ast_node_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        cct_ast_free_node(list->nodes[i]);
    }
    free(list->nodes);
    free(list);
}

void cct_ast_free_param_list(cct_ast_param_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        free(list->params[i]->name);
        cct_ast_free_type(list->params[i]->type);
        free(list->params[i]);
    }
    free(list->params);
    free(list);
}

void cct_ast_free_field_list(cct_ast_field_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        free(list->fields[i]->name);
        cct_ast_free_type(list->fields[i]->type);
        free(list->fields[i]);
    }
    free(list->fields);
    free(list);
}

void cct_ast_free_enum_item_list(cct_ast_enum_item_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        free(list->items[i]->name);
        free(list->items[i]);
    }
    free(list->items);
    free(list);
}

void cct_ast_free_ordo_variant_list(cct_ast_ordo_variant_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        cct_ast_ordo_variant_t *variant = list->variants[i];
        if (!variant) continue;
        free(variant->name);
        if (variant->fields) {
            for (u32 j = 0; j < variant->field_count; j++) {
                cct_ast_ordo_field_t *field = variant->fields[j];
                if (!field) continue;
                free(field->name);
                cct_ast_free_type(field->type);
                free(field);
            }
        }
        free(variant->fields);
        free(variant);
    }
    free(list->variants);
    free(list);
}

void cct_ast_free_type_param_list(cct_ast_type_param_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        if (!list->params[i]) continue;
        free(list->params[i]->name);
        free(list->params[i]->constraint_pactum_name);
        free(list->params[i]);
    }
    free(list->params);
    free(list);
}

void cct_ast_free_type_list(cct_ast_type_list_t *list) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        cct_ast_free_type(list->types[i]);
    }
    free(list->types);
    free(list);
}

/* Free program */
void cct_ast_free_program(cct_ast_program_t *prog) {
    if (!prog) return;
    free(prog->name);
    cct_ast_free_node_list(prog->declarations);
    free(prog);
}

/* Free node - comprehensive */
void cct_ast_free_node(cct_ast_node_t *node) {
    if (!node) return;

    switch (node->type) {
        case AST_IMPORT:
            free(node->as.import.filename);
            break;

        case AST_RITUALE:
            free(node->as.rituale.name);
            cct_ast_free_type_param_list(node->as.rituale.type_params);
            cct_ast_free_param_list(node->as.rituale.params);
            cct_ast_free_type(node->as.rituale.return_type);
            cct_ast_free_node(node->as.rituale.body);
            break;

        case AST_CODEX:
            free(node->as.codex.name);
            cct_ast_free_node_list(node->as.codex.declarations);
            break;

        case AST_SIGILLUM:
            free(node->as.sigillum.name);
            cct_ast_free_type_param_list(node->as.sigillum.type_params);
            free(node->as.sigillum.pactum_name);
            cct_ast_free_field_list(node->as.sigillum.fields);
            cct_ast_free_node_list(node->as.sigillum.methods);
            break;

        case AST_ORDO:
            free(node->as.ordo.name);
            cct_ast_free_ordo_variant_list(node->as.ordo.variants);
            cct_ast_free_enum_item_list(node->as.ordo.items);
            break;

        case AST_PACTUM:
            free(node->as.pactum.name);
            cct_ast_free_node_list(node->as.pactum.signatures);
            break;

        case AST_BLOCK:
            cct_ast_free_node_list(node->as.block.statements);
            break;

        case AST_EVOCA:
            cct_ast_free_type(node->as.evoca.var_type);
            free(node->as.evoca.name);
            cct_ast_free_node(node->as.evoca.initializer);
            break;

        case AST_VINCIRE:
            cct_ast_free_node(node->as.vincire.target);
            cct_ast_free_node(node->as.vincire.value);
            break;

        case AST_REDDE:
            cct_ast_free_node(node->as.redde.value);
            break;

        case AST_SI:
            cct_ast_free_node(node->as.si.condition);
            cct_ast_free_node(node->as.si.then_branch);
            cct_ast_free_node(node->as.si.else_branch);
            break;

        case AST_QUANDO:
            cct_ast_free_node(node->as.quando.expression);
            if (node->as.quando.cases) {
                for (size_t i = 0; i < node->as.quando.case_count; i++) {
                    cct_ast_case_node_t *case_node = &node->as.quando.cases[i];
                    if (case_node->literals) {
                        for (size_t j = 0; j < case_node->literal_count; j++) {
                            cct_ast_free_node(case_node->literals[j]);
                        }
                        free(case_node->literals);
                    }
                    if (case_node->binding_names) {
                        for (size_t j = 0; j < case_node->binding_count; j++) {
                            free(case_node->binding_names[j]);
                        }
                        free(case_node->binding_names);
                    }
                    cct_ast_free_node(case_node->body);
                }
                free(node->as.quando.cases);
            }
            cct_ast_free_node(node->as.quando.else_body);
            break;

        case AST_DUM:
            cct_ast_free_node(node->as.dum.condition);
            cct_ast_free_node(node->as.dum.body);
            break;

        case AST_DONEC:
            cct_ast_free_node(node->as.donec.body);
            cct_ast_free_node(node->as.donec.condition);
            break;

        case AST_REPETE:
            free(node->as.repete.iterator);
            cct_ast_free_node(node->as.repete.start);
            cct_ast_free_node(node->as.repete.end);
            cct_ast_free_node(node->as.repete.step);
            cct_ast_free_node(node->as.repete.body);
            break;

        case AST_ITERUM:
            free(node->as.iterum.item_name);
            free(node->as.iterum.value_name);
            cct_ast_free_node(node->as.iterum.collection);
            cct_ast_free_node(node->as.iterum.body);
            break;

        case AST_TEMPTA:
            cct_ast_free_node(node->as.tempta.try_block);
            cct_ast_free_type(node->as.tempta.cape_type);
            free(node->as.tempta.cape_name);
            cct_ast_free_node(node->as.tempta.cape_block);
            cct_ast_free_node(node->as.tempta.semper_block);
            break;

        case AST_IACE:
            cct_ast_free_node(node->as.iace.value);
            break;

        case AST_FRANGE:
        case AST_RECEDE:
            // No additional fields to free
            break;

        case AST_TRANSITUS:
            free(node->as.transitus.label);
            break;

        case AST_ANUR:
            cct_ast_free_node(node->as.anur.value);
            break;

        case AST_DIMITTE:
            cct_ast_free_node(node->as.dimitte.target);
            break;

        case AST_EXPR_STMT:
            cct_ast_free_node(node->as.expr_stmt.expression);
            break;

        case AST_LITERAL_INT:
        case AST_LITERAL_REAL:
        case AST_LITERAL_BOOL:
        case AST_LITERAL_NIHIL:
            // No additional fields to free
            break;

        case AST_MOLDE:
            if (node->as.molde.parts) {
                for (size_t i = 0; i < node->as.molde.part_count; i++) {
                    cct_ast_molde_part_t *part = &node->as.molde.parts[i];
                    free(part->literal_text);
                    free(part->fmt_spec);
                    cct_ast_free_node(part->expr);
                }
                free(node->as.molde.parts);
            }
            break;

        case AST_LITERAL_STRING:
            free(node->as.literal_string.string_value);
            break;

        case AST_IDENTIFIER:
            free(node->as.identifier.name);
            free(node->as.identifier.ordo_name);
            free(node->as.identifier.variant_name);
            break;

        case AST_BINARY_OP:
            cct_ast_free_node(node->as.binary_op.left);
            cct_ast_free_node(node->as.binary_op.right);
            break;

        case AST_UNARY_OP:
            cct_ast_free_node(node->as.unary_op.operand);
            break;

        case AST_CALL:
            cct_ast_free_node(node->as.call.callee);
            cct_ast_free_node_list(node->as.call.arguments);
            free(node->as.call.ordo_name);
            free(node->as.call.variant_name);
            break;

        case AST_CONIURA:
            free(node->as.coniura.name);
            cct_ast_free_type_list(node->as.coniura.type_args);
            cct_ast_free_node_list(node->as.coniura.arguments);
            break;

        case AST_OBSECRO:
            free(node->as.obsecro.name);
            cct_ast_free_node_list(node->as.obsecro.arguments);
            break;

        case AST_FIELD_ACCESS:
            cct_ast_free_node(node->as.field_access.object);
            free(node->as.field_access.field);
            break;

        case AST_INDEX_ACCESS:
            cct_ast_free_node(node->as.index_access.array);
            cct_ast_free_node(node->as.index_access.index);
            break;

        case AST_MENSURA:
            cct_ast_free_type(node->as.mensura.type);
            break;

        default:
            // Unknown node type - should never happen
            break;
    }

    free(node);
}

/* ========================================================================
 * PART 4: Printing/Display Functions
 * ======================================================================== */

/* Helper: print indentation */
static void print_indent(u32 depth) {
    for (u32 i = 0; i < depth; i++) {
        printf("  ");
    }
}

static void print_type_inline(const cct_ast_type_t *type) {
    if (!type) {
        printf("(null)");
        return;
    }

    if (type->is_pointer) {
        printf("SPECULUM ");
        print_type_inline(type->element_type);
        return;
    }

    if (type->is_array) {
        printf("SERIES ");
        print_type_inline(type->element_type);
        printf("[%u]", type->array_size);
        return;
    }

    printf("%s", type->name ? type->name : "(anon)");
    if (type->generic_args && type->generic_args->count > 0) {
        printf(" GENUS(");
        for (size_t i = 0; i < type->generic_args->count; i++) {
            if (i > 0) printf(", ");
            print_type_inline(type->generic_args->types[i]);
        }
        printf(")");
    }
}

/* Helper: get node type name */
static const char* get_node_type_name(cct_ast_node_type_t type) {
    switch (type) {
        case AST_IMPORT: return "IMPORT";
        case AST_RITUALE: return "RITUALE";
        case AST_CODEX: return "CODEX";
        case AST_SIGILLUM: return "SIGILLUM";
        case AST_ORDO: return "ORDO";
        case AST_PACTUM: return "PACTUM";
        case AST_BLOCK: return "BLOCK";
        case AST_EVOCA: return "EVOCA";
        case AST_VINCIRE: return "VINCIRE";
        case AST_REDDE: return "REDDE";
        case AST_SI: return "SI";
        case AST_QUANDO: return "QUANDO";
        case AST_DUM: return "DUM";
        case AST_DONEC: return "DONEC";
        case AST_REPETE: return "REPETE";
        case AST_ITERUM: return "ITERUM";
        case AST_TEMPTA: return "TEMPTA";
        case AST_IACE: return "IACE";
        case AST_FRANGE: return "FRANGE";
        case AST_RECEDE: return "RECEDE";
        case AST_TRANSITUS: return "TRANSITUS";
        case AST_ANUR: return "ANUR";
        case AST_DIMITTE: return "DIMITTE";
        case AST_EXPR_STMT: return "EXPR_STMT";
        case AST_LITERAL_INT: return "LITERAL_INT";
        case AST_LITERAL_REAL: return "LITERAL_REAL";
        case AST_LITERAL_STRING: return "LITERAL_STRING";
        case AST_LITERAL_BOOL: return "LITERAL_BOOL";
        case AST_LITERAL_NIHIL: return "LITERAL_NIHIL";
        case AST_MOLDE: return "MOLDE";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_BINARY_OP: return "BINARY_OP";
        case AST_UNARY_OP: return "UNARY_OP";
        case AST_CALL: return "CALL";
        case AST_CONIURA: return "CONIURA";
        case AST_OBSECRO: return "OBSECRO";
        case AST_FIELD_ACCESS: return "FIELD_ACCESS";
        case AST_INDEX_ACCESS: return "INDEX_ACCESS";
        case AST_MENSURA: return "MENSURA";
        default: return "UNKNOWN";
    }
}

const char* cct_ast_node_type_string(cct_ast_node_type_t type) {
    return get_node_type_name(type);
}

/* Print type */
static void print_type(cct_ast_type_t *type, u32 depth) {
    if (!type) {
        print_indent(depth);
        printf("Type: (null)\n");
        return;
    }
    print_indent(depth);
    printf("Type: ");
    print_type_inline(type);
    printf("\n");
}

/* Forward declaration */
static void print_node(cct_ast_node_t *node, u32 depth);

/* Print node list */
static void print_node_list(cct_ast_node_list_t *list, u32 depth) {
    if (!list) return;
    for (u32 i = 0; i < list->count; i++) {
        print_node(list->nodes[i], depth);
    }
}

/* Print param list */
static void print_param_list(cct_ast_param_list_t *list, u32 depth) {
    if (!list) return;
    print_indent(depth);
    printf("Parameters: (%zu)\n", list->count);
    for (u32 i = 0; i < list->count; i++) {
        print_indent(depth + 1);
        if (list->params[i]->is_constans) {
            printf("- %s: CONSTANS ", list->params[i]->name);
        } else {
            printf("- %s: ", list->params[i]->name);
        }
        print_type_inline(list->params[i]->type);
        printf("\n");
    }
}

/* Print field list */
static void print_field_list(cct_ast_field_list_t *list, u32 depth) {
    if (!list) return;
    print_indent(depth);
    printf("Fields: (%zu)\n", list->count);
    for (u32 i = 0; i < list->count; i++) {
        print_indent(depth + 1);
        printf("- %s: ", list->fields[i]->name);
        print_type_inline(list->fields[i]->type);
        printf("\n");
    }
}

/* Print enum item list */
static void print_enum_item_list(cct_ast_enum_item_list_t *list, u32 depth) {
    if (!list) return;
    print_indent(depth);
    printf("Items: (%zu)\n", list->count);
    for (u32 i = 0; i < list->count; i++) {
        print_indent(depth + 1);
        printf("- %s", list->items[i]->name);
        if (list->items[i]->has_value) {
            printf(" = %lld", (long long)list->items[i]->value);
        }
        printf("\n");
    }
}

static void print_ordo_variant_list(cct_ast_ordo_variant_list_t *list, u32 depth) {
    if (!list) return;
    print_indent(depth);
    printf("Variants: (%zu)\n", list->count);
    for (u32 i = 0; i < list->count; i++) {
        cct_ast_ordo_variant_t *variant = list->variants[i];
        if (!variant) continue;
        print_indent(depth + 1);
        printf("- %s (tag=%lld", variant->name ? variant->name : "<anon>",
               (long long)variant->tag_value);
        if (variant->has_value) {
            printf(", value=%lld", (long long)variant->value);
        }
        printf(")\n");
        if (variant->field_count > 0) {
            print_indent(depth + 2);
            printf("Payload fields: (%zu)\n", variant->field_count);
            for (u32 j = 0; j < variant->field_count; j++) {
                cct_ast_ordo_field_t *field = variant->fields[j];
                if (!field) continue;
                print_indent(depth + 3);
                printf("- %s: ", field->name ? field->name : "<field>");
                print_type_inline(field->type);
                printf("\n");
            }
        }
    }
}

static void print_type_param_list(cct_ast_type_param_list_t *list, u32 depth) {
    if (!list || list->count == 0) return;
    print_indent(depth);
    printf("GENUS Parameters: (%zu)\n", list->count);
    for (u32 i = 0; i < list->count; i++) {
        print_indent(depth + 1);
        if (!list->params[i]) {
            printf("- (null)\n");
            continue;
        }
        if (list->params[i]->constraint_pactum_name && list->params[i]->constraint_pactum_name[0]) {
            printf("- %s PACTUM %s\n", list->params[i]->name, list->params[i]->constraint_pactum_name);
        } else {
            printf("- %s\n", list->params[i]->name);
        }
    }
}

/* Print node - comprehensive */
static void print_node(cct_ast_node_t *node, u32 depth) {
    if (!node) return;

    print_indent(depth);
    printf("%s (line %u, col %u)\n", get_node_type_name(node->type), node->line, node->column);

    switch (node->type) {
        case AST_IMPORT:
            print_indent(depth + 1);
            printf("Filename: \"%s\"\n", node->as.import.filename);
            break;

        case AST_RITUALE:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.rituale.name);
            if (node->is_internal) {
                print_indent(depth + 1);
                printf("Visibility: ARCANUM (internal)\n");
            }
            print_type_param_list(node->as.rituale.type_params, depth + 1);
            print_param_list(node->as.rituale.params, depth + 1);
            print_type(node->as.rituale.return_type, depth + 1);
            if (node->as.rituale.body) {
                print_indent(depth + 1);
                printf("Body:\n");
                print_node(node->as.rituale.body, depth + 2);
            }
            break;

        case AST_CODEX:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.codex.name);
            print_indent(depth + 1);
            printf("Declarations:\n");
            print_node_list(node->as.codex.declarations, depth + 2);
            break;

        case AST_SIGILLUM:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.sigillum.name);
            if (node->as.sigillum.pactum_name) {
                print_indent(depth + 1);
                printf("Conforms PACTUM: %s\n", node->as.sigillum.pactum_name);
            }
            if (node->is_internal) {
                print_indent(depth + 1);
                printf("Visibility: ARCANUM (internal)\n");
            }
            print_type_param_list(node->as.sigillum.type_params, depth + 1);
            print_field_list(node->as.sigillum.fields, depth + 1);
            if (node->as.sigillum.methods && node->as.sigillum.methods->count > 0) {
                print_indent(depth + 1);
                printf("Methods:\n");
                print_node_list(node->as.sigillum.methods, depth + 2);
            }
            break;

        case AST_ORDO:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.ordo.name);
            if (node->is_internal) {
                print_indent(depth + 1);
                printf("Visibility: ARCANUM (internal)\n");
            }
            if (node->as.ordo.variants) {
                print_ordo_variant_list(node->as.ordo.variants, depth + 1);
            } else {
                print_enum_item_list(node->as.ordo.items, depth + 1);
            }
            break;

        case AST_PACTUM:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.pactum.name);
            print_indent(depth + 1);
            printf("Signatures:\n");
            print_node_list(node->as.pactum.signatures, depth + 2);
            break;

        case AST_BLOCK:
            print_indent(depth + 1);
            printf("Statements: (%zu)\n",
                   node->as.block.statements ? node->as.block.statements->count : 0);
            print_node_list(node->as.block.statements, depth + 2);
            break;

        case AST_EVOCA:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.evoca.name);
            if (node->as.evoca.is_constans) {
                print_indent(depth + 1);
                printf("Qualifier: CONSTANS\n");
            }
            print_type(node->as.evoca.var_type, depth + 1);
            if (node->as.evoca.initializer) {
                print_indent(depth + 1);
                printf("Initializer:\n");
                print_node(node->as.evoca.initializer, depth + 2);
            }
            break;

        case AST_VINCIRE:
            print_indent(depth + 1);
            printf("Target:\n");
            print_node(node->as.vincire.target, depth + 2);
            print_indent(depth + 1);
            printf("Value:\n");
            print_node(node->as.vincire.value, depth + 2);
            break;

        case AST_REDDE:
            if (node->as.redde.value) {
                print_indent(depth + 1);
                printf("Value:\n");
                print_node(node->as.redde.value, depth + 2);
            }
            break;

        case AST_SI:
            print_indent(depth + 1);
            printf("Condition:\n");
            print_node(node->as.si.condition, depth + 2);
            print_indent(depth + 1);
            printf("Then:\n");
            print_node(node->as.si.then_branch, depth + 2);
            if (node->as.si.else_branch) {
                print_indent(depth + 1);
                printf("Else:\n");
                print_node(node->as.si.else_branch, depth + 2);
            }
            break;

        case AST_QUANDO:
            print_indent(depth + 1);
            printf("Expression:\n");
            print_node(node->as.quando.expression, depth + 2);
            print_indent(depth + 1);
            printf("Cases: (%zu)\n", node->as.quando.case_count);
            for (size_t i = 0; i < node->as.quando.case_count; i++) {
                cct_ast_case_node_t *case_node = &node->as.quando.cases[i];
                print_indent(depth + 2);
                printf("Case %zu literals: (%zu)\n", i + 1, case_node->literal_count);
                for (size_t j = 0; j < case_node->literal_count; j++) {
                    print_node(case_node->literals[j], depth + 3);
                }
                if (case_node->binding_count > 0 && case_node->binding_names) {
                    print_indent(depth + 2);
                    printf("Bindings: (%zu)\n", case_node->binding_count);
                    for (size_t j = 0; j < case_node->binding_count; j++) {
                        print_indent(depth + 3);
                        printf("- %s\n", case_node->binding_names[j] ? case_node->binding_names[j] : "<anon>");
                    }
                }
                print_indent(depth + 2);
                printf("Body:\n");
                print_node(case_node->body, depth + 3);
            }
            if (node->as.quando.else_body) {
                print_indent(depth + 1);
                printf("Else:\n");
                print_node(node->as.quando.else_body, depth + 2);
            }
            break;

        case AST_DUM:
            print_indent(depth + 1);
            printf("Condition:\n");
            print_node(node->as.dum.condition, depth + 2);
            print_indent(depth + 1);
            printf("Body:\n");
            print_node(node->as.dum.body, depth + 2);
            break;

        case AST_DONEC:
            print_indent(depth + 1);
            printf("Body:\n");
            print_node(node->as.donec.body, depth + 2);
            print_indent(depth + 1);
            printf("Condition:\n");
            print_node(node->as.donec.condition, depth + 2);
            break;

        case AST_REPETE:
            print_indent(depth + 1);
            printf("Iterator: %s\n", node->as.repete.iterator);
            print_indent(depth + 1);
            printf("Start:\n");
            print_node(node->as.repete.start, depth + 2);
            print_indent(depth + 1);
            printf("End:\n");
            print_node(node->as.repete.end, depth + 2);
            if (node->as.repete.step) {
                print_indent(depth + 1);
                printf("Step:\n");
                print_node(node->as.repete.step, depth + 2);
            }
            print_indent(depth + 1);
            printf("Body:\n");
            print_node(node->as.repete.body, depth + 2);
            break;

        case AST_ITERUM:
            print_indent(depth + 1);
            printf("Item: %s\n", node->as.iterum.item_name ? node->as.iterum.item_name : "(null)");
            if (node->as.iterum.value_name) {
                print_indent(depth + 1);
                printf("Value: %s\n", node->as.iterum.value_name);
            }
            print_indent(depth + 1);
            printf("Collection:\n");
            print_node(node->as.iterum.collection, depth + 2);
            print_indent(depth + 1);
            printf("Body:\n");
            print_node(node->as.iterum.body, depth + 2);
            break;

        case AST_TEMPTA:
            print_indent(depth + 1);
            printf("Try:\n");
            print_node(node->as.tempta.try_block, depth + 2);
            print_indent(depth + 1);
            printf("Cape Type:\n");
            print_type(node->as.tempta.cape_type, depth + 2);
            print_indent(depth + 1);
            printf("Cape Name: %s\n", node->as.tempta.cape_name ? node->as.tempta.cape_name : "(null)");
            print_indent(depth + 1);
            printf("Cape:\n");
            print_node(node->as.tempta.cape_block, depth + 2);
            if (node->as.tempta.semper_block) {
                print_indent(depth + 1);
                printf("Semper:\n");
                print_node(node->as.tempta.semper_block, depth + 2);
            }
            break;

        case AST_IACE:
            print_indent(depth + 1);
            printf("Value:\n");
            print_node(node->as.iace.value, depth + 2);
            break;

        case AST_FRANGE:
        case AST_RECEDE:
            // No additional fields
            break;

        case AST_TRANSITUS:
            print_indent(depth + 1);
            printf("Label: %s\n", node->as.transitus.label);
            break;

        case AST_ANUR:
            if (node->as.anur.value) {
                print_indent(depth + 1);
                printf("Value:\n");
                print_node(node->as.anur.value, depth + 2);
            }
            break;

        case AST_DIMITTE:
            print_indent(depth + 1);
            printf("Target:\n");
            print_node(node->as.dimitte.target, depth + 2);
            break;

        case AST_EXPR_STMT:
            print_node(node->as.expr_stmt.expression, depth + 1);
            break;

        case AST_LITERAL_INT:
            print_indent(depth + 1);
            printf("Value: %lld\n", (long long)node->as.literal_int.int_value);
            break;

        case AST_LITERAL_REAL:
            print_indent(depth + 1);
            printf("Value: %f\n", node->as.literal_real.real_value);
            break;

        case AST_LITERAL_STRING:
            print_indent(depth + 1);
            printf("Value: \"%s\"\n", node->as.literal_string.string_value);
            break;

        case AST_LITERAL_BOOL:
            print_indent(depth + 1);
            printf("Value: %s\n", node->as.literal_bool.bool_value ? "VERO" : "FALSO");
            break;

        case AST_LITERAL_NIHIL:
            // No additional fields
            break;

        case AST_MOLDE:
            print_indent(depth + 1);
            printf("Parts: (%zu)\n", node->as.molde.part_count);
            for (size_t i = 0; i < node->as.molde.part_count; i++) {
                cct_ast_molde_part_t *part = &node->as.molde.parts[i];
                print_indent(depth + 2);
                if (part->kind == CCT_AST_MOLDE_PART_LITERAL) {
                    printf("Literal: \"%s\"\n", part->literal_text ? part->literal_text : "");
                } else {
                    if (part->fmt_spec) {
                        printf("Expr (fmt: %s):\n", part->fmt_spec);
                    } else {
                        printf("Expr:\n");
                    }
                    print_node(part->expr, depth + 3);
                }
            }
            break;

        case AST_IDENTIFIER:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.identifier.name);
            break;

        case AST_BINARY_OP:
            print_indent(depth + 1);
            printf("Operator: %s\n", cct_token_type_string(node->as.binary_op.operator));
            print_indent(depth + 1);
            printf("Left:\n");
            print_node(node->as.binary_op.left, depth + 2);
            print_indent(depth + 1);
            printf("Right:\n");
            print_node(node->as.binary_op.right, depth + 2);
            break;

        case AST_UNARY_OP:
            print_indent(depth + 1);
            printf("Operator: %s\n", cct_token_type_string(node->as.unary_op.operator));
            print_indent(depth + 1);
            printf("Operand:\n");
            print_node(node->as.unary_op.operand, depth + 2);
            break;

        case AST_CALL:
            print_indent(depth + 1);
            printf("Callee:\n");
            print_node(node->as.call.callee, depth + 2);
            print_indent(depth + 1);
            printf("Arguments: (%zu)\n",
                   node->as.call.arguments ? node->as.call.arguments->count : 0);
            print_node_list(node->as.call.arguments, depth + 2);
            break;

        case AST_CONIURA:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.coniura.name);
            if (node->as.coniura.type_args && node->as.coniura.type_args->count > 0) {
                print_indent(depth + 1);
                printf("GENUS Args: (%zu)\n", node->as.coniura.type_args->count);
                for (size_t i = 0; i < node->as.coniura.type_args->count; i++) {
                    print_indent(depth + 2);
                    print_type_inline(node->as.coniura.type_args->types[i]);
                    printf("\n");
                }
            }
            print_indent(depth + 1);
            printf("Arguments: (%zu)\n",
                   node->as.coniura.arguments ? node->as.coniura.arguments->count : 0);
            print_node_list(node->as.coniura.arguments, depth + 2);
            break;

        case AST_OBSECRO:
            print_indent(depth + 1);
            printf("Name: %s\n", node->as.obsecro.name);
            print_indent(depth + 1);
            printf("Arguments: (%zu)\n",
                   node->as.obsecro.arguments ? node->as.obsecro.arguments->count : 0);
            print_node_list(node->as.obsecro.arguments, depth + 2);
            break;

        case AST_FIELD_ACCESS:
            print_indent(depth + 1);
            printf("Object:\n");
            print_node(node->as.field_access.object, depth + 2);
            print_indent(depth + 1);
            printf("Field: %s\n", node->as.field_access.field);
            break;

        case AST_INDEX_ACCESS:
            print_indent(depth + 1);
            printf("Array:\n");
            print_node(node->as.index_access.array, depth + 2);
            print_indent(depth + 1);
            printf("Index:\n");
            print_node(node->as.index_access.index, depth + 2);
            break;

        case AST_MENSURA:
            print_type(node->as.mensura.type, depth + 1);
            break;

        default:
            // Unknown node type - should never happen
            print_indent(depth + 1);
            printf("(unknown node type)\n");
            break;
    }
}

/* Print program - public API */
void cct_ast_print_program(const cct_ast_program_t *prog) {
    if (!prog) {
        printf("(null program)\n");
        return;
    }

    printf("PROGRAM: %s\n", prog->name);
    printf("Declarations: (%zu)\n", prog->declarations ? prog->declarations->count : 0);
    print_node_list(prog->declarations, 1);
}

void cct_ast_print_node(const cct_ast_node_t *node, int indent) {
    if (indent < 0) indent = 0;
    print_node((cct_ast_node_t*)node, (u32)indent);
}
