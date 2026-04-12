# Bibliotheca Canonica

## 1. What It Is

Bibliotheca Canonica is the official CCT standard-library layer.

In FASE 11A, this layer is foundational: namespace, resolver contract, distribution contract, and public policy are defined. Large module families are implemented in later 11.x subphases.

## 2. Reserved Namespace: `cct/...`

Canonical imports use the reserved namespace:

```cct
ADVOCARE "cct/verbum.cct"
ADVOCARE "cct/io.cct"
```

Resolver contract:
- `cct/...` is reserved to Bibliotheca Canonica.
- These imports are resolved from the canonical stdlib installation directory.
- User-local modules cannot override canonical `cct/...` modules.
- Resolver precedence: `CCT_STDLIB_DIR` environment variable, then build-time canonical stdlib root.

## 3. Distribution Policy

- Bibliotheca Canonica is distributed with the compiler.
- Versioning is compiler-coupled in FASE 11 (no independent package version graph yet).
- Current repository baseline location: `lib/cct/`.

## 4. Stability Classes

Public standard-library surfaces are classified as:
- Canonical Stable
- Canonical Experimental
- Runtime Internal

FASE 11A establishes this classification model; each module/doc later declares its class explicitly.

## 5. Error Contract

Two macro API styles are allowed:
- Strict API: fail through `IACE` with `FRACTUM`.
- Tentative API: explicit success/failure return channel.

Disallowed as dominant policy:
- silent errors
- arbitrary sentinel-only error signaling

## 6. Memory and Ownership Contract

Bibliotheca Canonica APIs must use explicit ownership semantics:
- allocation/discard responsibility is documented
- lifecycle expectations are predictable
- shallow/deep-copy behavior is explicit when relevant

No implicit ownership magic.

## 7. Import Side-Effect Policy

Importing `cct/...` is for symbol availability, not operational side effects.

If minimal initialization is unavoidable, it must be:
- rare
- controlled
- documented
- runtime-justified

## 8. Inspection Compatibility

Canonical modules must remain visible in standard analysis flows:
- `--ast`
- `--ast-composite`
- `--check`

No hidden, special-case opaque behavior for stdlib modules.

## 9. Naming Guidance

Public ritual/type names should be:
- concise
- predictable
- semantically clear
- consistent across modules

Avoid synonym sprawl and decorative ambiguity.

## 10. Builtins vs Runtime vs Canonical Library

| Layer | Meaning | Example |
|---|---|---|
| Builtin | Language-level intrinsic surface | `OBSECRO scribe`, `OBSECRO pete`, `OBSECRO libera`, `MENSURA` |
| Runtime helper | Generated C support internals | emitted helper symbols in `.cgen.c` |
| Bibliotheca Canonica | Importable standard modules | `ADVOCARE "cct/..."` |

## 11. Current Status (FASE 40 Baseline)

Delivered in 11A:
- canonical reserved namespace contract (`cct/...`)
- deterministic stdlib path resolution model
- physical stdlib root in repository (`lib/cct/`)
- foundational docs and test fixtures

Delivered after foundation:
- textual subsystem (`11B.1`, `11B.2`)
- collections and baseline algorithms (`11C`)
- memory utility foundation (`11D.1`)
- dynamic-vector canonical API (`11D.3`)
- IO and filesystem baseline (`11E.1`)
- path ergonomics baseline (`11E.2`)
- math/random utility baseline (`11F.1`)
- parse/cmp utility closure + moderate alg expansion (`11F.2`)
- canonical showcase/public face consolidation (`11G`)
- final subset/stability freeze and packaging closure (`11H`)
- language/runtime/tooling expansion phases (`12A` through `12H`)
- sigilo tooling, release hardening, and closure governance (`13`, `13M`, `14`)
- semantic/operator/freestanding bridge consolidation (`15`, `16`)
- canonical-library expansion for host tooling and low-level modules (`17`, `18`)
- language-surface expansion (`19A` through `19D`) integrating `ELIGE` (`CUM` legacy alias), `FORMA`, payload `ORDO`, and `ITERUM` over map/set.
- application-library expansion (`20A` through `20F`) integrating JSON, sockets/networking, HTTP, config, and SQLite.
- cryptography, encodings, and regex (`32A` through `32C`): `cct/crypto`, `cct/encoding`, `cct/regex`.
- date/time, TOML, and compression (`33A` through `33C`): `cct/datetime`, `cct/toml`, `cct/compress`.
- file types, media, images, language detection, advanced strings (`34A` through `34E`): `cct/filetype`, `cct/media`, `cct/image`, `cct/langdetect`, `cct/verbum_extra`.
- operational platform layer (`35A` through `35K`): `cct/lexer`, `cct/uuid`, `cct/slug`, `cct/i18n`, `cct/form`, `cct/log`, `cct/trace`, `cct/metrics`, `cct/signal`, `cct/watch`, `cct/audit`.
- operational database layer (`36A` through `36D`): `cct/db_postgres`, `cct/db_postgres_search`, `cct/redis`, `cct/db_postgres_lock`.
- transactional mail layer (`37A` through `37C`): `cct/mail`, `cct/mail_spool`, `cct/mail_webhook`.
- runtime instrumentation (`38A` through `38B`): `cct/instrument`, `cct/context_local`.
- trace visualization (`39A` through `39B`): `cct sigilo trace render/compare` CLI, animated SVG renderer, operational category overlays (C-only CLI tools; not CCT library modules).
- media bridges and packaging (`40A` through `40C`): `cct/media_store`, `cct/archive_zip`, `cct/object_storage`.

Current phase closure references:
- `docs/release/FASE_40_RELEASE_NOTES.md`
- `docs/release/FASE_39_RELEASE_NOTES.md`
- `docs/release/FASE_38_RELEASE_NOTES.md`
- `docs/release/FASE_37_RELEASE_NOTES.md`
- `docs/release/FASE_36_RELEASE_NOTES.md`
- `docs/release/FASE_35_RELEASE_NOTES.md`
- `docs/release/FASE_34_RELEASE_NOTES.md`
- `docs/release/FASE_33_RELEASE_NOTES.md`
- `docs/release/FASE_32_RELEASE_NOTES.md`
- `docs/release/FASE_20_RELEASE_NOTES.md`
- `docs/bootstrap/FASE_20_HANDOFF.md`
- `docs/release/FASE_19_RELEASE_NOTES.md`
- `docs/bootstrap/FASE_19_HANDOFF.md`

Operational note:
- Bibliotheca Canonica now reflects the complete public standard-library surface delivered through FASE 40.
- Host-only modules added in FASE 32 through FASE 40 remain part of the validated Linux baseline.
- The immediate publication target for this repository state is release `v0.40`.

## 11A. `cct/db_sqlite` (FASE 20E)

`cct/db_sqlite` is the canonical host-only persistence module for local SQLite workflows.

Import:

```cct
ADVOCARE "cct/db_sqlite.cct"
```

Public API in the current delivery:
- `db_open(VERBUM path) -> SPECULUM NIHIL`
- `db_close(SPECULUM NIHIL db) -> NIHIL`
- `db_exec(SPECULUM NIHIL db, VERBUM sql) -> NIHIL`
- `db_last_error(SPECULUM NIHIL db) -> VERBUM`
- `db_query(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `rows_next(SPECULUM NIHIL rows) -> VERUM`
- `rows_get_text(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `rows_get_int(SPECULUM NIHIL rows, REX col) -> REX`
- `rows_get_real(SPECULUM NIHIL rows, REX col) -> UMBRA`
- `rows_close(SPECULUM NIHIL rows) -> NIHIL`
- `db_prepare(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `stmt_bind_text(SPECULUM NIHIL stmt, REX idx, VERBUM v) -> NIHIL`
- `stmt_bind_int(SPECULUM NIHIL stmt, REX idx, REX v) -> NIHIL`
- `stmt_bind_real(SPECULUM NIHIL stmt, REX idx, UMBRA v) -> NIHIL`
- `stmt_step(SPECULUM NIHIL stmt) -> VERUM`
- `stmt_reset(SPECULUM NIHIL stmt) -> NIHIL`
- `stmt_finalize(SPECULUM NIHIL stmt) -> NIHIL`
- `db_begin(SPECULUM NIHIL db) -> NIHIL`
- `db_commit(SPECULUM NIHIL db) -> NIHIL`
- `db_rollback(SPECULUM NIHIL db) -> NIHIL`
- `db_scalar_int(SPECULUM NIHIL db, VERBUM sql) -> REX`
- `db_scalar_text(SPECULUM NIHIL db, VERBUM sql) -> VERBUM`

Dependency and fallback policy:
- available only in the host profile
- generated host programs add `-lsqlite3` only when the SQLite surface is used
- if the host toolchain does not provide SQLite, the host compile/link step must fail clearly

Ownership policy:
- `db_open` allocates the DB handle; callers close it with `db_close`
- `db_query` allocates a rows handle; callers release it with `rows_close`
- `db_prepare` allocates a statement handle; callers release it with `stmt_finalize`
- textual getters return copied `VERBUM` values in the current host-backed subset

## 12. `cct/verbum` (FASE 11B.1)

`cct/verbum` is the canonical text core module.

Import:

```cct
ADVOCARE "cct/verbum.cct"
```

Public API:
- `len(VERBUM s) -> REX`
- `concat(VERBUM a, VERBUM b) -> VERBUM`
- `compare(VERBUM a, VERBUM b) -> REX`
- `substring(VERBUM s, REX inicio, REX fim) -> VERBUM`
- `trim(VERBUM s) -> VERBUM`
- `contains(VERBUM s, VERBUM sub) -> VERUM`
- `find(VERBUM s, VERBUM sub) -> REX`

Ownership policy:
- string literals are runtime-managed
- `concat`, `substring`, and `trim` allocate new text values
- caller-owned discard policy remains explicit in the current subset

Error policy:
- `substring` is strict and fails on invalid bounds
- `find` returns `-1` when not found
- `contains` returns `FALSUM` when not found

Encoding policy:
- pragmatic ASCII/UTF-8-bytes behavior
- no advanced Unicode normalization in this subset

## 13. `cct/fmt` (FASE 11B.2)

`cct/fmt` is the canonical formatting and conversion layer, built on `cct/verbum`.

Import:

```cct
ADVOCARE "cct/fmt.cct"
```

Public API:
- `stringify_int(REX)` -> `VERBUM`
- `stringify_real(UMBRA)` -> `VERBUM`
- `stringify_float(FLAMMA)` -> `VERBUM`
- `stringify_bool(VERUM)` -> `VERBUM`
- `fmt_parse_int(VERBUM)` -> `REX`
- `fmt_parse_real(VERBUM)` -> `UMBRA`
- `fmt_parse_bool(VERBUM)` -> `VERUM`
- `format_pair(VERBUM, VERBUM)` -> `VERBUM`

Policy:
- stringify operations return freshly allocated textual values
- parse operations are strict in this subset and fail clearly on invalid input
- `format_pair` provides canonical label-value composition (`label: value`)

## 14. `cct/series` (FASE 11C)

`cct/series` is the canonical static-collection helper module for `SERIES` workflows.

Import:

```cct
ADVOCARE "cct/series.cct"
```

Public API:
- `series_len GENUS(T)(SPECULUM T arr, REX size) -> REX`
- `series_fill GENUS(T)(SPECULUM T arr, T valor, REX size) -> NIHIL`
- `series_copy GENUS(T)(SPECULUM T dest, SPECULUM T src, REX size) -> NIHIL`
- `series_reverse GENUS(T)(SPECULUM T arr, REX size) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX size) -> VERUM`

Policy:
- explicit-size (`size`) contract is caller-driven in this subset
- generic mutation helpers are supported (`series_fill`, `series_copy`, `series_reverse`)
- `series_contains` is intentionally integer-specialized in this first delivery

## 15. `cct/alg` (FASE 11C)

`cct/alg` is the baseline deterministic algorithm module for integer-array workflows.

Import:

```cct
ADVOCARE "cct/alg.cct"
```

Public API:
- `alg_linear_search(SPECULUM REX arr, REX valor, REX size) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX size) -> VERUM`

Policy:
- algorithms are intentionally minimal in 11C
- deterministic, predictable behavior over integer arrays
- advanced algorithms remain planned for later subphases

## 16. `cct/mem` (FASE 11D.1)

`cct/mem` is the canonical runtime-backed memory utility module for explicit ownership workflows.

Import:

```cct
ADVOCARE "cct/mem.cct"
```

Public API:
- `alloc(REX size) -> SPECULUM REX`
- `free(SPECULUM REX ptr) -> NIHIL`
- `realloc(SPECULUM REX ptr, REX new_size) -> SPECULUM REX`
- `copy(SPECULUM REX dest, SPECULUM REX src, REX size) -> NIHIL`
- `set(SPECULUM REX ptr, REX value, REX size) -> NIHIL`
- `zero(SPECULUM REX ptr, REX size) -> NIHIL`
- `mem_compare(SPECULUM REX a, SPECULUM REX b, REX size) -> REX`

Policy:
- explicit ownership: allocation callers must release with `free`
- no implicit lifetime management in this subset
- module is the memory foundation for `FLUXUS` storage phases (11D.2/11D.3)

Reference:
- `docs/ownership_contract.md`

## 17. `cct/fluxus` (FASE 11D.3)

`cct/fluxus` is the canonical dynamic-vector API built on the 11D.2 runtime storage core.

Import:

```cct
ADVOCARE "cct/fluxus.cct"
```

Public API:
- `fluxus_init(REX elem_size) -> SPECULUM NIHIL`
- `fluxus_free(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_push(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_pop(SPECULUM NIHIL flux, SPECULUM NIHIL out) -> NIHIL`
- `fluxus_len(SPECULUM NIHIL flux) -> REX`
- `fluxus_get(SPECULUM NIHIL flux, REX idx) -> SPECULUM NIHIL`
- `fluxus_clear(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_reserve(SPECULUM NIHIL flux, REX cap) -> NIHIL`
- `fluxus_capacity(SPECULUM NIHIL flux) -> REX`

Policy:
- explicit ownership: caller must release with `fluxus_free`

## 18. `cct/io` (FASE 11E.1)

`cct/io` is the canonical standard IO facade for text output and line input.

Import:

```cct
ADVOCARE "cct/io.cct"
```

Public API:
- `print(VERBUM s) -> NIHIL`
- `println(VERBUM s) -> NIHIL`
- `print_int(REX n) -> NIHIL`
- `read_line() -> VERBUM`

Policy:
- runtime-backed host IO bridge
- `read_line` returns caller-owned text value
- predictable, minimal subset for CLI/program interaction

## 19. `cct/fs` (FASE 11E.1)

`cct/fs` is the canonical filesystem facade for whole-file operations.

Import:

```cct
ADVOCARE "cct/fs.cct"
```

Public API:
- `read_all(VERBUM path) -> VERBUM`
- `write_all(VERBUM path, VERBUM content) -> NIHIL`
- `append_all(VERBUM path, VERBUM content) -> NIHIL`
- `exists(VERBUM path) -> VERUM`
- `size(VERBUM path) -> REX`

Policy:
- strict error behavior on invalid read/size paths
- explicit ownership for text returned by `read_all`
- runtime-backed host filesystem bridge (no advanced path model here; reserved for 11E.2)

## 20. `cct/path` (FASE 11E.2)

`cct/path` is the canonical path-manipulation facade with a minimal ergonomic surface.

Import:

```cct
ADVOCARE "cct/path.cct"
```

Public API:
- `path_join(VERBUM a, VERBUM b) -> VERBUM`
- `path_basename(VERBUM p) -> VERBUM`
- `path_dirname(VERBUM p) -> VERBUM`
- `path_ext(VERBUM p) -> VERBUM`

Policy:
- canonical output separator is `/`
- input accepts both `/` and `\\` for basic parsing
- API stays intentionally small and filesystem-agnostic
- integrates with `cct/fs` by plain `VERBUM` composition (no hidden side effects)

## 21. `cct/math` (FASE 11F.1)

`cct/math` is the canonical minimal numeric utility module.

Import:

```cct
ADVOCARE "cct/math.cct"
```

Public API:
- `abs(REX x) -> REX`
- `min(REX a, REX b) -> REX`
- `max(REX a, REX b) -> REX`
- `clamp(REX x, REX lo, REX hi) -> REX`

Policy:
- deterministic integer-oriented helpers
- strict invalid-interval behavior for `clamp(lo > hi)`

## 22. `cct/random` (FASE 11F.1)

`cct/random` is the canonical basic PRNG utility module.

Import:

```cct
ADVOCARE "cct/random.cct"
```

Public API:
- `seed(REX s) -> NIHIL`
- `random_int(REX lo, REX hi) -> REX`
- `random_real() -> FLAMMA`

Policy:
- runtime-backed pseudo-random generation
- reproducible sequence baseline through `seed`
- strict invalid-range behavior for `random_int(lo > hi)`

## 23. `cct/parse` (FASE 11F.2)

`cct/parse` is the canonical strict textual-conversion module.

Import:

```cct
ADVOCARE "cct/parse.cct"
```

Public API:
- `parse_int(VERBUM text) -> REX`
- `parse_real(VERBUM text) -> UMBRA`
- `parse_bool(VERBUM text) -> VERUM`

Policy:
- strict conversion semantics for primitive values
- invalid integer/real input fails clearly
- invalid boolean input fails with explicit failure message

## 24. `cct/cmp` (FASE 11F.2)

`cct/cmp` is the canonical comparator module.

Import:

```cct
ADVOCARE "cct/cmp.cct"
```

Public API:
- `cmp_int(REX a, REX b) -> REX`
- `cmp_real(UMBRA a, UMBRA b) -> REX`
- `cmp_bool(VERUM a, VERUM b) -> REX`
- `cmp_verbum(VERBUM a, VERBUM b) -> REX`

Policy:
- comparator return contract is canonical (`<0`, `0`, `>0`)
- comparator APIs remain intentionally small and deterministic

## 25. `cct/alg` Expansion Notes (FASE 11F.2)

Added in 11F.2:
- `alg_binary_search(SPECULUM REX arr, REX size, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX size) -> NIHIL`

Policy:
- moderate expansion only (no framework-like algorithm surface)
- integer-array focused and explicit-size oriented

## 26. Canonical Showcases (FASE 11G)

The following examples are part of the official public-facing stdlib surface:

- `examples/showcase_stdlib_string_11g.cct`
- `examples/showcase_stdlib_collection_11g.cct`
- `examples/showcase_stdlib_io_fs_11g.cct`
- `examples/showcase_stdlib_parse_math_random_11g.cct`
- `examples/showcase_stdlib_modular_11g_main.cct`

Module mapping:
- string showcase: `cct/verbum`, `cct/fmt`, `cct/io`
- collection showcase: `cct/series`, `cct/alg`, `cct/fluxus`, `cct/io`
- io/fs showcase: `cct/path`, `cct/fs`, `cct/io`
- parse/math/random showcase: `cct/parse`, `cct/math`, `cct/random`, `cct/cmp`, `cct/io`
- modular showcase: user modules + `cct/verbum`, `cct/fmt`, `cct/parse`, `cct/cmp`, `cct/math`, `cct/random`, `cct/io`, `cct/fs`, `cct/path`

Showcase observability contract:
- all examples are validated in `tests/integration/showcase_*_11g.cct`
- multi-module showcase is validated with `--ast-composite`
- sigilo metadata includes stdlib usage counters and `stdlib_modules_used` inventory

## 27. Final Subset and Stability Freeze (FASE 11H)

Final canonical subset reference:
- `docs/stdlib_subset_11h.md`

Final stability matrix reference:
- `docs/stdlib_stability_matrix_11h.md`

Release notes:
- `docs/release/FASE_11_RELEASE_NOTES.md`

Installation/packaging guide:
- `docs/install.md`

## 28. `cct/option` and `cct/result` (FASE 12C)

FASE 12C adds a functional error/value-absence layer on top of the existing
failure-control model (`IACE`/`TEMPTA`/`CAPE`/`SEMPER`), without replacing it.

### `cct/option`

Canonical Option surface:
- constructors: `Some GENUS(T)(value)`, `None GENUS(T)()`
- checks: `option_is_some`, `option_is_none`
- extraction: `option_unwrap`, `option_unwrap_or`, `option_expect`
- lifecycle: `option_free`

### `cct/result`

Canonical Result surface:
- constructors: `Ok GENUS(T,E)(value)`, `Err GENUS(T,E)(error)`
- checks: `result_is_ok`, `result_is_err`
- extraction: `result_unwrap`, `result_unwrap_or`, `result_unwrap_err`, `result_expect`
- lifecycle: `result_free`

### Contract notes

- Option/Result values are represented as opaque pointers in this subset.
- Callers own returned instances and must release them with `option_free` / `result_free`.
- `unwrap`/`expect` variants fail clearly on invalid state (`None`/`Err`/`Ok` mismatch).
- Sigilo metadata now tracks `option_ops_count` and `result_ops_count`.

## 29. `cct/map` and `cct/set` (FASE 12D.1)

FASE 12D.1 introduces canonical hash-backed containers for key/value and unique-item workflows.

### `cct/map`

Canonical Map surface:
- lifecycle: `map_init GENUS(K,V)()`, `map_free GENUS(K,V)(map)`
- mutation: `map_insert`, `map_remove`, `map_clear`, `map_reserve`
- query: `map_get`, `map_contains`, `map_len`, `map_is_empty`, `map_capacity`

Behavior notes:
- map instances are opaque pointer-backed runtime objects
- `map_get` returns Option payload pointers (`Some` / `None` contract)
- duplicate key insertion updates value in place and preserves key cardinality
- `ITERUM key, value IN map` iterates entries in insertion order (FASE 19D)

### `cct/set`

Canonical Set surface:
- lifecycle: `set_init GENUS(T)()`, `set_free GENUS(T)(set)`
- mutation: `set_insert`, `set_remove`, `set_clear`
- query: `set_contains`, `set_len`, `set_is_empty`

Behavior notes:
- set instances are opaque pointer-backed runtime objects
- duplicate insertion is ignored (`set_insert` returns `FALSUM` for existing items)
- callers own container lifetime and must release with `set_free`
- `ITERUM item IN set` iterates items in insertion order (FASE 19D)

Sigilo integration:
- metadata tracks `map_ops_count` and `set_ops_count`

## 30. `cct/collection_ops` (FASE 12D.2)

FASE 12D.2 adds canonical functional combinators for FLUXUS and SERIES workflows.

### `cct/collection_ops`

Canonical surface:
- FLUXUS: `fluxus_map`, `fluxus_filter`, `fluxus_fold`, `fluxus_find`, `fluxus_any`, `fluxus_all`
- SERIES: `series_map`, `series_filter`, `series_reduce`, `series_find`, `series_any`, `series_all`

Behavior notes:
- callback parameters are rituale pointers (`SPECULUM NIHIL`) in this subset
- callbacks must be named rituales (no closure capture in 12D.2)
- bridge is runtime-backed and currently validated for word-sized callback payload flows
- `fluxus_find` / `series_find` return Option pointers, preserving the 12C contract

Sigilo integration:
- metadata tracks `collection_ops_count`
- metadata tracks `collection_ops_module_used`

## 31. Reference Module: `ordo_samples` (FASE 19C/19D)

`lib/cct/ordo_samples.cct` is not a required production runtime module.
It exists as an idiomatic reference for payload-bearing ORDO and for combined use
of `ELIGE` and `FORMA`.

Canonical reference patterns:
- `Resultado` (return with error): `Ok(REX value)` / `Err(VERBUM msg)`
- `Opcao` (value present/absent): `Algum(REX value)` / `Nenhum`

`Resultado` pattern example:

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO
```

`Opcao` pattern example:

```cct
ORDO Opcao
  Algum(REX valor),
  Nenhum
FIN ORDO
```

Guidelines:
- use these definitions as the baseline for domain types.
- for pointer-backed APIs, `cct/option` and `cct/result` remain available.
- the FASE 19D expansion of `ITERUM` complements these patterns with iteration in
  insertion order for `map` and `set`.

### 31.1 Expansions 17-19: functional guide by family

Text parsing/composition (17A/17B/18A):
- `cct/char`, `cct/args`, `cct/verbum_scan`, `cct/verbum_builder`, `cct/code_writer`, `cct/verbum`, `cct/fmt`, `cct/parse`.
- focus on host tooling, deterministic text construction, strict parsing, and safe parsing (`try_*`).

Host environment and bytes (17D):
- `cct/env`, `cct/time`, `cct/bytes`.
- focus on process/time introspection and mutable byte buffers for compiler/tooling flows.

IO/filesystem/path/process (18B/18D):
- `cct/io`, `cct/fs`, `cct/path`, `cct/process`.
- focus on host operations with explicit error contracts and profile boundaries (host vs freestanding).

Collections and algorithms (18C + 19D integration):
- `cct/fluxus`, `cct/set`, `cct/map`, `cct/series`, `cct/alg`, `cct/collection_ops`.
- focus on collection composition, functional transformations, and deterministic iteration.
- in FASE 19D, `ITERUM` extends to `map`/`set` in the core language.

Low-level utility modules (18D):
- `cct/hash`, `cct/bit`, `cct/random`.
- focus on deterministic hashing, bit operations, and non-cryptographic utility randomness.

ADT/pattern integration reference (19C/19D):
- `cct/ordo_samples` as the idiomatic baseline for payload `ORDO` with `ELIGE` and `FORMA`.
- generic pointer-backed contracts (`cct/option`, `cct/result`) remain valid for interoperability.

Application-stack modules (20A-20E):
- `cct/json`, `cct/socket`, `cct/net`, `cct/http`, `cct/config`, `cct/db_sqlite`.
- focus on practical host-side application workflows with explicit ownership and failure contracts.
- protocol/model/config logic stays in CCT where possible; runtime C additions remain narrow at host boundaries.

## 32. Complete Function Inventory (FASE 20F)

- This section consolidates the complete Bibliotheca Canonica surface delivered through FASE 19D.4.
- The goal is full traceability: no canonical module function is left undocumented.
- For detailed domain semantics, use the descriptive sections earlier in this document and `docs/spec.md`.

<!-- BEGIN AUTO API INVENTORY 19D4 -->
<!-- AUTO-GENERATED from lib/cct/*.cct and lib/cct/kernel/*.cct -->
**Coverage**: complete inventory of canonical functions in `lib/cct` (FASE 20F).

### `cct/alg`

- `alg_linear_search(SPECULUM REX arr, REX valor, REX size) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX size) -> VERUM`
- `alg_binary_search(SPECULUM REX arr, REX size, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX size) -> NIHIL`
- `alg_sum(SPECULUM REX arr, REX n) -> REX`
- `alg_sum_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_min(SPECULUM REX arr, REX n) -> REX`
- `alg_max(SPECULUM REX arr, REX n) -> REX`
- `alg_min_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_max_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_reverse_range(SPECULUM REX arr, REX begin, REX end_exclusive) -> NIHIL`
- `alg_reverse(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_fill(SPECULUM REX arr, REX n, REX value) -> NIHIL`
- `alg_fill_real(SPECULUM UMBRA arr, REX n, UMBRA value) -> NIHIL`
- `alg_rotate(SPECULUM REX arr, REX n, REX k) -> NIHIL`
- `alg_quicksort(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_mergesort(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_sort_verbum(SPECULUM NIHIL arr, REX n) -> NIHIL`
- `alg_is_sorted(SPECULUM REX arr, REX n) -> VERUM`
- `alg_count(SPECULUM REX arr, REX n, REX value) -> REX`
- `alg_deduplicate_sorted(SPECULUM REX arr, REX n) -> REX`
- `alg_dot_product(SPECULUM REX a, SPECULUM REX b, REX n) -> REX`
- `alg_dot_product_real(SPECULUM UMBRA a, SPECULUM UMBRA b, REX n) -> UMBRA`

Module total: **23**

### `cct/args`

- `argc() -> REX`
- `arg(REX i) -> VERBUM`

Module total: **2**

### `cct/ast_node`

- `ast_make_ident(SPECULUM AstIdent p) -> Variant`
- `ast_make_literal_int(SPECULUM AstLiteralInt p) -> Variant`
- `ast_make_binary(SPECULUM AstBinary p) -> Variant`
- `ast_is_ident(Variant n) -> VERUM`
- `ast_is_literal_int(Variant n) -> VERUM`
- `ast_is_binary(Variant n) -> VERUM`
- `ast_as_ident(Variant n) -> SPECULUM AstIdent`
- `ast_as_literal_int(Variant n) -> SPECULUM AstLiteralInt`
- `ast_as_binary(Variant n) -> SPECULUM AstBinary`

Module total: **9**

### `cct/bit`

- `bit_get(REX n, REX pos) -> VERUM`
- `bit_set(REX n, REX pos) -> REX`
- `bit_clear(REX n, REX pos) -> REX`
- `bit_toggle(REX n, REX pos) -> REX`
- `popcount(REX n) -> REX`
- `leading_zeros(REX n) -> REX`
- `trailing_zeros(REX n) -> REX`
- `rotate_left(REX n, REX k) -> REX`
- `rotate_right(REX n, REX k) -> REX`
- `next_power_of_2(REX n) -> REX`
- `is_power_of_2(REX n) -> VERUM`
- `byte_swap(REX n) -> REX`
- `parity(REX n) -> VERUM`
- `bit_extract(REX n, REX start, REX len) -> REX`

Module total: **14**

### `cct/bytes`

- `bytes_new(REX size) -> SPECULUM NIHIL`
- `bytes_len(SPECULUM NIHIL b) -> REX`
- `bytes_get(SPECULUM NIHIL b, REX i) -> MILES`
- `bytes_set(SPECULUM NIHIL b, REX i, MILES v) -> NIHIL`
- `bytes_free(SPECULUM NIHIL b) -> NIHIL`

Module total: **5**

### `cct/char`

- `is_digit(MILES c) -> VERUM`
- `is_alpha(MILES c) -> VERUM`
- `is_whitespace(MILES c) -> VERUM`

Module total: **3**

### `cct/cmp`

- `cmp_int(REX a, REX b) -> REX`
- `cmp_real(UMBRA a, UMBRA b) -> REX`
- `cmp_bool(VERUM a, VERUM b) -> REX`
- `cmp_verbum(VERBUM a, VERBUM b) -> REX`

Module total: **4**

### `cct/code_writer`

- `writer_init() -> SPECULUM NIHIL`
- `writer_indent(SPECULUM NIHIL w) -> NIHIL`
- `writer_dedent(SPECULUM NIHIL w) -> NIHIL`
- `writer_write(SPECULUM NIHIL w, VERBUM s) -> NIHIL`
- `writer_writeln(SPECULUM NIHIL w, VERBUM s) -> NIHIL`
- `writer_write_int(SPECULUM NIHIL w, REX v) -> NIHIL`
- `writer_writeln_int(SPECULUM NIHIL w, REX v) -> NIHIL`
- `writer_write_bool(SPECULUM NIHIL w, VERUM v) -> NIHIL`
- `writer_to_verbum(SPECULUM NIHIL w) -> VERBUM`
- `writer_free(SPECULUM NIHIL w) -> NIHIL`

Module total: **10**

### `cct/collection_ops`

- `fluxus_map GENUS(T, R) -> SPECULUM NIHIL`
- `fluxus_filter GENUS(T) -> SPECULUM NIHIL`
- `fluxus_fold GENUS(T, Acc) -> Acc`
- `fluxus_find GENUS(T) -> SPECULUM NIHIL`
- `fluxus_any GENUS(T) -> VERUM`
- `fluxus_all GENUS(T) -> VERUM`
- `series_map GENUS(T, R) -> SPECULUM NIHIL`
- `series_filter GENUS(T) -> SPECULUM NIHIL`
- `series_reduce GENUS(T, Acc) -> Acc`
- `series_find GENUS(T) -> SPECULUM NIHIL`
- `series_any GENUS(T) -> VERUM`
- `series_all GENUS(T) -> VERUM`

Module total: **12**

### `cct/config`

- `config_alloc(REX bytes) -> SPECULUM NIHIL`
- `config_values_new() -> REX`
- `config_sections_new_handle() -> REX`
- `config_values_len(REX handle) -> REX`
- `config_values_get_ptr(REX handle, REX idx) -> SPECULUM NIHIL`
- `config_values_push(REX handle, SPECULUM NIHIL ptr) -> NIHIL`
- `config_sections_len(SPECULUM NIHIL secs_ptr) -> REX`
- `config_sections_count(REX handle) -> REX`
- `config_section_get_ptr(REX handle, REX idx) -> SPECULUM NIHIL`
- `config_section_name_at(SPECULUM NIHIL secs_ptr, REX idx) -> VERBUM`
- `config_trim_cr(VERBUM s) -> VERBUM`
- `config_split_first_line(VERBUM s) -> ConfigChunk`
- `config_ref(SPECULUM NIHIL cfg_ptr) -> SPECULUM ConfigState`
- `config_track_section(SPECULUM NIHIL cfg_ptr, VERBUM secao) -> NIHIL`
- `config_find_value_index(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> REX`
- `config_set(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave, VERBUM valor) -> NIHIL`
- `config_remove(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> VERUM`
- `config_new() -> SPECULUM NIHIL`
- `config_parse_ini(VERBUM src) -> SPECULUM NIHIL`
- `config_load_ini(VERBUM path) -> SPECULUM NIHIL`
- `config_sections(SPECULUM NIHIL cfg_ptr) -> SPECULUM NIHIL`
- `config_has(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> VERUM`
- `config_get_or(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave, VERBUM padrao) -> VERBUM`
- `config_get(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> VERBUM`
- `config_get_int(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> REX`
- `config_get_bool(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> VERUM`
- `config_get_real(SPECULUM NIHIL cfg_ptr, VERBUM secao, VERBUM chave) -> UMBRA`
- `config_stringify_ini(SPECULUM NIHIL cfg_ptr) -> VERBUM`
- `config_write_ini(SPECULUM NIHIL cfg_ptr, VERBUM path) -> NIHIL`
- `config_env_name(VERBUM prefixo, VERBUM secao, VERBUM chave) -> VERBUM`
- `config_apply_env_prefix(SPECULUM NIHIL cfg_ptr, VERBUM prefixo) -> NIHIL`
- `config_json_value_to_verbum(Json j) -> VERBUM`
- `config_section_to_json(SPECULUM NIHIL cfg_ptr, VERBUM secao) -> Json`
- `config_from_json(Json j) -> SPECULUM NIHIL`

Module total: **34**

### `cct/db_sqlite`

- `db_open(VERBUM path) -> SPECULUM NIHIL`
- `db_close(SPECULUM NIHIL db) -> NIHIL`
- `db_exec(SPECULUM NIHIL db, VERBUM sql) -> NIHIL`
- `db_last_error(SPECULUM NIHIL db) -> VERBUM`
- `db_query(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `rows_next(SPECULUM NIHIL rows) -> VERUM`
- `rows_get_text(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `rows_get_int(SPECULUM NIHIL rows, REX col) -> REX`
- `rows_get_real(SPECULUM NIHIL rows, REX col) -> UMBRA`
- `rows_close(SPECULUM NIHIL rows) -> NIHIL`
- `db_prepare(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `stmt_bind_text(SPECULUM NIHIL stmt, REX idx, VERBUM v) -> NIHIL`
- `stmt_bind_int(SPECULUM NIHIL stmt, REX idx, REX v) -> NIHIL`
- `stmt_bind_real(SPECULUM NIHIL stmt, REX idx, UMBRA v) -> NIHIL`
- `stmt_step(SPECULUM NIHIL stmt) -> VERUM`
- `stmt_reset(SPECULUM NIHIL stmt) -> NIHIL`
- `stmt_finalize(SPECULUM NIHIL stmt) -> NIHIL`
- `db_begin(SPECULUM NIHIL db) -> NIHIL`
- `db_commit(SPECULUM NIHIL db) -> NIHIL`
- `db_rollback(SPECULUM NIHIL db) -> NIHIL`
- `db_scalar_int(SPECULUM NIHIL db, VERBUM sql) -> REX`
- `db_scalar_text(SPECULUM NIHIL db, VERBUM sql) -> VERBUM`

Module total: **22**

### `cct/env`

- `getenv(VERBUM name) -> VERBUM`
- `has_env(VERBUM name) -> VERUM`
- `cwd() -> VERBUM`

Module total: **3**

### `cct/fluxus`

- `fluxus_init(REX elem_size) -> SPECULUM NIHIL`
- `fluxus_free(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_push(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_pop(SPECULUM NIHIL flux, SPECULUM NIHIL out) -> NIHIL`
- `fluxus_len(SPECULUM NIHIL flux) -> REX`
- `fluxus_get(SPECULUM NIHIL flux, REX idx) -> SPECULUM NIHIL`
- `fluxus_clear(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_reserve(SPECULUM NIHIL flux, REX cap) -> NIHIL`
- `fluxus_capacity(SPECULUM NIHIL flux) -> REX`
- `fluxus_peek(SPECULUM NIHIL flux) -> SPECULUM NIHIL`
- `fluxus_set(SPECULUM NIHIL flux, REX idx, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_remove(SPECULUM NIHIL flux, REX idx) -> NIHIL`
- `fluxus_insert(SPECULUM NIHIL flux, REX idx, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_contains(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> VERUM`
- `fluxus_is_empty(SPECULUM NIHIL flux) -> VERUM`
- `fluxus_concat GENUS(T) -> SPECULUM NIHIL`
- `fluxus_slice GENUS(T) -> SPECULUM NIHIL`
- `fluxus_copy GENUS(T) -> SPECULUM NIHIL`
- `fluxus_reverse(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_sort_int(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_sort_verbum(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_to_ptr(SPECULUM NIHIL flux) -> SPECULUM NIHIL`

Module total: **22**

### `cct/fmt`

- `stringify_int(REX x) -> VERBUM`
- `stringify_real(UMBRA x) -> VERBUM`
- `stringify_float(FLAMMA x) -> VERBUM`
- `stringify_bool(VERUM b) -> VERBUM`
- `fmt_parse_int(VERBUM s) -> REX`
- `fmt_parse_real(VERBUM s) -> UMBRA`
- `fmt_parse_bool(VERBUM s) -> VERUM`
- `format_pair(VERBUM label, VERBUM value) -> VERBUM`
- `stringify_int_hex(REX n) -> VERBUM`
- `stringify_int_hex_upper(REX n) -> VERBUM`
- `stringify_int_oct(REX n) -> VERBUM`
- `stringify_int_bin(REX n) -> VERBUM`
- `stringify_uint(DUX n) -> VERBUM`
- `stringify_int_padded(REX n, REX width, MILES fill) -> VERBUM`
- `stringify_real_prec(UMBRA x, REX prec) -> VERBUM`
- `stringify_real_sci(UMBRA x) -> VERBUM`
- `stringify_real_fixed(UMBRA x, REX prec) -> VERBUM`
- `stringify_char(MILES c) -> VERBUM`
- `format_1(VERBUM tmpl, VERBUM a) -> VERBUM`
- `format_2(VERBUM tmpl, VERBUM a, VERBUM b) -> VERBUM`
- `format_3(VERBUM tmpl, VERBUM a, VERBUM b, VERBUM c) -> VERBUM`
- `format_4(VERBUM tmpl, VERBUM a, VERBUM b, VERBUM c, VERBUM d) -> VERBUM`
- `repeat_char(MILES c, REX n) -> VERBUM`
- `table_row(SPECULUM NIHIL parts, SPECULUM REX widths, REX ncols) -> VERBUM`

Module total: **24**

### `cct/fs`

- `read_all(VERBUM path) -> VERBUM`
- `write_all(VERBUM path, VERBUM content) -> NIHIL`
- `append_all(VERBUM path, VERBUM content) -> NIHIL`
- `mkdir(VERBUM path) -> NIHIL`
- `mkdir_all(VERBUM path) -> NIHIL`
- `delete_file(VERBUM path) -> NIHIL`
- `delete_dir(VERBUM path) -> NIHIL`
- `rename(VERBUM from, VERBUM to) -> NIHIL`
- `copy(VERBUM from, VERBUM to) -> NIHIL`
- `move(VERBUM from, VERBUM to) -> NIHIL`
- `is_file(VERBUM path) -> VERUM`
- `is_dir(VERBUM path) -> VERUM`
- `is_symlink(VERBUM path) -> VERUM`
- `is_readable(VERBUM path) -> VERUM`
- `is_writable(VERBUM path) -> VERUM`
- `modified_time(VERBUM path) -> REX`
- `chmod(VERBUM path, REX mode) -> NIHIL`
- `list_dir(VERBUM path) -> SPECULUM NIHIL`
- `read_lines(VERBUM path) -> SPECULUM NIHIL`
- `create_temp_file() -> VERBUM`
- `create_temp_dir() -> VERBUM`
- `truncate(VERBUM path, REX size) -> NIHIL`
- `symlink(VERBUM alvo, VERBUM link) -> NIHIL`
- `same_file(VERBUM a, VERBUM b) -> VERUM`
- `exists(VERBUM path) -> VERUM`
- `size(VERBUM path) -> REX`

Module total: **26**

### `cct/hash`

- `djb2(VERBUM s) -> REX`
- `fnv1a(VERBUM s) -> REX`
- `fnv1a_bytes(SPECULUM NIHIL data, REX len) -> REX`
- `crc32(VERBUM s) -> REX`
- `murmur3(VERBUM s, REX seed) -> REX`
- `combine(REX h1, REX h2) -> REX`

Module total: **6**

### `cct/http`

- `http_alloc(REX bytes) -> SPECULUM NIHIL`
- `http_headers_new() -> REX`
- `http_header_set(SPECULUM HttpHeader out, VERBUM nome, VERBUM valor) -> NIHIL`
- `http_headers_push(REX headers_handle, SPECULUM NIHIL h_ptr) -> NIHIL`
- `http_headers_len(REX headers_handle) -> REX`
- `http_headers_get_ptr(REX headers_handle, REX idx) -> SPECULUM NIHIL`
- `http_headers_find_index(REX headers_handle, VERBUM nome) -> REX`
- `http_headers_has(REX headers_handle, VERBUM nome) -> VERUM`
- `http_headers_get_or(REX headers_handle, VERBUM nome, VERBUM padrao) -> VERBUM`
- `http_headers_set_or_push(REX headers_handle, VERBUM nome, VERBUM valor) -> NIHIL`
- `http_method_get() -> HttpMethod`
- `http_method_name(HttpMethod m) -> VERBUM`
- `http_status_text(REX status) -> VERBUM`
- `http_request_new(VERBUM method, VERBUM path) -> HttpRequest`
- `http_request_with_body(VERBUM method, VERBUM path, VERBUM body) -> HttpRequest`
- `http_response_new(REX status, VERBUM body) -> HttpResponse`
- `http_response_with_headers(REX status, REX headers, VERBUM body) -> HttpResponse`
- `http_request_headers_handle(HttpRequest req) -> REX`
- `http_request_method(HttpRequest req) -> VERBUM`
- `http_request_path(HttpRequest req) -> VERBUM`
- `http_request_body(HttpRequest req) -> VERBUM`
- `http_response_headers_handle(HttpResponse res) -> REX`
- `http_response_status(HttpResponse res) -> REX`
- `http_response_body(HttpResponse res) -> VERBUM`
- `http_response_header(HttpResponse res, VERBUM nome, VERBUM padrao) -> VERBUM`
- `http_request_header(HttpRequest req, VERBUM nome, VERBUM padrao) -> VERBUM`
- `http_crlf() -> VERBUM`
- `http_trim_cr(VERBUM s) -> VERBUM`
- `http_split_head_body(VERBUM raw) -> HttpRequest`
- `http_parse_header_into(SPECULUM HttpHeader out, VERBUM line) -> NIHIL`
- `http_split_first_line(VERBUM s) -> HttpRequest`
- `http_parse_request_line(VERBUM line) -> HttpRequest`
- `http_parse_status_line(VERBUM line) -> HttpResponse`
- `http_parse_request(VERBUM raw) -> HttpRequest`
- `http_parse_response(VERBUM raw) -> HttpResponse`
- `http_stringify_request(HttpRequest req) -> VERBUM`
- `http_stringify_response(HttpResponse res) -> VERBUM`
- `http_parse_url(VERBUM url) -> HttpUrl`
- `http_read_headers(SPECULUM NIHIL sock) -> VERBUM`
- `http_content_length_from_headers(REX headers_handle) -> REX`
- `http_read_remaining(SPECULUM NIHIL sock) -> VERBUM`
- `http_read_request_raw(SPECULUM NIHIL sock) -> VERBUM`
- `http_read_response_raw(SPECULUM NIHIL sock) -> VERBUM`
- `http_request_prepare(HttpRequest req, VERBUM host, REX port, VERBUM path) -> HttpRequest`
- `http_response_prepare(HttpResponse res) -> HttpResponse`
- `http_request(HttpRequest req) -> HttpResponse`
- `http_get(VERBUM url) -> HttpResponse`
- `http_post(VERBUM url, VERBUM body) -> HttpResponse`
- `http_get_json(VERBUM url) -> Json`
- `http_server_listen(VERBUM host, REX port) -> SPECULUM NIHIL`
- `http_server_accept(SPECULUM NIHIL srv_ptr) -> HttpRequest`
- `http_server_reply(SPECULUM NIHIL srv_ptr, HttpResponse resp) -> NIHIL`
- `http_serve_once(VERBUM host, REX port, SPECULUM NIHIL handler) -> NIHIL`

Module total: **53**

### `cct/io`

- `print(VERBUM s) -> NIHIL`
- `println(VERBUM s) -> NIHIL`
- `print_int(REX n) -> NIHIL`
- `print_real(UMBRA x) -> NIHIL`
- `print_bool(VERUM b) -> NIHIL`
- `print_char(MILES c) -> NIHIL`
- `print_hex(REX n) -> NIHIL`
- `eprint(VERBUM s) -> NIHIL`
- `eprintln(VERBUM s) -> NIHIL`
- `eprint_int(REX n) -> NIHIL`
- `eprint_real(UMBRA x) -> NIHIL`
- `eprint_bool(VERUM b) -> NIHIL`
- `flush() -> NIHIL`
- `flush_err() -> NIHIL`
- `read_char() -> MILES`
- `read_line() -> VERBUM`
- `read_all_stdin() -> VERBUM`
- `read_line_prompt(VERBUM prompt) -> VERBUM`
- `is_tty() -> VERUM`

Module total: **19**

### `cct/json`

- `json_null() -> Json`
- `json_bool(VERUM v) -> Json`
- `json_num(UMBRA v) -> Json`
- `json_str(VERBUM v) -> Json`
- `json_arr() -> Json`
- `json_obj() -> Json`
- `json_kind(Json j) -> REX`
- `json_is_null(Json j) -> VERUM`
- `json_is_bool(Json j) -> VERUM`
- `json_is_num(Json j) -> VERUM`
- `json_is_str(Json j) -> VERUM`
- `json_is_arr(Json j) -> VERUM`
- `json_is_obj(Json j) -> VERUM`
- `json_arr_push(Json j, Json valor) -> NIHIL`
- `json_arr_len(Json j) -> REX`
- `json_arr_get(Json j, REX idx) -> Json`
- `json_obj_push(Json j, VERBUM chave, Json valor) -> NIHIL`
- `json_obj_len(Json j) -> REX`
- `json_obj_key_at(Json j, REX idx) -> VERBUM`
- `json_obj_value_at(Json j, REX idx) -> Json`
- `json_is_digit(MILES c) -> VERUM`
- `json_is_ws(MILES c) -> VERUM`
- `json_skip_ws(SPECULUM NIHIL c) -> NIHIL`
- `json_consume_literal(SPECULUM NIHIL c, VERBUM literal) -> NIHIL`
- `json_parse_string(SPECULUM NIHIL c) -> VERBUM`
- `json_parse_number(VERBUM s, SPECULUM NIHIL c) -> Json`
- `json_parse_value(VERBUM s, SPECULUM NIHIL c) -> Json`
- `json_append_escaped(SPECULUM NIHIL b, VERBUM s) -> NIHIL`
- `json_write_compact_to(SPECULUM NIHIL b, Json j) -> NIHIL`
- `json_indent_prefix(REX depth, REX n) -> VERBUM`
- `json_write_pretty_to(SPECULUM NIHIL b, Json j, REX depth, REX indent) -> NIHIL`
- `json_stringify(Json j) -> VERBUM`
- `json_stringify_pretty_indent(Json j, REX n) -> VERBUM`
- `json_stringify_pretty(Json j) -> VERBUM`
- `json_write_file(Json j, VERBUM path) -> NIHIL`
- `json_len(Json j) -> REX`
- `json_get_index(Json j, REX i) -> Json`
- `json_set_index(Json j, REX i, Json v) -> NIHIL`
- `json_push(Json j, Json v) -> NIHIL`
- `json_get_key(Json j, VERBUM chave) -> Json`
- `json_has_key(Json j, VERBUM chave) -> VERUM`
- `json_set_key(Json j, VERBUM chave, Json v) -> NIHIL`
- `json_keys(Json j) -> SPECULUM NIHIL`
- `json_values(Json j) -> SPECULUM NIHIL`
- `json_expect_num(Json j) -> UMBRA`
- `json_expect_str(Json j) -> VERBUM`
- `json_expect_bool(Json j) -> VERUM`
- `json_expect_arr(Json j) -> SPECULUM NIHIL`
- `json_expect_obj(Json j) -> SPECULUM NIHIL`
- `json_parse(VERBUM s) -> Json`
- `json_try_parse(VERBUM s) -> SPECULUM NIHIL`
- `json_parse_file(VERBUM path) -> Json`
- `json_try_parse_file(VERBUM path) -> SPECULUM NIHIL`

Module total: **53**

### `cct/map`

- `map_init GENUS(K, V) -> SPECULUM NIHIL`
- `map_free GENUS(K, V) -> NIHIL`
- `map_insert GENUS(K, V) -> NIHIL`
- `map_remove GENUS(K, V) -> VERUM`
- `map_get GENUS(K, V) -> SPECULUM NIHIL`
- `map_contains GENUS(K, V) -> VERUM`
- `map_len GENUS(K, V) -> REX`
- `map_is_empty GENUS(K, V) -> VERUM`
- `map_capacity GENUS(K, V) -> REX`
- `map_clear GENUS(K, V) -> NIHIL`
- `map_reserve GENUS(K, V) -> NIHIL`
- `map_get_or_default GENUS(K, V) -> V`
- `map_update_or_insert GENUS(K, V) -> VERUM`
- `map_copy GENUS(K, V) -> SPECULUM NIHIL`
- `map_keys GENUS(K, V) -> SPECULUM NIHIL`
- `map_values GENUS(K, V) -> SPECULUM NIHIL`
- `map_merge GENUS(K, V) -> NIHIL`

Module total: **17**

### `cct/math`

- `pi() -> UMBRA`
- `e() -> UMBRA`
- `tau() -> UMBRA`
- `phi() -> UMBRA`
- `sqrt2() -> UMBRA`
- `abs(REX x) -> REX`
- `min(REX a, REX b) -> REX`
- `max(REX a, REX b) -> REX`
- `clamp(REX x, REX lo, REX hi) -> REX`
- `sign(REX x) -> REX`
- `is_even(REX x) -> VERUM`
- `is_odd(REX x) -> VERUM`
- `abs_real(UMBRA x) -> UMBRA`
- `sign_real(UMBRA x) -> REX`
- `min_real(UMBRA a, UMBRA b) -> UMBRA`
- `max_real(UMBRA a, UMBRA b) -> UMBRA`
- `clamp_real(UMBRA x, UMBRA lo, UMBRA hi) -> UMBRA`
- `floor_real(UMBRA x) -> REX`
- `ceil_real(UMBRA x) -> REX`
- `round_real(UMBRA x) -> REX`
- `trunc_real(UMBRA x) -> REX`
- `pow_int(REX base, REX exponent) -> REX`
- `gcd(REX a, REX b) -> REX`
- `lcm(REX a, REX b) -> REX`
- `factorial(REX n) -> REX`
- `is_prime(REX n) -> VERUM`
- `sqrt_real(UMBRA x) -> UMBRA`
- `cbrt_real(UMBRA x) -> UMBRA`
- `pow_real(UMBRA base, UMBRA exponent) -> UMBRA`
- `hypot(UMBRA x, UMBRA y) -> UMBRA`
- `sin(UMBRA radians) -> UMBRA`
- `cos(UMBRA radians) -> UMBRA`
- `tan(UMBRA radians) -> UMBRA`
- `asin(UMBRA x) -> UMBRA`
- `acos(UMBRA x) -> UMBRA`
- `atan(UMBRA x) -> UMBRA`
- `atan2(UMBRA y, UMBRA x) -> UMBRA`
- `deg_to_rad(UMBRA degrees) -> UMBRA`
- `rad_to_deg(UMBRA radians) -> UMBRA`
- `exp(UMBRA x) -> UMBRA`
- `log_natural(UMBRA x) -> UMBRA`
- `log10(UMBRA x) -> UMBRA`
- `log2(UMBRA x) -> UMBRA`

Module total: **43**

### `cct/net`

- `net_parse_addr(VERBUM s) -> SPECULUM NIHIL`
- `tcp_connect(VERBUM host, REX port) -> SPECULUM NIHIL`
- `tcp_listen(VERBUM host, REX port) -> SPECULUM NIHIL`
- `tcp_accept(SPECULUM NIHIL listener) -> SPECULUM NIHIL`
- `udp_bind(VERBUM host, REX port) -> SPECULUM NIHIL`
- `udp_send_to(SPECULUM NIHIL sock, VERBUM host, REX port, VERBUM data) -> NIHIL`
- `udp_recv_from(SPECULUM NIHIL sock) -> SPECULUM NIHIL`
- `net_read_until(SPECULUM NIHIL sock, VERBUM delim) -> VERBUM`
- `net_read_line(SPECULUM NIHIL sock) -> VERBUM`
- `net_write_line(SPECULUM NIHIL sock, VERBUM s) -> NIHIL`
- `net_read_exact(SPECULUM NIHIL sock, REX n) -> VERBUM`
- `net_close(SPECULUM NIHIL sock) -> NIHIL`

Module total: **12**

### `cct/mem`

- `alloc(REX size) -> SPECULUM REX`
- `free(SPECULUM REX ptr) -> NIHIL`
- `realloc(SPECULUM REX ptr, REX new_size) -> SPECULUM REX`
- `copy(SPECULUM REX dest, SPECULUM REX src, REX size) -> NIHIL`
- `set(SPECULUM REX ptr, REX valor, REX size) -> NIHIL`
- `zero(SPECULUM REX ptr, REX size) -> NIHIL`
- `mem_compare(SPECULUM REX a, SPECULUM REX b, REX size) -> REX`

Module total: **7**

### `cct/option`

- `Some GENUS(T) -> SPECULUM NIHIL`
- `None GENUS(T) -> SPECULUM NIHIL`
- `option_is_some GENUS(T) -> VERUM`
- `option_is_none GENUS(T) -> VERUM`
- `option_unwrap GENUS(T) -> T`
- `option_unwrap_or GENUS(T) -> T`
- `option_expect GENUS(T) -> T`
- `option_free GENUS(T) -> NIHIL`

Module total: **8**

### `cct/ordo_samples`

- `resultado_ok(REX v) -> Resultado`
- `resultado_err(VERBUM m) -> Resultado`
- `resultado_e_ok(Resultado r) -> VERUM`
- `resultado_valor(Resultado r) -> REX`
- `opcao_algum(REX v) -> Opcao`
- `opcao_nenhum() -> Opcao`
- `opcao_ou_padrao(Opcao o, REX padrao) -> REX`
- `token_descrever(Token t) -> VERBUM`

Module total: **8**

### `cct/parse`

- `parse_int(VERBUM s) -> REX`
- `parse_real(VERBUM s) -> UMBRA`
- `parse_bool(VERBUM s) -> VERUM`
- `try_int(VERBUM s) -> SPECULUM NIHIL`
- `try_real(VERBUM s) -> SPECULUM NIHIL`
- `try_bool(VERBUM s) -> SPECULUM NIHIL`
- `parse_int_hex(VERBUM s) -> REX`
- `try_int_hex(VERBUM s) -> SPECULUM NIHIL`
- `parse_int_radix(VERBUM s, REX radix) -> REX`
- `try_int_radix(VERBUM s, REX radix) -> SPECULUM NIHIL`
- `is_int(VERBUM s) -> VERUM`
- `is_real(VERBUM s) -> VERUM`
- `parse_lines(VERBUM s) -> SPECULUM NIHIL`
- `parse_csv_line(VERBUM s) -> SPECULUM NIHIL`
- `parse_csv_line_sep(VERBUM s, MILES sep) -> SPECULUM NIHIL`

Module total: **15**

### `cct/path`

- `path_join(VERBUM a, VERBUM b) -> VERBUM`
- `path_basename(VERBUM p) -> VERBUM`
- `path_dirname(VERBUM p) -> VERBUM`
- `path_ext(VERBUM p) -> VERBUM`
- `stem(VERBUM p) -> VERBUM`
- `normalize(VERBUM p) -> VERBUM`
- `is_absolute(VERBUM p) -> VERUM`
- `is_relative(VERBUM p) -> VERUM`
- `resolve(VERBUM p) -> VERBUM`
- `relative_to(VERBUM path, VERBUM base) -> VERBUM`
- `with_ext(VERBUM p, VERBUM extension) -> VERBUM`
- `without_ext(VERBUM p) -> VERBUM`
- `parent(VERBUM p) -> VERBUM`
- `home_dir() -> VERBUM`
- `temp_dir() -> VERBUM`
- `split_path(VERBUM p) -> SPECULUM NIHIL`

Module total: **16**

### `cct/process`

- `run(VERBUM cmd) -> REX`
- `run_capture(VERBUM cmd) -> VERBUM`
- `run_capture_err(VERBUM cmd) -> VERBUM`
- `run_with_input(VERBUM cmd, VERBUM input) -> VERBUM`
- `run_env(VERBUM cmd, SPECULUM NIHIL env_pairs) -> REX`
- `run_timeout(VERBUM cmd, REX timeout_ms) -> REX`

Module total: **6**

### `cct/random`

- `seed(REX s) -> NIHIL`
- `random_int(REX lo, REX hi) -> REX`
- `random_real() -> FLAMMA`
- `random_real_unit() -> FLAMMA`
- `random_bool() -> VERUM`
- `random_real_range(UMBRA lo, UMBRA hi) -> UMBRA`
- `random_verbum(REX comprimento) -> VERBUM`
- `random_verbum_from(REX comprimento, VERBUM alphabet) -> VERBUM`
- `random_choice_int(SPECULUM REX arr, REX n) -> REX`
- `shuffle_int(SPECULUM REX arr, REX n) -> NIHIL`
- `random_bytes(REX n) -> SPECULUM NIHIL`

Module total: **11**

### `cct/result`

- `Ok GENUS(T, E) -> SPECULUM NIHIL`
- `Err GENUS(T, E) -> SPECULUM NIHIL`
- `result_is_ok GENUS(T, E) -> VERUM`
- `result_is_err GENUS(T, E) -> VERUM`
- `result_unwrap GENUS(T, E) -> T`
- `result_unwrap_or GENUS(T, E) -> T`
- `result_unwrap_err GENUS(T, E) -> E`
- `result_expect GENUS(T, E) -> T`
- `result_free GENUS(T, E) -> NIHIL`

Module total: **9**

### `cct/series`

- `series_len GENUS(T) -> REX`
- `series_fill GENUS(T) -> NIHIL`
- `series_copy GENUS(T) -> NIHIL`
- `series_reverse GENUS(T) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX size) -> VERUM`
- `series_sum(SPECULUM REX arr, REX n) -> REX`
- `series_sum_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `series_min(SPECULUM REX arr, REX n) -> REX`
- `series_max(SPECULUM REX arr, REX n) -> REX`
- `series_is_sorted(SPECULUM REX arr, REX n) -> VERUM`
- `series_sort(SPECULUM REX arr, REX n) -> NIHIL`
- `series_count_val(SPECULUM REX arr, REX n, REX val) -> REX`

Module total: **12**

### `cct/socket`

- `socket_tcp() -> SPECULUM NIHIL`
- `socket_udp() -> SPECULUM NIHIL`
- `sock_connect(SPECULUM NIHIL s, VERBUM host, REX port) -> NIHIL`
- `sock_bind(SPECULUM NIHIL s, VERBUM host, REX port) -> NIHIL`
- `sock_listen(SPECULUM NIHIL s, REX backlog) -> NIHIL`
- `sock_accept(SPECULUM NIHIL s) -> SPECULUM NIHIL`
- `sock_send(SPECULUM NIHIL s, VERBUM data) -> REX`
- `sock_recv(SPECULUM NIHIL s, REX max_bytes) -> VERBUM`
- `sock_close(SPECULUM NIHIL s) -> NIHIL`
- `sock_set_timeout_ms(SPECULUM NIHIL s, REX ms) -> NIHIL`
- `sock_peer_addr(SPECULUM NIHIL s) -> VERBUM`
- `sock_local_addr(SPECULUM NIHIL s) -> VERBUM`
- `sock_last_error() -> VERBUM`

Module total: **13**

### `cct/set`

- `set_init GENUS(T) -> SPECULUM NIHIL`
- `set_free GENUS(T) -> NIHIL`
- `set_insert GENUS(T) -> VERUM`
- `set_remove GENUS(T) -> VERUM`
- `set_contains GENUS(T) -> VERUM`
- `set_len GENUS(T) -> REX`
- `set_is_empty GENUS(T) -> VERUM`
- `set_clear GENUS(T) -> NIHIL`
- `set_union GENUS(T) -> SPECULUM NIHIL`
- `set_intersection GENUS(T) -> SPECULUM NIHIL`
- `set_difference GENUS(T) -> SPECULUM NIHIL`
- `set_symmetric_difference GENUS(T) -> SPECULUM NIHIL`
- `set_is_subset GENUS(T) -> VERUM`
- `set_is_superset GENUS(T) -> VERUM`
- `set_equals GENUS(T) -> VERUM`
- `set_copy GENUS(T) -> SPECULUM NIHIL`
- `set_to_fluxus GENUS(T) -> SPECULUM NIHIL`
- `set_reserve GENUS(T) -> NIHIL`
- `set_capacity GENUS(T) -> REX`

Module total: **19**

### `cct/stub_test`

- `test_value() -> REX`

Module total: **1**

### `cct/time`

- `now_ms() -> REX`
- `now_ns() -> REX`
- `sleep_ms(REX ms) -> NIHIL`

Module total: **3**

### `cct/variant`

- `variant_make(REX tag, SPECULUM NIHIL payload) -> Variant`
- `variant_tag(Variant v) -> REX`
- `variant_payload(Variant v) -> SPECULUM NIHIL`

Module total: **3**

### `cct/variant_helpers`

- `variant_is_tag(Variant v, REX tag) -> VERUM`
- `variant_expect_tag(Variant v, REX tag, VERBUM message) -> NIHIL`
- `variant_payload_if(Variant v, REX tag) -> SPECULUM NIHIL`
- `variant_payload_expect(Variant v, REX tag, VERBUM message) -> SPECULUM NIHIL`

Module total: **4**

### `cct/verbum`

- `len(VERBUM s) -> REX`
- `concat(VERBUM a, VERBUM b) -> VERBUM`
- `compare(VERBUM a, VERBUM b) -> REX`
- `substring(VERBUM s, REX inicio, REX fim) -> VERBUM`
- `trim(VERBUM s) -> VERBUM`
- `find(VERBUM s, VERBUM sub) -> REX`
- `char_at(VERBUM s, REX i) -> MILES`
- `from_char(MILES c) -> VERBUM`
- `contains(VERBUM s, VERBUM sub) -> VERUM`
- `starts_with(VERBUM s, VERBUM prefix) -> VERUM`
- `ends_with(VERBUM s, VERBUM suffix) -> VERUM`
- `strip_prefix(VERBUM s, VERBUM prefix) -> VERBUM`
- `strip_suffix(VERBUM s, VERBUM suffix) -> VERBUM`
- `replace(VERBUM s, VERBUM from, VERBUM to) -> VERBUM`
- `replace_all(VERBUM s, VERBUM from, VERBUM to) -> VERBUM`
- `to_upper(VERBUM s) -> VERBUM`
- `to_lower(VERBUM s) -> VERBUM`
- `trim_left(VERBUM s) -> VERBUM`
- `trim_right(VERBUM s) -> VERBUM`
- `trim_char(VERBUM s, MILES c) -> VERBUM`
- `repeat(VERBUM s, REX n) -> VERBUM`
- `pad_left(VERBUM s, REX width, MILES fill) -> VERBUM`
- `pad_right(VERBUM s, REX width, MILES fill) -> VERBUM`
- `center(VERBUM s, REX width, MILES fill) -> VERBUM`
- `last_find(VERBUM s, VERBUM sub) -> REX`
- `find_from(VERBUM s, VERBUM sub, REX offset) -> REX`
- `count_occurrences(VERBUM s, VERBUM sub) -> REX`
- `reverse(VERBUM s) -> VERBUM`
- `is_empty(VERBUM s) -> VERUM`
- `equals_ignore_case(VERBUM a, VERBUM b) -> VERUM`
- `slice(VERBUM s, REX inicio, REX comprimento) -> VERBUM`
- `is_ascii(VERBUM s) -> VERUM`
- `split(VERBUM s, VERBUM sep) -> SPECULUM NIHIL`
- `split_char(VERBUM s, MILES c) -> SPECULUM NIHIL`
- `join(SPECULUM NIHIL parts, VERBUM sep) -> VERBUM`
- `lines(VERBUM s) -> SPECULUM NIHIL`
- `words(VERBUM s) -> SPECULUM NIHIL`

Module total: **37**

### `cct/verbum_builder`

- `builder_init() -> SPECULUM NIHIL`
- `builder_append(SPECULUM NIHIL b, VERBUM s) -> NIHIL`
- `builder_append_char(SPECULUM NIHIL b, MILES c) -> NIHIL`
- `builder_append_int(SPECULUM NIHIL b, REX v) -> NIHIL`
- `builder_append_bool(SPECULUM NIHIL b, VERUM v) -> NIHIL`
- `builder_append_real(SPECULUM NIHIL b, UMBRA v) -> NIHIL`
- `builder_len(SPECULUM NIHIL b) -> REX`
- `builder_to_verbum(SPECULUM NIHIL b) -> VERBUM`
- `builder_clear(SPECULUM NIHIL b) -> NIHIL`
- `builder_free(SPECULUM NIHIL b) -> NIHIL`

Module total: **10**

### `cct/verbum_scan`

- `cursor_init(VERBUM s) -> SPECULUM NIHIL`
- `cursor_pos(SPECULUM NIHIL c) -> REX`
- `cursor_eof(SPECULUM NIHIL c) -> VERUM`
- `cursor_peek(SPECULUM NIHIL c) -> MILES`
- `cursor_next(SPECULUM NIHIL c) -> MILES`
- `cursor_free(SPECULUM NIHIL c) -> NIHIL`

Module total: **6**

### `cct/kernel`

- `kernel_halt() -> NIHIL`
- `kernel_outb(REX port, REX valor) -> NIHIL`
- `kernel_inb(REX port) -> REX`
- `kernel_memcpy(SPECULUM REX dst, SPECULUM REX src, REX n) -> NIHIL`
- `kernel_memset(SPECULUM REX dst, REX valor, REX n) -> NIHIL`

Module total: **5**

**Total geral de funcoes inventariadas (FASE 20F baseline)**: **609**

---

## 33. `cct/crypto` (FASE 32A)

Cryptographic hash, HMAC, and constant-time comparison.

Import:

```cct
ADVOCARE "cct/crypto.cct"
```

Public API:
- `crypto_sha256(VERBUM data) -> VERBUM` — hex-encoded SHA-256 digest
- `crypto_sha512(VERBUM data) -> VERBUM` — hex-encoded SHA-512 digest
- `crypto_hmac_sha256(VERBUM key, VERBUM data) -> VERBUM` — hex-encoded HMAC-SHA-256
- `crypto_hmac_sha512(VERBUM key, VERBUM data) -> VERBUM` — hex-encoded HMAC-SHA-512
- `crypto_secure_compare(VERBUM a, VERBUM b) -> VERUM` — constant-time equality

Host-only. Backed by host TLS library (OpenSSL or equivalent). Rejected in freestanding profile.

## 34. `cct/encoding` (FASE 32B)

Base64 and hex encoding/decoding.

Import:

```cct
ADVOCARE "cct/encoding.cct"
```

Public API:
- `encoding_base64_encode(VERBUM data) -> VERBUM`
- `encoding_base64_decode(VERBUM encoded) -> VERBUM`
- `encoding_hex_encode(VERBUM data) -> VERBUM`
- `encoding_hex_decode(VERBUM hex) -> VERBUM`

Host-only. Pure C implementation; no external library dependency.

## 35. `cct/regex` (FASE 32C)

POSIX extended regular expression matching, capture groups, and split.

Import:

```cct
ADVOCARE "cct/regex.cct"
```

Public API:
- `regex_match(VERBUM pattern, VERBUM text) -> VERUM` — full-string match
- `regex_find(VERBUM pattern, VERBUM text) -> VERBUM` — first match substring or empty
- `regex_capture(VERBUM pattern, VERBUM text) -> FLUXUS GENUS(VERBUM)` — capture groups
- `regex_split(VERBUM pattern, VERBUM text) -> FLUXUS GENUS(VERBUM)` — split on pattern
- `regex_replace(VERBUM pattern, VERBUM text, VERBUM replacement) -> VERBUM`

Host-only. Uses POSIX extended regex (`regcomp`/`regexec`).

## 36. `cct/datetime` (FASE 33A)

UTC and local date/time, parsing, formatting, and arithmetic.

Import:

```cct
ADVOCARE "cct/datetime.cct"
```

Public API:
- `datetime_now_utc() -> SPECULUM NIHIL`
- `datetime_now_local() -> SPECULUM NIHIL`
- `datetime_from_unix(REX epoch_s) -> SPECULUM NIHIL`
- `datetime_to_unix(SPECULUM NIHIL dt) -> REX`
- `datetime_format(SPECULUM NIHIL dt, VERBUM fmt) -> VERBUM` — strftime-like format string
- `datetime_parse(VERBUM s, VERBUM fmt) -> SPECULUM NIHIL`
- `datetime_add_seconds(SPECULUM NIHIL dt, REX secs) -> SPECULUM NIHIL`
- `datetime_diff_seconds(SPECULUM NIHIL a, SPECULUM NIHIL b) -> REX`
- `datetime_free(SPECULUM NIHIL dt)`

`DateTimeResult` fields: `year`, `month`, `day`, `hour`, `minute`, `second`, `weekday`, `unix_epoch`.

Host-only.

## 37. `cct/toml` (FASE 33B)

TOML configuration file parsing and typed value extraction.

Import:

```cct
ADVOCARE "cct/toml.cct"
```

Public API:
- `toml_load(VERBUM path) -> SPECULUM NIHIL`
- `toml_get_string(SPECULUM NIHIL doc, VERBUM key) -> VERBUM`
- `toml_get_int(SPECULUM NIHIL doc, VERBUM key) -> REX`
- `toml_get_float(SPECULUM NIHIL doc, VERBUM key) -> COMES`
- `toml_get_bool(SPECULUM NIHIL doc, VERBUM key) -> VERUM`
- `toml_has(SPECULUM NIHIL doc, VERBUM key) -> VERUM`
- `toml_free(SPECULUM NIHIL doc)`

Key syntax: `"section.key"` for nested access. Host-only.

## 38. `cct/compress` (FASE 33C)

In-memory lossless compression and decompression (zlib/deflate and gzip).

Import:

```cct
ADVOCARE "cct/compress.cct"
```

Public API:
- `compress_deflate(VERBUM data) -> VERBUM`
- `compress_inflate(VERBUM data) -> VERBUM`
- `compress_gzip(VERBUM data) -> VERBUM`
- `compress_gunzip(VERBUM data) -> VERBUM`
- `compress_level(REX level)` — set compression level 1–9; default is 6

Host-only. Backed by zlib.

## 39. `cct/filetype` (FASE 34A)

Magic-byte file type detection.

Import:

```cct
ADVOCARE "cct/filetype.cct"
```

Public API:
- `filetype_detect(VERBUM path) -> VERBUM` — returns MIME type string
- `filetype_detect_bytes(VERBUM bytes) -> VERBUM` — detect from raw bytes
- `filetype_is(VERBUM path, VERBUM mime_prefix) -> VERUM`

Host-only.

## 40. `cct/media` (FASE 34B)

Audio/video metadata extraction (duration, codec, dimensions).

Import:

```cct
ADVOCARE "cct/media.cct"
```

Public API:
- `media_probe(VERBUM path) -> SPECULUM NIHIL`
- `media_duration_ms(SPECULUM NIHIL info) -> REX`
- `media_codec(SPECULUM NIHIL info) -> VERBUM`
- `media_width(SPECULUM NIHIL info) -> REX`
- `media_height(SPECULUM NIHIL info) -> REX`
- `media_free(SPECULUM NIHIL info)`

Host-only.

## 41. `cct/image` (FASE 34C)

Image dimension and format inspection.

Import:

```cct
ADVOCARE "cct/image.cct"
```

Public API:
- `image_probe(VERBUM path) -> SPECULUM NIHIL`
- `image_width(SPECULUM NIHIL info) -> REX`
- `image_height(SPECULUM NIHIL info) -> REX`
- `image_format(SPECULUM NIHIL info) -> VERBUM` — `"png"`, `"jpeg"`, `"webp"`, etc.
- `image_free(SPECULUM NIHIL info)`

Host-only.

## 42. `cct/langdetect` (FASE 34D)

Natural language detection from text samples.

Import:

```cct
ADVOCARE "cct/langdetect.cct"
```

Public API:
- `langdetect_detect(VERBUM text) -> VERBUM` — BCP 47 language tag (e.g. `"pt"`, `"en"`)
- `langdetect_confidence(VERBUM text) -> COMES` — confidence [0.0, 1.0]
- `langdetect_top_n(VERBUM text, REX n) -> FLUXUS GENUS(VERBUM)`

Host-only.

## 43. `cct/verbum_extra` (FASE 34E)

Advanced string operations beyond the core `cct/verbum` API.

Import:

```cct
ADVOCARE "cct/verbum_extra.cct"
```

Public API:
- `verbum_levenshtein(VERBUM a, VERBUM b) -> REX`
- `verbum_soundex(VERBUM s) -> VERBUM`
- `verbum_truncate_words(VERBUM s, REX max_words) -> VERBUM`
- `verbum_wrap(VERBUM s, REX width) -> VERBUM`
- `verbum_count_words(VERBUM s) -> REX`
- `verbum_normalize_whitespace(VERBUM s) -> VERBUM`
- `verbum_is_ascii(VERBUM s) -> VERUM`

Host-only.

## 44. `cct/lexer` (FASE 35A)

Cursor-based lexer utilities for tokenizing structured text formats.

Import:

```cct
ADVOCARE "cct/lexer.cct"
```

Public API:
- `lexer_new(VERBUM src) -> SPECULUM NIHIL`
- `lexer_peek(SPECULUM NIHIL lx) -> VERBUM`
- `lexer_advance(SPECULUM NIHIL lx) -> VERBUM`
- `lexer_expect(SPECULUM NIHIL lx, VERBUM tok)` — assertion; errors on mismatch
- `lexer_at_end(SPECULUM NIHIL lx) -> VERUM`
- `lexer_free(SPECULUM NIHIL lx)`

Host-only.

## 45. `cct/uuid` (FASE 35B)

UUID generation and parsing.

Import:

```cct
ADVOCARE "cct/uuid.cct"
```

Public API:
- `uuid_v4() -> VERBUM` — random UUID v4
- `uuid_v7() -> VERBUM` — time-ordered UUID v7
- `uuid_parse(VERBUM s) -> SPECULUM NIHIL`
- `uuid_is_valid(VERBUM s) -> VERUM`
- `uuid_free(SPECULUM NIHIL u)`

Host-only.

## 46. `cct/slug` (FASE 35C)

URL-safe slug generation.

Import:

```cct
ADVOCARE "cct/slug.cct"
```

Public API:
- `slug_from(VERBUM text) -> VERBUM` — lowercase, hyphenated slug
- `slug_from_locale(VERBUM text, VERBUM locale) -> VERBUM` — locale-aware transliteration
- `slug_truncate(VERBUM slug, REX max_len) -> VERBUM`

Host-only.

## 47. `cct/i18n` (FASE 35D)

Locale-based string translation and pluralization.

Import:

```cct
ADVOCARE "cct/i18n.cct"
```

Public API:
- `i18n_load(VERBUM locale, VERBUM catalog_path)` — load translation catalog
- `i18n_t(VERBUM key) -> VERBUM` — translate key in active locale
- `i18n_plural(VERBUM key, REX count) -> VERBUM`
- `i18n_set_locale(VERBUM locale)`
- `i18n_locale() -> VERBUM`

Host-only.

## 48. `cct/form` (FASE 35E)

URL-encoded form (`application/x-www-form-urlencoded`) encoding and decoding.

Import:

```cct
ADVOCARE "cct/form.cct"
```

Public API:
- `form_parse(VERBUM body) -> SPECULUM NIHIL`
- `form_get(SPECULUM NIHIL form, VERBUM key) -> VERBUM`
- `form_has(SPECULUM NIHIL form, VERBUM key) -> VERUM`
- `form_encode(SPECULUM NIHIL form) -> VERBUM`
- `form_free(SPECULUM NIHIL form)`
- `url_encode(VERBUM s) -> VERBUM`
- `url_decode(VERBUM s) -> VERBUM`

Host-only.

## 49. `cct/log` (FASE 35F)

Structured leveled logging with pluggable backends and context propagation.

Import:

```cct
ADVOCARE "cct/log.cct"
```

Log levels: `LOG_DEBUG` (0), `LOG_INFO` (1), `LOG_WARN` (2), `LOG_ERROR` (3), `LOG_FATAL` (4).

Public API:
- `log_debug(VERBUM msg)` / `log_info(...)` / `log_warn(...)` / `log_error(...)` / `log_fatal(...)`
- `log_with(VERBUM key, VERBUM value)` — attach structured field to next log line
- `log_set_level(REX level)`
- `log_set_backend(VERBUM backend)` — `"stderr"` | `"file"` | `"json"`
- `log_set_output(VERBUM path)` — output path for file backend

Integrates with `cct/context_local` to automatically attach `request_id` and `trace_id` to log lines. Host-only.

## 50. `cct/trace` (FASE 35G)

Distributed tracing — span creation, parent-child propagation, and `.ctrace` file emission.

Import:

```cct
ADVOCARE "cct/trace.cct"
```

Public API:
- `trace_start(VERBUM name) -> REX` — opens root span; returns span_id
- `trace_finish(REX span_id)` — records end timestamp
- `trace_child(REX parent_id, VERBUM name) -> REX` — opens child span
- `trace_attr(REX span_id, VERBUM key, VERBUM value)` — annotate span
- `trace_flush(VERBUM path)` — write buffered spans to `.ctrace` JSON Lines file
- `trace_reset()` — clear span buffer (for tests)

`.ctrace` format: one JSON object per line with `span_id`, `parent_id`, `name`, `start_us`, `end_us`, `attrs`. Compatible with `cct sigilo trace view` and the FASE 39 SVG renderer. Host-only.

## 51. `cct/metrics` (FASE 35H)

In-process counter, gauge, and histogram collection with Prometheus text export.

Import:

```cct
ADVOCARE "cct/metrics.cct"
```

Public API:
- `metrics_counter(VERBUM name) -> REX` — register counter; returns handle
- `metrics_gauge(VERBUM name) -> REX`
- `metrics_histogram(VERBUM name) -> REX`
- `metrics_incr(REX handle)` — increment counter by 1
- `metrics_set(REX handle, COMES value)` — set gauge value
- `metrics_observe(REX handle, COMES value)` — record histogram sample
- `metrics_export_text() -> VERBUM` — Prometheus text exposition format
- `metrics_reset()`

Host-only.

## 52. `cct/signal` (FASE 35I)

POSIX signal handling registration and safe delivery.

Import:

```cct
ADVOCARE "cct/signal.cct"
```

Public API:
- `signal_handle(VERBUM signame, RITUALE handler())` — register handler for named signal
- `signal_ignore(VERBUM signame)`
- `signal_default(VERBUM signame)` — restore default disposition
- `signal_wait(VERBUM signame)` — block until signal received
- `signal_pending(VERBUM signame) -> VERUM`

Supported signal names: `"SIGTERM"`, `"SIGINT"`, `"SIGHUP"`, `"SIGUSR1"`, `"SIGUSR2"`. Host-only.

## 53. `cct/watch` (FASE 35J)

Filesystem path watching with event callbacks.

Import:

```cct
ADVOCARE "cct/watch.cct"
```

Public API:
- `watch_open(VERBUM path) -> SPECULUM NIHIL`
- `watch_on(SPECULUM NIHIL w, VERBUM event, RITUALE callback(VERBUM path))` — register event handler
- `watch_poll(SPECULUM NIHIL w)` — process pending events (non-blocking)
- `watch_close(SPECULUM NIHIL w)`

Event kinds: `"created"`, `"modified"`, `"deleted"`, `"renamed"`. Host-only.

## 54. `cct/audit` (FASE 35K)

Append-only structured audit log for security-sensitive events.

Import:

```cct
ADVOCARE "cct/audit.cct"
```

Public API:
- `audit_open(VERBUM path) -> SPECULUM NIHIL`
- `audit_log(SPECULUM NIHIL al, VERBUM event, VERBUM actor, VERBUM resource)`
- `audit_log_with(SPECULUM NIHIL al, VERBUM event, VERBUM actor, VERBUM resource, SPECULUM NIHIL fields)`
- `audit_close(SPECULUM NIHIL al)`

Entries written as JSON Lines. Each entry includes `ts` (ISO 8601), `event`, `actor`, `resource`, and optional extra fields. Host-only.

## 55. `cct/db_postgres` (FASE 36A)

PostgreSQL client with prepared statements, typed bind/accessors, transaction control, and LISTEN/NOTIFY.

Import:

```cct
ADVOCARE "cct/db_postgres.cct"
```

Public API:
- `postgres_open(VERBUM dsn) -> SPECULUM NIHIL`
- `postgres_prepare(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `postgres_bind_text(SPECULUM NIHIL stmt, REX pos, VERBUM val)`
- `postgres_bind_int(SPECULUM NIHIL stmt, REX pos, REX val)`
- `postgres_bind_real(SPECULUM NIHIL stmt, REX pos, COMES val)`
- `postgres_bind_bool(SPECULUM NIHIL stmt, REX pos, VERUM val)`
- `postgres_bind_null(SPECULUM NIHIL stmt, REX pos)`
- `postgres_step(SPECULUM NIHIL stmt) -> SPECULUM NIHIL`
- `postgres_column_text(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `postgres_column_int(SPECULUM NIHIL rows, REX col) -> REX`
- `postgres_column_real(SPECULUM NIHIL rows, REX col) -> COMES`
- `postgres_column_bool(SPECULUM NIHIL rows, REX col) -> VERUM`
- `postgres_column_json(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `postgres_exec(SPECULUM NIHIL db, VERBUM sql)`
- `postgres_begin(SPECULUM NIHIL db)` / `postgres_commit(...)` / `postgres_rollback(...)`
- `postgres_listen(SPECULUM NIHIL db, VERBUM channel)` / `postgres_notify_wait(...)`
- `postgres_finalize(SPECULUM NIHIL stmt)`
- `postgres_close(SPECULUM NIHIL db)`

API surface mirrors `cct/db_sqlite` where semantically equivalent. Backed by libpq. Host-only.

## 56. `cct/db_postgres_search` (FASE 36B)

PostgreSQL full-text search SQL fragment builders.

Import:

```cct
ADVOCARE "cct/db_postgres_search.cct"
```

Public API:
- `postgres_search_config(VERBUM dictionary) -> SPECULUM NIHIL` — stable FTS configuration handle
- `postgres_search_query(SPECULUM NIHIL cfg, VERBUM query) -> VERBUM` — generates `to_tsquery` expression
- `postgres_search_rank(SPECULUM NIHIL cfg, VERBUM tsvector_col, VERBUM query) -> VERBUM` — `ts_rank` SQL fragment
- `postgres_search_headline(SPECULUM NIHIL cfg, VERBUM text_col, VERBUM query) -> VERBUM` — `ts_headline` SQL fragment
- `postgres_search_document(VERBUM col1, VERBUM col2) -> VERBUM` — concatenated `to_tsvector` source
- `postgres_search_ensure_index(SPECULUM NIHIL db, VERBUM table, VERBUM col)` — GIN index creation (idempotent)

Generates SQL fragments for composition; does not execute queries directly. Host-only.

## 57. `cct/redis` (FASE 36C)

Redis client with string, hash, list, set, pub/sub, and raw RESP command support.

Import:

```cct
ADVOCARE "cct/redis.cct"
```

DSN format: `redis://[:password@]host:port[/db]`

Public API:
- `redis_open(VERBUM dsn) -> SPECULUM NIHIL`
- `redis_get(SPECULUM NIHIL r, VERBUM key) -> VERBUM`
- `redis_set(SPECULUM NIHIL r, VERBUM key, VERBUM value, REX ttl_seconds)`
- `redis_del(SPECULUM NIHIL r, VERBUM key)`
- `redis_exists(SPECULUM NIHIL r, VERBUM key) -> VERUM`
- `redis_incr(SPECULUM NIHIL r, VERBUM key) -> REX`
- `redis_hget(SPECULUM NIHIL r, VERBUM key, VERBUM field) -> VERBUM`
- `redis_hset(SPECULUM NIHIL r, VERBUM key, VERBUM field, VERBUM value)`
- `redis_hdel(SPECULUM NIHIL r, VERBUM key, VERBUM field)`
- `redis_hgetall(SPECULUM NIHIL r, VERBUM key) -> SPECULUM NIHIL`
- `redis_lpush(SPECULUM NIHIL r, VERBUM key, VERBUM value)` / `redis_rpush(...)`
- `redis_lpop(SPECULUM NIHIL r, VERBUM key) -> VERBUM` / `redis_rpop(...)`
- `redis_llen(SPECULUM NIHIL r, VERBUM key) -> REX`
- `redis_sadd(SPECULUM NIHIL r, VERBUM key, VERBUM member)`
- `redis_srem(SPECULUM NIHIL r, VERBUM key, VERBUM member)`
- `redis_smembers(SPECULUM NIHIL r, VERBUM key) -> FLUXUS GENUS(VERBUM)`
- `redis_sismember(SPECULUM NIHIL r, VERBUM key, VERBUM member) -> VERUM`
- `redis_publish(SPECULUM NIHIL r, VERBUM channel, VERBUM message)`
- `redis_subscribe(SPECULUM NIHIL r, VERBUM channel)`
- `redis_receive(SPECULUM NIHIL r) -> VERBUM`
- `redis_raw(SPECULUM NIHIL r, FLUXUS GENUS(VERBUM) args) -> VERBUM` — raw RESP command escape hatch
- `redis_close(SPECULUM NIHIL r)`

RESP protocol implemented in C; no external library required. Host-only.

## 58. `cct/db_postgres_lock` (FASE 36D)

PostgreSQL advisory locks for distributed coordination.

Import:

```cct
ADVOCARE "cct/db_postgres_lock.cct"
```

Public API:
- `postgres_lock_open(SPECULUM NIHIL db, VERBUM name) -> SPECULUM NIHIL` — named lock handle
- `postgres_lock_acquire(SPECULUM NIHIL lock)` — blocking acquisition
- `postgres_lock_try(SPECULUM NIHIL lock) -> VERUM` — non-blocking try; returns `FALSUM` if unavailable
- `postgres_lock_release(SPECULUM NIHIL lock)`
- `postgres_lock_with(SPECULUM NIHIL db, VERBUM name, RITUALE callback())` — acquire, run, release pattern
- `postgres_lock_close(SPECULUM NIHIL lock)`

Lock scopes: session-level (survives transaction) and transaction-level (auto-released on COMMIT/ROLLBACK). Host-only.

## 59. `cct/mail` (FASE 37A)

Email message construction, SMTP delivery, and dev/test backends.

Import:

```cct
ADVOCARE "cct/mail.cct"
```

Public API:
- `mail_new() -> SPECULUM NIHIL`
- `mail_set_from(SPECULUM NIHIL msg, VERBUM addr)`
- `mail_set_to(SPECULUM NIHIL msg, VERBUM addr)`
- `mail_set_subject(SPECULUM NIHIL msg, VERBUM subject)`
- `mail_set_body_text(SPECULUM NIHIL msg, VERBUM text)`
- `mail_set_body_html(SPECULUM NIHIL msg, VERBUM html)`
- `mail_add_attachment(SPECULUM NIHIL msg, VERBUM path, VERBUM name)`
- `mail_smtp_open(SPECULUM NIHIL config) -> SPECULUM NIHIL`
- `mail_smtp_close(SPECULUM NIHIL smtp)`
- `mail_send(SPECULUM NIHIL smtp, SPECULUM NIHIL msg) -> SPECULUM NIHIL`
- `mail_file_backend_open(VERBUM dir) -> SPECULUM NIHIL`
- `mail_memory_backend_open() -> SPECULUM NIHIL`
- `mail_memory_drain(SPECULUM NIHIL backend) -> FLUXUS GENUS(SPECULUM NIHIL)`
- `mail_free(SPECULUM NIHIL msg)`

SMTP authentication modes: PLAIN, LOGIN, STARTTLS, SMTPS. Config fields: `host`, `port`, `username`, `password`, `auth_mode`, `tls_verify`. Backed by OpenSSL for STARTTLS/SMTPS. Host-only.

## 60. `cct/mail_spool` (FASE 37B)

Persistent mail queue with retry and dead-letter semantics.

Import:

```cct
ADVOCARE "cct/mail_spool.cct"
```

State machine: `PENDING → SENT` / `PENDING → FAILED → SENT` / `FAILED → DEAD`

Public API:
- `mail_spool_open(VERBUM spool_dir) -> SPECULUM NIHIL`
- `mail_spool_enqueue(SPECULUM NIHIL spool, SPECULUM NIHIL msg) -> VERBUM` — returns entry ID
- `mail_spool_get(SPECULUM NIHIL spool, VERBUM id) -> SPECULUM NIHIL`
- `mail_spool_mark_sent(SPECULUM NIHIL spool, VERBUM id)`
- `mail_spool_mark_failed(SPECULUM NIHIL spool, VERBUM id, VERBUM reason)`
- `mail_spool_list_pending(SPECULUM NIHIL spool) -> FLUXUS GENUS(SPECULUM NIHIL)`
- `mail_spool_drain_memory(SPECULUM NIHIL spool, SPECULUM NIHIL smtp)` — attempt all PENDING
- `mail_spool_retry_dead(SPECULUM NIHIL spool)` — move DEAD back to PENDING
- `mail_spool_delete(SPECULUM NIHIL spool, VERBUM id)`
- `mail_spool_close(SPECULUM NIHIL spool)`

Persistence format: one JSON file per message in `spool_dir/`. `SpoolEntry` fields: `id`, `state`, `attempt_count`, `last_error`, `enqueued_at`, `last_attempt_at`. Host-only.

## 61. `cct/mail_webhook` (FASE 37C)

Delivery event normalization from mail service provider webhooks.

Import:

```cct
ADVOCARE "cct/mail_webhook.cct"
```

Public API:
- `mail_webhook_parse(VERBUM provider, VERBUM raw_body) -> SPECULUM NIHIL`
- `mail_webhook_event_kind(SPECULUM NIHIL event) -> VERBUM` — `"delivered"` | `"bounce"` | `"complaint"` | `"open"` | `"click"`
- `mail_webhook_recipient(SPECULUM NIHIL event) -> VERBUM`
- `mail_webhook_message_id(SPECULUM NIHIL event) -> VERBUM`
- `mail_mime_scan(VERBUM raw) -> SPECULUM NIHIL` — lightweight MIME scanner
- `mail_headers_parse(VERBUM raw) -> SPECULUM NIHIL` — RFC 5322 header block parser
- `mail_header_get(SPECULUM NIHIL headers, VERBUM name) -> VERBUM`

Supported providers: `"mailgun"`, `"sendgrid"`. Host-only.

## 62. `cct/instrument` (FASE 38A)

Span emission for runtime instrumentation with mode control and `.ctrace` output.

Import:

```cct
ADVOCARE "cct/instrument.cct"
```

Span categories (`InstrumentKind`): `INSTRUMENT_CALL`, `INSTRUMENT_DB`, `INSTRUMENT_CACHE`, `INSTRUMENT_MAIL`, `INSTRUMENT_STORAGE`, `INSTRUMENT_TASK`, `INSTRUMENT_HTTP`, `INSTRUMENT_CUSTOM`.

Public API:
- `instrument_open(VERBUM name, REX kind) -> REX` — returns span_id; 0 if mode is off
- `instrument_close(REX span_id)` — records end time; no-op if span_id is 0
- `instrument_attr(REX span_id, VERBUM key, VERBUM value)` — annotate span; no-op if span_id is 0
- `instrument_set_mode(VERBUM mode)` — `"active"` | `"off"`
- `instrument_flush(VERBUM path)` — write buffered spans to `.ctrace` JSON Lines file
- `instrument_reset()` — clear span buffer

Mode is **off by default**. Activates via `instrument_set_mode("active")` or `CCT_INSTRUMENT=1` env var. Span ID 0 is the null/no-op sentinel. Emits `.ctrace` format compatible with `cct sigilo trace view` and the FASE 39 SVG renderer. Host-only.

## 63. `cct/context_local` (FASE 38B)

Request and task-scoped key-value context store without explicit parameter threading.

Import:

```cct
ADVOCARE "cct/context_local.cct"
```

Well-known keys: `request_id`, `trace_id`, `user_id`, `locale`, `route_id`, `task_id`.

Public API:
- `ctx_set(VERBUM key, VERBUM value)`
- `ctx_get(VERBUM key) -> VERBUM` — empty string if not set
- `ctx_has(VERBUM key) -> VERUM`
- `ctx_clear(VERBUM key)`
- `ctx_reset()` — clear entire context (call at request/task boundary)

Per-thread storage. Multi-threaded applications must call `ctx_reset()` at the start of each request handler. Integrates with `cct/log` and `cct/audit` for automatic `request_id`/`trace_id` annotation. Host-only.

## 64. Expanded Function Inventory (FASE 32–39)

This section extends the function inventory of §32 with the modules delivered in FASE 32–39. All modules listed here are host-only.

### `cct/audit`

- `audit_open(VERBUM path) -> SPECULUM NIHIL`
- `audit_log(SPECULUM NIHIL al, VERBUM event, VERBUM actor, VERBUM resource) -> NIHIL`
- `audit_log_with(SPECULUM NIHIL al, VERBUM event, VERBUM actor, VERBUM resource, SPECULUM NIHIL fields) -> NIHIL`
- `audit_close(SPECULUM NIHIL al) -> NIHIL`

Module total: **4**

### `cct/compress`

- `compress_deflate(VERBUM data) -> VERBUM`
- `compress_inflate(VERBUM data) -> VERBUM`
- `compress_gzip(VERBUM data) -> VERBUM`
- `compress_gunzip(VERBUM data) -> VERBUM`
- `compress_level(REX level) -> NIHIL`

Module total: **5**

### `cct/context_local`

- `ctx_set(VERBUM key, VERBUM value) -> NIHIL`
- `ctx_get(VERBUM key) -> VERBUM`
- `ctx_has(VERBUM key) -> VERUM`
- `ctx_clear(VERBUM key) -> NIHIL`
- `ctx_reset() -> NIHIL`

Module total: **5**

### `cct/crypto`

- `crypto_sha256(VERBUM data) -> VERBUM`
- `crypto_sha512(VERBUM data) -> VERBUM`
- `crypto_hmac_sha256(VERBUM key, VERBUM data) -> VERBUM`
- `crypto_hmac_sha512(VERBUM key, VERBUM data) -> VERBUM`
- `crypto_secure_compare(VERBUM a, VERBUM b) -> VERUM`

Module total: **5**

### `cct/datetime`

- `datetime_now_utc() -> SPECULUM NIHIL`
- `datetime_now_local() -> SPECULUM NIHIL`
- `datetime_from_unix(REX epoch_s) -> SPECULUM NIHIL`
- `datetime_to_unix(SPECULUM NIHIL dt) -> REX`
- `datetime_format(SPECULUM NIHIL dt, VERBUM fmt) -> VERBUM`
- `datetime_parse(VERBUM s, VERBUM fmt) -> SPECULUM NIHIL`
- `datetime_add_seconds(SPECULUM NIHIL dt, REX secs) -> SPECULUM NIHIL`
- `datetime_diff_seconds(SPECULUM NIHIL a, SPECULUM NIHIL b) -> REX`
- `datetime_free(SPECULUM NIHIL dt) -> NIHIL`

Module total: **9**

### `cct/db_postgres`

- `postgres_open(VERBUM dsn) -> SPECULUM NIHIL`
- `postgres_prepare(SPECULUM NIHIL db, VERBUM sql) -> SPECULUM NIHIL`
- `postgres_bind_text(SPECULUM NIHIL stmt, REX pos, VERBUM val) -> NIHIL`
- `postgres_bind_int(SPECULUM NIHIL stmt, REX pos, REX val) -> NIHIL`
- `postgres_bind_real(SPECULUM NIHIL stmt, REX pos, COMES val) -> NIHIL`
- `postgres_bind_bool(SPECULUM NIHIL stmt, REX pos, VERUM val) -> NIHIL`
- `postgres_bind_null(SPECULUM NIHIL stmt, REX pos) -> NIHIL`
- `postgres_step(SPECULUM NIHIL stmt) -> SPECULUM NIHIL`
- `postgres_column_text(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `postgres_column_int(SPECULUM NIHIL rows, REX col) -> REX`
- `postgres_column_real(SPECULUM NIHIL rows, REX col) -> COMES`
- `postgres_column_bool(SPECULUM NIHIL rows, REX col) -> VERUM`
- `postgres_column_json(SPECULUM NIHIL rows, REX col) -> VERBUM`
- `postgres_exec(SPECULUM NIHIL db, VERBUM sql) -> NIHIL`
- `postgres_begin(SPECULUM NIHIL db) -> NIHIL`
- `postgres_commit(SPECULUM NIHIL db) -> NIHIL`
- `postgres_rollback(SPECULUM NIHIL db) -> NIHIL`
- `postgres_listen(SPECULUM NIHIL db, VERBUM channel) -> NIHIL`
- `postgres_notify_wait(SPECULUM NIHIL db) -> VERBUM`
- `postgres_finalize(SPECULUM NIHIL stmt) -> NIHIL`
- `postgres_close(SPECULUM NIHIL db) -> NIHIL`

Module total: **21**

### `cct/db_postgres_lock`

- `postgres_lock_open(SPECULUM NIHIL db, VERBUM name) -> SPECULUM NIHIL`
- `postgres_lock_acquire(SPECULUM NIHIL lock) -> NIHIL`
- `postgres_lock_try(SPECULUM NIHIL lock) -> VERUM`
- `postgres_lock_release(SPECULUM NIHIL lock) -> NIHIL`
- `postgres_lock_with(SPECULUM NIHIL db, VERBUM name, RITUALE callback()) -> NIHIL`
- `postgres_lock_close(SPECULUM NIHIL lock) -> NIHIL`

Module total: **6**

### `cct/db_postgres_search`

- `postgres_search_config(VERBUM dictionary) -> SPECULUM NIHIL`
- `postgres_search_query(SPECULUM NIHIL cfg, VERBUM query) -> VERBUM`
- `postgres_search_rank(SPECULUM NIHIL cfg, VERBUM tsvector_col, VERBUM query) -> VERBUM`
- `postgres_search_headline(SPECULUM NIHIL cfg, VERBUM text_col, VERBUM query) -> VERBUM`
- `postgres_search_document(VERBUM col1, VERBUM col2) -> VERBUM`
- `postgres_search_ensure_index(SPECULUM NIHIL db, VERBUM table, VERBUM col) -> NIHIL`

Module total: **6**

### `cct/encoding`

- `encoding_base64_encode(VERBUM data) -> VERBUM`
- `encoding_base64_decode(VERBUM encoded) -> VERBUM`
- `encoding_hex_encode(VERBUM data) -> VERBUM`
- `encoding_hex_decode(VERBUM hex) -> VERBUM`

Module total: **4**

### `cct/filetype`

- `filetype_detect(VERBUM path) -> VERBUM`
- `filetype_detect_bytes(VERBUM bytes) -> VERBUM`
- `filetype_is(VERBUM path, VERBUM mime_prefix) -> VERUM`

Module total: **3**

### `cct/form`

- `form_parse(VERBUM body) -> SPECULUM NIHIL`
- `form_get(SPECULUM NIHIL form, VERBUM key) -> VERBUM`
- `form_has(SPECULUM NIHIL form, VERBUM key) -> VERUM`
- `form_encode(SPECULUM NIHIL form) -> VERBUM`
- `form_free(SPECULUM NIHIL form) -> NIHIL`
- `url_encode(VERBUM s) -> VERBUM`
- `url_decode(VERBUM s) -> VERBUM`

Module total: **7**

### `cct/i18n`

- `i18n_load(VERBUM locale, VERBUM catalog_path) -> NIHIL`
- `i18n_t(VERBUM key) -> VERBUM`
- `i18n_plural(VERBUM key, REX count) -> VERBUM`
- `i18n_set_locale(VERBUM locale) -> NIHIL`
- `i18n_locale() -> VERBUM`

Module total: **5**

### `cct/image`

- `image_probe(VERBUM path) -> SPECULUM NIHIL`
- `image_width(SPECULUM NIHIL info) -> REX`
- `image_height(SPECULUM NIHIL info) -> REX`
- `image_format(SPECULUM NIHIL info) -> VERBUM`
- `image_free(SPECULUM NIHIL info) -> NIHIL`

Module total: **5**

### `cct/instrument`

- `instrument_open(VERBUM name, REX kind) -> REX`
- `instrument_close(REX span_id) -> NIHIL`
- `instrument_attr(REX span_id, VERBUM key, VERBUM value) -> NIHIL`
- `instrument_set_mode(VERBUM mode) -> NIHIL`
- `instrument_flush(VERBUM path) -> NIHIL`
- `instrument_reset() -> NIHIL`

Module total: **6**

### `cct/langdetect`

- `langdetect_detect(VERBUM text) -> VERBUM`
- `langdetect_confidence(VERBUM text) -> COMES`
- `langdetect_top_n(VERBUM text, REX n) -> FLUXUS GENUS(VERBUM)`

Module total: **3**

### `cct/lexer`

- `lexer_new(VERBUM src) -> SPECULUM NIHIL`
- `lexer_peek(SPECULUM NIHIL lx) -> VERBUM`
- `lexer_advance(SPECULUM NIHIL lx) -> VERBUM`
- `lexer_expect(SPECULUM NIHIL lx, VERBUM tok) -> NIHIL`
- `lexer_at_end(SPECULUM NIHIL lx) -> VERUM`
- `lexer_free(SPECULUM NIHIL lx) -> NIHIL`

Module total: **6**

### `cct/log`

- `log_debug(VERBUM msg) -> NIHIL`
- `log_info(VERBUM msg) -> NIHIL`
- `log_warn(VERBUM msg) -> NIHIL`
- `log_error(VERBUM msg) -> NIHIL`
- `log_fatal(VERBUM msg) -> NIHIL`
- `log_with(VERBUM key, VERBUM value) -> NIHIL`
- `log_set_level(REX level) -> NIHIL`
- `log_set_backend(VERBUM backend) -> NIHIL`
- `log_set_output(VERBUM path) -> NIHIL`

Module total: **9**

### `cct/mail`

- `mail_new() -> SPECULUM NIHIL`
- `mail_set_from(SPECULUM NIHIL msg, VERBUM addr) -> NIHIL`
- `mail_set_to(SPECULUM NIHIL msg, VERBUM addr) -> NIHIL`
- `mail_set_subject(SPECULUM NIHIL msg, VERBUM subject) -> NIHIL`
- `mail_set_body_text(SPECULUM NIHIL msg, VERBUM text) -> NIHIL`
- `mail_set_body_html(SPECULUM NIHIL msg, VERBUM html) -> NIHIL`
- `mail_add_attachment(SPECULUM NIHIL msg, VERBUM path, VERBUM name) -> NIHIL`
- `mail_smtp_open(SPECULUM NIHIL config) -> SPECULUM NIHIL`
- `mail_smtp_close(SPECULUM NIHIL smtp) -> NIHIL`
- `mail_send(SPECULUM NIHIL smtp, SPECULUM NIHIL msg) -> SPECULUM NIHIL`
- `mail_file_backend_open(VERBUM dir) -> SPECULUM NIHIL`
- `mail_memory_backend_open() -> SPECULUM NIHIL`
- `mail_memory_drain(SPECULUM NIHIL backend) -> FLUXUS GENUS(SPECULUM NIHIL)`
- `mail_free(SPECULUM NIHIL msg) -> NIHIL`

Module total: **14**

### `cct/mail_spool`

- `mail_spool_open(VERBUM spool_dir) -> SPECULUM NIHIL`
- `mail_spool_enqueue(SPECULUM NIHIL spool, SPECULUM NIHIL msg) -> VERBUM`
- `mail_spool_get(SPECULUM NIHIL spool, VERBUM id) -> SPECULUM NIHIL`
- `mail_spool_mark_sent(SPECULUM NIHIL spool, VERBUM id) -> NIHIL`
- `mail_spool_mark_failed(SPECULUM NIHIL spool, VERBUM id, VERBUM reason) -> NIHIL`
- `mail_spool_list_pending(SPECULUM NIHIL spool) -> FLUXUS GENUS(SPECULUM NIHIL)`
- `mail_spool_drain_memory(SPECULUM NIHIL spool, SPECULUM NIHIL smtp) -> NIHIL`
- `mail_spool_retry_dead(SPECULUM NIHIL spool) -> NIHIL`
- `mail_spool_delete(SPECULUM NIHIL spool, VERBUM id) -> NIHIL`
- `mail_spool_close(SPECULUM NIHIL spool) -> NIHIL`

Module total: **10**

### `cct/mail_webhook`

- `mail_webhook_parse(VERBUM provider, VERBUM raw_body) -> SPECULUM NIHIL`
- `mail_webhook_event_kind(SPECULUM NIHIL event) -> VERBUM`
- `mail_webhook_recipient(SPECULUM NIHIL event) -> VERBUM`
- `mail_webhook_message_id(SPECULUM NIHIL event) -> VERBUM`
- `mail_mime_scan(VERBUM raw) -> SPECULUM NIHIL`
- `mail_headers_parse(VERBUM raw) -> SPECULUM NIHIL`
- `mail_header_get(SPECULUM NIHIL headers, VERBUM name) -> VERBUM`

Module total: **7**

### `cct/media`

- `media_probe(VERBUM path) -> SPECULUM NIHIL`
- `media_duration_ms(SPECULUM NIHIL info) -> REX`
- `media_codec(SPECULUM NIHIL info) -> VERBUM`
- `media_width(SPECULUM NIHIL info) -> REX`
- `media_height(SPECULUM NIHIL info) -> REX`
- `media_free(SPECULUM NIHIL info) -> NIHIL`

Module total: **6**

### `cct/metrics`

- `metrics_counter(VERBUM name) -> REX`
- `metrics_gauge(VERBUM name) -> REX`
- `metrics_histogram(VERBUM name) -> REX`
- `metrics_incr(REX handle) -> NIHIL`
- `metrics_set(REX handle, COMES value) -> NIHIL`
- `metrics_observe(REX handle, COMES value) -> NIHIL`
- `metrics_export_text() -> VERBUM`
- `metrics_reset() -> NIHIL`

Module total: **8**

### `cct/redis`

- `redis_open(VERBUM dsn) -> SPECULUM NIHIL`
- `redis_get(SPECULUM NIHIL r, VERBUM key) -> VERBUM`
- `redis_set(SPECULUM NIHIL r, VERBUM key, VERBUM value, REX ttl_seconds) -> NIHIL`
- `redis_del(SPECULUM NIHIL r, VERBUM key) -> NIHIL`
- `redis_exists(SPECULUM NIHIL r, VERBUM key) -> VERUM`
- `redis_incr(SPECULUM NIHIL r, VERBUM key) -> REX`
- `redis_hget(SPECULUM NIHIL r, VERBUM key, VERBUM field) -> VERBUM`
- `redis_hset(SPECULUM NIHIL r, VERBUM key, VERBUM field, VERBUM value) -> NIHIL`
- `redis_hdel(SPECULUM NIHIL r, VERBUM key, VERBUM field) -> NIHIL`
- `redis_hgetall(SPECULUM NIHIL r, VERBUM key) -> SPECULUM NIHIL`
- `redis_lpush(SPECULUM NIHIL r, VERBUM key, VERBUM value) -> NIHIL`
- `redis_rpush(SPECULUM NIHIL r, VERBUM key, VERBUM value) -> NIHIL`
- `redis_lpop(SPECULUM NIHIL r, VERBUM key) -> VERBUM`
- `redis_rpop(SPECULUM NIHIL r, VERBUM key) -> VERBUM`
- `redis_llen(SPECULUM NIHIL r, VERBUM key) -> REX`
- `redis_sadd(SPECULUM NIHIL r, VERBUM key, VERBUM member) -> NIHIL`
- `redis_srem(SPECULUM NIHIL r, VERBUM key, VERBUM member) -> NIHIL`
- `redis_smembers(SPECULUM NIHIL r, VERBUM key) -> FLUXUS GENUS(VERBUM)`
- `redis_sismember(SPECULUM NIHIL r, VERBUM key, VERBUM member) -> VERUM`
- `redis_publish(SPECULUM NIHIL r, VERBUM channel, VERBUM message) -> NIHIL`
- `redis_subscribe(SPECULUM NIHIL r, VERBUM channel) -> NIHIL`
- `redis_receive(SPECULUM NIHIL r) -> VERBUM`
- `redis_raw(SPECULUM NIHIL r, FLUXUS GENUS(VERBUM) args) -> VERBUM`
- `redis_close(SPECULUM NIHIL r) -> NIHIL`

Module total: **24**

### `cct/regex`

- `regex_match(VERBUM pattern, VERBUM text) -> VERUM`
- `regex_find(VERBUM pattern, VERBUM text) -> VERBUM`
- `regex_capture(VERBUM pattern, VERBUM text) -> FLUXUS GENUS(VERBUM)`
- `regex_split(VERBUM pattern, VERBUM text) -> FLUXUS GENUS(VERBUM)`
- `regex_replace(VERBUM pattern, VERBUM text, VERBUM replacement) -> VERBUM`

Module total: **5**

### `cct/signal`

- `signal_handle(VERBUM signame, RITUALE handler()) -> NIHIL`
- `signal_ignore(VERBUM signame) -> NIHIL`
- `signal_default(VERBUM signame) -> NIHIL`
- `signal_wait(VERBUM signame) -> NIHIL`
- `signal_pending(VERBUM signame) -> VERUM`

Module total: **5**

### `cct/slug`

- `slug_from(VERBUM text) -> VERBUM`
- `slug_from_locale(VERBUM text, VERBUM locale) -> VERBUM`
- `slug_truncate(VERBUM slug, REX max_len) -> VERBUM`

Module total: **3**

### `cct/toml`

- `toml_load(VERBUM path) -> SPECULUM NIHIL`
- `toml_get_string(SPECULUM NIHIL doc, VERBUM key) -> VERBUM`
- `toml_get_int(SPECULUM NIHIL doc, VERBUM key) -> REX`
- `toml_get_float(SPECULUM NIHIL doc, VERBUM key) -> COMES`
- `toml_get_bool(SPECULUM NIHIL doc, VERBUM key) -> VERUM`
- `toml_has(SPECULUM NIHIL doc, VERBUM key) -> VERUM`
- `toml_free(SPECULUM NIHIL doc) -> NIHIL`

Module total: **7**

### `cct/trace`

- `trace_start(VERBUM name) -> REX`
- `trace_finish(REX span_id) -> NIHIL`
- `trace_child(REX parent_id, VERBUM name) -> REX`
- `trace_attr(REX span_id, VERBUM key, VERBUM value) -> NIHIL`
- `trace_flush(VERBUM path) -> NIHIL`
- `trace_reset() -> NIHIL`

Module total: **6**

### `cct/uuid`

- `uuid_v4() -> VERBUM`
- `uuid_v7() -> VERBUM`
- `uuid_parse(VERBUM s) -> SPECULUM NIHIL`
- `uuid_is_valid(VERBUM s) -> VERUM`
- `uuid_free(SPECULUM NIHIL u) -> NIHIL`

Module total: **5**

### `cct/verbum_extra`

- `verbum_levenshtein(VERBUM a, VERBUM b) -> REX`
- `verbum_soundex(VERBUM s) -> VERBUM`
- `verbum_truncate_words(VERBUM s, REX max_words) -> VERBUM`
- `verbum_wrap(VERBUM s, REX width) -> VERBUM`
- `verbum_count_words(VERBUM s) -> REX`
- `verbum_normalize_whitespace(VERBUM s) -> VERBUM`
- `verbum_is_ascii(VERBUM s) -> VERUM`

Module total: **7**

### `cct/watch`

- `watch_open(VERBUM path) -> SPECULUM NIHIL`
- `watch_on(SPECULUM NIHIL w, VERBUM event, RITUALE callback(VERBUM path)) -> NIHIL`
- `watch_poll(SPECULUM NIHIL w) -> NIHIL`
- `watch_close(SPECULUM NIHIL w) -> NIHIL`

Module total: **4**

**New functions in §64 (FASE 32–39)**: **220**

**Total geral de funcoes inventariadas (historical FASE 39 baseline)**: **829**

---

## 65. `cct/media_store` (FASE 40A)

Local media store with lifecycle zones, UUID/hash naming, SHA-256 checksum, and atomic promotion.

Import:

```cct
ADVOCARE "cct/media_store.cct"
```

Zones: `MEDIA_ZONE_TMP`, `MEDIA_ZONE_QUARANTINE`, `MEDIA_ZONE_PROCESSED`, `MEDIA_ZONE_PUBLIC`, `MEDIA_ZONE_PRIVATE`.

Public API:
- `media_store_default_options(VERBUM root_dir) -> MediaStoreOptions`
- `media_store_open(MediaStoreOptions options) -> Result GENUS(MediaStore, VERBUM)`
- `media_store_ensure_layout(MediaStore store) -> Result GENUS(NIHIL, VERBUM)`
- `media_store_put_file(MediaStore store, MediaZone zone, VERBUM source_path, VERBUM original_filename) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_promote(MediaStore store, MediaArtifact artifact, MediaZone destination) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_copy(MediaStore store, MediaArtifact artifact, MediaZone destination) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_delete(MediaStore store, MediaArtifact artifact) -> Result GENUS(NIHIL, VERBUM)`
- `media_store_exists(MediaStore store, MediaArtifact artifact) -> VERUM`
- `media_store_absolute_path(MediaStore store, MediaArtifact artifact) -> VERBUM`
- `media_store_rechecksum(MediaStore store, MediaArtifact artifact) -> Result GENUS(VERBUM, VERBUM)`
- `media_store_zone_name(MediaZone zone) -> VERBUM`

`MediaArtifact` fields: `artifact_id`, `zone`, `relative_path`, `filename`, `content_type`, `size_bytes`, `checksum`, `created_at_iso`, `updated_at_iso`. Host-only. Bridge: `src/runtime/runtime_media_store.c`.

## 66. `cct/archive_zip` (FASE 40B)

ZIP archive creation, text/file entry writing, listing, reading, and safe extraction.

Import:

```cct
ADVOCARE "cct/archive_zip.cct"
```

Public API:
- `zip_create(VERBUM archive_path) -> Result GENUS(ZipArchive, VERBUM)`
- `zip_open(VERBUM archive_path) -> Result GENUS(ZipArchive, VERBUM)`
- `zip_close(SPECULUM ZipArchive archive)`
- `zip_add_file(ZipArchive archive, VERBUM source_path, VERBUM entry_name) -> Result GENUS(NIHIL, VERBUM)` — rejects `..` and absolute paths
- `zip_add_text(ZipArchive archive, VERBUM entry_name, VERBUM content) -> Result GENUS(NIHIL, VERBUM)`
- `zip_list(ZipArchive archive) -> FLUXUS GENUS(ZipEntry)`
- `zip_entry_count(ZipArchive archive) -> REX`
- `zip_read_text(ZipArchive archive, VERBUM entry_name) -> Result GENUS(VERBUM, VERBUM)`
- `zip_extract_all(ZipArchive archive, ZipExtractOptions options) -> Result GENUS(NIHIL, VERBUM)`
- `zip_extract_entry(ZipArchive archive, VERBUM entry_name, ZipExtractOptions options) -> Result GENUS(NIHIL, VERBUM)`

Path traversal is a hard `Err`. Backed by libzip or miniz. Host-only. Bridge: `src/runtime/runtime_archive_zip.c`.

## 67. `cct/object_storage` (FASE 40C)

Optional S3-compatible object storage bridge. Works with AWS S3, MinIO, Cloudflare R2, or any S3-compatible backend.

Import:

```cct
ADVOCARE "cct/object_storage.cct"
```

Public API:
- `object_storage_default_options(VERBUM endpoint, VERBUM bucket, VERBUM access_key, VERBUM secret_key) -> ObjectStorageOptions`
- `object_storage_open(ObjectStorageOptions options) -> Result GENUS(ObjectStorageClient, VERBUM)`
- `object_storage_close(SPECULUM ObjectStorageClient client)`
- `object_storage_put_file(ObjectStorageClient client, VERBUM key, VERBUM source_path, VERBUM content_type) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_put_text(ObjectStorageClient client, VERBUM key, VERBUM content, VERBUM content_type) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_get_to_file(ObjectStorageClient client, VERBUM key, VERBUM dest_path) -> Result GENUS(NIHIL, VERBUM)`
- `object_storage_exists(ObjectStorageClient client, VERBUM key) -> Result GENUS(VERUM, VERBUM)` — distinguishes 404 from error
- `object_storage_head(ObjectStorageClient client, VERBUM key) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_delete(ObjectStorageClient client, VERBUM key) -> Result GENUS(NIHIL, VERBUM)`
- `object_storage_signed_url(ObjectStorageClient client, VERBUM key, REX ttl_seconds) -> Result GENUS(SignedUrl, VERBUM)`

Tests skip (exit 0) when `APP_OBJ_STORAGE_ENDPOINT` is absent. Optional for v1. Host-only. Bridge: `src/runtime/runtime_object_storage.c`.

## 68. Expanded Function Inventory (FASE 40)

### `cct/media_store`

- `media_store_default_options(VERBUM root_dir) -> MediaStoreOptions`
- `media_store_open(MediaStoreOptions options) -> Result GENUS(MediaStore, VERBUM)`
- `media_store_ensure_layout(MediaStore store) -> Result GENUS(NIHIL, VERBUM)`
- `media_store_put_file(MediaStore store, MediaZone zone, VERBUM source_path, VERBUM original_filename) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_promote(MediaStore store, MediaArtifact artifact, MediaZone destination) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_copy(MediaStore store, MediaArtifact artifact, MediaZone destination) -> Result GENUS(MediaArtifact, VERBUM)`
- `media_store_delete(MediaStore store, MediaArtifact artifact) -> Result GENUS(NIHIL, VERBUM)`
- `media_store_exists(MediaStore store, MediaArtifact artifact) -> VERUM`
- `media_store_absolute_path(MediaStore store, MediaArtifact artifact) -> VERBUM`
- `media_store_rechecksum(MediaStore store, MediaArtifact artifact) -> Result GENUS(VERBUM, VERBUM)`
- `media_store_zone_name(MediaZone zone) -> VERBUM`

Module total: **11**

### `cct/archive_zip`

- `zip_create(VERBUM archive_path) -> Result GENUS(ZipArchive, VERBUM)`
- `zip_open(VERBUM archive_path) -> Result GENUS(ZipArchive, VERBUM)`
- `zip_close(SPECULUM ZipArchive archive) -> NIHIL`
- `zip_add_file(ZipArchive archive, VERBUM source_path, VERBUM entry_name) -> Result GENUS(NIHIL, VERBUM)`
- `zip_add_text(ZipArchive archive, VERBUM entry_name, VERBUM content) -> Result GENUS(NIHIL, VERBUM)`
- `zip_list(ZipArchive archive) -> FLUXUS GENUS(ZipEntry)`
- `zip_entry_count(ZipArchive archive) -> REX`
- `zip_read_text(ZipArchive archive, VERBUM entry_name) -> Result GENUS(VERBUM, VERBUM)`
- `zip_extract_all(ZipArchive archive, ZipExtractOptions options) -> Result GENUS(NIHIL, VERBUM)`
- `zip_extract_entry(ZipArchive archive, VERBUM entry_name, ZipExtractOptions options) -> Result GENUS(NIHIL, VERBUM)`

Module total: **10**

### `cct/object_storage`

- `object_storage_default_options(VERBUM endpoint, VERBUM bucket, VERBUM access_key, VERBUM secret_key) -> ObjectStorageOptions`
- `object_storage_open(ObjectStorageOptions options) -> Result GENUS(ObjectStorageClient, VERBUM)`
- `object_storage_close(SPECULUM ObjectStorageClient client) -> NIHIL`
- `object_storage_put_file(ObjectStorageClient client, VERBUM key, VERBUM source_path, VERBUM content_type) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_put_text(ObjectStorageClient client, VERBUM key, VERBUM content, VERBUM content_type) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_get_to_file(ObjectStorageClient client, VERBUM key, VERBUM dest_path) -> Result GENUS(NIHIL, VERBUM)`
- `object_storage_exists(ObjectStorageClient client, VERBUM key) -> Result GENUS(VERUM, VERBUM)`
- `object_storage_head(ObjectStorageClient client, VERBUM key) -> Result GENUS(ObjectRef, VERBUM)`
- `object_storage_delete(ObjectStorageClient client, VERBUM key) -> Result GENUS(NIHIL, VERBUM)`
- `object_storage_signed_url(ObjectStorageClient client, VERBUM key, REX ttl_seconds) -> Result GENUS(SignedUrl, VERBUM)`

Module total: **10**

**New functions in §68 (FASE 40)**: **31**

**Total geral de funcoes inventariadas (FASE 40)**: **860**
<!-- END AUTO API INVENTORY 20F -->
