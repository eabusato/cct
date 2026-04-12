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

cleanup_codegen_artifacts() {
  local src="$1"
  local exe="${src%.cct}"
  rm -f "$exe" "$exe.cgen.c" "$exe.sigil" "$exe.svg" "$exe.system.sigil" "$exe.system.svg"
  rm -f "$exe".__mod_*.sigil "$exe".__mod_*.svg
}

run_exit_code_test() {
  local src="$1"
  local expected="$2"
  cleanup_codegen_artifacts "$src"
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15ab_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15ab_run.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $src: got $rc expected $expected"
    return 1
  fi
}

run_semantic_fail_test() {
  local src="$1"
  local needle="$2"
  set +e
  "$CCT_BIN" --check "$src" >$CCT_TMP_DIR/cct_15ab_check.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "expected semantic failure for $src"
    return 1
  fi
  if ! grep -q "$needle" $CCT_TMP_DIR/cct_15ab_check.out; then
    echo "missing expected diagnostic for $src: $needle"
    cat $CCT_TMP_DIR/cct_15ab_check.out
    return 1
  fi
}

if [ $# -ne 1 ]; then
  echo "usage: $0 <case-id>"
  exit 2
fi

case "$1" in
  dum_et_frange)
    run_exit_code_test "tests/integration/cross_dum_et_frange_15ab.cct" 6
    ;;
  dum_vel_recede)
    run_exit_code_test "tests/integration/cross_dum_vel_recede_15ab.cct" 13
    ;;
  donec_et_recede)
    run_exit_code_test "tests/integration/cross_donec_et_recede_15ab.cct" 4
    ;;
  donec_vel_frange)
    run_exit_code_test "tests/integration/cross_donec_vel_frange_15ab.cct" 3
    ;;
  repete_et_recede)
    run_exit_code_test "tests/integration/cross_repete_et_recede_15ab.cct" 27
    ;;
  repete_vel_frange)
    run_exit_code_test "tests/integration/cross_repete_vel_frange_15ab.cct" 5
    ;;
  iterum_et_frange)
    run_exit_code_test "tests/integration/cross_iterum_et_frange_15ab.cct" 2
    ;;
  iterum_vel_recede)
    run_exit_code_test "tests/integration/cross_iterum_vel_recede_15ab.cct" 9
    ;;
  nested_mix_loops)
    run_exit_code_test "tests/integration/cross_nested_mix_loops_15ab.cct" 22
    ;;
  short_circuit_et_anur)
    run_exit_code_test "tests/integration/cross_short_circuit_et_anur_15ab.cct" 0
    ;;
  short_circuit_vel_anur)
    run_exit_code_test "tests/integration/cross_short_circuit_vel_anur_15ab.cct" 1
    ;;
  semantic_et_string)
    run_semantic_fail_test "tests/integration/cross_semantic_et_string_15ab.cct" "operator ET requires boolean or integer operands"
    ;;
  semantic_vel_string)
    run_semantic_fail_test "tests/integration/cross_semantic_vel_string_15ab.cct" "operator VEL requires boolean or integer operands"
    ;;
  precedence_vel_et_dum)
    run_exit_code_test "tests/integration/cross_precedence_vel_et_dum_15ab.cct" 1
    ;;
  *)
    echo "unknown case-id: $1"
    exit 2
    ;;
esac

echo "phase15ab cross-case '$1': PASS"
