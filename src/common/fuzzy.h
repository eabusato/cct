/*
 * CCT — Clavicula Turing
 * Fuzzy Matching Helpers
 *
 * FASE 12A: typo suggestions for diagnostics.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_COMMON_FUZZY_H
#define CCT_COMMON_FUZZY_H

#include <stddef.h>

int cct_levenshtein_distance(const char *left, const char *right);
const char* cct_fuzzy_match(const char *target, const char **candidates, size_t count);

#endif /* CCT_COMMON_FUZZY_H */
