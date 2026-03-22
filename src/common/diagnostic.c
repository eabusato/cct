/*
 * CCT — Clavicula Turing
 * Diagnostic Helpers Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "diagnostic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool g_colors_set = false;
static bool g_colors_enabled = false;

static const char *C_RED = "\x1b[31m";
static const char *C_YELLOW = "\x1b[33m";
static const char *C_CYAN = "\x1b[36m";
static const char *C_GREEN = "\x1b[32m";
static const char *C_BOLD = "\x1b[1m";
static const char *C_RESET = "\x1b[0m";

static void cct_diagnostic_autoconfigure_colors(void) {
    if (g_colors_set) return;

    const char *no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') {
        g_colors_enabled = false;
        g_colors_set = true;
        return;
    }

    const char *force = getenv("CCT_FORCE_COLOR");
    if (force && strcmp(force, "1") == 0) {
        g_colors_enabled = true;
        g_colors_set = true;
        return;
    }

    g_colors_enabled = isatty(fileno(stderr));
    g_colors_set = true;
}

void cct_diagnostic_set_colors(bool enabled) {
    g_colors_enabled = enabled;
    g_colors_set = true;
}

bool cct_diagnostic_colors_enabled(void) {
    cct_diagnostic_autoconfigure_colors();
    return g_colors_enabled;
}

const char* cct_diagnostic_level_text(cct_diagnostic_level_t level) {
    switch (level) {
        case CCT_DIAG_ERROR: return "error";
        case CCT_DIAG_WARNING: return "warning";
        case CCT_DIAG_NOTE: return "note";
        case CCT_DIAG_HINT: return "hint";
        default: return "diagnostic";
    }
}

static const char* cct_diag_level_color(cct_diagnostic_level_t level) {
    switch (level) {
        case CCT_DIAG_ERROR: return C_RED;
        case CCT_DIAG_WARNING: return C_YELLOW;
        case CCT_DIAG_NOTE: return C_CYAN;
        case CCT_DIAG_HINT: return C_GREEN;
        default: return "";
    }
}

char* cct_read_source_line(const char *file_path, u32 line_number) {
    if (!file_path || !line_number) return NULL;

    FILE *fp = fopen(file_path, "rb");
    if (!fp) return NULL;

    size_t cap = 256;
    size_t len = 0;
    char *line = (char*)malloc(cap);
    if (!line) {
        fclose(fp);
        return NULL;
    }

    u32 current_line = 1;
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (current_line == line_number) {
            if (ch == '\r') continue;
            if (ch == '\n') break;

            if (len + 1 >= cap) {
                size_t next_cap = cap * 2;
                char *next = (char*)realloc(line, next_cap);
                if (!next) {
                    free(line);
                    fclose(fp);
                    return NULL;
                }
                line = next;
                cap = next_cap;
            }

            line[len++] = (char)ch;
        } else if (ch == '\n') {
            current_line++;
            if (current_line > line_number) break;
        }
    }

    fclose(fp);

    if (current_line != line_number) {
        free(line);
        return NULL;
    }

    line[len] = '\0';
    return line;
}

static void cct_diag_emit_snippet(const cct_diagnostic_t *diag) {
    if (!diag || !diag->file_path || !diag->line) return;

    char *src = cct_read_source_line(diag->file_path, diag->line);
    if (!src) return;

    fprintf(stderr, "  --> %s:%u:%u\n", diag->file_path, diag->line, diag->column);
    fprintf(stderr, "   |\n");
    fprintf(stderr, "%4u | %s\n", diag->line, src);

    size_t marker_indent = 0;
    if (diag->column > 1) {
        marker_indent = (size_t)(diag->column - 1);
        size_t src_len = strlen(src);
        if (marker_indent > src_len) marker_indent = src_len;
    }

    fprintf(stderr, "   | ");
    for (size_t i = 0; i < marker_indent; i++) {
        fputc(src[i] == '\t' ? '\t' : ' ', stderr);
    }
    fprintf(stderr, "^\n");

    free(src);
}

void cct_diagnostic_emit(const cct_diagnostic_t *diag) {
    if (!diag || !diag->message) return;

    bool colors = cct_diagnostic_colors_enabled();
    const char *level = cct_diagnostic_level_text(diag->level);

    if (diag->file_path && diag->line) {
        if (colors) {
            fprintf(stderr,
                    "cct: %s:%u:%u: %s%s%s%s: %s",
                    diag->file_path,
                    diag->line,
                    diag->column,
                    cct_diag_level_color(diag->level),
                    C_BOLD,
                    level,
                    C_RESET,
                    diag->message);
        } else {
            fprintf(stderr,
                    "cct: %s:%u:%u: %s: %s",
                    diag->file_path,
                    diag->line,
                    diag->column,
                    level,
                    diag->message);
        }
    } else {
        if (colors) {
            fprintf(stderr,
                    "cct: %s%s%s%s: %s",
                    cct_diag_level_color(diag->level),
                    C_BOLD,
                    level,
                    C_RESET,
                    diag->message);
        } else {
            fprintf(stderr, "cct: %s: %s", level, diag->message);
        }
    }

    if (diag->code_label && diag->code_label[0] != '\0') {
        fprintf(stderr, " [%s]", diag->code_label);
    }
    fputc('\n', stderr);

    cct_diag_emit_snippet(diag);

    if (diag->suggestion && diag->suggestion[0] != '\0') {
        fprintf(stderr, "   = suggestion: %s\n", diag->suggestion);
        fprintf(stderr, "   = hint: %s\n", diag->suggestion);
    }
}

void cct_error(const char *message) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_ERROR,
        .message = message ? message : "unknown error",
        .file_path = NULL,
        .line = 0,
        .column = 0,
        .suggestion = NULL,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}

void cct_error_at(const char *file, u32 line, u32 column, const char *message) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_ERROR,
        .message = message ? message : "unknown error",
        .file_path = file,
        .line = line,
        .column = column,
        .suggestion = NULL,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}

void cct_error_with_suggestion(const char *file, u32 line, u32 column, const char *message, const char *suggestion) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_ERROR,
        .message = message ? message : "unknown error",
        .file_path = file,
        .line = line,
        .column = column,
        .suggestion = suggestion,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}

void cct_warning(const char *message) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_WARNING,
        .message = message ? message : "warning",
        .file_path = NULL,
        .line = 0,
        .column = 0,
        .suggestion = NULL,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}

void cct_note(const char *message) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_NOTE,
        .message = message ? message : "note",
        .file_path = NULL,
        .line = 0,
        .column = 0,
        .suggestion = NULL,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}

void cct_hint(const char *message) {
    cct_diagnostic_t diag = {
        .level = CCT_DIAG_HINT,
        .message = message ? message : "hint",
        .file_path = NULL,
        .line = 0,
        .column = 0,
        .suggestion = NULL,
        .code_label = NULL,
    };
    cct_diagnostic_emit(&diag);
}
