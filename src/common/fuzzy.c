/*
 * CCT — Clavicula Turing
 * Fuzzy Matching Helpers Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "fuzzy.h"

#include <stdlib.h>
#include <string.h>

static int cct_min3(int a, int b, int c) {
    int m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

int cct_levenshtein_distance(const char *left, const char *right) {
    if (!left || !right) return 9999;

    size_t n = strlen(left);
    size_t m = strlen(right);

    int *prev = (int*)malloc((m + 1) * sizeof(int));
    int *curr = (int*)malloc((m + 1) * sizeof(int));
    if (!prev || !curr) {
        free(prev);
        free(curr);
        return 9999;
    }

    for (size_t j = 0; j <= m; j++) {
        prev[j] = (int)j;
    }

    for (size_t i = 1; i <= n; i++) {
        curr[0] = (int)i;
        for (size_t j = 1; j <= m; j++) {
            int cost = (left[i - 1] == right[j - 1]) ? 0 : 1;
            curr[j] = cct_min3(
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            );
        }

        int *tmp = prev;
        prev = curr;
        curr = tmp;
    }

    int dist = prev[m];
    free(prev);
    free(curr);
    return dist;
}

const char* cct_fuzzy_match(const char *target, const char **candidates, size_t count) {
    if (!target || !candidates || count == 0) return NULL;

    const char *best = NULL;
    int best_dist = 9999;
    for (size_t i = 0; i < count; i++) {
        const char *cand = candidates[i];
        if (!cand) continue;

        int d = cct_levenshtein_distance(target, cand);
        if (d < best_dist) {
            best_dist = d;
            best = cand;
        }
    }

    if (!best) return NULL;

    size_t tlen = strlen(target);
    int threshold = (tlen <= 4) ? 1 : 2;
    if (best_dist > threshold) return NULL;

    return best;
}
