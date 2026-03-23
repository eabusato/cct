#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"

if [ -z "${CCT_BIN:-}" ]; then
  if [ -x ./cct-host ]; then
    CCT_BIN="./cct-host"
  else
    CCT_BIN="./cct"
  fi
fi

if [ ! -x ./cct_lexer_bootstrap ]; then
  echo "ERROR: ./cct_lexer_bootstrap nao encontrado"
  exit 1
fi

normalize_tokens_table_21e1() {
  local in_file="$1"
  local out_file="$2"
  awk '
    FNR > 3 && NF >= 3 {
      loc = $1
      type = $2
      lex = ""
      for (i = 3; i <= NF; i++) {
        if (i > 3) {
          lex = lex " "
        }
        lex = lex $i
      }
      sub(/^"/, "", lex)
      sub(/"$/, "", lex)
      print loc "|" type "|" lex
    }
  ' "$in_file" > "$out_file"
}

collect_files_21e1() {
  if [ "$#" -gt 0 ]; then
    printf '%s\n' "$@"
    return
  fi

  find lib/cct tests/integration -name '*.cct' | sort
}

PASS=0
FAIL=0
TOTAL=0
INDEX=0

echo "========================================="
echo "FASE 21E1: Full Suite Validation"
echo "========================================="

while IFS= read -r file; do
  [ -n "$file" ] || continue
  TOTAL=$((TOTAL + 1))
  INDEX=$((INDEX + 1))

  c_raw="$CCT_TMP_DIR/lexer_full_${INDEX}_c.raw"
  b_raw="$CCT_TMP_DIR/lexer_full_${INDEX}_b.raw"
  c_norm="$CCT_TMP_DIR/lexer_full_${INDEX}_c.norm"
  b_norm="$CCT_TMP_DIR/lexer_full_${INDEX}_b.norm"
  diff_out="$CCT_TMP_DIR/lexer_full_${INDEX}.diff"

  "$CCT_BIN" --tokens "$file" >"$c_raw" 2>"$CCT_TMP_DIR/lexer_full_${INDEX}_c.err"
  c_status=$?

  ./cct_lexer_bootstrap "$file" >"$b_raw" 2>"$CCT_TMP_DIR/lexer_full_${INDEX}_b.err"
  b_status=$?

  normalize_tokens_table_21e1 "$c_raw" "$c_norm"
  normalize_tokens_table_21e1 "$b_raw" "$b_norm"

  if diff -u "$c_norm" "$b_norm" >"$diff_out"; then
    PASS=$((PASS + 1))
  else
    echo "FAIL: $file (tokens divergem)"
    echo "  status lexer C:   $c_status"
    echo "  status lexer CCT: $b_status"
    if [ -f "$diff_out" ]; then
      head -20 "$diff_out"
    fi
    FAIL=$((FAIL + 1))
  fi
done < <(collect_files_21e1 "$@")

echo ""
echo "========================================="
echo "Resultados:"
echo "  Total:  $TOTAL"
echo "  PASS:   $PASS"
echo "  FAIL:   $FAIL"
echo "========================================="

if [ "$FAIL" -eq 0 ]; then
  echo "SUCESSO: Todos arquivos tokenizaram identicamente"
  exit 0
fi

echo "FALHA: $FAIL arquivos divergem"
exit 1
