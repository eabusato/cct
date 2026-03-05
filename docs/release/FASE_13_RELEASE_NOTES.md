# FASE 13 Release Notes

## Summary

FASE 13 expands sigilo from a static artifact into an operational tooling surface with deterministic contracts for inspection, drift control, CI gating, and release audit.

## Subphase Highlights

### 13A — Inspection, Diff, and Baseline Foundations

- canonical reader runtime (`test_sigil_parse`)
- structural diff runtime (`test_sigil_diff`)
- operational CLI: `sigilo inspect|diff|check`
- baseline workflow: `sigilo baseline check|update`

### 13B — Workflow and CI Integration

- local/project opt-in sigilo checks (`build|test|bench --sigilo-check`)
- CI profiles (`advisory`, `gated`, `release`) with deterministic exit behavior
- operator-facing report/explain observability contract

### 13C — Governance and Compatibility

- schema governance (`cct.sigil.v1`) and additive policy
- analytical metadata block expansion (additive)
- consumer profiles (`legacy-tolerant`, `current-default`, `strict-contract`)
- strict/tolerant validator via `sigilo validate`

### 13D.1 / 13D.2 — Regression and Determinism Closure Foundations

- dedicated phase-13 regression matrix runner
- dedicated determinism audit runner with repeated byte-equivalence checks
- release audit documentation integrated in `docs/release`

## Operational Commands

- `./cct sigilo inspect <artifact.sigil> --summary`
- `./cct sigilo diff <left.sigil> <right.sigil> --summary`
- `./cct sigilo check <left.sigil> <right.sigil> --strict --summary`
- `./cct sigilo baseline check <artifact.sigil> --baseline <path> --summary`
- `./cct sigilo baseline update <artifact.sigil> --baseline <path> --force`
- `./cct sigilo validate <artifact.sigil> --summary --consumer-profile current-default`

## Known Limits Carried in This Release

- LIMIT-13-001
- LIMIT-13-002
- LIMIT-13-003
- LIMIT-13-004

See: `docs/release/FASE_13_KNOWN_LIMITS.md`.

## Verification Artifacts

- `tests/run_phase13_regression.sh`
- `tests/run_phase13_determinism_audit.sh`
- `docs/release/FASE_13_FINAL_SNAPSHOT.md`
- `docs/release/FASE_13_STABILITY_MATRIX.md`
- `docs/release/FASE_13_COMPATIBILITY_MATRIX.md`
