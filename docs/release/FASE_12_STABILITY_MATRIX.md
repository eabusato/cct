# FASE 12 — Stability Matrix

> **Official classification of component stability**
> **Updated:** March 2026 (FASE 12H freeze)

---

## Purpose

This document provides the **official stability classification** for all components delivered in FASE 12. It defines what users can depend on as stable API surface and what remains experimental or internal.

---

## Stability Levels

### `stable`
- **Guarantee:** Public API and behavior will not change in breaking ways without major version increment
- **Changes allowed:** Bug fixes, performance improvements, additive features (with deprecation cycle for removals)
- **Expectation:** Production-ready, suitable for long-term dependence

### `experimental`
- **Guarantee:** Functional and useful, but API may change based on feedback
- **Changes allowed:** API refinements, behavior adjustments, performance tuning
- **Expectation:** Usable for real projects, but expect minor migrations in future phases
- **Commitment:** No silent breakage—changes will be documented in release notes

### `internal`
- **Guarantee:** No API contract, can change at any time
- **Changes allowed:** Arbitrary refactoring, removal, replacement
- **Expectation:** Not for direct use by CCT programs; implementation detail only

---

## Component Classification

### Language Core

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| Lexer                    | Token stream                     | `stable` | 0–4   | Base consolidated                        |
| Parser                   | AST structure                    | `stable` | 0–4   | Core syntax frozen                       |
| Semantic Analysis        | Type checking + validation       | `stable` | 0–4   | Contract mature                          |
| Code Generator           | `.cgen.c` emission               | `stable` | 0–4   | Output format stable                     |
| Flow Control             | SI/ALITER, DUM, DONEC, REPETE    | `stable` | 1–3   | Syntax and semantics frozen              |
| Function Calls           | CONIURA, REDDE, ANUR             | `stable` | 2–3   | Contract consolidated                    |
| Data Types               | REX, UMBRA, FLAMMA, VERBUM       | `stable` | 1–4   | Core scalar types mature                 |
| Arrays                   | SERIES                           | `stable` | 4     | Fixed-size array type stable             |
| Enums                    | ORDO                             | `stable` | 5     | Enum subset frozen                       |
| Pointers                 | SPECULUM                         | `stable` | 7     | Pointer subset consolidated              |
| Allocation               | OBSECRO pete/libera, DIMITTE     | `stable` | 7     | Memory primitives mature                 |
| Structs                  | SIGILLUM                         | `stable` | 7     | Struct subset frozen                     |
| Error Handling           | IACE, TEMPTA/CAPE, SEMPER        | `stable` | 8     | Failure-control contract stable          |
| Numeric Casts            | cast GENUS(T)(expr)              | `stable` | 12B   | Controlled cast syntax frozen            |
| Iterator Syntax          | ITERUM ... IN ... COM            | `experimental` | 12D.3 | Syntax stable, protocol limited     |

---

### Module System

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| Module Loading           | ADVOCARE                         | `stable` | 9     | Resolution rules frozen                  |
| Module Closure           | Transitive dependency graph      | `stable` | 9     | Deterministic closure contract           |
| Visibility Rules         | Direct-import only               | `stable` | 9     | No implicit transitive visibility        |
| Internal Visibility      | ARCANUM                          | `stable` | 9     | Top-level internal marker frozen         |
| Cycle Detection          | Import graph validation          | `stable` | 9     | Contract consolidated                    |

---

### Sigilo System

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| Local Sigils             | Per-module `.svg`/`.sigil`       | `stable` | 9     | Deterministic generation                 |
| System Sigils            | Composed `.system.svg`           | `stable` | 9     | Sigil-of-sigils architecture frozen      |
| Emission Modes           | essencial / completo             | `stable` | 10    | Two-mode contract stable                 |
| Sigil Styles             | network / seal / scriptum        | `stable` | 9     | Visual styles frozen                     |
| Metadata Files           | `.sigil` JSON format             | `stable` | 9     | Metadata contract stable                 |
| CLI Flags                | --sigilo-* options               | `stable` | 9     | Flag contract consolidated               |

---

### Advanced Typing

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| Generics                 | GENUS(...) declarations          | `stable` | 10    | Explicit instantiation contract frozen   |
| Explicit Instantiation   | GENUS(Concrete)                  | `stable` | 10    | Monomorphization deterministic           |
| Contracts                | PACTUM declarations              | `stable` | 10    | Contract declaration syntax frozen       |
| Conformance              | SIGILLUM ... PACTUM ...          | `stable` | 10    | Explicit conformance frozen              |
| Constrained Generics     | GENUS(T PACTUM C)                | `stable` | 10    | Single-constraint form stable            |

---

### Diagnostics (FASE 12A)

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| Error Messages           | Structured diagnostic format     | `stable` | 12A   | Format contract mature                   |
| Source Snippets          | Context-aware code display       | `stable` | 12A   | Snippet rendering stable                 |
| Suggestions              | Actionable fix hints             | `stable` | 12A   | Suggestion format frozen                 |
| Position Info            | Line/column precision            | `stable` | 12A   | Position contract stable                 |
| Color Support            | ANSI color codes + --no-color    | `stable` | 12A   | Color/plain mode contract stable         |

---

### Standard Library

| Module                   | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `cct/verbum`             | String operations                | `stable` | 11    | Core string API frozen                   |
| `cct/fmt`                | Formatting and conversion        | `stable` | 11    | Format API stable                        |
| `cct/series`             | Static array operations          | `stable` | 11    | Array helper API stable                  |
| `cct/alg`                | Baseline algorithms              | `stable` | 11    | Core algorithm API frozen                |
| `cct/mem`                | Memory utilities                 | `stable` | 11    | Memory API stable                        |
| `cct/fluxus`             | Dynamic vector                   | `stable` | 11    | Vector API frozen                        |
| `cct/io`                 | I/O operations                   | `stable` | 11    | I/O API stable                           |
| `cct/fs`                 | Filesystem operations            | `stable` | 11    | Filesystem API frozen                    |
| `cct/path`               | Path manipulation                | `stable` | 11    | Path API stable                          |
| `cct/math`               | Math utilities                   | `stable` | 11    | Math API frozen                          |
| `cct/random`             | Random number generation         | `stable` | 11    | RNG API stable                           |
| `cct/parse`              | String parsing                   | `stable` | 11    | Parse API frozen                         |
| `cct/cmp`                | Comparison utilities             | `stable` | 11    | Compare API stable                       |
| `cct/option`             | Optional values (Some/None)      | `experimental` | 12C | Core API functional, minor refinements possible |
| `cct/result`             | Result values (Ok/Err)           | `experimental` | 12C | Core API functional, minor refinements possible |
| `cct/map`                | HashMap implementation           | `experimental` | 12D.1 | API may be refined for ergonomics      |
| `cct/set`                | HashSet implementation           | `experimental` | 12D.1 | API may be refined for ergonomics      |
| `cct/collection_ops`     | Functional collection operations | `experimental` | 12D.2 | Combinator set may expand              |

---

### Tooling — Formatter (FASE 12E.1)

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `cct fmt` Command        | Format files in place            | `stable` | 12E.1 | Command contract frozen                  |
| `--check` Flag           | Verify formatting without write  | `stable` | 12E.1 | Exit code contract stable (0/2)          |
| `--diff` Flag            | Show diff without write          | `stable` | 12E.1 | Diff output format stable                |
| Idempotency              | Repeat formatting = no change    | `stable` | 12E.1 | Guaranteed invariant                     |
| Semantic Preservation    | Formatting preserves behavior    | `stable` | 12E.1 | No semantic changes allowed              |
| Formatting Rules         | Indent, spacing, alignment       | `stable` | 12E.1 | Rules consolidated and frozen            |

---

### Tooling — Linter (FASE 12E.2)

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `cct lint` Command       | Run lint rules                   | `experimental` | 12E.2 | Command functional, rule set may expand |
| `--strict` Flag          | Treat warnings as errors         | `stable` | 12E.2 | Exit code contract stable (0/2)          |
| `--fix` Flag             | Apply safe automatic fixes       | `experimental` | 12E.2 | Fix set may expand                      |
| `unused-variable` Rule   | Detect unused local vars         | `stable` | 12E.2 | Rule definition frozen                   |
| `unused-parameter` Rule  | Detect unused function params    | `stable` | 12E.2 | Rule definition frozen                   |
| `unused-import` Rule     | Detect unused imports            | `stable` | 12E.2 | Rule definition frozen                   |
| `dead-code-*` Rules      | Detect unreachable code          | `stable` | 12E.2 | Rule definition frozen                   |
| `shadowing-local` Rule   | Detect local variable shadowing  | `stable` | 12E.2 | Rule definition frozen                   |

**Stability note for linter:**
- Core rule set is `stable` and will not change behavior
- Linter command itself is `experimental` because new rules may be added
- Existing rules will not be removed or change detection logic
- `--fix` is `experimental` because safe fix coverage may expand

---

### Tooling — Build System (FASE 12F)

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `cct build` Command      | Compile project                  | `stable` | 12F   | Command contract frozen                  |
| `cct run` Command        | Build and execute                | `stable` | 12F   | Command contract frozen                  |
| `cct test` Command       | Run `*.test.cct` files           | `stable` | 12F   | Test discovery contract stable           |
| `cct bench` Command      | Run `*.bench.cct` files          | `stable` | 12F   | Benchmark discovery contract stable      |
| `cct clean` Command      | Remove build artifacts           | `stable` | 12F   | Clean contract stable                    |
| Project Structure        | `src/`, `lib/`, `tests/`, `bench/` | `stable` | 12F | Directory conventions frozen           |
| Entry Point              | `src/main.cct` default           | `stable` | 12F   | Entry resolution stable                  |
| `cct.toml` Format        | Basic project configuration      | `stable` | 12F   | Baseline config format stable            |
| Incremental Cache        | `.cct/cache/` internal storage   | `stable` | 12F   | Cache invalidation contract stable       |
| `--project` Flag         | Specify project root             | `stable` | 12F   | Flag contract frozen                     |
| `--release` Flag         | Optimization mode (future)       | `experimental` | 12F | Placeholder for future optimization    |

---

### Tooling — Documentation Generator (FASE 12G)

| Component                | Surface                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `cct doc` Command        | Generate API docs                | `experimental` | 12G | Command functional, features may expand |
| `--format` Flag          | markdown / html / both           | `stable` | 12G   | Format options frozen                    |
| `--include-internal`     | Toggle ARCANUM visibility        | `stable` | 12G   | Visibility flag contract stable          |
| `--warn-missing-docs`    | Emit doc warnings                | `stable` | 12G   | Warning flag contract stable             |
| `--strict-docs`          | Treat warnings as errors         | `stable` | 12G   | Exit code contract stable (0/2)          |
| `--no-timestamp`         | Deterministic output             | `stable` | 12G   | Determinism contract frozen              |
| Doc Comment Format       | `///` triple-slash comments      | `stable` | 12G   | Comment syntax frozen                    |
| `@param` Tag             | Parameter documentation          | `stable` | 12G   | Tag syntax frozen                        |
| `@return` Tag            | Return value documentation       | `stable` | 12G   | Tag syntax frozen                        |
| `@example` Tag           | Example code documentation       | `stable` | 12G   | Tag syntax frozen                        |
| Output Directory         | `docs/api/` default location     | `stable` | 12G   | Output path contract stable              |

**Stability note for doc generator:**
- Core command and output formats are functional
- Marked `experimental` because tag set and rendering options may expand
- No breaking changes expected to existing tags or output structure

---

### CLI Global Flags

| Flag                     | Purpose                          | Status   | Since | Notes                                    |
|--------------------------|----------------------------------|----------|-------|------------------------------------------|
| `--tokens`               | Show token stream                | `stable` | 0     | Debug inspection stable                  |
| `--ast`                  | Show single-module AST           | `stable` | 0     | Debug inspection stable                  |
| `--ast-composite`        | Show multi-module AST            | `stable` | 9     | Debug inspection stable                  |
| `--check`                | Syntax + semantic checks only    | `stable` | 1     | Check-only mode stable                   |
| `--sigilo-only`          | Generate sigil without binary    | `stable` | 9     | Sigil-only mode stable                   |
| `--no-color`             | Disable ANSI colors              | `stable` | 12A   | Color flag stable                        |
| `--sigilo-style`         | Sigil visual style               | `stable` | 9     | Style flag stable                        |
| `--sigilo-mode`          | Sigil emission mode              | `stable` | 10    | Mode flag stable                         |
| `--sigilo-out`           | Custom sigil output path         | `stable` | 9     | Output flag stable                       |
| `--sigilo-no-meta`       | Skip metadata file               | `stable` | 9     | Metadata flag stable                     |
| `--sigilo-no-svg`        | Skip SVG file                    | `stable` | 9     | SVG flag stable                          |

---

### Exit Codes

| Exit Code | Meaning                          | Status   | Since | Notes                                    |
|-----------|----------------------------------|----------|-------|------------------------------------------|
| `0`       | Success                          | `stable` | 0     | Universal success code                   |
| `1`       | Error (compile/runtime/general)  | `stable` | 0     | Universal error code                     |
| `2`       | Warnings/mismatch in strict mode | `stable` | 12E   | Used by `--check`, `--strict`, etc.      |

---

### Internal Components

| Component                | Purpose                          | Status   | Notes                                    |
|--------------------------|----------------------------------|----------|------------------------------------------|
| C Runtime Helpers        | Internal support code            | `internal` | Not exposed to CCT programs            |
| `.cct/` Artifact Storage | Build cache and internals        | `internal` | Directory structure may change         |
| `.cgen.c` Format Details | Generated C code structure       | `internal` | Output format is implementation detail |
| Hash Algorithms          | Internal sigil/cache hashing     | `internal` | Algorithm may change                   |

---

## Interpretation Guide

### For CCT Users

- **Depend on `stable`:** These APIs and behaviors will not break in future phases
- **Use `experimental` with caution:** Functional and useful, but expect minor API migrations
- **Avoid `internal`:** Not guaranteed to exist or work the same way across versions

### For the Maintainer

- **`stable` requires discipline:** Changes must be additive or go through deprecation cycle
- **`experimental` allows iteration:** Can refine based on real-world feedback
- **`internal` is flexible:** Can change freely without documentation

---

## Version Contract

This stability matrix applies to **FASE 12** and will be updated in future phases:

- **FASE 13:** May promote `experimental` → `stable`, add new `experimental` features
- **FASE 14+:** May introduce new stability tiers if needed (e.g., `deprecated`)

---

## Summary Statistics

| Stability Level | Component Count |
|-----------------|-----------------|
| `stable`        | 67              |
| `experimental`  | 15              |
| `internal`      | 4               |

**Total:** 86 classified components

---

**Document version:** 1.0
**Last updated:** March 2026 (FASE 12H)
**Maintainer:** Erick Andrade Busato
