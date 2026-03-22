# FASE 26 - Handoff

**Date:** 2026-03-21
**Status:** Completed
**Next Phase:** 27

## What Is Stable

- bootstrap codegen context and emitter model
- expression and statement lowering for the core subset
- declaration/program emission path
- foundational CCT-to-C type mapping
- runtime bridge / translation-unit support
- end-to-end generated-C compile/run validation for the foundational subset

## Contracts the Next Phase Can Rely On

- code generation should continue to flow through the emitter/context model rather than bypassing it
- phase-local codegen work is validated through generated C and host compilation, not only textual inspection
- runtime bridge responsibilities are explicit and central, not hidden inside arbitrary emitters

## Open Boundaries Carried Forward

- structured data lowering
- advanced selection lowering
- generic code generation and advanced control flow

## Expected Use by FASE 27

FASE 27 should extend the existing backend infrastructure with structural data lowering rather than creating a second backend path for `SIGILLUM` and `ORDO`.
