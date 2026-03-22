# FASE 28 - Advanced Bootstrap Codegen - Release Notes

**Date:** 2026-03-21
**Version:** 0.28.0
**Status:** Completed

## Executive Summary

FASE 28 completed the bootstrap backend for the intended language surface by adding generic materialization in codegen, failure-control lowering, advanced loop-control lowering, and `FORMA` support.

## Scope Closed

Delivered subphases:
- generic instance materialization in the backend
- lowering of `TEMPTA`, `CAPE`, `SEMPER`, and `IACE`
- lowering of advanced loop-control constructs such as `FRANGE` and `RECEDE`
- parser/semantic/bootstrap support required by those constructs
- `FORMA` support in parser, semantic analysis, and backend lowering
- integrated validation gates for mixed advanced scenarios

## Key Deliverables

Implementation delivered in:
- `src/bootstrap/codegen/codegen_generic.cct`
- `src/bootstrap/codegen/codegen_failure.cct`
- `src/bootstrap/codegen/codegen_flow.cct`
- `src/bootstrap/codegen/codegen_forma.cct`
- supporting updates in bootstrap parser, semantic, and runtime code

## Major Technical Decisions

- generic code generation consumes semantic instance identity rather than recovering instance shape from AST text
- failure control is lowered through an explicit runtime strategy (`setjmp` / `longjmp` bridge) rather than pseudo-lowering hidden in statement codegen
- `FORMA` received a dedicated bootstrap representation and runtime-assisted formatting path instead of being treated as simple string concatenation

## Validation Summary

The phase closed with tests covering:
- generic code generation
- failure-control paths
- advanced loop-control interaction
- `FORMA` interpolation and error handling
- mixed end-to-end fixtures combining advanced constructs

## User-Facing Impact

After FASE 28, the bootstrap backend covered the intended practical language surface rather than only a reduced core. That made stage convergence in FASE 29 realistic.

## Residual Limits at Phase Close

The remaining challenge was no longer language coverage. The remaining challenge was build orchestration, multi-stage convergence, performance, and operationalization.

## Transition to FASE 29

FASE 29 could assume the bootstrap compiler was functionally complete enough to compile itself through multiple stages.
