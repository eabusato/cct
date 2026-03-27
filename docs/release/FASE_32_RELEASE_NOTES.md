# FASE 32 - Security, Cryptography, and Media - Release Notes

**Date:** 2026-03-22
**Version:** 0.32.0
**Status:** Completed

## Executive Summary

FASE 32 delivered the security and media processing layer of the CCT standard library. Ten new host-only modules cover cryptographic primitives, standard encodings, regular expressions, date/datetime handling, TOML configuration parsing, gzip compression, file type detection, media metadata extraction, image manipulation, and language detection.

These modules establish the foundation for production-grade applications that require cryptographic correctness, structured data handling, and media pipeline capabilities.

## Scope Closed

Delivered subphases:

- **32A**: `cct/crypto` — cryptographic primitives (SHA, HMAC, PBKDF2, CSPRNG)
- **32B**: `cct/encoding` — base64, hex, URL, HTML encodings
- **32C**: `cct/regex` — regular expression engine
- **32D**: `cct/date` — date and datetime types with arithmetic and formatting
- **32E**: `cct/toml` — TOML configuration parser with environment overlay
- **32F**: `cct/compress` — gzip compression via zlib
- **32G**: `cct/filetype` — magic-byte file type detection
- **32H**: `cct/media_probe` — media metadata extraction via ffprobe
- **32I**: `cct/image_ops` — image loading, transformation, and format conversion
- **32J**: `cct/text_lang` — automatic language detection

## Key Deliverables

### 32A — `cct/crypto`

- `crypto_sha256(VERBUM input) REDDE VERBUM` — hex-encoded SHA-256
- `crypto_sha512(VERBUM input) REDDE VERBUM` — hex-encoded SHA-512
- `crypto_hmac_sha256(VERBUM key, VERBUM msg) REDDE VERBUM`
- `crypto_hmac_sha512(VERBUM key, VERBUM msg) REDDE VERBUM`
- `crypto_pbkdf2(VERBUM pass, VERBUM salt, REX iter, REX keylen) REDDE VERBUM`
- `crypto_random_bytes(REX n) REDDE VERBUM` — CSPRNG, hex-encoded
- `crypto_const_eq(VERBUM a, VERBUM b) REDDE VERUM` — constant-time comparison

### 32B — `cct/encoding`

- Base64 encode/decode (standard and URL-safe)
- Hexadecimal encode/decode
- URL percent-encoding encode/decode
- HTML entity encode/decode

### 32C — `cct/regex`

- `regex_compile(VERBUM pattern) REDDE SPECULUM NIHIL`
- `regex_match`, `regex_search`, `regex_find_all`, `regex_replace`, `regex_split`
- Flags: case-insensitive, multiline, dotall

### 32D — `cct/date`

- `Date` and `DateTime` types (opaque handles)
- ISO 8601 parsing and configurable formatting
- Arithmetic: `date_add_days`, `date_add_months`, `datetime_diff_seconds`
- Timezone offset support, Unix timestamp conversion

### 32E — `cct/toml`

- `toml_parse_file(VERBUM path)`, `toml_parse_string(VERBUM src)`
- Access helpers: `toml_get_str`, `toml_get_int`, `toml_get_bool`, `toml_get_float`
- Table/array traversal, environment variable overlay

### 32F — `cct/compress`

- `compress_gzip(VERBUM data) REDDE VERBUM`
- `decompress_gzip(VERBUM data) REDDE VERBUM`

### 32G — `cct/filetype`

- `filetype_detect(VERBUM path) REDDE VERBUM` — magic-byte detection
- Categories: images (JPEG/PNG/GIF/WebP/BMP/SVG), video, audio, documents, text

### 32H — `cct/media_probe`

- `media_probe(VERBUM path) REDDE SPECULUM NIHIL` — metadata extraction via ffprobe
- Accessors: codec, width, height, fps, bitrate, duration, stream count

### 32I — `cct/image_ops`

- Load, save, resize, crop, rotate
- Format conversion: JPEG, PNG, GIF, BMP, WebP with quality control

### 32J — `cct/text_lang`

- `text_lang_detect(VERBUM text) REDDE VERBUM` — detected language code
- `text_lang_candidates(VERBUM text) REDDE FLUXUS GENUS(VERBUM)` — ranked candidates with confidence scores
- 10 supported languages via n-gram analysis

## Host-Only Contract

All FASE 32 modules are host-only. Programs using them must run in a hosted environment. Freestanding use is rejected at compile time with a clear diagnostic.

## Validation Summary

- ✅ 10 modules with complete CCT implementation and C bridge
- ✅ Freestanding rejection tests for each module
- ✅ Round-trip correctness tests (encoding, crypto, compress)
- ✅ Functional integration tests (regex, date arithmetic, TOML parsing)

## Breaking Changes

None. FASE 32 is purely additive.

## Dependencies

- 32H requires `ffprobe` in PATH at runtime
- 32I requires an image processing C library (libstb or equivalent) linked at build time
- 32F links zlib (assumed available on supported platforms)

---

**Next:** FASE 33 — Advanced Text and Parsing (`verbum` expansion, `lexer_util`, `uuid`, `slug`, `gettext`, `form_codec`)
