# CCT — Clavicula Turing

> "To name is to invoke. To invoke is to bind. To bind is to compute."

CCT is a compiled, ritual-themed programming language with deterministic sigil generation.

<div align="center">
  <img src="docs/example_sigil_ars_magna.svg" alt="CCT System Sigil Example - Ars Magna Omniversal" width="600"/>
  <p><em>System sigil generated from a multi-module CCT program (Ars Magna Omniversal)</em></p>
  <p><sub>Each program generates a unique, deterministic visual sigil representing its structure, calls, and module dependencies</sub></p>
</div>

## Status

**Current status: FASE 15D.4 completed** (FASE 15 closure gate completed; baseline is ready for FASE 16 work).

Implemented phases: **0 → 13D.4 + 13M.B2 + 14A.1/14A.2/14A.3/14A.4 + 15A.1/15A.2/15A.3/15A.4/15B.1/15B.2/15B.3/15B.4/15C.1/15C.2/15C.3/15C.4/15D.1/15D.2/15D.3/15D.4**.

Highlights of the current baseline:
- Real end-to-end compiler pipeline (`.cct -> parse/semantic -> codegen -> .cgen.c -> host C compiler -> binary`)
- Deterministic sigil generation (`.svg` + `.sigil`) integrated into normal compile and `--sigilo-only`
- Multi-module support with `ADVOCARE`, cycle detection, direct-import visibility rules, and internal visibility via `ARCANUM`
- Modular sigils with two official emission modes (`essencial` / `completo`, aliases `essential` / `complete`)
- Advanced typing subset consolidated: `GENUS`, `PACTUM`, and basic constraints `GENUS(T PACTUM C)`
- Bibliotheca Canonica foundation: reserved namespace `cct/...` with canonical stdlib resolution
- Canonical text-core module: `cct/verbum` (`len`, `concat`, `compare`, `substring`, `trim`, `contains`, `find`)
- Canonical formatting/conversion module: `cct/fmt` (`stringify_*`, `fmt_parse_*`, `format_pair`)
- Canonical static-collection module: `cct/series` (`series_len`, `series_fill`, `series_copy`, `series_reverse`, `series_contains`)
- Canonical baseline algorithms module: `cct/alg` (`alg_linear_search`, `alg_compare_arrays`, `alg_binary_search`, `alg_sort_insertion`)
- Canonical memory utility module: `cct/mem` (`alloc`, `free`, `realloc`, `copy`, `set`, `zero`, `mem_compare`)
- Canonical dynamic-vector module: `cct/fluxus` (`fluxus_init`, `fluxus_free`, `fluxus_push`, `fluxus_pop`, `fluxus_len`, `fluxus_get`, `fluxus_clear`, `fluxus_reserve`, `fluxus_capacity`)
- Canonical IO module: `cct/io` (`print`, `println`, `print_int`, `read_line`)
- Canonical filesystem module: `cct/fs` (`read_all`, `write_all`, `append_all`, `exists`, `size`)
- Canonical path module: `cct/path` (`path_join`, `path_basename`, `path_dirname`, `path_ext`)
- Canonical math module: `cct/math` (`abs`, `min`, `max`, `clamp`)
- Canonical random module: `cct/random` (`seed`, `random_int`, `random_real`)
- Canonical parse module: `cct/parse` (`parse_int`, `parse_real`, `parse_bool`)
- Canonical compare module: `cct/cmp` (`cmp_int`, `cmp_real`, `cmp_bool`, `cmp_verbum`)
- Canonical Option/Result modules: `cct/option` and `cct/result` (`Some`/`None`, `Ok`/`Err`, `unwrap`/`unwrap_or`/`expect`)
- Moderate canonical algorithm extras: `cct/alg` (`alg_binary_search`, `alg_sort_insertion`)
- Canonical public showcases for stdlib usage (`string`, `collection`, `io/fs`, `parse/math/random`, `multi-module`)
- Sigilo metadata now exposes stdlib usage counters and module list in showcase/public flows
- Final stdlib stability matrix and release notes are published for the 11H freeze
- Relocatable distribution bundle (`make dist`) with wrapper-based stdlib resolution
- Structured diagnostics with source snippets and actionable suggestions (FASE 12A)
- Numeric cast expression baseline (`cast GENUS(T)(value)`) in FASE 12B
- Functional error ergonomics via Option/Result in FASE 12C
- Hash-backed canonical collections via `cct/map` and `cct/set` in FASE 12D.1
- Functional collection combinators via `cct/collection_ops` in FASE 12D.2 (`fluxus_map`, `fluxus_filter`, `fluxus_fold`, `fluxus_find`, `fluxus_any`, `fluxus_all`, `series_map`, `series_filter`, `series_reduce`, `series_find`, `series_any`, `series_all`)
- Baseline collection iterator syntax in FASE 12D.3 (`ITERUM item IN collection COM ... FIN ITERUM`) for `FLUXUS`, `SERIES`, and collection-op results
- Standalone formatter command in FASE 12E.1 (`cct fmt`, `cct fmt --check`, `cct fmt --diff`)
- Canonical linter command in FASE 12E.2 (`cct lint`, `cct lint --strict`, `cct lint --fix`)
- Canonical project workflow in FASE 12F (`cct build`, `cct run`, `cct test`, `cct bench`, `cct clean`) with basic incremental cache
- Canonical documentation generator in FASE 12G (`cct doc`) for module/symbol API pages (markdown/html)
- Common math operators in FASE 13M: `**` (power), `//` (floor integer division), `%%` (euclidean modulo)
- FASE 14A hardening: canonical diagnostic taxonomy + canonical exit-code contract + sigilo explain mode + deterministic sigilo diagnostic ordering
- FASE 15 closure set: `FRANGE`/`RECEDE` loop-control stability, logical `ET`/`VEL` with precedence/parentheses, stable bitwise/shift operators, and `CONSTANS` semantic+codegen enforcement (locals, parameters, and const-pointer binding)

## Build

Requirements:
- C compiler (`gcc` or `clang`)
- `make`

Build:

```bash
make
```

Run full test suite:

```bash
make test
```

## CLI

Basic usage:

```bash
./cct [options] <file.cct>
```

Main commands:
- `./cct <file.cct>`: compile (and generate sigil artifacts)
- `./cct fmt <file.cct> [more.cct ...]`: format file(s) in place
- `./cct fmt --check <file.cct> [more.cct ...]`: check formatting only (exit `2` on mismatch)
- `./cct fmt --diff <file.cct> [more.cct ...]`: show formatting diff without writing
- `./cct lint <file.cct>`: run canonical lint rule set
- `./cct lint --strict <file.cct>`: treat lint warnings as CI failure (exit `2`)
- `./cct lint --fix <file.cct>`: apply safe automatic fixes
- `./cct build [--project DIR]`: build project using canonical structure
- `./cct run [--project DIR] [-- --args]`: build and run project binary
- `./cct test [pattern] [--project DIR]`: run `*.test.cct` project tests
- `./cct bench [pattern] [--project DIR]`: run `*.bench.cct` project benchmarks
- `./cct build|test|bench ... --sigilo-check [--sigilo-strict] [--sigilo-baseline PATH]`: opt-in sigilo baseline gate in project workflow
- `./cct build|test|bench ... --sigilo-ci-profile advisory|gated|release`: CI profile contract for progressive sigilo gates
- `./cct build|test|bench ... --sigilo-override-behavioral-risk`: explicit/audited override for behavioral-risk CI blocks
- `./cct build|test|bench ... --sigilo-report summary|detailed`: operational report verbosity for sigilo-check (default `summary`)
- `./cct build|test|bench ... --sigilo-explain`: actionable diagnosis line with probable cause and recommended next step
- `./cct clean [--project DIR] [--all]`: clean `.cct` artifacts (and `dist` with `--all`)
- `./cct doc [--project DIR] [--format markdown|html|both]`: generate API docs under `docs/api`
- `./cct --tokens <file.cct>`: token stream
- `./cct --ast <file.cct>`: single-module AST
- `./cct --ast-composite <entry.cct>`: composed AST for module closure
- `./cct --check <file.cct>`: syntax + semantic checks only
- `./cct --sigilo-only <file.cct>`: generate sigil artifacts without executable
- `./cct sigilo inspect <artifact.sigil>`: inspect sigilo metadata
- `./cct sigilo validate <artifact.sigil>`: run formal sigilo validation (tolerant/strict profiles)
- `./cct sigilo diff <left.sigil> <right.sigil>`: compare two sigilo artifacts
- `./cct sigilo check <left.sigil> <right.sigil> --strict --summary`: gate drift by severity
- `./cct sigilo inspect|diff|check ... --consumer-profile legacy-tolerant|current-default|strict-contract`: explicit consumer compatibility profile
- `./cct sigilo validate ... --consumer-profile legacy-tolerant|current-default|strict-contract`: explicit strict/tolerant validator profile
- `./cct sigilo baseline check <artifact.sigil> [--baseline PATH]`: compare artifact vs persisted baseline
- `./cct sigilo baseline update <artifact.sigil> [--baseline PATH] [--force]`: explicit baseline update
- `./cct --no-color ...`: disable ANSI colors in diagnostics

Sigil options:
- `--sigilo-style network|seal|scriptum`
- `--sigilo-mode essencial|completo` (aliases: `essential|complete`)
- `--sigilo-out <basepath>`
- `--sigilo-no-meta`
- `--sigilo-no-svg`

## What Is Implemented

### Core Language and Execution
- Lexer, parser, AST, semantic analysis, and executable code generation
- Structured flow control: `SI/ALITER`, `DUM`, `DONEC`, `REPETE`, `ITERUM`
- Calls and returns: `CONIURA`, `REDDE`, `ANUR`
- Scalars, booleans, strings, and real-number subset (`UMBRA`, `FLAMMA`)
- Basic arrays (`SERIES`) and practical enum subset (`ORDO`)

### Memory and Structural Subset (FASE 7 block)
- `SPECULUM` pointers (supported subset)
- Address-of, dereference read/write, and pass-by-reference patterns
- Runtime-backed allocation/discard primitives:
  - `OBSECRO pete(...)`
  - `OBSECRO libera(...)`
  - `DIMITTE`
  - `MENSURA(...)`
- Executable `SIGILLUM` subset with nested composition and controlled by-reference mutation

### Failure-Control Subset (FASE 8 block)
- `IACE`, `TEMPTA ... CAPE`, `SEMPER`
- Local catch and multi-call propagation subset
- Documented runtime-failure bridging subset and clear direct-abort behavior outside integrated paths

### Modules and Sigils (FASE 9 block)
- `ADVOCARE` module loading with deterministic closure
- Direct-import resolution (no implicit transitive symbol visibility)
- Visibility boundary with `ARCANUM` for internal top-level declarations
- Two-level sigil architecture:
  - local sigil per module
  - composed system sigil (`.system.svg` / `.system.sigil`)
- System sigil rendered as **sigil-of-sigils** (inline vector composition of module sigils)

### Advanced Typing (FASE 10 block)
- `GENUS(...)` generic declarations and explicit instantiation
- Pragmatic executable monomorphization (deterministic naming and dedup)
- `PACTUM` contract declarations and explicit `SIGILLUM ... PACTUM ...` conformance
- Basic constrained generics: `GENUS(T PACTUM C)` in generic `RITUALE`
- Final 10E consolidation: harmonized boundary diagnostics and finalized metadata contract

## Current Typing Contract (FASE 10E)

Supported in the final FASE 10 subset:
- Explicit generic instantiation (`GENUS(...)`) for executable materialization
- Explicit contract conformance (`PACTUM`) for named `SIGILLUM`
- Single-constraint form per type parameter in generic rituals: `GENUS(T PACTUM C)`

Out of scope in this subset:
- Type argument inference
- Multiple contracts per type parameter
- Advanced constraint solver behavior
- Dynamic dispatch runtime for contracts

## Sigil Artifacts

For a valid input program, CCT emits:
- `<base>.svg`
- `<base>.sigil`

For modular system sigils:
- `<entry>.system.svg`
- `<entry>.system.sigil`

In `--sigilo-mode completo`, imported module sigils are also emitted as deterministic module-indexed artifacts.

## Project Workflow (FASE 12F)

Canonical project layout:

```text
project/
├── src/main.cct
├── lib/
├── tests/*.test.cct
├── bench/*.bench.cct
└── cct.toml (optional)
```

Typical local flow:

```bash
./cct test --project .
./cct build --project .
./cct run --project .
./cct bench --project . --iterations 5
./cct clean --project . --all
./cct doc --project . --format both
```

Sigilo-focused local workflows (FASE 13B.1):
- minimal daily loop and strict pre-merge loop are consolidated in `docs/sigilo_operations_14b2.md`
- strict baseline gate uses:
  - `./cct sigilo baseline check <artifact.sigil> --strict --summary`
  - exit code `2` for blocking drift (`review-required` or `behavioral-risk`)

Sigilo-focused CI profiles (FASE 13B.3):
- profiles:
  - `advisory`: informative; blocks only `behavioral-risk` unless explicit override
  - `gated`: blocks `review-required` and `behavioral-risk`
  - `release`: strict profile; requires baseline and blocks `review-required` and `behavioral-risk`
- commands:
  - `./cct build --project . --sigilo-check --sigilo-ci-profile advisory`
  - `./cct test --project . --sigilo-check --sigilo-ci-profile gated`
  - `./cct build --project . --sigilo-check --sigilo-ci-profile release`
  - `./cct build --project . --sigilo-check --sigilo-ci-profile advisory --sigilo-override-behavioral-risk`
- operational contract reference: `docs/sigilo_operations_14b2.md`

Sigilo operational observability (FASE 13B.4):
- report signature: `format=cct.sigilo.report.v1`
- default output is summary-oriented and script-safe
- detailed output (`--sigilo-report detailed`) adds per-item `domain`, `before`, and `after`
- explain output (`--sigilo-explain`) adds probable cause + recommended action + troubleshooting doc reference
- troubleshooting playbook: `docs/sigilo_troubleshooting_13b4.md`

Sigilo consumer compatibility (FASE 13C.3):
- profiles:
  - `legacy-tolerant`: maximum compatibility for legacy readers
  - `current-default`: canonical default profile in FASE 13 tooling
  - `strict-contract`: blocking contract enforcement (`--strict` alias)
- migration and fallback behavior is covered in current operational guidance and validator profile docs

Sigilo strict/tolerant validation (FASE 13C.4):
- canonical validator command: `./cct sigilo validate <artifact.sigil> [--strict] [--consumer-profile ...]`
- tolerant profiles keep compatibility-first behavior with warning classification
- strict-contract profile blocks contractual violations for release gates

FASE 13 release package (FASE 13D.4):
- `docs/release/FASE_13_RELEASE_NOTES.md`

FASE 13M addendum package (FASE 13M.B2):
- details were consolidated into historical internal release records

## Common Math Operators (FASE 13M)

Stable additions:
- `**`: exponentiation (right-associative)
- `//`: integer floor division (integer operands only)
- `%%`: euclidean modulo (integer operands only)
- `%`: preserved with legacy behavior

Executable example:

```bash
./cct examples/math_common_ops_13m.cct
./examples/math_common_ops_13m
```

Expected output excerpt:
- `pow 2**5 = 32`
- `idiv -7//3 = -3`
- `emod -7%%3 = 2`

## Bibliotheca Canonica (Standard Library)

The standard library is now formally introduced as **Bibliotheca Canonica**.

Current delivery in FASE 11A:
- reserved import namespace `cct/...`
- canonical physical stdlib root (`lib/cct/`)
- deterministic resolver path for canonical modules

Current delivery in FASE 11B.1:
- `cct/verbum` public text primitives
- strict substring bounds behavior
- predictable text operations for later `cct/fmt`, IO, and parse modules

Current delivery in FASE 11B.2:
- `cct/fmt` formatting and conversion surface
- canonical stringify for integer/real/float/bool
- canonical parse façade (`fmt_parse_int`, `fmt_parse_real`, `fmt_parse_bool`)
- simple formatting composition (`format_pair`)

Current delivery in FASE 11C:
- `cct/series` static-collection helpers (generic mutation helpers + integer-lookup helper)
- `cct/alg` baseline algorithms for integer arrays
- practical interop with `cct/fmt` in collection workflows

Current delivery in FASE 11D.1:
- `cct/mem` memory utility primitives for allocation, release, resize, and raw buffer operations
- explicit ownership/discard contract for stdlib dynamic-storage evolution
- dedicated ownership reference: `docs/ownership_contract.md`

Current delivery in FASE 11D.2:
- standalone FLUXUS storage runtime core (`cct_rt_fluxus_init/free/reserve/grow/push/pop/get/clear`)
- deterministic growth/capacity semantics validated through dedicated runtime tests

Current delivery in FASE 11D.3:
- canonical stdlib module `cct/fluxus`
- ergonomic dynamic-vector API backed by runtime storage core
- dedicated 11D.3 integration tests and sigilo metadata counters for FLUXUS operations
- usage guide: `docs/fluxus_usage.md`

Current delivery in FASE 11E.1:
- canonical stdlib modules `cct/io` and `cct/fs`
- runtime-backed IO primitives (`print`, `println`, `print_int`, `read_line`)
- runtime-backed filesystem primitives (`read_all`, `write_all`, `append_all`, `exists`, `size`)
- dedicated 11E.1 integration tests and sigilo compatibility coverage

Current delivery in FASE 11E.2:
- canonical stdlib module `cct/path`
- stable path API for composition and decomposition (`path_join`, `path_basename`, `path_dirname`, `path_ext`)
- verified integration with `cct/fs` workflows
- sigilo metadata support for path usage counters

Current delivery in FASE 11F.1:
- canonical stdlib modules `cct/math` and `cct/random`
- deterministic numeric helpers (`abs`, `min`, `max`, `clamp`)
- reproducible pseudo-random baseline (`seed`, `random_int`, `random_real`)
- sigilo metadata support for math/random usage counters

Current delivery in FASE 11F.2:
- canonical stdlib modules `cct/parse` and `cct/cmp`
- strict textual conversions (`parse_int`, `parse_real`, `parse_bool`)
- canonical comparator contract (`cmp_int`, `cmp_real`, `cmp_bool`, `cmp_verbum`)
- moderate `cct/alg` expansion (`alg_binary_search`, `alg_sort_insertion`)
- sigilo metadata support for parse/cmp/alg usage counters

Current delivery in FASE 11G:
- canonical showcase suite under `examples/` and `tests/integration/`
- modular showcase exercising stdlib + user modules with `--ast-composite`
- sigilo metadata enrichment for stdlib usage counters and module inventory
- public-facing usage narrative aligned across README/spec/docs

Current delivery in FASE 11H:
- final stdlib subset manifest freeze (`docs/stdlib_subset_11h.md`)
- final stability matrix (stable/experimental/runtime-internal) (`docs/stdlib_stability_matrix_11h.md`)
- packaging/install closure (`make dist`, `make install`, `make uninstall`)
- public technical release notes (`docs/release/FASE_11_RELEASE_NOTES.md`)

Current delivery in FASE 12D.1:
- canonical stdlib modules `cct/option` and `cct/result`
- Option baseline (`Some`, `None`, `option_is_some`, `option_unwrap`, `option_unwrap_or`, `option_expect`, `option_free`)
- Result baseline (`Ok`, `Err`, `result_is_ok`, `result_unwrap`, `result_unwrap_or`, `result_unwrap_err`, `result_expect`, `result_free`)
- integration with FASE 12B numeric cast flow (`cast GENUS(T)(value)`)
- canonical HashMap baseline (`map_init`, `map_insert`, `map_get`, `map_remove`, `map_contains`, `map_len`, `map_is_empty`, `map_capacity`, `map_clear`, `map_reserve`, `map_free`)
- canonical Set baseline (`set_init`, `set_insert`, `set_remove`, `set_contains`, `set_len`, `set_is_empty`, `set_clear`, `set_free`)
- sigilo metadata counters for Option/Result and Map/Set usage

Current delivery in FASE 12D.2:
- canonical stdlib module `cct/collection_ops`
- functional combinators for FLUXUS and SERIES (`map/filter/fold/find/any/all`)
- callback bridge through rituale-pointer arguments in collection operations
- Option integration preserved via `fluxus_find`/`series_find`
- sigilo metadata counter for collection-ops usage (`collection_ops_count`)

Current delivery in FASE 12D.3:
- baseline iterator statement `ITERUM item IN collection COM ... FIN ITERUM`
- semantic/type checks for iterator collections (FLUXUS and SERIES subset)
- codegen lowering to deterministic C loops
- sigilo metadata counter for iterator usage (`iterum_count`)

Current delivery in FASE 12E.1:
- standalone formatter command integrated in CLI (`cct fmt`)
- check/diff formatter modes for CI/editor integration
- deterministic indentation and spacing normalization for core CCT syntax
- formatter coverage tests in `tests/formatter/` plus `tests/run_tests.sh`

Current delivery in FASE 12E.2:
- standalone linter command integrated in CLI (`cct lint`)
- canonical rule IDs: `unused-variable`, `unused-parameter`, `unused-import`, `dead-code-after-return`, `dead-code-after-throw`, `shadowing-local`
- strict lint mode (`--strict`) and safe auto-fix mode (`--fix`)
- dedicated lint documentation (`docs/linter.md`) and integration tests

Example import:

```cct
ADVOCARE "cct/stub_test.cct"
```

Reference: `docs/bibliotheca_canonica.md`.

## Canonical Showcases (FASE 11G)

Run canonical showcase programs:

```bash
./cct examples/showcase_stdlib_string_11g.cct && ./examples/showcase_stdlib_string_11g
./cct examples/showcase_stdlib_collection_11g.cct && ./examples/showcase_stdlib_collection_11g
./cct examples/showcase_stdlib_io_fs_11g.cct && ./examples/showcase_stdlib_io_fs_11g
./cct examples/showcase_stdlib_parse_math_random_11g.cct && ./examples/showcase_stdlib_parse_math_random_11g
./cct examples/showcase_stdlib_modular_11g_main.cct && ./examples/showcase_stdlib_modular_11g_main
```

Inspect modular composition and sigilo:

```bash
./cct --ast-composite examples/showcase_stdlib_modular_11g_main.cct
./cct --sigilo-only --sigilo-mode essencial examples/showcase_stdlib_modular_11g_main.cct
./cct --sigilo-only --sigilo-mode completo examples/showcase_stdlib_modular_11g_main.cct
```

## Packaging and Install (FASE 11H)

Build a relocatable distribution bundle:

```bash
make dist
./dist/cct/bin/cct --version
./dist/cct/bin/cct --check tests/integration/stdlib_resolution_basic_11a.cct
```

Install under default prefix (`/usr/local`):

```bash
make install
```

Install under custom prefix:

```bash
make install PREFIX="$HOME/.local"
```

References:
- `docs/install.md`
- `docs/stdlib_subset_11h.md`
- `docs/stdlib_stability_matrix_11h.md`
- `docs/release/FASE_11_RELEASE_NOTES.md`

## Quick Examples

Tokenize:

```bash
./cct --tokens examples/hello.cct
```

Semantic check:

```bash
./cct --check examples/hello.cct
```

Compile and run:

```bash
./cct examples/hello.cct
./examples/hello
```

Sigil-only (system + local in essential mode):

```bash
./cct --sigilo-only --sigilo-mode essencial tests/integration/sigilo_final_modular_entry.cct
```

## Repository Layout

- `src/`: compiler implementation
  - `lexer/`, `parser/`, `semantic/`, `codegen/`, `sigilo/`, `module/`, `runtime/`, `cli/`, `common/`
- `tests/`: integration and phase regression suite
- `examples/`: language examples
- `docs/`: specification, architecture, and roadmap
- `FASE_*_CCT.md`: phase planning/execution documents

## Release Documentation Packages

The current project baseline is **FASE 15D.4 completed**. Historical release packages remain available for traceability and migration references.

**Current-phase release documentation:**
- `docs/release/FASE_15_RELEASE_NOTES.md` — FASE 15 completion summary and compatibility notes

**Historical package documentation:**
- `docs/release/FASE_11_RELEASE_NOTES.md` — Early stdlib/platform release notes
- `docs/release/FASE_12_RELEASE_NOTES.md` — FASE 12 delivery notes
- `docs/release/FASE_13_RELEASE_NOTES.md` — Highlights and operational guidance
- `docs/release/FASE_14_RELEASE_NOTES.md` — Hardening-stream release notes
- detailed matrices/snapshots from older phases were archived from the public `docs/release` surface

**Quick reference:**
- FASE 0–14 public contracts remain stable
- FASE 15 closure set is complete (`FRANGE`, `RECEDE`, logical operators, bitwise operators, and `CONSTANS`)
- Bootstrap-oriented architecture work starts in FASE 16
- Zero silent-breaking-change policy remains active

See `docs/roadmap.md` and `docs/spec.md` for current-phase status and language-surface details.

## Documentation

CCT documentation is organized by audience and purpose. Choose your reading path:

### For New Users (Start Here)
1. This README (you're reading it!)
2. [Installation Guide](docs/install.md) - Setup and verification
3. [Spec - Sections 1-3, 12](docs/spec.md) - Basic syntax and examples
4. [Project Conventions](docs/project_conventions.md) - Code organization

**Estimated time**: 1 hour

### For Language Learners
1. [Language Specification](docs/spec.md) - Complete language reference
2. [Bibliotheca Canonica](docs/bibliotheca_canonica.md) - Standard library guide
3. [FLUXUS Usage](docs/fluxus_usage.md) - Dynamic vectors in depth
4. [Build System](docs/build_system.md) - Project workflow
5. Explore `examples/showcase_stdlib_*.cct` for real-world patterns

**Estimated time**: 4-6 hours

### Quick Reference (Keep Handy)
- [Spec - Sections 1, 4-11](docs/spec.md) - Language reference
- [Bibliotheca Canonica - Sections 12+](docs/bibliotheca_canonica.md) - Stdlib API
- [Linter Rules](docs/linter.md) - Lint rule reference
- [Doc Generator](docs/doc_generator.md) - Doc comment syntax

### For Advanced Users and Contributors
1. [Architecture](docs/architecture.md) - Compiler internals
2. [Roadmap](docs/roadmap.md) - Phase history and future plans
3. [Release Documentation (Historical Packages)](docs/release/):
   - [FASE 15 Release Notes](docs/release/FASE_15_RELEASE_NOTES.md) - FASE 15 closure summary and compatibility notes
   - [FASE 14 Release Notes](docs/release/FASE_14_RELEASE_NOTES.md) - Hardening-stream highlights
   - [FASE 13 Release Notes](docs/release/FASE_13_RELEASE_NOTES.md) - Highlights and migration guide
   - [FASE 12 Release Notes](docs/release/FASE_12_RELEASE_NOTES.md) - FASE 12 delivery summary
   - [FASE 11 Release Notes](docs/release/FASE_11_RELEASE_NOTES.md) - Early stdlib/platform notes

**Estimated time**: 3-4 hours

### Documentation Philosophy
- **spec.md**: Authoritative language reference (what is valid CCT)
- **architecture.md**: How the compiler works internally
- **bibliotheca_canonica.md**: Standard library concepts and APIs
- **roadmap.md**: Where we came from, where we're going
- **release/**: phase release notes and public-facing closure summaries

### All Documentation Files
Primary docs:
- `docs/spec.md`
- `docs/architecture.md`
- `docs/roadmap.md`
- `docs/bibliotheca_canonica.md`
- `docs/release/FASE_15_RELEASE_NOTES.md` — current phase release notes
- `docs/release/` — phase release-note index (11..15)

Tooling and guides:
- `docs/install.md`
- `docs/build_system.md`
- `docs/project_conventions.md`
- `docs/fluxus_usage.md`
- `docs/linter.md`
- `docs/doc_generator.md`
- `docs/sigilo_operations_14b2.md`

Project and phase dossiers:
- `PROJETO_CCT.md`
- `PROJETO_CCT_V2.md`
- `md_out/FASE_*_CCT.md` (phase execution plans and records, including the full FASE 15 track)

## License

MIT License - Copyright (c) 2026 Erick Andrade Busato

See [LICENSE](LICENSE) file for details.
