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

## 11. Current Status (Post-19D4)

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

Current phase closure references:
- `docs/release/FASE_19_RELEASE_NOTES.md`
- `docs/bootstrap/FASE_19_HANDOFF.md`

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

## 32. Complete Function Inventory (FASE 19D.4)

- This section consolidates the complete Bibliotheca Canonica surface delivered through FASE 19D.4.
- The goal is full traceability: no canonical module function is left undocumented.
- For detailed domain semantics, use the descriptive sections earlier in this document and `docs/spec.md`.

<!-- BEGIN AUTO API INVENTORY 19D4 -->
<!-- AUTO-GENERATED from lib/cct/*.cct and lib/cct/kernel/*.cct -->
**Coverage**: complete inventory of canonical functions in `lib/cct` (FASE 19D.4).

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

**Total geral de funcoes inventariadas**: **422**
<!-- END AUTO API INVENTORY 19D4 -->
