/*
 * CCT — Clavicula Turing
 * Lexical Analyzer Implementation
 *
 * FASE 1: Complete tokenization implementation
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "lexer.h"
#include "keywords.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================
 * Helper Functions - Character Classification
 * ======================================================================== */

static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c == '_');
}

static inline bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static inline bool is_at_end(const cct_lexer_t *lexer) {
    return lexer->current >= lexer->source_len;
}

/* ========================================================================
 * Helper Functions - Character Navigation
 * ======================================================================== */

static char peek(const cct_lexer_t *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

static char peek_next(const cct_lexer_t *lexer) {
    if (lexer->current + 1 >= lexer->source_len) return '\0';
    return lexer->source[lexer->current + 1];
}

static char advance(cct_lexer_t *lexer) {
    if (is_at_end(lexer)) return '\0';
    char c = lexer->source[lexer->current++];
    lexer->column++;
    return c;
}

static bool match(cct_lexer_t *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (lexer->source[lexer->current] != expected) return false;

    lexer->current++;
    lexer->column++;
    return true;
}

/* ========================================================================
 * Helper Functions - Whitespace and Comments
 * ======================================================================== */

static void skip_whitespace(cct_lexer_t *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 0;
                advance(lexer);
                break;
            default:
                return;
        }
    }
}

static void skip_comment(cct_lexer_t *lexer) {
    /* Skip until end of line */
    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
        advance(lexer);
    }
}

/* ========================================================================
 * Helper Functions - Token Creation
 * ======================================================================== */

static char* make_lexeme(const cct_lexer_t *lexer) {
    size_t length = lexer->current - lexer->start;
    char *lexeme = (char*)malloc(length + 1);
    if (!lexeme) {
        fprintf(stderr, "cct: fatal: out of memory\n");
        exit(CCT_ERROR_OUT_OF_MEMORY);
    }
    memcpy(lexeme, lexer->source + lexer->start, length);
    lexeme[length] = '\0';
    return lexeme;
}

static cct_token_t make_token(cct_lexer_t *lexer, cct_token_type_t type) {
    cct_token_t token;
    token.type = type;
    token.lexeme = make_lexeme(lexer);
    token.line = lexer->line;
    token.column = lexer->column - (lexer->current - lexer->start);
    return token;
}

static cct_token_t error_token(cct_lexer_t *lexer, const char *message) {
    lexer->had_error = true;

    /* Report error */
    cct_error_at_location(
        CCT_ERROR_LEXICAL,
        lexer->filename,
        lexer->line,
        lexer->column,
        message
    );

    cct_token_t token;
    token.type = TOKEN_INVALID;
    token.lexeme = strdup(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

/* ========================================================================
 * Token Recognition - Keywords and Identifiers
 * ======================================================================== */

static cct_token_type_t check_keyword(const char *lexeme) {
    /* Linear search through keyword table */
    for (size_t i = 0; i < CCT_KEYWORD_COUNT; i++) {
        if (strcmp(lexeme, cct_keywords[i].keyword) == 0) {
            return cct_keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static cct_token_t identifier(cct_lexer_t *lexer) {
    while (is_alnum(peek(lexer))) {
        advance(lexer);
    }

    /* Check if it's a keyword */
    char *lexeme = make_lexeme(lexer);
    cct_token_type_t type = check_keyword(lexeme);

    cct_token_t token;
    token.type = type;
    token.lexeme = lexeme;
    token.line = lexer->line;
    token.column = lexer->column - (lexer->current - lexer->start);

    return token;
}

/* ========================================================================
 * Token Recognition - Numbers
 * ======================================================================== */

static cct_token_t number(cct_lexer_t *lexer) {
    /* Consume all digits */
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }

    /* Check for decimal point */
    bool is_real = false;
    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        is_real = true;
        advance(lexer);  /* Consume '.' */

        /* Consume fractional digits */
        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }

    cct_token_type_t type = is_real ? TOKEN_REAL : TOKEN_INTEGER;
    return make_token(lexer, type);
}

/* ========================================================================
 * Token Recognition - Strings
 * ======================================================================== */

static cct_token_t string(cct_lexer_t *lexer) {
    /* Starting quote already consumed */

    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        char c = peek(lexer);

        /* Handle newline in string (error) */
        if (c == '\n') {
            return error_token(lexer, "Unterminated string");
        }

        /* Handle escape sequences */
        if (c == '\\') {
            advance(lexer);  /* Consume '\\' */
            if (is_at_end(lexer)) {
                return error_token(lexer, "Unterminated string");
            }

            char next = peek(lexer);
            /* Validate basic escapes: \", \\, \n, \t */
            if (next != '"' && next != '\\' && next != 'n' && next != 't') {
                return error_token(lexer, "Invalid escape sequence");
            }
        }

        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }

    /* Consume closing quote */
    advance(lexer);

    return make_token(lexer, TOKEN_STRING);
}

/* ========================================================================
 * Main Tokenization Function
 * ======================================================================== */

cct_token_t cct_lexer_next_token(cct_lexer_t *lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance(lexer);

    /* Identifiers and keywords */
    if (is_alpha(c)) {
        return identifier(lexer);
    }

    /* Numbers */
    if (is_digit(c)) {
        return number(lexer);
    }

    /* Two-character tokens and comments */
    switch (c) {
        case '-':
            /* Check for comment '--' */
            if (peek(lexer) == '-') {
                advance(lexer);  /* Consume second '-' */
                skip_comment(lexer);
                return cct_lexer_next_token(lexer);  /* Recursively get next token */
            }
            /* Check for decrement '--' (rare in our language) */
            if (match(lexer, '-')) return make_token(lexer, TOKEN_MINUS_MINUS);
            return make_token(lexer, TOKEN_MINUS);

        case '+':
            if (match(lexer, '+')) return make_token(lexer, TOKEN_PLUS_PLUS);
            return make_token(lexer, TOKEN_PLUS);

        case '=':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_EQ_EQ);
            return make_token(lexer, TOKEN_EQ);

        case '!':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_BANG_EQ);
            /* '!' alone is not valid in CCT - we use 'NON' keyword */
            return error_token(lexer, "Unexpected character '!'");

        case '<':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_LESS_EQ);
            return make_token(lexer, TOKEN_LESS);

        case '>':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_GREATER_EQ);
            return make_token(lexer, TOKEN_GREATER);

        /* Single-character tokens */
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return make_token(lexer, TOKEN_DOT);
        case ':': return make_token(lexer, TOKEN_COLON);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case '*':
            if (match(lexer, '*')) return make_token(lexer, TOKEN_STAR_STAR);
            return make_token(lexer, TOKEN_STAR);
        case '/':
            if (match(lexer, '/')) return make_token(lexer, TOKEN_SLASH_SLASH);
            return make_token(lexer, TOKEN_SLASH);
        case '%':
            if (match(lexer, '%')) return make_token(lexer, TOKEN_PERCENT_PERCENT);
            return make_token(lexer, TOKEN_PERCENT);
        case '?': return make_token(lexer, TOKEN_QUESTION);

        /* String literals */
        case '"': return string(lexer);

        default: {
            /* Unknown character */
            char msg[64];
            snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
            return error_token(lexer, msg);
        }
    }
}

/* ========================================================================
 * Public API Functions
 * ======================================================================== */

void cct_lexer_init(cct_lexer_t *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->source_len = strlen(source);
    lexer->current = 0;
    lexer->start = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename;
    lexer->had_error = false;
}

void cct_token_free(cct_token_t *token) {
    if (token->lexeme) {
        free(token->lexeme);
        token->lexeme = NULL;
    }
}

bool cct_lexer_had_error(const cct_lexer_t *lexer) {
    return lexer->had_error;
}

const char* cct_token_type_string(cct_token_type_t type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_INVALID: return "INVALID";

        /* Literals */
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_REAL: return "REAL";
        case TOKEN_STRING: return "STRING";

        /* Keywords - Structure */
        case TOKEN_INCIPIT: return "INCIPIT";
        case TOKEN_EXPLICIT: return "EXPLICIT";
        case TOKEN_RITUALE: return "RITUALE";
        case TOKEN_EVOCA: return "EVOCA";
        case TOKEN_VINCIRE: return "VINCIRE";
        case TOKEN_DIMITTE: return "DIMITTE";
        case TOKEN_CIRCULUS: return "CIRCULUS";
        case TOKEN_CONIURA: return "CONIURA";
        case TOKEN_CODEX: return "CODEX";
        case TOKEN_SIGILLUM: return "SIGILLUM";
        case TOKEN_PACTUM: return "PACTUM";
        case TOKEN_ORDO: return "ORDO";
        case TOKEN_ARCANUM: return "ARCANUM";
        case TOKEN_GENUS: return "GENUS";
        case TOKEN_ADVOCARE: return "ADVOCARE";
        case TOKEN_CONSTANS: return "CONSTANS";
        case TOKEN_FIN: return "FIN";

        /* Keywords - Control Flow */
        case TOKEN_SI: return "SI";
        case TOKEN_ALITER: return "ALITER";
        case TOKEN_DUM: return "DUM";
        case TOKEN_DONEC: return "DONEC";
        case TOKEN_REPETE: return "REPETE";
        case TOKEN_ITERUM: return "ITERUM";
        case TOKEN_PRO: return "PRO";
        case TOKEN_FRANGE: return "FRANGE";
        case TOKEN_RECEDE: return "RECEDE";
        case TOKEN_REDDE: return "REDDE";
        case TOKEN_TRANSITUS: return "TRANSITUS";

        /* Keywords - Exception Handling */
        case TOKEN_TEMPTA: return "TEMPTA";
        case TOKEN_CAPE: return "CAPE";
        case TOKEN_SEMPER: return "SEMPER";
        case TOKEN_IACE: return "IACE";
        case TOKEN_FRACTUM: return "FRACTUM";

        /* Keywords - Logical/Bitwise */
        case TOKEN_ET: return "ET";
        case TOKEN_VEL: return "VEL";
        case TOKEN_NON: return "NON";
        case TOKEN_ET_BIT: return "ET_BIT";
        case TOKEN_VEL_BIT: return "VEL_BIT";
        case TOKEN_XOR: return "XOR";
        case TOKEN_NON_BIT: return "NON_BIT";
        case TOKEN_SINISTER: return "SINISTER";
        case TOKEN_DEXTER: return "DEXTER";

        /* Keywords - Types */
        case TOKEN_REX: return "REX";
        case TOKEN_DUX: return "DUX";
        case TOKEN_COMES: return "COMES";
        case TOKEN_MILES: return "MILES";
        case TOKEN_UMBRA: return "UMBRA";
        case TOKEN_FLAMMA: return "FLAMMA";
        case TOKEN_VERBUM: return "VERBUM";
        case TOKEN_VERUM: return "VERUM";
        case TOKEN_FALSUM: return "FALSUM";
        case TOKEN_NIHIL: return "NIHIL";

        /* Keywords - Type Modifiers */
        case TOKEN_SPECULUM: return "SPECULUM";
        case TOKEN_SERIES: return "SERIES";
        case TOKEN_FLUXUS: return "FLUXUS";
        case TOKEN_VOLATILE: return "VOLATILE";

        /* Keywords - Special */
        case TOKEN_OBSECRO: return "OBSECRO";
        case TOKEN_ANUR: return "ANUR";
        case TOKEN_MENSURA: return "MENSURA";
        case TOKEN_DE: return "DE";
        case TOKEN_AD: return "AD";
        case TOKEN_IN: return "IN";
        case TOKEN_GRADUS: return "GRADUS";

        /* Operators */
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_STAR_STAR: return "STAR_STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_SLASH_SLASH: return "SLASH_SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_PERCENT_PERCENT: return "PERCENT_PERCENT";
        case TOKEN_EQ_EQ: return "EQ_EQ";
        case TOKEN_BANG_EQ: return "BANG_EQ";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQ: return "LESS_EQ";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQ: return "GREATER_EQ";
        case TOKEN_EQ: return "EQ";
        case TOKEN_PLUS_PLUS: return "PLUS_PLUS";
        case TOKEN_MINUS_MINUS: return "MINUS_MINUS";

        /* Punctuation */
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_QUESTION: return "QUESTION";

        default: return "UNKNOWN";
    }
}
