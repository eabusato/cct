/*
 * CCT — Sigil Diff Tests (FASE 13A.2)
 */

#include "../../src/sigilo/sigil_parse.h"
#include "../../src/sigilo/sigil_diff.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int ensure_parent_dir(const char *path) {
    if (!path) return 0;
    const char *slash = strrchr(path, '/');
    if (!slash) return 1;

    char dir[1024];
    size_t len = (size_t)(slash - path);
    if (len == 0 || len >= sizeof(dir)) return 0;

    memcpy(dir, path, len);
    dir[len] = '\0';

    for (char *p = dir + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(dir, 0777) != 0 && errno != EEXIST) return 0;
        *p = '/';
    }

    return mkdir(dir, 0777) == 0 || errno == EEXIST;
}

static int write_text_file(const char *path, const char *content) {
    if (!ensure_parent_dir(path)) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t n = strlen(content);
    size_t w = fwrite(content, 1, n, f);
    fclose(f);
    return w == n;
}

static int file_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long n = ftell(f);
    if (n < 0) {
        fclose(f);
        return 0;
    }
    rewind(f);

    char *buf = (char*)calloc(1, (size_t)n + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }
    size_t read_n = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (read_n != (size_t)n) {
        free(buf);
        return 0;
    }

    int ok = strstr(buf, needle) != NULL;
    free(buf);
    return ok;
}

static int parse_tolerant(const char *path, const char *content, cct_sigil_document_t *out_doc) {
    if (!write_text_file(path, content)) return 0;
    return cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, out_doc);
}

static int test_diff_none(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_none_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_none_b.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "[totals]\n"
        "rituale = 2\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content, &a)) return 0;
    if (!parse_tolerant(path_b, content, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count == 0 && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_NONE;

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

static int test_diff_order_invariant(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_order_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_order_b.sigil";
    const char *content_a =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "[totals]\n"
        "rituale = 2\n"
        "ordo = 1\n";
    const char *content_b =
        "sigilo_scope = local\n"
        "format = cct.sigil.v1\n"
        "semantic_hash = abcdef0123456789\n"
        "[totals]\n"
        "ordo = 1\n"
        "rituale = 2\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content_a, &a)) return 0;
    if (!parse_tolerant(path_b, content_b, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count == 0 && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_NONE;

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

static int test_diff_informational_totals_change(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_info_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_info_b.sigil";
    const char *content_a =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "[totals]\n"
        "rituale = 2\n";
    const char *content_b =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "[totals]\n"
        "rituale = 3\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content_a, &a)) return 0;
    if (!parse_tolerant(path_b, content_b, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count > 0;
    ok = ok && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL;
    ok = ok && diff.informational_count >= 1;

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

static int test_diff_review_required_hash_change(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_review_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_review_b.sigil";
    const char *content_a =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n";
    const char *content_b =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = 1111111111111111\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content_a, &a)) return 0;
    if (!parse_tolerant(path_b, content_b, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count > 0;
    ok = ok && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;
    ok = ok && diff.review_required_count >= 1;

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

static int test_diff_behavioral_risk_and_renderers(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_risk_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_risk_b.sigil";
    const char *content_a =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n";
    const char *content_b =
        "format = cct.sigil.v1\n"
        "sigilo_scope = system\n"
        "system_hash = abcdef0123456789\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content_a, &a)) return 0;
    if (!parse_tolerant(path_b, content_b, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count > 0;
    ok = ok && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK;

    FILE *f_struct = fopen("tests/.tmp/cct_sigil_diff_structured.out", "wb");
    if (!f_struct) ok = 0;
    if (f_struct) {
        ok = ok && cct_sigil_diff_render_structured(f_struct, &diff);
        fclose(f_struct);
    }

    FILE *f_text = fopen("tests/.tmp/cct_sigil_diff_text.out", "wb");
    if (!f_text) ok = 0;
    if (f_text) {
        ok = ok && cct_sigil_diff_render_text(f_text, &diff, false);
        fclose(f_text);
    }

    ok = ok && file_contains("tests/.tmp/cct_sigil_diff_structured.out", "highest_severity = behavioral-risk");
    ok = ok && file_contains("tests/.tmp/cct_sigil_diff_text.out", "kind=changed");
    ok = ok && file_contains("tests/.tmp/cct_sigil_diff_text.out", "key=sigilo_scope");

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

static int test_diff_13c2_analytical_blocks_severity(void) {
    const char *path_a = "tests/.tmp/cct_sigil_diff_13c2_a.sigil";
    const char *path_b = "tests/.tmp/cct_sigil_diff_13c2_b.sigil";
    const char *content_a =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "\n"
        "[analysis_summary]\n"
        "control_flow_pressure = 2\n"
        "\n"
        "[compatibility_hints]\n"
        "schema_contract = cct.sigil.v1\n";
    const char *content_b =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "\n"
        "[analysis_summary]\n"
        "control_flow_pressure = 5\n"
        "\n"
        "[compatibility_hints]\n"
        "schema_contract = cct.sigil.v2\n";

    cct_sigil_document_t a;
    cct_sigil_document_t b;
    if (!parse_tolerant(path_a, content_a, &a)) return 0;
    if (!parse_tolerant(path_b, content_b, &b)) {
        cct_sigil_document_dispose(&a);
        return 0;
    }

    cct_sigil_diff_result_t diff;
    int ok = cct_sigil_diff_documents(&a, &b, &diff);
    ok = ok && diff.count == 2;
    ok = ok && diff.review_required_count == 1;
    ok = ok && diff.informational_count == 1;
    ok = ok && diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&a);
    cct_sigil_document_dispose(&b);
    return ok;
}

int main(void) {
    if (!test_diff_none()) {
        fprintf(stderr, "test_diff_none failed\n");
        return 1;
    }
    if (!test_diff_order_invariant()) {
        fprintf(stderr, "test_diff_order_invariant failed\n");
        return 1;
    }
    if (!test_diff_informational_totals_change()) {
        fprintf(stderr, "test_diff_informational_totals_change failed\n");
        return 1;
    }
    if (!test_diff_review_required_hash_change()) {
        fprintf(stderr, "test_diff_review_required_hash_change failed\n");
        return 1;
    }
    if (!test_diff_behavioral_risk_and_renderers()) {
        fprintf(stderr, "test_diff_behavioral_risk_and_renderers failed\n");
        return 1;
    }
    if (!test_diff_13c2_analytical_blocks_severity()) {
        fprintf(stderr, "test_diff_13c2_analytical_blocks_severity failed\n");
        return 1;
    }

    printf("test_sigil_diff: ok\n");
    return 0;
}
