# CCT Compiler Architecture

This document defines the current technical architecture of CCT and organizes it by phase evolution.

## Scope

CCT is a compiled language toolchain that transforms `.cct` sources into:
- executable binaries (via generated C + host C compiler)
- deterministic sigil artifacts (`.svg` + `.sigil`)

This architecture document preserves the detailed host-compiler architecture that was stabilized through **FASE 20**, and extends it with the validated bootstrap, self-hosting, and operational-platform architecture delivered in **FASE 21 through FASE 30**.

## Current Architecture (Host Baseline Through FASE 20, Extended Through FASE 30)

### End-to-End Pipeline

```text
.cct source(s)
  -> module closure/resolution (ADVOCARE)
  -> lexer
  -> parser + AST
  -> semantic analysis
  -> sigilo generation (local/system)
  -> C code generation (.cgen.c)
  -> host C compiler (cc/gcc/clang)
  -> executable
```

### Core Architectural Decisions

- **Backend remains C-hosted**: `.cgen.c` + host C compiler is the official backend.
- **Determinism-first** for sigilo and generic materialization naming.
- **Metadata-first sigilo model**: structural counters and status fields are first-class outputs.
- **Single-entry module closure** with deterministic import graph traversal.
- **Incremental phase evolution**: no large redesign between phases unless explicitly planned.
- **Project workflow layer**: canonical `build/run/test/bench/clean` orchestration over the same compiler backend.
- **Documentation layer**: canonical `cct doc` API generation over project/module closure.

## Subsystem Map

### `src/common/`
Purpose: shared types and error infrastructure.

Main files:
- `types.h`
- `errors.h`
- `errors.c`
- `diagnostic.h`
- `diagnostic.c`
- `fuzzy.h`
- `fuzzy.c`

Status: stable foundation with 12A diagnostic infrastructure (location snippets + suggestions).

### `src/cli/`
Purpose: command parsing and user-facing compiler interface.

Main files:
- `cli.h`
- `cli.c`

Status: supports compile/check/ast/tokens/sigilo commands, formatter/linter, and 12F project workflow commands.

### `src/project/`
Purpose: canonical project discovery/cache/runner orchestration (`build/run/test/bench/clean`).

Main files:
- `project.h`
- `project.c`
- `project_discovery.h`
- `project_discovery.c`
- `project_cache.h`
- `project_cache.c`
- `project_runner.h`
- `project_runner.c`

Status: introduced in 12F with deterministic root discovery, basic incremental cache, and canonical project command execution.

### `src/doc/`
Purpose: API documentation generation (`cct doc`) with markdown/html output.

Main files:
- `doc.h`
- `doc.c`

Status: introduced in 12G with deterministic module/symbol page generation and strict warning gate mode.

### `src/module/`
Purpose: module discovery, graph closure, symbol visibility boundaries, inter-module resolution.

Main files:
- `module.h`
- `module.c`

Status: stable through 9A/9B/9C, integrated with 9D/9E sigilo system behavior and 10C/10D contract/constraint visibility checks, extended in 11A with canonical stdlib namespace resolution (`cct/...`).

### `src/lexer/`
Purpose: lexical analysis.

Main files:
- `lexer.h`
- `lexer.c`
- `keywords.h`

Status: complete for current language surface.

### `src/parser/`
Purpose: recursive-descent parsing and AST construction.

Main files:
- `parser.h`
- `parser.c`
- `ast.h`
- `ast.c`

Status: supports current syntax through 19D.4, including `ELIGE` (`CUM` legacy alias), `FORMA`, payload `ORDO`, and expanded `ITERUM` arity forms in supported contexts.

### `src/semantic/`
Purpose: symbol registration, scope checks, type checks, contract/constraint validation.

Main files:
- `semantic.h`
- `semantic.c`

Status: consolidated through 19D.4 for the defined subset, including ORDO exhaustiveness diagnostics and ITERUM arity validation by collection kind.

### `src/codegen/`
Purpose: executable code generation and generic materialization.

Main files:
- `codegen.h`
- `codegen.c`
- `codegen_internal.h`
- `codegen_contract.c`
- `codegen_runtime_bridge.c`

Status: pragmatic executable subset with deterministic monomorphization and dedup behavior.

### `src/runtime/`
Purpose: generated-code runtime helpers and low-level support paths.

Main files:
- `runtime.h`
- `runtime.c`

Status: minimal but functional runtime layer for current executable subset.

### `lib/cct/`
Purpose: physical root of Bibliotheca Canonica modules distributed with the compiler.

Status: introduced in 11A as foundation (namespace/resolution contract + test stub module), expanded in later 11.x subphases.

### `src/sigilo/`
Purpose: deterministic local and system sigilo generation.

Main files:
- `sigilo.h`
- `sigilo.c`

Status: mature through 9D/9D2/9E and extended in FASE 14T with source-context extraction, tooltip normalization, conditional SVG wrapper/title emission, deterministic local node/call-edge `data-*`, and root-level semantic toggles.

## Phase Evolution (Completed)

### Foundation and Core Compiler
- **FASE 0**: project and build foundation
- **FASE 1**: lexer
- **FASE 2A/2B**: parser + AST hardening
- **FASE 3**: semantic core
- **FASE 4A/4B/4C**: executable codegen subset establishment
- **FASE 5/5B/6A**: sigilo v1 -> v2 -> style/metadata refinement
- **FASE 6B/6C/6D**: executable language expansion + backend/runtime hardening

### Memory, Structures, and Failure Control
- **FASE 7A/7B/7C/7D**: pointer/memory subset + advanced `SIGILLUM` subset + phase-7 consolidation
- **FASE 8A/8B/8C/8D**: failure-control model (`IACE`, `TEMPTA/CAPE/SEMPER`) and final consolidation

### Modular System and Composite Sigilo
- **FASE 9A**: module discovery (`ADVOCARE`)
- **FASE 9B**: inter-module resolution and linking policy
- **FASE 9C**: visibility boundary (`ARCANUM` internal)
- **FASE 9D**: official local + system sigilo model with essential/complete modes
- **FASE 9D2**: system sigilo as vector inline sigil-of-sigils composition
- **FASE 9E**: modular architecture closure and CLI contract stabilization

### Advanced Typing System (Current Closure)
- **FASE 10A**: `GENUS` core model
- **FASE 10B**: executable monomorphization and dedup
- **FASE 10C**: `PACTUM` semantic conformance
- **FASE 10D**: `GENUS + PACTUM` constraints (`GENUS(T PACTUM C)`) in supported scope
- **FASE 10E**: final consolidation, harmonized diagnostics, frozen subset contract

### Bibliotheca Canonica Foundation
- **FASE 11A**: reserved namespace `cct/...`, canonical stdlib path resolution, distribution contract, and stdlib/runtime/builtin boundary documentation
- **FASE 11B.1 / 11B.2**: canonical text and formatting modules (`cct/verbum`, `cct/fmt`)
- **FASE 11C**: canonical static collections and baseline algorithms (`cct/series`, `cct/alg`)
- **FASE 11D.1 / 11D.2 / 11D.3**: canonical memory and dynamic-vector stack (`cct/mem`, runtime storage core, `cct/fluxus`)
- **FASE 11E.1 / 11E.2**: canonical IO/filesystem/path stack (`cct/io`, `cct/fs`, `cct/path`)
- **FASE 11F.1**: canonical numeric/random baseline (`cct/math`, `cct/random`)
- **FASE 11F.2**: canonical parse/compare layer and moderate algorithm expansion (`cct/parse`, `cct/cmp`, `alg_binary_search`, `alg_sort_insertion`)
- **FASE 11G**: canonical showcase/public face consolidation + stdlib usage metadata integration in sigilo
- **FASE 11H**: final stdlib subset freeze, stability governance, packaging/install closure

### FASE 12 Incremental Additions
- **FASE 12A**: structured diagnostics with source snippets, optional correction hints, typo fuzzy matching, and `--no-color` CLI switch
- **FASE 12B**: explicit numeric cast baseline (`cast GENUS(T)(expr)`) for executable subset
- **FASE 12C**: canonical `Option`/`Result` pointer-backed baseline in stdlib/runtime bridge
- **FASE 12D.1**: canonical hash-backed containers (`cct/map`, `cct/set`) integrated with Option and sigilo counters
- **FASE 12D.2**: canonical collection combinators (`cct/collection_ops`) for FLUXUS/SERIES map/filter/fold/find/any/all with callback-pointer bridge
- **FASE 12D.3**: iterator statement baseline (`ITERUM item IN collection COM ... FIN ITERUM`) over FLUXUS/SERIES and collection-op outputs
- **FASE 12E.1**: standalone formatter command (`cct fmt`) with in-place, check, and diff modes
- **FASE 12E.2**: canonical linter command (`cct lint`) with stable rule IDs, strict mode, and safe-fix subset

### FASE 13 Sigilo Tooling Expansion (Implemented Through 13D.4)
- **FASE 13A.1**: canonical `.sigil` parser/reader runtime suite (`test_sigil_parse`)
- **FASE 13A.2**: structural diff engine runtime suite (`test_sigil_diff`)
- **FASE 13A.3**: `sigilo inspect|diff|check` operational CLI contract
- **FASE 13A.4**: baseline check/update contract for local and CI workflows
- **FASE 13B.1/13B.2**: local + project workflow opt-in integration (`build|test|bench --sigilo-check`)
- **FASE 13B.3/13B.4**: CI profile gates (`advisory|gated|release`) and operator-facing report/explain outputs
- **FASE 13C.1–13C.4**: schema governance, compatibility profiles, and strict/tolerant validator contract (`sigilo validate`)
- **FASE 13D.1**: dedicated regression matrix runner:
  - `tests/run_phase13_regression.sh`
  - canonical fixture tree `tests/integration/phase13_regression_13d1/`
  - integrated in the global runner (`tests/run_tests.sh`)
- **FASE 13D.2**: determinism audit runner and release audit evidence:
  - `tests/run_phase13_determinism_audit.sh`
- **FASE 13D.3**: release-document consolidation package:
  - snapshot + stability/compatibility/limits/release-notes documents under `docs/release/`
- **FASE 13D.4**: final closure gate and residual-risk register for phase exit governance

### FASE 13M Common Math Operators Addendum (Implemented Through 13M.B2)
- **FASE 13M.A1**: scope freeze and semantic contract (`**`, `//`, `%%`; `^` deferred)
- **FASE 13M.A2**: compiler implementation in lexer/parser/semantic/codegen/runtime bridge
- **FASE 13M.B1**: deep test matrix and non-regression proof integrated in `tests/run_tests.sh`
- **FASE 13M.B2**: documentation/release closure with executable examples and phase closure gate

### FASE 14A Release Hardening Block (Implemented Through 14A.4)
- **FASE 14A.1**: canonical diagnostic taxonomy (`error|warning|note|hint`) with compatibility preservation (`suggestion:` + `hint:`)
- **FASE 14A.2**: canonical exit-code contract harmonized across CLI and strict gates (`0|1|2|3|4`)
- **FASE 14A.3**: explain/troubleshooting mode (`--explain`) on sigilo operational commands with actionable guidance
- **FASE 14A.4**: deterministic sigilo diagnostic ordering and log-noise hygiene for repeatable CI outputs

### FASE 14B Documentation Hardening (In Progress)
- **FASE 14B.1**: public contract harmonization across `README`, `spec`, architecture, roadmap, and CLI status strings
- **FASE 14B.2**: consolidated sigilo operations guide for high-value workflows:
  - `docs/sigilo_operations_14b2.md`
- **FASE 14B.3**: publication visibility policy (`md_out` private vs curated `docs` public) with manifests:
- **FASE 14B.4**: release-doc reference pack templates:

### FASE 14C Technical Audit Hardening (Implemented)
- **FASE 14C.1**: expanded regression matrix with composed risk blocks:
  - `tests/run_phase14c1_regression_matrix.sh`
- **FASE 14C.2**: stress/soak operational stability gate:
  - `tests/run_phase14c2_stress_soak.sh`
- **FASE 14C.3**: performance baseline/budget gate:
  - `tests/run_phase14c3_perf_budget.sh`
- **FASE 14C.4**: residual risk + known limits hardening:
  - public summary in `docs/release/FASE_14_RELEASE_NOTES.md`

### FASE 14T Sigilo SVG Instrumentation (Implemented)
- source-context extraction normalizes LF/CRLF input and indexes semantic elements by original source location
- tooltip builder prepares deterministic, XML-safe, clipped hover text for ritual nodes, structural nodes, local edges, and system nodes/edges
- local SVG emission gained conditional wrappers for hoverable elements plus additive deterministic `data-*` on local nodes and call edges
- local/system roots gained lightweight semantic metadata (`role`, `aria-label`, `desc`) when instrumentation is enabled
- CLI controls `--sigilo-no-titles` and `--sigilo-no-data` allow explicit generation of partially instrumented or plain pre-14T SVGs without changing the underlying layout engine
- practical effect: the rendered SVG itself became an inspection surface, where circles and lines can be hovered directly to reveal semantic context
  - detailed governance artifacts in internal release records

### FASE 14D Release Finalization Block (Implemented)
- **FASE 14D.1**: reproducible packaging gate:
  - `tests/run_phase14d1_packaging_repro.sh`
- **FASE 14D.2**: release candidate validation matrix:
  - `tests/run_phase14d2_rc_validation.sh`
- **FASE 14D.3**: final release artifact consolidation:
  - `docs/release/FASE_14_RELEASE_NOTES.md`
  - consolidated internal release artifacts under private governance records
- **FASE 14D.4**: closure gate and rollback:

## Current Architectural Contract (Post-19D4)

### Stable and Official
- C-hosted backend is official.
- Sigilo architecture is dual-level and stable:
  - local sigilo per module
  - system sigilo (`.system`) as composed sigil-of-sigils
- Generic execution requires explicit instantiation.
- Contract conformance is explicit (`SIGILLUM ... PACTUM ...`).
- Constraint form is intentionally limited to the stabilized subset.
- Bibliotheca Canonica foundational contract is active:
  - reserved namespace `cct/...`
  - canonical stdlib path resolution
  - stdlib modules remain observable in `--ast-composite` and `--check`
- Bibliotheca Canonica operational baseline now includes:
  - text/format layer (`verbum`, `fmt`)
  - collection/memory layer (`series`, `alg`, `mem`, `fluxus`, `collection_ops`, `map`, `set`)
  - external interaction layer (`io`, `fs`, `path`)
  - utility baseline (`math`, `random`, `parse`, `cmp`)
- IO/FS/PATH boundaries are explicit:
  - `io`: terminal input/output primitives
  - `fs`: whole-file operations
  - `path`: textual path composition/decomposition (no mini-OS scope)
- MATH/RANDOM boundaries are explicit:
  - `math`: minimal deterministic numeric helpers
  - `random`: runtime-backed PRNG baseline (`seed`, `random_int`, `random_real`)
- PARSE/CMP boundaries are explicit:
  - `parse`: strict textual conversion for primitive values
  - `cmp`: canonical comparator contract (`<0`, `0`, `>0`) across core scalar/text domains
- Option/Result + hash-backed collection boundaries are explicit:
  - `option` / `result`: pointer-backed value-absence and success/failure wrappers
  - `map` / `set`: hash-backed opaque containers with explicit lifecycle (`*_free`)
- Public showcase layer is now part of platform architecture:
  - curated canonical examples in `examples/showcase_stdlib_*_11g.cct`
  - mirrored integration fixtures in `tests/integration/showcase_*_11g.cct`
  - multi-module showcase validated with `--ast-composite` and modular sigilo modes
- Sigilo metadata now carries explicit stdlib usage inventory/counters used by public showcase and module workflows
- Release packaging contract is now defined:
  - `make dist` builds relocatable bundle (`bin`, `lib/cct`, `docs`, `examples`)
  - wrapper binary can inject stdlib root through `CCT_STDLIB_DIR`
  - `make install` / `make uninstall` are prefix-driven and install wrapper + stdlib tree

### Intentionally Out of Scope (as of 10E)
- Type argument inference
- Multi-contract constraints per type parameter
- Advanced constraint solver and dynamic contract dispatch
- Full ownership/lifetime system

## Phase Architecture Trajectory (Completed and Planned)

### FASE 11 — Standard Library and Ecosystem Base
Architecture direction:
- introduce a coherent stdlib surface over the stabilized language core
- define packaging/import conventions for reusable modules
- keep compiler core contracts stable while expanding library ergonomics

### FASE 12 — Backend/Runtime Evolution and Optimization
Architecture direction:
- performance and codegen/runtime quality improvements
- possible backend specialization paths while preserving public compiler behavior
- stronger runtime structure without breaking existing subset guarantees

### FASE 13 — Sigilo and Tooling Expansion
Architecture direction:
- richer tooling around sigilo analysis and workflows
- persisted, versioned baseline contract for sigilo drift checks
- explicit schema governance for `.sigil` evolution (13C.1), including additive-only policy in FASE 13
- canonical local workflow profiles (minimal/strict) to operationalize sigilo checks without coupling to CI gates
- optional visual/metadata enhancements while preserving deterministic model
- no regression of local/system sigilo contracts

### FASE 14 — v0.x Release Hardening
Architecture direction:
- release engineering, diagnostics polish, compatibility audits
- documentation and operational consistency across compiler and tooling

### FASE 15 — Language and Semantic Surface Consolidation (Completed)
Architecture direction:
- close the loop-control and operator gap in the executable subset with full regression coverage
- preserve deterministic and compatibility-first behavior while expanding semantics

Implemented closure set:
- 15A.1/15A.2/15A.3/15A.4: `FRANGE` and `RECEDE` stabilized across `DUM`, `DONEC`, `REPETE`, and `ITERUM`, including outside-loop diagnostics and REPETE increment safety
- 15B.1/15B.2/15B.3/15B.4: logical `ET`/`VEL` finalized with short-circuit guarantees, precedence (`NON > ET > VEL`), parentheses stress support, and comparator integration
- 15C.1/15C.2/15C.3/15C.4: bitwise and shift family finalized (`ET_BIT`, `VEL_BIT`, `XOR`, `NON_BIT`, `SINISTER`, `DEXTER`) with integer-only enforcement and closure integration checks
- 15D.1/15D.2/15D.3/15D.4: `CONSTANS` finalized for locals and rituale parameters, `const` codegen guarantees, pointer-binding (`CONSTANS SPECULUM`) behavior, edge-case closure tests, and phase-closure governance

### FASE 16 — Bootstrap and ASM Bridge Trajectory (Implemented Through 16D.4)
Architecture direction:
- start the bootstrap-oriented track while preserving host defaults
- keep CCT host-executed while introducing freestanding/ASM delivery contracts for LBOS integration
- enforce ABI/profile compatibility gates before broader bootstrap expansion

Reference architecture dossier:
- `md_out/FASE_16_CCT.md`

### FASE 17 — Canonical Library Expansion (Implemented Through 17D.4)
Architecture direction:
- unlock bootstrap/tooling workflows with practical text, cursor, args, variant, AST, and utility modules.
- preserve deterministic module/tooling behavior while expanding canonical host APIs.

### FASE 18 — Canonical Library Expansion (Implemented Through 18D.4)
Architecture direction:
- deliver broad host utility surface (`fs`, `io`, `path`, `process`) and deeper canonical algorithms/collections.
- extend deterministic utility baseline (`hash`, `bit`, `random` expansions) without destabilizing compiler contracts.

### FASE 19 — Language Surface Expansion (Implemented Through 19D.4)
Architecture direction:
- add first-class selection/pattern ergonomics (`ELIGE`, with `CUM` kept for compatibility) while preserving deterministic codegen contracts
- add interpolated string construction (`FORMA`) through runtime-backed builders in host profile
- add payload-capable ORDO representation with union-tagged lowering and scoped destructuring bindings
- unify collection iteration ergonomics by extending `ITERUM` to `map`/`set` with insertion-order semantics
- close documentation/release/handoff artifacts for the full phase (`19D2`, `19D3`, `19D4`)

Codegen/runtime notes:
- `ELIGE` lowers to integer/string dispatch chains and ORDO-tag switch forms with payload extraction locals.
- `FORMA` lowers to runtime helper sequences (`molde_begin/append/end`) and keeps ownership behavior explicit.
- payload `ORDO` lowers to tagged C structs with variant-specific payload unions.
- `ITERUM` now emits dedicated runtime iterator helpers for `map` and `set` (`cct_rt_map_iter_*`, `cct_rt_set_iter_*`), preserving insertion order.

### FASE 20 — Application Library Stack (Implemented Through 20F)
Architecture direction:
- turn the stabilized host subset into a practical application platform while keeping the language core unchanged
- keep protocol/model/config logic in CCT wherever possible
- keep runtime C additions thin and limited to host boundary surfaces such as sockets and SQLite

Library/runtime notes:
- `cct/json`, `cct/http`, `cct/config`, and most of `cct/net` are implemented as canonical CCT modules over existing runtime services.
- `cct/socket` and `cct/db_sqlite` extend the generated-host runtime with narrow OS/library bindings only.
- generated C now links `-lsqlite3` on demand when SQLite builtins are actually used by the source program.
- host-only boundaries remain explicit: sockets, HTTP, config over host FS/env, and SQLite are not part of the freestanding profile.

Operational closure:
- documentation, examples, release notes, and handoff artifacts are part of the phase definition of done.
- examples now cover HTTP JSON client/server, config + SQLite integration, and UDP loopback/demo flows.

## Reliability and Testing Model

Architecture quality is enforced by a phase-accumulated regression suite (`tests/run_tests.sh`) covering:
- lexical, parser, semantic, codegen, runtime, sigilo, module behavior
- deterministic output constraints
- boundary diagnostics and subset-policy enforcement

The architecture is considered stable only when full historical regression remains green.
Latest closure evidence (FASE 20F final gate): `Passed: 1181`, `Failed: 0`.

## Document Ownership

This file is an architectural snapshot and organization layer.
For detailed language behavior, execution subset boundaries, and roadmap specifics, also see:
- `docs/spec.md`
- `docs/roadmap.md`
- `README.md`

## FASE 21-30 Bootstrap and Self-Host Addendum

### Bootstrap Compiler Architecture

From FASE 21 onward, CCT gained a second compiler implementation written in CCT itself. The bootstrap compiler mirrors the host architecture:
- bootstrap lexer in `src/bootstrap/lexer/`
- bootstrap parser/AST in `src/bootstrap/parser/`
- bootstrap semantic analyzer in `src/bootstrap/semantic/`
- bootstrap code generator in `src/bootstrap/codegen/`
- self-hosted compiler entrypoints in `src/bootstrap/main_*.cct`

This was not introduced as a prototype-only experiment. The bootstrap stack is a validated architectural layer with dedicated regression gates and stage convergence requirements.

### Backend Evolution Through FASE 30

The host backend remains generated C plus a host C compiler. The bootstrap compiler converges to the same backend model:

```text
CCT source
  -> bootstrap front-end in CCT
  -> bootstrap semantic model in CCT
  -> bootstrap C code generation in CCT
  -> host C compiler
  -> executable
```

This preserves a single executable artifact strategy while allowing self-hosted evolution.

### Structural Data and Advanced Control Closure

The post-FASE-20 architecture gained the following validated backend milestones:
- FASE 26: foundational bootstrap code generation
- FASE 27: structural code generation for `SIGILLUM`, `ORDO`, and `ELIGE`
- FASE 28: generic materialization, advanced control flow, failure-control lowering, and `FORMA`
- FASE 29: stage0/stage1/stage2 convergence
- FASE 30: operational self-host workflows and mature application modules

### Current Test and Release Architecture

CCT now has multiple validation planes:
- historical legacy suites
- rebased legacy compatibility suites
- bootstrap compiler suites
- self-host convergence suites
- operational self-host platform suites

The authoritative whole-project gate is `make test-all-0-30`.

### Architectural Consequence

CCT should now be understood as a dual-implementation compiler platform:
- host compiler in C for baseline production stability
- bootstrap/self-host compiler in CCT for language self-sufficiency and future primary-toolchain evolution

This duality is intentional and is part of the current architecture, not technical debt.
