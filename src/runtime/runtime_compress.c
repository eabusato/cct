/*
 * CCT — Clavicula Turing
 * Runtime Compression Helper Emission
 *
 * FASE 32F: cct/compress host-runtime bridge
 */

#include "runtime.h"

bool cct_runtime_emit_compress_helpers(FILE *out) {
    if (!out) return false;

    fputs("static char cct_rt_compress_last_error_buf[256] = {0};\n\n", out);

    fputs("static void cct_rt_compress_clear_error(void) {\n", out);
    fputs("    cct_rt_compress_last_error_buf[0] = '\\0';\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_compress_set_error(const char *msg) {\n", out);
    fputs("    snprintf(cct_rt_compress_last_error_buf, sizeof(cct_rt_compress_last_error_buf), \"%s\", (msg && *msg) ? msg : \"compress failure\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_compress_last_error(void) {\n", out);
    fputs("    return cct_rt_compress_last_error_buf;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_compress_dup(const char *s) {\n", out);
    fputs("    size_t n = s ? strlen(s) : 0u;\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1u);\n", out);
    fputs("    if (n > 0u) memcpy(out_s, s, n);\n", out);
    fputs("    out_s[n] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_compress_hex_encode(const unsigned char *bytes, size_t len) {\n", out);
    fputs("    static const char digits[] = \"0123456789abcdef\";\n", out);
    fputs("    char *hex = (char*)cct_rt_alloc_or_fail((len * 2u) + 1u);\n", out);
    fputs("    for (size_t i = 0; i < len; i++) {\n", out);
    fputs("        unsigned char b = bytes[i];\n", out);
    fputs("        hex[i * 2u] = digits[(b >> 4) & 0x0Fu];\n", out);
    fputs("        hex[i * 2u + 1u] = digits[b & 0x0Fu];\n", out);
    fputs("    }\n", out);
    fputs("    hex[len * 2u] = '\\0';\n", out);
    fputs("    return hex;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_compress_hex_value(char ch) {\n", out);
    fputs("    if (ch >= '0' && ch <= '9') return ch - '0';\n", out);
    fputs("    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');\n", out);
    fputs("    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');\n", out);
    fputs("    return -1;\n", out);
    fputs("}\n\n", out);

    fputs("static unsigned char *cct_rt_compress_hex_decode(const char *hex, size_t *out_len) {\n", out);
    fputs("    size_t n = hex ? strlen(hex) : 0u;\n", out);
    fputs("    if ((n % 2u) != 0u) { cct_rt_compress_set_error(\"gzip hex invalido\"); return NULL; }\n", out);
    fputs("    unsigned char *bytes = (unsigned char*)cct_rt_alloc_or_fail((n / 2u) ? (n / 2u) : 1u);\n", out);
    fputs("    for (size_t i = 0; i < n; i += 2u) {\n", out);
    fputs("        int hi = cct_rt_compress_hex_value(hex[i]);\n", out);
    fputs("        int lo = cct_rt_compress_hex_value(hex[i + 1u]);\n", out);
    fputs("        if (hi < 0 || lo < 0) { cct_rt_compress_set_error(\"gzip hex invalido\"); return NULL; }\n", out);
    fputs("        bytes[i / 2u] = (unsigned char)((hi << 4) | lo);\n", out);
    fputs("    }\n", out);
    fputs("    *out_len = n / 2u;\n", out);
    fputs("    return bytes;\n", out);
    fputs("}\n\n", out);

    fputs("static unsigned char *cct_rt_compress_gzip_deflate(const unsigned char *data, size_t len, size_t *out_len) {\n", out);
    fputs("    z_stream zs;\n", out);
    fputs("    memset(&zs, 0, sizeof(zs));\n", out);
    fputs("    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {\n", out);
    fputs("        cct_rt_compress_set_error(\"gzip init falhou\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    size_t cap = compressBound((uLong)len) + 64u;\n", out);
    fputs("    unsigned char *dest = (unsigned char*)cct_rt_alloc_or_fail(cap ? cap : 1u);\n", out);
    fputs("    zs.next_in = (Bytef*)(data ? data : (const unsigned char*)\"\");\n", out);
    fputs("    zs.avail_in = (uInt)len;\n", out);
    fputs("    zs.next_out = dest;\n", out);
    fputs("    zs.avail_out = (uInt)cap;\n", out);
    fputs("    int rc = deflate(&zs, Z_FINISH);\n", out);
    fputs("    if (rc != Z_STREAM_END) {\n", out);
    fputs("        deflateEnd(&zs);\n", out);
    fputs("        cct_rt_compress_set_error(\"gzip compressao falhou\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    *out_len = (size_t)zs.total_out;\n", out);
    fputs("    deflateEnd(&zs);\n", out);
    fputs("    return dest;\n", out);
    fputs("}\n\n", out);

    fputs("static unsigned char *cct_rt_compress_gzip_inflate(const unsigned char *data, size_t len, size_t *out_len) {\n", out);
    fputs("    z_stream zs;\n", out);
    fputs("    memset(&zs, 0, sizeof(zs));\n", out);
    fputs("    if (inflateInit2(&zs, 15 + 16) != Z_OK) {\n", out);
    fputs("        cct_rt_compress_set_error(\"gzip inflate init falhou\");\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    size_t cap = (len * 4u) + 64u;\n", out);
    fputs("    unsigned char *dest = (unsigned char*)cct_rt_alloc_or_fail(cap ? cap : 1u);\n", out);
    fputs("    zs.next_in = (Bytef*)(data ? data : (const unsigned char*)\"\");\n", out);
    fputs("    zs.avail_in = (uInt)len;\n", out);
    fputs("    int rc = Z_OK;\n", out);
    fputs("    do {\n", out);
    fputs("        if (zs.total_out >= cap) {\n", out);
    fputs("            cap *= 2u;\n", out);
    fputs("            dest = (unsigned char*)realloc(dest, cap);\n", out);
    fputs("            if (!dest) cct_rt_fail(\"gzip inflate realloc falhou\");\n", out);
    fputs("        }\n", out);
    fputs("        zs.next_out = dest + zs.total_out;\n", out);
    fputs("        zs.avail_out = (uInt)(cap - zs.total_out);\n", out);
    fputs("        rc = inflate(&zs, Z_NO_FLUSH);\n", out);
    fputs("        if (rc == Z_STREAM_END) break;\n", out);
    fputs("        if (rc != Z_OK) {\n", out);
    fputs("            inflateEnd(&zs);\n", out);
    fputs("            cct_rt_compress_set_error(\"gzip descompressao falhou\");\n", out);
    fputs("            return NULL;\n", out);
    fputs("        }\n", out);
    fputs("    } while (1);\n", out);
    fputs("    *out_len = (size_t)zs.total_out;\n", out);
    fputs("    inflateEnd(&zs);\n", out);
    fputs("    return dest;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_gzip_compress_text(const char *text) {\n", out);
    fputs("    cct_rt_compress_clear_error();\n", out);
    fputs("    size_t out_len = 0u;\n", out);
    fputs("    const unsigned char *src = (const unsigned char*)(text ? text : \"\");\n", out);
    fputs("    unsigned char *compressed = cct_rt_compress_gzip_deflate(src, strlen((const char*)src), &out_len);\n", out);
    fputs("    if (!compressed) return cct_rt_compress_dup(\"\");\n", out);
    fputs("    return cct_rt_compress_hex_encode(compressed, out_len);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_gzip_compress_bytes(void *bytes_ptr, long long length) {\n", out);
    fputs("    cct_rt_compress_clear_error();\n", out);
    fputs("    if (length < 0) { cct_rt_compress_set_error(\"gzip tamanho invalido\"); return cct_rt_compress_dup(\"\"); }\n", out);
    fputs("    cct_rt_bytes_t *bytes = cct_rt_bytes_require(bytes_ptr, \"gzip bytes recebeu buffer nulo\");\n", out);
    fputs("    if (length > bytes->len) { cct_rt_compress_set_error(\"gzip tamanho excede buffer\"); return cct_rt_compress_dup(\"\"); }\n", out);
    fputs("    size_t out_len = 0u;\n", out);
    fputs("    unsigned char *compressed = cct_rt_compress_gzip_deflate(bytes->data, (size_t)length, &out_len);\n", out);
    fputs("    if (!compressed) return cct_rt_compress_dup(\"\");\n", out);
    fputs("    return cct_rt_compress_hex_encode(compressed, out_len);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_gzip_decompress_text(const char *hex) {\n", out);
    fputs("    cct_rt_compress_clear_error();\n", out);
    fputs("    size_t data_len = 0u;\n", out);
    fputs("    unsigned char *data = cct_rt_compress_hex_decode(hex ? hex : \"\", &data_len);\n", out);
    fputs("    if (!data) return cct_rt_compress_dup(\"\");\n", out);
    fputs("    size_t out_len = 0u;\n", out);
    fputs("    unsigned char *plain = cct_rt_compress_gzip_inflate(data, data_len, &out_len);\n", out);
    fputs("    if (!plain) return cct_rt_compress_dup(\"\");\n", out);
    fputs("    char *text = (char*)cct_rt_alloc_or_fail(out_len + 1u);\n", out);
    fputs("    if (out_len > 0u) memcpy(text, plain, out_len);\n", out);
    fputs("    text[out_len] = '\\0';\n", out);
    fputs("    return text;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_gzip_decompress_bytes(const char *hex) {\n", out);
    fputs("    cct_rt_compress_clear_error();\n", out);
    fputs("    size_t data_len = 0u;\n", out);
    fputs("    unsigned char *data = cct_rt_compress_hex_decode(hex ? hex : \"\", &data_len);\n", out);
    fputs("    if (!data) return NULL;\n", out);
    fputs("    size_t out_len = 0u;\n", out);
    fputs("    unsigned char *plain = cct_rt_compress_gzip_inflate(data, data_len, &out_len);\n", out);
    fputs("    if (!plain) return NULL;\n", out);
    fputs("    cct_rt_bytes_t *bytes = (cct_rt_bytes_t*)cct_rt_bytes_new((long long)out_len);\n", out);
    fputs("    if (out_len > 0u) memcpy(bytes->data, plain, out_len);\n", out);
    fputs("    return bytes;\n", out);
    fputs("}\n\n", out);

    return true;
}
