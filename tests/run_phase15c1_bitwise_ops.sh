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
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15c1_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15c1_run.out 2>&1
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
  "$CCT_BIN" --check "$src" >$CCT_TMP_DIR/cct_15c1_check.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "expected semantic failure for $src"
    return 1
  fi
  if ! grep -q "$needle" $CCT_TMP_DIR/cct_15c1_check.out; then
    echo "missing expected diagnostic for $src: $needle"
    cat $CCT_TMP_DIR/cct_15c1_check.out
    return 1
  fi
}

run_exit_code_test "tests/integration/bitwise_and_basic_15c.cct" 8
run_exit_code_test "tests/integration/bitwise_or_basic_15c.cct" 14
run_exit_code_test "tests/integration/bitwise_xor_basic_15c.cct" 6
run_exit_code_test "tests/integration/bitwise_flag_operations_15c.cct" 1
run_exit_code_test "tests/integration/bitwise_xor_toggle_15c.cct" 7
run_semantic_fail_test "tests/integration/bitwise_type_error_15c.cct" "require integer operands"

echo "phase15c1 bitwise-ops contract: PASS"
