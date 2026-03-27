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
#include "sigilo/trace_render.h"
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

static void trace_view_emit_span(const TraceRenderTrace *trace, int idx, int depth, int max_depth, bool show_attrs) {
    int i = 0;
    if (!trace || idx < 0 || idx >= trace->span_count) return;
    if (max_depth >= 0 && depth > max_depth) return;
    for (i = 0; i < depth; i++) printf("    ");
    printf("%s%s (%lldms)\n", depth > 0 ? "└── " : "", trace->spans[idx].name ? trace->spans[idx].name : "<span>", trace->spans[idx].duration_us);
    if (show_attrs && trace->spans[idx].attributes_json && trace->spans[idx].attributes_json[0] != '\0' &&
        strcmp(trace->spans[idx].attributes_json, "{}") != 0) {
        for (i = 0; i < depth + 1; i++) printf("    ");
        printf("attrs=%s\n", trace->spans[idx].attributes_json);
    }
    for (i = 0; i < trace->span_count; i++) {
        if (trace->spans[i].parent_index == idx) trace_view_emit_span(trace, i, depth + 1, max_depth, show_attrs);
    }
}

static int cmd_sigilo_trace_view(const char *path, bool strict, bool summary, bool show_attrs, int max_depth) {
    TraceRenderTrace trace;
    int rc = 0;
    int i = 0;
    long long total = 0;
    if (trace_render_load_ctrace(path, &trace) != 0) return (int)CCT_ERROR_INVALID_ARGUMENT;
    if (strict && trace.errors > 0) {
        trace_render_free(&trace);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    for (i = 0; i < trace.span_count; i++) if (trace.spans[i].duration_us > total) total = trace.spans[i].duration_us;
    printf("trace %s spans=%d total=%lldms warnings=%zu errors=%zu\n",
           trace.trace_id ? trace.trace_id : "<unknown>",
           trace.span_count, total, trace.warnings, trace.errors);
    if (!summary) {
        for (i = 0; i < trace.span_count; i++) {
            if (trace.spans[i].parent_index < 0 || !trace.spans[i].parent_id || trace.spans[i].parent_id[0] == '\0') {
                trace_view_emit_span(&trace, i, 0, max_depth, show_attrs);
            }
        }
    }
    trace_render_free(&trace);
    return rc;
}

static int cmd_sigilo_trace_render_svg(const char *trace_a_path,
                                       const char *trace_b_path,
                                       const char *sigil_path,
                                       const char *output_path,
                                       bool strict,
                                       bool animated,
                                       int step_index,
                                       bool hide_timeline,
                                       const char *filter_kind,
                                       const char *focus_route) {
    TraceRenderTrace trace_a;
    TraceRenderTrace trace_b;
    TraceRenderInput input;
    const char *out_path = output_path;
    char *derived = NULL;
    FILE *out = NULL;
    int rc = 0;
    memset(&trace_a, 0, sizeof(trace_a));
    memset(&trace_b, 0, sizeof(trace_b));
    memset(&input, 0, sizeof(input));
    if (trace_render_load_ctrace(trace_a_path, &trace_a) != 0) {
        trace_render_free(&trace_a);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (strict && trace_a.errors > 0) {
        trace_render_free(&trace_a);
        return (int)CCT_ERROR_INVALID_ARGUMENT;
    }
    if (trace_b_path) {
        if (trace_render_load_ctrace(trace_b_path, &trace_b) != 0) {
            trace_render_free(&trace_a);
            trace_render_free(&trace_b);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        if (strict && trace_b.errors > 0) {
            trace_render_free(&trace_a);
            trace_render_free(&trace_b);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
    }
    if (!out_path || out_path[0] == '\0') {
        derived = replace_path_suffix(trace_a_path, ".ctrace", trace_b_path ? ".compare.svg" : ".svg");
        out_path = derived ? derived : (trace_b_path ? "trace.compare.svg" : "trace.svg");
    }
    out = fopen(out_path, "wb");
    if (!out) {
        free(derived);
        trace_render_free(&trace_a);
        trace_render_free(&trace_b);
        return (int)CCT_ERROR_FILE_WRITE;
    }
    input.mode = trace_b_path ? TRACE_RENDER_COMPARE : (step_index >= 0 ? TRACE_RENDER_STEP : (animated ? TRACE_RENDER_SINGLE : TRACE_RENDER_STATIC));
    input.trace_a = trace_a;
    input.trace_b = trace_b;
    input.sigil_path = sigil_path;
    input.step_index = step_index;
    input.animated = animated ? 1 : 0;
    input.hide_timeline = hide_timeline ? 1 : 0;
    input.filter_kind = filter_kind;
    input.focus_route = focus_route;
    if (trace_b_path) rc = trace_render_compare(&input, out);
    else if (step_index >= 0) rc = trace_render_step(&input, out);
    else rc = trace_render_single(&input, out);
    fclose(out);
    free(derived);
    trace_render_free(&trace_a);
    trace_render_free(&trace_b);
    if (rc == 1) return (int)CCT_ERROR_INVALID_ARGUMENT;
    return rc == 0 ? 0 : (int)CCT_ERROR_INVALID_ARGUMENT;
}

static int cmd_sigilo_trace_tools(int argc, char **argv) {
    const char *sub = NULL;
    const char *trace_a_path = NULL;
    const char *trace_b_path = NULL;
    const char *sigil_path = NULL;
    const char *output_path = NULL;
    const char *format = "svg";
    const char *filter_kind = NULL;
    const char *focus_route = NULL;
    bool strict = false;
    bool summary = false;
    bool show_attrs = false;
    bool animated = false;
    bool hide_timeline = false;
    int max_depth = -1;
    int step_index = -1;

    if (argc < 4) {
        return (int)CCT_ERROR_MISSING_ARGUMENT;
    }
    sub = argv[3];
    if (strcmp(sub, "view") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  cct sigilo trace view <artifact.ctrace> [--summary] [--strict] [--show-attrs] [--max-depth N]\n");
            return (int)CCT_ERROR_MISSING_ARGUMENT;
        }
        trace_a_path = argv[4];
        if (!has_ctrace_extension(trace_a_path)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "trace input must have .ctrace extension: %s", trace_a_path);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        for (int i = 5; i < argc; i++) {
            const char *arg = argv[i];
            if (strcmp(arg, "--strict") == 0) strict = true;
            else if (strcmp(arg, "--summary") == 0) summary = true;
            else if (strcmp(arg, "--show-attrs") == 0) show_attrs = true;
            else if (strcmp(arg, "--max-depth") == 0 && i + 1 < argc) max_depth = atoi(argv[++i]);
            else {
                cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace option: %s", arg);
                return (int)CCT_ERROR_UNKNOWN_COMMAND;
            }
        }
        return cmd_sigilo_trace_view(trace_a_path, strict, summary, show_attrs, max_depth);
    }

    if (strcmp(sub, "render") == 0) {
        for (int i = 4; i < argc; i++) {
            const char *arg = argv[i];
            if (!trace_a_path && has_ctrace_extension(arg)) {
                trace_a_path = arg;
            } else if (strcmp(arg, "--trace") == 0 && i + 1 < argc) {
                trace_a_path = argv[++i];
            } else if (strcmp(arg, "--sigil") == 0 && i + 1 < argc) {
                sigil_path = argv[++i];
            } else if ((strcmp(arg, "--out") == 0 || strcmp(arg, "--output") == 0) && i + 1 < argc) {
                output_path = argv[++i];
            } else if (strcmp(arg, "--strict") == 0) {
                strict = true;
            } else if (strcmp(arg, "--animated") == 0) {
                animated = true;
            } else if (strcmp(arg, "--static") == 0) {
                animated = false;
            } else if (strcmp(arg, "--step") == 0 && i + 1 < argc) {
                step_index = atoi(argv[++i]);
            } else if (strcmp(arg, "--hide-timeline") == 0) {
                hide_timeline = true;
            } else if (strcmp(arg, "--filter-kind") == 0 && i + 1 < argc) {
                filter_kind = argv[++i];
            } else if (strcmp(arg, "--focus-route") == 0 && i + 1 < argc) {
                focus_route = argv[++i];
            } else {
                cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace render option: %s", arg);
                return (int)CCT_ERROR_UNKNOWN_COMMAND;
            }
        }
        if (!trace_a_path || !has_ctrace_extension(trace_a_path)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "sigilo trace render currently supports only .ctrace inputs");
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        return cmd_sigilo_trace_render_svg(trace_a_path, NULL, sigil_path, output_path, strict, animated, step_index, hide_timeline, filter_kind, focus_route);
    }

    if (strcmp(sub, "compare") == 0) {
        if (argc < 6) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  cct sigilo trace compare <trace_a.ctrace> <trace_b.ctrace> [--sigil path.sigil] [--out path.svg] [--strict]\n");
            return (int)CCT_ERROR_MISSING_ARGUMENT;
        }
        trace_a_path = argv[4];
        trace_b_path = argv[5];
        if (!has_ctrace_extension(trace_a_path) || !has_ctrace_extension(trace_b_path)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "trace compare inputs must have .ctrace extension");
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        for (int i = 6; i < argc; i++) {
            const char *arg = argv[i];
            if (strcmp(arg, "--sigil") == 0 && i + 1 < argc) sigil_path = argv[++i];
            else if ((strcmp(arg, "--out") == 0 || strcmp(arg, "--output") == 0) && i + 1 < argc) output_path = argv[++i];
            else if (strcmp(arg, "--strict") == 0) strict = true;
            else if (strcmp(arg, "--animated") == 0) animated = true;
            else if (strcmp(arg, "--static") == 0) animated = false;
            else if (strcmp(arg, "--hide-timeline") == 0) hide_timeline = true;
            else if (strcmp(arg, "--filter-kind") == 0 && i + 1 < argc) filter_kind = argv[++i];
            else if (strcmp(arg, "--focus-route") == 0 && i + 1 < argc) focus_route = argv[++i];
            else {
                cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace compare option: %s", arg);
                return (int)CCT_ERROR_UNKNOWN_COMMAND;
            }
        }
        return cmd_sigilo_trace_render_svg(trace_a_path, trace_b_path, sigil_path, output_path, strict, animated, -1, hide_timeline, filter_kind, focus_route);
    }

    if (strcmp(sub, "export") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  cct sigilo trace export <artifact.ctrace> [--sigil path.sigil] [--format svg] [--output path.svg] [--strict]\n");
            return (int)CCT_ERROR_MISSING_ARGUMENT;
        }
        trace_a_path = argv[4];
        if (!has_ctrace_extension(trace_a_path)) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "trace input must have .ctrace extension: %s", trace_a_path);
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        for (int i = 5; i < argc; i++) {
            const char *arg = argv[i];
            if (strcmp(arg, "--strict") == 0) strict = true;
            else if (strcmp(arg, "--sigil") == 0 && i + 1 < argc) sigil_path = argv[++i];
            else if ((strcmp(arg, "--output") == 0 || strcmp(arg, "--out") == 0) && i + 1 < argc) output_path = argv[++i];
            else if (strcmp(arg, "--format") == 0 && i + 1 < argc) format = argv[++i];
            else if (strcmp(arg, "--animated") == 0) animated = true;
            else if (strcmp(arg, "--static") == 0) animated = false;
            else if (strcmp(arg, "--hide-timeline") == 0) hide_timeline = true;
            else if (strcmp(arg, "--filter-kind") == 0 && i + 1 < argc) filter_kind = argv[++i];
            else if (strcmp(arg, "--focus-route") == 0 && i + 1 < argc) focus_route = argv[++i];
            else if (strcmp(arg, "--step") == 0 && i + 1 < argc) step_index = atoi(argv[++i]);
            else {
                cct_error_printf(CCT_ERROR_UNKNOWN_COMMAND, "Unknown sigilo trace export option: %s", arg);
                return (int)CCT_ERROR_UNKNOWN_COMMAND;
            }
        }
        if (strcmp(format, "svg") != 0) {
            cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "sigilo trace export currently supports only --format svg");
            return (int)CCT_ERROR_INVALID_ARGUMENT;
        }
        return cmd_sigilo_trace_render_svg(trace_a_path, NULL, sigil_path, output_path, strict, animated, step_index, hide_timeline, filter_kind, focus_route);
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
