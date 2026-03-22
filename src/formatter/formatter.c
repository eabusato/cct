/*
 * CCT — Clavicula Turing
 * Formatter Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "formatter.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    BLOCK_GENERIC = 0,
    BLOCK_RITUALE,
    BLOCK_DONEC
} cct_fmt_block_kind_t;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} cct_fmt_buffer_t;

typedef struct {
    cct_token_t *items;
    size_t count;
    size_t cap;
} cct_fmt_tokens_t;

typedef struct {
    cct_fmt_block_kind_t *items;
    size_t count;
    size_t cap;
} cct_fmt_stack_t;

static void fmt_buffer_dispose(cct_fmt_buffer_t *buf) {
    if (!buf) return;
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

static bool fmt_buffer_reserve(cct_fmt_buffer_t *buf, size_t extra) {
    if (!buf) return false;
    size_t need = buf->len + extra + 1;
    if (need <= buf->cap) return true;
    size_t new_cap = (buf->cap == 0) ? 128 : buf->cap;
    while (new_cap < need) new_cap *= 2;
    char *tmp = (char*)realloc(buf->data, new_cap);
    if (!tmp) return false;
    buf->data = tmp;
    buf->cap = new_cap;
    return true;
}

static bool fmt_buffer_append_n(cct_fmt_buffer_t *buf, const char *s, size_t n) {
    if (!buf || (!s && n > 0)) return false;
    if (!fmt_buffer_reserve(buf, n)) return false;
    if (n > 0) memcpy(buf->data + buf->len, s, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
    return true;
}

static bool fmt_buffer_append(cct_fmt_buffer_t *buf, const char *s) {
    return fmt_buffer_append_n(buf, s, s ? strlen(s) : 0);
}

static bool fmt_buffer_append_char(cct_fmt_buffer_t *buf, char c) {
    if (!fmt_buffer_reserve(buf, 1)) return false;
    buf->data[buf->len++] = c;
    buf->data[buf->len] = '\0';
    return true;
}

static bool fmt_tokens_push(cct_fmt_tokens_t *tokens, cct_token_t tk) {
    if (!tokens) return false;
    if (tokens->count == tokens->cap) {
        size_t new_cap = (tokens->cap == 0) ? 16 : tokens->cap * 2;
        cct_token_t *tmp = (cct_token_t*)realloc(tokens->items, new_cap * sizeof(*tokens->items));
        if (!tmp) return false;
        tokens->items = tmp;
        tokens->cap = new_cap;
    }
    tokens->items[tokens->count++] = tk;
    return true;
}

static void fmt_tokens_dispose(cct_fmt_tokens_t *tokens) {
    if (!tokens) return;
    for (size_t i = 0; i < tokens->count; i++) {
        cct_token_free(&tokens->items[i]);
    }
    free(tokens->items);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->cap = 0;
}

static bool fmt_stack_push(cct_fmt_stack_t *stack, cct_fmt_block_kind_t kind) {
    if (!stack) return false;
    if (stack->count == stack->cap) {
        size_t new_cap = (stack->cap == 0) ? 16 : stack->cap * 2;
        cct_fmt_block_kind_t *tmp = (cct_fmt_block_kind_t*)realloc(stack->items, new_cap * sizeof(*stack->items));
        if (!tmp) return false;
        stack->items = tmp;
        stack->cap = new_cap;
    }
    stack->items[stack->count++] = kind;
    return true;
}

static cct_fmt_block_kind_t fmt_stack_top(const cct_fmt_stack_t *stack) {
    if (!stack || stack->count == 0) return BLOCK_GENERIC;
    return stack->items[stack->count - 1];
}

static cct_fmt_block_kind_t fmt_stack_pop(cct_fmt_stack_t *stack) {
    if (!stack || stack->count == 0) return BLOCK_GENERIC;
    return stack->items[--stack->count];
}

static void fmt_stack_dispose(cct_fmt_stack_t *stack) {
    if (!stack) return;
    free(stack->items);
    stack->items = NULL;
    stack->count = 0;
    stack->cap = 0;
}

static char* fmt_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *copy = (char*)malloc(n + 1);
    if (!copy) return NULL;
    memcpy(copy, s, n + 1);
    return copy;
}

static char* fmt_read_file(const char *file_path) {
    FILE *f = fopen(file_path, "rb");
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
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[rd] = '\0';
    return buf;
}

bool cct_formatter_write_file(const char *file_path, const char *content) {
    FILE *f = fopen(file_path, "wb");
    if (!f) return false;
    size_t len = content ? strlen(content) : 0;
    bool ok = fwrite(content, 1, len, f) == len;
    fclose(f);
    return ok;
}

static bool fmt_validate_parseable(const char *file_path, const char *source) {
    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, file_path);
    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, file_path);
    cct_ast_program_t *program = cct_parser_parse_program(&parser);
    bool ok = program != NULL && !cct_parser_had_error(&parser) && !cct_lexer_had_error(&lexer);
    if (program) cct_ast_free_program(program);
    cct_parser_dispose(&parser);
    return ok;
}

static bool fmt_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool fmt_tokenize_code_segment(const char *segment, cct_fmt_tokens_t *out_tokens) {
    if (!segment || !out_tokens) return false;
    cct_lexer_t lx;
    cct_lexer_init(&lx, segment, "<fmt-line>");
    while (true) {
        cct_token_t tk = cct_lexer_next_token(&lx);
        if (tk.type == TOKEN_EOF) {
            cct_token_free(&tk);
            break;
        }
        if (tk.type == TOKEN_INVALID) {
            cct_token_free(&tk);
            return false;
        }
        if (!fmt_tokens_push(out_tokens, tk)) {
            cct_token_free(&tk);
            return false;
        }
    }
    return true;
}

static bool fmt_requires_space_between(cct_token_type_t prev, cct_token_type_t cur) {
    if (prev == TOKEN_EOF) return false;
    if (cur == TOKEN_COMMA || cur == TOKEN_RPAREN || cur == TOKEN_RBRACKET ||
        cur == TOKEN_DOT || cur == TOKEN_COLON || cur == TOKEN_SEMICOLON) {
        return false;
    }
    if (prev == TOKEN_LPAREN || prev == TOKEN_LBRACKET || prev == TOKEN_DOT || prev == TOKEN_COLON) {
        return false;
    }
    if (cur == TOKEN_LPAREN || cur == TOKEN_LBRACKET) {
        return false;
    }
    return true;
}

static bool fmt_render_tokens_line(const cct_fmt_tokens_t *tokens, cct_fmt_buffer_t *line_buf) {
    cct_token_type_t prev = TOKEN_EOF;
    for (size_t i = 0; i < tokens->count; i++) {
        const cct_token_t *tk = &tokens->items[i];
        if (fmt_requires_space_between(prev, tk->type) && !fmt_buffer_append_char(line_buf, ' ')) {
            return false;
        }
        if (!fmt_buffer_append(line_buf, tk->lexeme)) {
            return false;
        }
        prev = tk->type;
    }
    return true;
}

static bool fmt_line_split_comment(const char *line, char **out_code, char **out_comment) {
    if (!line || !out_code || !out_comment) return false;
    *out_code = NULL;
    *out_comment = NULL;
    bool in_string = false;
    for (size_t i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        if (c == '"' && (i == 0 || line[i - 1] != '\\')) {
            in_string = !in_string;
        }
        if (!in_string && c == '-' && line[i + 1] == '-') {
            *out_code = (char*)malloc(i + 1);
            if (!*out_code) return false;
            memcpy(*out_code, line, i);
            (*out_code)[i] = '\0';
            *out_comment = fmt_strdup(line + i);
            if (!*out_comment) {
                free(*out_code);
                *out_code = NULL;
                return false;
            }
            return true;
        }
    }
    *out_code = fmt_strdup(line);
    if (!*out_code) return false;
    *out_comment = NULL;
    return true;
}

static void fmt_trim_inplace(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    size_t start = 0;
    while (start < n && fmt_is_space(s[start])) start++;
    size_t end = n;
    while (end > start && fmt_is_space(s[end - 1])) end--;
    if (start > 0) memmove(s, s + start, end - start);
    s[end - start] = '\0';
}

static bool fmt_line_is_top_level_decl(cct_token_type_t first, cct_token_type_t second) {
    if (first == TOKEN_RITUALE || first == TOKEN_SIGILLUM || first == TOKEN_ORDO ||
        first == TOKEN_PACTUM || first == TOKEN_CODEX) {
        return true;
    }
    if (first == TOKEN_EXPLICIT && second != TOKEN_RITUALE) {
        return true;
    }
    return false;
}

static bool fmt_should_dedent_before(cct_token_type_t first, cct_token_type_t second, const cct_fmt_stack_t *stack) {
    if (first == TOKEN_FIN) return true;
    if (first == TOKEN_EXPLICIT && second == TOKEN_RITUALE) return true;
    if (first == TOKEN_ALITER || first == TOKEN_CAPE || first == TOKEN_SEMPER) return true;
    if (first == TOKEN_DUM && fmt_stack_top(stack) == BLOCK_DONEC) return true;
    return false;
}

static bool fmt_should_open_block(cct_token_type_t first, cct_token_type_t second, const cct_fmt_stack_t *stack, cct_fmt_block_kind_t *kind) {
    (void)second;
    if (kind) *kind = BLOCK_GENERIC;
    if (first == TOKEN_RITUALE) {
        if (kind) *kind = BLOCK_RITUALE;
        return true;
    }
    if (first == TOKEN_SIGILLUM || first == TOKEN_ORDO || first == TOKEN_PACTUM || first == TOKEN_CODEX ||
        first == TOKEN_SI || first == TOKEN_DUM || first == TOKEN_REPETE || first == TOKEN_ITERUM ||
        first == TOKEN_TEMPTA) {
        return true;
    }
    if (first == TOKEN_DONEC) {
        if (kind) *kind = BLOCK_DONEC;
        return true;
    }
    if ((first == TOKEN_ALITER || first == TOKEN_CAPE || first == TOKEN_SEMPER) && fmt_stack_top(stack) != BLOCK_DONEC) {
        return true;
    }
    return false;
}

static char* cct_formatter_format_source(const char *source, int indent_size) {
    cct_fmt_buffer_t out = {0};
    cct_fmt_stack_t stack = {0};
    int indent = 0;
    bool pending_blank = false;
    bool prev_emitted_blank = false;
    bool prev_emitted_code = false;

    const char *cursor = source;
    while (true) {
        const char *line_end = strchr(cursor, '\n');
        size_t line_len = line_end ? (size_t)(line_end - cursor) : strlen(cursor);

        char *line = (char*)malloc(line_len + 1);
        if (!line) goto fail;
        memcpy(line, cursor, line_len);
        line[line_len] = '\0';
        if (line_len > 0 && line[line_len - 1] == '\r') {
            line[line_len - 1] = '\0';
        }

        char *code = NULL;
        char *comment = NULL;
        if (!fmt_line_split_comment(line, &code, &comment)) {
            free(line);
            goto fail;
        }
        free(line);

        fmt_trim_inplace(code);
        if (comment) fmt_trim_inplace(comment);

        cct_fmt_tokens_t tokens = {0};
        cct_fmt_buffer_t rendered = {0};
        cct_token_type_t first = TOKEN_EOF;
        cct_token_type_t second = TOKEN_EOF;

        if (code && code[0] != '\0') {
            if (!fmt_tokenize_code_segment(code, &tokens)) {
                fmt_tokens_dispose(&tokens);
                fmt_buffer_dispose(&rendered);
                free(code);
                free(comment);
                goto fail;
            }
            if (tokens.count > 0) {
                first = tokens.items[0].type;
                second = (tokens.count > 1) ? tokens.items[1].type : TOKEN_EOF;
            }
            if (!fmt_render_tokens_line(&tokens, &rendered)) {
                fmt_tokens_dispose(&tokens);
                fmt_buffer_dispose(&rendered);
                free(code);
                free(comment);
                goto fail;
            }
        }

        bool has_code = rendered.len > 0;
        bool has_comment = comment && comment[0] != '\0';

        if (!has_code && !has_comment) {
            if (prev_emitted_code) pending_blank = true;
        } else {
            if (fmt_should_dedent_before(first, second, &stack) && indent > 0) {
                indent--;
                if (first == TOKEN_FIN || (first == TOKEN_EXPLICIT && second == TOKEN_RITUALE) ||
                    (first == TOKEN_DUM && fmt_stack_top(&stack) == BLOCK_DONEC)) {
                    (void)fmt_stack_pop(&stack);
                }
            }

            if (pending_blank && prev_emitted_code && !prev_emitted_blank) {
                if (!fmt_buffer_append_char(&out, '\n')) {
                    fmt_tokens_dispose(&tokens);
                    fmt_buffer_dispose(&rendered);
                    free(code);
                    free(comment);
                    goto fail;
                }
                prev_emitted_blank = true;
            }

            if (indent == 0 && has_code && prev_emitted_code && !prev_emitted_blank &&
                fmt_line_is_top_level_decl(first, second)) {
                if (!fmt_buffer_append_char(&out, '\n')) {
                    fmt_tokens_dispose(&tokens);
                    fmt_buffer_dispose(&rendered);
                    free(code);
                    free(comment);
                    goto fail;
                }
            }

            for (int i = 0; i < indent * indent_size; i++) {
                if (!fmt_buffer_append_char(&out, ' ')) {
                    fmt_tokens_dispose(&tokens);
                    fmt_buffer_dispose(&rendered);
                    free(code);
                    free(comment);
                    goto fail;
                }
            }

            if (has_code && !fmt_buffer_append_n(&out, rendered.data, rendered.len)) {
                fmt_tokens_dispose(&tokens);
                fmt_buffer_dispose(&rendered);
                free(code);
                free(comment);
                goto fail;
            }
            if (has_comment) {
                if (has_code && !fmt_buffer_append(&out, "  ")) {
                    fmt_tokens_dispose(&tokens);
                    fmt_buffer_dispose(&rendered);
                    free(code);
                    free(comment);
                    goto fail;
                }
                if (!fmt_buffer_append(&out, comment)) {
                    fmt_tokens_dispose(&tokens);
                    fmt_buffer_dispose(&rendered);
                    free(code);
                    free(comment);
                    goto fail;
                }
            }

            if (!fmt_buffer_append_char(&out, '\n')) {
                fmt_tokens_dispose(&tokens);
                fmt_buffer_dispose(&rendered);
                free(code);
                free(comment);
                goto fail;
            }

            cct_fmt_block_kind_t opened = BLOCK_GENERIC;
            if (has_code && fmt_should_open_block(first, second, &stack, &opened)) {
                if (first == TOKEN_DUM && fmt_stack_top(&stack) == BLOCK_DONEC) {
                    /* DONEC trailer closes block and does not open another one. */
                } else {
                    if (!fmt_stack_push(&stack, opened)) {
                        fmt_tokens_dispose(&tokens);
                        fmt_buffer_dispose(&rendered);
                        free(code);
                        free(comment);
                        goto fail;
                    }
                    indent++;
                }
            }

            pending_blank = false;
            prev_emitted_blank = false;
            prev_emitted_code = true;
        }

        fmt_tokens_dispose(&tokens);
        fmt_buffer_dispose(&rendered);
        free(code);
        free(comment);

        if (!line_end) break;
        cursor = line_end + 1;
    }

    while (out.len > 1 && out.data[out.len - 1] == '\n' && out.data[out.len - 2] == '\n') {
        out.data[--out.len] = '\0';
    }
    if (out.len == 0 || out.data[out.len - 1] != '\n') {
        if (!fmt_buffer_append_char(&out, '\n')) goto fail;
    }

    fmt_stack_dispose(&stack);
    return out.data;

fail:
    fmt_stack_dispose(&stack);
    fmt_buffer_dispose(&out);
    return NULL;
}

cct_formatter_options_t cct_formatter_default_options(void) {
    cct_formatter_options_t opts;
    opts.indent_size = 2;
    return opts;
}

cct_formatter_result_t cct_formatter_format_file(
    const char *file_path,
    const cct_formatter_options_t *options
) {
    cct_formatter_result_t result;
    memset(&result, 0, sizeof(result));
    result.error_code = CCT_OK;

    if (!file_path) {
        result.error_code = CCT_ERROR_INVALID_ARGUMENT;
        result.error_message = fmt_strdup("formatter: missing file path");
        return result;
    }

    char *source = fmt_read_file(file_path);
    if (!source) {
        result.error_code = CCT_ERROR_FILE_READ;
        result.error_message = fmt_strdup("formatter: could not read file");
        return result;
    }

    if (!fmt_validate_parseable(file_path, source)) {
        free(source);
        result.error_code = CCT_ERROR_SYNTAX;
        result.error_message = fmt_strdup("formatter: syntax errors prevent formatting");
        return result;
    }

    int indent_size = (options && options->indent_size > 0) ? options->indent_size : 2;
    char *formatted = cct_formatter_format_source(source, indent_size);
    if (!formatted) {
        free(source);
        result.error_code = CCT_ERROR_OUT_OF_MEMORY;
        result.error_message = fmt_strdup("formatter: out of memory");
        return result;
    }

    result.success = true;
    result.original_source = source;
    result.formatted_source = formatted;
    result.changed = strcmp(source, formatted) != 0;
    return result;
}

void cct_formatter_print_diff(
    FILE *out,
    const char *file_path,
    const char *original_source,
    const char *formatted_source
) {
    if (!out || !original_source || !formatted_source || strcmp(original_source, formatted_source) == 0) {
        return;
    }

    fprintf(out, "--- %s (original)\n", file_path ? file_path : "<input>");
    fprintf(out, "+++ %s (formatted)\n", file_path ? file_path : "<input>");

    const char *orig = original_source;
    const char *fmt = formatted_source;
    while (*orig || *fmt) {
        const char *orig_end = strchr(orig, '\n');
        const char *fmt_end = strchr(fmt, '\n');
        size_t orig_len = orig_end ? (size_t)(orig_end - orig) : strlen(orig);
        size_t fmt_len = fmt_end ? (size_t)(fmt_end - fmt) : strlen(fmt);

        bool same = (orig_len == fmt_len) && (strncmp(orig, fmt, orig_len) == 0);
        if (!same) {
            fprintf(out, "- %.*s\n", (int)orig_len, orig);
            fprintf(out, "+ %.*s\n", (int)fmt_len, fmt);
        }

        orig = orig_end ? (orig_end + 1) : (orig + orig_len);
        fmt = fmt_end ? (fmt_end + 1) : (fmt + fmt_len);
        if (!orig_end && !fmt_end) break;
    }
}

void cct_formatter_result_dispose(cct_formatter_result_t *result) {
    if (!result) return;
    free(result->original_source);
    free(result->formatted_source);
    free(result->error_message);
    memset(result, 0, sizeof(*result));
}
