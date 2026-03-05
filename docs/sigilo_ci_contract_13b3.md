# Sigilo CI Contract (FASE 13B.3)

This document defines the canonical CI gate policy for project-level sigilo checks.

## Scope

- Integrates with project commands (`cct build`, `cct test`, `cct bench`) via:
  - `--sigilo-check`
  - `--sigilo-ci-profile advisory|gated|release`
  - `--sigilo-baseline PATH` (optional override)
  - `--sigilo-override-behavioral-risk` (explicit override with audit warning)
- Baseline updates remain explicit-only (`cct sigilo baseline update ...`).

## Profiles

- `advisory`
  - Non-blocking for `none`, `informational`, `review-required`
  - Blocking for `behavioral-risk` unless override is explicit
- `gated`
  - Blocking for `review-required` and `behavioral-risk`
  - Non-blocking for `none`, `informational`
- `release`
  - Requires baseline presence
  - Blocking for `review-required` and `behavioral-risk`
  - Non-blocking for `none`; `informational` is warning-only

## Severity Decision Table

| Highest severity   | advisory | gated | release |
|--------------------|----------|-------|---------|
| `none`             | pass     | pass  | pass    |
| `informational`    | pass     | pass  | pass (warning) |
| `review-required`  | pass (warning) | fail (`2`) | fail (`2`) |
| `behavioral-risk`  | fail (`2`) unless explicit override | fail (`2`) unless explicit override | fail (`2`) unless explicit override |

## Exit Code Contract

- `0`: gate passed
- `2`: policy block (profile decision)
- `1`: invalid artifact/baseline contract or execution error

## Audit and Override

- Override is only accepted through `--sigilo-override-behavioral-risk`.
- Override always emits an audit-visible warning.
- Override does not mutate baseline or metadata files.

## CI Reproduction Commands

Advisory:

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile advisory
```

Gated:

```bash
./cct test --project . --sigilo-check --sigilo-ci-profile gated
```

Release:

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile release
```

Behavioral-risk override (exception flow):

```bash
./cct build --project . --sigilo-check --sigilo-ci-profile advisory --sigilo-override-behavioral-risk
```
