/*
 * CCT — Clavicula Turing
 * Sigilo Parser Implementation
 *
 * FASE 14A: Sigilo inspection, validation, and diff tooling
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "sigil_parse.h"
#include "sigil_validate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static char* sp_strdup(const char *s) {
    if (!s) return NULL;
    char *out = strdup(s);
    return out;
}

static void sp_trim_inplace(char *s) {
    if (!s) return;
    size_t start = 0;
    size_t len = strlen(s);
    while (start < len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start++;
    }
    size_t end = len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end--;
    }
    if (start > 0) memmove(s, s + start, end - start);
    s[end - start] = '\0';
}

static bool sp_push_diag(
    cct_sigil_diag_list_t *list,
    cct_sigil_diag_level_t level,
    cct_sigil_diag_kind_t kind,
    u32 line,
    u32 column,
    const char *message
) {
    if (!list || !message) return false;
    if (list->count >= list->capacity) {
        size_t next = list->capacity == 0 ? 8 : list->capacity * 2;
        cct_sigil_diag_t *grown = (cct_sigil_diag_t*)realloc(list->items, next * sizeof(*grown));
        if (!grown) return false;
        list->items = grown;
        list->capacity = next;
    }
    cct_sigil_diag_t *d = &list->items[list->count++];
    d->level = level;
    d->kind = kind;
    d->line = line;
    d->column = column;
    d->message = sp_strdup(message);
    return d->message != NULL;
}

static bool sp_push_entry(
    cct_sigil_document_t *doc,
    const char *section,
    const char *key,
    const char *value,
    u32 line
) {
    if (!doc || !key || !value) return false;
    if (doc->entry_count >= doc->entry_capacity) {
        size_t next = doc->entry_capacity == 0 ? 16 : doc->entry_capacity * 2;
        cct_sigil_kv_t *grown = (cct_sigil_kv_t*)realloc(doc->entries, next * sizeof(*grown));
        if (!grown) return false;
        doc->entries = grown;
        doc->entry_capacity = next;
    }
    cct_sigil_kv_t *kv = &doc->entries[doc->entry_count++];
    kv->section = sp_strdup(section ? section : "");
    kv->key = sp_strdup(key);
    kv->value = sp_strdup(value);
    kv->line = line;
    return kv->section && kv->key && kv->value;
}

static bool sp_is_known_top_level_key(const char *key) {
    return key &&
           (strcmp(key, "format") == 0 ||
            strcmp(key, "sigilo_scope") == 0 ||
            strcmp(key, "visual_engine") == 0 ||
            strcmp(key, "sigilo_style") == 0 ||
            strcmp(key, "semantic_hash") == 0 ||
            strcmp(key, "system_hash") == 0);
}

static bool sp_is_deprecated_top_level_key(const char *key) {
    return key && strcmp(key, "sigilo_style") == 0;
}

static bool sp_lookup_duplicate_key(const cct_sigil_document_t *doc, const char *section, const char *key) {
    if (!doc || !key) return false;
    const char *s = section ? section : "";
    for (size_t i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        if (strcmp(kv->section ? kv->section : "", s) == 0 && strcmp(kv->key, key) == 0) return true;
    }
    return false;
}

static bool sp_parse_u64(const char *value, u64 *out) {
    if (!value || !out || value[0] == '\0') return false;
    u64 acc = 0;
    for (size_t i = 0; value[i] != '\0'; i++) {
        char c = value[i];
        if (c < '0' || c > '9') return false;
        acc = (acc * 10ULL) + (u64)(c - '0');
    }
    *out = acc;
    return true;
}

static cct_sigil_web_route_t* sp_ensure_web_route(cct_sigil_document_t *doc, u32 section_index) {
    if (!doc) return NULL;
    for (size_t i = 0; i < doc->web_routes_count; i++) {
        if (doc->web_routes[i].section_index == section_index) return &doc->web_routes[i];
    }
    if (doc->web_routes_count >= doc->web_routes_capacity) {
        size_t next = doc->web_routes_capacity == 0 ? 4 : doc->web_routes_capacity * 2;
        cct_sigil_web_route_t *grown = (cct_sigil_web_route_t*)realloc(doc->web_routes, next * sizeof(*grown));
        if (!grown) return NULL;
        memset(grown + doc->web_routes_capacity, 0, (next - doc->web_routes_capacity) * sizeof(*grown));
        doc->web_routes = grown;
        doc->web_routes_capacity = next;
    }
    cct_sigil_web_route_t *route = &doc->web_routes[doc->web_routes_count++];
    memset(route, 0, sizeof(*route));
    route->section_index = section_index;
    return route;
}

static void sp_assign_dup(char **slot, const char *value) {
    if (!slot) return;
    free(*slot);
    *slot = sp_strdup(value ? value : "");
}

static void sp_capture_extended_fields(cct_sigil_document_t *doc) {
    if (!doc) return;
    for (size_t i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        const char *section = kv->section ? kv->section : "";
        const char *key = kv->key ? kv->key : "";
        const char *value = kv->value ? kv->value : "";

        if (strcmp(section, "web_routes") == 0) {
            doc->has_web_routes = true;
            if (strcmp(key, "route_count") == 0) {
                (void)sp_parse_u64(value, &doc->web_route_count);
            } else if (strcmp(key, "group_count") == 0) {
                (void)sp_parse_u64(value, &doc->web_group_count);
            } else if (strcmp(key, "middleware_count") == 0) {
                (void)sp_parse_u64(value, &doc->web_middleware_count);
            } else if (strcmp(key, "web_topology_hash") == 0) {
                sp_assign_dup(&doc->web_topology_hash, value);
            }
            continue;
        }

        if (strcmp(section, "manifest_provenance") == 0) {
            doc->has_manifest_provenance = true;
            if (strcmp(key, "manifest_format") == 0) {
                sp_assign_dup(&doc->manifest_format, value);
            } else if (strcmp(key, "manifest_producer") == 0) {
                sp_assign_dup(&doc->manifest_producer, value);
            } else if (strcmp(key, "manifest_project") == 0) {
                sp_assign_dup(&doc->manifest_project, value);
            }
            continue;
        }

        if (strncmp(section, "web_route.", 10) == 0) {
            u64 idx64 = 0;
            if (!sp_parse_u64(section + 10, &idx64)) continue;
            cct_sigil_web_route_t *route = sp_ensure_web_route(doc, (u32)idx64);
            if (!route) continue;
            if (strcmp(key, "route_id") == 0) sp_assign_dup(&route->route_id, value);
            else if (strcmp(key, "method") == 0) sp_assign_dup(&route->method, value);
            else if (strcmp(key, "path") == 0) sp_assign_dup(&route->path, value);
            else if (strcmp(key, "route_name") == 0) sp_assign_dup(&route->route_name, value);
            else if (strcmp(key, "handler") == 0) sp_assign_dup(&route->handler, value);
            else if (strcmp(key, "module") == 0) sp_assign_dup(&route->module, value);
            else if (strcmp(key, "group") == 0) sp_assign_dup(&route->group, value);
            else if (strcmp(key, "middleware") == 0) sp_assign_dup(&route->middleware, value);
            else if (strcmp(key, "path_params") == 0) sp_assign_dup(&route->path_params, value);
            else if (strcmp(key, "route_hash") == 0) sp_assign_dup(&route->route_hash, value);
            else if (strcmp(key, "source_origin") == 0) sp_assign_dup(&route->source_origin, value);
        }
    }
}

static void sp_capture_known_section(cct_sigil_document_t *doc, const char *section) {
    if (!doc || !section) return;
    if (strcmp(section, "analysis_summary") == 0) {
        doc->has_analysis_summary = true;
    } else if (strcmp(section, "diff_fingerprint_context") == 0) {
        doc->has_diff_fingerprint_context = true;
    } else if (strcmp(section, "module_structural_summary") == 0) {
        doc->has_module_structural_summary = true;
    } else if (strcmp(section, "compatibility_hints") == 0) {
        doc->has_compatibility_hints = true;
    }
}

static void sp_capture_known_field(cct_sigil_document_t *doc, const char *section, const char *key, const char *value) {
    if (!doc || !section || !key || !value) return;
    if (section[0] != '\0') return;
    if (strcmp(key, "format") == 0) {
        free(doc->format);
        doc->format = sp_strdup(value);
    } else if (strcmp(key, "sigilo_scope") == 0) {
        free(doc->sigilo_scope);
        doc->sigilo_scope = sp_strdup(value);
    } else if (strcmp(key, "visual_engine") == 0) {
        free(doc->visual_engine);
        doc->visual_engine = sp_strdup(value);
    } else if (strcmp(key, "sigilo_style") == 0) {
        if (!doc->visual_engine || doc->visual_engine[0] == '\0') {
            free(doc->visual_engine);
            doc->visual_engine = sp_strdup(value);
        }
    } else if (strcmp(key, "semantic_hash") == 0) {
        free(doc->semantic_hash);
        doc->semantic_hash = sp_strdup(value);
    } else if (strcmp(key, "system_hash") == 0) {
        free(doc->system_hash);
        doc->system_hash = sp_strdup(value);
    }
}

static bool sp_load_file(const char *path, char **out_buf, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long n = ftell(f);
    if (n < 0) {
        fclose(f);
        return false;
    }
    rewind(f);
    char *buf = (char*)calloc(1, (size_t)n + 1);
    if (!buf) {
        fclose(f);
        return false;
    }
    size_t read_n = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (read_n != (size_t)n) {
        free(buf);
        return false;
    }
    *out_buf = buf;
    *out_len = (size_t)n;
    return true;
}

static bool sp_mode_is_strict_contract(cct_sigil_parse_mode_t mode) {
    return mode == CCT_SIGIL_PARSE_MODE_STRICT || mode == CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT;
}

const char* cct_sigil_diag_kind_str(cct_sigil_diag_kind_t kind) {
    switch (kind) {
        case CCT_SIGIL_PARSE_OK: return "ok";
        case CCT_SIGIL_PARSE_SYNTAX: return "syntax";
        case CCT_SIGIL_PARSE_TYPE: return "type";
        case CCT_SIGIL_PARSE_MISSING_REQUIRED: return "missing_required";
        case CCT_SIGIL_PARSE_DUPLICATE_KEY: return "duplicate_key";
        case CCT_SIGIL_PARSE_UNKNOWN_FIELD: return "unknown_field";
        case CCT_SIGIL_PARSE_DEPRECATED_FIELD: return "deprecated_field";
        case CCT_SIGIL_PARSE_SCHEMA_MISMATCH: return "schema_mismatch";
        case CCT_SIGIL_PARSE_IO: return "io";
        default: return "unknown";
    }
}

bool cct_sigil_parse_file(
    const char *path,
    cct_sigil_parse_mode_t mode,
    cct_sigil_document_t *out_doc
) {
    if (!path || !out_doc) return false;
    memset(out_doc, 0, sizeof(*out_doc));
    out_doc->mode = mode;
    out_doc->input_path = sp_strdup(path);

    char *buf = NULL;
    size_t len = 0;
    if (!sp_load_file(path, &buf, &len)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "could not read sigil metadata file '%s': %s", path, strerror(errno));
        (void)sp_push_diag(&out_doc->diagnostics, CCT_SIGIL_DIAG_ERROR, CCT_SIGIL_PARSE_IO, 0, 0, msg);
        return false;
    }

    char *cursor = buf;
    char *section = sp_strdup("");
    u32 line_no = 0;
    bool ok = true;

    while (cursor && *cursor) {
        line_no++;
        char *line = cursor;
        char *nl = strchr(cursor, '\n');
        if (nl) {
            *nl = '\0';
            cursor = nl + 1;
        } else {
            cursor = NULL;
        }

        sp_trim_inplace(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        if (line[0] == '[') {
            size_t n = strlen(line);
            if (n < 3 || line[n - 1] != ']') {
                (void)sp_push_diag(&out_doc->diagnostics, CCT_SIGIL_DIAG_ERROR, CCT_SIGIL_PARSE_SYNTAX, line_no, 1, "invalid section header");
                ok = false;
                continue;
            }
            line[n - 1] = '\0';
            char *raw_section = line + 1;
            sp_trim_inplace(raw_section);
            free(section);
            section = sp_strdup(raw_section);
            if (!section) {
                ok = false;
                break;
            }
            sp_capture_known_section(out_doc, section);
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq) {
            (void)sp_push_diag(&out_doc->diagnostics, CCT_SIGIL_DIAG_ERROR, CCT_SIGIL_PARSE_SYNTAX, line_no, 1, "expected key = value");
            ok = false;
            continue;
        }
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        sp_trim_inplace(key);
        sp_trim_inplace(value);
        if (key[0] == '\0') {
            (void)sp_push_diag(&out_doc->diagnostics, CCT_SIGIL_DIAG_ERROR, CCT_SIGIL_PARSE_SYNTAX, line_no, 1, "empty key in key-value pair");
            ok = false;
            continue;
        }

        if (sp_lookup_duplicate_key(out_doc, section, key)) {
            cct_sigil_diag_level_t lvl = sp_mode_is_strict_contract(mode)
                ? CCT_SIGIL_DIAG_ERROR
                : CCT_SIGIL_DIAG_WARNING;
            (void)sp_push_diag(&out_doc->diagnostics, lvl, CCT_SIGIL_PARSE_DUPLICATE_KEY, line_no, 1, "duplicate key in same section");
            if (sp_mode_is_strict_contract(mode)) ok = false;
        }

        if (section[0] == '\0' && !sp_is_known_top_level_key(key)) {
            cct_sigil_diag_level_t lvl = CCT_SIGIL_DIAG_WARNING;
            (void)sp_push_diag(&out_doc->diagnostics, lvl, CCT_SIGIL_PARSE_UNKNOWN_FIELD, line_no, 1, "unknown top-level field");
        }
        if (section[0] == '\0' && sp_is_deprecated_top_level_key(key)) {
            (void)sp_push_diag(&out_doc->diagnostics,
                               CCT_SIGIL_DIAG_WARNING,
                               CCT_SIGIL_PARSE_DEPRECATED_FIELD,
                               line_no,
                               1,
                               "deprecated top-level field: sigilo_style (use visual_engine)");
        }

        if (!sp_push_entry(out_doc, section, key, value, line_no)) {
            ok = false;
            break;
        }
        sp_capture_known_field(out_doc, section, key, value);
    }

    free(section);
    free(buf);
    sp_capture_extended_fields(out_doc);
    if (!ok) return false;
    return cct_sigil_validate(out_doc, mode);
}

bool cct_sigil_validate(cct_sigil_document_t *doc, cct_sigil_parse_mode_t mode) {
    if (!doc) return false;
    cct_sigil_validation_result_t result;
    if (!cct_sigil_validate_collect(doc, mode, &result)) return false;

    bool ok = true;
    for (size_t i = 0; i < result.count; i++) {
        const cct_sigil_validation_issue_t *it = &result.items[i];
        (void)sp_push_diag(&doc->diagnostics, it->level, it->kind, it->line, it->column, it->message);
        if (it->level == CCT_SIGIL_DIAG_ERROR) ok = false;
    }
    if (sp_mode_is_strict_contract(mode)) {
        for (size_t i = 0; i < doc->diagnostics.count; i++) {
            if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) {
                ok = false;
                break;
            }
        }
    }

    return ok;
}

bool cct_sigil_document_has_errors(const cct_sigil_document_t *doc) {
    if (!doc) return true;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) return true;
    }
    return false;
}

void cct_sigil_document_dispose(cct_sigil_document_t *doc) {
    if (!doc) return;
    free(doc->input_path);
    free(doc->format);
    free(doc->sigilo_scope);
    free(doc->visual_engine);
    free(doc->semantic_hash);
    free(doc->system_hash);
    free(doc->web_topology_hash);
    free(doc->manifest_format);
    free(doc->manifest_producer);
    free(doc->manifest_project);

    for (size_t i = 0; i < doc->entry_count; i++) {
        free(doc->entries[i].section);
        free(doc->entries[i].key);
        free(doc->entries[i].value);
    }
    free(doc->entries);

    for (size_t i = 0; i < doc->web_routes_count; i++) {
        free(doc->web_routes[i].route_id);
        free(doc->web_routes[i].method);
        free(doc->web_routes[i].path);
        free(doc->web_routes[i].route_name);
        free(doc->web_routes[i].handler);
        free(doc->web_routes[i].module);
        free(doc->web_routes[i].group);
        free(doc->web_routes[i].middleware);
        free(doc->web_routes[i].path_params);
        free(doc->web_routes[i].route_hash);
        free(doc->web_routes[i].source_origin);
    }
    free(doc->web_routes);

    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        free(doc->diagnostics.items[i].message);
    }
    free(doc->diagnostics.items);
    memset(doc, 0, sizeof(*doc));
}
