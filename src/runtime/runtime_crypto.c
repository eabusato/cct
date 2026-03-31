/*
 * CCT — Clavicula Turing
 * Runtime Crypto Helper Emission
 *
 * FASE 32A: cct/crypto host-runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_crypto_helpers(FILE *out) {
    if (!out) return false;

    fputs("/* ===== Crypto Runtime Helpers (FASE 32A) ===== */\n", out);
    fputs("static void cct_rt_crypto_secure_zero(void *ptr, size_t len) {\n", out);
    fputs("    if (!ptr || len == 0U) return;\n", out);
    fputs("    volatile unsigned char *p = (volatile unsigned char*)ptr;\n", out);
    fputs("    while (len--) *p++ = 0U;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_hex_encode(const unsigned char *bytes, size_t len) {\n", out);
    fputs("    static const char digits[] = \"0123456789abcdef\";\n", out);
    fputs("    char *hex = (char*)cct_rt_alloc_or_fail((len * 2U) + 1U);\n", out);
    fputs("    for (size_t i = 0; i < len; i++) {\n", out);
    fputs("        unsigned char b = bytes[i];\n", out);
    fputs("        hex[i * 2U] = digits[(b >> 4) & 0x0FU];\n", out);
    fputs("        hex[i * 2U + 1U] = digits[b & 0x0FU];\n", out);
    fputs("    }\n", out);
    fputs("    hex[len * 2U] = '\\0';\n", out);
    fputs("    return hex;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_base64_encode(const unsigned char *bytes, size_t len) {\n", out);
    fputs("    static const char table[] = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\";\n", out);
    fputs("    size_t out_len = ((len + 2U) / 3U) * 4U;\n", out);
    fputs("    char *out_s = (char*)cct_rt_alloc_or_fail(out_len + 1U);\n", out);
    fputs("    size_t i = 0U;\n", out);
    fputs("    size_t j = 0U;\n", out);
    fputs("    while (i < len) {\n", out);
    fputs("        size_t remain = len - i;\n", out);
    fputs("        unsigned int octet_a = i < len ? bytes[i++] : 0U;\n", out);
    fputs("        unsigned int octet_b = i < len ? bytes[i++] : 0U;\n", out);
    fputs("        unsigned int octet_c = i < len ? bytes[i++] : 0U;\n", out);
    fputs("        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;\n", out);
    fputs("        out_s[j++] = table[(triple >> 18) & 0x3FU];\n", out);
    fputs("        out_s[j++] = table[(triple >> 12) & 0x3FU];\n", out);
    fputs("        out_s[j++] = remain > 1U ? table[(triple >> 6) & 0x3FU] : '=';\n", out);
    fputs("        out_s[j++] = remain > 2U ? table[triple & 0x3FU] : '=';\n", out);
    fputs("    }\n", out);
    fputs("    out_s[out_len] = '\\0';\n", out);
    fputs("    return out_s;\n", out);
    fputs("}\n\n", out);

    fputs("static cct_rt_bytes_t *cct_rt_crypto_require_bytes(void *ptr, const char *ctx) {\n", out);
    fputs("    return cct_rt_bytes_require(ptr, ctx ? ctx : \"crypto bytes buffer invalido\");\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha1_text(const char *text) {\n", out);
    fputs("    const unsigned char *data = (const unsigned char*)((text) ? text : \"\");\n", out);
    fputs("    unsigned char digest[20];\n", out);
    fputs("    if (!SHA1(data, strlen((const char*)data), digest)) cct_rt_fail(\"crypto sha1 failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha1_bytes(void *ptr, long long length) {\n", out);
    fputs("    if (length < 0) cct_rt_fail(\"crypto sha1_bytes expects length >= 0\");\n", out);
    fputs("    cct_rt_bytes_t *bytes = cct_rt_crypto_require_bytes(ptr, \"crypto sha1_bytes recebeu buffer nulo\");\n", out);
    fputs("    if (length > bytes->len) cct_rt_fail(\"crypto sha1_bytes length exceeds buffer size\");\n", out);
    fputs("    unsigned char digest[20];\n", out);
    fputs("    if (!SHA1(bytes->data ? bytes->data : (const unsigned char*)\"\", (size_t)length, digest)) cct_rt_fail(\"crypto sha1_bytes failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha256_text(const char *text) {\n", out);
    fputs("    const unsigned char *data = (const unsigned char*)((text) ? text : \"\");\n", out);
    fputs("    unsigned char digest[32];\n", out);
    fputs("    if (!SHA256(data, strlen((const char*)data), digest)) cct_rt_fail(\"crypto sha256 failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha256_bytes(void *ptr, long long length) {\n", out);
    fputs("    if (length < 0) cct_rt_fail(\"crypto sha256_bytes expects length >= 0\");\n", out);
    fputs("    cct_rt_bytes_t *bytes = cct_rt_crypto_require_bytes(ptr, \"crypto sha256_bytes recebeu buffer nulo\");\n", out);
    fputs("    if (length > bytes->len) cct_rt_fail(\"crypto sha256_bytes length exceeds buffer size\");\n", out);
    fputs("    unsigned char digest[32];\n", out);
    fputs("    if (!SHA256(bytes->data ? bytes->data : (const unsigned char*)\"\", (size_t)length, digest)) cct_rt_fail(\"crypto sha256_bytes failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha512_text(const char *text) {\n", out);
    fputs("    const unsigned char *data = (const unsigned char*)((text) ? text : \"\");\n", out);
    fputs("    unsigned char digest[64];\n", out);
    fputs("    if (!SHA512(data, strlen((const char*)data), digest)) cct_rt_fail(\"crypto sha512 failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_sha512_bytes(void *ptr, long long length) {\n", out);
    fputs("    if (length < 0) cct_rt_fail(\"crypto sha512_bytes expects length >= 0\");\n", out);
    fputs("    cct_rt_bytes_t *bytes = cct_rt_crypto_require_bytes(ptr, \"crypto sha512_bytes recebeu buffer nulo\");\n", out);
    fputs("    if (length > bytes->len) cct_rt_fail(\"crypto sha512_bytes length exceeds buffer size\");\n", out);
    fputs("    unsigned char digest[64];\n", out);
    fputs("    if (!SHA512(bytes->data ? bytes->data : (const unsigned char*)\"\", (size_t)length, digest)) cct_rt_fail(\"crypto sha512_bytes failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_hmac_sha256(const char *key, const char *message) {\n", out);
    fputs("    unsigned char digest[EVP_MAX_MD_SIZE];\n", out);
    fputs("    unsigned int digest_len = 0U;\n", out);
    fputs("    const char *key_s = key ? key : \"\";\n", out);
    fputs("    const char *msg_s = message ? message : \"\";\n", out);
    fputs("    if (!HMAC(EVP_sha256(), key_s, (int)strlen(key_s), (const unsigned char*)msg_s, strlen(msg_s), digest, &digest_len)) cct_rt_fail(\"crypto hmac_sha256 failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, digest_len);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_hmac_sha512(const char *key, const char *message) {\n", out);
    fputs("    unsigned char digest[EVP_MAX_MD_SIZE];\n", out);
    fputs("    unsigned int digest_len = 0U;\n", out);
    fputs("    const char *key_s = key ? key : \"\";\n", out);
    fputs("    const char *msg_s = message ? message : \"\";\n", out);
    fputs("    if (!HMAC(EVP_sha512(), key_s, (int)strlen(key_s), (const unsigned char*)msg_s, strlen(msg_s), digest, &digest_len)) cct_rt_fail(\"crypto hmac_sha512 failed\");\n", out);
    fputs("    return cct_rt_crypto_hex_encode(digest, digest_len);\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_pbkdf2_sha256(const char *password, const char *salt, long long iterations, long long key_length) {\n", out);
    fputs("    const char *pwd = password ? password : \"\";\n", out);
    fputs("    const char *salt_s = salt ? salt : \"\";\n", out);
    fputs("    if (iterations < 100000LL) cct_rt_fail(\"crypto pbkdf2_sha256 requires at least 100000 iterations\");\n", out);
    fputs("    if (strlen(salt_s) < 16U) cct_rt_fail(\"crypto pbkdf2_sha256 requires salt length >= 16\");\n", out);
    fputs("    if (key_length < 16LL) cct_rt_fail(\"crypto pbkdf2_sha256 requires key_length >= 16\");\n", out);
    fputs("    unsigned char *derived = (unsigned char*)cct_rt_alloc_or_fail((size_t)key_length);\n", out);
    fputs("    if (PKCS5_PBKDF2_HMAC(pwd, (int)strlen(pwd), (const unsigned char*)salt_s, (int)strlen(salt_s), (int)iterations, EVP_sha256(), (int)key_length, derived) != 1) {\n", out);
    fputs("        cct_rt_crypto_secure_zero(derived, (size_t)key_length);\n", out);
    fputs("        cct_rt_free_ptr(derived);\n", out);
    fputs("        cct_rt_fail(\"crypto pbkdf2_sha256 failed\");\n", out);
    fputs("    }\n", out);
    fputs("    char *hex = cct_rt_crypto_hex_encode(derived, (size_t)key_length);\n", out);
    fputs("    cct_rt_crypto_secure_zero(derived, (size_t)key_length);\n", out);
    fputs("    cct_rt_free_ptr(derived);\n", out);
    fputs("    return hex;\n", out);
    fputs("}\n\n", out);

    fputs("static void *cct_rt_crypto_csprng_bytes(long long count) {\n", out);
    fputs("    if (count < 0) cct_rt_fail(\"crypto csprng_bytes expects count >= 0\");\n", out);
    fputs("    cct_rt_bytes_t *bytes = (cct_rt_bytes_t*)cct_rt_bytes_new(count);\n", out);
    fputs("    if (count == 0) return bytes;\n", out);
    fputs("    if (RAND_bytes(bytes->data, (int)count) != 1) cct_rt_fail(\"crypto csprng_bytes failed\");\n", out);
    fputs("    return bytes;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_crypto_constant_time_compare(const char *a, const char *b) {\n", out);
    fputs("    const unsigned char *aa = (const unsigned char*)((a) ? a : \"\");\n", out);
    fputs("    const unsigned char *bb = (const unsigned char*)((b) ? b : \"\");\n", out);
    fputs("    size_t len_a = strlen((const char*)aa);\n", out);
    fputs("    size_t len_b = strlen((const char*)bb);\n", out);
    fputs("    size_t max_len = (len_a > len_b) ? len_a : len_b;\n", out);
    fputs("    unsigned int diff = (unsigned int)(len_a ^ len_b);\n", out);
    fputs("    for (size_t i = 0; i < max_len; i++) {\n", out);
    fputs("        unsigned char va = (i < len_a) ? aa[i] : 0U;\n", out);
    fputs("        unsigned char vb = (i < len_b) ? bb[i] : 0U;\n", out);
    fputs("        diff |= (unsigned int)(va ^ vb);\n", out);
    fputs("    }\n", out);
    fputs("    return diff == 0U ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);

    fputs("static char *cct_rt_crypto_ws_accept_key(const char *client_key) {\n", out);
    fputs("    static const char ws_guid[] = \"258EAFA5-E914-47DA-95CA-C5AB0DC85B11\";\n", out);
    fputs("    const char *key_s = client_key ? client_key : \"\";\n", out);
    fputs("    size_t key_len = strlen(key_s);\n", out);
    fputs("    size_t guid_len = sizeof(ws_guid) - 1U;\n", out);
    fputs("    size_t joined_len = key_len + guid_len;\n", out);
    fputs("    char *joined = (char*)cct_rt_alloc_or_fail(joined_len + 1U);\n", out);
    fputs("    unsigned char digest[20];\n", out);
    fputs("    memcpy(joined, key_s, key_len);\n", out);
    fputs("    memcpy(joined + key_len, ws_guid, guid_len);\n", out);
    fputs("    joined[joined_len] = '\\0';\n", out);
    fputs("    if (!SHA1((const unsigned char*)joined, joined_len, digest)) {\n", out);
    fputs("        cct_rt_free_ptr(joined);\n", out);
    fputs("        cct_rt_fail(\"crypto ws_accept_key failed\");\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_free_ptr(joined);\n", out);
    fputs("    return cct_rt_crypto_base64_encode(digest, sizeof(digest));\n", out);
    fputs("}\n\n", out);

    return true;
}
