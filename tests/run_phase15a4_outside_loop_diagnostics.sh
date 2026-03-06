#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

source "$ROOT/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT"

CCT_BIN="./cct"

cleanup_codegen_artifacts() {
  local src="$1"
  local exe="${src%.cct}"
  rm -f "$exe" "$exe.cgen.c" "$exe.sigil" "$exe.svg" "$exe.system.sigil" "$exe.system.svg"
  rm -f "$exe".__mod_*.sigil "$exe".__mod_*.svg
}

run_semantic_fail_contract() {
  local src="$1"
  local keyword="$2"

  cleanup_codegen_artifacts "$src"

  set +e
  "$CCT_BIN" --check "$src" >$CCT_TMP_DIR/cct_15a4_check.out 2>&1
  local check_rc=$?
  set -e
  if [ "$check_rc" -eq 0 ]; then
    echo "expected semantic failure for $src in --check"
    return 1
  fi

  if ! grep -q "error:" $CCT_TMP_DIR/cct_15a4_check.out; then
    echo "missing canonical error marker in --check output for $src"
    cat $CCT_TMP_DIR/cct_15a4_check.out
    return 1
  fi
  if ! grep -E -q ":[0-9]+:[0-9]+: error:" $CCT_TMP_DIR/cct_15a4_check.out; then
    echo "missing line/column location in --check output for $src"
    cat $CCT_TMP_DIR/cct_15a4_check.out
    return 1
  fi
  if ! grep -q "$keyword" $CCT_TMP_DIR/cct_15a4_check.out; then
    echo "missing keyword '$keyword' in semantic diagnostic for $src"
    cat $CCT_TMP_DIR/cct_15a4_check.out
    return 1
  fi
  if ! grep -qi "loop" $CCT_TMP_DIR/cct_15a4_check.out; then
    echo "missing loop-context hint in semantic diagnostic for $src"
    cat $CCT_TMP_DIR/cct_15a4_check.out
    return 1
  fi

  set +e
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15a4_compile.out 2>&1
  local compile_rc=$?
  set -e
  if [ "$compile_rc" -eq 0 ]; then
    echo "expected non-zero compile exit code for $src"
    return 1
  fi
}

run_exit_code_test() {
  local src="$1"
  local expected="$2"

  cleanup_codegen_artifacts "$src"
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15a4_compile_ok.out 2>&1

  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15a4_run_ok.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $src: got $rc expected $expected"
    return 1
  fi
}

run_semantic_fail_contract "tests/integration/frange_outside_rituale_15a.cct" "FRANGE"
run_semantic_fail_contract "tests/integration/recede_outside_rituale_15a.cct" "RECEDE"
run_semantic_fail_contract "tests/integration/frange_inside_si_outside_loop_15a.cct" "FRANGE"
run_semantic_fail_contract "tests/integration/recede_inside_si_outside_loop_15a.cct" "RECEDE"
run_exit_code_test "tests/integration/frange_inside_si_inside_loop_15a.cct" 3
run_exit_code_test "tests/integration/recede_inside_si_inside_loop_15a.cct" 13

echo "phase15a4 outside-loop diagnostics contract: PASS"
