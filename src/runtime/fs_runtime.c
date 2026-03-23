/*
 * CCT — Clavicula Turing
 * Filesystem Runtime Implementation
 *
 * FASE 10A: Host runtime support modules
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "fs_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define cct_rt_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define cct_rt_mkdir(path) mkdir((path), 0777)
#endif

static void cct_rt_fs_abort(const char *msg) {
    fprintf(stderr, "cct runtime: %s\n", msg ? msg : "fs failure");
    exit(1);
}

static FILE* cct_rt_fs_open_or_fail(const char *path, const char *mode, const char *ctx) {
    const char *p = path ? path : "";
    FILE *f = fopen(p, mode);
    if (!f) cct_rt_fs_abort(ctx);
    return f;
}

static void cct_rt_fs_abort1(const char *fmt, const char *a) {
    char msg[1024];
    const char *sa = a ? a : "";
    snprintf(msg, sizeof(msg), fmt ? fmt : "fs failure", sa);
    cct_rt_fs_abort(msg);
}

static int cct_rt_fs_is_dir_local(const char *path) {
    struct stat st;
    if (stat(path ? path : "", &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

static char* cct_rt_fs_make_atomic_tmp_path(const char *path) {
    static unsigned long counter = 0;
    const char *p = path ? path : "";
    size_t len = strlen(p);
    char *tmp = (char*)malloc(len + 64U);
    if (!tmp) cct_rt_fs_abort("fs write_all tmp allocation failed");
    snprintf(tmp, len + 64U, "%s.tmp.%ld.%lu", p, (long)getpid(), ++counter);
    return tmp;
}

char* cct_rt_fs_read_all(const char *path) {
    FILE *f = cct_rt_fs_open_or_fail(path, "rb", "fs read_all open failed");
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        cct_rt_fs_abort("fs read_all seek-end failed");
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        cct_rt_fs_abort("fs read_all tell failed");
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        cct_rt_fs_abort("fs read_all seek-set failed");
    }

    char *buf = (char*)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        cct_rt_fs_abort("fs read_all allocation failed");
    }

    size_t read_n = fread(buf, 1, (size_t)size, f);
    if (read_n != (size_t)size && ferror(f)) {
        free(buf);
        fclose(f);
        cct_rt_fs_abort("fs read_all read failed");
    }

    buf[read_n] = '\0';
    fclose(f);
    return buf;
}

void cct_rt_fs_write_all(const char *path, const char *content) {
    const char *dst = path ? path : "";
    char *tmp_path = cct_rt_fs_make_atomic_tmp_path(dst);
    FILE *f = cct_rt_fs_open_or_fail(tmp_path, "wb", "fs write_all open failed");
    const char *src = content ? content : "";
    size_t len = strlen(src);
    if (len > 0 && fwrite(src, 1, len, f) != len) {
        fclose(f);
        unlink(tmp_path);
        free(tmp_path);
        cct_rt_fs_abort("fs write_all write failed");
    }
    if (fflush(f) != 0) {
        fclose(f);
        unlink(tmp_path);
        free(tmp_path);
        cct_rt_fs_abort("fs write_all flush failed");
    }
#ifndef _WIN32
    if (fsync(fileno(f)) != 0) {
        fclose(f);
        unlink(tmp_path);
        free(tmp_path);
        cct_rt_fs_abort("fs write_all sync failed");
    }
#endif
    if (fclose(f) != 0) {
        unlink(tmp_path);
        free(tmp_path);
        cct_rt_fs_abort("fs write_all close failed");
    }
    if (rename(tmp_path, dst) != 0) {
        unlink(tmp_path);
        free(tmp_path);
        cct_rt_fs_abort("fs write_all rename failed");
    }
    free(tmp_path);
}

void cct_rt_fs_append_all(const char *path, const char *content) {
    FILE *f = cct_rt_fs_open_or_fail(path, "ab", "fs append_all open failed");
    const char *src = content ? content : "";
    size_t len = strlen(src);
    if (len > 0 && fwrite(src, 1, len, f) != len) {
        fclose(f);
        cct_rt_fs_abort("fs append_all write failed");
    }
    fclose(f);
}

void cct_rt_fs_mkdir_all(const char *path) {
    const char *src = path ? path : "";
    size_t n = strlen(src);
    if (n == 0) return;

    char *tmp = (char*)malloc(n + 1);
    if (!tmp) cct_rt_fs_abort("fs mkdir_all allocation failed");
    memcpy(tmp, src, n + 1);

    for (size_t i = 1; i < n; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char hold = tmp[i];
            tmp[i] = '\0';
            if (strlen(tmp) > 0) {
                if (cct_rt_mkdir(tmp) != 0 && errno != EEXIST) {
                    free(tmp);
                    cct_rt_fs_abort1("fs mkdir_all failed: %s", src);
                }
            }
            tmp[i] = hold;
        }
    }

    if (cct_rt_mkdir(tmp) != 0) {
        if (errno != EEXIST || !cct_rt_fs_is_dir_local(tmp)) {
            free(tmp);
            cct_rt_fs_abort1("fs mkdir_all failed: %s", src);
        }
    }

    free(tmp);
}

long long cct_rt_fs_exists(const char *path) {
    const char *p = path ? path : "";
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

long long cct_rt_fs_size(const char *path) {
    FILE *f = cct_rt_fs_open_or_fail(path, "rb", "fs size open failed");
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        cct_rt_fs_abort("fs size seek-end failed");
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        cct_rt_fs_abort("fs size tell failed");
    }
    fclose(f);
    return (long long)size;
}
