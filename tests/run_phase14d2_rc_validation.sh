#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"
TMP_DIR="$CCT_TMP_DIR/cct_phase14d2_rc"
DIST_DIR="$TMP_DIR/dist"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

make -C "$ROOT_DIR" dist DIST_DIR="$DIST_DIR" >$CCT_TMP_DIR/cct_phase14d2_dist.out 2>&1

if [ -x "$DIST_DIR/bin/cct" ]; then
  CCT_BIN="$DIST_DIR/bin/cct"
elif [ -f "$DIST_DIR/bin/cct.bat" ]; then
  CCT_BIN="$DIST_DIR/bin/cct.bat"
else
  echo "phase14d2: missing packaged cct entrypoint" >&2
  exit 1
fi

"$CCT_BIN" --version >$CCT_TMP_DIR/cct_phase14d2_version.out 2>&1
"$CCT_BIN" --help >$CCT_TMP_DIR/cct_phase14d2_help.out 2>&1

cat > "$TMP_DIR/rc_minimal.cct" <<'CCTEOF'
INCIPIT grimoire "rc"
RITUALE main() REDDE REX
  REDDE 0
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF

"$CCT_BIN" --check "$TMP_DIR/rc_minimal.cct" >$CCT_TMP_DIR/cct_phase14d2_check.out 2>&1
"$CCT_BIN" --sigilo-only "$TMP_DIR/rc_minimal.cct" >$CCT_TMP_DIR/cct_phase14d2_sigilo.out 2>&1

cat > "$TMP_DIR/rc_contract_valid.sigil" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1234567890abcdef
[totals]
rituale = 1
SIGEOF

"$CCT_BIN" sigilo inspect "$TMP_DIR/rc_contract_valid.sigil" --summary >$CCT_TMP_DIR/cct_phase14d2_inspect.out 2>&1
"$CCT_BIN" sigilo validate "$TMP_DIR/rc_contract_valid.sigil" --summary >$CCT_TMP_DIR/cct_phase14d2_validate.out 2>&1

if ! grep -q "Usage:" $CCT_TMP_DIR/cct_phase14d2_help.out; then
  echo "phase14d2: packaged help missing usage contract" >&2
  exit 1
fi
if ! grep -q "sigilo.inspect.summary" $CCT_TMP_DIR/cct_phase14d2_inspect.out; then
  echo "phase14d2: packaged sigilo inspect summary contract failed" >&2
  exit 1
fi
if ! grep -q "result=pass" $CCT_TMP_DIR/cct_phase14d2_validate.out; then
  echo "phase14d2: packaged sigilo validate summary did not pass" >&2
  exit 1
fi

echo "phase14d2: RC validation matrix ok"
