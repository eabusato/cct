#include "mem_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cct_rt_mem_abort(const char *msg) {
    fprintf(stderr, "cct runtime: %s\n", msg ? msg : "mem failure");
    exit(1);
}

void* cct_rt_mem_alloc_n(size_t size) {
    if (size == 0) cct_rt_mem_abort("mem alloc size must be > 0");
    void *p = malloc(size);
    if (!p) cct_rt_mem_abort("mem alloc failed");
    return p;
}

void* cct_rt_mem_realloc_n(void *ptr, size_t size) {
    if (size == 0) cct_rt_mem_abort("mem realloc size must be > 0");
    void *p = realloc(ptr, size);
    if (!p) cct_rt_mem_abort("mem realloc failed");
    return p;
}

void cct_rt_mem_free_n(void *ptr) {
    free(ptr);
}

void cct_rt_mem_copy_n(void *dest, const void *src, size_t size) {
    if (size > 0) memcpy(dest, src, size);
}

void cct_rt_mem_set_n(void *ptr, int value, size_t size) {
    if (size > 0) memset(ptr, value, size);
}

void cct_rt_mem_zero_n(void *ptr, size_t size) {
    cct_rt_mem_set_n(ptr, 0, size);
}

int cct_rt_mem_compare_n(const void *a, const void *b, size_t size) {
    if (size == 0) return 0;
    return memcmp(a, b, size);
}
