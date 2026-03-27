/*
 * CCT — Clavicula Turing
 * Runtime Regex Helper Emission
 *
 * FASE 32C: cct/regex host-runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_regex_helpers(FILE *out) {
    if (!out) return false;

    fputs("typedef struct {\n", out);
    fputs("    regex_t compiled;\n", out);
    fputs("    long long flags;\n", out);
    fputs("} cct_rt_regex_t;\n\n", out);

    fputs("static char cct_rt_regex_last_error_buf[256] = {0};\n\n", out);

    fputs("static void cct_rt_regex_set_error(const char *msg) {\n", out);
    fputs("    snprintf(cct_rt_regex_last_error_buf, sizeof(cct_rt_regex_last_error_buf), \"%s\", (msg && *msg) ? msg : \"regex failure\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_regex_dup_span(const char *src, size_t len) {\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(len + 1u);\n", out);
    fputs("    if (len > 0u) memcpy(out_s, src, len);\n", out);
    fputs("    out_s[len] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_regex_push_verbum(void *flux_ptr, const char *src, size_t len) {\n", out);
    fputs("    char *copy = cct_rt_regex_dup_span(src, len);\n", out);
    fputs("    cct_rt_fluxus_push(flux_ptr, (const void*)&copy);\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_regex_compile(const char *pattern, long long flags) {\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)cct_rt_alloc_or_fail(sizeof(cct_rt_regex_t));\n", out);
    fputs("    int cflags = REG_EXTENDED;\n", out);
    fputs("    if (flags & 1LL) cflags |= REG_ICASE;\n", out);
    fputs("    if (flags & 2LL) cflags |= REG_NEWLINE;\n", out);
    fputs("    int rc = regcomp(&rx->compiled, pattern ? pattern : \"\", cflags);\n", out);
    fputs("    if (rc != 0) {\n", out);
    fputs("        regerror(rc, &rx->compiled, cct_rt_regex_last_error_buf, sizeof(cct_rt_regex_last_error_buf));\n", out);
    fputs("        cct_rt_free_ptr(rx);\n", out);
    fputs("        return NULL;\n", out);
    fputs("    }\n", out);
    fputs("    rx->flags = flags;\n", out);
    fputs("    cct_rt_regex_last_error_buf[0] = '\\0';\n", out);
    fputs("    return rx;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_regex_last_error(void) {\n", out);
    fputs("    return cct_rt_regex_last_error_buf;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_regex_match(void *handle, const char *text) {\n", out);
    fputs("    if (!handle) return 0LL;\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    regmatch_t match;\n", out);
    fputs("    int rc = regexec(&rx->compiled, text ? text : \"\", 1, &match, 0);\n", out);
    fputs("    if (rc != 0) return 0LL;\n", out);
    fputs("    size_t len = strlen(text ? text : \"\");\n", out);
    fputs("    return (match.rm_so == 0 && match.rm_eo == (regoff_t)len) ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_regex_search(void *handle, const char *text) {\n", out);
    fputs("    if (!handle) return cct_rt_regex_dup_span(\"\", 0u);\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    regmatch_t match;\n", out);
    fputs("    const char *src = text ? text : \"\";\n", out);
    fputs("    int rc = regexec(&rx->compiled, src, 1, &match, 0);\n", out);
    fputs("    if (rc != 0 || match.rm_so < 0 || match.rm_eo < match.rm_so) return cct_rt_regex_dup_span(\"\", 0u);\n", out);
    fputs("    return cct_rt_regex_dup_span(src + match.rm_so, (size_t)(match.rm_eo - match.rm_so));\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_regex_find_all(void *handle, const char *text) {\n", out);
    fputs("    void *flux = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
    fputs("    if (!handle) return flux;\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    const char *cursor = text ? text : \"\";\n", out);
    fputs("    size_t offset = 0u;\n", out);
    fputs("    size_t total_len = strlen(cursor);\n", out);
    fputs("    while (offset <= total_len) {\n", out);
    fputs("        regmatch_t match;\n", out);
    fputs("        int rc = regexec(&rx->compiled, cursor + offset, 1, &match, 0);\n", out);
    fputs("        if (rc != 0 || match.rm_so < 0 || match.rm_eo < match.rm_so) break;\n", out);
    fputs("        size_t start = offset + (size_t)match.rm_so;\n", out);
    fputs("        size_t end = offset + (size_t)match.rm_eo;\n", out);
    fputs("        cct_rt_regex_push_verbum(flux, cursor + start, end - start);\n", out);
    fputs("        if (match.rm_eo == match.rm_so) {\n", out);
    fputs("            if (offset == total_len) break;\n", out);
    fputs("            offset += 1u;\n", out);
    fputs("        } else {\n", out);
    fputs("            offset = end;\n", out);
    fputs("        }\n", out);
    fputs("    }\n", out);
    fputs("    return flux;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_regex_replace(void *handle, const char *text, const char *replacement, long long all) {\n", out);
    fputs("    if (!handle) return cct_rt_regex_dup_span(text ? text : \"\", strlen(text ? text : \"\"));\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    const char *src = text ? text : \"\";\n", out);
    fputs("    const char *rep = replacement ? replacement : \"\";\n", out);
    fputs("    size_t src_len = strlen(src);\n", out);
    fputs("    size_t rep_len = strlen(rep);\n", out);
    fputs("    size_t cap = src_len + rep_len + 16u;\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(cap);\n", out);
    fputs("    size_t out_len = 0u;\n", out);
    fputs("    size_t offset = 0u;\n", out);
    fputs("    while (offset <= src_len) {\n", out);
    fputs("        regmatch_t match;\n", out);
    fputs("        int rc = regexec(&rx->compiled, src + offset, 1, &match, 0);\n", out);
    fputs("        if (rc != 0 || match.rm_so < 0 || match.rm_eo < match.rm_so) break;\n", out);
    fputs("        size_t start = offset + (size_t)match.rm_so;\n", out);
    fputs("        size_t end = offset + (size_t)match.rm_eo;\n", out);
    fputs("        size_t prefix = start - offset;\n", out);
    fputs("        while (out_len + prefix + rep_len + 1u > cap) { cap *= 2u; out_s = (char*)realloc(out_s, cap); if (!out_s) cct_rt_fail(\"regex replace realloc failed\"); }\n", out);
    fputs("        if (prefix > 0u) memcpy(out_s + out_len, src + offset, prefix);\n", out);
    fputs("        out_len += prefix;\n", out);
    fputs("        if (rep_len > 0u) memcpy(out_s + out_len, rep, rep_len);\n", out);
    fputs("        out_len += rep_len;\n", out);
    fputs("        if (!all) { offset = end; break; }\n", out);
    fputs("        if (match.rm_eo == match.rm_so) {\n", out);
    fputs("            if (offset == src_len) break;\n", out);
    fputs("            while (out_len + 2u > cap) { cap *= 2u; out_s = (char*)realloc(out_s, cap); if (!out_s) cct_rt_fail(\"regex replace realloc failed\"); }\n", out);
    fputs("            out_s[out_len++] = src[offset];\n", out);
    fputs("            offset += 1u;\n", out);
    fputs("        } else {\n", out);
    fputs("            offset = end;\n", out);
    fputs("        }\n", out);
    fputs("    }\n", out);
    fputs("    if (offset < src_len) {\n", out);
    fputs("        size_t tail = src_len - offset;\n", out);
    fputs("        while (out_len + tail + 1u > cap) { cap *= 2u; out_s = (char*)realloc(out_s, cap); if (!out_s) cct_rt_fail(\"regex replace realloc failed\"); }\n", out);
    fputs("        memcpy(out_s + out_len, src + offset, tail);\n", out);
    fputs("        out_len += tail;\n", out);
    fputs("    }\n", out);
    fputs("    out_s[out_len] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_regex_split(void *handle, const char *text) {\n", out);
    fputs("    void *flux = cct_rt_fluxus_create((long long)sizeof(char*));\n", out);
    fputs("    const char *src = text ? text : \"\";\n", out);
    fputs("    if (!handle) {\n", out);
    fputs("        cct_rt_regex_push_verbum(flux, src, strlen(src));\n", out);
    fputs("        return flux;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    size_t src_len = strlen(src);\n", out);
    fputs("    size_t offset = 0u;\n", out);
    fputs("    while (offset <= src_len) {\n", out);
    fputs("        regmatch_t match;\n", out);
    fputs("        int rc = regexec(&rx->compiled, src + offset, 1, &match, 0);\n", out);
    fputs("        if (rc != 0 || match.rm_so < 0 || match.rm_eo < match.rm_so) break;\n", out);
    fputs("        size_t start = offset + (size_t)match.rm_so;\n", out);
    fputs("        size_t end = offset + (size_t)match.rm_eo;\n", out);
    fputs("        cct_rt_regex_push_verbum(flux, src + offset, start - offset);\n", out);
    fputs("        if (match.rm_eo == match.rm_so) {\n", out);
    fputs("            if (offset == src_len) break;\n", out);
    fputs("            offset += 1u;\n", out);
    fputs("        } else {\n", out);
    fputs("            offset = end;\n", out);
    fputs("        }\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_regex_push_verbum(flux, src + offset, src_len - offset);\n", out);
    fputs("    return flux;\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_regex_free(void *handle) {\n", out);
    fputs("    if (!handle) return;\n", out);
    fputs("    cct_rt_regex_t *rx = (cct_rt_regex_t*)handle;\n", out);
    fputs("    regfree(&rx->compiled);\n", out);
    fputs("    cct_rt_free_ptr(rx);\n", out);
    fputs("}\n\n", out);

    return true;
}
