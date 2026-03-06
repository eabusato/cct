#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"
CCT_BIN="$ROOT_DIR/cct"
TMP_DIR="$CCT_TMP_DIR/cct_phase14c2_stress"
ROUNDS="${1:-8}"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

if [ ! -x "$CCT_BIN" ]; then
  echo "phase14c2: missing compiler binary at $CCT_BIN" >&2
  exit 1
fi

FAILURES=0

cat > "$TMP_DIR/valid.sigil" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = c2c2c2c2c2c2c2c2
[totals]
rituale = 1
SIGEOF

for i in $(seq 1 "$ROUNDS"); do
  "$CCT_BIN" --check "$ROOT_DIR/tests/integration/codegen_minimal.cct" >$CCT_TMP_DIR/cct_phase14c2_check_"$i".out 2>&1 || FAILURES=$((FAILURES + 1))
  "$CCT_BIN" --sigilo-only "$ROOT_DIR/tests/integration/sigilo_minimal.cct" >$CCT_TMP_DIR/cct_phase14c2_sigilo_"$i".out 2>&1 || FAILURES=$((FAILURES + 1))
  "$CCT_BIN" sigilo validate "$TMP_DIR/valid.sigil" --summary >$CCT_TMP_DIR/cct_phase14c2_validate_"$i".out 2>&1 || FAILURES=$((FAILURES + 1))
done

cat > "$TMP_DIR/report.txt" <<EOF
phase=14C2
rounds=$ROUNDS
failures=$FAILURES
status=$( [ "$FAILURES" -eq 0 ] && echo stable || echo unstable )
EOF

if [ "$FAILURES" -ne 0 ]; then
  echo "phase14c2: instability detected, failures=$FAILURES" >&2
  exit 1
fi

echo "phase14c2: stable (rounds=$ROUNDS)"
