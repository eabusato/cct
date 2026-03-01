# CCT Linter (FASE 12E.2)

`cct lint` is the canonical static-quality tool for CCT sources.

## Commands

- `./cct lint <file.cct>`
- `./cct lint --strict <file.cct>`
- `./cct lint --fix <file.cct>`
- `./cct lint --quiet <file.cct>`
- `./cct lint --fix --format-after-fix <file.cct>`

## Exit Codes

- `0`: lint completed (warnings are allowed in non-strict mode)
- `1`: parse/semantic failure blocks lint
- `2`: warnings found in `--strict`
- `3`: internal lint/fix failure

## Rule Set (12E.2 Baseline)

- `unused-variable`
- `unused-parameter`
- `unused-import`
- `dead-code-after-return`
- `dead-code-after-throw`
- `shadowing-local`

## Safe Auto-Fix Scope

`--fix` is intentionally limited to deterministic edits:

- remove unused import lines (`unused-import`)
- remove single-line statements after `REDDE` or `IACE`

Anything outside this safe subset is reported only.

## Typical CI Workflow

```bash
./cct lint --strict tests/integration/lint_clean_ok_12e2.cct
./cct fmt --check tests/integration/lint_clean_ok_12e2.cct
```
