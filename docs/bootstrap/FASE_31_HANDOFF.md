# FASE 31 Handoff

## Title

FASE 31 - Self-Hosted Compiler Promotion

## Closure Statement

FASE 31 closes the transition from validated self-hosting to promoted operational self-hosting.

The compiler written in CCT is no longer only a bootstrap artifact. It now participates directly in the user-facing compiler-entry model through the promoted wrapper path.

## What Was Delivered

### 31A - Self-Host Wrapper Parity
- explicit self-host wrapper exposed as `cct-selfhost`
- output handling, stdlib resolution, and direct compile workflows aligned with the current operational contract
- wrapper-level compiler identity reporting via `--which-compiler`

### 31B - CLI Contract Parity
- core compile-facing CLI surface aligned across wrappers
- `--check`, `--ast`, and `--tokens` carried through the promoted path
- explicit host fallback retained where the repository still treats tooling as host-backed

### 31C - Project Workflow Parity
- `build`, `run`, `test`, `bench`, `clean`, and package-oriented workflows available through the promoted wrapper model
- phase-30 operational project scenarios preserved under the phase-31 compiler-entry model

### 31D - Promotion and Demotion Infrastructure
- `make bootstrap-promote`
- `make bootstrap-demote`
- explicit host and self-host wrappers preserved
- default mode made reversible rather than destructive

### 31E - Default Switch and Final Gate
- final wrapper-mode validation closed
- self-hosted compiler promotion treated as a tested operational state, not an informal convention

## Operational Model After Handoff

Users should now understand the repository this way:
- `./cct`: default wrapper
- `./cct-host`: explicit host fallback
- `./cct-selfhost`: explicit self-host path
- `./cct --which-compiler`: inspect the current default mode

## Validation Model After Handoff

Recommended daily validation:

```bash
make bootstrap-stage-identity
make test
make test-host-legacy
```

Recommended release validation:

```bash
make bootstrap-stage-identity
make test
make test-host-legacy
make test-all-0-31
make test-phase30-final
make test-phase31-final
```

## Residual Limits

FASE 31 is a compiler-promotion phase, not a declaration that every tooling path is already self-host-native.

Residual limits that remain legitimate after handoff:
- selected tooling commands may still delegate to host implementation layers
- host fallback remains part of the supported architecture
- promotion success should continue to be judged by gates, not by assumption

## What Future Work Should Not Reopen

Future phases should not reopen:
- whether the host compiler must remain available as fallback
- whether the wrapper layer exists at all
- whether promotion/demotion should be destructive

Those questions are closed by FASE 31.

## Forward Pointer

Post-FASE-31 work should focus on platform maturation:
- diagnostics quality
- performance optimization
- stdlib parity and promotion of still-host-backed surfaces
- tooling parity where it is worth carrying into the self-host implementation
