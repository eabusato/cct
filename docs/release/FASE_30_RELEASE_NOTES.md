# FASE 30 - Operational Self-Hosted Platform - Release Notes

**Date:** 2026-03-21
**Version:** 0.30.0
**Status:** Completed

## Executive Summary

FASE 30 closed the bootstrap era by turning self-hosting into an operational platform baseline. The phase delivered self-hosted workflow targets, self-host-compatible runtime and stdlib support, mature application-library modules, project packaging/distribution flow, and the final release/handoff package for the phase-0-through-phase-30 baseline.

## Scope Closed

Delivered subphases:
- self-hosted toolchain as a supported developer workflow
- self-host-compatible stdlib/runtime path for the validated subset
- application libraries `cct/csv`, `cct/https`, and `cct/orm_lite`
- self-hosted project build/run/test/package workflows
- final operational validation and release package

## Key Deliverables

Implementation and operational deliverables include:
- `src/bootstrap/main_compiler.cct`
- `src/bootstrap/selfhost_prelude.cct`
- `src/bootstrap/selfhost_support.cct`
- `lib/cct/csv.cct`
- `lib/cct/https.cct`
- `lib/cct/orm_lite.cct`
- operational `Makefile` targets for self-hosted projects
- operational examples under `examples/phase30_data_app/`
- release and handoff artifacts for the full bootstrap era

## Major Technical Decisions

- the self-hosted compiler path is treated as an operationally supported path, not a demo-only side channel
- application-library additions were deliberately small and mature rather than broad and speculative
- release validation moved to aggregated multi-block runners that cover legacy, bootstrap, self-host, and operational suites together

## Validation Summary

The phase closed with:
- operational self-host project workflow gates
- phase-30 final runner coverage
- preserved stage identity from FASE 29
- full aggregated validation path `make test-all-0-30`

## User-Facing Impact

This is the first phase where CCT can be presented not only as a compiler project but as an operational self-hosted platform with realistic project workflows and mature application-library support.

## Residual Limits at Phase Close

Future work after FASE 30 should focus on platform maturity, parity improvements, performance, diagnostics, and ecosystem depth rather than bootstrap enablement.

## Release Outcome

FASE 30 defines the current publication baseline:
- host compiler and bootstrap compiler both validated
- self-hosted workflows operational
- full aggregated validation available from phase 0 through phase 30
