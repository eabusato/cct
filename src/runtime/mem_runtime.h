/*
 * CCT — Clavicula Turing
 * Memory Runtime Definitions
 *
 * FASE 10A: Host runtime support modules
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_MEM_RUNTIME_H
#define CCT_MEM_RUNTIME_H

#include <stddef.h>

void* cct_rt_mem_alloc_n(size_t size);
void* cct_rt_mem_realloc_n(void *ptr, size_t size);
void  cct_rt_mem_free_n(void *ptr);
void  cct_rt_mem_copy_n(void *dest, const void *src, size_t size);
void  cct_rt_mem_set_n(void *ptr, int value, size_t size);
void  cct_rt_mem_zero_n(void *ptr, size_t size);
int   cct_rt_mem_compare_n(const void *a, const void *b, size_t size);

#endif
