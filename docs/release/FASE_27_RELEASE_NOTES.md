# FASE 27 - Structural Bootstrap Codegen - Release Notes

**Date:** 2026-03-21
**Version:** 0.27.0
**Status:** Completed

## Executive Summary

FASE 27 closed structural data lowering in the bootstrap backend. The phase implemented `SIGILLUM` code generation, simple and payload `ORDO` lowering, and `ELIGE` lowering over scalar, string, and tagged payload cases.

## Scope Closed

Delivered subphases:
- `SIGILLUM` materialization as C `struct`
- simple `ORDO` materialization as C `enum`
- payload `ORDO` materialization as tagged unions / structured compound literals
- `ELIGE` lowering over integers, booleans, strings, simple `ORDO`, and payload `ORDO`
- structural end-to-end codegen gates

## Key Deliverables

Implementation concentrated in:
- `src/bootstrap/codegen/codegen_struct.cct`
- `src/bootstrap/codegen/codegen_ordo.cct`
- `src/bootstrap/codegen/codegen_elige.cct`
- `src/bootstrap/codegen/codegen_stmt.cct`
- `src/bootstrap/codegen/codegen_expr.cct`
- `src/bootstrap/codegen/codegen_context.cct`

## Major Technical Decisions

- `ORDO` payload cases were lowered as explicit tagged unions instead of backend-side implicit conventions
- `ELIGE` lowering uses the correct underlying C strategy by subject type (`switch`, `strcmp` chain, or tag-switch)
- payload binding in `CASUS` was treated as a first-class lowering concern, with unsupported combinations rejected explicitly rather than miscompiled silently

## Validation Summary

The phase closed with dedicated tests for:
- `SIGILLUM` type emission
- simple `ORDO` emission and usage
- payload `ORDO` constructors and returns
- `ELIGE` over scalar, string, simple enum, and payload enum subjects
- structural codegen end-to-end gates

## User-Facing Impact

User-facing structured language constructs now have a validated bootstrap backend path. This was the point at which the bootstrap compiler stopped being limited to mostly scalar/procedural code generation.

## Residual Limits at Phase Close

Generic materialization in codegen, advanced control flow lowering, and `FORMA` still remained open for FASE 28.

## Transition to FASE 28

FASE 28 could rely on:
- stable structural type lowering
- explicit payload-tag model
- selection lowering patterns ready to coexist with more advanced control-flow lowering
