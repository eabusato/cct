#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"

BENCH_FILE="${CCT_BENCH_FILE:-lib/cct/fluxus.cct}"
ITERATIONS="${CCT_BENCH_ITERATIONS:-100}"
LOG_FILE="$CCT_TMP_DIR/benchmark_lexer_21e2.latest.log"

if [ ! -x ./cct_lexer_bootstrap ]; then
  echo "ERROR: ./cct_lexer_bootstrap nao encontrado" | tee "$LOG_FILE"
  exit 1
fi

if [ ! -f "$BENCH_FILE" ]; then
  echo "ERROR: $BENCH_FILE nao encontrado" | tee "$LOG_FILE"
  exit 1
fi

measure_seconds() {
  local cmd="$1"
  TIMEFORMAT=%R
  { time bash -lc "$cmd"; } 2>&1 >/dev/null | tail -1
}

TIME_C=$(measure_seconds "for ((i=0; i<$ITERATIONS; i++)); do ./cct --tokens '$BENCH_FILE' >/dev/null 2>&1; done")
TIME_CCT=$(measure_seconds "for ((i=0; i<$ITERATIONS; i++)); do ./cct_lexer_bootstrap '$BENCH_FILE' >/dev/null 2>&1; done")
PER_C=$(awk -v t="$TIME_C" -v n="$ITERATIONS" 'BEGIN { printf "%.4f", t / n }')
PER_CCT=$(awk -v t="$TIME_CCT" -v n="$ITERATIONS" 'BEGIN { printf "%.4f", t / n }')
RATIO=$(awk -v a="$TIME_C" -v b="$TIME_CCT" 'BEGIN { if (a == 0) { print "inf" } else { printf "%.2f", b / a } }')

{
  echo "========================================="
  echo "FASE 21E2: Performance Baseline"
  echo "========================================="
  echo ""
  echo "Arquivo: $BENCH_FILE"
  echo "Iteracoes: $ITERATIONS"
  echo ""
  echo "Benchmarking Lexer C..."
  echo "  Tempo total: ${TIME_C}s"
  echo "  Tempo por iteracao: ${PER_C}s"
  echo ""
  echo "Benchmarking Lexer CCT..."
  echo "  Tempo total: ${TIME_CCT}s"
  echo "  Tempo por iteracao: ${PER_CCT}s"
  echo ""
  echo "========================================="
  echo "Ratio: ${RATIO}x"
  echo "========================================="
} | tee "$LOG_FILE"

if awk -v r="$RATIO" 'BEGIN { exit !(r + 0 < 10.0) }'; then
  echo "PASS: Ratio aceitavel (< 10x)" | tee -a "$LOG_FILE"
  exit 0
fi

echo "FAIL: Ratio muito alto (>= 10x)" | tee -a "$LOG_FILE"
exit 1
