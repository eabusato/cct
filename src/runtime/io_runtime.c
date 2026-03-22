/*
 * CCT — Clavicula Turing
 * I/O Runtime Implementation
 *
 * FASE 10A: Host runtime support modules
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "io_runtime.h"

#include <stdio.h>
#include <stdlib.h>

static void cct_rt_io_abort(const char *msg) {
    fprintf(stderr, "cct runtime: %s\n", msg ? msg : "io failure");
    exit(1);
}

void cct_rt_io_print(const char *s) {
    fputs(s ? s : "", stdout);
}

void cct_rt_io_println(const char *s) {
    fputs(s ? s : "", stdout);
    fputc('\n', stdout);
}

void cct_rt_io_print_int(long long n) {
    printf("%lld", n);
}

char* cct_rt_io_read_line(void) {
    size_t cap = 128;
    size_t len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) cct_rt_io_abort("io read_line allocation failed");

    int ch = 0;
    while ((ch = fgetc(stdin)) != EOF) {
        if (ch == '\r') continue;
        if (ch == '\n') break;
        if (len + 1 >= cap) {
            size_t next_cap = cap * 2;
            char *next = (char*)realloc(buf, next_cap);
            if (!next) {
                free(buf);
                cct_rt_io_abort("io read_line reallocation failed");
            }
            buf = next;
            cap = next_cap;
        }
        buf[len++] = (char)ch;
    }

    buf[len] = '\0';
    return buf;
}
