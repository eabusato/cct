/*
 * CCT — Clavicula Turing
 * Compiler Entry Point
 *
 * FASE 12H: Structural maturity milestone
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
#include "sigilo/sigil_parse.h"
#include "sigilo/sigil_diff.h"
#include "project/sigilo_baseline.h"
#include "module/module.h"
#include "formatter/formatter.h"
#include "lint/lint.h"
#include "project/project.h"
#include "doc/doc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

static bool has_sigil_extension(const char *filename) {
    if (!filename) return false;
    size_t len = strlen(filename);
    return len >= 6 && strcmp(filename + len - 6, ".sigil") == 0;
}

static bool has_ctrace_extension(const char *filename) {
    if (!filename) return false;
    size_t len = strlen(filename);
    return len >= 7 && strcmp(filename + len - 7, ".ctrace") == 0;
}

static char* replace_path_suffix(const char *path, const char *old_suffix, const char *new_suffix) {
    if (!path || !old_suffix || !new_suffix) return NULL;
    size_t path_len = strlen(path);
    size_t old_len = strlen(old_suffix);
    size_t new_len = strlen(new_suffix);
    if (path_len < old_len) return NULL;
    if (strcmp(path + (path_len - old_len), old_suffix) != 0) return NULL;

    size_t out_len = path_len - old_len + new_len;
    char *out = (char*)malloc(out_len + 1);
    if (!out) return NULL;
    memcpy(out, path, path_len - old_len);
    memcpy(out + (path_len - old_len), new_suffix, new_len);
    out[out_len] = '\0';
    return out;
}

static int cct_system_exit_code(int rc) {
    if (rc < 0) return rc;
    if (rc > 255) return (rc >> 8) & 0xff;
    return rc;
}

typedef enum {
    CCT_CROSS_COMPILER_NONE = 0,
    CCT_CROSS_COMPILER_GNU_M32,
    CCT_CROSS_COMPILER_CLANG_I386_ELF
} cct_cross_compiler_mode_t;

typedef struct {
    char cc[512];
    cct_cross_compiler_mode_t mode;
} cct_cross_compiler_t;

static bool cct_cross_compiler_supports_gnu_m32(const char *cc) {
    if (!cc || cc[0] == '\0') return false;
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "printf 'int __cct_probe = 0;\\n' | \"%s\" "
             "-m32 -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin -fwrapv "
             "-fno-asynchronous-unwind-tables -fno-unwind-tables "
             "-x c -c -o /dev/null - >/dev/null 2>&1 && "
             "printf '' | \"%s\" -m32 -dM -E -x c - 2>/dev/null | grep -q '__i386__'",
             cc, cc);
    return system(cmd) == 0;
}

static bool cct_cross_compiler_supports_clang_i386_target(const char *cc) {
    if (!cc || cc[0] == '\0') return false;
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "printf 'int __cct_probe = 0;\\n' | \"%s\" "
             "-target i386-unknown-none-elf "
             "-ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin -fwrapv "
             "-fno-asynchronous-unwind-tables -fno-unwind-tables "
             "-x c -c -o /dev/null - >/dev/null 2>&1 && "
             "printf '' | \"%s\" -target i386-unknown-none-elf -dM -E -x c - 2>/dev/null | grep -q '__i386__'",
             cc, cc);
    return system(cmd) == 0;
}

static bool cct_resolve_cross_compiler(cct_cross_compiler_t *out) {
    if (!out) return false;
    out->cc[0] = '\0';
    out->mode = CCT_CROSS_COMPILER_NONE;

    const char *env_cc = getenv("CCT_CROSS_CC");
    if (env_cc && env_cc[0] != '\0') {
        if (cct_cross_compiler_supports_gnu_m32(env_cc)) {
            snprintf(out->cc, sizeof(out->cc), "%s", env_cc);
            out->mode = CCT_CROSS_COMPILER_GNU_M32;
            return true;
        }
        if (cct_cross_compiler_supports_clang_i386_target(env_cc)) {
            snprintf(out->cc, sizeof(out->cc), "%s", env_cc);
            out->mode = CCT_CROSS_COMPILER_CLANG_I386_ELF;
            return true;
        }
        return false;
    }

    if (cct_cross_compiler_supports_gnu_m32("i686-elf-gcc")) {
        snprintf(out->cc, sizeof(out->cc), "%s", "i686-elf-gcc");
        out->mode = CCT_CROSS_COMPILER_GNU_M32;
        return true;
    }

    if (cct_cross_compiler_supports_clang_i386_target("clang")) {
        snprintf(out->cc, sizeof(out->cc), "%s", "clang");
        out->mode = CCT_CROSS_COMPILER_CLANG_I386_ELF;
        return true;
    }

    if (cct_cross_compiler_supports_gnu_m32("gcc")) {
        snprintf(out->cc, sizeof(out->cc), "%s", "gcc");
        out->mode = CCT_CROSS_COMPILER_GNU_M32;
        return true;
    }

    if (cct_cross_compiler_supports_clang_i386_target("gcc")) {
        snprintf(out->cc, sizeof(out->cc), "%s", "gcc");
        out->mode = CCT_CROSS_COMPILER_CLANG_I386_ELF;
        return true;
    }

    return false;
}

static const char *cct_cross_compiler_cflags(cct_cross_compiler_mode_t mode) {
    switch (mode) {
        case CCT_CROSS_COMPILER_GNU_M32:
            return "-m32";
        case CCT_CROSS_COMPILER_CLANG_I386_ELF:
            return "-target i386-unknown-none-elf";
        case CCT_CROSS_COMPILER_NONE:
        default:
            return "";
    }
}

static void cct_runtime_include_dir(char *out, size_t out_cap) {
    if (!out || out_cap == 0) return;
    const char *header = CCT_FREESTANDING_RT_HEADER;
    if (!header || header[0] == '\0') {
        snprintf(out, out_cap, "%s", ".");
        return;
    }

    const char *slash = strrchr(header, '/');
#ifdef _WIN32
    const char *backslash = strrchr(header, '\\');
    if (!slash || (backslash && backslash > slash)) slash = backslash;
#endif
    if (!slash) {
        snprintf(out, out_cap, "%s", ".");
        return;
    }

    size_t dir_len = (size_t)(slash - header);
    if (dir_len >= out_cap) dir_len = out_cap - 1;
    memcpy(out, header, dir_len);
    out[dir_len] = '\0';
}

static cct_error_code_t cct_emit_asm_from_cgen(const char *input_file) {
    cct_cross_compiler_t compiler;
    if (!cct_resolve_cross_compiler(&compiler)) {
        fprintf(stderr,
                "cct: --emit-asm requer cross-compiler (-m32) ou clang -target i386-unknown-none-elf; instale i686-elf-gcc/clang ou defina CCT_CROSS_CC\n");
        return CCT_ERROR_CODEGEN;
    }

    char *cgen_c_path = replace_path_suffix(input_file, ".cct", ".cgen.c");
    char *cgen_s_path = replace_path_suffix(input_file, ".cct", ".cgen.s");
    if (!cgen_c_path || !cgen_s_path) {
        free(cgen_c_path);
        free(cgen_s_path);
        cct_error_printf(CCT_ERROR_INTERNAL, "falha ao derivar caminhos .cgen.c/.cgen.s");
        return CCT_ERROR_INTERNAL;
    }

    char runtime_dir[1024];
    cct_runtime_include_dir(runtime_dir, sizeof(runtime_dir));

    char command[8192];
    snprintf(command, sizeof(command),
             "\"%s\" %s -S -masm=intel "
             "-ffreestanding -nostdlib -fno-pic -fwrapv "
             "-fno-stack-protector -fno-builtin "
             "-fno-asynchronous-unwind-tables -fno-unwind-tables "
             "-I \"%s\" \"%s\" -o \"%s\"",
             compiler.cc, cct_cross_compiler_cflags(compiler.mode),
             runtime_dir, cgen_c_path, cgen_s_path);

    int rc = system(command);
    if (rc != 0) {
        fprintf(stderr, "cct: cross-compiler falhou (exit %d)\n", cct_system_exit_code(rc));
        free(cgen_c_path);
        free(cgen_s_path);
        return CCT_ERROR_CODEGEN;
    }

    printf("ASM emitted: %s\n", cgen_s_path);
    printf("Cross compiler: %s\n", compiler.cc);

    free(cgen_c_path);
    free(cgen_s_path);
    return CCT_OK;
}

static cct_diagnostic_level_t sigilo_diag_to_level(cct_sigil_diag_level_t level) {
    return level == CCT_SIGIL_DIAG_ERROR ? CCT_DIAG_ERROR : CCT_DIAG_WARNING;
}

static const cct_sigil_document_t *g_sigilo_sort_doc = NULL;

static int compare_sigil_diag_indices(const void *a, const void *b) {
    const cct_sigil_document_t *doc = g_sigilo_sort_doc;
    if (!doc) return 0;
    size_t ia = *(const size_t*)a;
    size_t ib = *(const size_t*)b;
    const cct_sigil_diag_t *da = &doc->diagnostics.items[ia];
    const cct_sigil_diag_t *db = &doc->diagnostics.items[ib];

    /* Keep errors before warnings, then deterministic positional/text order. */
    if (da->level != db->level) {
        return (da->level == CCT_SIGIL_DIAG_ERROR) ? -1 : 1;
    }
    if (da->kind != db->kind) {
        return (int)da->kind - (int)db->kind;
    }
    if (da->line != db->line) {
        return (da->line < db->line) ? -1 : 1;
    }
    if (da->column != db->column) {
        return (da->column < db->column) ? -1 : 1;
    }
    const char *ma = da->message ? da->message : "";
    const char *mb = db->message ? db->message : "";
    int msg_cmp = strcmp(ma, mb);
    if (msg_cmp != 0) return msg_cmp;
    if (ia < ib) return -1;
    if (ia > ib) return 1;
    return 0;
}

static size_t* sigilo_sorted_diag_indices(const cct_sigil_document_t *doc) {
    if (!doc || doc->diagnostics.count == 0) return NULL;
    size_t *indices = (size_t*)malloc(sizeof(size_t) * doc->diagnostics.count);
    if (!indices) return NULL;
    for (size_t i = 0; i < doc->diagnostics.count; i++) indices[i] = i;
    g_sigilo_sort_doc = doc;
    qsort(indices, doc->diagnostics.count, sizeof(size_t), compare_sigil_diag_indices);
    g_sigilo_sort_doc = NULL;
    return indices;
}

static void sigilo_print_explain(const char *command,
                                 const char *cause,
                                 const char *action,
                                 bool blocked) {
    fprintf(stderr,
            "sigilo.explain probable_cause=%s recommended_action=%s docs=docs/sigilo_troubleshooting_13b4.md blocked=%s command=%s\n",
            cause ? cause : "unknown",
            action ? action : "inspect diagnostic output and correct input",
            blocked ? "true" : "false",
            command ? command : "sigilo");
}

static void print_sigil_diagnostics(const cct_sigil_document_t *doc) {
    if (!doc) return;
    size_t *indices = sigilo_sorted_diag_indices(doc);
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        size_t idx = indices ? indices[i] : i;
        const cct_sigil_diag_t *d = &doc->diagnostics.items[idx];
        char msg[2048];
        snprintf(msg,
                 sizeof(msg),
                 "sigilo[%s]: %s",
                 cct_sigil_diag_kind_str(d->kind),
                 d->message ? d->message : "<no message>");

        cct_diagnostic_t diag = {
            .level = sigilo_diag_to_level(d->level),
            .message = msg,
            .file_path = (d->line > 0) ? (doc->input_path ? doc->input_path : "<sigil>") : NULL,
            .line = d->line,
            .column = d->column,
            .suggestion = NULL,
            .code_label = "sigilo",
        };
        cct_diagnostic_emit(&diag);
    }
    free(indices);
}

static int sigilo_emit_inspect_text(const cct_sigil_document_t *doc, bool summary) {
    if (!doc) return (int)CCT_ERROR_INTERNAL;
    size_t warnings = 0;
    size_t errors = 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) errors++;
        else warnings++;
    }
    printf("sigilo.inspect.summary scope=%s format=%s entries=%zu warnings=%zu errors=%zu\n",
           doc->sigilo_scope ? doc->sigilo_scope : "<none>",
           doc->format ? doc->format : "<none>",
           doc->entry_count,
           warnings,
           errors);
    if (doc->has_web_routes) {
        printf("sigilo.inspect.web route_count=%zu group_count=%llu middleware_count=%llu web_hash=%s\n",
               doc->web_routes_count,
               (unsigned long long)doc->web_group_count,
               (unsigned long long)doc->web_middleware_count,
               doc->web_topology_hash ? doc->web_topology_hash : "");
    }
    if (summary) return 0;

    printf("input_path=%s\n", doc->input_path ? doc->input_path : "<none>");
    if (doc->semantic_hash) printf("semantic_hash=%s\n", doc->semantic_hash);
    if (doc->system_hash) printf("system_hash=%s\n", doc->system_hash);
    if (doc->visual_engine) printf("visual_engine=%s\n", doc->visual_engine);
    for (size_t i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        printf("[%zu] section=%s key=%s value=%s\n",
               i,
               kv->section ? kv->section : "",
               kv->key ? kv->key : "",
               kv->value ? kv->value : "");
    }
    return 0;
}

static int sigilo_emit_inspect_structured(const cct_sigil_document_t *doc, bool summary) {
    if (!doc) return (int)CCT_ERROR_INTERNAL;
    size_t warnings = 0;
    size_t errors = 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) errors++;
        else warnings++;
    }
    printf("format = cct.sigil.inspect.v1\n");
    printf("input_path = %s\n", doc->input_path ? doc->input_path : "");
    printf("sigilo_scope = %s\n", doc->sigilo_scope ? doc->sigilo_scope : "");
    printf("sigil_format = %s\n", doc->format ? doc->format : "");
    printf("entries = %zu\n", doc->entry_count);
    printf("warnings = %zu\n", warnings);
    printf("errors = %zu\n", errors);
    if (doc->semantic_hash) printf("semantic_hash = %s\n", doc->semantic_hash);
    if (doc->system_hash) printf("system_hash = %s\n", doc->system_hash);
    if (doc->has_web_routes) {
        printf("web_route_count = %zu\n", doc->web_routes_count);
        printf("web_group_count = %llu\n", (unsigned long long)doc->web_group_count);
        printf("web_middleware_count = %llu\n", (unsigned long long)doc->web_middleware_count);
        if (doc->web_topology_hash) printf("web_topology_hash = %s\n", doc->web_topology_hash);
    }
    if (doc->has_manifest_provenance) {
        if (doc->manifest_format) printf("manifest_format = %s\n", doc->manifest_format);
        if (doc->manifest_producer) printf("manifest_producer = %s\n", doc->manifest_producer);
        if (doc->manifest_project) printf("manifest_project = %s\n", doc->manifest_project);
    }
    if (summary) return 0;

    for (size_t i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        printf("\n[entry.%zu]\n", i);
        printf("section = %s\n", kv->section ? kv->section : "");
        printf("key = %s\n", kv->key ? kv->key : "");
        printf("value = %s\n", kv->value ? kv->value : "");
    }
    return 0;
}

static const char* sigilo_parse_mode_name(cct_sigil_parse_mode_t mode) {
    switch (mode) {
        case CCT_SIGIL_PARSE_MODE_LEGACY_TOLERANT: return "legacy-tolerant";
        case CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT: return "current-default";
        case CCT_SIGIL_PARSE_MODE_STRICT:
        case CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT: return "strict-contract";
        case CCT_SIGIL_PARSE_MODE_TOLERANT:
        default: return "tolerant";
    }
}

static int sigilo_emit_validate_text(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    bool summary
) {
    if (!doc) return (int)CCT_ERROR_INTERNAL;
    size_t warnings = 0;
    size_t errors = 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) errors++;
        else warnings++;
    }
    printf("sigilo.validate.summary profile=%s scope=%s format=%s entries=%zu warnings=%zu errors=%zu result=%s\n",
           sigilo_parse_mode_name(mode),
           doc->sigilo_scope ? doc->sigilo_scope : "<none>",
           doc->format ? doc->format : "<none>",
           doc->entry_count,
           warnings,
           errors,
           errors == 0 ? "pass" : "fail");
    if (summary) return 0;
    print_sigil_diagnostics(doc);
    return 0;
}

static int sigilo_emit_validate_structured(
    const cct_sigil_document_t *doc,
    cct_sigil_parse_mode_t mode,
    bool summary
) {
    if (!doc) return (int)CCT_ERROR_INTERNAL;
    size_t warnings = 0;
    size_t errors = 0;
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        if (doc->diagnostics.items[i].level == CCT_SIGIL_DIAG_ERROR) errors++;
        else warnings++;
    }
    printf("format = cct.sigil.validate.v1\n");
    printf("profile = %s\n", sigilo_parse_mode_name(mode));
    printf("input_path = %s\n", doc->input_path ? doc->input_path : "");
    printf("sigilo_scope = %s\n", doc->sigilo_scope ? doc->sigilo_scope : "");
    printf("sigil_format = %s\n", doc->format ? doc->format : "");
    printf("entries = %zu\n", doc->entry_count);
    printf("warnings = %zu\n", warnings);
    printf("errors = %zu\n", errors);
    printf("result = %s\n", errors == 0 ? "pass" : "fail");
    if (summary) return 0;
    size_t *indices = sigilo_sorted_diag_indices(doc);
    for (size_t i = 0; i < doc->diagnostics.count; i++) {
        size_t idx = indices ? indices[i] : i;
        const cct_sigil_diag_t *d = &doc->diagnostics.items[idx];
        printf("\n[diag.%zu]\n", i);
        printf("level = %s\n", d->level == CCT_SIGIL_DIAG_ERROR ? "error" : "warning");
        printf("kind = %s\n", cct_sigil_diag_kind_str(d->kind));
        printf("line = %u\n", d->line);
        printf("column = %u\n", d->column);
        printf("message = %s\n", d->message ? d->message : "");
    }
    free(indices);
    return 0;
}

typedef struct {
    char *trace_id;
    char *span_id;
    char *parent_id;
    char *name;
    long long start_time_ms;
    long long end_time_ms;
    long long duration_ms;
    char *attributes_json;
    char *route_id;
    char *handler;
    char *module_path;
    char *http_method;
    char *http_path;
    int parent_index;
} cct_ctrace_span_t;

typedef struct {
    cct_ctrace_span_t *items;
    size_t count;
    size_t capacity;
    size_t warnings;
    size_t errors;
    char *input_path;
    char *trace_id;
} cct_ctrace_document_t;

static void ctrace_span_dispose(cct_ctrace_span_t *span) {
    if (!span) return;
    free(span->trace_id);
    free(span->span_id);
    free(span->parent_id);
    free(span->name);
    free(span->attributes_json);
    free(span->route_id);
    free(span->handler);
    free(span->module_path);
    free(span->http_method);
    free(span->http_path);
    memset(span, 0, sizeof(*span));
    span->parent_index = -1;
}

static void ctrace_document_dispose(cct_ctrace_document_t *doc) {
    if (!doc) return;
    for (size_t i = 0; i < doc->count; i++) ctrace_span_dispose(&doc->items[i]);
    free(doc->items);
    free(doc->input_path);
    free(doc->trace_id);
    memset(doc, 0, sizeof(*doc));
}

static bool ctrace_push_span(cct_ctrace_document_t *doc, cct_ctrace_span_t *span) {
    if (!doc || !span) return false;
    if (doc->count >= doc->capacity) {
        size_t next = doc->capacity == 0 ? 8 : doc->capacity * 2;
        cct_ctrace_span_t *grown = (cct_ctrace_span_t*)realloc(doc->items, next * sizeof(*grown));
        if (!grown) return false;
        memset(grown + doc->capacity, 0, (next - doc->capacity) * sizeof(*grown));
        doc->items = grown;
        doc->capacity = next;
    }
    doc->items[doc->count++] = *span;
    return true;
}

static char* ctrace_strdup_range(const char *start, const char *end) {
    if (!start || !end || end < start) return NULL;
    size_t len = (size_t)(end - start);
    char *out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static const char* ctrace_find_key(const char *json, const char *key) {
    static char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key ? key : "");
    const char *p = strstr(json ? json : "", pattern);
    return p ? p + strlen(pattern) : NULL;
}

static char* ctrace_extract_json_string(const char *json, const char *key) {
    const char *p = ctrace_find_key(json, key);
    if (!p) return NULL;
    if (*p != '"') return NULL;
    p++;
    const char *start = p;
    while (*p) {
        if (*p == '\\' && p[1] != '\0') {
            p += 2;
            continue;
        }
        if (*p == '"') break;
        p++;
    }
    if (*p != '"') return NULL;
    return ctrace_strdup_range(start, p);
}

static bool ctrace_extract_json_int(const char *json, const char *key, long long *out) {
    const char *p = ctrace_find_key(json, key);
    if (!p || !out) return false;
    char *end = NULL;
    long long v = strtoll(p, &end, 10);
    if (end == p) return false;
    *out = v;
    return true;
}

static char* ctrace_extract_json_object_text(const char *json, const char *key) {
    const char *p = ctrace_find_key(json, key);
    if (!p || *p != '{') return NULL;
    const char *start = p;
    int depth = 0;
    bool in_string = false;
    while (*p) {
        if (*p == '"' && (p == start || p[-1] != '\\')) {
            in_string = !in_string;
        } else if (!in_string) {
            if (*p == '{') depth++;
            else if (*p == '}') {
                depth--;
                if (depth == 0) {
                    p++;
                    return ctrace_strdup_range(start, p);
                }
            }
        }
        p++;
    }
    return NULL;
}

static char* ctrace_extract_attr_string(const char *attrs_json, const char *key) {
    return ctrace_extract_json_string(attrs_json, key);
}

static bool ctrace_parse_line(const char *line, cct_ctrace_span_t *out) {
    if (!line || !out || line[0] == '\0') return false;
    memset(out, 0, sizeof(*out));
    out->parent_index = -1;
    out->trace_id = ctrace_extract_json_string(line, "trace_id");
    out->span_id = ctrace_extract_json_string(line, "span_id");
    out->parent_id = ctrace_extract_json_string(line, "parent_id");
    out->name = ctrace_extract_json_string(line, "name");
    out->attributes_json = ctrace_extract_json_object_text(line, "attributes");
    if (!out->trace_id || !out->span_id || !out->name || !out->parent_id) {
        ctrace_span_dispose(out);
        return false;
    }
    if (!ctrace_extract_json_int(line, "start_time_ms", &out->start_time_ms) ||
        !ctrace_extract_json_int(line, "end_time_ms", &out->end_time_ms)) {
        ctrace_span_dispose(out);
        return false;
    }
    if (!ctrace_extract_json_int(line, "duration_ms", &out->duration_ms)) {
        out->duration_ms = out->end_time_ms - out->start_time_ms;
    }
    if (out->attributes_json) {
        out->route_id = ctrace_extract_attr_string(out->attributes_json, "route_id");
        out->handler = ctrace_extract_attr_string(out->attributes_json, "handler");
        out->module_path = ctrace_extract_attr_string(out->attributes_json, "module");
        out->http_method = ctrace_extract_attr_string(out->attributes_json, "http.method");
        out->http_path = ctrace_extract_attr_string(out->attributes_json, "http.path");
    }
    return true;
}

static bool ctrace_parse_document(const char *path, bool strict, cct_ctrace_document_t *out) {
    if (!path || !out) return false;
    memset(out, 0, sizeof(*out));
    out->input_path = strdup(path);
    char *text = read_file(path);
    if (!text) return false;
    char *save = NULL;
    char *line = strtok_r(text, "\n", &save);
    while (line) {
        while (*line == ' ' || *line == '\t' || *line == '\r') line++;
        if (*line != '\0') {
            cct_ctrace_span_t span;
            if (!ctrace_parse_line(line, &span)) {
                out->errors++;
                if (strict) {
                    free(text);
                    return false;
                }
            } else {
                if (!out->trace_id) {
                    out->trace_id = strdup(span.trace_id);
                }
                if (out->trace_id && strcmp(out->trace_id, span.trace_id) != 0) {
                    out->errors++;
                    ctrace_span_dispose(&span);
                    if (strict) {
                        free(text);
                        return false;
                    }
                } else if (!ctrace_push_span(out, &span)) {
                    ctrace_span_dispose(&span);
                    free(text);
                    return false;
                }
            }
        }
        line = strtok_r(NULL, "\n", &save);
    }
    free(text);
    for (size_t i = 0; i < out->count; i++) {
        for (size_t j = 0; j < out->count; j++) {
            if (i == j) continue;
            if (out->items[i].parent_id && out->items[j].span_id &&
                strcmp(out->items[i].parent_id, out->items[j].span_id) == 0) {
                out->items[i].parent_index = (int)j;
                break;
            }
        }
    }
    if (out->count == 0) {
        out->errors++;
        return !strict;
    }
    return out->errors == 0 || !strict;
}

static void ctrace_emit_view_span(const cct_ctrace_document_t *doc, int idx, int depth, int max_depth, bool show_attrs) {
    if (!doc || idx < 0 || (size_t)idx >= doc->count) return;
    const cct_ctrace_span_t *span = &doc->items[idx];
    if (max_depth >= 0 && depth > max_depth) return;
    for (int i = 0; i < depth; i++) printf("    ");
    printf("%s%s (%lldms)\n", depth > 0 ? "└── " : "", span->name ? span->name : "<span>", span->duration_ms);
    if (show_attrs && span->attributes_json && span->attributes_json[0] != '\0' && strcmp(span->attributes_json, "{}") != 0) {
        for (int i = 0; i < depth + 1; i++) printf("    ");
        printf("attrs=%s\n", span->attributes_json);
    }
    for (size_t i = 0; i < doc->count; i++) {
        if (doc->items[i].parent_index == idx) ctrace_emit_view_span(doc, (int)i, depth + 1, max_depth, show_attrs);
    }
}

static int cmd_sigilo_trace_view(const char *path, bool strict, bool summary, bool show_attrs, int max_depth) {
    cct_ctrace_document_t doc;
    if (!ctrace_parse_document(path, strict, &doc)) {
        ctrace_document_dispose(&doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    long long total = 0;
    for (size_t i = 0; i < doc.count; i++) if (doc.items[i].duration_ms > total) total = doc.items[i].duration_ms;
    printf("trace %s spans=%zu total=%lldms warnings=%zu errors=%zu\n",
           doc.trace_id ? doc.trace_id : "<unknown>",
           doc.count, total, doc.warnings, doc.errors);
    if (!summary) {
        for (size_t i = 0; i < doc.count; i++) {
            if (doc.items[i].parent_index < 0 || !doc.items[i].parent_id || doc.items[i].parent_id[0] == '\0') {
                ctrace_emit_view_span(&doc, (int)i, 0, max_depth, show_attrs);
            }
        }
    }
    ctrace_document_dispose(&doc);
    return 0;
}

static void ctrace_svg_escape(FILE *f, const char *text) {
    const char *p = text ? text : "";
    while (*p) {
        if (*p == '&') fputs("&amp;", f);
        else if (*p == '<') fputs("&lt;", f);
        else if (*p == '>') fputs("&gt;", f);
        else if (*p == '"') fputs("&quot;", f);
        else fputc(*p, f);
        p++;
    }
}

static const cct_sigil_web_route_t* sigilo_find_route_by_id(const cct_sigil_document_t *doc, const char *route_id) {
    if (!doc || !route_id) return NULL;
    for (size_t i = 0; i < doc->web_routes_count; i++) {
        if (doc->web_routes[i].route_id && strcmp(doc->web_routes[i].route_id, route_id) == 0) return &doc->web_routes[i];
    }
    return NULL;
}

static void ctrace_hash_u8(u64 *hash, u8 v) {
    *hash ^= (u64)v;
    *hash *= 1099511628211ULL;
}

static void ctrace_hash_str(u64 *hash, const char *text) {
    const char *p = text ? text : "";
    while (*p) {
        ctrace_hash_u8(hash, (u8)*p);
        p++;
    }
    ctrace_hash_u8(hash, 0);
}

static u64 ctrace_compute_trace_hash(const cct_ctrace_document_t *doc) {
    u64 hash = 1469598103934665603ULL;
    if (!doc) return hash;
    ctrace_hash_str(&hash, doc->trace_id);
    for (size_t i = 0; i < doc->count; i++) {
        ctrace_hash_str(&hash, doc->items[i].span_id);
        ctrace_hash_str(&hash, doc->items[i].parent_id);
        ctrace_hash_str(&hash, doc->items[i].name);
        ctrace_hash_str(&hash, doc->items[i].route_id);
        ctrace_hash_str(&hash, doc->items[i].attributes_json);
        for (int b = 0; b < 8; b++) {
            ctrace_hash_u8(&hash, (u8)((doc->items[i].duration_ms >> (b * 8)) & 0xFF));
        }
    }
    return hash;
}

#define CCT_TRACE_PI 3.14159265358979323846

static double ctrace_hash_unit(u64 seed, u32 salt) {
    u64 x = seed ^ (0x9e3779b97f4a7c15ULL * (u64)(salt + 1));
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return (double)(x & 0xFFFFFFULL) / (double)0x1000000ULL;
}

static u64 ctrace_route_seed(const cct_sigil_web_route_t *route) {
    u64 seed = 1469598103934665603ULL;
    const char *hex = route ? route->route_hash : NULL;
    if (hex && hex[0] != '\0') {
        char *end = NULL;
        unsigned long long parsed = strtoull(hex, &end, 16);
        if (end && *end == '\0') return (u64)parsed;
    }
    ctrace_hash_str(&seed, route ? route->route_id : NULL);
    ctrace_hash_str(&seed, route ? route->path : NULL);
    return seed;
}

static void ctrace_svg_arc(FILE *f, const char *cls, double x, double y, double r, double a0, double a1) {
    double sx = x + cos(a0) * r;
    double sy = y + sin(a0) * r;
    double ex = x + cos(a1) * r;
    double ey = y + sin(a1) * r;
    int large_arc = fabs(a1 - a0) > CCT_TRACE_PI ? 1 : 0;
    int sweep = a1 >= a0 ? 1 : 0;
    fprintf(f,
            "<path class=\"%s\" d=\"M %.2f %.2f A %.2f %.2f 0 %d %d %.2f %.2f\"/>",
            cls, sx, sy, r, r, large_arc, sweep, ex, ey);
}

static void ctrace_emit_route_hit_sigil(FILE *f, const cct_sigil_web_route_t *route, double x, double y, double base_r) {
    u64 seed = ctrace_route_seed(route);
    double shell_r = base_r * (1.22 + ctrace_hash_unit(seed, 1) * 0.24);
    double mid_r = base_r * (0.82 + ctrace_hash_unit(seed, 2) * 0.18);
    double core_r = base_r * (0.42 + ctrace_hash_unit(seed, 3) * 0.12);
    double tilt = ctrace_hash_unit(seed, 4) * (2.0 * CCT_TRACE_PI);
    u32 spoke_count = 3u + (u32)(ctrace_hash_unit(seed, 5) * 5.0);

    fprintf(f, "<circle class=\"route-shell\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, shell_r);
    fprintf(f, "<circle class=\"route-mid\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, mid_r);
    ctrace_svg_arc(f, "route-arc", x, y, shell_r * 0.92, tilt - 0.68, tilt + 0.84);
    ctrace_svg_arc(f, "route-arc", x, y, shell_r * 0.74, tilt + 1.52, tilt + 2.66);
    for (u32 i = 0; i < spoke_count; i++) {
        double a = tilt + (2.0 * CCT_TRACE_PI) * (double)i / (double)spoke_count;
        double ex = x + cos(a) * (shell_r - 1.8);
        double ey = y + sin(a) * (shell_r - 1.8);
        fprintf(f, "<path class=\"route-spoke\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\"/>",
                x, y,
                x + cos(a + CCT_TRACE_PI * 0.5) * (base_r * 0.12),
                y + sin(a + CCT_TRACE_PI * 0.5) * (base_r * 0.12),
                ex, ey);
    }
    fprintf(f, "<circle class=\"route-hit\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, core_r);
    fprintf(f,
            "<path class=\"route-hit-stroke\" d=\"M %.2f %.2f L %.2f %.2f L %.2f %.2f Z\"/>",
            x + cos(tilt) * (core_r + 1.0),
            y + sin(tilt) * (core_r + 1.0),
            x + cos(tilt + CCT_TRACE_PI * 0.75) * (core_r + 0.6),
            y + sin(tilt + CCT_TRACE_PI * 0.75) * (core_r + 0.6),
            x + cos(tilt - CCT_TRACE_PI * 0.75) * (core_r + 0.6),
            y + sin(tilt - CCT_TRACE_PI * 0.75) * (core_r + 0.6));
}

static bool cmd_sigilo_trace_export_svg(const char *ctrace_path, const char *sigil_path, const char *output_path, bool strict) {
    cct_ctrace_document_t doc;
    if (!ctrace_parse_document(ctrace_path, strict, &doc)) {
        ctrace_document_dispose(&doc);
        return false;
    }

    cct_sigil_document_t sigil_doc;
    bool has_sigil = false;
    memset(&sigil_doc, 0, sizeof(sigil_doc));
    if (sigil_path && sigil_path[0] != '\0') {
        has_sigil = cct_sigil_parse_file(sigil_path, strict ? CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT : CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT, &sigil_doc) &&
                    !cct_sigil_document_has_errors(&sigil_doc);
    }

    const char *out_path = output_path;
    char *derived = NULL;
    if (!out_path || out_path[0] == '\0') {
        derived = replace_path_suffix(ctrace_path, ".ctrace", ".svg");
        out_path = derived ? derived : "trace.svg";
    }

    FILE *f = fopen(out_path, "wb");
    if (!f) {
        free(derived);
        if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
        ctrace_document_dispose(&doc);
        return false;
    }

    const double width = 960.0;
    const double height = 720.0;
    const double cx = width * 0.5;
    const double cy = height * 0.5 + 20.0;
    const double core_r = 82.0;
    u64 trace_hash = ctrace_compute_trace_hash(&doc);
    int *depths = (int*)calloc(doc.count ? doc.count : 1, sizeof(int));
    size_t *depth_totals = (size_t*)calloc(doc.count ? doc.count : 1, sizeof(size_t));
    size_t *depth_seen = (size_t*)calloc(doc.count ? doc.count : 1, sizeof(size_t));
    const cct_sigil_web_route_t **route_hits = (const cct_sigil_web_route_t**)calloc(doc.count ? doc.count : 1, sizeof(*route_hits));
    double *route_hit_x = (double*)calloc(doc.count ? doc.count : 1, sizeof(double));
    double *route_hit_y = (double*)calloc(doc.count ? doc.count : 1, sizeof(double));
    double *x_pos = (double*)calloc(doc.count ? doc.count : 1, sizeof(double));
    double *y_pos = (double*)calloc(doc.count ? doc.count : 1, sizeof(double));
    double *node_radius = (double*)calloc(doc.count ? doc.count : 1, sizeof(double));
    size_t route_hit_count = 0;
    int max_depth = 0;
    if (!depths || !depth_totals || !depth_seen || !route_hits || !route_hit_x || !route_hit_y || !x_pos || !y_pos || !node_radius) {
        free(depths);
        free(depth_totals);
        free(depth_seen);
        free(route_hits);
        free(route_hit_x);
        free(route_hit_y);
        free(x_pos);
        free(y_pos);
        free(node_radius);
        fclose(f);
        free(derived);
        if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
        ctrace_document_dispose(&doc);
        return false;
    }

    for (size_t i = 0; i < doc.count; i++) {
        int depth = 0;
        int parent = doc.items[i].parent_index;
        while (parent >= 0) {
            depth++;
            parent = doc.items[parent].parent_index;
        }
        depths[i] = depth;
        if (depth > max_depth) max_depth = depth;
        if ((size_t)depth < doc.count) depth_totals[depth]++;
        if (has_sigil && doc.items[i].route_id) {
            const cct_sigil_web_route_t *route = sigilo_find_route_by_id(&sigil_doc, doc.items[i].route_id);
            bool seen = false;
            if (route) {
                for (size_t r = 0; r < route_hit_count; r++) {
                    if (route_hits[r] == route) {
                        seen = true;
                        break;
                    }
                }
                if (!seen) route_hits[route_hit_count++] = route;
            }
        }
    }

    memset(depth_seen, 0, sizeof(size_t) * (doc.count ? doc.count : 1));
    for (size_t i = 0; i < doc.count; i++) {
        int depth = depths[i];
        size_t total_at_depth = ((size_t)depth < doc.count) ? depth_totals[depth] : 1;
        size_t slot = ((size_t)depth < doc.count) ? depth_seen[depth]++ : 0;
        double spread = (total_at_depth <= 1) ? 0.0 : (0.92 + (double)total_at_depth * 0.14);
        double a = (depth == 0)
            ? (-CCT_TRACE_PI * 0.5)
            : ((total_at_depth <= 1)
                ? (-CCT_TRACE_PI * 0.5)
                : ((-CCT_TRACE_PI * 0.5) - (spread * 0.5) + ((double)slot * spread / (double)(total_at_depth - 1))));
        double rr = (depth == 0) ? 0.0 : (118.0 + (double)depth * 68.0 + (ctrace_hash_unit(trace_hash, 950 + (u32)i) - 0.5) * 14.0);
        a += (ctrace_hash_unit(trace_hash, 900 + (u32)i) - 0.5) * 0.24;
        x_pos[i] = (depth == 0) ? cx : cx + cos(a) * rr;
        y_pos[i] = (depth == 0) ? cy : cy + sin(a) * rr;
        node_radius[i] = (depth == 0) ? 17.0 : (10.0 + ctrace_hash_unit(trace_hash, 1100 + (u32)i) * 4.0);
        if (!isfinite(x_pos[i]) || !isfinite(y_pos[i])) {
            x_pos[i] = cx + ((double)((int)(i % 5) - 2) * 44.0);
            y_pos[i] = cy + 120.0 + (double)i * 28.0;
        }
    }

    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %.0f %.0f\" width=\"%.0f\" height=\"%.0f\" role=\"img\" aria-label=\"CCT trace export\">\n", width, height, width, height);
    fprintf(f,
            "<style><![CDATA["
            ".bg{fill:#f7f3ea}.halo{fill:none;stroke:#dccfbf;stroke-width:1.1;opacity:.92}.halo-soft{fill:none;stroke:#ece2d5;stroke-width:.8;opacity:.78}"
            ".title{fill:#211812;font:600 18px monospace}.subtitle{fill:#5a5046;font:10px monospace}.hash{fill:#2a221c;font:10px monospace}"
            ".core{fill:#fffaf2;stroke:#786856;stroke-width:1.6}.core-mark{fill:none;stroke:#a39383;stroke-width:1.0;opacity:.95}"
            ".route-shell{fill:rgba(255,252,246,0.74);stroke:#5a4b3f;stroke-width:1.0}.route-mid{fill:none;stroke:#b9aa96;stroke-width:.9;opacity:.8}"
            ".route-arc{fill:none;stroke:#736252;stroke-width:.92;opacity:.86;stroke-linecap:round}.route-spoke{fill:none;stroke:#968471;stroke-width:.86;opacity:.8;stroke-linecap:round}"
            ".route-hit{fill:#5e4d3d}.route-hit-stroke{fill:none;stroke:#f3e7d7;stroke-width:1.0;opacity:.92;stroke-linejoin:round}"
            ".span-link{fill:none;stroke:#776757;stroke-width:1.0;opacity:.72}.route-flow{fill:none;stroke:#69584b;stroke-width:.95;opacity:.58}"
            ".span-node{fill:#fbfaf6;stroke:#6c6256;stroke-width:1.0}.span-hit{stroke:#5e4d3d;stroke-width:1.55}.span-core{fill:#efe5d6;stroke:#5e5143;stroke-width:1.1}.trace-orn{fill:none;stroke:#b9a895;stroke-width:.9;opacity:.62}"
            ".node-wrap,.edge-wrap{cursor:help}"
            "]]></style>\n");
    fprintf(f, "<rect class=\"bg\" x=\"0\" y=\"0\" width=\"%.0f\" height=\"%.0f\"/>\n", width, height);
    fprintf(f, "<circle class=\"halo-soft\" cx=\"%.2f\" cy=\"%.2f\" r=\"300\"/>\n", cx, cy);
    fprintf(f, "<circle class=\"halo\" cx=\"%.2f\" cy=\"%.2f\" r=\"228\"/>\n", cx, cy);
    fprintf(f, "<circle class=\"halo-soft\" cx=\"%.2f\" cy=\"%.2f\" r=\"154\"/>\n", cx, cy);
    fprintf(f, "<text class=\"title\" x=\"24\" y=\"30\">trace %s</text>\n", doc.trace_id ? doc.trace_id : "<unknown>");
    fprintf(f, "<text class=\"subtitle\" x=\"24\" y=\"48\">spans=%zu max_depth=%d hash=%016llx</text>\n",
            doc.count, max_depth, (unsigned long long)trace_hash);
    fprintf(f, "<g id=\"trace_core\"><circle class=\"core\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/><circle class=\"core-mark\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>"
               "<path class=\"core-mark\" d=\"M %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f Z\"/></g>\n",
            cx, cy, core_r, cx, cy, core_r - 17.0,
            cx, cy - 24.0, cx + 21.0, cy, cx, cy + 24.0, cx - 21.0, cy);

    fprintf(f, "<g id=\"trace_ornaments\">\n");
    for (u32 oi = 0; oi < 12; oi++) {
        double a = (2.0 * CCT_TRACE_PI) * (double)oi / 12.0;
        a += (ctrace_hash_unit(trace_hash, 500 + oi) - 0.5) * 0.34;
        double rr = 274.0 + (ctrace_hash_unit(trace_hash, 540 + oi) - 0.5) * 36.0;
        double ox = cx + cos(a) * rr;
        double oy = cy + sin(a) * rr;
        double rad = 1.2 + ctrace_hash_unit(trace_hash, 580 + oi) * 2.0;
        fprintf(f, "<circle class=\"trace-orn\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", ox, oy, rad);
    }
    fprintf(f, "</g>\n");

    if (route_hit_count > 0) {
        fprintf(f, "<g id=\"trace_route_hits\">\n");
        for (size_t i = 0; i < route_hit_count; i++) {
            const cct_sigil_web_route_t *route = route_hits[i];
            double a = (-CCT_TRACE_PI * 0.5) + ((double)i * (2.0 * CCT_TRACE_PI) / (double)route_hit_count);
            a += (ctrace_hash_unit(trace_hash, 700 + (u32)i) - 0.5) * 0.22;
            double rr = 252.0 + (ctrace_hash_unit(trace_hash, 740 + (u32)i) - 0.5) * 18.0;
            double rx = cx + cos(a) * rr;
            double ry = cy + sin(a) * rr;
            route_hit_x[i] = rx;
            route_hit_y[i] = ry;
            fprintf(f, "<g class=\"node-wrap\" data-route-id=\"");
            ctrace_svg_escape(f, route->route_id ? route->route_id : "");
            fprintf(f, "\">");
            ctrace_emit_route_hit_sigil(f, route, rx, ry, 10.0);
            fprintf(f, "<title>route ");
            ctrace_svg_escape(f, route->route_id ? route->route_id : "");
            fprintf(f, " → ");
            ctrace_svg_escape(f, route->method ? route->method : "");
            fprintf(f, " ");
            ctrace_svg_escape(f, route->path ? route->path : "");
            fprintf(f, "</title></g>\n");
            if (i > 0) {
                double mx = (route_hit_x[i - 1] + rx) * 0.5 + cos(a) * 14.0;
                double my = (route_hit_y[i - 1] + ry) * 0.5 + sin(a) * 14.0;
                fprintf(f, "<path class=\"route-flow\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\"/>\n",
                        route_hit_x[i - 1], route_hit_y[i - 1], mx, my, rx, ry);
            }
        }
        fprintf(f, "</g>\n");
    }

    fprintf(f, "<g id=\"trace_links\">\n");
    for (size_t i = 0; i < doc.count; i++) {
        doc.items[i].duration_ms = doc.items[i].duration_ms;
        if (doc.items[i].parent_index >= 0) {
            int p = doc.items[i].parent_index;
            double px = x_pos[p];
            double py = y_pos[p];
            double x = x_pos[i];
            double y = y_pos[i];
            double angle = atan2(y - py, x - px);
            double mx = (px + x) * 0.5 + cos(angle + CCT_TRACE_PI * 0.5) * 18.0;
            double my = (py + y) * 0.5 + sin(angle + CCT_TRACE_PI * 0.5) * 18.0;
            if (!isfinite(mx) || !isfinite(my)) {
                mx = (px + x) * 0.5;
                my = (py + y) * 0.5;
            }
            fprintf(f, "<path class=\"span-link\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\"/>", px, py, mx, my, x, y);
        }
        if (route_hit_count > 0 && doc.items[i].route_id) {
            for (size_t r = 0; r < route_hit_count; r++) {
                const cct_sigil_web_route_t *route = route_hits[r];
                if (!route || !route->route_id) continue;
                if (strcmp(route->route_id, doc.items[i].route_id) != 0) continue;
                fprintf(f, "<path class=\"route-flow\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\"/>",
                        route_hit_x[r], route_hit_y[r],
                        (route_hit_x[r] + x_pos[i]) * 0.5 + 18.0,
                        (route_hit_y[r] + y_pos[i]) * 0.5 - 12.0,
                        x_pos[i], y_pos[i]);
                break;
            }
        }
    }
    fprintf(f, "</g>\n");

    fprintf(f, "<g id=\"trace_nodes\">\n");
    for (size_t i = 0; i < doc.count; i++) {
        double x = x_pos[i];
        double y = y_pos[i];
        double node_r = node_radius[i];
        bool hit = has_sigil && doc.items[i].route_id && sigilo_find_route_by_id(&sigil_doc, doc.items[i].route_id);
        fprintf(f, "<g class=\"node-wrap\"");
        if (doc.items[i].route_id) {
            fprintf(f, " data-route-id=\"");
            ctrace_svg_escape(f, doc.items[i].route_id);
            fprintf(f, "\"");
        }
        fprintf(f, ">");
        fprintf(f, "<circle class=\"%s%s\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>",
                depths[i] == 0 ? "span-core" : "span-node",
                hit ? " span-hit" : "",
                x, y, node_r);
        if (hit) {
            double marker_a = ctrace_hash_unit(trace_hash, 1300 + (u32)i) * (2.0 * CCT_TRACE_PI);
            fprintf(f, "<circle class=\"route-hit\" cx=\"%.2f\" cy=\"%.2f\" r=\"4.5\"/>",
                    x + cos(marker_a) * (node_r + 6.0), y + sin(marker_a) * (node_r + 6.0));
        }
        fprintf(f, "<title>");
        ctrace_svg_escape(f, doc.items[i].name ? doc.items[i].name : "");
        fprintf(f, " (%lldms)", doc.items[i].duration_ms);
        if (doc.items[i].attributes_json && doc.items[i].attributes_json[0] != '\0' && strcmp(doc.items[i].attributes_json, "{}") != 0) {
            fprintf(f, "\n");
            ctrace_svg_escape(f, doc.items[i].attributes_json);
        }
        fprintf(f, "</title></g>\n");
    }
    fprintf(f, "</g>\n");
    fprintf(f, "<text class=\"hash\" x=\"930\" y=\"704\" text-anchor=\"end\">%016llx</text>\n", (unsigned long long)trace_hash);
    fputs("</svg>\n", f);
    fclose(f);
    free(depths);
    free(depth_totals);
    free(depth_seen);
    free(route_hits);
    free(route_hit_x);
    free(route_hit_y);
    free(x_pos);
    free(y_pos);
    free(node_radius);
    free(derived);
    if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
    ctrace_document_dispose(&doc);
    return true;
}

static int cmd_sigilo_trace_tools(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  cct sigilo trace view <artifact.ctrace> [--summary] [--strict] [--show-attrs] [--max-depth N]\n");
        fprintf(stderr, "  cct sigilo trace export <artifact.ctrace> [--sigil path.sigil] [--format svg] [--output path.svg] [--strict]\n");
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }

    const char *sub = argv[3];
    const char *path = argv[4];
    if (!has_ctrace_extension(path)) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "trace input must have .ctrace extension: %s", path);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    bool strict = false;
    bool summary = false;
    bool show_attrs = false;
    int max_depth = -1;
    const char *sigil_path = NULL;
    const char *output_path = NULL;
    const char *format = "svg";

    for (int i = 5; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--strict") == 0) {
            strict = true;
            continue;
        }
        if (strcmp(arg, "--summary") == 0) {
            summary = true;
            continue;
        }
        if (strcmp(arg, "--show-attrs") == 0) {
            show_attrs = true;
            continue;
        }
        if (strcmp(arg, "--max-depth") == 0 && i + 1 < argc) {
            max_depth = atoi(argv[++i]);
            continue;
        }
        if (strcmp(arg, "--sigil") == 0 && i + 1 < argc) {
            sigil_path = argv[++i];
            continue;
        }
        if (strcmp(arg, "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
            continue;
        }
        if (strcmp(arg, "--format") == 0 && i + 1 < argc) {
            format = argv[++i];
            continue;
        }
        cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace option: %s", arg);
        return (int)CCT_ERROR_UNKNOWN_COMMAND;
    }

    if (strcmp(sub, "view") == 0) {
        return cmd_sigilo_trace_view(path, strict, summary, show_attrs, max_depth);
    }
    if (strcmp(sub, "export") == 0) {
        if (strcmp(format, "svg") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "sigilo trace export currently supports only --format svg");
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        return cmd_sigilo_trace_export_svg(path, sigil_path, output_path, strict) ? 0 : (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace subcommand: %s", sub);
    return (int)CCT_ERROR_UNKNOWN_COMMAND;
}

static int cmd_sigilo_baseline_tools(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  cct sigilo baseline check <artifact.sigil> [--baseline <path>] [--format text|structured] [--summary] [--strict] [--explain]\n");
        fprintf(stderr, "  cct sigilo baseline update <artifact.sigil> [--baseline <path>] [--force]\n");
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }

    const char *sub = argv[3];
    bool is_check = strcmp(sub, "check") == 0;
    bool is_update = strcmp(sub, "update") == 0;
    if (!is_check && !is_update) {
        cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo baseline subcommand: %s", sub);
        return (int)CCT_ERROR_UNKNOWN_COMMAND;
    }

    const char *artifact = argv[4];
    if (!has_sigil_extension(artifact)) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "Input artifact must have .sigil extension: %s", artifact);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    const char *baseline_override = NULL;
    bool strict = false;
    bool summary = true;
    bool structured = false;
    bool force = false;
    bool explain = false;

    for (int i = 5; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--baseline") == 0) {
            if (i + 1 >= argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT, "--baseline requires a path");
                return (int)CCT_ERROR_MISSING_ARGUMENT;
            }
            baseline_override = argv[++i];
            continue;
        }
        if (strcmp(arg, "--summary") == 0) {
            summary = true;
            continue;
        }
        if (strcmp(arg, "--strict") == 0) {
            if (!is_check) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "--strict is only valid for sigilo baseline check");
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            strict = true;
            continue;
        }
        if (strcmp(arg, "--explain") == 0) {
            if (!is_check) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "--explain is only valid for sigilo baseline check");
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            explain = true;
            continue;
        }
        if (strcmp(arg, "--force") == 0) {
            if (!is_update) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "--force is only valid for sigilo baseline update");
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            force = true;
            continue;
        }
        if (strcmp(arg, "--format") == 0) {
            if (!is_check) {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "--format is only valid for sigilo baseline check");
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            if (i + 1 >= argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT, "--format requires text|structured");
                return (int)CCT_ERROR_MISSING_ARGUMENT;
            }
            const char *fmt = argv[++i];
            if (strcmp(fmt, "text") == 0) {
                structured = false;
            } else if (strcmp(fmt, "structured") == 0) {
                structured = true;
            } else {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "Invalid --format value: %s", fmt);
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            continue;
        }
        cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo baseline option: %s", arg);
        return (int)CCT_ERROR_UNKNOWN_COMMAND;
    }

    cct_sigil_parse_mode_t parse_mode = strict ? CCT_SIGIL_PARSE_MODE_STRICT : CCT_SIGIL_PARSE_MODE_TOLERANT;
    cct_sigil_document_t artifact_doc;
    if (!cct_sigil_parse_file(artifact, parse_mode, &artifact_doc)) {
        print_sigil_diagnostics(&artifact_doc);
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 "artifact could not be parsed under the selected profile",
                                 "run `cct sigilo validate <artifact> --strict --format structured` and fix listed diagnostics",
                                 true);
        }
        cct_sigil_document_dispose(&artifact_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (cct_sigil_document_has_errors(&artifact_doc)) {
        print_sigil_diagnostics(&artifact_doc);
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 "artifact violates sigilo schema contract",
                                 "fix required fields/type consistency then rerun baseline check",
                                 true);
        }
        cct_sigil_document_dispose(&artifact_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    char *baseline_path = NULL;
    char *baseline_error = NULL;
    if (!cct_sigilo_baseline_resolve_path(&artifact_doc, baseline_override, &baseline_path, &baseline_error)) {
        cct_error_printf(CCT_ERROR_INTERNAL,
                         "%s",
                         baseline_error ? baseline_error : "could not resolve baseline path");
        free(baseline_error);
        cct_sigil_document_dispose(&artifact_doc);
        return (int)CCT_ERROR_INTERNAL;
    }

    if (is_update) {
        char *meta_path = NULL;
        if (!cct_sigilo_baseline_write(artifact, &artifact_doc, baseline_path, force, &meta_path, &baseline_error)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                             "%s",
                             baseline_error ? baseline_error : "could not update sigilo baseline");
            free(baseline_error);
            free(meta_path);
            free(baseline_path);
            cct_sigil_document_dispose(&artifact_doc);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }

        printf("sigilo.baseline.update status=written baseline=%s meta=%s scope=%s\n",
               baseline_path,
               meta_path ? meta_path : "<none>",
               artifact_doc.sigilo_scope ? artifact_doc.sigilo_scope : "<none>");

        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        return 0;
    }

    if (!cct_sigilo_baseline_file_exists(baseline_path)) {
        if (structured) {
            printf("format = cct.sigil.baseline.check.v1\n");
            printf("status = missing\n");
            printf("baseline_path = %s\n", baseline_path);
            printf("current_path = %s\n", artifact);
            printf("strict = %s\n", strict ? "true" : "false");
        } else {
            printf("sigilo.baseline.check status=missing baseline=%s current=%s\n",
                   baseline_path,
                   artifact);
        }
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 "baseline file is missing",
                                 "create baseline via `cct sigilo baseline update <artifact> --baseline <path>`",
                                 strict);
        }
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        return 0;
    }

    char *meta_path = NULL;
    bool meta_exists = false;
    if (!cct_sigilo_baseline_compute_meta_path(baseline_path, &meta_path, &baseline_error)) {
        cct_error_printf(CCT_ERROR_INTERNAL,
                         "%s",
                         baseline_error ? baseline_error : "could not resolve baseline metadata path");
        free(baseline_error);
        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        return (int)CCT_ERROR_INTERNAL;
    }
    if (!cct_sigilo_baseline_validate_meta(meta_path, &meta_exists, &baseline_error)) {
        cct_error_printf(CCT_ERROR_INVALID_ARGUMENT,
                         "%s",
                         baseline_error ? baseline_error : "invalid baseline metadata");
        free(baseline_error);
        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_sigil_document_t baseline_doc;
    if (!cct_sigil_parse_file(baseline_path, parse_mode, &baseline_doc)) {
        print_sigil_diagnostics(&baseline_doc);
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 "baseline could not be parsed under the selected profile",
                                 "repair/regenerate baseline then rerun check",
                                 true);
        }
        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (cct_sigil_document_has_errors(&baseline_doc)) {
        print_sigil_diagnostics(&baseline_doc);
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 "baseline violates sigilo schema contract",
                                 "update baseline from a valid artifact after review approval",
                                 true);
        }
        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_sigil_diff_result_t diff;
    if (!cct_sigil_diff_documents(&baseline_doc, &artifact_doc, &diff)) {
        free(meta_path);
        free(baseline_path);
        cct_sigil_document_dispose(&artifact_doc);
        cct_sigil_document_dispose(&baseline_doc);
        return (int)CCT_ERROR_INTERNAL;
    }

    const char *status = (diff.count == 0) ? "ok" : "drift";
    if (structured) {
        printf("format = cct.sigil.baseline.check.v1\n");
        printf("status = %s\n", status);
        printf("baseline_path = %s\n", baseline_path);
        printf("current_path = %s\n", artifact);
        printf("meta_path = %s\n", meta_path ? meta_path : "");
        printf("meta_exists = %s\n", meta_exists ? "true" : "false");
        printf("strict = %s\n", strict ? "true" : "false");
        printf("\n");
        if (!cct_sigil_diff_render_structured(stdout, &diff)) {
            cct_sigil_diff_result_dispose(&diff);
            free(meta_path);
            free(baseline_path);
            cct_sigil_document_dispose(&artifact_doc);
            cct_sigil_document_dispose(&baseline_doc);
            return (int)CCT_ERROR_INTERNAL;
        }
    } else {
        printf("sigilo.baseline.check status=%s baseline=%s current=%s meta=%s\n",
               status,
               baseline_path,
               artifact,
               meta_exists ? "present" : "missing");
        if (!cct_sigil_diff_render_text(stdout, &diff, summary)) {
            cct_sigil_diff_result_dispose(&diff);
            free(meta_path);
            free(baseline_path);
            cct_sigil_document_dispose(&artifact_doc);
            cct_sigil_document_dispose(&baseline_doc);
            return (int)CCT_ERROR_INTERNAL;
        }
    }

    int rc = 0;
    if (strict && diff.highest_severity >= CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
        rc = (int)CCT_ERROR_CONTRACT_VIOLATION;
        if (!structured) {
            fprintf(stderr,
                    "cct: sigilo baseline check blocked (strict mode). action: review drift and update baseline explicitly with 'cct sigilo baseline update %s --baseline %s --force' when approved.\n",
                    artifact,
                    baseline_path);
        } else {
            fprintf(stderr,
                    "cct: sigilo baseline check blocked (strict mode). action: update baseline only after review approval.\n");
        }
        if (explain) {
            sigilo_print_explain("baseline-check",
                                 diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK
                                     ? "critical metadata drift (format/scope contract) was detected"
                                     : "review-required metadata drift (hash/topology contract) was detected",
                                 "review diff and update baseline explicitly with `cct sigilo baseline update <artifact> --baseline <path> --force` only when approved",
                                 true);
        }
    } else if (explain && diff.count > 0) {
        sigilo_print_explain("baseline-check",
                             "informational metadata drift was detected",
                             "record accepted drift and refresh baseline when appropriate",
                             false);
    }

    cct_sigil_diff_result_dispose(&diff);
    free(meta_path);
    free(baseline_path);
    cct_sigil_document_dispose(&artifact_doc);
    cct_sigil_document_dispose(&baseline_doc);
    return rc;
}

static int cmd_sigilo_tools(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  cct sigilo inspect <artifact.sigil> [--format text|structured] [--summary] [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract] [--explain]\n");
        fprintf(stderr, "  cct sigilo validate <artifact.sigil> [--format text|structured] [--summary] [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract] [--explain]\n");
        fprintf(stderr, "  cct sigilo diff <left.sigil> <right.sigil> [--format text|structured] [--summary] [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract] [--explain]\n");
        fprintf(stderr, "  cct sigilo check <left.sigil> <right.sigil> [--format text|structured] [--summary] [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract] [--explain]\n");
        fprintf(stderr, "  cct sigilo trace view <artifact.ctrace> [--summary] [--strict] [--show-attrs] [--max-depth N]\n");
        fprintf(stderr, "  cct sigilo trace export <artifact.ctrace> [--sigil path.sigil] [--format svg] [--output path.svg] [--strict]\n");
        fprintf(stderr, "  cct sigilo baseline check <artifact.sigil> [--baseline <path>] [--format text|structured] [--summary] [--strict] [--explain]\n");
        fprintf(stderr, "  cct sigilo baseline update <artifact.sigil> [--baseline <path>] [--force]\n");
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }

    const char *sub = argv[2];
    if (strcmp(sub, "trace") == 0) {
        return cmd_sigilo_trace_tools(argc, argv);
    }
    if (strcmp(sub, "baseline") == 0) {
        return cmd_sigilo_baseline_tools(argc, argv);
    }
    bool is_inspect = strcmp(sub, "inspect") == 0;
    bool is_validate = strcmp(sub, "validate") == 0;
    bool is_diff = strcmp(sub, "diff") == 0;
    bool is_check = strcmp(sub, "check") == 0;
    if (!is_inspect && !is_validate && !is_diff && !is_check) {
        cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo subcommand: %s", sub);
        return (int)CCT_ERROR_UNKNOWN_COMMAND;
    }

    bool structured = false;
    bool summary = is_check;
    bool strict = false;
    bool explain = false;
    cct_sigil_parse_mode_t parse_mode = CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT;

    const char *left = NULL;
    const char *right = NULL;
    int i = 3;
    if (is_inspect || is_validate) {
        left = argv[i++];
        if (!has_sigil_extension(left)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "Input artifact must have .sigil extension: %s", left);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
    } else {
        if (argc < 5) {
            cct_error_printf(CCT_ERROR_MISSING_ARGUMENT, "sigilo %s requires two .sigil paths", sub);
            return (int)CCT_ERROR_MISSING_ARGUMENT;
        }
        left = argv[i++];
        right = argv[i++];
        if (!has_sigil_extension(left) || !has_sigil_extension(right)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "sigilo %s expects .sigil inputs", sub);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
    }

    for (; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--summary") == 0) {
            summary = true;
            continue;
        }
        if (strcmp(arg, "--strict") == 0) {
            strict = true;
            continue;
        }
        if (strcmp(arg, "--explain") == 0) {
            explain = true;
            continue;
        }
        if (strcmp(arg, "--consumer-profile") == 0) {
            if (i + 1 >= argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT, "--consumer-profile requires legacy-tolerant|current-default|strict-contract");
                return (int)CCT_ERROR_MISSING_ARGUMENT;
            }
            const char *profile = argv[++i];
            if (strcmp(profile, "legacy-tolerant") == 0) {
                parse_mode = CCT_SIGIL_PARSE_MODE_LEGACY_TOLERANT;
            } else if (strcmp(profile, "current-default") == 0) {
                parse_mode = CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT;
            } else if (strcmp(profile, "strict-contract") == 0) {
                parse_mode = CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT;
            } else {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "Invalid --consumer-profile value: %s", profile);
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            continue;
        }
        if (strcmp(arg, "--format") == 0) {
            if (i + 1 >= argc) {
                cct_error_printf(CCT_ERROR_MISSING_ARGUMENT, "--format requires text|structured");
                return (int)CCT_ERROR_MISSING_ARGUMENT;
            }
            const char *fmt = argv[++i];
            if (strcmp(fmt, "text") == 0) {
                structured = false;
            } else if (strcmp(fmt, "structured") == 0) {
                structured = true;
            } else {
                cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "Invalid --format value: %s", fmt);
                return (int)CCT_ERROR_INVALID_ARGUMENT;
            }
            continue;
        }
        cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo option: %s", arg);
        return (int)CCT_ERROR_UNKNOWN_COMMAND;
    }

    if (strict) parse_mode = CCT_SIGIL_PARSE_MODE_STRICT_CONTRACT;
    cct_sigil_document_t left_doc;
    bool left_parsed = cct_sigil_parse_file(left, parse_mode, &left_doc);
    bool left_has_errors = cct_sigil_document_has_errors(&left_doc);

    if (is_validate) {
        size_t errors = 0;
        for (size_t d = 0; d < left_doc.diagnostics.count; d++) {
            if (left_doc.diagnostics.items[d].level == CCT_SIGIL_DIAG_ERROR) errors++;
        }
        int rc = structured
            ? sigilo_emit_validate_structured(&left_doc, parse_mode, summary)
            : sigilo_emit_validate_text(&left_doc, parse_mode, summary);
        if (errors > 0 || !left_parsed || left_has_errors) rc = (int)CCT_ERROR_CONTRACT_VIOLATION;
        if (explain && rc != 0) {
            sigilo_print_explain("validate",
                                 "artifact violates schema/consistency contract",
                                 "fix the reported diagnostics and rerun `cct sigilo validate <artifact> --strict`",
                                 true);
        }
        cct_sigil_document_dispose(&left_doc);
        return rc;
    }
    if (!left_parsed || left_has_errors) {
        print_sigil_diagnostics(&left_doc);
        if (explain) {
            sigilo_print_explain("inspect",
                                 "artifact could not be parsed cleanly",
                                 "run validation in strict mode and correct diagnostics first",
                                 true);
        }
        cct_sigil_document_dispose(&left_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    if (is_inspect) {
        int rc = structured
            ? sigilo_emit_inspect_structured(&left_doc, summary)
            : sigilo_emit_inspect_text(&left_doc, summary);
        cct_sigil_document_dispose(&left_doc);
        return rc;
    }
    cct_sigil_document_t right_doc;
    if (!cct_sigil_parse_file(right, parse_mode, &right_doc)) {
        print_sigil_diagnostics(&right_doc);
        if (explain) {
            sigilo_print_explain(is_check ? "check" : "diff",
                                 "right artifact could not be parsed",
                                 "validate both artifacts before diff/check",
                                 true);
        }
        cct_sigil_document_dispose(&left_doc);
        cct_sigil_document_dispose(&right_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (cct_sigil_document_has_errors(&right_doc)) {
        print_sigil_diagnostics(&right_doc);
        if (explain) {
            sigilo_print_explain(is_check ? "check" : "diff",
                                 "right artifact has schema errors",
                                 "fix right artifact diagnostics then rerun diff/check",
                                 true);
        }
        cct_sigil_document_dispose(&left_doc);
        cct_sigil_document_dispose(&right_doc);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_sigil_diff_result_t diff;
    if (!cct_sigil_diff_documents(&left_doc, &right_doc, &diff)) {
        cct_sigil_document_dispose(&left_doc);
        cct_sigil_document_dispose(&right_doc);
        return (int)CCT_ERROR_INTERNAL;
    }

    bool rendered = structured
        ? cct_sigil_diff_render_structured(stdout, &diff)
        : cct_sigil_diff_render_text(stdout, &diff, summary);
    if (!rendered) {
        cct_sigil_diff_result_dispose(&diff);
        cct_sigil_document_dispose(&left_doc);
        cct_sigil_document_dispose(&right_doc);
        return (int)CCT_ERROR_INTERNAL;
    }

    int rc = 0;
    if (strict && diff.highest_severity >= CCT_SIGIL_DIFF_SEVERITY_REVIEW_REQUIRED) {
        rc = (int)CCT_ERROR_CONTRACT_VIOLATION;
        if (explain) {
            sigilo_print_explain(is_check ? "check" : "diff",
                                 diff.highest_severity == CCT_SIGIL_DIFF_SEVERITY_BEHAVIORAL_RISK
                                     ? "critical metadata drift (format/scope contract) was detected"
                                     : "review-required metadata drift (hash/topology contract) was detected",
                                 "review diff output and update approved baseline/artifact accordingly",
                                 true);
        }
    } else if (explain && diff.count > 0) {
        sigilo_print_explain(is_check ? "check" : "diff",
                             "informational metadata drift was detected",
                             "record drift context and accept/update artifacts if intended",
                             false);
    }

    cct_sigil_diff_result_dispose(&diff);
    cct_sigil_document_dispose(&left_doc);
    cct_sigil_document_dispose(&right_doc);
    return rc;
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
        return (int)CCT_ERROR_CONTRACT_VIOLATION;
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
    if (!cct_module_bundle_build(file, CCT_PROFILE_HOST, &bundle, &bundle_status)) {
        return 1;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, file, CCT_PROFILE_HOST);
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
    if (!cct_module_bundle_build(filename, CCT_PROFILE_HOST, &bundle, &bundle_status)) {
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
static cct_error_code_t cmd_check_semantic(const char *filename, cct_profile_t profile) {
    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, profile, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename, profile);

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
    sg->emit_titles = args->sigilo_emit_titles;
    sg->emit_data_attrs = args->sigilo_emit_data_attrs;
    sg->manifest_path = args->sigilo_manifest_path;
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
    if (!cct_module_bundle_build(filename, args->profile, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename, args->profile);
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
    cct_profile_t compile_profile = args->profile;
    if (args->emit_asm && compile_profile != CCT_PROFILE_FREESTANDING) {
        fprintf(stderr, "cct: aviso: --emit-asm força perfil freestanding\n");
        compile_profile = CCT_PROFILE_FREESTANDING;
    }

    if (args->entry_rituale && compile_profile != CCT_PROFILE_FREESTANDING) {
        fprintf(stderr, "cct: --entry é suportado apenas em perfil freestanding\n");
        return CCT_ERROR_INVALID_ARGUMENT;
    }

    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(filename, compile_profile, &bundle, &bundle_status)) {
        return bundle_status;
    }

    cct_semantic_analyzer_t sem;
    cct_semantic_init(&sem, filename, compile_profile);
    bool sem_ok = cct_semantic_analyze_program(&sem, bundle.program);

    if (!sem_ok || cct_semantic_had_error(&sem)) {
        cct_semantic_dispose(&sem);
        cct_module_bundle_dispose(&bundle);
        return CCT_ERROR_SEMANTIC;
    }

    cct_error_code_t sigilo_status = generate_sigilo_artifacts_for_bundle(args, &bundle);

    cct_codegen_t cg;
    cct_codegen_init(&cg, filename);
    cg.profile = compile_profile;
    cg.entry_rituale_name = args->entry_rituale;
    bool cg_ok = false;
    bool cg_had_error = false;
    cct_error_code_t asm_status = CCT_OK;
    if (sigilo_status == CCT_OK) {
        cg_ok = cct_codegen_compile_program(&cg, bundle.program, filename, NULL);
        cg_had_error = cct_codegen_had_error(&cg);
        if (cg_ok && !cg_had_error && args->emit_asm) {
            asm_status = cct_emit_asm_from_cgen(filename);
        }
    }
    cct_codegen_dispose(&cg);

    cct_semantic_dispose(&sem);
    cct_module_bundle_dispose(&bundle);

    if (sigilo_status != CCT_OK || !cg_ok || cg_had_error || asm_status != CCT_OK) {
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

    if (argc >= 2 && strcmp(argv[1], "sigilo") == 0) {
        return cmd_sigilo_tools(argc, argv);
    }
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
            return (int)cmd_check_semantic(args.input_file, args.profile);

        case CCT_CMD_SIGILO_ONLY:
            return (int)cmd_sigilo_only(&args);

        case CCT_CMD_NONE:
        default:
            cct_cli_show_help();
            return EXIT_SUCCESS;
    }
}
