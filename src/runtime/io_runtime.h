/*
 * CCT — Clavicula Turing
 * I/O Runtime Definitions
 *
 * FASE 10A: Host runtime support modules
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_IO_RUNTIME_H
#define CCT_IO_RUNTIME_H

char* cct_rt_io_read_line(void);
void cct_rt_io_print(const char *s);
void cct_rt_io_println(const char *s);
void cct_rt_io_print_int(long long n);

#endif
