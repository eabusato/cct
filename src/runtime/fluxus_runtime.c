/*
 * CCT — Clavicula Turing
 * FLUXUS Runtime Implementation
 *
 * FASE 10A: Dynamic vector runtime implementation
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "fluxus_runtime.h"

#include "mem_runtime.h"

#include <stdio.h>
#include <stdlib.h>

static void cct_rt_fluxus_abort(const char *msg) {
    fprintf(stderr, "cct runtime: %s\n", msg ? msg : "fluxus failure");
    exit(1);
}

cct_fluxus_t* cct_rt_fluxus_create(size_t elem_size) {
    cct_fluxus_t *flux = (cct_fluxus_t*)cct_rt_mem_alloc_n(sizeof(cct_fluxus_t));
    cct_rt_fluxus_init(flux, elem_size);
    return flux;
}

void cct_rt_fluxus_destroy(cct_fluxus_t *flux) {
    if (!flux) return;
    cct_rt_fluxus_free(flux);
    cct_rt_mem_free_n(flux);
}

void cct_rt_fluxus_init(cct_fluxus_t *flux, size_t elem_size) {
    if (!flux) cct_rt_fluxus_abort("fluxus init received null instance");
    if (elem_size == 0) cct_rt_fluxus_abort("fluxus init element size must be > 0");
    flux->data = NULL;
    flux->len = 0;
    flux->capacity = 0;
    flux->elem_size = elem_size;
}

void cct_rt_fluxus_free(cct_fluxus_t *flux) {
    if (!flux) return;
    if (flux->data) {
        cct_rt_mem_free_n(flux->data);
    }
    flux->data = NULL;
    flux->len = 0;
    flux->capacity = 0;
}

void cct_rt_fluxus_reserve(cct_fluxus_t *flux, size_t new_capacity) {
    if (!flux) cct_rt_fluxus_abort("fluxus reserve received null instance");
    if (new_capacity <= flux->capacity) return;

    size_t bytes = new_capacity * flux->elem_size;
    if (flux->data == NULL) {
        flux->data = cct_rt_mem_alloc_n(bytes);
    } else {
        flux->data = cct_rt_mem_realloc_n(flux->data, bytes);
    }
    flux->capacity = new_capacity;
}

void cct_rt_fluxus_grow(cct_fluxus_t *flux) {
    if (!flux) cct_rt_fluxus_abort("fluxus grow received null instance");
    size_t next = (flux->capacity == 0) ? 4 : (flux->capacity * 2);
    cct_rt_fluxus_reserve(flux, next);
}

size_t cct_rt_fluxus_len(const cct_fluxus_t *flux) {
    return flux ? flux->len : 0;
}

size_t cct_rt_fluxus_capacity(const cct_fluxus_t *flux) {
    return flux ? flux->capacity : 0;
}

void cct_rt_fluxus_push(cct_fluxus_t *flux, const void *elem) {
    if (!flux || !elem) cct_rt_fluxus_abort("fluxus push requires valid instance and element");
    if (flux->len == flux->capacity) {
        cct_rt_fluxus_grow(flux);
    }
    unsigned char *base = (unsigned char*)flux->data;
    cct_rt_mem_copy_n(base + (flux->len * flux->elem_size), elem, flux->elem_size);
    flux->len += 1;
}

void cct_rt_fluxus_pop(cct_fluxus_t *flux, void *out) {
    if (!flux) cct_rt_fluxus_abort("fluxus pop received null instance");
    if (flux->len == 0) cct_rt_fluxus_abort("fluxus pop on empty vector");
    flux->len -= 1;
    if (out) {
        unsigned char *base = (unsigned char*)flux->data;
        cct_rt_mem_copy_n(out, base + (flux->len * flux->elem_size), flux->elem_size);
    }
}

void* cct_rt_fluxus_get(cct_fluxus_t *flux, size_t index) {
    if (!flux) cct_rt_fluxus_abort("fluxus get received null instance");
    if (index >= flux->len) cct_rt_fluxus_abort("fluxus get index out of bounds");
    unsigned char *base = (unsigned char*)flux->data;
    return (void*)(base + (index * flux->elem_size));
}

void cct_rt_fluxus_clear(cct_fluxus_t *flux) {
    if (!flux) return;
    flux->len = 0;
}
