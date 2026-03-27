/*
 * CCT — Clavicula Turing
 * Image Operations Runtime Helper Emission
 *
 * FASE 32I: image operations host runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_image_ops_helpers(FILE *out) {
    if (!out) return false;

    fputs("enum {\n", out);
    fputs("    CCT_RT_IMAGE_FMT_JPEG = 0,\n", out);
    fputs("    CCT_RT_IMAGE_FMT_PNG = 1,\n", out);
    fputs("    CCT_RT_IMAGE_FMT_GIF = 2,\n", out);
    fputs("    CCT_RT_IMAGE_FMT_BMP = 3,\n", out);
    fputs("    CCT_RT_IMAGE_FMT_WEBP = 4,\n", out);
    fputs("    CCT_RT_IMAGE_FMT_UNKNOWN = 5\n", out);
    fputs("};\n\n", out);

    fputs("typedef struct {\n", out);
    fputs("    char *path;\n", out);
    fputs("    long long width;\n", out);
    fputs("    long long height;\n", out);
    fputs("    long long channels;\n", out);
    fputs("    long long format;\n", out);
    fputs("    int owns_path;\n", out);
    fputs("} cct_rt_image_t;\n\n", out);

    fputs("static char cct_rt_image_last_error_buf[512];\n", out);
    fputs("static unsigned long long cct_rt_image_counter = 0ULL;\n\n", out);

    fputs("static const char *cct_rt_image_last_error(void) {\n", out);
    fputs("    return cct_rt_image_last_error_buf;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_image_set_error(const char *msg) {\n", out);
    fputs("    snprintf(cct_rt_image_last_error_buf, sizeof(cct_rt_image_last_error_buf), \"%s\", (msg && *msg) ? msg : \"image_ops failure\");\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_image_set_errorf(const char *fmt, const char *arg) {\n", out);
    fputs("    snprintf(cct_rt_image_last_error_buf, sizeof(cct_rt_image_last_error_buf), fmt ? fmt : \"%s\", arg ? arg : \"\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_image_strdup(const char *s) {\n", out);
    fputs("    size_t n = s ? strlen(s) : 0u;\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1u);\n", out);
    fputs("    if (n > 0u) memcpy(out_s, s, n);\n", out);
    fputs("    out_s[n] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static const char *cct_rt_image_format_extension(long long fmt) {\n", out);
    fputs("    switch ((int)fmt) {\n", out);
    fputs("        case CCT_RT_IMAGE_FMT_JPEG: return \"jpg\";\n", out);
    fputs("        case CCT_RT_IMAGE_FMT_PNG: return \"png\";\n", out);
    fputs("        case CCT_RT_IMAGE_FMT_GIF: return \"gif\";\n", out);
    fputs("        case CCT_RT_IMAGE_FMT_BMP: return \"bmp\";\n", out);
    fputs("        case CCT_RT_IMAGE_FMT_WEBP: return \"webp\";\n", out);
    fputs("        default: return \"png\";\n", out);
    fputs("    }\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_image_ext_eq(const char *a, const char *b) {\n", out);
    fputs("    size_t i = 0u;\n", out);
    fputs("    if (!a || !b) return 0;\n", out);
    fputs("    while (a[i] && b[i]) {\n", out);
    fputs("        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;\n", out);
    fputs("        i++;\n", out);
    fputs("    }\n", out);
    fputs("    return a[i] == '\\0' && b[i] == '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_format_from_path(const char *path) {\n", out);
    fputs("    const char *dot = path ? strrchr(path, '.') : NULL;\n", out);
    fputs("    if (!dot || !dot[1]) return CCT_RT_IMAGE_FMT_UNKNOWN;\n", out);
    fputs("    dot++;\n", out);
    fputs("    if (cct_rt_image_ext_eq(dot, \"jpg\") || cct_rt_image_ext_eq(dot, \"jpeg\")) return CCT_RT_IMAGE_FMT_JPEG;\n", out);
    fputs("    if (cct_rt_image_ext_eq(dot, \"png\")) return CCT_RT_IMAGE_FMT_PNG;\n", out);
    fputs("    if (cct_rt_image_ext_eq(dot, \"gif\")) return CCT_RT_IMAGE_FMT_GIF;\n", out);
    fputs("    if (cct_rt_image_ext_eq(dot, \"bmp\")) return CCT_RT_IMAGE_FMT_BMP;\n", out);
    fputs("    if (cct_rt_image_ext_eq(dot, \"webp\")) return CCT_RT_IMAGE_FMT_WEBP;\n", out);
    fputs("    return CCT_RT_IMAGE_FMT_UNKNOWN;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_channels_from_pix_fmt(const char *pix_fmt) {\n", out);
    fputs("    if (!pix_fmt || !*pix_fmt) return 0;\n", out);
    fputs("    if (strstr(pix_fmt, \"rgba\") || strstr(pix_fmt, \"bgra\") || strstr(pix_fmt, \"argb\") || strstr(pix_fmt, \"abgr\") || strstr(pix_fmt, \"yuva\") || strstr(pix_fmt, \"ya\")) return 4;\n", out);
    fputs("    if (strstr(pix_fmt, \"gray\") || strstr(pix_fmt, \"mono\")) return 1;\n", out);
    fputs("    return 3;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_image_ensure_cache_dir(void) {\n", out);
    fputs("    struct stat st;\n", out);
    fputs("    if (stat(\".cct_image_ops_runtime\", &st) == 0) return S_ISDIR(st.st_mode) ? 1 : 0;\n", out);
    fputs("    if (mkdir(\".cct_image_ops_runtime\", 0755) == 0) return 1;\n", out);
    fputs("    return errno == EEXIST;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_shell_quote(const char *s) {\n", out);
    fputs("    size_t n = s ? strlen(s) : 0u;\n", out);
    fputs("    size_t extra = 2u;\n", out);
    fputs("    for (size_t i = 0; i < n; i++) extra += (s[i] == '\\'') ? 4u : 1u;\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(extra + 1u);\n", out);
    fputs("    size_t j = 0u;\n", out);
    fputs("    out_s[j++] = '\\'';\n", out);
    fputs("    for (size_t i = 0; i < n; i++) {\n", out);
    fputs("        if (s[i] == '\\'') {\n", out);
    fputs("            out_s[j++] = '\\'';\n", out);
    fputs("            out_s[j++] = '\\\\';\n", out);
    fputs("            out_s[j++] = '\\'';\n", out);
    fputs("            out_s[j++] = '\\'';\n", out);
    fputs("        } else {\n", out);
    fputs("            out_s[j++] = s[i];\n", out);
    fputs("        }\n", out);
    fputs("    }\n", out);
    fputs("    out_s[j++] = '\\'';\n", out);
    fputs("    out_s[j] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_image_run_command(const char *cmd) {\n", out);
    fputs("    int rc;\n", out);
    fputs("    if (!cmd || !*cmd) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops empty command\");\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    rc = system(cmd);\n", out);
    fputs("    if (rc != 0) {\n", out);
    fputs("        cct_rt_image_set_errorf(\"image_ops command failed: %s\", cmd);\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    return 1;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_image_make_temp_path(long long fmt) {\n", out);
    fputs("    const char *ext = cct_rt_image_format_extension(fmt);\n", out);
    fputs("    char buf[512];\n", out);
    fputs("    if (!cct_rt_image_ensure_cache_dir()) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops could not create local cache directory\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    snprintf(buf, sizeof(buf), \".cct_image_ops_runtime/img_%lld_%llu.%s\", (long long)getpid(), ++cct_rt_image_counter, ext);\n", out);
    fputs("    return cct_rt_image_strdup(buf);\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_image_probe_path(const char *path, long long *width, long long *height, long long *channels) {\n", out);
    fputs("    char *qpath = NULL;\n", out);
    fputs("    char cmd[1024];\n", out);
    fputs("    FILE *pipe = NULL;\n", out);
    fputs("    char line[256];\n", out);
    fputs("    long long w = 0;\n", out);
    fputs("    long long h = 0;\n", out);
    fputs("    long long ch = 0;\n", out);
    fputs("    int line_no = 0;\n", out);
    fputs("    if (!path || !*path) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops invalid path\");\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    qpath = cct_rt_shell_quote(path);\n", out);
    fputs("    snprintf(cmd, sizeof(cmd), \"ffprobe -v error -select_streams v:0 -show_entries stream=width,height,pix_fmt -of default=noprint_wrappers=1:nokey=1 %s 2>/dev/null\", qpath);\n", out);
    fputs("    free(qpath);\n", out);
    fputs("    pipe = popen(cmd, \"r\");\n", out);
    fputs("    if (!pipe) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops could not launch ffprobe\");\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    while (fgets(line, sizeof(line), pipe)) {\n", out);
    fputs("        size_t n = strlen(line);\n", out);
    fputs("        while (n > 0u && (line[n - 1u] == '\\n' || line[n - 1u] == '\\r')) line[--n] = '\\0';\n", out);
    fputs("        if (line_no == 0) w = atoll(line);\n", out);
    fputs("        else if (line_no == 1) h = atoll(line);\n", out);
    fputs("        else if (line_no == 2) ch = cct_rt_image_channels_from_pix_fmt(line);\n", out);
    fputs("        line_no++;\n", out);
    fputs("    }\n", out);
    fputs("    pclose(pipe);\n", out);
    fputs("    if (w <= 0 || h <= 0) {\n", out);
    fputs("        cct_rt_image_set_errorf(\"image_ops could not probe image: %s\", path);\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    if (width) *width = w;\n", out);
    fputs("    if (height) *height = h;\n", out);
    fputs("    if (channels) *channels = ch > 0 ? ch : 3;\n", out);
    fputs("    cct_rt_image_last_error_buf[0] = '\\0';\n", out);
    fputs("    return 1;\n", out);
    fputs("}\n\n", out);

    fputs("static cct_rt_image_t *cct_rt_image_alloc_from_path(const char *path, int owns_path, long long forced_fmt) {\n", out);
    fputs("    cct_rt_image_t *img = NULL;\n", out);
    fputs("    long long width = 0;\n", out);
    fputs("    long long height = 0;\n", out);
    fputs("    long long channels = 0;\n", out);
    fputs("    if (!cct_rt_image_probe_path(path, &width, &height, &channels)) return NULL;\n", out);
    fputs("    img = (cct_rt_image_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_image_t));\n", out);
    fputs("    img->path = cct_rt_image_strdup(path);\n", out);
    fputs("    img->width = width;\n", out);
    fputs("    img->height = height;\n", out);
    fputs("    img->channels = channels;\n", out);
    fputs("    img->format = forced_fmt == CCT_RT_IMAGE_FMT_UNKNOWN ? cct_rt_image_format_from_path(path) : forced_fmt;\n", out);
    fputs("    img->owns_path = owns_path;\n", out);
    fputs("    return img;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_image_free(void *ptr) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    if (!img) return;\n", out);
    fputs("    if (img->owns_path && img->path && img->path[0]) unlink(img->path);\n", out);
    fputs("    free(img->path);\n", out);
    fputs("    free(img);\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_load(const char *path) {\n", out);
    fputs("    struct stat st;\n", out);
    fputs("    cct_rt_image_last_error_buf[0] = '\\0';\n", out);
    fputs("    if (!path || !*path) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops load path vazio\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {\n", out);
    fputs("        cct_rt_image_set_errorf(\"image_ops arquivo inexistente: %s\", path);\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    return cct_rt_image_alloc_from_path(path, 0, CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_save(void *ptr, const char *path, long long quality) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    char *qsrc = NULL;\n", out);
    fputs("    char *qdst = NULL;\n", out);
    fputs("    char cmd[2048];\n", out);
    fputs("    long long fmt;\n", out);
    fputs("    if (!img || !img->path || !path || !*path) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops save recebeu imagem ou caminho invalido\");\n", out);
    fputs("        return 0;\n", out);
    fputs("    }\n", out);
    fputs("    fmt = cct_rt_image_format_from_path(path);\n", out);
    fputs("    qsrc = cct_rt_shell_quote(img->path);\n", out);
    fputs("    qdst = cct_rt_shell_quote(path);\n", out);
    fputs("    if (fmt == CCT_RT_IMAGE_FMT_JPEG) {\n", out);
    fputs("        long long qv = 31 - (quality * 29 / 100);\n", out);
    fputs("        if (qv < 2) qv = 2;\n", out);
    fputs("        if (qv > 31) qv = 31;\n", out);
    fputs("        snprintf(cmd, sizeof(cmd), \"ffmpeg -y -v error -i %s -frames:v 1 -q:v %lld %s >/dev/null 2>&1\", qsrc, qv, qdst);\n", out);
    fputs("    } else if (fmt == CCT_RT_IMAGE_FMT_PNG) {\n", out);
    fputs("        long long lvl = 9 - (quality * 9 / 100);\n", out);
    fputs("        if (lvl < 0) lvl = 0;\n", out);
    fputs("        if (lvl > 9) lvl = 9;\n", out);
    fputs("        snprintf(cmd, sizeof(cmd), \"ffmpeg -y -v error -i %s -frames:v 1 -compression_level %lld %s >/dev/null 2>&1\", qsrc, lvl, qdst);\n", out);
    fputs("    } else {\n", out);
    fputs("        snprintf(cmd, sizeof(cmd), \"ffmpeg -y -v error -i %s -frames:v 1 %s >/dev/null 2>&1\", qsrc, qdst);\n", out);
    fputs("    }\n", out);
    fputs("    free(qsrc);\n", out);
    fputs("    free(qdst);\n", out);
    fputs("    return cct_rt_image_run_command(cmd) ? 1 : 0;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_transform_basic(cct_rt_image_t *img, const char *filter, long long forced_fmt) {\n", out);
    fputs("    char *dst = NULL;\n", out);
    fputs("    char *qsrc = NULL;\n", out);
    fputs("    char *qdst = NULL;\n", out);
    fputs("    char *qfilter = NULL;\n", out);
    fputs("    char cmd[4096];\n", out);
    fputs("    if (!img || !img->path || !filter || !*filter) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops transform recebeu parametros invalidos\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    dst = cct_rt_image_make_temp_path(forced_fmt == CCT_RT_IMAGE_FMT_UNKNOWN ? img->format : forced_fmt);\n", out);
    fputs("    if (!dst) return NULL;\n", out);
    fputs("    qsrc = cct_rt_shell_quote(img->path);\n", out);
    fputs("    qdst = cct_rt_shell_quote(dst);\n", out);
    fputs("    qfilter = cct_rt_shell_quote(filter);\n", out);
    fputs("    snprintf(cmd, sizeof(cmd), \"ffmpeg -y -v error -i %s -vf %s -frames:v 1 %s >/dev/null 2>&1\", qsrc, qfilter, qdst);\n", out);
    fputs("    free(qsrc);\n", out);
    fputs("    free(qdst);\n", out);
    fputs("    free(qfilter);\n", out);
    fputs("    if (!cct_rt_image_run_command(cmd)) {\n", out);
    fputs("        unlink(dst);\n", out);
    fputs("        free(dst);\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_image_t *out_img = cct_rt_image_alloc_from_path(dst, 1, forced_fmt == CCT_RT_IMAGE_FMT_UNKNOWN ? img->format : forced_fmt);\n", out);
    fputs("    free(dst);\n", out);
    fputs("    return out_img;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_resize(void *ptr, long long width, long long height, long long mode) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    char filter[256];\n", out);
    fputs("    const char *flag = \"bicubic\";\n", out);
    fputs("    if (!img) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops resize recebeu handle nulo\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (width <= 0 || height <= 0) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops resize recebeu dimensoes invalidas\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (mode == 0) flag = \"neighbor\";\n", out);
    fputs("    else if (mode == 1) flag = \"bilinear\";\n", out);
    fputs("    else if (mode == 2) flag = \"bicubic\";\n", out);
    fputs("    else if (mode == 3) flag = \"lanczos\";\n", out);
    fputs("    snprintf(filter, sizeof(filter), \"scale=%lld:%lld:flags=%s\", width, height, flag);\n", out);
    fputs("    return cct_rt_image_transform_basic(img, filter, CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_crop(void *ptr, long long x, long long y, long long width, long long height) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    char filter[256];\n", out);
    fputs("    if (!img) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops crop recebeu handle nulo\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > img->width || y + height > img->height) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops crop fora dos limites\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    snprintf(filter, sizeof(filter), \"crop=%lld:%lld:%lld:%lld\", width, height, x, y);\n", out);
    fputs("    return cct_rt_image_transform_basic(img, filter, CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_rotate(void *ptr, long long angle) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    if (!img) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops rotate recebeu handle nulo\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (angle == 90) return cct_rt_image_transform_basic(img, \"transpose=1\", CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("    if (angle == 180) return cct_rt_image_transform_basic(img, \"transpose=1,transpose=1\", CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("    if (angle == 270) return cct_rt_image_transform_basic(img, \"transpose=2\", CCT_RT_IMAGE_FMT_UNKNOWN);\n", out);
    fputs("    cct_rt_image_set_error(\"image_ops rotate aceita apenas 90, 180 ou 270 graus\");\n", out);
    fputs("    return NULL;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_image_convert(void *ptr, long long fmt) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    char *dst = NULL;\n", out);
    fputs("    char *qsrc = NULL;\n", out);
    fputs("    char *qdst = NULL;\n", out);
    fputs("    char cmd[2048];\n", out);
    fputs("    if (!img) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops convert recebeu handle nulo\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    if (fmt < CCT_RT_IMAGE_FMT_JPEG || fmt > CCT_RT_IMAGE_FMT_WEBP) {\n", out);
    fputs("        cct_rt_image_set_error(\"image_ops convert recebeu formato invalido\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    dst = cct_rt_image_make_temp_path(fmt);\n", out);
    fputs("    if (!dst) return NULL;\n", out);
    fputs("    qsrc = cct_rt_shell_quote(img->path);\n", out);
    fputs("    qdst = cct_rt_shell_quote(dst);\n", out);
    fputs("    snprintf(cmd, sizeof(cmd), \"ffmpeg -y -v error -i %s -frames:v 1 %s >/dev/null 2>&1\", qsrc, qdst);\n", out);
    fputs("    free(qsrc);\n", out);
    fputs("    free(qdst);\n", out);
    fputs("    if (!cct_rt_image_run_command(cmd)) {\n", out);
    fputs("        unlink(dst);\n", out);
    fputs("        free(dst);\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_image_t *out_img = cct_rt_image_alloc_from_path(dst, 1, fmt);\n", out);
    fputs("    free(dst);\n", out);
    fputs("    return out_img;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_get_width(void *ptr) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    return img ? img->width : 0;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_get_height(void *ptr) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    return img ? img->height : 0;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_get_channels(void *ptr) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    return img ? img->channels : 0;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_image_get_format(void *ptr) {\n", out);
    fputs("    cct_rt_image_t *img = (cct_rt_image_t*)ptr;\n", out);
    fputs("    return img ? img->format : CCT_RT_IMAGE_FMT_UNKNOWN;\n", out);
    fputs("}\n\n", out);

    return true;
}
