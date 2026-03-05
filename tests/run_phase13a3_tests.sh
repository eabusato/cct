#!/bin/bash
#
# CCT — FASE 13A.3 test runner
#

set -euo pipefail

echo "FASE 13A.3 — Sigilo CLI Tests"
echo "========================================"

SIG_A="/tmp/cct_sigilo_cli_13a3_a.sigil"
SIG_B="/tmp/cct_sigilo_cli_13a3_b.sigil"
SIG_C="/tmp/cct_sigilo_cli_13a3_c.sigil"

cat > "$SIG_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

cat > "$SIG_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

cat > "$SIG_C" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 2
SIGEOF

./cct sigilo inspect "$SIG_A" --format structured --summary >/tmp/cct_sigilo_cli_13a3_inspect.out
if ! grep -q "format = cct.sigil.inspect.v1" /tmp/cct_sigilo_cli_13a3_inspect.out; then
  echo "inspect structured output mismatch"
  exit 1
fi

./cct sigilo diff "$SIG_A" "$SIG_B" --format text --summary >/tmp/cct_sigilo_cli_13a3_diff.out
if ! grep -q "highest=behavioral-risk" /tmp/cct_sigilo_cli_13a3_diff.out; then
  echo "diff summary severity mismatch"
  exit 1
fi

set +e
./cct sigilo check "$SIG_A" "$SIG_B" --strict --summary >/tmp/cct_sigilo_cli_13a3_check.out
RC=$?
set -e
if [ "$RC" -ne 2 ]; then
  echo "expected strict check rc=2, got rc=$RC"
  exit 1
fi

set +e
./cct sigilo check "$SIG_A" "$SIG_C" --strict --summary >/tmp/cct_sigilo_cli_13a3_check_info.out
RC=$?
set -e
if [ "$RC" -ne 0 ]; then
  echo "expected informational strict check rc=0, got rc=$RC"
  exit 1
fi

echo "FASE 13A.3 tests: ok"
