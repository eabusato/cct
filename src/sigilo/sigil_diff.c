/*
 * CCT — Clavicula Turing
 * Sigilo Diff Implementation
 *
 * FASE 14A: Sigilo inspection, validation, and diff tooling
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "sigil_diff.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *section;
    const char *key;
    const char *value;
} cct_sigil_entry_ref_t;

static char* sd_strdup(const char *s) {
    if (!s) return NULL;
    return strdup(s);
}

static int sd_ref_cmp(const void *a, const void *b) {
    const cct_sigil_entry_ref_t *ra = (const cct_sigil_entry_ref_t*)a;
    const cct_sigil_entry_ref_t *rb = (const cct_sigil_entry_ref_t*)b;
    int sec = strcmp(ra->section ? ra->section : "", rb->section ? rb->section : "");
    if (sec != 0) return sec;
    int key = strcmp(ra->key ? ra->key : "", rb->key ? rb->key : "");
    if (key != 0) return key;
    return strcmp(ra->value ? ra->value : "", rb->value ? rb->value : "");
}

const char* cct_sigil_diff_severity_str(cct_sigil_diff_severity_t severity) {
    switch (severity) {
        case CCT_SIGIL_DIFF_SEVERITY_NONE: return "none";
        case CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL: return "informational";
        case CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED: return "review-required";
        case CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK: return "behavioral-risk";
        default: return "unknown";
    }
}

const char* cct_sigil_diff_kind_str(cct_sigil_diff_kind_t kind) {
    switch (kind) {
        case CCT_SIGIL_DIFF_ADDED: return "added";
        case CCT_SIGIL_DIFF_REMOVED: return "removed";
        case CCT_SIGIL_DIFF_CHANGED: return "changed";
        default: return "unknown";
    }
}

static cct_sigil_diff_severity_t sd_max_severity(cct_sigil_diff_severity_t a, cct_sigil_diff_severity_t b) {
    return a > b ? a : b;
}

static cct_sigil_diff_severity_t sd_classify_severity(const char *section, const char *key) {
    const char *sec = section ? section : "";
    const char *k = key ? key : "";
    if (strcmp(k, "format") == 0 || strcmp(k, "sigilo_scope") == 0) {
        return CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK;
    }
    if (strcmp(k, "semantic_hash") == 0 || strcmp(k, "system_hash") == 0) {
        return CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;
    }
    if (sec[0] == '\0') {
        return CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;
    }
    if (strcmp(sec, "module_sigils") == 0 || strcmp(sec, "modules") == 0) {
        return CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;
    }
    if (strcmp(sec, "analysis_summary") == 0 ||
        strcmp(sec, "diff_fingerprint_context") == 0 ||
        strcmp(sec, "module_structural_summary") == 0) {
        return CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED;
    }
    if (strcmp(sec, "compatibility_hints") == 0) {
        return CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL;
    }
    return CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL;
}

static bool sd_push_item(
    cct_sigil_diff_result_t *res,
    cct_sigil_diff_kind_t kind,
    cct_sigil_diff_severity_t severity,
    const char *section,
    const char *key,
    const char *left_value,
    const char *right_value
) {
    if (!res || !key) return false;
    if (res->count >= res->capacity) {
        size_t next = res->capacity == 0 ? 16 : res->capacity * 2;
        cct_sigil_diff_item_t *grown = (cct_sigil_diff_item_t*)realloc(res->items, next * sizeof(*grown));
        if (!grown) return false;
        res->items = grown;
        res->capacity = next;
    }
    cct_sigil_diff_item_t *it = &res->items[res->count++];
    it->kind = kind;
    it->severity = severity;
    it->section = sd_strdup(section ? section : "");
    it->key = sd_strdup(key);
    it->left_value = sd_strdup(left_value);
    it->right_value = sd_strdup(right_value);
    if (!it->section || !it->key || (left_value && !it->left_value) || (right_value && !it->right_value)) return false;

    if (severity == CCT_SIGIL_DIFF_SEVERITY_INFORMATIONAL) res->informational_count++;
    else if (severity == CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) res->review_required_count++;
    else if (severity == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK) res->behavioral_risk_count++;

    res->highest_severity = sd_max_severity(res->highest_severity, severity);
    return true;
}

static cct_sigil_entry_ref_t* sd_build_sorted_refs(const cct_sigil_document_t *doc) {
    if (!doc || doc->entry_count == 0) return NULL;
    cct_sigil_entry_ref_t *refs = (cct_sigil_entry_ref_t*)calloc(doc->entry_count, sizeof(*refs));
    if (!refs) return NULL;
    for (size_t i = 0; i < doc->entry_count; i++) {
        refs[i].section = doc->entries[i].section ? doc->entries[i].section : "";
        refs[i].key = doc->entries[i].key ? doc->entries[i].key : "";
        refs[i].value = doc->entries[i].value ? doc->entries[i].value : "";
    }
    qsort(refs, doc->entry_count, sizeof(*refs), sd_ref_cmp);
    return refs;
}

static int sd_key_cmp(const cct_sigil_entry_ref_t *a, const cct_sigil_entry_ref_t *b) {
    int sec = strcmp(a->section ? a->section : "", b->section ? b->section : "");
    if (sec != 0) return sec;
    return strcmp(a->key ? a->key : "", b->key ? b->key : "");
}

bool cct_sigil_diff_documents(
    const cct_sigil_document_t *left,
    const cct_sigil_document_t *right,
    cct_sigil_diff_result_t *out_result
) {
    if (!left || !right || !out_result) return false;
    memset(out_result, 0, sizeof(*out_result));

    cct_sigil_entry_ref_t *lrefs = sd_build_sorted_refs(left);
    cct_sigil_entry_ref_t *rrefs = sd_build_sorted_refs(right);
    if ((left->entry_count > 0 && !lrefs) || (right->entry_count > 0 && !rrefs)) {
        free(lrefs);
        free(rrefs);
        return false;
    }

    size_t li = 0;
    size_t ri = 0;
    bool ok = true;
    while (li < left->entry_count || ri < right->entry_count) {
        if (li >= left->entry_count) {
            cct_sigil_diff_severity_t sev = sd_classify_severity(rrefs[ri].section, rrefs[ri].key);
            ok = ok && sd_push_item(out_result, CCT_SIGIL_DIFF_ADDED, sev, rrefs[ri].section, rrefs[ri].key, NULL, rrefs[ri].value);
            ri++;
            continue;
        }
        if (ri >= right->entry_count) {
            cct_sigil_diff_severity_t sev = sd_classify_severity(lrefs[li].section, lrefs[li].key);
            ok = ok && sd_push_item(out_result, CCT_SIGIL_DIFF_REMOVED, sev, lrefs[li].section, lrefs[li].key, lrefs[li].value, NULL);
            li++;
            continue;
        }

        int key_cmp = sd_key_cmp(&lrefs[li], &rrefs[ri]);
        if (key_cmp == 0) {
            if (strcmp(lrefs[li].value, rrefs[ri].value) != 0) {
                cct_sigil_diff_severity_t sev = sd_classify_severity(lrefs[li].section, lrefs[li].key);
                ok = ok && sd_push_item(out_result, CCT_SIGIL_DIFF_CHANGED, sev, lrefs[li].section, lrefs[li].key, lrefs[li].value, rrefs[ri].value);
            }
            li++;
            ri++;
            continue;
        }
        if (key_cmp < 0) {
            cct_sigil_diff_severity_t sev = sd_classify_severity(lrefs[li].section, lrefs[li].key);
            ok = ok && sd_push_item(out_result, CCT_SIGIL_DIFF_REMOVED, sev, lrefs[li].section, lrefs[li].key, lrefs[li].value, NULL);
            li++;
        } else {
            cct_sigil_diff_severity_t sev = sd_classify_severity(rrefs[ri].section, rrefs[ri].key);
            ok = ok && sd_push_item(out_result, CCT_SIGIL_DIFF_ADDED, sev, rrefs[ri].section, rrefs[ri].key, NULL, rrefs[ri].value);
            ri++;
        }
    }

    free(lrefs);
    free(rrefs);
    if (!ok) {
        cct_sigil_diff_result_dispose(out_result);
        return false;
    }
    return true;
}

void cct_sigil_diff_result_dispose(cct_sigil_diff_result_t *result) {
    if (!result) return;
    for (size_t i = 0; i < result->count; i++) {
        free(result->items[i].section);
        free(result->items[i].key);
        free(result->items[i].left_value);
        free(result->items[i].right_value);
    }
    free(result->items);
    memset(result, 0, sizeof(*result));
}

bool cct_sigil_diff_render_text(
    FILE *out,
    const cct_sigil_diff_result_t *result,
    bool summary_only
) {
    if (!out || !result) return false;
    fprintf(out, "sigil_diff.summary highest=%s total=%zu informational=%zu review_required=%zu behavioral_risk=%zu\n",
            cct_sigil_diff_severity_str(result->highest_severity),
            result->count,
            result->informational_count,
            result->review_required_count,
            result->behavioral_risk_count);

    if (summary_only) return true;
    for (size_t i = 0; i < result->count; i++) {
        const cct_sigil_diff_item_t *it = &result->items[i];
        fprintf(out, "[%zu] kind=%s severity=%s section=%s key=%s",
                i,
                cct_sigil_diff_kind_str(it->kind),
                cct_sigil_diff_severity_str(it->severity),
                it->section ? it->section : "",
                it->key ? it->key : "");
        if (it->left_value) fprintf(out, " left=%s", it->left_value);
        if (it->right_value) fprintf(out, " right=%s", it->right_value);
        fputc('\n', out);
    }
    return true;
}

bool cct_sigil_diff_render_structured(
    FILE *out,
    const cct_sigil_diff_result_t *result
) {
    if (!out || !result) return false;
    fprintf(out, "format = cct.sigil.diff.v1\n");
    fprintf(out, "highest_severity = %s\n", cct_sigil_diff_severity_str(result->highest_severity));
    fprintf(out, "total = %zu\n", result->count);
    fprintf(out, "informational = %zu\n", result->informational_count);
    fprintf(out, "review_required = %zu\n", result->review_required_count);
    fprintf(out, "behavioral_risk = %zu\n", result->behavioral_risk_count);
    for (size_t i = 0; i < result->count; i++) {
        const cct_sigil_diff_item_t *it = &result->items[i];
        fprintf(out, "\n[item.%zu]\n", i);
        fprintf(out, "kind = %s\n", cct_sigil_diff_kind_str(it->kind));
        fprintf(out, "severity = %s\n", cct_sigil_diff_severity_str(it->severity));
        fprintf(out, "section = %s\n", it->section ? it->section : "");
        fprintf(out, "key = %s\n", it->key ? it->key : "");
        if (it->left_value) fprintf(out, "left = %s\n", it->left_value);
        if (it->right_value) fprintf(out, "right = %s\n", it->right_value);
    }
    return true;
}
