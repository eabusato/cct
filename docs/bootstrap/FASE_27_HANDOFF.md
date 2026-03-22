# FASE 27 - Handoff

**Date:** 2026-03-21
**Status:** Completed
**Next Phase:** 28

## What Is Stable

- `SIGILLUM` lowering
- simple and payload `ORDO` lowering
- `ELIGE` lowering over scalar, string, and tagged-union subjects
- payload binding model for supported `CASUS` forms
- structural end-to-end backend validation

## Contracts the Next Phase Can Rely On

- structural types now have explicit C-level representation in the bootstrap backend
- payload tags are a real backend contract, not an inferred convention
- unsupported destructuring combinations fail explicitly instead of silently miscompiling

## Open Boundaries Carried Forward

- generic materialization in codegen
- advanced loop/failure-control lowering
- `FORMA`

## Expected Use by FASE 28

FASE 28 should compose with the structural data model already delivered here, especially for mixed control-flow and generic materialization scenarios.
