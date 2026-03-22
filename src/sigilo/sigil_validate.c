/*
 * CCT — Clavicula Turing
 * Sigilo Validator Implementation
 *
 * FASE 14A: Sigilo inspection, validation, and diff tooling
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "sigil_validate.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static bool sv_mode_is_strict_contract(cct_sigil_parse_mode_t mode) {
    return mode == CCT_SIGIL_PARSE_MODE_STRICT || mode == CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT;
}

static bool sv_push_issue(
    cct_sigil_validation_result_t *out,
    cct_sigil_diag_level_t level,
    cct_sigil_diag_kind_t kind,
    u32 line,
    u32 column,
    const char *message
) {
    if (!out || !message) return false;
    if (out->count >= CCT_SIGIL_VALIDATE_MAX_ISSUES) return false;
    cct_sigil_validation_issue_t *it = &out->items[out->count++];
    it->level = level;
    it->kind = kind;
    it->line = line;
    it->column = column;
    snprintf(it->message, sizeof(it->message), "%s", message);
    return true;
}

static bool sv_parse_schema_version(const char *format, unsigned *out_version) {
    if (!format || !out_version) return false;
    const char *prefix = "cct.sigil.v";
    size_t n = strlen(prefix);
    if (strncmp(format, prefix, n) != 0) return false;
    const char *digits = format + n;
    if (*digits == '\0') return false;
    unsigned v = 0;
    while (*digits) {
        if (*digits < '0' || *digits > '9') return false;
        v = (v * 10u) + (unsigned)(*digits - '0');
        digits++;
    }
    *out_version = v;
    return true;
}

static bool sv_is_hash_hex_16(const char *s) {
    if (!s || s[0] == '\0') return false;
    if (strlen(s) != 16) return false;
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        bool is_hex = (c >= '0' && c <= '9') ||
                      (c >= 'a' && c <= 'f') ||
                      (c >= 'A' && c <= 'F');
        if (!is_hex) return false;
    }
    return true;
}

static bool sv_parse_u64(const char *value, u64 *out) {
    if (!value || value[0] == '\0' || !out) return false;
    u64 acc = 0;
    for (size_t i = 0; value[i] != '\0'; i++) {
        char c = value[i];
        if (c < '0' || c > '9') return false;
        acc = (acc * 10ULL) + (u64)(c - '0');
    }
    *out = acc;
    return true;
}

static const cct_sigil_kv_t* sv_find_entry(const cct_sigil_document_t *doc, const char *section, const char *key) {
    if (!doc || !key) return NULL;
    const char *s = section ? section : "";
    const cct_sigil_kv_t *found = NULL;
    for (size_t i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        if (strcmp(kv->section ? kv->section : "", s) != 0) continue;
        if (strcmp(kv->key ? kv->key : "", key) != 0) continue;
        found = kv;
    }
    return found;
}

static void sv_validate_required_core(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    cct_sigil_validation_result_t *out
) {
    bool strict = sv_mode_is_strict_contract(mode);
    if (!doc->format || doc->format[0] == '\0') {
        (void)sv_push_issue(out,
                            CCT_SIGIL_DIAG_ERROR,
                            CCT_SIGIL_PARSE_MISSING_REQUIRED,
                            0,
                            0,
                            "missing required field: format (action: set 'format = cct.sigil.v1')");
        return;
    }
    if (strcmp(doc->format, "cct.sigil.v1") != 0) {
        unsigned schema_version = 0;
        if (sv_parse_schema_version(doc->format, &schema_version) && schema_version > 1 && !strict) {
            (void)sv_push_issue(out,
                                CCT_SIGIL_DIAG_WARNING,
                                CCT_SIGIL_PARSE_SCHEMA_MISMATCH,
                                0,
                                0,
                                "unsupported higher sigil format; applying v1 compatibility fallback (action: migrate consumer or use strict-contract to block)");
        } else {
            (void)sv_push_issue(out,
                                CCT_SIGIL_DIAG_ERROR,
                                CCT_SIGIL_PARSE_SCHEMA_MISMATCH,
                                0,
                                0,
                                "unsupported sigil format (expected cct.sigil.v1) (action: regenerate artifact with canonical producer)");
        }
    }

    if (!doc->sigilo_scope || doc->sigilo_scope[0] == '\0') {
        (void)sv_push_issue(out,
                            CCT_SIGIL_DIAG_ERROR,
                            CCT_SIGIL_PARSE_MISSING_REQUIRED,
                            0,
                            0,
                            "missing required field: sigilo_scope (action: set 'sigilo_scope = local|system')");
        return;
    }
    if (strcmp(doc->sigilo_scope, "local") != 0 && strcmp(doc->sigilo_scope, "system") != 0) {
        (void)sv_push_issue(out,
                            CCT_SIGIL_DIAG_ERROR,
                            CCT_SIGIL_PARSE_TYPE,
                            0,
                            0,
                            "invalid sigilo_scope (expected local|system) (action: normalize sigilo_scope)");
        return;
    }

    if (strcmp(doc->sigilo_scope, "local") == 0) {
        cct_sigil_diag_level_t lvl = strict ? CCT_SIGIL_DIAG_ERROR : CCT_SIGIL_DIAG_WARNING;
        if (!sv_is_hash_hex_16(doc->semantic_hash)) {
            (void)sv_push_issue(out,
                                lvl,
                                CCT_SIGIL_PARSE_MISSING_REQUIRED,
                                0,
                                0,
                                "local scope requires valid 16-hex semantic_hash (action: regenerate local .sigil)");
        }
    } else {
        cct_sigil_diag_level_t lvl = strict ? CCT_SIGIL_DIAG_ERROR : CCT_SIGIL_DIAG_WARNING;
        if (!sv_is_hash_hex_16(doc->system_hash)) {
            (void)sv_push_issue(out,
                                lvl,
                                CCT_SIGIL_PARSE_MISSING_REQUIRED,
                                0,
                                0,
                                "system scope requires valid 16-hex system_hash (action: regenerate system .sigil)");
        }
    }
}

static void sv_validate_numeric_key_if_present(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    cct_sigil_validation_result_t *out,
    const char *section,
    const char *key
) {
    const cct_sigil_kv_t *kv = sv_find_entry(doc, section, key);
    if (!kv) return;
    u64 parsed = 0;
    if (sv_parse_u64(kv->value, &parsed)) return;
    cct_sigil_diag_level_t lvl = sv_mode_is_strict_contract(mode) ? CCT_SIGIL_DIAG_ERROR : CCT_SIGIL_DIAG_WARNING;
    char msg[220];
    snprintf(msg,
             sizeof(msg),
             "invalid numeric value for %s%s%s%s%s (action: use non-negative integer literal)",
             (section && section[0]) ? "[" : "",
             (section && section[0]) ? section : "",
             (section && section[0]) ? "]." : "",
             key ? key : "<key>",
             (section && section[0]) ? "" : "");
    (void)sv_push_issue(out, lvl, CCT_SIGIL_PARSE_TYPE, kv->line, 1, msg);
}

static void sv_validate_consistency(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    cct_sigil_validation_result_t *out
) {
    cct_sigil_diag_level_t lvl = sv_mode_is_strict_contract(mode) ? CCT_SIGIL_DIAG_ERROR : CCT_SIGIL_DIAG_WARNING;

    const cct_sigil_kv_t *summary_scope = sv_find_entry(doc, "analysis_summary", "scope");
    if (summary_scope && doc->sigilo_scope && strcmp(summary_scope->value, doc->sigilo_scope) != 0) {
        (void)sv_push_issue(out,
                            lvl,
                            CCT_SIGIL_PARSE_TYPE,
                            summary_scope->line,
                            1,
                            "inconsistent scope between [analysis_summary].scope and sigilo_scope (action: keep both aligned)");
    }

    const cct_sigil_kv_t *top_module_count = sv_find_entry(doc, "", "module_count");
    const cct_sigil_kv_t *summary_module_count = sv_find_entry(doc, "module_structural_summary", "module_count");
    if (top_module_count && summary_module_count) {
        u64 top = 0;
        u64 sec = 0;
        if (sv_parse_u64(top_module_count->value, &top) && sv_parse_u64(summary_module_count->value, &sec) && top != sec) {
            (void)sv_push_issue(out,
                                lvl,
                                CCT_SIGIL_PARSE_TYPE,
                                summary_module_count->line,
                                1,
                                "inconsistent module_count between top-level and [module_structural_summary] (action: regenerate artifact to normalize metadata)");
        }
    }

    const cct_sigil_kv_t *top_mod_res = sv_find_entry(doc, "", "module_resolution_status");
    const cct_sigil_kv_t *sec_mod_res = sv_find_entry(doc, "module_structural_summary", "module_resolution_status");
    if (top_mod_res && sec_mod_res && strcmp(top_mod_res->value, sec_mod_res->value) != 0) {
        (void)sv_push_issue(out,
                            lvl,
                            CCT_SIGIL_PARSE_TYPE,
                            sec_mod_res->line,
                            1,
                            "inconsistent module_resolution_status between top-level and [module_structural_summary] (action: regenerate artifact to keep consistency)");
    }
}

bool cct_sigil_validate_collect(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    cct_sigil_validation_result_t *out_result
) {
    if (!doc || !out_result) return false;
    memset(out_result, 0, sizeof(*out_result));

    sv_validate_required_core(doc, mode, out_result);

    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "module_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "import_edge_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "import_edges");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "cross_module_call_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "cross_module_calls");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "cross_module_type_ref_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "cross_module_type_refs");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "public_symbol_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "exported_symbol_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "", "internal_symbol_count");
    sv_validate_numeric_key_if_present(doc, mode, out_result, "module_structural_summary", "module_count");

    sv_validate_consistency(doc, mode, out_result);
    return true;
}
