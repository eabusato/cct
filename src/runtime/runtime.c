/*
 * CCT — Clavicula Turing
 * Runtime Support (FASE 8D)
 *
 * Minimal helper emission for the C-hosted backend.
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

#include <string.h>

void cct_runtime_codegen_config_defaults(cct_runtime_codegen_config_t *cfg) {
    if (!cfg) return;
    cfg->emit_scribe_helpers = true;
    cfg->emit_fail_helper = true;
    cfg->emit_memory_helpers = true;
    cfg->emit_fluxus_helpers = true;
    cfg->emit_io_helpers = true;
    cfg->emit_fs_helpers = true;
    cfg->emit_path_helpers = true;
    cfg->emit_random_helpers = true;
    cfg->emit_verbum_helpers = true;
    cfg->emit_fmt_helpers = true;
}

bool cct_runtime_emit_c_helpers(FILE *out, const cct_runtime_codegen_config_t *cfg) {
    if (!out || !cfg) return false;

    fputs("/* ===== CCT Runtime Helpers (FASE 8D / final phase-8 failure-control + memory runtime layer) ===== */\n", out);

    if (cfg->emit_fail_helper) {
        fputs("static void cct_rt_fail(const char *msg) {\n", out);
        fputs("    fprintf(stderr, \"cct runtime: %s\\n\", (msg && *msg) ? msg : \"failure\");\n", out);
        fputs("    exit(1);\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_fractum_active = 0;\n", out);
        fputs("static const char *cct_rt_fractum_msg = NULL;\n\n", out);

        fputs("static void cct_rt_fractum_throw_str(const char *msg) {\n", out);
        fputs("    cct_rt_fractum_active = 1;\n", out);
        fputs("    cct_rt_fractum_msg = msg ? msg : \"\";\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_fractum_is_active(void) {\n", out);
        fputs("    return cct_rt_fractum_active;\n", out);
        fputs("}\n\n", out);

        fputs("static const char *cct_rt_fractum_peek(void) {\n", out);
        fputs("    return cct_rt_fractum_msg ? cct_rt_fractum_msg : \"\";\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fractum_clear(void) {\n", out);
        fputs("    cct_rt_fractum_active = 0;\n", out);
        fputs("    cct_rt_fractum_msg = NULL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fractum_uncaught_abort(void) {\n", out);
        fputs("    fprintf(stderr, \"cct runtime: uncaught FRACTUM: %s\\n\", cct_rt_fractum_peek());\n", out);
        fputs("    cct_rt_fractum_clear();\n", out);
        fputs("    exit(1);\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_argc = 0;\n", out);
        fputs("static char **cct_rt_argv = NULL;\n\n", out);

        fputs("static void cct_rt_args_init(int argc, char **argv) {\n", out);
        fputs("    cct_rt_argc = argc;\n", out);
        fputs("    cct_rt_argv = argv;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_args_argc(void) {\n", out);
        fputs("    return (long long)cct_rt_argc;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_args_arg(long long i) {\n", out);
        fputs("    if (i < 0 || i >= (long long)cct_rt_argc) cct_rt_fail(\"args arg index out of bounds\");\n", out);
        fputs("    return (cct_rt_argv && cct_rt_argv[i]) ? cct_rt_argv[i] : \"\";\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_time_now_ns(void) {\n", out);
        fputs("    struct timespec ts;\n", out);
        fputs("    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) cct_rt_fail(\"time now_ns failed\");\n", out);
        fputs("    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_time_now_ms(void) {\n", out);
        fputs("    return cct_rt_time_now_ns() / 1000000LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_time_sleep_ms(long long ms) {\n", out);
        fputs("    if (ms < 0) cct_rt_fail(\"time sleep_ms expects ms >= 0\");\n", out);
        fputs("    struct timespec req;\n", out);
        fputs("    req.tv_sec = (time_t)(ms / 1000LL);\n", out);
        fputs("    req.tv_nsec = (long)((ms % 1000LL) * 1000000LL);\n", out);
        fputs("    while (nanosleep(&req, &req) != 0) {\n", out);
        fputs("        if (errno != EINTR) cct_rt_fail(\"time sleep_ms failed\");\n", out);
        fputs("    }\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_scribe_helpers) {
        fputs("static void cct_rt_scribe_str(const char *s) {\n", out);
        fputs("    fputs(s ? s : \"\", stdout);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_scribe_int(long long v) {\n", out);
        fputs("    printf(\"%lld\", v);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_scribe_bool(long long v) {\n", out);
        fputs("    printf(\"%lld\", (v ? 1LL : 0LL));\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_scribe_real(double v) {\n", out);
        fputs("    printf(\"%.17g\", v);\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_memory_helpers) {
        fputs("static union { long double ld; void *ptr; unsigned char bytes[4096]; } cct_rt_null_sentinel;\n\n", out);

        fputs("static void *cct_rt_alloc_or_fail(size_t n) {\n", out);
        fputs("    if (n == 0) n = 1;\n", out);
        fputs("    void *p = malloc(n);\n", out);
        fputs("    if (!p) cct_rt_fail(\"allocation failed\");\n", out);
        fputs("    return p;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_alloc_bytes(size_t n) {\n", out);
        fputs("    return cct_rt_alloc_or_fail(n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_free_ptr(void *p) {\n", out);
        fputs("    free(p);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_mem_alloc(long long n) {\n", out);
        fputs("    if (n <= 0) cct_rt_fail(\"mem alloc size must be > 0\");\n", out);
        fputs("    return cct_rt_alloc_or_fail((size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_mem_free(void *p) {\n", out);
        fputs("    cct_rt_free_ptr(p);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_mem_realloc(void *p, long long n) {\n", out);
        fputs("    if (n <= 0) cct_rt_fail(\"mem realloc size must be > 0\");\n", out);
        fputs("    void *out_p = realloc(p, (size_t)n);\n", out);
        fputs("    if (!out_p) cct_rt_fail(\"mem realloc failed\");\n", out);
        fputs("    return out_p;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_mem_copy(void *dest, const void *src, long long n) {\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"mem copy size must be >= 0\");\n", out);
        fputs("    if (n > 0) memcpy(dest, src, (size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_mem_set(void *p, long long value, long long n) {\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"mem set size must be >= 0\");\n", out);
        fputs("    if (n > 0) memset(p, (int)(value & 0xFF), (size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_mem_zero(void *p, long long n) {\n", out);
        fputs("    cct_rt_mem_set(p, 0LL, n);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_mem_compare(const void *a, const void *b, long long n) {\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"mem compare size must be >= 0\");\n", out);
        fputs("    if (n == 0) return 0LL;\n", out);
        fputs("    int cmp = memcmp(a, b, (size_t)n);\n", out);
        fputs("    if (cmp < 0) return -1LL;\n", out);
        fputs("    if (cmp > 0) return 1LL;\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    unsigned char *data;\n", out);
        fputs("    long long len;\n", out);
        fputs("} cct_rt_bytes_t;\n\n", out);

        fputs("static cct_rt_bytes_t *cct_rt_bytes_require(void *ptr, const char *ctx) {\n", out);
        fputs("    cct_rt_bytes_t *b = (cct_rt_bytes_t*)ptr;\n", out);
        fputs("    if (!b) cct_rt_fail((ctx && *ctx) ? ctx : \"bytes null buffer\");\n", out);
        fputs("    return b;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_bytes_new(long long size) {\n", out);
        fputs("    if (size < 0) cct_rt_fail(\"bytes_new expects size >= 0\");\n", out);
        fputs("    cct_rt_bytes_t *b = (cct_rt_bytes_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_bytes_t));\n", out);
        fputs("    b->len = size;\n", out);
        fputs("    if (size == 0) {\n", out);
        fputs("        b->data = NULL;\n", out);
        fputs("        return b;\n", out);
        fputs("    }\n", out);
        fputs("    b->data = (unsigned char*)cct_rt_alloc_or_fail((size_t)size);\n", out);
        fputs("    memset(b->data, 0, (size_t)size);\n", out);
        fputs("    return b;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_bytes_len(void *ptr) {\n", out);
        fputs("    cct_rt_bytes_t *b = cct_rt_bytes_require(ptr, \"bytes_len received null buffer\");\n", out);
        fputs("    return b->len;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_bytes_get(void *ptr, long long i) {\n", out);
        fputs("    cct_rt_bytes_t *b = cct_rt_bytes_require(ptr, \"bytes_get received null buffer\");\n", out);
        fputs("    if (i < 0 || i >= b->len) cct_rt_fail(\"bytes index out of bounds\");\n", out);
        fputs("    return (long long)b->data[(size_t)i];\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_bytes_set(void *ptr, long long i, long long v) {\n", out);
        fputs("    cct_rt_bytes_t *b = cct_rt_bytes_require(ptr, \"bytes_set received null buffer\");\n", out);
        fputs("    if (i < 0 || i >= b->len) cct_rt_fail(\"bytes index out of bounds\");\n", out);
        fputs("    if (v < 0 || v > 255) cct_rt_fail(\"bytes_set expects byte range 0..255\");\n", out);
        fputs("    b->data[(size_t)i] = (unsigned char)v;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_bytes_free(void *ptr) {\n", out);
        fputs("    cct_rt_bytes_t *b = (cct_rt_bytes_t*)ptr;\n", out);
        fputs("    if (!b) return;\n", out);
        fputs("    if (b->data) cct_rt_free_ptr(b->data);\n", out);
        fputs("    cct_rt_free_ptr(b);\n", out);
        fputs("}\n\n", out);

        if (cfg->emit_fluxus_helpers) {
            fputs("typedef struct {\n", out);
            fputs("    void *data;\n", out);
            fputs("    long long len;\n", out);
            fputs("    long long capacity;\n", out);
            fputs("    long long elem_size;\n", out);
            fputs("} cct_rt_fluxus_t;\n\n", out);

            fputs("static void cct_rt_fluxus_require(void *flux_ptr, const char *ctx) {\n", out);
            fputs("    if (!flux_ptr) cct_rt_fail((ctx && *ctx) ? ctx : \"fluxus null instance\");\n", out);
            fputs("}\n\n", out);

            fputs("static cct_rt_fluxus_t *cct_rt_fluxus_create(long long elem_size) {\n", out);
            fputs("    if (elem_size <= 0) cct_rt_fail(\"fluxus init element size must be > 0\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_fluxus_t));\n", out);
            fputs("    flux->data = NULL;\n", out);
            fputs("    flux->len = 0;\n", out);
            fputs("    flux->capacity = 0;\n", out);
            fputs("    flux->elem_size = elem_size;\n", out);
            fputs("    return flux;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_destroy(void *flux_ptr) {\n", out);
            fputs("    if (!flux_ptr) return;\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->data) cct_rt_mem_free(flux->data);\n", out);
            fputs("    cct_rt_free_ptr(flux);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_reserve(void *flux_ptr, long long new_capacity) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus reserve received null instance\");\n", out);
            fputs("    if (new_capacity < 0) cct_rt_fail(\"fluxus reserve capacity must be >= 0\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (new_capacity <= flux->capacity) return;\n", out);
            fputs("    size_t bytes = (size_t)new_capacity * (size_t)flux->elem_size;\n", out);
            fputs("    if (flux->data == NULL) flux->data = cct_rt_mem_alloc((long long)(bytes == 0 ? 1 : bytes));\n", out);
            fputs("    else flux->data = cct_rt_mem_realloc(flux->data, (long long)(bytes == 0 ? 1 : bytes));\n", out);
            fputs("    flux->capacity = new_capacity;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_grow(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus grow received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    long long next = (flux->capacity == 0) ? 8 : (flux->capacity * 2);\n", out);
            fputs("    if (next <= flux->capacity) cct_rt_fail(\"fluxus capacity overflow\");\n", out);
            fputs("    cct_rt_fluxus_reserve(flux, next);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_push(void *flux_ptr, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus push received null instance\");\n", out);
            fputs("    if (!elem_ptr) cct_rt_fail(\"fluxus push requires non-null element pointer\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->len == flux->capacity) cct_rt_fluxus_grow(flux);\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    cct_rt_mem_copy(base + ((size_t)flux->len * (size_t)flux->elem_size), elem_ptr, flux->elem_size);\n", out);
            fputs("    flux->len += 1;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_pop(void *flux_ptr, void *out_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus pop received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->len <= 0) cct_rt_fail(\"fluxus pop on empty vector\");\n", out);
            fputs("    flux->len -= 1;\n", out);
            fputs("    if (out_ptr) {\n", out);
            fputs("        unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("        cct_rt_mem_copy(out_ptr, base + ((size_t)flux->len * (size_t)flux->elem_size), flux->elem_size);\n", out);
            fputs("    }\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_fluxus_len(const void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require((void*)flux_ptr, \"fluxus len received null instance\");\n", out);
            fputs("    return ((const cct_rt_fluxus_t*)flux_ptr)->len;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_fluxus_capacity(const void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require((void*)flux_ptr, \"fluxus capacity received null instance\");\n", out);
            fputs("    return ((const cct_rt_fluxus_t*)flux_ptr)->capacity;\n", out);
            fputs("}\n\n", out);

            fputs("static void *cct_rt_fluxus_get(void *flux_ptr, long long index) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus get received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (index < 0 || index >= flux->len) cct_rt_fail(\"fluxus get index out of bounds\");\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    return (void*)(base + ((size_t)index * (size_t)flux->elem_size));\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_clear(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus clear received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    flux->len = 0;\n", out);
            fputs("}\n\n", out);
        }

        fputs("static void *cct_rt_check_not_null(void *p, const char *ctx) {\n", out);
        fputs("    if (!p) cct_rt_fail((ctx && *ctx) ? ctx : \"null pointer\");\n", out);
        fputs("    return p;\n", out);
        fputs("}\n\n", out);

        if (cfg->emit_fail_helper) {
            fputs("static void *cct_rt_check_not_null_fractum(void *p, const char *ctx) {\n", out);
            fputs("    if (!p) {\n", out);
            fputs("        cct_rt_fractum_throw_str((ctx && *ctx) ? ctx : \"null pointer\");\n", out);
            fputs("        return (void*)&cct_rt_null_sentinel;\n", out);
            fputs("    }\n", out);
            fputs("    return p;\n", out);
            fputs("}\n\n", out);
        }

        fputs("typedef struct {\n", out);
        fputs("    long long is_some;\n", out);
        fputs("    size_t value_size;\n", out);
        fputs("    unsigned char payload[];\n", out);
        fputs("} cct_rt_option_t;\n\n", out);

        fputs("static cct_rt_option_t *cct_rt_option_require(void *opt) {\n", out);
        fputs("    if (!opt) cct_rt_fail(\"option runtime received null option pointer\");\n", out);
        fputs("    return (cct_rt_option_t*)opt;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_option_some(const void *value_ptr, size_t value_size) {\n", out);
        fputs("    if (!value_ptr) cct_rt_fail(\"option Some requires non-null value pointer\");\n", out);
        fputs("    cct_rt_option_t *opt = (cct_rt_option_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_option_t) + value_size);\n", out);
        fputs("    opt->is_some = 1;\n", out);
        fputs("    opt->value_size = value_size;\n", out);
        fputs("    if (value_size > 0) memcpy(opt->payload, value_ptr, value_size);\n", out);
        fputs("    return opt;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_option_none(size_t value_size) {\n", out);
        fputs("    cct_rt_option_t *opt = (cct_rt_option_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_option_t) + value_size);\n", out);
        fputs("    opt->is_some = 0;\n", out);
        fputs("    opt->value_size = value_size;\n", out);
        fputs("    if (value_size > 0) memset(opt->payload, 0, value_size);\n", out);
        fputs("    return opt;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_option_is_some(const void *opt_ptr) {\n", out);
        fputs("    const cct_rt_option_t *opt = (const cct_rt_option_t*)opt_ptr;\n", out);
        fputs("    if (!opt) cct_rt_fail(\"option runtime received null option pointer\");\n", out);
        fputs("    return opt->is_some ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_option_is_none(const void *opt_ptr) {\n", out);
        fputs("    return cct_rt_option_is_some(opt_ptr) ? 0LL : 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_option_unwrap_ptr(void *opt_ptr) {\n", out);
        fputs("    cct_rt_option_t *opt = cct_rt_option_require(opt_ptr);\n", out);
        fputs("    if (!opt->is_some) cct_rt_fail(\"option_unwrap on None\");\n", out);
        fputs("    return (void*)opt->payload;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_option_expect_ptr(void *opt_ptr, const char *message) {\n", out);
        fputs("    cct_rt_option_t *opt = cct_rt_option_require(opt_ptr);\n", out);
        fputs("    if (!opt->is_some) cct_rt_fail((message && *message) ? message : \"option_expect on None\");\n", out);
        fputs("    return (void*)opt->payload;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_option_free(void *opt_ptr) {\n", out);
        fputs("    if (!opt_ptr) return;\n", out);
        fputs("    cct_rt_free_ptr(opt_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    long long is_ok;\n", out);
        fputs("    size_t ok_size;\n", out);
        fputs("    size_t err_size;\n", out);
        fputs("    unsigned char payload[];\n", out);
        fputs("} cct_rt_result_t;\n\n", out);

        fputs("static size_t cct_rt_result_payload_size(size_t ok_size, size_t err_size) {\n", out);
        fputs("    return ok_size >= err_size ? ok_size : err_size;\n", out);
        fputs("}\n\n", out);

        fputs("static cct_rt_result_t *cct_rt_result_require(void *res) {\n", out);
        fputs("    if (!res) cct_rt_fail(\"result runtime received null result pointer\");\n", out);
        fputs("    return (cct_rt_result_t*)res;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_result_ok(const void *value_ptr, size_t ok_size, size_t err_size) {\n", out);
        fputs("    if (!value_ptr) cct_rt_fail(\"result Ok requires non-null payload pointer\");\n", out);
        fputs("    size_t payload_size = cct_rt_result_payload_size(ok_size, err_size);\n", out);
        fputs("    cct_rt_result_t *res = (cct_rt_result_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_result_t) + payload_size);\n", out);
        fputs("    res->is_ok = 1;\n", out);
        fputs("    res->ok_size = ok_size;\n", out);
        fputs("    res->err_size = err_size;\n", out);
        fputs("    if (payload_size > 0) memset(res->payload, 0, payload_size);\n", out);
        fputs("    if (ok_size > 0) memcpy(res->payload, value_ptr, ok_size);\n", out);
        fputs("    return res;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_result_err(const void *err_ptr, size_t ok_size, size_t err_size) {\n", out);
        fputs("    if (!err_ptr) cct_rt_fail(\"result Err requires non-null payload pointer\");\n", out);
        fputs("    size_t payload_size = cct_rt_result_payload_size(ok_size, err_size);\n", out);
        fputs("    cct_rt_result_t *res = (cct_rt_result_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_result_t) + payload_size);\n", out);
        fputs("    res->is_ok = 0;\n", out);
        fputs("    res->ok_size = ok_size;\n", out);
        fputs("    res->err_size = err_size;\n", out);
        fputs("    if (payload_size > 0) memset(res->payload, 0, payload_size);\n", out);
        fputs("    if (err_size > 0) memcpy(res->payload, err_ptr, err_size);\n", out);
        fputs("    return res;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_result_is_ok(const void *res_ptr) {\n", out);
        fputs("    const cct_rt_result_t *res = (const cct_rt_result_t*)res_ptr;\n", out);
        fputs("    if (!res) cct_rt_fail(\"result runtime received null result pointer\");\n", out);
        fputs("    return res->is_ok ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_result_is_err(const void *res_ptr) {\n", out);
        fputs("    return cct_rt_result_is_ok(res_ptr) ? 0LL : 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_result_unwrap_ptr(void *res_ptr) {\n", out);
        fputs("    cct_rt_result_t *res = cct_rt_result_require(res_ptr);\n", out);
        fputs("    if (!res->is_ok) cct_rt_fail(\"result_unwrap on Err\");\n", out);
        fputs("    return (void*)res->payload;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_result_unwrap_err_ptr(void *res_ptr) {\n", out);
        fputs("    cct_rt_result_t *res = cct_rt_result_require(res_ptr);\n", out);
        fputs("    if (res->is_ok) cct_rt_fail(\"result_unwrap_err on Ok\");\n", out);
        fputs("    return (void*)res->payload;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_result_expect_ptr(void *res_ptr, const char *message) {\n", out);
        fputs("    cct_rt_result_t *res = cct_rt_result_require(res_ptr);\n", out);
        fputs("    if (!res->is_ok) cct_rt_fail((message && *message) ? message : \"result_expect on Err\");\n", out);
        fputs("    return (void*)res->payload;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_result_free(void *res_ptr) {\n", out);
        fputs("    if (!res_ptr) return;\n", out);
        fputs("    cct_rt_free_ptr(res_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("typedef long long (*cct_rt_collection_map_fn_t)(long long);\n", out);
        fputs("typedef long long (*cct_rt_collection_pred_fn_t)(long long);\n", out);
        fputs("typedef long long (*cct_rt_collection_fold_fn_t)(long long, long long);\n\n", out);

        fputs("static long long cct_rt_collection_require_word_size(long long size, const char *ctx) {\n", out);
        fputs("    if (size != (long long)sizeof(long long)) {\n", out);
        fputs("        cct_rt_fail((ctx && *ctx) ? ctx : \"collection operations in subset 12D.2 require word-sized types\");\n", out);
        fputs("    }\n", out);
        fputs("    return size;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_collection_read_word(const void *ptr, long long size, const char *ctx) {\n", out);
        fputs("    if (!ptr) cct_rt_fail((ctx && *ctx) ? ctx : \"collection read received null pointer\");\n", out);
        fputs("    cct_rt_collection_require_word_size(size, ctx);\n", out);
        fputs("    long long v = 0;\n", out);
        fputs("    memcpy(&v, ptr, sizeof(long long));\n", out);
        fputs("    return v;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_collection_write_word(void *ptr, long long size, long long value, const char *ctx) {\n", out);
        fputs("    if (!ptr) cct_rt_fail((ctx && *ctx) ? ctx : \"collection write received null pointer\");\n", out);
        fputs("    cct_rt_collection_require_word_size(size, ctx);\n", out);
        fputs("    memcpy(ptr, &value, sizeof(long long));\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_fluxus_map(void *flux_ptr, long long item_size, long long result_size, void *fn_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_map received null fluxus\");\n", out);
        fputs("    if (!fn_ptr) cct_rt_fail(\"collection_fluxus_map received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_map only supports word-sized input in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_require_word_size(result_size, \"collection_fluxus_map only supports word-sized output in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_map_fn_t fn = (cct_rt_collection_map_fn_t)fn_ptr;\n", out);
        fputs("    void *result_flux = cct_rt_fluxus_create(result_size);\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_map input read failed\");\n", out);
        fputs("        long long mapped = fn(item);\n", out);
        fputs("        cct_rt_fluxus_push(result_flux, &mapped);\n", out);
        fputs("    }\n", out);
        fputs("    return result_flux;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_fluxus_filter(void *flux_ptr, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_filter received null fluxus\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_fluxus_filter received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_filter only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    void *result_flux = cct_rt_fluxus_create(item_size);\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_filter input read failed\");\n", out);
        fputs("        if (pred(item)) cct_rt_fluxus_push(result_flux, &item);\n", out);
        fputs("    }\n", out);
        fputs("    return result_flux;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_fluxus_fold(void *flux_ptr, long long item_size, const void *initial_ptr, long long acc_size, void *fn_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_fold received null fluxus\");\n", out);
        fputs("    if (!initial_ptr) cct_rt_fail(\"collection_fluxus_fold received null initial accumulator pointer\");\n", out);
        fputs("    if (!fn_ptr) cct_rt_fail(\"collection_fluxus_fold received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_fold only supports word-sized element type in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_require_word_size(acc_size, \"collection_fluxus_fold only supports word-sized accumulator type in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_fold_fn_t fn = (cct_rt_collection_fold_fn_t)fn_ptr;\n", out);
        fputs("    long long acc = cct_rt_collection_read_word(initial_ptr, acc_size, \"collection_fluxus_fold failed to read initial accumulator\");\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_fold input read failed\");\n", out);
        fputs("        acc = fn(acc, item);\n", out);
        fputs("    }\n", out);
        fputs("    void *acc_ptr = cct_rt_alloc_or_fail((size_t)acc_size);\n", out);
        fputs("    cct_rt_collection_write_word(acc_ptr, acc_size, acc, \"collection_fluxus_fold failed to materialize accumulator\");\n", out);
        fputs("    return acc_ptr;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_fluxus_find(void *flux_ptr, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_find received null fluxus\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_fluxus_find received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_find only supports word-sized element types in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_find input read failed\");\n", out);
        fputs("        if (pred(item)) return cct_rt_option_some(item_ptr, (size_t)item_size);\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_option_none((size_t)item_size);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_collection_fluxus_any(void *flux_ptr, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_any received null fluxus\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_fluxus_any received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_any only supports word-sized element types in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_any input read failed\");\n", out);
        fputs("        if (pred(item)) return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_collection_fluxus_all(void *flux_ptr, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    cct_rt_fluxus_require(flux_ptr, \"collection_fluxus_all received null fluxus\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_fluxus_all received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_fluxus_all only supports word-sized element types in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    long long len = cct_rt_fluxus_len(flux_ptr);\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        void *item_ptr = cct_rt_fluxus_get(flux_ptr, i);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_fluxus_all input read failed\");\n", out);
        fputs("        if (!pred(item)) return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_series_map(void *arr_ptr, long long len, long long item_size, long long result_size, void *fn_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_map received null array pointer\");\n", out);
        fputs("    if (!fn_ptr) cct_rt_fail(\"collection_series_map received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_map only supports word-sized input in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_require_word_size(result_size, \"collection_series_map only supports word-sized output in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_map_fn_t fn = (cct_rt_collection_map_fn_t)fn_ptr;\n", out);
        fputs("    void *result_flux = cct_rt_fluxus_create(result_size);\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        long long item = cct_rt_collection_read_word(base + ((size_t)i * (size_t)item_size), item_size, \"collection_series_map input read failed\");\n", out);
        fputs("        long long mapped = fn(item);\n", out);
        fputs("        cct_rt_fluxus_push(result_flux, &mapped);\n", out);
        fputs("    }\n", out);
        fputs("    return result_flux;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_series_filter(void *arr_ptr, long long len, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_filter received null array pointer\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_series_filter received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_filter only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    void *result_flux = cct_rt_fluxus_create(item_size);\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        long long item = cct_rt_collection_read_word(base + ((size_t)i * (size_t)item_size), item_size, \"collection_series_filter input read failed\");\n", out);
        fputs("        if (pred(item)) cct_rt_fluxus_push(result_flux, &item);\n", out);
        fputs("    }\n", out);
        fputs("    return result_flux;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_series_reduce(void *arr_ptr, long long len, long long item_size, const void *initial_ptr, long long acc_size, void *fn_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_reduce received null array pointer\");\n", out);
        fputs("    if (!initial_ptr) cct_rt_fail(\"collection_series_reduce received null initial accumulator pointer\");\n", out);
        fputs("    if (!fn_ptr) cct_rt_fail(\"collection_series_reduce received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_reduce only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_require_word_size(acc_size, \"collection_series_reduce only supports word-sized accumulators in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_fold_fn_t fn = (cct_rt_collection_fold_fn_t)fn_ptr;\n", out);
        fputs("    long long acc = cct_rt_collection_read_word(initial_ptr, acc_size, \"collection_series_reduce failed to read initial accumulator\");\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        long long item = cct_rt_collection_read_word(base + ((size_t)i * (size_t)item_size), item_size, \"collection_series_reduce input read failed\");\n", out);
        fputs("        acc = fn(acc, item);\n", out);
        fputs("    }\n", out);
        fputs("    void *acc_ptr = cct_rt_alloc_or_fail((size_t)acc_size);\n", out);
        fputs("    cct_rt_collection_write_word(acc_ptr, acc_size, acc, \"collection_series_reduce failed to materialize accumulator\");\n", out);
        fputs("    return acc_ptr;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_collection_series_find(void *arr_ptr, long long len, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_find received null array pointer\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_series_find received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_find only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        unsigned char *item_ptr = base + ((size_t)i * (size_t)item_size);\n", out);
        fputs("        long long item = cct_rt_collection_read_word(item_ptr, item_size, \"collection_series_find input read failed\");\n", out);
        fputs("        if (pred(item)) return cct_rt_option_some(item_ptr, (size_t)item_size);\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_option_none((size_t)item_size);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_collection_series_any(void *arr_ptr, long long len, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_any received null array pointer\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_series_any received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_any only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        long long item = cct_rt_collection_read_word(base + ((size_t)i * (size_t)item_size), item_size, \"collection_series_any input read failed\");\n", out);
        fputs("        if (pred(item)) return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_collection_series_all(void *arr_ptr, long long len, long long item_size, void *predicate_ptr) {\n", out);
        fputs("    if (!arr_ptr) cct_rt_fail(\"collection_series_all received null array pointer\");\n", out);
        fputs("    if (!predicate_ptr) cct_rt_fail(\"collection_series_all received null callback\");\n", out);
        fputs("    cct_rt_collection_require_word_size(item_size, \"collection_series_all only supports word-sized elements in subset 12D.2\");\n", out);
        fputs("    cct_rt_collection_pred_fn_t pred = (cct_rt_collection_pred_fn_t)predicate_ptr;\n", out);
        fputs("    unsigned char *base = (unsigned char*)arr_ptr;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        long long item = cct_rt_collection_read_word(base + ((size_t)i * (size_t)item_size), item_size, \"collection_series_all input read failed\");\n", out);
        fputs("        if (!pred(item)) return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct cct_rt_map_entry {\n", out);
        fputs("    unsigned char *key;\n", out);
        fputs("    unsigned char *value;\n", out);
        fputs("    struct cct_rt_map_entry *next;\n", out);
        fputs("} cct_rt_map_entry_t;\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_map_entry_t **buckets;\n", out);
        fputs("    size_t capacity;\n", out);
        fputs("    size_t len;\n", out);
        fputs("    size_t key_size;\n", out);
        fputs("    size_t value_size;\n", out);
        fputs("} cct_rt_map_t;\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_map_t *map;\n", out);
        fputs("} cct_rt_set_t;\n\n", out);

        fputs("static size_t cct_rt_map_min_storage_size(size_t n) {\n", out);
        fputs("    return (n == 0) ? 1 : n;\n", out);
        fputs("}\n\n", out);

        fputs("static size_t cct_rt_map_hash_bytes(const void *data, size_t size) {\n", out);
        fputs("    const unsigned char *p = (const unsigned char*)data;\n", out);
        fputs("    size_t h = (size_t)1469598103934665603ULL;\n", out);
        fputs("    for (size_t i = 0; i < size; i++) {\n", out);
        fputs("        h ^= (size_t)p[i];\n", out);
        fputs("        h *= (size_t)1099511628211ULL;\n", out);
        fputs("    }\n", out);
        fputs("    return h;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_require(void *map_ptr, const char *ctx) {\n", out);
        fputs("    if (!map_ptr) cct_rt_fail((ctx && *ctx) ? ctx : \"map received null instance\");\n", out);
        fputs("}\n\n", out);

        fputs("static cct_rt_map_entry_t *cct_rt_map_entry_new(const void *key_ptr, size_t key_size, const void *value_ptr, size_t value_size) {\n", out);
        fputs("    cct_rt_map_entry_t *e = (cct_rt_map_entry_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_map_entry_t));\n", out);
        fputs("    e->key = (unsigned char*)cct_rt_alloc_or_fail(key_size);\n", out);
        fputs("    e->value = (unsigned char*)cct_rt_alloc_or_fail(value_size);\n", out);
        fputs("    memcpy(e->key, key_ptr, key_size);\n", out);
        fputs("    memcpy(e->value, value_ptr, value_size);\n", out);
        fputs("    e->next = NULL;\n", out);
        fputs("    return e;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_entry_free(cct_rt_map_entry_t *e) {\n", out);
        fputs("    if (!e) return;\n", out);
        fputs("    cct_rt_free_ptr(e->key);\n", out);
        fputs("    cct_rt_free_ptr(e->value);\n", out);
        fputs("    cct_rt_free_ptr(e);\n", out);
        fputs("}\n\n", out);

        fputs("static size_t cct_rt_map_bucket_index(cct_rt_map_t *map, const void *key_ptr) {\n", out);
        fputs("    size_t h = cct_rt_map_hash_bytes(key_ptr, map->key_size);\n", out);
        fputs("    return (map->capacity == 0) ? 0 : (h % map->capacity);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_rehash(cct_rt_map_t *map, size_t new_capacity) {\n", out);
        fputs("    cct_rt_map_entry_t **new_buckets = (cct_rt_map_entry_t**)calloc(new_capacity, sizeof(cct_rt_map_entry_t*));\n", out);
        fputs("    if (!new_buckets) cct_rt_fail(\"map rehash allocation failed\");\n", out);
        fputs("    for (size_t i = 0; i < map->capacity; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = map->buckets[i];\n", out);
        fputs("        while (e) {\n", out);
        fputs("            cct_rt_map_entry_t *next = e->next;\n", out);
        fputs("            size_t idx = cct_rt_map_hash_bytes(e->key, map->key_size) % new_capacity;\n", out);
        fputs("            e->next = new_buckets[idx];\n", out);
        fputs("            new_buckets[idx] = e;\n", out);
        fputs("            e = next;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_free_ptr(map->buckets);\n", out);
        fputs("    map->buckets = new_buckets;\n", out);
        fputs("    map->capacity = new_capacity;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_map_init(long long key_size, long long value_size) {\n", out);
        fputs("    if (key_size <= 0) cct_rt_fail(\"map_init key size must be > 0\");\n", out);
        fputs("    if (value_size < 0) cct_rt_fail(\"map_init value size must be >= 0\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_map_t));\n", out);
        fputs("    map->capacity = 16;\n", out);
        fputs("    map->len = 0;\n", out);
        fputs("    map->key_size = cct_rt_map_min_storage_size((size_t)key_size);\n", out);
        fputs("    map->value_size = cct_rt_map_min_storage_size((size_t)value_size);\n", out);
        fputs("    map->buckets = (cct_rt_map_entry_t**)calloc(map->capacity, sizeof(cct_rt_map_entry_t*));\n", out);
        fputs("    if (!map->buckets) cct_rt_fail(\"map_init buckets allocation failed\");\n", out);
        fputs("    return map;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_insert(void *map_ptr, const void *key_ptr, const void *value_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_insert received null map\");\n", out);
        fputs("    if (!key_ptr) cct_rt_fail(\"map_insert received null key pointer\");\n", out);
        fputs("    if (!value_ptr) cct_rt_fail(\"map_insert received null value pointer\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    if (map->capacity == 0) cct_rt_fail(\"map_insert map capacity invalid\");\n", out);
        fputs("    if ((map->len + 1) * 4 >= map->capacity * 3) {\n", out);
        fputs("        cct_rt_map_rehash(map, map->capacity * 2);\n", out);
        fputs("    }\n", out);
        fputs("    size_t idx = cct_rt_map_bucket_index(map, key_ptr);\n", out);
        fputs("    cct_rt_map_entry_t *e = map->buckets[idx];\n", out);
        fputs("    while (e) {\n", out);
        fputs("        if (memcmp(e->key, key_ptr, map->key_size) == 0) {\n", out);
        fputs("            memcpy(e->value, value_ptr, map->value_size);\n", out);
        fputs("            return;\n", out);
        fputs("        }\n", out);
        fputs("        e = e->next;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_map_entry_t *ne = cct_rt_map_entry_new(key_ptr, map->key_size, value_ptr, map->value_size);\n", out);
        fputs("    ne->next = map->buckets[idx];\n", out);
        fputs("    map->buckets[idx] = ne;\n", out);
        fputs("    map->len += 1;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_map_get_ptr(void *map_ptr, const void *key_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_get_ptr received null map\");\n", out);
        fputs("    if (!key_ptr) cct_rt_fail(\"map_get_ptr received null key pointer\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    size_t idx = cct_rt_map_bucket_index(map, key_ptr);\n", out);
        fputs("    cct_rt_map_entry_t *e = map->buckets[idx];\n", out);
        fputs("    while (e) {\n", out);
        fputs("        if (memcmp(e->key, key_ptr, map->key_size) == 0) return (void*)e->value;\n", out);
        fputs("        e = e->next;\n", out);
        fputs("    }\n", out);
        fputs("    return NULL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_contains(void *map_ptr, const void *key_ptr) {\n", out);
        fputs("    return cct_rt_map_get_ptr(map_ptr, key_ptr) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_remove(void *map_ptr, const void *key_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_remove received null map\");\n", out);
        fputs("    if (!key_ptr) cct_rt_fail(\"map_remove received null key pointer\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    size_t idx = cct_rt_map_bucket_index(map, key_ptr);\n", out);
        fputs("    cct_rt_map_entry_t *e = map->buckets[idx];\n", out);
        fputs("    cct_rt_map_entry_t *prev = NULL;\n", out);
        fputs("    while (e) {\n", out);
        fputs("        if (memcmp(e->key, key_ptr, map->key_size) == 0) {\n", out);
        fputs("            if (prev) prev->next = e->next;\n", out);
        fputs("            else map->buckets[idx] = e->next;\n", out);
        fputs("            cct_rt_map_entry_free(e);\n", out);
        fputs("            map->len -= 1;\n", out);
        fputs("            return 1LL;\n", out);
        fputs("        }\n", out);
        fputs("        prev = e;\n", out);
        fputs("        e = e->next;\n", out);
        fputs("    }\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_len(void *map_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_len received null map\");\n", out);
        fputs("    return (long long)((cct_rt_map_t*)map_ptr)->len;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_is_empty(void *map_ptr) {\n", out);
        fputs("    return cct_rt_map_len(map_ptr) == 0 ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_capacity(void *map_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_capacity received null map\");\n", out);
        fputs("    return (long long)((cct_rt_map_t*)map_ptr)->capacity;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_clear(void *map_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_clear received null map\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    for (size_t i = 0; i < map->capacity; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = map->buckets[i];\n", out);
        fputs("        while (e) {\n", out);
        fputs("            cct_rt_map_entry_t *next = e->next;\n", out);
        fputs("            cct_rt_map_entry_free(e);\n", out);
        fputs("            e = next;\n", out);
        fputs("        }\n", out);
        fputs("        map->buckets[i] = NULL;\n", out);
        fputs("    }\n", out);
        fputs("    map->len = 0;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_reserve(void *map_ptr, long long additional) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_reserve received null map\");\n", out);
        fputs("    if (additional < 0) cct_rt_fail(\"map_reserve additional must be >= 0\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    size_t need = map->len + (size_t)additional;\n", out);
        fputs("    size_t target = map->capacity;\n", out);
        fputs("    while (need * 4 >= target * 3) {\n", out);
        fputs("        target *= 2;\n", out);
        fputs("    }\n", out);
        fputs("    if (target > map->capacity) cct_rt_map_rehash(map, target);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_free(void *map_ptr) {\n", out);
        fputs("    if (!map_ptr) return;\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    cct_rt_map_clear(map);\n", out);
        fputs("    cct_rt_free_ptr(map->buckets);\n", out);
        fputs("    cct_rt_free_ptr(map);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_init(long long item_size) {\n", out);
        fputs("    cct_rt_set_t *set = (cct_rt_set_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_set_t));\n", out);
        fputs("    set->map = (cct_rt_map_t*)cct_rt_map_init(item_size, 1);\n", out);
        fputs("    return set;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_insert(void *set_ptr, const void *item_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_insert received null set\");\n", out);
        fputs("    if (!item_ptr) cct_rt_fail(\"set_insert received null item pointer\");\n", out);
        fputs("    cct_rt_set_t *set = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    size_t before = set->map->len;\n", out);
        fputs("    unsigned char marker = 1;\n", out);
        fputs("    cct_rt_map_insert(set->map, item_ptr, &marker);\n", out);
        fputs("    return (set->map->len > before) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_remove(void *set_ptr, const void *item_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_remove received null set\");\n", out);
        fputs("    if (!item_ptr) cct_rt_fail(\"set_remove received null item pointer\");\n", out);
        fputs("    return cct_rt_map_remove(((cct_rt_set_t*)set_ptr)->map, item_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_contains(void *set_ptr, const void *item_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_contains received null set\");\n", out);
        fputs("    if (!item_ptr) cct_rt_fail(\"set_contains received null item pointer\");\n", out);
        fputs("    return cct_rt_map_contains(((cct_rt_set_t*)set_ptr)->map, item_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_len(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_len received null set\");\n", out);
        fputs("    return cct_rt_map_len(((cct_rt_set_t*)set_ptr)->map);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_is_empty(void *set_ptr) {\n", out);
        fputs("    return cct_rt_set_len(set_ptr) == 0 ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_set_clear(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_clear received null set\");\n", out);
        fputs("    cct_rt_map_clear(((cct_rt_set_t*)set_ptr)->map);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_set_free(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) return;\n", out);
        fputs("    cct_rt_set_t *set = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    cct_rt_map_free(set->map);\n", out);
        fputs("    cct_rt_free_ptr(set);\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_verbum_helpers) {
        fputs("static long long cct_rt_verbum_len(const char *s) {\n", out);
        fputs("    return (long long)strlen(s ? s : \"\");\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_concat(const char *a, const char *b) {\n", out);
        fputs("    const char *sa = a ? a : \"\";\n", out);
        fputs("    const char *sb = b ? b : \"\";\n", out);
        fputs("    size_t la = strlen(sa);\n", out);
        fputs("    size_t lb = strlen(sb);\n", out);
        fputs("    char *out_s = (char*)malloc(la + lb + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum concat allocation failed\");\n", out);
        fputs("    memcpy(out_s, sa, la);\n", out);
        fputs("    memcpy(out_s + la, sb, lb);\n", out);
        fputs("    out_s[la + lb] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_compare(const char *a, const char *b) {\n", out);
        fputs("    int cmp = strcmp(a ? a : \"\", b ? b : \"\");\n", out);
        fputs("    if (cmp < 0) return -1LL;\n", out);
        fputs("    if (cmp > 0) return 1LL;\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_substring(const char *s, long long inicio, long long fim) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    if (inicio < 0 || fim < 0 || inicio > fim || (size_t)fim > slen) {\n", out);
        fputs("        cct_rt_fail(\"verbum substring bounds invalid\");\n", out);
        fputs("    }\n", out);
        fputs("    size_t start = (size_t)inicio;\n", out);
        fputs("    size_t end = (size_t)fim;\n", out);
        fputs("    size_t out_len = end - start;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum substring allocation failed\");\n", out);
        fputs("    if (out_len > 0) memcpy(out_s, src + start, out_len);\n", out);
        fputs("    out_s[out_len] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_trim(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t begin = 0;\n", out);
        fputs("    while (begin < slen && isspace((unsigned char)src[begin])) begin++;\n", out);
        fputs("    size_t end = slen;\n", out);
        fputs("    while (end > begin && isspace((unsigned char)src[end - 1])) end--;\n", out);
        fputs("    size_t out_len = end - begin;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum trim allocation failed\");\n", out);
        fputs("    if (out_len > 0) memcpy(out_s, src + begin, out_len);\n", out);
        fputs("    out_s[out_len] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_find(const char *s, const char *sub) {\n", out);
        fputs("    const char *hay = s ? s : \"\";\n", out);
        fputs("    const char *needle = sub ? sub : \"\";\n", out);
        fputs("    if (*needle == '\\0') return 0LL;\n", out);
        fputs("    const char *p = strstr(hay, needle);\n", out);
        fputs("    if (!p) return -1LL;\n", out);
        fputs("    return (long long)(p - hay);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_char_at(const char *s, long long i) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    if (i < 0 || (size_t)i >= n) cct_rt_fail(\"verbum char_at index out of bounds\");\n", out);
        fputs("    return (long long)(unsigned char)src[(size_t)i];\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_from_char(long long c) {\n", out);
        fputs("    if (c < 0 || c > 255) cct_rt_fail(\"verbum from_char expects byte range 0..255\");\n", out);
        fputs("    char *out_s = (char*)malloc(2);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum from_char allocation failed\");\n", out);
        fputs("    out_s[0] = (char)(unsigned char)c;\n", out);
        fputs("    out_s[1] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_char_is_digit(long long c) {\n", out);
        fputs("    return (c >= 48LL && c <= 57LL) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_char_is_alpha(long long c) {\n", out);
        fputs("    if (c >= 65LL && c <= 90LL) return 1LL;\n", out);
        fputs("    if (c >= 97LL && c <= 122LL) return 1LL;\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_char_is_whitespace(long long c) {\n", out);
        fputs("    return (c == 32LL || c == 9LL || c == 10LL || c == 13LL) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_env_get(const char *name) {\n", out);
        fputs("    const char *k = name ? name : \"\";\n", out);
        fputs("    const char *v = getenv(k);\n", out);
        fputs("    return (char*)(v ? v : \"\");\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_env_has(const char *name) {\n", out);
        fputs("    const char *k = name ? name : \"\";\n", out);
        fputs("    return getenv(k) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_env_cwd(void) {\n", out);
        fputs("    char buf[4096];\n", out);
        fputs("    if (!cct_rt_getcwd(buf, sizeof(buf))) cct_rt_fail(\"env cwd failed\");\n", out);
        fputs("    size_t n = strlen(buf);\n", out);
        fputs("    char *out_s = (char*)malloc(n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"env cwd allocation failed\");\n", out);
        fputs("    memcpy(out_s, buf, n + 1);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    const char *src;\n", out);
        fputs("    long long len;\n", out);
        fputs("    long long pos;\n", out);
        fputs("} cct_rt_scan_t;\n\n", out);

        fputs("static void *cct_rt_scan_init(const char *s) {\n", out);
        fputs("    cct_rt_scan_t *c = (cct_rt_scan_t*)malloc(sizeof(cct_rt_scan_t));\n", out);
        fputs("    if (!c) cct_rt_fail(\"scan init allocation failed\");\n", out);
        fputs("    c->src = s ? s : \"\";\n", out);
        fputs("    c->len = (long long)strlen(c->src);\n", out);
        fputs("    c->pos = 0;\n", out);
        fputs("    return c;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_scan_pos(void *p) {\n", out);
        fputs("    cct_rt_scan_t *c = (cct_rt_scan_t*)p;\n", out);
        fputs("    if (!c) cct_rt_fail(\"scan pos null cursor\");\n", out);
        fputs("    return c->pos;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_scan_eof(void *p) {\n", out);
        fputs("    cct_rt_scan_t *c = (cct_rt_scan_t*)p;\n", out);
        fputs("    if (!c) cct_rt_fail(\"scan eof null cursor\");\n", out);
        fputs("    return (c->pos >= c->len) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_scan_peek(void *p) {\n", out);
        fputs("    cct_rt_scan_t *c = (cct_rt_scan_t*)p;\n", out);
        fputs("    if (!c) cct_rt_fail(\"scan peek null cursor\");\n", out);
        fputs("    if (c->pos >= c->len) cct_rt_fail(\"scan peek at eof\");\n", out);
        fputs("    return (long long)(unsigned char)c->src[c->pos];\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_scan_next(void *p) {\n", out);
        fputs("    cct_rt_scan_t *c = (cct_rt_scan_t*)p;\n", out);
        fputs("    if (!c) cct_rt_fail(\"scan next null cursor\");\n", out);
        fputs("    if (c->pos >= c->len) cct_rt_fail(\"scan next at eof\");\n", out);
        fputs("    long long out_ch = (long long)(unsigned char)c->src[c->pos];\n", out);
        fputs("    c->pos += 1;\n", out);
        fputs("    return out_ch;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_scan_free(void *p) {\n", out);
        fputs("    if (!p) return;\n", out);
        fputs("    free(p);\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    char *data;\n", out);
        fputs("    long long len;\n", out);
        fputs("    long long cap;\n", out);
        fputs("} cct_rt_builder_t;\n\n", out);

        fputs("static cct_rt_builder_t *cct_rt_builder_require_nonnull(void *p, const char *ctx) {\n", out);
        fputs("    cct_rt_builder_t *b = (cct_rt_builder_t*)p;\n", out);
        fputs("    if (!b) cct_rt_fail(ctx ? ctx : \"builder null pointer\");\n", out);
        fputs("    return b;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_builder_reserve(cct_rt_builder_t *b, long long extra) {\n", out);
        fputs("    if (!b) cct_rt_fail(\"builder reserve null pointer\");\n", out);
        fputs("    if (extra < 0) cct_rt_fail(\"builder reserve negative extra\");\n", out);
        fputs("    if (b->cap <= 0) cct_rt_fail(\"builder reserve invalid capacity\");\n", out);
        fputs("    long long need = b->len + extra + 1;\n", out);
        fputs("    if (need <= b->cap) return;\n", out);
        fputs("    long long new_cap = b->cap;\n", out);
        fputs("    while (new_cap < need) {\n", out);
        fputs("        if (new_cap > (9223372036854775807LL / 2)) {\n", out);
        fputs("            new_cap = need;\n", out);
        fputs("            break;\n", out);
        fputs("        }\n", out);
        fputs("        new_cap *= 2;\n", out);
        fputs("    }\n", out);
        fputs("    char *new_data = (char*)realloc(b->data, (size_t)new_cap);\n", out);
        fputs("    if (!new_data) cct_rt_fail(\"builder reserve allocation failed\");\n", out);
        fputs("    b->data = new_data;\n", out);
        fputs("    b->cap = new_cap;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_builder_init(void) {\n", out);
        fputs("    cct_rt_builder_t *b = (cct_rt_builder_t*)malloc(sizeof(cct_rt_builder_t));\n", out);
        fputs("    if (!b) cct_rt_fail(\"builder init allocation failed\");\n", out);
        fputs("    b->cap = 32;\n", out);
        fputs("    b->len = 0;\n", out);
        fputs("    b->data = (char*)malloc((size_t)b->cap);\n", out);
        fputs("    if (!b->data) cct_rt_fail(\"builder init buffer allocation failed\");\n", out);
        fputs("    b->data[0] = '\\0';\n", out);
        fputs("    return b;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_builder_append(void *p, const char *s) {\n", out);
        fputs("    cct_rt_builder_t *b = cct_rt_builder_require_nonnull(p, \"builder append null pointer\");\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    long long slen = (long long)strlen(src);\n", out);
        fputs("    cct_rt_builder_reserve(b, slen);\n", out);
        fputs("    memcpy(b->data + b->len, src, (size_t)slen + 1);\n", out);
        fputs("    b->len += slen;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_builder_append_char(void *p, long long c) {\n", out);
        fputs("    cct_rt_builder_t *b = cct_rt_builder_require_nonnull(p, \"builder append_char null pointer\");\n", out);
        fputs("    if (c < 0 || c > 255) cct_rt_fail(\"builder append_char expects byte range 0..255\");\n", out);
        fputs("    cct_rt_builder_reserve(b, 1);\n", out);
        fputs("    b->data[b->len] = (char)(unsigned char)c;\n", out);
        fputs("    b->len += 1;\n", out);
        fputs("    b->data[b->len] = '\\0';\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_builder_len(void *p) {\n", out);
        fputs("    cct_rt_builder_t *b = cct_rt_builder_require_nonnull(p, \"builder len null pointer\");\n", out);
        fputs("    return b->len;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_builder_to_verbum(void *p) {\n", out);
        fputs("    cct_rt_builder_t *b = cct_rt_builder_require_nonnull(p, \"builder to_verbum null pointer\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)b->len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"builder to_verbum allocation failed\");\n", out);
        fputs("    memcpy(out_s, b->data, (size_t)b->len + 1);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_builder_clear(void *p) {\n", out);
        fputs("    cct_rt_builder_t *b = cct_rt_builder_require_nonnull(p, \"builder clear null pointer\");\n", out);
        fputs("    b->len = 0;\n", out);
        fputs("    b->data[0] = '\\0';\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_builder_free(void *p) {\n", out);
        fputs("    cct_rt_builder_t *b = (cct_rt_builder_t*)p;\n", out);
        fputs("    if (!b) return;\n", out);
        fputs("    free(b->data);\n", out);
        fputs("    free(b);\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_builder_t *buf;\n", out);
        fputs("    long long indent;\n", out);
        fputs("    int at_line_start;\n", out);
        fputs("} cct_rt_writer_t;\n\n", out);

        fputs("static cct_rt_writer_t *cct_rt_writer_require_nonnull(void *p, const char *ctx) {\n", out);
        fputs("    cct_rt_writer_t *w = (cct_rt_writer_t*)p;\n", out);
        fputs("    if (!w) cct_rt_fail(ctx ? ctx : \"writer null pointer\");\n", out);
        fputs("    return w;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_apply_indent(cct_rt_writer_t *w) {\n", out);
        fputs("    if (!w || !w->at_line_start) return;\n", out);
        fputs("    long long spaces = w->indent * 2;\n", out);
        fputs("    for (long long i = 0; i < spaces; i++) {\n", out);
        fputs("        cct_rt_builder_append_char((void*)w->buf, 32LL);\n", out);
        fputs("    }\n", out);
        fputs("    w->at_line_start = 0;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_writer_init(void) {\n", out);
        fputs("    cct_rt_writer_t *w = (cct_rt_writer_t*)malloc(sizeof(cct_rt_writer_t));\n", out);
        fputs("    if (!w) cct_rt_fail(\"writer init allocation failed\");\n", out);
        fputs("    w->buf = (cct_rt_builder_t*)cct_rt_builder_init();\n", out);
        fputs("    w->indent = 0;\n", out);
        fputs("    w->at_line_start = 1;\n", out);
        fputs("    return w;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_indent(void *p) {\n", out);
        fputs("    cct_rt_writer_t *w = cct_rt_writer_require_nonnull(p, \"writer indent null pointer\");\n", out);
        fputs("    w->indent += 1;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_dedent(void *p) {\n", out);
        fputs("    cct_rt_writer_t *w = cct_rt_writer_require_nonnull(p, \"writer dedent null pointer\");\n", out);
        fputs("    if (w->indent <= 0) cct_rt_fail(\"writer dedent underflow\");\n", out);
        fputs("    w->indent -= 1;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_write(void *p, const char *s) {\n", out);
        fputs("    cct_rt_writer_t *w = cct_rt_writer_require_nonnull(p, \"writer write null pointer\");\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    if (n == 0) return;\n", out);
        fputs("    cct_rt_writer_apply_indent(w);\n", out);
        fputs("    cct_rt_builder_append((void*)w->buf, src);\n", out);
        fputs("    w->at_line_start = (src[n - 1] == '\\n') ? 1 : 0;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_writeln(void *p, const char *s) {\n", out);
        fputs("    cct_rt_writer_t *w = cct_rt_writer_require_nonnull(p, \"writer writeln null pointer\");\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    cct_rt_writer_apply_indent(w);\n", out);
        fputs("    cct_rt_builder_append((void*)w->buf, src);\n", out);
        fputs("    cct_rt_builder_append_char((void*)w->buf, 10LL);\n", out);
        fputs("    w->at_line_start = 1;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_writer_to_verbum(void *p) {\n", out);
        fputs("    cct_rt_writer_t *w = cct_rt_writer_require_nonnull(p, \"writer to_verbum null pointer\");\n", out);
        fputs("    return cct_rt_builder_to_verbum((void*)w->buf);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_writer_free(void *p) {\n", out);
        fputs("    cct_rt_writer_t *w = (cct_rt_writer_t*)p;\n", out);
        fputs("    if (!w) return;\n", out);
        fputs("    cct_rt_builder_free((void*)w->buf);\n", out);
        fputs("    free(w);\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_fmt_helpers) {
        fputs("static char *cct_rt_fmt_stringify_int(long long x) {\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%lld\", x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_int formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_int allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%lld\", x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_real(double x) {\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%.17g\", x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_real formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_real allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%.17g\", x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_float(float x) {\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%.9g\", (double)x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_float formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_float allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%.9g\", (double)x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fmt_parse_int_or_fail(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    while (*src && isspace((unsigned char)*src)) src++;\n", out);
        fputs("    if (!*src) cct_rt_fail(\"fmt parse_int invalid input\");\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    long long v = strtoll(src, &end, 10);\n", out);
        fputs("    if (errno != 0 || end == src) cct_rt_fail(\"fmt parse_int invalid input\");\n", out);
        fputs("    while (*end && isspace((unsigned char)*end)) end++;\n", out);
        fputs("    if (*end) cct_rt_fail(\"fmt parse_int invalid input\");\n", out);
        fputs("    return v;\n", out);
        fputs("}\n\n", out);

        fputs("static double cct_rt_fmt_parse_real_or_fail(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    while (*src && isspace((unsigned char)*src)) src++;\n", out);
        fputs("    if (!*src) cct_rt_fail(\"fmt parse_real invalid input\");\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    double v = strtod(src, &end);\n", out);
        fputs("    if (errno != 0 || end == src) cct_rt_fail(\"fmt parse_real invalid input\");\n", out);
        fputs("    while (*end && isspace((unsigned char)*end)) end++;\n", out);
        fputs("    if (*end) cct_rt_fail(\"fmt parse_real invalid input\");\n", out);
        fputs("    return v;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_io_helpers) {
        fputs("static void cct_rt_io_print(const char *s) {\n", out);
        fputs("    fputs(s ? s : \"\", stdout);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_println(const char *s) {\n", out);
        fputs("    fputs(s ? s : \"\", stdout);\n", out);
        fputs("    fputc('\\n', stdout);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_print_int(long long n) {\n", out);
        fputs("    printf(\"%lld\", n);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_io_read_line(void) {\n", out);
        fputs("    size_t cap = 128;\n", out);
        fputs("    size_t len = 0;\n", out);
        fputs("    char *buf = (char*)malloc(cap);\n", out);
        fputs("    if (!buf) cct_rt_fail(\"io read_line allocation failed\");\n", out);
        fputs("    int ch = 0;\n", out);
        fputs("    while ((ch = fgetc(stdin)) != EOF) {\n", out);
        fputs("        if (ch == '\\r') continue;\n", out);
        fputs("        if (ch == '\\n') break;\n", out);
        fputs("        if (len + 1 >= cap) {\n", out);
        fputs("            size_t next_cap = cap * 2;\n", out);
        fputs("            char *next = (char*)realloc(buf, next_cap);\n", out);
        fputs("            if (!next) {\n", out);
        fputs("                free(buf);\n", out);
        fputs("                cct_rt_fail(\"io read_line reallocation failed\");\n", out);
        fputs("            }\n", out);
        fputs("            buf = next;\n", out);
        fputs("            cap = next_cap;\n", out);
        fputs("        }\n", out);
        fputs("        buf[len++] = (char)ch;\n", out);
        fputs("    }\n", out);
        fputs("    buf[len] = '\\0';\n", out);
        fputs("    return buf;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_fs_helpers) {
        fputs("static FILE *cct_rt_fs_open_or_fail(const char *path, const char *mode, const char *ctx) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    FILE *f = fopen(p, mode);\n", out);
        fputs("    if (!f) cct_rt_fail((ctx && *ctx) ? ctx : \"fs open failed\");\n", out);
        fputs("    return f;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fs_read_all(const char *path) {\n", out);
        fputs("    FILE *f = cct_rt_fs_open_or_fail(path, \"rb\", \"fs read_all open failed\");\n", out);
        fputs("    if (fseek(f, 0, SEEK_END) != 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs read_all seek-end failed\");\n", out);
        fputs("    }\n", out);
        fputs("    long sz = ftell(f);\n", out);
        fputs("    if (sz < 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs read_all tell failed\");\n", out);
        fputs("    }\n", out);
        fputs("    if (fseek(f, 0, SEEK_SET) != 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs read_all seek-set failed\");\n", out);
        fputs("    }\n", out);
        fputs("    char *buf = (char*)malloc((size_t)sz + 1);\n", out);
        fputs("    if (!buf) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs read_all allocation failed\");\n", out);
        fputs("    }\n", out);
        fputs("    size_t read_n = fread(buf, 1, (size_t)sz, f);\n", out);
        fputs("    if (read_n != (size_t)sz && ferror(f)) {\n", out);
        fputs("        free(buf);\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs read_all read failed\");\n", out);
        fputs("    }\n", out);
        fputs("    buf[read_n] = '\\0';\n", out);
        fputs("    fclose(f);\n", out);
        fputs("    return buf;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_write_all(const char *path, const char *content) {\n", out);
        fputs("    FILE *f = cct_rt_fs_open_or_fail(path, \"wb\", \"fs write_all open failed\");\n", out);
        fputs("    const char *src = content ? content : \"\";\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    if (len > 0 && fwrite(src, 1, len, f) != len) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs write_all write failed\");\n", out);
        fputs("    }\n", out);
        fputs("    fclose(f);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_append_all(const char *path, const char *content) {\n", out);
        fputs("    FILE *f = cct_rt_fs_open_or_fail(path, \"ab\", \"fs append_all open failed\");\n", out);
        fputs("    const char *src = content ? content : \"\";\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    if (len > 0 && fwrite(src, 1, len, f) != len) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs append_all write failed\");\n", out);
        fputs("    }\n", out);
        fputs("    fclose(f);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_exists(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    FILE *f = fopen(p, \"rb\");\n", out);
        fputs("    if (!f) return 0LL;\n", out);
        fputs("    fclose(f);\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_size(const char *path) {\n", out);
        fputs("    FILE *f = cct_rt_fs_open_or_fail(path, \"rb\", \"fs size open failed\");\n", out);
        fputs("    if (fseek(f, 0, SEEK_END) != 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs size seek-end failed\");\n", out);
        fputs("    }\n", out);
        fputs("    long sz = ftell(f);\n", out);
        fputs("    if (sz < 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        cct_rt_fail(\"fs size tell failed\");\n", out);
        fputs("    }\n", out);
        fputs("    fclose(f);\n", out);
        fputs("    return (long long)sz;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_path_helpers) {
        fputs("static int cct_rt_path_is_sep(char ch) {\n", out);
        fputs("    return ch == '/' || ch == '\\\\';\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_dup_normalized(const char *src, size_t start, size_t end) {\n", out);
        fputs("    if (!src || end <= start) {\n", out);
        fputs("        char *empty = (char*)malloc(1);\n", out);
        fputs("        if (!empty) cct_rt_fail(\"path allocation failed\");\n", out);
        fputs("        empty[0] = '\\0';\n", out);
        fputs("        return empty;\n", out);
        fputs("    }\n", out);
        fputs("    size_t n = end - start;\n", out);
        fputs("    char *out_s = (char*)malloc(n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"path allocation failed\");\n", out);
        fputs("    for (size_t i = 0; i < n; i++) {\n", out);
        fputs("        char ch = src[start + i];\n", out);
        fputs("        out_s[i] = (ch == '\\\\') ? '/' : ch;\n", out);
        fputs("    }\n", out);
        fputs("    out_s[n] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_join(const char *a, const char *b) {\n", out);
        fputs("    const char *sa = a ? a : \"\";\n", out);
        fputs("    const char *sb = b ? b : \"\";\n", out);
        fputs("    size_t la = strlen(sa);\n", out);
        fputs("    size_t lb = strlen(sb);\n", out);
        fputs("    if (la == 0) return cct_rt_path_dup_normalized(sb, 0, lb);\n", out);
        fputs("    if (lb == 0) return cct_rt_path_dup_normalized(sa, 0, la);\n", out);
        fputs("\n", out);
        fputs("    size_t a_end = la;\n", out);
        fputs("    while (a_end > 0 && cct_rt_path_is_sep(sa[a_end - 1])) a_end--;\n", out);
        fputs("    int a_root_only = 0;\n", out);
        fputs("    if (a_end == 0) {\n", out);
        fputs("        for (size_t i = 0; i < la; i++) {\n", out);
        fputs("            if (!cct_rt_path_is_sep(sa[i])) { a_root_only = 0; break; }\n", out);
        fputs("            a_root_only = 1;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("\n", out);
        fputs("    size_t b_start = 0;\n", out);
        fputs("    while (b_start < lb && cct_rt_path_is_sep(sb[b_start])) b_start++;\n", out);
        fputs("    int b_all_sep = (b_start == lb);\n", out);
        fputs("\n", out);
        fputs("    char *left = NULL;\n", out);
        fputs("    if (a_root_only) left = cct_rt_path_dup_normalized(\"/\", 0, 1);\n", out);
        fputs("    else left = cct_rt_path_dup_normalized(sa, 0, a_end);\n", out);
        fputs("\n", out);
        fputs("    if (b_all_sep) {\n", out);
        fputs("        size_t left_len = strlen(left);\n", out);
        fputs("        if (left_len == 0) {\n", out);
        fputs("            free(left);\n", out);
        fputs("            return cct_rt_path_dup_normalized(\"/\", 0, 1);\n", out);
        fputs("        }\n", out);
        fputs("        if (left[left_len - 1] == '/') return left;\n", out);
        fputs("        char *out_s = (char*)malloc(left_len + 2);\n", out);
        fputs("        if (!out_s) {\n", out);
        fputs("            free(left);\n", out);
        fputs("            cct_rt_fail(\"path join allocation failed\");\n", out);
        fputs("        }\n", out);
        fputs("        memcpy(out_s, left, left_len);\n", out);
        fputs("        out_s[left_len] = '/';\n", out);
        fputs("        out_s[left_len + 1] = '\\0';\n", out);
        fputs("        free(left);\n", out);
        fputs("        return out_s;\n", out);
        fputs("    }\n", out);
        fputs("\n", out);
        fputs("    char *right = cct_rt_path_dup_normalized(sb, b_start, lb);\n", out);
        fputs("    size_t left_len = strlen(left);\n", out);
        fputs("    size_t right_len = strlen(right);\n", out);
        fputs("    int left_is_root = (left_len == 1 && left[0] == '/');\n", out);
        fputs("    size_t out_len = left_len + right_len + (left_is_root ? 0 : 1);\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) {\n", out);
        fputs("        free(left);\n", out);
        fputs("        free(right);\n", out);
        fputs("        cct_rt_fail(\"path join allocation failed\");\n", out);
        fputs("    }\n", out);
        fputs("    size_t pos = 0;\n", out);
        fputs("    memcpy(out_s + pos, left, left_len);\n", out);
        fputs("    pos += left_len;\n", out);
        fputs("    if (!left_is_root) out_s[pos++] = '/';\n", out);
        fputs("    memcpy(out_s + pos, right, right_len);\n", out);
        fputs("    pos += right_len;\n", out);
        fputs("    out_s[pos] = '\\0';\n", out);
        fputs("    free(left);\n", out);
        fputs("    free(right);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_basename(const char *p) {\n", out);
        fputs("    const char *src = p ? p : \"\";\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    while (len > 0 && cct_rt_path_is_sep(src[len - 1])) len--;\n", out);
        fputs("    if (len == 0) return cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("    size_t start = len;\n", out);
        fputs("    while (start > 0 && !cct_rt_path_is_sep(src[start - 1])) start--;\n", out);
        fputs("    return cct_rt_path_dup_normalized(src, start, len);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_dirname(const char *p) {\n", out);
        fputs("    const char *src = p ? p : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t len = slen;\n", out);
        fputs("    while (len > 0 && cct_rt_path_is_sep(src[len - 1])) len--;\n", out);
        fputs("    if (len == 0) {\n", out);
        fputs("        for (size_t i = 0; i < slen; i++) {\n", out);
        fputs("            if (!cct_rt_path_is_sep(src[i])) return cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("        }\n", out);
        fputs("        return (slen > 0) ? cct_rt_path_dup_normalized(\"/\", 0, 1) : cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("    }\n", out);
        fputs("    size_t start = len;\n", out);
        fputs("    while (start > 0 && !cct_rt_path_is_sep(src[start - 1])) start--;\n", out);
        fputs("    if (start == 0) return cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("    size_t end = start;\n", out);
        fputs("    while (end > 0 && cct_rt_path_is_sep(src[end - 1])) end--;\n", out);
        fputs("    if (end == 0) return cct_rt_path_dup_normalized(\"/\", 0, 1);\n", out);
        fputs("    return cct_rt_path_dup_normalized(src, 0, end);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_ext(const char *p) {\n", out);
        fputs("    char *base = cct_rt_path_basename(p);\n", out);
        fputs("    size_t len = strlen(base);\n", out);
        fputs("    size_t last_dot = (size_t)-1;\n", out);
        fputs("    for (size_t i = 0; i < len; i++) {\n", out);
        fputs("        if (base[i] == '.') last_dot = i;\n", out);
        fputs("    }\n", out);
        fputs("    if (last_dot == (size_t)-1 || last_dot == 0) {\n", out);
        fputs("        free(base);\n", out);
        fputs("        return cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("    }\n", out);
        fputs("    char *ext = cct_rt_path_dup_normalized(base, last_dot, len);\n", out);
        fputs("    free(base);\n", out);
        fputs("    return ext;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_random_helpers) {
        fputs("static void cct_rt_random_seed(long long s) {\n", out);
        fputs("    srand((unsigned int)(s & 0xFFFFFFFFu));\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_random_int(long long lo, long long hi) {\n", out);
        fputs("    if (lo > hi) cct_rt_fail(\"random_int invalid range (lo > hi)\");\n", out);
        fputs("    unsigned long long span = (unsigned long long)(hi - lo) + 1ULL;\n", out);
        fputs("    if (span == 0ULL) cct_rt_fail(\"random_int invalid range span overflow\");\n", out);
        fputs("    unsigned long long r = (unsigned long long)rand();\n", out);
        fputs("    return lo + (long long)(r % span);\n", out);
        fputs("}\n\n", out);

        fputs("static double cct_rt_random_real(void) {\n", out);
        fputs("    return (double)rand() / ((double)RAND_MAX + 1.0);\n", out);
        fputs("}\n\n", out);
    }

    fputs("/* ===== End CCT Runtime Helpers ===== */\n\n", out);
    return true;
}
