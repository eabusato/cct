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

run_exit_code_test() {
  local src="$1"
  local expected="$2"
  cleanup_codegen_artifacts "$src"
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15a1_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15a1_run.out 2>&1
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
  if "$CCT_BIN" --check "$src" >$CCT_TMP_DIR/cct_15a1_check.out 2>&1; then
    echo "expected semantic failure for $src"
    return 1
  fi
  if ! grep -q "$needle" $CCT_TMP_DIR/cct_15a1_check.out; then
    echo "missing expected diagnostic for $src: $needle"
    cat $CCT_TMP_DIR/cct_15a1_check.out
    return 1
  fi
}

run_exit_code_test "tests/integration/frange_dum_basic_15a.cct" 3
run_exit_code_test "tests/integration/recede_dum_basic_15a.cct" 18
run_exit_code_test "tests/integration/frange_donec_basic_15a.cct" 4
run_exit_code_test "tests/integration/recede_donec_basic_15a.cct" 4
run_exit_code_test "tests/integration/frange_dum_nested_15a.cct" 6
run_exit_code_test "tests/integration/frange_recede_dum_paths_15a.cct" 8

run_semantic_fail_test "tests/integration/frange_outside_loop_15a.cct" "FRANGE outside loop context"
run_semantic_fail_test "tests/integration/recede_outside_loop_15a.cct" "RECEDE outside loop context"

echo "phase15a1 loop-control contract: PASS"
