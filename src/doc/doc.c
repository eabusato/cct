#include "doc.h"

#include "../common/errors.h"
#include "../lexer/lexer.h"
#include "../module/module.h"
#include "../parser/ast.h"
#include "../parser/parser.h"
#include "../project/project_discovery.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    DOC_FORMAT_MARKDOWN = 0,
    DOC_FORMAT_HTML,
    DOC_FORMAT_BOTH
} cct_doc_format_t;

typedef struct {
    const char *project_override;
    const char *entry_override;
    const char *output_dir_override;
    cct_doc_format_t format;
    bool include_internal;
    bool no_examples;
    bool warn_missing_docs;
    bool strict_docs;
    bool no_timestamp;
} cct_doc_options_t;

typedef struct {
    char *text;
    bool has_doc;
    int param_count;
    int return_count;
    int example_count;
    int invalid_tag_count;
} cct_doc_block_t;

typedef struct {
    char *id;
    char *path;
    bool is_stdlib;
} cct_doc_module_info_t;

typedef struct {
    char *id;
    char *name;
    char *kind;
    char *signature;
    char *module_id;
    char *module_path;
    cct_doc_block_t doc;
    bool is_internal;
} cct_doc_symbol_info_t;

typedef struct {
    cct_doc_module_info_t *items;
    size_t count;
    size_t capacity;
} cct_doc_module_list_t;

typedef struct {
    cct_doc_symbol_info_t *items;
    size_t count;
    size_t capacity;
} cct_doc_symbol_list_t;

static void doc_free_block(cct_doc_block_t *b) {
    if (!b) return;
    free(b->text);
    memset(b, 0, sizeof(*b));
}

static void doc_modules_dispose(cct_doc_module_list_t *mods) {
    if (!mods) return;
    for (size_t i = 0; i < mods->count; i++) {
        free(mods->items[i].id);
        free(mods->items[i].path);
    }
    free(mods->items);
    memset(mods, 0, sizeof(*mods));
}

static void doc_symbols_dispose(cct_doc_symbol_list_t *symbols) {
    if (!symbols) return;
    for (size_t i = 0; i < symbols->count; i++) {
        free(symbols->items[i].id);
        free(symbols->items[i].name);
        free(symbols->items[i].kind);
        free(symbols->items[i].signature);
        free(symbols->items[i].module_id);
        free(symbols->items[i].module_path);
        doc_free_block(&symbols->items[i].doc);
    }
    free(symbols->items);
    memset(symbols, 0, sizeof(*symbols));
}

static bool doc_push_module(cct_doc_module_list_t *mods, cct_doc_module_info_t item) {
    if (mods->count == mods->capacity) {
        size_t next = mods->capacity == 0 ? 8 : mods->capacity * 2;
        cct_doc_module_info_t *grown = (cct_doc_module_info_t *)realloc(mods->items, next * sizeof(*mods->items));
        if (!grown) return false;
        mods->items = grown;
        mods->capacity = next;
    }
    mods->items[mods->count++] = item;
    return true;
}

static bool doc_push_symbol(cct_doc_symbol_list_t *symbols, cct_doc_symbol_info_t item) {
    if (symbols->count == symbols->capacity) {
        size_t next = symbols->capacity == 0 ? 16 : symbols->capacity * 2;
        cct_doc_symbol_info_t *grown = (cct_doc_symbol_info_t *)realloc(symbols->items, next * sizeof(*symbols->items));
        if (!grown) return false;
        symbols->items = grown;
        symbols->capacity = next;
    }
    symbols->items[symbols->count++] = item;
    return true;
}

static bool doc_has_cct_extension(const char *filename) {
    if (!filename) return false;
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".cct") == 0;
}

static bool doc_ensure_dir(const char *path) {
    if (!path || path[0] == '\0') return false;
    if (cct_project_path_is_dir(path)) return true;

    char tmp[CCT_PROJECT_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (tmp[0] != '\0' && !cct_project_path_is_dir(tmp)) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
            }
            *p = '/';
        }
    }

    if (!cct_project_path_is_dir(tmp)) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
    }

    return true;
}

static char *doc_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static char *doc_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

static void doc_trim(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
}

static const char *doc_basename(const char *path) {
    const char *p = strrchr(path, '/');
    return p ? (p + 1) : path;
}

static char *doc_sanitize_id(const char *raw) {
    if (!raw) return doc_strdup("id");
    size_t n = strlen(raw);
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)raw[i];
        if (isalnum(c) || c == '_') {
            out[i] = (char)tolower(c);
        } else {
            out[i] = '_';
        }
    }
    out[n] = '\0';
    return out;
}

static void doc_type_to_string(const cct_ast_type_t *type, char *out, size_t out_size) {
    if (!out || out_size == 0) return;
    if (!type) {
        snprintf(out, out_size, "<unknown>");
        return;
    }

    char base[256];
    snprintf(base, sizeof(base), "%s", type->name ? type->name : "<anon>");

    if (type->generic_args && type->generic_args->count > 0) {
        size_t used = strlen(base);
        if (used + 1 < sizeof(base)) {
            base[used++] = '<';
            base[used] = '\0';
        }
        for (size_t i = 0; i < type->generic_args->count; i++) {
            char argbuf[128];
            doc_type_to_string(type->generic_args->types[i], argbuf, sizeof(argbuf));
            size_t left = sizeof(base) - strlen(base) - 1;
            if (left == 0) break;
            strncat(base, argbuf, left);
            if (i + 1 < type->generic_args->count) {
                left = sizeof(base) - strlen(base) - 1;
                if (left > 0) strncat(base, ",", left);
            }
        }
        size_t left = sizeof(base) - strlen(base) - 1;
        if (left > 0) strncat(base, ">", left);
    }

    if (type->is_pointer) {
        snprintf(out, out_size, "SPECULUM %s", base);
        return;
    }
    if (type->is_array) {
        if (type->array_size > 0) {
            snprintf(out, out_size, "SERIES %s[%u]", base, type->array_size);
        } else {
            snprintf(out, out_size, "SERIES %s[]", base);
        }
        return;
    }

    snprintf(out, out_size, "%s", base);
}

static char *doc_rituale_signature(const cct_ast_node_t *decl) {
    if (!decl || decl->type != AST_RITUALE) return doc_strdup("RITUALE");

    char buf[2048];
    snprintf(buf, sizeof(buf), "RITUALE %s(", decl->as.rituale.name ? decl->as.rituale.name : "<anon>");

    if (decl->as.rituale.params) {
        for (size_t i = 0; i < decl->as.rituale.params->count; i++) {
            cct_ast_param_t *p = decl->as.rituale.params->params[i];
            char tbuf[256];
            doc_type_to_string(p ? p->type : NULL, tbuf, sizeof(tbuf));
            size_t left = sizeof(buf) - strlen(buf) - 1;
            if (left == 0) break;
            strncat(buf, tbuf, left);
            left = sizeof(buf) - strlen(buf) - 1;
            if (left == 0) break;
            strncat(buf, " ", left);
            left = sizeof(buf) - strlen(buf) - 1;
            if (left == 0) break;
            strncat(buf, p && p->name ? p->name : "arg", left);
            if (i + 1 < decl->as.rituale.params->count) {
                left = sizeof(buf) - strlen(buf) - 1;
                if (left > 0) strncat(buf, ", ", left);
            }
        }
    }

    size_t left = sizeof(buf) - strlen(buf) - 1;
    if (left > 0) strncat(buf, ") REDDE ", left);

    char rbuf[256];
    doc_type_to_string(decl->as.rituale.return_type, rbuf, sizeof(rbuf));
    left = sizeof(buf) - strlen(buf) - 1;
    if (left > 0) strncat(buf, rbuf, left);

    return doc_strdup(buf);
}

static char *doc_sigillum_signature(const cct_ast_node_t *decl) {
    char buf[512];
    snprintf(buf, sizeof(buf), "SIGILLUM %s (fields=%zu)",
             decl && decl->as.sigillum.name ? decl->as.sigillum.name : "<anon>",
             (decl && decl->as.sigillum.fields) ? decl->as.sigillum.fields->count : 0);
    return doc_strdup(buf);
}

static char *doc_ordo_signature(const cct_ast_node_t *decl) {
    char buf[512];
    snprintf(buf, sizeof(buf), "ORDO %s (items=%zu)",
             decl && decl->as.ordo.name ? decl->as.ordo.name : "<anon>",
             (decl && decl->as.ordo.items) ? decl->as.ordo.items->count : 0);
    return doc_strdup(buf);
}

static char *doc_pactum_signature(const cct_ast_node_t *decl) {
    char buf[512];
    snprintf(buf, sizeof(buf), "PACTUM %s (signatures=%zu)",
             decl && decl->as.pactum.name ? decl->as.pactum.name : "<anon>",
             (decl && decl->as.pactum.signatures) ? decl->as.pactum.signatures->count : 0);
    return doc_strdup(buf);
}

static char *doc_signature_for_decl(const cct_ast_node_t *decl) {
    if (!decl) return doc_strdup("<unknown>");
    switch (decl->type) {
        case AST_RITUALE: return doc_rituale_signature(decl);
        case AST_SIGILLUM: return doc_sigillum_signature(decl);
        case AST_ORDO: return doc_ordo_signature(decl);
        case AST_PACTUM: return doc_pactum_signature(decl);
        default: return doc_strdup("<unsupported declaration>");
    }
}

static const char *doc_kind_for_decl(const cct_ast_node_t *decl) {
    if (!decl) return "unknown";
    switch (decl->type) {
        case AST_RITUALE: return "rituale";
        case AST_SIGILLUM: return "sigillum";
        case AST_ORDO: return "ordo";
        case AST_PACTUM: return "pactum";
        default: return "declaration";
    }
}

static const char *doc_name_for_decl(const cct_ast_node_t *decl) {
    if (!decl) return "<unknown>";
    switch (decl->type) {
        case AST_RITUALE: return decl->as.rituale.name ? decl->as.rituale.name : "<rituale>";
        case AST_SIGILLUM: return decl->as.sigillum.name ? decl->as.sigillum.name : "<sigillum>";
        case AST_ORDO: return decl->as.ordo.name ? decl->as.ordo.name : "<ordo>";
        case AST_PACTUM: return decl->as.pactum.name ? decl->as.pactum.name : "<pactum>";
        default: return "<decl>";
    }
}

static bool doc_is_supported_decl(const cct_ast_node_t *decl) {
    if (!decl) return false;
    return decl->type == AST_RITUALE || decl->type == AST_SIGILLUM ||
           decl->type == AST_ORDO || decl->type == AST_PACTUM;
}

static char **doc_split_lines(const char *src, size_t *out_count) {
    *out_count = 0;
    if (!src) return NULL;

    size_t lines = 1;
    for (const char *p = src; *p; p++) {
        if (*p == '\n') lines++;
    }

    char **arr = (char **)calloc(lines, sizeof(char *));
    if (!arr) return NULL;

    const char *start = src;
    size_t idx = 0;
    for (const char *p = src; ; p++) {
        if (*p == '\n' || *p == '\0') {
            size_t len = (size_t)(p - start);
            arr[idx] = (char *)malloc(len + 1);
            if (!arr[idx]) {
                for (size_t i = 0; i < idx; i++) free(arr[i]);
                free(arr);
                return NULL;
            }
            memcpy(arr[idx], start, len);
            arr[idx][len] = '\0';
            idx++;
            if (*p == '\0') break;
            start = p + 1;
        }
    }

    *out_count = idx;
    return arr;
}

static void doc_free_lines(char **lines, size_t count) {
    if (!lines) return;
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

static cct_doc_block_t doc_extract_doc_block(char **lines, size_t line_count, u32 decl_line) {
    cct_doc_block_t block;
    memset(&block, 0, sizeof(block));
    if (!lines || line_count == 0 || decl_line == 0) return block;

    long i = (long)decl_line - 2;
    char *assembled = NULL;

    while (i >= 0) {
        char copy[1024];
        snprintf(copy, sizeof(copy), "%s", lines[i]);
        doc_trim(copy);

        if (copy[0] == '\0') {
            i--;
            continue;
        }

        if (strncmp(copy, "--!", 3) != 0) {
            break;
        }

        char *content = copy + 3;
        while (*content && isspace((unsigned char)*content)) content++;

        char line_buf[1024];
        snprintf(line_buf, sizeof(line_buf), "%s", content);

        if (line_buf[0] == '@') {
            if (strncmp(line_buf, "@param", 6) == 0) {
                block.param_count++;
            } else if (strncmp(line_buf, "@return", 7) == 0) {
                block.return_count++;
            } else if (strncmp(line_buf, "@example", 8) == 0) {
                block.example_count++;
            } else {
                block.invalid_tag_count++;
            }
        }

        if (!assembled) {
            assembled = doc_strdup(line_buf);
        } else {
            size_t add_len = strlen(line_buf);
            size_t old_len = strlen(assembled);
            char *grown = (char *)realloc(assembled, old_len + add_len + 2);
            if (grown) {
                assembled = grown;
                memmove(assembled + add_len + 1, assembled, old_len + 1);
                memcpy(assembled, line_buf, add_len);
                assembled[add_len] = '\n';
            }
        }

        i--;
    }

    block.text = assembled;
    block.has_doc = assembled != NULL && assembled[0] != '\0';
    return block;
}

static cct_ast_program_t *doc_parse_program(const char *filename) {
    char *source = doc_read_file(filename);
    if (!source) return NULL;

    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, filename);

    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, filename);

    cct_ast_program_t *program = cct_parser_parse_program(&parser);
    bool failed = !program || cct_parser_had_error(&parser) || cct_lexer_had_error(&lexer);

    cct_parser_dispose(&parser);
    free(source);

    if (failed) {
        if (program) cct_ast_free_program(program);
        return NULL;
    }

    return program;
}

static void doc_write_html_header(FILE *f, const char *title) {
    fprintf(f, "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>%s</title>", title);
    fputs("<link rel=\"stylesheet\" href=\"../assets/style.css\"></head><body>\n", f);
}

static void doc_write_css(const char *assets_dir) {
    char css_path[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(assets_dir, "style.css", css_path, sizeof(css_path))) return;
    FILE *f = fopen(css_path, "w");
    if (!f) return;
    fputs("body{font-family:ui-sans-serif,system-ui,sans-serif;margin:2rem;max-width:960px;}\n", f);
    fputs("code,pre{font-family:ui-monospace,Menlo,Monaco,monospace;}\n", f);
    fputs("pre{background:#f6f8fa;padding:1rem;border-radius:8px;}\n", f);
    fputs("a{color:#0a66c2;text-decoration:none;}a:hover{text-decoration:underline;}\n", f);
    fclose(f);
}

static bool doc_write_module_page(const cct_doc_options_t *opts,
                                  const char *modules_dir,
                                  const char *symbols_dir,
                                  const cct_doc_module_info_t *module,
                                  const cct_doc_symbol_list_t *symbols,
                                  const char *timestamp_text) {
    char md_path[CCT_PROJECT_PATH_MAX];
    char html_path[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(modules_dir, module->id, md_path, sizeof(md_path))) return false;
    if (!cct_project_join_path(modules_dir, module->id, html_path, sizeof(html_path))) return false;
    strncat(md_path, ".md", sizeof(md_path) - strlen(md_path) - 1);
    strncat(html_path, ".html", sizeof(html_path) - strlen(html_path) - 1);

    if (opts->format == DOC_FORMAT_MARKDOWN || opts->format == DOC_FORMAT_BOTH) {
        FILE *md = fopen(md_path, "w");
        if (!md) return false;
        fprintf(md, "# Module %s\n\n", module->id);
        fprintf(md, "- Path: `%s`\n", module->path);
        fprintf(md, "- Origin: %s\n", module->is_stdlib ? "stdlib (cct/...)" : "user module");
        if (timestamp_text) fprintf(md, "- Generated: %s\n", timestamp_text);
        fputs("\n## Symbols\n\n", md);
        for (size_t i = 0; i < symbols->count; i++) {
            const cct_doc_symbol_info_t *s = &symbols->items[i];
            if (strcmp(s->module_id, module->id) != 0) continue;
            fprintf(md, "- [%s](../symbols/%s.md) (%s)\n", s->name, s->id, s->kind);
        }
        fclose(md);
    }

    if (opts->format == DOC_FORMAT_HTML || opts->format == DOC_FORMAT_BOTH) {
        FILE *html = fopen(html_path, "w");
        if (!html) return false;
        doc_write_html_header(html, module->id);
        fprintf(html, "<h1>Module %s</h1>\n", module->id);
        fprintf(html, "<p><strong>Path:</strong> <code>%s</code></p>\n", module->path);
        fprintf(html, "<p><strong>Origin:</strong> %s</p>\n", module->is_stdlib ? "stdlib (cct/...)" : "user module");
        if (timestamp_text) fprintf(html, "<p><strong>Generated:</strong> %s</p>\n", timestamp_text);
        fputs("<h2>Symbols</h2><ul>\n", html);
        for (size_t i = 0; i < symbols->count; i++) {
            const cct_doc_symbol_info_t *s = &symbols->items[i];
            if (strcmp(s->module_id, module->id) != 0) continue;
            fprintf(html, "<li><a href=\"../symbols/%s.html\">%s</a> (%s)</li>\n", s->id, s->name, s->kind);
        }
        fputs("</ul></body></html>\n", html);
        fclose(html);
    }

    (void)symbols_dir;
    return true;
}

static bool doc_write_symbol_page(const cct_doc_options_t *opts,
                                  const char *symbols_dir,
                                  const cct_doc_symbol_info_t *symbol,
                                  const char *timestamp_text) {
    char md_path[CCT_PROJECT_PATH_MAX];
    char html_path[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(symbols_dir, symbol->id, md_path, sizeof(md_path))) return false;
    if (!cct_project_join_path(symbols_dir, symbol->id, html_path, sizeof(html_path))) return false;
    strncat(md_path, ".md", sizeof(md_path) - strlen(md_path) - 1);
    strncat(html_path, ".html", sizeof(html_path) - strlen(html_path) - 1);

    if (opts->format == DOC_FORMAT_MARKDOWN || opts->format == DOC_FORMAT_BOTH) {
        FILE *md = fopen(md_path, "w");
        if (!md) return false;
        fprintf(md, "# %s\n\n", symbol->name);
        fprintf(md, "- Kind: `%s`\n", symbol->kind);
        fprintf(md, "- Module: `%s`\n", symbol->module_path);
        fprintf(md, "- Visibility: `%s`\n", symbol->is_internal ? "ARCANUM" : "public");
        if (timestamp_text) fprintf(md, "- Generated: %s\n", timestamp_text);
        fputs("\n## Signature\n\n```cct\n", md);
        fprintf(md, "%s\n", symbol->signature ? symbol->signature : "<unknown>");
        fputs("```\n\n", md);
        if (symbol->doc.has_doc) {
            fputs("## Description\n\n", md);
            fprintf(md, "%s\n\n", symbol->doc.text);
        }
        if (!opts->no_examples && symbol->doc.example_count > 0) {
            fputs("## Examples\n\n", md);
            fputs("Example tags were found in this declaration doc block.\n\n", md);
        }
        fclose(md);
    }

    if (opts->format == DOC_FORMAT_HTML || opts->format == DOC_FORMAT_BOTH) {
        FILE *html = fopen(html_path, "w");
        if (!html) return false;
        doc_write_html_header(html, symbol->name);
        fprintf(html, "<h1>%s</h1>\n", symbol->name);
        fprintf(html, "<p><strong>Kind:</strong> <code>%s</code></p>\n", symbol->kind);
        fprintf(html, "<p><strong>Module:</strong> <code>%s</code></p>\n", symbol->module_path);
        fprintf(html, "<p><strong>Visibility:</strong> <code>%s</code></p>\n", symbol->is_internal ? "ARCANUM" : "public");
        if (timestamp_text) fprintf(html, "<p><strong>Generated:</strong> %s</p>\n", timestamp_text);
        fputs("<h2>Signature</h2><pre><code>", html);
        fprintf(html, "%s", symbol->signature ? symbol->signature : "<unknown>");
        fputs("</code></pre>\n", html);
        if (symbol->doc.has_doc) {
            fputs("<h2>Description</h2><pre>", html);
            fprintf(html, "%s", symbol->doc.text);
            fputs("</pre>\n", html);
        }
        if (!opts->no_examples && symbol->doc.example_count > 0) {
            fputs("<h2>Examples</h2><p>Example tags were found in this declaration doc block.</p>\n", html);
        }
        fputs("</body></html>\n", html);
        fclose(html);
    }

    return true;
}

static bool doc_write_index(const cct_doc_options_t *opts,
                            const char *out_dir,
                            const cct_doc_module_list_t *modules,
                            const cct_doc_symbol_list_t *symbols,
                            int warning_count,
                            const char *timestamp_text) {
    char md_path[CCT_PROJECT_PATH_MAX];
    char html_path[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(out_dir, "index.md", md_path, sizeof(md_path))) return false;
    if (!cct_project_join_path(out_dir, "index.html", html_path, sizeof(html_path))) return false;

    if (opts->format == DOC_FORMAT_MARKDOWN || opts->format == DOC_FORMAT_BOTH) {
        FILE *md = fopen(md_path, "w");
        if (!md) return false;
        fputs("# CCT API Documentation\n\n", md);
        fprintf(md, "- Modules: %zu\n", modules->count);
        fprintf(md, "- Symbols: %zu\n", symbols->count);
        fprintf(md, "- Warnings: %d\n", warning_count);
        if (timestamp_text) fprintf(md, "- Generated: %s\n", timestamp_text);
        fputs("\n## Modules\n\n", md);
        for (size_t i = 0; i < modules->count; i++) {
            fprintf(md, "- [%s](modules/%s.md)\n", modules->items[i].id, modules->items[i].id);
        }
        fputs("\n## Symbols\n\n", md);
        for (size_t i = 0; i < symbols->count; i++) {
            fprintf(md, "- [%s](symbols/%s.md) (%s)\n", symbols->items[i].name, symbols->items[i].id, symbols->items[i].kind);
        }
        fclose(md);
    }

    if (opts->format == DOC_FORMAT_HTML || opts->format == DOC_FORMAT_BOTH) {
        FILE *html = fopen(html_path, "w");
        if (!html) return false;
        fprintf(html, "<!doctype html><html><head><meta charset=\"utf-8\"><title>CCT API Documentation</title>");
        fputs("<link rel=\"stylesheet\" href=\"assets/style.css\"></head><body>\n", html);
        fputs("<h1>CCT API Documentation</h1>\n", html);
        fprintf(html, "<p>Modules: %zu | Symbols: %zu | Warnings: %d</p>\n", modules->count, symbols->count, warning_count);
        if (timestamp_text) fprintf(html, "<p>Generated: %s</p>\n", timestamp_text);
        fputs("<h2>Modules</h2><ul>\n", html);
        for (size_t i = 0; i < modules->count; i++) {
            fprintf(html, "<li><a href=\"modules/%s.html\">%s</a></li>\n", modules->items[i].id, modules->items[i].id);
        }
        fputs("</ul><h2>Symbols</h2><ul>\n", html);
        for (size_t i = 0; i < symbols->count; i++) {
            fprintf(html, "<li><a href=\"symbols/%s.html\">%s</a> (%s)</li>\n", symbols->items[i].id, symbols->items[i].name, symbols->items[i].kind);
        }
        fputs("</ul></body></html>\n", html);
        fclose(html);
    }

    return true;
}

static int doc_collect_symbols_for_module(const cct_doc_options_t *opts,
                                          const cct_doc_module_info_t *module,
                                          cct_ast_program_t *program,
                                          char **source_lines,
                                          size_t source_line_count,
                                          cct_doc_symbol_list_t *out_symbols,
                                          int *warning_count) {
    if (!program || !program->declarations) return 0;

    for (size_t i = 0; i < program->declarations->count; i++) {
        cct_ast_node_t *decl = program->declarations->nodes[i];
        if (!doc_is_supported_decl(decl)) continue;
        if (decl->is_internal && !opts->include_internal) continue;

        cct_doc_symbol_info_t sym;
        memset(&sym, 0, sizeof(sym));

        char raw_id[512];
        snprintf(raw_id, sizeof(raw_id), "%s_%s", module->id, doc_name_for_decl(decl));
        sym.id = doc_sanitize_id(raw_id);
        sym.name = doc_strdup(doc_name_for_decl(decl));
        sym.kind = doc_strdup(doc_kind_for_decl(decl));
        sym.signature = doc_signature_for_decl(decl);
        sym.module_id = doc_strdup(module->id);
        sym.module_path = doc_strdup(module->path);
        sym.is_internal = decl->is_internal;
        sym.doc = doc_extract_doc_block(source_lines, source_line_count, decl->line);

        if (sym.doc.invalid_tag_count > 0) {
            fprintf(stderr, "[doc] warning: invalid doc tag near %s (%s)\n", sym.name, sym.module_path);
            *warning_count += sym.doc.invalid_tag_count;
        }

        if (opts->warn_missing_docs && !sym.doc.has_doc) {
            fprintf(stderr, "[doc] warning: missing docs for %s (%s)\n", sym.name, sym.module_path);
            (*warning_count)++;
        }

        if (!doc_push_symbol(out_symbols, sym)) {
            free(sym.id);
            free(sym.name);
            free(sym.kind);
            free(sym.signature);
            free(sym.module_id);
            free(sym.module_path);
            doc_free_block(&sym.doc);
            return 1;
        }
    }

    return 0;
}

static int doc_parse_options(int argc, char **argv, cct_doc_options_t *opts) {
    memset(opts, 0, sizeof(*opts));
    opts->format = DOC_FORMAT_BOTH;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--project") == 0) {
            if (i + 1 >= argc) return 1;
            opts->project_override = argv[++i];
            continue;
        }
        if (strcmp(arg, "--entry") == 0) {
            if (i + 1 >= argc) return 1;
            opts->entry_override = argv[++i];
            continue;
        }
        if (strcmp(arg, "--output-dir") == 0) {
            if (i + 1 >= argc) return 1;
            opts->output_dir_override = argv[++i];
            continue;
        }
        if (strcmp(arg, "--format") == 0) {
            if (i + 1 >= argc) return 1;
            const char *fmt = argv[++i];
            if (strcmp(fmt, "markdown") == 0) opts->format = DOC_FORMAT_MARKDOWN;
            else if (strcmp(fmt, "html") == 0) opts->format = DOC_FORMAT_HTML;
            else if (strcmp(fmt, "both") == 0) opts->format = DOC_FORMAT_BOTH;
            else return 1;
            continue;
        }

        if (strcmp(arg, "--include-internal") == 0) {
            opts->include_internal = true;
            continue;
        }
        if (strcmp(arg, "--no-examples") == 0) {
            opts->no_examples = true;
            continue;
        }
        if (strcmp(arg, "--warn-missing-docs") == 0) {
            opts->warn_missing_docs = true;
            continue;
        }
        if (strcmp(arg, "--strict-docs") == 0) {
            opts->strict_docs = true;
            continue;
        }
        if (strcmp(arg, "--no-timestamp") == 0) {
            opts->no_timestamp = true;
            continue;
        }

        fprintf(stderr, "cct: error: unknown doc option: %s\n", arg);
        return 1;
    }

    return 0;
}

int cct_doc_command(int argc, char **argv) {
    cct_doc_options_t opts;
    if (doc_parse_options(argc, argv, &opts) != 0) {
        fprintf(stderr, "Usage: cct doc [--project DIR] [--entry FILE] [--output-dir DIR] [--format markdown|html|both] \\\n"
                        "              [--include-internal] [--no-examples] [--warn-missing-docs] [--strict-docs] [--no-timestamp]\n");
        return 1;
    }

    char cwd[CCT_PROJECT_PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "[doc] error: could not determine current directory\n");
        return 1;
    }

    cct_project_layout_t layout;
    char err[256];
    if (!cct_project_discover(cwd, opts.project_override, opts.entry_override, &layout, err, sizeof(err))) {
        fprintf(stderr, "[doc] error: %s\n", err);
        return 1;
    }

    char out_dir[CCT_PROJECT_PATH_MAX];
    if (opts.output_dir_override && opts.output_dir_override[0] != '\0') {
        if (opts.output_dir_override[0] == '/') {
            snprintf(out_dir, sizeof(out_dir), "%s", opts.output_dir_override);
        } else if (!cct_project_join_path(layout.root_path, opts.output_dir_override, out_dir, sizeof(out_dir))) {
            fprintf(stderr, "[doc] error: output directory path too long\n");
            return 1;
        }
    } else if (!cct_project_join_path(layout.root_path, "docs/api", out_dir, sizeof(out_dir))) {
        fprintf(stderr, "[doc] error: output directory path too long\n");
        return 1;
    }

    char modules_dir[CCT_PROJECT_PATH_MAX];
    char symbols_dir[CCT_PROJECT_PATH_MAX];
    char assets_dir[CCT_PROJECT_PATH_MAX];
    if (!cct_project_join_path(out_dir, "modules", modules_dir, sizeof(modules_dir)) ||
        !cct_project_join_path(out_dir, "symbols", symbols_dir, sizeof(symbols_dir)) ||
        !cct_project_join_path(out_dir, "assets", assets_dir, sizeof(assets_dir))) {
        fprintf(stderr, "[doc] error: output path too long\n");
        return 1;
    }

    if (!doc_ensure_dir(modules_dir) || !doc_ensure_dir(symbols_dir) || !doc_ensure_dir(assets_dir)) {
        fprintf(stderr, "[doc] error: could not create output directories\n");
        return 1;
    }

    cct_module_bundle_t bundle;
    cct_error_code_t bundle_status = CCT_OK;
    if (!cct_module_bundle_build(layout.entry_path, &bundle, &bundle_status)) {
        return 1;
    }

    cct_doc_module_list_t modules = {0};
    cct_doc_symbol_list_t symbols = {0};
    int warning_count = 0;

    for (u32 i = 0; i < bundle.module_path_count; i++) {
        const char *path = bundle.module_paths[i];
        if (!path) continue;

        cct_doc_module_info_t mod;
        memset(&mod, 0, sizeof(mod));

        const char *base = doc_basename(path);
        char base_no_ext[256];
        snprintf(base_no_ext, sizeof(base_no_ext), "%s", base);
        if (doc_has_cct_extension(base_no_ext)) {
            base_no_ext[strlen(base_no_ext) - 4] = '\0';
        }

        char raw_module_id[320];
        snprintf(raw_module_id, sizeof(raw_module_id), "mod_%03u_%s", i, base_no_ext);
        mod.id = doc_sanitize_id(raw_module_id);
        mod.path = doc_strdup(path);
        mod.is_stdlib = (bundle.module_origins && i < bundle.module_path_count &&
                         bundle.module_origins[i] == CCT_MODULE_ORIGIN_STDLIB);

        if (!mod.id || !mod.path || !doc_push_module(&modules, mod)) {
            free(mod.id);
            free(mod.path);
            doc_modules_dispose(&modules);
            doc_symbols_dispose(&symbols);
            cct_module_bundle_dispose(&bundle);
            fprintf(stderr, "[doc] error: out of memory while collecting modules\n");
            return 1;
        }

        cct_ast_program_t *program = doc_parse_program(path);
        if (!program) {
            fprintf(stderr, "[doc] warning: skipped module due parse failure: %s\n", path);
            warning_count++;
            continue;
        }

        char *source = doc_read_file(path);
        if (!source) {
            cct_ast_free_program(program);
            fprintf(stderr, "[doc] warning: could not read source for docs: %s\n", path);
            warning_count++;
            continue;
        }

        size_t line_count = 0;
        char **lines = doc_split_lines(source, &line_count);
        free(source);
        if (!lines) {
            cct_ast_free_program(program);
            fprintf(stderr, "[doc] warning: could not split source lines for docs: %s\n", path);
            warning_count++;
            continue;
        }

        if (doc_collect_symbols_for_module(&opts, &modules.items[modules.count - 1], program, lines, line_count, &symbols, &warning_count) != 0) {
            doc_free_lines(lines, line_count);
            cct_ast_free_program(program);
            doc_modules_dispose(&modules);
            doc_symbols_dispose(&symbols);
            cct_module_bundle_dispose(&bundle);
            fprintf(stderr, "[doc] error: out of memory while collecting symbols\n");
            return 1;
        }

        doc_free_lines(lines, line_count);
        cct_ast_free_program(program);
    }

    char timestamp[64];
    char *timestamp_ptr = NULL;
    if (!opts.no_timestamp) {
        time_t now = time(NULL);
        struct tm tmv;
        localtime_r(&now, &tmv);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tmv);
        timestamp_ptr = timestamp;
    }

    if (opts.format == DOC_FORMAT_HTML || opts.format == DOC_FORMAT_BOTH) {
        doc_write_css(assets_dir);
    }

    for (size_t i = 0; i < modules.count; i++) {
        if (!doc_write_module_page(&opts, modules_dir, symbols_dir, &modules.items[i], &symbols, timestamp_ptr)) {
            fprintf(stderr, "[doc] error: could not write module pages\n");
            doc_modules_dispose(&modules);
            doc_symbols_dispose(&symbols);
            cct_module_bundle_dispose(&bundle);
            return 1;
        }
    }

    for (size_t i = 0; i < symbols.count; i++) {
        if (!doc_write_symbol_page(&opts, symbols_dir, &symbols.items[i], timestamp_ptr)) {
            fprintf(stderr, "[doc] error: could not write symbol pages\n");
            doc_modules_dispose(&modules);
            doc_symbols_dispose(&symbols);
            cct_module_bundle_dispose(&bundle);
            return 1;
        }
    }

    if (!doc_write_index(&opts, out_dir, &modules, &symbols, warning_count, timestamp_ptr)) {
        fprintf(stderr, "[doc] error: could not write index files\n");
        doc_modules_dispose(&modules);
        doc_symbols_dispose(&symbols);
        cct_module_bundle_dispose(&bundle);
        return 1;
    }

    printf("[doc] project root: %s\n", layout.root_path);
    printf("[doc] entry: %s\n", layout.entry_path);
    printf("[doc] output: %s\n", out_dir);
    printf("[doc] modules: %zu\n", modules.count);
    printf("[doc] symbols: %zu\n", symbols.count);
    printf("[doc] warnings: %d\n", warning_count);

    doc_modules_dispose(&modules);
    doc_symbols_dispose(&symbols);
    cct_module_bundle_dispose(&bundle);

    if (opts.strict_docs && warning_count > 0) {
        return 2;
    }
    return 0;
}
