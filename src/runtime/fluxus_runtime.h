/*
 * CCT — Clavicula Turing
 * Fluxus Runtime Definitions
 *
 * FASE 10A: Host runtime support modules
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_FLUXUS_RUNTIME_H
#define CCT_FLUXUS_RUNTIME_H

#include <stddef.h>

typedef struct {
    void *data;
    size_t len;
    size_t capacity;
    size_t elem_size;
} cct_fluxus_t;

cct_fluxus_t* cct_rt_fluxus_create(size_t elem_size);
void cct_rt_fluxus_destroy(cct_fluxus_t *flux);
void cct_rt_fluxus_init(cct_fluxus_t *flux, size_t elem_size);
void cct_rt_fluxus_free(cct_fluxus_t *flux);
void cct_rt_fluxus_reserve(cct_fluxus_t *flux, size_t new_capacity);
void cct_rt_fluxus_grow(cct_fluxus_t *flux);
size_t cct_rt_fluxus_len(const cct_fluxus_t *flux);
size_t cct_rt_fluxus_capacity(const cct_fluxus_t *flux);
void cct_rt_fluxus_push(cct_fluxus_t *flux, const void *elem);
void cct_rt_fluxus_pop(cct_fluxus_t *flux, void *out);
void* cct_rt_fluxus_get(cct_fluxus_t *flux, size_t index);
void cct_rt_fluxus_clear(cct_fluxus_t *flux);

#endif
