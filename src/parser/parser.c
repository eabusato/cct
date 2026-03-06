/*
 * CCT — Clavicula Turing
 * Recursive Descent Parser
 *
 * FASE 2B: Hardened syntax analysis and AST construction
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "parser.h"
#include "../common/diagnostic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Token Management and Error Handling
 * ======================================================================== */

static void free_parser_token(cct_token_t *token) {
    if (!token) return;
    cct_token_free(token);
    token->type = TOKEN_EOF;
    token->line = 0;
    token->column = 0;
}

static void advance(cct_parser_t *parser) {
    /* previous owns the prior token lexeme; release before overwriting */
    free_parser_token(&parser->previous);
    parser->previous = parser->current;
    parser->current = cct_lexer_next_token(parser->lexer);
}

static bool check(const cct_parser_t *parser, cct_token_type_t type) {
    return parser->current.type == type;
}

static bool match(cct_parser_t *parser, cct_token_type_t type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static const char* parser_error_suggestion(const cct_token_t *token, const char *message) {
    if (!token || !message) return NULL;

    if (token->type == TOKEN_FIN && strcmp(message, "Expected EXPLICIT") == 0) {
        return "use 'EXPLICIT ...' to close the declaration block (for example: EXPLICIT RITUALE)";
    }
    if (token->type == TOKEN_FIN && strcmp(message, "Unexpected block terminator") == 0) {
        return "this declaration closes with EXPLICIT, not FIN (example: EXPLICIT RITUALE)";
    }
    if (token->type == TOKEN_EXPLICIT && strcmp(message, "Expected FIN") == 0) {
        return "use 'FIN ...' to close control-flow blocks such as SI, DUM or REPETE";
    }

    return NULL;
}

static void error_at(cct_parser_t *parser, const cct_token_t *token, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    cct_error_at_location_with_suggestion(
        CCT_ERROR_SYNTAX,
        parser->filename,
        token->line,
        token->column,
        message,
        parser_error_suggestion(token, message)
    );
}

static void error_at_current(cct_parser_t *parser, const char *message) {
    error_at(parser, &parser->current, message);
}

static void expect(cct_parser_t *parser, cct_token_type_t type, const char *message) {
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    error_at_current(parser, message);
}

static bool is_sync_point(cct_token_type_t type) {
    switch (type) {
        case TOKEN_INCIPIT:
        case TOKEN_EXPLICIT:
        case TOKEN_FIN:
        case TOKEN_ALITER:
        case TOKEN_ADVOCARE:
        case TOKEN_RITUALE:
        case TOKEN_CODEX:
        case TOKEN_SIGILLUM:
        case TOKEN_ORDO:
        case TOKEN_PACTUM:
        case TOKEN_EVOCA:
        case TOKEN_VINCIRE:
        case TOKEN_SI:
        case TOKEN_DUM:
        case TOKEN_DONEC:
        case TOKEN_REPETE:
        case TOKEN_ITERUM:
        case TOKEN_REDDE:
        case TOKEN_FRANGE:
        case TOKEN_RECEDE:
        case TOKEN_TRANSITUS:
        case TOKEN_OBSECRO:
        case TOKEN_CONIURA:
        case TOKEN_ANUR:
        case TOKEN_DIMITTE:
        case TOKEN_TEMPTA:
        case TOKEN_IACE:
        case TOKEN_CAPE:
        case TOKEN_SEMPER:
        case TOKEN_GENUS:
            return true;
        default:
            return false;
    }
}

static void synchronize(cct_parser_t *parser) {
    if (!parser->panic_mode) return;
    parser->panic_mode = false;

    if (check(parser, TOKEN_EOF)) return;

    /* Always consume at least one token so recovery makes progress. */
    advance(parser);

    while (!check(parser, TOKEN_EOF)) {
        if (is_sync_point(parser->current.type)) {
            return;
        }
        advance(parser);
    }
}

static bool consume_fin_clause(cct_parser_t *parser, cct_token_type_t expected, const char *message) {
    expect(parser, TOKEN_FIN, "Expected FIN");
    if (!check(parser, expected)) {
        error_at_current(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

static bool consume_explicit_clause(cct_parser_t *parser, cct_token_type_t expected, const char *message) {
    expect(parser, TOKEN_EXPLICIT, "Expected EXPLICIT");
    if (!check(parser, expected)) {
        error_at_current(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

static void consume_optional_semicolon(cct_parser_t *parser) {
    (void)match(parser, TOKEN_SEMICOLON);
}

static char* dup_string_contents(const char *lexeme) {
    if (!lexeme) return NULL;
    size_t len = strlen(lexeme);
    if (len >= 2 && lexeme[0] == '"' && lexeme[len - 1] == '"') {
        size_t inner_len = len - 2;
        char *copy = (char*)malloc(inner_len + 1);
        if (!copy) exit(CCT_ERROR_OUT_OF_MEMORY);
        memcpy(copy, lexeme + 1, inner_len);
        copy[inner_len] = '\0';
        return copy;
    }

    char *copy = strdup(lexeme);
    if (!copy) exit(CCT_ERROR_OUT_OF_MEMORY);
    return copy;
}

/* ========================================================================
 * Forward Declarations
 * ======================================================================== */

static cct_ast_node_t* parse_declaration(cct_parser_t *parser);
static cct_ast_node_t* parse_statement(cct_parser_t *parser);
static cct_ast_node_t* parse_expression(cct_parser_t *parser);
static cct_ast_type_t* parse_type(cct_parser_t *parser);
static cct_ast_type_param_list_t* parse_optional_type_params(
    cct_parser_t *parser,
    const char *decl_kind,
    bool allow_pactum_constraint
);
static cct_ast_type_list_t* parse_optional_generic_type_args(cct_parser_t *parser, const char *context);

/* ========================================================================
 * Token/Grammar Helpers
 * ======================================================================== */

static bool is_primitive_type_token(cct_token_type_t type) {
    switch (type) {
        case TOKEN_REX:
        case TOKEN_DUX:
        case TOKEN_COMES:
        case TOKEN_MILES:
        case TOKEN_UMBRA:
        case TOKEN_FLAMMA:
        case TOKEN_VERBUM:
        case TOKEN_VERUM: /* bool type */
        case TOKEN_NIHIL:
        case TOKEN_FRACTUM:
            return true;
        default:
            return false;
    }
}

static bool is_type_start_token(cct_token_type_t type) {
    return type == TOKEN_IDENTIFIER ||
           is_primitive_type_token(type) ||
           type == TOKEN_SPECULUM ||
           type == TOKEN_SERIES ||
           type == TOKEN_FLUXUS ||
           type == TOKEN_CONSTANS ||
           type == TOKEN_VOLATILE;
}

static bool is_expression_start_token(cct_token_type_t type) {
    switch (type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_INTEGER:
        case TOKEN_REAL:
        case TOKEN_STRING:
        case TOKEN_VERUM:
        case TOKEN_FALSUM:
        case TOKEN_NIHIL:
        case TOKEN_LPAREN:
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        case TOKEN_NON:
        case TOKEN_NON_BIT:
        case TOKEN_STAR:
        case TOKEN_SPECULUM:
        case TOKEN_CONIURA:
        case TOKEN_OBSECRO:
        case TOKEN_MENSURA:
            return true;
        default:
            return false;
    }
}

static bool is_statement_terminator(cct_token_type_t type) {
    switch (type) {
        case TOKEN_FIN:
        case TOKEN_EXPLICIT:
        case TOKEN_ALITER:
        case TOKEN_CAPE:
        case TOKEN_SEMPER:
        case TOKEN_EOF:
            return true;
        default:
            return false;
    }
}

static cct_ast_node_t* parse_condition_expr(cct_parser_t *parser) {
    return parse_expression(parser);
}

/* ========================================================================
 * Type Parsing (ritual syntax first, with small compatibility tolerance)
 * ======================================================================== */

static cct_ast_type_t* parse_type(cct_parser_t *parser) {
    if (match(parser, TOKEN_CONSTANS) || match(parser, TOKEN_VOLATILE)) {
        /* AST does not model qualifiers yet; preserve underlying type. */
        return parse_type(parser);
    }

    if (match(parser, TOKEN_SPECULUM)) {
        cct_ast_type_t *base = parse_type(parser);
        if (!base) return NULL;
        return cct_ast_create_pointer_type(base);
    }

    if (match(parser, TOKEN_SERIES)) {
        cct_ast_type_t *elem = parse_type(parser);
        if (!elem) return NULL;

        /* Compatibility: if legacy postfix parsing already produced `type[n]`,
         * accept it as the SERIES payload instead of requiring another `[n]`. */
        if (elem->is_array) {
            return elem;
        }

        expect(parser, TOKEN_LBRACKET, "Expected '[' after SERIES element type");
        if (!check(parser, TOKEN_INTEGER)) {
            error_at_current(parser, "Expected array size");
            return cct_ast_create_array_type(elem, 0);
        }
        u32 size = (u32)strtoll(parser->current.lexeme, NULL, 10);
        advance(parser);
        expect(parser, TOKEN_RBRACKET, "Expected ']' after array size");
        return cct_ast_create_array_type(elem, size);
    }

    if (match(parser, TOKEN_FLUXUS)) {
        cct_ast_type_t *elem = parse_type(parser);
        if (!elem) return NULL;
        return cct_ast_create_array_type(elem, 0);
    }

    if (!is_type_start_token(parser->current.type) ||
        (!is_primitive_type_token(parser->current.type) && parser->current.type != TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected type name");
        return NULL;
    }

    char *type_name = strdup(parser->current.lexeme);
    if (!type_name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_type_t *type = cct_ast_create_type(type_name);
    free(type_name);

    type->generic_args = parse_optional_generic_type_args(parser, "type");

    /* Tolerate legacy postfix pointer syntax used in 2A internals (REX *). */
    while (match(parser, TOKEN_STAR)) {
        type = cct_ast_create_pointer_type(type);
    }

    /* Tolerate legacy postfix array syntax (type[n]). */
    while (match(parser, TOKEN_LBRACKET)) {
        if (!check(parser, TOKEN_INTEGER)) {
            error_at_current(parser, "Expected array size");
            break;
        }
        u32 size = (u32)strtoll(parser->current.lexeme, NULL, 10);
        advance(parser);
        expect(parser, TOKEN_RBRACKET, "Expected ']' after array size");
        type = cct_ast_create_array_type(type, size);
    }

    return type;
}

static bool type_param_list_contains(const cct_ast_type_param_list_t *list, const char *name) {
    if (!list || !name) return false;
    for (size_t i = 0; i < list->count; i++) {
        cct_ast_type_param_t *p = list->params[i];
        if (p && p->name && strcmp(p->name, name) == 0) return true;
    }
    return false;
}

static cct_ast_type_param_list_t* parse_optional_type_params(
    cct_parser_t *parser,
    const char *decl_kind,
    bool allow_pactum_constraint
) {
    cct_ast_type_param_list_t *list = cct_ast_create_type_param_list();
    if (!check(parser, TOKEN_GENUS)) {
        return list;
    }

    advance(parser); /* GENUS */
    expect(parser, TOKEN_LPAREN, "Expected '(' after GENUS");

    if (check(parser, TOKEN_RPAREN)) {
        error_at_current(parser, "GENUS() is invalid in subset 10A (expected at least one type parameter)");
        advance(parser);
        return list;
    }

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RPAREN)) {
        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "GENUS parameters must be identifiers in subset 10A");
            break;
        }

        char *param_name = strdup(parser->current.lexeme ? parser->current.lexeme : "");
        if (!param_name) exit(CCT_ERROR_OUT_OF_MEMORY);
        u32 param_line = parser->current.line;
        u32 param_col = parser->current.column;
        advance(parser);

        const char *constraint_pactum_name = NULL;
        if (match(parser, TOKEN_PACTUM)) {
            if (!allow_pactum_constraint) {
                char msg[224];
                snprintf(msg, sizeof(msg),
                         "GENUS constraint 'T PACTUM C' is only supported in RITUALE declarations in subset 10D (subset final da FASE 10; not in %s)",
                         decl_kind ? decl_kind : "this declaration");
                error_at(parser, &parser->previous, msg);
            }

            if (!check(parser, TOKEN_IDENTIFIER)) {
                error_at_current(parser, "Expected contract identifier after PACTUM in GENUS parameter (subset 10D)");
            } else {
                constraint_pactum_name = parser->current.lexeme;
                advance(parser);
            }

            if (check(parser, TOKEN_PACTUM)) {
                error_at_current(parser,
                                 "multiple PACTUM constraints for one GENUS parameter are outside subset 10D (subset final da FASE 10 uses exactly one)");
                while (match(parser, TOKEN_PACTUM)) {
                    if (check(parser, TOKEN_IDENTIFIER)) advance(parser);
                }
            }
        }

        if (type_param_list_contains(list, param_name)) {
            error_at_current(parser, "duplicate GENUS parameter in declaration (subset 10A)");
        } else {
            cct_ast_type_param_t *param = cct_ast_create_type_param(
                param_name, constraint_pactum_name, param_line, param_col
            );
            cct_ast_type_param_list_append(list, param);
        }
        free(param_name);

        if (!match(parser, TOKEN_COMMA)) break;
    }

    expect(parser, TOKEN_RPAREN, "Expected ')' after GENUS parameters");

    if (check(parser, TOKEN_GENUS)) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "multiple GENUS clauses are invalid for %s in subset 10A",
                 decl_kind ? decl_kind : "declaration");
        error_at_current(parser, msg);
    }

    return list;
}

static cct_ast_type_list_t* parse_optional_generic_type_args(cct_parser_t *parser, const char *context) {
    if (!check(parser, TOKEN_GENUS)) {
        return NULL;
    }

    cct_ast_type_list_t *list = cct_ast_create_type_list();
    advance(parser); /* GENUS */
    expect(parser, TOKEN_LPAREN, "Expected '(' after GENUS");

    if (check(parser, TOKEN_RPAREN)) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "GENUS() is invalid in subset 10B for %s (expected at least one type argument)",
                 context ? context : "generic application");
        error_at_current(parser, msg);
        advance(parser);
        return list;
    }

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RPAREN)) {
        cct_ast_type_t *arg = parse_type(parser);
        if (arg) cct_ast_type_list_append(list, arg);
        if (!match(parser, TOKEN_COMMA)) break;
    }

    expect(parser, TOKEN_RPAREN, "Expected ')' after GENUS type arguments");
    return list;
}

/* ========================================================================
 * Expression Parsing
 * ======================================================================== */

static cct_ast_node_list_t* parse_argument_list(cct_parser_t *parser) {
    cct_ast_node_list_t *args = cct_ast_create_node_list();

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            cct_ast_node_t *arg = parse_expression(parser);
            if (arg) {
                cct_ast_node_list_append(args, arg);
            }
        } while (match(parser, TOKEN_COMMA));
    }

    return args;
}

static cct_ast_node_t* parse_primary(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;

    if (match(parser, TOKEN_INTEGER)) {
        i64 value = strtoll(parser->previous.lexeme, NULL, 10);
        return cct_ast_create_literal_int(value, line, col);
    }

    if (match(parser, TOKEN_REAL)) {
        f64 value = strtod(parser->previous.lexeme, NULL);
        return cct_ast_create_literal_real(value, line, col);
    }

    if (match(parser, TOKEN_STRING)) {
        char *str = dup_string_contents(parser->previous.lexeme);
        cct_ast_node_t *node = cct_ast_create_literal_string(str, line, col);
        free(str);
        return node;
    }

    if (match(parser, TOKEN_VERUM)) {
        return cct_ast_create_literal_bool(true, line, col);
    }

    if (match(parser, TOKEN_FALSUM)) {
        return cct_ast_create_literal_bool(false, line, col);
    }

    if (match(parser, TOKEN_NIHIL)) {
        return cct_ast_create_literal_nihil(line, col);
    }

    if (match(parser, TOKEN_CONIURA)) {
        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "Expected ritual name after CONIURA");
            return NULL;
        }

        char *name = strdup(parser->current.lexeme);
        if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
        advance(parser);

        cct_ast_type_list_t *type_args = parse_optional_generic_type_args(parser, "CONIURA");

        expect(parser, TOKEN_LPAREN, "Expected '(' after ritual name");
        cct_ast_node_list_t *args = parse_argument_list(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after arguments");

        cct_ast_node_t *node = cct_ast_create_coniura(name, type_args, args, line, col);
        free(name);
        return node;
    }

    if (match(parser, TOKEN_OBSECRO)) {
        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "Expected builtin name after OBSECRO");
            return NULL;
        }

        char *name = strdup(parser->current.lexeme);
        if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
        advance(parser);

        expect(parser, TOKEN_LPAREN, "Expected '(' after builtin name");
        cct_ast_node_list_t *args = parse_argument_list(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after arguments");

        cct_ast_node_t *node = cct_ast_create_obsecro(name, args, line, col);
        free(name);
        return node;
    }

    if (match(parser, TOKEN_MENSURA)) {
        expect(parser, TOKEN_LPAREN, "Expected '(' after MENSURA");
        cct_ast_type_t *type = parse_type(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after type");
        return cct_ast_create_mensura(type, line, col);
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        if (strcmp(parser->previous.lexeme, "cast") == 0 && check(parser, TOKEN_GENUS)) {
            cct_ast_type_list_t *type_args = parse_optional_generic_type_args(parser, "cast");
            if (!type_args || type_args->count != 1) {
                error_at_current(parser, "cast requires exactly one target type via GENUS(T)");
                return NULL;
            }

            expect(parser, TOKEN_LPAREN, "Expected '(' after cast GENUS(T)");
            cct_ast_node_list_t *args = parse_argument_list(parser);
            expect(parser, TOKEN_RPAREN, "Expected ')' after cast argument");

            if (!args || args->count != 1) {
                error_at_current(parser, "cast requires exactly one source expression");
                return NULL;
            }

            return cct_ast_create_coniura("__cast", type_args, args, line, col);
        }
        return cct_ast_create_identifier(parser->previous.lexeme, line, col);
    }

    if (match(parser, TOKEN_LPAREN)) {
        cct_ast_node_t *expr = parse_expression(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }

    error_at_current(parser, "Expected expression");
    return NULL;
}

static cct_ast_node_t* parse_postfix(cct_parser_t *parser) {
    cct_ast_node_t *expr = parse_primary(parser);
    if (!expr) return NULL;

    while (true) {
        u32 line = parser->current.line;
        u32 col = parser->current.column;

        if (match(parser, TOKEN_LPAREN)) {
            cct_ast_node_list_t *args = parse_argument_list(parser);
            expect(parser, TOKEN_RPAREN, "Expected ')' after arguments");
            expr = cct_ast_create_call(expr, args, line, col);
            continue;
        }

        if (match(parser, TOKEN_DOT)) {
            if (!check(parser, TOKEN_IDENTIFIER)) {
                error_at_current(parser, "Expected field name after '.'");
                return expr;
            }
            char *field = strdup(parser->current.lexeme);
            if (!field) exit(CCT_ERROR_OUT_OF_MEMORY);
            advance(parser);
            expr = cct_ast_create_field_access(expr, field, line, col);
            free(field);
            continue;
        }

        if (match(parser, TOKEN_LBRACKET)) {
            cct_ast_node_t *index = parse_expression(parser);
            expect(parser, TOKEN_RBRACKET, "Expected ']' after index");
            expr = cct_ast_create_index_access(expr, index, line, col);
            continue;
        }

        break;
    }

    return expr;
}

static cct_ast_node_t* parse_unary(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;

    if (match(parser, TOKEN_MINUS) ||
        match(parser, TOKEN_PLUS) ||
        match(parser, TOKEN_NON) ||
        match(parser, TOKEN_NON_BIT) ||
        match(parser, TOKEN_STAR) ||
        match(parser, TOKEN_SPECULUM)) {
        cct_token_type_t op = parser->previous.type;
        cct_ast_node_t *operand = parse_unary(parser);
        return cct_ast_create_unary_op(op, operand, line, col);
    }

    return parse_postfix(parser);
}

static int get_precedence(cct_token_type_t type) {
    switch (type) {
        case TOKEN_STAR_STAR:
            return 12;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_SLASH_SLASH:
        case TOKEN_PERCENT:
        case TOKEN_PERCENT_PERCENT:
            return 11;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 10;
        case TOKEN_SINISTER:
        case TOKEN_DEXTER:
            return 9;
        case TOKEN_LESS:
        case TOKEN_LESS_EQ:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQ:
            return 8;
        case TOKEN_EQ_EQ:
        case TOKEN_BANG_EQ:
            return 7;
        case TOKEN_ET_BIT:
            return 6;
        case TOKEN_XOR:
            return 5;
        case TOKEN_VEL_BIT:
            return 4;
        case TOKEN_ET:
            return 3;
        case TOKEN_VEL:
            return 2;
        default:
            return 0;
    }
}

static cct_ast_node_t* parse_precedence(cct_parser_t *parser, int min_prec) {
    cct_ast_node_t *left = parse_unary(parser);
    if (!left) return NULL;

    while (true) {
        int prec = get_precedence(parser->current.type);
        if (prec < min_prec || prec == 0) break;

        u32 line = parser->current.line;
        u32 col = parser->current.column;
        cct_token_type_t op = parser->current.type;
        advance(parser);

        cct_ast_node_t *right = parse_precedence(
            parser,
            (op == TOKEN_STAR_STAR) ? prec : (prec + 1)
        );
        left = cct_ast_create_binary_op(op, left, right, line, col);
    }

    return left;
}

static cct_ast_node_t* parse_expression(cct_parser_t *parser) {
    return parse_precedence(parser, 1);
}

/* ========================================================================
 * Statement Parsing (ritual syntax)
 * ======================================================================== */

static cct_ast_node_t* parse_ritual_block_until_explicit(cct_parser_t *parser, cct_token_type_t keyword) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    cct_ast_node_list_t *stmts = cct_ast_create_node_list();

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_EXPLICIT)) {
        cct_ast_node_t *stmt = parse_statement(parser);
        if (stmt) cct_ast_node_list_append(stmts, stmt);
        if (parser->panic_mode) {
            synchronize(parser);
        }
    }

    consume_explicit_clause(parser, keyword, "Unexpected EXPLICIT clause");
    return cct_ast_create_block(stmts, line, col);
}

static cct_ast_node_t* parse_ritual_block_until_fin(cct_parser_t *parser, cct_token_type_t keyword, bool stop_on_aliter) {
    (void)keyword;
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    cct_ast_node_list_t *stmts = cct_ast_create_node_list();

    while (!check(parser, TOKEN_EOF)) {
        if (check(parser, TOKEN_FIN)) break;
        if (stop_on_aliter && check(parser, TOKEN_ALITER)) break;

        cct_ast_node_t *stmt = parse_statement(parser);
        if (stmt) cct_ast_node_list_append(stmts, stmt);
        if (parser->panic_mode) {
            synchronize(parser);
        }
    }

    return cct_ast_create_block(stmts, line, col);
}

static cct_ast_node_t* parse_ritual_block_until_token(cct_parser_t *parser, cct_token_type_t terminator) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    cct_ast_node_list_t *stmts = cct_ast_create_node_list();

    while (!check(parser, TOKEN_EOF) && !check(parser, terminator)) {
        cct_ast_node_t *stmt = parse_statement(parser);
        if (stmt) cct_ast_node_list_append(stmts, stmt);
        if (parser->panic_mode) synchronize(parser);
    }

    return cct_ast_create_block(stmts, line, col);
}

static cct_ast_node_t* parse_ritual_block_until_failure_clause(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    cct_ast_node_list_t *stmts = cct_ast_create_node_list();

    while (!check(parser, TOKEN_EOF)) {
        if (check(parser, TOKEN_CAPE) || check(parser, TOKEN_SEMPER) || check(parser, TOKEN_FIN)) break;
        cct_ast_node_t *stmt = parse_statement(parser);
        if (stmt) cct_ast_node_list_append(stmts, stmt);
        if (parser->panic_mode) synchronize(parser);
    }

    return cct_ast_create_block(stmts, line, col);
}

static cct_ast_node_t* parse_legacy_brace_block(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_LBRACE, "Expected '{' to start block");

    cct_ast_node_list_t *stmts = cct_ast_create_node_list();
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        cct_ast_node_t *stmt = parse_statement(parser);
        if (stmt) cct_ast_node_list_append(stmts, stmt);
        if (parser->panic_mode) synchronize(parser);
    }
    expect(parser, TOKEN_RBRACE, "Expected '}' after block");

    return cct_ast_create_block(stmts, line, col);
}

static cct_ast_node_t* parse_evoca(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_EVOCA, "Expected EVOCA");

    bool is_constans = false;
    bool parsed_modifier = true;
    while (parsed_modifier) {
        parsed_modifier = false;
        if (match(parser, TOKEN_CONSTANS)) {
            is_constans = true;
            parsed_modifier = true;
            continue;
        }
        if (match(parser, TOKEN_VOLATILE)) {
            parsed_modifier = true;
        }
    }

    cct_ast_type_t *type = parse_type(parser);

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected variable name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_node_t *init = NULL;
    if (match(parser, TOKEN_AD)) {
        init = parse_expression(parser);
    }

    consume_optional_semicolon(parser);

    cct_ast_node_t *node = cct_ast_create_evoca(type, name, init, is_constans, line, col);
    free(name);
    return node;
}

static cct_ast_node_t* parse_vincire(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_VINCIRE, "Expected VINCIRE");

    cct_ast_node_t *target = parse_unary(parser);
    expect(parser, TOKEN_AD, "Expected AD in VINCIRE statement");
    cct_ast_node_t *value = parse_expression(parser);

    consume_optional_semicolon(parser);
    return cct_ast_create_vincire(target, value, line, col);
}

static cct_ast_node_t* parse_redde(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_REDDE, "Expected REDDE");

    cct_ast_node_t *value = NULL;
    if (is_expression_start_token(parser->current.type) && !is_statement_terminator(parser->current.type)) {
        value = parse_expression(parser);
    }

    consume_optional_semicolon(parser);
    return cct_ast_create_redde(value, line, col);
}

static cct_ast_node_t* parse_anur(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_ANUR, "Expected ANUR");

    cct_ast_node_t *value = NULL;
    if (is_expression_start_token(parser->current.type) && !is_statement_terminator(parser->current.type)) {
        value = parse_expression(parser);
    }

    consume_optional_semicolon(parser);
    return cct_ast_create_anur(value, line, col);
}

static cct_ast_node_t* parse_dimitte(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_DIMITTE, "Expected DIMITTE");

    if (!is_expression_start_token(parser->current.type)) {
        error_at_current(parser, "Expected pointer symbol after DIMITTE");
        return NULL;
    }

    /* Keep syntax flexible for future phases; semantic/codegen will restrict subset. */
    cct_ast_node_t *target = parse_unary(parser);
    consume_optional_semicolon(parser);
    return cct_ast_create_dimitte(target, line, col);
}

static cct_ast_node_t* parse_iace(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_IACE, "Expected IACE");

    if (!is_expression_start_token(parser->current.type)) {
        error_at_current(parser, "IACE requires VERBUM or FRACTUM expression payload in FASE 8A");
        return NULL;
    }

    cct_ast_node_t *value = parse_expression(parser);
    consume_optional_semicolon(parser);
    return cct_ast_create_iace(value, line, col);
}

static void parse_skip_until_fin_tempta(cct_parser_t *parser) {
    while (!check(parser, TOKEN_EOF)) {
        if (check(parser, TOKEN_FIN)) {
            advance(parser);
            if (check(parser, TOKEN_TEMPTA)) {
                advance(parser);
                return;
            }
            continue;
        }
        advance(parser);
    }
}

static cct_ast_node_t* parse_tempta(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_TEMPTA, "Expected TEMPTA");

    cct_ast_node_t *try_block = parse_ritual_block_until_failure_clause(parser);

    if (match(parser, TOKEN_SEMPER)) {
        error_at(parser, &parser->previous,
                 "SEMPER in FASE 8C must appear after CAPE FRACTUM ident (subset final da FASE 8 preserves this order)");
        parse_skip_until_fin_tempta(parser);
        return cct_ast_create_tempta(try_block, NULL, NULL, NULL, NULL, line, col);
    }

    if (!match(parser, TOKEN_CAPE)) {
        error_at_current(parser,
                         "TEMPTA requires CAPE FRACTUM ident in subset 8A (subset final da FASE 8 keeps CAPE FRACTUM obligatorio)");
        if (check(parser, TOKEN_FIN)) {
            (void)consume_fin_clause(parser, TOKEN_TEMPTA, "Expected TEMPTA after FIN");
        } else {
            parse_skip_until_fin_tempta(parser);
        }
        return cct_ast_create_tempta(try_block, NULL, NULL, NULL, NULL, line, col);
    }

    cct_ast_type_t *cape_type = parse_type(parser);
    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected CAPE binding name after CAPE FRACTUM");
        parse_skip_until_fin_tempta(parser);
        return cct_ast_create_tempta(try_block, cape_type, NULL, NULL, NULL, line, col);
    }

    char *cape_name = strdup(parser->current.lexeme);
    if (!cape_name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_node_t *cape_block = parse_ritual_block_until_failure_clause(parser);
    cct_ast_node_t *semper_block = NULL;

    if (check(parser, TOKEN_CAPE)) {
        error_at_current(parser,
                         "Multiple CAPE handlers are outside subset 8B (subset final da FASE 8 allows exactly one CAPE)");
        parse_skip_until_fin_tempta(parser);
        cct_ast_node_t *node = cct_ast_create_tempta(try_block, cape_type, cape_name, cape_block, NULL, line, col);
        free(cape_name);
        return node;
    }

    if (match(parser, TOKEN_SEMPER)) {
        /* FASE 8C: optional guaranteed cleanup block after CAPE. */
        semper_block = parse_ritual_block_until_failure_clause(parser);
        if (check(parser, TOKEN_SEMPER)) {
            error_at_current(parser,
                             "Multiple SEMPER blocks are outside subset 8C (subset final da FASE 8 allows at most one SEMPER)");
            parse_skip_until_fin_tempta(parser);
            cct_ast_node_t *node = cct_ast_create_tempta(try_block, cape_type, cape_name, cape_block, semper_block, line, col);
            free(cape_name);
            return node;
        }
        if (check(parser, TOKEN_CAPE)) {
            error_at_current(parser, "SEMPER must appear after CAPE and before FIN TEMPTA (CAPE cannot follow SEMPER)");
            parse_skip_until_fin_tempta(parser);
            cct_ast_node_t *node = cct_ast_create_tempta(try_block, cape_type, cape_name, cape_block, semper_block, line, col);
            free(cape_name);
            return node;
        }
    }

    consume_fin_clause(parser, TOKEN_TEMPTA, "Expected TEMPTA after FIN");

    cct_ast_node_t *node = cct_ast_create_tempta(try_block, cape_type, cape_name, cape_block, semper_block, line, col);
    free(cape_name);
    return node;
}

static cct_ast_node_t* parse_si(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_SI, "Expected SI");

    cct_ast_node_t *cond = parse_condition_expr(parser);
    cct_ast_node_t *then_branch = parse_ritual_block_until_fin(parser, TOKEN_SI, true);

    cct_ast_node_t *else_branch = NULL;
    if (match(parser, TOKEN_ALITER)) {
        if (check(parser, TOKEN_SI)) {
            /* Support ALITER SI as nested SI inside the else branch. */
            cct_ast_node_list_t *else_list = cct_ast_create_node_list();
            cct_ast_node_t *nested = parse_si(parser);
            if (nested) cct_ast_node_list_append(else_list, nested);
            else_branch = cct_ast_create_block(else_list, line, col);
        } else {
            else_branch = parse_ritual_block_until_fin(parser, TOKEN_SI, false);
        }
    }

    consume_fin_clause(parser, TOKEN_SI, "Expected SI after FIN");
    return cct_ast_create_si(cond, then_branch, else_branch, line, col);
}

static cct_ast_node_t* parse_dum(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_DUM, "Expected DUM");

    cct_ast_node_t *cond = parse_condition_expr(parser);
    cct_ast_node_t *body = parse_ritual_block_until_fin(parser, TOKEN_DUM, false);

    consume_fin_clause(parser, TOKEN_DUM, "Expected DUM after FIN");
    return cct_ast_create_dum(cond, body, line, col);
}

static cct_ast_node_t* parse_donec(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_DONEC, "Expected DONEC");

    cct_ast_node_t *body = parse_ritual_block_until_token(parser, TOKEN_DUM);

    if (!check(parser, TOKEN_DUM)) {
        /* If we stopped at FIN, this is likely malformed DONEC body. */
        error_at_current(parser, "Expected DUM after DONEC body");
        return cct_ast_create_donec(body, NULL, line, col);
    }

    advance(parser); /* consume DUM */
    cct_ast_node_t *cond = parse_condition_expr(parser);
    consume_optional_semicolon(parser);

    return cct_ast_create_donec(body, cond, line, col);
}

static cct_ast_node_t* parse_repete(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_REPETE, "Expected REPETE");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected iterator name after REPETE");
        return NULL;
    }

    char *iter = strdup(parser->current.lexeme);
    if (!iter) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    expect(parser, TOKEN_DE, "Expected DE after iterator");
    cct_ast_node_t *start = parse_expression(parser);
    expect(parser, TOKEN_AD, "Expected AD after REPETE start value");
    cct_ast_node_t *end = parse_expression(parser);

    cct_ast_node_t *step = NULL;
    if (match(parser, TOKEN_GRADUS)) {
        step = parse_expression(parser);
    }

    cct_ast_node_t *body = parse_ritual_block_until_fin(parser, TOKEN_REPETE, false);
    consume_fin_clause(parser, TOKEN_REPETE, "Expected REPETE after FIN");

    cct_ast_node_t *node = cct_ast_create_repete(iter, start, end, step, body, line, col);
    free(iter);
    return node;
}

static cct_ast_node_t* parse_iterum(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_ITERUM, "Expected ITERUM");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected iterator item name after ITERUM");
        return NULL;
    }
    char *item_name = strdup(parser->current.lexeme);
    if (!item_name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    expect(parser, TOKEN_IN, "Expected IN after ITERUM item name");
    cct_ast_node_t *collection = parse_expression(parser);
    if (check(parser, TOKEN_IDENTIFIER) && parser->current.lexeme &&
        strcmp(parser->current.lexeme, "COM") == 0) {
        advance(parser);
    }

    cct_ast_node_t *body = parse_ritual_block_until_fin(parser, TOKEN_ITERUM, false);
    consume_fin_clause(parser, TOKEN_ITERUM, "Expected ITERUM after FIN");

    cct_ast_node_t *node = cct_ast_create_iterum(item_name, collection, body, line, col);
    free(item_name);
    return node;
}

static cct_ast_node_t* parse_statement(cct_parser_t *parser) {
    if (check(parser, TOKEN_LBRACE)) {
        return parse_legacy_brace_block(parser);
    }

    if (check(parser, TOKEN_EVOCA)) return parse_evoca(parser);
    if (check(parser, TOKEN_VINCIRE)) return parse_vincire(parser);
    if (check(parser, TOKEN_REDDE)) return parse_redde(parser);
    if (check(parser, TOKEN_ANUR)) return parse_anur(parser);
    if (check(parser, TOKEN_DIMITTE)) return parse_dimitte(parser);
    if (check(parser, TOKEN_IACE)) return parse_iace(parser);
    if (check(parser, TOKEN_TEMPTA)) return parse_tempta(parser);
    if (check(parser, TOKEN_CAPE)) {
        error_at_current(parser, "CAPE can only appear inside TEMPTA");
        return NULL;
    }
    if (check(parser, TOKEN_SEMPER)) {
        error_at_current(parser, "SEMPER can only appear inside TEMPTA after CAPE");
        return NULL;
    }
    if (check(parser, TOKEN_ARCANUM)) {
        error_at_current(parser, "ARCANUM is only valid for top-level visibility markers in subset 9C");
        return NULL;
    }
    if (check(parser, TOKEN_GENUS)) {
        error_at_current(parser, "GENUS in subset 10A only supports RITUALE and SIGILLUM declarations");
        return NULL;
    }

    if (check(parser, TOKEN_SI)) return parse_si(parser);
    if (check(parser, TOKEN_DUM)) return parse_dum(parser);
    if (check(parser, TOKEN_DONEC)) return parse_donec(parser);
    if (check(parser, TOKEN_REPETE)) return parse_repete(parser);
    if (check(parser, TOKEN_ITERUM)) return parse_iterum(parser);

    if (match(parser, TOKEN_FRANGE)) {
        u32 line = parser->previous.line;
        u32 col = parser->previous.column;
        consume_optional_semicolon(parser);
        return cct_ast_create_frange(line, col);
    }

    if (match(parser, TOKEN_RECEDE)) {
        u32 line = parser->previous.line;
        u32 col = parser->previous.column;
        consume_optional_semicolon(parser);
        return cct_ast_create_recede(line, col);
    }

    if (match(parser, TOKEN_TRANSITUS)) {
        u32 line = parser->previous.line;
        u32 col = parser->previous.column;
        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "Expected label after TRANSITUS");
            return NULL;
        }
        char *label = strdup(parser->current.lexeme);
        if (!label) exit(CCT_ERROR_OUT_OF_MEMORY);
        advance(parser);
        consume_optional_semicolon(parser);
        cct_ast_node_t *node = cct_ast_create_transitus(label, line, col);
        free(label);
        return node;
    }

    if (is_statement_terminator(parser->current.type)) {
        error_at_current(parser, "Unexpected block terminator");
        return NULL;
    }

    /* Expression statement */
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    cct_ast_node_t *expr = parse_expression(parser);
    consume_optional_semicolon(parser);
    return cct_ast_create_expr_stmt(expr, line, col);
}

/* ========================================================================
 * Declaration Parsing
 * ======================================================================== */

static cct_ast_param_t* parse_parameter(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;

    bool is_constans = false;
    bool parsed_modifier = true;
    while (parsed_modifier) {
        parsed_modifier = false;
        if (match(parser, TOKEN_CONSTANS)) {
            is_constans = true;
            parsed_modifier = true;
            continue;
        }
        if (match(parser, TOKEN_VOLATILE)) {
            parsed_modifier = true;
        }
    }

    cct_ast_type_t *type = parse_type(parser);

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected parameter name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_param_t *param = cct_ast_create_param(name, type, is_constans, line, col);
    free(name);
    return param;
}

static cct_ast_param_list_t* parse_parameter_list(cct_parser_t *parser) {
    cct_ast_param_list_t *params = cct_ast_create_param_list();

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            cct_ast_param_t *param = parse_parameter(parser);
            if (param) cct_ast_param_list_append(params, param);
        } while (match(parser, TOKEN_COMMA));
    }

    return params;
}

static cct_ast_node_t* parse_rituale_signature_only(cct_parser_t *parser, u32 line, u32 col, char *name) {
    cct_ast_type_param_list_t *type_params = cct_ast_create_type_param_list();
    if (check(parser, TOKEN_GENUS)) {
        error_at_current(parser, "GENUS is outside subset 10A for PACTUM signatures");
        cct_ast_free_type_param_list(type_params);
        type_params = parse_optional_type_params(parser, "PACTUM RITUALE signature", false);
    }

    cct_ast_param_list_t *params = NULL;

    if (match(parser, TOKEN_LPAREN)) {
        params = parse_parameter_list(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    } else {
        params = cct_ast_create_param_list();
    }

    cct_ast_type_t *return_type = NULL;
    if (match(parser, TOKEN_REDDE)) {
        return_type = parse_type(parser);
    }

    consume_optional_semicolon(parser);
    cct_ast_node_t *sig = cct_ast_create_rituale(name, type_params, params, return_type, NULL, line, col);
    return sig;
}

static cct_ast_node_t* parse_rituale(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_RITUALE, "Expected RITUALE");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected ritual name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_type_param_list_t *type_params = parse_optional_type_params(parser, "RITUALE", true);

    cct_ast_param_list_t *params = NULL;
    if (match(parser, TOKEN_LPAREN)) {
        params = parse_parameter_list(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    } else {
        params = cct_ast_create_param_list();
    }

    cct_ast_type_t *return_type = NULL;
    if (match(parser, TOKEN_REDDE)) {
        return_type = parse_type(parser);
    }

    cct_ast_node_t *body = NULL;
    if (check(parser, TOKEN_LBRACE)) {
        body = parse_legacy_brace_block(parser);
    } else {
        body = parse_ritual_block_until_explicit(parser, TOKEN_RITUALE);
    }

    cct_ast_node_t *node = cct_ast_create_rituale(name, type_params, params, return_type, body, line, col);
    free(name);
    return node;
}

static cct_ast_field_t* parse_sigillum_field(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected field name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    expect(parser, TOKEN_COLON, "Expected ':' after field name");
    cct_ast_type_t *type = parse_type(parser);
    consume_optional_semicolon(parser);

    cct_ast_field_t *field = cct_ast_create_field(name, type, line, col);
    free(name);
    return field;
}

static cct_ast_node_t* parse_sigillum(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_SIGILLUM, "Expected SIGILLUM");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected SIGILLUM name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_type_param_list_t *type_params = parse_optional_type_params(parser, "SIGILLUM", false);
    char *pactum_name = NULL;
    if (match(parser, TOKEN_PACTUM)) {
        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "Expected PACTUM name after SIGILLUM ... PACTUM");
        } else {
            pactum_name = strdup(parser->current.lexeme);
            if (!pactum_name) exit(CCT_ERROR_OUT_OF_MEMORY);
            advance(parser);
        }
    }

    cct_ast_field_list_t *fields = cct_ast_create_field_list();
    cct_ast_node_list_t *methods = cct_ast_create_node_list();

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_FIN)) {
        if (check(parser, TOKEN_RITUALE)) {
            cct_ast_node_t *method = parse_rituale(parser);
            if (method) cct_ast_node_list_append(methods, method);
        } else if (check(parser, TOKEN_GENUS)) {
            error_at_current(parser, "GENUS in subset 10A only supports RITUALE/SIGILLUM declaration headers");
            synchronize(parser);
        } else if (check(parser, TOKEN_ARCANUM)) {
            error_at_current(parser, "ARCANUM is not supported inside SIGILLUM body in subset 9C");
            synchronize(parser);
        } else if (check(parser, TOKEN_PACTUM)) {
            advance(parser); /* consume PACTUM */
            if (check(parser, TOKEN_IDENTIFIER)) {
                if (pactum_name) {
                    error_at_current(parser,
                                     "multiple PACTUM clauses in SIGILLUM are outside subset 10C (use exactly one PACTUM)");
                } else {
                    error_at_current(parser,
                                     "PACTUM clause must appear in SIGILLUM header: SIGILLUM X PACTUM Y (subset 10C)");
                }
                advance(parser); /* consume name to keep parser in sync */
            } else {
                error_at_current(parser, "Expected PACTUM name in SIGILLUM declaration");
            }
            synchronize(parser);
        } else {
            cct_ast_field_t *field = parse_sigillum_field(parser);
            if (field) cct_ast_field_list_append(fields, field);
        }
        if (parser->panic_mode) synchronize(parser);
    }

    consume_fin_clause(parser, TOKEN_SIGILLUM, "Expected SIGILLUM after FIN");

    cct_ast_node_t *node = cct_ast_create_sigillum(name, type_params, pactum_name, fields, methods, line, col);
    free(pactum_name);
    free(name);
    return node;
}

static cct_ast_enum_item_t* parse_enum_item(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected enum item name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    bool has_value = false;
    i64 value = 0;
    if (match(parser, TOKEN_EQ)) {
        has_value = true;
        if (!check(parser, TOKEN_INTEGER)) {
            error_at_current(parser, "Expected integer enum value");
        } else {
            value = strtoll(parser->current.lexeme, NULL, 10);
            advance(parser);
        }
    }

    cct_ast_enum_item_t *item = cct_ast_create_enum_item(name, value, has_value, line, col);
    free(name);
    return item;
}

static cct_ast_node_t* parse_ordo(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_ORDO, "Expected ORDO");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected ORDO name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    if (check(parser, TOKEN_GENUS)) {
        error_at_current(parser, "GENUS in subset 10A only supports RITUALE and SIGILLUM declarations");
        cct_ast_type_param_list_t *discard = parse_optional_type_params(parser, "ORDO", false);
        cct_ast_free_type_param_list(discard);
    }

    cct_ast_enum_item_list_t *items = cct_ast_create_enum_item_list();
    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_FIN)) {
        cct_ast_enum_item_t *item = parse_enum_item(parser);
        if (item) cct_ast_enum_item_list_append(items, item);
        (void)match(parser, TOKEN_COMMA);
        consume_optional_semicolon(parser);
        if (parser->panic_mode) synchronize(parser);
    }

    consume_fin_clause(parser, TOKEN_ORDO, "Expected ORDO after FIN");

    cct_ast_node_t *node = cct_ast_create_ordo(name, items, line, col);
    free(name);
    return node;
}

static cct_ast_node_t* parse_codex(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_CODEX, "Expected CODEX");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected CODEX name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_node_list_t *decls = cct_ast_create_node_list();
    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_FIN)) {
        cct_ast_node_t *decl = parse_declaration(parser);
        if (decl) cct_ast_node_list_append(decls, decl);
        if (parser->panic_mode) synchronize(parser);
    }

    consume_fin_clause(parser, TOKEN_CODEX, "Expected CODEX after FIN");

    cct_ast_node_t *node = cct_ast_create_codex(name, decls, line, col);
    free(name);
    return node;
}

static cct_ast_node_t* parse_pactum(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_PACTUM, "Expected PACTUM");

    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected PACTUM name");
        return NULL;
    }

    char *name = strdup(parser->current.lexeme);
    if (!name) exit(CCT_ERROR_OUT_OF_MEMORY);
    advance(parser);

    cct_ast_node_list_t *sigs = cct_ast_create_node_list();
    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_FIN)) {
        if (!match(parser, TOKEN_RITUALE)) {
            error_at_current(parser, "Expected RITUALE signature in PACTUM");
            synchronize(parser);
            continue;
        }

        u32 sig_line = parser->previous.line;
        u32 sig_col = parser->previous.column;

        if (!check(parser, TOKEN_IDENTIFIER)) {
            error_at_current(parser, "Expected ritual name in PACTUM");
            synchronize(parser);
            continue;
        }

        char *sig_name = strdup(parser->current.lexeme);
        if (!sig_name) exit(CCT_ERROR_OUT_OF_MEMORY);
        advance(parser);

        cct_ast_node_t *sig = parse_rituale_signature_only(parser, sig_line, sig_col, sig_name);
        if (sig) cct_ast_node_list_append(sigs, sig);
        free(sig_name);

        if (parser->panic_mode) synchronize(parser);
    }

    consume_fin_clause(parser, TOKEN_PACTUM, "Expected PACTUM after FIN");

    cct_ast_node_t *node = cct_ast_create_pactum(name, sigs, line, col);
    free(name);
    return node;
}

static cct_ast_node_t* parse_import(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_ADVOCARE, "Expected ADVOCARE");

    if (!check(parser, TOKEN_STRING)) {
        error_at_current(parser, "Expected filename string after ADVOCARE");
        return NULL;
    }

    char *filename = dup_string_contents(parser->current.lexeme);
    advance(parser);
    consume_optional_semicolon(parser);

    cct_ast_node_t *node = cct_ast_create_import(filename, line, col);
    free(filename);
    return node;
}

static cct_ast_node_t* parse_arcanum_top_level(cct_parser_t *parser) {
    u32 line = parser->current.line;
    u32 col = parser->current.column;
    expect(parser, TOKEN_ARCANUM, "Expected ARCANUM");

    cct_ast_node_t *decl = NULL;
    if (check(parser, TOKEN_RITUALE)) {
        decl = parse_rituale(parser);
    } else if (check(parser, TOKEN_SIGILLUM)) {
        decl = parse_sigillum(parser);
    } else if (check(parser, TOKEN_ORDO)) {
        decl = parse_ordo(parser);
    } else {
        error_at_current(
            parser,
            "ARCANUM in subset 9C only supports top-level RITUALE, SIGILLUM or ORDO declarations"
        );
        return NULL;
    }

    if (decl) {
        decl->is_internal = true;
        /* Preserve source location of ARCANUM marker for visibility diagnostics. */
        decl->line = line;
        decl->column = col;
    }
    return decl;
}

static cct_ast_node_t* parse_declaration(cct_parser_t *parser) {
    if (check(parser, TOKEN_ARCANUM)) return parse_arcanum_top_level(parser);
    if (check(parser, TOKEN_GENUS)) {
        error_at_current(parser, "GENUS in subset 10A only supports RITUALE and SIGILLUM declarations");
        return NULL;
    }
    if (check(parser, TOKEN_ADVOCARE)) return parse_import(parser);
    if (check(parser, TOKEN_RITUALE)) return parse_rituale(parser);
    if (check(parser, TOKEN_SIGILLUM)) return parse_sigillum(parser);
    if (check(parser, TOKEN_ORDO)) return parse_ordo(parser);
    if (check(parser, TOKEN_CODEX)) return parse_codex(parser);
    if (check(parser, TOKEN_PACTUM)) return parse_pactum(parser);

    error_at_current(parser, "Expected top-level declaration");
    return NULL;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void cct_parser_init(cct_parser_t *parser, cct_lexer_t *lexer, const char *filename) {
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->filename = filename;

    parser->current.type = TOKEN_EOF;
    parser->current.lexeme = NULL;
    parser->current.line = 0;
    parser->current.column = 0;

    parser->previous = parser->current;

    advance(parser);
}

void cct_parser_dispose(cct_parser_t *parser) {
    free_parser_token(&parser->previous);
    free_parser_token(&parser->current);
}

cct_ast_program_t* cct_parser_parse_program(cct_parser_t *parser) {
    expect(parser, TOKEN_INCIPIT, "Expected INCIPIT at start of program");

    if (!check(parser, TOKEN_IDENTIFIER) || strcmp(parser->current.lexeme, "grimoire") != 0) {
        error_at_current(parser, "Expected 'grimoire' after INCIPIT");
        return NULL;
    }
    advance(parser);

    if (!check(parser, TOKEN_STRING)) {
        error_at_current(parser, "Expected program name string");
        return NULL;
    }

    char *prog_name = dup_string_contents(parser->current.lexeme);
    advance(parser);

    cct_ast_program_t *program = cct_ast_create_program(prog_name);
    free(prog_name);

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_EXPLICIT)) {
        cct_ast_node_t *decl = parse_declaration(parser);
        if (decl) cct_ast_node_list_append(program->declarations, decl);
        if (parser->panic_mode) synchronize(parser);
        else if (!decl && !check(parser, TOKEN_EOF) && !check(parser, TOKEN_EXPLICIT)) {
            /* Ensure progress in non-panic unexpected paths. */
            advance(parser);
        }
    }

    if (!consume_explicit_clause(parser, TOKEN_IDENTIFIER, "Expected 'grimoire' after EXPLICIT")) {
        cct_ast_free_program(program);
        return NULL;
    }

    if (strcmp(parser->previous.lexeme, "grimoire") != 0) {
        /* previous is the identifier consumed by consume_explicit_clause */
        error_at(parser, &parser->previous, "Expected 'grimoire' after EXPLICIT");
    }

    if (parser->had_error) {
        cct_ast_free_program(program);
        return NULL;
    }

    return program;
}

bool cct_parser_had_error(const cct_parser_t *parser) {
    return parser->had_error;
}
