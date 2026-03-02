/*
 * CCT — Clavicula Turing
 * Project Management Header
 *
 * FASE 12C: Project-level build, test, and workflow orchestration
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PROJECT_H
#define CCT_PROJECT_H

#include <stdbool.h>

bool cct_project_is_command(const char *arg);
int cct_project_dispatch(int argc, char **argv, const char *self_path);

#endif
