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
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15b4_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15b4_run.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $src: got $rc expected $expected"
    return 1
  fi
}

run_exit_code_test "tests/integration/et_with_comparators_15b.cct" 1
run_exit_code_test "tests/integration/vel_with_comparators_15b.cct" 15
run_exit_code_test "tests/integration/comparator_before_et_precedence_15b.cct" 1
run_exit_code_test "tests/integration/et_vel_arith_complex_15b.cct" 1

echo "phase15b4 comparator-integration contract: PASS"
