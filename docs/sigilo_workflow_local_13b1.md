# CCT Sigilo Local Workflow (FASE 13B.1)

This guide defines the canonical local workflows for sigilo validation.

## 1) Minimal Daily Workflow

Use this loop during normal development for low friction:

1. `./cct fmt <files.cct...>`
2. `./cct lint <file.cct>`
3. `./cct build --project <dir>` or `./cct test --project <dir>`
4. `./cct sigilo baseline check <artifact.sigil> [--baseline <path>] --summary`

Recommended policy:
- keep baseline checks informative at first;
- update baseline only with explicit review agreement.

## 2) Strict Pre-merge Workflow

Use this loop before merge/release:

1. `./cct fmt --check <files.cct...>`
2. `./cct lint --strict <file.cct>`
3. `./cct test --project <dir>`
4. `./cct sigilo baseline check <artifact.sigil> --strict --summary`
5. `./cct doc --project <dir> --strict-docs` (when docs are in scope)

Strict behavior:
- baseline drift with severity `review-required` or `behavioral-risk` returns exit code `2`.

## 3) Progressive Adoption

- Week 1: informative mode (`baseline check` without strict gates)
- Week 2: gate `review-required` drift for selected branches
- Week 3+: strict workflow for protected branches and release candidates

## 4) Corrective Actions

When strict baseline check blocks:

1. inspect drift (`sigilo baseline check ... --format structured`)
2. review if drift is expected
3. update baseline explicitly if approved:
   `./cct sigilo baseline update <artifact.sigil> [--baseline <path>] --force`

Notes:
- `check` is read-only;
- `update` is the only operation that mutates baseline files.

## 5) Minimal vs Strict Matrix

- Use minimal workflow for fast local iteration and feature spikes.
- Use strict workflow before merge/release, or when branch policy requires deterministic baseline enforcement.
