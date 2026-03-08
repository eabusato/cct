/*
 * CCT — Internal Sigilo Source Context Tests (FASE 14TA1)
 */

#include "../../src/common/types.h"
#include "../../src/common/errors.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/semantic/semantic.h"
#include "../../src/sigilo/sigilo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/sigilo/sigilo.c"

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static char *read_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    long n;
    char *buf;
    size_t read_n;

    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    n = ftell(f);
    if (n < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    buf = (char*)calloc(1, (size_t)n + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    read_n = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (read_n != (size_t)n) {
        free(buf);
        return NULL;
    }
    return buf;
}

static cct_ast_program_t *load_semantic_program(const char *path) {
    char *source = read_text_file(path);
    cct_lexer_t lexer;
    cct_parser_t parser;
    cct_ast_program_t *program = NULL;
    cct_semantic_analyzer_t sem;
    bool ok;

    if (!source) return NULL;

    cct_lexer_init(&lexer, source, path);
    cct_parser_init(&parser, &lexer, path);
    program = cct_parser_parse_program(&parser);
    ok = program != NULL && !cct_parser_had_error(&parser) && !cct_lexer_had_error(&lexer);
    cct_parser_dispose(&parser);
    if (!ok) {
        if (program) cct_ast_free_program(program);
        free(source);
        return NULL;
    }

    cct_semantic_init(&sem, path, CCT_PROFILE_HOST);
    ok = cct_semantic_analyze_program(&sem, program) && !cct_semantic_had_error(&sem);
    cct_semantic_dispose(&sem);
    free(source);
    if (!ok) {
        cct_ast_free_program(program);
        return NULL;
    }

    return program;
}

static int expect_line(
    const sg_source_buffer_t *src,
    u32 line,
    const char *expected
) {
    const char *actual;
    size_t actual_len = 0;
    size_t expected_len;

    actual = sg_source_get_line(src, line, &actual_len);
    if (!actual || !expected) return 0;
    expected_len = strlen(expected);
    return actual_len == expected_len && strncmp(actual, expected, actual_len) == 0;
}

static int test_basic_line_lookup(void) {
    sg_source_buffer_t src;
    int ok;

    memset(&src, 0, sizeof(src));
    ok = sg_source_load(&src, "tests/integration/sigilo_source_context_basic_14ta1.cct");
    ok = ok && src.available;
    ok = ok && src.line_count == 7;
    ok = ok && expect_line(&src, 1, "INCIPIT grimoire \"sigilo_source_context_14ta1\"");
    ok = ok && expect_line(&src, 3, "RITUALE main() REDDE REX");
    ok = ok && expect_line(&src, 4, "  REDDE 0");
    sg_source_dispose(&src);
    return ok;
}

static int test_multiline_span(void) {
    sg_source_buffer_t src;
    char *span;
    int ok;

    memset(&src, 0, sizeof(src));
    ok = sg_source_load(&src, "tests/integration/sigilo_source_context_multiline_14ta1.cct");
    span = sg_source_extract_span(&src, 4, 9);
    ok = ok && src.available;
    ok = ok && src.line_count == 13;
    ok = ok && span != NULL;
    ok = ok && strcmp(
        span,
        "  EVOCA REX x AD 1\n"
        "  SI x < 2\n"
        "    VINCIRE x AD x + 8\n"
        "  ALITER\n"
        "    VINCIRE x AD x + 3\n"
        "  FIN SI"
    ) == 0;
    free(span);
    sg_source_dispose(&src);
    return ok;
}

static int test_crlf_normalization(void) {
    sg_source_buffer_t lf;
    sg_source_buffer_t crlf;
    char *span_lf;
    char *span_crlf;
    int ok;

    memset(&lf, 0, sizeof(lf));
    memset(&crlf, 0, sizeof(crlf));
    ok = sg_source_load(&lf, "tests/integration/sigilo_source_context_basic_14ta1.cct");
    ok = ok && sg_source_load(&crlf, "tests/integration/sigilo_source_context_crlf_14ta1.cct");
    ok = ok && lf.line_count == crlf.line_count;
    ok = ok && expect_line(&crlf, 1, "INCIPIT grimoire \"sigilo_source_context_14ta1\"");
    ok = ok && expect_line(&crlf, 4, "  REDDE 0");

    span_lf = sg_source_extract_span(&lf, 3, 4);
    span_crlf = sg_source_extract_span(&crlf, 3, 4);
    ok = ok && span_lf != NULL && span_crlf != NULL;
    ok = ok && strcmp(span_lf, span_crlf) == 0;

    free(span_lf);
    free(span_crlf);
    sg_source_dispose(&lf);
    sg_source_dispose(&crlf);
    return ok;
}

static int test_missing_source_state(void) {
    sg_source_buffer_t src;
    size_t len = 0;
    int ok;

    memset(&src, 0, sizeof(src));
    ok = !sg_source_load(&src, "tests/integration/nao_existe_sigilo_source_context_14ta1.cct");
    ok = ok && !src.available;
    ok = ok && src.path != NULL;
    ok = ok && sg_source_get_line(&src, 1, &len) == NULL;
    ok = ok && len == 0;
    ok = ok && sg_source_extract_span(&src, 1, 1) == NULL;
    sg_source_dispose(&src);
    return ok;
}

static int test_missing_source_is_non_fatal_for_sigilo(void) {
    const char *src_path = "tests/integration/sigilo_source_context_basic_14ta1.cct";
    const char *out_base = "tests/integration/sigilo_source_context_missing_source_14ta1";
    cct_ast_program_t *program = load_semantic_program(src_path);
    cct_sigilo_t sg;
    int ok;

    if (!program) return 0;

    remove("tests/integration/sigilo_source_context_missing_source_14ta1.svg");
    remove("tests/integration/sigilo_source_context_missing_source_14ta1.sigil");

    cct_sigilo_init(&sg, src_path);
    ok = cct_sigilo_generate_artifacts(
        &sg,
        program,
        "tests/integration/nao_existe_sigilo_source_context_14ta1.cct",
        out_base
    );
    ok = ok && file_exists("tests/integration/sigilo_source_context_missing_source_14ta1.svg");
    ok = ok && file_exists("tests/integration/sigilo_source_context_missing_source_14ta1.sigil");

    cct_sigilo_dispose(&sg);
    cct_ast_free_program(program);
    return ok;
}

int main(void) {
    if (!test_basic_line_lookup()) return 1;
    if (!test_multiline_span()) return 2;
    if (!test_crlf_normalization()) return 3;
    if (!test_missing_source_state()) return 4;
    if (!test_missing_source_is_non_fatal_for_sigilo()) return 5;
    return 0;
}
