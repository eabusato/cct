# Sigilo Consumer Compatibility Guide (FASE 13C.3)

This guide defines cross-version compatibility behavior for `.sigil` consumers.

## Consumer profiles

| Profile | CLI selection | Contract |
|---|---|---|
| `legacy-tolerant` | `--consumer-profile legacy-tolerant` | Prioritizes read continuity; additive fields are warning-only |
| `current-default` | `--consumer-profile current-default` (default) | Canonical default for FASE 13 tooling |
| `strict-contract` | `--consumer-profile strict-contract` or `--strict` | Enforces schema/version/hash contract with blocking errors |

## Fallback policy

| Scenario | legacy-tolerant | current-default | strict-contract |
|---|---|---|---|
| Unknown additive field | warning, continue | warning, continue | warning, continue |
| Missing required field by scope/hash | warning, continue | warning, continue | error, fail |
| Higher schema in same family (`cct.sigil.v2+`) | warning + fallback to v1-compatible read | warning + fallback to v1-compatible read | error (`schema_mismatch`) |
| Incompatible non-family schema format | error | error | error |

## Migration checklist for external consumers

1. Keep parser tolerant to unknown additive fields.
2. Validate `sigilo_scope` and the corresponding required hash (`semantic_hash` or `system_hash`).
3. If consuming higher schema in `cct.sigil.vN`, either:
   - run in tolerant mode with explicit warning handling, or
   - run strict and block until consumer support is updated.
4. Prefer `visual_engine`; keep `sigilo_style` as legacy alias only.
5. Treat analytical sections (`analysis_summary`, `diff_fingerprint_context`, `module_structural_summary`, `compatibility_hints`) as optional/additive.

## Robust reader examples

### Legacy-compatible read

```bash
./cct sigilo inspect artifact.sigil --summary --consumer-profile legacy-tolerant
```

### Current default read

```bash
./cct sigilo inspect artifact.sigil --summary --consumer-profile current-default
```

### Strict contract gate

```bash
./cct sigilo inspect artifact.sigil --strict --summary
```
