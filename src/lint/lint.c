/*
 * CCT — Clavicula Turing
 * Linter Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "lint.h"
#include "../formatter/formatter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CCT_LINT_BINDING_VARIABLE = 0,
    CCT_LINT_BINDING_PARAMETER
} cct_lint_binding_kind_t;

typedef struct {
    char *name;
    cct_lint_binding_kind_t kind;
    u32 line;
    u32 column;
    u32 uses;
} cct_lint_binding_t;

typedef struct {
    char **names;
    size_t *binding_indices;
    size_t count;
    size_t capacity;
} cct_lint_scope_t;

typedef struct {
    cct_lint_report_t *report;

    cct_lint_scope_t *scopes;
    size_t scope_count;
    size_t scope_capacity;

    cct_lint_binding_t *bindings;
    size_t binding_count;
    size_t binding_capacity;

    char **local_rituales;
    size_t local_rituale_count;
    size_t local_rituale_capacity;
    char **local_types;
    size_t local_type_count;
    size_t local_type_capacity;

    cct_ast_node_t **imports;
    size_t import_count;
    size_t import_capacity;
    bool imported_symbol_used;
} cct_lint_context_t;

static char* lint_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *copy = (char*)malloc(n + 1);
    if (!copy) return NULL;
    memcpy(copy, s, n + 1);
    return copy;
}

const char* cct_lint_rule_id(cct_lint_rule_t rule) {
    switch (rule) {
        case CCT_LINT_RULE_UNUSED_VARIABLE: return "unused-variable";
        case CCT_LINT_RULE_UNUSED_PARAMETER: return "unused-parameter";
        case CCT_LINT_RULE_UNUSED_IMPORT: return "unused-import";
        case CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN: return "dead-code-after-return";
        case CCT_LINT_RULE_DEAD_CODE_AFTER_THROW: return "dead-code-after-throw";
        case CCT_LINT_RULE_SHADOWING_LOCAL: return "shadowing-local";
        default: return "unknown-rule";
    }
}

void cct_lint_report_init(cct_lint_report_t *report) {
    if (!report) return;
    memset(report, 0, sizeof(*report));
}

void cct_lint_report_dispose(cct_lint_report_t *report) {
    if (!report) return;
    for (size_t i = 0; i < report->issues.count; i++) {
        free(report->issues.items[i].message);
    }
    free(report->issues.items);
    memset(report, 0, sizeof(*report));
}

static bool lint_issue_push(cct_lint_report_t *report, cct_lint_rule_t rule, u32 line, u32 column, const char *message) {
    if (!report || !message) return false;
    if (report->issues.count == report->issues.capacity) {
        size_t new_cap = report->issues.capacity == 0 ? 16 : report->issues.capacity * 2;
        cct_lint_issue_t *tmp = (cct_lint_issue_t*)realloc(report->issues.items, new_cap * sizeof(*report->issues.items));
        if (!tmp) return false;
        report->issues.items = tmp;
        report->issues.capacity = new_cap;
    }
    cct_lint_issue_t *issue = &report->issues.items[report->issues.count++];
    issue->rule = rule;
    issue->line = line;
    issue->column = column;
    issue->message = lint_strdup(message);
    return issue->message != NULL;
}

static bool lint_vec_push_str(char ***items, size_t *count, size_t *capacity, const char *value) {
    if (!items || !count || !capacity || !value) return false;
    if (*count == *capacity) {
        size_t new_cap = *capacity == 0 ? 8 : (*capacity * 2);
        char **tmp = (char**)realloc(*items, new_cap * sizeof(**items));
        if (!tmp) return false;
        *items = tmp;
        *capacity = new_cap;
    }
    (*items)[*count] = lint_strdup(value);
    if (!(*items)[*count]) return false;
    (*count)++;
    return true;
}

static bool lint_scope_push(cct_lint_context_t *ctx) {
    if (!ctx) return false;
    if (ctx->scope_count == ctx->scope_capacity) {
        size_t new_cap = ctx->scope_capacity == 0 ? 8 : ctx->scope_capacity * 2;
        cct_lint_scope_t *tmp = (cct_lint_scope_t*)realloc(ctx->scopes, new_cap * sizeof(*ctx->scopes));
        if (!tmp) return false;
        ctx->scopes = tmp;
        ctx->scope_capacity = new_cap;
    }
    cct_lint_scope_t *scope = &ctx->scopes[ctx->scope_count++];
    memset(scope, 0, sizeof(*scope));
    return true;
}

static void lint_scope_pop(cct_lint_context_t *ctx) {
    if (!ctx || ctx->scope_count == 0) return;
    cct_lint_scope_t *scope = &ctx->scopes[ctx->scope_count - 1];
    for (size_t i = 0; i < scope->count; i++) {
        free(scope->names[i]);
    }
    free(scope->names);
    free(scope->binding_indices);
    memset(scope, 0, sizeof(*scope));
    ctx->scope_count--;
}

static bool lint_scope_add_name(cct_lint_scope_t *scope, const char *name, size_t binding_index) {
    if (!scope || !name) return false;
    if (scope->count == scope->capacity) {
        size_t new_cap = scope->capacity == 0 ? 8 : scope->capacity * 2;
        char **new_names = (char**)realloc(scope->names, new_cap * sizeof(*scope->names));
        size_t *new_indices = (size_t*)realloc(scope->binding_indices, new_cap * sizeof(*scope->binding_indices));
        if (!new_names || !new_indices) {
            free(new_names);
            free(new_indices);
            return false;
        }
        scope->names = new_names;
        scope->binding_indices = new_indices;
        scope->capacity = new_cap;
    }
    scope->names[scope->count] = lint_strdup(name);
    if (!scope->names[scope->count]) return false;
    scope->binding_indices[scope->count] = binding_index;
    scope->count++;
    return true;
}

static bool lint_binding_push(cct_lint_context_t *ctx, const char *name, cct_lint_binding_kind_t kind, u32 line, u32 column, size_t *out_index) {
    if (!ctx || !name) return false;
    if (ctx->binding_count == ctx->binding_capacity) {
        size_t new_cap = ctx->binding_capacity == 0 ? 16 : ctx->binding_capacity * 2;
        cct_lint_binding_t *tmp = (cct_lint_binding_t*)realloc(ctx->bindings, new_cap * sizeof(*ctx->bindings));
        if (!tmp) return false;
        ctx->bindings = tmp;
        ctx->binding_capacity = new_cap;
    }
    cct_lint_binding_t *b = &ctx->bindings[ctx->binding_count];
    b->name = lint_strdup(name);
    if (!b->name) return false;
    b->kind = kind;
    b->line = line;
    b->column = column;
    b->uses = 0;
    if (out_index) *out_index = ctx->binding_count;
    ctx->binding_count++;
    return true;
}

static bool lint_push_import(cct_lint_context_t *ctx, cct_ast_node_t *import_node) {
    if (!ctx || !import_node) return false;
    if (ctx->import_count == ctx->import_capacity) {
        size_t new_cap = ctx->import_capacity == 0 ? 8 : ctx->import_capacity * 2;
        cct_ast_node_t **tmp = (cct_ast_node_t**)realloc(ctx->imports, new_cap * sizeof(*ctx->imports));
        if (!tmp) return false;
        ctx->imports = tmp;
        ctx->import_capacity = new_cap;
    }
    ctx->imports[ctx->import_count++] = import_node;
    return true;
}

static void lint_context_dispose(cct_lint_context_t *ctx) {
    if (!ctx) return;
    while (ctx->scope_count > 0) lint_scope_pop(ctx);
    free(ctx->scopes);
    for (size_t i = 0; i < ctx->binding_count; i++) {
        free(ctx->bindings[i].name);
    }
    free(ctx->bindings);
    for (size_t i = 0; i < ctx->local_rituale_count; i++) free(ctx->local_rituales[i]);
    for (size_t i = 0; i < ctx->local_type_count; i++) free(ctx->local_types[i]);
    free(ctx->local_rituales);
    free(ctx->local_types);
    free(ctx->imports);
    memset(ctx, 0, sizeof(*ctx));
}

static ssize_t lint_find_binding(cct_lint_context_t *ctx, const char *name, bool skip_current_scope) {
    if (!ctx || !name || ctx->scope_count == 0) return -1;
    ssize_t start = (ssize_t)ctx->scope_count - 1;
    if (skip_current_scope) start--;
    for (ssize_t si = start; si >= 0; si--) {
        cct_lint_scope_t *scope = &ctx->scopes[si];
        for (ssize_t i = (ssize_t)scope->count - 1; i >= 0; i--) {
            if (strcmp(scope->names[i], name) == 0) {
                return (ssize_t)scope->binding_indices[i];
            }
        }
    }
    return -1;
}

static bool lint_is_local_rituale(const cct_lint_context_t *ctx, const char *name) {
    if (!ctx || !name) return false;
    for (size_t i = 0; i < ctx->local_rituale_count; i++) {
        if (strcmp(ctx->local_rituales[i], name) == 0) return true;
    }
    return false;
}

static bool lint_is_local_type(const cct_lint_context_t *ctx, const char *name) {
    if (!ctx || !name) return false;
    for (size_t i = 0; i < ctx->local_type_count; i++) {
        if (strcmp(ctx->local_types[i], name) == 0) return true;
    }
    return false;
}

static bool lint_is_builtin_type(const char *name) {
    if (!name) return false;
    return strcmp(name, "REX") == 0 ||
           strcmp(name, "DUX") == 0 ||
           strcmp(name, "COMES") == 0 ||
           strcmp(name, "MILES") == 0 ||
           strcmp(name, "UMBRA") == 0 ||
           strcmp(name, "FLAMMA") == 0 ||
           strcmp(name, "VERBUM") == 0 ||
           strcmp(name, "VERUM") == 0 ||
           strcmp(name, "FALSUM") == 0 ||
           strcmp(name, "NIHIL") == 0 ||
           strcmp(name, "FRACTUM") == 0;
}

static void lint_scan_type(cct_lint_context_t *ctx, const cct_ast_type_t *type) {
    if (!ctx || !type) return;
    if (type->name && !lint_is_builtin_type(type->name) && !lint_is_local_type(ctx, type->name)) {
        ctx->imported_symbol_used = true;
    }
    lint_scan_type(ctx, type->element_type);
    if (type->generic_args) {
        for (size_t i = 0; i < type->generic_args->count; i++) {
            lint_scan_type(ctx, type->generic_args->types[i]);
        }
    }
}

static bool lint_declare_binding(cct_lint_context_t *ctx, const char *name, cct_lint_binding_kind_t kind, u32 line, u32 column) {
    if (!ctx || !name) return false;
    if (lint_find_binding(ctx, name, true) >= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "local '%s' shadows symbol from outer scope", name);
        if (!lint_issue_push(ctx->report, CCT_LINT_RULE_SHADOWING_LOCAL, line, column, msg)) return false;
    }
    size_t idx = 0;
    if (!lint_binding_push(ctx, name, kind, line, column, &idx)) return false;
    if (!lint_scope_add_name(&ctx->scopes[ctx->scope_count - 1], name, idx)) return false;
    return true;
}

static void lint_mark_identifier_use(cct_lint_context_t *ctx, const char *name) {
    if (!ctx || !name) return;
    ssize_t idx = lint_find_binding(ctx, name, false);
    if (idx >= 0) {
        ctx->bindings[idx].uses++;
        return;
    }
    if (!lint_is_local_rituale(ctx, name) && !lint_is_local_type(ctx, name)) {
        ctx->imported_symbol_used = true;
    }
}

static bool lint_walk_node(cct_lint_context_t *ctx, const cct_ast_node_t *node);

static bool lint_walk_block(cct_lint_context_t *ctx, const cct_ast_node_t *block) {
    if (!ctx || !block || block->type != AST_BLOCK) return true;
    cct_ast_node_list_t *stmts = block->as.block.statements;
    bool terminated = false;
    cct_lint_rule_t dead_rule = CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN;
    for (size_t i = 0; stmts && i < stmts->count; i++) {
        cct_ast_node_t *stmt = stmts->nodes[i];
        if (terminated) {
            const char *msg = (dead_rule == CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN)
                                ? "statement is unreachable after REDDE"
                                : "statement is unreachable after IACE";
            if (!lint_issue_push(ctx->report, dead_rule, stmt->line, stmt->column, msg)) return false;
        }
        if (!lint_walk_node(ctx, stmt)) return false;
        if (stmt->type == AST_REDDE) {
            terminated = true;
            dead_rule = CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN;
        } else if (stmt->type == AST_IACE) {
            terminated = true;
            dead_rule = CCT_LINT_RULE_DEAD_CODE_AFTER_THROW;
        }
    }
    return true;
}

static bool lint_walk_node_list(cct_lint_context_t *ctx, const cct_ast_node_list_t *list) {
    if (!list) return true;
    for (size_t i = 0; i < list->count; i++) {
        if (!lint_walk_node(ctx, list->nodes[i])) return false;
    }
    return true;
}

static bool lint_walk_node(cct_lint_context_t *ctx, const cct_ast_node_t *node) {
    if (!ctx || !node) return true;
    switch (node->type) {
        case AST_IMPORT:
            if (!lint_push_import(ctx, (cct_ast_node_t*)node)) return false;
            return true;

        case AST_BLOCK:
            if (!lint_scope_push(ctx)) return false;
            {
                bool ok = lint_walk_block(ctx, node);
                lint_scope_pop(ctx);
                return ok;
            }

        case AST_EVOCA:
            lint_scan_type(ctx, node->as.evoca.var_type);
            if (node->as.evoca.initializer && !lint_walk_node(ctx, node->as.evoca.initializer)) return false;
            if (!lint_declare_binding(ctx, node->as.evoca.name, CCT_LINT_BINDING_VARIABLE, node->line, node->column)) return false;
            return true;

        case AST_VINCIRE:
            return lint_walk_node(ctx, node->as.vincire.target) && lint_walk_node(ctx, node->as.vincire.value);

        case AST_REDDE:
            return lint_walk_node(ctx, node->as.redde.value);

        case AST_SI:
            return lint_walk_node(ctx, node->as.si.condition) &&
                   lint_walk_node(ctx, node->as.si.then_branch) &&
                   lint_walk_node(ctx, node->as.si.else_branch);

        case AST_QUANDO:
            if (!lint_walk_node(ctx, node->as.quando.expression)) return false;
            for (size_t i = 0; i < node->as.quando.case_count; i++) {
                cct_ast_case_node_t *case_node = &node->as.quando.cases[i];
                for (size_t j = 0; j < case_node->literal_count; j++) {
                    if (!lint_walk_node(ctx, case_node->literals[j])) return false;
                }
                if (!lint_walk_node(ctx, case_node->body)) return false;
            }
            return lint_walk_node(ctx, node->as.quando.else_body);

        case AST_DUM:
            return lint_walk_node(ctx, node->as.dum.condition) && lint_walk_node(ctx, node->as.dum.body);

        case AST_DONEC:
            return lint_walk_node(ctx, node->as.donec.body) && lint_walk_node(ctx, node->as.donec.condition);

        case AST_REPETE: {
            if (!lint_walk_node(ctx, node->as.repete.start) ||
                !lint_walk_node(ctx, node->as.repete.end) ||
                !lint_walk_node(ctx, node->as.repete.step)) return false;
            if (!lint_scope_push(ctx)) return false;
            if (!lint_declare_binding(ctx, node->as.repete.iterator, CCT_LINT_BINDING_VARIABLE, node->line, node->column)) return false;
            bool ok = lint_walk_node(ctx, node->as.repete.body);
            lint_scope_pop(ctx);
            return ok;
        }

        case AST_ITERUM: {
            if (!lint_walk_node(ctx, node->as.iterum.collection)) return false;
            if (!lint_scope_push(ctx)) return false;
            if (!lint_declare_binding(ctx, node->as.iterum.item_name, CCT_LINT_BINDING_VARIABLE, node->line, node->column)) return false;
            if (node->as.iterum.value_name &&
                !lint_declare_binding(ctx, node->as.iterum.value_name, CCT_LINT_BINDING_VARIABLE, node->line, node->column)) {
                lint_scope_pop(ctx);
                return false;
            }
            bool ok = lint_walk_node(ctx, node->as.iterum.body);
            lint_scope_pop(ctx);
            return ok;
        }

        case AST_TEMPTA: {
            if (!lint_walk_node(ctx, node->as.tempta.try_block)) return false;
            if (!lint_scope_push(ctx)) return false;
            if (node->as.tempta.cape_name &&
                !lint_declare_binding(ctx, node->as.tempta.cape_name, CCT_LINT_BINDING_VARIABLE, node->line, node->column)) {
                lint_scope_pop(ctx);
                return false;
            }
            bool ok = lint_walk_node(ctx, node->as.tempta.cape_block) &&
                      lint_walk_node(ctx, node->as.tempta.semper_block);
            lint_scope_pop(ctx);
            return ok;
        }

        case AST_IACE:
            return lint_walk_node(ctx, node->as.iace.value);

        case AST_DIMITTE:
            return lint_walk_node(ctx, node->as.dimitte.target);

        case AST_EXPR_STMT:
            return lint_walk_node(ctx, node->as.expr_stmt.expression);

        case AST_MOLDE:
            if (node->as.molde.parts) {
                for (size_t i = 0; i < node->as.molde.part_count; i++) {
                    cct_ast_molde_part_t *part = &node->as.molde.parts[i];
                    if (part && part->kind == CCT_AST_MOLDE_PART_EXPR) {
                        if (!lint_walk_node(ctx, part->expr)) return false;
                    }
                }
            }
            return true;

        case AST_IDENTIFIER:
            lint_mark_identifier_use(ctx, node->as.identifier.name);
            return true;

        case AST_BINARY_OP:
            return lint_walk_node(ctx, node->as.binary_op.left) && lint_walk_node(ctx, node->as.binary_op.right);

        case AST_UNARY_OP:
            return lint_walk_node(ctx, node->as.unary_op.operand);

        case AST_CALL:
            return lint_walk_node(ctx, node->as.call.callee) && lint_walk_node_list(ctx, node->as.call.arguments);

        case AST_CONIURA:
            if (node->as.coniura.name && !lint_is_local_rituale(ctx, node->as.coniura.name)) {
                ctx->imported_symbol_used = true;
            }
            if (node->as.coniura.type_args) {
                for (size_t i = 0; i < node->as.coniura.type_args->count; i++) {
                    lint_scan_type(ctx, node->as.coniura.type_args->types[i]);
                }
            }
            return lint_walk_node_list(ctx, node->as.coniura.arguments);

        case AST_OBSECRO:
            return lint_walk_node_list(ctx, node->as.obsecro.arguments);

        case AST_FIELD_ACCESS:
            return lint_walk_node(ctx, node->as.field_access.object);

        case AST_INDEX_ACCESS:
            return lint_walk_node(ctx, node->as.index_access.array) && lint_walk_node(ctx, node->as.index_access.index);

        case AST_MENSURA:
            lint_scan_type(ctx, node->as.mensura.type);
            return true;

        case AST_RITUALE: {
            if (node->as.rituale.return_type) lint_scan_type(ctx, node->as.rituale.return_type);
            if (!lint_scope_push(ctx)) return false;
            if (node->as.rituale.params) {
                for (size_t i = 0; i < node->as.rituale.params->count; i++) {
                    cct_ast_param_t *p = node->as.rituale.params->params[i];
                    lint_scan_type(ctx, p->type);
                    if (!lint_declare_binding(ctx, p->name, CCT_LINT_BINDING_PARAMETER, p->line, p->column)) {
                        lint_scope_pop(ctx);
                        return false;
                    }
                }
            }
            bool ok = lint_walk_node(ctx, node->as.rituale.body);
            lint_scope_pop(ctx);
            return ok;
        }

        case AST_SIGILLUM:
            if (node->as.sigillum.fields) {
                for (size_t i = 0; i < node->as.sigillum.fields->count; i++) {
                    lint_scan_type(ctx, node->as.sigillum.fields->fields[i]->type);
                }
            }
            return lint_walk_node_list(ctx, node->as.sigillum.methods);

        case AST_PACTUM:
            return lint_walk_node_list(ctx, node->as.pactum.signatures);

        case AST_CODEX:
            return lint_walk_node_list(ctx, node->as.codex.declarations);

        case AST_ORDO:
        case AST_LITERAL_INT:
        case AST_LITERAL_REAL:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
        case AST_LITERAL_NIHIL:
        case AST_PARAM:
        case AST_FIELD:
        case AST_ENUM_ITEM:
        case AST_FRANGE:
        case AST_RECEDE:
        case AST_TRANSITUS:
        case AST_ANUR:
        case AST_TYPE:
        case AST_PROGRAM:
            return true;
    }
    return true;
}

static bool lint_collect_locals(cct_lint_context_t *ctx, const cct_ast_program_t *program) {
    if (!ctx || !program || !program->declarations) return false;
    for (size_t i = 0; i < program->declarations->count; i++) {
        cct_ast_node_t *decl = program->declarations->nodes[i];
        if (decl->type == AST_RITUALE) {
            if (!lint_vec_push_str(&ctx->local_rituales, &ctx->local_rituale_count, &ctx->local_rituale_capacity, decl->as.rituale.name)) {
                return false;
            }
        } else if (decl->type == AST_SIGILLUM) {
            if (!lint_vec_push_str(&ctx->local_types, &ctx->local_type_count, &ctx->local_type_capacity, decl->as.sigillum.name)) {
                return false;
            }
        } else if (decl->type == AST_ORDO) {
            if (!lint_vec_push_str(&ctx->local_types, &ctx->local_type_count, &ctx->local_type_capacity, decl->as.ordo.name)) {
                return false;
            }
        } else if (decl->type == AST_PACTUM) {
            if (!lint_vec_push_str(&ctx->local_types, &ctx->local_type_count, &ctx->local_type_capacity, decl->as.pactum.name)) {
                return false;
            }
        }
    }
    return true;
}

bool cct_lint_run_program(
    const cct_ast_program_t *program,
    const cct_lint_options_t *options,
    cct_lint_report_t *out_report
) {
    (void)options;
    if (!program || !out_report) return false;

    cct_lint_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.report = out_report;

    if (!lint_scope_push(&ctx)) {
        lint_context_dispose(&ctx);
        return false;
    }
    if (!lint_collect_locals(&ctx, program)) {
        lint_context_dispose(&ctx);
        return false;
    }

    if (!program->declarations) {
        lint_context_dispose(&ctx);
        return true;
    }

    for (size_t i = 0; i < program->declarations->count; i++) {
        if (!lint_walk_node(&ctx, program->declarations->nodes[i])) {
            lint_context_dispose(&ctx);
            return false;
        }
    }

    for (size_t i = 0; i < ctx.binding_count; i++) {
        cct_lint_binding_t *b = &ctx.bindings[i];
        if (b->uses == 0) {
            cct_lint_rule_t rule = (b->kind == CCT_LINT_BINDING_PARAMETER)
                ? CCT_LINT_RULE_UNUSED_PARAMETER
                : CCT_LINT_RULE_UNUSED_VARIABLE;
            char msg[256];
            snprintf(msg, sizeof(msg), "'%s' is declared but never used", b->name);
            if (!lint_issue_push(out_report, rule, b->line, b->column, msg)) {
                lint_context_dispose(&ctx);
                return false;
            }
        }
    }

    if (ctx.import_count > 0 && !ctx.imported_symbol_used) {
        for (size_t i = 0; i < ctx.import_count; i++) {
            cct_ast_node_t *imp = ctx.imports[i];
            char msg[256];
            snprintf(msg, sizeof(msg), "import '%s' is never used", imp->as.import.filename);
            if (!lint_issue_push(out_report, CCT_LINT_RULE_UNUSED_IMPORT, imp->line, imp->column, msg)) {
                lint_context_dispose(&ctx);
                return false;
            }
        }
    }

    lint_context_dispose(&ctx);
    return true;
}

void cct_lint_emit_report(
    FILE *out,
    const char *file_path,
    const cct_lint_report_t *report
) {
    if (!out || !report) return;
    const char *path = file_path ? file_path : "<input>";
    for (size_t i = 0; i < report->issues.count; i++) {
        const cct_lint_issue_t *issue = &report->issues.items[i];
        fprintf(out, "%s:%u:%u: warning[%s]: %s\n",
                path,
                issue->line,
                issue->column,
                cct_lint_rule_id(issue->rule),
                issue->message ? issue->message : "");
    }
}

static char* lint_read_file(const char *file_path) {
    FILE *f = fopen(file_path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[rd] = '\0';
    return buf;
}

static bool lint_should_fix_rule(cct_lint_rule_t rule) {
    return rule == CCT_LINT_RULE_UNUSED_IMPORT ||
           rule == CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN ||
           rule == CCT_LINT_RULE_DEAD_CODE_AFTER_THROW;
}

static bool lint_mark_line(u32 **lines, size_t *count, size_t *cap, u32 line) {
    if (!lines || !count || !cap || line == 0) return false;
    for (size_t i = 0; i < *count; i++) {
        if ((*lines)[i] == line) return true;
    }
    if (*count == *cap) {
        size_t new_cap = *cap == 0 ? 16 : *cap * 2;
        u32 *tmp = (u32*)realloc(*lines, new_cap * sizeof(**lines));
        if (!tmp) return false;
        *lines = tmp;
        *cap = new_cap;
    }
    (*lines)[(*count)++] = line;
    return true;
}

static int lint_cmp_u32(const void *a, const void *b) {
    const u32 va = *(const u32*)a;
    const u32 vb = *(const u32*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

bool cct_lint_apply_fixes_to_file(
    const char *file_path,
    const cct_lint_report_t *report,
    bool format_after_fix
) {
    if (!file_path || !report) return false;
    u32 *lines = NULL;
    size_t line_count = 0;
    size_t line_cap = 0;

    for (size_t i = 0; i < report->issues.count; i++) {
        const cct_lint_issue_t *issue = &report->issues.items[i];
        if (lint_should_fix_rule(issue->rule)) {
            if (!lint_mark_line(&lines, &line_count, &line_cap, issue->line)) {
                free(lines);
                return false;
            }
        }
    }

    if (line_count == 0) {
        free(lines);
        return true;
    }

    qsort(lines, line_count, sizeof(*lines), lint_cmp_u32);

    char *source = lint_read_file(file_path);
    if (!source) {
        free(lines);
        return false;
    }

    size_t src_len = strlen(source);
    char *out = (char*)malloc(src_len + 1);
    if (!out) {
        free(lines);
        free(source);
        return false;
    }
    size_t out_len = 0;
    size_t line_no = 1;
    size_t line_idx = 0;
    char *cursor = source;
    while (*cursor) {
        char *line_end = strchr(cursor, '\n');
        size_t line_len = line_end ? (size_t)(line_end - cursor + 1) : strlen(cursor);
        bool remove_line = (line_idx < line_count && lines[line_idx] == (u32)line_no);
        if (remove_line) {
            line_idx++;
        } else {
            memcpy(out + out_len, cursor, line_len);
            out_len += line_len;
        }
        cursor += line_len;
        line_no++;
    }
    out[out_len] = '\0';

    bool ok = false;
    if (cct_formatter_write_file(file_path, out)) {
        ok = true;
        if (format_after_fix) {
            cct_formatter_options_t opts = cct_formatter_default_options();
            cct_formatter_result_t fmt = cct_formatter_format_file(file_path, &opts);
            if (fmt.success) {
                if (fmt.changed) {
                    ok = cct_formatter_write_file(file_path, fmt.formatted_source);
                }
            } else {
                ok = false;
            }
            cct_formatter_result_dispose(&fmt);
        }
    }

    free(lines);
    free(source);
    free(out);
    return ok;
}
