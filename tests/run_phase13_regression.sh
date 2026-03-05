#!/bin/bash
#
# CCT — FASE 13D.1 dedicated regression runner
#

set -u -o pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CCT_BIN="$ROOT_DIR/cct"
FIX_ROOT="$ROOT_DIR/tests/integration/phase13_regression_13d1"
TMP_DIR="/tmp/cct_phase13d1_regression"

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

run_cmd() {
    local name="$1"
    shift
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    if "$@" >/tmp/cct_phase13d1_last.out 2>&1; then
        pass "$name"
    else
        fail "$name" "exit=$?"
    fi
}

run_expect_exit() {
    local name="$1"
    local expected="$2"
    shift 2
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    "$@" >/tmp/cct_phase13d1_last.out 2>&1
    local code=$?
    if [ "$code" -eq "$expected" ]; then
        pass "$name"
    else
        fail "$name" "expected_exit=$expected got=$code"
    fi
}

run_expect_contains() {
    local name="$1"
    local expected_substr="$2"
    shift 2
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    local out
    out=$("$@" 2>&1)
    local code=$?
    if [ "$code" -eq 0 ] && echo "$out" | grep -q "$expected_substr"; then
        pass "$name"
    else
        fail "$name" "exit=$code missing='$expected_substr'"
    fi
}

if [ ! -x "$CCT_BIN" ]; then
    echo "[ERROR] cct binary not found: $CCT_BIN"
    exit 1
fi

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

echo "FASE 13D.1 — Sigilo Tooling Regression Suite"
echo "========================================"

# 1) Parser legacy + new schema compatibility contract
run_expect_contains \
    "parser accepts legacy schema v1" \
    "sigilo.inspect.summary" \
    "$CCT_BIN" sigilo inspect "$FIX_ROOT/artifacts/legacy_v1.sigil" --summary

run_expect_contains \
    "parser accepts expanded metadata in tolerant profile" \
    "sigilo.inspect.summary" \
    "$CCT_BIN" sigilo inspect "$FIX_ROOT/artifacts/expanded_metadata_v1.sigil" --summary --consumer-profile current-default

run_expect_exit \
    "strict-contract rejects higher schema artifact" \
    1 \
    "$CCT_BIN" sigilo inspect "$FIX_ROOT/artifacts/higher_schema_v2.sigil" --summary --consumer-profile strict-contract

# 2) diff severity coverage: none / informational / review / behavioral-risk
DIFF_BASE="$TMP_DIR/diff_base.sigil"
DIFF_INFO="$TMP_DIR/diff_info.sigil"
DIFF_REVIEW="$TMP_DIR/diff_review.sigil"
DIFF_BEHAV="$TMP_DIR/diff_behavior.sigil"

cat > "$DIFF_BASE" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

cp "$DIFF_BASE" "$DIFF_INFO"
echo "future_optional = additive" >> "$DIFF_INFO"
cp "$DIFF_BASE" "$DIFF_REVIEW"
sed -i 's/semantic_hash = .*/semantic_hash = 1111111111111111/' "$DIFF_REVIEW"
cat > "$DIFF_BEHAV" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

run_expect_contains \
    "diff none remains none" \
    "highest=none" \
    "$CCT_BIN" sigilo diff "$DIFF_BASE" "$DIFF_BASE" --summary

run_expect_contains \
    "diff informational is classified correctly" \
    "highest=informational" \
    "$CCT_BIN" sigilo diff "$DIFF_BASE" "$DIFF_INFO" --summary

run_expect_contains \
    "diff review-required is classified correctly" \
    "highest=review-required" \
    "$CCT_BIN" sigilo diff "$DIFF_BASE" "$DIFF_REVIEW" --summary

run_expect_contains \
    "diff behavioral-risk is classified correctly" \
    "highest=behavioral-risk" \
    "$CCT_BIN" sigilo diff "$DIFF_BASE" "$DIFF_BEHAV" --summary

# 3) baseline update/check with and without drift
BASELINE="$TMP_DIR/smoke_baseline.sigil"
run_cmd "baseline update writes baseline" "$CCT_BIN" sigilo baseline update "$DIFF_BASE" --baseline "$BASELINE" --force
run_expect_contains \
    "baseline check stable reports highest=none" \
    "highest=none" \
    "$CCT_BIN" sigilo baseline check "$DIFF_BASE" --baseline "$BASELINE" --summary

sed -i 's/semantic_hash = .*/semantic_hash = deadbeefcafebabe/' "$BASELINE"
run_expect_exit \
    "baseline strict blocks drift" \
    2 \
    "$CCT_BIN" sigilo baseline check "$DIFF_BASE" --baseline "$BASELINE" --strict --summary

# 4) CI profiles advisory/gated/release
PROJ="$TMP_DIR/project_ci"
cp -R "$FIX_ROOT/project" "$PROJ"
run_cmd "project build bootstrap" "$CCT_BIN" build --project "$PROJ"
SYS_SIGIL="$PROJ/src/main.system.sigil"
CI_BASE="$TMP_DIR/project_ci_base.sigil"
run_cmd "ci baseline update" "$CCT_BIN" sigilo baseline update "$SYS_SIGIL" --baseline "$CI_BASE" --force
sed -i 's/system_hash = .*/system_hash = 0123456789abcdef/' "$CI_BASE"

run_expect_exit \
    "advisory keeps review-required drift non-blocking" \
    0 \
    "$CCT_BIN" build --project "$PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$CI_BASE"

run_expect_exit \
    "gated blocks review-required drift" \
    2 \
    "$CCT_BIN" build --project "$PROJ" --sigilo-check --sigilo-ci-profile gated --sigilo-baseline "$CI_BASE"

run_expect_exit \
    "release blocks review-required drift" \
    2 \
    "$CCT_BIN" build --project "$PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline "$CI_BASE"

# 5) strict/tolerant validation
TOL_SIG="$TMP_DIR/tolerant_validate.sigil"
cp "$FIX_ROOT/artifacts/expanded_metadata_v1.sigil" "$TOL_SIG"
run_expect_exit \
    "tolerant validate accepts additive metadata" \
    0 \
    "$CCT_BIN" sigilo validate "$TOL_SIG" --summary --consumer-profile legacy-tolerant

run_expect_exit \
    "strict validate blocks missing required field" \
    2 \
    "$CCT_BIN" sigilo validate "$FIX_ROOT/artifacts/invalid_missing_required.sigil" --summary --strict

# 6) integration with build/test/doc
run_cmd "project test integration works" "$CCT_BIN" test --project "$PROJ"
run_cmd "project doc integration works" "$CCT_BIN" doc --project "$PROJ" --output-dir "$TMP_DIR/docs" --format markdown --no-timestamp

# 7) legacy command regression
MULTI_MAIN="$FIX_ROOT/multi_module/main.cct"
run_cmd "legacy single-file compile still works" "$CCT_BIN" "$FIX_ROOT/single_file/smoke.cct"
run_cmd "legacy --sigilo-only still works" "$CCT_BIN" --sigilo-only "$MULTI_MAIN"
run_expect_exit \
    "legacy sigilo check strict passes for identical artifacts" \
    0 \
    "$CCT_BIN" sigilo check "$DIFF_BASE" "$DIFF_BASE" --strict --summary

echo ""
echo "----------------------------------------"
echo "13D.1 regression summary: total=$TESTS_TOTAL failed=$TESTS_FAILED"
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
