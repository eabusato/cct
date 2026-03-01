#include "fs_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    FILE *f = cct_rt_fs_open_or_fail(path, "wb", "fs write_all open failed");
    const char *src = content ? content : "";
    size_t len = strlen(src);
    if (len > 0 && fwrite(src, 1, len, f) != len) {
        fclose(f);
        cct_rt_fs_abort("fs write_all write failed");
    }
    fclose(f);
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
