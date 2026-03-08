/*
 * CCT — Internal Sigilo Context Tests (FASE 14TA2)
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

static int build_model_and_geom(
    const char *source_path,
    const char *sigilo_source_path,
    cct_sigilo_model_t *model,
    cct_sigilo_geom_t *geom
) {
    cct_ast_program_t *program = load_semantic_program(source_path);
    cct_sigilo_t sg;
    int ok;

    if (!program) return 0;

    cct_sigilo_init(&sg, source_path);
    ok = sg_extract_model(&sg, program, sigilo_source_path, model);
    if (ok) sg_build_geom(model, geom);
    cct_sigilo_dispose(&sg);
    cct_ast_free_program(program);
    return ok;
}

static int test_rituale_context(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom(
        "tests/integration/sigilo_context_rituale_14ta2.cct",
        "tests/integration/sigilo_context_rituale_14ta2.cct",
        &model,
        &geom
    );
    ok = ok && model.ritual_count == 1;
    ok = ok && model.rituals[0].signature_excerpt != NULL;
    ok = ok && strcmp(
        model.rituals[0].signature_excerpt,
        "RITUALE normalizar_numero(VERBUM txt) REDDE VERBUM"
    ) == 0;
    ok = ok && geom.node_count >= 1;
    ok = ok && contains_text(geom.nodes[0].summary_label, "rituale: normalizar_numero");
    ok = ok && contains_text(geom.nodes[0].summary_label, "signature: RITUALE normalizar_numero(VERBUM txt) REDDE VERBUM");
    ok = ok && strcmp(geom.nodes[0].source_excerpt, "RITUALE normalizar_numero(VERBUM txt) REDDE VERBUM") == 0;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

static int test_loop_node_context(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;
    cct_sigilo_node_t *loop_node = NULL;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom(
        "tests/integration/sigilo_context_loop_14ta2.cct",
        "tests/integration/sigilo_context_loop_14ta2.cct",
        &model,
        &geom
    );
    for (u32 i = 0; ok && i < geom.node_count; i++) {
        if (geom.nodes[i].kind == SGN_DUM) {
            loop_node = &geom.nodes[i];
            break;
        }
    }
    ok = ok && loop_node != NULL;
    ok = ok && loop_node->line == 5;
    ok = ok && contains_text(loop_node->summary_label, "stmt: DUM");
    ok = ok && contains_text(loop_node->summary_label, "rituale: loop_contexto");
    ok = ok && contains_text(loop_node->summary_label, "line: 5");
    ok = ok && strcmp(loop_node->source_excerpt, "DUM i < 3") == 0;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

static int test_call_edge_contexts(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;
    int found_main_helper = 0;
    int found_self_loop = 0;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom(
        "tests/integration/sigilo_context_call_edge_14ta2.cct",
        "tests/integration/sigilo_context_call_edge_14ta2.cct",
        &model,
        &geom
    );
    for (u32 i = 0; ok && i < geom.link_count; i++) {
        cct_sigilo_link_t *link = &geom.links[i];
        if (link->kind != SGL_CONIURA) continue;
        if (contains_text(link->summary_label, "main -> helper") &&
            contains_text(link->summary_label, "calls: 2") &&
            strcmp(link->source_excerpt, "main -> helper") == 0) {
            found_main_helper = 1;
        }
        if (contains_text(link->summary_label, "factorial -> factorial") &&
            contains_text(link->summary_label, "calls: 1")) {
            found_self_loop = 1;
        }
    }
    ok = ok && found_main_helper;
    ok = ok && found_self_loop;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

static int test_missing_source_fallback(void) {
    cct_sigilo_model_t model;
    cct_sigilo_geom_t geom;
    int ok;

    memset(&model, 0, sizeof(model));
    memset(&geom, 0, sizeof(geom));
    ok = build_model_and_geom(
        "tests/integration/sigilo_context_rituale_14ta2.cct",
        "tests/integration/nao_existe_sigilo_context_14ta2.cct",
        &model,
        &geom
    );
    ok = ok && strcmp(model.rituals[0].signature_excerpt, "RITUALE normalizar_numero") == 0;
    ok = ok && geom.node_count >= 1;
    ok = ok && geom.nodes[0].source_excerpt != NULL;
    ok = ok && strcmp(geom.nodes[0].source_excerpt, "RITUALE normalizar_numero") == 0;
    sg_geom_dispose(&geom);
    sg_free_model(&model);
    return ok;
}

int main(void) {
    if (!test_rituale_context()) return 1;
    if (!test_loop_node_context()) return 2;
    if (!test_call_edge_contexts()) return 3;
    if (!test_missing_source_fallback()) return 4;
    return 0;
}
