#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

source "$ROOT/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT"

if [ -z "${CCT_BIN:-}" ]; then
  if [ -x ./cct-host ]; then
    CCT_BIN="./cct-host"
  else
    CCT_BIN="./cct"
  fi
fi

run_generated_case() {
  local name="$1"
  local expr="$2"
  local expected="$3"
  local src="$CCT_TMP_DIR/cct_${name}.cct"
  local exe="$CCT_TMP_DIR/cct_${name}"

  cat > "$src" <<EOF
INCIPIT grimoire "${name}"

RITUALE main() REDDE REX
  EVOCA REX x AD 1
  SI ${expr}
    REDDE 1
  FIN SI
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
EOF

  rm -f "$exe" "$exe.cgen.c" "$exe.sigil" "$exe.svg" "$exe.system.sigil" "$exe.system.svg"
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15b3_paren_compile.out 2>&1

  set +e
  "$exe" >$CCT_TMP_DIR/cct_15b3_paren_run.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $name: got $rc expected $expected"
    return 1
  fi
}

build_nested_expr() {
  local depth="$1"
  local open=""
  local close=""
  for _ in $(seq 1 "$depth"); do
    open="${open}("
    close=")${close}"
  done
  printf "%sx == 1%s" "$open" "$close"
}

build_series_expr() {
  local terms="$1"
  local expr=""
  for i in $(seq 1 "$terms"); do
    if [ "$i" -eq 1 ]; then
      expr="((x == 1))"
    else
      expr="${expr} ET ((x == 1))"
    fi
  done
  printf "%s" "$expr"
}

build_or_chain_expr() {
  local terms="$1"
  local expr=""
  for i in $(seq 1 "$terms"); do
    if [ "$i" -eq 1 ]; then
      expr="((x == 0))"
    elif [ "$i" -eq "$terms" ]; then
      expr="${expr} VEL ((x == 1))"
    else
      expr="${expr} VEL ((x == 0))"
    fi
  done
  printf "%s" "$expr"
}

NESTED_256="$(build_nested_expr 256)"
NESTED_1024="$(build_nested_expr 1024)"
SERIES_256="$(build_series_expr 256)"
ORCHAIN_256="$(build_or_chain_expr 256)"

run_generated_case "paren_nested_256" "$NESTED_256" 1
run_generated_case "paren_nested_1024" "$NESTED_1024" 1
run_generated_case "paren_series_256" "$SERIES_256" 1
run_generated_case "paren_or_chain_256" "$ORCHAIN_256" 1

echo "phase15b3 parentheses-stress contract: PASS"
