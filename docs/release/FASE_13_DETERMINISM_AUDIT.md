# FASE 13 Determinism Audit (13D.2)

## Scope

This report records the deterministic audit required by FASE 13D.2.
The audit covers the sigilo-tooling chain delivered in 13A-13D.1 and validates deterministic behavior for parsing, diff/check, baseline checks, and validator outputs.

## Audit Plan

Runner:
- `tests/run_phase13_determinism_audit.sh`

Execution model:
- repeated execution per scenario (`CCT_DETERMINISM_REPEAT`, default `5`)
- byte-level output comparison between run 1 and all subsequent runs
- expected exit code validation where applicable
- volatile-field scan over generated deterministic outputs

Approval criteria:
- all repeated runs in each scenario must be byte-equivalent
- expected decision/exit code must be stable across repetitions
- no volatile tokens in deterministic output artifacts

## Scenarios

1. `sigilo inspect --format structured --summary` on stable input
2. `sigilo diff --format structured --summary` on stable pair
3. `sigilo baseline check --format structured --summary` on stable baseline
4. `sigilo baseline check --format structured --summary --strict` on drifted baseline (stable blocking decision)
5. `sigilo check --format structured --summary --strict` on identical inputs
6. `sigilo validate --format structured --summary` on additive-compatible input
7. volatile-field absence scan in deterministic output artifacts

## Environment Controls

The runner enforces:
- `LC_ALL=C`
- `TZ=UTC`

This minimizes locale/timezone noise and improves reproducibility.

## Result Consolidation

The runner writes a technical report to:
- `/tmp/cct_phase13_determinism_audit/audit_report.txt`

Expected summary fields:
- scenario inventory
- repetition count
- pass/fail totals
- failed case inventory (if any)

## Known Limits

- The audit targets determinism contracts for sigilo-tooling output behavior only.
- It intentionally does not benchmark performance or runtime throughput.

## Integration Notes

- Integrated into `tests/run_tests.sh` as FASE 13D.2 checks.
- Feeds final closure gates planned for 13D.3/13D.4.
