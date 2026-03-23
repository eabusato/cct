#!/bin/bash

# Bootstrap Parity Test Runner
# Runs subset of tests 0-30 using the promoted self-hosted compiler
# to validate behavioral parity with the host compiler.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Ensure we're using the promoted compiler
export CCT_WRAPPER_MODE=default

# Verify promotion is active
ACTIVE_COMPILER="$("$ROOT_DIR/cct" --which-compiler 2>/dev/null || echo "unknown")"
if [ "$ACTIVE_COMPILER" != "selfhost" ]; then
    echo -e "${RED}ERROR: Bootstrap parity requires promoted compiler (selfhost mode)${NC}" >&2
    echo "Current active compiler: $ACTIVE_COMPILER" >&2
    echo "Run 'make bootstrap-promote' first" >&2
    exit 1
fi

echo "========================================"
echo "Bootstrap Compiler Parity Validation"
echo "========================================"
echo ""
echo "Active compiler: $ACTIVE_COMPILER (via ./cct wrapper)"
echo "Parity matrix: docs/bootstrap_parity_matrix.txt"
echo ""
echo "Running MUST_PASS test phases against promoted bootstrap compiler..."
echo ""

# Track results
PARITY_PASSED=0
PARITY_FAILED=0
PARITY_REPORT="$ROOT_DIR/out/bootstrap/phase31/parity/parity_report.txt"

mkdir -p "$(dirname "$PARITY_REPORT")"
rm -f "$PARITY_REPORT"

# Header
{
    echo "Bootstrap Compiler Parity Report"
    echo "Generated: $(date)"
    echo "Active compiler: $ACTIVE_COMPILER"
    echo "================================"
    echo ""
} > "$PARITY_REPORT"

run_phase_tests() {
    phase_name="$1"
    phase_group="$2"
    parity_status="$3"

    if [ "$parity_status" != "MUST_PASS" ] && [ "$parity_status" != "SHOULD_PASS" ]; then
        echo -e "${YELLOW}Skipping $phase_name ($parity_status)${NC}"
        echo "SKIPPED: $phase_name ($parity_status)" >> "$PARITY_REPORT"
        return 0
    fi

    echo -e "Testing $phase_name..."

    if CCT_TEST_GROUP="$phase_group" bash "$ROOT_DIR/tests/run_tests.sh" >/dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: $phase_name"
        echo "PASS: $phase_name" >> "$PARITY_REPORT"
        PARITY_PASSED=$((PARITY_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $phase_name"
        echo "FAIL: $phase_name" >> "$PARITY_REPORT"
        PARITY_FAILED=$((PARITY_FAILED + 1))
    fi
}

# ============================================================================
# PHASE 21-29: Bootstrap Core (MUST_PASS: 100%)
# ============================================================================

echo "Testing Bootstrap Core Phases (21-29)..."
echo ""

run_phase_tests "FASE 21: Lexer Bootstrap" "bootstrap-lexer" "MUST_PASS"
run_phase_tests "FASE 22: Parser Bootstrap" "bootstrap-parser" "MUST_PASS"
run_phase_tests "FASE 23: Advanced Syntax" "bootstrap" "MUST_PASS"
run_phase_tests "FASE 24-25: Semantic Analysis" "bootstrap" "MUST_PASS"
run_phase_tests "FASE 26-28: Code Generation" "bootstrap" "MUST_PASS"
run_phase_tests "FASE 29: Self-Hosting" "bootstrap" "MUST_PASS"

echo ""
echo "Testing Operational Phases (30-31)..."
echo ""

run_phase_tests "FASE 30: Operational Platform" "operational" "MUST_PASS"

# Phase 31 tests via explicit phase runner
echo -e "Testing FASE 31: Promotion (31A-31E)..."
if CCT_TEST_PHASES=31A,31B,31C,31D,31E bash "$ROOT_DIR/tests/run_tests.sh" >/dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS${NC}: FASE 31: Promotion"
    echo "PASS: FASE 31: Promotion" >> "$PARITY_REPORT"
    PARITY_PASSED=$((PARITY_PASSED + 1))
else
    echo -e "${RED}✗ FAIL${NC}: FASE 31: Promotion"
    echo "FAIL: FASE 31: Promotion" >> "$PARITY_REPORT"
    PARITY_FAILED=$((PARITY_FAILED + 1))
fi

# ============================================================================
# Summary
# ============================================================================

echo ""
echo "========================================" | tee -a "$PARITY_REPORT"
echo "Parity Test Summary" | tee -a "$PARITY_REPORT"
echo "========================================" | tee -a "$PARITY_REPORT"
echo "Passed: $PARITY_PASSED" | tee -a "$PARITY_REPORT"
echo "Failed: $PARITY_FAILED" | tee -a "$PARITY_REPORT"

TOTAL=$((PARITY_PASSED + PARITY_FAILED))
if [ "$TOTAL" -gt 0 ]; then
    PASS_RATE=$((PARITY_PASSED * 100 / TOTAL))
    echo "Pass rate: ${PASS_RATE}%" | tee -a "$PARITY_REPORT"
fi

echo "" | tee -a "$PARITY_REPORT"
echo "Full report: $PARITY_REPORT" | tee -a "$PARITY_REPORT"

# Parity gate: require 95%+ pass rate for core phases
MINIMUM_REQUIRED=95

if [ "$TOTAL" -gt 0 ] && [ "$PASS_RATE" -ge "$MINIMUM_REQUIRED" ]; then
    echo -e "${GREEN}PARITY GATE: PASSED${NC} (${PASS_RATE}% >= ${MINIMUM_REQUIRED}%)" | tee -a "$PARITY_REPORT"
    exit 0
else
    echo -e "${RED}PARITY GATE: FAILED${NC} (${PASS_RATE}% < ${MINIMUM_REQUIRED}%)" | tee -a "$PARITY_REPORT"
    echo "" | tee -a "$PARITY_REPORT"
    echo "Bootstrap compiler does not meet minimum parity requirements." | tee -a "$PARITY_REPORT"
    echo "See docs/bootstrap_parity_matrix.txt for expectations." | tee -a "$PARITY_REPORT"
    exit 1
fi
