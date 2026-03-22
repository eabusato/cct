/*
 * CCT — Clavicula Turing
 * Freestanding Runtime Shim Implementation
 *
 * FASE 16B.1: Freestanding runtime shim
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "cct_freestanding_rt.h"

void cct_fs_memcpy(void *dst, const void *src, size_t n) {
    if (!dst || !src) cct_fs_halt();
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
}

void cct_fs_memset(void *dst, int val, size_t n) {
    if (!dst) cct_fs_halt();
    uint8_t *d = (uint8_t*)dst;
    uint8_t v = (uint8_t)val;
    while (n--) {
        *d++ = v;
    }
}

__attribute__((noreturn)) void cct_fs_panic(void) {
    cct_fs_halt();
}

cct_rex_t cct_fs_pow_i32(cct_rex_t base, cct_rex_t exp) {
    if (exp < 0) return 0;
    cct_rex_t result = 1;
    while (exp > 0) {
        if ((exp & 1) != 0) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}
