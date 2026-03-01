# FASE 12 — Final Snapshot

> **Status:** Canonized and frozen as of FASE 12H
> **Date:** March 2026
> **Compiler:** CCT (Clavicula Turing)

---

## Executive Summary

The **FASE 12** block represents the **structural maturity milestone** of the CCT language project. It consolidates:

- High-quality diagnostics with actionable suggestions
- Controlled type coercions and numeric casts
- Ergonomic error handling with Option/Result
- Hash-backed collections (HashMap/HashSet)
- Functional collection operations
- Iterator syntax for common collection types
- Standalone formatter (`cct fmt`)
- Canonical linter (`cct lint`)
- Project-based build system with incremental compilation
- API documentation generator (`cct doc`)

This phase transforms CCT from a language implementation into a **usable toolchain** with:
- end-to-end developer workflow support
- consistent CLI experience across all tools
- stable stdlib foundation (`cct/...` canonical modules)
- deterministic sigil generation integrated throughout

---

## Official Scope — What Is In FASE 12

### FASE 12A — High-Quality Diagnostics

**Status:** `stable`

**What was delivered:**
- Structured diagnostic messages with source snippets
- Precise line/column positioning
- Actionable suggestions for common errors
- Consistent format across parser/semantic/codegen
- Support for `--no-color` flag

**Public surface:**
- Error message format contract
- Diagnostic output structure

**CLI impact:**
- All compile/check/build commands emit harmonized diagnostics

---

### FASE 12B — Numeric Casts and Coercions

**Status:** `stable`

**What was delivered:**
- Explicit numeric cast syntax: `cast GENUS(T)(expr)`
- Controlled coercion rules for numeric literals
- Type-safe conversions between `REX`, `UMBRA`, `FLAMMA`
- Clear error messages for invalid casts

**Public surface:**
- `cast GENUS(T)(value)` expression form
- Supported target types: `REX`, `UMBRA`, `FLAMMA`

**Language impact:**
- Required explicit casts prevent silent precision loss
- Predictable numeric type promotion

---

### FASE 12C — Option and Result Types

**Status:** `experimental`

**What was delivered:**
- Canonical `cct/option` module with `Some`/`None` constructors
- Canonical `cct/result` module with `Ok`/`Err` constructors
- Ergonomic API: `unwrap()`, `unwrap_or(default)`, `expect(msg)`
- Pattern matching integration with existing `ORDO` mechanisms
- Nullable-free alternative for fallible operations

**Public surface:**
- `cct/option.cct` API
- `cct/result.cct` API

**Stability note:**
- Core API is functional and useful
- May receive minor ergonomic refinements in future phases
- Not breaking changes expected, but not yet fully hardened

---

### FASE 12D.1 — HashMap and HashSet

**Status:** `experimental`

**What was delivered:**
- Canonical `cct/map` module for hash-based key-value storage
- Canonical `cct/set` module for hash-based unique element storage
- Core operations: `insert`, `get`, `contains`, `remove`, `len`, `clear`
- Deterministic iteration order (insertion-order preservation)

**Public surface:**
- `cct/map.cct` API
- `cct/set.cct` API

**Stability note:**
- Functional for common use cases
- API may be refined for ergonomics or performance
- No lazy evaluation or advanced iterator protocol yet

---

### FASE 12D.2 — Functional Collection Operations

**Status:** `experimental`

**What was delivered:**
- Canonical `cct/collection_ops` module
- Operations on `FLUXUS` (dynamic vector): `fluxus_map`, `fluxus_filter`, `fluxus_fold`, `fluxus_find`, `fluxus_any`, `fluxus_all`
- Operations on `SERIES` (arrays): `series_map`, `series_filter`, `series_reduce`, `series_find`, `series_any`, `series_all`
- Functional programming patterns without lazy evaluation

**Public surface:**
- `cct/collection_ops.cct` API

**Stability note:**
- Useful for data transformation pipelines
- Eager evaluation model (no lazy streams)
- May add more combinators in future phases

---

### FASE 12D.3 — Iterator Syntax

**Status:** `experimental`

**What was delivered:**
- Baseline iterator syntax: `ITERUM item IN collection COM ... FIN ITERUM`
- Support for `FLUXUS`, `SERIES`, and collection-op result iterators
- Syntax sugar for common iteration patterns
- Parser/semantic/codegen support

**Public surface:**
- `ITERUM ... IN ... COM ... FIN ITERUM` language construct

**Stability note:**
- Syntax is stable for supported collection types
- Advanced iterator protocol (custom iterators) is not yet exposed
- Future phases may add iterator composition without breaking existing code

---

### FASE 12E.1 — Standalone Formatter

**Status:** `stable`

**What was delivered:**
- `cct fmt <file.cct> [more.cct ...]` — format files in place
- `cct fmt --check <file.cct>` — verify formatting (exit 2 on mismatch)
- `cct fmt --diff <file.cct>` — show formatting diff without writing
- Idempotent formatting guarantee
- Semantic-preserving transformations only

**Public surface:**
- `cct fmt` command and flags
- Exit codes: `0` (success), `2` (formatting mismatch in `--check`)

**Quality contract:**
- Formatting is deterministic and idempotent
- Running `cct fmt` twice produces identical output
- Formatted code compiles and behaves identically to original

---

### FASE 12E.2 — Canonical Linter

**Status:** `experimental`

**What was delivered:**
- `cct lint <file.cct>` — run canonical lint rules
- `cct lint --strict <file.cct>` — treat warnings as CI failure (exit 2)
- `cct lint --fix <file.cct>` — apply safe automatic fixes
- Rules: `unused-variable`, `unused-parameter`, `unused-import`, `dead-code-after-return`, `dead-code-after-throw`, `shadowing-local`

**Public surface:**
- `cct lint` command and flags
- Exit codes: `0` (clean), `1` (warnings in normal mode), `2` (warnings in strict mode)

**Stability note:**
- Core rule set is functional and useful
- No advanced dataflow analysis yet
- Rule set may expand in future phases
- `--fix` is safe (preserves semantics) but limited to simple cases

---

### FASE 12F — Project-Based Build System

**Status:** `stable`

**What was delivered:**
- `cct build [--project DIR]` — compile project with canonical structure
- `cct run [--project DIR] [-- --args]` — build and execute project binary
- `cct test [pattern] [--project DIR]` — run `*.test.cct` test files
- `cct bench [pattern] [--project DIR]` — run `*.bench.cct` benchmark files
- `cct clean [--project DIR] [--all]` — remove build artifacts
- Incremental compilation with cache invalidation
- Project conventions: `src/main.cct` entry, `lib/`, `tests/`, `bench/`, `cct.toml`

**Public surface:**
- `cct build|run|test|bench|clean` commands
- Project structure conventions
- `cct.toml` configuration format (basic)

**Quality contract:**
- Incremental builds are correct (invalidate on source/import changes)
- `cct test` discovers and executes all `*.test.cct` files
- `cct bench` discovers and executes all `*.bench.cct` files
- `cct clean` removes only build artifacts, not source files
- Legacy single-file workflow (`./cct file.cct`) remains compatible

---

### FASE 12G — API Documentation Generator

**Status:** `experimental`

**What was delivered:**
- `cct doc [--project DIR]` — generate API documentation
- `--format markdown|html|both` — output format selection
- `--include-internal` — toggle `ARCANUM` symbol visibility in docs
- `--warn-missing-docs` — emit warnings for undocumented symbols
- `--strict-docs` — treat doc warnings as failure (exit 2)
- `--no-timestamp` — deterministic output for version control
- Doc tags: `@param`, `@return`, `@example` (parsed and rendered)

**Public surface:**
- `cct doc` command and flags
- Output directory: `docs/api/`
- Doc comment format (triple-slash `///`)

**Stability note:**
- Static documentation generation (no runtime introspection)
- Functional for module/symbol API pages
- May add more doc tags and rendering options in future phases
- Deterministic output (`--no-timestamp`) is stable

---

## Official Stability Classification

| Component                | Surface                             | Status        | Since | Notes                                |
|--------------------------|-------------------------------------|---------------|-------|--------------------------------------|
| Core Compiler            | lexer/parser/semantic/codegen       | `stable`      | 0–4   | Base consolidated                    |
| Sigilo Engine            | local/system essencial/completo     | `stable`      | 9–10  | Deterministic                        |
| Module System            | ADVOCARE/resolution/visibility      | `stable`      | 9     | Contract consolidated                |
| Advanced Typing          | GENUS/PACTUM/constraints            | `stable`      | 10    | Subset frozen                        |
| Diagnostics              | format + suggestions                | `stable`      | 12A   | Mature contract                      |
| Casts                    | cast GENUS(T)(expr)                 | `stable`      | 12B   | Numeric controlled                   |
| Option/Result            | cct/option cct/result               | `experimental`| 12C   | Future API refinements possible      |
| HashMap/Set              | cct/map cct/set                     | `experimental`| 12D.1 | API may receive adjustments          |
| Collection Ops           | cct/collection_ops                  | `experimental`| 12D.2 | No lazy eval                         |
| Iterators                | ITERUM ... IN ... COM               | `experimental`| 12D.3 | No advanced protocol yet             |
| Formatter                | cct fmt                             | `stable`      | 12E.1 | Idempotency required                 |
| Linter                   | cct lint                            | `experimental`| 12E.2 | Basic rules                          |
| Project Build            | cct build/test/run/bench/clean      | `stable`      | 12F   | No package manager                   |
| Doc Generator            | cct doc                             | `experimental`| 12G   | Static only                          |
| Runtime Internal Helpers | internals C helpers                 | `internal`    | 0–12  | Not public API                       |

---

## CLI Commands — Official Surface

### Compilation and Inspection

```bash
./cct <file.cct>                  # compile (with sigil generation)
./cct --tokens <file.cct>         # show token stream
./cct --ast <file.cct>            # show single-module AST
./cct --ast-composite <file.cct>  # show multi-module composed AST
./cct --check <file.cct>          # syntax + semantic checks only
./cct --sigilo-only <file.cct>    # generate sigil artifacts only
```

### Formatting

```bash
./cct fmt <file.cct> [more.cct ...]        # format in place
./cct fmt --check <file.cct>               # verify formatting
./cct fmt --diff <file.cct>                # show diff without writing
```

### Linting

```bash
./cct lint <file.cct>       # run lint rules
./cct lint --strict <file>  # treat warnings as failure
./cct lint --fix <file>     # apply safe automatic fixes
```

### Project Workflow

```bash
./cct build [--project DIR]              # compile project
./cct run [--project DIR] [-- --args]    # build and run
./cct test [pattern] [--project DIR]     # run tests
./cct bench [pattern] [--project DIR]    # run benchmarks
./cct clean [--project DIR] [--all]      # clean artifacts
```

### Documentation

```bash
./cct doc [--project DIR]                          # generate markdown docs
./cct doc --format html [--project DIR]            # generate HTML docs
./cct doc --format both [--project DIR]            # generate both formats
./cct doc --include-internal [...]                 # include ARCANUM symbols
./cct doc --warn-missing-docs [...]                # warn on missing docs
./cct doc --strict-docs [...]                      # treat doc warnings as errors
./cct doc --no-timestamp [...]                     # deterministic output
```

### Global Options

```bash
--no-color                 # disable ANSI colors in diagnostics
--sigilo-style <style>     # sigil visual style (network|seal|scriptum)
--sigilo-mode <mode>       # sigil emission mode (essencial|completo)
--sigilo-out <path>        # custom sigil output path
--sigilo-no-meta           # skip metadata file
--sigilo-no-svg            # skip SVG file
```

---

## Exit Code Contract

| Command           | Success | Warnings/Mismatch | Errors |
|-------------------|---------|-------------------|--------|
| `cct compile`     | 0       | —                 | 1      |
| `cct --check`     | 0       | —                 | 1      |
| `cct fmt`         | 0       | —                 | 1      |
| `cct fmt --check` | 0       | 2 (unformatted)   | 1      |
| `cct lint`        | 0       | 1 (warnings)      | 1      |
| `cct lint --strict` | 0     | 2 (warnings)      | 1      |
| `cct build`       | 0       | —                 | 1      |
| `cct test`        | 0       | —                 | 1      |
| `cct doc`         | 0       | 0 (normal)        | 1      |
| `cct doc --strict-docs` | 0 | 2 (doc warnings)  | 1      |

---

## Canonical Stdlib Modules — FASE 12 State

| Module                  | Status        | Purpose                                      |
|-------------------------|---------------|----------------------------------------------|
| `cct/verbum`            | `stable`      | String operations (len, concat, substring)   |
| `cct/fmt`               | `stable`      | Formatting and conversion                    |
| `cct/series`            | `stable`      | Static array operations                      |
| `cct/alg`               | `stable`      | Baseline algorithms (search, sort)           |
| `cct/mem`               | `stable`      | Memory utilities (alloc, free, copy)         |
| `cct/fluxus`            | `stable`      | Dynamic vector                               |
| `cct/io`                | `stable`      | I/O operations (print, read_line)            |
| `cct/fs`                | `stable`      | Filesystem operations                        |
| `cct/path`              | `stable`      | Path manipulation                            |
| `cct/math`              | `stable`      | Math utilities (abs, min, max, clamp)        |
| `cct/random`            | `stable`      | Random number generation                     |
| `cct/parse`             | `stable`      | String parsing (parse_int, parse_real)       |
| `cct/cmp`               | `stable`      | Comparison utilities                         |
| `cct/option`            | `experimental`| Optional values (Some/None)                  |
| `cct/result`            | `experimental`| Result values (Ok/Err)                       |
| `cct/map`               | `experimental`| HashMap implementation                       |
| `cct/set`               | `experimental`| HashSet implementation                       |
| `cct/collection_ops`    | `experimental`| Functional collection operations             |

---

## What Is NOT In FASE 12

The following were explicitly **out of scope** for FASE 12:

### Language Features
- Advanced iterator protocol for custom types
- Lazy evaluation or stream processing
- Advanced pattern matching (beyond basic `ORDO` support)
- Type inference for generic parameters
- Trait/interface system beyond `PACTUM` baseline
- Async/await or concurrency primitives
- Foreign function interface (FFI)

### Tooling
- Package manager (local or remote)
- Package registry
- Dependency resolution from remote sources
- Plugin system
- Language server protocol (LSP) support
- Advanced refactoring tools
- Code coverage tools
- Profiler

### Build System
- Remote dependency fetching
- Complex build graph visualization
- Distributed compilation
- Cross-compilation targets

### Documentation
- Runtime documentation introspection
- Interactive documentation browser
- Doc testing (code examples in docs that run as tests)

### Standard Library
- Network I/O
- Thread/process management
- Regular expressions
- JSON/XML parsing
- Compression
- Cryptography

---

## Compatibility Guarantees

FASE 12 preserves full backward compatibility with:

1. **Legacy single-file workflow**: `./cct file.cct` continues to work
2. **Inspection commands**: `--tokens`, `--ast`, `--ast-composite`, `--check` unchanged
3. **Sigilo generation**: All modes (`essencial`/`completo`) remain deterministic
4. **Module system**: `ADVOCARE` resolution and visibility rules stable
5. **Advanced typing**: `GENUS`/`PACTUM` contracts unchanged

No breaking changes were introduced in FASE 12 to previously stable features.

---

## Recommended Daily Workflow

```bash
# Format code
./cct fmt src/main.cct lib/*.cct

# Lint code
./cct lint --strict src/main.cct

# Build project
./cct build

# Run tests
./cct test

# Generate documentation
./cct doc --format both --no-timestamp

# Build release
./cct build --project . --release
```

This workflow is **not enforced** by default, but represents best practices for FASE 12 projects.

---

## Distribution Bundle

FASE 12 includes a relocatable distribution bundle (`make dist`) with:

- `cct` binary
- Canonical stdlib modules (`cct/*.cct`)
- Essential documentation
- Canonical examples

The bundle uses wrapper-based stdlib resolution and is self-contained for local development.

---

## Transition to FASE 13

FASE 13 will build on this stable foundation. Known areas for future work:

1. **Tooling maturity**:
   - Promote linter to `stable` after rule refinement
   - Promote doc generator to `stable` after feature freeze

2. **Stdlib refinement**:
   - Promote `cct/option`, `cct/result` to `stable`
   - Refine `cct/map`, `cct/set` API based on usage feedback
   - Add more collection operations if needed

3. **Ecosystem**:
   - Package manager design (local first)
   - Registry exploration (future phases)

4. **Performance**:
   - Incremental compilation optimization
   - Build parallelization

5. **Developer experience**:
   - LSP server exploration
   - Editor integration improvements

---

## Conclusion

**FASE 12** represents the **structural maturity milestone** of CCT. The language now has:

- A complete developer toolchain
- Consistent CLI experience
- Stable stdlib foundation
- Project-based workflow support
- Documentation generation
- Quality gates (format/lint/test)

This phase establishes CCT as a **usable programming language** with a **complete development environment**, ready for real-world projects and further ecosystem growth.

**Status:** ✅ Frozen and canonized as of FASE 12H

---

**Document version:** 1.0
**Last updated:** March 2026
**Maintainer:** Erick Andrade Busato
