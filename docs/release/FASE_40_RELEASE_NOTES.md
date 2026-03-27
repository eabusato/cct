# FASE 40 - Media Bridges and Packaging - Release Notes

**Date:** 2026-03-27
**Version:** 0.40.0
**Status:** Completed

## Executive Summary

FASE 40 delivered the media and packaging layer of the CCT standard library. Three modules provide a canonical local media store with explicit lifecycle zones, ZIP archive creation and extraction, and an optional S3-compatible object storage bridge.

Before FASE 40, applications managed files with manual filesystem conventions, ad hoc directory naming, and no guaranteed atomicity or checksum. FASE 40 closes that gap: files are now first-class operational artifacts with identity, zone lifecycle, transport packaging, and an optional path to external object storage.

## Scope Closed

Delivered subphases:

- **40A**: `cct/media_store` — local media store with named zones, UUID/hash naming, SHA-256 checksum, and atomic promotion
- **40B**: `cct/archive_zip` — ZIP creation, listing, reading, and safe extraction
- **40C**: `cct/object_storage` — optional S3-compatible object storage bridge with signed URL support

## Key Deliverables

### 40A — `cct/media_store`

Local media store with five canonical zones and lifecycle-aware artifact management:

```cct
ADVOCARE "cct/media_store.cct"

EVOCA MediaStoreOptions opts AD CONIURA media_store_default_options("/var/app/media")
EVOCA MediaStore store AD CONIURA result_unwrap GENUS(MediaStore, VERBUM)(
  CONIURA media_store_open(opts)
)
CONIURA media_store_ensure_layout(store)

-- Ingest upload into tmp
EVOCA MediaArtifact art AD CONIURA result_unwrap GENUS(MediaArtifact, VERBUM)(
  CONIURA media_store_put_file(store, MEDIA_ZONE_TMP, "/upload/raw.jpg", "photo.jpg")
)

-- Promote atomically to processed after validation
EVOCA MediaArtifact ready AD CONIURA result_unwrap GENUS(MediaArtifact, VERBUM)(
  CONIURA media_store_promote(store, art, MEDIA_ZONE_PROCESSED)
)
```

#### Zones

| Zone | Lifetime | Use case |
|------|----------|----------|
| `MEDIA_ZONE_TMP` | Minutes–hours | Initial upload landing, not yet validated |
| `MEDIA_ZONE_QUARANTINE` | Minutes | Validation, virus scan, format check |
| `MEDIA_ZONE_PROCESSED` | Permanent | Final internal artifact |
| `MEDIA_ZONE_PUBLIC` | Permanent | Avatars, public thumbnails |
| `MEDIA_ZONE_PRIVATE` | Permanent | Documents, private media |

#### Naming Policies

- `MEDIA_NAMING_UUID` — UUID v4 only
- `MEDIA_NAMING_HASH` — SHA-256 digest
- `MEDIA_NAMING_UUID_WITH_EXT` — UUID + original extension (default)
- `MEDIA_NAMING_HASH_WITH_EXT` — hash + original extension

#### Public API

- `media_store_default_options(VERBUM root_dir) REDDE MediaStoreOptions`
- `media_store_open(MediaStoreOptions options) REDDE Result GENUS(MediaStore, VERBUM)`
- `media_store_ensure_layout(MediaStore store) REDDE Result GENUS(NIHIL, VERBUM)` — creates the 5 zone subdirectories
- `media_store_put_file(MediaStore store, MediaZone zone, VERBUM source_path, VERBUM original_filename) REDDE Result GENUS(MediaArtifact, VERBUM)`
- `media_store_promote(MediaStore store, MediaArtifact artifact, MediaZone destination) REDDE Result GENUS(MediaArtifact, VERBUM)` — atomic `rename(2)` when possible
- `media_store_copy(MediaStore store, MediaArtifact artifact, MediaZone destination) REDDE Result GENUS(MediaArtifact, VERBUM)` — copy preserving original; new `artifact_id`
- `media_store_delete(MediaStore store, MediaArtifact artifact) REDDE Result GENUS(NIHIL, VERBUM)` — path-validated removal
- `media_store_exists(MediaStore store, MediaArtifact artifact) REDDE VERUM`
- `media_store_absolute_path(MediaStore store, MediaArtifact artifact) REDDE VERBUM`
- `media_store_rechecksum(MediaStore store, MediaArtifact artifact) REDDE Result GENUS(VERBUM, VERBUM)` — recompute SHA-256 for audit
- `media_store_zone_name(MediaZone zone) REDDE VERBUM`

`MediaArtifact` fields: `artifact_id`, `zone`, `relative_path`, `filename`, `content_type`, `size_bytes`, `checksum`, `created_at_iso`, `updated_at_iso`.

### 40B — `cct/archive_zip`

ZIP archive creation, text/file entry writing, listing, reading, and safe extraction:

```cct
ADVOCARE "cct/archive_zip.cct"

EVOCA ZipArchive w AD CONIURA result_unwrap GENUS(ZipArchive, VERBUM)(
  CONIURA zip_create("/tmp/trace_package.zip")
)
CONIURA zip_add_file(w, "trace.svg", "path/to/trace.svg")
CONIURA zip_add_text(w, "metadata.json", "{\"trace_id\":\"abc123\"}")
CONIURA zip_add_text(w, "index.html", "<html><body><img src='trace.svg'/></body></html>")
CONIURA zip_close(SPECULUM w)
```

#### Public API

- `zip_create(VERBUM archive_path) REDDE Result GENUS(ZipArchive, VERBUM)`
- `zip_open(VERBUM archive_path) REDDE Result GENUS(ZipArchive, VERBUM)`
- `zip_close(SPECULUM ZipArchive archive)`
- `zip_add_file(ZipArchive archive, VERBUM source_path, VERBUM entry_name) REDDE Result GENUS(NIHIL, VERBUM)` — rejects traversal paths
- `zip_add_text(ZipArchive archive, VERBUM entry_name, VERBUM content) REDDE Result GENUS(NIHIL, VERBUM)` — for JSON, SVG, HTML, CSV
- `zip_list(ZipArchive archive) REDDE FLUXUS GENUS(ZipEntry)`
- `zip_entry_count(ZipArchive archive) REDDE REX`
- `zip_read_text(ZipArchive archive, VERBUM entry_name) REDDE Result GENUS(VERBUM, VERBUM)`
- `zip_extract_all(ZipArchive archive, ZipExtractOptions options) REDDE Result GENUS(NIHIL, VERBUM)` — validates every entry path against `output_dir`
- `zip_extract_entry(ZipArchive archive, VERBUM entry_name, ZipExtractOptions options) REDDE Result GENUS(NIHIL, VERBUM)`

`ZipEntry` fields: `name`, `size_bytes`, `is_dir`. `ZipExtractOptions` fields: `output_dir`, `overwrite`.

Path traversal (`..` and absolute paths) is rejected at both write and extract time. The underlying library is `libzip` or `miniz`; the CCT contract is stable regardless of implementation.

### 40C — `cct/object_storage`

Optional S3-compatible object storage bridge. Works with AWS S3, MinIO, Cloudflare R2, or any S3-compatible backend:

```cct
ADVOCARE "cct/object_storage.cct"

EVOCA ObjectStorageOptions opts AD CONIURA object_storage_default_options(
  endpoint, bucket, access_key, secret_key
)
EVOCA ObjectStorageClient client AD CONIURA result_unwrap GENUS(ObjectStorageClient, VERBUM)(
  CONIURA object_storage_open(opts)
)

CONIURA object_storage_put_file(client, "media/photo.jpg", local_path, "image/jpeg")
EVOCA SignedUrl su AD CONIURA result_unwrap GENUS(SignedUrl, VERBUM)(
  CONIURA object_storage_signed_url(client, "media/photo.jpg", 3600)
)
```

#### Public API

- `object_storage_default_options(VERBUM endpoint, VERBUM bucket, VERBUM access_key, VERBUM secret_key) REDDE ObjectStorageOptions`
- `object_storage_open(ObjectStorageOptions options) REDDE Result GENUS(ObjectStorageClient, VERBUM)`
- `object_storage_close(SPECULUM ObjectStorageClient client)`
- `object_storage_put_file(ObjectStorageClient client, VERBUM key, VERBUM source_path, VERBUM content_type) REDDE Result GENUS(ObjectRef, VERBUM)`
- `object_storage_put_text(ObjectStorageClient client, VERBUM key, VERBUM content, VERBUM content_type) REDDE Result GENUS(ObjectRef, VERBUM)`
- `object_storage_get_to_file(ObjectStorageClient client, VERBUM key, VERBUM dest_path) REDDE Result GENUS(NIHIL, VERBUM)`
- `object_storage_exists(ObjectStorageClient client, VERBUM key) REDDE Result GENUS(VERUM, VERBUM)` — distinguishes 404 from backend error
- `object_storage_head(ObjectStorageClient client, VERBUM key) REDDE Result GENUS(ObjectRef, VERBUM)`
- `object_storage_delete(ObjectStorageClient client, VERBUM key) REDDE Result GENUS(NIHIL, VERBUM)`
- `object_storage_signed_url(ObjectStorageClient client, VERBUM key, REX ttl_seconds) REDDE Result GENUS(SignedUrl, VERBUM)` — AWS Signature V4 or equivalent

`ObjectRef` fields: `bucket`, `key`, `content_type`, `etag`, `size_bytes`. `SignedUrl` fields: `url`, `expires_at_iso`.

Tests skip gracefully (exit code 0) when `APP_OBJ_STORAGE_ENDPOINT` is not set.

## Design Decisions

### Media as Artifacts with Explicit Lifecycle

The central idea of FASE 40 is not "file manipulation" — it is a predictable lifecycle: `tmp → quarantine → processed → public/private`. Every transition is a named operation with a documented contract. Applications no longer invent directory conventions; they call `media_store_promote`.

### Atomic Promotion

`media_store_promote` uses `rename(2)` when source and destination are on the same filesystem (the common case). A copy-then-delete fallback handles cross-device moves. The `artifact_id` and `checksum` are preserved across promotion; only `zone` and `relative_path` change.

### ZIP as Transport, Not Just Compression

`cct/archive_zip` exists primarily as a packaging tool for trace exports, offline-navigable bundles, and content import/export. The `zip_add_text` function handles the common case of adding JSON, SVG, and HTML payloads without requiring a separate file on disk.

### Path Traversal Is a Hard Error

Both `zip_add_file`/`zip_add_text` and `zip_extract_all`/`zip_extract_entry` reject entry names containing `..` or absolute paths. This is a hard `Err`, not a warning. ZIP slip is a classic vulnerability and the contract makes it impossible at the API level.

### Object Storage Is Optional for v1

`40C` is designed to be absent without breaking anything. The local `media_store` pipeline is fully functional without S3. `40C` adds the growth path for when local storage is no longer sufficient, without requiring it at launch.

## Host-Only Contract

All FASE 40 modules are host-only:
- `40A` requires host filesystem access (`runtime_media_store.c`)
- `40B` requires libzip or miniz (`runtime_archive_zip.c`)
- `40C` requires host networking with AWS Signature V4 support (`runtime_object_storage.c`)

Freestanding profile rejects all three modules at compile time.

## Validation Summary

- ✅ `media_store_default_options`: correct `naming_policy` and `checksum_algorithm` defaults
- ✅ `media_store_ensure_layout`: creates 5 canonical subdirectories
- ✅ `media_store_put_file`: non-empty `checksum` and `size_bytes > 0` on ingest
- ✅ `media_store_promote`: preserves `artifact_id` and `checksum`; updates `zone` and `relative_path`
- ✅ `media_store_rechecksum`: matches original checksum when file is unmodified
- ✅ `media_store_delete`: `exists` returns `FALSUM` after removal
- ✅ `media_store_delete`: rejects `relative_path` outside store root (no path traversal)
- ✅ `zip_create` + `zip_add_file` + `zip_add_text`: valid ZIP openable by external tools
- ✅ `zip_entry_count` and `zip_list`: correct count and entry names after write
- ✅ `zip_read_text`: returns exact content added by `zip_add_text`; `Err` for missing entry
- ✅ `zip_add_text`: rejects `..` and absolute entry names
- ✅ `zip_extract_all`: extracts to correct `output_dir` with path validation
- ✅ `zip_trace_package`: SVG + `.ctrace` + metadata + index packaged and readable
- ✅ `object_storage_put_file`: non-empty `etag` on success
- ✅ `object_storage_exists`: `Ok(FALSUM)` for missing key; `Err` for backend error
- ✅ `object_storage_head`: correct `content_type` and `size_bytes`
- ✅ `object_storage_get_to_file`: downloaded content matches uploaded content
- ✅ `object_storage_delete`: `exists` returns `Ok(FALSUM)` after deletion
- ✅ `object_storage_signed_url`: non-empty `url` and `expires_at_iso` in ISO 8601
- ✅ All three modules rejected in freestanding profile
- ✅ `40C` tests skip cleanly when `APP_OBJ_STORAGE_ENDPOINT` is absent

## Breaking Changes

None. FASE 40 is purely additive.

## Runtime Dependencies

- **40A**: no external library (filesystem operations via host libc)
- **40B**: libzip or miniz linked at build time
- **40C**: HTTP library with AWS Signature V4 support; live S3-compatible backend required for integration tests

---

**Next:** FASE 41 — (forthcoming)
