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
  "$CCT_BIN" "$src" >$CCT_TMP_DIR/cct_15a3_compile.out 2>&1
  local exe="${src%.cct}"
  set +e
  "$exe" >$CCT_TMP_DIR/cct_15a3_run.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne "$expected" ]; then
    echo "unexpected exit code for $src: got $rc expected $expected"
    return 1
  fi
}

run_exit_code_test "tests/integration/frange_iterum_series_15a.cct" 30
run_exit_code_test "tests/integration/recede_iterum_series_15a.cct" 12
run_exit_code_test "tests/integration/frange_iterum_no_corrupt_15a.cct" 42
run_exit_code_test "tests/integration/iterum_nested_frange_recede_15a.cct" 26

echo "phase15a3 iterum-control contract: PASS"
