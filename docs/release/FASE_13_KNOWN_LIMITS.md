# FASE 13 Known Limits

This document records explicit operational limits for the FASE 13 release line.

## Limits

### LIMIT-13-001 — Schema major upgrade blocking in strict profile

Description:
- `strict-contract` intentionally blocks unsupported higher schema versions (`cct.sigil.v2+`).

Rationale:
- release gates must fail closed on unsupported schema contracts.

Workaround:
- use tolerant/current profile for exploratory reading, or upgrade consumer implementation.

### LIMIT-13-002 — Analytical metadata remains experimental

Description:
- analytical blocks introduced in 13C.2 are additive and deterministic, but remain classified as experimental in stability governance.

Rationale:
- preserve compatibility while allowing iterative enrichment under explicit review.

Workaround:
- consumers should treat these blocks as optional and avoid hard failure on absence/presence.

### LIMIT-13-003 — Baseline governance requires explicit operator action

Description:
- baseline updates are never implicit; overwrite requires `--force`.

Rationale:
- protect reviewability and avoid silent drift acceptance.

Workaround:
- use explicit update workflows after human review.

### LIMIT-13-004 — Determinism audit scope is output-contract focused

Description:
- 13D.2 audit validates deterministic output contracts; it is not a performance benchmark suite.

Rationale:
- deterministic correctness and reproducibility are prioritized for release gating.

Workaround:
- use dedicated benchmark flows for performance analysis.

## References

- `docs/release/FASE_13_RELEASE_NOTES.md`
