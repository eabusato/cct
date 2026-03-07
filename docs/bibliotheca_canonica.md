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
- language-surface expansion (`19A` through `19D`) integrating `QUANDO`, `MOLDE`, payload `ORDO`, and `ITERUM` over map/set.

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
- `series_len GENUS(T)(SPECULUM T arr, REX tamanho) -> REX`
- `series_fill GENUS(T)(SPECULUM T arr, T valor, REX tamanho) -> NIHIL`
- `series_copy GENUS(T)(SPECULUM T dest, SPECULUM T src, REX tamanho) -> NIHIL`
- `series_reverse GENUS(T)(SPECULUM T arr, REX tamanho) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX tamanho) -> VERUM`

Policy:
- explicit-size (`tamanho`) contract is caller-driven in this subset
- generic mutation helpers are supported (`series_fill`, `series_copy`, `series_reverse`)
- `series_contains` is intentionally integer-specialized in this first delivery

## 15. `cct/alg` (FASE 11C)

`cct/alg` is the baseline deterministic algorithm module for integer-array workflows.

Import:

```cct
ADVOCARE "cct/alg.cct"
```

Public API:
- `alg_linear_search(SPECULUM REX arr, REX valor, REX tamanho) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX tamanho) -> VERUM`

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
- `alg_binary_search(SPECULUM REX arr, REX tamanho, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX tamanho) -> NIHIL`

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

## 31. Modulo de Referencia: `ordo_samples` (FASE 19C/19D)

`lib/cct/ordo_samples.cct` nao e um modulo de runtime obrigatorio de producao.
Ele existe como referencia idiomatica para ORDO com payload e para o uso conjunto
de `QUANDO` e `MOLDE`.

Padroes canonicos de referencia:
- `Resultado` (retorno com erro): `Ok(REX valor)` / `Err(VERBUM msg)`
- `Opcao` (valor presente/ausente): `Algum(REX valor)` / `Nenhum`

Exemplo de padrao `Resultado`:

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO
```

Exemplo de padrao `Opcao`:

```cct
ORDO Opcao
  Algum(REX valor),
  Nenhum
FIN ORDO
```

Diretrizes:
- use essas definicoes como baseline para tipos de dominio.
- para APIs ponteiro-backed, `cct/option` e `cct/result` continuam disponiveis.
- a expansao de `ITERUM` na FASE 19D complementa esses padroes com iteracao em
  ordem de insercao para `map` e `set`.
