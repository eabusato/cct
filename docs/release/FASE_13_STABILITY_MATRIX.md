# FASE 13 Stability Matrix

## Scope

This matrix classifies the operational stability of all major surfaces delivered in FASE 13 (13A-13D.2), as consolidated in 13D.3.

## Component Status Manifest

- SIGILO_CLI: stable
- SIGILO_BASELINE_WORKFLOW: stable
- SIGILO_CI_GATES: stable
- SIGILO_SCHEMA_GOVERNANCE: stable
- SIGILO_ANALYTICAL_METADATA: experimental
- SIGILO_CONSUMER_PROFILES: stable
- SIGILO_VALIDATOR_STRICT_TOLERANT: stable
- SIGILO_REGRESSION_MATRIX: stable
- SIGILO_DETERMINISM_AUDIT: stable

## Detailed Matrix

| Component | Status | Since | Evidence | Notes |
|---|---|---|---|---|
| `sigilo inspect|diff|check` CLI contract | stable | 13A.3 | `tests/run_tests.sh` (13A.3 + 13D.1 + 13D.2) | command/format contract maintained |
| Baseline check/update contract | stable | 13A.4 | `tests/run_tests.sh` (13A.4 + 13D.1 + 13D.2) | explicit update, read-only check |
| Project sigilo opt-in workflow | stable | 13B.2 | `tests/run_tests.sh` (13B.2 + 13D.1) | opt-in only, no default flow regression |
| CI profiles (`advisory/gated/release`) | stable | 13B.3 | `tests/run_tests.sh` (13B.3 + 13D.1) | deterministic gate behavior |
| Sigilo report/explain observability | stable | 13B.4 | `tests/run_tests.sh` (13B.3/13B.4) | stable operator-oriented report signature |
| Schema governance (`cct.sigil.v1`) | stable | 13C.1 | `docs/sigilo_schema_13c1.md`, tests 13C.1 | additive evolution policy |
| Analytical metadata blocks | experimental | 13C.2 | tests 13C.2 + 13D.2 | additive and deterministic, still marked experimental |
| Consumer compatibility profiles | stable | 13C.3 | tests 13C.3 + 13D.1 | strict/tolerant fallback contract |
| Strict/tolerant validator (`sigilo validate`) | stable | 13C.4 | tests 13C.4 + 13D.1 + 13D.2 | deterministic error/warning policy |
| Regression matrix suite | stable | 13D.1 | `tests/run_phase13_regression.sh` | mandatory phase-13 regression safety net |
| Determinism audit suite | stable | 13D.2 | `tests/run_phase13_determinism_audit.sh` | repeated byte-equivalence checks |

## Classification Rules

- `stable`: behavior and contract are expected to remain compatible in the current release line.
- `experimental`: additive surface with active governance and stricter review before compatibility commitments are widened.

## References

- `docs/release/FASE_13_FINAL_SNAPSHOT.md`
- `docs/release/FASE_13_COMPATIBILITY_MATRIX.md`
- `docs/release/FASE_13_KNOWN_LIMITS.md`
- `docs/release/FASE_13_RELEASE_NOTES.md`
