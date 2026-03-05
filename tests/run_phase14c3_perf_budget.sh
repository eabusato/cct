#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CCT_BIN="$ROOT_DIR/cct"
TMP_DIR="/tmp/cct_phase14c3_perf"
ROUNDS="${1:-3}"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

if [ ! -x "$CCT_BIN" ]; then
  echo "phase14c3: missing compiler binary at $CCT_BIN" >&2
  exit 1
fi

cat > "$TMP_DIR/valid.sigil" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

measure_ms() {
  local cmd="$1"
  local total=0
  local i
  for i in $(seq 1 "$ROUNDS"); do
    local start end delta
    start=$(date +%s%N)
    eval "$cmd" >/dev/null 2>&1
    end=$(date +%s%N)
    delta=$(( (end - start) / 1000000 ))
    total=$((total + delta))
  done
  echo $(( total / ROUNDS ))
}

HELP_MS=$(measure_ms "\"$CCT_BIN\" --help")
CHECK_MS=$(measure_ms "\"$CCT_BIN\" --check \"$ROOT_DIR/tests/integration/codegen_minimal.cct\"")
VALIDATE_MS=$(measure_ms "\"$CCT_BIN\" sigilo validate \"$TMP_DIR/valid.sigil\" --summary")

# Conservative budgets for local/CI variability.
BUDGET_HELP_MS=4000
BUDGET_CHECK_MS=10000
BUDGET_VALIDATE_MS=7000

cat > "$TMP_DIR/baseline.txt" <<EOF
phase=14C3
rounds=$ROUNDS
help_avg_ms=$HELP_MS
check_avg_ms=$CHECK_MS
validate_avg_ms=$VALIDATE_MS
budget_help_ms=$BUDGET_HELP_MS
budget_check_ms=$BUDGET_CHECK_MS
budget_validate_ms=$BUDGET_VALIDATE_MS
EOF

if [ "$HELP_MS" -gt "$BUDGET_HELP_MS" ] || [ "$CHECK_MS" -gt "$BUDGET_CHECK_MS" ] || [ "$VALIDATE_MS" -gt "$BUDGET_VALIDATE_MS" ]; then
  echo "phase14c3: budget exceeded (help=$HELP_MS, check=$CHECK_MS, validate=$VALIDATE_MS)" >&2
  exit 1
fi

echo "phase14c3: budget ok (help=$HELP_MS, check=$CHECK_MS, validate=$VALIDATE_MS)"

