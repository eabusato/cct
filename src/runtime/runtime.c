/*
 * CCT — Clavicula Turing
 * Runtime Helper Emission Implementation
 *
 * FASE 12H: Structural maturity milestone
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
    cfg->emit_process_helpers = true;
    cfg->emit_hash_helpers = true;
    cfg->emit_crypto_helpers = false;
    cfg->emit_regex_helpers = false;
    cfg->emit_toml_helpers = false;
    cfg->emit_compress_helpers = false;
    cfg->emit_filetype_helpers = false;
    cfg->emit_image_ops_helpers = false;
    cfg->emit_signal_helpers = false;
    cfg->emit_postgres_helpers = false;
    cfg->emit_mail_helpers = false;
    cfg->emit_instrument_helpers = false;
    cfg->emit_verbum_helpers = true;
    cfg->emit_fmt_helpers = true;
    cfg->emit_db_helpers = false;
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

        fputs("static long long cct_rt_date_now_unix(void) {\n", out);
        fputs("    time_t now = time(NULL);\n", out);
        fputs("    if (now == (time_t)-1) cct_rt_fail(\"date now_unix failed\");\n", out);
        fputs("    return (long long)now;\n", out);
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

            fputs("static void *cct_rt_fluxus_peek(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus peek received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->len <= 0) cct_rt_fail(\"fluxus peek em fluxus vazio\");\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    return (void*)(base + ((size_t)(flux->len - 1) * (size_t)flux->elem_size));\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_set(void *flux_ptr, long long idx, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus set received null instance\");\n", out);
            fputs("    if (!elem_ptr) cct_rt_fail(\"fluxus set requires non-null element pointer\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (idx < 0 || idx >= flux->len) cct_rt_fail(\"fluxus set indice fora dos limites\");\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    cct_rt_mem_copy(base + ((size_t)idx * (size_t)flux->elem_size), elem_ptr, flux->elem_size);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_remove(void *flux_ptr, long long idx) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus remove received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (idx < 0 || idx >= flux->len) cct_rt_fail(\"fluxus remove indice fora dos limites\");\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    long long tail = flux->len - idx - 1;\n", out);
            fputs("    if (tail > 0) {\n", out);
            fputs("        size_t bytes = (size_t)tail * (size_t)flux->elem_size;\n", out);
            fputs("        memmove(base + ((size_t)idx * (size_t)flux->elem_size),\n", out);
            fputs("                base + ((size_t)(idx + 1) * (size_t)flux->elem_size), bytes);\n", out);
            fputs("    }\n", out);
            fputs("    flux->len -= 1;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_insert(void *flux_ptr, long long idx, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus insert received null instance\");\n", out);
            fputs("    if (!elem_ptr) cct_rt_fail(\"fluxus insert requires non-null element pointer\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (idx < 0 || idx > flux->len) cct_rt_fail(\"fluxus insert indice fora dos limites\");\n", out);
            fputs("    if (flux->len == flux->capacity) cct_rt_fluxus_grow(flux);\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    if (idx < flux->len) {\n", out);
            fputs("        size_t bytes = (size_t)(flux->len - idx) * (size_t)flux->elem_size;\n", out);
            fputs("        memmove(base + ((size_t)(idx + 1) * (size_t)flux->elem_size),\n", out);
            fputs("                base + ((size_t)idx * (size_t)flux->elem_size), bytes);\n", out);
            fputs("    }\n", out);
            fputs("    cct_rt_mem_copy(base + ((size_t)idx * (size_t)flux->elem_size), elem_ptr, flux->elem_size);\n", out);
            fputs("    flux->len += 1;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_fluxus_contains(void *flux_ptr, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus contains received null instance\");\n", out);
            fputs("    if (!elem_ptr) cct_rt_fail(\"fluxus contains requires non-null element pointer\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    size_t elem_size = (size_t)flux->elem_size;\n", out);
            fputs("    for (long long i = 0; i < flux->len; i++) {\n", out);
            fputs("        if (memcmp(base + ((size_t)i * elem_size), elem_ptr, elem_size) == 0) return 1LL;\n", out);
            fputs("    }\n", out);
            fputs("    return 0LL;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_fluxus_contains_verbum(void *flux_ptr, const char *value) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus contains_verbum received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->elem_size != (long long)sizeof(char*)) cct_rt_fail(\"fluxus contains_verbum requer elem_size == sizeof(char*)\");\n", out);
            fputs("    const char *needle = value ? value : \"\";\n", out);
            fputs("    for (long long i = 0; i < flux->len; i++) {\n", out);
            fputs("        char **slot = (char**)(((unsigned char*)flux->data) + ((size_t)i * sizeof(char*)));\n", out);
            fputs("        const char *item = (slot && *slot) ? *slot : \"\";\n", out);
            fputs("        if (strcmp(item, needle) == 0) return 1LL;\n", out);
            fputs("    }\n", out);
            fputs("    return 0LL;\n", out);
            fputs("}\n\n", out);

            fputs("typedef struct cct_rt_gettext_message_entry {\n", out);
            fputs("    char *key;\n", out);
            fputs("    char *value;\n", out);
            fputs("    struct cct_rt_gettext_message_entry *next;\n", out);
            fputs("} cct_rt_gettext_message_entry_t;\n\n", out);

            fputs("typedef struct cct_rt_gettext_plural_entry {\n", out);
            fputs("    char *singular_key;\n", out);
            fputs("    char *plural_key;\n", out);
            fputs("    char *singular_value;\n", out);
            fputs("    char *plural_value;\n", out);
            fputs("    struct cct_rt_gettext_plural_entry *next;\n", out);
            fputs("} cct_rt_gettext_plural_entry_t;\n\n", out);

            fputs("typedef struct cct_rt_gettext_catalog {\n", out);
            fputs("    char *locale;\n", out);
            fputs("    cct_rt_gettext_message_entry_t *messages;\n", out);
            fputs("    cct_rt_gettext_plural_entry_t *plurals;\n", out);
            fputs("} cct_rt_gettext_catalog_t;\n\n", out);

            fputs("static cct_rt_gettext_catalog_t *cct_rt_gettext_default_catalog = NULL;\n", out);
            fputs("static char cct_rt_gettext_last_error[256] = \"\";\n\n", out);

            fputs("static char *cct_rt_gettext_dup(const char *src) {\n", out);
            fputs("    const char *text = src ? src : \"\";\n", out);
            fputs("    size_t n = strlen(text);\n", out);
            fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1);\n", out);
            fputs("    memcpy(out_s, text, n + 1);\n", out);
            fputs("    return out_s;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_gettext_set_error(const char *msg) {\n", out);
            fputs("    const char *text = (msg && *msg) ? msg : \"gettext error\";\n", out);
            fputs("    snprintf(cct_rt_gettext_last_error, sizeof(cct_rt_gettext_last_error), \"%s\", text);\n", out);
            fputs("    cct_rt_gettext_last_error[sizeof(cct_rt_gettext_last_error) - 1] = '\\0';\n", out);
            fputs("}\n\n", out);

            fputs("static const char *cct_rt_gettext_catalog_last_error(void) {\n", out);
            fputs("    return cct_rt_gettext_last_error[0] ? cct_rt_gettext_last_error : \"gettext error\";\n", out);
            fputs("}\n\n", out);

            fputs("static cct_rt_gettext_message_entry_t *cct_rt_gettext_find_message(cct_rt_gettext_catalog_t *cat, const char *key) {\n", out);
            fputs("    const char *needle = key ? key : \"\";\n", out);
            fputs("    cct_rt_gettext_message_entry_t *entry = cat ? cat->messages : NULL;\n", out);
            fputs("    while (entry) {\n", out);
            fputs("        if (strcmp(entry->key, needle) == 0) return entry;\n", out);
            fputs("        entry = entry->next;\n", out);
            fputs("    }\n", out);
            fputs("    return NULL;\n", out);
            fputs("}\n\n", out);

            fputs("static cct_rt_gettext_plural_entry_t *cct_rt_gettext_find_plural(cct_rt_gettext_catalog_t *cat, const char *singular) {\n", out);
            fputs("    const char *needle = singular ? singular : \"\";\n", out);
            fputs("    cct_rt_gettext_plural_entry_t *entry = cat ? cat->plurals : NULL;\n", out);
            fputs("    while (entry) {\n", out);
            fputs("        if (strcmp(entry->singular_key, needle) == 0) return entry;\n", out);
            fputs("        entry = entry->next;\n", out);
            fputs("    }\n", out);
            fputs("    return NULL;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_catalog_new(const char *locale) {\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_gettext_catalog_t));\n", out);
            fputs("    cat->locale = cct_rt_gettext_dup(locale);\n", out);
            fputs("    cat->messages = NULL;\n", out);
            fputs("    cat->plurals = NULL;\n", out);
            fputs("    cct_rt_gettext_last_error[0] = '\\0';\n", out);
            fputs("    return (long long)(size_t)cat;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_catalog_add(long long catalog_handle, const char *key, const char *value) {\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)(size_t)catalog_handle;\n", out);
            fputs("    if (!cat) { cct_rt_gettext_set_error(\"catalogo gettext invalido\"); return 0LL; }\n", out);
            fputs("    cct_rt_gettext_message_entry_t *entry = cct_rt_gettext_find_message(cat, key);\n", out);
            fputs("    if (entry) {\n", out);
            fputs("        entry->value = cct_rt_gettext_dup(value);\n", out);
            fputs("        return 1LL;\n", out);
            fputs("    }\n", out);
            fputs("    entry = (cct_rt_gettext_message_entry_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_gettext_message_entry_t));\n", out);
            fputs("    entry->key = cct_rt_gettext_dup(key);\n", out);
            fputs("    entry->value = cct_rt_gettext_dup(value);\n", out);
            fputs("    entry->next = cat->messages;\n", out);
            fputs("    cat->messages = entry;\n", out);
            fputs("    return 1LL;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_catalog_add_plural(long long catalog_handle, const char *singular, const char *plural, const char *translated_singular, const char *translated_plural) {\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)(size_t)catalog_handle;\n", out);
            fputs("    if (!cat) { cct_rt_gettext_set_error(\"catalogo gettext invalido\"); return 0LL; }\n", out);
            fputs("    cct_rt_gettext_plural_entry_t *entry = cct_rt_gettext_find_plural(cat, singular);\n", out);
            fputs("    if (!entry) {\n", out);
            fputs("        entry = (cct_rt_gettext_plural_entry_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_gettext_plural_entry_t));\n", out);
            fputs("        entry->next = cat->plurals;\n", out);
            fputs("        cat->plurals = entry;\n", out);
            fputs("        entry->singular_key = NULL;\n", out);
            fputs("        entry->plural_key = NULL;\n", out);
            fputs("        entry->singular_value = NULL;\n", out);
            fputs("        entry->plural_value = NULL;\n", out);
            fputs("    }\n", out);
            fputs("    entry->singular_key = cct_rt_gettext_dup(singular);\n", out);
            fputs("    entry->plural_key = cct_rt_gettext_dup(plural);\n", out);
            fputs("    entry->singular_value = cct_rt_gettext_dup(translated_singular);\n", out);
            fputs("    entry->plural_value = cct_rt_gettext_dup(translated_plural);\n", out);
            fputs("    cct_rt_gettext_catalog_add(catalog_handle, singular, translated_singular);\n", out);
            fputs("    cct_rt_gettext_catalog_add(catalog_handle, plural, translated_plural);\n", out);
            fputs("    return 1LL;\n", out);
            fputs("}\n\n", out);

            fputs("static const char *cct_rt_gettext_translate(long long catalog_handle, const char *key) {\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)(size_t)catalog_handle;\n", out);
            fputs("    const char *needle = key ? key : \"\";\n", out);
            fputs("    if (!cat) return needle;\n", out);
            fputs("    cct_rt_gettext_message_entry_t *entry = cct_rt_gettext_find_message(cat, needle);\n", out);
            fputs("    return (entry && entry->value) ? entry->value : needle;\n", out);
            fputs("}\n\n", out);

            fputs("static const char *cct_rt_gettext_translate_plural(long long catalog_handle, const char *singular, const char *plural, long long n) {\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)(size_t)catalog_handle;\n", out);
            fputs("    if (cat) {\n", out);
            fputs("        cct_rt_gettext_plural_entry_t *entry = cct_rt_gettext_find_plural(cat, singular);\n", out);
            fputs("        if (entry) return (n == 1LL) ? entry->singular_value : entry->plural_value;\n", out);
            fputs("    }\n", out);
            fputs("    return (n == 1LL) ? cct_rt_gettext_translate(catalog_handle, singular) : cct_rt_gettext_translate(catalog_handle, plural);\n", out);
            fputs("}\n\n", out);

            fputs("static const char *cct_rt_gettext_skip_ws(const char *s) {\n", out);
            fputs("    while (*s == ' ' || *s == '\\t') s++;\n", out);
            fputs("    return s;\n", out);
            fputs("}\n\n", out);

            fputs("static int cct_rt_gettext_starts_with(const char *s, const char *prefix) {\n", out);
            fputs("    size_t n = strlen(prefix);\n", out);
            fputs("    return strncmp(s, prefix, n) == 0;\n", out);
            fputs("}\n\n", out);

            fputs("static char *cct_rt_gettext_append(char *base, const char *suffix) {\n", out);
            fputs("    const char *left = base ? base : \"\";\n", out);
            fputs("    const char *right = suffix ? suffix : \"\";\n", out);
            fputs("    size_t a = strlen(left);\n", out);
            fputs("    size_t b = strlen(right);\n", out);
            fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(a + b + 1);\n", out);
            fputs("    memcpy(out_s, left, a);\n", out);
            fputs("    memcpy(out_s + a, right, b + 1);\n", out);
            fputs("    return out_s;\n", out);
            fputs("}\n\n", out);

            fputs("static char *cct_rt_gettext_extract_quoted(const char *line) {\n", out);
            fputs("    const char *start = strchr(line, '\"');\n", out);
            fputs("    if (!start) return cct_rt_gettext_dup(\"\");\n", out);
            fputs("    start += 1;\n", out);
            fputs("    size_t cap = strlen(start) + 1;\n", out);
            fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(cap + 1);\n", out);
            fputs("    size_t out_len = 0;\n", out);
            fputs("    while (*start && *start != '\"') {\n", out);
            fputs("        if (*start == '\\\\' && start[1]) {\n", out);
            fputs("            char esc = start[1];\n", out);
            fputs("            if (esc == 'n') out_s[out_len++] = '\\n';\n", out);
            fputs("            else if (esc == 't') out_s[out_len++] = '\\t';\n", out);
            fputs("            else out_s[out_len++] = esc;\n", out);
            fputs("            start += 2;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        out_s[out_len++] = *start;\n", out);
            fputs("        start += 1;\n", out);
            fputs("    }\n", out);
            fputs("    out_s[out_len] = '\\0';\n", out);
            fputs("    return out_s;\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_gettext_commit_entry(cct_rt_gettext_catalog_t *cat, const char *msgid, const char *msgid_plural, const char *msgstr0, const char *msgstr1) {\n", out);
            fputs("    if (!cat || !msgid || !*msgid) return;\n", out);
            fputs("    if (msgid_plural && *msgid_plural) {\n", out);
            fputs("        cct_rt_gettext_catalog_add_plural((long long)(size_t)cat, msgid, msgid_plural, (msgstr0 && *msgstr0) ? msgstr0 : msgid, (msgstr1 && *msgstr1) ? msgstr1 : msgid_plural);\n", out);
            fputs("        return;\n", out);
            fputs("    }\n", out);
            fputs("    if (msgstr0 && *msgstr0) cct_rt_gettext_catalog_add((long long)(size_t)cat, msgid, msgstr0);\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_catalog_load(const char *path, const char *locale) {\n", out);
            fputs("    FILE *fp = fopen(path ? path : \"\", \"rb\");\n", out);
            fputs("    if (!fp) { cct_rt_gettext_set_error(\"catalogo inexistente\"); return 0LL; }\n", out);
            fputs("    cct_rt_gettext_catalog_t *cat = (cct_rt_gettext_catalog_t*)(size_t)cct_rt_gettext_catalog_new(locale);\n", out);
            fputs("    char line[4096];\n", out);
            fputs("    char *msgid = cct_rt_gettext_dup(\"\");\n", out);
            fputs("    char *msgid_plural = cct_rt_gettext_dup(\"\");\n", out);
            fputs("    char *msgstr0 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("    char *msgstr1 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("    int mode = 0;\n", out);
            fputs("    while (fgets(line, sizeof(line), fp)) {\n", out);
            fputs("        size_t len = strlen(line);\n", out);
            fputs("        while (len > 0 && (line[len - 1] == '\\n' || line[len - 1] == '\\r')) line[--len] = '\\0';\n", out);
            fputs("        const char *trimmed = cct_rt_gettext_skip_ws(line);\n", out);
            fputs("        if (*trimmed == '\\0') {\n", out);
            fputs("            cct_rt_gettext_commit_entry(cat, msgid, msgid_plural, msgstr0, msgstr1);\n", out);
            fputs("            msgid = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            msgid_plural = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            msgstr0 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            msgstr1 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            mode = 0;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (*trimmed == '#') continue;\n", out);
            fputs("        if (cct_rt_gettext_starts_with(trimmed, \"msgid_plural \")) {\n", out);
            fputs("            msgid_plural = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            mode = 2;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (cct_rt_gettext_starts_with(trimmed, \"msgid \")) {\n", out);
            fputs("            cct_rt_gettext_commit_entry(cat, msgid, msgid_plural, msgstr0, msgstr1);\n", out);
            fputs("            msgid = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            msgid_plural = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            msgstr0 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            msgstr1 = cct_rt_gettext_dup(\"\");\n", out);
            fputs("            mode = 1;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (cct_rt_gettext_starts_with(trimmed, \"msgstr[0] \")) {\n", out);
            fputs("            msgstr0 = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            mode = 3;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (cct_rt_gettext_starts_with(trimmed, \"msgstr[1] \")) {\n", out);
            fputs("            msgstr1 = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            mode = 4;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (cct_rt_gettext_starts_with(trimmed, \"msgstr \")) {\n", out);
            fputs("            msgstr0 = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            mode = 3;\n", out);
            fputs("            continue;\n", out);
            fputs("        }\n", out);
            fputs("        if (*trimmed == '\"') {\n", out);
            fputs("            char *cont = cct_rt_gettext_extract_quoted(trimmed);\n", out);
            fputs("            if (mode == 1) msgid = cct_rt_gettext_append(msgid, cont);\n", out);
            fputs("            else if (mode == 2) msgid_plural = cct_rt_gettext_append(msgid_plural, cont);\n", out);
            fputs("            else if (mode == 3) msgstr0 = cct_rt_gettext_append(msgstr0, cont);\n", out);
            fputs("            else if (mode == 4) msgstr1 = cct_rt_gettext_append(msgstr1, cont);\n", out);
            fputs("        }\n", out);
            fputs("    }\n", out);
            fputs("    fclose(fp);\n", out);
            fputs("    cct_rt_gettext_commit_entry(cat, msgid, msgid_plural, msgstr0, msgstr1);\n", out);
            fputs("    cct_rt_gettext_last_error[0] = '\\0';\n", out);
            fputs("    return (long long)(size_t)cat;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_default_set(long long catalog_handle) {\n", out);
            fputs("    cct_rt_gettext_default_catalog = (cct_rt_gettext_catalog_t*)(size_t)catalog_handle;\n", out);
            fputs("    return 1LL;\n", out);
            fputs("}\n\n", out);

            fputs("static const char *cct_rt_gettext_default_translate(const char *key) {\n", out);
            fputs("    return cct_rt_gettext_translate((long long)(size_t)cct_rt_gettext_default_catalog, key);\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_gettext_builtin_catalog_new(const char *locale) { return cct_rt_gettext_catalog_new(locale); }\n", out);
            fputs("static long long cct_rt_gettext_builtin_catalog_add(long long handle, const char *key, const char *value) { return cct_rt_gettext_catalog_add(handle, key, value); }\n", out);
            fputs("static long long cct_rt_gettext_builtin_catalog_add_plural(long long handle, const char *singular, const char *plural, const char *translated_singular, const char *translated_plural) { return cct_rt_gettext_catalog_add_plural(handle, singular, plural, translated_singular, translated_plural); }\n", out);
            fputs("static long long cct_rt_gettext_builtin_catalog_load(const char *path, const char *locale) { return cct_rt_gettext_catalog_load(path, locale); }\n", out);
            fputs("static const char *cct_rt_gettext_builtin_catalog_last_error(void) { return cct_rt_gettext_catalog_last_error(); }\n", out);
            fputs("static const char *cct_rt_gettext_builtin_translate(long long handle, const char *key) { return cct_rt_gettext_translate(handle, key); }\n", out);
            fputs("static const char *cct_rt_gettext_builtin_translate_plural(long long handle, const char *singular, const char *plural, long long n) { return cct_rt_gettext_translate_plural(handle, singular, plural, n); }\n", out);
            fputs("static long long cct_rt_gettext_builtin_default_set(long long handle) { return cct_rt_gettext_default_set(handle); }\n", out);
            fputs("static const char *cct_rt_gettext_builtin_default_translate(const char *key) { return cct_rt_gettext_default_translate(key); }\n\n", out);

            fputs("static void cct_rt_fluxus_reverse(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus reverse received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->len <= 1) return;\n", out);
            fputs("    size_t elem_size = (size_t)flux->elem_size;\n", out);
            fputs("    unsigned char *tmp = (unsigned char*)cct_rt_alloc_or_fail(elem_size);\n", out);
            fputs("    unsigned char *base = (unsigned char*)flux->data;\n", out);
            fputs("    long long i = 0;\n", out);
            fputs("    long long j = flux->len - 1;\n", out);
            fputs("    while (i < j) {\n", out);
            fputs("        unsigned char *left = base + ((size_t)i * elem_size);\n", out);
            fputs("        unsigned char *right = base + ((size_t)j * elem_size);\n", out);
            fputs("        memcpy(tmp, left, elem_size);\n", out);
            fputs("        memcpy(left, right, elem_size);\n", out);
            fputs("        memcpy(right, tmp, elem_size);\n", out);
            fputs("        i += 1;\n", out);
            fputs("        j -= 1;\n", out);
            fputs("    }\n", out);
            fputs("    cct_rt_free_ptr(tmp);\n", out);
            fputs("}\n\n", out);

            fputs("static int cct_rt_fluxus_cmp_int(const void *a, const void *b) {\n", out);
            fputs("    long long av = *(const long long*)a;\n", out);
            fputs("    long long bv = *(const long long*)b;\n", out);
            fputs("    if (av < bv) return -1;\n", out);
            fputs("    if (av > bv) return 1;\n", out);
            fputs("    return 0;\n", out);
            fputs("}\n\n", out);

            fputs("static int cct_rt_fluxus_cmp_verbum(const void *a, const void *b) {\n", out);
            fputs("    const char *sa = *(const char *const *)a;\n", out);
            fputs("    const char *sb = *(const char *const *)b;\n", out);
            fputs("    return strcmp(sa ? sa : \"\", sb ? sb : \"\");\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_sort_int(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus sort_int received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->elem_size != (long long)sizeof(long long)) cct_rt_fail(\"fluxus sort_int requer elem_size == sizeof(long long)\");\n", out);
            fputs("    if (flux->len <= 1) return;\n", out);
            fputs("    qsort(flux->data, (size_t)flux->len, sizeof(long long), cct_rt_fluxus_cmp_int);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_fluxus_sort_verbum(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus sort_verbum received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    if (flux->elem_size != (long long)sizeof(char*)) cct_rt_fail(\"fluxus sort_verbum requer elem_size == sizeof(char*)\");\n", out);
            fputs("    if (flux->len <= 1) return;\n", out);
            fputs("    qsort(flux->data, (size_t)flux->len, sizeof(char*), cct_rt_fluxus_cmp_verbum);\n", out);
            fputs("}\n\n", out);

            fputs("static int cct_rt_alg_cmp_verbum_slot(const void *a, const void *b) {\n", out);
            fputs("    const char *sa = *(const char *const*)a;\n", out);
            fputs("    const char *sb = *(const char *const*)b;\n", out);
            fputs("    return strcmp(sa ? sa : \"\", sb ? sb : \"\");\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_alg_sort_verbum(void *arr_ptr, long long n) {\n", out);
            fputs("    if (n < 0) cct_rt_fail(\"alg_sort_verbum n must be >= 0\");\n", out);
            fputs("    if (n <= 1) return;\n", out);
            fputs("    if (!arr_ptr) cct_rt_fail(\"alg_sort_verbum received null array pointer\");\n", out);
            fputs("    qsort(arr_ptr, (size_t)n, sizeof(char*), cct_rt_alg_cmp_verbum_slot);\n", out);
            fputs("}\n\n", out);

            fputs("static void *cct_rt_fluxus_to_ptr(void *flux_ptr) {\n", out);
            fputs("    cct_rt_fluxus_require(flux_ptr, \"fluxus to_ptr received null instance\");\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)flux_ptr;\n", out);
            fputs("    return flux->data;\n", out);
            fputs("}\n\n", out);

            fputs("static cct_rt_fluxus_t *cct_rt_json_handle_require(long long handle, const char *ctx) {\n", out);
            fputs("    cct_rt_fluxus_t *flux = (cct_rt_fluxus_t*)(intptr_t)handle;\n", out);
            fputs("    if (!flux) cct_rt_fail((ctx && *ctx) ? ctx : \"json handle nulo\");\n", out);
            fputs("    return flux;\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_json_arr_handle_new(long long elem_size) {\n", out);
            fputs("    return (long long)(intptr_t)cct_rt_fluxus_create(elem_size);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_json_arr_handle_push(long long handle, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_push((void*)cct_rt_json_handle_require(handle, \"json arr handle invalido\"), elem_ptr);\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_json_arr_handle_len(long long handle) {\n", out);
            fputs("    return cct_rt_fluxus_len((void*)cct_rt_json_handle_require(handle, \"json arr handle invalido\"));\n", out);
            fputs("}\n\n", out);

            fputs("static void *cct_rt_json_arr_handle_get(long long handle, long long index) {\n", out);
            fputs("    return cct_rt_fluxus_get((void*)cct_rt_json_handle_require(handle, \"json arr handle invalido\"), index);\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_json_obj_handle_new(long long elem_size) {\n", out);
            fputs("    return (long long)(intptr_t)cct_rt_fluxus_create(elem_size);\n", out);
            fputs("}\n\n", out);

            fputs("static void cct_rt_json_obj_handle_push(long long handle, const void *elem_ptr) {\n", out);
            fputs("    cct_rt_fluxus_push((void*)cct_rt_json_handle_require(handle, \"json obj handle invalido\"), elem_ptr);\n", out);
            fputs("}\n\n", out);

            fputs("static long long cct_rt_json_obj_handle_len(long long handle) {\n", out);
            fputs("    return cct_rt_fluxus_len((void*)cct_rt_json_handle_require(handle, \"json obj handle invalido\"));\n", out);
            fputs("}\n\n", out);

            fputs("static void *cct_rt_json_obj_handle_get(long long handle, long long index) {\n", out);
            fputs("    return cct_rt_fluxus_get((void*)cct_rt_json_handle_require(handle, \"json obj handle invalido\"), index);\n", out);
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
        fputs("    cct_rt_map_entry_t **insert_order;\n", out);
        fputs("    size_t insert_capacity;\n", out);
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

        fputs("static void cct_rt_map_insert_order_reserve(cct_rt_map_t *map, size_t needed) {\n", out);
        fputs("    if (needed <= map->insert_capacity) return;\n", out);
        fputs("    size_t cap = map->insert_capacity ? map->insert_capacity * 2 : 16;\n", out);
        fputs("    while (cap < needed) cap *= 2;\n", out);
        fputs("    cct_rt_map_entry_t **next = (cct_rt_map_entry_t**)realloc(map->insert_order, cap * sizeof(cct_rt_map_entry_t*));\n", out);
        fputs("    if (!next) cct_rt_fail(\"map insert-order allocation failed\");\n", out);
        fputs("    map->insert_order = next;\n", out);
        fputs("    map->insert_capacity = cap;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_insert_order_append(cct_rt_map_t *map, cct_rt_map_entry_t *entry) {\n", out);
        fputs("    cct_rt_map_insert_order_reserve(map, map->len + 1);\n", out);
        fputs("    map->insert_order[map->len] = entry;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_insert_order_remove(cct_rt_map_t *map, cct_rt_map_entry_t *entry) {\n", out);
        fputs("    if (!map || !entry || map->len == 0) return;\n", out);
        fputs("    for (size_t i = 0; i < map->len; i++) {\n", out);
        fputs("        if (map->insert_order[i] != entry) continue;\n", out);
        fputs("        if (i + 1 < map->len) {\n", out);
        fputs("            memmove(&map->insert_order[i], &map->insert_order[i + 1], (map->len - i - 1) * sizeof(cct_rt_map_entry_t*));\n", out);
        fputs("        }\n", out);
        fputs("        map->insert_order[map->len - 1] = NULL;\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
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
        fputs("    map->insert_order = NULL;\n", out);
        fputs("    map->insert_capacity = 0;\n", out);
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
        fputs("    cct_rt_map_insert_order_append(map, ne);\n", out);
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
        fputs("            cct_rt_map_insert_order_remove(map, e);\n", out);
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
        fputs("    if (map->insert_order && map->len > 0) {\n", out);
        fputs("        memset(map->insert_order, 0, map->len * sizeof(cct_rt_map_entry_t*));\n", out);
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

        fputs("static void *cct_rt_map_copy(void *map_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_copy received null map\");\n", out);
        fputs("    cct_rt_map_t *src = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    cct_rt_map_t *dst = (cct_rt_map_t*)cct_rt_map_init((long long)src->key_size, (long long)src->value_size);\n", out);
        fputs("    cct_rt_map_reserve(dst, (long long)src->len);\n", out);
        fputs("    for (size_t i = 0; i < src->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = src->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_map_insert(dst, (const void*)e->key, (const void*)e->value);\n", out);
        fputs("    }\n", out);
        fputs("    return dst;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_map_keys(void *map_ptr, long long key_size) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_keys received null map\");\n", out);
        fputs("    if (key_size <= 0) cct_rt_fail(\"map_keys key_size must be > 0\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    if (cct_rt_map_min_storage_size((size_t)key_size) != map->key_size) cct_rt_fail(\"map_keys key_size mismatch\");\n", out);
        fputs("    void *out_f = cct_rt_fluxus_create(key_size);\n", out);
        fputs("    cct_rt_fluxus_reserve(out_f, (long long)map->len);\n", out);
        fputs("    for (size_t i = 0; i < map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_fluxus_push(out_f, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_map_values(void *map_ptr, long long value_size) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_values received null map\");\n", out);
        fputs("    if (value_size <= 0) cct_rt_fail(\"map_values value_size must be > 0\");\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    if (cct_rt_map_min_storage_size((size_t)value_size) != map->value_size) cct_rt_fail(\"map_values value_size mismatch\");\n", out);
        fputs("    void *out_f = cct_rt_fluxus_create(value_size);\n", out);
        fputs("    cct_rt_fluxus_reserve(out_f, (long long)map->len);\n", out);
        fputs("    for (size_t i = 0; i < map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_fluxus_push(out_f, (const void*)e->value);\n", out);
        fputs("    }\n", out);
        fputs("    return out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_merge(void *dest_ptr, void *src_ptr) {\n", out);
        fputs("    cct_rt_map_require(dest_ptr, \"map_merge received null destination map\");\n", out);
        fputs("    cct_rt_map_require(src_ptr, \"map_merge received null source map\");\n", out);
        fputs("    cct_rt_map_t *dest = (cct_rt_map_t*)dest_ptr;\n", out);
        fputs("    cct_rt_map_t *src = (cct_rt_map_t*)src_ptr;\n", out);
        fputs("    if (dest->key_size != src->key_size || dest->value_size != src->value_size) cct_rt_fail(\"map_merge requires compatible key/value sizes\");\n", out);
        fputs("    cct_rt_map_reserve(dest, (long long)src->len);\n", out);
        fputs("    for (size_t i = 0; i < src->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = src->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_map_insert(dest, (const void*)e->key, (const void*)e->value);\n", out);
        fputs("    }\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_map_t *map;\n", out);
        fputs("    size_t index;\n", out);
        fputs("} cct_rt_map_iter_t;\n\n", out);

        fputs("static void *cct_rt_map_iter_begin(void *map_ptr) {\n", out);
        fputs("    cct_rt_map_require(map_ptr, \"map_iter_begin received null map\");\n", out);
        fputs("    cct_rt_map_iter_t *it = (cct_rt_map_iter_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_map_iter_t));\n", out);
        fputs("    it->map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    it->index = 0;\n", out);
        fputs("    return it;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_map_iter_next(void *iter_ptr, void **key_ptr_out, void **value_ptr_out) {\n", out);
        fputs("    if (!iter_ptr) cct_rt_fail(\"map_iter_next received null iterator\");\n", out);
        fputs("    if (!key_ptr_out || !value_ptr_out) cct_rt_fail(\"map_iter_next requires key/value out pointers\");\n", out);
        fputs("    cct_rt_map_iter_t *it = (cct_rt_map_iter_t*)iter_ptr;\n", out);
        fputs("    if (!it->map) cct_rt_fail(\"map_iter_next iterator has null map\");\n", out);
        fputs("    while (it->index < it->map->len) {\n", out);
        fputs("        cct_rt_map_entry_t *e = it->map->insert_order[it->index++];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        *key_ptr_out = (void*)e->key;\n", out);
        fputs("        *value_ptr_out = (void*)e->value;\n", out);
        fputs("        return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_iter_end(void *iter_ptr) {\n", out);
        fputs("    if (!iter_ptr) return;\n", out);
        fputs("    cct_rt_free_ptr(iter_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_map_free(void *map_ptr) {\n", out);
        fputs("    if (!map_ptr) return;\n", out);
        fputs("    cct_rt_map_t *map = (cct_rt_map_t*)map_ptr;\n", out);
        fputs("    cct_rt_map_clear(map);\n", out);
        fputs("    cct_rt_free_ptr(map->buckets);\n", out);
        fputs("    cct_rt_free_ptr(map->insert_order);\n", out);
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

        fputs("static void cct_rt_set_reserve(void *set_ptr, long long additional) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_reserve received null set\");\n", out);
        fputs("    cct_rt_map_reserve(((cct_rt_set_t*)set_ptr)->map, additional);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_capacity(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_capacity received null set\");\n", out);
        fputs("    return cct_rt_map_capacity(((cct_rt_set_t*)set_ptr)->map);\n", out);
        fputs("}\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_set_t *set;\n", out);
        fputs("    size_t index;\n", out);
        fputs("} cct_rt_set_iter_t;\n\n", out);

        fputs("static void *cct_rt_set_iter_begin(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_iter_begin received null set\");\n", out);
        fputs("    cct_rt_set_iter_t *it = (cct_rt_set_iter_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_set_iter_t));\n", out);
        fputs("    it->set = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    it->index = 0;\n", out);
        fputs("    return it;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_iter_next(void *iter_ptr, void **elem_ptr_out) {\n", out);
        fputs("    if (!iter_ptr) cct_rt_fail(\"set_iter_next received null iterator\");\n", out);
        fputs("    if (!elem_ptr_out) cct_rt_fail(\"set_iter_next requires elem out pointer\");\n", out);
        fputs("    cct_rt_set_iter_t *it = (cct_rt_set_iter_t*)iter_ptr;\n", out);
        fputs("    if (!it->set || !it->set->map) cct_rt_fail(\"set_iter_next iterator has null set/map\");\n", out);
        fputs("    while (it->index < it->set->map->len) {\n", out);
        fputs("        cct_rt_map_entry_t *e = it->set->map->insert_order[it->index++];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        *elem_ptr_out = (void*)e->key;\n", out);
        fputs("        return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_set_iter_end(void *iter_ptr) {\n", out);
        fputs("    if (!iter_ptr) return;\n", out);
        fputs("    cct_rt_free_ptr(iter_ptr);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_validate_item_size(cct_rt_set_t *set, long long item_size, const char *ctx) {\n", out);
        fputs("    if (!set) cct_rt_fail((ctx && *ctx) ? ctx : \"set validation received null set\");\n", out);
        fputs("    if (item_size <= 0) cct_rt_fail(\"set operation requires item_size > 0\");\n", out);
        fputs("    if (cct_rt_map_min_storage_size((size_t)item_size) != set->map->key_size) {\n", out);
        fputs("        cct_rt_fail(\"set operation item_size mismatch\");\n", out);
        fputs("    }\n", out);
        fputs("    return item_size;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_copy(void *set_ptr, long long item_size) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_copy received null set\");\n", out);
        fputs("    cct_rt_set_t *src = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(src, item_size, \"set_copy invalid input\");\n", out);
        fputs("    cct_rt_set_t *dst = (cct_rt_set_t*)cct_rt_set_init(item_size);\n", out);
        fputs("    cct_rt_set_reserve(dst, (long long)src->map->len);\n", out);
        fputs("    for (size_t i = 0; i < src->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = src->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_set_insert(dst, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return dst;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_to_fluxus(void *set_ptr, long long item_size) {\n", out);
        fputs("    if (!set_ptr) cct_rt_fail(\"set_to_fluxus received null set\");\n", out);
        fputs("    cct_rt_set_t *set = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(set, item_size, \"set_to_fluxus invalid input\");\n", out);
        fputs("    void *out_f = cct_rt_fluxus_create(item_size);\n", out);
        fputs("    cct_rt_fluxus_reserve(out_f, (long long)set->map->len);\n", out);
        fputs("    for (size_t i = 0; i < set->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = set->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_fluxus_push(out_f, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_is_subset(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_is_subset received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_is_subset invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_is_subset invalid right set\");\n", out);
        fputs("    if (a->map->len > b->map->len) return 0LL;\n", out);
        fputs("    for (size_t i = 0; i < a->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = a->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        if (!cct_rt_set_contains(b, (const void*)e->key)) return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_set_equals(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_equals received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_equals invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_equals invalid right set\");\n", out);
        fputs("    if (a->map->len != b->map->len) return 0LL;\n", out);
        fputs("    return cct_rt_set_is_subset(a, b, item_size);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_union(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_union received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_union invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_union invalid right set\");\n", out);
        fputs("    cct_rt_set_t *out_s = (cct_rt_set_t*)cct_rt_set_copy(a, item_size);\n", out);
        fputs("    cct_rt_set_reserve(out_s, (long long)b->map->len);\n", out);
        fputs("    for (size_t i = 0; i < b->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = b->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        cct_rt_set_insert(out_s, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_intersection(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_intersection received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_intersection invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_intersection invalid right set\");\n", out);
        fputs("    cct_rt_set_t *out_s = (cct_rt_set_t*)cct_rt_set_init(item_size);\n", out);
        fputs("    cct_rt_set_t *probe = (a->map->len <= b->map->len) ? a : b;\n", out);
        fputs("    cct_rt_set_t *other = (probe == a) ? b : a;\n", out);
        fputs("    cct_rt_set_reserve(out_s, (long long)probe->map->len);\n", out);
        fputs("    for (size_t i = 0; i < probe->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = probe->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        if (cct_rt_set_contains(other, (const void*)e->key)) cct_rt_set_insert(out_s, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_difference(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_difference received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_difference invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_difference invalid right set\");\n", out);
        fputs("    cct_rt_set_t *out_s = (cct_rt_set_t*)cct_rt_set_init(item_size);\n", out);
        fputs("    cct_rt_set_reserve(out_s, (long long)a->map->len);\n", out);
        fputs("    for (size_t i = 0; i < a->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = a->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        if (!cct_rt_set_contains(b, (const void*)e->key)) cct_rt_set_insert(out_s, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_set_symmetric_difference(void *a_ptr, void *b_ptr, long long item_size) {\n", out);
        fputs("    if (!a_ptr || !b_ptr) cct_rt_fail(\"set_symmetric_difference received null set\");\n", out);
        fputs("    cct_rt_set_t *a = (cct_rt_set_t*)a_ptr;\n", out);
        fputs("    cct_rt_set_t *b = (cct_rt_set_t*)b_ptr;\n", out);
        fputs("    cct_rt_set_validate_item_size(a, item_size, \"set_symmetric_difference invalid left set\");\n", out);
        fputs("    cct_rt_set_validate_item_size(b, item_size, \"set_symmetric_difference invalid right set\");\n", out);
        fputs("    cct_rt_set_t *out_s = (cct_rt_set_t*)cct_rt_set_init(item_size);\n", out);
        fputs("    cct_rt_set_reserve(out_s, (long long)(a->map->len + b->map->len));\n", out);
        fputs("    for (size_t i = 0; i < a->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = a->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        if (!cct_rt_set_contains(b, (const void*)e->key)) cct_rt_set_insert(out_s, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    for (size_t i = 0; i < b->map->len; i++) {\n", out);
        fputs("        cct_rt_map_entry_t *e = b->map->insert_order[i];\n", out);
        fputs("        if (!e) continue;\n", out);
        fputs("        if (!cct_rt_set_contains(a, (const void*)e->key)) cct_rt_set_insert(out_s, (const void*)e->key);\n", out);
        fputs("    }\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_set_free(void *set_ptr) {\n", out);
        fputs("    if (!set_ptr) return;\n", out);
        fputs("    cct_rt_set_t *set = (cct_rt_set_t*)set_ptr;\n", out);
        fputs("    cct_rt_map_free(set->map);\n", out);
        fputs("    cct_rt_free_ptr(set);\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_verbum_helpers) {
        fputs("typedef struct {\n", out);
        fputs("    char *buf;\n", out);
        fputs("    size_t len;\n", out);
        fputs("    size_t cap;\n", out);
        fputs("} cct_rt_molde_ctx_t;\n\n", out);

        fputs("static cct_rt_molde_ctx_t *cct_rt_molde_begin(void) {\n", out);
        fputs("    cct_rt_molde_ctx_t *m = (cct_rt_molde_ctx_t*)malloc(sizeof(cct_rt_molde_ctx_t));\n", out);
        fputs("    if (!m) cct_rt_fail(\"MOLDE allocation failed\");\n", out);
        fputs("    m->cap = 128;\n", out);
        fputs("    m->len = 0;\n", out);
        fputs("    m->buf = (char*)malloc(m->cap);\n", out);
        fputs("    if (!m->buf) cct_rt_fail(\"MOLDE buffer allocation failed\");\n", out);
        fputs("    m->buf[0] = '\\0';\n", out);
        fputs("    return m;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_append(cct_rt_molde_ctx_t *m, const char *s, size_t slen) {\n", out);
        fputs("    if (!m || !s || slen == 0) return;\n", out);
        fputs("    while (m->len + slen + 1 > m->cap) {\n", out);
        fputs("        size_t next = m->cap * 2;\n", out);
        fputs("        if (next <= m->cap) cct_rt_fail(\"MOLDE buffer overflow\");\n", out);
        fputs("        char *grown = (char*)realloc(m->buf, next);\n", out);
        fputs("        if (!grown) cct_rt_fail(\"MOLDE realloc failed\");\n", out);
        fputs("        m->buf = grown;\n", out);
        fputs("        m->cap = next;\n", out);
        fputs("    }\n", out);
        fputs("    memcpy(m->buf + m->len, s, slen);\n", out);
        fputs("    m->len += slen;\n", out);
        fputs("    m->buf[m->len] = '\\0';\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_str(cct_rt_molde_ctx_t *m, const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    cct_rt_molde_append(m, src, strlen(src));\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_rex(cct_rt_molde_ctx_t *m, long long v) {\n", out);
        fputs("    char tmp[64];\n", out);
        fputs("    int n = snprintf(tmp, sizeof(tmp), \"%lld\", v);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"MOLDE rex formatting failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_dux(cct_rt_molde_ctx_t *m, unsigned long long v) {\n", out);
        fputs("    char tmp[64];\n", out);
        fputs("    int n = snprintf(tmp, sizeof(tmp), \"%llu\", v);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"MOLDE dux formatting failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_umbra(cct_rt_molde_ctx_t *m, double v) {\n", out);
        fputs("    char tmp[128];\n", out);
        fputs("    int n = snprintf(tmp, sizeof(tmp), \"%g\", v);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"MOLDE umbra formatting failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_verum(cct_rt_molde_ctx_t *m, long long v) {\n", out);
        fputs("    const char *txt = v ? \"verum\" : \"falsum\";\n", out);
        fputs("    cct_rt_molde_append(m, txt, strlen(txt));\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_fmt_build(char *fmt, size_t fmt_cap, const char *spec, const char *length_mod) {\n", out);
        fputs("    if (!fmt || fmt_cap == 0) cct_rt_fail(\"MOLDE format buffer invalid\");\n", out);
        fputs("    if (!spec || !spec[0]) cct_rt_fail(\"MOLDE format spec vazio\");\n", out);
        fputs("    size_t spec_len = strlen(spec);\n", out);
        fputs("    char conv = spec[spec_len - 1];\n", out);
        fputs("    size_t prefix_len = spec_len - 1;\n", out);
        fputs("    int n;\n", out);
        fputs("    if (length_mod && length_mod[0]) n = snprintf(fmt, fmt_cap, \"%%%.*s%s%c\", (int)prefix_len, spec, length_mod, conv);\n", out);
        fputs("    else n = snprintf(fmt, fmt_cap, \"%%%s\", spec);\n", out);
        fputs("    if (n < 0 || (size_t)n >= fmt_cap) cct_rt_fail(\"MOLDE format spec overflow\");\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_rex_fmt(cct_rt_molde_ctx_t *m, long long v, const char *spec) {\n", out);
        fputs("    char fmt[64];\n", out);
        fputs("    cct_rt_molde_fmt_build(fmt, sizeof(fmt), spec, \"ll\");\n", out);
        fputs("    int need = snprintf(NULL, 0, fmt, v);\n", out);
        fputs("    if (need < 0) cct_rt_fail(\"MOLDE rex fmt sizing failed\");\n", out);
        fputs("    char *tmp = (char*)malloc((size_t)need + 1);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"MOLDE rex fmt allocation failed\");\n", out);
        fputs("    int wrote = snprintf(tmp, (size_t)need + 1, fmt, v);\n", out);
        fputs("    if (wrote != need) cct_rt_fail(\"MOLDE rex fmt write failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)wrote);\n", out);
        fputs("    free(tmp);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_dux_fmt(cct_rt_molde_ctx_t *m, unsigned long long v, const char *spec) {\n", out);
        fputs("    char fmt[64];\n", out);
        fputs("    cct_rt_molde_fmt_build(fmt, sizeof(fmt), spec, \"ll\");\n", out);
        fputs("    int need = snprintf(NULL, 0, fmt, v);\n", out);
        fputs("    if (need < 0) cct_rt_fail(\"MOLDE dux fmt sizing failed\");\n", out);
        fputs("    char *tmp = (char*)malloc((size_t)need + 1);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"MOLDE dux fmt allocation failed\");\n", out);
        fputs("    int wrote = snprintf(tmp, (size_t)need + 1, fmt, v);\n", out);
        fputs("    if (wrote != need) cct_rt_fail(\"MOLDE dux fmt write failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)wrote);\n", out);
        fputs("    free(tmp);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_umbra_fmt(cct_rt_molde_ctx_t *m, double v, const char *spec) {\n", out);
        fputs("    char fmt[64];\n", out);
        fputs("    cct_rt_molde_fmt_build(fmt, sizeof(fmt), spec, NULL);\n", out);
        fputs("    int need = snprintf(NULL, 0, fmt, v);\n", out);
        fputs("    if (need < 0) cct_rt_fail(\"MOLDE umbra fmt sizing failed\");\n", out);
        fputs("    char *tmp = (char*)malloc((size_t)need + 1);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"MOLDE umbra fmt allocation failed\");\n", out);
        fputs("    int wrote = snprintf(tmp, (size_t)need + 1, fmt, v);\n", out);
        fputs("    if (wrote != need) cct_rt_fail(\"MOLDE umbra fmt write failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)wrote);\n", out);
        fputs("    free(tmp);\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_molde_parse_align_spec(const char *spec, char *dir, long long *width) {\n", out);
        fputs("    if (!spec || !dir || !width) return 0;\n", out);
        fputs("    if (spec[0] != '<' && spec[0] != '>') return 0;\n", out);
        fputs("    if (!spec[1]) return 0;\n", out);
        fputs("    long long acc = 0;\n", out);
        fputs("    for (size_t i = 1; spec[i]; i++) {\n", out);
        fputs("        if (spec[i] < '0' || spec[i] > '9') return 0;\n", out);
        fputs("        acc = (acc * 10LL) + (long long)(spec[i] - '0');\n", out);
        fputs("        if (acc < 0) cct_rt_fail(\"MOLDE alignment width overflow\");\n", out);
        fputs("    }\n", out);
        fputs("    *dir = spec[0];\n", out);
        fputs("    *width = acc;\n", out);
        fputs("    return 1;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_append_spaces(cct_rt_molde_ctx_t *m, size_t count) {\n", out);
        fputs("    while (count > 0) {\n", out);
        fputs("        cct_rt_molde_append(m, \" \", 1);\n", out);
        fputs("        count--;\n", out);
        fputs("    }\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_molde_str_fmt(cct_rt_molde_ctx_t *m, const char *s, const char *spec) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    char dir = '\\0';\n", out);
        fputs("    long long width = 0;\n", out);
        fputs("    if (cct_rt_molde_parse_align_spec(spec, &dir, &width)) {\n", out);
        fputs("        size_t slen = strlen(src);\n", out);
        fputs("        size_t pad = (width > (long long)slen) ? (size_t)(width - (long long)slen) : 0;\n", out);
        fputs("        if (dir == '>') cct_rt_molde_append_spaces(m, pad);\n", out);
        fputs("        cct_rt_molde_append(m, src, slen);\n", out);
        fputs("        if (dir == '<') cct_rt_molde_append_spaces(m, pad);\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    char fmt[64];\n", out);
        fputs("    cct_rt_molde_fmt_build(fmt, sizeof(fmt), spec, NULL);\n", out);
        fputs("    int need = snprintf(NULL, 0, fmt, src);\n", out);
        fputs("    if (need < 0) cct_rt_fail(\"MOLDE str fmt sizing failed\");\n", out);
        fputs("    char *tmp = (char*)malloc((size_t)need + 1);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"MOLDE str fmt allocation failed\");\n", out);
        fputs("    int wrote = snprintf(tmp, (size_t)need + 1, fmt, src);\n", out);
        fputs("    if (wrote != need) cct_rt_fail(\"MOLDE str fmt write failed\");\n", out);
        fputs("    cct_rt_molde_append(m, tmp, (size_t)wrote);\n", out);
        fputs("    free(tmp);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_molde_end(cct_rt_molde_ctx_t *m) {\n", out);
        fputs("    if (!m) return NULL;\n", out);
        fputs("    char *out_s = m->buf;\n", out);
        fputs("    m->buf = NULL;\n", out);
        fputs("    free(m);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

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

        fputs("static long long cct_rt_verbum_starts_with(const char *s, const char *prefix) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *pre = prefix ? prefix : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t plen = strlen(pre);\n", out);
        fputs("    if (plen > slen) return 0LL;\n", out);
        fputs("    return (strncmp(src, pre, plen) == 0) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_ends_with(const char *s, const char *suffix) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *suf = suffix ? suffix : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t suflen = strlen(suf);\n", out);
        fputs("    if (suflen > slen) return 0LL;\n", out);
        fputs("    return (memcmp(src + (slen - suflen), suf, suflen) == 0) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_strip_prefix(const char *s, const char *prefix) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *pre = prefix ? prefix : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t plen = strlen(pre);\n", out);
        fputs("    if (!cct_rt_verbum_starts_with(src, pre)) return (char*)src;\n", out);
        fputs("    return cct_rt_verbum_substring(src, (long long)plen, (long long)slen);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_strip_suffix(const char *s, const char *suffix) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *suf = suffix ? suffix : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t suflen = strlen(suf);\n", out);
        fputs("    if (!cct_rt_verbum_ends_with(src, suf)) return (char*)src;\n", out);
        fputs("    return cct_rt_verbum_substring(src, 0LL, (long long)(slen - suflen));\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_replace(const char *s, const char *from, const char *to) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *needle = from ? from : \"\";\n", out);
        fputs("    const char *repl = to ? to : \"\";\n", out);
        fputs("    size_t from_len = strlen(needle);\n", out);
        fputs("    if (from_len == 0) return (char*)src;\n", out);
        fputs("    const char *found = strstr(src, needle);\n", out);
        fputs("    if (!found) return (char*)src;\n", out);
        fputs("    size_t pre = (size_t)(found - src);\n", out);
        fputs("    size_t to_len = strlen(repl);\n", out);
        fputs("    size_t src_len = strlen(src);\n", out);
        fputs("    size_t out_len = src_len - from_len + to_len;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum replace allocation failed\");\n", out);
        fputs("    if (pre > 0) memcpy(out_s, src, pre);\n", out);
        fputs("    if (to_len > 0) memcpy(out_s + pre, repl, to_len);\n", out);
        fputs("    strcpy(out_s + pre + to_len, found + from_len);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_replace_all(const char *s, const char *from, const char *to) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *needle = from ? from : \"\";\n", out);
        fputs("    const char *repl = to ? to : \"\";\n", out);
        fputs("    size_t from_len = strlen(needle);\n", out);
        fputs("    if (from_len == 0) return (char*)src;\n", out);
        fputs("    size_t to_len = strlen(repl);\n", out);
        fputs("    size_t src_len = strlen(src);\n", out);
        fputs("    size_t count = 0;\n", out);
        fputs("    const char *p = src;\n", out);
        fputs("    while ((p = strstr(p, needle)) != NULL) {\n", out);
        fputs("        count++;\n", out);
        fputs("        p += from_len;\n", out);
        fputs("    }\n", out);
        fputs("    if (count == 0) return (char*)src;\n", out);
        fputs("    size_t out_len = src_len;\n", out);
        fputs("    if (to_len >= from_len) {\n", out);
        fputs("        out_len += count * (to_len - from_len);\n", out);
        fputs("    } else {\n", out);
        fputs("        out_len -= count * (from_len - to_len);\n", out);
        fputs("    }\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum replace_all allocation failed\");\n", out);
        fputs("    const char *cur = src;\n", out);
        fputs("    char *w = out_s;\n", out);
        fputs("    while ((p = strstr(cur, needle)) != NULL) {\n", out);
        fputs("        size_t seg_len = (size_t)(p - cur);\n", out);
        fputs("        if (seg_len > 0) memcpy(w, cur, seg_len);\n", out);
        fputs("        w += seg_len;\n", out);
        fputs("        if (to_len > 0) memcpy(w, repl, to_len);\n", out);
        fputs("        w += to_len;\n", out);
        fputs("        cur = p + from_len;\n", out);
        fputs("    }\n", out);
        fputs("    strcpy(w, cur);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_to_upper(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    char *out_s = (char*)malloc(slen + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum to_upper allocation failed\");\n", out);
        fputs("    for (size_t i = 0; i < slen; i++) {\n", out);
        fputs("        unsigned char ch = (unsigned char)src[i];\n", out);
        fputs("        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - ('a' - 'A'));\n", out);
        fputs("        out_s[i] = (char)ch;\n", out);
        fputs("    }\n", out);
        fputs("    out_s[slen] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_to_lower(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    char *out_s = (char*)malloc(slen + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum to_lower allocation failed\");\n", out);
        fputs("    for (size_t i = 0; i < slen; i++) {\n", out);
        fputs("        unsigned char ch = (unsigned char)src[i];\n", out);
        fputs("        if (ch >= 'A' && ch <= 'Z') ch = (unsigned char)(ch + ('a' - 'A'));\n", out);
        fputs("        out_s[i] = (char)ch;\n", out);
        fputs("    }\n", out);
        fputs("    out_s[slen] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_trim_left(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t begin = 0;\n", out);
        fputs("    while (begin < slen && isspace((unsigned char)src[begin])) begin++;\n", out);
        fputs("    size_t out_len = slen - begin;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum trim_left allocation failed\");\n", out);
        fputs("    if (out_len > 0) memcpy(out_s, src + begin, out_len);\n", out);
        fputs("    out_s[out_len] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_trim_right(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t end = strlen(src);\n", out);
        fputs("    while (end > 0 && isspace((unsigned char)src[end - 1])) end--;\n", out);
        fputs("    char *out_s = (char*)malloc(end + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum trim_right allocation failed\");\n", out);
        fputs("    if (end > 0) memcpy(out_s, src, end);\n", out);
        fputs("    out_s[end] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_trim_char(const char *s, long long c) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (c < 0 || c > 255) cct_rt_fail(\"verbum trim_char expects byte range 0..255\");\n", out);
        fputs("    unsigned char ch = (unsigned char)c;\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t begin = 0;\n", out);
        fputs("    size_t end = slen;\n", out);
        fputs("    while (begin < slen && (unsigned char)src[begin] == ch) begin++;\n", out);
        fputs("    while (end > begin && (unsigned char)src[end - 1] == ch) end--;\n", out);
        fputs("    size_t out_len = end - begin;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum trim_char allocation failed\");\n", out);
        fputs("    if (out_len > 0) memcpy(out_s, src + begin, out_len);\n", out);
        fputs("    out_s[out_len] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_repeat(const char *s, long long n) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"verbum repeat count negativo\");\n", out);
        fputs("    if (n == 0) return (char*)\"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    if (slen == 0) return (char*)\"\";\n", out);
        fputs("    size_t count = (size_t)n;\n", out);
        fputs("    if (slen > (SIZE_MAX - 1) / count) cct_rt_fail(\"verbum repeat allocation overflow\");\n", out);
        fputs("    size_t out_len = slen * count;\n", out);
        fputs("    char *out_s = (char*)malloc(out_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum repeat allocation failed\");\n", out);
        fputs("    char *w = out_s;\n", out);
        fputs("    for (size_t i = 0; i < count; i++) {\n", out);
        fputs("        memcpy(w, src, slen);\n", out);
        fputs("        w += slen;\n", out);
        fputs("    }\n", out);
        fputs("    out_s[out_len] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_pad_left(const char *s, long long width, long long fill) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (fill < 0 || fill > 255) cct_rt_fail(\"verbum pad fill expects byte range 0..255\");\n", out);
        fputs("    if (width < 0) return (char*)src;\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t w = (size_t)width;\n", out);
        fputs("    if (slen >= w) return (char*)src;\n", out);
        fputs("    size_t pad = w - slen;\n", out);
        fputs("    char *out_s = (char*)malloc(w + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum pad_left allocation failed\");\n", out);
        fputs("    memset(out_s, (int)(unsigned char)fill, pad);\n", out);
        fputs("    memcpy(out_s + pad, src, slen);\n", out);
        fputs("    out_s[w] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_pad_right(const char *s, long long width, long long fill) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (fill < 0 || fill > 255) cct_rt_fail(\"verbum pad fill expects byte range 0..255\");\n", out);
        fputs("    if (width < 0) return (char*)src;\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t w = (size_t)width;\n", out);
        fputs("    if (slen >= w) return (char*)src;\n", out);
        fputs("    size_t pad = w - slen;\n", out);
        fputs("    char *out_s = (char*)malloc(w + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum pad_right allocation failed\");\n", out);
        fputs("    memcpy(out_s, src, slen);\n", out);
        fputs("    memset(out_s + slen, (int)(unsigned char)fill, pad);\n", out);
        fputs("    out_s[w] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_center(const char *s, long long width, long long fill) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (fill < 0 || fill > 255) cct_rt_fail(\"verbum pad fill expects byte range 0..255\");\n", out);
        fputs("    if (width < 0) return (char*)src;\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t w = (size_t)width;\n", out);
        fputs("    if (slen >= w) return (char*)src;\n", out);
        fputs("    size_t total = w - slen;\n", out);
        fputs("    size_t left = total / 2;\n", out);
        fputs("    size_t right = total - left;\n", out);
        fputs("    char *out_s = (char*)malloc(w + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum center allocation failed\");\n", out);
        fputs("    memset(out_s, (int)(unsigned char)fill, left);\n", out);
        fputs("    memcpy(out_s + left, src, slen);\n", out);
        fputs("    memset(out_s + left + slen, (int)(unsigned char)fill, right);\n", out);
        fputs("    out_s[w] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_last_find(const char *s, const char *sub) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *needle = sub ? sub : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t nlen = strlen(needle);\n", out);
        fputs("    if (nlen == 0) return (long long)slen;\n", out);
        fputs("    const char *last = NULL;\n", out);
        fputs("    const char *p = src;\n", out);
        fputs("    while ((p = strstr(p, needle)) != NULL) {\n", out);
        fputs("        last = p;\n", out);
        fputs("        p += 1;\n", out);
        fputs("    }\n", out);
        fputs("    if (!last) return -1LL;\n", out);
        fputs("    return (long long)(last - src);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_find_from(const char *s, const char *sub, long long offset) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *needle = sub ? sub : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    if (offset < 0 || (size_t)offset > slen) cct_rt_fail(\"verbum find_from offset invalido\");\n", out);
        fputs("    size_t nlen = strlen(needle);\n", out);
        fputs("    if (nlen == 0) return offset;\n", out);
        fputs("    size_t i = (size_t)offset;\n", out);
        fputs("    while (i + nlen <= slen) {\n", out);
        fputs("        if (memcmp(src + i, needle, nlen) == 0) return (long long)i;\n", out);
        fputs("        i++;\n", out);
        fputs("    }\n", out);
        fputs("    return -1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_count_occurrences(const char *s, const char *sub) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *needle = sub ? sub : \"\";\n", out);
        fputs("    size_t nlen = strlen(needle);\n", out);
        fputs("    if (nlen == 0) return 0LL;\n", out);
        fputs("    long long count = 0;\n", out);
        fputs("    const char *p = src;\n", out);
        fputs("    while ((p = strstr(p, needle)) != NULL) {\n", out);
        fputs("        count++;\n", out);
        fputs("        p += nlen;\n", out);
        fputs("    }\n", out);
        fputs("    return count;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_reverse(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    char *out_s = (char*)malloc(slen + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum reverse allocation failed\");\n", out);
        fputs("    for (size_t i = 0; i < slen; i++) {\n", out);
        fputs("        out_s[i] = src[slen - 1 - i];\n", out);
        fputs("    }\n", out);
        fputs("    out_s[slen] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_equals_ignore_case(const char *a, const char *b) {\n", out);
        fputs("    const char *sa = a ? a : \"\";\n", out);
        fputs("    const char *sb = b ? b : \"\";\n", out);
        fputs("    size_t la = strlen(sa);\n", out);
        fputs("    size_t lb = strlen(sb);\n", out);
        fputs("    if (la != lb) return 0LL;\n", out);
        fputs("    for (size_t i = 0; i < la; i++) {\n", out);
        fputs("        unsigned char ca = (unsigned char)sa[i];\n", out);
        fputs("        unsigned char cb = (unsigned char)sb[i];\n", out);
        fputs("        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));\n", out);
        fputs("        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));\n", out);
        fputs("        if (ca != cb) return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_slice(const char *s, long long inicio, long long comprimento) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    long long slen = (long long)strlen(src);\n", out);
        fputs("    if (inicio < 0 || comprimento < 0 || inicio > slen || comprimento > (slen - inicio)) {\n", out);
        fputs("        cct_rt_fail(\"verbum slice bounds invalid\");\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_verbum_substring(src, inicio, inicio + comprimento);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_verbum_is_ascii(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    for (const unsigned char *p = (const unsigned char*)src; *p; ++p) {\n", out);
        fputs("        if (*p > 127U) return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    return 1LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_verbum_split(const char *s, const char *sep) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    const char *delim = sep ? sep : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    size_t dlen = strlen(delim);\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    if (slen == 0) {\n", out);
        fputs("        char *empty = (char*)\"\";\n", out);
        fputs("        cct_rt_fluxus_push((void*)out_f, (const void*)&empty);\n", out);
        fputs("        return (void*)out_f;\n", out);
        fputs("    }\n", out);
        fputs("    if (dlen == 0) {\n", out);
        fputs("        for (size_t i = 0; i < slen; i++) {\n", out);
        fputs("            char *piece = cct_rt_verbum_substring(src, (long long)i, (long long)(i + 1));\n", out);
        fputs("            cct_rt_fluxus_push((void*)out_f, (const void*)&piece);\n", out);
        fputs("        }\n", out);
        fputs("        return (void*)out_f;\n", out);
        fputs("    }\n", out);
        fputs("    const char *cur = src;\n", out);
        fputs("    const char *p = strstr(cur, delim);\n", out);
        fputs("    while (p) {\n", out);
        fputs("        char *piece = cct_rt_verbum_substring(src, (long long)(cur - src), (long long)(p - src));\n", out);
        fputs("        cct_rt_fluxus_push((void*)out_f, (const void*)&piece);\n", out);
        fputs("        cur = p + dlen;\n", out);
        fputs("        p = strstr(cur, delim);\n", out);
        fputs("    }\n", out);
        fputs("    char *tail = cct_rt_verbum_substring(src, (long long)(cur - src), (long long)slen);\n", out);
        fputs("    cct_rt_fluxus_push((void*)out_f, (const void*)&tail);\n", out);
        fputs("    return (void*)out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_verbum_split_char(const char *s, long long c) {\n", out);
        fputs("    if (c < 0 || c > 255) cct_rt_fail(\"verbum split_char expects byte range 0..255\");\n", out);
        fputs("    char sep_buf[2];\n", out);
        fputs("    sep_buf[0] = (char)(unsigned char)c;\n", out);
        fputs("    sep_buf[1] = '\\0';\n", out);
        fputs("    return cct_rt_verbum_split(s, sep_buf);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_verbum_join(void *parts_ptr, const char *sep) {\n", out);
        fputs("    if (!parts_ptr) cct_rt_fail(\"verbum join received null parts\");\n", out);
        fputs("    cct_rt_fluxus_t *parts = (cct_rt_fluxus_t*)parts_ptr;\n", out);
        fputs("    if (parts->elem_size != (long long)sizeof(char*)) cct_rt_fail(\"verbum join requer fluxus de VERBUM\");\n", out);
        fputs("    if (parts->len <= 0) return (char*)\"\";\n", out);
        fputs("    const char *delim = sep ? sep : \"\";\n", out);
        fputs("    size_t dlen = strlen(delim);\n", out);
        fputs("    char **items = (char**)parts->data;\n", out);
        fputs("    size_t total = 0;\n", out);
        fputs("    for (long long i = 0; i < parts->len; i++) {\n", out);
        fputs("        const char *piece = items[i] ? items[i] : \"\";\n", out);
        fputs("        size_t plen = strlen(piece);\n", out);
        fputs("        if (SIZE_MAX - total < plen) cct_rt_fail(\"verbum join allocation overflow\");\n", out);
        fputs("        total += plen;\n", out);
        fputs("        if (i + 1 < parts->len) {\n", out);
        fputs("            if (SIZE_MAX - total < dlen) cct_rt_fail(\"verbum join allocation overflow\");\n", out);
        fputs("            total += dlen;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    char *out_s = (char*)malloc(total + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"verbum join allocation failed\");\n", out);
        fputs("    char *w = out_s;\n", out);
        fputs("    for (long long i = 0; i < parts->len; i++) {\n", out);
        fputs("        const char *piece = items[i] ? items[i] : \"\";\n", out);
        fputs("        size_t plen = strlen(piece);\n", out);
        fputs("        if (plen > 0) {\n", out);
        fputs("            memcpy(w, piece, plen);\n", out);
        fputs("            w += plen;\n", out);
        fputs("        }\n", out);
        fputs("        if (i + 1 < parts->len && dlen > 0) {\n", out);
        fputs("            memcpy(w, delim, dlen);\n", out);
        fputs("            w += dlen;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    *w = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_verbum_lines(const char *s) {\n", out);
        fputs("    cct_rt_fluxus_t *out_f = (cct_rt_fluxus_t*)cct_rt_verbum_split_char(s, 10LL);\n", out);
        fputs("    if (out_f->elem_size != (long long)sizeof(char*)) cct_rt_fail(\"verbum lines requer fluxus de VERBUM\");\n", out);
        fputs("    char **items = (char**)out_f->data;\n", out);
        fputs("    for (long long i = 0; i < out_f->len; i++) {\n", out);
        fputs("        const char *piece = items[i] ? items[i] : \"\";\n", out);
        fputs("        size_t plen = strlen(piece);\n", out);
        fputs("        if (plen > 0 && (unsigned char)piece[plen - 1] == 13U) {\n", out);
        fputs("            items[i] = cct_rt_verbum_substring(piece, 0LL, (long long)(plen - 1));\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    return (void*)out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_verbum_words(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t slen = strlen(src);\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    size_t i = 0;\n", out);
        fputs("    while (i < slen) {\n", out);
        fputs("        while (i < slen) {\n", out);
        fputs("            unsigned char ch = (unsigned char)src[i];\n", out);
        fputs("            if (!(ch == 32U || ch == 9U || ch == 10U || ch == 13U)) break;\n", out);
        fputs("            i++;\n", out);
        fputs("        }\n", out);
        fputs("        if (i >= slen) break;\n", out);
        fputs("        size_t start = i;\n", out);
        fputs("        while (i < slen) {\n", out);
        fputs("            unsigned char ch = (unsigned char)src[i];\n", out);
        fputs("            if (ch == 32U || ch == 9U || ch == 10U || ch == 13U) break;\n", out);
        fputs("            i++;\n", out);
        fputs("        }\n", out);
        fputs("        char *piece = cct_rt_verbum_substring(src, (long long)start, (long long)i);\n", out);
        fputs("        cct_rt_fluxus_push((void*)out_f, (const void*)&piece);\n", out);
        fputs("    }\n", out);
        fputs("    return (void*)out_f;\n", out);
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

        fputs("static char *cct_rt_fmt_unsigned_base(unsigned long long v, unsigned base, const char *digits, const char *prefix) {\n", out);
        fputs("    if (base < 2U) cct_rt_fail(\"fmt stringify base invalida\");\n", out);
        fputs("    const char *alpha = digits ? digits : \"0123456789abcdef\";\n", out);
        fputs("    const char *pre = prefix ? prefix : \"\";\n", out);
        fputs("    char buf[128];\n", out);
        fputs("    size_t i = sizeof(buf);\n", out);
        fputs("    buf[--i] = '\\0';\n", out);
        fputs("    do {\n", out);
        fputs("        unsigned d = (unsigned)(v % (unsigned long long)base);\n", out);
        fputs("        buf[--i] = alpha[d];\n", out);
        fputs("        v /= (unsigned long long)base;\n", out);
        fputs("    } while (v > 0ULL);\n", out);
        fputs("    size_t pre_len = strlen(pre);\n", out);
        fputs("    size_t body_len = strlen(buf + i);\n", out);
        fputs("    char *out_s = (char*)malloc(pre_len + body_len + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify base allocation failed\");\n", out);
        fputs("    if (pre_len > 0) memcpy(out_s, pre, pre_len);\n", out);
        fputs("    memcpy(out_s + pre_len, buf + i, body_len + 1);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_int_hex(long long x) {\n", out);
        fputs("    unsigned long long v = (x < 0) ? (((unsigned long long)x) & 0xffffffffULL) : (unsigned long long)x;\n", out);
        fputs("    return cct_rt_fmt_unsigned_base(v, 16U, \"0123456789abcdef\", \"0x\");\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_int_hex_upper(long long x) {\n", out);
        fputs("    unsigned long long v = (x < 0) ? (((unsigned long long)x) & 0xffffffffULL) : (unsigned long long)x;\n", out);
        fputs("    return cct_rt_fmt_unsigned_base(v, 16U, \"0123456789ABCDEF\", \"0x\");\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_int_oct(long long x) {\n", out);
        fputs("    unsigned long long v = (x < 0) ? (((unsigned long long)x) & 0xffffffffULL) : (unsigned long long)x;\n", out);
        fputs("    return cct_rt_fmt_unsigned_base(v, 8U, \"01234567\", \"0o\");\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_int_bin(long long x) {\n", out);
        fputs("    unsigned long long v = (x < 0) ? (((unsigned long long)x) & 0xffffffffULL) : (unsigned long long)x;\n", out);
        fputs("    return cct_rt_fmt_unsigned_base(v, 2U, \"01\", \"0b\");\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_uint(long long x) {\n", out);
        fputs("    if (x < 0) cct_rt_fail(\"fmt stringify_uint expects valor >= 0\");\n", out);
        fputs("    unsigned long long v = (unsigned long long)x;\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%llu\", v);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_uint formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_uint allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%llu\", v);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_int_padded(long long x, long long width, long long fill) {\n", out);
        fputs("    if (fill < 0 || fill > 255) cct_rt_fail(\"fmt stringify_int_padded fill fora do intervalo 0..255\");\n", out);
        fputs("    char *base = cct_rt_fmt_stringify_int(x);\n", out);
        fputs("    if (width <= 0) return base;\n", out);
        fputs("    size_t len = strlen(base);\n", out);
        fputs("    size_t w = (size_t)width;\n", out);
        fputs("    if (len >= w) return base;\n", out);
        fputs("    size_t pad = w - len;\n", out);
        fputs("    char *out_s = (char*)malloc(w + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_int_padded allocation failed\");\n", out);
        fputs("    memset(out_s, (int)(unsigned char)fill, pad);\n", out);
        fputs("    memcpy(out_s + pad, base, len + 1);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_real_prec(double x, long long prec) {\n", out);
        fputs("    if (prec < 0) cct_rt_fail(\"fmt stringify_real_prec prec negativo\");\n", out);
        fputs("    if (prec > 1000) cct_rt_fail(\"fmt stringify_real_prec prec invalido\");\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%.*f\", (int)prec, x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_real_prec formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_real_prec allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%.*f\", (int)prec, x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_real_sci(double x) {\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%e\", x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_real_sci formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_real_sci allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%e\", x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_stringify_real_fixed(double x, long long prec) {\n", out);
        fputs("    if (prec < 0) cct_rt_fail(\"fmt stringify_real_fixed prec negativo\");\n", out);
        fputs("    if (prec > 1000) cct_rt_fail(\"fmt stringify_real_fixed prec invalido\");\n", out);
        fputs("    int n = snprintf(NULL, 0, \"%.*f\", (int)prec, x);\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"fmt stringify_real_fixed formatting failed\");\n", out);
        fputs("    char *out_s = (char*)malloc((size_t)n + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt stringify_real_fixed allocation failed\");\n", out);
        fputs("    snprintf(out_s, (size_t)n + 1, \"%.*f\", (int)prec, x);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_format_1(const char *tmpl, const char *a) {\n", out);
        fputs("    const char *src = tmpl ? tmpl : \"\";\n", out);
        fputs("    const char *arg = a ? a : \"\";\n", out);
        fputs("    const char *pos = strstr(src, \"{}\");\n", out);
        fputs("    if (!pos) return (char*)src;\n", out);
        fputs("    size_t pre = (size_t)(pos - src);\n", out);
        fputs("    size_t alen = strlen(arg);\n", out);
        fputs("    size_t rest = strlen(pos + 2);\n", out);
        fputs("    char *out_s = (char*)malloc(pre + alen + rest + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt format_1 allocation failed\");\n", out);
        fputs("    if (pre > 0) memcpy(out_s, src, pre);\n", out);
        fputs("    if (alen > 0) memcpy(out_s + pre, arg, alen);\n", out);
        fputs("    memcpy(out_s + pre + alen, pos + 2, rest + 1);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_format_2(const char *tmpl, const char *a, const char *b) {\n", out);
        fputs("    char *step1 = cct_rt_fmt_format_1(tmpl, a);\n", out);
        fputs("    return cct_rt_fmt_format_1(step1, b);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_format_3(const char *tmpl, const char *a, const char *b, const char *c) {\n", out);
        fputs("    char *step = cct_rt_fmt_format_2(tmpl, a, b);\n", out);
        fputs("    return cct_rt_fmt_format_1(step, c);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_format_4(const char *tmpl, const char *a, const char *b, const char *c, const char *d) {\n", out);
        fputs("    char *step = cct_rt_fmt_format_3(tmpl, a, b, c);\n", out);
        fputs("    return cct_rt_fmt_format_1(step, d);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_repeat_char(long long c, long long n) {\n", out);
        fputs("    if (c < 0 || c > 255) cct_rt_fail(\"fmt repeat_char expects byte range 0..255\");\n", out);
        fputs("    if (n <= 0) return (char*)\"\";\n", out);
        fputs("    size_t count = (size_t)n;\n", out);
        fputs("    char *out_s = (char*)malloc(count + 1);\n", out);
        fputs("    if (!out_s) cct_rt_fail(\"fmt repeat_char allocation failed\");\n", out);
        fputs("    memset(out_s, (int)(unsigned char)c, count);\n", out);
        fputs("    out_s[count] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fmt_table_row(void *parts_ptr, const long long *widths, long long ncols) {\n", out);
        fputs("    if (!parts_ptr) cct_rt_fail(\"fmt table_row received null parts\");\n", out);
        fputs("    if (!widths) cct_rt_fail(\"fmt table_row received null widths\");\n", out);
        fputs("    if (ncols < 0) cct_rt_fail(\"fmt table_row ncols invalido\");\n", out);
        fputs("    cct_rt_fluxus_t *parts = (cct_rt_fluxus_t*)parts_ptr;\n", out);
        fputs("    if (parts->elem_size != (long long)sizeof(char*)) cct_rt_fail(\"fmt table_row requer fluxus de VERBUM\");\n", out);
        fputs("    char **items = (char**)parts->data;\n", out);
        fputs("    cct_rt_builder_t *b = (cct_rt_builder_t*)cct_rt_builder_init();\n", out);
        fputs("    for (long long i = 0; i < ncols; i++) {\n", out);
        fputs("        const char *piece = \"\";\n", out);
        fputs("        if (i < parts->len && items) piece = items[i] ? items[i] : \"\";\n", out);
        fputs("        long long w = widths[i];\n", out);
        fputs("        if (w < 0) w = 0;\n", out);
        fputs("        size_t plen = strlen(piece);\n", out);
        fputs("        size_t copy_n = plen;\n", out);
        fputs("        if ((long long)copy_n > w) copy_n = (size_t)w;\n", out);
        fputs("        if (copy_n > 0) {\n", out);
        fputs("            char *slice = cct_rt_verbum_substring(piece, 0LL, (long long)copy_n);\n", out);
        fputs("            cct_rt_builder_append((void*)b, slice);\n", out);
        fputs("        }\n", out);
        fputs("        for (long long pad = (long long)copy_n; pad < w; pad++) {\n", out);
        fputs("            cct_rt_builder_append_char((void*)b, 32LL);\n", out);
        fputs("        }\n", out);
        fputs("        if (i + 1 < ncols) cct_rt_builder_append((void*)b, \" | \");\n", out);
        fputs("    }\n", out);
        fputs("    char *out_s = cct_rt_builder_to_verbum((void*)b);\n", out);
        fputs("    cct_rt_builder_free((void*)b);\n", out);
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

        fputs("static int cct_rt_parse_has_edge_ws(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    if (n == 0) return 1;\n", out);
        fputs("    if (isspace((unsigned char)src[0])) return 1;\n", out);
        fputs("    if (isspace((unsigned char)src[n - 1])) return 1;\n", out);
        fputs("    return 0;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_try_int(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (cct_rt_parse_has_edge_ws(src)) return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    long long v = strtoll(src, &end, 10);\n", out);
        fputs("    if (errno != 0 || end == src || *end != '\\0') return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    return cct_rt_option_some((const void*)&v, sizeof(long long));\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_try_real(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (cct_rt_parse_has_edge_ws(src)) return cct_rt_option_none(sizeof(double));\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    double v = strtod(src, &end);\n", out);
        fputs("    if (errno != 0 || end == src || *end != '\\0') return cct_rt_option_none(sizeof(double));\n", out);
        fputs("    return cct_rt_option_some((const void*)&v, sizeof(double));\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_try_bool(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (strcmp(src, \"true\") == 0) {\n", out);
        fputs("        long long v = 1LL;\n", out);
        fputs("        return cct_rt_option_some((const void*)&v, sizeof(long long));\n", out);
        fputs("    }\n", out);
        fputs("    if (strcmp(src, \"false\") == 0) {\n", out);
        fputs("        long long v = 0LL;\n", out);
        fputs("        return cct_rt_option_some((const void*)&v, sizeof(long long));\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_parse_int_hex_raw(const char *s, int *ok) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (ok) *ok = 0;\n", out);
        fputs("    if (cct_rt_parse_has_edge_ws(src)) return 0LL;\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    size_t pos = 0;\n", out);
        fputs("    if (src[pos] == '+' || src[pos] == '-') pos += 1;\n", out);
        fputs("    if (pos >= n) return 0LL;\n", out);
        fputs("    if (src[pos] == '0' && (pos + 1) < n && (src[pos + 1] == 'x' || src[pos + 1] == 'X')) pos += 2;\n", out);
        fputs("    if (pos >= n) return 0LL;\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    long long v = strtoll(src, &end, 16);\n", out);
        fputs("    if (errno != 0 || end == src || *end != '\\0') return 0LL;\n", out);
        fputs("    if (ok) *ok = 1;\n", out);
        fputs("    return v;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_try_int_hex(const char *s) {\n", out);
        fputs("    int ok = 0;\n", out);
        fputs("    long long v = cct_rt_parse_int_hex_raw(s, &ok);\n", out);
        fputs("    if (!ok) return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    return cct_rt_option_some((const void*)&v, sizeof(long long));\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_parse_int_hex(const char *s) {\n", out);
        fputs("    int ok = 0;\n", out);
        fputs("    long long v = cct_rt_parse_int_hex_raw(s, &ok);\n", out);
        fputs("    if (!ok) cct_rt_fail(\"parse parse_int_hex valor invalido\");\n", out);
        fputs("    return v;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_try_int_radix(const char *s, long long radix) {\n", out);
        fputs("    if (radix < 2LL || radix > 36LL) return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    if (cct_rt_parse_has_edge_ws(src)) return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    errno = 0;\n", out);
        fputs("    char *end = NULL;\n", out);
        fputs("    long long v = strtoll(src, &end, (int)radix);\n", out);
        fputs("    if (errno != 0 || end == src || *end != '\\0') return cct_rt_option_none(sizeof(long long));\n", out);
        fputs("    return cct_rt_option_some((const void*)&v, sizeof(long long));\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_parse_int_radix(const char *s, long long radix) {\n", out);
        fputs("    if (radix < 2LL || radix > 36LL) cct_rt_fail(\"parse parse_int_radix radix invalida\");\n", out);
        fputs("    void *opt = cct_rt_parse_try_int_radix(s, radix);\n", out);
        fputs("    if (!cct_rt_option_is_some(opt)) cct_rt_fail(\"parse parse_int_radix valor invalido\");\n", out);
        fputs("    long long out_v = *(long long*)cct_rt_option_unwrap_ptr(opt);\n", out);
        fputs("    cct_rt_option_free(opt);\n", out);
        fputs("    return out_v;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_parse_is_int(const char *s) {\n", out);
        fputs("    void *opt = cct_rt_parse_try_int(s);\n", out);
        fputs("    long long ok = cct_rt_option_is_some(opt);\n", out);
        fputs("    cct_rt_option_free(opt);\n", out);
        fputs("    return ok;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_parse_is_real(const char *s) {\n", out);
        fputs("    void *opt = cct_rt_parse_try_real(s);\n", out);
        fputs("    long long ok = cct_rt_option_is_some(opt);\n", out);
        fputs("    cct_rt_option_free(opt);\n", out);
        fputs("    return ok;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_parse_csv_line(const char *s, long long sep) {\n", out);
        fputs("    if (sep < 0LL || sep > 255LL) cct_rt_fail(\"parse parse_csv_line separador fora do intervalo 0..255\");\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    size_t i = 0;\n", out);
        fputs("    unsigned char usep = (unsigned char)sep;\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    for (;;) {\n", out);
        fputs("        if (i == n) {\n", out);
        fputs("            char *empty = (char*)\"\";\n", out);
        fputs("            cct_rt_fluxus_push((void*)out_f, (const void*)&empty);\n", out);
        fputs("            break;\n", out);
        fputs("        }\n", out);
        fputs("        if ((unsigned char)src[i] == 34U) {\n", out);
        fputs("            size_t j = i + 1;\n", out);
        fputs("            while (j < n && (unsigned char)src[j] != 34U) j++;\n", out);
        fputs("            if (j >= n) cct_rt_fail(\"parse parse_csv_line aspas sem fechamento\");\n", out);
        fputs("            char *field = cct_rt_verbum_substring(src, (long long)(i + 1), (long long)j);\n", out);
        fputs("            cct_rt_fluxus_push((void*)out_f, (const void*)&field);\n", out);
        fputs("            i = j + 1;\n", out);
        fputs("            if (i >= n) break;\n", out);
        fputs("            if ((unsigned char)src[i] != usep) cct_rt_fail(\"parse parse_csv_line formato invalido\");\n", out);
        fputs("            i += 1;\n", out);
        fputs("            continue;\n", out);
        fputs("        }\n", out);
        fputs("        size_t j = i;\n", out);
        fputs("        while (j < n && (unsigned char)src[j] != usep) j++;\n", out);
        fputs("        char *field = cct_rt_verbum_substring(src, (long long)i, (long long)j);\n", out);
        fputs("        cct_rt_fluxus_push((void*)out_f, (const void*)&field);\n", out);
        fputs("        if (j >= n) break;\n", out);
        fputs("        i = j + 1;\n", out);
        fputs("    }\n", out);
        fputs("    return (void*)out_f;\n", out);
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

        fputs("static void cct_rt_io_print_real(double x) {\n", out);
        fputs("    printf(\"%g\", x);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_print_char(long long c) {\n", out);
        fputs("    if (c < -1 || c > 255) cct_rt_fail(\"io print_char expects byte range -1..255\");\n", out);
        fputs("    if (c == -1) return;\n", out);
        fputs("    fputc((int)(unsigned char)c, stdout);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_eprint(const char *s) {\n", out);
        fputs("    fputs(s ? s : \"\", stderr);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_eprintln(const char *s) {\n", out);
        fputs("    fputs(s ? s : \"\", stderr);\n", out);
        fputs("    fputc('\\n', stderr);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_eprint_int(long long n) {\n", out);
        fputs("    fprintf(stderr, \"%lld\", n);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_eprint_real(double x) {\n", out);
        fputs("    fprintf(stderr, \"%g\", x);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_flush(void) {\n", out);
        fputs("    fflush(stdout);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_io_flush_err(void) {\n", out);
        fputs("    fflush(stderr);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_io_read_char(void) {\n", out);
        fputs("    int ch = fgetc(stdin);\n", out);
        fputs("    if (ch == EOF) return -1LL;\n", out);
        fputs("    return (long long)(unsigned char)ch;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_io_is_tty(void) {\n", out);
        fputs("    return cct_rt_isatty(cct_rt_fileno(stdout)) ? 1LL : 0LL;\n", out);
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

        fputs("static char *cct_rt_fs_make_atomic_tmp_path(const char *path) {\n", out);
        fputs("    static unsigned long counter = 0;\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    size_t len = strlen(p);\n", out);
        fputs("    char *tmp = (char*)malloc(len + 64U);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"fs write_all tmp allocation failed\");\n", out);
        fputs("    snprintf(tmp, len + 64U, \"%s.tmp.%ld.%lu\", p, (long)getpid(), ++counter);\n", out);
        fputs("    return tmp;\n", out);
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
        fputs("    const char *dst = path ? path : \"\";\n", out);
        fputs("    char *tmp_path = cct_rt_fs_make_atomic_tmp_path(dst);\n", out);
        fputs("    FILE *f = cct_rt_fs_open_or_fail(tmp_path, \"wb\", \"fs write_all open failed\");\n", out);
        fputs("    const char *src = content ? content : \"\";\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    if (len > 0 && fwrite(src, 1, len, f) != len) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        unlink(tmp_path);\n", out);
        fputs("        free(tmp_path);\n", out);
        fputs("        cct_rt_fail(\"fs write_all write failed\");\n", out);
        fputs("    }\n", out);
        fputs("    if (fflush(f) != 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        unlink(tmp_path);\n", out);
        fputs("        free(tmp_path);\n", out);
        fputs("        cct_rt_fail(\"fs write_all flush failed\");\n", out);
        fputs("    }\n", out);
        fputs("#ifndef _WIN32\n", out);
        fputs("    if (fsync(fileno(f)) != 0) {\n", out);
        fputs("        fclose(f);\n", out);
        fputs("        unlink(tmp_path);\n", out);
        fputs("        free(tmp_path);\n", out);
        fputs("        cct_rt_fail(\"fs write_all sync failed\");\n", out);
        fputs("    }\n", out);
        fputs("#endif\n", out);
        fputs("    if (fclose(f) != 0) {\n", out);
        fputs("        unlink(tmp_path);\n", out);
        fputs("        free(tmp_path);\n", out);
        fputs("        cct_rt_fail(\"fs write_all close failed\");\n", out);
        fputs("    }\n", out);
        fputs("    if (rename(tmp_path, dst) != 0) {\n", out);
        fputs("        unlink(tmp_path);\n", out);
        fputs("        free(tmp_path);\n", out);
        fputs("        cct_rt_fail(\"fs write_all rename failed\");\n", out);
        fputs("    }\n", out);
        fputs("    free(tmp_path);\n", out);
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

        fputs("static void cct_rt_fs_fail1(const char *fmt, const char *a) {\n", out);
        fputs("    char msg[1024];\n", out);
        fputs("    const char *sa = a ? a : \"\";\n", out);
        fputs("    snprintf(msg, sizeof(msg), fmt ? fmt : \"fs error\", sa);\n", out);
        fputs("    cct_rt_fail(msg);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_fail2(const char *fmt, const char *a, const char *b) {\n", out);
        fputs("    char msg[1024];\n", out);
        fputs("    const char *sa = a ? a : \"\";\n", out);
        fputs("    const char *sb = b ? b : \"\";\n", out);
        fputs("    snprintf(msg, sizeof(msg), fmt ? fmt : \"fs error\", sa, sb);\n", out);
        fputs("    cct_rt_fail(msg);\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_fs_is_dir(const char *path) {\n", out);
        fputs("    struct stat st;\n", out);
        fputs("    if (stat(path ? path : \"\", &st) != 0) return 0;\n", out);
        fputs("    return S_ISDIR(st.st_mode) ? 1 : 0;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_is_file(const char *path) {\n", out);
        fputs("    struct stat st;\n", out);
        fputs("    if (stat(path ? path : \"\", &st) != 0) return 0LL;\n", out);
        fputs("    return S_ISREG(st.st_mode) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_is_symlink(const char *path) {\n", out);
        fputs("    struct stat st;\n", out);
        fputs("    if (lstat(path ? path : \"\", &st) != 0) return 0LL;\n", out);
        fputs("    return S_ISLNK(st.st_mode) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_is_readable(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    return (access(p, R_OK) == 0) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_is_writable(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    return (access(p, W_OK) == 0) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_modified_time(const char *path) {\n", out);
        fputs("    struct stat st;\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (stat(p, &st) != 0) cct_rt_fs_fail1(\"fs modified_time falhou: %s\", p);\n", out);
        fputs("    return (long long)st.st_mtime;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_fs_same_file(const char *a, const char *b) {\n", out);
        fputs("    struct stat sa;\n", out);
        fputs("    struct stat sb;\n", out);
        fputs("    const char *pa = a ? a : \"\";\n", out);
        fputs("    const char *pb = b ? b : \"\";\n", out);
        fputs("    if (stat(pa, &sa) != 0) return 0LL;\n", out);
        fputs("    if (stat(pb, &sb) != 0) return 0LL;\n", out);
        fputs("    return (sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino) ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_mkdir(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (cct_rt_mkdir(p) == 0) return;\n", out);
        fputs("    if (errno == EEXIST) cct_rt_fs_fail1(\"fs mkdir ja existe: %s\", p);\n", out);
        fputs("    if (errno == ENOENT) cct_rt_fs_fail1(\"fs mkdir pai nao existe: %s\", p);\n", out);
        fputs("    cct_rt_fs_fail1(\"fs mkdir falhou: %s\", p);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_mkdir_all(const char *path) {\n", out);
        fputs("    const char *src = path ? path : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    if (n == 0) return;\n", out);
        fputs("    char *tmp = (char*)malloc(n + 1);\n", out);
        fputs("    if (!tmp) cct_rt_fail(\"fs mkdir_all allocation failed\");\n", out);
        fputs("    memcpy(tmp, src, n + 1);\n", out);
        fputs("    for (size_t i = 1; i < n; i++) {\n", out);
        fputs("        if (tmp[i] == '/' || tmp[i] == '\\\\') {\n", out);
        fputs("            char hold = tmp[i];\n", out);
        fputs("            tmp[i] = '\\0';\n", out);
        fputs("            if (strlen(tmp) > 0) {\n", out);
        fputs("                if (cct_rt_mkdir(tmp) != 0 && errno != EEXIST) {\n", out);
        fputs("                    free(tmp);\n", out);
        fputs("                    cct_rt_fs_fail1(\"fs mkdir_all falhou: %s\", src);\n", out);
        fputs("                }\n", out);
        fputs("            }\n", out);
        fputs("            tmp[i] = hold;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    if (cct_rt_mkdir(tmp) != 0) {\n", out);
        fputs("        if (errno != EEXIST || !cct_rt_fs_is_dir(tmp)) {\n", out);
        fputs("            free(tmp);\n", out);
        fputs("            cct_rt_fs_fail1(\"fs mkdir_all falhou: %s\", src);\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    free(tmp);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_delete_file(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (cct_rt_fs_is_dir(p)) cct_rt_fs_fail1(\"fs delete_file e um diretorio: %s\", p);\n", out);
        fputs("    if (remove(p) == 0) return;\n", out);
        fputs("    if (errno == ENOENT) cct_rt_fs_fail1(\"fs delete_file nao encontrado: %s\", p);\n", out);
        fputs("    cct_rt_fs_fail1(\"fs delete_file falhou: %s\", p);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_delete_dir(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (rmdir(p) == 0) return;\n", out);
        fputs("    if (errno == ENOTEMPTY || errno == EEXIST) cct_rt_fs_fail1(\"fs delete_dir nao vazio: %s\", p);\n", out);
        fputs("    cct_rt_fs_fail1(\"fs delete_dir falhou: %s\", p);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_rename(const char *de, const char *para) {\n", out);
        fputs("    const char *src = de ? de : \"\";\n", out);
        fputs("    const char *dst = para ? para : \"\";\n", out);
        fputs("    if (rename(src, dst) == 0) return;\n", out);
        fputs("    if (errno == EXDEV) cct_rt_fail(\"fs rename cross-filesystem nao suportado\");\n", out);
        fputs("    cct_rt_fs_fail2(\"fs rename falhou: %s -> %s\", src, dst);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_copy(const char *de, const char *para) {\n", out);
        fputs("    const char *src = de ? de : \"\";\n", out);
        fputs("    const char *dst = para ? para : \"\";\n", out);
        fputs("    FILE *in = fopen(src, \"rb\");\n", out);
        fputs("    if (!in) cct_rt_fs_fail1(\"fs copy origem invalida: %s\", src);\n", out);
        fputs("    FILE *out_f = fopen(dst, \"wb\");\n", out);
        fputs("    if (!out_f) {\n", out);
        fputs("        fclose(in);\n", out);
        fputs("        cct_rt_fs_fail1(\"fs copy destino invalido: %s\", dst);\n", out);
        fputs("    }\n", out);
        fputs("    char buf[8192];\n", out);
        fputs("    for (;;) {\n", out);
        fputs("        size_t nread = fread(buf, 1, sizeof(buf), in);\n", out);
        fputs("        if (nread > 0 && fwrite(buf, 1, nread, out_f) != nread) {\n", out);
        fputs("            fclose(in);\n", out);
        fputs("            fclose(out_f);\n", out);
        fputs("            cct_rt_fs_fail1(\"fs copy write falhou: %s\", dst);\n", out);
        fputs("        }\n", out);
        fputs("        if (nread < sizeof(buf)) {\n", out);
        fputs("            if (ferror(in)) {\n", out);
        fputs("                fclose(in);\n", out);
        fputs("                fclose(out_f);\n", out);
        fputs("                cct_rt_fs_fail1(\"fs copy read falhou: %s\", src);\n", out);
        fputs("            }\n", out);
        fputs("            break;\n", out);
        fputs("        }\n", out);
        fputs("    }\n", out);
        fputs("    fclose(in);\n", out);
        fputs("    fclose(out_f);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_move(const char *de, const char *para) {\n", out);
        fputs("    const char *src = de ? de : \"\";\n", out);
        fputs("    const char *dst = para ? para : \"\";\n", out);
        fputs("    if (rename(src, dst) == 0) return;\n", out);
        fputs("    if (errno == EXDEV) {\n", out);
        fputs("        cct_rt_fs_copy(src, dst);\n", out);
        fputs("        cct_rt_fs_delete_file(src);\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_fs_fail2(\"fs move falhou: %s -> %s\", src, dst);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_chmod(const char *path, long long mode) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (mode < 0) cct_rt_fail(\"fs chmod mode invalido\");\n", out);
        fputs("    if (chmod(p, (mode_t)mode) != 0) cct_rt_fs_fail1(\"fs chmod falhou: %s\", p);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_fs_list_dir(const char *path) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    DIR *dir = opendir(p);\n", out);
        fputs("    if (!dir) cct_rt_fs_fail1(\"fs list_dir falhou: %s\", p);\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    struct dirent *ent = NULL;\n", out);
        fputs("    while ((ent = readdir(dir)) != NULL) {\n", out);
        fputs("        const char *name = ent->d_name;\n", out);
        fputs("        if (strcmp(name, \".\") == 0 || strcmp(name, \"..\") == 0) continue;\n", out);
        fputs("        size_t n = strlen(name);\n", out);
        fputs("        char *copy = (char*)malloc(n + 1);\n", out);
        fputs("        if (!copy) {\n", out);
        fputs("            closedir(dir);\n", out);
        fputs("            cct_rt_fail(\"fs list_dir allocation failed\");\n", out);
        fputs("        }\n", out);
        fputs("        memcpy(copy, name, n + 1);\n", out);
        fputs("        cct_rt_fluxus_push((void*)out_f, (const void*)&copy);\n", out);
        fputs("    }\n", out);
        fputs("    closedir(dir);\n", out);
        fputs("    return (void*)out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fs_tmp_template(const char *prefix) {\n", out);
        fputs("    const char *base = getenv(\"TMPDIR\");\n", out);
        fputs("    const char *sp = prefix ? prefix : \"cct_tmp_\";\n", out);
        fputs("    if (!base || !*base) base = \".\";\n", out);
        fputs("    size_t lb = strlen(base);\n", out);
        fputs("    size_t lp = strlen(sp);\n", out);
        fputs("    int need_sep = (lb > 0 && base[lb - 1] != '/' && base[lb - 1] != '\\\\');\n", out);
        fputs("    size_t total = lb + (need_sep ? 1U : 0U) + lp + 6U + 1U;\n", out);
        fputs("    char *tmpl = (char*)malloc(total);\n", out);
        fputs("    if (!tmpl) cct_rt_fail(\"fs temp allocation failed\");\n", out);
        fputs("    snprintf(tmpl, total, \"%s%s%sXXXXXX\", base, need_sep ? \"/\" : \"\", sp);\n", out);
        fputs("    return tmpl;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fs_create_temp_file(void) {\n", out);
        fputs("    char *tmpl = cct_rt_fs_tmp_template(\"cct_tmp_file_\");\n", out);
        fputs("    int fd = mkstemp(tmpl);\n", out);
        fputs("    if (fd < 0) {\n", out);
        fputs("        free(tmpl);\n", out);
        fputs("        cct_rt_fail(\"fs create_temp_file falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    close(fd);\n", out);
        fputs("    return tmpl;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_fs_create_temp_dir(void) {\n", out);
        fputs("    char *tmpl = cct_rt_fs_tmp_template(\"cct_tmp_dir_\");\n", out);
        fputs("    if (!mkdtemp(tmpl)) {\n", out);
        fputs("        free(tmpl);\n", out);
        fputs("        cct_rt_fail(\"fs create_temp_dir falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    return tmpl;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_truncate(const char *path, long long size) {\n", out);
        fputs("    const char *p = path ? path : \"\";\n", out);
        fputs("    if (size < 0) cct_rt_fail(\"fs truncate tamanho invalido\");\n", out);
        fputs("    if (truncate(p, (off_t)size) != 0) cct_rt_fs_fail1(\"fs truncate falhou: %s\", p);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_fs_symlink(const char *alvo, const char *link) {\n", out);
        fputs("    const char *src = alvo ? alvo : \"\";\n", out);
        fputs("    const char *dst = link ? link : \"\";\n", out);
        fputs("    if (symlink(src, dst) != 0) cct_rt_fs_fail2(\"fs symlink falhou: %s -> %s\", src, dst);\n", out);
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

        fputs("static long long cct_rt_path_is_absolute(const char *p) {\n", out);
        fputs("    const char *src = p ? p : \"\";\n", out);
        fputs("    return (src[0] == '/') ? 1LL : 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_path_split_free(void *parts_ptr) {\n", out);
        fputs("    cct_rt_fluxus_t *parts = (cct_rt_fluxus_t*)parts_ptr;\n", out);
        fputs("    if (!parts) return;\n", out);
        fputs("    long long n = cct_rt_fluxus_len(parts);\n", out);
        fputs("    for (long long i = 0; i < n; i++) {\n", out);
        fputs("        char **pp = (char**)cct_rt_fluxus_get(parts, i);\n", out);
        fputs("        if (pp && *pp) free(*pp);\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_fluxus_destroy(parts);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_normalize(const char *p) {\n", out);
        fputs("    const char *src = p ? p : \"\";\n", out);
        fputs("    int is_abs = cct_rt_path_is_absolute(src) ? 1 : 0;\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    cct_rt_fluxus_t *stack = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    size_t i = 0;\n", out);
        fputs("    while (i < len) {\n", out);
        fputs("        while (i < len && cct_rt_path_is_sep(src[i])) i++;\n", out);
        fputs("        size_t start = i;\n", out);
        fputs("        while (i < len && !cct_rt_path_is_sep(src[i])) i++;\n", out);
        fputs("        size_t end = i;\n", out);
        fputs("        if (end <= start) continue;\n", out);
        fputs("        char *part = cct_rt_path_dup_normalized(src, start, end);\n", out);
        fputs("        if (strcmp(part, \".\") == 0) {\n", out);
        fputs("            free(part);\n", out);
        fputs("            continue;\n", out);
        fputs("        }\n", out);
        fputs("        if (strcmp(part, \"..\") == 0) {\n", out);
        fputs("            long long n = cct_rt_fluxus_len(stack);\n", out);
        fputs("            if (n > 0) {\n", out);
        fputs("                char **top = (char**)cct_rt_fluxus_get(stack, n - 1);\n", out);
        fputs("                if (top && *top && strcmp(*top, \"..\") != 0) {\n", out);
        fputs("                    free(*top);\n", out);
        fputs("                    cct_rt_fluxus_remove(stack, n - 1);\n", out);
        fputs("                    free(part);\n", out);
        fputs("                    continue;\n", out);
        fputs("                }\n", out);
        fputs("            }\n", out);
        fputs("            if (is_abs) {\n", out);
        fputs("                free(part);\n", out);
        fputs("                continue;\n", out);
        fputs("            }\n", out);
        fputs("        }\n", out);
        fputs("        cct_rt_fluxus_push(stack, (const void*)&part);\n", out);
        fputs("    }\n", out);
        fputs("    long long n = cct_rt_fluxus_len(stack);\n", out);
        fputs("    if (n == 0) {\n", out);
        fputs("        cct_rt_fluxus_destroy(stack);\n", out);
        fputs("        if (is_abs) return cct_rt_path_dup_normalized(\"/\", 0, 1);\n", out);
        fputs("        return cct_rt_path_dup_normalized(\".\", 0, 1);\n", out);
        fputs("    }\n", out);
        fputs("    char *joined = cct_rt_verbum_join(stack, \"/\");\n", out);
        fputs("    for (long long k = 0; k < n; k++) {\n", out);
        fputs("        char **pp = (char**)cct_rt_fluxus_get(stack, k);\n", out);
        fputs("        if (pp && *pp) free(*pp);\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_fluxus_destroy(stack);\n", out);
        fputs("    if (!is_abs) return joined;\n", out);
        fputs("    size_t jl = strlen(joined);\n", out);
        fputs("    char *out_s = (char*)malloc(jl + 2);\n", out);
        fputs("    if (!out_s) {\n", out);
        fputs("        free(joined);\n", out);
        fputs("        cct_rt_fail(\"path normalize allocation failed\");\n", out);
        fputs("    }\n", out);
        fputs("    out_s[0] = '/';\n", out);
        fputs("    memcpy(out_s + 1, joined, jl + 1);\n", out);
        fputs("    free(joined);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_without_ext(const char *p) {\n", out);
        fputs("    const char *src = p ? p : \"\";\n", out);
        fputs("    size_t len = strlen(src);\n", out);
        fputs("    size_t last_dot = (size_t)-1;\n", out);
        fputs("    size_t last_sep = (size_t)-1;\n", out);
        fputs("    for (size_t i = 0; i < len; i++) {\n", out);
        fputs("        if (src[i] == '.') last_dot = i;\n", out);
        fputs("        if (cct_rt_path_is_sep(src[i])) last_sep = i;\n", out);
        fputs("    }\n", out);
        fputs("    if (last_dot == (size_t)-1) return cct_rt_path_dup_normalized(src, 0, len);\n", out);
        fputs("    if (last_sep == (size_t)-1) {\n", out);
        fputs("        if (last_dot == 0) return cct_rt_path_dup_normalized(src, 0, len);\n", out);
        fputs("    } else if (last_dot <= last_sep + 1) {\n", out);
        fputs("        return cct_rt_path_dup_normalized(src, 0, len);\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_path_dup_normalized(src, 0, last_dot);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_with_ext(const char *p, const char *ext) {\n", out);
        fputs("    char *core = cct_rt_path_without_ext(p);\n", out);
        fputs("    const char *sx = ext ? ext : \"\";\n", out);
        fputs("    if (*sx == '\\0') return core;\n", out);
        fputs("    if (sx[0] == '.') {\n", out);
        fputs("        char *out_s = cct_rt_verbum_concat(core, sx);\n", out);
        fputs("        free(core);\n", out);
        fputs("        return out_s;\n", out);
        fputs("    }\n", out);
        fputs("    char *dot_ext = cct_rt_verbum_concat(\".\", sx);\n", out);
        fputs("    char *out_s = cct_rt_verbum_concat(core, dot_ext);\n", out);
        fputs("    free(dot_ext);\n", out);
        fputs("    free(core);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_stem(const char *p) {\n", out);
        fputs("    char *base = cct_rt_path_basename(p);\n", out);
        fputs("    char *st = cct_rt_path_without_ext(base);\n", out);
        fputs("    free(base);\n", out);
        fputs("    return st;\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_path_split(const char *p) {\n", out);
        fputs("    char *norm = cct_rt_path_normalize(p);\n", out);
        fputs("    int is_abs = cct_rt_path_is_absolute(norm) ? 1 : 0;\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    if (is_abs) {\n", out);
        fputs("        char *root = cct_rt_path_dup_normalized(\"/\", 0, 1);\n", out);
        fputs("        cct_rt_fluxus_push(out_f, (const void*)&root);\n", out);
        fputs("    }\n", out);
        fputs("    size_t len = strlen(norm);\n", out);
        fputs("    size_t i = is_abs ? 1U : 0U;\n", out);
        fputs("    size_t start = i;\n", out);
        fputs("    while (i <= len) {\n", out);
        fputs("        if (i == len || norm[i] == '/') {\n", out);
        fputs("            if (i > start) {\n", out);
        fputs("                char *part = cct_rt_path_dup_normalized(norm, start, i);\n", out);
        fputs("                cct_rt_fluxus_push(out_f, (const void*)&part);\n", out);
        fputs("            }\n", out);
        fputs("            start = i + 1;\n", out);
        fputs("        }\n", out);
        fputs("        i++;\n", out);
        fputs("    }\n", out);
        fputs("    if (!is_abs && cct_rt_fluxus_len(out_f) == 0) {\n", out);
        fputs("        char *dot = cct_rt_path_dup_normalized(\".\", 0, 1);\n", out);
        fputs("        cct_rt_fluxus_push(out_f, (const void*)&dot);\n", out);
        fputs("    }\n", out);
        fputs("    free(norm);\n", out);
        fputs("    return (void*)out_f;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_resolve(const char *p) {\n", out);
        fputs("    const char *src = (p && *p) ? p : \".\";\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    char *resolved = _fullpath(NULL, src, 0);\n", out);
        fputs("    if (resolved) {\n", out);
        fputs("        for (char *q = resolved; *q; ++q) if (*q == '\\\\') *q = '/';\n", out);
        fputs("        return resolved;\n", out);
        fputs("    }\n", out);
        fputs("#else\n", out);
        fputs("    char *resolved = realpath(src, NULL);\n", out);
        fputs("    if (resolved) return resolved;\n", out);
        fputs("#endif\n", out);
        fputs("    char *norm = cct_rt_path_normalize(src);\n", out);
        fputs("    if (cct_rt_path_is_absolute(norm)) return norm;\n", out);
        fputs("    char *cwd = cct_rt_getcwd(NULL, 0);\n", out);
        fputs("    if (!cwd) {\n", out);
        fputs("        free(norm);\n", out);
        fputs("        cct_rt_fail(\"path resolve cwd falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    char *joined = cct_rt_path_join(cwd, norm);\n", out);
        fputs("    free(cwd);\n", out);
        fputs("    free(norm);\n", out);
        fputs("    return joined;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_relative_to(const char *path, const char *base) {\n", out);
        fputs("    char *pn = cct_rt_path_normalize(path);\n", out);
        fputs("    char *bn = cct_rt_path_normalize(base);\n", out);
        fputs("    int pa = cct_rt_path_is_absolute(pn) ? 1 : 0;\n", out);
        fputs("    int ba = cct_rt_path_is_absolute(bn) ? 1 : 0;\n", out);
        fputs("    if (pa != ba) {\n", out);
        fputs("        free(pn);\n", out);
        fputs("        free(bn);\n", out);
        fputs("        cct_rt_fail(\"path relative_to requer ambos absolutos ou ambos relativos\");\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_fluxus_t *pp = (cct_rt_fluxus_t*)cct_rt_path_split(pn);\n", out);
        fputs("    cct_rt_fluxus_t *bp = (cct_rt_fluxus_t*)cct_rt_path_split(bn);\n", out);
        fputs("    long long pi = pa ? 1LL : 0LL;\n", out);
        fputs("    long long bi = ba ? 1LL : 0LL;\n", out);
        fputs("    long long plen = cct_rt_fluxus_len(pp);\n", out);
        fputs("    long long blen = cct_rt_fluxus_len(bp);\n", out);
        fputs("    while (pi < plen && bi < blen) {\n", out);
        fputs("        char **pseg = (char**)cct_rt_fluxus_get(pp, pi);\n", out);
        fputs("        char **bseg = (char**)cct_rt_fluxus_get(bp, bi);\n", out);
        fputs("        const char *ps = (pseg && *pseg) ? *pseg : \"\";\n", out);
        fputs("        const char *bs = (bseg && *bseg) ? *bseg : \"\";\n", out);
        fputs("        if (strcmp(ps, bs) != 0) break;\n", out);
        fputs("        pi++;\n", out);
        fputs("        bi++;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_fluxus_t *out_f = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
        fputs("    const char *up = \"..\";\n", out);
        fputs("    for (long long i = bi; i < blen; i++) cct_rt_fluxus_push(out_f, (const void*)&up);\n", out);
        fputs("    for (long long i = pi; i < plen; i++) {\n", out);
        fputs("        char **pseg = (char**)cct_rt_fluxus_get(pp, i);\n", out);
        fputs("        const char *ps = (pseg && *pseg) ? *pseg : \"\";\n", out);
        fputs("        if (*ps) cct_rt_fluxus_push(out_f, (const void*)pseg);\n", out);
        fputs("    }\n", out);
        fputs("    char *out_s = NULL;\n", out);
        fputs("    if (cct_rt_fluxus_len(out_f) == 0) out_s = cct_rt_path_dup_normalized(\".\", 0, 1);\n", out);
        fputs("    else out_s = cct_rt_verbum_join(out_f, \"/\");\n", out);
        fputs("    cct_rt_fluxus_destroy(out_f);\n", out);
        fputs("    cct_rt_path_split_free(pp);\n", out);
        fputs("    cct_rt_path_split_free(bp);\n", out);
        fputs("    free(pn);\n", out);
        fputs("    free(bn);\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_home_dir(void) {\n", out);
        fputs("    const char *home = getenv(\"HOME\");\n", out);
        fputs("    if (home && *home) return cct_rt_path_dup_normalized(home, 0, strlen(home));\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    home = getenv(\"USERPROFILE\");\n", out);
        fputs("    if (home && *home) return cct_rt_path_dup_normalized(home, 0, strlen(home));\n", out);
        fputs("#else\n", out);
        fputs("    struct passwd *pw = getpwuid(getuid());\n", out);
        fputs("    if (pw && pw->pw_dir && *pw->pw_dir) return cct_rt_path_dup_normalized(pw->pw_dir, 0, strlen(pw->pw_dir));\n", out);
        fputs("#endif\n", out);
        fputs("    cct_rt_fail(\"path home_dir nao disponivel\");\n", out);
        fputs("    return cct_rt_path_dup_normalized(\"\", 0, 0);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_path_temp_dir(void) {\n", out);
        fputs("    const char *tmp = getenv(\"TMPDIR\");\n", out);
        fputs("    if (tmp && *tmp) return cct_rt_path_dup_normalized(tmp, 0, strlen(tmp));\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    tmp = getenv(\"TEMP\");\n", out);
        fputs("    if (tmp && *tmp) return cct_rt_path_dup_normalized(tmp, 0, strlen(tmp));\n", out);
        fputs("    tmp = getenv(\"TMP\");\n", out);
        fputs("    if (tmp && *tmp) return cct_rt_path_dup_normalized(tmp, 0, strlen(tmp));\n", out);
        fputs("#endif\n", out);
        fputs("    return cct_rt_path_dup_normalized(\"/tmp\", 0, 4);\n", out);
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

        fputs("static char *cct_rt_random_verbum(long long len) {\n", out);
        fputs("    static const char alphabet[] = \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\";\n", out);
        fputs("    if (len < 0) cct_rt_fail(\"random_verbum len < 0\");\n", out);
        fputs("    size_t n = (size_t)len;\n", out);
        fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1U);\n", out);
        fputs("    size_t alpha_n = sizeof(alphabet) - 1U;\n", out);
        fputs("    for (size_t i = 0; i < n; i++) {\n", out);
        fputs("        out_s[i] = alphabet[(size_t)(rand() % (int)alpha_n)];\n", out);
        fputs("    }\n", out);
        fputs("    out_s[n] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_random_verbum_from(long long len, const char *alphabet) {\n", out);
        fputs("    if (len < 0) cct_rt_fail(\"random_verbum_from len < 0\");\n", out);
        fputs("    if (!alphabet || !*alphabet) cct_rt_fail(\"random_verbum_from alphabet empty\");\n", out);
        fputs("    size_t n = (size_t)len;\n", out);
        fputs("    size_t alpha_n = strlen(alphabet);\n", out);
        fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1U);\n", out);
        fputs("    for (size_t i = 0; i < n; i++) {\n", out);
        fputs("        out_s[i] = alphabet[(size_t)(rand() % (int)alpha_n)];\n", out);
        fputs("    }\n", out);
        fputs("    out_s[n] = '\\0';\n", out);
        fputs("    return out_s;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_random_shuffle_int(long long *arr, long long n) {\n", out);
        fputs("    if (n < 0) cct_rt_fail(\"random_shuffle_int n < 0\");\n", out);
        fputs("    if (n > 0 && !arr) cct_rt_fail(\"random_shuffle_int null arr\");\n", out);
        fputs("    for (long long i = n - 1; i > 0; i--) {\n", out);
        fputs("        long long j = (long long)(rand() % (int)(i + 1));\n", out);
        fputs("        long long tmp = arr[i];\n", out);
        fputs("        arr[i] = arr[j];\n", out);
        fputs("        arr[j] = tmp;\n", out);
        fputs("    }\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_random_bytes(long long len) {\n", out);
        fputs("    if (len < 0) cct_rt_fail(\"random_bytes len < 0\");\n", out);
        fputs("    size_t n = (size_t)len;\n", out);
        fputs("    unsigned char *buf = (unsigned char*)cct_rt_alloc_or_fail(n == 0U ? 1U : n);\n", out);
        fputs("    for (size_t i = 0; i < n; i++) {\n", out);
        fputs("        buf[i] = (unsigned char)(rand() & 0xFF);\n", out);
        fputs("    }\n", out);
        fputs("    return (void*)buf;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_hash_helpers) {
        fputs("static long long cct_rt_hash_fnv1a_bytes(const void *data, long long len) {\n", out);
        fputs("    if (len < 0) cct_rt_fail(\"hash_fnv1a_bytes len < 0\");\n", out);
        fputs("    if (len > 0 && !data) cct_rt_fail(\"hash_fnv1a_bytes null data\");\n", out);
        fputs("    const unsigned char *p = (const unsigned char*)data;\n", out);
        fputs("    uint64_t h = 14695981039346656037ULL;\n", out);
        fputs("    for (long long i = 0; i < len; i++) {\n", out);
        fputs("        h ^= (uint64_t)p[(size_t)i];\n", out);
        fputs("        h *= 1099511628211ULL;\n", out);
        fputs("    }\n", out);
        fputs("    return (long long)h;\n", out);
        fputs("}\n\n", out);

        fputs("static uint32_t cct_rt_hash_crc32_table[256];\n", out);
        fputs("static int cct_rt_hash_crc32_table_ready = 0;\n\n", out);

        fputs("static void cct_rt_hash_crc32_init(void) {\n", out);
        fputs("    if (cct_rt_hash_crc32_table_ready) return;\n", out);
        fputs("    for (uint32_t i = 0; i < 256U; i++) {\n", out);
        fputs("        uint32_t c = i;\n", out);
        fputs("        for (int j = 0; j < 8; j++) {\n", out);
        fputs("            if (c & 1U) c = (c >> 1) ^ 0xEDB88320U;\n", out);
        fputs("            else c >>= 1;\n", out);
        fputs("        }\n", out);
        fputs("        cct_rt_hash_crc32_table[i] = c;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_hash_crc32_table_ready = 1;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_hash_crc32(const char *s) {\n", out);
        fputs("    cct_rt_hash_crc32_init();\n", out);
        fputs("    const unsigned char *p = (const unsigned char*)(s ? s : \"\");\n", out);
        fputs("    uint32_t crc = 0xFFFFFFFFU;\n", out);
        fputs("    while (*p) {\n", out);
        fputs("        uint32_t idx = (crc ^ (uint32_t)(*p++)) & 0xFFU;\n", out);
        fputs("        crc = (crc >> 8) ^ cct_rt_hash_crc32_table[idx];\n", out);
        fputs("    }\n", out);
        fputs("    return (long long)(crc ^ 0xFFFFFFFFU);\n", out);
        fputs("}\n\n", out);

        fputs("static uint32_t cct_rt_hash_rotl32(uint32_t x, int r) {\n", out);
        fputs("    return (x << r) | (x >> (32 - r));\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_hash_murmur3(const char *s, long long seed) {\n", out);
        fputs("    const uint8_t *data = (const uint8_t*)(s ? s : \"\");\n", out);
        fputs("    size_t len = strlen((const char*)data);\n", out);
        fputs("    uint32_t h1 = (uint32_t)(seed & 0xFFFFFFFFULL);\n", out);
        fputs("    const uint32_t c1 = 0xcc9e2d51U;\n", out);
        fputs("    const uint32_t c2 = 0x1b873593U;\n", out);
        fputs("    size_t nblocks = len / 4U;\n", out);
        fputs("    for (size_t i = 0; i < nblocks; i++) {\n", out);
        fputs("        const uint8_t *blk = data + (i * 4U);\n", out);
        fputs("        uint32_t k1 = (uint32_t)blk[0] |\n", out);
        fputs("                      ((uint32_t)blk[1] << 8) |\n", out);
        fputs("                      ((uint32_t)blk[2] << 16) |\n", out);
        fputs("                      ((uint32_t)blk[3] << 24);\n", out);
        fputs("        k1 *= c1;\n", out);
        fputs("        k1 = cct_rt_hash_rotl32(k1, 15);\n", out);
        fputs("        k1 *= c2;\n", out);
        fputs("        h1 ^= k1;\n", out);
        fputs("        h1 = cct_rt_hash_rotl32(h1, 13);\n", out);
        fputs("        h1 = h1 * 5U + 0xe6546b64U;\n", out);
        fputs("    }\n", out);
        fputs("    const uint8_t *tail = data + (nblocks * 4U);\n", out);
        fputs("    uint32_t k1 = 0U;\n", out);
        fputs("    switch (len & 3U) {\n", out);
        fputs("        case 3: k1 ^= (uint32_t)tail[2] << 16; /* fallthrough */\n", out);
        fputs("        case 2: k1 ^= (uint32_t)tail[1] << 8;  /* fallthrough */\n", out);
        fputs("        case 1:\n", out);
        fputs("            k1 ^= (uint32_t)tail[0];\n", out);
        fputs("            k1 *= c1;\n", out);
        fputs("            k1 = cct_rt_hash_rotl32(k1, 15);\n", out);
        fputs("            k1 *= c2;\n", out);
        fputs("            h1 ^= k1;\n", out);
        fputs("            break;\n", out);
        fputs("        default:\n", out);
        fputs("            break;\n", out);
        fputs("    }\n", out);
        fputs("    h1 ^= (uint32_t)len;\n", out);
        fputs("    h1 ^= h1 >> 16;\n", out);
        fputs("    h1 *= 0x85ebca6bU;\n", out);
        fputs("    h1 ^= h1 >> 13;\n", out);
        fputs("    h1 *= 0xc2b2ae35U;\n", out);
        fputs("    h1 ^= h1 >> 16;\n", out);
        fputs("    return (long long)h1;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_process_helpers) {
        fputs("static long long cct_rt_process_decode_wait_status(int status) {\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    return (status == -1) ? -1LL : (long long)status;\n", out);
        fputs("#else\n", out);
        fputs("    if (status == -1) return -1LL;\n", out);
        fputs("    if (WIFEXITED(status)) return (long long)WEXITSTATUS(status);\n", out);
        fputs("    if (WIFSIGNALED(status)) return 128LL + (long long)WTERMSIG(status);\n", out);
        fputs("    return -1LL;\n", out);
        fputs("#endif\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_process_read_stream(FILE *fp, const char *alloc_msg) {\n", out);
        fputs("    size_t cap = 256U;\n", out);
        fputs("    size_t len = 0U;\n", out);
        fputs("    char *buf = (char*)malloc(cap);\n", out);
        fputs("    if (!buf) cct_rt_fail((alloc_msg && *alloc_msg) ? alloc_msg : \"process allocation failed\");\n", out);
        fputs("    int ch = 0;\n", out);
        fputs("    while ((ch = fgetc(fp)) != EOF) {\n", out);
        fputs("        if (len + 1U >= cap) {\n", out);
        fputs("            size_t new_cap = cap * 2U;\n", out);
        fputs("            char *next = (char*)realloc(buf, new_cap);\n", out);
        fputs("            if (!next) {\n", out);
        fputs("                free(buf);\n", out);
        fputs("                cct_rt_fail((alloc_msg && *alloc_msg) ? alloc_msg : \"process realloc failed\");\n", out);
        fputs("            }\n", out);
        fputs("            buf = next;\n", out);
        fputs("            cap = new_cap;\n", out);
        fputs("        }\n", out);
        fputs("        buf[len++] = (char)ch;\n", out);
        fputs("    }\n", out);
        fputs("    buf[len] = '\\0';\n", out);
        fputs("    return buf;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_process_run(const char *cmd) {\n", out);
        fputs("    const char *src = cmd ? cmd : \"\";\n", out);
        fputs("    int status = system(src);\n", out);
        fputs("    if (status == -1) cct_rt_fail(\"process run falhou\");\n", out);
        fputs("    return cct_rt_process_decode_wait_status(status);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_process_run_capture(const char *cmd) {\n", out);
        fputs("    const char *src = cmd ? cmd : \"\";\n", out);
        fputs("    FILE *fp = popen(src, \"r\");\n", out);
        fputs("    if (!fp) cct_rt_fail(\"process run_capture popen falhou\");\n", out);
        fputs("    char *buf = cct_rt_process_read_stream(fp, \"process run_capture allocation failed\");\n", out);
        fputs("    if (pclose(fp) == -1) cct_rt_fail(\"process run_capture pclose falhou\");\n", out);
        fputs("    return buf;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_process_run_with_input(const char *cmd, const char *input) {\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    (void)cmd;\n", out);
        fputs("    (void)input;\n", out);
        fputs("    cct_rt_fail(\"process run_with_input unsupported on _WIN32\");\n", out);
        fputs("    return \"\";\n", out);
        fputs("#else\n", out);
        fputs("    int stdin_pipe[2];\n", out);
        fputs("    int stdout_pipe[2];\n", out);
        fputs("    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) cct_rt_fail(\"process run_with_input pipe falhou\");\n", out);
        fputs("    pid_t pid = fork();\n", out);
        fputs("    if (pid < 0) cct_rt_fail(\"process run_with_input fork falhou\");\n", out);
        fputs("    if (pid == 0) {\n", out);
        fputs("        close(stdin_pipe[1]);\n", out);
        fputs("        close(stdout_pipe[0]);\n", out);
        fputs("        if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) _exit(127);\n", out);
        fputs("        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) _exit(127);\n", out);
        fputs("        close(stdin_pipe[0]);\n", out);
        fputs("        close(stdout_pipe[1]);\n", out);
        fputs("        execl(\"/bin/sh\", \"sh\", \"-c\", cmd ? cmd : \"\", (char*)NULL);\n", out);
        fputs("        _exit(127);\n", out);
        fputs("    }\n", out);
        fputs("    close(stdin_pipe[0]);\n", out);
        fputs("    close(stdout_pipe[1]);\n", out);
        fputs("    const char *src = input ? input : \"\";\n", out);
        fputs("    size_t left = strlen(src);\n", out);
        fputs("    const char *p = src;\n", out);
        fputs("    while (left > 0U) {\n", out);
        fputs("        ssize_t wrote = write(stdin_pipe[1], p, left);\n", out);
        fputs("        if (wrote < 0) {\n", out);
        fputs("            if (errno == EINTR) continue;\n", out);
        fputs("            if (errno == EPIPE) break;\n", out);
        fputs("            cct_rt_fail(\"process run_with_input write falhou\");\n", out);
        fputs("        }\n", out);
        fputs("        p += (size_t)wrote;\n", out);
        fputs("        left -= (size_t)wrote;\n", out);
        fputs("    }\n", out);
        fputs("    close(stdin_pipe[1]);\n", out);
        fputs("    FILE *fp = fdopen(stdout_pipe[0], \"r\");\n", out);
        fputs("    if (!fp) {\n", out);
        fputs("        close(stdout_pipe[0]);\n", out);
        fputs("        cct_rt_fail(\"process run_with_input fdopen falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    char *captured = cct_rt_process_read_stream(fp, \"process run_with_input capture allocation failed\");\n", out);
        fputs("    fclose(fp);\n", out);
        fputs("    int status = 0;\n", out);
        fputs("    while (waitpid(pid, &status, 0) < 0) {\n", out);
        fputs("        if (errno == EINTR) continue;\n", out);
        fputs("        cct_rt_fail(\"process run_with_input waitpid falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    return captured;\n", out);
        fputs("#endif\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_process_run_env(const char *cmd, void *env_pairs) {\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    (void)cmd;\n", out);
        fputs("    (void)env_pairs;\n", out);
        fputs("    cct_rt_fail(\"process run_env unsupported on _WIN32\");\n", out);
        fputs("    return -1LL;\n", out);
        fputs("#else\n", out);
        fputs("    pid_t pid = fork();\n", out);
        fputs("    if (pid < 0) cct_rt_fail(\"process run_env fork falhou\");\n", out);
        fputs("    if (pid == 0) {\n", out);
        fputs("        if (env_pairs) {\n", out);
        fputs("            long long n = cct_rt_fluxus_len(env_pairs);\n", out);
        fputs("            for (long long i = 0; i < n; i++) {\n", out);
        fputs("                char **slot = (char**)cct_rt_fluxus_get(env_pairs, i);\n", out);
        fputs("                const char *pair = (slot && *slot) ? *slot : \"\";\n", out);
        fputs("                const char *eq = strchr(pair, '=');\n", out);
        fputs("                if (!eq || eq == pair) continue;\n", out);
        fputs("                size_t key_len = (size_t)(eq - pair);\n", out);
        fputs("                char *key = (char*)malloc(key_len + 1U);\n", out);
        fputs("                if (!key) _exit(127);\n", out);
        fputs("                memcpy(key, pair, key_len);\n", out);
        fputs("                key[key_len] = '\\0';\n", out);
        fputs("                setenv(key, eq + 1, 1);\n", out);
        fputs("                free(key);\n", out);
        fputs("            }\n", out);
        fputs("        }\n", out);
        fputs("        execl(\"/bin/sh\", \"sh\", \"-c\", cmd ? cmd : \"\", (char*)NULL);\n", out);
        fputs("        _exit(127);\n", out);
        fputs("    }\n", out);
        fputs("    int status = 0;\n", out);
        fputs("    while (waitpid(pid, &status, 0) < 0) {\n", out);
        fputs("        if (errno == EINTR) continue;\n", out);
        fputs("        cct_rt_fail(\"process run_env waitpid falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    return cct_rt_process_decode_wait_status(status);\n", out);
        fputs("#endif\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_process_run_timeout(const char *cmd, long long timeout_ms) {\n", out);
        fputs("    if (timeout_ms < 0) cct_rt_fail(\"process run_timeout timeout_ms invalido\");\n", out);
        fputs("#ifdef _WIN32\n", out);
        fputs("    (void)cmd;\n", out);
        fputs("    cct_rt_fail(\"process run_timeout unsupported on _WIN32\");\n", out);
        fputs("    return -1LL;\n", out);
        fputs("#else\n", out);
        fputs("    pid_t pid = fork();\n", out);
        fputs("    if (pid < 0) cct_rt_fail(\"process run_timeout fork falhou\");\n", out);
        fputs("    if (pid == 0) {\n", out);
        fputs("        execl(\"/bin/sh\", \"sh\", \"-c\", cmd ? cmd : \"\", (char*)NULL);\n", out);
        fputs("        _exit(127);\n", out);
        fputs("    }\n", out);
        fputs("    long long start = cct_rt_time_now_ms();\n", out);
        fputs("    int status = 0;\n", out);
        fputs("    for (;;) {\n", out);
        fputs("        pid_t waited = waitpid(pid, &status, WNOHANG);\n", out);
        fputs("        if (waited == pid) return cct_rt_process_decode_wait_status(status);\n", out);
        fputs("        if (waited < 0) {\n", out);
        fputs("            if (errno == EINTR) continue;\n", out);
        fputs("            cct_rt_fail(\"process run_timeout waitpid falhou\");\n", out);
        fputs("        }\n", out);
        fputs("        long long now = cct_rt_time_now_ms();\n", out);
        fputs("        if (now - start >= timeout_ms) {\n", out);
        fputs("            kill(pid, SIGKILL);\n", out);
        fputs("            while (waitpid(pid, &status, 0) < 0) {\n", out);
        fputs("                if (errno == EINTR) continue;\n", out);
        fputs("                break;\n", out);
        fputs("            }\n", out);
        fputs("            return -1LL;\n", out);
        fputs("        }\n", out);
        fputs("        struct timespec req;\n", out);
        fputs("        req.tv_sec = 0;\n", out);
        fputs("        req.tv_nsec = 1000000L;\n", out);
        fputs("        nanosleep(&req, NULL);\n", out);
        fputs("    }\n", out);
        fputs("#endif\n", out);
        fputs("}\n\n", out);
    }

    fputs("typedef struct {\n", out);
    fputs("    int fd;\n", out);
    fputs("    int kind;\n", out);
    fputs("    int domain;\n", out);
    fputs("    int closed;\n", out);
    fputs("    int owns_unix_path;\n", out);
    fputs("    char unix_path[108];\n", out);
    fputs("} cct_rt_socket_t;\n\n", out);

    fputs("static cct_rt_socket_t *cct_rt_socket_require(void *sock_ptr, const char *ctx) {\n", out);
    fputs("    cct_rt_socket_t *sock = (cct_rt_socket_t*)sock_ptr;\n", out);
    fputs("    if (!sock) cct_rt_fail((ctx && *ctx) ? ctx : \"socket nulo\");\n", out);
    fputs("    return sock;\n", out);
    fputs("}\n\n", out);

    fputs("static cct_rt_socket_t *cct_rt_socket_alloc(int fd, int kind, int domain) {\n", out);
    fputs("    if (fd < 0) cct_rt_fail(\"socket fd invalido\");\n", out);
    fputs("    cct_rt_socket_t *sock = (cct_rt_socket_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_socket_t));\n", out);
    fputs("    sock->fd = fd;\n", out);
    fputs("    sock->kind = kind;\n", out);
    fputs("    sock->domain = domain;\n", out);
    fputs("    sock->closed = 0;\n", out);
    fputs("    sock->owns_unix_path = 0;\n", out);
    fputs("    sock->unix_path[0] = '\\0';\n", out);
    fputs("    return sock;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_check_open(cct_rt_socket_t *sock, const char *ctx) {\n", out);
    fputs("    if (!sock) cct_rt_fail((ctx && *ctx) ? ctx : \"socket nulo\");\n", out);
    fputs("    if (sock->closed || sock->fd < 0) cct_rt_fail((ctx && *ctx) ? ctx : \"socket fechado\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_socket_empty_string(void) {\n", out);
    fputs("    char *buf = (char*)cct_rt_alloc_or_fail(1U);\n", out);
    fputs("    buf[0] = '\\0';\n", out);
    fputs("    return buf;\n", out);
    fputs("}\n\n", out);

    fputs("static char cct_rt_socket_last_error[256] = \"\";\n\n", out);

    fputs("static char *cct_rt_socket_dup_cstr(const char *s) {\n", out);
    fputs("    const char *src = s ? s : \"\";\n", out);
    fputs("    size_t n = strlen(src);\n", out);
    fputs("    char *buf = (char*)cct_rt_alloc_or_fail(n + 1U);\n", out);
    fputs("    memcpy(buf, src, n + 1U);\n", out);
    fputs("    return buf;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_set_last_error(const char *msg) {\n", out);
    fputs("    const char *src = msg ? msg : \"\";\n", out);
    fputs("    size_t n = strlen(src);\n", out);
    fputs("    if (n >= sizeof(cct_rt_socket_last_error)) n = sizeof(cct_rt_socket_last_error) - 1U;\n", out);
    fputs("    memcpy(cct_rt_socket_last_error, src, n);\n", out);
    fputs("    cct_rt_socket_last_error[n] = '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_clear_last_error(void) {\n", out);
    fputs("    cct_rt_socket_last_error[0] = '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_sock_last_error(void) {\n", out);
    fputs("    return cct_rt_socket_dup_cstr(cct_rt_socket_last_error);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_socket_addr_string_from_sockaddr(const struct sockaddr *sa, socklen_t len) {\n", out);
    fputs("    if (!sa || len == 0) return cct_rt_socket_dup_cstr(\"\");\n", out);
    fputs("    if (sa->sa_family == AF_INET && len >= (socklen_t)sizeof(struct sockaddr_in)) {\n", out);
    fputs("        const struct sockaddr_in *in = (const struct sockaddr_in*)sa;\n", out);
    fputs("        char host[INET_ADDRSTRLEN];\n", out);
    fputs("        if (!inet_ntop(AF_INET, &in->sin_addr, host, sizeof(host))) return cct_rt_socket_dup_cstr(\"inet:?\");\n", out);
    fputs("        char out_buf[64];\n", out);
    fputs("        snprintf(out_buf, sizeof(out_buf), \"%s:%u\", host, (unsigned)ntohs(in->sin_port));\n", out);
    fputs("        return cct_rt_socket_dup_cstr(out_buf);\n", out);
    fputs("    }\n", out);
    fputs("    if (sa->sa_family == AF_UNIX && len >= (socklen_t)sizeof(struct sockaddr_un)) {\n", out);
    fputs("        const struct sockaddr_un *un = (const struct sockaddr_un*)sa;\n", out);
    fputs("        if (un->sun_path[0]) {\n", out);
    fputs("            char out_buf[160];\n", out);
    fputs("            snprintf(out_buf, sizeof(out_buf), \"unix:%s\", un->sun_path);\n", out);
    fputs("            return cct_rt_socket_dup_cstr(out_buf);\n", out);
    fputs("        }\n", out);
    fputs("        return cct_rt_socket_dup_cstr(\"unix\");\n", out);
    fputs("    }\n", out);
    fputs("    return cct_rt_socket_dup_cstr(\"unknown\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_sock_peer_addr(void *sock_ptr) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    cct_rt_fail(\"sock_peer_addr unsupported on _WIN32\");\n", out);
    fputs("    return \"\";\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_peer_addr recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_peer_addr recebeu socket fechado\");\n", out);
    fputs("    struct sockaddr_storage ss;\n", out);
    fputs("    socklen_t slen = (socklen_t)sizeof(ss);\n", out);
    fputs("    memset(&ss, 0, sizeof(ss));\n", out);
    fputs("    if (getpeername(sock->fd, (struct sockaddr*)&ss, &slen) != 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_peer_addr falhou\");\n", out);
    fputs("        return cct_rt_socket_empty_string();\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("    return cct_rt_socket_addr_string_from_sockaddr((const struct sockaddr*)&ss, slen);\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_sock_local_addr(void *sock_ptr) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    cct_rt_fail(\"sock_local_addr unsupported on _WIN32\");\n", out);
    fputs("    return \"\";\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_local_addr recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_local_addr recebeu socket fechado\");\n", out);
    fputs("    struct sockaddr_storage ss;\n", out);
    fputs("    socklen_t slen = (socklen_t)sizeof(ss);\n", out);
    fputs("    memset(&ss, 0, sizeof(ss));\n", out);
    fputs("    if (getsockname(sock->fd, (struct sockaddr*)&ss, &slen) != 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_local_addr falhou\");\n", out);
    fputs("        return cct_rt_socket_empty_string();\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("    return cct_rt_socket_addr_string_from_sockaddr((const struct sockaddr*)&ss, slen);\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_enable_reuseaddr(int fd) {\n", out);
    fputs("    int yes = 1;\n", out);
    fputs("    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, (socklen_t)sizeof(yes));\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_validate_port(long long port) {\n", out);
    fputs("    if (port < 0 || port > 65535) cct_rt_fail(\"sock port invalido\");\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_socket_is_local_host(const char *host) {\n", out);
    fputs("    if (!host || !*host) return 1;\n", out);
    fputs("    return strcmp(host, \"127.0.0.1\") == 0;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_socket_make_unix_path(long long port, char out_path[108]) {\n", out);
    fputs("    cct_rt_socket_validate_port(port);\n", out);
    fputs("    int n = snprintf(out_path, 108, \"tests/.tmp/cct_sock_%lld.sock\", port);\n", out);
    fputs("    if (n <= 0 || n >= 108) cct_rt_fail(\"sock unix path invalido\");\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_socket_tcp(void) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    cct_rt_fail(\"socket_tcp unsupported on _WIN32\");\n", out);
    fputs("    return NULL;\n", out);
    fputs("#else\n", out);
    fputs("    int fd = socket(AF_INET, SOCK_STREAM, 0);\n", out);
    fputs("    if (fd < 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"socket tcp create falhou\");\n", out);
    fputs("        cct_rt_fractum_throw_str(\"socket tcp create falhou\");\n", out);
    fputs("        return (void*)&cct_rt_null_sentinel;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("    return (void*)cct_rt_socket_alloc(fd, SOCK_STREAM, AF_INET);\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_socket_udp(void) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    cct_rt_fail(\"socket_udp unsupported on _WIN32\");\n", out);
    fputs("    return NULL;\n", out);
    fputs("#else\n", out);
    fputs("    int fd = socket(AF_INET, SOCK_DGRAM, 0);\n", out);
    fputs("    if (fd < 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"socket udp create falhou\");\n", out);
    fputs("        cct_rt_fractum_throw_str(\"socket udp create falhou\");\n", out);
    fputs("        return (void*)&cct_rt_null_sentinel;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("    return (void*)cct_rt_socket_alloc(fd, SOCK_DGRAM, AF_INET);\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_sock_connect(void *sock_ptr, const char *host, long long port) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)host;\n", out);
    fputs("    (void)port;\n", out);
    fputs("    cct_rt_fail(\"sock_connect unsupported on _WIN32\");\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_connect recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_connect recebeu socket fechado\");\n", out);
    fputs("    cct_rt_socket_validate_port(port);\n", out);
    fputs("    struct sockaddr_in addr;\n", out);
    fputs("    memset(&addr, 0, sizeof(addr));\n", out);
    fputs("    addr.sin_family = AF_INET;\n", out);
    fputs("    addr.sin_port = htons((uint16_t)port);\n", out);
    fputs("    const char *node = (host && *host) ? host : \"127.0.0.1\";\n", out);
    fputs("    if (inet_pton(AF_INET, node, &addr.sin_addr) != 1) cct_rt_fail(\"sock_connect host invalido\");\n", out);
    fputs("    if (connect(sock->fd, (const struct sockaddr*)&addr, (socklen_t)sizeof(addr)) == 0) {\n", out);
    fputs("        sock->domain = AF_INET;\n", out);
    fputs("        cct_rt_socket_clear_last_error();\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    if (!cct_rt_socket_is_local_host(host)) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_connect falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_connect falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    char unix_path[108];\n", out);
    fputs("    cct_rt_socket_make_unix_path(port, unix_path);\n", out);
    fputs("    int ufd = socket(AF_UNIX, sock->kind, 0);\n", out);
    fputs("    if (ufd < 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_connect falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_connect falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    struct sockaddr_un uaddr;\n", out);
    fputs("    memset(&uaddr, 0, sizeof(uaddr));\n", out);
    fputs("    uaddr.sun_family = AF_UNIX;\n", out);
    fputs("    strncpy(uaddr.sun_path, unix_path, sizeof(uaddr.sun_path) - 1);\n", out);
    fputs("    uaddr.sun_path[sizeof(uaddr.sun_path) - 1] = '\\0';\n", out);
    fputs("    if (connect(ufd, (const struct sockaddr*)&uaddr, (socklen_t)sizeof(uaddr)) != 0) {\n", out);
    fputs("        close(ufd);\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_connect falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_connect falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    (void)close(sock->fd);\n", out);
    fputs("    sock->fd = ufd;\n", out);
    fputs("    sock->domain = AF_UNIX;\n", out);
    fputs("    sock->owns_unix_path = 0;\n", out);
    fputs("    sock->unix_path[0] = '\\0';\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_sock_bind(void *sock_ptr, const char *host, long long port) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)host;\n", out);
    fputs("    (void)port;\n", out);
    fputs("    cct_rt_fail(\"sock_bind unsupported on _WIN32\");\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_bind recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_bind recebeu socket fechado\");\n", out);
    fputs("    cct_rt_socket_validate_port(port);\n", out);
    fputs("    struct sockaddr_in addr;\n", out);
    fputs("    memset(&addr, 0, sizeof(addr));\n", out);
    fputs("    addr.sin_family = AF_INET;\n", out);
    fputs("    addr.sin_port = htons((uint16_t)port);\n", out);
    fputs("    if (!host || !*host) addr.sin_addr.s_addr = htonl(INADDR_ANY);\n", out);
    fputs("    else if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) cct_rt_fail(\"sock_bind host invalido\");\n", out);
    fputs("    cct_rt_socket_enable_reuseaddr(sock->fd);\n", out);
    fputs("    if (bind(sock->fd, (const struct sockaddr*)&addr, (socklen_t)sizeof(addr)) == 0) {\n", out);
    fputs("        sock->domain = AF_INET;\n", out);
    fputs("        sock->owns_unix_path = 0;\n", out);
    fputs("        sock->unix_path[0] = '\\0';\n", out);
    fputs("        cct_rt_socket_clear_last_error();\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    if (!cct_rt_socket_is_local_host(host)) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_bind falhou\");\n", out);
    fputs("        cct_rt_fractum_throw_str(\"sock_bind falhou\");\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    char unix_path[108];\n", out);
    fputs("    cct_rt_socket_make_unix_path(port, unix_path);\n", out);
    fputs("    int ufd = socket(AF_UNIX, sock->kind, 0);\n", out);
    fputs("    if (ufd < 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_bind falhou\");\n", out);
    fputs("        cct_rt_fractum_throw_str(\"sock_bind falhou\");\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    struct sockaddr_un uaddr;\n", out);
    fputs("    memset(&uaddr, 0, sizeof(uaddr));\n", out);
    fputs("    uaddr.sun_family = AF_UNIX;\n", out);
    fputs("    strncpy(uaddr.sun_path, unix_path, sizeof(uaddr.sun_path) - 1);\n", out);
    fputs("    uaddr.sun_path[sizeof(uaddr.sun_path) - 1] = '\\0';\n", out);
    fputs("    unlink(unix_path);\n", out);
    fputs("    if (bind(ufd, (const struct sockaddr*)&uaddr, (socklen_t)sizeof(uaddr)) != 0) {\n", out);
    fputs("        close(ufd);\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_bind falhou\");\n", out);
    fputs("        cct_rt_fractum_throw_str(\"sock_bind falhou\");\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    (void)close(sock->fd);\n", out);
    fputs("    sock->fd = ufd;\n", out);
    fputs("    sock->domain = AF_UNIX;\n", out);
    fputs("    sock->owns_unix_path = 1;\n", out);
    fputs("    strncpy(sock->unix_path, unix_path, sizeof(sock->unix_path) - 1);\n", out);
    fputs("    sock->unix_path[sizeof(sock->unix_path) - 1] = '\\0';\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_sock_listen(void *sock_ptr, long long backlog) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)backlog;\n", out);
    fputs("    cct_rt_fail(\"sock_listen unsupported on _WIN32\");\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_listen recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_listen recebeu socket fechado\");\n", out);
    fputs("    if (backlog < 0) cct_rt_fail(\"sock_listen backlog invalido\");\n", out);
    fputs("    if (listen(sock->fd, (int)backlog) != 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_listen falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_listen falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_sock_accept(void *sock_ptr) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    cct_rt_fail(\"sock_accept unsupported on _WIN32\");\n", out);
    fputs("    return NULL;\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_accept recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_accept recebeu socket fechado\");\n", out);
    fputs("    for (;;) {\n", out);
    fputs("        int fd = accept(sock->fd, NULL, NULL);\n", out);
    fputs("        if (fd >= 0) {\n", out);
    fputs("            cct_rt_socket_clear_last_error();\n", out);
    fputs("            return (void*)cct_rt_socket_alloc(fd, sock->kind, sock->domain);\n", out);
    fputs("        }\n", out);
    fputs("        if (errno == EINTR) continue;\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_accept falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_accept falhou\");\n", out);
    fputs("    }\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_sock_send(void *sock_ptr, const char *data) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)data;\n", out);
    fputs("    cct_rt_fail(\"sock_send unsupported on _WIN32\");\n", out);
    fputs("    return 0LL;\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_send recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_send recebeu socket fechado\");\n", out);
    fputs("    const char *src = data ? data : \"\";\n", out);
    fputs("    size_t total = strlen(src);\n", out);
    fputs("    size_t sent = 0U;\n", out);
    fputs("    while (sent < total) {\n", out);
    fputs("        ssize_t rc = send(sock->fd, src + sent, total - sent, 0);\n", out);
    fputs("        if (rc > 0) {\n", out);
    fputs("            sent += (size_t)rc;\n", out);
    fputs("            continue;\n", out);
    fputs("        }\n", out);
    fputs("        if (rc < 0 && errno == EINTR) continue;\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_send falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_send falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("    return (long long)sent;\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_sock_recv(void *sock_ptr, long long max_bytes) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)max_bytes;\n", out);
    fputs("    cct_rt_fail(\"sock_recv unsupported on _WIN32\");\n", out);
    fputs("    return \"\";\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_recv recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_recv recebeu socket fechado\");\n", out);
    fputs("    if (max_bytes < 0) cct_rt_fail(\"sock_recv max_bytes invalido\");\n", out);
    fputs("    if (max_bytes == 0) return cct_rt_socket_empty_string();\n", out);
    fputs("    char *buf = (char*)cct_rt_alloc_or_fail((size_t)max_bytes + 1U);\n", out);
    fputs("    for (;;) {\n", out);
    fputs("        ssize_t rc = recv(sock->fd, buf, (size_t)max_bytes, 0);\n", out);
    fputs("        if (rc > 0) {\n", out);
    fputs("            buf[rc] = '\\0';\n", out);
    fputs("            cct_rt_socket_clear_last_error();\n", out);
    fputs("            return buf;\n", out);
    fputs("        }\n", out);
    fputs("        if (rc == 0) {\n", out);
    fputs("            buf[0] = '\\0';\n", out);
    fputs("            cct_rt_socket_clear_last_error();\n", out);
    fputs("            return buf;\n", out);
    fputs("        }\n", out);
    fputs("        if (errno == EINTR) continue;\n", out);
    fputs("        if (errno == EAGAIN || errno == EWOULDBLOCK) {\n", out);
    fputs("            cct_rt_free_ptr(buf);\n", out);
    fputs("            cct_rt_socket_clear_last_error();\n", out);
    fputs("            return cct_rt_socket_empty_string();\n", out);
    fputs("        }\n", out);
    fputs("        cct_rt_free_ptr(buf);\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_recv falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_recv falhou\");\n", out);
    fputs("    }\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_sock_close(void *sock_ptr) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    cct_rt_fail(\"sock_close unsupported on _WIN32\");\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = (cct_rt_socket_t*)sock_ptr;\n", out);
    fputs("    if (!sock) return;\n", out);
    fputs("    if (!sock->closed && sock->fd >= 0) {\n", out);
    fputs("        while (close(sock->fd) != 0) {\n", out);
    fputs("            if (errno == EINTR) continue;\n", out);
    fputs("            break;\n", out);
    fputs("        }\n", out);
    fputs("    }\n", out);
    fputs("    if (sock->owns_unix_path && sock->unix_path[0]) {\n", out);
    fputs("        (void)unlink(sock->unix_path);\n", out);
    fputs("    }\n", out);
    fputs("    sock->fd = -1;\n", out);
    fputs("    sock->closed = 1;\n", out);
    fputs("    sock->owns_unix_path = 0;\n", out);
    fputs("    sock->unix_path[0] = '\\0';\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_sock_set_timeout_ms(void *sock_ptr, long long timeout_ms) {\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    (void)sock_ptr;\n", out);
    fputs("    (void)timeout_ms;\n", out);
    fputs("    cct_rt_fail(\"sock_set_timeout_ms unsupported on _WIN32\");\n", out);
    fputs("#else\n", out);
    fputs("    cct_rt_socket_t *sock = cct_rt_socket_require(sock_ptr, \"sock_set_timeout_ms recebeu socket nulo\");\n", out);
    fputs("    cct_rt_socket_check_open(sock, \"sock_set_timeout_ms recebeu socket fechado\");\n", out);
    fputs("    if (timeout_ms < 0) cct_rt_fail(\"sock_set_timeout_ms invalido\");\n", out);
    fputs("    struct timeval tv;\n", out);
    fputs("    tv.tv_sec = (time_t)(timeout_ms / 1000LL);\n", out);
    fputs("    tv.tv_usec = (suseconds_t)((timeout_ms % 1000LL) * 1000LL);\n", out);
    fputs("    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, (socklen_t)sizeof(tv)) != 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_set_timeout_ms recv falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_set_timeout_ms recv falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, (socklen_t)sizeof(tv)) != 0) {\n", out);
    fputs("        cct_rt_socket_set_last_error(\"sock_set_timeout_ms send falhou\");\n", out);
    fputs("        cct_rt_fail(\"sock_set_timeout_ms send falhou\");\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_socket_clear_last_error();\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    if (cfg->emit_db_helpers) {
        fputs("typedef struct {\n", out);
        fputs("    sqlite3 *db;\n", out);
        fputs("    char last_error[256];\n", out);
        fputs("} cct_rt_db_t;\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_db_t *owner;\n", out);
        fputs("    sqlite3_stmt *stmt;\n", out);
        fputs("    int has_row;\n", out);
        fputs("} cct_rt_rows_t;\n\n", out);

        fputs("typedef struct {\n", out);
        fputs("    cct_rt_db_t *owner;\n", out);
        fputs("    sqlite3_stmt *stmt;\n", out);
        fputs("} cct_rt_stmt_t;\n\n", out);

        fputs("static char *cct_rt_db_dup_cstr(const char *s) {\n", out);
        fputs("    const char *src = s ? s : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    char *buf = (char*)cct_rt_alloc_or_fail(n + 1U);\n", out);
        fputs("    memcpy(buf, src, n + 1U);\n", out);
        fputs("    return buf;\n", out);
        fputs("}\n\n", out);

        fputs("static cct_rt_db_t *cct_rt_db_require(void *db_ptr, const char *ctx) {\n", out);
        fputs("    cct_rt_db_t *db = (cct_rt_db_t*)db_ptr;\n", out);
        fputs("    if (!db) cct_rt_fail((ctx && *ctx) ? ctx : \"db nulo\");\n", out);
        fputs("    return db;\n", out);
        fputs("}\n\n", out);

        fputs("static cct_rt_rows_t *cct_rt_rows_require(void *rows_ptr, const char *ctx) {\n", out);
        fputs("    cct_rt_rows_t *rows = (cct_rt_rows_t*)rows_ptr;\n", out);
        fputs("    if (!rows) cct_rt_fail((ctx && *ctx) ? ctx : \"rows nulo\");\n", out);
        fputs("    return rows;\n", out);
        fputs("}\n\n", out);

        fputs("static cct_rt_stmt_t *cct_rt_stmt_require(void *stmt_ptr, const char *ctx) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = (cct_rt_stmt_t*)stmt_ptr;\n", out);
        fputs("    if (!stmt) cct_rt_fail((ctx && *ctx) ? ctx : \"stmt nulo\");\n", out);
        fputs("    return stmt;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_set_last_error(cct_rt_db_t *db, const char *msg) {\n", out);
        fputs("    const char *src = msg ? msg : \"\";\n", out);
        fputs("    size_t n = strlen(src);\n", out);
        fputs("    if (n >= sizeof(db->last_error)) n = sizeof(db->last_error) - 1U;\n", out);
        fputs("    memcpy(db->last_error, src, n);\n", out);
        fputs("    db->last_error[n] = '\\0';\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_clear_last_error(cct_rt_db_t *db) {\n", out);
        fputs("    db->last_error[0] = '\\0';\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_db_open(const char *path) {\n", out);
        fputs("    cct_rt_db_t *wrap = (cct_rt_db_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_db_t));\n", out);
        fputs("    wrap->db = NULL;\n", out);
        fputs("    wrap->last_error[0] = '\\0';\n", out);
        fputs("    if (sqlite3_open((path && *path) ? path : \":memory:\", &wrap->db) != SQLITE_OK) {\n", out);
        fputs("        cct_rt_db_set_last_error(wrap, wrap->db ? sqlite3_errmsg(wrap->db) : \"db_open falhou\");\n", out);
        fputs("    }\n", out);
        fputs("    return (void*)wrap;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_close(void *db_ptr) {\n", out);
        fputs("    cct_rt_db_t *db = (cct_rt_db_t*)db_ptr;\n", out);
        fputs("    if (!db) return;\n", out);
        fputs("    if (db->db) {\n", out);
        fputs("        (void)sqlite3_close(db->db);\n", out);
        fputs("        db->db = NULL;\n", out);
        fputs("    }\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_exec(void *db_ptr, const char *sql) {\n", out);
        fputs("    cct_rt_db_t *db = cct_rt_db_require(db_ptr, \"db_exec recebeu db nulo\");\n", out);
        fputs("    if (!db->db) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, \"db nao aberto\");\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    char *err = NULL;\n", out);
        fputs("    if (sqlite3_exec(db->db, (sql && *sql) ? sql : \"\", NULL, NULL, &err) != SQLITE_OK) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, err ? err : sqlite3_errmsg(db->db));\n", out);
        fputs("        if (err) sqlite3_free(err);\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (err) sqlite3_free(err);\n", out);
        fputs("    cct_rt_db_clear_last_error(db);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_db_last_error(void *db_ptr) {\n", out);
        fputs("    cct_rt_db_t *db = cct_rt_db_require(db_ptr, \"db_last_error recebeu db nulo\");\n", out);
        fputs("    return cct_rt_db_dup_cstr(db->last_error);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_db_query(void *db_ptr, const char *sql) {\n", out);
        fputs("    cct_rt_db_t *db = cct_rt_db_require(db_ptr, \"db_query recebeu db nulo\");\n", out);
        fputs("    cct_rt_rows_t *rows = (cct_rt_rows_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_rows_t));\n", out);
        fputs("    rows->owner = db;\n", out);
        fputs("    rows->stmt = NULL;\n", out);
        fputs("    rows->has_row = 0;\n", out);
        fputs("    if (!db->db) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, \"db nao aberto\");\n", out);
        fputs("        return (void*)rows;\n", out);
        fputs("    }\n", out);
        fputs("    if (sqlite3_prepare_v2(db->db, (sql && *sql) ? sql : \"\", -1, &rows->stmt, NULL) != SQLITE_OK) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, sqlite3_errmsg(db->db));\n", out);
        fputs("        if (rows->stmt) {\n", out);
        fputs("            (void)sqlite3_finalize(rows->stmt);\n", out);
        fputs("            rows->stmt = NULL;\n", out);
        fputs("        }\n", out);
        fputs("        return (void*)rows;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_db_clear_last_error(db);\n", out);
        fputs("    return (void*)rows;\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_rows_next(void *rows_ptr) {\n", out);
        fputs("    cct_rt_rows_t *rows = cct_rt_rows_require(rows_ptr, \"rows_next recebeu rows nulo\");\n", out);
        fputs("    rows->has_row = 0;\n", out);
        fputs("    if (!rows->stmt) return 0LL;\n", out);
        fputs("    int rc = sqlite3_step(rows->stmt);\n", out);
        fputs("    if (rc == SQLITE_ROW) {\n", out);
        fputs("        rows->has_row = 1;\n", out);
        fputs("        if (rows->owner) cct_rt_db_clear_last_error(rows->owner);\n", out);
        fputs("        return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    if (rc == SQLITE_DONE) {\n", out);
        fputs("        if (rows->owner) cct_rt_db_clear_last_error(rows->owner);\n", out);
        fputs("        return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    if (rows->owner) cct_rt_db_set_last_error(rows->owner, sqlite3_errmsg(rows->owner->db));\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static int cct_rt_rows_has_col(cct_rt_rows_t *rows, long long col) {\n", out);
        fputs("    if (!rows || !rows->stmt || !rows->has_row) return 0;\n", out);
        fputs("    if (col < 0) return 0;\n", out);
        fputs("    return col < (long long)sqlite3_column_count(rows->stmt);\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_rows_get_text(void *rows_ptr, long long col) {\n", out);
        fputs("    cct_rt_rows_t *rows = cct_rt_rows_require(rows_ptr, \"rows_get_text recebeu rows nulo\");\n", out);
        fputs("    if (!cct_rt_rows_has_col(rows, col)) return cct_rt_db_dup_cstr(\"\");\n", out);
        fputs("    const unsigned char *text = sqlite3_column_text(rows->stmt, (int)col);\n", out);
        fputs("    return cct_rt_db_dup_cstr(text ? (const char*)text : \"\");\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_rows_get_int(void *rows_ptr, long long col) {\n", out);
        fputs("    cct_rt_rows_t *rows = cct_rt_rows_require(rows_ptr, \"rows_get_int recebeu rows nulo\");\n", out);
        fputs("    if (!cct_rt_rows_has_col(rows, col)) return 0LL;\n", out);
        fputs("    return (long long)sqlite3_column_int64(rows->stmt, (int)col);\n", out);
        fputs("}\n\n", out);

        fputs("static double cct_rt_rows_get_real(void *rows_ptr, long long col) {\n", out);
        fputs("    cct_rt_rows_t *rows = cct_rt_rows_require(rows_ptr, \"rows_get_real recebeu rows nulo\");\n", out);
        fputs("    if (!cct_rt_rows_has_col(rows, col)) return 0.0;\n", out);
        fputs("    return sqlite3_column_double(rows->stmt, (int)col);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_rows_close(void *rows_ptr) {\n", out);
        fputs("    cct_rt_rows_t *rows = (cct_rt_rows_t*)rows_ptr;\n", out);
        fputs("    if (!rows) return;\n", out);
        fputs("    if (rows->stmt) {\n", out);
        fputs("        (void)sqlite3_finalize(rows->stmt);\n", out);
        fputs("        rows->stmt = NULL;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_free_ptr(rows);\n", out);
        fputs("}\n\n", out);

        fputs("static void *cct_rt_db_prepare(void *db_ptr, const char *sql) {\n", out);
        fputs("    cct_rt_db_t *db = cct_rt_db_require(db_ptr, \"db_prepare recebeu db nulo\");\n", out);
        fputs("    cct_rt_stmt_t *stmt = (cct_rt_stmt_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_stmt_t));\n", out);
        fputs("    stmt->owner = db;\n", out);
        fputs("    stmt->stmt = NULL;\n", out);
        fputs("    if (!db->db) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, \"db nao aberto\");\n", out);
        fputs("        return (void*)stmt;\n", out);
        fputs("    }\n", out);
        fputs("    if (sqlite3_prepare_v2(db->db, (sql && *sql) ? sql : \"\", -1, &stmt->stmt, NULL) != SQLITE_OK) {\n", out);
        fputs("        cct_rt_db_set_last_error(db, sqlite3_errmsg(db->db));\n", out);
        fputs("        if (stmt->stmt) {\n", out);
        fputs("            (void)sqlite3_finalize(stmt->stmt);\n", out);
        fputs("            stmt->stmt = NULL;\n", out);
        fputs("        }\n", out);
        fputs("        return (void*)stmt;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_db_clear_last_error(db);\n", out);
        fputs("    return (void*)stmt;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_stmt_bind_text(void *stmt_ptr, long long idx, const char *value) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = cct_rt_stmt_require(stmt_ptr, \"stmt_bind_text recebeu stmt nulo\");\n", out);
        fputs("    if (!stmt->stmt) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, \"stmt nao preparado\");\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (sqlite3_bind_text(stmt->stmt, (int)idx, value ? value : \"\", -1, SQLITE_TRANSIENT) != SQLITE_OK) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, sqlite3_errmsg(stmt->owner->db));\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (stmt->owner) cct_rt_db_clear_last_error(stmt->owner);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_stmt_bind_int(void *stmt_ptr, long long idx, long long value) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = cct_rt_stmt_require(stmt_ptr, \"stmt_bind_int recebeu stmt nulo\");\n", out);
        fputs("    if (!stmt->stmt) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, \"stmt nao preparado\");\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (sqlite3_bind_int64(stmt->stmt, (int)idx, (sqlite3_int64)value) != SQLITE_OK) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, sqlite3_errmsg(stmt->owner->db));\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (stmt->owner) cct_rt_db_clear_last_error(stmt->owner);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_stmt_bind_real(void *stmt_ptr, long long idx, double value) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = cct_rt_stmt_require(stmt_ptr, \"stmt_bind_real recebeu stmt nulo\");\n", out);
        fputs("    if (!stmt->stmt) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, \"stmt nao preparado\");\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (sqlite3_bind_double(stmt->stmt, (int)idx, value) != SQLITE_OK) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, sqlite3_errmsg(stmt->owner->db));\n", out);
        fputs("        return;\n", out);
        fputs("    }\n", out);
        fputs("    if (stmt->owner) cct_rt_db_clear_last_error(stmt->owner);\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_stmt_step(void *stmt_ptr) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = cct_rt_stmt_require(stmt_ptr, \"stmt_step recebeu stmt nulo\");\n", out);
        fputs("    if (!stmt->stmt) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, \"stmt nao preparado\");\n", out);
        fputs("        return 0LL;\n", out);
        fputs("    }\n", out);
        fputs("    int rc = sqlite3_step(stmt->stmt);\n", out);
        fputs("    if (rc == SQLITE_ROW || rc == SQLITE_DONE) {\n", out);
        fputs("        if (stmt->owner) cct_rt_db_clear_last_error(stmt->owner);\n", out);
        fputs("        return 1LL;\n", out);
        fputs("    }\n", out);
        fputs("    if (stmt->owner) cct_rt_db_set_last_error(stmt->owner, sqlite3_errmsg(stmt->owner->db));\n", out);
        fputs("    return 0LL;\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_stmt_reset(void *stmt_ptr) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = cct_rt_stmt_require(stmt_ptr, \"stmt_reset recebeu stmt nulo\");\n", out);
        fputs("    if (!stmt->stmt) return;\n", out);
        fputs("    (void)sqlite3_reset(stmt->stmt);\n", out);
        fputs("    (void)sqlite3_clear_bindings(stmt->stmt);\n", out);
        fputs("    if (stmt->owner) cct_rt_db_clear_last_error(stmt->owner);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_stmt_finalize(void *stmt_ptr) {\n", out);
        fputs("    cct_rt_stmt_t *stmt = (cct_rt_stmt_t*)stmt_ptr;\n", out);
        fputs("    if (!stmt) return;\n", out);
        fputs("    if (stmt->stmt) {\n", out);
        fputs("        (void)sqlite3_finalize(stmt->stmt);\n", out);
        fputs("        stmt->stmt = NULL;\n", out);
        fputs("    }\n", out);
        fputs("    cct_rt_free_ptr(stmt);\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_begin(void *db_ptr) {\n", out);
        fputs("    cct_rt_db_exec(db_ptr, \"BEGIN TRANSACTION;\");\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_commit(void *db_ptr) {\n", out);
        fputs("    cct_rt_db_exec(db_ptr, \"COMMIT;\");\n", out);
        fputs("}\n\n", out);

        fputs("static void cct_rt_db_rollback(void *db_ptr) {\n", out);
        fputs("    cct_rt_db_exec(db_ptr, \"ROLLBACK;\");\n", out);
        fputs("}\n\n", out);

        fputs("static long long cct_rt_db_scalar_int(void *db_ptr, const char *sql) {\n", out);
        fputs("    cct_rt_rows_t *rows = (cct_rt_rows_t*)cct_rt_db_query(db_ptr, sql);\n", out);
        fputs("    long long value = 0LL;\n", out);
        fputs("    if (rows && cct_rt_rows_next(rows)) value = cct_rt_rows_get_int(rows, 0);\n", out);
        fputs("    cct_rt_rows_close(rows);\n", out);
        fputs("    return value;\n", out);
        fputs("}\n\n", out);

        fputs("static char *cct_rt_db_scalar_text(void *db_ptr, const char *sql) {\n", out);
        fputs("    cct_rt_rows_t *rows = (cct_rt_rows_t*)cct_rt_db_query(db_ptr, sql);\n", out);
        fputs("    char *value = cct_rt_db_dup_cstr(\"\");\n", out);
        fputs("    if (rows && cct_rt_rows_next(rows)) value = cct_rt_rows_get_text(rows, 0);\n", out);
        fputs("    cct_rt_rows_close(rows);\n", out);
        fputs("    return value;\n", out);
        fputs("}\n\n", out);
    }

    if (cfg->emit_crypto_helpers) {
        if (!cct_runtime_emit_crypto_helpers(out)) return false;
    }
    if (cfg->emit_regex_helpers) {
        if (!cct_runtime_emit_regex_helpers(out)) return false;
    }
    if (cfg->emit_toml_helpers) {
        if (!cct_runtime_emit_toml_helpers(out)) return false;
    }
    if (cfg->emit_compress_helpers) {
        if (!cct_runtime_emit_compress_helpers(out)) return false;
    }
    if (cfg->emit_filetype_helpers) {
        if (!cct_runtime_emit_filetype_helpers(out)) return false;
    }
    if (cfg->emit_image_ops_helpers) {
        if (!cct_runtime_emit_image_ops_helpers(out)) return false;
    }
    if (cfg->emit_signal_helpers) {
        if (!cct_runtime_emit_signal_helpers(out)) return false;
    }
    if (cfg->emit_postgres_helpers) {
        if (!cct_runtime_emit_postgres_helpers(out)) return false;
    }
    if (cfg->emit_mail_helpers) {
        if (!cct_runtime_emit_mail_helpers(out)) return false;
    }
    if (cfg->emit_instrument_helpers) {
        if (!cct_runtime_emit_instrument_helpers(out)) return false;
    }

    fputs("/* ===== End CCT Runtime Helpers ===== */\n\n", out);
    return true;
}
