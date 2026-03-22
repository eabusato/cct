/*
 * CCT — Clavicula Turing
 * Project Workflow Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PROJECT_H
#define CCT_PROJECT_H

#include <stdbool.h>

bool cct_project_is_command(const char *arg);
int cct_project_dispatch(int argc, char **argv, const char *self_path);

#endif
