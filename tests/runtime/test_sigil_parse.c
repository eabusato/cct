/*
 * CCT — Sigil Parse Tests (FASE 13A.1)
 */

#include "../../src/sigilo/sigil_parse.h"

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

static int has_diag_kind_level(const cct_sigil_document_t *doc, cct_sigil_diag_kind_t kind, int level_or_any) {
    if (!doc) return 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        const cct_sigil_diag_t *d = &doc->diagnostics.items[i];
        if (d->kind != kind) continue;
        if (level_or_any < 0 || (int)d->level == level_or_any) return 1;
    }
    return 0;
}

static int has_diag_message_fragment(const cct_sigil_document_t *doc, const char *fragment) {
    if (!doc || !fragment) return 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        const cct_sigil_diag_t *d = &doc->diagnostics.items[i];
        if (d->message && strstr(d->message, fragment)) return 1;
    }
    return 0;
}

static int test_valid_local(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_valid_local.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "visual_engine = sigillum_v2_1\n"
        "semantic_hash = abcdef0123456789\n"
        "\n"
        "[totals]\n"
        "rituale = 2\n";

    if (!write_text_file(path, content)) return 0;
    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;
    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && doc.format && strcmp(doc.format, "cct.sigil.v1") == 0;
    ok = ok && doc.sigilo_scope && strcmp(doc.sigilo_scope, "local") == 0;
    ok = ok && doc.semantic_hash && strcmp(doc.semantic_hash, "abcdef0123456789") == 0;
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_unknown_tolerant(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_unknown_tolerant.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "new_future_field = x\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_UNKNOWN_FIELD, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_duplicate_tolerant_warns_and_keeps_last_value(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_duplicate_tolerant.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = 1111111111111111\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_DUPLICATE_KEY, CCT_SIGIL_DIAG_WARNING);
    ok = ok && doc.semantic_hash && strcmp(doc.semantic_hash, "abcdef0123456789") == 0;
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_duplicate_strict_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_duplicate_strict.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = 1111111111111111\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    int has_errors = cct_sigil_document_has_errors(&doc);
    int has_dup_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_DUPLICATE_KEY, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_errors && has_dup_error;
}

static int test_invalid_scope_type_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_invalid_scope.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = universal\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    int has_type_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_TYPE, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_type_error;
}

static int test_system_hash_tolerant_warns(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_system_hash_warn.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = system\n"
        "system_hash = not_hex\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_MISSING_REQUIRED, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_schema_mismatch_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_schema_mismatch.sigil";
    const char *content =
        "format = cct.sigil.v2\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    int has_schema_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_SCHEMA_MISMATCH, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_schema_error;
}

static int test_deprecated_field_warns_and_is_accepted(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_deprecated_field.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "sigilo_style = network\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_DEPRECATED_FIELD, CCT_SIGIL_DIAG_WARNING);
    ok = ok && doc.visual_engine && strcmp(doc.visual_engine, "network") == 0;
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_unknown_top_level_in_strict_is_warning_only(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_unknown_strict_warning.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "future_schema_marker = canary\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_UNKNOWN_FIELD, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_syntax_error_missing_equals_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_syntax_error.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    int has_syntax_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_SYNTAX, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_syntax_error;
}

static int test_missing_required_strict(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_missing_required_strict.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    int has_errors = cct_sigil_document_has_errors(&doc);
    int has_missing_required = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_MISSING_REQUIRED, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_errors && has_missing_required;
}

static int test_13c2_analytical_sections_are_recognized(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c2_sections.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "\n"
        "[analysis_summary]\n"
        "scope = local\n"
        "\n"
        "[diff_fingerprint_context]\n"
        "scope_anchor = local\n"
        "\n"
        "[module_structural_summary]\n"
        "module_count = 1\n"
        "\n"
        "[compatibility_hints]\n"
        "schema_contract = cct.sigil.v1\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && doc.has_analysis_summary;
    ok = ok && doc.has_diff_fingerprint_context;
    ok = ok && doc.has_module_structural_summary;
    ok = ok && doc.has_compatibility_hints;
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_13c3_current_profile_fallbacks_higher_schema(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c3_current_fallback.sigil";
    const char *content =
        "format = cct.sigil.v2\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_SCHEMA_MISMATCH, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_13c3_legacy_profile_fallbacks_higher_schema(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c3_legacy_fallback.sigil";
    const char *content =
        "format = cct.sigil.v3\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "future_field = additive\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_LEGACY_TOLERANT, &doc);
    if (!ok) return 0;

    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_SCHEMA_MISMATCH, CCT_SIGIL_DIAG_WARNING);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_UNKNOWN_FIELD, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

static int test_13c3_strict_contract_rejects_higher_schema(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c3_strict_contract.sigil";
    const char *content =
        "format = cct.sigil.v2\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT, &doc);
    int has_schema_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_SCHEMA_MISMATCH, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_schema_error;
}

static int test_13c4_invalid_numeric_type_strict_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c4_invalid_numeric.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "module_count = many\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT, &doc);
    int has_type_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_TYPE, CCT_SIGIL_DIAG_ERROR);
    int has_action_hint = has_diag_message_fragment(&doc, "action:");
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_type_error && has_action_hint;
}

static int test_13c4_consistency_strict_fails(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c4_consistency_strict.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "module_count = 2\n"
        "\n"
        "[module_structural_summary]\n"
        "module_count = 1\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT, &doc);
    int has_type_error = has_diag_kind_level(&doc, CCT_SIGIL_PARSE_TYPE, CCT_SIGIL_DIAG_ERROR);
    cct_sigil_document_dispose(&doc);
    return (!ok) && has_type_error;
}

static int test_13c4_consistency_tolerant_warns(void) {
    const char *path = "tests/.tmp/cct_sigil_parse_13c4_consistency_tolerant.sigil";
    const char *content =
        "format = cct.sigil.v1\n"
        "sigilo_scope = local\n"
        "semantic_hash = abcdef0123456789\n"
        "module_count = 2\n"
        "\n"
        "[module_structural_summary]\n"
        "module_count = 1\n";
    if (!write_text_file(path, content)) return 0;

    cct_sigil_document_t doc;
    int ok = cct_sigil_parse_file(path, CCT_SIGIL_PARSE_MODE_TOLERANT, &doc);
    if (!ok) return 0;
    ok = !cct_sigil_document_has_errors(&doc);
    ok = ok && has_diag_kind_level(&doc, CCT_SIGIL_PARSE_TYPE, CCT_SIGIL_DIAG_WARNING);
    cct_sigil_document_dispose(&doc);
    return ok;
}

int main(void) {
    if (!test_valid_local()) {
        fprintf(stderr, "test_valid_local failed\n");
        return 1;
    }
    if (!test_unknown_tolerant()) {
        fprintf(stderr, "test_unknown_tolerant failed\n");
        return 1;
    }
    if (!test_duplicate_tolerant_warns_and_keeps_last_value()) {
        fprintf(stderr, "test_duplicate_tolerant_warns_and_keeps_last_value failed\n");
        return 1;
    }
    if (!test_duplicate_strict_fails()) {
        fprintf(stderr, "test_duplicate_strict_fails failed\n");
        return 1;
    }
    if (!test_invalid_scope_type_fails()) {
        fprintf(stderr, "test_invalid_scope_type_fails failed\n");
        return 1;
    }
    if (!test_system_hash_tolerant_warns()) {
        fprintf(stderr, "test_system_hash_tolerant_warns failed\n");
        return 1;
    }
    if (!test_schema_mismatch_fails()) {
        fprintf(stderr, "test_schema_mismatch_fails failed\n");
        return 1;
    }
    if (!test_deprecated_field_warns_and_is_accepted()) {
        fprintf(stderr, "test_deprecated_field_warns_and_is_accepted failed\n");
        return 1;
    }
    if (!test_unknown_top_level_in_strict_is_warning_only()) {
        fprintf(stderr, "test_unknown_top_level_in_strict_is_warning_only failed\n");
        return 1;
    }
    if (!test_syntax_error_missing_equals_fails()) {
        fprintf(stderr, "test_syntax_error_missing_equals_fails failed\n");
        return 1;
    }
    if (!test_missing_required_strict()) {
        fprintf(stderr, "test_missing_required_strict failed\n");
        return 1;
    }
    if (!test_13c2_analytical_sections_are_recognized()) {
        fprintf(stderr, "test_13c2_analytical_sections_are_recognized failed\n");
        return 1;
    }
    if (!test_13c3_current_profile_fallbacks_higher_schema()) {
        fprintf(stderr, "test_13c3_current_profile_fallbacks_higher_schema failed\n");
        return 1;
    }
    if (!test_13c3_legacy_profile_fallbacks_higher_schema()) {
        fprintf(stderr, "test_13c3_legacy_profile_fallbacks_higher_schema failed\n");
        return 1;
    }
    if (!test_13c3_strict_contract_rejects_higher_schema()) {
        fprintf(stderr, "test_13c3_strict_contract_rejects_higher_schema failed\n");
        return 1;
    }
    if (!test_13c4_invalid_numeric_type_strict_fails()) {
        fprintf(stderr, "test_13c4_invalid_numeric_type_strict_fails failed\n");
        return 1;
    }
    if (!test_13c4_consistency_strict_fails()) {
        fprintf(stderr, "test_13c4_consistency_strict_fails failed\n");
        return 1;
    }
    if (!test_13c4_consistency_tolerant_warns()) {
        fprintf(stderr, "test_13c4_consistency_tolerant_warns failed\n");
        return 1;
    }

    printf("test_sigil_parse: ok\n");
    return 0;
}
