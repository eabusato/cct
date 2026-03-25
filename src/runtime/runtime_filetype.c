/*
 * CCT — Clavicula Turing
 * Runtime Filetype Helper Emission
 *
 * FASE 32G: cct/filetype host-runtime bridge
 */

#include "runtime.h"

bool cct_runtime_emit_filetype_helpers(FILE *out) {
    if (!out) return false;

    fputs("enum {\n", out);
    fputs("    CCT_RT_FILETYPE_UNKNOWN = 0,\n", out);
    fputs("    CCT_RT_FILETYPE_JPEG = 1,\n", out);
    fputs("    CCT_RT_FILETYPE_PNG = 2,\n", out);
    fputs("    CCT_RT_FILETYPE_GIF = 3,\n", out);
    fputs("    CCT_RT_FILETYPE_BMP = 4,\n", out);
    fputs("    CCT_RT_FILETYPE_WEBP = 5,\n", out);
    fputs("    CCT_RT_FILETYPE_MP4 = 6,\n", out);
    fputs("    CCT_RT_FILETYPE_WEBM = 7,\n", out);
    fputs("    CCT_RT_FILETYPE_AVI = 8,\n", out);
    fputs("    CCT_RT_FILETYPE_MP3 = 9,\n", out);
    fputs("    CCT_RT_FILETYPE_OGG = 10,\n", out);
    fputs("    CCT_RT_FILETYPE_WAV = 11,\n", out);
    fputs("    CCT_RT_FILETYPE_PDF = 12,\n", out);
    fputs("    CCT_RT_FILETYPE_ZIP = 13,\n", out);
    fputs("    CCT_RT_FILETYPE_TEXT = 14\n", out);
    fputs("};\n\n", out);

    fputs("static int cct_rt_filetype_has_prefix(const unsigned char *buf, size_t len, const unsigned char *prefix, size_t prefix_len) {\n", out);
    fputs("    if (!buf || !prefix || len < prefix_len) return 0;\n", out);
    fputs("    return memcmp(buf, prefix, prefix_len) == 0;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_filetype_eq_at(const unsigned char *buf, size_t len, size_t off, const char *text) {\n", out);
    fputs("    size_t n = text ? strlen(text) : 0u;\n", out);
    fputs("    if (!buf || !text || len < off + n) return 0;\n", out);
    fputs("    return memcmp(buf + off, text, n) == 0;\n", out);
    fputs("}\n\n", out);

    fputs("static int cct_rt_filetype_looks_text(const unsigned char *buf, size_t len) {\n", out);
    fputs("    if (!buf || len == 0u) return 0;\n", out);
    fputs("    size_t printable = 0u;\n", out);
    fputs("    for (size_t i = 0; i < len; i++) {\n", out);
    fputs("        unsigned char ch = buf[i];\n", out);
    fputs("        if (ch == 0u) return 0;\n", out);
    fputs("        if (ch == 9u || ch == 10u || ch == 13u || (ch >= 32u && ch <= 126u) || ch >= 128u) printable++;\n", out);
    fputs("    }\n", out);
    fputs("    return printable == len;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_filetype_detect_buf(const unsigned char *buf, size_t len) {\n", out);
    fputs("    static const unsigned char jpeg_sig[] = {0xFFu, 0xD8u, 0xFFu};\n", out);
    fputs("    static const unsigned char png_sig[] = {0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au};\n", out);
    fputs("    static const unsigned char gif_sig[] = {0x47u, 0x49u, 0x46u, 0x38u};\n", out);
    fputs("    static const unsigned char webm_sig[] = {0x1Au, 0x45u, 0xDFu, 0xA3u};\n", out);
    fputs("    if (!buf || len < 2u) return CCT_RT_FILETYPE_UNKNOWN;\n", out);
    fputs("    if (cct_rt_filetype_has_prefix(buf, len, jpeg_sig, sizeof(jpeg_sig))) return CCT_RT_FILETYPE_JPEG;\n", out);
    fputs("    if (cct_rt_filetype_has_prefix(buf, len, png_sig, sizeof(png_sig))) return CCT_RT_FILETYPE_PNG;\n", out);
    fputs("    if (cct_rt_filetype_has_prefix(buf, len, gif_sig, sizeof(gif_sig))) return CCT_RT_FILETYPE_GIF;\n", out);
    fputs("    if (buf[0] == 'B' && buf[1] == 'M') return CCT_RT_FILETYPE_BMP;\n", out);
    fputs("    if (len >= 12u && cct_rt_filetype_eq_at(buf, len, 0u, \"RIFF\") && cct_rt_filetype_eq_at(buf, len, 8u, \"WEBP\")) return CCT_RT_FILETYPE_WEBP;\n", out);
    fputs("    if (len >= 12u && cct_rt_filetype_eq_at(buf, len, 4u, \"ftyp\")) return CCT_RT_FILETYPE_MP4;\n", out);
    fputs("    if (cct_rt_filetype_has_prefix(buf, len, webm_sig, sizeof(webm_sig))) return CCT_RT_FILETYPE_WEBM;\n", out);
    fputs("    if (len >= 12u && cct_rt_filetype_eq_at(buf, len, 0u, \"RIFF\") && cct_rt_filetype_eq_at(buf, len, 8u, \"AVI \")) return CCT_RT_FILETYPE_AVI;\n", out);
    fputs("    if ((len >= 3u && cct_rt_filetype_eq_at(buf, len, 0u, \"ID3\")) || (len >= 2u && buf[0] == 0xFFu && (buf[1] & 0xE0u) == 0xE0u)) return CCT_RT_FILETYPE_MP3;\n", out);
    fputs("    if (len >= 4u && cct_rt_filetype_eq_at(buf, len, 0u, \"OggS\")) return CCT_RT_FILETYPE_OGG;\n", out);
    fputs("    if (len >= 12u && cct_rt_filetype_eq_at(buf, len, 0u, \"RIFF\") && cct_rt_filetype_eq_at(buf, len, 8u, \"WAVE\")) return CCT_RT_FILETYPE_WAV;\n", out);
    fputs("    if (len >= 4u && cct_rt_filetype_eq_at(buf, len, 0u, \"%PDF\")) return CCT_RT_FILETYPE_PDF;\n", out);
    fputs("    if (len >= 4u && buf[0] == 'P' && buf[1] == 'K' && (buf[2] == 3 || buf[2] == 5 || buf[2] == 7) && (buf[3] == 4 || buf[3] == 6 || buf[3] == 8)) return CCT_RT_FILETYPE_ZIP;\n", out);
    fputs("    if (cct_rt_filetype_looks_text(buf, len)) return CCT_RT_FILETYPE_TEXT;\n", out);
    fputs("    return CCT_RT_FILETYPE_UNKNOWN;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_filetype_detect_path(const char *path) {\n", out);
    fputs("    if (!path || !*path) return CCT_RT_FILETYPE_UNKNOWN;\n", out);
    fputs("    FILE *fp = fopen(path, \"rb\");\n", out);
    fputs("    if (!fp) return CCT_RT_FILETYPE_UNKNOWN;\n", out);
    fputs("    unsigned char buf[64];\n", out);
    fputs("    size_t n = fread(buf, 1u, sizeof(buf), fp);\n", out);
    fputs("    fclose(fp);\n", out);
    fputs("    return cct_rt_filetype_detect_buf(buf, n);\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_filetype_detect_bytes(void *bytes_ptr, long long length) {\n", out);
    fputs("    if (length <= 0) return CCT_RT_FILETYPE_UNKNOWN;\n", out);
    fputs("    cct_rt_bytes_t *bytes = cct_rt_bytes_require(bytes_ptr, \"filetype bytes recebeu buffer nulo\");\n", out);
    fputs("    size_t n = (size_t)length;\n", out);
    fputs("    if (n > bytes->len) n = bytes->len;\n", out);
    fputs("    return cct_rt_filetype_detect_buf(bytes->data, n);\n", out);
    fputs("}\n\n", out);

    return true;
}
