/*
 * CCT — Clavicula Turing
 * PostgreSQL runtime helper emission
 *
 * FASE 36A: cct/db_postgres host-runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_postgres_helpers(FILE *out) {
    if (!out) return false;

    fputs("typedef struct {\n", out);
    fputs("    void *lib_handle;\n", out);
    fputs("    int attempted;\n", out);
    fputs("    int available;\n", out);
    fputs("    char last_error[256];\n", out);
    fputs("    void *(*PQconnectdb_fn)(const char *);\n", out);
    fputs("    void (*PQfinish_fn)(void *);\n", out);
    fputs("    char *(*PQerrorMessage_fn)(const void *);\n", out);
    fputs("    int (*PQstatus_fn)(const void *);\n", out);
    fputs("    void *(*PQexec_fn)(void *, const char *);\n", out);
    fputs("    int (*PQresultStatus_fn)(const void *);\n", out);
    fputs("    void (*PQclear_fn)(void *);\n", out);
    fputs("    int (*PQntuples_fn)(const void *);\n", out);
    fputs("    int (*PQnfields_fn)(const void *);\n", out);
    fputs("    char *(*PQgetvalue_fn)(const void *, int, int);\n", out);
    fputs("    int (*PQgetisnull_fn)(const void *, int, int);\n", out);
    fputs("    int (*PQconsumeInput_fn)(void *);\n", out);
    fputs("    int (*PQsocket_fn)(const void *);\n", out);
    fputs("    void *(*PQnotifies_fn)(void *);\n", out);
    fputs("    void (*PQfreemem_fn)(void *);\n", out);
    fputs("} cct_rt_pg_api_t;\n\n", out);

    fputs("typedef struct {\n", out);
    fputs("    char *relname;\n", out);
    fputs("    int be_pid;\n", out);
    fputs("    char *extra;\n", out);
    fputs("} cct_rt_pg_notify_t;\n\n", out);

    fputs("typedef struct {\n", out);
    fputs("    void *conn;\n", out);
    fputs("    int open;\n", out);
    fputs("    char last_error[512];\n", out);
    fputs("    char last_payload[1024];\n", out);
    fputs("} cct_rt_pg_db_t;\n\n", out);

    fputs("typedef struct {\n", out);
    fputs("    cct_rt_pg_db_t *owner;\n", out);
    fputs("    void *result;\n", out);
    fputs("    int row_count;\n", out);
    fputs("    int column_count;\n", out);
    fputs("    int current_row;\n", out);
    fputs("} cct_rt_pg_rows_t;\n\n", out);

    fputs("static cct_rt_pg_api_t cct_rt_pg_api;\n\n", out);

    fputs("static void cct_rt_pg_copy_error(char *dst, size_t cap, const char *msg) {\n", out);
    fputs("    const char *src = msg ? msg : \"\";\n", out);
    fputs("    size_t n = strlen(src);\n", out);
    fputs("    if (cap == 0) return;\n", out);
    fputs("    if (n >= cap) n = cap - 1U;\n", out);
    fputs("    memcpy(dst, src, n);\n", out);
    fputs("    dst[n] = '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_pg_dup_cstr(const char *s) {\n", out);
    fputs("    const char *src = s ? s : \"\";\n", out);
    fputs("    size_t n = strlen(src);\n", out);
    fputs("    char *buf = (char*)cct_rt_alloc_or_fail(n + 1U);\n", out);
    fputs("    memcpy(buf, src, n + 1U);\n", out);
    fputs("    return buf;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_set_api_error(const char *msg) {\n", out);
    fputs("    cct_rt_pg_copy_error(cct_rt_pg_api.last_error, sizeof(cct_rt_pg_api.last_error), msg);\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_pg_load_api(void) {\n", out);
    fputs("    if (cct_rt_pg_api.attempted) return cct_rt_pg_api.available;\n", out);
    fputs("    cct_rt_pg_api.attempted = 1;\n", out);
    fputs("    cct_rt_pg_api.available = 0;\n", out);
    fputs("    cct_rt_pg_api.last_error[0] = '\\0';\n", out);
    fputs("#ifdef _WIN32\n", out);
    fputs("    cct_rt_pg_set_api_error(\"PostgreSQL indisponivel neste target\");\n", out);
    fputs("    return 0;\n", out);
    fputs("#else\n", out);
    fputs("#ifdef __APPLE__\n", out);
    fputs("    const char *candidates[] = {\"libpq.5.dylib\", \"libpq.dylib\", NULL};\n", out);
    fputs("#else\n", out);
    fputs("    const char *candidates[] = {\"libpq.so.5\", \"libpq.so\", NULL};\n", out);
    fputs("#endif\n", out);
    fputs("    for (size_t i = 0; candidates[i]; i++) {\n", out);
    fputs("        cct_rt_pg_api.lib_handle = dlopen(candidates[i], RTLD_LAZY | RTLD_LOCAL);\n", out);
    fputs("        if (cct_rt_pg_api.lib_handle) break;\n", out);
    fputs("    }\n", out);
    fputs("    if (!cct_rt_pg_api.lib_handle) {\n", out);
    fputs("        cct_rt_pg_set_api_error(\"libpq nao encontrada no sistema\");\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("#define CCT_RT_PG_LOAD(field, name) do { \\\n", out);
    fputs("        cct_rt_pg_api.field = dlsym(cct_rt_pg_api.lib_handle, name); \\\n", out);
    fputs("        if (!cct_rt_pg_api.field) { \\\n", out);
    fputs("            cct_rt_pg_set_api_error(\"simbolo libpq ausente: \" name); \\\n", out);
    fputs("            return 0; \\\n", out);
    fputs("        } \\\n", out);
    fputs("    } while (0)\n", out);
    fputs("    CCT_RT_PG_LOAD(PQconnectdb_fn, \"PQconnectdb\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQfinish_fn, \"PQfinish\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQerrorMessage_fn, \"PQerrorMessage\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQstatus_fn, \"PQstatus\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQexec_fn, \"PQexec\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQresultStatus_fn, \"PQresultStatus\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQclear_fn, \"PQclear\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQntuples_fn, \"PQntuples\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQnfields_fn, \"PQnfields\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQgetvalue_fn, \"PQgetvalue\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQgetisnull_fn, \"PQgetisnull\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQconsumeInput_fn, \"PQconsumeInput\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQsocket_fn, \"PQsocket\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQnotifies_fn, \"PQnotifies\");\n", out);
    fputs("    CCT_RT_PG_LOAD(PQfreemem_fn, \"PQfreemem\");\n", out);
    fputs("#undef CCT_RT_PG_LOAD\n", out);
    fputs("    cct_rt_pg_api.available = 1;\n", out);
    fputs("    return 1;\n", out);
    fputs("#endif\n", out);
    fputs("}\n\n", out);

    fputs("static cct_rt_pg_db_t *cct_rt_pg_require_db(void *db_ptr, const char *ctx) {\n", out);
    fputs("    cct_rt_pg_db_t *db = (cct_rt_pg_db_t*)db_ptr;\n", out);
    fputs("    if (!db) cct_rt_fail((ctx && *ctx) ? ctx : \"postgres db nulo\");\n", out);
    fputs("    return db;\n", out);
    fputs("}\n\n", out);

    fputs("static cct_rt_pg_rows_t *cct_rt_pg_require_rows(void *rows_ptr, const char *ctx) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = (cct_rt_pg_rows_t*)rows_ptr;\n", out);
    fputs("    if (!rows) cct_rt_fail((ctx && *ctx) ? ctx : \"postgres rows nulo\");\n", out);
    fputs("    return rows;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_db_set_error(cct_rt_pg_db_t *db, const char *msg) {\n", out);
    fputs("    if (!db) return;\n", out);
    fputs("    cct_rt_pg_copy_error(db->last_error, sizeof(db->last_error), msg);\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_db_clear_error(cct_rt_pg_db_t *db) {\n", out);
    fputs("    if (!db) return;\n", out);
    fputs("    db->last_error[0] = '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_pg_db_open(const char *conninfo) {\n", out);
    fputs("    cct_rt_pg_db_t *db = (cct_rt_pg_db_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_pg_db_t));\n", out);
    fputs("    db->conn = NULL;\n", out);
    fputs("    db->open = 0;\n", out);
    fputs("    db->last_error[0] = '\\0';\n", out);
    fputs("    db->last_payload[0] = '\\0';\n", out);
    fputs("    if (!cct_rt_pg_load_api()) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.last_error);\n", out);
    fputs("        return db;\n", out);
    fputs("    }\n", out);
    fputs("    db->conn = cct_rt_pg_api.PQconnectdb_fn((conninfo && *conninfo) ? conninfo : \"\");\n", out);
    fputs("    if (!db->conn) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, \"PQconnectdb falhou\");\n", out);
    fputs("        return db;\n", out);
    fputs("    }\n", out);
    fputs("    if (cct_rt_pg_api.PQstatus_fn(db->conn) != 0) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        cct_rt_pg_api.PQfinish_fn(db->conn);\n", out);
    fputs("        db->conn = NULL;\n", out);
    fputs("        return db;\n", out);
    fputs("    }\n", out);
    fputs("    db->open = 1;\n", out);
    fputs("    return db;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_pg_db_is_open(void *db_ptr) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_is_open recebeu db nulo\");\n", out);
    fputs("    return (db->open && db->conn) ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_pg_db_last_error(void *db_ptr) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_last_error recebeu db nulo\");\n", out);
    fputs("    return cct_rt_pg_dup_cstr(db->last_error);\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_db_close(void *db_ptr) {\n", out);
    fputs("    cct_rt_pg_db_t *db = (cct_rt_pg_db_t*)db_ptr;\n", out);
    fputs("    if (!db) return;\n", out);
    fputs("    if (db->conn && cct_rt_pg_load_api()) cct_rt_pg_api.PQfinish_fn(db->conn);\n", out);
    fputs("    db->conn = NULL;\n", out);
    fputs("    db->open = 0;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_pg_result_ok(int status) {\n", out);
    fputs("    return status == 1 || status == 2 || status == 9;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_db_exec(void *db_ptr, const char *sql) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_exec recebeu db nulo\");\n", out);
    fputs("    if (!db->open || !db->conn) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, \"postgres conexao fechada\");\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    void *result = cct_rt_pg_api.PQexec_fn(db->conn, (sql && *sql) ? sql : \"\");\n", out);
    fputs("    if (!result) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    if (!cct_rt_pg_result_ok(cct_rt_pg_api.PQresultStatus_fn(result))) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        cct_rt_pg_api.PQclear_fn(result);\n", out);
    fputs("        return;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_pg_api.PQclear_fn(result);\n", out);
    fputs("    cct_rt_pg_db_clear_error(db);\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_pg_db_query(void *db_ptr, const char *sql) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_query recebeu db nulo\");\n", out);
    fputs("    cct_rt_pg_rows_t *rows = (cct_rt_pg_rows_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_pg_rows_t));\n", out);
    fputs("    rows->owner = db;\n", out);
    fputs("    rows->result = NULL;\n", out);
    fputs("    rows->row_count = 0;\n", out);
    fputs("    rows->column_count = 0;\n", out);
    fputs("    rows->current_row = -1;\n", out);
    fputs("    if (!db->open || !db->conn) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, \"postgres conexao fechada\");\n", out);
    fputs("        return rows;\n", out);
    fputs("    }\n", out);
    fputs("    rows->result = cct_rt_pg_api.PQexec_fn(db->conn, (sql && *sql) ? sql : \"\");\n", out);
    fputs("    if (!rows->result) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        return rows;\n", out);
    fputs("    }\n", out);
    fputs("    if (!cct_rt_pg_result_ok(cct_rt_pg_api.PQresultStatus_fn(rows->result))) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        cct_rt_pg_api.PQclear_fn(rows->result);\n", out);
    fputs("        rows->result = NULL;\n", out);
    fputs("        return rows;\n", out);
    fputs("    }\n", out);
    fputs("    rows->row_count = cct_rt_pg_api.PQntuples_fn(rows->result);\n", out);
    fputs("    rows->column_count = cct_rt_pg_api.PQnfields_fn(rows->result);\n", out);
    fputs("    cct_rt_pg_db_clear_error(db);\n", out);
    fputs("    return rows;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_pg_rows_count(void *rows_ptr) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = cct_rt_pg_require_rows(rows_ptr, \"pg_builtin_rows_count recebeu rows nulo\");\n", out);
    fputs("    return (long long)rows->row_count;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_pg_rows_columns(void *rows_ptr) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = cct_rt_pg_require_rows(rows_ptr, \"pg_builtin_rows_columns recebeu rows nulo\");\n", out);
    fputs("    return (long long)rows->column_count;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_pg_rows_next(void *rows_ptr) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = cct_rt_pg_require_rows(rows_ptr, \"pg_builtin_rows_next recebeu rows nulo\");\n", out);
    fputs("    if (!rows->result) return 0LL;\n", out);
    fputs("    rows->current_row += 1;\n", out);
    fputs("    return rows->current_row < rows->row_count ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_pg_rows_valid(cct_rt_pg_rows_t *rows, long long col) {\n", out);
    fputs("    return rows && rows->result && rows->current_row >= 0 && rows->current_row < rows->row_count && col >= 0 && col < rows->column_count;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_pg_rows_get_text(void *rows_ptr, long long col) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = cct_rt_pg_require_rows(rows_ptr, \"pg_builtin_rows_get_text recebeu rows nulo\");\n", out);
    fputs("    if (!cct_rt_pg_rows_valid(rows, col)) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    if (cct_rt_pg_api.PQgetisnull_fn(rows->result, rows->current_row, (int)col)) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    return cct_rt_pg_dup_cstr(cct_rt_pg_api.PQgetvalue_fn(rows->result, rows->current_row, (int)col));\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_pg_rows_is_null(void *rows_ptr, long long col) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = cct_rt_pg_require_rows(rows_ptr, \"pg_builtin_rows_is_null recebeu rows nulo\");\n", out);
    fputs("    if (!cct_rt_pg_rows_valid(rows, col)) return 1LL;\n", out);
    fputs("    return cct_rt_pg_api.PQgetisnull_fn(rows->result, rows->current_row, (int)col) ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_pg_rows_close(void *rows_ptr) {\n", out);
    fputs("    cct_rt_pg_rows_t *rows = (cct_rt_pg_rows_t*)rows_ptr;\n", out);
    fputs("    if (!rows) return;\n", out);
    fputs("    if (rows->result && cct_rt_pg_load_api()) cct_rt_pg_api.PQclear_fn(rows->result);\n", out);
    fputs("    rows->result = NULL;\n", out);
    fputs("    cct_rt_free_ptr(rows);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_pg_db_poll_channel(void *db_ptr, long long timeout_ms) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_poll_channel recebeu db nulo\");\n", out);
    fputs("    db->last_payload[0] = '\\0';\n", out);
    fputs("    if (!db->open || !db->conn) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    int fd = cct_rt_pg_api.PQsocket_fn(db->conn);\n", out);
    fputs("    if (fd < 0) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    fd_set rfds;\n", out);
    fputs("    FD_ZERO(&rfds);\n", out);
    fputs("    FD_SET(fd, &rfds);\n", out);
    fputs("    struct timeval tv;\n", out);
    fputs("    struct timeval *tv_ptr = NULL;\n", out);
    fputs("    if (timeout_ms >= 0) {\n", out);
    fputs("        tv.tv_sec = (time_t)(timeout_ms / 1000LL);\n", out);
    fputs("        tv.tv_usec = (suseconds_t)((timeout_ms % 1000LL) * 1000LL);\n", out);
    fputs("        tv_ptr = &tv;\n", out);
    fputs("    }\n", out);
    fputs("    if (select(fd + 1, &rfds, NULL, NULL, tv_ptr) <= 0) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    if (!cct_rt_pg_api.PQconsumeInput_fn(db->conn)) {\n", out);
    fputs("        cct_rt_pg_db_set_error(db, cct_rt_pg_api.PQerrorMessage_fn(db->conn));\n", out);
    fputs("        return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_pg_notify_t *notify = (cct_rt_pg_notify_t*)cct_rt_pg_api.PQnotifies_fn(db->conn);\n", out);
    fputs("    if (!notify) return cct_rt_pg_dup_cstr(\"\");\n", out);
    fputs("    cct_rt_pg_copy_error(db->last_payload, sizeof(db->last_payload), notify->extra ? notify->extra : \"\");\n", out);
    fputs("    char *channel = cct_rt_pg_dup_cstr(notify->relname ? notify->relname : \"\");\n", out);
    fputs("    cct_rt_pg_api.PQfreemem_fn(notify);\n", out);
    fputs("    return channel;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_pg_db_poll_payload(void *db_ptr) {\n", out);
    fputs("    cct_rt_pg_db_t *db = cct_rt_pg_require_db(db_ptr, \"pg_builtin_poll_payload recebeu db nulo\");\n", out);
    fputs("    return cct_rt_pg_dup_cstr(db->last_payload);\n", out);
    fputs("}\n\n", out);

    return true;
}
