# Sigilo Troubleshooting (FASE 13B.4)

This playbook helps reduce MTTR for `--sigilo-check` failures in local and CI flows.

## Report format contract

Operational report signature:

- `format=cct.sigilo.report.v1`
- short mode by default (`--sigilo-report summary`)
- detailed mode on demand (`--sigilo-report detailed`)
- explain mode on demand (`--sigilo-explain`)

## Common incidents

### 1) Baseline missing

Symptoms:

- `sigilo.ci ... status=missing`
- release profile blocks with exit `2`

Actions:

1. Generate or refresh artifact (`cct build|test|bench ...`).
2. Create baseline explicitly:
   - `./cct sigilo baseline update <artifact.sigil> --baseline <path>`
3. Re-run the same project command with `--sigilo-check`.

### 2) Baseline corrupted

Symptoms:

- baseline parse/metadata validation errors
- exit `1`

Actions:

1. Validate baseline source file ownership and content.
2. Run explicit check for diagnostics:
   - `./cct sigilo baseline check <artifact.sigil> --baseline <path> --format structured`
3. Recreate baseline with explicit review agreement:
   - `./cct sigilo baseline update <artifact.sigil> --baseline <path> --force`

### 3) Canonical hash drift (`semantic_hash`/`system_hash`)

Symptoms:

- highest severity `review-required`
- `gated` and `release` profiles block with exit `2`

Actions:

1. Inspect details:
   - `./cct build --project . --sigilo-check --sigilo-ci-profile gated --sigilo-report detailed --sigilo-explain`
2. Confirm expected code/module changes.
3. Update baseline only after review approval.

### 4) Schema incompatibility

Symptoms:

- unsupported format/schema diagnostics
- exit `1`

Actions:

1. Confirm artifact/baseline produced by compatible CCT revision.
2. Re-generate artifacts and baseline with current toolchain.
3. If migration is needed, perform controlled baseline refresh in a dedicated change.

### 5) Permission failure

Symptoms:

- read/write errors for artifact or baseline paths
- exit `1`

Actions:

1. Verify file permissions and directory access.
2. Ensure CI user can read artifact and baseline sidecar (`.baseline.meta`).
3. Re-run command with absolute baseline path to avoid path ambiguity.

## Fast triage commands

- Summary mode (default):
  - `./cct build --project . --sigilo-check --sigilo-ci-profile advisory`
- Detailed mode:
  - `./cct build --project . --sigilo-check --sigilo-ci-profile gated --sigilo-report detailed`
- Explain mode:
  - `./cct build --project . --sigilo-check --sigilo-ci-profile release --sigilo-explain`
