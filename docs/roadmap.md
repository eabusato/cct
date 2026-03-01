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

- **Current completed phase:** FASE 12H (structural maturity milestone)
- **Compiler maturity:** complete development toolchain with formatter, linter, build system, and doc generator
- **Backend strategy:** C-hosted backend is official (`.cgen.c` + host C compiler)
- **Sigilo model:** dual-level modular model is stable (local + system sigilo)
- **Typing model:** advanced typing subset stabilized (`GENUS`, `PACTUM`, basic constraints)

## Completed Phases (0 → 10E)

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
- install and release notes documentation delivered (`docs/install.md`, `docs/release_11_notes.md`)

Architecture impact:
- FASE 11 is formally closed as a publishable language + canonical stdlib kit baseline

Quality gate achieved:
- dedicated 11H release-readiness tests green
- full historical regression remains green

## Phase Status Matrix

- **Completed:** FASE 0 to FASE 11H
- **Next active phase:** FASE 12
- **Long-term targets:** FASE 12 to FASE 15

## Forward Roadmap (Planned)

### FASE 11 — Standard Library and Ecosystem Base ✅ (closed at 11H)

Primary objective:
- provide a practical standard-library baseline on top of the stabilized language core

Planned scope:
- foundational library modules (text, collections baseline, utility/math baseline, runtime-safe helpers)
- packaging/import conventions for reusable module sets
- stable developer workflows for composing larger CCT applications

Out of scope:
- major compiler redesign
- advanced package manager ecosystem features (if not explicitly scoped)

Definition of done:
- a documented and test-covered stdlib baseline used by real multi-module examples
- no regressions in core language/toolchain behavior

### FASE 12 — Backend and Runtime Evolution

Primary objective:
- improve code quality and runtime performance without breaking language contracts

Delivered through 12G:
- 12A: structured diagnostics and actionable suggestions
- 12B: numeric cast baseline (`cast GENUS(T)(value)`)
- 12C: canonical Option/Result ergonomics
- 12D.1: hash-backed Map/Set baseline
- 12D.2: collection combinators (`fluxus_*`, `series_*`)
- 12D.3: baseline iterator syntax (`ITERUM`)
- 12E.1: formatter command (`cct fmt`)
- 12E.2: linter command (`cct lint`)
- 12F: canonical project workflow (`cct build|run|test|bench|clean`) with basic incremental cache
- 12G: canonical API doc generator (`cct doc`) with markdown/html output and strict warning mode

Out of scope:
- destabilizing rewrite of public compiler behavior

Definition of done:
- measurable quality/performance improvements with unchanged semantic behavior
- deterministic outputs preserved where required

### FASE 13 — Sigilo Tooling Expansion

Primary objective:
- expand sigilo workflows and tooling while preserving deterministic core behavior

Planned scope:
- tooling around sigilo inspection/comparison and development workflows
- metadata enrichment for analysis use-cases
- optional visualization refinements with backward-compatible semantics

Out of scope:
- breaking local/system sigilo architecture or deterministic guarantees

Definition of done:
- tooling value added without compromising determinism or regression stability

### FASE 14 — Release Hardening (v0.x)

Primary objective:
- prepare a robust technical release baseline

Planned scope:
- diagnostics and UX polish
- documentation finalization and consistency pass
- reliability/performance hardening under realistic workloads
- release process/checklist formalization

Out of scope:
- large new language feature blocks unless explicitly approved

Definition of done:
- release-quality docs + predictable tooling behavior + stable regression story

### FASE 15 — Bootstrap Trajectory

Primary objective:
- establish the practical path toward self-hosting

Planned scope:
- staged bootstrap plan with explicit checkpoints
- compiler-core migration strategy to CCT where feasible
- compatibility and fallback strategy during transition

Out of scope:
- all-at-once rewrite

Definition of done:
- validated, test-backed bootstrap milestones with clear rollback strategy

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

**Next phase to execute:** FASE 12, backend/runtime evolution on top of the frozen FASE 11 baseline.
