/*
 * CCT — Clavicula Turing
 * Main Entry Point
 *
 * FASE 9E: C-hosted backend + modular subset final (ADVOCARE + visibility + dual sigilo + AST composed view)
 *
 * This is the entry point for the CCT compiler.
 * Currently implements:
 * - FASE 0: Foundation - CLI and error handling
 * - FASE 1: Lexical analysis - tokenization
 * - FASE 2: Syntax analysis and AST construction
 * - FASE 3: Semantic analysis (--check)
 * - FASE 4A/4B/4C: Executable code generation subset (compile .cct)
 * - FASE 5/5B/6A: Sigil generation (.svg + .sigil), styles, --sigilo-only
 * - FASE 9A/9B/9C/9D/9E: ADVOCARE discovery + resolution/visibility + dual-level modular sigilo + composed AST CLI
 *
 * Future phases will add:
 * - FASE 6+: Advanced features, optimization, bootstrap
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "common/types.h"
#include "common/errors.h"
#include "common/diagnostic.h"
#include "cli/cli.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "semantic/semantic.h"
#include "codegen/codegen.h"
#include "sigilo/sigilo.h"
#include "module/module.h"
#include "formatter/formatter.h"
#include "lint/lint.h"
#include "project/project.h"
#include "doc/doc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Read entire file into a string
 *
 * Returns: allocated string with file contents (caller must free)
 *          NULL on error
 */
static char* read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        cct_error_printf(CCT_ERROR_FILE_NOT_FOUND,
                        "Could not open file: %s", filename);
        return NULL;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    /* Allocate buffer */
    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        cct_error_printf(CCT_ERROR_OUT_OF_MEMORY,
                        "Could not allocate memory for file");
        return NULL;
    }

    /* Read file */
    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < size) {
        cct_error_printf(CCT_ERROR_FILE_READ,
                        "Could not read file: %s", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static bool has_cct_extension(const char *filename) {
    if (!filename) return false;
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".cct") == 0;
}

static int cmd_format(int argc, char **argv) {
    bool check_only = false;
    bool diff_only = false;
    int first_file = 2;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--check") == 0) {
            check_only = true;
            first_file = i + 1;
            continue;
        }
        if (strcmp(arg, "--diff") == 0) {
            diff_only = true;
            first_file = i + 1;
            continue;
        }
        first_file = i;
        break;
    }

    if (first_file >= argc) {
        fprintf(stderr, "Usage: cct fmt [--check|--diff] <file1.cct> [file2.cct ...]\n");
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }
    if (check_only && diff_only) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "fmt options --check and --diff cannot be combined");
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_formatter_options_t fmt_opts = cct_formatter_default_options();
    bool check_failed = false;

    for (int i = first_file; i < argc; i++) {
        const char *file = argv[i];
        if (file[0] == '-') {
            cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown fmt option: %s", file);
            return (int)CCT_ERROR_UNKNOWN_COMMAND;
        }
        if (!has_cct_extension(file)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Input file must have .cct extension: %s", file);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }

        cct_formatter_result_t res = cct_formatter_format_file(file, &fmt_opts);
        if (!res.success) {
            cct_error_printf(res.error_code == CCT_OK ? CCT_ERROR_INTERNAL : res.error_code,
                            "%s: %s",
                            file,
                            res.error_message ? res.error_message : "formatter failed");
            cct_formatter_result_dispose(&res);
            return (int)(res.error_code == CCT_OK ? CCT_ERROR_INTERNAL : res.error_code);
        }

        if (check_only) {
            if (res.changed) {
                fprintf(stderr, "Not formatted: %s\n", file);
                check_failed = true;
            }
        } else if (diff_only) {
            if (res.changed) {
                cct_formatter_print_diff(stdout, file, res.original_source, res.formatted_source);
            }
        } else {
            if (res.changed) {
                if (!cct_formatter_write_file(file, res.formatted_source)) {
                    cct_error_printf(CCT_ERROR_FILE_WRITE,
                                    "formatter could not write file: %s",
                                    file);
                    cct_formatter_result_dispose(&res);
                    return (int)CCT_ERROR_FILE_WRITE;
                }
                printf("Formatted: %s\n", file);
            } else {
                printf("Already formatted: %s\n", file);
            }
        }

        cct_formatter_result_dispose(&res);
    }

    if (check_only && check_failed) {
        return 2;
    }
    return (int)CCT_OK;
}

static int cmd_lint(int argc, char **argv) {
    bool strict = false;
    bool fix = false;
    bool quiet = false;
    bool format_after_fix = false;
    const char *file = NULL;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--strict") == 0) {
            strict = true;
            continue;
        }
        if (strcmp(arg, "--fix") == 0) {
            fix = true;
            continue;
        }
        if (strcmp(arg, "--quiet") == 0) {
            quiet = true;
            continue;
        }
        if (strcmp(arg, "--format-after-fix") == 0) {
            format_after_fix = true;
            continue;
        }
        if (arg[0] == '-') {
            cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown lint option: %s", arg);
            return (int)CCT_ERROR_UNKNOWN_COMMAND;
        }
        if (file) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                            "Multiple input files are not supported for lint: %s",
                            arg);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        file = arg;
    }

    if (!file) {
        fprintf(stderr, "Usage: cct lint [--strict] [--fix] [--quiet] [--format-after-fix] <file.cct>\n");
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }
    if (!has_cct_extension(file)) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "Input file must have .cct extension: %s",
                        file);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (format_after_fix && !fix) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "--format-after-fix requires --fix");
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(file, &bundle, &bundle_status)) {
        return 1;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, file);
    bool sem_ok = cct_semantic_analyze_program(&sem, bundle.program);
    bool sem_error = !sem_ok || cct_semantic_had_error(&sem);
    cct_semantic_dispose(&sem);
    cct_module_bundle_dispose(&bundle);
    if (sem_error) {
        return 1;
    }

    char *source = read_file(file);
    if (!source) {
        return 1;
    }
    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, file);
    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, file);
    cct_ast_program_t *local_program = cct_parser_parse_program(&parser);
    bool parse_error = local_program == NULL || cct_parser_had_error(&parser) || cct_lexer_had_error(&lexer);
    cct_parser_dispose(&parser);
    free(source);
    if (parse_error) {
        if (local_program) cct_ast_free_program(local_program);
        return 1;
    }

    cct_lint_options_t opts;
    opts.strict = strict;
    opts.fix = fix;
    opts.quiet = quiet;
    opts.format_after_fix = format_after_fix;

    cct_lint_report_t report;
    cct_lint_report_init(&report);
    bool lint_ok = cct_lint_run_program(local_program, &opts, &report);
    cct_ast_free_program(local_program);
    if (!lint_ok) {
        cct_lint_report_dispose(&report);
        return 3;
    }

    if (!quiet) {
        cct_lint_emit_report(stdout, file, &report);
    }

    bool fixed = false;
    if (fix) {
        fixed = cct_lint_apply_fixes_to_file(file, &report, format_after_fix);
        if (!fixed) {
            cct_lint_report_dispose(&report);
            return 3;
        }
        if (!quiet && report.issues.count > 0) {
            printf("Applied safe fixes: %s\n", file);
        }
    }

    int exit_code = 0;
    if (strict && report.issues.count > 0) {
        exit_code = 2;
    }
    if (!quiet && report.issues.count == 0) {
        printf("No lint issues: %s\n", file);
    } else if (!quiet) {
        printf("Lint issues: %zu\n", report.issues.count);
    }
    (void)fixed;
    cct_lint_report_dispose(&report);
    return exit_code;
}

/*
 * Process --tokens command
 *
 * Tokenizes the input file and prints all tokens
 */
static cct_error_code_t cmd_show_tokens(const char *filename) {
    /* Read source file */
    char *source = read_file(filename);
    if (!source) {
        return CCT_ERROR_FILE_READ;
    }

    /* Initialize lexer */
    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, filename);

    /* Tokenize and print */
    printf("Tokenizing: %s\n", filename);
    printf("%-6s %-20s %s\n", "LINE:COL", "TYPE", "LEXEME");
    printf("--------------------------------------------------------\n");

    while (true) {
        cct_token_t token = cct_lexer_next_token(&lexer);

        /* Print token */
        printf("%u:%-4u %-20s \"%s\"\n",
               token.line,
               token.column,
               cct_token_type_string(token.type),
               token.lexeme);

        bool is_eof = token.type == TOKEN_EOF;
        cct_token_free(&token);

        if (is_eof) break;
    }

    free(source);

    /* Check for errors */
    if (cct_lexer_had_error(&lexer)) {
        return CCT_ERROR_LEXICAL;
    }

    return CCT_OK;
}

/*
 * Process --ast command
 *
 * Parses the input file and prints the AST
 */
static cct_error_code_t cmd_show_ast(const char *filename) {
    /* Read source file */
    char *source = read_file(filename);
    if (!source) {
        return CCT_ERROR_FILE_READ;
    }

    /* Initialize lexer */
    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, filename);

    /* Initialize parser */
    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, filename);

    /* Parse program */
    printf("Parsing: %s\n", filename);
    printf("========================================\n\n");

    cct_ast_program_t *program = cct_parser_parse_program(&parser);

    if (!program) {
        cct_parser_dispose(&parser);
        free(source);
        return CCT_ERROR_SYNTAX;
    }

    /* Print AST */
    cct_ast_print_program(program);

    /* Cleanup */
    cct_ast_free_program(program);
    cct_parser_dispose(&parser);
    free(source);

    /* Check for errors */
    if (cct_parser_had_error(&parser)) {
        return CCT_ERROR_SYNTAX;
    }

    return CCT_OK;
}

/*
 * Process --ast-composite command (FASE 9E)
 *
 * Builds deterministic module closure and prints a composed AST view.
 */
static cct_error_code_t cmd_show_ast_composite(const char *filename) {
    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, &bundle, &bundle_status)) {
        return bundle_status;
    }

    printf("Composite parsing: %s\n", filename);
    printf("========================================\n\n");
    printf("Modules in closure: %u\n", bundle.module_count);
    printf("Import edges: %u\n\n", bundle.import_edge_count);

    cct_ast_print_program(bundle.program);

    cct_module_bundle_dispose(&bundle);
    return CCT_OK;
}

/*
 * Process --check command
 *
 * Parses the input file and runs semantic analysis.
 */
static cct_error_code_t cmd_check_semantic(const char *filename) {
    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename);

    bool ok = cct_semantic_analyze_program(&sem, bundle.program);
    if (ok) {
        printf("Semantic check OK: %s\n", filename);
    }

    bool had_sem_error = cct_semantic_had_error(&sem);
    cct_semantic_dispose(&sem);
    cct_module_bundle_dispose(&bundle);

    return had_sem_error ? CCT_ERROR_SEMANTIC : CCT_OK;
}

/*
 * Process --sigilo-only command
 *
 * Full validation pipeline without executable code generation:
 *   lexer -> parser -> semantic -> sigilo (.svg + .sigil)
 */
static cct_error_code_t configure_sigilo(cct_sigilo_t *sg, const cct_cli_args_t *args) {
    if (!sg || !args) return CCT_ERROR_INTERNAL;
    if (!cct_sigilo_set_style(sg, args->sigilo_style ? args->sigilo_style : "network")) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "Invalid sigilo style: %s", args->sigilo_style ? args->sigilo_style : "<null>");
        return CCT_ERROR_INVALID_ARGUMENT;
    }
    sg->emit_svg = args->sigilo_emit_svg;
    sg->emit_meta = args->sigilo_emit_meta;
    if (!sg->emit_svg && !sg->emit_meta) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                        "Invalid sigilo configuration: both SVG and metadata outputs are disabled");
        return CCT_ERROR_INVALID_ARGUMENT;
    }
    return CCT_OK;
}

static cct_ast_program_t* parse_program_for_sigilo(const char *filename, cct_error_code_t *out_error) {
    if (out_error) *out_error = CCT_OK;
    char *source = read_file(filename);
    if (!source) {
        if (out_error) *out_error = CCT_ERROR_FILE_READ;
        return NULL;
    }

    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, filename);

    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, filename);

    cct_ast_program_t *program = cct_parser_parse_program(&parser);
    bool parse_error = cct_parser_had_error(&parser) || !program;

    cct_parser_dispose(&parser);
    free(source);

    if (parse_error) {
        if (program) cct_ast_free_program(program);
        if (out_error) *out_error = CCT_ERROR_SYNTAX;
        return NULL;
    }

    return program;
}

static cct_error_code_t generate_single_local_sigilo(
    const cct_cli_args_t *args,
    const cct_module_bundle_t *bundle,
    const cct_ast_program_t *program,
    const char *source_path,
    const char *output_base_override
) {
    cct_sigilo_t sg;
    cct_sigilo_init(&sg, source_path);
    cct_error_code_t sg_cfg = configure_sigilo(&sg, args);
    if (sg_cfg != CCT_OK) {
        cct_sigilo_dispose(&sg);
        return sg_cfg;
    }

    cct_sigilo_set_module_context(
        &sg,
        bundle->module_count,
        bundle->import_edge_count,
        bundle->entry_module_path,
        bundle->cross_module_call_count,
        bundle->cross_module_type_ref_count,
        bundle->module_resolution_ok,
        bundle->public_symbol_count,
        bundle->internal_symbol_count
    );

    bool ok = cct_sigilo_generate_artifacts(
        &sg,
        program,
        source_path,
        output_base_override
    );
    bool had_error = cct_sigilo_had_error(&sg);
    cct_sigilo_dispose(&sg);

    if (!ok || had_error) {
        return CCT_ERROR_CODEGEN;
    }

    return CCT_OK;
}

static cct_error_code_t generate_sigilo_artifacts_for_bundle(
    const cct_cli_args_t *args,
    const cct_module_bundle_t *bundle
) {
    u32 parsed_count = (bundle->module_path_count > 0) ? bundle->module_path_count : 1;
    cct_ast_program_t **module_programs = (cct_ast_program_t**)calloc(parsed_count, sizeof(*module_programs));
    const char **module_paths = (const char**)calloc(parsed_count, sizeof(*module_paths));
    if (!module_programs || !module_paths) {
        free(module_programs);
        free(module_paths);
        return CCT_ERROR_OUT_OF_MEMORY;
    }

    cct_error_code_t parse_status = CCT_OK;
    for (u32 i = 0; i < parsed_count; i++) {
        const char *path = NULL;
        if (bundle->module_paths && i < bundle->module_path_count) {
            path = bundle->module_paths[i];
        }
        if (!path && i == 0) {
            path = args->input_file;
        }
        if (!path) {
            parse_status = CCT_ERROR_INTERNAL;
            goto cleanup;
        }
        module_paths[i] = path;
        module_programs[i] = parse_program_for_sigilo(path, &parse_status);
        if (!module_programs[i]) {
            goto cleanup;
        }
    }

    cct_error_code_t local_status = generate_single_local_sigilo(
        args,
        bundle,
        module_programs[0],
        args->input_file,
        args->sigilo_out_base
    );
    if (local_status != CCT_OK) {
        parse_status = local_status;
        goto cleanup;
    }

    if (args->sigilo_mode == CCT_SIGILO_MODE_COMPLETE) {
        for (u32 i = 1; i < parsed_count; i++) {
            const char *module_path = module_paths[i];
            if (!module_path) continue;

            char *mod_out_base = NULL;
            if (args->sigilo_out_base) {
                size_t needed = strlen(args->sigilo_out_base) + 32;
                mod_out_base = (char*)malloc(needed);
                if (!mod_out_base) {
                    parse_status = CCT_ERROR_OUT_OF_MEMORY;
                    goto cleanup;
                }
                snprintf(mod_out_base, needed, "%s.__mod_%03u", args->sigilo_out_base, i);
            }

            local_status = generate_single_local_sigilo(
                args,
                bundle,
                module_programs[i],
                module_path,
                mod_out_base
            );
            free(mod_out_base);
            if (local_status != CCT_OK) {
                parse_status = local_status;
                goto cleanup;
            }
        }
    }

    if (bundle->module_count <= 1) {
        parse_status = CCT_OK;
        goto cleanup;
    }

    cct_sigilo_t system_sg;
    cct_sigilo_init(&system_sg, args->input_file);
    cct_error_code_t sg_cfg = configure_sigilo(&system_sg, args);
    if (sg_cfg != CCT_OK) {
        cct_sigilo_dispose(&system_sg);
        parse_status = sg_cfg;
        goto cleanup;
    }

    cct_sigilo_set_module_context(
        &system_sg,
        bundle->module_count,
        bundle->import_edge_count,
        bundle->entry_module_path,
        bundle->cross_module_call_count,
        bundle->cross_module_type_ref_count,
        bundle->module_resolution_ok,
        bundle->public_symbol_count,
        bundle->internal_symbol_count
    );

    bool system_ok = cct_sigilo_generate_system_artifacts(
        &system_sg,
        args->input_file,
        args->sigilo_out_base,
        (const cct_ast_program_t *const *)module_programs,
        module_paths,
        parsed_count,
        bundle->import_from_indices,
        bundle->import_to_indices,
        bundle->import_graph_edge_count,
        bundle->cross_module_call_count,
        bundle->cross_module_type_ref_count,
        bundle->public_symbol_count,
        bundle->internal_symbol_count,
        bundle->module_resolution_ok
    );
    bool system_had_error = cct_sigilo_had_error(&system_sg);
    cct_sigilo_dispose(&system_sg);

    if (!system_ok || system_had_error) {
        parse_status = CCT_ERROR_CODEGEN;
        goto cleanup;
    }

    parse_status = CCT_OK;

cleanup:
    for (u32 i = 0; i < parsed_count; i++) {
        if (module_programs[i]) cct_ast_free_program(module_programs[i]);
    }
    free(module_programs);
    free(module_paths);
    return parse_status;
}

static cct_error_code_t cmd_sigilo_only(const cct_cli_args_t *args) {
    const char *filename = args->input_file;
    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename);
    bool sem_ok = cct_semantic_analyze_program(&sem, bundle.program);
    if (!sem_ok || cct_semantic_had_error(&sem)) {
        cct_semantic_dispose(&sem);
        cct_module_bundle_dispose(&bundle);
        return CCT_ERROR_SEMANTIC;
    }

    cct_error_code_t sigilo_status = generate_sigilo_artifacts_for_bundle(args, &bundle);

    cct_semantic_dispose(&sem);
    cct_module_bundle_dispose(&bundle);

    return sigilo_status;
}

/*
 * Process compilation command (FASE 4A/4B/4C + FASE 5/5B/6A sigilo integration)
 *
 * Full pipeline:
 *   lexer -> parser -> semantic -> sigilo -> codegen -> executable
 */
static cct_error_code_t cmd_compile_file(const cct_cli_args_t *args) {
    const char *filename = args->input_file;
    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename);
    bool sem_ok = cct_semantic_analyze_program(&sem, bundle.program);

    if (!sem_ok || cct_semantic_had_error(&sem)) {
        cct_semantic_dispose(&sem);
        cct_module_bundle_dispose(&bundle);
        return CCT_ERROR_SEMANTIC;
    }

    cct_error_code_t sigilo_status = generate_sigilo_artifacts_for_bundle(args, &bundle);

    cct_codegen_t cg;
    cct_codegen_init(&cg, filename);
    bool cg_ok = false;
    bool cg_had_error = false;
    if (sigilo_status == CCT_OK) {
        cg_ok = cct_codegen_compile_program(&cg, bundle.program, filename, NULL);
        cg_had_error = cct_codegen_had_error(&cg);
    }
    cct_codegen_dispose(&cg);

    cct_semantic_dispose(&sem);
    cct_module_bundle_dispose(&bundle);

    if (sigilo_status != CCT_OK || !cg_ok || cg_had_error) {
        return CCT_ERROR_CODEGEN;
    }

    return CCT_OK;
}

/*
 * Main entry point
 */
int main(int argc, char **argv) {
    cct_cli_args_t args;
    cct_error_code_t result;

    if (argc >= 2 && cct_project_is_command(argv[1])) {
        return cct_project_dispatch(argc, argv, argv[0]);
    }
    if (argc >= 2 && strcmp(argv[1], "doc") == 0) {
        return cct_doc_command(argc, argv);
    }
    if (argc >= 2 && strcmp(argv[1], "fmt") == 0) {
        return cmd_format(argc, argv);
    }
    if (argc >= 2 && strcmp(argv[1], "lint") == 0) {
        return cmd_lint(argc, argv);
    }

    /* Parse command-line arguments */
    result = cct_cli_parse(argc, argv, &args);

    /* Handle errors */
    if (result != CCT_OK) {
        return (int)result;
    }

    if (args.no_color) {
        cct_diagnostic_set_colors(false);
    }

    /* Execute command */
    switch (args.command) {
        case CCT_CMD_HELP:
            cct_cli_show_help();
            return EXIT_SUCCESS;

        case CCT_CMD_VERSION:
            cct_cli_show_version();
            return EXIT_SUCCESS;

        case CCT_CMD_COMPILE:
            /* FASE 4A/4B/4C: Full compile pipeline */
            return (int)cmd_compile_file(&args);

        case CCT_CMD_SHOW_TOKENS:
            /* FASE 1: Lexical analysis */
            return (int)cmd_show_tokens(args.input_file);

        case CCT_CMD_SHOW_AST:
            /* FASE 2: Syntax analysis and AST */
            return (int)cmd_show_ast(args.input_file);

        case CCT_CMD_SHOW_AST_COMPOSITE:
            /* FASE 9E: composed AST for multi-module closure */
            return (int)cmd_show_ast_composite(args.input_file);

        case CCT_CMD_CHECK_ONLY:
            /* FASE 3: Semantic analysis */
            return (int)cmd_check_semantic(args.input_file);

        case CCT_CMD_SIGILO_ONLY:
            return (int)cmd_sigilo_only(&args);

        case CCT_CMD_NONE:
        default:
            cct_cli_show_help();
            return EXIT_SUCCESS;
    }
}
