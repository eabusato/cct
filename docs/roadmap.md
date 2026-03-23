# CCT Development Roadmap

This roadmap is the authoritative phase-by-phase plan for CCT.

It has two goals:
- record what is already complete and stable
- define the next phases with clear scope boundaries and completion gates

## Planning Principles

- Incremental delivery by phase, with strict regression control
- Determinism first (compiler behavior, sigilo metadata, generic materialization)
- No silent scope expansion inside a phase
- Architecture continuity (avoid unnecessary rewrites of stable subsystems)
- Honest subset declarations (supported vs restricted vs out-of-scope)

## Current Project Snapshot

- Current completed phase: FASE 31
- Current completed subphase: FASE 31E
- Current phase context: the self-hosted compiler is now promoted to the operational default compiler path
- Phase-21 delivery status: bootstrap foundations and lexer implemented and closed
- Phase-22 and 23 delivery status: bootstrap parser, AST, advanced syntax surface, and modular parsing implemented and closed
- Phase-24 and 25 delivery status: bootstrap semantic core, generic semantics, constraints, and deduplicated instantiation implemented and closed
- Phase-26, 27, and 28 delivery status: bootstrap code generation, structural data lowering, advanced control flow, `FORMA`, and generic materialization implemented and closed
- Phase-29 delivery status: stage0/stage1/stage2 self-host convergence and identity validation implemented and closed
- Phase-30 delivery status: self-hosted workflows, mature application libraries, packaging, and final operational handoff implemented and closed
- Phase-31 delivery status: self-hosted compiler promoted to default, CLI parity, workflow integration, promotion/demotion infrastructure, and parity validation matrix implemented and closed
- Current whole-project regression gate: `make test-all-0-31`
- **Compiler maturity:** host compiler plus validated bootstrap/self-host compiler stack
- **Backend strategy:** generated C plus host C compiler remains the official backend for both host and bootstrap paths
- **Sigilo model:** dual-level modular model remains stable and continues to be part of the release contract
- **Validation model:** legacy, rebased legacy, bootstrap, self-host, and operational platform suites are first-class gates
- **Typing model:** advanced typing subset plus bootstrap generic semantics and materialization are closed on the validated baseline

## Completed Phases (0 → 19D4)

### FASE 0 — Foundation ✅

Objectives completed:
- repository structure and build system
- CLI skeleton (`--help`, `--version`)
- core error model and test harness

Architecture impact:
- established the project layout still used today

Quality gate achieved:
- deterministic base build and baseline tests

### FASE 1 — Lexical Analysis ✅

Objectives completed:
- full tokenizer implementation
- keywords, identifiers, literals, comments, location tracking
- `--tokens` CLI integration

Architecture impact:
- stable token stream contract used by parser and diagnostics

Quality gate achieved:
- lexical regression coverage and clear location-aware errors

### FASE 2A + 2B — Parser and AST ✅

Objectives completed:
- recursive-descent parser and AST model
- expression precedence and core ritual syntax
- parser hardening and AST printing (`--ast`)

Architecture impact:
- stable AST backbone for semantic/codegen/sigilo

Quality gate achieved:
- robust parser regression with syntax error behavior preserved

### FASE 3 — Semantic Core ✅

Objectives completed:
- symbol registration and scope management
- type checks for core executable subset
- declaration/use and call compatibility validation
- `--check` integration

Architecture impact:
- semantic pass model used by every later phase

Quality gate achieved:
- semantic pass became mandatory gate before codegen and sigilo emission

### FASE 4A + 4B + 4C — Executable Subset Foundation ✅

Objectives completed:
- first executable pipeline via generated C
- structured control flow and call execution
- consolidated loop/call behavior with explicit subset boundaries

Architecture impact:
- C-hosted backend became official strategy

Quality gate achieved:
- end-to-end compile/run tests across core subset

### FASE 5 + 5B + 6A — Sigilo Foundation and Visual Engine ✅

Objectives completed:
- deterministic sigilo generation integrated with compile
- layered SVG model and `.sigil` metadata
- visual engine redesign and style options
- sigilo CLI controls (`--sigilo-only`, style/output selectors)

Architecture impact:
- sigilo became first-class output (not post-processing)

Quality gate achieved:
- determinism tests and style/metadata regressions

### FASE 6B + 6C + 6D — Runtime and Backend Hardening ✅

Objectives completed:
- executable language expansion (`UMBRA`, `FLAMMA`, `VERBUM`, `SERIES`, base `SIGILLUM`)
- runtime helper layer and cleaner generated C organization
- internal contract hardening across semantic/codegen/runtime

Architecture impact:
- backend and runtime interfaces stabilized for large feature growth

Quality gate achieved:
- generated C structure and runtime-path regressions became explicit

### FASE 7A + 7B + 7C + 7D — Memory and Structural Consolidation ✅

Objectives completed:
- practical pointer/memory subset (`SPECULUM`, alloc/free/discard patterns)
- advanced executable `SIGILLUM` subset and controlled by-reference mutation
- dynamic indexed-memory subset and phase-7 policy closure

Architecture impact:
- memory/structural behavior moved from demo-level to operational subset

Quality gate achieved:
- policy-level diagnostics and behavior consistency in phase-7 final tests

### FASE 8A + 8B + 8C + 8D — Failure-Control System ✅

Objectives completed:
- core throw/catch model (`IACE`, `TEMPTA`, `CAPE`)
- propagation across ritual calls
- guaranteed finalization path (`SEMPER`) in supported subset
- final phase-8 harmonization and policy closure

Architecture impact:
- failure-control became a coherent execution axis integrated with runtime behavior

Quality gate achieved:
- local/propgation/cleanup/final-policy tests consolidated

### FASE 9A + 9B + 9C + 9D + 9D2 + 9E — Modular Architecture Closure ✅

Objectives completed:
- multi-file closure and deterministic import graph (`ADVOCARE`)
- inter-module resolution rules and visibility boundaries (`ARCANUM`)
- official dual-level sigilo architecture (local + system)
- system sigilo rendered as vector inline sigil-of-sigils
- final CLI contract stabilization for modular workflows

Architecture impact:
- modules and system sigilo became stable platform capabilities

Quality gate achieved:
- essential/complete mode behavior, modular diagnostics, and system sigilo determinism

### FASE 10A + 10B + 10C + 10D + 10E — Advanced Typing Consolidation ✅

Objectives completed:
- generic model (`GENUS`) with explicit instantiation
- pragmatic executable monomorphization and deterministic dedup
- contract model (`PACTUM`) with explicit conformance
- basic constraints (`GENUS(T PACTUM C)`) in supported contexts
- final harmonization and frozen subset contract (10E)

Architecture impact:
- advanced typing subset is stable enough to support larger library/ecosystem phases

Quality gate achieved:
- full regression closure through phase-10 final tests and metadata contract checks

### FASE 11A — Bibliotheca Canonica Foundation ✅

Objectives completed:
- official reserved namespace `cct/...`
- canonical stdlib physical root (`lib/cct/`) and deterministic resolver contract
- foundational stdlib module-origin metadata in module closure
- platform contracts documented (error, ownership, side effects, stability classes)
- inspection compatibility preserved (`--ast`, `--ast-composite`, `--check`)

Architecture impact:
- standard-library foundation is now a concrete platform layer, not only roadmap intent

Quality gate achieved:
- canonical namespace resolution tests
- reserved-namespace diagnostics for missing stdlib modules
- full historical regression remains green

### FASE 11B.1 — VERBUM Canonical Text Core ✅

Objectives completed:
- `cct/verbum` module delivered in Bibliotheca Canonica
- text primitives: `len`, `concat`, `compare`, `substring`, `trim`, `contains`, `find`
- strict substring bounds policy with clear runtime failure behavior
- predictable text composition basis for `cct/fmt` and downstream stdlib modules

Architecture impact:
- platform now has a canonical text-manipulation foundation layer

Quality gate achieved:
- dedicated 11B.1 text tests green
- integration with `OBSECRO scribe` maintained
- full historical regression remains green

### FASE 11B.2 — FMT Canonical Formatting/Conversion ✅

Objectives completed:
- `cct/fmt` module delivered
- canonical stringify APIs for int/real/float/bool
- canonical parse APIs for int/real with strict invalid-input behavior
- textual composition helper (`format_pair`) for label-value output

Architecture impact:
- textual subsystem 11B is now complete (`verbum` + `fmt`)

Quality gate achieved:
- dedicated 11B.2 formatting/parse tests green
- scribe integration preserved
- full historical regression remains green

### FASE 11C — SERIES + ALG Canonical Collection Baseline ✅

Objectives completed:
- `cct/series` module delivered with static-collection helpers
- `cct/alg` module delivered with deterministic integer-array algorithms
- practical generic utility usage preserved where semantically valid (`series_fill`, `series_copy`, `series_reverse`)
- integration with existing textual modules validated

Architecture impact:
- static collections now have canonical reusable operations
- foundational algorithm layer exists for follow-up utility phases

Quality gate achieved:
- dedicated 11C collection/algorithm tests green
- sigilo generation remains stable with new stdlib modules
- full historical regression remains green

### FASE 11D.1 — MEM + Ownership Foundation ✅

Objectives completed:
- `cct/mem` module delivered
- runtime-backed memory builtins integrated (`alloc`, `free`, `realloc`, `copy`, `set`, `zero`, `compare`)
- explicit ownership/discard policy documented for canonical stdlib flows
- memory utility baseline prepared for `FLUXUS` storage phases

Architecture impact:
- canonical dynamic-memory foundation now exists for stdlib collections expansion

Quality gate achieved:
- dedicated 11D.1 memory tests green
- interop with existing stdlib modules preserved
- full historical regression remains green

### FASE 11D.2 — FLUXUS Storage Runtime Core ✅

Objectives completed:
- runtime-backed FLUXUS storage core delivered (`src/runtime/fluxus_runtime.*`, `src/runtime/mem_runtime.*`)
- deterministic capacity growth and storage primitives validated
- standalone runtime test target integrated (`make test_fluxus_storage`)

Architecture impact:
- dynamic-vector storage core became explicit and testable as a runtime layer

Quality gate achieved:
- dedicated 11D.2 runtime storage tests green
- full historical regression remains green

### FASE 11D.3 — FLUXUS Canonical API ✅

Objectives completed:
- canonical module `cct/fluxus` delivered
- ergonomic public API for dynamic vectors (`init/free/push/pop/len/get/clear/reserve/capacity`)
- sigilo metadata counters added for FLUXUS operations
- usage docs and example program added

Architecture impact:
- block 11D is now fully closed (`mem` + storage core + public fluxus facade)

Quality gate achieved:
- dedicated 11D.3 API/integration tests green
- full historical regression remains green

### FASE 11E.1 — IO + FS Baseline ✅

Objectives completed:
- canonical modules `cct/io` and `cct/fs` delivered
- runtime-backed IO surface integrated (`print`, `println`, `print_int`, `read_line`)
- runtime-backed filesystem surface integrated (`read_all`, `write_all`, `append_all`, `exists`, `size`)
- IO/FS integration with `VERBUM` covered by dedicated tests and fixtures

Architecture impact:
- canonical external-world interaction layer is now available on top of the 11D ownership/runtime foundation

Quality gate achieved:
- dedicated 11E.1 IO/FS tests green
- sigilo compatibility preserved with stdlib imports
- full historical regression remains green

### FASE 11E.2 — PATH + Text/File Integration ✅

Objectives completed:
- canonical module `cct/path` delivered
- minimal stable path API shipped (`path_join`, `path_basename`, `path_dirname`, `path_ext`)
- integration with `cct/fs` validated through read/write path-composition tests
- sigilo metadata counters for PATH usage added

Architecture impact:
- block 11E is now closed (`cct/io` + `cct/fs` + `cct/path`) with clear IO/FS/PATH boundaries

Quality gate achieved:
- dedicated 11E.2 PATH tests green
- full historical regression remains green

### FASE 11F.1 — MATH + RANDOM Baseline ✅

Objectives completed:
- canonical modules `cct/math` and `cct/random` delivered
- deterministic numeric helper surface (`abs`, `min`, `max`, `clamp`)
- reproducible random baseline (`seed`, `random_int`, `random_real`)
- sigilo metadata counters for math/random operations added

Architecture impact:
- utility block 11F started with a practical numeric+random baseline without expanding into heavy statistical scope

Quality gate achieved:
- dedicated 11F.1 tests green
- full historical regression remains green

### FASE 11F.2 — PARSE + CMP + Moderate ALG Expansion ✅

Objectives completed:
- canonical modules `cct/parse` and `cct/cmp` delivered
- strict textual conversions shipped (`parse_int`, `parse_real`, `parse_bool`)
- canonical comparator layer shipped (`cmp_int`, `cmp_real`, `cmp_bool`, `cmp_verbum`)
- moderate `cct/alg` expansion added (`alg_binary_search`, `alg_sort_insertion`)
- sigilo metadata counters for parse/cmp/alg operations added

Architecture impact:
- utility block 11F closed with a practical numeric + parse + compare baseline, without expanding into heavy parsing/statistics framework scope

Quality gate achieved:
- dedicated 11F.2 tests green
- full historical regression remains green

### FASE 11G — Canonical Showcase + Sigilo Integration ✅

Objectives completed:
- canonical showcase suite delivered (`string`, `collection`, `io/fs`, `parse/math/random`, `multi-module`)
- public examples mirrored by deterministic integration fixtures in `tests/integration/`
- stdlib usage counters/list consolidated in `.sigil` and `.system.sigil`
- showcase workflows validated for `--check`, `--ast`, `--ast-composite`, and `--sigilo-only`

Architecture impact:
- Bibliotheca Canonica now has a public-facing, executable showcase layer on top of the technical stdlib baseline

Quality gate achieved:
- dedicated 11G tests (428–439) green
- essential/complete sigilo mode stability preserved in multi-module showcase
- full historical regression remains green

### FASE 11H — Final Consolidation + Public Technical Release Readiness ✅

Objectives completed:
- final stdlib subset manifest frozen (`docs/stdlib_subset_11h.md`)
- final stability matrix published (`docs/stdlib_stability_matrix_11h.md`)
- final naming boundary freeze documented (including `cct/fmt` parse façade)
- packaging/install contract finalized (`make dist`, `make install`, `make uninstall`)
- install and release notes documentation delivered (`docs/install.md`, `docs/release/FASE_11_RELEASE_NOTES.md`)

Architecture impact:
- FASE 11 is formally closed as a publishable language + canonical stdlib kit baseline

Quality gate achieved:
- dedicated 11H release-readiness tests green
- full historical regression remains green

### FASE 12 — Runtime/Tooling Expansion Closure ✅

Objectives completed:
- structured diagnostics and cast baseline (`12A`, `12B`)
- Option/Result, Map/Set, and collection combinators (`12C`, `12D.1`, `12D.2`)
- `ITERUM` baseline syntax and semantics (`12D.3`)
- formatter/linter/project workflow/doc generator (`12E`, `12F`, `12G`)
- closure and governance gate (`12H`)

Quality gate achieved:
- full suite green at phase closure
- legacy behavior preserved while tooling surface expanded

### FASE 13 + 13M — Sigilo Tooling + Math Addendum Closure ✅

Objectives completed:
- sigilo inspect/diff/check/baseline/validate ecosystem (`13A`..`13D`)
- determinism audit and release governance closure (`13D.2`..`13D.4`)
- common math operator addendum (`13M`: `**`, `//`, `%%`) with full matrix validation

Quality gate achieved:
- deterministic contracts preserved
- complete regression closure and release artifacts published

### FASE 14 — Release Hardening Closure ✅

Objectives completed:
- diagnostics/exit-code/explain-mode harmonization (`14A`)
- docs/publication/release process hardening (`14B`, `14D`)
- regression matrix, stress/soak, performance budget, residual risk register (`14C`)

Quality gate achieved:
- release engineering gates stabilized without breaking legacy workflows

### FASE 15 — Semantic Surface Consolidation Closure ✅

Objectives completed:
- loop-control finalization (`FRANGE`, `RECEDE`) across supported loops
- logical and bitwise/shift operator closure
- `CONSTANS` semantic and codegen stabilization

Quality gate achieved:
- full closure gate passed with no regressions

### FASE 16 — Freestanding/ASM Bridge Closure ✅

Objectives completed:
- `--profile freestanding`, `--emit-asm`, and entrypoint contract stabilization
- freestanding runtime shim and `cct/kernel` bridge surface
- ABI/linkability and host zero-regression closure

Quality gate achieved:
- bridge path validated while preserving host baseline behavior

### FASE 17 — Canonical Library Expansion Closure ✅

Objectives completed:
- lexer/tooling-oriented canonical modules (`verbum_scan`, `args`, `char`)
- text-construction utilities (`verbum_builder`, `code_writer`)
- host utility modules (`env`, `time`, `bytes`) and closure artifacts

Quality gate achieved:
- full historical regression green at 17D.4 closure

### FASE 18 — Canonical Library Expansion Closure ✅

Objectives completed:
- large stdlib expansion in text/format/parse, fs/io/path, collections/algorithms
- new modules: `process`, `hash`, `bit`
- closure docs and governance finalized in 18D.4

Quality gate achieved:
- full suite green and canonical library baseline consolidated

### FASE 19 — Language Surface Expansion Closure ✅

Objectives completed:
- `ELIGE`/`CASUS`/`ALIOQUIN` over integer, `VERBUM`, and payload `ORDO` (`CUM` preserved as a compatibility alias)
- `FORMA` interpolation with formatting specs (host profile)
- payload-capable `ORDO` declaration/construction/destructuring
- `ITERUM` expansion to `map`/`set` with arity validation and insertion-order contract
- documentation/release/handoff closure (`19D2`, `19D3`, `19D4`)

Quality gate achieved:
- final closure gate: `Passed: 1069 / Failed: 0`

## Phase Status Matrix

- **Completed:** FASE 0 to FASE 31, plus FASE 14T
- **Current operational baseline:** promoted self-hosted compiler as default, aggregated validation through phase 31
- **Next work category:** post-bootstrap platform maturity (diagnostics, performance, stdlib expansion)
## Bootstrap Delivery Record (Historical Plan, Now Executed)

### FASE 14T — Sigilo SVG Instrumentation Interstitial (Completed)

Primary objective:
- upgrade sigilo SVG artifacts from deterministic visual outputs into deterministic, hover-readable semantic reading instruments.

Delivered scope:
- `14TA`: source-context extraction/normalization and tooltip text pipeline.
- `14TB`: SVG `<title>` emission for ritual nodes, structural nodes, local edges, and system-sigilo module/system edges.
- `14TC`: deterministic additive `data-*` on local nodes/call edges plus lightweight root semantics.
- `14TD`: hover polish, instrumentation toggles, documentation, release notes, handoff, and closure gate.

Architectural policy:
- no JavaScript dependency;
- keep SVG pure, exportable, and diff-friendly;
- preserve deterministic output contracts;
- keep `.sigil` schema stability unless additive metadata becomes strictly necessary.

Out of scope:
- full web viewer, animated navigation, search UI, or a large visual redesign of sigilo layout.

Closure evidence:
- major SVG elements now expose useful hover titles and stable metadata attributes without breaking current sigilo workflows.
- local and system sigilo components can now be hovered directly in the SVG to inspect meaning without a separate viewer.
- instrumentation can be disabled via `--sigilo-no-titles` and `--sigilo-no-data`, including full plain-mode fallback.
- dedicated FASE 14T regression coverage: 46 tests.
- final closure gate: `Passed: 1120 / Failed: 0`.

### FASE 20 — Application Library Stack Expansion ✅

Primary objective completed:
- turned the post-19 language baseline into a practical application platform through canonical JSON, networking, HTTP, configuration, and local database modules.

Delivered scope:
- `20A`: `cct/json` with JSON value model, strict parser, stringify/pretty-print, and navigation helpers.
- `20B`: `cct/socket` / `cct/net` with host-only TCP/UDP runtime bridge plus high-level wrappers in CCT.
- `20C`: `cct/http` client/server baseline over `cct/net`, covering HTTP/1.1 text workflows and JSON integration.
- `20D`: `cct/config` for canonical application configuration parsing/loading/writing.
- `20E`: `cct/db_sqlite` for local persistence via a thin runtime bridge and ergonomic CCT wrappers.
- `20F`: documentation, examples, release closure, and hardening of the new stack.

Architectural policy:
- implement parsing, modeling, protocol logic, wrappers, and high-level ergonomics in CCT wherever possible;
- keep C runtime additions thin and limited to OS/binding boundaries such as sockets, timeouts, and SQLite APIs.

Out of scope:
- TLS/HTTPS, HTTP/2+, WebSocket, remote DB drivers, ORM layers, and a broad new wave of core-language syntax work.
- large ownership/lifetime redesigns.

Closure evidence:
- 61 new integration tests across 20A-20E.
- canonical examples published for HTTP JSON client/server, config + SQLite, and UDP loopback.
- release/handoff artifacts published for FASE 20.
- final closure gate: `Passed: 1181 / Failed: 0`.

### FASE 21 — Bootstrap: Foundations + Lexer ✅

Primary objective:
- begin compiler self-hosting by porting lexer to CCT and establishing library foundations needed for bootstrap.

Planned scope:
- `21A`: library foundations (`cct/char`, `cct/verbum` expansions, `cct/diagnostic`)
- `21B`: token model and keyword table in CCT
- `21C`: lexer core (character scanning, literals)
- `21D`: lexer advanced (comments, error recovery, line tracking)
- `21E`: validation (lexer CCT tokenizes suite identically to lexer C)

Out of scope:
- parser, semantic, codegen (FASE 22-28)
- self-hosting completion (FASE 29)
- library maturation (FASE 30)

Architecture impact:
- first compiler component rewritten in CCT
- library grows incrementally per bootstrap needs

Definition of done:
- lexer CCT produces identical tokens to lexer C on full suite
- ~1500 LOC CCT (library ~800 + lexer ~700)
- 2-3 months duration

### FASE 22 — Bootstrap: Parser + AST Baseline ✅

Primary objective:
- port core parser and AST construction to CCT.

Planned scope:
- `22A`: AST model (94 node types as ORDO/SIGILLUM)
- `22B`: expression parsing (literals, binary, unary, call)
- `22C`: basic statement parsing (EVOCA, REDDE, SI, DUM)
- `22D`: declaration parsing (RITUALE, SIGILLUM, simple ORDO)
- `22E`: error recovery + synchronization
- `22F`: validation (parse simple programs from suite)

Critical challenge:
- AST representation without C unions (ORDO payload vs SPECULUM NIHIL tradeoff)

Out of scope:
- advanced control flow (TEMPTA/CAPE, ELIGE with QUANDO fallback, nested loops) → FASE 23
- GENUS parsing → FASE 23
- semantic analysis → FASE 24

Definition of done:
- parser CCT generates valid AST for simple programs
- ~4500 LOC CCT
- 3-4 months duration

### FASE 23 — Bootstrap: Parser Advanced + Modularity ✅

Primary objective:
- complete parser with advanced control flow, generics, and module system.

Planned scope:
- `23A`: complex control flow (TEMPTA/CAPE, ELIGE with QUANDO fallback, nested loops)
- `23B`: GENUS parsing (type parameters, constraints)
- `23C`: PACTUM/CODEX parsing
- `23D`: import/module system parsing
- `23E`: validation (parse full suite including generics)

Out of scope:
- semantic analysis → FASE 24

Definition of done:
- parser CCT handles full language surface
- ~2500 LOC CCT
- 2-3 months duration

### FASE 24 — Bootstrap: Semantic Analyzer — Types & Scopes ✅

Primary objective:
- port core semantic analysis (symbol tables, types, scopes) to CCT.

Planned scope:
- `24A`: symbol table + scope chains
- `24B`: type system model (13 primitive + composite types)
- `24C`: type resolution (primitives, SIGILLUM, ORDO)
- `24D`: expression type checking
- `24E`: statement validation
- `24F`: function signature checking
- `24G`: validation (semantic check on programs without GENUS)

Critical challenge:
- symbol lookup O(n) may require hashmap (add if profiling shows bottleneck)
- scope parent chains with SPECULUM
- type equality and compatibility

Out of scope:
- generic type instantiation → FASE 25

Definition of done:
- semantic analyzer CCT validates non-generic programs correctly
- ~6500 LOC CCT
- 4-5 months duration

### FASE 25 — Bootstrap: Semantic Advanced — Generics ✅

Primary objective:
- complete semantic analysis with generic type instantiation and constraint checking.

Planned scope:
- `25A`: generic type parameter tracking
- `25B`: generic type instantiation
- `25C`: constraint checking (PACTUM conformance)
- `25D`: generic deduplication (FNV hash + canonicalization)
- `25E`: validation (full semantic check with GENUS)

Critical challenge:
- instantiation cache and deduplication strategy

Definition of done:
- semantic analyzer CCT handles full language with generics
- ~2500 LOC CCT
- 3-4 months duration

### FASE 26 — Bootstrap: Codegen Foundation ✅

Primary objective:
- port core code generation (expressions, statements, functions) to CCT.

Planned scope:
- `26A`: codegen context + C emission infrastructure
- `26B`: expression emission (literals, binary, unary, call)
- `26C`: statement emission (assign, if, loops)
- `26D`: function emission (signature, body, return)
- `26E`: type mapping CCT → C
- `26F`: runtime bridge emission (includes, helpers)
- `26G`: validation (generate C for simple programs, compile and execute)

Critical challenge:
- string building for C code (~311 sprintf uses in C compiler)
- string pool management
- temporary variable generation

Out of scope:
- struct/enum codegen → FASE 27
- generic instantiation codegen → FASE 28

Definition of done:
- codegen CCT produces valid C for simple programs
- ~5500 LOC CCT
- 4-5 months duration

### FASE 27 — Bootstrap: Codegen — Structs & Enums ✅

Primary objective:
- complete codegen for structured types (SIGILLUM, ORDO with/without payload).

Planned scope:
- `27A`: SIGILLUM codegen (typedef, field access, copy)
- `27B`: simple ORDO codegen (C enum)
- `27C`: ORDO payload codegen (tagged union)
- `27D`: ELIGE codegen (QUANDO fallback compatibility; switch vs if-chain, payload destructuring)
- `27E`: validation (generate C for programs with structs/enums)

Definition of done:
- codegen CCT handles structured types
- ~2500 LOC CCT
- 2-3 months duration

### FASE 28 — Bootstrap: Codegen — Generics & Control Flow ✅

Primary objective:
- complete codegen with generic instantiation, exceptions, and advanced control flow.

Planned scope:
- `28A`: generic instantiation codegen
- `28B`: exception handling codegen (TEMPTA/CAPE/SEMPER)
- `28C`: advanced control flow (nested loops, break/continue labels)
- `28D`: FORMA (string interpolation) codegen
- `28E`: validation (generate C for complex programs from suite)

Definition of done:
- codegen CCT handles full language surface
- ~3500 LOC CCT
- 3-4 months duration

### FASE 29 — Bootstrap: Self-Hosting Complete ✅

Primary objective:
- achieve full self-hosting with multi-stage bootstrap validation.

Planned scope:
- `29A`: compile CCT compiler with C compiler → stage 0
- `29B`: stage 0 compiles itself → stage 1
- `29C`: stage 1 compiles itself → stage 2
- `29D`: identity validation (stage 1 == stage 2)
- `29E`: regression testing (full suite with bootstrap compiler)
- `29F`: performance profiling + critical optimizations

Critical gate:
- 3-stage bootstrap complete with identical outputs
- full suite (1181+ tests) passes with bootstrap compiler
- performance acceptable (10× slower than C compiler is OK)

Definition of done:
- CCT compiler successfully compiles itself
- ~500 LOC CCT (scripts + harness)
- 2-3 months duration

### FASE 30 — Operational Self-Hosted Platform ✅

Primary objective:
- close the operational transition from validated bootstrap to usable self-hosted platform.

Delivered scope:
- `30A`: self-hosted toolchain as default operational path
- `30B`: self-hosted stdlib/runtime compatibility subset
- `30C`: mature application libraries (`csv`, `https`, `orm_lite`)
- `30D`: self-hosted project workflows, packaging, and distribution
- `30E`: final gate, release notes, and handoff

Out of scope:
- further language surface expansion (post-bootstrap phases)

Definition of done:
- self-hosted compiler is an operational path
- application-library subset is usable over the self-hosted compiler
- project workflows are validated over the self-hosted path
- release/handoff docs match the validated state

### FASE 31 — Self-Hosted Compiler Promotion ✅

Primary objective:
- promote the validated self-hosted compiler from bootstrap artifact to operational default compiler path

Delivered scope:
- `31A`: self-host wrapper parity (compile, output handling, stdlib resolution)
- `31B`: CLI contract parity (--check, --ast, --tokens, fallback delegation)
- `31C`: project workflow parity (build, run, test, bench, clean, package)
- `31D`: promotion and demotion infrastructure (explicit, reversible, testable)
- `31E`: default switch and final validation gate

Out of scope:
- complete tooling parity (fmt, lint, doc remain host-delegated)
- pending stdlib modules (config, json, db_sqlite, http)
- performance optimization (bootstrap 2-3x slower than host acceptable)

Definition of done:
- `./cct` defaults to self-hosted compiler after promotion
- `cct-host` and `cct-selfhost` explicit paths available
- promotion/demotion via `make bootstrap-promote` / `make bootstrap-demote`
- formal parity matrix documented (`docs/bootstrap_parity_matrix.txt`)
- parity validation automated (`make test-bootstrap-parity`)
- 31 promotion tests passing (1724-1754)
- host compiler preserved as fallback

## Bootstrap Phase Summary

| Phase | Scope | Duration | LOC CCT | Cumulative |
|-------|-------|----------|---------|------------|
| 21 | Foundations + Lexer | 2-3 mo | ~1500 | 1500 |
| 22 | Parser básico | 3-4 mo | ~4500 | 6000 |
| 23 | Parser avançado | 2-3 mo | ~2500 | 8500 |
| 24 | Semantic básico | 4-5 mo | ~6500 | 15000 |
| 25 | Semantic generics | 3-4 mo | ~2500 | 17500 |
| 26 | Codegen básico | 4-5 mo | ~5500 | 23000 |
| 27 | Codegen structs/enums | 2-3 mo | ~2500 | 25500 |
| 28 | Codegen generics | 3-4 mo | ~3500 | 29000 |
| 29 | Self-hosting | 2-3 mo | ~500 | 29500 |
| 30 | Closure + library | 2-3 mo | library | 30000+ |
| 31 | Promotion + parity | 1-2 mo | wrapper | 30000+ |
| **TOTAL** | **11 phases** | **29-40 mo** | **~30K LOC** | |

**Bootstrap status:** phases 21-31 completed on the current validated baseline.

## Version Milestone Targets

### v0.1 — Principium

Target profile:
- stable public technical preview
- complete core language/toolchain baseline through advanced typing consolidation
- mature sigilo outputs and modular workflows

### v0.2 — Corpus

Target profile:
- stronger stdlib and runtime maturity
- improved backend quality and broader practical programming surface

### v0.3 — Arcanum

Target profile:
- deeper ecosystem maturity and advanced tooling
- strong release confidence for larger projects

### v1.0 — Clavicula Seipsam Aperit

Target profile:
- self-hosting trajectory completed (or functionally equivalent production maturity)
- full production-grade stability and ecosystem readiness

## Cross-Phase Engineering Gates

Every phase must pass these gates before closure:
- full regression suite remains green
- deterministic behavior preserved where promised
- subset boundaries documented (supported/restricted/out-of-scope)
- docs updated (`README`, `spec`, `architecture`, roadmap)
- no silent API contract breaks in stable public entry points

## Immediate Next Step

The bootstrap roadmap (phases 21-31) is complete. The self-hosted compiler is now the operational default.

Immediate engineering focus:
- keep `make test-all-0-31` green
- maintain parity validation (`make test-bootstrap-parity`)
- preserve host/bootstrap/self-host behavioral convergence
- grow the post-bootstrap platform without regressing the validated operational baseline

Post-31 priorities:
- diagnostics quality (error messages, source highlighting, suggestions)
- performance optimization (reduce 2-3x overhead vs host)
- stdlib parity (export pending modules: config, json, db_sqlite, http)
- developer experience (LSP, formatter, linter via selfhost)

Historical traceability note: closure artifacts from the host-era and bootstrap-era phases are preserved under `docs/release/` and `docs/bootstrap/`.

## FASE 31 Closure Summary

FASE 31 closes the compiler-promotion problem.

Closed outcomes:
- the self-hosted compiler is no longer only a validated bootstrap artifact
- the repository exposes explicit host and self-host wrappers side by side
- the default wrapper path can be promoted or demoted intentionally
- the testing model now distinguishes product validation, host fallback validation, bootstrap identity, and phase-specific promotion gates

## Next Frontier After Promotion

The roadmap after FASE 31 should be read as platform maturation, not bootstrap enablement.

Immediate forward frontier:
- preserve host/self-host behavioral convergence
- continue moving practical workflows from host delegation to fully self-hosted implementations where justified
- improve diagnostics, performance, and stdlib parity without breaking the promoted operational model
