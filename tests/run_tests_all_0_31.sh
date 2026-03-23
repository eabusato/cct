#!/bin/bash
#
# CCT — Clavicula Turing
# Full test runner aggregator
#
# Runs the restored legacy suite (phases 0-20) and then the current
# bootstrap/self-hosted suites (phases 21-31) without changing the
# current tests/run_tests.sh contract.

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1

run_block() {
    local label="$1"
    shift

    echo ""
    echo "========================================"
    echo "$label"
    echo "========================================"

    "$@"
    local rc=$?
    if [ "$rc" -ne 0 ]; then
        echo "[full-runner] block failed: $label" >&2
        exit "$rc"
    fi
}

run_block "LEGACY 0-20 (rebased)" bash tests/run_tests_legacy_0_20_rebased.sh
run_block "BOOTSTRAP 21-28" env -u CCT_TEST_PHASES CCT_TEST_GROUP=bootstrap bash tests/run_tests.sh
run_block "SELFHOST 29" env -u CCT_TEST_PHASES CCT_TEST_GROUP=bootstrap-selfhost bash tests/run_tests.sh
run_block "OPERATIONAL 30" env -u CCT_TEST_GROUP CCT_TEST_PHASES=30A,30B,30C,30D,30E bash tests/run_tests.sh
run_block "PROMOTION 31" env -u CCT_TEST_GROUP CCT_TEST_PHASES=31A,31B,31C,31D,31E bash tests/run_tests.sh

echo ""
echo "[full-runner] all blocks passed"
exit 0
