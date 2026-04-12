#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"

LEAK_FILE="${CCT_VALGRIND_FILE:-tests/integration/codegen_minimal.cct}"
LOG_FILE="$CCT_TMP_DIR/valgrind_lexer_21e3.log"

{
  echo "========================================="
  echo "FASE 21E3: Memory Leak Check"
  echo "========================================="
  echo ""
  echo "Arquivo: $LEAK_FILE"
  echo ""
} | tee "$LOG_FILE"

if [ ! -x ./cct_lexer_bootstrap ]; then
  echo "ERROR: ./cct_lexer_bootstrap nao encontrado" | tee -a "$LOG_FILE"
  exit 1
fi

if [ ! -f "$LEAK_FILE" ]; then
  echo "ERROR: $LEAK_FILE nao encontrado" | tee -a "$LOG_FILE"
  exit 1
fi

if ! command -v valgrind >/dev/null 2>&1; then
  echo "SKIP: valgrind indisponivel neste ambiente" | tee -a "$LOG_FILE"
  exit 0
fi

if valgrind --leak-check=full \
            --show-leak-kinds=all \
            --track-origins=yes \
            --error-exitcode=1 \
            --log-file="$LOG_FILE.raw" \
            ./cct_lexer_bootstrap "$LEAK_FILE" >/dev/null 2>&1; then
  echo "PASS: Nenhum leak detectado" | tee -a "$LOG_FILE"
  echo "" | tee -a "$LOG_FILE"
  echo "Heap summary:" | tee -a "$LOG_FILE"
  grep "HEAP SUMMARY" -A 3 "$LOG_FILE.raw" | tee -a "$LOG_FILE"
  exit 0
fi

echo "FAIL: Memory leaks ou invalid access detectados" | tee -a "$LOG_FILE"
cat "$LOG_FILE.raw" | tee -a "$LOG_FILE"
exit 1
