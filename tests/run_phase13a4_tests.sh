#!/bin/bash
#
# CCT — FASE 13A.4 test runner
#

set -euo pipefail

echo "FASE 13A.4 — Sigilo Baseline Tests"
echo "========================================"

SIG_A="/tmp/cct_sigilo_cli_13a4_a.sigil"
SIG_B="/tmp/cct_sigilo_cli_13a4_b.sigil"
BASE="/tmp/cct_sigilo_cli_13a4_baseline.sigil"

cat > "$SIG_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

cat > "$SIG_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
[totals]
rituale = 1
SIGEOF

./cct sigilo baseline check "$SIG_A" --baseline "$BASE" --summary >/tmp/cct_sigilo_cli_13a4_missing.out
if ! grep -q "status=missing" /tmp/cct_sigilo_cli_13a4_missing.out; then
  echo "expected missing baseline status"
  exit 1
fi

./cct sigilo baseline update "$SIG_A" --baseline "$BASE" >/tmp/cct_sigilo_cli_13a4_update.out
if ! grep -q "status=written" /tmp/cct_sigilo_cli_13a4_update.out; then
  echo "expected baseline update status=written"
  exit 1
fi

./cct sigilo baseline check "$SIG_A" --baseline "$BASE" --summary >/tmp/cct_sigilo_cli_13a4_ok.out
if ! grep -q "status=ok" /tmp/cct_sigilo_cli_13a4_ok.out; then
  echo "expected baseline check status=ok"
  exit 1
fi

set +e
./cct sigilo baseline check "$SIG_B" --baseline "$BASE" --strict --summary >/tmp/cct_sigilo_cli_13a4_strict.out
RC=$?
set -e
if [ "$RC" -ne 2 ]; then
  echo "expected strict drift rc=2, got rc=$RC"
  exit 1
fi

echo "FASE 13A.4 tests: ok"
