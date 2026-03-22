# FASE 29 - Self-Hosting and Multi-Stage Convergence - Release Notes

**Date:** 2026-03-21
**Version:** 0.29.0
**Status:** Completed

## Executive Summary

FASE 29 achieved practical self-hosting. The phase built the explicit stage0/stage1/stage2 pipeline, validated convergence and identity, added regression execution under the bootstrap compiler, and recorded performance metrics for the first complete self-host cycle.

## Scope Closed

Delivered subphases:
- stage0 build pipeline
- stage1 self-compilation
- stage2 recompilation and convergence check
- stage identity / artifact-diff validation
- bootstrap-compiler regression execution
- performance capture and critical-path tuning

## Key Deliverables

Implementation and operations delivered in:
- `Makefile` stage targets (`bootstrap-stage0`, `bootstrap-stage1`, `bootstrap-stage2`, `bootstrap-stage-identity`, related helpers)
- `tools/gen_selfhost_aliases.awk`
- bootstrap/runtime self-host bridge updates
- `tests/run_tests.sh` phase-29 gates
- `out/bootstrap/phase29/bench/metrics.txt`

## Major Technical Decisions

- stage convergence is treated as a formal engineering gate, not as an anecdotal milestone
- identity validation focuses on semantically relevant outputs (generated C, runnable artifacts, convergence) rather than pretending platform binary formats are byte-identical everywhere
- self-host aliasing/support layers are explicit and checked into the repository as part of the operational build chain

## Validation Summary

The phase closed with:
- green stage identity target
- stage1/stage2 convergence checks
- bootstrap regression execution under the self-host compiler
- recorded benchmark metrics for stage0/stage1/stage2

Representative measurements captured during phase closure:
- `stage0_real_seconds=7.45`
- `stage1_real_seconds=10.16`
- `stage2_real_seconds=11.42`

## User-Facing Impact

This phase moved CCT from “has a bootstrap compiler” to “can compile itself in a repeatable, validated pipeline.” That is the real self-hosting milestone.

## Residual Limits at Phase Close

The compiler could self-host, but operational use as a day-to-day primary toolchain still required packaging, workflows, mature application-library coverage, and final release/handoff discipline.

## Transition to FASE 30

FASE 30 could assume:
- self-host compilation works
- stage identity is testable
- repository build automation knows how to produce converged self-host artifacts
