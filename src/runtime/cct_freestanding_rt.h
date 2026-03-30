/*
 * CCT — Clavicula Turing
 * Freestanding Runtime Shim Definitions
 *
 * FASE 16B.1: Freestanding runtime shim
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
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
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("cli\n\thlt" : : : "memory");
#else
    __builtin_trap();
#endif
    __builtin_unreachable();
}

static inline __attribute__((noreturn)) void cct_svc_halt(void) {
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("cli\n\thlt" : : : "memory");
#else
    __builtin_trap();
#endif
    __builtin_unreachable();
}

static inline void cct_svc_outb(long long porta, long long dado) {
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
    uint8_t d = (uint8_t)(dado & 0xFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("out{b %0, %1 | %1, %0}" : : "a"(d), "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
    (void)d;
#endif
}

static inline long long cct_svc_inb(long long porta) {
    uint8_t r = 0;
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("in{b %1, %0 | %0, %1}" : "=a"(r) : "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
#endif
    return (long long)r;
}

static inline void cct_svc_outw(long long porta, long long dado) {
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
    uint16_t d = (uint16_t)(dado & 0xFFFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("out{w %0, %1 | %1, %0}" : : "a"(d), "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
    (void)d;
#endif
}

static inline long long cct_svc_inw(long long porta) {
    uint16_t r = 0;
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("in{w %1, %0 | %0, %1}" : "=a"(r) : "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
#endif
    return (long long)(uint16_t)r;
}

static inline void cct_svc_outl(long long porta, long long dado) {
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
    uint32_t d = (uint32_t)(dado & 0xFFFFFFFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("out{l %0, %1 | %1, %0}" : : "a"(d), "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
    (void)d;
#endif
}

static inline long long cct_svc_inl(long long porta) {
    uint32_t r = 0;
    uint16_t p = (uint16_t)(porta & 0xFFFFLL);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("in{l %1, %0 | %0, %1}" : "=a"(r) : "Nd"(p) : "memory");
    __asm__ volatile("clc" : : : "cc");
#else
    (void)p;
#endif
    return (long long)(int32_t)r;
}

static inline void cct_svc_memcpy(void *dst, const void *src, long long n) {
    if (!dst || !src || n < 0) cct_svc_halt();
    cct_fs_memcpy(dst, src, (size_t)n);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("clc" : : : "cc");
#endif
}

static inline void cct_svc_memset(void *dst, long long val, long long n) {
    if (!dst || n < 0) cct_svc_halt();
    cct_fs_memset(dst, (int)(val & 0xFFLL), (size_t)n);
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("clc" : : : "cc");
#endif
}

#define CCT_VGA_COLS ((int32_t)80)
#define CCT_VGA_ROWS ((int32_t)25)
#define CCT_VGA_BASE ((volatile uint16_t*)0xB8000)

#define CCT_VGA_CRTC_ADDR ((uint16_t)0x3D4)
#define CCT_VGA_CRTC_DATA ((uint16_t)0x3D5)
#define CCT_VGA_CURSOR_HI ((uint8_t)0x0E)
#define CCT_VGA_CURSOR_LO ((uint8_t)0x0F)

extern volatile int32_t cct_vga_row;
extern volatile int32_t cct_vga_col;
extern volatile uint8_t cct_vga_attr;

typedef struct {
    uint8_t  *base;
    uint8_t  *top;
    uint8_t  *limit;
    uint32_t  total;
    uint32_t  allocated;
    uint32_t  alloc_count;
} cct_bump_heap_t;

extern cct_bump_heap_t cct_fs_heap;

#define CCT_HEAP_ALIGN ((size_t)8)
#define CCT_ALIGN_UP(n) (((n) + CCT_HEAP_ALIGN - 1u) & ~(CCT_HEAP_ALIGN - 1u))

typedef struct {
    char     *buf;
    uint32_t  len;
    uint32_t  cap;
} cct_verbum_builder_t;

#define CCT_BUILDER_INITIAL_CAP ((uint32_t)64)

typedef struct {
    uint8_t  *data;
    uint32_t  len;
    uint32_t  cap;
    uint32_t  esize;
} cct_fluxus_fs_t;

#define CCT_FLUXUS_INITIAL_CAP ((uint32_t)8)

static inline uint16_t cct_vga_cell(uint8_t c, uint8_t attr) {
    return (uint16_t)(((uint16_t)attr << 8) | (uint16_t)c);
}

static inline size_t cct_fs_cstr_len(const char *text) {
    size_t len = 0;
    if (!text) return 0;
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

static inline void cct_vga_update_cursor(int32_t row, int32_t col) {
    uint16_t pos = (uint16_t)(row * CCT_VGA_COLS + col);
    cct_svc_outb(CCT_VGA_CRTC_ADDR, CCT_VGA_CURSOR_HI);
    cct_svc_outb(CCT_VGA_CRTC_DATA, (long long)((pos >> 8) & 0xFFu));
    cct_svc_outb(CCT_VGA_CRTC_ADDR, CCT_VGA_CURSOR_LO);
    cct_svc_outb(CCT_VGA_CRTC_DATA, (long long)(pos & 0xFFu));
}

static inline void cct_vga_scroll(void) {
    volatile uint16_t *vga = CCT_VGA_BASE;
    for (int32_t row = 0; row < CCT_VGA_ROWS - 1; row++) {
        for (int32_t col = 0; col < CCT_VGA_COLS; col++) {
            vga[(row * CCT_VGA_COLS) + col] = vga[((row + 1) * CCT_VGA_COLS) + col];
        }
    }
    for (int32_t col = 0; col < CCT_VGA_COLS; col++) {
        vga[((CCT_VGA_ROWS - 1) * CCT_VGA_COLS) + col] = cct_vga_cell((uint8_t)' ', cct_vga_attr);
    }
}

static inline void cct_svc_console_clear(void) {
    volatile uint16_t *vga = CCT_VGA_BASE;
    for (int32_t row = 0; row < CCT_VGA_ROWS; row++) {
        for (int32_t col = 0; col < CCT_VGA_COLS; col++) {
            vga[(row * CCT_VGA_COLS) + col] = cct_vga_cell((uint8_t)' ', cct_vga_attr);
        }
    }
    cct_vga_row = 0;
    cct_vga_col = 0;
    cct_vga_update_cursor(cct_vga_row, cct_vga_col);
}

static inline void cct_svc_console_putc(long long c) {
    volatile uint16_t *vga = CCT_VGA_BASE;
    uint8_t ch = (uint8_t)(c & 0xFFLL);

    if (ch == (uint8_t)'\n') {
        cct_vga_col = 0;
        cct_vga_row++;
    } else if (ch == (uint8_t)'\r') {
        cct_vga_col = 0;
    } else if (ch == (uint8_t)'\b') {
        if (cct_vga_col > 0) {
            cct_vga_col--;
        } else if (cct_vga_row > 0) {
            cct_vga_row--;
            cct_vga_col = CCT_VGA_COLS - 1;
        }
        vga[(cct_vga_row * CCT_VGA_COLS) + cct_vga_col] = cct_vga_cell((uint8_t)' ', cct_vga_attr);
    } else if (ch == (uint8_t)'\t') {
        cct_vga_col = (int32_t)((cct_vga_col + 8) & ~7);
        if (cct_vga_col >= CCT_VGA_COLS) {
            cct_vga_col = 0;
            cct_vga_row++;
        }
    } else {
        vga[(cct_vga_row * CCT_VGA_COLS) + cct_vga_col] = cct_vga_cell(ch, cct_vga_attr);
        cct_vga_col++;
        if (cct_vga_col >= CCT_VGA_COLS) {
            cct_vga_col = 0;
            cct_vga_row++;
        }
    }

    if (cct_vga_row >= CCT_VGA_ROWS) {
        cct_vga_scroll();
        cct_vga_row = CCT_VGA_ROWS - 1;
    }

    cct_vga_update_cursor(cct_vga_row, cct_vga_col);
}

static inline void cct_svc_console_write(const char *text) {
    if (!text) cct_svc_halt();
    while (*text) {
        cct_svc_console_putc((long long)(unsigned char)(*text));
        text++;
    }
}

static inline void cct_svc_console_write_centered(const char *text) {
    size_t len = cct_fs_cstr_len(text);
    int32_t padding = 0;
    if (!text) cct_svc_halt();
    if (len < (size_t)CCT_VGA_COLS) {
        padding = (int32_t)((CCT_VGA_COLS - (int32_t)len) / 2);
    }
    cct_vga_col = 0;
    cct_vga_update_cursor(cct_vga_row, cct_vga_col);
    for (int32_t i = 0; i < padding; i++) {
        cct_svc_console_putc((long long)' ');
    }
    cct_svc_console_write(text);
    cct_svc_console_putc((long long)'\n');
}

static inline void cct_svc_console_set_attr(long long attr) {
    cct_vga_attr = (uint8_t)(attr & 0xFFLL);
}

static inline void cct_svc_console_init(void) {
    cct_vga_row = 0;
    cct_vga_col = 0;
    cct_vga_attr = 0x07u;
    cct_svc_console_clear();
}

static inline void cct_svc_console_set_cursor(long long linha, long long col) {
    if (linha < 0 || linha >= CCT_VGA_ROWS) return;
    if (col < 0 || col >= CCT_VGA_COLS) return;
    cct_vga_row = (int32_t)linha;
    cct_vga_col = (int32_t)col;
    cct_vga_update_cursor(cct_vga_row, cct_vga_col);
}

static inline long long cct_svc_console_get_row(void) {
    return (long long)cct_vga_row;
}

static inline long long cct_svc_console_get_col(void) {
    return (long long)cct_vga_col;
}

static inline void cct_svc_heap_init(void *base, long long tamanho) {
    if (!base || tamanho <= 0) cct_fs_panic();
    cct_fs_heap.base = (uint8_t*)base;
    cct_fs_heap.top = (uint8_t*)base;
    cct_fs_heap.limit = (uint8_t*)base + (size_t)tamanho;
    cct_fs_heap.total = (uint32_t)tamanho;
    cct_fs_heap.allocated = 0u;
    cct_fs_heap.alloc_count = 0u;
    cct_fs_memset(base, 0, (size_t)tamanho);
}

static inline void cct_svc_heap_require_ready(void) {
    if (!cct_fs_heap.base || !cct_fs_heap.top || !cct_fs_heap.limit || cct_fs_heap.top > cct_fs_heap.limit) {
        cct_fs_panic();
    }
}

static inline void *cct_svc_alloc(long long n) {
    cct_svc_heap_require_ready();
    if (n < 0) cct_fs_panic();

    size_t aligned = CCT_ALIGN_UP((size_t)n);
    if (cct_fs_heap.top + aligned > cct_fs_heap.limit) {
        cct_fs_panic();
    }

    void *ptr = (void*)cct_fs_heap.top;
    cct_fs_heap.top += aligned;
    cct_fs_heap.allocated += (uint32_t)aligned;
    cct_fs_heap.alloc_count++;

    if (aligned > 0) {
        cct_fs_memset(ptr, 0, aligned);
    }
    return ptr;
}

static inline void *cct_svc_realloc(void *ptr, long long old_size, long long new_size) {
    cct_svc_heap_require_ready();
    if (old_size < 0 || new_size < 0) cct_fs_panic();
    if (new_size <= old_size) return ptr;

    void *new_ptr = cct_svc_alloc(new_size);
    if (ptr && old_size > 0) {
        cct_fs_memcpy(new_ptr, ptr, (size_t)old_size);
    }
    return new_ptr;
}

static inline void cct_svc_free(void *ptr) {
    (void)ptr;
}

static inline long long cct_svc_heap_available(void) {
    cct_svc_heap_require_ready();
    return (long long)(cct_fs_heap.limit - cct_fs_heap.top);
}

static inline long long cct_svc_heap_allocated(void) {
    cct_svc_heap_require_ready();
    return (long long)cct_fs_heap.allocated;
}

static inline long long cct_svc_heap_total(void) {
    cct_svc_heap_require_ready();
    return (long long)cct_fs_heap.total;
}

static inline long long cct_svc_heap_base_addr(void) {
    cct_svc_heap_require_ready();
    return (long long)(uintptr_t)cct_fs_heap.base;
}

static inline long long cct_svc_heap_alloc_count(void) {
    cct_svc_heap_require_ready();
    return (long long)cct_fs_heap.alloc_count;
}

static inline long long cct_svc_byte_at(const void *ptr, long long idx) {
    if (!ptr || idx < 0) return 0;
    return (long long)((const uint8_t*)ptr)[idx];
}

static inline long long cct_svc_verbum_byte(const char *s, long long i) {
    if (!s || i < 0) return 0;
    return (long long)(unsigned char)s[i];
}

static inline long long cct_svc_verbum_len(const char *s) {
    return (long long)cct_fs_cstr_len(s);
}

static inline void cct_svc_verbum_copy(char *dst, const char *src, long long n) {
    if (!dst || !src || n < 0) cct_fs_panic();
    for (long long i = 0; i < n; i++) {
        dst[i] = src[i];
    }
    dst[n] = '\0';
}

static inline void cct_svc_verbum_copy_offset(char *dst, const char *src, long long offset, long long n) {
    if (!dst || !src || offset < 0 || n < 0) cct_fs_panic();
    for (long long i = 0; i < n; i++) {
        dst[i] = src[offset + i];
    }
    dst[n] = '\0';
}

static inline const char *cct_svc_verbum_copy_slice(const char *src, long long offset, long long n) {
    if (!src || offset < 0 || n < 0) cct_fs_panic();
    char *dst = (char*)cct_svc_alloc(n + 1);
    cct_svc_verbum_copy_offset(dst, src, offset, n);
    return (const char*)dst;
}

static inline void cct_svc_buf_set_byte(void *dst, long long idx, long long value) {
    if (!dst || idx < 0) cct_fs_panic();
    ((uint8_t*)dst)[idx] = (uint8_t)(value & 0xFFLL);
}

static inline void *cct_svc_buf_slice(const void *src, long long offset, long long n) {
    if (!src || offset < 0 || n < 0) cct_fs_panic();
    uint8_t *dst = (uint8_t*)cct_svc_alloc(n);
    if (n > 0) {
        cct_fs_memcpy(dst, ((const uint8_t*)src) + offset, (size_t)n);
    }
    return dst;
}

static inline cct_verbum_builder_t *cct_svc_builder_new(void) {
    cct_verbum_builder_t *b = (cct_verbum_builder_t*)cct_svc_alloc((long long)sizeof(cct_verbum_builder_t));
    b->buf = (char*)cct_svc_alloc((long long)CCT_BUILDER_INITIAL_CAP);
    b->len = 0u;
    b->cap = CCT_BUILDER_INITIAL_CAP;
    b->buf[0] = '\0';
    return b;
}

static inline void cct_svc_builder_ensure(cct_verbum_builder_t *b, long long need) {
    if (!b || !b->buf || need < 0) cct_fs_panic();
    while ((uint64_t)b->len + (uint64_t)need + 1ull > (uint64_t)b->cap) {
        uint32_t new_cap = (b->cap == 0u) ? CCT_BUILDER_INITIAL_CAP : (b->cap * 2u);
        b->buf = (char*)cct_svc_realloc(b->buf, (long long)b->cap, (long long)new_cap);
        b->cap = new_cap;
    }
}

static inline void cct_svc_builder_append(cct_verbum_builder_t *b, const char *s) {
    if (!b || !s) cct_fs_panic();
    long long slen = cct_svc_verbum_len(s);
    cct_svc_builder_ensure(b, slen);
    for (long long i = 0; i < slen; i++) {
        b->buf[b->len + (uint32_t)i] = s[i];
    }
    b->len += (uint32_t)slen;
    b->buf[b->len] = '\0';
}

static inline void cct_svc_builder_append_char(cct_verbum_builder_t *b, long long c) {
    if (!b) cct_fs_panic();
    cct_svc_builder_ensure(b, 1);
    b->buf[b->len++] = (char)(c & 0xFFLL);
    b->buf[b->len] = '\0';
}

static inline const char *cct_svc_builder_build(cct_verbum_builder_t *b) {
    if (!b || !b->buf) cct_fs_panic();
    return cct_svc_verbum_copy_slice(b->buf, 0, (long long)b->len);
}

static inline void cct_svc_builder_clear(cct_verbum_builder_t *b) {
    if (!b || !b->buf) cct_fs_panic();
    b->len = 0u;
    b->buf[0] = '\0';
}

static inline long long cct_svc_builder_len(cct_verbum_builder_t *b) {
    if (!b) cct_fs_panic();
    return (long long)b->len;
}

static inline cct_verbum_builder_t *cct_svc_builder_new_raw(void) {
    return cct_svc_builder_new();
}

static inline void cct_svc_builder_backspace(cct_verbum_builder_t *b) {
    if (!b || !b->buf) cct_fs_panic();
    if (b->len == 0u) return;
    b->len--;
    b->buf[b->len] = '\0';
}

static inline cct_fluxus_fs_t *cct_svc_fluxus_new(long long elem_size) {
    if (elem_size <= 0) cct_fs_panic();
    cct_fluxus_fs_t *f = (cct_fluxus_fs_t*)cct_svc_alloc((long long)sizeof(cct_fluxus_fs_t));
    f->esize = (uint32_t)elem_size;
    f->len = 0u;
    f->cap = CCT_FLUXUS_INITIAL_CAP;
    f->data = (uint8_t*)cct_svc_alloc((long long)(f->esize * f->cap));
    return f;
}

static inline void cct_svc_fluxus_grow(cct_fluxus_fs_t *f) {
    if (!f) cct_fs_panic();
    if (f->len < f->cap) return;
    uint32_t old_cap = f->cap;
    uint32_t new_cap = (old_cap == 0u) ? CCT_FLUXUS_INITIAL_CAP : (old_cap * 2u);
    f->data = (uint8_t*)cct_svc_realloc(
        f->data,
        (long long)(f->esize * old_cap),
        (long long)(f->esize * new_cap)
    );
    f->cap = new_cap;
}

static inline void cct_svc_fluxus_push(cct_fluxus_fs_t *f, void *elem) {
    if (!f || !elem) cct_fs_panic();
    cct_svc_fluxus_grow(f);
    uint8_t *dst = f->data + ((size_t)f->len * f->esize);
    cct_fs_memcpy(dst, elem, (size_t)f->esize);
    f->len++;
}

static inline void cct_svc_fluxus_pop(cct_fluxus_fs_t *f, void *out) {
    if (!f || f->len == 0u) cct_fs_panic();
    f->len--;
    if (out) {
        uint8_t *src = f->data + ((size_t)f->len * f->esize);
        cct_fs_memcpy(out, src, (size_t)f->esize);
    }
}

static inline void *cct_svc_fluxus_get(cct_fluxus_fs_t *f, long long idx) {
    if (!f || idx < 0 || (uint32_t)idx >= f->len) cct_fs_panic();
    return (void*)(f->data + ((size_t)idx * f->esize));
}

static inline void cct_svc_fluxus_set(cct_fluxus_fs_t *f, long long idx, void *elem) {
    if (!f || !elem || idx < 0 || (uint32_t)idx >= f->len) cct_fs_panic();
    uint8_t *dst = f->data + ((size_t)idx * f->esize);
    cct_fs_memcpy(dst, elem, (size_t)f->esize);
}

static inline void cct_svc_fluxus_clear(cct_fluxus_fs_t *f) {
    if (!f) cct_fs_panic();
    if (f->len > 0u) {
        cct_fs_memset(f->data, 0, (size_t)(f->len * f->esize));
    }
    f->len = 0u;
}

static inline void cct_svc_fluxus_reserve(cct_fluxus_fs_t *f, long long cap) {
    if (!f || cap < 0) cct_fs_panic();
    if ((uint32_t)cap <= f->cap) return;
    f->data = (uint8_t*)cct_svc_realloc(
        f->data,
        (long long)(f->esize * f->cap),
        (long long)(f->esize * (uint32_t)cap)
    );
    f->cap = (uint32_t)cap;
}

static inline void cct_svc_fluxus_free(cct_fluxus_fs_t *f) {
    (void)f;
}

static inline long long cct_svc_fluxus_len(cct_fluxus_fs_t *f) {
    if (!f) cct_fs_panic();
    return (long long)f->len;
}

static inline long long cct_svc_fluxus_cap(cct_fluxus_fs_t *f) {
    if (!f) cct_fs_panic();
    return (long long)f->cap;
}

static inline void *cct_svc_fluxus_peek(cct_fluxus_fs_t *f) {
    if (!f || f->len == 0u) return NULL;
    return (void*)(f->data + ((size_t)(f->len - 1u) * f->esize));
}

static inline void *cct_svc_fluxus_remove_first(cct_fluxus_fs_t *f) {
    void *out = NULL;
    if (!f || f->len == 0u) return NULL;
    out = cct_svc_alloc((long long)f->esize);
    cct_fs_memcpy(out, f->data, (size_t)f->esize);
    if (f->len > 1u) {
        cct_fs_memcpy(f->data, f->data + f->esize, (size_t)((f->len - 1u) * f->esize));
    }
    f->len--;
    return out;
}

extern void pic_mask_irq(uint8_t irq);
extern void pic_unmask_irq(uint8_t irq);
extern void idt_register_irq_handler(uint8_t irq, void (*handler)(void));
extern void idt_unregister_irq_handler(uint8_t irq);
extern long long keyboard_getc_block(void);
extern long long keyboard_poll_char(void);
extern long long keyboard_available_count(void);
extern void keyboard_flush_buffer(void);
extern long long keyboard_self_test(void);
extern long long timer_uptime_ms(void);
extern long long timer_ticks_count(void);
extern void timer_sleep_ms(long long ms);
extern void cct_svc_pci_init(void);
extern long long cct_svc_pci_count(void);
extern long long cct_svc_pci_vendor(long long idx);
extern long long cct_svc_pci_device_id(long long idx);
extern long long cct_svc_pci_class(long long idx);
extern long long cct_svc_pci_bar0(long long idx);
extern long long cct_svc_pci_irq(long long idx);
extern long long cct_svc_pci_find(long long vendor_id, long long device_id);
extern void cct_svc_pci_enable_busmaster(long long idx);
extern long long cct_svc_net_init(long long iobase);
extern void cct_svc_net_send(const void *data, long long len);
extern long long cct_svc_net_recv(void *buf, long long max_len);
extern void cct_svc_net_mac(void *mac_buf);
extern void cct_svc_net_poll(void);
extern void cct_svc_net_dispatch_init(void);
extern void cct_svc_tcp_init(long long port);
extern long long cct_svc_tcp_accept(void);
extern long long cct_svc_tcp_recv(void *buf, long long max_len);
extern void cct_svc_tcp_send(const void *data, long long len);
extern void cct_svc_tcp_close(void);
extern long long cct_svc_tcp_state(void);
extern void cct_svc_http_server_init(long long port);
extern long long cct_svc_http_server_accept(long long timeout_ms);
extern long long cct_svc_http_server_read(long long hdr_timeout_ms, long long body_timeout_ms);
extern void cct_svc_http_server_send(const void *data, long long len);
extern void cct_svc_http_server_close(void);
extern long long cct_svc_http_server_req_len(void);
extern void cct_svc_http_server_req_copy(void *dst, long long max_len);
extern long long cct_svc_http_server_req_count(void);
extern long long cct_svc_http_parse(const void *buf, long long len);
extern void cct_svc_http_req_method(void *dst, long long max_len);
extern void cct_svc_http_req_path(void *dst, long long max_len);
extern void cct_svc_http_req_query(void *dst, long long max_len);
extern void cct_svc_http_req_version(void *dst, long long max_len);
extern const char *cct_svc_http_req_method_ptr(void);
extern const char *cct_svc_http_req_path_ptr(void);
extern const char *cct_svc_http_req_query_ptr(void);
extern const char *cct_svc_http_req_version_ptr(void);
extern long long cct_svc_http_req_method_is(const char *method);
extern long long cct_svc_http_req_path_is(const char *path);
extern long long cct_svc_http_req_path_starts(const char *prefix);
extern long long cct_svc_http_req_header_count(void);
extern void cct_svc_http_req_header_name(long long idx, void *dst, long long max_len);
extern void cct_svc_http_req_header_value(long long idx, void *dst, long long max_len);
extern const char *cct_svc_http_req_header_name_ptr(long long idx);
extern const char *cct_svc_http_req_header_value_ptr(long long idx);
extern long long cct_svc_http_req_body_len(void);
extern void cct_svc_http_req_body_copy(void *dst, long long max_len);
extern long long cct_svc_http_req_find_header(const char *name);
extern void cct_svc_http_res_begin(long long status_code);
extern void cct_svc_http_res_header(const char *name, const char *value);
extern void cct_svc_http_res_finish(const char *content_type, const void *body, long long body_len);
extern void cct_svc_http_res_build(long long status_code, const char *content_type, const void *body, long long body_len);
extern void cct_svc_http_res_send(void);
extern long long cct_svc_http_res_len(void);
extern void cct_svc_http_router_init(void);
extern void cct_svc_http_router_add(const char *method, const char *path, long long match_type, long long handler_id);
extern long long cct_svc_http_router_dispatch(const char *method, const char *path);
extern void cct_svc_http_router_set_404(long long handler_id);

static inline void cct_svc_irq_enable(void) {
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("sti" : : : "memory");
#endif
}

static inline void cct_svc_irq_disable(void) {
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("cli" : : : "memory");
#endif
}

static inline long long cct_svc_irq_flags(void) {
    uint32_t flags = 0u;
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("pushf\n\tpop %0" : "=r"(flags) : : "memory");
#endif
    return (long long)((flags >> 9) & 1u);
}

static inline void cct_svc_irq_mask(long long irq) {
    if (irq < 0 || irq > 15) cct_fs_panic();
    pic_mask_irq((uint8_t)irq);
}

static inline void cct_svc_irq_unmask(long long irq) {
    if (irq < 0 || irq > 15) cct_fs_panic();
    pic_unmask_irq((uint8_t)irq);
}

static inline void cct_svc_irq_register(long long irq, void *handler_fn) {
    if (irq < 0 || irq > 15 || !handler_fn) cct_fs_panic();
    idt_register_irq_handler((uint8_t)irq, (void (*)(void))handler_fn);
}

static inline void cct_svc_irq_unregister(long long irq) {
    if (irq < 0 || irq > 15) cct_fs_panic();
    idt_unregister_irq_handler((uint8_t)irq);
}

static inline __attribute__((noreturn)) void cct_svc_reboot(void) {
#if defined(__i386__) || defined(__x86_64__)
    uint8_t status = 0x02u;
    while ((status & 0x02u) != 0u) {
        __asm__ volatile("in{b %1, %0 | %0, %1}" : "=a"(status) : "Nd"((uint16_t)0x64) : "memory");
    }
    __asm__ volatile("out{b %0, %1 | %1, %0}" : : "a"((uint8_t)0xFEu), "Nd"((uint16_t)0x64) : "memory");
#endif
    cct_fs_halt();
}

static inline long long cct_svc_keyboard_getc(void) {
    return keyboard_getc_block();
}

static inline long long cct_svc_keyboard_poll(void) {
    return keyboard_poll_char();
}

static inline long long cct_svc_keyboard_available(void) {
    return keyboard_available_count();
}

static inline void cct_svc_keyboard_flush(void) {
    keyboard_flush_buffer();
}

static inline long long cct_svc_keyboard_self_test(void) {
    return keyboard_self_test();
}

static inline long long cct_svc_timer_ms(void) {
    return timer_uptime_ms();
}

static inline long long cct_svc_timer_ticks(void) {
    return timer_ticks_count();
}

static inline void cct_svc_timer_sleep(long long ms) {
    if (ms < 0) cct_fs_panic();
    timer_sleep_ms(ms);
}

#endif /* CCT_FREESTANDING_RT_H */
