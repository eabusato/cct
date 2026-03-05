# Sigilo Operations Guide (FASE 14B.2)

This guide consolidates high-value sigilo operations for local development, CI checks, and troubleshooting.

## 1. Fast Local Validation

Validate one artifact quickly:

```bash
./cct sigilo validate path/to/module.sigil --summary
```

Strict contract validation:

```bash
./cct sigilo validate path/to/module.sigil --strict --format structured
```

## 2. Compare Two Artifacts

Human-readable drift summary:

```bash
./cct sigilo check before.sigil after.sigil --summary
```

Strict gate for review-required/behavioral-risk drift:

```bash
./cct sigilo check before.sigil after.sigil --strict --summary
```

Explain mode for operator guidance:

```bash
./cct sigilo check before.sigil after.sigil --strict --summary --explain
```

## 3. Baseline Workflow

Create/update baseline (explicit, never implicit):

```bash
./cct sigilo baseline update artifact.sigil --baseline docs/sigilo/baseline/main.sigil --force
```

Check artifact against baseline:

```bash
./cct sigilo baseline check artifact.sigil --baseline docs/sigilo/baseline/main.sigil --summary
```

Strict baseline gate (CI-oriented):

```bash
./cct sigilo baseline check artifact.sigil --baseline docs/sigilo/baseline/main.sigil --strict --summary
```

Troubleshooting hints for missing baseline or blocked drift:

```bash
./cct sigilo baseline check artifact.sigil --baseline docs/sigilo/baseline/main.sigil --strict --summary --explain
```

## 4. Project Workflow Integration

Accepted profile selector:

```bash
--sigilo-ci-profile advisory|gated|release
```

Advisory profile (non-blocking for review drift):

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile advisory
```

Gated profile:

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile gated --sigilo-baseline docs/sigilo/baseline/main.sigil
```

Release profile (strictest):

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile release --sigilo-baseline docs/sigilo/baseline/main.sigil --sigilo-explain
```

## 5. Consumer Compatibility Profiles

Legacy compatibility read:

```bash
./cct sigilo inspect artifact.sigil --consumer-profile legacy-tolerant --summary
```

Current default:

```bash
./cct sigilo inspect artifact.sigil --consumer-profile current-default --summary
```

Strict contract consumer:

```bash
./cct sigilo inspect artifact.sigil --consumer-profile strict-contract --summary
```

## 6. Determinism and Automation Notes

- Prefer `--summary` in automation unless full detail is required.
- Prefer `--format structured` for machine parsing.
- Use `--explain` only when operator guidance is needed (opt-in by design).
- Equivalent runs with equivalent inputs produce deterministic sigilo validation ordering.

## 7. Canonical Troubleshooting Entry

For common blocked scenarios and operator-facing diagnostics:

- `docs/sigilo_troubleshooting_13b4.md`
