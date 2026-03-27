/*
 * CCT — Clavicula Turing
 * Media store runtime helper emission
 *
 * FASE 40A: cct/media_store host-runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_media_store_helpers(FILE *out) {
    if (!out) return false;

#define EMIT(line) fputs(line "\n", out)

    EMIT("static char cct_rt_media_store_last_error_buf[512];");
    EMIT("");
    EMIT("static void cct_rt_media_store_copy_text(char *dst, size_t cap, const char *src) {");
    EMIT("    const char *text = src ? src : \"\";");
    EMIT("    size_t n = strlen(text);");
    EMIT("    if (cap == 0u) return;");
    EMIT("    if (n >= cap) n = cap - 1u;");
    EMIT("    memcpy(dst, text, n);");
    EMIT("    dst[n] = '\\0';");
    EMIT("}");
    EMIT("");
    EMIT("static void cct_rt_media_store_set_error(const char *msg) {");
    EMIT("    cct_rt_media_store_copy_text(cct_rt_media_store_last_error_buf, sizeof(cct_rt_media_store_last_error_buf), msg);");
    EMIT("}");
    EMIT("");
    EMIT("static void cct_rt_media_store_clear_error(void) {");
    EMIT("    cct_rt_media_store_last_error_buf[0] = '\\0';");
    EMIT("}");
    EMIT("");
    EMIT("static char *cct_rt_media_store_dup_cstr(const char *src) {");
    EMIT("    const char *text = src ? src : \"\";");
    EMIT("    size_t n = strlen(text);");
    EMIT("    char *out_s = (char*)cct_rt_alloc_or_fail(n + 1u);");
    EMIT("    memcpy(out_s, text, n + 1u);");
    EMIT("    return out_s;");
    EMIT("}");
    EMIT("");
    EMIT("static char *cct_rt_media_store_last_error(void) {");
    EMIT("    return cct_rt_media_store_dup_cstr(cct_rt_media_store_last_error_buf);");
    EMIT("}");
    EMIT("");
    EMIT("static char *cct_rt_media_store_sha256_file(const char *path) {");
    EMIT("    FILE *fp = NULL;");
    EMIT("    EVP_MD_CTX *ctx = NULL;");
    EMIT("    unsigned char digest[EVP_MAX_MD_SIZE];");
    EMIT("    unsigned int digest_len = 0u;");
    EMIT("    unsigned char buf[8192];");
    EMIT("    char hex[EVP_MAX_MD_SIZE * 2 + 1];");
    EMIT("    size_t nread = 0u;");
    EMIT("    size_t i = 0u;");
    EMIT("    cct_rt_media_store_clear_error();");
    EMIT("    if (!path || !*path) {");
    EMIT("        cct_rt_media_store_set_error(\"media_store: path de checksum ausente\");");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    fp = fopen(path, \"rb\");");
    EMIT("    if (!fp) {");
    EMIT("        char msg[512];");
    EMIT("        snprintf(msg, sizeof(msg), \"media_store: falha ao abrir arquivo para checksum: %s\", path);");
    EMIT("        cct_rt_media_store_set_error(msg);");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    ctx = EVP_MD_CTX_new();");
    EMIT("    if (!ctx) {");
    EMIT("        fclose(fp);");
    EMIT("        cct_rt_media_store_set_error(\"media_store: falha ao criar contexto SHA-256\");");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {");
    EMIT("        EVP_MD_CTX_free(ctx);");
    EMIT("        fclose(fp);");
    EMIT("        cct_rt_media_store_set_error(\"media_store: falha ao iniciar SHA-256\");");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    while ((nread = fread(buf, 1u, sizeof(buf), fp)) > 0u) {");
    EMIT("        if (EVP_DigestUpdate(ctx, buf, nread) != 1) {");
    EMIT("            EVP_MD_CTX_free(ctx);");
    EMIT("            fclose(fp);");
    EMIT("            cct_rt_media_store_set_error(\"media_store: falha ao atualizar SHA-256\");");
    EMIT("            return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("        }");
    EMIT("    }");
    EMIT("    if (ferror(fp)) {");
    EMIT("        EVP_MD_CTX_free(ctx);");
    EMIT("        fclose(fp);");
    EMIT("        cct_rt_media_store_set_error(\"media_store: falha ao ler arquivo para checksum\");");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {");
    EMIT("        EVP_MD_CTX_free(ctx);");
    EMIT("        fclose(fp);");
    EMIT("        cct_rt_media_store_set_error(\"media_store: falha ao finalizar SHA-256\");");
    EMIT("        return cct_rt_media_store_dup_cstr(\"\");");
    EMIT("    }");
    EMIT("    EVP_MD_CTX_free(ctx);");
    EMIT("    fclose(fp);");
    EMIT("    for (i = 0u; i < (size_t)digest_len; i++) snprintf(hex + (i * 2u), 3u, \"%02x\", digest[i]);");
    EMIT("    hex[(size_t)digest_len * 2u] = '\\0';");
    EMIT("    return cct_rt_media_store_dup_cstr(hex);");
    EMIT("}");

#undef EMIT
    return true;
}
