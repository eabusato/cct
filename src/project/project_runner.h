/*
 * CCT — Clavicula Turing
 * Project Runner Header
 *
 * FASE 12C: Build, test, bench, and clean workflow execution
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_PROJECT_RUNNER_H
#define CCT_PROJECT_RUNNER_H

#include "project_discovery.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CCT_PROJECT_PROFILE_DEBUG = 0,
    CCT_PROJECT_PROFILE_RELEASE = 1
} cct_project_profile_t;

typedef struct {
    const char *project_override;
    const char *entry_override;
    const char *out_override;
    const char *pattern;
    cct_project_profile_t profile;
    bool strict_lint;
    bool fmt_check;
    bool run_lint;
    bool clean_all;
    int iterations;
    int passthrough_argc;
    char **passthrough_argv;
} cct_project_options_t;

int cct_project_cmd_build(const char *self_path, const cct_project_options_t *options, char *built_output, size_t built_output_size);
int cct_project_cmd_run(const char *self_path, const cct_project_options_t *options);
int cct_project_cmd_test(const char *self_path, const cct_project_options_t *options);
int cct_project_cmd_bench(const char *self_path, const cct_project_options_t *options);
int cct_project_cmd_clean(const cct_project_options_t *options);

#endif
