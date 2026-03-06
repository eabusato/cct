# FASE 15 Release Notes

## Summary

FASE 15 closes the language-surface consolidation cycle before bootstrap work.  
The phase is completed at **15D.4**, with full regression green and public contracts updated.

## Highlights

### 15A — Loop-Control Completion

- `FRANGE` and `RECEDE` are stable across `DUM`, `DONEC`, `REPETE`, and `ITERUM`.
- `RECEDE` behavior in `REPETE` preserves increment semantics.
- Outside-loop diagnostics for loop-control statements are enforced semantically.

### 15B — Logical Operator Finalization

- Logical `ET` and `VEL` finalized with short-circuit semantics.
- Precedence contract stabilized (`NON > ET > VEL`) with deep-parentheses support.
- Comparator + logical integration paths validated under regression.

### 15C — Bitwise/Shift Family Stabilization

- Stable operators: `ET_BIT`, `VEL_BIT`, `XOR`, `NON_BIT`, `SINISTER`, `DEXTER`.
- Integer-operand enforcement is explicit and regression-protected.
- Integration scenarios (bitwise + control flow + logical contexts) are stable.

### 15D — `CONSTANS` Finalization

- Semantic reassignment enforcement is stable for locals and rituale parameters.
- Generated C emits `const` consistently in supported paths.
- `CONSTANS SPECULUM` pointer-binding behavior is closed and documented.
- Edge-case closure tests and phase-governance gate completed in 15D.4.

## Compatibility Notes

- No intentional breaking change was introduced to the stable CLI surface.
- Existing historical language/tooling contracts remain valid.
- FASE 16 is now the official start of the bootstrap-oriented track.

## References

- `docs/spec.md`
- `docs/architecture.md`
- `docs/roadmap.md`
- `md_out/FASE_15D4_CCT.md`
- `md_out/docs/release/FASE_15D4_PHASE15_CLOSURE_SUMMARY.md`
