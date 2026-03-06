/*
 * cct_freestanding_rt.h — CCT Freestanding Runtime Shim (sem libc)
 * Gerado/mantido pelo pipeline CCT.
 */

#ifndef CCT_FREESTANDING_RT_H
#define CCT_FREESTANDING_RT_H

#include <stdint.h>
#include <stddef.h>

typedef int32_t  cct_rex_t;
typedef uint32_t cct_dux_t;
typedef float    cct_comes_t;
typedef double   cct_miles_t;
typedef int32_t  cct_verum_t;
typedef uint32_t cct_verbum_ptr_t;

#define CCT_VERUM  ((cct_verum_t)1)
#define CCT_FALSUM ((cct_verum_t)0)

void cct_fs_memcpy(void *dst, const void *src, size_t n);
void cct_fs_memset(void *dst, int val, size_t n);
__attribute__((noreturn)) void cct_fs_panic(void);
cct_rex_t cct_fs_pow_i32(cct_rex_t base, cct_rex_t exp);

static inline __attribute__((noreturn)) void cct_fs_halt(void) {
    __asm__ volatile("cli\n\thlt" : : : "memory");
    __builtin_unreachable();
}

static inline __attribute__((noreturn)) void cct_svc_halt(void) {
    __asm__ volatile("cli\n\thlt" : : : "memory");
    __builtin_unreachable();
}

static inline void cct_svc_outb(long long porta, long long dado) {
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
    uint8_t d = (uint8_t)(dado & 0xFFLL);
    __asm__ volatile("out{b %0, %1 | %1, %0}" : : "a"(d), "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
}

static inline long long cct_svc_inb(long long porta) {
    uint8_t r = 0;
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
    __asm__ volatile("in{b %1, %0 | %0, %1}" : "=a"(r) : "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
    return (long long)r;
}

static inline void cct_svc_memcpy(void *dst, const void *src, long long n) {
    if (!dst || !src || n < 0) cct_svc_halt();
    cct_fs_memcpy(dst, src, (size_t)n);
    __asm__ volatile("clc" : : : "cc");
}

static inline void cct_svc_memset(void *dst, long long val, long long n) {
    if (!dst || n < 0) cct_svc_halt();
    cct_fs_memset(dst, (int)(val & 0xFFLL), (size_t)n);
    __asm__ volatile("clc" : : : "cc");
}

#endif /* CCT_FREESTANDING_RT_H */
