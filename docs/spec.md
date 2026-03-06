# CCT Language Specification

## Document Purpose

This document is the practical language manual for CCT.

It is written to help you:
- understand the current language surface
- compile and run programs with the current toolchain
- know which constructs are stable, restricted, or reserved

## Status

Specification baseline: **FASE 16D.4** (FASE 13/13M closures, FASE 14 hardening contracts, full FASE 15 closure set, and FASE 16 freestanding bridge closure).

The language is fully usable in its current subset, with explicit boundaries documented below.

## 1. Compiler Command Reference

### 1.1 Primary Commands

- `./cct --help`
Behavior: print command usage and available options.

- `./cct --version`
Behavior: print compiler version and build status.

- `./cct <file.cct>`
Behavior: parse, run semantic checks, generate sigils, generate `.cgen.c`, invoke host C compiler, produce executable.

- `./cct --profile host|freestanding <file.cct>`
Behavior: select explicit compilation profile (`host` by default, `freestanding` for no-libc target flow).

- `./cct --profile freestanding --emit-asm [--entry <rituale>] <file.cct>`
Behavior: generate freestanding `.cgen.s` using Intel GAS syntax for x86 32-bit flow.

- `./cct --tokens <file.cct>`
Behavior: print token stream.

- `./cct --ast <file.cct>`
Behavior: print AST for the entry file only.

- `./cct --ast-composite <entry.cct>`
Behavior: print composed AST for the full module closure.

- `./cct --check <file.cct>`
Behavior: syntax + semantic validation only.

- `./cct --sigilo-only <file.cct>`
Behavior: emit sigilo artifacts without executable generation.

- `./cct sigilo inspect <artifact.sigil>`
Behavior: inspect parsed sigilo metadata in text or structured formats.

- `./cct sigilo validate <artifact.sigil> [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract]`
Behavior: run canonical strict/tolerant validation engine and emit normalized violation report.

- `./cct sigilo diff <left.sigil> <right.sigil>`
Behavior: structural/semantic diff with severity classification.

- `./cct sigilo check <left.sigil> <right.sigil> [--strict]`
Behavior: same as diff with CI-friendly exit code policy (`2` for strict blocking severities).

- `./cct sigilo baseline check <artifact.sigil> [--baseline PATH] [--strict]`
Behavior: compare current artifact against persisted project baseline without mutating files.

- `./cct sigilo baseline update <artifact.sigil> [--baseline PATH] [--force]`
Behavior: explicitly write/update baseline artifacts and metadata (never implicit).

- `./cct fmt <file1.cct> [file2.cct ...]`
Behavior: format one or more CCT files in place.

- `./cct fmt --check <file1.cct> [file2.cct ...]`
Behavior: formatting check mode only; returns exit code `2` when any file is not formatted.

- `./cct fmt --diff <file1.cct> [file2.cct ...]`
Behavior: print formatter diff without rewriting files.

- `./cct lint <file.cct>`
Behavior: run canonical lint rule set and emit warnings.

- `./cct lint --strict <file.cct>`
Behavior: return exit code `2` when warnings are present.

- `./cct lint --fix <file.cct>`
Behavior: apply only deterministic safe fixes for selected lint rules.

- `./cct build [--project DIR] [--entry FILE.cct] [--release] [--out PATH]`
Behavior: build a project using canonical discovery (`cct.toml` or `src/main.cct`) and produce dist artifact.

- `./cct run [--project DIR] [--entry FILE.cct] [--release] [-- --args]`
Behavior: build project then execute binary, forwarding exit code.

- `./cct test [PATTERN] [--project DIR] [--strict-lint] [--fmt-check]`
Behavior: discover and execute `*.test.cct` under `tests/`.

- `./cct bench [PATTERN] [--project DIR] [--iterations N] [--release]`
Behavior: discover and execute `*.bench.cct` under `bench/`, reporting average and total runtime.

Exit code policy (FASE 14A.2):
- `0`: success
- `1`: invalid argument
- `2`: contract violation gate (strict checks, strict lint/docs, strict sigilo gates)
- `3`: missing required argument
- `4`: unknown command/option

- `./cct clean [--project DIR] [--all]`
Behavior: clean `.cct` project artifacts; `--all` also removes generated project binaries in `dist/`.

- `./cct doc [--project DIR] [--entry FILE] [--output-dir DIR] [--format markdown|html|both]`
Behavior: generate deterministic API docs for module closure (`docs/api` by default).

### 1.2 Sigilo Options

- `--sigilo-style network|seal|scriptum`
- `--sigilo-mode essencial|completo`
- aliases: `essential|complete`
- `--sigilo-out <basepath>`
- `--sigilo-no-meta`
- `--sigilo-no-svg`

### 1.3 Input Constraints

- input must be a `.cct` file
- one positional input file per invocation

### 1.4 Freestanding Profile (FASE 16)

The `freestanding` profile enables the CCT-to-LBOS bridge flow and keeps target code independent from host libc/runtime assumptions.

Canonical flags:
- `--profile freestanding`
- `--emit-asm`
- `--entry <rituale>` (freestanding only)

Profile contract:
- `host` remains default and preserves legacy behavior from phases 0-15.
- `freestanding` enforces semantic/codegen restrictions required by ABI V0.
- `--emit-asm` operates in freestanding mode and emits `.cgen.s`.
- `--entry` is valid only in freestanding and chooses exported `cct_fn_<mod>_<entry>`.

Freestanding module policy:
- `cct/kernel` is supported only in freestanding.
- `cct/io`, `cct/fs`, and dynamic runtime-heavy modules remain blocked in freestanding.
- `cct/kernel` must be rejected in host profile.

Supported freestanding surface (summary):
- scalar arithmetic and control flow subset used by the bridge;
- bitwise operators and static data forms validated in FASE 16;
- kernel service lowering via `cct/kernel` wrappers.

Restricted/prohibited freestanding surface (summary):
- host I/O/filesystem modules (`cct/io`, `cct/fs`);
- heap/dynamic structures outside validated subset;
- dynamic `GENUS/PACTUM` paths;
- unsupported soft-float target paths (`COMES`/`MILES` keep warning-only diagnostics where applicable).

Bridge command reference:
- `make lbos-bridge`
Behavior: runs the canonical bridge pipeline and publishes `build/lbos-bridge/cct_kernel.o` for LBOS-side consumption.

## 2. Source File Structure

A valid CCT source file uses `INCIPIT ... EXPLICIT grimoire`.

Example:

```cct
INCIPIT grimoire "hello"

RITUALE main() REDDE REX
  OBSECRO scribe("Ave Mundi!\n")
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

## 3. Top-Level Declarations

### 3.1 `ADVOCARE` (module import)

Syntax:

```cct
ADVOCARE "relative/path/to/module.cct"
```

Rules:
- path is resolved relative to the importer file
- import graph is deduplicated by canonical path
- import cycles are rejected
- visibility and direct-import rules apply (see Module section)

### 3.2 `ARCANUM` (internal visibility)

Syntax:

```cct
ARCANUM RITUALE ...
ARCANUM SIGILLUM ...
ARCANUM ORDO ...
```

Rules:
- only valid on top-level `RITUALE`, `SIGILLUM`, or `ORDO`
- marks symbol as internal to its module

### 3.3 `RITUALE` (function)

Syntax:

```cct
RITUALE name(<params>) REDDE <type>
  ...
EXPLICIT RITUALE
```

Notes:
- parameter list is optional
- return type is optional; if omitted, semantic defaults apply
- entry ritual can be `main` or `principium` with zero parameters in the executable subset

### 3.4 `SIGILLUM` (struct-like type)

Syntax:

```cct
SIGILLUM Name
  field: Type
  ...
FIN SIGILLUM
```

With contract conformance:

```cct
SIGILLUM Name PACTUM ContractName
  ...
FIN SIGILLUM
```

### 3.5 `ORDO` (enum-like type)

Syntax:

```cct
ORDO Status
  QUIETUS,
  ACTUS = 10
FIN ORDO
```

### 3.6 `PACTUM` (contract/interface)

Syntax:

```cct
PACTUM Numerabilis
  RITUALE valor(REX x) REDDE REX
FIN PACTUM
```

Current conformance model:
- `SIGILLUM X PACTUM C` declares explicit conformance
- implementation is validated through top-level rituals matching required signatures under the current subset contract

### 3.7 `CODEX` (namespace block)

Syntax:

```cct
CODEX Name
  ...
FIN CODEX
```

Status:
- parsed and semantically traversed
- not part of the stable executable subset for direct codegen workflows

## 4. Type System (Current Subset)

### 4.1 Primitive Types

- `REX` (int64)
- `DUX` (int32)
- `COMES` (int16)
- `MILES` (uint8)
- `UMBRA` (double)
- `FLAMMA` (float)
- `VERBUM` (string)
- `VERUM` (boolean type/literal family)
- `NIHIL` (void/null family)
- `FRACTUM` (failure payload type in failure-control subset)

### 4.2 Pointer and Array Forms

- pointer form: `SPECULUM T`
- dereference operator: `*p`
- address-of operator: `SPECULUM x`
- static array form: `SERIES T [N]`

### 4.3 Size Query

- `MENSURA(Type)` returns size as integer

### 4.4 Type Modifiers: `CONSTANS`

Syntax:

```cct
EVOCA CONSTANS T nome AD expr
```

Semantic contract:
- a `CONSTANS` binding cannot be reassigned through `VINCIRE`
- applies to local variables and `RITUALE` parameters
- reassignment attempts are semantic errors with source location

Code generation contract:
- `CONSTANS` bindings are emitted with `const` in generated C
- this provides defense-in-depth with host C compiler diagnostics

`CONSTANS SPECULUM T` contract:
- the pointer binding is constant (cannot be rebound to another address)
- the pointed value can still be mutated through dereference when allowed by type/rules

### 4.5 Generic Types and Params (`GENUS`)

Type parameter declaration:

```cct
RITUALE identitas GENUS(T)(T x) REDDE T
```

Generic type instantiation:

```cct
EVOCA Capsa GENUS(REX) c
```

Generic call instantiation (explicit only):

```cct
CONIURA identitas GENUS(REX)(7)
```

## 5. Statement Reference

### 5.1 `EVOCA` (declaration)

Syntax:

```cct
EVOCA <Type> name
EVOCA <Type> name AD <expr>
```

### 5.2 `VINCIRE` (assignment)

Syntax:

```cct
VINCIRE <target> AD <expr>
```

Targets supported in the current subset:
- identifiers
- field access
- index access
- pointer dereference

### 5.3 `REDDE` (return)

Syntax:

```cct
REDDE
REDDE <expr>
```

### 5.4 `ANUR` (process exit)

Syntax:

```cct
ANUR <int-expr>
```

### 5.5 `SI / ALITER` (if/else)

Syntax:

```cct
SI <condition>
  ...
ALITER
  ...
FIN SI
```

`ALITER` is optional.

### 5.6 `DUM` (while)

Syntax:

```cct
DUM <condition>
  ...
FIN DUM
```

### 5.7 `DONEC` (do-while)

Syntax:

```cct
DONEC
  ...
DUM <condition>
```

### 5.8 `REPETE` (for-range)

Syntax:

```cct
REPETE i DE <start> AD <end>
  ...
FIN REPETE
```

Optional step:

```cct
REPETE i DE <start> AD <end> GRADUS <step>
  ...
FIN REPETE
```

### 5.9 `ITERUM` (collection iterator)

Syntax:

```cct
ITERUM item IN collection COM
  ...
FIN ITERUM
```

Supported collections in the current subset:
- `FLUXUS` values
- `SERIES T[N]` static arrays
- collection-operation results that materialize as `FLUXUS`/`SERIES`

Notes:
- `COM` is part of the canonical surface syntax.
- iterator variable scope is local to the `ITERUM` body.
- this subset does not include lazy iterators or HashMap/Set iterators.

### 5.10 `DIMITTE` (explicit release)

Syntax:

```cct
DIMITTE pointer_symbol
```

### 5.11 Failure Control: `TEMPTA`, `CAPE`, `SEMPER`, `IACE`

Throw:

```cct
IACE "message"
```

Try/catch:

```cct
TEMPTA
  ...
CAPE FRACTUM erro
  ...
FIN TEMPTA
```

Try/catch/finally:

```cct
TEMPTA
  ...
CAPE FRACTUM erro
  ...
SEMPER
  ...
FIN TEMPTA
```

Subset constraints:
- exactly one `CAPE` in current official subset
- `SEMPER` must come after `CAPE`

### 5.11 `FRANGE`, `RECEDE`, `TRANSITUS`

Status:
- `FRANGE` and `RECEDE`: Stable; valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM`
- `TRANSITUS`: parsed and represented in AST; currently outside the stable executable subset in codegen

## 6. Expression Reference

### 6.1 Literals

- integer: `42`
- real: `3.14`
- string: `"text"`
- boolean: `VERUM`, `FALSUM`
- null-like: `NIHIL`

### 6.2 Calls

- preferred ritual call form for executable subset:

```cct
CONIURA nome(...)
```

- generic call form:

```cct
CONIURA nome GENUS(T1, T2)(...)
```

- direct call expressions are not the recommended executable path; use `CONIURA`

### 6.3 Field and Index Access

- field: `obj.field`
- index: `arr[i]`, `ptr[i]` (with subset rules)

### 6.4 Unary Operators

- `+x`, `-x`
- `NON x`
- `NON_BIT x`
- `*p`
- `SPECULUM x` (address-of)

### 6.5 Binary Operators

Arithmetic and comparison are stable in executable subset:
- `+`, `-`, `*`, `/`, `%`, `**`, `//`, `%%`
- `==`, `!=`, `<`, `<=`, `>`, `>=`

Logical/bitwise/shift forms:
- `ET`, `VEL` (stable in executable subset)
- `ET_BIT`, `VEL_BIT`, `XOR`
- `SINISTER`, `DEXTER`

Executable support note:
- arithmetic/comparison, logical `ET`/`VEL`, and bitwise/shift forms are stable codegen paths
- bitwise/shift operators require integer operands

13M arithmetic addendum:
- `**` is exponentiation and is right-associative (`2 ** 3 ** 2` = `2 ** (3 ** 2)`).
- `//` is floor integer division (requires integer operands).
- `%%` is Euclidean modulo (requires integer operands), coherent with `//`.
- `%` remains available with historical behavior for compatibility.

### 6.6 Precedence and Associativity (Stable Executable Core)

From higher to lower priority in arithmetic/comparison core:

1. unary: `+x`, `-x`, `NON x`, `*p`, `SPECULUM x`
2. power: `**` (right-associative)
3. multiplicative: `*`, `/`, `%`, `//`, `%%`
4. additive: `+`, `-`
5. comparison/equality: `==`, `!=`, `<`, `<=`, `>`, `>=`

Examples:
- `2 + 3 ** 2 * 4` => `2 + ((3 ** 2) * 4)`
- `20 // 3 + 1` => `(20 // 3) + 1`
- `20 // (3 + 1)` => `20 // 4`

## 7. `OBSECRO` Builtins

### 7.1 Stable in current executable subset

- `OBSECRO scribe(...)`
Prints values. Supports mixed scalar/string/real forms in current subset.

- `OBSECRO pete(size_expr)`
Allocates memory and returns pointer expression value.

- `OBSECRO libera(ptr_expr)`
Releases memory when used as a statement.

### 7.2 Recognized but currently restricted in executable subset

- `OBSECRO lege(...)`
- `OBSECRO aperi(...)`
- `OBSECRO claude(...)`

These names are recognized in semantic builtin tables, but executable support is not fully part of the stable subset.

## 8. Modules and Visibility

### 8.1 Import and Resolution

- imports are declared with `ADVOCARE "path.cct"`
- no implicit transitive symbol visibility
- duplicate global symbol declarations across module closure are rejected
- import cycles are rejected
- reserved `cct/...` imports are resolved from the canonical Bibliotheca Canonica root
- stdlib root resolution order for `cct/...` imports:
  1. `CCT_STDLIB_DIR` environment variable (when set)
  2. build-time canonical stdlib path

### 8.2 Bibliotheca Canonica Namespace

Canonical stdlib modules use the reserved namespace:

```cct
ADVOCARE "cct/verbum.cct"
ADVOCARE "cct/io.cct"
```

FASE 11A policy:
- `cct/...` is reserved for Bibliotheca Canonica
- canonical modules resolve from compiler-distributed stdlib path
- user-local modules do not override canonical `cct/...` targets

### 8.3 Visibility

- top-level declarations are public by default
- `ARCANUM` marks declarations as internal to their module
- external access to internal symbols is rejected

### 8.4 Canonical Text Module (`cct/verbum`)

Import:

```cct
ADVOCARE "cct/verbum.cct"
```

Available operations in the 11B.1 subset:
- `len(s)` -> `REX`
- `concat(a, b)` -> `VERBUM`
- `compare(a, b)` -> `REX` (`-1`, `0`, `1`)
- `substring(s, inicio, fim)` -> `VERBUM` (strict bounds)
- `trim(s)` -> `VERBUM`
- `contains(s, sub)` -> `VERUM`
- `find(s, sub)` -> `REX` (`-1` when missing)

### 8.5 Canonical Formatting Module (`cct/fmt`)

Import:

```cct
ADVOCARE "cct/fmt.cct"
```

Available operations in the 11B.2 subset:
- stringify: `stringify_int`, `stringify_real`, `stringify_float`, `stringify_bool`
- parse façade: `fmt_parse_int`, `fmt_parse_real`, `fmt_parse_bool`
- composition: `format_pair`

Subset behavior:
- stringify produces textual values suitable for display/logging
- `parse_int` / `parse_real` are strict and fail on invalid textual input

### 8.6 Canonical Static-Collection Module (`cct/series`)

Import:

```cct
ADVOCARE "cct/series.cct"
```

Available operations in the 11C subset:
- `series_len GENUS(T)(SPECULUM T arr, REX tamanho) -> REX`
- `series_fill GENUS(T)(SPECULUM T arr, T valor, REX tamanho) -> NIHIL`
- `series_copy GENUS(T)(SPECULUM T dest, SPECULUM T src, REX tamanho) -> NIHIL`
- `series_reverse GENUS(T)(SPECULUM T arr, REX tamanho) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX tamanho) -> VERUM`

Subset behavior:
- `series_fill`, `series_copy`, and `series_reverse` are generic mutation helpers
- `series_contains` is integer-focused in this subset (`REX`)
- caller provides explicit `tamanho` and is responsible for shape correctness

### 8.7 Canonical Baseline Algorithms Module (`cct/alg`)

Import:

```cct
ADVOCARE "cct/alg.cct"
```

Available operations in the 11F.2 subset:
- `alg_linear_search(SPECULUM REX arr, REX valor, REX tamanho) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX tamanho) -> VERUM`
- `alg_binary_search(SPECULUM REX arr, REX tamanho, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX tamanho) -> NIHIL`

Subset behavior:
- algorithms are intentionally small and deterministic
- both routines target integer arrays in this initial subset
- binary search expects sorted input
- insertion sort is in-place and intentionally minimal

### 8.8 Canonical Memory Utility Module (`cct/mem`)

Import:

```cct
ADVOCARE "cct/mem.cct"
```

Available operations in the 11D.1 subset:
- `alloc(size)` -> pointer
- `free(ptr)` -> `NIHIL`
- `realloc(ptr, new_size)` -> pointer
- `copy(dest, src, size)` -> `NIHIL`
- `set(ptr, value, size)` -> `NIHIL`
- `zero(ptr, size)` -> `NIHIL`
- `mem_compare(a, b, size)` -> `REX` (`-1`, `0`, `1`)

Subset behavior:
- ownership is explicit: allocator side must release with `free`
- memory API is runtime-backed and deterministic
- this is the foundation for dynamic collections in the 11D block

### 8.9 Canonical Dynamic-Vector Module (`cct/fluxus`)

Import:

```cct
ADVOCARE "cct/fluxus.cct"
```

Available operations in the 11D.3 subset:
- `fluxus_init(elem_size)` -> pointer
- `fluxus_free(flux)` -> `NIHIL`
- `fluxus_push(flux, elem_ptr)` -> `NIHIL`
- `fluxus_pop(flux, out_ptr)` -> `NIHIL`
- `fluxus_len(flux)` -> `REX`
- `fluxus_get(flux, index)` -> pointer
- `fluxus_clear(flux)` -> `NIHIL`
- `fluxus_reserve(flux, capacity)` -> `NIHIL`
- `fluxus_capacity(flux)` -> `REX`

Subset behavior:
- explicit ownership: caller must release instances with `fluxus_free`
- deterministic growth strategy (x2, initial growth bucket of 8)
- strict runtime failure for invalid operations (null instance, pop empty, out-of-bounds get)

### 8.10 Canonical IO Module (`cct/io`)

Import:

```cct
ADVOCARE "cct/io.cct"
```

Available operations in the 11E.1 subset:
- `print(s)` -> `NIHIL`
- `println(s)` -> `NIHIL`
- `print_int(n)` -> `NIHIL`
- `read_line()` -> `VERBUM`

Subset behavior:
- runtime-backed stdout/stderr bridge with deterministic textual output semantics
- `read_line` reads one stdin line and returns a caller-owned string
- subset intentionally small and terminal-focused

### 8.11 Canonical Filesystem Module (`cct/fs`)

Import:

```cct
ADVOCARE "cct/fs.cct"
```

Available operations in the 11E.1 subset:
- `read_all(path)` -> `VERBUM`
- `write_all(path, content)` -> `NIHIL`
- `append_all(path, content)` -> `NIHIL`
- `exists(path)` -> `VERUM`
- `size(path)` -> `REX`

Subset behavior:
- strict read/size error paths with clear diagnostics
- ownership is explicit for text returned by `read_all`
- API is whole-file focused (no directory listing/streaming in this subset)

### 8.12 Canonical Path Module (`cct/path`)

Import:

```cct
ADVOCARE "cct/path.cct"
```

Available operations in the 11E.2 subset:
- `path_join(a, b)` -> `VERBUM`
- `path_basename(p)` -> `VERBUM`
- `path_dirname(p)` -> `VERBUM`
- `path_ext(p)` -> `VERBUM`

Subset behavior:
- canonical separator in produced output is `/`
- both `/` and `\\` are accepted as input separators in parsing logic
- API is intentionally minimal and text-semantic (not a full OS path framework)

### 8.13 Canonical Math Module (`cct/math`)

Import:

```cct
ADVOCARE "cct/math.cct"
```

Available operations in the 11F.1 subset:
- `abs(x)` -> `REX`
- `min(a, b)` -> `REX`
- `max(a, b)` -> `REX`
- `clamp(x, lo, hi)` -> `REX`

Subset behavior:
- deterministic integer-oriented helpers for everyday numeric logic
- `clamp` is strict and fails when `lo > hi`

### 8.14 Canonical Random Module (`cct/random`)

Import:

```cct
ADVOCARE "cct/random.cct"
```

Available operations in the 11F.1 subset:
- `seed(s)` -> `NIHIL`
- `random_int(lo, hi)` -> `REX`
- `random_real()` -> `FLAMMA`

Subset behavior:
- deterministic seeded PRNG baseline for reproducible workflows in this environment
- `random_int` is inclusive on both bounds and fails when `lo > hi`
- `random_real` returns values in `[0, 1)`

### 8.15 Canonical Parse Module (`cct/parse`)

Import:

```cct
ADVOCARE "cct/parse.cct"
```

Available operations in the 11F.2 subset:
- `parse_int(text)` -> `REX`
- `parse_real(text)` -> `UMBRA`
- `parse_bool(text)` -> `VERUM`

Subset behavior:
- strict conversion behavior for primitive textual parsing
- invalid integer/real text fails clearly
- invalid boolean text fails clearly (`parse_bool invalid input`)

### 8.16 Canonical Compare Module (`cct/cmp`)

Import:

```cct
ADVOCARE "cct/cmp.cct"
```

Available operations in the 11F.2 subset:
- `cmp_int(a, b)` -> `REX`
- `cmp_real(a, b)` -> `REX`
- `cmp_bool(a, b)` -> `REX`
- `cmp_verbum(a, b)` -> `REX`

Subset behavior:
- canonical comparator contract: `<0` (left < right), `0` (equal), `>0` (left > right)
- stable cross-module comparison API for ordering/equality workflows

### 8.17 Canonical Option Module (`cct/option`)

Import:

```cct
ADVOCARE "cct/option.cct"
```

Available operations in the 12C subset:
- constructors: `Some GENUS(T)(value)`, `None GENUS(T)()`
- checks: `option_is_some GENUS(T)(opt)`, `option_is_none GENUS(T)(opt)`
- extraction: `option_unwrap GENUS(T)(opt)`, `option_unwrap_or GENUS(T)(opt, default)`, `option_expect GENUS(T)(opt, message)`
- lifecycle: `option_free GENUS(T)(opt)`

Subset behavior:
- Option values are opaque pointer-backed instances
- callers must release instances with `option_free`
- `option_unwrap`/`option_expect` fail clearly on `None`

### 8.18 Canonical Result Module (`cct/result`)

Import:

```cct
ADVOCARE "cct/result.cct"
```

Available operations in the 12C subset:
- constructors: `Ok GENUS(T,E)(value)`, `Err GENUS(T,E)(error)`
- checks: `result_is_ok GENUS(T,E)(res)`, `result_is_err GENUS(T,E)(res)`
- extraction: `result_unwrap GENUS(T,E)(res)`, `result_unwrap_or GENUS(T,E)(res, default)`, `result_unwrap_err GENUS(T,E)(res)`, `result_expect GENUS(T,E)(res, message)`
- lifecycle: `result_free GENUS(T,E)(res)`

Subset behavior:
- Result values are opaque pointer-backed instances
- callers must release instances with `result_free`
- `result_unwrap` and `result_expect` fail clearly on `Err`
- `result_unwrap_err` fails clearly on `Ok`

### 8.19 Canonical Map Module (`cct/map`)

Import:

```cct
ADVOCARE "cct/map.cct"
```

Available operations in the 12D.1 subset:
- lifecycle: `map_init GENUS(K,V)()`, `map_free GENUS(K,V)(map)`
- mutation: `map_insert GENUS(K,V)(map, key, value)`, `map_remove GENUS(K,V)(map, key)`, `map_clear GENUS(K,V)(map)`, `map_reserve GENUS(K,V)(map, additional)`
- query: `map_get GENUS(K,V)(map, key)`, `map_contains GENUS(K,V)(map, key)`, `map_len GENUS(K,V)(map)`, `map_is_empty GENUS(K,V)(map)`, `map_capacity GENUS(K,V)(map)`

Subset behavior:
- map instances are opaque pointer-backed runtime structures
- `map_get` returns a pointer-backed Option payload (`Some`/`None` model from `cct/option`)
- `map_insert` updates existing keys in-place and does not duplicate key cardinality
- callers must release instances with `map_free`

### 8.20 Canonical Set Module (`cct/set`)

Import:

```cct
ADVOCARE "cct/set.cct"
```

Available operations in the 12D.1 subset:
- lifecycle: `set_init GENUS(T)()`, `set_free GENUS(T)(set)`
- mutation: `set_insert GENUS(T)(set, item)`, `set_remove GENUS(T)(set, item)`, `set_clear GENUS(T)(set)`
- query: `set_contains GENUS(T)(set, item)`, `set_len GENUS(T)(set)`, `set_is_empty GENUS(T)(set)`

Subset behavior:
- set instances are opaque pointer-backed runtime structures
- duplicates are ignored (`set_insert` returns `FALSUM` when item already exists)
- callers must release instances with `set_free`

### 8.21 Canonical Collection Ops Module (`cct/collection_ops`)

Import:

```cct
ADVOCARE "cct/collection_ops.cct"
```

Available operations in the 12D.2 subset:
- FLUXUS: `fluxus_map`, `fluxus_filter`, `fluxus_fold`, `fluxus_find`, `fluxus_any`, `fluxus_all`
- SERIES: `series_map`, `series_filter`, `series_reduce`, `series_find`, `series_any`, `series_all`

Current 12D.2 callback model:
- callbacks are passed as rituale pointers (`SPECULUM NIHIL`) and must be named rituales
- runtime bridge currently supports word-sized callback payloads (canonical `REX`/`VERUM`/pointer-sized flows)
- no closures in this subset (closure model remains for later phases)

### 8.22 Canonical Showcase Usage Patterns (11G)

Canonical showcase programs are part of the public stdlib contract:
- `examples/showcase_stdlib_string_11g.cct`
- `examples/showcase_stdlib_collection_11g.cct`
- `examples/showcase_stdlib_io_fs_11g.cct`
- `examples/showcase_stdlib_parse_math_random_11g.cct`
- `examples/showcase_stdlib_modular_11g_main.cct`

These examples must remain compatible with:
- `--check`
- `--ast` (single-file showcases)
- `--ast-composite` (multi-module showcase)
- `--sigilo-only`
- `--sigilo-mode essencial|completo`

Sigilo metadata in 11G+12D.1 includes stdlib-usage fields such as:
- `stdlib_module_count`
- `stdlib_modules_used`
- module operation counters (`verbum_ops_count`, `fmt_ops_count`, `series_ops_count`, `fluxus_ops_count`, `mem_ops_count`, `io_ops_count`, `fs_ops_count`, `path_ops_count`, `math_ops_count`, `random_ops_count`, `parse_ops_count`, `cmp_ops_count`, `alg_ops_count`, `option_ops_count`, `result_ops_count`, `map_ops_count`, `set_ops_count`)

### 8.23 Final Subset and Stability References (11H)

The frozen final stdlib subset and stability classes are documented in:
- `docs/stdlib_subset_11h.md`
- `docs/stdlib_stability_matrix_11h.md`

## 9. Advanced Typing (FASE 10E Contract)

### 9.1 `GENUS`

- explicit generic parameters in declarations
- explicit type arguments at use sites
- executable materialization through pragmatic monomorphization

### 9.2 `PACTUM`

- explicit contract declarations
- explicit conformance on `SIGILLUM`
- semantic conformance validation enabled in current subset

### 9.3 Basic Constraints

Supported form:

```cct
GENUS(T PACTUM Contract)
```

Current subset limits:
- one contract per type parameter
- constrained usage in supported generic ritual contexts
- no advanced inference or solver logic

## 10. Sigilo Specification (Operational)

For valid programs, CCT can emit:
- local sigilo: `<base>.svg` and `<base>.sigil`
- system sigilo for module closure: `<entry>.system.svg` and `<entry>.system.sigil`

Emission modes for multi-module workflows:
- essential (`essencial`): entry local + system
- complete (`completo`): entry local + imported module locals + system

System sigilo model:
- vector inline sigil-of-sigils composition
- deterministic metadata and hash behavior by structure

Baseline contract (FASE 13A.4):
- default baseline location: `docs/sigilo/baseline/local.sigil` or `docs/sigilo/baseline/system.sigil`
- baseline metadata sidecar: `<baseline>.baseline.meta`
- `check` is read-only (no implicit updates)
- `update` is explicit, and overwrite requires `--force`

Project sigilo operational report contract (FASE 13B.4):
- output signature: `format=cct.sigilo.report.v1`
- default mode: summary (`--sigilo-report summary`)
- detailed mode: `--sigilo-report detailed` (includes per-diff domain and before/after when CI profile gate computes diff)
- explain mode: `--sigilo-explain` (probable cause + recommended action + troubleshooting reference)
- compatibility note: legacy `sigilo.ci ...` line is preserved for existing script consumers
- consolidated operations guide: `docs/sigilo_operations_14b2.md`

Sigilo schema governance contract (FASE 13C.1):
- canonical schema identifier remains `format = cct.sigil.v1` during FASE 13
- evolution is additive by default (no incompatible format break in this phase)
- unknown additive top-level fields are warning-only in tolerant/strict parser modes
- deprecated field support is explicit (`sigilo_style` accepted with warning; replacement: `visual_engine`)
- normative schema governance remains enforced by the current validator/parser contract (historical 13C1 governance notes are archived privately)

Sigilo analytical metadata expansion (FASE 13C.2):
- additive analytical sections: `analysis_summary`, `diff_fingerprint_context`, `module_structural_summary`, `compatibility_hints`
- deterministic serialization order for analytical blocks in local/system `.sigil`
- no volatile timestamp fields in deterministic profile
- diff classification treats analytical blocks as review-required, except `compatibility_hints` (informational)

Sigilo consumer compatibility contract (FASE 13C.3):
- consumer profiles: `legacy-tolerant`, `current-default`, `strict-contract`
- CLI selection: `--consumer-profile legacy-tolerant|current-default|strict-contract` (`--strict` maps to `strict-contract`)
- higher same-family schema (`cct.sigil.v2+`) uses warning + v1-compatible fallback in non-strict profiles
- strict-contract keeps schema mismatch as blocking error
- migration/fallback behavior is covered by current operational guidance and `--consumer-profile` validation contracts

Sigilo strict/tolerant validator contract (FASE 13C.4):
- canonical validator entrypoint: `cct sigilo validate <artifact.sigil>`
- validation domains: required fields, type/cardinality checks, and cross-section consistency
- tolerant profiles classify recoverable issues as warnings and keep processing when safe
- strict-contract profile blocks contractual violations with deterministic non-zero exit status
- diagnostics are harmonized with objective message + corrective action hint (`action: ...`)

Diagnostic taxonomy contract (FASE 14A.1):
- canonical diagnostic levels: `error`, `warning`, `note`, `hint`
- actionable diagnostics preserve legacy `suggestion:` line and additionally emit `hint:` line
- sigilo text diagnostics are emitted through the canonical diagnostic envelope (`[sigilo]` code label)

Explain and troubleshooting contract (FASE 14A.3):
- `cct sigilo inspect|validate|diff|check|baseline check` accepts `--explain`
- explain mode appends `sigilo.explain probable_cause=... recommended_action=... docs=... blocked=... command=...`
- default mode remains concise and does not emit explain lines unless explicitly requested

Deterministic output/log hygiene contract (FASE 14A.4):
- sigilo diagnostics are emitted in deterministic order (`level`, `kind`, `line`, `column`, `message`)
- structured validation output (`[diag.N]`) follows the same deterministic ordering
- deterministic ordering is guaranteed for equivalent artifact content/profile inputs

FASE 13 release references (13D.3):
- release notes: `docs/release/FASE_13_RELEASE_NOTES.md`
- detailed 13D closure matrices/snapshots are preserved in archived/internal release governance records

Publication boundary policy reference (FASE 14B.3):

Release documentation template pack (FASE 14B.4):

## 11. Keyword Catalog

Legend:
- **Stable**: part of the documented usable subset
- **Partial**: parsed and/or semantically recognized, with executable or contextual restrictions
- **Reserved**: tokenized keyword reserved for future phases

### 11.1 Structure, Module, Declaration Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `INCIPIT` | file/block start | Stable | mandatory file opener |
| `EXPLICIT` | file/ritual end | Stable | file closes with `EXPLICIT grimoire` |
| `FIN` | block/declaration terminator | Stable | used in `SI`, `DUM`, `REPETE`, `ITERUM`, `SIGILLUM`, `ORDO`, `PACTUM`, `CODEX` |
| `ADVOCARE` | import module | Stable | multi-module closure |
| `ARCANUM` | internal visibility marker | Stable | top-level `RITUALE`, `SIGILLUM`, `ORDO` only |
| `RITUALE` | ritual/function declaration | Stable | core executable declaration |
| `SIGILLUM` | struct-like type | Stable | advanced subset supported |
| `ORDO` | enum-like type | Stable | pragmatic subset |
| `PACTUM` | contract/interface | Stable | explicit conformance model |
| `GENUS` | generic params and explicit type args | Stable | explicit use only |
| `CODEX` | namespace-like block | Partial | parsed/semantic; not primary executable path |
| `CIRCULUS` | reserved scope keyword | Reserved | tokenized only |
| `CONSTANS` | type modifier | Stable | reassignment blocked in semantic analysis; emitted as `const` in generated C (locals + rituale parameters) |
| `VOLATILE` | type modifier | Partial | parsed as modifier; no full semantic enforcement |
| `FLUXUS` | dynamic-array modifier keyword | Partial | tokenized/parsed in type syntax with restrictions |

### 11.2 Control and Flow Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `SI` | if | Stable | supports `ALITER` |
| `ALITER` | else | Stable | optional |
| `DUM` | while / do-while trailer | Stable | used by both `DUM` and `DONEC ... DUM` |
| `DONEC` | do-while block start | Stable | post-condition form |
| `REPETE` | for-range loop | Stable | `DE`, `AD`, optional `GRADUS` |
| `ITERUM` | collection iteration loop | Stable | `ITERUM item IN collection COM ... FIN ITERUM` |
| `DE` | from | Stable | `REPETE` header |
| `AD` | to/assign delimiter | Stable | in `REPETE` and `VINCIRE`/`EVOCA` initializer syntax |
| `GRADUS` | step | Stable | optional in `REPETE` |
| `PRO` | foreach-like keyword | Reserved | tokenized; not active grammar |
| `IN` | iterator membership keyword | Stable | used in `ITERUM` headers |
| `FRANGE` | break | Stable | valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM` |
| `RECEDE` | continue | Stable | valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM` |
| `TRANSITUS` | goto | Partial | parsed; currently outside stable codegen path |

### 11.3 Execution and Memory Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `EVOCA` | variable declaration | Stable | optional initializer with `AD` |
| `VINCIRE` | assignment | Stable | `VINCIRE target AD expr` |
| `REDDE` | return | Stable | expression optional depending on ritual type |
| `ANUR` | process exit | Stable | integer expression expected |
| `CONIURA` | explicit ritual call | Stable | preferred executable call form |
| `OBSECRO` | builtin call namespace | Partial | stable for `scribe`, `pete`, `libera` subset |
| `DIMITTE` | pointer release statement | Stable | explicit discard path |
| `MENSURA` | size-of by type | Stable | returns integer size |

### 11.4 Failure-Control Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `TEMPTA` | try | Stable | current failure subset |
| `CAPE` | catch | Stable | one handler in final phase-8 subset |
| `SEMPER` | finally | Stable | must follow `CAPE` |
| `IACE` | throw | Stable | payload subset restrictions apply |
| `FRACTUM` | failure payload type | Stable | failure-control subset type |

### 11.5 Type and Literal Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `REX` | int64 | Stable | |
| `DUX` | int32 | Stable | |
| `COMES` | int16 | Stable | |
| `MILES` | uint8 | Stable | |
| `UMBRA` | double | Stable | |
| `FLAMMA` | float | Stable | |
| `VERBUM` | string | Stable | |
| `VERUM` | boolean true / bool family | Stable | literal and type family |
| `FALSUM` | boolean false | Stable | literal |
| `NIHIL` | null/void family | Stable | |
| `SPECULUM` | pointer modifier / address-of unary | Stable | subset-limited pointer system |
| `SERIES` | static array form | Stable | subset-limited array system |

### 11.6 Logical, Bitwise, and Shift Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `ET` | logical and | Stable | short-circuit in conditionals and value expressions |
| `VEL` | logical or | Stable | short-circuit in conditionals and value expressions |
| `NON` | logical not | Stable | unary |
| `ET_BIT` | bitwise and | Stable | integer operands required |
| `VEL_BIT` | bitwise or | Stable | integer operands required |
| `XOR` | bitwise xor | Stable | integer operands required |
| `NON_BIT` | bitwise not | Stable | integer operand required |
| `SINISTER` | shift left | Stable | integer operands required |
| `DEXTER` | shift right | Stable | integer operands required |

## 12. Basic Programming Examples

### 12.1 Hello World

```cct
INCIPIT grimoire "hello"

RITUALE main() REDDE REX
  OBSECRO scribe("Hello from CCT\n")
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

### 12.2 If/Loop/Assignment

```cct
INCIPIT grimoire "flow"

RITUALE main() REDDE REX
  EVOCA REX i AD 0
  DUM i < 3
    VINCIRE i AD i + 1
  FIN DUM

  SI i == 3
    REDDE 42
  ALITER
    REDDE 1
  FIN SI
EXPLICIT RITUALE

EXPLICIT grimoire
```

### 12.3 Module Import + Internal Visibility

```cct
INCIPIT grimoire "main"

ADVOCARE "lib.cct"

RITUALE main() REDDE REX
  REDDE CONIURA public_value()
EXPLICIT RITUALE

EXPLICIT grimoire
```

## 13. Known Subset Boundaries (Important)

- Some tokenized keywords are reserved and not active grammar yet.
- Some parsed/semantic constructs are outside current stable executable codegen subset.
- Generic use is explicit by design in current phases.
- Contract and constraint models are intentionally conservative in this baseline.

Always rely on:
- `--check` for semantic diagnostics
- integration tests in `tests/` for canonical supported patterns

## 14. Project Workflow (FASE 12F)

Canonical project layout:

```text
project/
├── src/main.cct
├── lib/
├── tests/
├── bench/
└── cct.toml (optional)
```

Conventions:
- test files end with `.test.cct`
- benchmark files end with `.bench.cct`
- project-local artifacts are stored under `.cct/`
- distribution binaries are emitted under `dist/`

Incremental contract:
- `cct build` fingerprints entry + module closure + profile
- unchanged fingerprint and existing output => `up-to-date`
- cache is stored at `.cct/cache/manifest.txt`

Quality-gate contract:
- `cct test --strict-lint` makes lint warnings fail the run (exit `2`)
- `cct test --fmt-check` makes formatting mismatches fail the run (exit `2`)

## 15. API Documentation Generation (FASE 12G)

`cct doc` generates API pages from project module closure.

Default output:

```text
<project>/docs/api/
```

Key options:
- `--format markdown|html|both`
- `--include-internal`
- `--warn-missing-docs`
- `--strict-docs`
- `--no-timestamp`

Strict contract:
- warnings are non-fatal by default
- with `--strict-docs`, warnings return exit code `2`
