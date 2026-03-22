/*
 * CCT — Clavicula Turing
 * Linter Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_LINT_H
#define CCT_LINT_H

#include "../common/types.h"
#include "../parser/ast.h"
#include <stdio.h>

typedef enum {
    CCT_LINT_RULE_UNUSED_VARIABLE = 0,
    CCT_LINT_RULE_UNUSED_PARAMETER,
    CCT_LINT_RULE_UNUSED_IMPORT,
    CCT_LINT_RULE_DEAD_CODE_AFTER_RETURN,
    CCT_LINT_RULE_DEAD_CODE_AFTER_THROW,
    CCT_LINT_RULE_SHADOWING_LOCAL
} cct_lint_rule_t;

typedef struct {
    cct_lint_rule_t rule;
    u32 line;
    u32 column;
    char *message;
} cct_lint_issue_t;

typedef struct {
    cct_lint_issue_t *items;
    size_t count;
    size_t capacity;
} cct_lint_issue_list_t;

typedef struct {
    bool strict;
    bool fix;
    bool quiet;
    bool format_after_fix;
} cct_lint_options_t;

typedef struct {
    cct_lint_issue_list_t issues;
    bool fixed_anything;
} cct_lint_report_t;

void cct_lint_report_init(cct_lint_report_t *report);
void cct_lint_report_dispose(cct_lint_report_t *report);

bool cct_lint_run_program(
    const cct_ast_program_t *program,
    const cct_lint_options_t *options,
    cct_lint_report_t *out_report
);

void cct_lint_emit_report(
    FILE *out,
    const char *file_path,
    const cct_lint_report_t *report
);

bool cct_lint_apply_fixes_to_file(
    const char *file_path,
    const cct_lint_report_t *report,
    bool format_after_fix
);

const char* cct_lint_rule_id(cct_lint_rule_t rule);

#endif /* CCT_LINT_H */
