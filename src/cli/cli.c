/*
 * CCT — Clavicula Turing
 * Command Line Interface Implementation
 *
 * FASE 7A: CLI integration for lexer/parser/semantic/codegen/sigilo commands
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "cli.h"
#include <stdio.h>
#include <string.h>

static bool cli_has_cct_extension(const char *filename) {
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".cct") == 0;
}

static bool cli_is_valid_sigilo_style(const char *style) {
    return style &&
           (strcmp(style, "network") == 0 ||
            strcmp(style, "seal") == 0 ||
            strcmp(style, "scriptum") == 0);
}

static bool cli_parse_sigilo_mode(const char *mode, cct_sigilo_mode_t *out_mode) {
    if (!mode || !out_mode) return false;
    if (strcmp(mode, "essential") == 0 || strcmp(mode, "essencial") == 0) {
        *out_mode = CCT_SIGILO_MODE_ESSENTIAL;
        return true;
    }
    if (strcmp(mode, "complete") == 0 || strcmp(mode, "completo") == 0) {
        *out_mode = CCT_SIGILO_MODE_COMPLETE;
        return true;
    }
    return false;
}

/*
 * Display help message
 */
void cct_cli_show_help(void) {
    printf("CCT — Clavicula Turing\n");
    printf("A ritual programming language with deterministic sigil generation\n\n");

    printf("Usage:\n");
    printf("  cct [options] <file.cct>    Compile a CCT source file\n");
    printf("  cct build [options]\n");
    printf("  cct run [options] [-- args]\n");
    printf("  cct test [pattern] [options]\n");
    printf("  cct bench [pattern] [options]\n");
    printf("  cct clean [options]\n");
    printf("  cct doc [options]\n");
    printf("  cct fmt [options] <file.cct> [more.cct ...]\n");
    printf("  cct lint [options] <file.cct>\n");
    printf("  cct --help                  Show this help message\n");
    printf("  cct --version               Show version information\n\n");

    printf("Options:\n");
    printf("  --help                      Display this help message\n");
    printf("  --version                   Display version information\n");
    printf("  --tokens <file.cct>         Display token stream (FASE 1)\n");
    printf("  --ast <file.cct>            Display abstract syntax tree (FASE 2B)\n");
    printf("  --ast-composite <file.cct>  Display composed AST for module closure (FASE 9E)\n");
    printf("  --check <file.cct>          Syntax + semantic check (FASE 3)\n");
    printf("  --no-color                  Disable colored diagnostics (FASE 12A)\n");
    printf("  --sigilo-only <file.cct>    Generate sigilo artifacts only (FASE 5+)\n");
    printf("  --sigilo-style <style>      Sigilo style: network|seal|scriptum (FASE 6A)\n");
    printf("  --sigilo-mode <mode>        Multi-module sigilo emission: essencial|completo\n");
    printf("                              (aliases kept: essential|complete) (FASE 9E)\n");
    printf("  --sigilo-out <basepath>     Output base for sigilo artifacts (FASE 6A)\n");
    printf("  --sigilo-no-meta            Skip .sigil metadata emission (FASE 6A)\n");
    printf("  --sigilo-no-svg             Skip .svg emission (FASE 6A)\n\n");
    printf("Formatter command:\n");
    printf("  cct fmt <file.cct> [...]    Format file(s) in place (FASE 12E.1)\n");
    printf("  cct fmt --check <file...>   Check formatting only (exit 2 on mismatch)\n");
    printf("  cct fmt --diff <file...>    Print formatting diff without writing\n\n");
    printf("Linter command:\n");
    printf("  cct lint <file.cct>         Run canonical lint checks (FASE 12E.2)\n");
    printf("  cct lint --strict <file>    Return exit 2 when warnings are present\n");
    printf("  cct lint --fix <file>       Apply safe auto-fixes (subset)\n");
    printf("  cct lint --quiet <file>     Suppress per-issue output\n\n");
    printf("Project commands (FASE 12F):\n");
    printf("  cct build [--release] [--project DIR] [--entry FILE] [--out PATH]\n");
    printf("            [--lint] [--fmt-check]\n");
    printf("  cct run [--release] [--project DIR] [--entry FILE] [--out PATH] [-- ARGS...]\n");
    printf("  cct test [pattern] [--project DIR] [--strict-lint] [--fmt-check]\n");
    printf("  cct bench [pattern] [--project DIR] [--iterations N] [--release]\n");
    printf("  cct clean [--project DIR] [--all]\n\n");
    printf("Documentation command (FASE 12G):\n");
    printf("  cct doc [--project DIR] [--entry FILE] [--output-dir DIR]\n");
    printf("          [--format markdown|html|both] [--include-internal]\n");
    printf("          [--no-examples] [--warn-missing-docs] [--strict-docs]\n");
    printf("          [--no-timestamp]\n\n");

    printf("Current status: FASE 12H (Release v0.12 - Structural maturity milestone)\n");
    printf("Complete toolchain: compiler, formatter, linter, project manager, and doc generator.\n");
    printf("Bibliotheca Canonica (stdlib) consolidated with stability guarantees.\n\n");

    printf("License: Copyright (c) Erick Andrade Busato. Todos os direitos reservados.\n");
    printf("Project: https://github.com/your-repo/cct\n");
}

/*
 * Display version information
 */
void cct_cli_show_version(void) {
    printf("Clavicula Turing (CCT) v%s\n", CCT_VERSION_STRING);
    printf("Target: Linux x86_64\n");
    printf("Build: FASE 12H - Structural maturity milestone (Release v0.12)\n");
    printf("\n");
    printf("Copyright (c) Erick Andrade Busato. Todos os direitos reservados.\n");
}

/*
 * Parse command-line arguments
 */
cct_error_code_t cct_cli_parse(int argc, char **argv, cct_cli_args_t *args) {
    /* Initialize args struct */
    args->command = CCT_CMD_NONE;
    args->input_file = NULL;
    args->output_file = NULL;
    args->sigilo_style = "network";
    args->sigilo_out_base = NULL;
    args->sigilo_mode = CCT_SIGILO_MODE_ESSENTIAL;
    args->sigilo_emit_svg = true;
    args->sigilo_emit_meta = true;
    args->no_color = false;
    args->verbose = false;
    args->debug = false;

    /* No arguments provided */
    if (argc < 2) {
        args->command = CCT_CMD_HELP;
        return CCT_OK;
    }

    /* Parse global flags first (valid for every command shape) */
    int normalized_argc = 1;
    char *normalized_argv[argc + 1];
    normalized_argv[0] = argv[0];
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) {
            args->no_color = true;
            continue;
        }
        normalized_argv[normalized_argc++] = argv[i];
    }
    normalized_argv[normalized_argc] = NULL;

    if (normalized_argc < 2) {
        args->command = CCT_CMD_HELP;
        return CCT_OK;
    }

    /* Parse first argument */
    const char *arg = normalized_argv[1];

    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
        args->command = CCT_CMD_HELP;
        return CCT_OK;
    }

    if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
        args->command = CCT_CMD_VERSION;
        return CCT_OK;
    }

    /* Handle --tokens: requires a file argument */
    if (strcmp(arg, "--tokens") == 0) {
        if (normalized_argc < 3) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                            "--tokens requires a file argument");
            fprintf(stderr, "Usage: cct --tokens <file.cct>\n");
            return CCT_ERROR_MISSING_ARGUMENT;
        }

        const char *filename = normalized_argv[2];
        size_t len = strlen(filename);
        if (len < 4 || strcmp(filename + len - 4, ".cct") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Input file must have .cct extension: %s", filename);
            return CCT_ERROR_INVALID_ARGUMENT;
        }

        args->command = CCT_CMD_SHOW_TOKENS;
        args->input_file = filename;
        return CCT_OK;
    }

    /* Handle --ast: FASE 2B */
    if (strcmp(arg, "--ast") == 0) {
        if (normalized_argc < 3) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                            "--ast requires a file argument");
            fprintf(stderr, "Usage: cct --ast <file.cct>\n");
            return CCT_ERROR_MISSING_ARGUMENT;
        }

        const char *filename = normalized_argv[2];
        size_t len = strlen(filename);
        if (len < 4 || strcmp(filename + len - 4, ".cct") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Input file must have .cct extension: %s", filename);
            return CCT_ERROR_INVALID_ARGUMENT;
        }

        args->command = CCT_CMD_SHOW_AST;
        args->input_file = filename;
        return CCT_OK;
    }

    if (strcmp(arg, "--ast-composite") == 0) {
        if (normalized_argc < 3) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                            "--ast-composite requires a file argument");
            fprintf(stderr, "Usage: cct --ast-composite <file.cct>\n");
            return CCT_ERROR_MISSING_ARGUMENT;
        }

        const char *filename = normalized_argv[2];
        size_t len = strlen(filename);
        if (len < 4 || strcmp(filename + len - 4, ".cct") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Input file must have .cct extension: %s", filename);
            return CCT_ERROR_INVALID_ARGUMENT;
        }

        args->command = CCT_CMD_SHOW_AST_COMPOSITE;
        args->input_file = filename;
        return CCT_OK;
    }

    if (strcmp(arg, "--check") == 0) {
        if (normalized_argc < 3) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                            "--check requires a file argument");
            fprintf(stderr, "Usage: cct --check <file.cct>\n");
            return CCT_ERROR_MISSING_ARGUMENT;
        }

        const char *filename = normalized_argv[2];
        size_t len = strlen(filename);
        if (len < 4 || strcmp(filename + len - 4, ".cct") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Input file must have .cct extension: %s", filename);
            return CCT_ERROR_INVALID_ARGUMENT;
        }

        args->command = CCT_CMD_CHECK_ONLY;
        args->input_file = filename;
        return CCT_OK;
    }

    /* Generic parse for compile / --sigilo-only with optional sigilo flags (FASE 6A) */
    const char *positional_file = NULL;
    bool sigilo_only = false;

    for (int i = 1; i < normalized_argc; i++) {
        const char *cur = normalized_argv[i];

        if (strcmp(cur, "--sigilo-only") == 0) {
            sigilo_only = true;
            continue;
        }
        if (strcmp(cur, "--sigilo-style") == 0) {
            if (i + 1 >= normalized_argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                                "--sigilo-style requires a style name (network|seal|scriptum)");
                return CCT_ERROR_MISSING_ARGUMENT;
            }
            args->sigilo_style = normalized_argv[++i];
            if (!cli_is_valid_sigilo_style(args->sigilo_style)) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                                "Invalid sigilo style: %s (expected network|seal|scriptum)",
                                args->sigilo_style);
                return CCT_ERROR_INVALID_ARGUMENT;
            }
            continue;
        }
        if (strcmp(cur, "--sigilo-mode") == 0) {
            cct_sigilo_mode_t parsed_mode = CCT_SIGILO_MODE_ESSENTIAL;
            if (i + 1 >= normalized_argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                                "--sigilo-mode requires a mode name (essencial|completo)");
                return CCT_ERROR_MISSING_ARGUMENT;
            }
            if (!cli_parse_sigilo_mode(normalized_argv[++i], &parsed_mode)) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                                "Invalid sigilo mode: %s (expected essencial|completo; aliases: essential|complete, subset modular da FASE 9)",
                                normalized_argv[i]);
                return CCT_ERROR_INVALID_ARGUMENT;
            }
            args->sigilo_mode = parsed_mode;
            continue;
        }
        if (strcmp(cur, "--sigilo-out") == 0) {
            if (i + 1 >= normalized_argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                                "--sigilo-out requires a base path");
                return CCT_ERROR_MISSING_ARGUMENT;
            }
            args->sigilo_out_base = normalized_argv[++i];
            args->output_file = args->sigilo_out_base;
            continue;
        }
        if (strcmp(cur, "--sigilo-no-meta") == 0) {
            args->sigilo_emit_meta = false;
            continue;
        }
        if (strcmp(cur, "--sigilo-no-svg") == 0) {
            args->sigilo_emit_svg = false;
            continue;
        }

        if (cur[0] == '-') {
            cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown option: %s", cur);
            fprintf(stderr, "Try 'cct --help' for more information.\n");
            return CCT_ERROR_UNKNOWN_COMMAND;
        }

        if (positional_file) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Multiple input files are not supported: %s", cur);
            return CCT_ERROR_INVALID_ARGUMENT;
        }
        positional_file = cur;
    }

    if (!args->sigilo_emit_svg && !args->sigilo_emit_meta) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "Invalid sigilo options: both --sigilo-no-svg and --sigilo-no-meta were set");
        return CCT_ERROR_INVALID_ARGUMENT;
    }

    if (!positional_file) {
        if (sigilo_only || args->sigilo_out_base || strcmp(args->sigilo_style, "network") != 0 ||
            args->sigilo_mode != CCT_SIGILO_MODE_ESSENTIAL ||
            !args->sigilo_emit_svg || !args->sigilo_emit_meta) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT,
                            "A .cct input file is required");
            return CCT_ERROR_MISSING_ARGUMENT;
        }
        args->command = CCT_CMD_HELP;
        return CCT_OK;
    }

    if (!cli_has_cct_extension(positional_file)) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "Input file must have .cct extension: %s", positional_file);
        return CCT_ERROR_INVALID_ARGUMENT;
    }

    args->input_file = positional_file;
    args->command = sigilo_only ? CCT_CMD_SIGILO_ONLY : CCT_CMD_COMPILE;
    return CCT_OK;
}
