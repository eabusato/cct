#!/bin/bash
#
# CCT — FASE 13D.2 determinism audit runner
#

set -u -o pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CCT_BIN="$ROOT_DIR/cct"
FIX_ROOT="$ROOT_DIR/tests/integration/phase13_regression_13d1"
TMP_DIR="/tmp/cct_phase13_determinism_audit"
REPORT_FILE="$TMP_DIR/audit_report.txt"
REPEAT_COUNT="${CCT_DETERMINISM_REPEAT:-5}"

export LC_ALL=C
export TZ=UTC

TESTS_TOTAL=0
TESTS_FAILED=0
FAILED_NAMES=()

pass() {
    local name="$1"
    echo "[PASS] $name"
}

fail() {
    local name="$1"
    local detail="$2"
    echo "[FAIL] $name :: $detail"
    TESTS_FAILED=$((TESTS_FAILED + 1))
    FAILED_NAMES+=("$name")
}

log() {
    echo "$1" >> "$REPORT_FILE"
}

run_repeated_success_bytes() {
    local name="$1"
    shift
    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    local ref_out="$TMP_DIR/${name}.ref.out"
    "$@" > "$ref_out" 2>&1
    local code=$?
    if [ "$code" -ne 0 ]; then
        fail "$name" "reference execution failed (exit=$code)"
        return
    fi

    local i
    for i in $(seq 2 "$REPEAT_COUNT"); do
        local run_out="$TMP_DIR/${name}.run${i}.out"
        "$@" > "$run_out" 2>&1
        local run_code=$?
        if [ "$run_code" -ne 0 ]; then
            fail "$name" "run=$i failed (exit=$run_code)"
            return
        fi
        if ! cmp -s "$ref_out" "$run_out"; then
            diff -u "$ref_out" "$run_out" > "$TMP_DIR/${name}.run${i}.diff" || true
            fail "$name" "byte mismatch on run=$i (see $TMP_DIR/${name}.run${i}.diff)"
            return
        fi
    done

    pass "$name"
}

run_repeated_expected_exit_bytes() {
    local name="$1"
    local expected_exit="$2"
    shift 2
    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    local ref_out="$TMP_DIR/${name}.ref.out"
    "$@" > "$ref_out" 2>&1
    local code=$?
    if [ "$code" -ne "$expected_exit" ]; then
        fail "$name" "reference exit mismatch (expected=$expected_exit got=$code)"
        return
    fi

    local i
    for i in $(seq 2 "$REPEAT_COUNT"); do
        local run_out="$TMP_DIR/${name}.run${i}.out"
        "$@" > "$run_out" 2>&1
        local run_code=$?
        if [ "$run_code" -ne "$expected_exit" ]; then
            fail "$name" "run=$i exit mismatch (expected=$expected_exit got=$run_code)"
            return
        fi
        if ! cmp -s "$ref_out" "$run_out"; then
            diff -u "$ref_out" "$run_out" > "$TMP_DIR/${name}.run${i}.diff" || true
            fail "$name" "byte mismatch on run=$i (see $TMP_DIR/${name}.run${i}.diff)"
            return
        fi
    done

    pass "$name"
}

if [ ! -x "$CCT_BIN" ]; then
    echo "[ERROR] cct binary not found: $CCT_BIN"
    exit 1
fi

if ! [[ "$REPEAT_COUNT" =~ ^[0-9]+$ ]] || [ "$REPEAT_COUNT" -lt 2 ]; then
    echo "[ERROR] invalid CCT_DETERMINISM_REPEAT='$REPEAT_COUNT' (expected integer >= 2)"
    exit 1
fi

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

cat > "$REPORT_FILE" <<'REPOF'
FASE 13D.2 Determinism Audit Report
===================================
REPOF

log "runner: tests/run_phase13_determinism_audit.sh"
log "repeat_count: $REPEAT_COUNT"
log "environment: LC_ALL=$LC_ALL TZ=$TZ"
log ""
log "scenarios:"
log "- inspect structured summary determinism"
log "- diff structured summary determinism"
log "- baseline check stable structured determinism"
log "- baseline check strict drift determinism (exit=2)"
log "- sigilo check strict structured determinism"
log "- validate structured determinism"
log "- absence of volatile fields in deterministic outputs"

BASE_SIG="$TMP_DIR/base.sigil"
REVIEW_SIG="$TMP_DIR/review.sigil"
INFO_SIG="$TMP_DIR/info.sigil"
BASELINE_SIG="$TMP_DIR/baseline.sigil"
BASELINE_DRIFT_SIG="$TMP_DIR/baseline_drift.sigil"

cat > "$BASE_SIG" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
module_count = 1
module_resolution_status = ok

[analysis_summary]
scope = local

[module_structural_summary]
module_count = 1
module_resolution_status = ok
SIGEOF

cp "$BASE_SIG" "$INFO_SIG"
echo "future_optional = additive" >> "$INFO_SIG"

cat > "$REVIEW_SIG" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
module_count = 1
module_resolution_status = ok

[analysis_summary]
scope = local

[module_structural_summary]
module_count = 1
module_resolution_status = ok
SIGEOF

"$CCT_BIN" sigilo baseline update "$BASE_SIG" --baseline "$BASELINE_SIG" --force >/tmp/cct_phase13d2_baseline_update.out 2>&1
cp "$BASELINE_SIG" "$BASELINE_DRIFT_SIG"
sed -i 's/semantic_hash = .*/semantic_hash = deadbeefcafebabe/' "$BASELINE_DRIFT_SIG"

echo "FASE 13D.2 — Determinism Audit"
echo "========================================"

run_repeated_success_bytes \
    "inspect_structured_summary" \
    "$CCT_BIN" sigilo inspect "$BASE_SIG" --format structured --summary

run_repeated_success_bytes \
    "diff_structured_summary" \
    "$CCT_BIN" sigilo diff "$BASE_SIG" "$REVIEW_SIG" --format structured --summary

run_repeated_success_bytes \
    "baseline_check_structured_stable" \
    "$CCT_BIN" sigilo baseline check "$BASE_SIG" --baseline "$BASELINE_SIG" --format structured --summary

run_repeated_expected_exit_bytes \
    "baseline_check_structured_strict_drift" \
    2 \
    "$CCT_BIN" sigilo baseline check "$BASE_SIG" --baseline "$BASELINE_DRIFT_SIG" --format structured --summary --strict

run_repeated_success_bytes \
    "sigilo_check_structured_strict" \
    "$CCT_BIN" sigilo check "$BASE_SIG" "$BASE_SIG" --format structured --summary --strict

run_repeated_success_bytes \
    "sigilo_validate_structured" \
    "$CCT_BIN" sigilo validate "$INFO_SIG" --format structured --summary --consumer-profile legacy-tolerant

TESTS_TOTAL=$((TESTS_TOTAL + 1))
if rg -n "timestamp|generated_at|time=|pid|nonce|uuid|session|random" "$TMP_DIR"/*.out >/tmp/cct_phase13d2_volatile_scan.out 2>&1; then
    fail "no_volatile_fields_in_outputs" "volatile token pattern detected"
else
    pass "no_volatile_fields_in_outputs"
fi

log ""
log "results: total=$TESTS_TOTAL failed=$TESTS_FAILED"
if [ "$TESTS_FAILED" -eq 0 ]; then
    log "status: pass"
else
    log "status: fail"
fi

if [ "$TESTS_FAILED" -ne 0 ]; then
    log "failed_cases:"
    for name in "${FAILED_NAMES[@]}"; do
        log "- $name"
    done
fi

echo ""
echo "----------------------------------------"
echo "13D.2 determinism summary: total=$TESTS_TOTAL failed=$TESTS_FAILED"
echo "report: $REPORT_FILE"
if [ "$TESTS_FAILED" -ne 0 ]; then
    echo "failed cases:"
    for name in "${FAILED_NAMES[@]}"; do
        echo "- $name"
    done
fi
echo "----------------------------------------"

if [ "$TESTS_FAILED" -eq 0 ]; then
    exit 0
fi
exit 1
