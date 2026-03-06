/*
 * CCT — Clavicula Turing
 * Command Line Interface
 *
 * FASE 7A: CLI argument parsing for compiler + sigilo generation UX
 *
 * This module handles command-line argument parsing and provides
 * a clean interface for the main program.
 *
 * Current commands:
 * - --help    : Display usage information
 * - --version : Display version information
 * - --tokens  : Display token stream (FASE 1)
 * - --ast     : Display abstract syntax tree (FASE 2B)
 * - --ast-composite : Display composed AST for module closure (FASE 9E)
 * - --check   : Syntax + semantic check (FASE 3)
 *
 * Current commands:
 * - --sigilo-only : Generate sigil artifacts only (FASE 5/5B/6A)
 * - --sigilo-style / --sigilo-out / --sigilo-no-meta / --sigilo-no-svg
 * - <file.cct>    : Compile source file + generate sigil artifacts
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_CLI_H
#define CCT_CLI_H

#include "../common/types.h"
#include "../common/errors.h"

/*
 * CLI command types
 */
typedef enum {
    CCT_CMD_NONE,
    CCT_CMD_HELP,
    CCT_CMD_VERSION,
    CCT_CMD_COMPILE,        /* FASE 4C + FASE 5/5B sigilo integration */
    CCT_CMD_SHOW_TOKENS,    /* FASE 1 */
    CCT_CMD_SHOW_AST,       /* FASE 2B */
    CCT_CMD_SHOW_AST_COMPOSITE, /* FASE 9E */
    CCT_CMD_CHECK_ONLY,     /* FASE 3 */
    CCT_CMD_SIGILO_ONLY,    /* FASE 5/5B */
} cct_command_t;

typedef enum {
    CCT_SIGILO_MODE_ESSENTIAL = 0,
    CCT_SIGILO_MODE_COMPLETE
} cct_sigilo_mode_t;

/*
 * Parsed CLI arguments
 */
typedef struct {
    cct_command_t command;
    const char *input_file;
    const char *output_file;
    const char *entry_rituale;      /* FASE 16B.4: explicit freestanding entry ritual name */
    const char *sigilo_style;
    const char *sigilo_out_base;
    cct_sigilo_mode_t sigilo_mode;
    bool sigilo_emit_svg;
    bool sigilo_emit_meta;
    bool emit_asm;                 /* FASE 16C.1: emit freestanding assembly (.cgen.s) */
    bool no_color;
    bool verbose;
    bool debug;
    cct_profile_t profile;          /* FASE 16A.2: compilation profile */
} cct_cli_args_t;

/*
 * Parse command-line arguments
 * Returns error code (CCT_OK on success)
 */
cct_error_code_t cct_cli_parse(int argc, char **argv, cct_cli_args_t *args);

/*
 * Display help message
 */
void cct_cli_show_help(void);

/*
 * Display version information
 */
void cct_cli_show_version(void);

#endif /* CCT_CLI_H */
