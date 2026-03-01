/*
 * CCT — Clavicula Turing
 * Lexical Analyzer
 *
 * FASE 1: Complete tokenization implementation
 *
 * This module handles tokenization of .cct source files:
 * - Keyword recognition (INCIPIT, EVOCA, VINCIRE, etc.)
 * - Number, string, identifier parsing
 * - Comment handling (-- style)
 * - Line/column tracking for error reporting
 * - Operator and punctuation recognition
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_LEXER_H
#define CCT_LEXER_H

#include "../common/types.h"
#include "../common/errors.h"
#include <stdio.h>

/*
 * Token Types
 *
 * Comprehensive set of all lexical elements in CCT
 */
typedef enum {
    /* Special tokens */
    TOKEN_EOF,
    TOKEN_INVALID,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_REAL,
    TOKEN_STRING,

    /* Keywords - Structure */
    TOKEN_INCIPIT,          /* begin */
    TOKEN_EXPLICIT,         /* end */
    TOKEN_RITUALE,          /* function */
    TOKEN_EVOCA,            /* declare variable */
    TOKEN_VINCIRE,          /* assign (bind) */
    TOKEN_DIMITTE,          /* free memory */
    TOKEN_CIRCULUS,         /* scope block */
    TOKEN_CONIURA,          /* call function */
    TOKEN_CODEX,            /* namespace/module */
    TOKEN_SIGILLUM,         /* struct/class */
    TOKEN_PACTUM,           /* interface */
    TOKEN_ORDO,             /* enum */
    TOKEN_ARCANUM,          /* internal visibility marker */
    TOKEN_GENUS,            /* generic parameter clause */
    TOKEN_ADVOCARE,         /* import */
    TOKEN_CONSTANS,         /* const */
    TOKEN_FIN,              /* end of block */

    /* Keywords - Control Flow */
    TOKEN_SI,               /* if */
    TOKEN_ALITER,           /* else */
    TOKEN_DUM,              /* while */
    TOKEN_DONEC,            /* do while */
    TOKEN_REPETE,           /* for */
    TOKEN_ITERUM,           /* collection iterator loop */
    TOKEN_PRO,              /* for each */
    TOKEN_FRANGE,           /* break */
    TOKEN_RECEDE,           /* continue */
    TOKEN_REDDE,            /* return */
    TOKEN_TRANSITUS,        /* goto */

    /* Keywords - Exception Handling */
    TOKEN_TEMPTA,           /* try */
    TOKEN_CAPE,             /* catch */
    TOKEN_SEMPER,           /* finally */
    TOKEN_IACE,             /* throw */
    TOKEN_FRACTUM,          /* exception type */

    /* Keywords - Logical/Bitwise */
    TOKEN_ET,               /* && logical and */
    TOKEN_VEL,              /* || logical or */
    TOKEN_NON,              /* ! logical not */
    TOKEN_ET_BIT,           /* & bitwise and */
    TOKEN_VEL_BIT,          /* | bitwise or */
    TOKEN_XOR,              /* ^ bitwise xor */
    TOKEN_NON_BIT,          /* ~ bitwise not */
    TOKEN_SINISTER,         /* << shift left */
    TOKEN_DEXTER,           /* >> shift right */

    /* Keywords - Types (primitives) */
    TOKEN_REX,              /* int64 */
    TOKEN_DUX,              /* int32 */
    TOKEN_COMES,            /* int16 */
    TOKEN_MILES,            /* uint8 */
    TOKEN_UMBRA,            /* double */
    TOKEN_FLAMMA,           /* float */
    TOKEN_VERBUM,           /* string */
    TOKEN_VERUM,            /* true */
    TOKEN_FALSUM,           /* false */
    TOKEN_NIHIL,            /* void/null */

    /* Keywords - Type Modifiers */
    TOKEN_SPECULUM,         /* pointer */
    TOKEN_SERIES,           /* array */
    TOKEN_FLUXUS,           /* dynamic array */
    TOKEN_VOLATILE,         /* volatile */

    /* Keywords - Special Constructs */
    TOKEN_OBSECRO,          /* syscall/builtin */
    TOKEN_ANUR,             /* exit */
    TOKEN_MENSURA,          /* sizeof */
    TOKEN_DE,               /* from */
    TOKEN_AD,               /* to/at */
    TOKEN_IN,               /* in */
    TOKEN_GRADUS,           /* step */

    /* Operators - Arithmetic */
    TOKEN_PLUS,             /* + */
    TOKEN_MINUS,            /* - */
    TOKEN_STAR,             /* * */
    TOKEN_SLASH,            /* / */
    TOKEN_PERCENT,          /* % */

    /* Operators - Comparison */
    TOKEN_EQ_EQ,            /* == */
    TOKEN_BANG_EQ,          /* != */
    TOKEN_LESS,             /* < */
    TOKEN_LESS_EQ,          /* <= */
    TOKEN_GREATER,          /* > */
    TOKEN_GREATER_EQ,       /* >= */

    /* Operators - Assignment and Other */
    TOKEN_EQ,               /* = */
    TOKEN_PLUS_PLUS,        /* ++ */
    TOKEN_MINUS_MINUS,      /* -- (if not comment) */

    /* Punctuation */
    TOKEN_LPAREN,           /* ( */
    TOKEN_RPAREN,           /* ) */
    TOKEN_LBRACE,           /* { */
    TOKEN_RBRACE,           /* } */
    TOKEN_LBRACKET,         /* [ */
    TOKEN_RBRACKET,         /* ] */
    TOKEN_COMMA,            /* , */
    TOKEN_DOT,              /* . */
    TOKEN_COLON,            /* : */
    TOKEN_SEMICOLON,        /* ; */
    TOKEN_QUESTION,         /* ? */

} cct_token_type_t;

/*
 * Token structure
 *
 * Represents a single lexical token with:
 * - type
 * - lexeme (actual text)
 * - source location (line, column)
 */
typedef struct {
    cct_token_type_t type;
    char *lexeme;           /* Owned string, must be freed */
    u32 line;
    u32 column;
} cct_token_t;

/*
 * Lexer state
 *
 * Maintains the current position and state during tokenization
 */
typedef struct {
    const char *source;     /* Source code string */
    size_t source_len;      /* Length of source */
    size_t current;         /* Current position in source */
    size_t start;           /* Start of current token */
    u32 line;               /* Current line number (1-based) */
    u32 column;             /* Current column number (1-based) */
    const char *filename;   /* Source filename (for errors) */
    bool had_error;         /* Whether any error occurred */
} cct_lexer_t;

/*
 * Initialize lexer with source code
 *
 * Args:
 *   lexer: Lexer structure to initialize
 *   source: Source code string (must remain valid during lexing)
 *   filename: Name of source file (for error reporting)
 */
void cct_lexer_init(cct_lexer_t *lexer, const char *source, const char *filename);

/*
 * Get next token from source
 *
 * Returns:
 *   Next token (caller must free token.lexeme when done)
 *   Returns TOKEN_EOF when end of source is reached
 *   Returns TOKEN_INVALID on lexical error
 */
cct_token_t cct_lexer_next_token(cct_lexer_t *lexer);

/*
 * Free token resources
 *
 * Frees the lexeme string owned by the token
 */
void cct_token_free(cct_token_t *token);

/*
 * Get string representation of token type
 *
 * Useful for debugging and display
 */
const char* cct_token_type_string(cct_token_type_t type);

/*
 * Check if lexer had any errors
 */
bool cct_lexer_had_error(const cct_lexer_t *lexer);

#endif /* CCT_LEXER_H */
