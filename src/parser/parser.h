/*
 * CCT — Clavicula Turing
 * Parser Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PARSER_H
#define CCT_PARSER_H

#include "../common/types.h"
#include "../common/errors.h"
#include "../lexer/lexer.h"
#include "ast.h"

/*
 * Parser state
 *
 * Maintains current position in token stream and error state
 */
typedef struct {
    cct_lexer_t *lexer;      /* Lexer for getting tokens */
    cct_token_t current;     /* Current token */
    cct_token_t previous;    /* Previous token */
    bool had_error;          /* Whether any error occurred */
    bool panic_mode;         /* Whether in error recovery */
    const char *filename;    /* Source filename */
} cct_parser_t;

/*
 * Initialize parser with lexer
 *
 * Args:
 *   parser: Parser structure to initialize
 *   lexer: Initialized lexer with source code
 *   filename: Name of source file (for error reporting)
 */
void cct_parser_init(cct_parser_t *parser, cct_lexer_t *lexer, const char *filename);
void cct_parser_dispose(cct_parser_t *parser);

/*
 * Parse complete program
 *
 * Returns:
 *   AST program node (caller must free)
 *   NULL on parse error
 */
cct_ast_program_t* cct_parser_parse_program(cct_parser_t *parser);

/*
 * Check if parser had any errors
 */
bool cct_parser_had_error(const cct_parser_t *parser);

#endif /* CCT_PARSER_H */
