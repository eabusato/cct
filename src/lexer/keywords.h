/*
 * CCT — Clavicula Turing
 * Keyword Table
 *
 * FASE 1: Keyword lookup table
 *
 * This file contains the mapping of keyword strings to token types.
 * Keywords are case-sensitive and derived from Latin.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_KEYWORDS_H
#define CCT_KEYWORDS_H

#include "lexer.h"

/*
 * Keyword entry
 */
typedef struct {
    const char *keyword;
    cct_token_type_t type;
} cct_keyword_entry_t;

/*
 * Keyword table
 *
 * All CCT keywords in alphabetical order for easier maintenance
 * and potential binary search optimization in the future
 */
static const cct_keyword_entry_t cct_keywords[] = {
    /* A */
    {"AD", TOKEN_AD},
    {"ADVOCARE", TOKEN_ADVOCARE},
    {"ALITER", TOKEN_ALITER},
    {"ANUR", TOKEN_ANUR},
    {"ARCANUM", TOKEN_ARCANUM},

    /* C */
    {"CAPE", TOKEN_CAPE},
    {"CIRCULUS", TOKEN_CIRCULUS},
    {"CODEX", TOKEN_CODEX},
    {"COMES", TOKEN_COMES},
    {"CONIURA", TOKEN_CONIURA},
    {"CONSTANS", TOKEN_CONSTANS},

    /* D */
    {"DE", TOKEN_DE},
    {"DEXTER", TOKEN_DEXTER},
    {"DIMITTE", TOKEN_DIMITTE},
    {"DONEC", TOKEN_DONEC},
    {"DUM", TOKEN_DUM},
    {"DUX", TOKEN_DUX},

    /* E */
    {"ET", TOKEN_ET},
    {"ET_BIT", TOKEN_ET_BIT},
    {"EVOCA", TOKEN_EVOCA},
    {"EXPLICIT", TOKEN_EXPLICIT},

    /* F */
    {"FALSUM", TOKEN_FALSUM},
    {"FIN", TOKEN_FIN},
    {"FLAMMA", TOKEN_FLAMMA},
    {"FLUXUS", TOKEN_FLUXUS},
    {"FRACTUM", TOKEN_FRACTUM},
    {"FRANGE", TOKEN_FRANGE},

    /* G */
    {"GENUS", TOKEN_GENUS},
    {"GRADUS", TOKEN_GRADUS},

    /* I */
    {"IACE", TOKEN_IACE},
    {"IN", TOKEN_IN},
    {"INCIPIT", TOKEN_INCIPIT},
    {"ITERUM", TOKEN_ITERUM},

    /* M */
    {"MENSURA", TOKEN_MENSURA},
    {"MILES", TOKEN_MILES},

    /* N */
    {"NIHIL", TOKEN_NIHIL},
    {"NON", TOKEN_NON},
    {"NON_BIT", TOKEN_NON_BIT},

    /* O */
    {"OBSECRO", TOKEN_OBSECRO},
    {"ORDO", TOKEN_ORDO},

    /* P */
    {"PACTUM", TOKEN_PACTUM},
    {"PRO", TOKEN_PRO},

    /* R */
    {"RECEDE", TOKEN_RECEDE},
    {"REDDE", TOKEN_REDDE},
    {"REPETE", TOKEN_REPETE},
    {"REX", TOKEN_REX},
    {"RITUALE", TOKEN_RITUALE},

    /* S */
    {"SEMPER", TOKEN_SEMPER},
    {"SERIES", TOKEN_SERIES},
    {"SI", TOKEN_SI},
    {"SIGILLUM", TOKEN_SIGILLUM},
    {"SINISTER", TOKEN_SINISTER},
    {"SPECULUM", TOKEN_SPECULUM},

    /* T */
    {"TEMPTA", TOKEN_TEMPTA},
    {"TRANSITUS", TOKEN_TRANSITUS},

    /* U */
    {"UMBRA", TOKEN_UMBRA},

    /* V */
    {"VEL", TOKEN_VEL},
    {"VEL_BIT", TOKEN_VEL_BIT},
    {"VERBUM", TOKEN_VERBUM},
    {"VERUM", TOKEN_VERUM},
    {"VINCIRE", TOKEN_VINCIRE},
    {"VOLATILE", TOKEN_VOLATILE},

    /* X */
    {"XOR", TOKEN_XOR},
};

#define CCT_KEYWORD_COUNT (sizeof(cct_keywords) / sizeof(cct_keywords[0]))

#endif /* CCT_KEYWORDS_H */
