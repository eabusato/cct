# FASE 30 - Operational Self-Hosted Platform - Release Notes

**Date:** 2026-03-21
**Version:** 0.30.0
**Status:** Completed

## Summary

Self-hosted workflows, operational stdlib/runtime subset, application libraries, packaging, and final platform release closed.

## Scope Closed

- implementation completed for the phase objective
- integration tests added and validated
- runner integration aligned with the repository validation model
- project-facing documentation synchronized

## Deliverables

- code in the relevant compiler/bootstrap/runtime/library areas
- integration fixtures under `tests/integration/`
- runner coverage in the project test harness
- release and handoff documentation for the phase
- `project-selfhost-build`
- `project-selfhost-run`
- `project-selfhost-test`
- `project-selfhost-package`
- converged compiler artifact lineage including `cct_stage2`

## Validation Snapshot

- phase-specific gates completed during implementation
- repository validation kept green at the time of phase closure
- current cross-era validation path available via `make test-all-0-30`

## Transition

This phase is closed and handed off to the next phase in the roadmap.
