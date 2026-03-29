/*
 * CCT — Clavicula Turing
 * Module System Implementation
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "module.h"

#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../common/fuzzy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef CCT_STDLIB_DIR
#define CCT_STDLIB_DIR "lib/cct"
#endif

typedef struct {
    char *canonical_path;
    cct_ast_program_t *program;
    cct_module_origin_t origin;
    size_t *import_indices;
    size_t import_count;
    size_t import_capacity;
} cct_module_unit_t;

typedef enum {
    MOD_GLOBAL_RITUALE = 0,
    MOD_GLOBAL_TYPE,
    MOD_GLOBAL_ENUM_ITEM,
    MOD_GLOBAL_PACTUM,
} cct_module_global_kind_t;

typedef struct {
    char *name;
    cct_module_global_kind_t kind;
    bool is_internal;
    size_t type_param_count; /* GENUS arity for RITUALE/SIGILLUM in subset 10B */
    const cct_ast_node_t *decl_node; /* Source top-level decl for richer checks (10D+) */
    size_t owner_module_index;
    u32 line;
    u32 column;
} cct_module_global_symbol_t;

typedef struct {
    cct_module_unit_t *modules;
    size_t module_count;
    size_t module_capacity;
    cct_profile_t profile;

    const char **active_stack;
    size_t active_count;
    size_t active_capacity;

    u32 import_edge_count;
    cct_module_global_symbol_t *globals;
    size_t global_count;
    size_t global_capacity;
    u32 cross_module_call_count;
    u32 cross_module_type_ref_count;
    u32 public_symbol_count;
    u32 internal_symbol_count;
    bool had_error;
    cct_error_code_t error_code;
} cct_module_ctx_t;

static void mod_report(cct_module_ctx_t *ctx,
                       cct_error_code_t code,
                       const char *filename,
                       u32 line,
                       u32 column,
                       const char *fmt,
                       ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (filename) cct_error_at_location(code, filename, line, column, buffer);
    else cct_error_printf(code, "%s", buffer);

    ctx->had_error = true;
    if (ctx->error_code == CCT_OK) {
        ctx->error_code = code;
    }
}

static void mod_report_with_suggestion(cct_module_ctx_t *ctx,
                                       cct_error_code_t code,
                                       const char *filename,
                                       u32 line,
                                       u32 column,
                                       const char *suggestion,
                                       const char *fmt,
                                       ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (filename) cct_error_at_location_with_suggestion(code, filename, line, column, buffer, suggestion);
    else cct_error_printf(code, "%s", buffer);

    ctx->had_error = true;
    if (ctx->error_code == CCT_OK) {
        ctx->error_code = code;
    }
}

static const char* mod_stdlib_import_hint_for_symbol(const char *name) {
    if (!name) return NULL;
    if (strcmp(name, "len") == 0 ||
        strcmp(name, "concat") == 0 ||
        strcmp(name, "compare") == 0 ||
        strcmp(name, "substring") == 0 ||
        strcmp(name, "trim") == 0 ||
        strcmp(name, "find") == 0 ||
        strcmp(name, "contains") == 0) {
        return "add ADVOCARE \"cct/verbum.cct\" in the module header";
    }
    if (strcmp(name, "print") == 0 ||
        strcmp(name, "println") == 0 ||
        strcmp(name, "print_int") == 0 ||
        strcmp(name, "read_line") == 0) {
        return "add ADVOCARE \"cct/io.cct\" in the module header";
    }
    if (strcmp(name, "Some") == 0 ||
        strcmp(name, "None") == 0 ||
        strcmp(name, "option_is_some") == 0 ||
        strcmp(name, "option_is_none") == 0 ||
        strcmp(name, "option_unwrap") == 0 ||
        strcmp(name, "option_unwrap_or") == 0 ||
        strcmp(name, "option_expect") == 0 ||
        strcmp(name, "option_free") == 0) {
        return "add ADVOCARE \"cct/option.cct\" in the module header";
    }
    if (strcmp(name, "Ok") == 0 ||
        strcmp(name, "Err") == 0 ||
        strcmp(name, "result_is_ok") == 0 ||
        strcmp(name, "result_is_err") == 0 ||
        strcmp(name, "result_unwrap") == 0 ||
        strcmp(name, "result_unwrap_or") == 0 ||
        strcmp(name, "result_unwrap_err") == 0 ||
        strcmp(name, "result_expect") == 0 ||
        strcmp(name, "result_free") == 0) {
        return "add ADVOCARE \"cct/result.cct\" in the module header";
    }
    if (strcmp(name, "map_init") == 0 ||
        strcmp(name, "map_free") == 0 ||
        strcmp(name, "map_insert") == 0 ||
        strcmp(name, "map_remove") == 0 ||
        strcmp(name, "map_get") == 0 ||
        strcmp(name, "map_contains") == 0 ||
        strcmp(name, "map_len") == 0 ||
        strcmp(name, "map_is_empty") == 0 ||
        strcmp(name, "map_capacity") == 0 ||
        strcmp(name, "map_clear") == 0 ||
        strcmp(name, "map_reserve") == 0) {
        return "add ADVOCARE \"cct/map.cct\" in the module header";
    }
    if (strcmp(name, "set_init") == 0 ||
        strcmp(name, "set_free") == 0 ||
        strcmp(name, "set_insert") == 0 ||
        strcmp(name, "set_remove") == 0 ||
        strcmp(name, "set_contains") == 0 ||
        strcmp(name, "set_len") == 0 ||
        strcmp(name, "set_is_empty") == 0 ||
        strcmp(name, "set_clear") == 0) {
        return "add ADVOCARE \"cct/set.cct\" in the module header";
    }
    if (strcmp(name, "fluxus_map") == 0 ||
        strcmp(name, "fluxus_filter") == 0 ||
        strcmp(name, "fluxus_fold") == 0 ||
        strcmp(name, "fluxus_find") == 0 ||
        strcmp(name, "fluxus_any") == 0 ||
        strcmp(name, "fluxus_all") == 0 ||
        strcmp(name, "series_map") == 0 ||
        strcmp(name, "series_filter") == 0 ||
        strcmp(name, "series_reduce") == 0 ||
        strcmp(name, "series_find") == 0 ||
        strcmp(name, "series_any") == 0 ||
        strcmp(name, "series_all") == 0) {
        return "add ADVOCARE \"cct/collection_ops.cct\" in the module header";
    }
    if (strcmp(name, "sha256") == 0 ||
        strcmp(name, "sha256_bytes") == 0 ||
        strcmp(name, "sha512") == 0 ||
        strcmp(name, "sha512_bytes") == 0 ||
        strcmp(name, "hmac_sha256") == 0 ||
        strcmp(name, "hmac_sha512") == 0 ||
        strcmp(name, "pbkdf2_sha256") == 0 ||
        strcmp(name, "csprng_bytes") == 0 ||
        strcmp(name, "constant_time_compare") == 0 ||
        strcmp(name, "bytes_to_hex") == 0) {
        return "add ADVOCARE \"cct/crypto.cct\" in the module header";
    }
    if (strcmp(name, "regex_compile") == 0 ||
        strcmp(name, "regex_compile_flags") == 0 ||
        strcmp(name, "regex_match") == 0 ||
        strcmp(name, "regex_search") == 0 ||
        strcmp(name, "regex_find_all") == 0 ||
        strcmp(name, "regex_replace") == 0 ||
        strcmp(name, "regex_replace_all") == 0 ||
        strcmp(name, "regex_split") == 0 ||
        strcmp(name, "regex_free") == 0 ||
        strcmp(name, "regex_is_match") == 0 ||
        strcmp(name, "regex_quick_search") == 0 ||
        strcmp(name, "regex_is_email") == 0 ||
        strcmp(name, "regex_is_url") == 0 ||
        strcmp(name, "regex_is_integer") == 0 ||
        strcmp(name, "regex_is_real") == 0 ||
        strcmp(name, "regex_flag_case_insensitive") == 0 ||
        strcmp(name, "regex_flag_multiline") == 0 ||
        strcmp(name, "regex_flag_dotall") == 0) {
        return "add ADVOCARE \"cct/regex.cct\" in the module header";
    }
    if (strcmp(name, "date_new") == 0 ||
        strcmp(name, "date_parse") == 0 ||
        strcmp(name, "date_format") == 0 ||
        strcmp(name, "date_add_days") == 0 ||
        strcmp(name, "date_add_months") == 0 ||
        strcmp(name, "date_diff_days") == 0 ||
        strcmp(name, "date_compare") == 0 ||
        strcmp(name, "datetime_new") == 0 ||
        strcmp(name, "datetime_parse") == 0 ||
        strcmp(name, "datetime_format") == 0 ||
        strcmp(name, "datetime_to_unix") == 0 ||
        strcmp(name, "datetime_from_unix") == 0 ||
        strcmp(name, "datetime_add_seconds") == 0 ||
        strcmp(name, "datetime_diff_seconds") == 0 ||
        strcmp(name, "datetime_compare") == 0 ||
        strcmp(name, "datetime_now") == 0 ||
        strcmp(name, "date_today") == 0) {
        return "add ADVOCARE \"cct/date.cct\" in the module header";
    }
    if (strcmp(name, "toml_parse") == 0 ||
        strcmp(name, "toml_parse_file") == 0 ||
        strcmp(name, "toml_has") == 0 ||
        strcmp(name, "toml_get_string") == 0 ||
        strcmp(name, "toml_get_int") == 0 ||
        strcmp(name, "toml_get_real") == 0 ||
        strcmp(name, "toml_get_bool") == 0 ||
        strcmp(name, "toml_get_date") == 0 ||
        strcmp(name, "toml_get_datetime") == 0 ||
        strcmp(name, "toml_get_table") == 0 ||
        strcmp(name, "toml_array_len") == 0 ||
        strcmp(name, "toml_array_get_string") == 0 ||
        strcmp(name, "toml_array_get_int") == 0 ||
        strcmp(name, "toml_array_get_real") == 0 ||
        strcmp(name, "toml_array_get_bool") == 0 ||
        strcmp(name, "toml_expand_env") == 0 ||
        strcmp(name, "toml_stringify") == 0 ||
        strcmp(name, "toml_get_value") == 0) {
        return "add ADVOCARE \"cct/toml.cct\" in the module header";
    }
    if (strcmp(name, "gzip_compress") == 0 ||
        strcmp(name, "gzip_decompress") == 0 ||
        strcmp(name, "gzip_compress_bytes") == 0 ||
        strcmp(name, "gzip_decompress_bytes") == 0) {
        return "add ADVOCARE \"cct/compress.cct\" in the module header";
    }
    if (strcmp(name, "media_probe") == 0 ||
        strcmp(name, "media_has_video") == 0 ||
        strcmp(name, "media_has_audio") == 0 ||
        strcmp(name, "media_get_resolution") == 0 ||
        strcmp(name, "media_get_duration_str") == 0) {
        return "add ADVOCARE \"cct/media_probe.cct\" in the module header";
    }
    if (strcmp(name, "image_load") == 0 ||
        strcmp(name, "image_free") == 0 ||
        strcmp(name, "image_save") == 0 ||
        strcmp(name, "image_resize") == 0 ||
        strcmp(name, "image_crop") == 0 ||
        strcmp(name, "image_rotate") == 0 ||
        strcmp(name, "image_convert_format") == 0 ||
        strcmp(name, "image_get_dimensions") == 0 ||
        strcmp(name, "image_is_valid") == 0 ||
        strcmp(name, "image_last_error") == 0) {
        return "add ADVOCARE \"cct/image_ops.cct\" in the module header";
    }
    if (strcmp(name, "lang_detect") == 0 ||
        strcmp(name, "lang_detect_from") == 0 ||
        strcmp(name, "lang_is") == 0 ||
        strcmp(name, "lang_to_string") == 0 ||
        strcmp(name, "lang_from_string") == 0 ||
        strcmp(name, "lang_to_iso639") == 0) {
        return "add ADVOCARE \"cct/text_lang.cct\" in the module header";
    }
    return NULL;
}

static const char* mod_find_rituale_suggestion(const cct_module_ctx_t *ctx, const char *name) {
    if (!ctx || !name || ctx->global_count == 0) return NULL;
    const char *candidates[256];
    size_t count = 0;
    for (size_t i = 0; i < ctx->global_count && count < 256; i++) {
        if (ctx->globals[i].kind == MOD_GLOBAL_RITUALE && ctx->globals[i].name) {
            candidates[count++] = ctx->globals[i].name;
        }
    }
    return cct_fuzzy_match(name, candidates, count);
}

static bool mod_has_cct_extension(const char *path) {
    if (!path) return false;
    size_t len = strlen(path);
    return len >= 4 && strcmp(path + len - 4, ".cct") == 0;
}

static bool mod_is_stdlib_import(const char *raw_import) {
    return raw_import && strncmp(raw_import, "cct/", 4) == 0;
}

static bool mod_is_cct_kernel_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/kernel") == 0 ||
           strcmp(canonical, "cct/kernel/kernel") == 0;
}

static bool mod_is_cct_console_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/console") == 0 ||
           strcmp(canonical, "cct/console/console") == 0;
}

static bool mod_is_cct_mem_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/mem_fs") == 0 ||
           strcmp(canonical, "cct/mem_fs/mem_fs") == 0;
}

static bool mod_is_cct_verbum_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/verbum_fs") == 0 ||
           strcmp(canonical, "cct/verbum_fs/verbum_fs") == 0;
}

static bool mod_is_cct_fluxus_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/fluxus_fs") == 0 ||
           strcmp(canonical, "cct/fluxus_fs/fluxus_fs") == 0;
}

static bool mod_is_cct_irq_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/irq_fs") == 0 ||
           strcmp(canonical, "cct/irq_fs/irq_fs") == 0;
}

static bool mod_is_cct_keyboard_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/keyboard_fs") == 0 ||
           strcmp(canonical, "cct/keyboard_fs/keyboard_fs") == 0;
}

static bool mod_is_cct_timer_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/timer_fs") == 0 ||
           strcmp(canonical, "cct/timer_fs/timer_fs") == 0;
}

static bool mod_is_cct_shell_fs_import(const char *raw_import) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    return strcmp(canonical, "cct/shell_fs") == 0 ||
           strcmp(canonical, "cct/shell_fs/shell_fs") == 0;
}

static bool mod_is_freestanding_forbidden_import(const char *raw_import, char *out_name, size_t out_name_size) {
    if (!raw_import) return false;
    size_t len = strlen(raw_import);
    if (len >= 4 && strcmp(raw_import + len - 4, ".cct") == 0) {
        len -= 4;
    }
    if (len == 0 || len >= 128) return false;

    char canonical[128];
    memcpy(canonical, raw_import, len);
    canonical[len] = '\0';

    static const char *forbidden[] = {
        "cct/io",
        "cct/fs",
        "cct/fluxus",
        "cct/map",
        "cct/set",
        "cct/random",
        "cct/crypto",
        "cct/regex",
        "cct/date",
        "cct/toml",
        "cct/compress",
        "cct/filetype",
        "cct/media_probe",
        "cct/image_ops",
        "cct/db_postgres",
        "cct/db_postgres_search",
        "cct/redis",
        "cct/db_postgres_lock",
        "cct/mail",
        "cct/mail_spool",
        "cct/mail_webhook",
    };

    for (size_t i = 0; i < sizeof(forbidden) / sizeof(forbidden[0]); i++) {
        if (strcmp(canonical, forbidden[i]) == 0) {
            if (out_name && out_name_size > 0) {
                snprintf(out_name, out_name_size, "%s", forbidden[i]);
            }
            return true;
        }
    }
    return false;
}

static const char* mod_path_leaf(const char *path) {
    if (!path || path[0] == '\0') return "";
    const char *slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static char* mod_append_suffix(const char *base, const char *suffix) {
    if (!base || !suffix) return NULL;
    size_t base_len = strlen(base);
    size_t suffix_len = strlen(suffix);
    char *out = (char*)malloc(base_len + suffix_len + 1);
    if (!out) return NULL;
    memcpy(out, base, base_len);
    memcpy(out + base_len, suffix, suffix_len);
    out[base_len + suffix_len] = '\0';
    return out;
}

static const char* mod_stdlib_root(void) {
    const char *env_root = getenv("CCT_STDLIB_DIR");
    if (env_root && env_root[0] != '\0') {
        return env_root;
    }
    return CCT_STDLIB_DIR;
}

static char* mod_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);
    return copy;
}

static char* mod_read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long lsize = ftell(file);
    if (lsize < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);

    size_t size = (size_t)lsize;
    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    fclose(file);
    if (bytes_read != size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static char* mod_realpath_dup(const char *path) {
    if (!path || path[0] == '\0') return NULL;

#ifdef _WIN32
    char stack_buf[PATH_MAX];
    if (_fullpath(stack_buf, path, PATH_MAX)) {
        return mod_strdup(stack_buf);
    }
    return NULL;
#else
    char *resolved = realpath(path, NULL);
    if (resolved) return resolved;

    char stack_buf[PATH_MAX];
    if (realpath(path, stack_buf)) {
        return mod_strdup(stack_buf);
    }

    return NULL;
#endif
}

static char* mod_dirname_dup(const char *path) {
    if (!path || path[0] == '\0') return mod_strdup(".");

    const char *slash = strrchr(path, '/');
    if (!slash) return mod_strdup(".");
    if (slash == path) return mod_strdup("/");

    size_t len = (size_t)(slash - path);
    char *dir = (char*)malloc(len + 1);
    if (!dir) return NULL;
    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

static char* mod_join_paths(const char *left, const char *right) {
    if (!left || !right) return NULL;

    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    bool need_slash = (left_len > 0 && left[left_len - 1] != '/');

    size_t total = left_len + (need_slash ? 1 : 0) + right_len + 1;
    char *out = (char*)malloc(total);
    if (!out) return NULL;

    memcpy(out, left, left_len);
    size_t pos = left_len;
    if (need_slash) out[pos++] = '/';
    memcpy(out + pos, right, right_len);
    out[pos + right_len] = '\0';
    return out;
}

static char* mod_try_join_and_realpath(const char *left, const char *right) {
    char *candidate = mod_join_paths(left, right);
    if (!candidate) return NULL;
    char *resolved = mod_realpath_dup(candidate);
    free(candidate);
    return resolved;
}

static bool mod_ctx_reserve_modules(cct_module_ctx_t *ctx, size_t needed) {
    if (needed <= ctx->module_capacity) return true;

    size_t new_cap = ctx->module_capacity ? ctx->module_capacity * 2 : 8;
    while (new_cap < needed) new_cap *= 2;

    cct_module_unit_t *new_modules = (cct_module_unit_t*)realloc(ctx->modules, new_cap * sizeof(*new_modules));
    if (!new_modules) return false;

    ctx->modules = new_modules;
    ctx->module_capacity = new_cap;
    return true;
}

static bool mod_ctx_reserve_stack(cct_module_ctx_t *ctx, size_t needed) {
    if (needed <= ctx->active_capacity) return true;

    size_t new_cap = ctx->active_capacity ? ctx->active_capacity * 2 : 8;
    while (new_cap < needed) new_cap *= 2;

    const char **new_stack = (const char**)realloc((void*)ctx->active_stack, new_cap * sizeof(*new_stack));
    if (!new_stack) return false;

    ctx->active_stack = new_stack;
    ctx->active_capacity = new_cap;
    return true;
}

static bool mod_ctx_reserve_globals(cct_module_ctx_t *ctx, size_t needed) {
    if (needed <= ctx->global_capacity) return true;

    size_t new_cap = ctx->global_capacity ? ctx->global_capacity * 2 : 16;
    while (new_cap < needed) new_cap *= 2;

    cct_module_global_symbol_t *new_globals = (cct_module_global_symbol_t*)realloc(
        ctx->globals,
        new_cap * sizeof(*new_globals)
    );
    if (!new_globals) return false;

    ctx->globals = new_globals;
    ctx->global_capacity = new_cap;
    return true;
}

static bool mod_unit_reserve_imports(cct_module_unit_t *unit, size_t needed) {
    if (needed <= unit->import_capacity) return true;

    size_t new_cap = unit->import_capacity ? unit->import_capacity * 2 : 4;
    while (new_cap < needed) new_cap *= 2;

    size_t *new_imports = (size_t*)realloc(unit->import_indices, new_cap * sizeof(*new_imports));
    if (!new_imports) return false;

    unit->import_indices = new_imports;
    unit->import_capacity = new_cap;
    return true;
}

static bool mod_unit_has_import(const cct_module_unit_t *unit, size_t module_index) {
    if (!unit) return false;
    for (size_t i = 0; i < unit->import_count; i++) {
        if (unit->import_indices[i] == module_index) return true;
    }
    return false;
}

static bool mod_unit_add_import(cct_module_unit_t *unit, size_t module_index) {
    if (!unit) return false;
    if (mod_unit_has_import(unit, module_index)) return true;

    if (!mod_unit_reserve_imports(unit, unit->import_count + 1)) {
        return false;
    }
    unit->import_indices[unit->import_count++] = module_index;
    return true;
}

static ssize_t mod_ctx_find_module(const cct_module_ctx_t *ctx, const char *canonical_path) {
    for (size_t i = 0; i < ctx->module_count; i++) {
        if (ctx->modules[i].canonical_path && strcmp(ctx->modules[i].canonical_path, canonical_path) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static ssize_t mod_ctx_find_stack(const cct_module_ctx_t *ctx, const char *canonical_path) {
    for (size_t i = 0; i < ctx->active_count; i++) {
        if (ctx->active_stack[i] && strcmp(ctx->active_stack[i], canonical_path) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static ssize_t mod_ctx_find_global_symbol(const cct_module_ctx_t *ctx, const char *name) {
    if (!ctx || !name) return -1;
    for (size_t i = 0; i < ctx->global_count; i++) {
        if (ctx->globals[i].name && strcmp(ctx->globals[i].name, name) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static bool mod_is_builtin_type_name(const char *name) {
    if (!name) return false;
    return strcmp(name, "NIHIL") == 0 ||
           strcmp(name, "VERUM") == 0 ||
           strcmp(name, "VERBUM") == 0 ||
           strcmp(name, "FRACTUM") == 0 ||
           strcmp(name, "REX") == 0 ||
           strcmp(name, "DUX") == 0 ||
           strcmp(name, "COMES") == 0 ||
           strcmp(name, "MILES") == 0 ||
           strcmp(name, "UMBRA") == 0 ||
           strcmp(name, "FLAMMA") == 0;
}

static void mod_ctx_cleanup_modules(cct_module_ctx_t *ctx) {
    if (!ctx) return;

    for (size_t i = 0; i < ctx->module_count; i++) {
        cct_module_unit_t *m = &ctx->modules[i];
        if (m->program) cct_ast_free_program(m->program);
        free(m->canonical_path);
        free(m->import_indices);
    }

    for (size_t i = 0; i < ctx->global_count; i++) {
        free(ctx->globals[i].name);
    }

    free(ctx->modules);
    free(ctx->globals);
    free((void*)ctx->active_stack);

    ctx->modules = NULL;
    ctx->active_stack = NULL;
    ctx->module_count = 0;
    ctx->module_capacity = 0;
    ctx->globals = NULL;
    ctx->global_count = 0;
    ctx->global_capacity = 0;
    ctx->active_count = 0;
    ctx->active_capacity = 0;
}

static bool mod_ctx_add_module(cct_module_ctx_t *ctx,
                               char *canonical_path,
                               cct_ast_program_t *program,
                               cct_module_origin_t origin) {
    if (!mod_ctx_reserve_modules(ctx, ctx->module_count + 1)) {
        return false;
    }

    cct_module_unit_t *slot = &ctx->modules[ctx->module_count++];
    memset(slot, 0, sizeof(*slot));
    slot->canonical_path = canonical_path;
    slot->program = program;
    slot->origin = origin;
    return true;
}

static bool mod_ctx_push_active(cct_module_ctx_t *ctx, const char *canonical_path) {
    if (!mod_ctx_reserve_stack(ctx, ctx->active_count + 1)) {
        return false;
    }
    ctx->active_stack[ctx->active_count++] = canonical_path;
    return true;
}

static void mod_ctx_pop_active(cct_module_ctx_t *ctx) {
    if (ctx->active_count == 0) return;
    ctx->active_count--;
}

static bool mod_parse_module_file(cct_module_ctx_t *ctx, const char *canonical_path, cct_ast_program_t **out_program) {
    char *source = mod_read_file(canonical_path);
    if (!source) {
        mod_report(ctx, CCT_ERROR_FILE_READ, NULL, 0, 0,
                   "Could not open file: %s", canonical_path);
        return false;
    }

    cct_lexer_t lexer;
    cct_lexer_init(&lexer, source, canonical_path);

    cct_parser_t parser;
    cct_parser_init(&parser, &lexer, canonical_path);

    cct_ast_program_t *program = cct_parser_parse_program(&parser);
    bool had_parse_error = cct_parser_had_error(&parser);

    cct_parser_dispose(&parser);
    free(source);

    if (!program || had_parse_error) {
        if (program) cct_ast_free_program(program);
        if (!ctx->had_error) {
            ctx->had_error = true;
            ctx->error_code = CCT_ERROR_SYNTAX;
        }
        return false;
    }

    *out_program = program;
    return true;
}

static char* mod_resolve_import_target(const char *importer_canonical_path,
                                       const char *raw_import,
                                       cct_module_origin_t *out_origin) {
    if (!raw_import || raw_import[0] == '\0') return NULL;
    if (out_origin) *out_origin = CCT_MODULE_ORIGIN_USER;

    if (mod_is_stdlib_import(raw_import)) {
        const char *stdlib_rel = raw_import + 4;
        if (!stdlib_rel[0]) return NULL;
        if (out_origin) *out_origin = CCT_MODULE_ORIGIN_STDLIB;

        if (mod_has_cct_extension(raw_import)) {
            return mod_try_join_and_realpath(mod_stdlib_root(), stdlib_rel);
        }

        char *flat_rel = mod_append_suffix(stdlib_rel, ".cct");
        char *resolved = NULL;
        if (flat_rel) {
            resolved = mod_try_join_and_realpath(mod_stdlib_root(), flat_rel);
            free(flat_rel);
        }
        if (resolved) return resolved;

        const char *leaf = mod_path_leaf(stdlib_rel);
        size_t nested_len = strlen(stdlib_rel) + 1 + strlen(leaf) + 4 + 1; /* / + .cct + NUL */
        char *nested_rel = (char*)malloc(nested_len);
        if (!nested_rel) return NULL;
        snprintf(nested_rel, nested_len, "%s/%s.cct", stdlib_rel, leaf);
        resolved = mod_try_join_and_realpath(mod_stdlib_root(), nested_rel);
        free(nested_rel);
        if (resolved) return resolved;

        return mod_try_join_and_realpath(mod_stdlib_root(), stdlib_rel);
    }

    char *candidate = NULL;
    if (raw_import[0] == '/') {
        candidate = mod_strdup(raw_import);
    } else {
        char *base_dir = mod_dirname_dup(importer_canonical_path);
        if (!base_dir) return NULL;
        candidate = mod_join_paths(base_dir, raw_import);
        free(base_dir);
    }

    if (!candidate) return NULL;
    char *resolved = mod_realpath_dup(candidate);
    free(candidate);
    return resolved;
}

static ssize_t mod_load_recursive(cct_module_ctx_t *ctx,
                                  const char *requested_path,
                                  const char *importer_filename,
                                  const cct_ast_node_t *import_node,
                                  cct_module_origin_t request_origin) {
    char *canonical = mod_realpath_dup(requested_path);
    if (!canonical) {
        if (import_node && importer_filename) {
            const char *raw_import = import_node->as.import.filename ? import_node->as.import.filename : "<null>";
            if (request_origin == CCT_MODULE_ORIGIN_STDLIB) {
                mod_report(ctx, CCT_ERROR_FILE_NOT_FOUND,
                           importer_filename, import_node->line, import_node->column,
                           "ADVOCARE target not found in Bibliotheca Canonica: %s "
                           "(cct/... is reserved namespace; subset 11A foundation)",
                           raw_import);
            } else {
                mod_report(ctx, CCT_ERROR_FILE_NOT_FOUND,
                           importer_filename, import_node->line, import_node->column,
                           "ADVOCARE target not found: %s (subset 9A)",
                           raw_import);
            }
        } else {
            mod_report(ctx, CCT_ERROR_FILE_NOT_FOUND, NULL, 0, 0,
                       "Could not open file: %s", requested_path ? requested_path : "<null>");
        }
        return -1;
    }

    ssize_t active_idx = mod_ctx_find_stack(ctx, canonical);
    if (active_idx >= 0) {
        if (import_node && importer_filename) {
            char cycle_desc[1024];
            size_t off = 0;
            off += (size_t)snprintf(cycle_desc + off, sizeof(cycle_desc) - off, "%s", ctx->active_stack[(size_t)active_idx]);
            for (size_t i = (size_t)active_idx + 1; i < ctx->active_count && off < sizeof(cycle_desc); i++) {
                off += (size_t)snprintf(cycle_desc + off, sizeof(cycle_desc) - off, " -> %s", ctx->active_stack[i]);
            }
            if (off < sizeof(cycle_desc)) {
                (void)snprintf(cycle_desc + off, sizeof(cycle_desc) - off, " -> %s", canonical);
            }
            mod_report(ctx, CCT_ERROR_SYNTAX,
                       importer_filename, import_node->line, import_node->column,
                       "ADVOCARE cycle detected (subset 9A): %s", cycle_desc);
        } else {
            mod_report(ctx, CCT_ERROR_SYNTAX, NULL, 0, 0,
                       "ADVOCARE cycle detected (subset 9A): %s", canonical);
        }
        free(canonical);
        return -1;
    }

    ssize_t existing_idx = mod_ctx_find_module(ctx, canonical);
    if (existing_idx >= 0) {
        free(canonical);
        return existing_idx;
    }

    cct_ast_program_t *program = NULL;
    if (!mod_parse_module_file(ctx, canonical, &program)) {
        free(canonical);
        return -1;
    }

    if (!mod_ctx_add_module(ctx, canonical, program, request_origin)) {
        mod_report(ctx, CCT_ERROR_OUT_OF_MEMORY, NULL, 0, 0,
                   "Out of memory while loading ADVOCARE module closure");
        free(canonical);
        cct_ast_free_program(program);
        return -1;
    }

    size_t current_module_index = ctx->module_count - 1;
    const char *module_path = ctx->modules[current_module_index].canonical_path;
    if (!mod_ctx_push_active(ctx, module_path)) {
        mod_report(ctx, CCT_ERROR_OUT_OF_MEMORY, NULL, 0, 0,
                   "Out of memory while tracking ADVOCARE dependency stack");
        return -1;
    }

    cct_ast_node_list_t *decls = program->declarations;
    if (decls) {
        for (size_t i = 0; i < decls->count; i++) {
            cct_ast_node_t *decl = decls->nodes[i];
            if (!decl || decl->type != AST_IMPORT) continue;

            ctx->import_edge_count++;

            const char *raw_import = decl->as.import.filename;
            if (!raw_import || raw_import[0] == '\0') {
                mod_report(ctx, CCT_ERROR_SYNTAX,
                           module_path, decl->line, decl->column,
                           "ADVOCARE requires non-empty .cct path string (subset 9A)");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (!mod_has_cct_extension(raw_import) && !mod_is_stdlib_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SYNTAX,
                           module_path, decl->line, decl->column,
                           "ADVOCARE path must end with .cct in subset 9A (got '%s')",
                           raw_import);
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_kernel_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/kernel disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_console_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/console disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_mem_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/mem_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_verbum_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/verbum_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_fluxus_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/fluxus_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_irq_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/irq_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_keyboard_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/keyboard_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_timer_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/timer_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile != CCT_PROFILE_FREESTANDING && mod_is_cct_shell_fs_import(raw_import)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC,
                           module_path, decl->line, decl->column,
                           "cct/shell_fs disponível apenas em perfil freestanding");
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (ctx->profile == CCT_PROFILE_FREESTANDING) {
                char forbidden_module[64];
                if (mod_is_freestanding_forbidden_import(raw_import, forbidden_module, sizeof(forbidden_module))) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC,
                               module_path, decl->line, decl->column,
                               "módulo '%s' não disponível em perfil freestanding",
                               forbidden_module);
                    mod_ctx_pop_active(ctx);
                    return -1;
                }
            }

            cct_module_origin_t import_origin = CCT_MODULE_ORIGIN_USER;
            char *resolved_import = mod_resolve_import_target(module_path, raw_import, &import_origin);
            if (!resolved_import) {
                if (import_origin == CCT_MODULE_ORIGIN_STDLIB || mod_is_stdlib_import(raw_import)) {
                    mod_report(ctx, CCT_ERROR_FILE_NOT_FOUND,
                               module_path, decl->line, decl->column,
                               "ADVOCARE target not found in Bibliotheca Canonica: %s "
                               "(cct/... is reserved namespace; subset 11A foundation)",
                               raw_import);
                } else {
                    mod_report(ctx, CCT_ERROR_FILE_NOT_FOUND,
                               module_path, decl->line, decl->column,
                               "ADVOCARE target not found: %s (subset 9A)", raw_import);
                }
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (strcmp(resolved_import, module_path) == 0) {
                mod_report(ctx, CCT_ERROR_SYNTAX,
                           module_path, decl->line, decl->column,
                           "ADVOCARE self-import is invalid in subset 9A: %s",
                           raw_import);
                free(resolved_import);
                mod_ctx_pop_active(ctx);
                return -1;
            }

            ssize_t imported_idx = mod_load_recursive(ctx, resolved_import, module_path, decl, import_origin);
            free(resolved_import);
            if (imported_idx < 0) {
                mod_ctx_pop_active(ctx);
                return -1;
            }

            if (!mod_unit_add_import(&ctx->modules[current_module_index], (size_t)imported_idx)) {
                mod_report(ctx, CCT_ERROR_OUT_OF_MEMORY,
                           module_path, decl->line, decl->column,
                           "Out of memory while recording ADVOCARE import edges (subset 9B)");
                mod_ctx_pop_active(ctx);
                return -1;
            }
        }
    }

    mod_ctx_pop_active(ctx);
    return (ssize_t)current_module_index;
}

static const cct_module_global_symbol_t* mod_find_global_symbol_by_kind(
    const cct_module_ctx_t *ctx,
    const char *name,
    cct_module_global_kind_t kind
) {
    if (!ctx || !name) return NULL;
    for (size_t i = 0; i < ctx->global_count; i++) {
        const cct_module_global_symbol_t *sym = &ctx->globals[i];
        if (sym->kind == kind && sym->name && strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

static bool mod_module_can_reference_owner(
    const cct_module_ctx_t *ctx,
    size_t consumer_module_index,
    size_t owner_module_index
) {
    if (!ctx || consumer_module_index >= ctx->module_count || owner_module_index >= ctx->module_count) return false;
    if (consumer_module_index == owner_module_index) return true;
    return mod_unit_has_import(&ctx->modules[consumer_module_index], owner_module_index);
}

static bool mod_symbol_is_externally_visible(
    const cct_module_global_symbol_t *sym,
    size_t consumer_module_index
) {
    if (!sym) return false;
    if (sym->owner_module_index == consumer_module_index) return true;
    return !sym->is_internal;
}

static bool mod_ctx_register_global_symbol(
    cct_module_ctx_t *ctx,
    const char *name,
    cct_module_global_kind_t kind,
    bool is_internal,
    size_t type_param_count,
    const cct_ast_node_t *decl_node,
    size_t owner_module_index,
    u32 line,
    u32 column
) {
    if (!ctx || !name || !name[0]) return true;

    ssize_t existing_idx = mod_ctx_find_global_symbol(ctx, name);
    if (existing_idx >= 0) {
        const cct_module_global_symbol_t *existing = &ctx->globals[(size_t)existing_idx];
        if (existing->owner_module_index == owner_module_index) {
            /* Keep same-module duplicate diagnostics in semantic pass. */
            return true;
        }
        const char *existing_module = ctx->modules[existing->owner_module_index].canonical_path;
        const char *new_module = ctx->modules[owner_module_index].canonical_path;
        mod_report(ctx, CCT_ERROR_SEMANTIC, new_module, line, column,
                   "duplicate global symbol '%s' between modules '%s' and '%s' (subset 9B requires global uniqueness)",
                   name,
                   existing_module ? existing_module : "<unknown>",
                   new_module ? new_module : "<unknown>");
        return false;
    }

    if (!mod_ctx_reserve_globals(ctx, ctx->global_count + 1)) {
        mod_report(ctx, CCT_ERROR_OUT_OF_MEMORY, NULL, 0, 0,
                   "Out of memory while building inter-module symbol table (subset 9B)");
        return false;
    }

    cct_module_global_symbol_t *slot = &ctx->globals[ctx->global_count++];
    memset(slot, 0, sizeof(*slot));
    slot->name = mod_strdup(name);
    slot->kind = kind;
    slot->is_internal = is_internal;
    slot->type_param_count = type_param_count;
    slot->decl_node = decl_node;
    slot->owner_module_index = owner_module_index;
    slot->line = line;
    slot->column = column;
    if (!slot->name) {
        mod_report(ctx, CCT_ERROR_OUT_OF_MEMORY, NULL, 0, 0,
                   "Out of memory while interning global symbol '%s' (subset 9B)", name);
        return false;
    }
    return true;
}

static void mod_collect_globals_in_decl(cct_module_ctx_t *ctx, size_t owner_module_index, const cct_ast_node_t *decl);

static void mod_collect_globals_in_list(cct_module_ctx_t *ctx, size_t owner_module_index, const cct_ast_node_list_t *list) {
    if (!ctx || !list) return;
    for (size_t i = 0; i < list->count; i++) {
        mod_collect_globals_in_decl(ctx, owner_module_index, list->nodes[i]);
    }
}

static void mod_collect_globals_in_decl(cct_module_ctx_t *ctx, size_t owner_module_index, const cct_ast_node_t *decl) {
    if (!ctx || !decl || ctx->had_error) return;

    switch (decl->type) {
        case AST_RITUALE:
            if (decl->is_internal) ctx->internal_symbol_count++;
            else ctx->public_symbol_count++;
            (void)mod_ctx_register_global_symbol(
                ctx, decl->as.rituale.name, MOD_GLOBAL_RITUALE, decl->is_internal,
                decl->as.rituale.type_params ? decl->as.rituale.type_params->count : 0,
                decl,
                owner_module_index, decl->line, decl->column
            );
            return;
        case AST_SIGILLUM:
            if (decl->is_internal) ctx->internal_symbol_count++;
            else ctx->public_symbol_count++;
            (void)mod_ctx_register_global_symbol(
                ctx, decl->as.sigillum.name, MOD_GLOBAL_TYPE, decl->is_internal,
                decl->as.sigillum.type_params ? decl->as.sigillum.type_params->count : 0,
                decl,
                owner_module_index, decl->line, decl->column
            );
            return;
        case AST_ORDO:
            if (decl->is_internal) ctx->internal_symbol_count++;
            else ctx->public_symbol_count++;
            (void)mod_ctx_register_global_symbol(
                ctx, decl->as.ordo.name, MOD_GLOBAL_TYPE, decl->is_internal,
                0,
                decl,
                owner_module_index, decl->line, decl->column
            );
            if (decl->as.ordo.items) {
                for (size_t i = 0; i < decl->as.ordo.items->count; i++) {
                    cct_ast_enum_item_t *item = decl->as.ordo.items->items[i];
                    if (!item) continue;
                    (void)mod_ctx_register_global_symbol(
                        ctx, item->name, MOD_GLOBAL_ENUM_ITEM, decl->is_internal,
                        0,
                        decl,
                        owner_module_index, item->line, item->column
                    );
                }
            }
            return;
        case AST_PACTUM:
            (void)mod_ctx_register_global_symbol(
                ctx, decl->as.pactum.name, MOD_GLOBAL_PACTUM, false,
                0,
                decl,
                owner_module_index, decl->line, decl->column
            );
            return;
        case AST_CODEX:
            mod_collect_globals_in_list(ctx, owner_module_index, decl->as.codex.declarations);
            return;
        default:
            return;
    }
}

static bool mod_validate_type_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const cct_ast_type_t *type,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params,
    const char *filename,
    u32 line,
    u32 column,
    const char *context
) {
    if (!type || ctx->had_error) return !ctx->had_error;
    (void)context;

    if (type->is_pointer || type->is_array) {
        return mod_validate_type_reference(
            ctx, module_index, type->element_type, local_type_params, outer_type_params,
            filename, line, column, context
        );
    }
    bool has_type_args = type->generic_args && type->generic_args->count > 0;
    if (!type->name || mod_is_builtin_type_name(type->name)) {
        if (has_type_args) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                       "GENUS(...) applied to non-generic builtin type '%s' (subset 10B)",
                       type->name ? type->name : "<builtin>");
            return false;
        }
        return true;
    }

    if (local_type_params) {
        for (size_t i = 0; i < local_type_params->count; i++) {
            cct_ast_type_param_t *tp = local_type_params->params[i];
            if (tp && tp->name && strcmp(tp->name, type->name) == 0) {
                if (has_type_args) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                               "GENUS(...) cannot be applied to type parameter '%s' (subset 10B)",
                               type->name);
                    return false;
                }
                return true;
            }
        }
    }
    if (outer_type_params) {
        for (size_t i = 0; i < outer_type_params->count; i++) {
            cct_ast_type_param_t *tp = outer_type_params->params[i];
            if (tp && tp->name && strcmp(tp->name, type->name) == 0) {
                if (has_type_args) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                               "GENUS(...) cannot be applied to type parameter '%s' (subset 10B)",
                               type->name);
                    return false;
                }
                return true;
            }
        }
    }

    const cct_module_global_symbol_t *sym = mod_find_global_symbol_by_kind(ctx, type->name, MOD_GLOBAL_TYPE);
    if (!sym) {
        /* Leave unknown-type diagnostics to semantic analysis so local GENUS/type-scope
         * errors in FASE 10A keep coherent semantic wording. */
        return true;
    }

    if (has_type_args) {
        if (sym->type_param_count == 0) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                       "GENUS(...) applied to non-generic type '%s' (subset 10B)",
                       type->name);
            return false;
        }
        if (type->generic_args->count != sym->type_param_count) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                       "generic type '%s' expects %zu type argument(s), got %zu (subset 10B)",
                       type->name, sym->type_param_count, type->generic_args->count);
            return false;
        }
        for (size_t i = 0; i < type->generic_args->count; i++) {
            if (!mod_validate_type_reference(
                    ctx, module_index, type->generic_args->types[i], local_type_params, outer_type_params,
                    filename, line, column, "GENUS type argument"
                )) {
                return false;
            }
        }
    } else if (sym->type_param_count > 0) {
        mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                   "generic type '%s' requires explicit GENUS(...) instantiation in subset 10B",
                   type->name);
        return false;
    }

    if (!mod_module_can_reference_owner(ctx, module_index, sym->owner_module_index)) {
        mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                   "type '%s' comes from module '%s' but '%s' does not directly import it (subset 9B has no transitive visibility)",
                   type->name,
                   ctx->modules[sym->owner_module_index].canonical_path ? ctx->modules[sym->owner_module_index].canonical_path : "<unknown>",
                   filename ? filename : "<unknown>");
        return false;
    }

    if (!mod_symbol_is_externally_visible(sym, module_index)) {
        mod_report(ctx, CCT_ERROR_SEMANTIC, filename, line, column,
                   "type '%s' is ARCANUM/internal in module '%s' and cannot be accessed from '%s' (subset 9C visibility boundary)",
                   type->name,
                   ctx->modules[sym->owner_module_index].canonical_path ? ctx->modules[sym->owner_module_index].canonical_path : "<unknown>",
                   filename ? filename : "<unknown>");
        return false;
    }

    if (sym->owner_module_index != module_index) {
        ctx->cross_module_type_ref_count++;
    }
    return true;
}

static bool mod_validate_expr_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *expr,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params
);
static bool mod_validate_stmt_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *stmt,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params
);
static bool mod_validate_decl_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *decl,
    const cct_ast_type_param_list_t *outer_type_params
);

static bool mod_validate_rituale_constraints_decl_10d(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *rituale_decl
) {
    if (!ctx || !rituale_decl || rituale_decl->type != AST_RITUALE || !rituale_decl->as.rituale.type_params) {
        return true;
    }

    for (size_t i = 0; i < rituale_decl->as.rituale.type_params->count; i++) {
        cct_ast_type_param_t *tp = rituale_decl->as.rituale.type_params->params[i];
        if (!tp || !tp->constraint_pactum_name || !tp->constraint_pactum_name[0]) continue;

        const cct_module_global_symbol_t *psym = mod_find_global_symbol_by_kind(
            ctx, tp->constraint_pactum_name, MOD_GLOBAL_PACTUM
        );
        if (!psym) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, tp->line, tp->column,
                       "GENUS constraint '%s PACTUM %s' references unknown contract (subset 10D; subset final da FASE 10)",
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name);
            return false;
        }

        if (!mod_module_can_reference_owner(ctx, module_index, psym->owner_module_index)) {
            const char *owner = ctx->modules[psym->owner_module_index].canonical_path;
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, tp->line, tp->column,
                       "GENUS constraint '%s PACTUM %s' is defined in module '%s' but not directly imported by '%s' (subset 10D has no transitive visibility for constraints; subset final da FASE 10)",
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name,
                       owner ? owner : "<unknown>",
                       filename ? filename : "<unknown>");
            return false;
        }

        if (!mod_symbol_is_externally_visible(psym, module_index)) {
            const char *owner = ctx->modules[psym->owner_module_index].canonical_path;
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, tp->line, tp->column,
                       "GENUS constraint '%s PACTUM %s' is ARCANUM/internal in module '%s' and cannot be accessed from '%s' (subset 10D visibility boundary; subset final da FASE 10)",
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name,
                       owner ? owner : "<unknown>",
                       filename ? filename : "<unknown>");
            return false;
        }

        if (psym->owner_module_index != module_index) {
            ctx->cross_module_type_ref_count++;
        }
    }

    return true;
}

static bool mod_validate_coniura_constraints_10d(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *expr,
    const cct_module_global_symbol_t *rituale_sym
) {
    if (!ctx || !expr || !rituale_sym || !expr->as.coniura.type_args ||
        !rituale_sym->decl_node || rituale_sym->decl_node->type != AST_RITUALE ||
        !rituale_sym->decl_node->as.rituale.type_params) {
        return true;
    }

    size_t n = expr->as.coniura.type_args->count;
    size_t pcount = rituale_sym->decl_node->as.rituale.type_params->count;
    if (n > pcount) n = pcount;

    for (size_t i = 0; i < n; i++) {
        cct_ast_type_param_t *tp = rituale_sym->decl_node->as.rituale.type_params->params[i];
        if (!tp || !tp->constraint_pactum_name || !tp->constraint_pactum_name[0]) continue;

        cct_ast_type_t *arg = expr->as.coniura.type_args->types[i];
        if (!arg || arg->is_pointer || arg->is_array || !arg->name ||
            mod_is_builtin_type_name(arg->name) ||
            (arg->generic_args && arg->generic_args->count > 0)) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "type argument for '%s PACTUM %s' must be named SIGILLUM (subset 10D; subset final da FASE 10)",
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name);
            return false;
        }

        const cct_module_global_symbol_t *psym = mod_find_global_symbol_by_kind(
            ctx, tp->constraint_pactum_name, MOD_GLOBAL_PACTUM
        );
        if (!psym) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "constraint contract '%s' is not declared (subset 10D; subset final da FASE 10)",
                       tp->constraint_pactum_name);
            return false;
        }
        if (!mod_module_can_reference_owner(ctx, module_index, psym->owner_module_index)) {
            const char *owner = ctx->modules[psym->owner_module_index].canonical_path;
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "constraint contract '%s' is defined in module '%s' but not directly imported by '%s' (subset 10D keeps 9B no transitive visibility; subset final da FASE 10)",
                       tp->constraint_pactum_name,
                       owner ? owner : "<unknown>",
                       filename ? filename : "<unknown>");
            return false;
        }
        if (!mod_symbol_is_externally_visible(psym, module_index)) {
            const char *owner = ctx->modules[psym->owner_module_index].canonical_path;
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "constraint contract '%s' is ARCANUM/internal in module '%s' and cannot be accessed from '%s' (subset 10D visibility boundary; subset final da FASE 10)",
                       tp->constraint_pactum_name,
                       owner ? owner : "<unknown>",
                       filename ? filename : "<unknown>");
            return false;
        }

        const cct_module_global_symbol_t *tsym = mod_find_global_symbol_by_kind(ctx, arg->name, MOD_GLOBAL_TYPE);
        if (!tsym || !tsym->decl_node || tsym->decl_node->type != AST_SIGILLUM) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "type argument '%s' for '%s PACTUM %s' must be named SIGILLUM (subset 10D; subset final da FASE 10)",
                       arg->name ? arg->name : "<type>",
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name);
            return false;
        }

        const char *provided = tsym->decl_node->as.sigillum.pactum_name;
        if (!provided || strcmp(provided, tp->constraint_pactum_name) != 0) {
            mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                       "type argument '%s' does not satisfy '%s PACTUM %s' (subset 10D requires explicit SIGILLUM ... PACTUM ... conformance; subset final da FASE 10)",
                       arg->name,
                       tp->name ? tp->name : "<T>",
                       tp->constraint_pactum_name);
            return false;
        }
    }

    return true;
}

static bool mod_validate_expr_list(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_list_t *list,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params
) {
    if (!list) return true;
    for (size_t i = 0; i < list->count; i++) {
        if (!mod_validate_expr_reference(
                ctx, module_index, filename, list->nodes[i], local_type_params, outer_type_params
            )) {
            return false;
        }
    }
    return true;
}

static bool mod_validate_expr_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *expr,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params
) {
    if (!expr || ctx->had_error) return !ctx->had_error;

    switch (expr->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_REAL:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
        case AST_LITERAL_NIHIL:
        case AST_IDENTIFIER:
            return true;

        case AST_BINARY_OP:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, expr->as.binary_op.left, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, expr->as.binary_op.right, local_type_params, outer_type_params
                   );

        case AST_UNARY_OP:
            return mod_validate_expr_reference(
                ctx, module_index, filename, expr->as.unary_op.operand, local_type_params, outer_type_params
            );

        case AST_CALL:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, expr->as.call.callee, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_list(
                       ctx, module_index, filename, expr->as.call.arguments, local_type_params, outer_type_params
                   );

        case AST_CONIURA: {
            if (expr->as.coniura.name && strcmp(expr->as.coniura.name, "__cast") == 0) {
                bool ok = true;
                if (!expr->as.coniura.type_args || expr->as.coniura.type_args->count != 1) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                               "cast requires exactly one target type in GENUS(T) (subset 12B)");
                    ok = false;
                } else {
                    ok = mod_validate_type_reference(
                        ctx,
                        module_index,
                        expr->as.coniura.type_args->types[0],
                        local_type_params,
                        outer_type_params,
                        filename,
                        expr->line,
                        expr->column,
                        "cast target type"
                    ) && ok;
                }
                if (!expr->as.coniura.arguments || expr->as.coniura.arguments->count != 1) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                               "cast requires exactly one source expression (subset 12B)");
                    ok = false;
                } else {
                    ok = mod_validate_expr_reference(
                        ctx,
                        module_index,
                        filename,
                        expr->as.coniura.arguments->nodes[0],
                        local_type_params,
                        outer_type_params
                    ) && ok;
                }
                return ok;
            }

            const cct_module_global_symbol_t *sym = mod_find_global_symbol_by_kind(
                ctx, expr->as.coniura.name, MOD_GLOBAL_RITUALE
            );
            if (!sym) {
                const char *name = expr->as.coniura.name ? expr->as.coniura.name : "<null>";
                const char *closest = mod_find_rituale_suggestion(ctx, name);
                const char *stdlib_hint = mod_stdlib_import_hint_for_symbol(name);
                char suggestion[256];
                const char *hint = NULL;
                if (closest) {
                    snprintf(suggestion, sizeof(suggestion), "did you mean '%s'?", closest);
                    hint = suggestion;
                } else if (stdlib_hint) {
                    hint = stdlib_hint;
                }

                mod_report_with_suggestion(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column, hint,
                                           "rituale '%s' is not declared (undefined rituale symbol in module '%s', subset 9B inter-module resolution)",
                                           name,
                                           filename ? filename : "<unknown>");
                return false;
            }

            if (!mod_module_can_reference_owner(ctx, module_index, sym->owner_module_index)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                           "rituale '%s' is defined in module '%s' but not directly imported by '%s' (subset 9B has no transitive visibility)",
                           expr->as.coniura.name ? expr->as.coniura.name : "<null>",
                           ctx->modules[sym->owner_module_index].canonical_path ? ctx->modules[sym->owner_module_index].canonical_path : "<unknown>",
                           filename ? filename : "<unknown>");
                return false;
            }

            if (!mod_symbol_is_externally_visible(sym, module_index)) {
                mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                           "rituale '%s' is ARCANUM/internal in module '%s' and cannot be accessed from '%s' (subset 9C visibility boundary)",
                           expr->as.coniura.name ? expr->as.coniura.name : "<null>",
                           ctx->modules[sym->owner_module_index].canonical_path ? ctx->modules[sym->owner_module_index].canonical_path : "<unknown>",
                           filename ? filename : "<unknown>");
                return false;
            }

            if (sym->owner_module_index != module_index) {
                ctx->cross_module_call_count++;
            }

            bool has_type_args = expr->as.coniura.type_args && expr->as.coniura.type_args->count > 0;
            if (sym->type_param_count > 0) {
                if (!has_type_args) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                               "generic rituale '%s' requires explicit GENUS(...) instantiation in subset 10B (subset final da FASE 10 has no type-arg inference)",
                               expr->as.coniura.name ? expr->as.coniura.name : "<null>");
                    return false;
                }
                if (expr->as.coniura.type_args->count != sym->type_param_count) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                               "generic rituale '%s' expects %zu type argument(s), got %zu (subset 10B)",
                               expr->as.coniura.name ? expr->as.coniura.name : "<null>",
                               sym->type_param_count,
                               expr->as.coniura.type_args->count);
                    return false;
                }
                for (size_t i = 0; i < expr->as.coniura.type_args->count; i++) {
                    if (!mod_validate_type_reference(
                            ctx, module_index, expr->as.coniura.type_args->types[i],
                            local_type_params, outer_type_params,
                            filename, expr->line, expr->column, "CONIURA GENUS argument"
                        )) {
                        return false;
                    }
                }
            } else if (has_type_args) {
                mod_report(ctx, CCT_ERROR_SEMANTIC, filename, expr->line, expr->column,
                           "GENUS(...) applied to non-generic rituale '%s' (subset 10B)",
                           expr->as.coniura.name ? expr->as.coniura.name : "<null>");
                return false;
            }
            if (!mod_validate_coniura_constraints_10d(ctx, module_index, filename, expr, sym)) {
                return false;
            }
            return mod_validate_expr_list(
                ctx, module_index, filename, expr->as.coniura.arguments, local_type_params, outer_type_params
            );
        }

        case AST_OBSECRO:
            return mod_validate_expr_list(
                ctx, module_index, filename, expr->as.obsecro.arguments, local_type_params, outer_type_params
            );

        case AST_FIELD_ACCESS:
            return mod_validate_expr_reference(
                ctx, module_index, filename, expr->as.field_access.object, local_type_params, outer_type_params
            );

        case AST_INDEX_ACCESS:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, expr->as.index_access.array, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, expr->as.index_access.index, local_type_params, outer_type_params
                   );

        case AST_MENSURA:
            return mod_validate_type_reference(
                ctx, module_index, expr->as.mensura.type, local_type_params, outer_type_params,
                filename, expr->line, expr->column, "MENSURA"
            );

        default:
            return true;
    }
}

static bool mod_validate_stmt_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *stmt,
    const cct_ast_type_param_list_t *local_type_params,
    const cct_ast_type_param_list_t *outer_type_params
) {
    if (!stmt || ctx->had_error) return !ctx->had_error;

    switch (stmt->type) {
        case AST_BLOCK:
            if (!stmt->as.block.statements) return true;
            for (size_t i = 0; i < stmt->as.block.statements->count; i++) {
                if (!mod_validate_stmt_reference(
                        ctx, module_index, filename, stmt->as.block.statements->nodes[i],
                        local_type_params, outer_type_params
                    )) {
                    return false;
                }
            }
            return true;

        case AST_EVOCA:
            return mod_validate_type_reference(
                       ctx, module_index, stmt->as.evoca.var_type, local_type_params, outer_type_params,
                       filename, stmt->line, stmt->column, "EVOCA"
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.evoca.initializer, local_type_params, outer_type_params
                   );

        case AST_VINCIRE:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.vincire.target, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.vincire.value, local_type_params, outer_type_params
                   );

        case AST_REDDE:
            return mod_validate_expr_reference(
                ctx, module_index, filename, stmt->as.redde.value, local_type_params, outer_type_params
            );

        case AST_SI:
            if (!mod_validate_expr_reference(
                    ctx, module_index, filename, stmt->as.si.condition, local_type_params, outer_type_params
                )) {
                    return false;
                }
            if (!mod_validate_stmt_reference(
                    ctx, module_index, filename, stmt->as.si.then_branch, local_type_params, outer_type_params
                )) {
                return false;
            }
            return mod_validate_stmt_reference(
                ctx, module_index, filename, stmt->as.si.else_branch, local_type_params, outer_type_params
            );

        case AST_DUM:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.dum.condition, local_type_params, outer_type_params
                   ) &&
                   mod_validate_stmt_reference(
                       ctx, module_index, filename, stmt->as.dum.body, local_type_params, outer_type_params
                   );

        case AST_DONEC:
            return mod_validate_stmt_reference(
                       ctx, module_index, filename, stmt->as.donec.body, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.donec.condition, local_type_params, outer_type_params
                   );

        case AST_REPETE:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.repete.start, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.repete.end, local_type_params, outer_type_params
                   ) &&
                   mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.repete.step, local_type_params, outer_type_params
                   ) &&
                   mod_validate_stmt_reference(
                       ctx, module_index, filename, stmt->as.repete.body, local_type_params, outer_type_params
                   );

        case AST_ITERUM:
            return mod_validate_expr_reference(
                       ctx, module_index, filename, stmt->as.iterum.collection, local_type_params, outer_type_params
                   ) &&
                   mod_validate_stmt_reference(
                       ctx, module_index, filename, stmt->as.iterum.body, local_type_params, outer_type_params
                   );

        case AST_TEMPTA:
            if (!mod_validate_stmt_reference(
                    ctx, module_index, filename, stmt->as.tempta.try_block, local_type_params, outer_type_params
                )) {
                return false;
            }
            if (!mod_validate_type_reference(
                    ctx, module_index, stmt->as.tempta.cape_type, local_type_params, outer_type_params,
                    filename, stmt->line, stmt->column, "CAPE"
                )) {
                return false;
            }
            if (!mod_validate_stmt_reference(
                    ctx, module_index, filename, stmt->as.tempta.cape_block, local_type_params, outer_type_params
                )) {
                return false;
            }
            return mod_validate_stmt_reference(
                ctx, module_index, filename, stmt->as.tempta.semper_block, local_type_params, outer_type_params
            );

        case AST_IACE:
            return mod_validate_expr_reference(
                ctx, module_index, filename, stmt->as.iace.value, local_type_params, outer_type_params
            );

        case AST_DIMITTE:
            return mod_validate_expr_reference(
                ctx, module_index, filename, stmt->as.dimitte.target, local_type_params, outer_type_params
            );

        case AST_EXPR_STMT:
            return mod_validate_expr_reference(
                ctx, module_index, filename, stmt->as.expr_stmt.expression, local_type_params, outer_type_params
            );

        case AST_ANUR:
            return mod_validate_expr_reference(
                ctx, module_index, filename, stmt->as.anur.value, local_type_params, outer_type_params
            );

        case AST_FRANGE:
        case AST_RECEDE:
        case AST_TRANSITUS:
            return true;

        default:
            return true;
    }
}

static bool mod_validate_decl_reference(
    cct_module_ctx_t *ctx,
    size_t module_index,
    const char *filename,
    const cct_ast_node_t *decl,
    const cct_ast_type_param_list_t *outer_type_params
) {
    if (!decl || ctx->had_error) return !ctx->had_error;

    switch (decl->type) {
        case AST_IMPORT:
        case AST_ORDO:
            return true;

        case AST_RITUALE: {
            const cct_ast_type_param_list_t *local_type_params = decl->as.rituale.type_params;
            if (local_type_params) {
                for (size_t i = 0; i < local_type_params->count; i++) {
                    cct_ast_type_param_t *a = local_type_params->params[i];
                    if (!a || !a->name) continue;
                    for (size_t j = i + 1; j < local_type_params->count; j++) {
                        cct_ast_type_param_t *b = local_type_params->params[j];
                        if (b && b->name && strcmp(a->name, b->name) == 0) {
                            mod_report(ctx, CCT_ERROR_SYNTAX, filename, b->line, b->column,
                                       "duplicate GENUS parameter '%s' in RITUALE declaration (subset 10A)",
                                       b->name);
                            return false;
                        }
                    }
                }
            }
            if (!mod_validate_rituale_constraints_decl_10d(ctx, module_index, filename, decl)) {
                return false;
            }
            if (!mod_validate_type_reference(
                    ctx, module_index, decl->as.rituale.return_type, local_type_params, outer_type_params,
                    filename, decl->line, decl->column, "RITUALE return"
                )) {
                return false;
            }
            if (decl->as.rituale.params) {
                for (size_t i = 0; i < decl->as.rituale.params->count; i++) {
                    cct_ast_param_t *param = decl->as.rituale.params->params[i];
                    if (!param) continue;
                    if (!mod_validate_type_reference(
                            ctx, module_index, param->type, local_type_params, outer_type_params,
                            filename, param->line, param->column, "RITUALE parameter"
                        )) {
                        return false;
                    }
                }
            }
            return mod_validate_stmt_reference(
                ctx, module_index, filename, decl->as.rituale.body, local_type_params, outer_type_params
            );
        }

        case AST_SIGILLUM:
            if (decl->as.sigillum.pactum_name && decl->as.sigillum.pactum_name[0]) {
                const cct_module_global_symbol_t *psym = mod_find_global_symbol_by_kind(
                    ctx, decl->as.sigillum.pactum_name, MOD_GLOBAL_PACTUM
                );
                if (!psym) {
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, decl->line, decl->column,
                               "PACTUM '%s' is not declared (subset 10C inter-module conformance)",
                               decl->as.sigillum.pactum_name);
                    return false;
                }
                if (!mod_module_can_reference_owner(ctx, module_index, psym->owner_module_index)) {
                    const char *owner_path = ctx->modules[psym->owner_module_index].canonical_path;
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, decl->line, decl->column,
                               "PACTUM '%s' is defined in module '%s' but not directly imported by '%s' (subset 10C has no transitive visibility for contracts)",
                               decl->as.sigillum.pactum_name,
                               owner_path ? owner_path : "<unknown>",
                               filename ? filename : "<unknown>");
                    return false;
                }
                if (!mod_symbol_is_externally_visible(psym, module_index)) {
                    const char *owner_path = ctx->modules[psym->owner_module_index].canonical_path;
                    mod_report(ctx, CCT_ERROR_SEMANTIC, filename, decl->line, decl->column,
                               "PACTUM '%s' is ARCANUM/internal in module '%s' and cannot be accessed from '%s' (subset 10C visibility boundary)",
                               decl->as.sigillum.pactum_name,
                               owner_path ? owner_path : "<unknown>",
                               filename ? filename : "<unknown>");
                    return false;
                }
                if (psym->owner_module_index != module_index) {
                    ctx->cross_module_type_ref_count++;
                }
            }
            if (decl->as.sigillum.type_params) {
                for (size_t i = 0; i < decl->as.sigillum.type_params->count; i++) {
                    cct_ast_type_param_t *a = decl->as.sigillum.type_params->params[i];
                    if (!a || !a->name) continue;
                    for (size_t j = i + 1; j < decl->as.sigillum.type_params->count; j++) {
                        cct_ast_type_param_t *b = decl->as.sigillum.type_params->params[j];
                        if (b && b->name && strcmp(a->name, b->name) == 0) {
                            mod_report(ctx, CCT_ERROR_SYNTAX, filename, b->line, b->column,
                                       "duplicate GENUS parameter '%s' in SIGILLUM declaration (subset 10A)",
                                       b->name);
                            return false;
                        }
                    }
                }
            }
            if (decl->as.sigillum.fields) {
                for (size_t i = 0; i < decl->as.sigillum.fields->count; i++) {
                    cct_ast_field_t *field = decl->as.sigillum.fields->fields[i];
                    if (!field) continue;
                    if (!mod_validate_type_reference(
                            ctx, module_index, field->type, decl->as.sigillum.type_params, outer_type_params,
                            filename, field->line, field->column, "SIGILLUM field"
                        )) {
                        return false;
                    }
                }
            }
            if (decl->as.sigillum.methods) {
                for (size_t i = 0; i < decl->as.sigillum.methods->count; i++) {
                    if (!mod_validate_decl_reference(
                            ctx, module_index, filename, decl->as.sigillum.methods->nodes[i], decl->as.sigillum.type_params
                        )) {
                        return false;
                    }
                }
            }
            return true;

        case AST_CODEX:
            if (!decl->as.codex.declarations) return true;
            for (size_t i = 0; i < decl->as.codex.declarations->count; i++) {
                if (!mod_validate_decl_reference(
                        ctx, module_index, filename, decl->as.codex.declarations->nodes[i], outer_type_params
                    )) {
                    return false;
                }
            }
            return true;

        case AST_PACTUM:
            if (!decl->as.pactum.signatures) return true;
            for (size_t i = 0; i < decl->as.pactum.signatures->count; i++) {
                if (!mod_validate_decl_reference(
                        ctx, module_index, filename, decl->as.pactum.signatures->nodes[i], outer_type_params
                    )) {
                    return false;
                }
            }
            return true;

        default:
            return true;
    }
}

static bool mod_validate_inter_module_resolution(cct_module_ctx_t *ctx) {
    if (!ctx) return false;

    ctx->cross_module_call_count = 0;
    ctx->cross_module_type_ref_count = 0;
    ctx->public_symbol_count = 0;
    ctx->internal_symbol_count = 0;

    for (size_t i = 0; i < ctx->module_count; i++) {
        cct_module_unit_t *unit = &ctx->modules[i];
        if (!unit->program || !unit->program->declarations) continue;
        mod_collect_globals_in_list(ctx, i, unit->program->declarations);
        if (ctx->had_error) return false;
    }

    for (size_t i = 0; i < ctx->module_count; i++) {
        const cct_module_unit_t *unit = &ctx->modules[i];
        const char *filename = unit->canonical_path;
        if (!unit->program || !unit->program->declarations) continue;
        for (size_t j = 0; j < unit->program->declarations->count; j++) {
            if (!mod_validate_decl_reference(ctx, i, filename, unit->program->declarations->nodes[j], NULL)) {
                return false;
            }
        }
    }

    return !ctx->had_error;
}

bool cct_module_bundle_build(const char *entry_input_path,
                             cct_profile_t profile,
                             cct_module_bundle_t *out_bundle,
                             cct_error_code_t *out_error) {
    if (out_error) *out_error = CCT_OK;
    if (!entry_input_path || !out_bundle) {
        if (out_error) *out_error = CCT_ERROR_INVALID_ARGUMENT;
        return false;
    }

    memset(out_bundle, 0, sizeof(*out_bundle));

    cct_module_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.error_code = CCT_OK;
    ctx.profile = profile;

    ssize_t entry_idx = mod_load_recursive(&ctx, entry_input_path, NULL, NULL, CCT_MODULE_ORIGIN_USER);
    if (entry_idx < 0 || ctx.module_count == 0) {
        if (!ctx.error_code) ctx.error_code = CCT_ERROR_INTERNAL;
        if (out_error) *out_error = ctx.error_code;
        mod_ctx_cleanup_modules(&ctx);
        return false;
    }

    if (!mod_validate_inter_module_resolution(&ctx)) {
        if (!ctx.error_code) ctx.error_code = CCT_ERROR_SEMANTIC;
        if (out_error) *out_error = ctx.error_code;
        mod_ctx_cleanup_modules(&ctx);
        return false;
    }

    const char *entry_name = ctx.modules[0].program && ctx.modules[0].program->name
        ? ctx.modules[0].program->name
        : "composed";
    cct_ast_program_t *composed = cct_ast_create_program(entry_name);

    for (size_t i = 0; i < ctx.module_count; i++) {
        cct_ast_program_t *mod_prog = ctx.modules[i].program;
        if (!mod_prog || !mod_prog->declarations) continue;

        cct_ast_node_list_t *decls = mod_prog->declarations;
        for (size_t j = 0; j < decls->count; j++) {
            cct_ast_node_t *decl = decls->nodes[j];
            if (!decl) continue;

            if (decl->type == AST_IMPORT) {
                cct_ast_free_node(decl);
            } else {
                cct_ast_node_list_append(composed->declarations, decl);
            }
            decls->nodes[j] = NULL;
        }
    }

    out_bundle->program = composed;
    out_bundle->module_count = (u32)ctx.module_count;
    out_bundle->import_edge_count = ctx.import_edge_count;
    out_bundle->entry_module_path = mod_strdup(entry_input_path);
    out_bundle->module_paths = NULL;
    out_bundle->module_path_count = (u32)ctx.module_count;
    out_bundle->import_from_indices = NULL;
    out_bundle->import_to_indices = NULL;
    out_bundle->import_graph_edge_count = 0;
    out_bundle->module_origins = NULL;
    out_bundle->stdlib_module_count = 0;
    out_bundle->cross_module_call_count = ctx.cross_module_call_count;
    out_bundle->cross_module_type_ref_count = ctx.cross_module_type_ref_count;
    out_bundle->module_resolution_ok = true;
    out_bundle->public_symbol_count = ctx.public_symbol_count;
    out_bundle->internal_symbol_count = ctx.internal_symbol_count;

    if (!out_bundle->entry_module_path) {
        cct_ast_free_program(composed);
        out_bundle->program = NULL;
        out_bundle->module_count = 0;
        out_bundle->import_edge_count = 0;
        out_bundle->module_paths = NULL;
        out_bundle->module_path_count = 0;
        out_bundle->import_from_indices = NULL;
        out_bundle->import_to_indices = NULL;
        out_bundle->import_graph_edge_count = 0;
        out_bundle->module_origins = NULL;
        out_bundle->stdlib_module_count = 0;
        out_bundle->cross_module_call_count = 0;
        out_bundle->cross_module_type_ref_count = 0;
        out_bundle->module_resolution_ok = false;
        out_bundle->public_symbol_count = 0;
        out_bundle->internal_symbol_count = 0;
        if (out_error) *out_error = CCT_ERROR_OUT_OF_MEMORY;
        mod_ctx_cleanup_modules(&ctx);
        return false;
    }

    if (ctx.module_count > 0) {
        out_bundle->module_paths = (char**)calloc(ctx.module_count, sizeof(char*));
        out_bundle->module_origins = (cct_module_origin_t*)calloc(ctx.module_count, sizeof(cct_module_origin_t));
        if (!out_bundle->module_paths || !out_bundle->module_origins) {
            cct_module_bundle_dispose(out_bundle);
            if (out_error) *out_error = CCT_ERROR_OUT_OF_MEMORY;
            mod_ctx_cleanup_modules(&ctx);
            return false;
        }

        for (size_t i = 0; i < ctx.module_count; i++) {
            out_bundle->module_paths[i] = mod_strdup(ctx.modules[i].canonical_path);
            if (!out_bundle->module_paths[i]) {
                cct_module_bundle_dispose(out_bundle);
                if (out_error) *out_error = CCT_ERROR_OUT_OF_MEMORY;
                mod_ctx_cleanup_modules(&ctx);
                return false;
            }
            out_bundle->module_origins[i] = ctx.modules[i].origin;
            if (ctx.modules[i].origin == CCT_MODULE_ORIGIN_STDLIB) {
                out_bundle->stdlib_module_count++;
            }
        }
    }

    size_t graph_edge_count = 0;
    for (size_t i = 0; i < ctx.module_count; i++) {
        graph_edge_count += ctx.modules[i].import_count;
    }
    out_bundle->import_graph_edge_count = (u32)graph_edge_count;

    if (graph_edge_count > 0) {
        out_bundle->import_from_indices = (u32*)calloc(graph_edge_count, sizeof(u32));
        out_bundle->import_to_indices = (u32*)calloc(graph_edge_count, sizeof(u32));
        if (!out_bundle->import_from_indices || !out_bundle->import_to_indices) {
            cct_module_bundle_dispose(out_bundle);
            if (out_error) *out_error = CCT_ERROR_OUT_OF_MEMORY;
            mod_ctx_cleanup_modules(&ctx);
            return false;
        }

        size_t edge_write = 0;
        for (size_t from = 0; from < ctx.module_count; from++) {
            cct_module_unit_t *unit = &ctx.modules[from];
            for (size_t ei = 0; ei < unit->import_count; ei++) {
                out_bundle->import_from_indices[edge_write] = (u32)from;
                out_bundle->import_to_indices[edge_write] = (u32)unit->import_indices[ei];
                edge_write++;
            }
        }
    }

    if (out_error) *out_error = CCT_OK;
    mod_ctx_cleanup_modules(&ctx);
    return true;
}

void cct_module_bundle_dispose(cct_module_bundle_t *bundle) {
    if (!bundle) return;
    if (bundle->program) cct_ast_free_program(bundle->program);
    free(bundle->entry_module_path);
    if (bundle->module_paths) {
        for (u32 i = 0; i < bundle->module_path_count; i++) {
            free(bundle->module_paths[i]);
        }
    }
    free(bundle->module_paths);
    free(bundle->module_origins);
    free(bundle->import_from_indices);
    free(bundle->import_to_indices);
    memset(bundle, 0, sizeof(*bundle));
}
