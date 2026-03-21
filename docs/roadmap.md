# CCT Development Roadmap

This roadmap is the authoritative high-level plan for CCT.

It serves three purposes:
- record what is complete and validated
- define the current architectural baseline
- identify the next post-bootstrap engineering direction

## Current Baseline

Current completed phase: **FASE 30**.

The project now includes:
- host compiler in C
- bootstrap compiler stack in CCT
- multi-stage self-host convergence
- operational self-hosted workflows
- mature baseline standard library and application-library subset
- complete aggregated regression path from phase 0 through phase 30

Bootstrap status:
- lexer, parser, semantic analysis, and code generation are complete in the bootstrap stack
- multi-stage self-host convergence is validated
- operational self-host workflows are available and covered by repository gates

Validation baseline:
- `make test-legacy-rebased`
- `make test-all-0-30`
- focused bootstrap/self-host/operational runners

## Completed Roadmap Summary

### FASE 0-10
Foundation, lexer, parser, semantic core, executable subset, sigilo system, modules, advanced typing.

### FASE 11-20
Standard-library growth, formatter, linter, project workflows, documentation generator, freestanding/ASM bridge, language-surface expansion, application-library stack.

### FASE 21-29
Bootstrap closure in CCT:
- FASE 21: bootstrap lexer
- FASE 22-23: bootstrap parser and syntax-surface completion
- FASE 24-25: semantic analysis and generic semantics
- FASE 26-28: bootstrap code generation, structural types, advanced control flow, `FORMA`, and generic materialization
- FASE 29: stage0/stage1/stage2 self-host convergence

### FASE 30
Operational self-hosted platform:
- self-hosted workflow targets
- validated self-hosted project flows
- mature `csv`, `https`, and `orm_lite`
- final release and handoff package for the bootstrap era

## Architectural Principles

- Determinism first
- Explicit phase boundaries
- Bootstrap and host paths must remain comparable and testable
- No hidden regressions in compatibility or validation tooling
- Operational workflows are part of the product, not optional scripts

## What Comes After FASE 30

Post-FASE-30 work should be organized around platform maturity rather than bootstrap enablement.

Recommended tracks:
1. Make the self-hosted compiler the primary developer-facing toolchain.
2. Expand self-hosted parity for project commands and stdlib coverage.
3. Improve performance, diagnostics quality, and packaging discipline.
4. Introduce new language/library features only when they fit the operational platform model.

## Operational Definition of Done

CCT is considered operationally healthy when all of the following remain green:
- legacy rebased regression suite
- bootstrap regression suite
- self-host regression suite
- operational platform suite
- stage-identity validation

That is the practical contract for future phases.
