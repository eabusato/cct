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
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15a2_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15a2_run.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $src: got $rc expected $expected"
    return 1
  fi
}

run_exit_code_test "tests/integration/frange_repete_basic_15a.cct" 4
run_exit_code_test "tests/integration/recede_repete_basic_15a.cct" 25
run_exit_code_test "tests/integration/frange_repete_gradus_15a.cct" 6
run_exit_code_test "tests/integration/recede_repete_no_skip_increment_15a.cct" 5
run_exit_code_test "tests/integration/frange_repete_nested_15a.cct" 8
run_exit_code_test "tests/integration/frange_recede_repete_paths_15a.cct" 13
run_exit_code_test "tests/integration/recede_frange_repete_nested_15a.cct" 8

echo "phase15a2 repete-control contract: PASS"
