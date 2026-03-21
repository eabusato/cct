# CCT Compiler Architecture

This document defines the current technical architecture of CCT after the closure of FASE 30.

## System Overview

CCT is built as a layered compiler platform with two implementation paths:
- a host compiler implemented in C
- a bootstrap compiler stack implemented in CCT

The bootstrap stack is no longer experimental. It is validated through multi-stage self-hosting and operational project workflows.

## End-to-End Pipeline

```text
.cct source
  -> module discovery / closure
  -> lexer
  -> parser / AST
  -> semantic analysis
  -> code generation (.cgen.c)
  -> host C compiler
  -> executable
```

Optional parallel artifact pipeline:

```text
.cct source
  -> sigilo generation
  -> .svg + .sigil
```

## Major Subsystems

### Host Compiler (`src/`)

Purpose:
- primary production compiler implementation in C
- compatibility reference for bootstrap validation
- distribution entrypoint today

Key areas:
- `src/lexer/`
- `src/parser/`
- `src/semantic/`
- `src/codegen/`
- `src/runtime/`
- `src/module/`
- `src/project/`
- `src/doc/`
- `src/sigilo/`

### Bootstrap Compiler (`src/bootstrap/`)

Purpose:
- self-hosting implementation path in CCT
- validation target for staged compiler replacement
- operational backend for self-hosted workflows

Layers:
- `src/bootstrap/lexer/`
- `src/bootstrap/parser/`
- `src/bootstrap/semantic/`
- `src/bootstrap/codegen/`
- `src/bootstrap/main_*.cct`
- `src/bootstrap/selfhost_*.cct`

### Standard Library (`lib/cct/`)

Purpose:
- canonical language library surface
- runtime-independent and host-bridged modules
- foundation for both user programs and bootstrap subsystems

Examples:
- text and collections
- filesystem and process
- JSON / HTTP / networking
- CSV / HTTPS / ORM-lite

## Self-Hosting Architecture

### Stage Model

- `stage0`: host compiler emits the bootstrap compiler
- `stage1`: `stage0` recompiles the compiler
- `stage2`: `stage1` recompiles the compiler again

Convergence gate:
- generated C output must be identical across stable stages
- binary identity is validated with platform-aware rules

### Operational Self-Hosted Layer

The self-hosted layer adds:
- wrapper binary and support prelude
- self-host workflow targets
- supported subset matrix for stdlib/runtime
- example project and negative project fixtures

## Testing Architecture

CCT now uses three validation tiers:

1. Historical frozen legacy validation
2. Rebased compatibility validation against the current compiler
3. Bootstrap / self-host / operational validation for the modern stack

The aggregated runner `tests/run_tests_all_0_30.sh` is the top-level cross-era gate.

## Architectural Contracts

- Deterministic code generation
- Deterministic sigilo emission
- Explicit ownership in runtime and bootstrap subsystems
- Stable multi-stage identity in self-host flows
- Project workflows treated as first-class compiler behavior

## Current Technical Direction

Bootstrap enablement is complete. The architecture should now evolve toward:
- stronger self-hosted parity
- broader operational stdlib/runtime support
- performance work
- packaging and ecosystem maturity

That work should not break the validation layering described above.
