#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"
CCT_BIN="$ROOT_DIR/cct"
TMP_DIR="$CCT_TMP_DIR/cct_phase14c1_regression"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

if [ ! -x "$CCT_BIN" ]; then
  echo "phase14c1: missing compiler binary at $CCT_BIN" >&2
  exit 1
fi

echo "phase14c1: risk-block A (sigilo contract + strict exits)"
cat > "$TMP_DIR/base.sigil" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
[totals]
rituale = 1
SIGEOF
cat > "$TMP_DIR/drift.sigil" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 2222222222222222
[totals]
rituale = 1
SIGEOF

set +e
"$CCT_BIN" sigilo check "$TMP_DIR/base.sigil" "$TMP_DIR/drift.sigil" --strict --summary >$CCT_TMP_DIR/cct_phase14c1_check.out 2>&1
RC=$?
set -e
if [ "$RC" -ne 2 ]; then
  echo "phase14c1: strict sigilo check did not return exit code 2" >&2
  exit 1
fi

"$CCT_BIN" sigilo baseline update "$TMP_DIR/base.sigil" --baseline "$TMP_DIR/baseline.sigil" --force >$CCT_TMP_DIR/cct_phase14c1_update.out 2>&1
set +e
"$CCT_BIN" sigilo baseline check "$TMP_DIR/drift.sigil" --baseline "$TMP_DIR/baseline.sigil" --strict --summary >$CCT_TMP_DIR/cct_phase14c1_baseline_check.out 2>&1
RC=$?
set -e
if [ "$RC" -ne 2 ]; then
  echo "phase14c1: strict baseline check drift did not return exit code 2" >&2
  exit 1
fi

echo "phase14c1: risk-block B (13M + parser/semantic stability)"
"$CCT_BIN" --check "$ROOT_DIR/examples/math_common_ops_13m.cct" >$CCT_TMP_DIR/cct_phase14c1_math_check.out 2>&1
"$CCT_BIN" --tokens "$ROOT_DIR/examples/math_common_ops_13m.cct" >$CCT_TMP_DIR/cct_phase14c1_math_tokens.out 2>&1
if ! grep -q "STAR_STAR" $CCT_TMP_DIR/cct_phase14c1_math_tokens.out; then
  echo "phase14c1: expected STAR_STAR token not found in 13M sample" >&2
  exit 1
fi

echo "phase14c1: risk-block C (project workflow + doc/sigilo options parsing)"
"$CCT_BIN" build --project "$ROOT_DIR/tests/integration/project_12f_basic" --sigilo-check --sigilo-report summary >$CCT_TMP_DIR/cct_phase14c1_build.out 2>&1
"$CCT_BIN" doc --project "$ROOT_DIR/tests/integration/project_12f_basic" --format markdown --no-timestamp >$CCT_TMP_DIR/cct_phase14c1_doc.out 2>&1

echo "phase14c1: ok"
