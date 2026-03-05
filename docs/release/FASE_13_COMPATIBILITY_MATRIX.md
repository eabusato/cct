# FASE 13 Compatibility Matrix

## Scope

Compatibility commitments for sigilo tooling delivered in FASE 13.

## Contracts

| Surface | Compatibility Level | Contract |
|---|---|---|
| `.sigil` schema identifier | stable | canonical format is `cct.sigil.v1` in FASE 13 |
| Additive unknown fields | backward-compatible | warning-only in tolerant/current profiles |
| Higher schema (`cct.sigil.v2+`) | profile-dependent | tolerant/current: fallback with warning; strict-contract: blocking mismatch |
| `--strict` | stable alias | maps to strict-contract behavior for consumer/validator commands |
| Baseline files | stable | check is read-only; update is explicit (`--force` for overwrite) |
| CI profile gates | stable | advisory/gated/release maintain deterministic exit behavior |

## Consumer Profile Behavior

| Scenario | legacy-tolerant | current-default | strict-contract |
|---|---|---|---|
| Unknown additive top-level field | warning, non-blocking | warning, non-blocking | warning, non-blocking |
| Higher schema format (`v2+`) | warning fallback | warning fallback | blocking error |
| Missing required core field | blocking (validation) | blocking (validation) | blocking (validation) |
| Diff severity `review-required` in CI | advisory: non-blocking | advisory: non-blocking | gated/release: blocking |
| Diff severity `behavioral-risk` in CI | blocking by default | blocking by default | blocking |

## CLI Compatibility Notes

Stable command families in FASE 13:
- `cct sigilo inspect`
- `cct sigilo diff`
- `cct sigilo check`
- `cct sigilo baseline check|update`
- `cct sigilo validate`
- `cct build|test|bench --sigilo-check ...`

## References

- `docs/sigilo_schema_13c1.md`
- `docs/sigilo_consumer_compat_13c3.md`
- `docs/sigilo_ci_contract_13b3.md`
- `docs/release/FASE_13_STABILITY_MATRIX.md`
