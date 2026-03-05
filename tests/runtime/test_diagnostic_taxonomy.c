#include "../../src/common/diagnostic.h"
#include "../../src/common/errors.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    cct_diagnostic_t diag;
} emit_ctx_t;

static int capture_stderr(void (*fn)(void *), void *ctx, char *out, size_t out_cap) {
    int pipefd[2];
    int saved_stderr;
    ssize_t n;

    if (!fn || !out || out_cap == 0) return 0;
    out[0] = '\0';

    if (pipe(pipefd) != 0) return 0;
    fflush(stderr);

    saved_stderr = dup(fileno(stderr));
    if (saved_stderr < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }

    if (dup2(pipefd[1], fileno(stderr)) < 0) {
        close(saved_stderr);
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }
    close(pipefd[1]);

    fn(ctx);
    fflush(stderr);

    if (dup2(saved_stderr, fileno(stderr)) < 0) {
        close(saved_stderr);
        close(pipefd[0]);
        return 0;
    }
    close(saved_stderr);

    n = read(pipefd[0], out, out_cap - 1);
    close(pipefd[0]);
    if (n < 0) return 0;
    out[n] = '\0';
    return 1;
}

static void emit_diag(void *ctx) {
    const emit_ctx_t *e = (const emit_ctx_t*)ctx;
    cct_diagnostic_emit(&e->diag);
}

static void emit_warning(void *ctx) {
    (void)ctx;
    cct_warning("warning taxonomy check");
}

static void emit_note(void *ctx) {
    (void)ctx;
    cct_note("note taxonomy check");
}

static void emit_hint(void *ctx) {
    (void)ctx;
    cct_hint("hint taxonomy check");
}

static void emit_error_printf(void *ctx) {
    (void)ctx;
    cct_error_printf(CCT_ERROR_INVALID_ARGUMENT, "invalid argument diagnostic check");
}

static int contains(const char *haystack, const char *needle) {
    return haystack && needle && strstr(haystack, needle) != NULL;
}

static int test_level_text_mapping(void) {
    return strcmp(cct_diagnostic_level_text(CCT_DIAG_ERROR), "error") == 0 &&
           strcmp(cct_diagnostic_level_text(CCT_DIAG_WARNING), "warning") == 0 &&
           strcmp(cct_diagnostic_level_text(CCT_DIAG_NOTE), "note") == 0 &&
           strcmp(cct_diagnostic_level_text(CCT_DIAG_HINT), "hint") == 0;
}

static int test_emit_warning_prefix(void) {
    char out[1024];
    if (!capture_stderr(emit_warning, NULL, out, sizeof(out))) return 0;
    return contains(out, "cct: warning: warning taxonomy check");
}

static int test_emit_note_prefix(void) {
    char out[1024];
    if (!capture_stderr(emit_note, NULL, out, sizeof(out))) return 0;
    return contains(out, "cct: note: note taxonomy check");
}

static int test_emit_hint_prefix(void) {
    char out[1024];
    if (!capture_stderr(emit_hint, NULL, out, sizeof(out))) return 0;
    return contains(out, "cct: hint: hint taxonomy check");
}

static int test_suggestion_and_hint_lines(void) {
    char out[2048];
    emit_ctx_t ctx;
    ctx.diag.level = CCT_DIAG_ERROR;
    ctx.diag.message = "typed diagnostic check";
    ctx.diag.file_path = "tests/integration/diagnostic_type_error_12a.cct";
    ctx.diag.line = 5;
    ctx.diag.column = 3;
    ctx.diag.suggestion = "use matching numeric type";
    ctx.diag.code_label = "Semantic error";

    if (!capture_stderr(emit_diag, &ctx, out, sizeof(out))) return 0;
    return contains(out, "cct: tests/integration/diagnostic_type_error_12a.cct:5:3: error: typed diagnostic check [Semantic error]") &&
           contains(out, "suggestion: use matching numeric type") &&
           contains(out, "hint: use matching numeric type");
}

static int test_error_printf_uses_canonical_diagnostic(void) {
    char out[1024];
    if (!capture_stderr(emit_error_printf, NULL, out, sizeof(out))) return 0;
    return contains(out, "cct: error: invalid argument diagnostic check") &&
           contains(out, "[Invalid argument]");
}

int main(void) {
    if (!test_level_text_mapping()) {
        fprintf(stderr, "test_level_text_mapping failed\n");
        return 1;
    }
    if (!test_emit_warning_prefix()) {
        fprintf(stderr, "test_emit_warning_prefix failed\n");
        return 1;
    }
    if (!test_emit_note_prefix()) {
        fprintf(stderr, "test_emit_note_prefix failed\n");
        return 1;
    }
    if (!test_emit_hint_prefix()) {
        fprintf(stderr, "test_emit_hint_prefix failed\n");
        return 1;
    }
    if (!test_suggestion_and_hint_lines()) {
        fprintf(stderr, "test_suggestion_and_hint_lines failed\n");
        return 1;
    }
    if (!test_error_printf_uses_canonical_diagnostic()) {
        fprintf(stderr, "test_error_printf_uses_canonical_diagnostic failed\n");
        return 1;
    }
    printf("test_diagnostic_taxonomy: ok\n");
    return 0;
}
