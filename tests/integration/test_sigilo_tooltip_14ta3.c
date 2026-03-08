/*
 * CCT — Internal Sigilo Tooltip Tests (FASE 14TA3)
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

static int contains_text(const char *haystack, const char *needle) {
    return haystack && needle && strstr(haystack, needle) != NULL;
}

static int build_model_and_geom(const char *source_path, cct_sigilo_model_t *model, cct_sigilo_geom_t *geom) {
    cct_ast_program_t *program = load_semantic_program(source_path);
    cct_sigilo_t sg;
    int ok;

    if (!program) return 0;

    cct_sigilo_init(&sg, source_path);
    ok = sg_extract_model(&sg, program, source_path, model);
    if (ok) sg_build_geom(model, geom);
    cct_sigilo_dispose(&sg);
    cct_ast_free_program(program);
    return ok;
}

static int test_prepare_tooltip_text_normalizes_and_escapes(void) {
    char *tooltip = sg_prepare_tooltip_text("stmt:\tSI  \n\n\nsource:\tEVOCA VERBUM s AD \"<a&b>\"   ");
    int ok = tooltip != NULL &&
        strcmp(tooltip, "stmt:  SI\n\nsource:  EVOCA VERBUM s AD \"&lt;a&amp;b&gt;\"") == 0;
    free(tooltip);
    return ok;
}

static int test_clip_tooltip_text_is_deterministic(void) {
    const char *raw =
        "linha 1\n"
        "linha 2\n"
        "linha 3\n"
        "linha 4\n"
        "linha 5\n"
        "linha 6\n"
        "linha 7\n"
        "linha 8\n"
        "linha 9";
    char *clipped = sg_clip_tooltip_text(raw, 8, 240);
    int ok = clipped != NULL &&
        strcmp(
            clipped,
            "linha 1\nlinha 2\nlinha 3\nlinha 4\nlinha 5\nlinha 6\nlinha 7\n..."
        ) == 0;
    free(clipped);
    return ok;
}

static int test_escape_fixture_generates_safe_tooltip(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;
    int found_binding = 0;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom("tests/integration/sigilo_tooltip_escape_14ta3.cct", &model, &geom);
    for (u32 i = 0; ok && i < geom.node_count; i++) {
        cct_sigilo_node_t *node = &geom.nodes[i];
        if (node->kind != SGN_BINDING) continue;
        if (contains_text(node->tooltip_text, "&lt;a&amp;b&gt;")) {
            found_binding = 1;
            break;
        }
    }
    ok = ok && found_binding;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

static int test_clip_and_tabs_fixture_generates_prepared_tooltip(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;
    int found_binding = 0;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom("tests/integration/sigilo_tooltip_tabs_14ta3.cct", &model, &geom);
    for (u32 i = 0; ok && i < geom.node_count; i++) {
        cct_sigilo_node_t *node = &geom.nodes[i];
        if (node->kind != SGN_BINDING) continue;
        if (node->tooltip_text &&
            strchr(node->tooltip_text, '\t') == NULL &&
            contains_text(node->tooltip_text, "EVOCA  REX  x AD 1")) {
            found_binding = 1;
            break;
        }
    }
    ok = ok && found_binding;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

static int test_long_source_line_is_clipped_in_tooltip(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;
    int found_clipped = 0;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom("tests/integration/sigilo_tooltip_clip_14ta3.cct", &model, &geom);
    for (u32 i = 0; ok && i < geom.node_count; i++) {
        cct_sigilo_node_t *node = &geom.nodes[i];
        if (node->kind != SGN_BINDING) continue;
        if (contains_text(node->tooltip_text, "...")) {
            found_clipped = 1;
            break;
        }
    }
    ok = ok && found_clipped;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

int main(void) {
    if (!test_prepare_tooltip_text_normalizes_and_escapes()) return 1;
    if (!test_clip_tooltip_text_is_deterministic()) return 2;
    if (!test_escape_fixture_generates_safe_tooltip()) return 3;
    if (!test_clip_and_tabs_fixture_generates_prepared_tooltip()) return 4;
    if (!test_long_source_line_is_clipped_in_tooltip()) return 5;
    return 0;
}
