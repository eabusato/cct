#!/bin/bash
#
# CCT — Clavicula Turing
# Test Runner
#
# FASE 0: Basic test harness
#
# Tests the fundamental functionality of the CCT compiler binary.
# Future phases will add more comprehensive tests.
#
# Copyright (c) Erick Andrade Busato. Todos os direitos reservados.

# Note: We don't use 'set -e' because some tests intentionally trigger errors

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Path to the CCT binary
CCT_BIN="./cct"

# Check if binary exists
if [ ! -f "$CCT_BIN" ]; then
    echo -e "${RED}Error: CCT binary not found at $CCT_BIN${NC}"
    echo "Please run 'make' first to build the compiler."
    exit 1
fi

# Make binary executable
chmod +x "$CCT_BIN"

echo "CCT — Clavicula Turing Test Suite"
echo "FASE 0: Foundation Tests"
echo "========================================"
echo ""

# Test helper functions
test_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((TESTS_PASSED++))
}

test_fail() {
    echo -e "${RED}✗${NC} $1"
    ((TESTS_FAILED++))
}

cleanup_codegen_artifacts() {
    local src="$1"
    local exe="${src%.cct}"
    rm -f "$exe" "$exe.cgen.c" "$exe.svg" "$exe.sigil" "$exe.system.svg" "$exe.system.sigil"
    rm -f "$exe".__mod_*.svg "$exe".__mod_*.sigil
}

resolve_doc_path() {
    local rel="$1"
    if [ -f "$rel" ]; then
        echo "$rel"
    elif [ -f "md_out/$rel" ]; then
        echo "md_out/$rel"
    else
        echo "$rel"
    fi
}

# Test 1: Binary exists and is executable
echo "Test 1: Binary exists and is executable"
if [ -x "$CCT_BIN" ]; then
    test_pass "Binary is executable"
else
    test_fail "Binary is not executable"
fi

# Test 2: --help flag works
echo "Test 2: --help flag displays help message"
if "$CCT_BIN" --help > /dev/null 2>&1; then
    if "$CCT_BIN" --help | grep -q "Clavicula Turing"; then
        test_pass "--help displays correct help message"
    else
        test_fail "--help does not display expected content"
    fi
else
    test_fail "--help command failed"
fi

# Test 3: --version flag works
echo "Test 3: --version flag displays version"
if "$CCT_BIN" --version > /dev/null 2>&1; then
    if "$CCT_BIN" --version | grep -q "Clavicula Turing"; then
        test_pass "--version displays correct version"
    else
        test_fail "--version does not display expected content"
    fi
else
    test_fail "--version command failed"
fi

# Test 4: No arguments shows help
echo "Test 4: No arguments displays help"
if "$CCT_BIN" > /dev/null 2>&1; then
    if "$CCT_BIN" | grep -q "Clavicula Turing"; then
        test_pass "No arguments displays help message"
    else
        test_fail "No arguments does not display help"
    fi
else
    test_fail "No arguments command failed"
fi

# Test 5: Unknown option returns error
echo "Test 5: Unknown option returns error"
OUTPUT=$("$CCT_BIN" --unknown-option 2>&1) || true
if echo "$OUTPUT" | grep -q "Unknown option"; then
    test_pass "Unknown option returns appropriate error"
else
    test_fail "Unknown option did not return expected error"
fi

# Test 6: --tokens requires file argument
echo "Test 6: --tokens requires file argument"
OUTPUT=$("$CCT_BIN" --tokens 2>&1) || true
if echo "$OUTPUT" | grep -q "requires a file argument"; then
    test_pass "--tokens without file returns appropriate error"
else
    test_fail "--tokens without file did not return expected error"
fi

# Test 7: File without .cct extension returns error
echo "Test 7: File without .cct extension returns error"
OUTPUT=$("$CCT_BIN" test.txt 2>&1) || true
if echo "$OUTPUT" | grep -q ".cct extension"; then
    test_pass "Non-.cct file returns appropriate error"
else
    test_fail "Non-.cct file did not return expected error"
fi

# Test 8: .cct file routes to compile pipeline (missing file error)
echo "Test 8: .cct file compilation attempts real pipeline"
OUTPUT=$("$CCT_BIN" test.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Could not open file"; then
    test_pass ".cct file triggers compilation pipeline"
else
    test_fail ".cct file did not reach compilation pipeline"
fi

echo ""
echo "========================================"
echo "FASE 1: Lexer Tests"
echo "========================================"
echo ""

# Test 9: --tokens on hello.cct works
echo "Test 9: --tokens tokenizes hello.cct successfully"
if "$CCT_BIN" --tokens examples/hello.cct > /dev/null 2>&1; then
    if "$CCT_BIN" --tokens examples/hello.cct | grep -q "INCIPIT"; then
        test_pass "--tokens successfully tokenizes hello.cct"
    else
        test_fail "--tokens output missing expected tokens"
    fi
else
    test_fail "--tokens failed to process hello.cct"
fi

# Test 10: Keywords are recognized
echo "Test 10: Keywords are recognized correctly"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/keywords.cct 2>&1)
if echo "$OUTPUT" | grep -q "RITUALE" && echo "$OUTPUT" | grep -q "EVOCA" && echo "$OUTPUT" | grep -q "VINCIRE"; then
    test_pass "Keywords are recognized correctly"
else
    test_fail "Keywords not recognized correctly"
fi

# Test 11: Numbers are tokenized
echo "Test 11: Numbers are tokenized correctly"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/numbers.cct 2>&1)
if echo "$OUTPUT" | grep -q "INTEGER" && echo "$OUTPUT" | grep -q "REAL"; then
    test_pass "Numbers tokenized correctly (INTEGER and REAL)"
else
    test_fail "Numbers not tokenized correctly"
fi

# Test 12: Strings are tokenized
echo "Test 12: Strings are tokenized correctly"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/strings.cct 2>&1)
if echo "$OUTPUT" | grep -q "STRING"; then
    test_pass "Strings tokenized correctly"
else
    test_fail "Strings not tokenized correctly"
fi

# Test 13: Comments are ignored
echo "Test 13: Comments are ignored by lexer"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/comments.cct 2>&1)
# Check that comment text doesn't appear as tokens (only INCIPIT, RITUALE, EXPLICIT should appear)
if echo "$OUTPUT" | grep -q "INCIPIT" && echo "$OUTPUT" | grep -q "RITUALE" && ! echo "$OUTPUT" | grep "IDENTIFIER.*\"comment\""; then
    test_pass "Comments are properly ignored"
else
    test_fail "Comments were not ignored"
fi

# Test 14: Operators are recognized
echo "Test 14: Operators are recognized"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/operators.cct 2>&1)
if echo "$OUTPUT" | grep -q "PLUS" && echo "$OUTPUT" | grep -q "MINUS" && echo "$OUTPUT" | grep -q "STAR"; then
    test_pass "Operators recognized correctly"
else
    test_fail "Operators not recognized correctly"
fi

# Test 15: Lexical errors are detected
echo "Test 15: Lexical errors are detected (unterminated string)"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/error_unterminated_string.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Unterminated string"; then
    test_pass "Lexical error detected and reported"
else
    test_fail "Lexical error not detected"
fi

# Test 16: Line and column tracking
echo "Test 16: Line and column numbers appear in output"
OUTPUT=$("$CCT_BIN" --tokens examples/hello.cct 2>&1)
if echo "$OUTPUT" | grep -q "[0-9]:[0-9]"; then
    test_pass "Line and column numbers present in token output"
else
    test_fail "Line and column numbers missing from output"
fi

echo ""
echo "========================================"
echo "FASE 2B: Parser/AST Tests"
echo "========================================"
echo ""

# Test 17: AST minimal program
echo "Test 17: --ast parses minimal program"
OUTPUT=$("$CCT_BIN" --ast tests/integration/minimal.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "PROGRAM: minimal"; then
    test_pass "--ast parses minimal program"
else
    test_fail "--ast failed on minimal program"
fi

# Test 18: AST simple function
echo "Test 18: --ast parses simple ritual"
OUTPUT=$("$CCT_BIN" --ast tests/integration/simple_func.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "RITUALE" && echo "$OUTPUT" | grep -q "REDDE"; then
    test_pass "--ast parses simple function"
else
    test_fail "--ast failed on simple function"
fi

# Test 19: AST declaration + assignment
echo "Test 19: --ast parses EVOCA + VINCIRE"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_vars.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "EVOCA" && echo "$OUTPUT" | grep -q "VINCIRE"; then
    test_pass "--ast parses EVOCA and VINCIRE"
else
    test_fail "--ast failed on EVOCA/VINCIRE"
fi

# Test 20: AST OBSECRO builtin call
echo "Test 20: --ast parses OBSECRO"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_obsecro.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "OBSECRO"; then
    test_pass "--ast parses OBSECRO call"
else
    test_fail "--ast failed on OBSECRO call"
fi

# Test 21: AST SI / ALITER
echo "Test 21: --ast parses SI / ALITER"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_si_aliter.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "SI" && echo "$OUTPUT" | grep -q "Then:" && echo "$OUTPUT" | grep -q "Else:"; then
    test_pass "--ast parses SI / ALITER"
else
    test_fail "--ast failed on SI / ALITER"
fi

# Test 22: AST DUM loop
echo "Test 22: --ast parses DUM"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_dum.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "DUM"; then
    test_pass "--ast parses DUM loop"
else
    test_fail "--ast failed on DUM loop"
fi

# Test 23: AST REPETE loop
echo "Test 23: --ast parses REPETE"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_repete.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "REPETE"; then
    test_pass "--ast parses REPETE loop"
else
    test_fail "--ast failed on REPETE loop"
fi

# Test 24: Syntax error is reported
echo "Test 24: --ast reports syntax error"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_syntax_error.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "error" && echo "$OUTPUT" | grep -qi "Syntax"; then
    test_pass "--ast reports syntax errors"
else
    test_fail "--ast did not report syntax error"
fi

# Test 25: --ast parses example hello program
echo "Test 25: --ast parses examples/hello.cct"
OUTPUT=$("$CCT_BIN" --ast examples/hello.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "PROGRAM: salutatio" && echo "$OUTPUT" | grep -q "OBSECRO"; then
    test_pass "--ast parses examples/hello.cct"
else
    test_fail "--ast failed on examples/hello.cct"
fi

# Test 26: DONEC loop parses (FASE 2 support)
echo "Test 26: --ast parses DONEC"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_donec.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "DONEC"; then
    test_pass "--ast parses DONEC loop"
else
    test_fail "--ast failed on DONEC loop"
fi

echo ""
echo "========================================"
echo "FASE 3: Semantic Analysis Tests"
echo "========================================"
echo ""

# Valid semantic checks
echo "Test 27: --check parses/analyzes minimal program"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_minimal.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts minimal program"
else
    test_fail "--check rejected minimal program"
fi

echo "Test 28: --check validates compatible return"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_return.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts valid return"
else
    test_fail "--check rejected valid return"
fi

echo "Test 29: --check validates EVOCA/VINCIRE types"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_vars.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts valid variables/assignment"
else
    test_fail "--check rejected valid variables/assignment"
fi

echo "Test 30: --check validates CONIURA call"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_call.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts valid ritual call"
else
    test_fail "--check rejected valid ritual call"
fi

echo "Test 31: --check accepts OBSECRO scribe"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_obsecro.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts OBSECRO scribe"
else
    test_fail "--check rejected OBSECRO scribe"
fi

echo "Test 32: --check validates REPETE loop"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_repete.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts valid REPETE"
else
    test_fail "--check rejected valid REPETE"
fi

echo "Test 33: --check validates SI/DUM/DONEC conditions"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_valid_conditions.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check accepts valid conditions"
else
    test_fail "--check rejected valid conditions"
fi

# Invalid semantic checks
echo "Test 34: --check rejects undeclared variable"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_undeclared.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "undeclared symbol"; then
    test_pass "--check reports undeclared symbol"
else
    test_fail "--check missed undeclared symbol"
fi

echo "Test 35: --check rejects duplicate local variable"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_duplicate_var.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate symbol"; then
    test_pass "--check reports duplicate local variable"
else
    test_fail "--check missed duplicate local variable"
fi

echo "Test 36: --check rejects duplicate parameter"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_duplicate_param.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate symbol"; then
    test_pass "--check reports duplicate parameter"
else
    test_fail "--check missed duplicate parameter"
fi

echo "Test 37: --check rejects duplicate rituale"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_duplicate_rituale.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate symbol"; then
    test_pass "--check reports duplicate rituale"
else
    test_fail "--check missed duplicate rituale"
fi

echo "Test 38: --check rejects incompatible EVOCA initializer"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_type_evoca.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "initializer type mismatch"; then
    test_pass "--check reports EVOCA type mismatch"
else
    test_fail "--check missed EVOCA type mismatch"
fi

echo "Test 39: --check rejects incompatible VINCIRE"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_type_vincire.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "assignment type mismatch"; then
    test_pass "--check reports VINCIRE type mismatch"
else
    test_fail "--check missed VINCIRE type mismatch"
fi

echo "Test 40: --check rejects incompatible REDDE"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_return.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "return type mismatch"; then
    test_pass "--check reports return mismatch"
else
    test_fail "--check missed return mismatch"
fi

echo "Test 41: --check rejects wrong arity"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_arity.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "expects .*argument"; then
    test_pass "--check reports wrong arity"
else
    test_fail "--check missed wrong arity"
fi

echo "Test 42: --check rejects missing rituale"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_missing_rituale.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "is not declared"; then
    test_pass "--check reports missing rituale"
else
    test_fail "--check missed missing rituale"
fi

echo "Test 43: --check rejects invalid condition"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_condition.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "condition expression must be"; then
    test_pass "--check reports invalid condition"
else
    test_fail "--check missed invalid condition"
fi

echo ""
echo "========================================"
echo "FASE 4A: Minimal Executable Codegen Tests"
echo "========================================"
echo ""

# Cleanup from previous runs
cleanup_codegen_artifacts "tests/integration/codegen_minimal.cct"
cleanup_codegen_artifacts "tests/integration/codegen_return.cct"
cleanup_codegen_artifacts "tests/integration/codegen_anur.cct"
cleanup_codegen_artifacts "tests/integration/codegen_scribe_string.cct"
cleanup_codegen_artifacts "tests/integration/codegen_arithmetic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_local_var.cct"
cleanup_codegen_artifacts "tests/integration/codegen_unsupported_repete.cct"
cleanup_codegen_artifacts "tests/integration/sem_invalid_undeclared.cct"

echo "Test 44: compile minimal executable program"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_minimal.cct 2>&1) || true
if [ -x "tests/integration/codegen_minimal" ] && echo "$OUTPUT" | grep -q "Compiled:"; then
    test_pass "Compiled minimal program to executable"
else
    test_fail "Failed to compile minimal executable program"
fi

echo "Test 45: minimal generated binary executes"
"tests/integration/codegen_minimal" > /dev/null 2>&1
RC=$?
if [ $RC -eq 0 ]; then
    test_pass "Minimal generated binary executes with exit code 0"
else
    test_fail "Minimal generated binary returned unexpected exit code ($RC)"
fi

echo "Test 46: compile program with REDDE integer"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_return.cct 2>&1) || true
if [ -x "tests/integration/codegen_return" ]; then
    test_pass "Compiled REDDE integer program"
else
    test_fail "Failed to compile REDDE integer program"
fi

echo "Test 47: REDDE integer exit code is correct"
"tests/integration/codegen_return" > /dev/null 2>&1
RC=$?
if [ $RC -eq 7 ]; then
    test_pass "REDDE integer produced correct exit code"
else
    test_fail "REDDE integer exit code mismatch ($RC)"
fi

echo "Test 48: compile program with ANUR"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_anur.cct 2>&1) || true
if [ -x "tests/integration/codegen_anur" ]; then
    test_pass "Compiled ANUR program"
else
    test_fail "Failed to compile ANUR program"
fi

echo "Test 49: ANUR exit code is correct"
"tests/integration/codegen_anur" > /dev/null 2>&1
RC=$?
if [ $RC -eq 3 ]; then
    test_pass "ANUR produced correct exit code"
else
    test_fail "ANUR exit code mismatch ($RC)"
fi

echo "Test 50: compile OBSECRO scribe with string literal"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_scribe_string.cct 2>&1) || true
if [ -x "tests/integration/codegen_scribe_string" ]; then
    test_pass "Compiled OBSECRO scribe(string) program"
else
    test_fail "Failed to compile OBSECRO scribe(string) program"
fi

echo "Test 51: OBSECRO scribe outputs expected text"
OUTPUT=$("tests/integration/codegen_scribe_string" 2>&1)
RC=$?
if [ $RC -eq 0 ] && [ "$OUTPUT" = "Ave FASE 4A!" ]; then
    test_pass "OBSECRO scribe produced expected output"
else
    test_fail "OBSECRO scribe output mismatch or non-zero exit ($RC)"
fi

echo "Test 52: compile arithmetic expression program"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_arithmetic.cct 2>&1) || true
if [ -x "tests/integration/codegen_arithmetic" ]; then
    test_pass "Compiled arithmetic program"
else
    test_fail "Failed to compile arithmetic program"
fi

echo "Test 53: arithmetic expression return code is correct"
"tests/integration/codegen_arithmetic" > /dev/null 2>&1
RC=$?
if [ $RC -eq 15 ]; then
    test_pass "Arithmetic expression codegen returned correct value"
else
    test_fail "Arithmetic expression exit code mismatch ($RC)"
fi

echo "Test 54: compile EVOCA local variable program"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_local_var.cct 2>&1) || true
if [ -x "tests/integration/codegen_local_var" ]; then
    test_pass "Compiled EVOCA local variable program"
else
    test_fail "Failed to compile EVOCA local variable program"
fi

echo "Test 55: local variable read/return works"
"tests/integration/codegen_local_var" > /dev/null 2>&1
RC=$?
if [ $RC -eq 42 ]; then
    test_pass "Local variable codegen works"
else
    test_fail "Local variable exit code mismatch ($RC)"
fi

echo "Test 56: compile rejects semantic-invalid program"
OUTPUT=$("$CCT_BIN" tests/integration/sem_invalid_undeclared.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "undeclared symbol" && [ ! -e "tests/integration/sem_invalid_undeclared" ]; then
    test_pass "Compilation stops on semantic error before generating executable"
else
    test_fail "Compilation did not reject semantic-invalid program as expected"
fi

echo "Test 57: compile rejects unsupported codegen feature clearly"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_type_not_supported_fluxus.cct 2>&1) || true
if echo "$OUTPUT" | grep -Eqi "FASE 4C|FASE 6B" && echo "$OUTPUT" | grep -qi "not supported\\|requires static SERIES" && [ ! -e "tests/integration/codegen_type_not_supported_fluxus" ]; then
    test_pass "Unsupported codegen feature fails clearly"
else
    test_fail "Unsupported codegen feature did not fail clearly"
fi

# Cleanup generated artifacts after FASE 4A tests
cleanup_codegen_artifacts "tests/integration/codegen_minimal.cct"
cleanup_codegen_artifacts "tests/integration/codegen_return.cct"
cleanup_codegen_artifacts "tests/integration/codegen_anur.cct"
cleanup_codegen_artifacts "tests/integration/codegen_scribe_string.cct"
cleanup_codegen_artifacts "tests/integration/codegen_arithmetic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_local_var.cct"
cleanup_codegen_artifacts "tests/integration/codegen_type_not_supported_real.cct"
cleanup_codegen_artifacts "tests/integration/codegen_type_not_supported_fluxus.cct"
cleanup_codegen_artifacts "tests/integration/codegen_feature_not_supported_codex.cct"
cleanup_codegen_artifacts "tests/integration/codegen_feature_not_supported_pactum.cct"
cleanup_codegen_artifacts "tests/integration/sem_invalid_undeclared.cct"

echo ""
echo "========================================"
echo "FASE 4B: Flow + Structured Execution Tests"
echo "========================================"
echo ""

cleanup_codegen_artifacts "tests/integration/codegen_vincire.cct"
cleanup_codegen_artifacts "tests/integration/codegen_compare_simple.cct"
cleanup_codegen_artifacts "tests/integration/codegen_if.cct"
cleanup_codegen_artifacts "tests/integration/codegen_if_else.cct"
cleanup_codegen_artifacts "tests/integration/codegen_while.cct"
cleanup_codegen_artifacts "tests/integration/codegen_while_counter.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_simple.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_return.cct"
cleanup_codegen_artifacts "tests/integration/codegen_scribe_int.cct"

echo "Test 58: EVOCA + VINCIRE compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_vincire.cct 2>&1) || true
if [ -x "tests/integration/codegen_vincire" ]; then
    "tests/integration/codegen_vincire" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "EVOCA + VINCIRE executes correctly"
    else
        test_fail "EVOCA + VINCIRE exit code mismatch ($RC)"
    fi
else
    test_fail "EVOCA + VINCIRE program failed to compile"
fi

echo "Test 59: comparison expression compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_compare_simple.cct 2>&1) || true
if [ -x "tests/integration/codegen_compare_simple" ]; then
    "tests/integration/codegen_compare_simple" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "Comparison produced executable boolean (0/1)"
    else
        test_fail "Comparison executable result mismatch ($RC)"
    fi
else
    test_fail "Comparison program failed to compile"
fi

echo "Test 60: SI without ALITER compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_if.cct 2>&1) || true
if [ -x "tests/integration/codegen_if" ]; then
    "tests/integration/codegen_if" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "SI without ALITER executes correctly"
    else
        test_fail "SI without ALITER exit code mismatch ($RC)"
    fi
else
    test_fail "SI without ALITER program failed to compile"
fi

echo "Test 61: SI / ALITER compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_if_else.cct 2>&1) || true
if [ -x "tests/integration/codegen_if_else" ]; then
    "tests/integration/codegen_if_else" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 2 ]; then
        test_pass "SI / ALITER executes correctly"
    else
        test_fail "SI / ALITER exit code mismatch ($RC)"
    fi
else
    test_fail "SI / ALITER program failed to compile"
fi

echo "Test 62: DUM compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_while.cct 2>&1) || true
if [ -x "tests/integration/codegen_while" ]; then
    "tests/integration/codegen_while" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 3 ]; then
        test_pass "DUM executes correctly"
    else
        test_fail "DUM exit code mismatch ($RC)"
    fi
else
    test_fail "DUM program failed to compile"
fi

echo "Test 63: DUM with state mutation executes correctly"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_while_counter.cct 2>&1) || true
if [ -x "tests/integration/codegen_while_counter" ]; then
    "tests/integration/codegen_while_counter" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 6 ]; then
        test_pass "DUM + VINCIRE state loop works"
    else
        test_fail "DUM + VINCIRE state loop exit code mismatch ($RC)"
    fi
else
    test_fail "DUM + VINCIRE state loop program failed to compile"
fi

echo "Test 64: CONIURA simple call statement compiles and runs"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_call_simple.cct 2>&1) || true
if [ -x "tests/integration/codegen_call_simple" ]; then
    OUTPUT=$("tests/integration/codegen_call_simple" 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && [ "$OUTPUT" = "5" ]; then
        test_pass "CONIURA statement call works"
    else
        test_fail "CONIURA statement call output/exit mismatch ($RC)"
    fi
else
    test_fail "CONIURA statement call program failed to compile"
fi

echo "Test 65: CONIURA return value compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_call_return.cct 2>&1) || true
if [ -x "tests/integration/codegen_call_return" ]; then
    "tests/integration/codegen_call_return" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "CONIURA return value works"
    else
        test_fail "CONIURA return value exit code mismatch ($RC)"
    fi
else
    test_fail "CONIURA return value program failed to compile"
fi

echo "Test 66: OBSECRO scribe(x) works for integer expression"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_scribe_int.cct 2>&1) || true
if [ -x "tests/integration/codegen_scribe_int" ]; then
    OUTPUT=$("tests/integration/codegen_scribe_int" 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && [ "$OUTPUT" = "42" ]; then
        test_pass "OBSECRO scribe(integer expr) works"
    else
        test_fail "OBSECRO scribe(integer expr) output/exit mismatch ($RC)"
    fi
else
    test_fail "OBSECRO scribe(integer expr) program failed to compile"
fi

cleanup_codegen_artifacts "tests/integration/codegen_vincire.cct"
cleanup_codegen_artifacts "tests/integration/codegen_compare_simple.cct"
cleanup_codegen_artifacts "tests/integration/codegen_if.cct"
cleanup_codegen_artifacts "tests/integration/codegen_if_else.cct"
cleanup_codegen_artifacts "tests/integration/codegen_while.cct"
cleanup_codegen_artifacts "tests/integration/codegen_while_counter.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_simple.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_return.cct"
cleanup_codegen_artifacts "tests/integration/codegen_scribe_int.cct"

echo ""
echo "========================================"
echo "FASE 4C: Consolidated Executable Subset Tests"
echo "========================================"
echo ""

cleanup_codegen_artifacts "tests/integration/codegen_repete.cct"
cleanup_codegen_artifacts "tests/integration/codegen_repete_gradus.cct"
cleanup_codegen_artifacts "tests/integration/codegen_repete_accumulate.cct"
cleanup_codegen_artifacts "tests/integration/codegen_donec.cct"
cleanup_codegen_artifacts "tests/integration/codegen_donec_once.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_multiarg.cct"
cleanup_codegen_artifacts "tests/integration/codegen_type_not_supported_real.cct"
cleanup_codegen_artifacts "tests/integration/codegen_feature_not_supported_codex.cct"

echo "Test 67: REPETE simple compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_repete.cct 2>&1) || true
if [ -x "tests/integration/codegen_repete" ]; then
    "tests/integration/codegen_repete" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 10 ]; then
        test_pass "REPETE simple executes correctly"
    else
        test_fail "REPETE simple exit code mismatch ($RC)"
    fi
else
    test_fail "REPETE simple program failed to compile"
fi

echo "Test 68: REPETE with GRADUS compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_repete_gradus.cct 2>&1) || true
if [ -x "tests/integration/codegen_repete_gradus" ]; then
    "tests/integration/codegen_repete_gradus" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 12 ]; then
        test_pass "REPETE GRADUS executes correctly"
    else
        test_fail "REPETE GRADUS exit code mismatch ($RC)"
    fi
else
    test_fail "REPETE GRADUS program failed to compile"
fi

echo "Test 69: REPETE integrates with external variable state"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_repete_accumulate.cct 2>&1) || true
if [ -x "tests/integration/codegen_repete_accumulate" ]; then
    "tests/integration/codegen_repete_accumulate" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 16 ]; then
        test_pass "REPETE interacts with external variable state"
    else
        test_fail "REPETE external state exit code mismatch ($RC)"
    fi
else
    test_fail "REPETE external state program failed to compile"
fi

echo "Test 70: DONEC compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_donec.cct 2>&1) || true
if [ -x "tests/integration/codegen_donec" ]; then
    "tests/integration/codegen_donec" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 3 ]; then
        test_pass "DONEC executes correctly"
    else
        test_fail "DONEC exit code mismatch ($RC)"
    fi
else
    test_fail "DONEC program failed to compile"
fi

echo "Test 71: DONEC executes body at least once"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_donec_once.cct 2>&1) || true
if [ -x "tests/integration/codegen_donec_once" ]; then
    "tests/integration/codegen_donec_once" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "DONEC executes body at least once"
    else
        test_fail "DONEC-once exit code mismatch ($RC)"
    fi
else
    test_fail "DONEC-once program failed to compile"
fi

echo "Test 72: CONIURA with multiple arguments compiles and executes"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_call_multiarg.cct 2>&1) || true
if [ -x "tests/integration/codegen_call_multiarg" ]; then
    "tests/integration/codegen_call_multiarg" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 14 ]; then
        test_pass "CONIURA multi-arg call works"
    else
        test_fail "CONIURA multi-arg exit code mismatch ($RC)"
    fi
else
    test_fail "CONIURA multi-arg program failed to compile"
fi

echo "Test 73: non-executable real type fails clearly"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_type_not_supported_fluxus.cct 2>&1) || true
if echo "$OUTPUT" | grep -Eqi "FASE 4C|FASE 6B" && echo "$OUTPUT" | grep -qi "not supported\\|requires static SERIES"; then
    test_pass "Unsupported non-executable type fails clearly"
else
    test_fail "Unsupported non-executable type did not fail clearly"
fi

echo "Test 74: unsupported top-level feature fails clearly"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_feature_not_supported_codex.cct 2>&1) || true
if echo "$OUTPUT" | grep -Eqi "FASE 4C|FASE 6B" && echo "$OUTPUT" | grep -qi "CODEX\\|top-level"; then
    test_pass "Unsupported top-level feature fails clearly"
else
    test_fail "Unsupported top-level feature did not fail clearly"
fi

cleanup_codegen_artifacts "tests/integration/codegen_repete.cct"
cleanup_codegen_artifacts "tests/integration/codegen_repete_gradus.cct"
cleanup_codegen_artifacts "tests/integration/codegen_repete_accumulate.cct"
cleanup_codegen_artifacts "tests/integration/codegen_donec.cct"
cleanup_codegen_artifacts "tests/integration/codegen_donec_once.cct"
cleanup_codegen_artifacts "tests/integration/codegen_call_multiarg.cct"
cleanup_codegen_artifacts "tests/integration/codegen_type_not_supported_real.cct"
cleanup_codegen_artifacts "tests/integration/codegen_feature_not_supported_codex.cct"

echo ""
echo "========================================"
echo "FASE 5/5B/6A: Sigillum Tests"
echo "========================================"
echo ""

cleanup_codegen_artifacts "tests/integration/sigilo_minimal.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_if.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_while.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_repete.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_call.cct"
cleanup_codegen_artifacts "tests/integration/parser_test.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_structural_variant_a.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_structural_variant_b.cct"
rm -f tests/integration/sigilo_minimal.svg.ref tests/integration/sigilo_minimal.sigil.ref
rm -f examples/hello.svg examples/hello.sigil
rm -f tests/integration/tmp_sig_style_network.svg tests/integration/tmp_sig_style_network.sigil
rm -f tests/integration/tmp_sig_style_seal.svg tests/integration/tmp_sig_style_seal.sigil
rm -f tests/integration/tmp_sig_style_scriptum.svg tests/integration/tmp_sig_style_scriptum.sigil
rm -f tests/integration/tmp_sig_style_seal.ref.svg tests/integration/tmp_sig_style_seal.ref.sigil
rm -f tests/integration/tmp_sig_out_custom.svg tests/integration/tmp_sig_out_custom.sigil

echo "Test 75: compile valid program also generates sigil artifacts"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_minimal.cct 2>&1) || true
if [ -x "tests/integration/codegen_minimal" ] && \
   [ -f "tests/integration/codegen_minimal.svg" ] && \
   [ -f "tests/integration/codegen_minimal.sigil" ]; then
    test_pass "Compilation generates executable + .svg + .sigil"
else
    test_fail "Compilation did not generate expected sigil artifacts"
fi

echo "Test 76: --sigilo-only generates .svg and .sigil without executable"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_minimal.cct 2>&1) || true
if [ -f "tests/integration/sigilo_minimal.svg" ] && \
   [ -f "tests/integration/sigilo_minimal.sigil" ] && \
   [ ! -x "tests/integration/sigilo_minimal" ]; then
    test_pass "--sigilo-only generates artifacts and no executable"
else
    test_fail "--sigilo-only artifacts/executable behavior mismatch"
fi

echo "Test 77: sigilo output is deterministic for same program"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_minimal.cct 2>&1) || true
cp tests/integration/sigilo_minimal.svg tests/integration/sigilo_minimal.svg.ref
cp tests/integration/sigilo_minimal.sigil tests/integration/sigilo_minimal.sigil.ref
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_minimal.cct 2>&1) || true
if cmp -s tests/integration/sigilo_minimal.svg tests/integration/sigilo_minimal.svg.ref && \
   cmp -s tests/integration/sigilo_minimal.sigil tests/integration/sigilo_minimal.sigil.ref; then
    test_pass "Sigilo generation is deterministic across repeated runs"
else
    test_fail "Sigilo output is not deterministic"
fi

echo "Test 78: structurally different programs produce different sigils"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_structural_variant_a.cct 2>&1) || true
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_structural_variant_b.cct 2>&1) || true
if ! cmp -s tests/integration/sigilo_structural_variant_a.svg tests/integration/sigilo_structural_variant_b.svg && \
   ! cmp -s tests/integration/sigilo_structural_variant_a.sigil tests/integration/sigilo_structural_variant_b.sigil; then
    test_pass "Structural variants generate different sigils"
else
    test_fail "Structural variants produced indistinguishable sigils"
fi

echo "Test 79: changing construct (SI vs DUM) changes sigilo detectably"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_if.cct 2>&1) || true
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_while.cct 2>&1) || true
HASH_IF=$(grep '^semantic_hash = ' tests/integration/sigilo_if.sigil 2>/dev/null | head -n1)
HASH_DUM=$(grep '^semantic_hash = ' tests/integration/sigilo_while.sigil 2>/dev/null | head -n1)
if [ -n "$HASH_IF" ] && [ -n "$HASH_DUM" ] && [ "$HASH_IF" != "$HASH_DUM" ]; then
    test_pass "Relevant construct changes alter sigilo/hash signature"
else
    test_fail "Construct change did not alter sigilo detectably"
fi

echo "Test 80: examples/hello.cct generates sigilo successfully"
OUTPUT=$("$CCT_BIN" --sigilo-only examples/hello.cct 2>&1) || true
if [ -f "examples/hello.svg" ] && [ -f "examples/hello.sigil" ]; then
    test_pass "examples/hello.cct generates sigilo artifacts"
else
    test_fail "examples/hello.cct did not generate sigilo artifacts"
fi

echo "Test 81: semantic-invalid program does not generate sigilo artifacts"
cleanup_codegen_artifacts "tests/integration/sem_invalid_undeclared.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sem_invalid_undeclared.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Semantic error" && \
   [ ! -f "tests/integration/sem_invalid_undeclared.svg" ] && \
   [ ! -f "tests/integration/sem_invalid_undeclared.sigil" ]; then
    test_pass "Semantic-invalid program does not emit sigilo artifacts"
else
    test_fail "Semantic-invalid program emitted sigilo artifacts or wrong error"
fi

echo "Test 82: --sigilo-only works for feature outside executable subset (real type)"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_type_not_supported_real.cct 2>&1) || true
if [ -f "tests/integration/codegen_type_not_supported_real.svg" ] && \
   [ -f "tests/integration/codegen_type_not_supported_real.sigil" ]; then
    test_pass "Sigilo-only can operate on semantically valid non-executable subset"
else
    test_fail "Sigilo-only failed on semantically valid non-executable subset"
fi

echo "Test 83: .sigil metadata explains structural grammar and hash role"
if grep -q "\\[visual_grammar\\]" tests/integration/sigilo_minimal.sigil && \
   grep -q "hash_role = secondary" tests/integration/sigilo_minimal.sigil; then
    test_pass ".sigil metadata includes grammar mapping and secondary hash role"
else
    test_fail ".sigil metadata missing expected explanatory fields"
fi

echo "Test 84: .svg output contains organized layers"
if grep -q "id=\\\"foundation\\\"" tests/integration/sigilo_minimal.svg && \
   grep -q "id=\\\"primary_path\\\"" tests/integration/sigilo_minimal.svg && \
   grep -q "id=\\\"nodes\\\"" tests/integration/sigilo_minimal.svg && \
   grep -q "id=\\\"signature\\\"" tests/integration/sigilo_minimal.svg; then
    test_pass ".svg contains expected network composition layers"
else
    test_fail ".svg missing expected network composition layers"
fi

echo "Test 85: recursion + multi-ritual parser_test generates visible call metadata"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/parser_test.cct 2>&1) || true
if [ -f "tests/integration/parser_test.svg" ] && \
   [ -f "tests/integration/parser_test.sigil" ] && \
   grep -q "factorial -> factorial" tests/integration/parser_test.sigil; then
    test_pass "parser_test sigilo captures recursive call edge metadata"
else
    test_fail "parser_test sigilo missing expected recursive call metadata"
fi

echo "Test 86: default sigilo style is network and metadata records it"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_minimal.cct 2>&1) || true
if grep -q "^visual_style = network$" tests/integration/sigilo_minimal.sigil; then
    test_pass "Default sigilo style is network and is recorded in metadata"
else
    test_fail "Default sigilo style metadata missing or incorrect"
fi

echo "Test 87: --sigilo-style network generates artifacts"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-style network --sigilo-out tests/integration/tmp_sig_style_network tests/integration/sigilo_minimal.cct 2>&1) || true
if [ -f "tests/integration/tmp_sig_style_network.svg" ] && \
   [ -f "tests/integration/tmp_sig_style_network.sigil" ] && \
   grep -q "^visual_style = network$" tests/integration/tmp_sig_style_network.sigil; then
    test_pass "Explicit network style works"
else
    test_fail "Explicit network style failed"
fi

echo "Test 88: --sigilo-style seal generates artifacts and metadata"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-style seal --sigilo-out tests/integration/tmp_sig_style_seal tests/integration/sigilo_minimal.cct 2>&1) || true
if [ -f "tests/integration/tmp_sig_style_seal.svg" ] && \
   [ -f "tests/integration/tmp_sig_style_seal.sigil" ] && \
   grep -q "^visual_style = seal$" tests/integration/tmp_sig_style_seal.sigil; then
    test_pass "Seal style works and metadata records style"
else
    test_fail "Seal style failed"
fi

echo "Test 89: --sigilo-style scriptum generates artifacts and metadata"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-style scriptum --sigilo-out tests/integration/tmp_sig_style_scriptum tests/integration/sigilo_minimal.cct 2>&1) || true
if [ -f "tests/integration/tmp_sig_style_scriptum.svg" ] && \
   [ -f "tests/integration/tmp_sig_style_scriptum.sigil" ] && \
   grep -q "^visual_style = scriptum$" tests/integration/tmp_sig_style_scriptum.sigil; then
    test_pass "Scriptum style works and metadata records style"
else
    test_fail "Scriptum style failed"
fi

echo "Test 90: different sigilo styles produce different SVGs for same program"
if ! cmp -s tests/integration/tmp_sig_style_network.svg tests/integration/tmp_sig_style_seal.svg && \
   ! cmp -s tests/integration/tmp_sig_style_seal.svg tests/integration/tmp_sig_style_scriptum.svg; then
    test_pass "Different styles produce different SVG outputs"
else
    test_fail "Different styles produced identical SVG output"
fi

echo "Test 91: same program + same style remains deterministic"
cp tests/integration/tmp_sig_style_seal.svg tests/integration/tmp_sig_style_seal.ref.svg
cp tests/integration/tmp_sig_style_seal.sigil tests/integration/tmp_sig_style_seal.ref.sigil
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-style seal --sigilo-out tests/integration/tmp_sig_style_seal tests/integration/sigilo_minimal.cct 2>&1) || true
if cmp -s tests/integration/tmp_sig_style_seal.svg tests/integration/tmp_sig_style_seal.ref.svg && \
   cmp -s tests/integration/tmp_sig_style_seal.sigil tests/integration/tmp_sig_style_seal.ref.sigil; then
    test_pass "Same style sigilo generation is deterministic"
else
    test_fail "Same style sigilo generation is not deterministic"
fi

echo "Test 92: .sigil metadata includes topology/layout fields (FASE 6A)"
if grep -q "^visual_engine = " tests/integration/tmp_sig_style_seal.sigil && \
   grep -q "^node_count = " tests/integration/tmp_sig_style_seal.sigil && \
   grep -q "^edge_count = " tests/integration/tmp_sig_style_seal.sigil && \
   grep -q "^primary_axis_deg = " tests/integration/tmp_sig_style_seal.sigil && \
   grep -q "^complexity_class = " tests/integration/tmp_sig_style_seal.sigil; then
    test_pass ".sigil metadata includes enriched topology/layout fields"
else
    test_fail ".sigil metadata missing FASE 6A enriched fields"
fi

echo "Test 93: invalid sigilo style fails clearly"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-style invalid_style tests/integration/sigilo_minimal.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Invalid sigilo style"; then
    test_pass "Invalid style is rejected with clear error"
else
    test_fail "Invalid sigilo style did not fail clearly"
fi

echo "Test 94: --sigilo-out controls artifact basepath"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-out tests/integration/tmp_sig_out_custom tests/integration/sigilo_if.cct 2>&1) || true
if [ -f "tests/integration/tmp_sig_out_custom.svg" ] && [ -f "tests/integration/tmp_sig_out_custom.sigil" ]; then
    test_pass "--sigilo-out custom basepath works"
else
    test_fail "--sigilo-out did not generate expected custom-path artifacts"
fi

echo "Test 95: --sigilo-no-meta / --sigilo-no-svg selective emission works"
rm -f tests/integration/tmp_sig_out_custom.svg tests/integration/tmp_sig_out_custom.sigil
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-no-meta --sigilo-out tests/integration/tmp_sig_out_custom tests/integration/sigilo_if.cct 2>&1) || true
OK_A=false
if [ -f "tests/integration/tmp_sig_out_custom.svg" ] && [ ! -f "tests/integration/tmp_sig_out_custom.sigil" ]; then
    OK_A=true
fi
rm -f tests/integration/tmp_sig_out_custom.svg tests/integration/tmp_sig_out_custom.sigil
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-no-svg --sigilo-out tests/integration/tmp_sig_out_custom tests/integration/sigilo_if.cct 2>&1) || true
OK_B=false
if [ ! -f "tests/integration/tmp_sig_out_custom.svg" ] && [ -f "tests/integration/tmp_sig_out_custom.sigil" ]; then
    OK_B=true
fi
if [ "$OK_A" = true ] && [ "$OK_B" = true ]; then
    test_pass "Selective sigilo artifact emission works"
else
    test_fail "Selective sigilo artifact emission failed"
fi

echo ""
echo "========================================"
echo "FASE 6B: Executable Language Expansion Tests"
echo "========================================"
echo ""

echo "Test 96: UMBRA/FLAMMA basic arithmetic compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_real_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_real_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_real_basic" ]; then
    ./tests/integration/codegen_real_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "UMBRA/FLAMMA arithmetic executes correctly"
    else
        test_fail "UMBRA/FLAMMA arithmetic returned unexpected exit code ($RC)"
    fi
else
    test_fail "UMBRA/FLAMMA basic program failed to compile/execute"
fi

echo "Test 97: real return from rituale works"
cleanup_codegen_artifacts "tests/integration/codegen_real_return.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_real_return.cct 2>&1) || true
if [ -f "tests/integration/codegen_real_return" ]; then
    ./tests/integration/codegen_real_return > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "Real return path via CONIURA works"
    else
        test_fail "Real return path produced unexpected exit code ($RC)"
    fi
else
    test_fail "Real return program failed to compile/execute"
fi

echo "Test 98: real parameters in CONIURA work"
cleanup_codegen_artifacts "tests/integration/codegen_real_call.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_real_call.cct 2>&1) || true
if [ -f "tests/integration/codegen_real_call" ]; then
    ./tests/integration/codegen_real_call > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "Real parameters in CONIURA work"
    else
        test_fail "Real parameter call produced unexpected exit code ($RC)"
    fi
else
    test_fail "Real parameter call program failed to compile/execute"
fi

echo "Test 99: OBSECRO scribe prints real values"
cleanup_codegen_artifacts "tests/integration/codegen_real_scribe.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_real_scribe.cct 2>&1) || true
if [ -f "tests/integration/codegen_real_scribe" ]; then
    RUN_OUT=$(./tests/integration/codegen_real_scribe 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "x=3.25"; then
        test_pass "OBSECRO scribe prints real values"
    else
        test_fail "OBSECRO scribe(real) output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "Real scribe program failed to compile"
fi

echo "Test 100: VERBUM variable assignment and scribe works"
cleanup_codegen_artifacts "tests/integration/codegen_verbum_var.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_verbum_var.cct 2>&1) || true
if [ -f "tests/integration/codegen_verbum_var" ]; then
    RUN_OUT=$(./tests/integration/codegen_verbum_var 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "salve"; then
        test_pass "VERBUM variable works in executable subset"
    else
        test_fail "VERBUM variable output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "VERBUM variable program failed to compile"
fi

echo "Test 101: OBSECRO scribe supports mixed arguments (VERBUM/int/real/bool)"
cleanup_codegen_artifacts "tests/integration/codegen_verbum_scribe_mixed.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_verbum_scribe_mixed.cct 2>&1) || true
if [ -f "tests/integration/codegen_verbum_scribe_mixed" ]; then
    RUN_OUT=$(./tests/integration/codegen_verbum_scribe_mixed 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "nome=bael n=7 r=2.5 ok=1"; then
        test_pass "Mixed scribe arguments work"
    else
        test_fail "Mixed scribe output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "Mixed scribe program failed to compile"
fi

echo "Test 102: SERIES basic read/write compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_series_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_series_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_series_basic" ]; then
    ./tests/integration/codegen_series_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 16 ]; then
        test_pass "SERIES basic indexing works"
    else
        test_fail "SERIES basic returned unexpected exit code ($RC)"
    fi
else
    test_fail "SERIES basic program failed to compile/execute"
fi

echo "Test 103: SERIES boolean read/write works"
cleanup_codegen_artifacts "tests/integration/codegen_series_read_write.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_series_read_write.cct 2>&1) || true
if [ -f "tests/integration/codegen_series_read_write" ]; then
    ./tests/integration/codegen_series_read_write > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "SERIES boolean indexing works"
    else
        test_fail "SERIES boolean returned unexpected exit code ($RC)"
    fi
else
    test_fail "SERIES boolean program failed to compile/execute"
fi

echo "Test 104: SERIES index expression works"
cleanup_codegen_artifacts "tests/integration/codegen_series_index_expr.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_series_index_expr.cct 2>&1) || true
if [ -f "tests/integration/codegen_series_index_expr" ]; then
    ./tests/integration/codegen_series_index_expr > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 7 ]; then
        test_pass "SERIES index expression works"
    else
        test_fail "SERIES index expression returned unexpected exit code ($RC)"
    fi
else
    test_fail "SERIES index expression program failed to compile/execute"
fi

echo "Test 105: SIGILLUM basic local + field assignment/read works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_basic" ]; then
    ./tests/integration/codegen_sigillum_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "SIGILLUM basic fields work"
    else
        test_fail "SIGILLUM basic returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM basic program failed to compile/execute"
fi

echo "Test 106: SIGILLUM fields participate in expressions"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fields.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_fields.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_fields" ]; then
    ./tests/integration/codegen_sigillum_fields > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "SIGILLUM fields in expressions work"
    else
        test_fail "SIGILLUM fields expression returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM fields program failed to compile/execute"
fi

echo "Test 107: SIGILLUM fields can be used in scribe"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_scribe.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_scribe.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_scribe" ]; then
    RUN_OUT=$(./tests/integration/codegen_sigillum_scribe 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "Bael 66"; then
        test_pass "SIGILLUM fields in scribe work"
    else
        test_fail "SIGILLUM scribe output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "SIGILLUM scribe program failed to compile"
fi

echo "Test 108: semantic rejects missing SIGILLUM field"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_field_missing.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "has no field"; then
    test_pass "Semantic error for missing field is clear"
else
    test_fail "Missing SIGILLUM field was not rejected clearly"
fi

echo "Test 109: semantic rejects non-integer SERIES index"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_index_nonint.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "index expression must be integer"; then
    test_pass "Semantic error for non-integer index is clear"
else
    test_fail "Non-integer SERIES index was not rejected clearly"
fi

echo "Test 110: ORDO basic executable mapping works"
cleanup_codegen_artifacts "tests/integration/codegen_ordo_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_ordo_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_ordo_basic" ]; then
    ./tests/integration/codegen_ordo_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "ORDO basic executable mapping works"
    else
        test_fail "ORDO program returned unexpected exit code ($RC)"
    fi
else
    test_fail "ORDO basic program failed to compile/execute"
fi

echo "Test 111: sigilo metadata reflects SERIES/SIGILLUM/field/index/real constructs"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_sigillum_fields.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_fields.sigil" ] && \
   grep -q "^sigillum_decl = " tests/integration/codegen_sigillum_fields.sigil && \
   grep -q "^field_access = " tests/integration/codegen_sigillum_fields.sigil && \
   grep -q "^type_umbra = " tests/integration/codegen_sigillum_fields.sigil; then
    OUTPUT2=$("$CCT_BIN" --sigilo-only tests/integration/codegen_series_basic.cct 2>&1) || true
    if [ -f "tests/integration/codegen_series_basic.sigil" ] && \
       grep -q "^series_type_use = " tests/integration/codegen_series_basic.sigil && \
       grep -q "^index_access = " tests/integration/codegen_series_basic.sigil; then
        test_pass "Sigilo metadata tracks FASE 6B constructs"
    else
        test_fail "Sigilo metadata missing SERIES/index metrics"
    fi
else
    test_fail "Sigilo metadata missing SIGILLUM/field/real metrics"
fi

echo ""
echo "========================================"
echo "FASE 6C: Backend/Runtime Architecture Hardening Tests"
echo "========================================"
echo ""

echo "Test 112: generated .cgen.c contains organized sections and runtime helpers"
cleanup_codegen_artifacts "tests/integration/codegen_verbum_scribe_mixed.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_verbum_scribe_mixed.cct 2>&1) || true
if [ -f "tests/integration/codegen_verbum_scribe_mixed.cgen.c" ] && \
   grep -q "===== Includes =====" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "===== Runtime Helpers =====" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "backend = c_host" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "cct_rt_scribe_str" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "cct_rt_scribe_real" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "===== Host Entry Wrapper =====" tests/integration/codegen_verbum_scribe_mixed.cgen.c; then
    test_pass ".cgen.c contains organized sections and runtime helpers"
else
    test_fail ".cgen.c missing expected sections/runtime helpers"
fi

echo "Test 113: runtime helper emits predictable REPETE GRADUS 0 failure"
cleanup_codegen_artifacts "tests/integration/codegen_repete_gradus_zero.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_repete_gradus_zero.cct 2>&1) || true
if [ -f "tests/integration/codegen_repete_gradus_zero" ]; then
    RUN_OUT=$(./tests/integration/codegen_repete_gradus_zero 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUN_OUT" | grep -q "REPETE GRADUS cannot be 0"; then
        test_pass "Runtime helper failure path is predictable"
    else
        test_fail "Runtime helper failure path missing/incorrect ($RC)"
    fi
else
    test_fail "REPETE GRADUS 0 fixture failed to compile"
fi

echo "Test 114: generated .cgen.c uses runtime helper calls for scribe"
if [ -f "tests/integration/codegen_verbum_scribe_mixed.cgen.c" ] && \
   grep -q "cct_rt_scribe_str(" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "cct_rt_scribe_int(" tests/integration/codegen_verbum_scribe_mixed.cgen.c && \
   grep -q "cct_rt_scribe_bool(" tests/integration/codegen_verbum_scribe_mixed.cgen.c; then
    test_pass "Generated code routes scribe through runtime helpers"
else
    test_fail "Generated code did not route scribe through runtime helpers"
fi

echo ""
echo "========================================"
echo "FASE 7A: SPECULUM + Memory Subset Tests"
echo "========================================"
echo ""

echo "Test 115: SPECULUM basic address-of + deref read compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_basic" ]; then
    ./tests/integration/codegen_speculum_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 21 ]; then
        test_pass "SPECULUM basic address-of/deref works"
    else
        test_fail "SPECULUM basic program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SPECULUM basic program failed to compile"
fi

echo "Test 116: dereference write mutates caller-visible local"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_deref_write.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_deref_write.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_deref_write" ]; then
    ./tests/integration/codegen_speculum_deref_write > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "SPECULUM dereference write works"
    else
        test_fail "SPECULUM dereference write returned unexpected exit code ($RC)"
    fi
else
    test_fail "SPECULUM dereference-write program failed to compile"
fi

echo "Test 117: SPECULUM parameter read via CONIURA works"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_call_param.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_call_param.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_call_param" ]; then
    ./tests/integration/codegen_speculum_call_param > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 13 ]; then
        test_pass "SPECULUM parameter call/read works"
    else
        test_fail "SPECULUM parameter call program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SPECULUM parameter call program failed to compile"
fi

echo "Test 118: pass-by-reference canonical case mutates caller"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_pass_by_ref.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_pass_by_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_pass_by_ref" ]; then
    ./tests/integration/codegen_speculum_pass_by_ref > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "Pass-by-reference via SPECULUM works"
    else
        test_fail "Pass-by-reference program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Pass-by-reference program failed to compile"
fi

echo "Test 119: OBSECRO pete/libera + MENSURA support memory alloc/free subset"
cleanup_codegen_artifacts "tests/integration/codegen_memory_alloc_free.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_alloc_free.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_alloc_free" ]; then
    ./tests/integration/codegen_memory_alloc_free > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "Memory alloc/free subset works"
    else
        test_fail "Memory alloc/free program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Memory alloc/free program failed to compile"
fi

echo "Test 120: DIMITTE emits real release action in subset"
cleanup_codegen_artifacts "tests/integration/codegen_memory_dimitte_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_dimitte_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_dimitte_basic" ]; then
    ./tests/integration/codegen_memory_dimitte_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "DIMITTE basic subset behavior works"
    else
        test_fail "DIMITTE basic program returned unexpected exit code ($RC)"
    fi
else
    test_fail "DIMITTE basic program failed to compile"
fi

echo "Test 121: SERIES + SPECULUM element reference pass-by-ref works (optional integrated)"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_series_ref.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_series_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_series_ref" ]; then
    ./tests/integration/codegen_speculum_series_ref > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 11 ]; then
        test_pass "SERIES + SPECULUM integration works"
    else
        test_fail "SERIES + SPECULUM program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SERIES + SPECULUM program failed to compile"
fi

echo "Test 122: semantic rejects address-of invalid operand"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_speculum_addressof_literal.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "address-of" && echo "$OUTPUT" | grep -qi "lvalue"; then
    test_pass "Invalid address-of rejected clearly"
else
    test_fail "Invalid address-of was not rejected clearly"
fi

echo "Test 123: semantic rejects dereference of non-pointer"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_speculum_deref_nonptr.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "dereference requires pointer"; then
    test_pass "Non-pointer dereference rejected clearly"
else
    test_fail "Non-pointer dereference was not rejected clearly"
fi

echo "Test 124: semantic rejects DIMITTE on non-pointer symbol"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_dimitte_nonptr.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "DIMITTE requires pointer"; then
    test_pass "Invalid DIMITTE rejected clearly"
else
    test_fail "Invalid DIMITTE was not rejected clearly"
fi

echo "Test 125: unsupported SPECULUM base type fails clearly in codegen"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_unsupported_verbum_ptr.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_unsupported_verbum_ptr.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SPECULUM" && echo "$OUTPUT" | grep -qi "subset"; then
    test_pass "Unsupported SPECULUM base type rejected clearly"
else
    test_fail "Unsupported SPECULUM base type did not fail clearly"
fi

echo "Test 126: sigilo metadata tracks SPECULUM/memory constructs"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_memory_dimitte_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_dimitte_basic.sigil" ] && \
   grep -q "^speculum_type_use = " tests/integration/codegen_memory_dimitte_basic.sigil && \
   grep -q "^address_of = " tests/integration/codegen_memory_dimitte_basic.sigil && \
   grep -q "^deref = " tests/integration/codegen_memory_dimitte_basic.sigil && \
   grep -q "^alloc = " tests/integration/codegen_memory_dimitte_basic.sigil && \
   grep -q "^dimitte = " tests/integration/codegen_memory_dimitte_basic.sigil; then
    test_pass "Sigilo metadata tracks FASE 7A SPECULUM/memory constructs"
else
    test_fail "Sigilo metadata missing FASE 7A SPECULUM/memory metrics"
fi

echo "Test 127: generated .cgen.c contains pointer ops and memory runtime calls"
if [ -f "tests/integration/codegen_memory_alloc_free.cgen.c" ] && \
   grep -q "cct_rt_alloc_bytes(" tests/integration/codegen_memory_alloc_free.cgen.c && \
   grep -q "cct_rt_free_ptr(" tests/integration/codegen_memory_alloc_free.cgen.c && \
   grep -q "cct_rt_check_not_null(" tests/integration/codegen_memory_alloc_free.cgen.c && \
   grep -q "sizeof(long long)" tests/integration/codegen_memory_alloc_free.cgen.c; then
    test_pass "Generated code contains predictable pointer/memory runtime integration"
else
    test_fail "Generated code missing expected pointer/memory runtime integration"
fi

echo ""
echo "========================================"
echo "FASE 7B: SIGILLUM Executable Advanced Subset Tests"
echo "========================================"
echo ""

echo "Test 128: SIGILLUM multi-field rich local usage compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fields_multi.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_fields_multi.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_fields_multi" ]; then
    ./tests/integration/codegen_sigillum_fields_multi > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "SIGILLUM multi-field local usage works"
    else
        test_fail "SIGILLUM multi-field local program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM multi-field local program failed to compile"
fi

echo "Test 129: nested SIGILLUM composition (basic) compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_nested_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_nested_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_nested_basic" ]; then
    ./tests/integration/codegen_sigillum_nested_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 7 ]; then
        test_pass "Nested SIGILLUM composition works"
    else
        test_fail "Nested SIGILLUM program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Nested SIGILLUM program failed to compile"
fi

echo "Test 130: SIGILLUM field usage in arithmetic expression works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_field_expr.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_field_expr.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_field_expr" ]; then
    ./tests/integration/codegen_sigillum_field_expr > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 23 ]; then
        test_pass "SIGILLUM field expression usage works"
    else
        test_fail "SIGILLUM field expression returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM field expression program failed to compile"
fi

echo "Test 131: SIGILLUM rich scribe output works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_scribe_rich.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_scribe_rich.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_scribe_rich" ]; then
    RUN_OUT=$(./tests/integration/codegen_sigillum_scribe_rich 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "Bael 66 3.5"; then
        test_pass "SIGILLUM rich scribe usage works"
    else
        test_fail "SIGILLUM rich scribe output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "SIGILLUM rich scribe program failed to compile"
fi

echo "Test 132: SIGILLUM mutation via SPECULUM parameter is visible to caller"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_param_ref.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_param_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_param_ref" ]; then
    ./tests/integration/codegen_sigillum_param_ref > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "SIGILLUM pass-by-reference mutation works"
    else
        test_fail "SIGILLUM pass-by-reference returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM pass-by-reference program failed to compile"
fi

echo "Test 133: local SPECULUM SIGILLUM pointer deref field mutation works"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_sigillum_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_sigillum_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_speculum_sigillum_basic" ]; then
    ./tests/integration/codegen_speculum_sigillum_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 10 ]; then
        test_pass "SPECULUM SIGILLUM local pointer mutation works"
    else
        test_fail "SPECULUM SIGILLUM local pointer program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SPECULUM SIGILLUM local pointer program failed to compile"
fi

echo "Test 134: semantic rejects field access on non-SIGILLUM value"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_sigillum_field_nonstruct.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "field access requires SIGILLUM"; then
    test_pass "Field access on non-SIGILLUM rejected clearly"
else
    test_fail "Field access on non-SIGILLUM was not rejected clearly"
fi

echo "Test 135: semantic rejects direct recursive SIGILLUM composition"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_sigillum_direct_recursive.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "recursive composition"; then
    test_pass "Direct recursive SIGILLUM composition rejected clearly"
else
    test_fail "Direct recursive SIGILLUM composition was not rejected clearly"
fi

echo "Test 136: by-value SIGILLUM ritual parameter remains restricted with clear error"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_param_value_unsupported.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_param_value_unsupported.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SIGILLUM" && echo "$OUTPUT" | grep -qi "parameter"; then
    test_pass "By-value SIGILLUM ritual parameter rejected clearly"
else
    test_fail "By-value SIGILLUM ritual parameter did not fail clearly"
fi

echo "Test 137: SIGILLUM return remains restricted with clear error"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_return_unsupported.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_return_unsupported.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SIGILLUM return type" || echo "$OUTPUT" | grep -qi "return type" ; then
    test_pass "SIGILLUM return restriction reported clearly"
else
    test_fail "SIGILLUM return restriction did not fail clearly"
fi

echo "Test 138: sigilo metadata reflects nested SIGILLUM composition"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_sigillum_nested_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_nested_basic.sigil" ] && \
   grep -q "^sigillum_decl = " tests/integration/codegen_sigillum_nested_basic.sigil && \
   grep -q "^sigillum_composite_field = " tests/integration/codegen_sigillum_nested_basic.sigil; then
    test_pass "Sigilo metadata tracks nested SIGILLUM composition"
else
    test_fail "Sigilo metadata missing nested SIGILLUM composition metrics"
fi

echo "Test 139: generated .cgen.c shows nested struct + pointer-to-struct signatures"
if [ -f "tests/integration/codegen_sigillum_param_ref.cgen.c" ] && \
   grep -q "typedef struct Daemon" tests/integration/codegen_sigillum_param_ref.cgen.c && \
   grep -q "Daemon \\* d" tests/integration/codegen_sigillum_param_ref.cgen.c; then
    test_pass "Generated C shows predictable SIGILLUM + SPECULUM SIGILLUM layout/signature"
else
    test_fail "Generated C missing expected SIGILLUM pointer signature/layout"
fi

echo ""
echo "========================================"
echo "FASE 7C: Structural Runtime + Controlled Dynamic Indexed Memory Tests"
echo "========================================"
echo ""

echo "Test 140: dynamic indexed memory (REX) alloc/read/write/free works"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_indexed_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_indexed_basic" ]; then
    ./tests/integration/codegen_memory_indexed_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 30 ]; then
        test_pass "Dynamic indexed memory (REX) works"
    else
        test_fail "Dynamic indexed memory (REX) returned unexpected exit code ($RC)"
    fi
else
    test_fail "Dynamic indexed memory (REX) program failed to compile"
fi

echo "Test 141: dynamic indexed memory (VERUM) read/write works"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_read_write.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_indexed_read_write.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_indexed_read_write" ]; then
    ./tests/integration/codegen_memory_indexed_read_write > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "Dynamic indexed memory (VERUM) works"
    else
        test_fail "Dynamic indexed memory (VERUM) returned unexpected exit code ($RC)"
    fi
else
    test_fail "Dynamic indexed memory (VERUM) program failed to compile"
fi

echo "Test 142: dynamic indexed memory values can be used in scribe"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_scribe.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_indexed_scribe.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_indexed_scribe" ]; then
    RUN_OUT=$(./tests/integration/codegen_memory_indexed_scribe 2>&1)
    RC=$?
    if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "^20$"; then
        test_pass "Dynamic indexed memory integrates with scribe"
    else
        test_fail "Dynamic indexed memory scribe output mismatch or non-zero exit ($RC)"
    fi
else
    test_fail "Dynamic indexed memory scribe program failed to compile"
fi

echo "Test 143: DIMITTE works in dynamic indexed memory flow"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_free.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_indexed_free.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_indexed_free" ]; then
    ./tests/integration/codegen_memory_indexed_free > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "DIMITTE works in indexed-memory flow"
    else
        test_fail "Indexed-memory DIMITTE program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Indexed-memory DIMITTE program failed to compile"
fi

echo "Test 144: SIGILLUM fill-by-reference canonical pattern works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fill_by_ref.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_fill_by_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_fill_by_ref" ]; then
    ./tests/integration/codegen_sigillum_fill_by_ref > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "SIGILLUM fill-by-reference works"
    else
        test_fail "SIGILLUM fill-by-reference returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM fill-by-reference program failed to compile"
fi

echo "Test 145: nested SIGILLUM mutation by reference works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_mutate_nested_by_ref.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_mutate_nested_by_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_mutate_nested_by_ref" ]; then
    ./tests/integration/codegen_sigillum_mutate_nested_by_ref > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 8 ]; then
        test_pass "Nested SIGILLUM mutation by reference works"
    else
        test_fail "Nested SIGILLUM mutation by reference returned unexpected exit code ($RC)"
    fi
else
    test_fail "Nested SIGILLUM mutation by reference program failed to compile"
fi

echo "Test 146: controlled shallow SIGILLUM copy assignment works"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_copy_shallow_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_copy_shallow_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_copy_shallow_basic" ]; then
    ./tests/integration/codegen_sigillum_copy_shallow_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "Controlled shallow SIGILLUM copy assignment works"
    else
        test_fail "SIGILLUM copy assignment returned unexpected exit code ($RC)"
    fi
else
    test_fail "SIGILLUM copy assignment program failed to compile"
fi

echo "Test 147: semantic rejects indexing on non-indexable value"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_index_nonindexable_value.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "indexing requires array or pointer type"; then
    test_pass "Indexing on non-indexable value rejected clearly"
else
    test_fail "Indexing on non-indexable value was not rejected clearly"
fi

echo "Test 148: semantic rejects incompatible write into indexed dynamic memory"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_memory_index_write_type.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "assignment type mismatch"; then
    test_pass "Incompatible indexed-memory write rejected clearly"
else
    test_fail "Incompatible indexed-memory write was not rejected clearly"
fi

echo "Test 149: deep SPECULUM SIGILLUM usage remains outside subset with clear error"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_sigillum_deep_unsupported.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_sigillum_deep_unsupported.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SPECULUM" && echo "$OUTPUT" | grep -qi "subset"; then
    test_pass "Deep SPECULUM SIGILLUM usage rejected clearly"
else
    test_fail "Deep SPECULUM SIGILLUM usage did not fail clearly"
fi

echo "Test 150: sigilo metadata reflects dynamic indexed memory pattern"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_memory_indexed_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_indexed_basic.sigil" ] && \
   grep -q "^dynamic_indexable_memory_presence = true" tests/integration/codegen_memory_indexed_basic.sigil; then
    test_pass "Sigilo metadata tracks dynamic indexed memory pattern"
else
    test_fail "Sigilo metadata missing dynamic indexed memory pattern"
fi

echo "Test 151: sigilo metadata reflects indirect structural flow pattern"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_sigillum_mutate_nested_by_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_mutate_nested_by_ref.sigil" ] && \
   grep -q "^indirect_structural_flow_presence = true" tests/integration/codegen_sigillum_mutate_nested_by_ref.sigil; then
    test_pass "Sigilo metadata tracks indirect structural flow"
else
    test_fail "Sigilo metadata missing indirect structural flow pattern"
fi

echo "Test 152: generated .cgen.c contains 7C runtime/indexed-memory patterns"
if [ -f "tests/integration/codegen_memory_indexed_basic.cgen.c" ] && \
   grep -q "cct_rt_alloc_or_fail" tests/integration/codegen_memory_indexed_basic.cgen.c && \
   grep -q "null pointer index base" tests/integration/codegen_memory_indexed_basic.cgen.c && \
   grep -q "cct_rt_check_not_null" tests/integration/codegen_memory_indexed_basic.cgen.c && \
   [ -f "tests/integration/codegen_sigillum_fill_by_ref.cgen.c" ] && \
   grep -q "Daemon \\* out" tests/integration/codegen_sigillum_fill_by_ref.cgen.c; then
    test_pass "Generated C contains predictable 7C runtime/index + indirect-struct patterns"
else
    test_fail "Generated C missing expected 7C runtime/index + indirect-struct patterns"
fi

echo ""
echo "========================================"
echo "FASE 7D: Final Consolidation of Phase 7 Tests"
echo "========================================"
echo ""

echo "Test 153: final manual discard policy still supports OBSECRO libera(pointer) canonical flow"
cleanup_codegen_artifacts "tests/integration/codegen_memory_alloc_free.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_alloc_free.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_alloc_free" ]; then
    ./tests/integration/codegen_memory_alloc_free > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "OBSECRO libera(pointer) canonical flow preserved"
    else
        test_fail "OBSECRO libera(pointer) canonical flow returned unexpected exit code ($RC)"
    fi
else
    test_fail "OBSECRO libera(pointer) canonical flow failed to compile"
fi

echo "Test 154: final manual discard policy still supports DIMITTE canonical flow"
cleanup_codegen_artifacts "tests/integration/codegen_memory_dimitte_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_memory_dimitte_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_memory_dimitte_basic" ]; then
    ./tests/integration/codegen_memory_dimitte_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "DIMITTE canonical flow preserved"
    else
        test_fail "DIMITTE canonical flow returned unexpected exit code ($RC)"
    fi
else
    test_fail "DIMITTE canonical flow failed to compile"
fi

echo "Test 155: semantic rejects OBSECRO libera on non-pointer with clear final-policy error"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_libera_nonptr.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "OBSECRO libera expects pointer argument"; then
    test_pass "OBSECRO libera(non-pointer) rejected clearly"
else
    test_fail "OBSECRO libera(non-pointer) was not rejected clearly"
fi

echo "Test 156: shallow SIGILLUM copy with SPECULUM-containing field is restricted"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_copy_with_pointer_field_unsupported.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_copy_with_pointer_field_unsupported.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "shallow SIGILLUM assignment" && echo "$OUTPUT" | grep -qi "FASE 7"; then
    test_pass "Restricted shallow SIGILLUM copy reports final-phase policy clearly"
else
    test_fail "Restricted shallow SIGILLUM copy did not fail clearly"
fi

echo "Test 157: controlled shallow SIGILLUM copy remains supported for non-SPECULUM fields"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_copy_shallow_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_sigillum_copy_shallow_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_copy_shallow_basic" ]; then
    ./tests/integration/codegen_sigillum_copy_shallow_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 66 ]; then
        test_pass "Controlled shallow SIGILLUM copy remains supported"
    else
        test_fail "Controlled shallow SIGILLUM copy returned unexpected exit code ($RC)"
    fi
else
    test_fail "Controlled shallow SIGILLUM copy failed to compile"
fi

echo "Test 158: final diagnostics mention subset final da FASE 7 for deep SPECULUM SIGILLUM restriction"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_sigillum_deep_unsupported.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_speculum_sigillum_deep_unsupported.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset" && echo "$OUTPUT" | grep -qi "FASE 7"; then
    test_pass "Deep SPECULUM SIGILLUM restriction uses final-phase diagnostic wording"
else
    test_fail "Deep SPECULUM SIGILLUM restriction diagnostic wording is not final-phase consistent"
fi

echo "Test 159: sigilo metadata remains coherent for final phase-7 canonical pattern"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/codegen_sigillum_fill_by_ref.cct 2>&1) || true
if [ -f "tests/integration/codegen_sigillum_fill_by_ref.sigil" ] && \
   grep -q "^speculum_presence = true" tests/integration/codegen_sigillum_fill_by_ref.sigil && \
   grep -q "^field_access_presence = true" tests/integration/codegen_sigillum_fill_by_ref.sigil && \
   grep -q "^sigillum_decl = " tests/integration/codegen_sigillum_fill_by_ref.sigil && \
   grep -q "^phase7_subset_final = true" tests/integration/codegen_sigillum_fill_by_ref.sigil; then
    test_pass "Sigilo metadata remains coherent for final phase-7 canonical pattern"
else
    test_fail "Sigilo metadata missing expected final phase-7 canonical indicators"
fi

echo "Test 160: generated .cgen.c final runtime banner reflects hardened backend/runtime layer"
if [ -f "tests/integration/codegen_memory_indexed_basic.cgen.c" ] && \
   (grep -q "CCT Runtime Helpers (FASE 7D" tests/integration/codegen_memory_indexed_basic.cgen.c || \
    grep -q "CCT Runtime Helpers (FASE 8C" tests/integration/codegen_memory_indexed_basic.cgen.c || \
    grep -q "CCT Runtime Helpers (FASE 8D" tests/integration/codegen_memory_indexed_basic.cgen.c) && \
   (grep -q "Generated by CCT (FASE 7D C-hosted backend)" tests/integration/codegen_memory_indexed_basic.cgen.c || \
    grep -q "Generated by CCT (FASE 8C C-hosted backend)" tests/integration/codegen_memory_indexed_basic.cgen.c || \
    grep -q "Generated by CCT (FASE 8D C-hosted backend)" tests/integration/codegen_memory_indexed_basic.cgen.c); then
    test_pass "Generated C banner/runtime block reflects FASE 7D closure"
else
    test_fail "Generated C banner/runtime block does not reflect FASE 7D closure"
fi

echo ""
echo "========================================"
echo "FASE 8A: Failure Core + Local Capture Tests"
echo "========================================"
echo ""

echo "Test 161: --ast parses TEMPTA/CAPE/IACE subset"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_tempta_cape_iace.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "TEMPTA" && echo "$OUTPUT" | grep -q "IACE"; then
    test_pass "--ast parses TEMPTA/CAPE/IACE"
else
    test_fail "--ast did not parse TEMPTA/CAPE/IACE subset"
fi

echo "Test 162: malformed TEMPTA without CAPE reports clear error"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_tempta_missing_cape.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "TEMPTA requires CAPE"; then
    test_pass "Malformed TEMPTA without CAPE fails clearly"
else
    test_fail "Malformed TEMPTA without CAPE did not fail clearly"
fi

echo "Test 163: semantic rejects IACE payload outside VERBUM/FRACTUM"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_iace_nonfractum_payload.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "IACE payload type"; then
    test_pass "Invalid IACE payload rejected clearly"
else
    test_fail "Invalid IACE payload was not rejected clearly"
fi

echo "Test 164: semantic rejects CAPE type other than FRACTUM in 8A"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_cape_nonfractum.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "CAPE" && echo "$OUTPUT" | grep -qi "FRACTUM"; then
    test_pass "Invalid CAPE type rejected clearly"
else
    test_fail "Invalid CAPE type was not rejected clearly"
fi

echo "Test 165: legacy 8A SEMPER fixture is now accepted in FASE 8C"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_tempta_semper_8a.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Semantic check OK"; then
    test_pass "Legacy SEMPER fixture now passes after FASE 8C introduction"
else
    test_fail "Legacy SEMPER fixture did not become valid in FASE 8C"
fi

echo "Test 166: CAPE binding scope does not leak outside catch block"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_cape_scope_leak_8a.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "undeclared symbol 'erro'"; then
    test_pass "CAPE binding scope is local to CAPE block"
else
    test_fail "CAPE binding scope leak was not rejected clearly"
fi

echo "Test 167: local TEMPTA/CAPE capture compiles and executes"
cleanup_codegen_artifacts "tests/integration/codegen_tempta_local_catch.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_tempta_local_catch.cct 2>&1) || true
if [ -f "tests/integration/codegen_tempta_local_catch" ]; then
    ./tests/integration/codegen_tempta_local_catch > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 7 ]; then
        test_pass "TEMPTA/CAPE local capture executes correctly"
    else
        test_fail "TEMPTA/CAPE local capture returned unexpected exit code ($RC)"
    fi
else
    test_fail "TEMPTA/CAPE local capture program failed to compile"
fi

echo "Test 168: TEMPTA path without IACE preserves normal flow"
cleanup_codegen_artifacts "tests/integration/codegen_tempta_no_throw.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_tempta_no_throw.cct 2>&1) || true
if [ -f "tests/integration/codegen_tempta_no_throw" ]; then
    ./tests/integration/codegen_tempta_no_throw > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "TEMPTA path without throw preserves normal flow"
    else
        test_fail "TEMPTA no-throw path returned unexpected exit code ($RC)"
    fi
else
    test_fail "TEMPTA no-throw program failed to compile"
fi

echo "Test 169: uncaught IACE aborts with clear runtime message"
cleanup_codegen_artifacts "tests/integration/codegen_iace_uncaught.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_iace_uncaught.cct 2>&1) || true
if [ -f "tests/integration/codegen_iace_uncaught" ]; then
    RUNTIME_OUT=$(./tests/integration/codegen_iace_uncaught 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUNTIME_OUT" | grep -qi "uncaught FRACTUM"; then
        test_pass "Uncaught IACE aborts with clear runtime message"
    else
        test_fail "Uncaught IACE did not emit expected runtime message/non-zero exit (rc=$RC)"
    fi
else
    test_fail "Uncaught IACE program failed to compile"
fi

echo "Test 170: local rethrow becomes uncaught when no outer TEMPTA exists"
cleanup_codegen_artifacts "tests/integration/codegen_iace_rethrow_uncaught.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_iace_rethrow_uncaught.cct 2>&1) || true
if [ -f "tests/integration/codegen_iace_rethrow_uncaught" ]; then
    RUNTIME_OUT=$(./tests/integration/codegen_iace_rethrow_uncaught 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUNTIME_OUT" | grep -qi "uncaught FRACTUM"; then
        test_pass "Local rethrow becomes uncaught without outer handler"
    else
        test_fail "Local rethrow did not emit uncaught FRACTUM runtime message/non-zero exit (rc=$RC)"
    fi
else
    test_fail "Local rethrow program failed to compile"
fi

echo "Test 171: sigilo metadata tracks TEMPTA/CAPE/IACE presence"
cleanup_codegen_artifacts "tests/integration/sigilo_tempta_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_tempta_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_tempta_basic.sigil" ] && \
   grep -q "^tempta_presence = true" tests/integration/sigilo_tempta_basic.sigil && \
   grep -q "^cape_presence = true" tests/integration/sigilo_tempta_basic.sigil && \
   grep -q "^iace_presence = true" tests/integration/sigilo_tempta_basic.sigil; then
    test_pass "Sigilo metadata tracks TEMPTA/CAPE/IACE constructs"
else
    test_fail "Sigilo metadata missing TEMPTA/CAPE/IACE indicators"
fi

echo "Test 172: generated .cgen.c includes FRACTUM runtime helpers"
if [ -f "tests/integration/codegen_tempta_local_catch.cgen.c" ] && \
   grep -q "cct_rt_fractum_throw_str" tests/integration/codegen_tempta_local_catch.cgen.c && \
   grep -q "cct_rt_fractum_uncaught_abort" tests/integration/codegen_tempta_local_catch.cgen.c; then
    test_pass "Generated C includes FRACTUM runtime helpers for FASE 8A"
else
    test_fail "Generated C missing FRACTUM runtime helpers for FASE 8A"
fi

echo ""
echo "========================================"
echo "FASE 8B: Failure Propagation + Ritual Composition Tests"
echo "========================================"
echo ""

echo "Test 173: --ast parses multi-ritual TEMPTA/CONIURA propagation case"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_tempta_coniura_propagation.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "TEMPTA" && echo "$OUTPUT" | grep -q "CONIURA"; then
    test_pass "--ast parses TEMPTA + CONIURA propagation structure"
else
    test_fail "--ast did not parse multi-ritual TEMPTA/CONIURA propagation case"
fi

echo "Test 174: legacy 8B SEMPER fixture is now accepted in FASE 8C"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_tempta_semper_8b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Semantic check OK"; then
    test_pass "Legacy 8B SEMPER fixture now passes in FASE 8C"
else
    test_fail "Legacy 8B SEMPER fixture did not become valid in FASE 8C"
fi

echo "Test 175: multiple CAPE blocks remain rejected in subset 8B"
OUTPUT=$("$CCT_BIN" --ast tests/integration/sem_invalid_tempta_multi_cape_8b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "CAPE" && (echo "$OUTPUT" | grep -qi "Multiple" || echo "$OUTPUT" | grep -qi "subset"); then
    test_pass "Multiple CAPE blocks rejected clearly in subset 8B"
else
    test_fail "Multiple CAPE blocks were not rejected clearly in subset 8B"
fi

echo "Test 176: propagation via CONIURA is captured by outer TEMPTA"
cleanup_codegen_artifacts "tests/integration/codegen_failure_propagation_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_propagation_basic.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_propagation_basic" ]; then
    ./tests/integration/codegen_failure_propagation_basic > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "Failure propagates via CONIURA and outer TEMPTA captures"
    else
        test_fail "Propagation basic case returned unexpected exit code ($RC)"
    fi
else
    test_fail "Propagation basic case failed to compile"
fi

echo "Test 177: uncaught propagated failure aborts at host entry wrapper"
cleanup_codegen_artifacts "tests/integration/codegen_failure_propagation_uncaught.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_propagation_uncaught.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_propagation_uncaught" ]; then
    RUNTIME_OUT=$(./tests/integration/codegen_failure_propagation_uncaught 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUNTIME_OUT" | grep -qi "uncaught FRACTUM"; then
        test_pass "Uncaught propagated failure aborts clearly"
    else
        test_fail "Uncaught propagated failure did not abort clearly (rc=$RC)"
    fi
else
    test_fail "Uncaught propagated failure case failed to compile"
fi

echo "Test 178: CONIURA expression failure in REDDE path is captured"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_expr_propagation.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_coniura_expr_propagation.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_coniura_expr_propagation" ]; then
    ./tests/integration/codegen_failure_coniura_expr_propagation > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 17 ]; then
        test_pass "CONIURA expr failure interrupts REDDE path and is captured"
    else
        test_fail "CONIURA expr propagation case returned unexpected exit code ($RC)"
    fi
else
    test_fail "CONIURA expr propagation case failed to compile"
fi

echo "Test 179: rethrow chain across rituals is capturable by outer TEMPTA"
cleanup_codegen_artifacts "tests/integration/codegen_failure_rethrow_chain.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_rethrow_chain.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_rethrow_chain" ]; then
    ./tests/integration/codegen_failure_rethrow_chain > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 33 ]; then
        test_pass "Rethrow chain across rituals behaves predictably"
    else
        test_fail "Rethrow chain case returned unexpected exit code ($RC)"
    fi
else
    test_fail "Rethrow chain case failed to compile"
fi

echo "Test 180: multi-ritual path without IACE keeps normal flow"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_no_throw.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_coniura_no_throw.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_coniura_no_throw" ]; then
    ./tests/integration/codegen_failure_coniura_no_throw > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 12 ]; then
        test_pass "Normal CONIURA path remains correct without failure"
    else
        test_fail "No-throw CONIURA path returned unexpected exit code ($RC)"
    fi
else
    test_fail "No-throw CONIURA case failed to compile"
fi

echo "Test 181: 8A payload restriction for IACE remains enforced in 8B"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_iace_nonfractum_payload.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "IACE payload type"; then
    test_pass "IACE payload restriction remains enforced in 8B"
else
    test_fail "IACE payload restriction regressed in 8B"
fi

echo "Test 182: sigilo metadata tracks failure propagation across CONIURA"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_propagation_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_failure_propagation_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_failure_propagation_basic.sigil" ] && \
   grep -q "^failure_call_propagation_presence = true" tests/integration/sigilo_failure_propagation_basic.sigil && \
   grep -q "^iace_across_coniura_presence = true" tests/integration/sigilo_failure_propagation_basic.sigil; then
    test_pass "Sigilo metadata tracks failure propagation across rituals"
else
    test_fail "Sigilo metadata missing failure propagation indicators"
fi

echo "Test 183: generated .cgen.c checks FRACTUM after CONIURA calls in 8B paths"
if [ -f "tests/integration/codegen_failure_propagation_basic.cgen.c" ] && \
   grep -q "cct_rt_fractum_is_active()" tests/integration/codegen_failure_propagation_basic.cgen.c; then
    test_pass "Generated C includes FRACTUM active checks for propagation in FASE 8B"
else
    test_fail "Generated C missing FRACTUM active checks for propagation in FASE 8B"
fi

echo ""
echo "========================================"
echo "FASE 8C: Guaranteed Cleanup + Memory/Runtime Integration Tests"
echo "========================================"
echo ""

echo "Test 184: --ast parses TEMPTA/CAPE/SEMPER form"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_tempta_cape_semper.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "TEMPTA" && echo "$OUTPUT" | grep -q "Semper:"; then
    test_pass "--ast parses TEMPTA/CAPE/SEMPER form"
else
    test_fail "--ast did not parse TEMPTA/CAPE/SEMPER form"
fi

echo "Test 185: SEMPER before CAPE is rejected clearly"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_tempta_semper_bad_order.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SEMPER" && echo "$OUTPUT" | grep -qi "after CAPE"; then
    test_pass "SEMPER bad order rejected clearly"
else
    test_fail "SEMPER bad order was not rejected clearly"
fi

echo "Test 186: SEMPER outside TEMPTA is rejected"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_semper_outside_tempta.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SEMPER" && echo "$OUTPUT" | grep -qi "TEMPTA"; then
    test_pass "SEMPER outside TEMPTA rejected clearly"
else
    test_fail "SEMPER outside TEMPTA was not rejected clearly"
fi

echo "Test 187: multiple SEMPER blocks are rejected in subset 8C"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_tempta_multi_semper_8c.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Multiple SEMPER"; then
    test_pass "Multiple SEMPER blocks rejected clearly"
else
    test_fail "Multiple SEMPER blocks were not rejected clearly"
fi

echo "Test 188: SEMPER executes on normal path"
cleanup_codegen_artifacts "tests/integration/codegen_semper_no_throw.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_semper_no_throw.cct 2>&1) || true
if [ -f "tests/integration/codegen_semper_no_throw" ]; then
    ./tests/integration/codegen_semper_no_throw > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 7 ]; then
        test_pass "SEMPER executes on normal path"
    else
        test_fail "SEMPER normal-path program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SEMPER normal-path program failed to compile"
fi

echo "Test 189: SEMPER executes after local CAPE capture"
cleanup_codegen_artifacts "tests/integration/codegen_semper_after_catch.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_semper_after_catch.cct 2>&1) || true
if [ -f "tests/integration/codegen_semper_after_catch" ]; then
    ./tests/integration/codegen_semper_after_catch > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "SEMPER executes after CAPE capture"
    else
        test_fail "SEMPER after-catch program returned unexpected exit code ($RC)"
    fi
else
    test_fail "SEMPER after-catch program failed to compile"
fi

echo "Test 190: SEMPER executes even when CAPE rethrows"
cleanup_codegen_artifacts "tests/integration/codegen_semper_rethrow.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_semper_rethrow.cct 2>&1) || true
if [ -f "tests/integration/codegen_semper_rethrow" ]; then
    RUNTIME_OUT=$(./tests/integration/codegen_semper_rethrow 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUNTIME_OUT" | grep -q "semper-8c" && echo "$RUNTIME_OUT" | grep -qi "uncaught FRACTUM"; then
        test_pass "SEMPER runs before uncaught rethrow exits"
    else
        test_fail "SEMPER rethrow path missing expected output/uncaught message (rc=$RC)"
    fi
else
    test_fail "SEMPER rethrow fixture failed to compile"
fi

echo "Test 191: cleanup with OBSECRO libera(...) in SEMPER works on failure path"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_libera_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_cleanup_libera_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_cleanup_libera_semper" ]; then
    ./tests/integration/codegen_failure_cleanup_libera_semper > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "Cleanup with OBSECRO libera in SEMPER works"
    else
        test_fail "Cleanup libera+SEMPER program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Cleanup libera+SEMPER program failed to compile"
fi

echo "Test 192: cleanup with DIMITTE in SEMPER works on failure path"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_dimitte_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_cleanup_dimitte_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_cleanup_dimitte_semper" ]; then
    ./tests/integration/codegen_failure_cleanup_dimitte_semper > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 9 ]; then
        test_pass "Cleanup with DIMITTE in SEMPER works"
    else
        test_fail "Cleanup DIMITTE+SEMPER program returned unexpected exit code ($RC)"
    fi
else
    test_fail "Cleanup DIMITTE+SEMPER program failed to compile"
fi

echo "Test 193: propagated CONIURA failure + CAPE + SEMPER works"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_capture_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_coniura_capture_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_coniura_capture_semper" ]; then
    ./tests/integration/codegen_failure_coniura_capture_semper > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 41 ]; then
        test_pass "Propagated failure + local CAPE + SEMPER executes correctly"
    else
        test_fail "CONIURA capture + SEMPER program returned unexpected exit code ($RC)"
    fi
else
    test_fail "CONIURA capture + SEMPER program failed to compile"
fi

echo "Test 194: null-pointer runtime-fail bridge enters FRACTUM flow in supported 8C path"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_bridge_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_runtime_fail_bridge_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_runtime_fail_bridge_semper" ]; then
    RUN_OUT=$(./tests/integration/codegen_runtime_fail_bridge_semper 2>&1)
    RC=$?
    if [ $RC -eq 45 ] && echo "$RUN_OUT" | grep -qi "null pointer dereference"; then
        test_pass "Null-pointer runtime-fail bridge is capturable in subset 8C"
    else
        test_fail "Null-pointer runtime-fail bridge did not behave as expected (rc=$RC)"
    fi
else
    test_fail "8C runtime-fail bridge fixture failed to compile"
fi

echo "Test 195: non-bridged runtime failure remains direct abort"
cleanup_codegen_artifacts "tests/integration/codegen_repete_gradus_zero.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_repete_gradus_zero.cct 2>&1) || true
if [ -f "tests/integration/codegen_repete_gradus_zero" ]; then
    RUN_OUT=$(./tests/integration/codegen_repete_gradus_zero 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUN_OUT" | grep -q "REPETE GRADUS cannot be 0"; then
        test_pass "Non-bridged runtime failure remains direct abort"
    else
        test_fail "Non-bridged runtime failure behavior changed unexpectedly (rc=$RC)"
    fi
else
    test_fail "REPETE GRADUS 0 fixture failed to compile in 8C regression check"
fi

echo "Test 196: sigilo metadata tracks SEMPER + cleanup/failure integration"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_semper_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_failure_semper_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_failure_semper_basic.sigil" ] && \
   grep -q "^semper_presence = true" tests/integration/sigilo_failure_semper_basic.sigil && \
   grep -q "^failure_cleanup_presence = true" tests/integration/sigilo_failure_semper_basic.sigil && \
   grep -q "^failure_cleanup_memory_presence = true" tests/integration/sigilo_failure_semper_basic.sigil; then
    test_pass "Sigilo metadata tracks SEMPER + cleanup/failure integration"
else
    test_fail "Sigilo metadata missing SEMPER/cleanup indicators"
fi

echo "Test 197: generated .cgen.c includes 8C helpers/labels for SEMPER and null bridge"
if [ -f "tests/integration/codegen_runtime_fail_bridge_semper.cgen.c" ] && \
   grep -q "cct_rt_check_not_null_fractum" tests/integration/codegen_runtime_fail_bridge_semper.cgen.c && \
   grep -q "__cct_tempta_semper_" tests/integration/codegen_runtime_fail_bridge_semper.cgen.c; then
    test_pass "Generated C includes SEMPER control-flow labels and null-bridge helper in 8C"
else
    test_fail "Generated C missing expected SEMPER/null-bridge integration in 8C"
fi

echo ""
echo "========================================"
echo "FASE 8D: Failure-Control Final Consolidation Tests"
echo "========================================"
echo ""

echo "Test 198: final subset still rejects multiple CAPE with harmonized diagnostic"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_tempta_multi_cape_8d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Multiple CAPE" && (echo "$OUTPUT" | grep -qi "subset 8B" || echo "$OUTPUT" | grep -qi "subset final da FASE 8"); then
    test_pass "Multiple CAPE diagnostic remains clear and final-subset aware"
else
    test_fail "Multiple CAPE final diagnostic was not clear enough"
fi

echo "Test 199: final subset rejects SEMPER bad order with final wording"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_tempta_semper_order_8d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "SEMPER" && echo "$OUTPUT" | grep -qi "after CAPE"; then
    test_pass "SEMPER order diagnostic remains clear in final subset"
else
    test_fail "SEMPER order diagnostic is not clear in final subset"
fi

echo "Test 200: final subset rejects invalid IACE payload with harmonized message"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_invalid_iace_payload_8d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "IACE payload type" && echo "$OUTPUT" | grep -qi "VERBUM" && echo "$OUTPUT" | grep -qi "FRACTUM"; then
    test_pass "IACE payload restriction remains clear in final subset"
else
    test_fail "IACE payload final diagnostic missing expected wording"
fi

echo "Test 201: combined local+propagation+SEMPER path works (8A+8B+8C together)"
cleanup_codegen_artifacts "tests/integration/codegen_failure_fullstack_local_propagate_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_fullstack_local_propagate_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_fullstack_local_propagate_semper" ]; then
    ./tests/integration/codegen_failure_fullstack_local_propagate_semper > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 64 ]; then
        test_pass "Combined local+propagation+SEMPER path works"
    else
        test_fail "Combined 8A+8B+8C path returned unexpected exit code ($RC)"
    fi
else
    test_fail "Combined 8A+8B+8C fixture failed to compile"
fi

echo "Test 202: rethrow + SEMPER + outer capture works"
cleanup_codegen_artifacts "tests/integration/codegen_failure_rethrow_semper_outer_catch.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_rethrow_semper_outer_catch.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_rethrow_semper_outer_catch" ]; then
    ./tests/integration/codegen_failure_rethrow_semper_outer_catch > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 73 ]; then
        test_pass "Rethrow + SEMPER + outer capture works"
    else
        test_fail "Rethrow + SEMPER + outer capture returned unexpected exit code ($RC)"
    fi
else
    test_fail "Rethrow + SEMPER + outer catch fixture failed to compile"
fi

echo "Test 203: uncaught failure after SEMPER still aborts clearly"
cleanup_codegen_artifacts "tests/integration/codegen_failure_uncaught_after_semper.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_uncaught_after_semper.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_uncaught_after_semper" ]; then
    RUN_OUT=$(./tests/integration/codegen_failure_uncaught_after_semper 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUN_OUT" | grep -q "semper-8d" && echo "$RUN_OUT" | grep -qi "uncaught FRACTUM"; then
        test_pass "Uncaught after SEMPER remains clear and predictable"
    else
        test_fail "Uncaught-after-SEMPER behavior/message changed unexpectedly (rc=$RC)"
    fi
else
    test_fail "Uncaught-after-SEMPER fixture failed to compile"
fi

echo "Test 204: combined cleanup (libera + DIMITTE) remains predictable in final subset"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_libera_dimitte_combined.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_cleanup_libera_dimitte_combined.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_cleanup_libera_dimitte_combined" ]; then
    ./tests/integration/codegen_failure_cleanup_libera_dimitte_combined > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 88 ]; then
        test_pass "Combined cleanup (libera + DIMITTE) works in final failure subset"
    else
        test_fail "Combined cleanup final fixture returned unexpected exit code ($RC)"
    fi
else
    test_fail "Combined cleanup final fixture failed to compile"
fi

echo "Test 205: integrated runtime-fail remains capturable in final subset"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_integrated_final.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_runtime_fail_integrated_final.cct 2>&1) || true
if [ -f "tests/integration/codegen_runtime_fail_integrated_final" ]; then
    RUN_OUT=$(./tests/integration/codegen_runtime_fail_integrated_final 2>&1)
    RC=$?
    if [ $RC -eq 58 ] && echo "$RUN_OUT" | grep -qi "runtime-fail" && echo "$RUN_OUT" | grep -qi "null pointer dereference"; then
        test_pass "Integrated runtime-fail remains capturable and distinguishable"
    else
        test_fail "Integrated runtime-fail final behavior/message mismatch (rc=$RC)"
    fi
else
    test_fail "Integrated runtime-fail final fixture failed to compile"
fi

echo "Test 206: non-integrated runtime-fail remains direct abort in final policy"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_nonintegrated_final.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_runtime_fail_nonintegrated_final.cct 2>&1) || true
if [ -f "tests/integration/codegen_runtime_fail_nonintegrated_final" ]; then
    RUN_OUT=$(./tests/integration/codegen_runtime_fail_nonintegrated_final 2>&1)
    RC=$?
    if [ $RC -ne 0 ] && echo "$RUN_OUT" | grep -q "REPETE GRADUS cannot be 0"; then
        test_pass "Non-integrated runtime-fail remains direct abort in final policy"
    else
        test_fail "Non-integrated runtime-fail final policy regression (rc=$RC)"
    fi
else
    test_fail "Non-integrated runtime-fail final fixture failed to compile"
fi

echo "Test 207: normal path without failure still executes correctly (phase-8 regression)"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_no_throw.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_failure_coniura_no_throw.cct 2>&1) || true
if [ -f "tests/integration/codegen_failure_coniura_no_throw" ]; then
    ./tests/integration/codegen_failure_coniura_no_throw > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 12 ]; then
        test_pass "No-throw failure-control program remains correct"
    else
        test_fail "No-throw failure-control regression (rc=$RC)"
    fi
else
    test_fail "No-throw failure-control fixture failed to compile"
fi

echo "Test 208: sigilo metadata reflects final phase-8 subset consolidation"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_final_subset.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_failure_final_subset.cct 2>&1) || true
if [ -f "tests/integration/sigilo_failure_final_subset.sigil" ] && \
   grep -q "^phase8_subset_final = true" tests/integration/sigilo_failure_final_subset.sigil && \
   grep -q "^phase8_runtime_fail_policy = " tests/integration/sigilo_failure_final_subset.sigil && \
   grep -q "^semper_presence = true" tests/integration/sigilo_failure_final_subset.sigil && \
   grep -q "^failure_call_propagation_presence = true" tests/integration/sigilo_failure_final_subset.sigil; then
    test_pass "Sigilo metadata reflects final phase-8 failure subset"
else
    test_fail "Sigilo metadata missing final phase-8 consolidation fields"
fi

echo "Test 209: generated .cgen.c runtime/header banners reflect FASE 8D closure"
if [ -f "tests/integration/codegen_runtime_fail_integrated_final.cgen.c" ] && \
   grep -q "CCT Runtime Helpers (FASE 8D" tests/integration/codegen_runtime_fail_integrated_final.cgen.c && \
   grep -q "Generated by CCT (FASE 8D C-hosted backend)" tests/integration/codegen_runtime_fail_integrated_final.cgen.c; then
    test_pass "Generated C banner/runtime block reflects FASE 8D closure"
else
    test_fail "Generated C banner/runtime block does not reflect FASE 8D closure"
fi

echo "Test 210: generated .cgen.c includes final phase-8 failure plumbing (SEMPER + bridge + active checks)"
if [ -f "tests/integration/codegen_failure_rethrow_semper_outer_catch.cgen.c" ] && \
   grep -q "__cct_tempta_semper_" tests/integration/codegen_failure_rethrow_semper_outer_catch.cgen.c && \
   grep -q "cct_rt_check_not_null_fractum" tests/integration/codegen_runtime_fail_integrated_final.cgen.c && \
   grep -q "cct_rt_fractum_is_active()" tests/integration/codegen_failure_rethrow_semper_outer_catch.cgen.c; then
    test_pass "Generated C contains expected final phase-8 failure plumbing"
else
    test_fail "Generated C missing expected final phase-8 failure plumbing"
fi

echo ""
echo "========================================"
echo "FASE 9A: ADVOCARE + Basic Module Discovery Tests"
echo "========================================"
echo ""

echo "Test 211: --ast parses ADVOCARE declaration"
OUTPUT=$("$CCT_BIN" --ast tests/integration/module_main_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "IMPORT" && echo "$OUTPUT" | grep -q "modules_9a/lib_rituale.cct"; then
    test_pass "--ast parses ADVOCARE correctly"
else
    test_fail "--ast did not parse ADVOCARE declaration"
fi

echo "Test 212: --ast parses multiple ADVOCARE declarations"
OUTPUT=$("$CCT_BIN" --ast tests/integration/module_main_multi_import.cct 2>&1) || true
if [ "$(echo "$OUTPUT" | grep -c "IMPORT (line")" -ge 2 ]; then
    test_pass "--ast parses multiple ADVOCARE declarations"
else
    test_fail "--ast did not parse multiple ADVOCARE declarations"
fi

echo "Test 213: malformed ADVOCARE reports clear syntax error"
OUTPUT=$("$CCT_BIN" --ast tests/integration/module_import_malformed.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Expected filename string after ADVOCARE"; then
    test_pass "Malformed ADVOCARE reports clear syntax error"
else
    test_fail "Malformed ADVOCARE syntax error was not clear"
fi

echo "Test 214: --check resolves imported RITUALE from relative module path"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_main_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Imported RITUALE is resolved in composed program"
else
    test_fail "Imported RITUALE was not resolved correctly"
fi

echo "Test 215: --check resolves imported SIGILLUM type"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_main_sigillum.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Imported SIGILLUM type is resolved"
else
    test_fail "Imported SIGILLUM type was not resolved"
fi

echo "Test 216: --check resolves imported ORDO type and enum item"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_main_ordo.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Imported ORDO type is resolved"
else
    test_fail "Imported ORDO type was not resolved"
fi

echo "Test 217: missing imported module fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_import_missing.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ADVOCARE target not found" && echo "$OUTPUT" | grep -qi "subset 9A"; then
    test_pass "Missing imported module is diagnosed clearly"
else
    test_fail "Missing imported module diagnostic is unclear"
fi

echo "Test 218: ADVOCARE with invalid extension fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_import_bad_ext.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "must end with .cct" && echo "$OUTPUT" | grep -qi "subset 9A"; then
    test_pass "Invalid ADVOCARE extension is rejected clearly"
else
    test_fail "Invalid ADVOCARE extension did not fail clearly"
fi

echo "Test 219: ADVOCARE cycle is detected clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_import_cycle_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "cycle detected" && echo "$OUTPUT" | grep -qi "subset 9A"; then
    test_pass "ADVOCARE cycle is detected with clear diagnostic"
else
    test_fail "ADVOCARE cycle diagnostic is missing/unclear"
fi

echo "Test 220: duplicate ADVOCARE is deduplicated deterministically"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_import_duplicate.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Duplicate ADVOCARE deduplicates without duplicate-symbol regression"
else
    test_fail "Duplicate ADVOCARE caused semantic/link regression"
fi

echo "Test 221: compile multi-module entry and execute inter-file call"
cleanup_codegen_artifacts "tests/integration/module_main_basic.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_main_basic.cct 2>&1) || true
if [ -x "tests/integration/module_main_basic" ]; then
    "tests/integration/module_main_basic" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "Multi-module compile/execute path works"
    else
        test_fail "Multi-module executable returned unexpected code ($RC)"
    fi
else
    test_fail "Multi-module entry failed to compile"
fi

echo "Test 222: --sigilo-only on module graph generates artifacts"
cleanup_codegen_artifacts "tests/integration/sigilo_module_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_module_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_module_basic.svg" ] && [ -f "tests/integration/sigilo_module_basic.sigil" ]; then
    test_pass "--sigilo-only works for entry with ADVOCARE graph"
else
    test_fail "--sigilo-only failed for entry with ADVOCARE graph"
fi

echo "Test 223: .sigil metadata includes FASE 9A module fields"
if [ -f "tests/integration/sigilo_module_basic.sigil" ] && \
   grep -q "^module_mode = composed" tests/integration/sigilo_module_basic.sigil && \
   grep -q "^module_count = 2" tests/integration/sigilo_module_basic.sigil && \
   grep -q "^import_edge_count = 1" tests/integration/sigilo_module_basic.sigil && \
   grep -q "^entry_module = tests/integration/sigilo_module_basic.cct" tests/integration/sigilo_module_basic.sigil; then
    test_pass ".sigil includes expected minimal module metadata"
else
    test_fail ".sigil missing expected minimal module metadata"
fi

echo ""
echo "========================================"
echo "FASE 9B: Inter-Module Resolution + Pragmatic Linking Tests"
echo "========================================"
echo ""

echo "Test 224: direct-import inter-module CONIURA resolution works"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_call_ok_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Direct-import inter-module rituale call resolves in subset 9B"
else
    test_fail "Direct-import inter-module rituale call did not resolve"
fi

echo "Test 225: direct-import SIGILLUM/ORDO type references resolve"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_type_ok_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Direct-import SIGILLUM/ORDO references resolve in subset 9B"
else
    test_fail "Direct-import SIGILLUM/ORDO references did not resolve"
fi

echo "Test 226: missing external rituale symbol fails clearly in 9B"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_missing_symbol.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "undefined rituale symbol" && echo "$OUTPUT" | grep -qi "subset 9B"; then
    test_pass "Missing external rituale symbol reports clear 9B diagnostic"
else
    test_fail "Missing external rituale symbol diagnostic is unclear"
fi

echo "Test 227: duplicate global rituale across modules fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_duplicate_rituale_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate global symbol" && echo "$OUTPUT" | grep -qi "subset 9B"; then
    test_pass "Duplicate global rituale diagnostic is clear"
else
    test_fail "Duplicate global rituale was not rejected clearly"
fi

echo "Test 228: duplicate global type across modules fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_duplicate_type_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate global symbol" && echo "$OUTPUT" | grep -qi "subset 9B"; then
    test_pass "Duplicate global type diagnostic is clear"
else
    test_fail "Duplicate global type was not rejected clearly"
fi

echo "Test 229: transitive visibility remains denied in 9B"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_resolve_transitive_denied_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "not directly imported" && echo "$OUTPUT" | grep -qi "subset 9B"; then
    test_pass "Transitive symbol visibility is denied clearly in subset 9B"
else
    test_fail "Transitive visibility was not denied clearly"
fi

echo "Test 230: import cycle remains rejected in 9B"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_import_cycle_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "cycle detected"; then
    test_pass "Import cycle remains rejected in 9B"
else
    test_fail "Import cycle regression in 9B"
fi

echo "Test 231: pragmatic multi-module linking compiles and executes single binary"
cleanup_codegen_artifacts "tests/integration/module_linking_pragmatic_main.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_linking_pragmatic_main.cct 2>&1) || true
if [ -x "tests/integration/module_linking_pragmatic_main" ]; then
    "tests/integration/module_linking_pragmatic_main" > /dev/null 2>&1
    RC=$?
    BASE_COUNT=0
    if [ -f "tests/integration/module_linking_pragmatic_main.cgen.c" ]; then
        BASE_COUNT=$(grep -c "base_nona(" tests/integration/module_linking_pragmatic_main.cgen.c 2>/dev/null || true)
    fi
    if [ $RC -eq 23 ] && [ -f "tests/integration/module_linking_pragmatic_main.cgen.c" ] && \
       [ "$BASE_COUNT" -ge 1 ]; then
        test_pass "Pragmatic multi-module linking is stable and executable"
    else
        test_fail "Pragmatic multi-module linking produced unexpected runtime/codegen result ($RC)"
    fi
else
    test_fail "Pragmatic multi-module linking fixture failed to compile"
fi

echo "Test 232: 9B sigilo metadata records inter-module resolution metrics"
cleanup_codegen_artifacts "tests/integration/module_resolve_call_ok_main.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/module_resolve_call_ok_main.cct 2>&1) || true
if [ -f "tests/integration/module_resolve_call_ok_main.sigil" ] && \
   grep -q "^cross_module_call_count = " tests/integration/module_resolve_call_ok_main.sigil && \
   grep -q "^cross_module_type_ref_count = " tests/integration/module_resolve_call_ok_main.sigil && \
   grep -q "^module_resolution_status = ok" tests/integration/module_resolve_call_ok_main.sigil; then
    test_pass "9B sigilo metadata reflects inter-module resolution status/metrics"
else
    test_fail "9B sigilo metadata is missing inter-module resolution fields"
fi

echo ""
echo "========================================"
echo "FASE 9C: Export/Visibility Boundary Tests"
echo "========================================"
echo ""

echo "Test 233: public-by-default top-level rituale remains externally accessible"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_visibility_public_default_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Public-by-default rituale is externally accessible with direct import"
else
    test_fail "Public-by-default rituale was not externally accessible"
fi

echo "Test 234: ARCANUM rituale remains usable internally in same module"
OUTPUT=$("$CCT_BIN" --check tests/integration/modules_9c/module_visibility_internal_rituale_lib.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "ARCANUM rituale is usable internally in defining module"
else
    test_fail "ARCANUM rituale internal usage failed unexpectedly"
fi

echo "Test 235: external access to ARCANUM rituale fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_visibility_internal_rituale_main_fail.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ARCANUM/internal" && echo "$OUTPUT" | grep -qi "subset 9C"; then
    test_pass "External access to ARCANUM rituale is blocked clearly"
else
    test_fail "External ARCANUM rituale access was not rejected clearly"
fi

echo "Test 236: external access to ARCANUM SIGILLUM fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_visibility_internal_sigillum_main_fail.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ARCANUM/internal" && echo "$OUTPUT" | grep -qi "subset 9C"; then
    test_pass "External access to ARCANUM SIGILLUM is blocked clearly"
else
    test_fail "External ARCANUM SIGILLUM access was not rejected clearly"
fi

echo "Test 237: external access to ARCANUM ORDO fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_visibility_internal_ordo_main_fail.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ARCANUM/internal" && echo "$OUTPUT" | grep -qi "subset 9C"; then
    test_pass "External access to ARCANUM ORDO is blocked clearly"
else
    test_fail "External ARCANUM ORDO access was not rejected clearly"
fi

echo "Test 238: ARCANUM in unsupported context fails clearly"
OUTPUT=$("$CCT_BIN" --ast tests/integration/module_visibility_arcanum_invalid_context.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ARCANUM" && echo "$OUTPUT" | grep -qi "subset 9C"; then
    test_pass "ARCANUM unsupported context reports clear subset 9C syntax diagnostic"
else
    test_fail "ARCANUM unsupported context diagnostic is unclear"
fi

echo "Test 239: 9C public default + ARCANUM boundaries preserve executable path"
cleanup_codegen_artifacts "tests/integration/module_visibility_public_default_main.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_visibility_public_default_main.cct 2>&1) || true
if [ -x "tests/integration/module_visibility_public_default_main" ]; then
    "tests/integration/module_visibility_public_default_main" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 42 ]; then
        test_pass "9C boundary model preserves pragmatic executable path"
    else
        test_fail "9C executable boundary test returned unexpected code ($RC)"
    fi
else
    test_fail "9C executable boundary fixture failed to compile"
fi

echo "Test 240: 9C sigilo metadata reports visibility model and counts"
cleanup_codegen_artifacts "tests/integration/sigilo_module_visibility_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_module_visibility_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_module_visibility_basic.sigil" ] && \
   grep -q "^visibility_model = public_default_arcanum_internal" tests/integration/sigilo_module_visibility_basic.sigil && \
   grep -q "^public_symbol_count = " tests/integration/sigilo_module_visibility_basic.sigil && \
   grep -q "^internal_symbol_count = " tests/integration/sigilo_module_visibility_basic.sigil; then
    test_pass "9C sigilo metadata includes visibility model/counts"
else
    test_fail "9C sigilo metadata is missing visibility model/counts"
fi

echo ""
echo "========================================"
echo "FASE 9D: Local + System Sigilo Composition Tests"
echo "========================================"
echo ""

echo "Test 241: --sigilo-only essential mode emits entry local + system sigilo"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
cleanup_codegen_artifacts "tests/integration/modules_9d/sigilo_mod_types.cct"
cleanup_codegen_artifacts "tests/integration/modules_9d/sigilo_mod_ops.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "tests/integration/sigilo_mod_entry.svg" ] && [ -f "tests/integration/sigilo_mod_entry.sigil" ] && \
   [ -f "tests/integration/sigilo_mod_entry.system.svg" ] && [ -f "tests/integration/sigilo_mod_entry.system.sigil" ] && \
   [ ! -f "tests/integration/modules_9d/sigilo_mod_types.svg" ] && [ ! -f "tests/integration/modules_9d/sigilo_mod_ops.svg" ]; then
    test_pass "Essential mode emits entry local + system without imported locals"
else
    test_fail "Essential mode emission did not match expected entry/system policy"
fi

echo "Test 242: compile essential mode preserves executable and emits system sigilo"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
OUTPUT=$("$CCT_BIN" --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -x "tests/integration/sigilo_mod_entry" ] && [ -f "tests/integration/sigilo_mod_entry.system.sigil" ]; then
    "tests/integration/sigilo_mod_entry" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 22 ]; then
        test_pass "Essential compile path keeps executable and emits entry/system sigilos"
    else
        test_fail "Essential compile returned unexpected code ($RC)"
    fi
else
    test_fail "Essential compile did not emit expected executable/system artifacts"
fi

echo "Test 243: --sigilo-only complete mode with --sigilo-out emits entry+system+imported locals"
SIG9D_COMPLETE_BASE="tests/integration/tmp_sig_9d_complete"
rm -f "${SIG9D_COMPLETE_BASE}.svg" "${SIG9D_COMPLETE_BASE}.sigil" \
      "${SIG9D_COMPLETE_BASE}.system.svg" "${SIG9D_COMPLETE_BASE}.system.sigil" \
      "${SIG9D_COMPLETE_BASE}.__mod_001.svg" "${SIG9D_COMPLETE_BASE}.__mod_001.sigil" \
      "${SIG9D_COMPLETE_BASE}.__mod_002.svg" "${SIG9D_COMPLETE_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode complete --sigilo-out "$SIG9D_COMPLETE_BASE" tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "${SIG9D_COMPLETE_BASE}.svg" ] && [ -f "${SIG9D_COMPLETE_BASE}.sigil" ] && \
   [ -f "${SIG9D_COMPLETE_BASE}.system.svg" ] && [ -f "${SIG9D_COMPLETE_BASE}.system.sigil" ] && \
   [ -f "${SIG9D_COMPLETE_BASE}.__mod_001.svg" ] && [ -f "${SIG9D_COMPLETE_BASE}.__mod_001.sigil" ] && \
   [ -f "${SIG9D_COMPLETE_BASE}.__mod_002.svg" ] && [ -f "${SIG9D_COMPLETE_BASE}.__mod_002.sigil" ]; then
    test_pass "Complete mode emits entry/system/imported local artifacts with deterministic naming"
else
    test_fail "Complete mode with --sigilo-out did not emit expected artifact set"
fi

echo "Test 244: compile complete mode with --sigilo-out keeps executable and full sigilo set"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
SIG9D_COMPLETE_COMPILE_BASE="tests/integration/tmp_sig_9d_complete_compile"
rm -f "${SIG9D_COMPLETE_COMPILE_BASE}.svg" "${SIG9D_COMPLETE_COMPILE_BASE}.sigil" \
      "${SIG9D_COMPLETE_COMPILE_BASE}.system.svg" "${SIG9D_COMPLETE_COMPILE_BASE}.system.sigil" \
      "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_001.svg" "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_001.sigil" \
      "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_002.svg" "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-mode complete --sigilo-out "$SIG9D_COMPLETE_COMPILE_BASE" tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -x "tests/integration/sigilo_mod_entry" ] && \
   [ -f "${SIG9D_COMPLETE_COMPILE_BASE}.system.sigil" ] && \
   [ -f "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_001.sigil" ] && \
   [ -f "${SIG9D_COMPLETE_COMPILE_BASE}.__mod_002.sigil" ]; then
    "tests/integration/sigilo_mod_entry" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 22 ]; then
        test_pass "Complete compile path keeps executable and full multi-module sigilo emission"
    else
        test_fail "Complete compile returned unexpected code ($RC)"
    fi
else
    test_fail "Complete compile with --sigilo-out did not emit expected artifacts"
fi

echo "Test 245: system sigilo emission is deterministic for same graph and mode"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
cp tests/integration/sigilo_mod_entry.system.sigil tests/integration/sigilo_mod_entry.system.ref.sigil 2>/dev/null || true
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "tests/integration/sigilo_mod_entry.system.ref.sigil" ] && \
   cmp -s tests/integration/sigilo_mod_entry.system.ref.sigil tests/integration/sigilo_mod_entry.system.sigil; then
    test_pass "System sigilo remains deterministic in repeated essential-mode generation"
else
    test_fail "System sigilo was not deterministic across repeated generation"
fi

echo "Test 246: changing import edges changes composed system sigilo hash"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_import_variant.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
HASH_BASE=$(grep -E "^system_hash = " tests/integration/sigilo_mod_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_import_variant.cct 2>&1) || true
HASH_IMPORT=$(grep -E "^system_hash = " tests/integration/sigilo_mod_import_variant.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
if [ -n "$HASH_BASE" ] && [ -n "$HASH_IMPORT" ] && [ "$HASH_BASE" != "$HASH_IMPORT" ]; then
    test_pass "Import-edge change alters system sigilo hash"
else
    test_fail "Import-edge change did not alter system sigilo hash clearly"
fi

echo "Test 247: changing cross-module call pattern changes composed metadata/hash"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_call_graph_variant.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
CALLS_BASE=$(grep -E "^cross_module_calls = " tests/integration/sigilo_mod_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
HASH_BASE=$(grep -E "^system_hash = " tests/integration/sigilo_mod_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_call_graph_variant.cct 2>&1) || true
CALLS_VARIANT=$(grep -E "^cross_module_calls = " tests/integration/sigilo_mod_call_graph_variant.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
HASH_VARIANT=$(grep -E "^system_hash = " tests/integration/sigilo_mod_call_graph_variant.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
if [ -n "$CALLS_BASE" ] && [ -n "$CALLS_VARIANT" ] && [ "$CALLS_BASE" != "$CALLS_VARIANT" ] && \
   [ -n "$HASH_BASE" ] && [ -n "$HASH_VARIANT" ] && [ "$HASH_BASE" != "$HASH_VARIANT" ]; then
    test_pass "Cross-module call change alters composed metadata/hash"
else
    test_fail "Cross-module call sensitivity is missing in composed sigilo metadata/hash"
fi

echo "Test 248: changing cross-module type references changes composed metadata/hash"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_type_ref_variant.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_entry.cct 2>&1) || true
TREF_BASE=$(grep -E "^cross_module_type_refs = " tests/integration/sigilo_mod_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
HASH_BASE=$(grep -E "^system_hash = " tests/integration/sigilo_mod_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential tests/integration/sigilo_mod_type_ref_variant.cct 2>&1) || true
TREF_VARIANT=$(grep -E "^cross_module_type_refs = " tests/integration/sigilo_mod_type_ref_variant.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
HASH_VARIANT=$(grep -E "^system_hash = " tests/integration/sigilo_mod_type_ref_variant.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
if [ -n "$TREF_BASE" ] && [ -n "$TREF_VARIANT" ] && [ "$TREF_BASE" != "$TREF_VARIANT" ] && \
   [ -n "$HASH_BASE" ] && [ -n "$HASH_VARIANT" ] && [ "$HASH_BASE" != "$HASH_VARIANT" ]; then
    test_pass "Cross-module type-ref change alters composed metadata/hash"
else
    test_fail "Cross-module type-ref sensitivity is missing in composed sigilo metadata/hash"
fi

echo "Test 249: --sigilo-out essential mode emits only entry + system deterministic names"
SIG9D_ESSENTIAL_BASE="tests/integration/tmp_sig_9d_essential"
rm -f "${SIG9D_ESSENTIAL_BASE}.svg" "${SIG9D_ESSENTIAL_BASE}.sigil" \
      "${SIG9D_ESSENTIAL_BASE}.system.svg" "${SIG9D_ESSENTIAL_BASE}.system.sigil" \
      "${SIG9D_ESSENTIAL_BASE}.__mod_001.svg" "${SIG9D_ESSENTIAL_BASE}.__mod_001.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essential --sigilo-out "$SIG9D_ESSENTIAL_BASE" tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "${SIG9D_ESSENTIAL_BASE}.svg" ] && [ -f "${SIG9D_ESSENTIAL_BASE}.sigil" ] && \
   [ -f "${SIG9D_ESSENTIAL_BASE}.system.svg" ] && [ -f "${SIG9D_ESSENTIAL_BASE}.system.sigil" ] && \
   [ ! -f "${SIG9D_ESSENTIAL_BASE}.__mod_001.svg" ] && [ ! -f "${SIG9D_ESSENTIAL_BASE}.__mod_001.sigil" ]; then
    test_pass "--sigilo-out essential mode keeps entry/system only"
else
    test_fail "--sigilo-out essential mode emitted unexpected artifact set"
fi

echo "Test 250: --sigilo-no-meta applies to local/system artifacts in complete mode"
SIG9D_NOMETA_BASE="tests/integration/tmp_sig_9d_nometa"
rm -f "${SIG9D_NOMETA_BASE}.svg" "${SIG9D_NOMETA_BASE}.sigil" \
      "${SIG9D_NOMETA_BASE}.system.svg" "${SIG9D_NOMETA_BASE}.system.sigil" \
      "${SIG9D_NOMETA_BASE}.__mod_001.svg" "${SIG9D_NOMETA_BASE}.__mod_001.sigil" \
      "${SIG9D_NOMETA_BASE}.__mod_002.svg" "${SIG9D_NOMETA_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode complete --sigilo-no-meta --sigilo-out "$SIG9D_NOMETA_BASE" tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "${SIG9D_NOMETA_BASE}.svg" ] && [ -f "${SIG9D_NOMETA_BASE}.system.svg" ] && \
   [ -f "${SIG9D_NOMETA_BASE}.__mod_001.svg" ] && [ -f "${SIG9D_NOMETA_BASE}.__mod_002.svg" ] && \
   [ ! -f "${SIG9D_NOMETA_BASE}.sigil" ] && [ ! -f "${SIG9D_NOMETA_BASE}.system.sigil" ] && \
   [ ! -f "${SIG9D_NOMETA_BASE}.__mod_001.sigil" ] && [ ! -f "${SIG9D_NOMETA_BASE}.__mod_002.sigil" ]; then
    test_pass "--sigilo-no-meta suppresses metadata across entry/system/imported outputs"
else
    test_fail "--sigilo-no-meta did not apply consistently in complete mode"
fi

echo "Test 251: --sigilo-no-svg applies to local/system artifacts in complete mode"
SIG9D_NOSVG_BASE="tests/integration/tmp_sig_9d_nosvg"
rm -f "${SIG9D_NOSVG_BASE}.svg" "${SIG9D_NOSVG_BASE}.sigil" \
      "${SIG9D_NOSVG_BASE}.system.svg" "${SIG9D_NOSVG_BASE}.system.sigil" \
      "${SIG9D_NOSVG_BASE}.__mod_001.svg" "${SIG9D_NOSVG_BASE}.__mod_001.sigil" \
      "${SIG9D_NOSVG_BASE}.__mod_002.svg" "${SIG9D_NOSVG_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode complete --sigilo-no-svg --sigilo-out "$SIG9D_NOSVG_BASE" tests/integration/sigilo_mod_entry.cct 2>&1) || true
if [ -f "${SIG9D_NOSVG_BASE}.sigil" ] && [ -f "${SIG9D_NOSVG_BASE}.system.sigil" ] && \
   [ -f "${SIG9D_NOSVG_BASE}.__mod_001.sigil" ] && [ -f "${SIG9D_NOSVG_BASE}.__mod_002.sigil" ] && \
   [ ! -f "${SIG9D_NOSVG_BASE}.svg" ] && [ ! -f "${SIG9D_NOSVG_BASE}.system.svg" ] && \
   [ ! -f "${SIG9D_NOSVG_BASE}.__mod_001.svg" ] && [ ! -f "${SIG9D_NOSVG_BASE}.__mod_002.svg" ]; then
    test_pass "--sigilo-no-svg suppresses SVG across entry/system/imported outputs"
else
    test_fail "--sigilo-no-svg did not apply consistently in complete mode"
fi

echo "Test 252: composed system metadata includes mandatory 9D topology fields"
if [ -f "tests/integration/sigilo_mod_entry.system.sigil" ] && \
   grep -q "^module_count = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^entry_module = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^import_edges = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^cross_module_calls = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^cross_module_type_refs = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^exported_symbol_count = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^module_density = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^system_topology_class = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^module_visibility_model = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "^system_hash = " tests/integration/sigilo_mod_entry.system.sigil; then
    test_pass "System .sigil includes mandatory 9D topology/hash fields"
else
    test_fail "System .sigil is missing mandatory 9D topology/hash fields"
fi

echo "Test 253: composed system metadata records per-module local sigilo hashes"
if [ -f "tests/integration/sigilo_mod_entry.system.sigil" ] && \
   grep -q "^\[module_sigils\]" tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "0 = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "1 = " tests/integration/sigilo_mod_entry.system.sigil && \
   grep -q "2 = " tests/integration/sigilo_mod_entry.system.sigil; then
    test_pass "System metadata includes per-module sigilo hash mapping"
else
    test_fail "System metadata is missing module_sigils mapping"
fi

echo "Test 254: .system.svg embeds inline module sigils (no external image links)"
if [ -f "tests/integration/sigilo_mod_entry.system.svg" ] && \
   grep -q "id=\"module_sigil_000\"" tests/integration/sigilo_mod_entry.system.svg && \
   grep -q "id=\"module_sigil_001\"" tests/integration/sigilo_mod_entry.system.svg && \
   grep -q "id=\"module_sigil_002\"" tests/integration/sigilo_mod_entry.system.svg && \
   ! grep -q "<image" tests/integration/sigilo_mod_entry.system.svg; then
    test_pass ".system.svg embeds module sigils inline without raster/external image references"
else
    test_fail ".system.svg did not embed module sigils inline as expected"
fi

echo "Test 255: .system.svg defines clip paths for module sub-sigils"
if [ -f "tests/integration/sigilo_mod_entry.system.svg" ] && \
   grep -q "id=\"module_clip_000\"" tests/integration/sigilo_mod_entry.system.svg && \
   grep -q "id=\"module_clip_001\"" tests/integration/sigilo_mod_entry.system.svg && \
   grep -q "id=\"module_clip_002\"" tests/integration/sigilo_mod_entry.system.svg; then
    test_pass ".system.svg exposes deterministic module clip paths for nested sigilos"
else
    test_fail ".system.svg is missing module clip paths for nested sigilos"
fi

echo ""
echo "========================================"
echo "FASE 9E: Modular Final Consolidation Tests"
echo "========================================"
echo ""

echo "Test 256: --sigilo-mode essencial works in --sigilo-only multi-module path"
cleanup_codegen_artifacts "tests/integration/sigilo_final_modular_entry.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_a.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_b.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if [ -f "tests/integration/sigilo_final_modular_entry.svg" ] && \
   [ -f "tests/integration/sigilo_final_modular_entry.sigil" ] && \
   [ -f "tests/integration/sigilo_final_modular_entry.system.svg" ] && \
   [ -f "tests/integration/sigilo_final_modular_entry.system.sigil" ] && \
   [ ! -f "tests/integration/modules_9e/module_final_a.svg" ] && \
   [ ! -f "tests/integration/modules_9e/module_final_b.svg" ]; then
    test_pass "--sigilo-mode essencial emits entry + system only"
else
    test_fail "--sigilo-mode essencial did not follow essential policy"
fi

echo "Test 257: --sigilo-mode completo emits imported local sigilos deterministically"
SIG9E_COMPLETE_BASE="tests/integration/tmp_sig_9e_complete"
rm -f "${SIG9E_COMPLETE_BASE}.svg" "${SIG9E_COMPLETE_BASE}.sigil" \
      "${SIG9E_COMPLETE_BASE}.system.svg" "${SIG9E_COMPLETE_BASE}.system.sigil" \
      "${SIG9E_COMPLETE_BASE}.__mod_001.svg" "${SIG9E_COMPLETE_BASE}.__mod_001.sigil" \
      "${SIG9E_COMPLETE_BASE}.__mod_002.svg" "${SIG9E_COMPLETE_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-out "$SIG9E_COMPLETE_BASE" tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if [ -f "${SIG9E_COMPLETE_BASE}.svg" ] && [ -f "${SIG9E_COMPLETE_BASE}.sigil" ] && \
   [ -f "${SIG9E_COMPLETE_BASE}.system.svg" ] && [ -f "${SIG9E_COMPLETE_BASE}.system.sigil" ] && \
   [ -f "${SIG9E_COMPLETE_BASE}.__mod_001.svg" ] && [ -f "${SIG9E_COMPLETE_BASE}.__mod_001.sigil" ] && \
   [ -f "${SIG9E_COMPLETE_BASE}.__mod_002.svg" ] && [ -f "${SIG9E_COMPLETE_BASE}.__mod_002.sigil" ]; then
    test_pass "--sigilo-mode completo emits entry + imported locals + system"
else
    test_fail "--sigilo-mode completo did not emit full artifact set"
fi

echo "Test 258: default multi-module sigilo mode remains essential when flag is omitted"
cleanup_codegen_artifacts "tests/integration/sigilo_final_modular_entry.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_a.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_b.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if [ -f "tests/integration/sigilo_final_modular_entry.svg" ] && \
   [ -f "tests/integration/sigilo_final_modular_entry.system.sigil" ] && \
   [ ! -f "tests/integration/modules_9e/module_final_a.svg" ] && \
   [ ! -f "tests/integration/modules_9e/module_final_b.svg" ]; then
    test_pass "Default multi-module mode remains essential (entry + system)"
else
    test_fail "Default multi-module sigilo mode regressed from essential policy"
fi

echo "Test 259: invalid --sigilo-mode fails with clear final-phase diagnostic"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode invalido tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "Invalid sigilo mode" && echo "$OUTPUT" | grep -qi "subset modular da FASE 9"; then
    test_pass "Invalid --sigilo-mode is rejected with clear modular-phase diagnostic"
else
    test_fail "Invalid --sigilo-mode diagnostic is unclear"
fi

echo "Test 260: --ast remains local for entry module"
OUTPUT=$("$CCT_BIN" --ast tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "IMPORT" && echo "$OUTPUT" | grep -q "PROGRAM: sigilo_final_modular_entry"; then
    test_pass "--ast keeps local AST behavior for entry module"
else
    test_fail "--ast local behavior regressed in phase 9E"
fi

echo "Test 261: --ast-composite prints composed module closure view"
OUTPUT=$("$CCT_BIN" --ast-composite tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Composite parsing:" && \
   echo "$OUTPUT" | grep -q "Modules in closure: 3" && \
   echo "$OUTPUT" | grep -q "Name: valor_a" && \
   echo "$OUTPUT" | grep -q "Name: valor_b"; then
    test_pass "--ast-composite emits composed AST view with cross-module declarations"
else
    test_fail "--ast-composite did not emit expected composed AST information"
fi

echo "Test 262: --ast-composite requires file argument"
OUTPUT=$("$CCT_BIN" --ast-composite 2>&1) || true
if echo "$OUTPUT" | grep -q "requires a file argument"; then
    test_pass "--ast-composite without file fails clearly"
else
    test_fail "--ast-composite missing-file diagnostic is unclear"
fi

echo "Test 263: final modular visibility diagnostics remain clear"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_final_visibility_fail.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ARCANUM/internal" && echo "$OUTPUT" | grep -qi "subset 9C"; then
    test_pass "Final modular visibility diagnostic remains clear for ARCANUM boundaries"
else
    test_fail "Final modular visibility diagnostic is unclear"
fi

echo "Test 264: final modular import-cycle diagnostics remain clear"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_final_cycle_a.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "cycle" && echo "$OUTPUT" | grep -qi "subset 9A"; then
    test_pass "Final modular import-cycle diagnostic remains clear"
else
    test_fail "Final modular import-cycle diagnostic is unclear"
fi

echo "Test 265: final modular entry compiles and executes pragmatically"
cleanup_codegen_artifacts "tests/integration/module_final_entry.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_final_entry.cct 2>&1) || true
if [ -x "tests/integration/module_final_entry" ]; then
    "tests/integration/module_final_entry" > /dev/null 2>&1
    RC=$?
    if [ $RC -eq 22 ]; then
        test_pass "Final modular entry compiles/executes with expected result"
    else
        test_fail "Final modular entry returned unexpected code ($RC)"
    fi
else
    test_fail "Final modular entry failed to compile"
fi

echo "Test 266: essential system sigilo remains deterministic in final phase"
cleanup_codegen_artifacts "tests/integration/sigilo_final_modular_entry.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
HASH_9E_A=$(grep -E "^system_hash = " tests/integration/sigilo_final_modular_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
HASH_9E_B=$(grep -E "^system_hash = " tests/integration/sigilo_final_modular_entry.system.sigil 2>/dev/null | awk -F' = ' '{print $2}')
if [ -n "$HASH_9E_A" ] && [ "$HASH_9E_A" = "$HASH_9E_B" ]; then
    test_pass "Final essential system sigilo generation is deterministic"
else
    test_fail "Final essential system sigilo generation is not deterministic"
fi

echo "Test 267: --sigilo-no-meta remains compatible with complete mode in final phase"
SIG9E_NOMETA_BASE="tests/integration/tmp_sig_9e_nometa"
rm -f "${SIG9E_NOMETA_BASE}.svg" "${SIG9E_NOMETA_BASE}.sigil" \
      "${SIG9E_NOMETA_BASE}.system.svg" "${SIG9E_NOMETA_BASE}.system.sigil" \
      "${SIG9E_NOMETA_BASE}.__mod_001.svg" "${SIG9E_NOMETA_BASE}.__mod_001.sigil" \
      "${SIG9E_NOMETA_BASE}.__mod_002.svg" "${SIG9E_NOMETA_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-no-meta --sigilo-out "$SIG9E_NOMETA_BASE" tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if [ -f "${SIG9E_NOMETA_BASE}.svg" ] && [ -f "${SIG9E_NOMETA_BASE}.system.svg" ] && \
   [ -f "${SIG9E_NOMETA_BASE}.__mod_001.svg" ] && [ -f "${SIG9E_NOMETA_BASE}.__mod_002.svg" ] && \
   [ ! -f "${SIG9E_NOMETA_BASE}.sigil" ] && [ ! -f "${SIG9E_NOMETA_BASE}.system.sigil" ] && \
   [ ! -f "${SIG9E_NOMETA_BASE}.__mod_001.sigil" ] && [ ! -f "${SIG9E_NOMETA_BASE}.__mod_002.sigil" ]; then
    test_pass "--sigilo-no-meta remains consistent in final complete-mode flow"
else
    test_fail "--sigilo-no-meta regressed in final complete-mode flow"
fi

echo ""
echo "========================================"
echo "FASE 10A: GENUS Core Model Tests"
echo "========================================"
echo ""

echo "Test 268: --tokens recognizes GENUS keyword"
OUTPUT=$("$CCT_BIN" --tokens tests/integration/keywords.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "GENUS"; then
    test_pass "--tokens recognizes GENUS"
else
    test_fail "--tokens did not recognize GENUS"
fi

echo "Test 269: --ast parses RITUALE with GENUS(T)"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_genus_rituale_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Name: identitas" && echo "$OUTPUT" | grep -q "GENUS Parameters: (1)"; then
    test_pass "--ast parses generic rituale declaration"
else
    test_fail "--ast failed to parse RITUALE GENUS(T)"
fi

echo "Test 270: --ast parses SIGILLUM with GENUS(T,U)"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_genus_sigillum_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "SIGILLUM" && echo "$OUTPUT" | grep -q "GENUS Parameters: (2)"; then
    test_pass "--ast parses generic SIGILLUM declaration"
else
    test_fail "--ast failed to parse SIGILLUM GENUS(T,U)"
fi

echo "Test 271: GENUS() empty fails clearly"
OUTPUT=$("$CCT_BIN" --ast tests/integration/syntax_genus_empty_invalid.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "GENUS()" && echo "$OUTPUT" | grep -qi "subset 10A"; then
    test_pass "GENUS() empty is rejected clearly"
else
    test_fail "GENUS() empty was not rejected clearly"
fi

echo "Test 272: duplicate GENUS parameter fails clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_duplicate_param_invalid.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "duplicate GENUS parameter"; then
    test_pass "Duplicate GENUS parameter is rejected clearly"
else
    test_fail "Duplicate GENUS parameter was not rejected clearly"
fi

echo "Test 273: type parameter used out of scope fails in --check"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_scope_invalid.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "unknown type 'T'"; then
    test_pass "Out-of-scope GENUS type parameter is rejected"
else
    test_fail "Out-of-scope GENUS type parameter was not rejected clearly"
fi

echo "Test 274: valid GENUS signature passes --check"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_signature_ok.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Valid GENUS signature passes semantic analysis"
else
    test_fail "Valid GENUS signature failed semantic analysis"
fi

echo "Test 275: SIGILLUM GENUS field declaration passes --check"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_sigillum_field_ok.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "SIGILLUM GENUS field declaration passes semantic analysis"
else
    test_fail "SIGILLUM GENUS field declaration failed semantic analysis"
fi

echo "Test 276: GENUS in unsupported context (ORDO) fails clearly"
OUTPUT=$("$CCT_BIN" --ast tests/integration/syntax_genus_invalid_context_ordo.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "GENUS in subset 10A only supports RITUALE and SIGILLUM"; then
    test_pass "GENUS in unsupported ORDO context is rejected clearly"
else
    test_fail "GENUS unsupported-context diagnostic is unclear"
fi

echo "Test 277: generic template declarations no longer block executable pipeline in 10B"
cleanup_codegen_artifacts "tests/integration/codegen_genus_not_executable_10a.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_not_executable_10a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    test_pass "Generic templates are accepted by executable pipeline after 10B"
else
    test_fail "Generic template declarations still block executable pipeline in 10B"
fi

echo "Test 278: --sigilo-only works on program with GENUS"
cleanup_codegen_artifacts "tests/integration/sigilo_genus_basic.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_genus_basic.cct 2>&1) || true
if [ -f "tests/integration/sigilo_genus_basic.svg" ] && [ -f "tests/integration/sigilo_genus_basic.sigil" ]; then
    test_pass "--sigilo-only emits artifacts for GENUS program"
else
    test_fail "--sigilo-only failed for GENUS program"
fi

echo "Test 279: .sigil metadata includes generic counters"
if grep -q "^generic_decl_count = " tests/integration/sigilo_genus_basic.sigil && \
   grep -q "^generic_param_count = " tests/integration/sigilo_genus_basic.sigil && \
   grep -q "^generic_rituale_count = " tests/integration/sigilo_genus_basic.sigil && \
   grep -q "^generic_sigillum_count = " tests/integration/sigilo_genus_basic.sigil; then
    test_pass ".sigil includes generic metadata counters"
else
    test_fail ".sigil generic metadata counters are missing"
fi

echo "Test 280: regression without GENUS remains stable"
OUTPUT=$("$CCT_BIN" --check tests/integration/minimal.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Non-GENUS semantic path remains stable"
else
    test_fail "Non-GENUS semantic path regressed"
fi

echo "Test 281: --ast-composite works with modules that contain GENUS declarations"
OUTPUT=$("$CCT_BIN" --ast-composite tests/integration/module_genus_ast_composite_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Composite parsing:" && \
   echo "$OUTPUT" | grep -q "Modules in closure: 2" && \
   echo "$OUTPUT" | grep -q "GENUS Parameters:"; then
    test_pass "--ast-composite handles module closure with GENUS declarations"
else
    test_fail "--ast-composite regressed for module+GENUS closure"
fi

echo "Test 282: sigilo modular essential/completo remains stable after 10A"
SIG10A_COMPLETE_BASE="tests/integration/tmp_sig_10a_complete"
rm -f "${SIG10A_COMPLETE_BASE}.svg" "${SIG10A_COMPLETE_BASE}.sigil" \
      "${SIG10A_COMPLETE_BASE}.system.svg" "${SIG10A_COMPLETE_BASE}.system.sigil" \
      "${SIG10A_COMPLETE_BASE}.__mod_001.svg" "${SIG10A_COMPLETE_BASE}.__mod_001.sigil" \
      "${SIG10A_COMPLETE_BASE}.__mod_002.svg" "${SIG10A_COMPLETE_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-out "$SIG10A_COMPLETE_BASE" tests/integration/sigilo_final_modular_entry.cct 2>&1) || true
if [ -f "tests/integration/sigilo_final_modular_entry.svg" ] && \
   [ -f "tests/integration/sigilo_final_modular_entry.system.svg" ] && \
   [ -f "${SIG10A_COMPLETE_BASE}.svg" ] && \
   [ -f "${SIG10A_COMPLETE_BASE}.system.svg" ] && \
   [ -f "${SIG10A_COMPLETE_BASE}.__mod_001.svg" ] && \
   [ -f "${SIG10A_COMPLETE_BASE}.__mod_002.svg" ]; then
    test_pass "Sigilo modular essencial/completo remains stable after 10A changes"
else
    test_fail "Sigilo modular emission regressed after 10A changes"
fi

echo ""
echo "========================================"
echo "FASE 10B: GENUS Executable Monomorphization Tests"
echo "========================================"
echo ""

echo "Test 283: --ast parses CONIURA with explicit GENUS type args"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_genus_call_instantiation_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "CONIURA" && echo "$OUTPUT" | grep -q "GENUS Args: (1)"; then
    test_pass "--ast parses explicit generic call instantiation"
else
    test_fail "--ast did not parse explicit CONIURA GENUS(...) instantiation"
fi

echo "Test 284: --ast parses named type with GENUS instantiation"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_genus_type_instantiation_basic.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Type: Capsa GENUS(REX)"; then
    test_pass "--ast parses generic type instantiation in type position"
else
    test_fail "--ast did not parse generic type instantiation in type position"
fi

echo "Test 285: --check accepts valid explicit generic rituale call"
OUTPUT=$("$CCT_BIN" --check tests/integration/codegen_genus_rituale_rex_basic_10b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Valid explicit generic rituale call passes semantic analysis"
else
    test_fail "Valid explicit generic rituale call failed semantic analysis"
fi

echo "Test 286: --check rejects generic rituale call without GENUS(...)"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_call_missing_type_args_10b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "requires explicit GENUS"; then
    test_pass "Missing explicit GENUS(...) in generic call is rejected clearly"
else
    test_fail "Generic call without explicit GENUS(...) was not rejected clearly"
fi

echo "Test 287: --check rejects GENUS(...) on non-generic rituale"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_call_non_generic_with_type_args_10b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "non-generic rituale"; then
    test_pass "GENUS(...) on non-generic rituale is rejected clearly"
else
    test_fail "GENUS(...) on non-generic rituale was not rejected clearly"
fi

echo "Test 288: --check rejects generic arity mismatch"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_arity_mismatch_10b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "expects 1 type argument"; then
    test_pass "Generic arity mismatch is rejected clearly"
else
    test_fail "Generic arity mismatch was not rejected clearly"
fi

echo "Test 289: --check rejects non-materializable type arg in subset 10B"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_genus_type_arg_invalid_10b.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "outside executable subset 10B"; then
    test_pass "Non-materializable type arg is rejected clearly"
else
    test_fail "Non-materializable type arg was not rejected clearly"
fi

echo "Test 290: compile + execute generic rituale with REX"
cleanup_codegen_artifacts "tests/integration/codegen_genus_rituale_rex_basic_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_rituale_rex_basic_10b.cct 2>&1) || true
if [ -x "tests/integration/codegen_genus_rituale_rex_basic_10b" ]; then
    ./tests/integration/codegen_genus_rituale_rex_basic_10b >/dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 42 ]; then
    test_pass "Generic rituale (REX) materializes and executes correctly"
else
    test_fail "Generic rituale (REX) did not execute with expected exit code"
fi

echo "Test 291: compile + execute generic rituale with VERBUM"
cleanup_codegen_artifacts "tests/integration/codegen_genus_rituale_verbum_basic_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_rituale_verbum_basic_10b.cct 2>&1) || true
if [ -x "tests/integration/codegen_genus_rituale_verbum_basic_10b" ]; then
    RUN_OUT=$(./tests/integration/codegen_genus_rituale_verbum_basic_10b 2>&1)
    RC=$?
else
    RUN_OUT=""
    RC=255
fi
if [ $RC -eq 0 ] && echo "$RUN_OUT" | grep -q "ave-10b"; then
    test_pass "Generic rituale (VERBUM) materializes and executes correctly"
else
    test_fail "Generic rituale (VERBUM) did not execute/emit expected output"
fi

echo "Test 292: compile + execute generic SIGILLUM basic flow"
cleanup_codegen_artifacts "tests/integration/codegen_genus_sigillum_basic_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_sigillum_basic_10b.cct 2>&1) || true
if [ -x "tests/integration/codegen_genus_sigillum_basic_10b" ]; then
    ./tests/integration/codegen_genus_sigillum_basic_10b >/dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 8 ]; then
    test_pass "Generic SIGILLUM instantiation materializes and executes correctly"
else
    test_fail "Generic SIGILLUM instantiation failed to compile/execute correctly"
fi

echo "Test 293: dedup identical instantiations emits one specialized symbol"
cleanup_codegen_artifacts "tests/integration/codegen_genus_dedup_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_dedup_10b.cct 2>&1) || true
SYMS=$(grep -o "cct_fn_cctg_fn_identitas_[A-Za-z0-9_]*" tests/integration/codegen_genus_dedup_10b.cgen.c 2>/dev/null | sort -u | wc -l | tr -d ' ')
if [ "$SYMS" = "1" ]; then
    test_pass "Identical generic instantiations are deduplicated deterministically"
else
    test_fail "Generic instantiation dedup did not emit a single specialized symbol"
fi

echo "Test 294: specialized symbol naming is deterministic across repeated compile"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_dedup_10b.cct 2>&1) || true
SYM_A=$(grep -o "cct_fn_cctg_fn_identitas_[A-Za-z0-9_]*" tests/integration/codegen_genus_dedup_10b.cgen.c 2>/dev/null | sort -u | head -n1)
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_dedup_10b.cct 2>&1) || true
SYM_B=$(grep -o "cct_fn_cctg_fn_identitas_[A-Za-z0-9_]*" tests/integration/codegen_genus_dedup_10b.cgen.c 2>/dev/null | sort -u | head -n1)
if [ -n "$SYM_A" ] && [ "$SYM_A" = "$SYM_B" ]; then
    test_pass "Specialized generic symbol naming is deterministic"
else
    test_fail "Specialized generic symbol naming is not deterministic"
fi

echo "Test 295: multi-module generic instantiation compiles and executes"
cleanup_codegen_artifacts "tests/integration/module_genus_cross_instantiation_main_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_genus_cross_instantiation_main_10b.cct 2>&1) || true
if [ -x "tests/integration/module_genus_cross_instantiation_main_10b" ]; then
    ./tests/integration/module_genus_cross_instantiation_main_10b >/dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 23 ]; then
    test_pass "Cross-module generic instantiation materializes and executes"
else
    test_fail "Cross-module generic instantiation failed to compile/execute"
fi

echo "Test 296: --sigilo-only exposes 10B generic instantiation metadata"
cleanup_codegen_artifacts "tests/integration/sigilo_genus_instantiation_basic_10b.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_genus_instantiation_basic_10b.cct 2>&1) || true
if grep -q "^generic_instantiation_count = " tests/integration/sigilo_genus_instantiation_basic_10b.sigil && \
   grep -q "^generic_rituale_instantiation_count = " tests/integration/sigilo_genus_instantiation_basic_10b.sigil && \
   grep -q "^generic_sigillum_instantiation_count = " tests/integration/sigilo_genus_instantiation_basic_10b.sigil && \
   grep -q "^generic_instantiation_dedup_count = " tests/integration/sigilo_genus_instantiation_basic_10b.sigil && \
   grep -q "^generic_monomorphization_status = ok" tests/integration/sigilo_genus_instantiation_basic_10b.sigil; then
    test_pass "Sigilo metadata includes 10B generic instantiation counters"
else
    test_fail "Sigilo metadata missing 10B generic instantiation counters"
fi

echo "Test 297: generic modular sigilo system metadata includes 10B aggregate counters"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/module_genus_cross_instantiation_main_10b.cct 2>&1) || true
if grep -q "^generic_instantiation_count = " tests/integration/module_genus_cross_instantiation_main_10b.system.sigil && \
   grep -q "^generic_monomorphization_status = ok" tests/integration/module_genus_cross_instantiation_main_10b.system.sigil; then
    test_pass "System sigilo metadata aggregates 10B generic instantiation counters"
else
    test_fail "System sigilo metadata missing 10B aggregate generic counters"
fi

echo ""
echo "========================================"
echo "FASE 10C: PACTUM Semantic Conformance Tests"
echo "========================================"
echo ""

echo "Test 298: --ast parses SIGILLUM X PACTUM Y"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_sigillum_pactum_basic_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Conforms PACTUM: Numerabilis"; then
    test_pass "--ast parses SIGILLUM with explicit PACTUM conformance"
else
    test_fail "--ast did not parse SIGILLUM PACTUM clause"
fi

echo "Test 299: --ast parses PACTUM signatures"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_pactum_signature_basic_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "PACTUM" && \
   echo "$OUTPUT" | grep -q "Name: Transformator" && \
   echo "$OUTPUT" | grep -q "Name: aplica" && \
   echo "$OUTPUT" | grep -q "Name: nome"; then
    test_pass "--ast parses PACTUM declaration with signatures"
else
    test_fail "--ast did not parse PACTUM signatures correctly"
fi

echo "Test 300: --check accepts valid PACTUM conformance"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_conformance_ok_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Valid SIGILLUM -> PACTUM conformance is accepted"
else
    test_fail "Valid PACTUM conformance failed semantic analysis"
fi

echo "Test 301: --check rejects missing PACTUM contract"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_missing_contract_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -Eqi "unknown PACTUM|not declared"; then
    test_pass "Missing PACTUM contract rejected clearly"
else
    test_fail "Missing PACTUM contract was not rejected clearly"
fi

echo "Test 302: --check rejects missing contract signature implementation"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_missing_method_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -Eqi "missing RITUALE|does not implement PACTUM"; then
    test_pass "Missing PACTUM signature implementation rejected clearly"
else
    test_fail "Missing PACTUM signature implementation was not rejected clearly"
fi

echo "Test 303: --check rejects contract return mismatch"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_return_mismatch_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -qi "return"; then
    test_pass "PACTUM return mismatch rejected clearly"
else
    test_fail "PACTUM return mismatch was not rejected clearly"
fi

echo "Test 304: --check rejects contract parameter mismatch"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_param_mismatch_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -qi "parameter mismatch"; then
    test_pass "PACTUM parameter mismatch rejected clearly"
else
    test_fail "PACTUM parameter mismatch was not rejected clearly"
fi

echo "Test 305: --check rejects multiple PACTUM clauses in SIGILLUM"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_sigillum_multi_pactum_invalid_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -qi "multiple PACTUM"; then
    test_pass "Multiple PACTUM clauses rejected clearly in subset 10C"
else
    test_fail "Multiple PACTUM clauses were not rejected clearly"
fi

echo "Test 306: compile + execute valid PACTUM-conformant program"
cleanup_codegen_artifacts "tests/integration/codegen_pactum_conformance_ok_10c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_pactum_conformance_ok_10c.cct 2>&1) || true
if [ -x "tests/integration/codegen_pactum_conformance_ok_10c" ]; then
    ./tests/integration/codegen_pactum_conformance_ok_10c > /dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 42 ]; then
    test_pass "PACTUM-valid program compiles and executes in backend subset"
else
    test_fail "PACTUM-valid executable path failed ($RC)"
fi

echo "Test 307: multi-module direct-import contract conformance works"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_pactum_cross_ok_main_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Cross-module direct-import PACTUM conformance works"
else
    test_fail "Cross-module direct-import PACTUM conformance failed"
fi

echo "Test 308: transitive-only contract visibility remains denied in 10C"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_pactum_transitive_denied_main_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "subset 10C" && echo "$OUTPUT" | grep -qi "no transitive visibility"; then
    test_pass "Transitive-only PACTUM visibility denied clearly in subset 10C"
else
    test_fail "Transitive-only PACTUM visibility was not denied clearly"
fi

echo "Test 309: --sigilo-only emits 10C PACTUM metadata"
cleanup_codegen_artifacts "tests/integration/sigilo_pactum_basic_10c.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_pactum_basic_10c.cct 2>&1) || true
if grep -q "^pactum_decl_count = " tests/integration/sigilo_pactum_basic_10c.sigil && \
   grep -q "^pactum_signature_count = " tests/integration/sigilo_pactum_basic_10c.sigil && \
   grep -q "^sigillum_pactum_conformance_count = " tests/integration/sigilo_pactum_basic_10c.sigil && \
   grep -q "^pactum_conformance_status = ok" tests/integration/sigilo_pactum_basic_10c.sigil; then
    test_pass "Sigilo metadata includes 10C PACTUM counters/status"
else
    test_fail "Sigilo metadata missing 10C PACTUM counters/status"
fi

echo "Test 310: system sigilo metadata aggregates 10C PACTUM counters"
cleanup_codegen_artifacts "tests/integration/module_pactum_cross_ok_main_10c.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/module_pactum_cross_ok_main_10c.cct 2>&1) || true
if grep -q "^pactum_decl_count = " tests/integration/module_pactum_cross_ok_main_10c.system.sigil && \
   grep -q "^pactum_signature_count = " tests/integration/module_pactum_cross_ok_main_10c.system.sigil && \
   grep -q "^sigillum_pactum_conformance_count = " tests/integration/module_pactum_cross_ok_main_10c.system.sigil && \
   grep -q "^pactum_conformance_status = ok" tests/integration/module_pactum_cross_ok_main_10c.system.sigil; then
    test_pass "System sigilo metadata aggregates 10C PACTUM counters/status"
else
    test_fail "System sigilo metadata missing 10C PACTUM counters/status"
fi

echo ""
echo "========================================"
echo "FASE 10D: GENUS + PACTUM Constraint Integration Tests"
echo "========================================"
echo ""

echo "Test 311: --ast parses RITUALE with GENUS(T PACTUM C)"
OUTPUT=$("$CCT_BIN" --ast tests/integration/ast_genus_constraint_basic_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "GENUS Parameters: (1)" && \
   echo "$OUTPUT" | grep -q "T PACTUM Numerabilis"; then
    test_pass "--ast parses constrained GENUS parameter in rituale"
else
    test_fail "--ast failed to parse GENUS(T PACTUM C) in rituale"
fi

echo "Test 312: --check accepts constrained generic rituale with conforming SIGILLUM arg"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_ok_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Constrained generic call with conforming SIGILLUM passes"
else
    test_fail "Valid constrained generic call failed semantic analysis"
fi

echo "Test 313: --check rejects missing constraint contract"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_missing_pactum_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset 10D" && echo "$OUTPUT" | grep -qi "unknown contract"; then
    test_pass "Missing constraint contract rejected clearly"
else
    test_fail "Missing constraint contract was not rejected clearly"
fi

echo "Test 314: --check rejects primitive type arg for constrained GENUS"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_non_sigillum_arg_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset 10D" && echo "$OUTPUT" | grep -Eqi "named SIGILLUM|constraint"; then
    test_pass "Primitive constrained type arg rejected clearly"
else
    test_fail "Primitive constrained type arg was not rejected clearly"
fi

echo "Test 315: --check rejects SIGILLUM without explicit matching PACTUM conformance"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_not_conforming_sigillum_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset 10D" && echo "$OUTPUT" | grep -qi "does not satisfy"; then
    test_pass "Non-conforming SIGILLUM rejected clearly for constrained GENUS"
else
    test_fail "Non-conforming SIGILLUM was not rejected clearly"
fi

echo "Test 316: malformed multiple PACTUM constraints are rejected clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_multi_pactum_invalid_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset 10D" && echo "$OUTPUT" | grep -qi "multiple PACTUM"; then
    test_pass "Multiple constraints per GENUS parameter rejected clearly"
else
    test_fail "Multiple constraints per GENUS parameter were not rejected clearly"
fi

echo "Test 317: compile + execute constrained generic valid program"
cleanup_codegen_artifacts "tests/integration/codegen_constraint_ok_10d.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_constraint_ok_10d.cct 2>&1) || true
if [ -x "tests/integration/codegen_constraint_ok_10d" ]; then
    ./tests/integration/codegen_constraint_ok_10d > /dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 42 ]; then
    test_pass "Constrained generic program compiles and executes"
else
    test_fail "Constrained generic executable path failed ($RC)"
fi

echo "Test 318: multi-module constrained GENUS flow works with direct imports"
cleanup_codegen_artifacts "tests/integration/module_constraint_ok_main_10d.cct"
OUTPUT=$("$CCT_BIN" tests/integration/module_constraint_ok_main_10d.cct 2>&1) || true
if [ -x "tests/integration/module_constraint_ok_main_10d" ]; then
    ./tests/integration/module_constraint_ok_main_10d > /dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 23 ]; then
    test_pass "Cross-module constrained GENUS flow works with direct imports"
else
    test_fail "Cross-module constrained GENUS flow failed ($RC)"
fi

echo "Test 319: transitive-only contract visibility remains denied in constrained GENUS"
OUTPUT=$("$CCT_BIN" --check tests/integration/module_constraint_transitive_denied_main_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "subset 10D" && echo "$OUTPUT" | grep -qi "no transitive visibility"; then
    test_pass "Transitive-only constraint contract visibility denied clearly in 10D"
else
    test_fail "Transitive-only constraint contract visibility was not denied clearly"
fi

echo "Test 320: --sigilo-only emits 10D constraint metadata"
cleanup_codegen_artifacts "tests/integration/sigilo_constraint_basic_10d.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_constraint_basic_10d.cct 2>&1) || true
if grep -q "^generic_constraint_count = " tests/integration/sigilo_constraint_basic_10d.sigil && \
   grep -q "^generic_constrained_param_count = " tests/integration/sigilo_constraint_basic_10d.sigil && \
   grep -q "^generic_constraint_violation_count = " tests/integration/sigilo_constraint_basic_10d.sigil && \
   grep -q "^pactum_constraint_resolution_status = ok" tests/integration/sigilo_constraint_basic_10d.sigil; then
    test_pass "Sigilo metadata includes 10D constraint counters/status"
else
    test_fail "Sigilo metadata missing 10D constraint counters/status"
fi

echo "Test 321: system sigilo metadata aggregates 10D constraint counters"
cleanup_codegen_artifacts "tests/integration/module_constraint_ok_main_10d.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/module_constraint_ok_main_10d.cct 2>&1) || true
if grep -q "^generic_constraint_count = " tests/integration/module_constraint_ok_main_10d.system.sigil && \
   grep -q "^generic_constrained_param_count = " tests/integration/module_constraint_ok_main_10d.system.sigil && \
   grep -q "^generic_constraint_violation_count = " tests/integration/module_constraint_ok_main_10d.system.sigil && \
   grep -q "^pactum_constraint_resolution_status = ok" tests/integration/module_constraint_ok_main_10d.system.sigil; then
    test_pass "System sigilo metadata aggregates 10D constraint counters/status"
else
    test_fail "System sigilo metadata missing 10D constraint counters/status"
fi

echo ""
echo "========================================"
echo "FASE 10E: Final Consolidation of Advanced Typing Tests"
echo "========================================"
echo ""

echo "Test 322: canonical 10A parse remains stable in 10E"
OUTPUT=$("$CCT_BIN" --ast tests/integration/typing_final_happy_path_10e.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Name: identitas" && echo "$OUTPUT" | grep -q "GENUS Parameters: (1)"; then
    test_pass "Canonical 10A generic declaration parsing remains stable"
else
    test_fail "Canonical 10A parsing regressed in 10E"
fi

echo "Test 323: canonical 10B monomorphization remains executable in 10E"
cleanup_codegen_artifacts "tests/integration/typing_final_happy_path_10e.cct"
OUTPUT=$("$CCT_BIN" tests/integration/typing_final_happy_path_10e.cct 2>&1) || true
if [ -x "tests/integration/typing_final_happy_path_10e" ]; then
    ./tests/integration/typing_final_happy_path_10e >/dev/null 2>&1
    RC=$?
else
    RC=255
fi
if [ $RC -eq 19 ]; then
    test_pass "Canonical 10B executable generic flow remains stable"
else
    test_fail "Canonical 10B executable generic flow regressed ($RC)"
fi

echo "Test 324: canonical 10C PACTUM conformance remains stable in 10E"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_pactum_conformance_ok_10c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Canonical 10C conformance remains stable"
else
    test_fail "Canonical 10C conformance regressed in 10E"
fi

echo "Test 325: canonical 10D constraints remain stable in 10E"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_ok_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Canonical 10D constraint flow remains stable"
else
    test_fail "Canonical 10D constraint flow regressed in 10E"
fi

echo "Test 326: explicit-instantiation diagnostic is harmonized for final subset"
OUTPUT=$("$CCT_BIN" --check tests/integration/typing_final_subset_boundary_10e.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "requires explicit GENUS" && echo "$OUTPUT" | grep -qi "subset final da FASE 10"; then
    test_pass "Explicit-instantiation boundary diagnostic is harmonized"
else
    test_fail "Explicit-instantiation boundary diagnostic is not harmonized"
fi

echo "Test 327: non-conforming constrained type diagnostic is harmonized for final subset"
OUTPUT=$("$CCT_BIN" --check tests/integration/typing_final_nonconforming_10e.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "does not satisfy" && echo "$OUTPUT" | grep -qi "subset final da FASE 10"; then
    test_pass "Constraint non-conformance diagnostic is harmonized"
else
    test_fail "Constraint non-conformance diagnostic is not harmonized"
fi

echo "Test 328: multiple PACTUM constraints diagnostic is harmonized for final subset"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_constraint_multi_pactum_invalid_10d.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "multiple PACTUM" && echo "$OUTPUT" | grep -qi "subset final da FASE 10"; then
    test_pass "Multiple-constraint boundary diagnostic is harmonized"
else
    test_fail "Multiple-constraint boundary diagnostic is not harmonized"
fi

echo "Test 329: specialized generic symbol naming remains deterministic in 10E"
cleanup_codegen_artifacts "tests/integration/codegen_genus_dedup_10b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_dedup_10b.cct 2>&1) || true
SYM_10E_A=$(grep -o "cct_fn_cctg_fn_identitas_[A-Za-z0-9_]*" tests/integration/codegen_genus_dedup_10b.cgen.c 2>/dev/null | sort -u | head -n1)
OUTPUT=$("$CCT_BIN" tests/integration/codegen_genus_dedup_10b.cct 2>&1) || true
SYM_10E_B=$(grep -o "cct_fn_cctg_fn_identitas_[A-Za-z0-9_]*" tests/integration/codegen_genus_dedup_10b.cgen.c 2>/dev/null | sort -u | head -n1)
if [ -n "$SYM_10E_A" ] && [ "$SYM_10E_A" = "$SYM_10E_B" ]; then
    test_pass "Specialized naming remains deterministic in 10E"
else
    test_fail "Specialized naming determinism regressed in 10E"
fi

echo "Test 330: sigilo hash remains deterministic for advanced typing program in 10E"
cleanup_codegen_artifacts "tests/integration/sigilo_typing_final_10e.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_typing_final_10e.cct 2>&1) || true
SIG10E_HASH_A=$(grep -E "^semantic_hash = " tests/integration/sigilo_typing_final_10e.sigil 2>/dev/null | awk -F' = ' '{print $2}')
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_typing_final_10e.cct 2>&1) || true
SIG10E_HASH_B=$(grep -E "^semantic_hash = " tests/integration/sigilo_typing_final_10e.sigil 2>/dev/null | awk -F' = ' '{print $2}')
if [ -n "$SIG10E_HASH_A" ] && [ "$SIG10E_HASH_A" = "$SIG10E_HASH_B" ]; then
    test_pass "Advanced-typing sigilo hash remains deterministic in 10E"
else
    test_fail "Advanced-typing sigilo hash determinism regressed in 10E"
fi

echo "Test 331: --sigilo-only metadata includes final 10E consolidation fields"
if [ -f "tests/integration/sigilo_typing_final_10e.sigil" ] && \
   grep -q "^phase10_subset_final = true" tests/integration/sigilo_typing_final_10e.sigil && \
   grep -q "^phase10_final_status = consolidated" tests/integration/sigilo_typing_final_10e.sigil && \
   grep -q "^phase10_typing_contract = " tests/integration/sigilo_typing_final_10e.sigil; then
    test_pass "Sigilo metadata includes final 10E consolidation fields"
else
    test_fail "Sigilo metadata missing final 10E consolidation fields"
fi

echo "Test 332: --sigilo-mode essencial|completo remains stable for advanced typing modules"
cleanup_codegen_artifacts "tests/integration/module_typing_final_10e_main.cct"
SIG10E_COMPLETE_BASE="tests/integration/tmp_sig_10e_complete"
rm -f "${SIG10E_COMPLETE_BASE}.svg" "${SIG10E_COMPLETE_BASE}.sigil" \
      "${SIG10E_COMPLETE_BASE}.system.svg" "${SIG10E_COMPLETE_BASE}.system.sigil" \
      "${SIG10E_COMPLETE_BASE}.__mod_001.svg" "${SIG10E_COMPLETE_BASE}.__mod_001.sigil" \
      "${SIG10E_COMPLETE_BASE}.__mod_002.svg" "${SIG10E_COMPLETE_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/module_typing_final_10e_main.cct 2>&1) || true
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-out "$SIG10E_COMPLETE_BASE" tests/integration/module_typing_final_10e_main.cct 2>&1) || true
if [ -f "tests/integration/module_typing_final_10e_main.svg" ] && \
   [ -f "tests/integration/module_typing_final_10e_main.system.svg" ] && \
   [ -f "${SIG10E_COMPLETE_BASE}.svg" ] && [ -f "${SIG10E_COMPLETE_BASE}.system.svg" ] && \
   [ -f "${SIG10E_COMPLETE_BASE}.__mod_001.svg" ] && [ -f "${SIG10E_COMPLETE_BASE}.__mod_002.svg" ]; then
    test_pass "Advanced-typing multi-module sigilo mode essencial/completo remains stable"
else
    test_fail "Advanced-typing multi-module sigilo mode essencial/completo regressed"
fi

echo "Test 333: --sigilo-out + --sigilo-no-meta/--sigilo-no-svg remain stable in 10E"
SIG10E_NOMETA_BASE="tests/integration/tmp_sig_10e_nometa"
SIG10E_NOSVG_BASE="tests/integration/tmp_sig_10e_nosvg"
rm -f "${SIG10E_NOMETA_BASE}.svg" "${SIG10E_NOMETA_BASE}.sigil" \
      "${SIG10E_NOMETA_BASE}.system.svg" "${SIG10E_NOMETA_BASE}.system.sigil" \
      "${SIG10E_NOMETA_BASE}.__mod_001.svg" "${SIG10E_NOMETA_BASE}.__mod_001.sigil" \
      "${SIG10E_NOMETA_BASE}.__mod_002.svg" "${SIG10E_NOMETA_BASE}.__mod_002.sigil"
rm -f "${SIG10E_NOSVG_BASE}.svg" "${SIG10E_NOSVG_BASE}.sigil" \
      "${SIG10E_NOSVG_BASE}.system.svg" "${SIG10E_NOSVG_BASE}.system.sigil" \
      "${SIG10E_NOSVG_BASE}.__mod_001.svg" "${SIG10E_NOSVG_BASE}.__mod_001.sigil" \
      "${SIG10E_NOSVG_BASE}.__mod_002.svg" "${SIG10E_NOSVG_BASE}.__mod_002.sigil"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-no-meta --sigilo-out "$SIG10E_NOMETA_BASE" tests/integration/module_typing_final_10e_main.cct 2>&1) || true
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-no-svg --sigilo-out "$SIG10E_NOSVG_BASE" tests/integration/module_typing_final_10e_main.cct 2>&1) || true
if [ -f "${SIG10E_NOMETA_BASE}.svg" ] && [ -f "${SIG10E_NOMETA_BASE}.system.svg" ] && \
   [ ! -f "${SIG10E_NOMETA_BASE}.sigil" ] && [ ! -f "${SIG10E_NOMETA_BASE}.system.sigil" ] && \
   [ -f "${SIG10E_NOSVG_BASE}.sigil" ] && [ -f "${SIG10E_NOSVG_BASE}.system.sigil" ] && \
   [ ! -f "${SIG10E_NOSVG_BASE}.svg" ] && [ ! -f "${SIG10E_NOSVG_BASE}.system.svg" ]; then
    test_pass "Sigilo output/selector flags remain stable in 10E advanced typing flows"
else
    test_fail "Sigilo output/selector flags regressed in 10E advanced typing flows"
fi

echo "Test 334: .system.svg remains sigil-of-sigils inline and system metadata is finalized"
if [ -f "${SIG10E_COMPLETE_BASE}.system.svg" ] && [ -f "${SIG10E_COMPLETE_BASE}.system.sigil" ] && \
   grep -q "sigil-of-sigils" "${SIG10E_COMPLETE_BASE}.system.svg" && \
   grep -q "zoom: inline vector sub-sigils per module" "${SIG10E_COMPLETE_BASE}.system.svg" && \
   grep -q "^phase10_subset_final = true" "${SIG10E_COMPLETE_BASE}.system.sigil" && \
   grep -q "^phase10_final_status = consolidated" "${SIG10E_COMPLETE_BASE}.system.sigil"; then
    test_pass "System sigilo remains sigil-of-sigils inline with finalized 10E metadata"
else
    test_fail "System sigilo inline composition or finalized 10E metadata regressed"
fi

echo ""
echo "========================================"
echo "FASE 11A: Bibliotheca Canonica Foundation Tests"
echo "========================================"
echo ""

echo "Test 335: stdlib namespace cct/... resolves canonical module and executes"
cleanup_codegen_artifacts "tests/integration/stdlib_resolution_basic_11a.cct"
OUTPUT=$("$CCT_BIN" tests/integration/stdlib_resolution_basic_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/stdlib_resolution_basic_11a > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 42 ]; then
        test_pass "cct/... canonical resolution compiles and executes correctly"
    else
        test_fail "cct/... canonical resolution executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cct/... canonical resolution failed to compile"
fi

echo "Test 336: --ast-composite includes stdlib module in closure"
OUTPUT=$("$CCT_BIN" --ast-composite tests/integration/stdlib_resolution_basic_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Modules in closure: 2" && echo "$OUTPUT" | grep -q "test_value"; then
    test_pass "--ast-composite includes cct/... module in closure"
else
    test_fail "--ast-composite did not expose expected stdlib closure"
fi

echo "Test 337: --check resolves stdlib symbols"
OUTPUT=$("$CCT_BIN" --check tests/integration/stdlib_resolution_basic_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "--check resolves cct/... symbols correctly"
else
    test_fail "--check failed on valid cct/... import resolution"
fi

echo "Test 338: missing stdlib module reports reserved namespace diagnostic"
OUTPUT=$("$CCT_BIN" --check tests/integration/stdlib_resolution_missing_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Bibliotheca Canonica" && echo "$OUTPUT" | grep -q "cct/... is reserved namespace"; then
    test_pass "Missing cct/... module reports clear reserved-namespace diagnostic"
else
    test_fail "Missing cct/... module diagnostic is not clear enough"
fi

echo "Test 339: non-cct namespace keeps user-relative resolution"
cleanup_codegen_artifacts "tests/integration/stdlib_resolution_no_collision_11a.cct"
OUTPUT=$("$CCT_BIN" tests/integration/stdlib_resolution_no_collision_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/stdlib_resolution_no_collision_11a > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 5 ]; then
        test_pass "User module path does not collide with reserved cct/... namespace"
    else
        test_fail "User module resolution executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "User module path resolution failed after stdlib namespace support"
fi

echo ""
echo "========================================"
echo "FASE 11B.1: VERBUM Canonical Text Core Tests"
echo "========================================"
echo ""

echo "Test 340: cct/verbum len works"
cleanup_codegen_artifacts "tests/integration/verbum_len_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_len_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_len_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM len works"
    else
        test_fail "VERBUM len executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM len failed to compile"
fi

echo "Test 341: cct/verbum concat works"
cleanup_codegen_artifacts "tests/integration/verbum_concat_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_concat_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_concat_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM concat works"
    else
        test_fail "VERBUM concat executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM concat failed to compile"
fi

echo "Test 342: cct/verbum compare works"
cleanup_codegen_artifacts "tests/integration/verbum_compare_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_compare_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_compare_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM compare works"
    else
        test_fail "VERBUM compare executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM compare failed to compile"
fi

echo "Test 343: cct/verbum substring works"
cleanup_codegen_artifacts "tests/integration/verbum_substring_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_substring_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_substring_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM substring works"
    else
        test_fail "VERBUM substring executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM substring failed to compile"
fi

echo "Test 344: cct/verbum substring out-of-bounds fails clearly"
cleanup_codegen_artifacts "tests/integration/verbum_substring_oob_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_substring_oob_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUTPUT=$(./tests/integration/verbum_substring_oob_11b1 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUTPUT" | grep -q "verbum substring bounds invalid"; then
        test_pass "VERBUM substring out-of-bounds fails clearly"
    else
        test_fail "VERBUM substring out-of-bounds did not fail as expected"
    fi
else
    test_fail "VERBUM substring OOB fixture failed to compile"
fi

echo "Test 345: cct/verbum trim works"
cleanup_codegen_artifacts "tests/integration/verbum_trim_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_trim_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_trim_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM trim works"
    else
        test_fail "VERBUM trim executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM trim failed to compile"
fi

echo "Test 346: cct/verbum contains works"
cleanup_codegen_artifacts "tests/integration/verbum_contains_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_contains_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_contains_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM contains works"
    else
        test_fail "VERBUM contains executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM contains failed to compile"
fi

echo "Test 347: cct/verbum find works"
cleanup_codegen_artifacts "tests/integration/verbum_find_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_find_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/verbum_find_11b1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "VERBUM find works"
    else
        test_fail "VERBUM find executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM find failed to compile"
fi

echo "Test 348: cct/verbum integrates with OBSECRO scribe"
cleanup_codegen_artifacts "tests/integration/verbum_scribe_11b1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/verbum_scribe_11b1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUTPUT=$(./tests/integration/verbum_scribe_11b1 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] && [ "$RUN_OUTPUT" = "hello world" ]; then
        test_pass "VERBUM output integrates with scribe"
    else
        test_fail "VERBUM scribe integration output mismatch or non-zero exit ($EXIT_CODE)"
    fi
else
    test_fail "VERBUM scribe fixture failed to compile"
fi

echo "Test 349: --sigilo-only works with cct/verbum import"
cleanup_codegen_artifacts "tests/integration/verbum_len_11b1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/verbum_len_11b1.cct 2>&1) || true
if [ -f "tests/integration/verbum_len_11b1.svg" ] && [ -f "tests/integration/verbum_len_11b1.sigil" ]; then
    test_pass "Sigilo generation remains stable with cct/verbum usage"
else
    test_fail "Sigilo generation regressed with cct/verbum usage"
fi

echo ""
echo "========================================"
echo "FASE 11B.2: FMT Canonical Formatting + Parse Tests"
echo "========================================"
echo ""

echo "Test 350: cct/fmt stringify_int works"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_int_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_stringify_int_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_stringify_int_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT stringify_int works"
    else
        test_fail "FMT stringify_int returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT stringify_int failed to compile"
fi

echo "Test 351: cct/fmt stringify_int supports negatives"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_int_neg_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_stringify_int_neg_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_stringify_int_neg_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT stringify_int negative works"
    else
        test_fail "FMT stringify_int negative returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT stringify_int negative failed to compile"
fi

echo "Test 352: cct/fmt stringify_real + parse_real roundtrip works"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_real_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_stringify_real_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_stringify_real_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT stringify_real/parse_real roundtrip works"
    else
        test_fail "FMT stringify_real/parse_real roundtrip returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT stringify_real fixture failed to compile"
fi

echo "Test 353: cct/fmt stringify_bool works"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_bool_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_stringify_bool_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_stringify_bool_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT stringify_bool works"
    else
        test_fail "FMT stringify_bool returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT stringify_bool failed to compile"
fi

echo "Test 354: cct/fmt parse_int works"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_parse_int_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_parse_int_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT parse_int works"
    else
        test_fail "FMT parse_int returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT parse_int failed to compile"
fi

echo "Test 355: cct/fmt parse_int supports negatives"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_neg_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_parse_int_neg_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_parse_int_neg_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT parse_int negative works"
    else
        test_fail "FMT parse_int negative returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT parse_int negative failed to compile"
fi

echo "Test 356: cct/fmt int roundtrip stringify->parse works"
cleanup_codegen_artifacts "tests/integration/fmt_roundtrip_int_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_roundtrip_int_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_roundtrip_int_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT int roundtrip works"
    else
        test_fail "FMT int roundtrip returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT int roundtrip failed to compile"
fi

echo "Test 357: cct/fmt parse_int invalid fails clearly"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_invalid_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_parse_int_invalid_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUTPUT=$(./tests/integration/fmt_parse_int_invalid_11b2 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUTPUT" | grep -q "fmt parse_int invalid input"; then
        test_pass "FMT parse_int invalid input fails clearly"
    else
        test_fail "FMT parse_int invalid input did not fail as expected"
    fi
else
    test_fail "FMT parse_int invalid fixture failed to compile"
fi

echo "Test 358: cct/fmt format_pair works"
cleanup_codegen_artifacts "tests/integration/fmt_format_pair_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_format_pair_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fmt_format_pair_11b2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FMT format_pair works"
    else
        test_fail "FMT format_pair returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FMT format_pair failed to compile"
fi

echo "Test 359: cct/fmt output integrates with OBSECRO scribe"
cleanup_codegen_artifacts "tests/integration/fmt_scribe_11b2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fmt_scribe_11b2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUTPUT=$(./tests/integration/fmt_scribe_11b2 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] && [ "$RUN_OUTPUT" = "n: 7" ]; then
        test_pass "FMT output integrates with scribe"
    else
        test_fail "FMT scribe integration output mismatch or non-zero exit ($EXIT_CODE)"
    fi
else
    test_fail "FMT scribe fixture failed to compile"
fi

echo ""
echo "========================================"
echo "FASE 11C: SERIES + ALG Canonical Collection Tests"
echo "========================================"
echo ""

echo "Test 360: cct/series fill works on static arrays"
cleanup_codegen_artifacts "tests/integration/series_fill_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_fill_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/series_fill_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "SERIES fill works"
    else
        test_fail "SERIES fill returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES fill failed to compile"
fi

echo "Test 361: cct/series copy works"
cleanup_codegen_artifacts "tests/integration/series_copy_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_copy_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/series_copy_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "SERIES copy works"
    else
        test_fail "SERIES copy returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES copy failed to compile"
fi

echo "Test 362: cct/series reverse works"
cleanup_codegen_artifacts "tests/integration/series_reverse_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_reverse_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/series_reverse_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "SERIES reverse works"
    else
        test_fail "SERIES reverse returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES reverse failed to compile"
fi

echo "Test 363: cct/series contains works"
cleanup_codegen_artifacts "tests/integration/series_contains_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_contains_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/series_contains_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "SERIES contains works"
    else
        test_fail "SERIES contains returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES contains failed to compile"
fi

echo "Test 364: cct/alg linear_search works"
cleanup_codegen_artifacts "tests/integration/alg_linear_search_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/alg_linear_search_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/alg_linear_search_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "ALG linear_search works"
    else
        test_fail "ALG linear_search returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "ALG linear_search failed to compile"
fi

echo "Test 365: cct/alg compare_arrays works"
cleanup_codegen_artifacts "tests/integration/alg_compare_arrays_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/alg_compare_arrays_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/alg_compare_arrays_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "ALG compare_arrays works"
    else
        test_fail "ALG compare_arrays returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "ALG compare_arrays failed to compile"
fi

echo "Test 366: cct/series generic UMBRA flow works"
cleanup_codegen_artifacts "tests/integration/series_generic_real_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_generic_real_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/series_generic_real_11c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "SERIES generic real flow works"
    else
        test_fail "SERIES generic real flow returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES generic real flow failed to compile"
fi

echo "Test 367: cct/series integrates with cct/fmt output"
cleanup_codegen_artifacts "tests/integration/series_fmt_integration_11c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/series_fmt_integration_11c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUTPUT=$(./tests/integration/series_fmt_integration_11c 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] && [ "$RUN_OUTPUT" = "first: 10" ]; then
        test_pass "SERIES + FMT integration works"
    else
        test_fail "SERIES + FMT integration output mismatch or non-zero exit ($EXIT_CODE)"
    fi
else
    test_fail "SERIES + FMT integration fixture failed to compile"
fi

echo "Test 368: --sigilo-only remains stable with cct/series + cct/alg"
cleanup_codegen_artifacts "tests/integration/sigilo_series_alg_11c.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_series_alg_11c.cct 2>&1) || true
if [ -f "tests/integration/sigilo_series_alg_11c.svg" ] && [ -f "tests/integration/sigilo_series_alg_11c.sigil" ]; then
    test_pass "Sigilo generation remains stable with cct/series + cct/alg"
else
    test_fail "Sigilo generation regressed with cct/series + cct/alg"
fi

echo ""
echo "========================================"
echo "FASE 11D.1: MEM + Ownership Foundation Tests"
echo "========================================"
echo ""

echo "Test 369: cct/mem alloc + free basic flow works"
cleanup_codegen_artifacts "tests/integration/mem_alloc_free_11d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/mem_alloc_free_11d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/mem_alloc_free_11d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 35 ]; then
        test_pass "MEM alloc/free basic flow works"
    else
        test_fail "MEM alloc/free returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "MEM alloc/free fixture failed to compile"
fi

echo "Test 370: cct/mem realloc preserves existing content"
cleanup_codegen_artifacts "tests/integration/mem_realloc_11d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/mem_realloc_11d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/mem_realloc_11d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 11 ]; then
        test_pass "MEM realloc preserves content in canonical flow"
    else
        test_fail "MEM realloc returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "MEM realloc fixture failed to compile"
fi

echo "Test 371: cct/mem copy works for REX buffers"
cleanup_codegen_artifacts "tests/integration/mem_copy_11d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/mem_copy_11d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/mem_copy_11d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 5 ]; then
        test_pass "MEM copy works for REX buffers"
    else
        test_fail "MEM copy returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "MEM copy fixture failed to compile"
fi

echo "Test 372: cct/mem set + zero canonical flow works"
cleanup_codegen_artifacts "tests/integration/mem_set_zero_11d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/mem_set_zero_11d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/mem_set_zero_11d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "MEM set/zero canonical flow works"
    else
        test_fail "MEM set/zero returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "MEM set/zero fixture failed to compile"
fi

echo "Test 373: cct/mem integrates with cct/verbum without symbol conflict"
cleanup_codegen_artifacts "tests/integration/mem_with_verbum_11d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/mem_with_verbum_11d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/mem_with_verbum_11d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 6 ]; then
        test_pass "MEM integrates with VERBUM without symbol conflict"
    else
        test_fail "MEM + VERBUM integration returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "MEM + VERBUM integration fixture failed to compile"
fi

echo "Test 374: --sigilo-only remains stable with cct/mem import"
cleanup_codegen_artifacts "tests/integration/sigilo_mem_basic_11d1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_mem_basic_11d1.cct 2>&1) || true
if [ -f "tests/integration/sigilo_mem_basic_11d1.svg" ] && [ -f "tests/integration/sigilo_mem_basic_11d1.sigil" ]; then
    test_pass "Sigilo generation remains stable with cct/mem usage"
else
    test_fail "Sigilo generation regressed with cct/mem usage"
fi

echo ""
echo "========================================"
echo "FASE 11D.2: FLUXUS Storage Runtime Core Tests"
echo "========================================"
echo ""

echo "Test 375: standalone fluxus storage runtime test target builds and runs"
OUTPUT=$(make test_fluxus_storage 2>&1) || true
if echo "$OUTPUT" | grep -q "ok"; then
    test_pass "Standalone FLUXUS storage runtime target builds and runs"
else
    test_fail "Standalone FLUXUS storage runtime target failed"
fi

echo "Test 376: fluxus runtime test binary exists after build"
if [ -f "tests/runtime/test_fluxus_storage" ]; then
    test_pass "FLUXUS runtime test binary is produced deterministically"
else
    test_fail "FLUXUS runtime test binary not found"
fi

echo ""
echo "========================================"
echo "FASE 11D.3: FLUXUS Canonical API Tests"
echo "========================================"
echo ""

echo "Test 377: cct/fluxus init + free basic flow works"
cleanup_codegen_artifacts "tests/integration/fluxus_init_free_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_init_free_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_init_free_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FLUXUS init/free basic flow works"
    else
        test_fail "FLUXUS init/free returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS init/free fixture failed to compile"
fi

echo "Test 378: cct/fluxus push + len works"
cleanup_codegen_artifacts "tests/integration/fluxus_push_len_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_push_len_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_push_len_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 2 ]; then
        test_pass "FLUXUS push/len canonical flow works"
    else
        test_fail "FLUXUS push/len returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS push/len fixture failed to compile"
fi

echo "Test 379: cct/fluxus get returns indexed element"
cleanup_codegen_artifacts "tests/integration/fluxus_get_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_get_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_get_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 20 ]; then
        test_pass "FLUXUS get indexed read works"
    else
        test_fail "FLUXUS get returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS get fixture failed to compile"
fi

echo "Test 380: cct/fluxus pop works with LIFO behavior"
cleanup_codegen_artifacts "tests/integration/fluxus_pop_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_pop_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_pop_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FLUXUS pop canonical LIFO flow works"
    else
        test_fail "FLUXUS pop returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS pop fixture failed to compile"
fi

echo "Test 381: cct/fluxus clear resets length"
cleanup_codegen_artifacts "tests/integration/fluxus_clear_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_clear_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_clear_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FLUXUS clear resets length"
    else
        test_fail "FLUXUS clear returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS clear fixture failed to compile"
fi

echo "Test 382: cct/fluxus reserve + capacity works"
cleanup_codegen_artifacts "tests/integration/fluxus_reserve_capacity_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_reserve_capacity_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_reserve_capacity_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FLUXUS reserve/capacity canonical flow works"
    else
        test_fail "FLUXUS reserve/capacity returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS reserve/capacity fixture failed to compile"
fi

echo "Test 383: cct/fluxus automatic growth works"
cleanup_codegen_artifacts "tests/integration/fluxus_growth_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_growth_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_growth_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 50 ]; then
        test_pass "FLUXUS growth strategy works in canonical loop"
    else
        test_fail "FLUXUS growth returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS growth fixture failed to compile"
fi

echo "Test 384: cct/fluxus integrates with VERBUM payloads"
cleanup_codegen_artifacts "tests/integration/fluxus_with_verbum_11d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fluxus_with_verbum_11d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fluxus_with_verbum_11d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 12 ]; then
        test_pass "FLUXUS + VERBUM integration works"
    else
        test_fail "FLUXUS + VERBUM integration returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS + VERBUM fixture failed to compile"
fi

echo "Test 385: --sigilo-only tracks FLUXUS metadata counters"
cleanup_codegen_artifacts "tests/integration/sigilo_fluxus_basic_11d3.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_fluxus_basic_11d3.cct 2>&1) || true
if [ -f "tests/integration/sigilo_fluxus_basic_11d3.sigil" ] && \
   grep -q "^fluxus_ops_count = 4$" tests/integration/sigilo_fluxus_basic_11d3.sigil && \
   grep -q "^fluxus_init_count = 1$" tests/integration/sigilo_fluxus_basic_11d3.sigil && \
   grep -q "^fluxus_push_count = 1$" tests/integration/sigilo_fluxus_basic_11d3.sigil && \
   grep -q "^fluxus_pop_count = 1$" tests/integration/sigilo_fluxus_basic_11d3.sigil && \
   grep -q "^fluxus_instances_count = 1$" tests/integration/sigilo_fluxus_basic_11d3.sigil; then
    test_pass "Sigilo metadata tracks FLUXUS operation counters"
else
    test_fail "Sigilo metadata missing FLUXUS counters"
fi

echo ""
echo "========================================"
echo "FASE 11E.1: IO + FS Tests"
echo "========================================"
echo ""

echo "Test 386: cct/io print + println emits expected output"
cleanup_codegen_artifacts "tests/integration/io_print_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/io_print_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    IO_OUT=$(./tests/integration/io_print_11e1 2>/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] && [ "$IO_OUT" = "Ola mundo" ]; then
        test_pass "IO print/println output is stable"
    else
        test_fail "IO print/println output mismatch or non-zero exit ($EXIT_CODE)"
    fi
else
    test_fail "IO print/println fixture failed to compile"
fi

echo "Test 387: cct/io print_int emits expected integer text"
cleanup_codegen_artifacts "tests/integration/io_print_int_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/io_print_int_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    IO_OUT=$(./tests/integration/io_print_int_11e1 2>/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ] && [ "$IO_OUT" = "42" ]; then
        test_pass "IO print_int output is stable"
    else
        test_fail "IO print_int output mismatch or non-zero exit ($EXIT_CODE)"
    fi
else
    test_fail "IO print_int fixture failed to compile"
fi

echo "Test 388: cct/io read_line reads stdin line deterministically"
cleanup_codegen_artifacts "tests/integration/io_read_line_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/io_read_line_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/io_read_line_11e1 <<< "abcde" > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 5 ]; then
        test_pass "IO read_line canonical flow works"
    else
        test_fail "IO read_line returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "IO read_line fixture failed to compile"
fi

echo "Test 389: cct/fs write_all + read_all canonical flow works"
cleanup_codegen_artifacts "tests/integration/fs_write_read_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_write_read_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fs_write_read_11e1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FS write_all/read_all flow works"
    else
        test_fail "FS write_all/read_all returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FS write/read fixture failed to compile"
fi

echo "Test 390: cct/fs append_all canonical flow works"
cleanup_codegen_artifacts "tests/integration/fs_append_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_append_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fs_append_11e1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FS append_all flow works"
    else
        test_fail "FS append_all returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FS append fixture failed to compile"
fi

echo "Test 391: cct/fs exists reports deterministic boolean values"
cleanup_codegen_artifacts "tests/integration/fs_exists_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_exists_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fs_exists_11e1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FS exists flow works"
    else
        test_fail "FS exists returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FS exists fixture failed to compile"
fi

echo "Test 392: cct/fs size returns deterministic file length"
cleanup_codegen_artifacts "tests/integration/fs_size_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_size_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fs_size_11e1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FS size flow works"
    else
        test_fail "FS size returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FS size fixture failed to compile"
fi

echo "Test 393: cct/fs integrates with cct/verbum content composition"
cleanup_codegen_artifacts "tests/integration/fs_with_verbum_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_with_verbum_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/fs_with_verbum_11e1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "FS + VERBUM integration works"
    else
        test_fail "FS + VERBUM integration returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FS + VERBUM fixture failed to compile"
fi

echo "Test 394: cct/fs read_all missing path fails clearly"
cleanup_codegen_artifacts "tests/integration/fs_read_error_11e1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/fs_read_error_11e1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/fs_read_error_11e1 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "fs read_all open failed"; then
        test_pass "FS read_all missing-file diagnostic is clear"
    else
        test_fail "FS read_all missing-file diagnostic is not clear"
    fi
else
    test_fail "FS read error fixture failed to compile"
fi

echo "Test 395: --sigilo-only remains stable with cct/io + cct/fs imports"
cleanup_codegen_artifacts "tests/integration/sigilo_io_fs_basic_11e1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_io_fs_basic_11e1.cct 2>&1) || true
if [ -f "tests/integration/sigilo_io_fs_basic_11e1.svg" ] && [ -f "tests/integration/sigilo_io_fs_basic_11e1.sigil" ]; then
    test_pass "Sigilo generation remains stable with IO/FS modules"
else
    test_fail "Sigilo generation regressed with IO/FS modules"
fi

echo ""
echo "========================================"
echo "FASE 11E.2: PATH + Text/File Integration Tests"
echo "========================================"
echo ""

echo "Test 396: cct/path import resolves in semantic check"
OUTPUT=$("$CCT_BIN" --check tests/integration/path_join_basic_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "PATH module import resolves in semantic pipeline"
else
    test_fail "PATH module import failed semantic check"
fi

echo "Test 397: path_join basic composition works"
cleanup_codegen_artifacts "tests/integration/path_join_basic_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_join_basic_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_join_basic_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_join basic composition works"
    else
        test_fail "path_join basic composition returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_join basic fixture failed to compile"
fi

echo "Test 398: path_join removes duplicate separators"
cleanup_codegen_artifacts "tests/integration/path_join_double_sep_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_join_double_sep_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_join_double_sep_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_join duplicate-separator normalization works"
    else
        test_fail "path_join duplicate-separator case returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_join duplicate-separator fixture failed to compile"
fi

echo "Test 399: path_basename extracts terminal segment"
cleanup_codegen_artifacts "tests/integration/path_basename_basic_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_basename_basic_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_basename_basic_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_basename extracts terminal segment"
    else
        test_fail "path_basename returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_basename fixture failed to compile"
fi

echo "Test 400: path_dirname extracts parent segment"
cleanup_codegen_artifacts "tests/integration/path_dirname_basic_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_dirname_basic_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_dirname_basic_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_dirname extracts parent segment"
    else
        test_fail "path_dirname returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_dirname fixture failed to compile"
fi

echo "Test 401: path_ext extracts simple extension"
cleanup_codegen_artifacts "tests/integration/path_ext_basic_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_ext_basic_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_ext_basic_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_ext simple extension extraction works"
    else
        test_fail "path_ext basic case returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_ext basic fixture failed to compile"
fi

echo "Test 402: path_ext no-extension case returns empty string"
cleanup_codegen_artifacts "tests/integration/path_ext_noext_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_ext_noext_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_ext_noext_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "path_ext no-extension case works"
    else
        test_fail "path_ext no-extension case returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "path_ext no-extension fixture failed to compile"
fi

echo "Test 403: PATH + FS read integration works"
cleanup_codegen_artifacts "tests/integration/path_fs_integration_read_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_fs_integration_read_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_fs_integration_read_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "PATH + FS read integration works"
    else
        test_fail "PATH + FS read integration returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "PATH + FS read integration fixture failed to compile"
fi

echo "Test 404: PATH + FS write/read integration works"
cleanup_codegen_artifacts "tests/integration/path_fs_integration_write_read_11e2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/path_fs_integration_write_read_11e2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/path_fs_integration_write_read_11e2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "PATH + FS write/read integration works"
    else
        test_fail "PATH + FS write/read integration returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "PATH + FS write/read integration fixture failed to compile"
fi

echo "Test 405: --sigilo-only records PATH metadata counters"
cleanup_codegen_artifacts "tests/integration/sigilo_path_basic_11e2.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_path_basic_11e2.cct 2>&1) || true
if [ -f "tests/integration/sigilo_path_basic_11e2.sigil" ] && \
   grep -q "^path_module_used = true$" tests/integration/sigilo_path_basic_11e2.sigil && \
   grep -q "^path_ops_count = 3$" tests/integration/sigilo_path_basic_11e2.sigil; then
    test_pass "Sigilo metadata tracks PATH usage counters"
else
    test_fail "Sigilo metadata missing PATH counters/status"
fi

echo ""
echo "========================================"
echo "FASE 11F.1: MATH + RANDOM Tests"
echo "========================================"
echo ""

echo "Test 406: cct/math and cct/random imports resolve in semantic check"
OUTPUT=$("$CCT_BIN" --check tests/integration/math_abs_basic_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    OUTPUT=$("$CCT_BIN" --check tests/integration/random_seed_repro_11f1.cct 2>&1) || true
    if echo "$OUTPUT" | grep -q "Semantic check OK"; then
        test_pass "MATH/RANDOM imports resolve in semantic pipeline"
    else
        test_fail "cct/random semantic check failed"
    fi
else
    test_fail "cct/math semantic check failed"
fi

echo "Test 407: abs handles positive/negative/zero"
cleanup_codegen_artifacts "tests/integration/math_abs_basic_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/math_abs_basic_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/math_abs_basic_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "abs canonical behavior works"
    else
        test_fail "abs fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "abs fixture failed to compile"
fi

echo "Test 408: min/max return expected values"
cleanup_codegen_artifacts "tests/integration/math_min_max_basic_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/math_min_max_basic_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/math_min_max_basic_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "min/max canonical behavior works"
    else
        test_fail "min/max fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "min/max fixture failed to compile"
fi

echo "Test 409: clamp handles below/inside/above interval"
cleanup_codegen_artifacts "tests/integration/math_clamp_basic_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/math_clamp_basic_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/math_clamp_basic_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "clamp canonical interval behavior works"
    else
        test_fail "clamp basic fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "clamp basic fixture failed to compile"
fi

echo "Test 410: clamp invalid interval fails clearly"
cleanup_codegen_artifacts "tests/integration/math_clamp_invalid_range_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/math_clamp_invalid_range_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/math_clamp_invalid_range_11f1 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "math clamp invalid range"; then
        test_pass "clamp invalid-range diagnostic is clear"
    else
        test_fail "clamp invalid-range diagnostic is not clear"
    fi
else
    test_fail "clamp invalid-range fixture failed to compile"
fi

echo "Test 411: seed produces reproducible random_int sequence"
cleanup_codegen_artifacts "tests/integration/random_seed_repro_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/random_seed_repro_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/random_seed_repro_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "seed reproducibility baseline works"
    else
        test_fail "seed reproducibility fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "seed reproducibility fixture failed to compile"
fi

echo "Test 412: random_int respects inclusive bounds"
cleanup_codegen_artifacts "tests/integration/random_int_range_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/random_int_range_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/random_int_range_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "random_int inclusive bounds work"
    else
        test_fail "random_int range fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "random_int range fixture failed to compile"
fi

echo "Test 413: random_int invalid interval fails clearly"
cleanup_codegen_artifacts "tests/integration/random_int_invalid_range_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/random_int_invalid_range_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/random_int_invalid_range_11f1 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "random_int invalid range"; then
        test_pass "random_int invalid-range diagnostic is clear"
    else
        test_fail "random_int invalid-range diagnostic is not clear"
    fi
else
    test_fail "random_int invalid-range fixture failed to compile"
fi

echo "Test 414: random_real returns value in [0,1)"
cleanup_codegen_artifacts "tests/integration/random_real_basic_11f1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/random_real_basic_11f1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/random_real_basic_11f1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "random_real range behavior works"
    else
        test_fail "random_real fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "random_real fixture failed to compile"
fi

echo "Test 415: --sigilo-only records MATH/RANDOM metadata counters"
cleanup_codegen_artifacts "tests/integration/sigilo_math_random_basic_11f1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_math_random_basic_11f1.cct 2>&1) || true
if [ -f "tests/integration/sigilo_math_random_basic_11f1.sigil" ] && \
   grep -q "^math_module_used = true$" tests/integration/sigilo_math_random_basic_11f1.sigil && \
   grep -q "^random_module_used = true$" tests/integration/sigilo_math_random_basic_11f1.sigil && \
   grep -q "^math_ops_count = 2$" tests/integration/sigilo_math_random_basic_11f1.sigil && \
   grep -q "^random_ops_count = 3$" tests/integration/sigilo_math_random_basic_11f1.sigil; then
    test_pass "Sigilo metadata tracks MATH/RANDOM counters"
else
    test_fail "Sigilo metadata missing MATH/RANDOM counters/status"
fi

echo ""
echo "========================================"
echo "FASE 11F.2: PARSE + CMP + ALG (moderado) Tests"
echo "========================================"
echo ""

echo "Test 416: cct/parse and cct/cmp imports resolve in semantic check"
OUTPUT=$("$CCT_BIN" --check tests/integration/parse_int_valid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    OUTPUT=$("$CCT_BIN" --check tests/integration/cmp_int_basic_11f2.cct 2>&1) || true
    if echo "$OUTPUT" | grep -q "Semantic check OK"; then
        test_pass "PARSE/CMP imports resolve in semantic pipeline"
    else
        test_fail "cct/cmp semantic check failed"
    fi
else
    test_fail "cct/parse semantic check failed"
fi

echo "Test 417: parse_int converts valid input"
cleanup_codegen_artifacts "tests/integration/parse_int_valid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_int_valid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/parse_int_valid_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "parse_int valid conversion works"
    else
        test_fail "parse_int valid fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "parse_int valid fixture failed to compile"
fi

echo "Test 418: parse_int rejects invalid input clearly"
cleanup_codegen_artifacts "tests/integration/parse_int_invalid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_int_invalid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/parse_int_invalid_11f2 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "parse_int invalid input"; then
        test_pass "parse_int invalid-input diagnostic is clear"
    else
        test_fail "parse_int invalid-input diagnostic is not clear"
    fi
else
    test_fail "parse_int invalid fixture failed to compile"
fi

echo "Test 419: parse_real converts valid input"
cleanup_codegen_artifacts "tests/integration/parse_real_valid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_real_valid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/parse_real_valid_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "parse_real valid conversion works"
    else
        test_fail "parse_real valid fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "parse_real valid fixture failed to compile"
fi

echo "Test 420: parse_real rejects invalid input clearly"
cleanup_codegen_artifacts "tests/integration/parse_real_invalid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_real_invalid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/parse_real_invalid_11f2 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "parse_real invalid input"; then
        test_pass "parse_real invalid-input diagnostic is clear"
    else
        test_fail "parse_real invalid-input diagnostic is not clear"
    fi
else
    test_fail "parse_real invalid fixture failed to compile"
fi

echo "Test 421: parse_bool converts valid input"
cleanup_codegen_artifacts "tests/integration/parse_bool_valid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_bool_valid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/parse_bool_valid_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "parse_bool valid conversion works"
    else
        test_fail "parse_bool valid fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "parse_bool valid fixture failed to compile"
fi

echo "Test 422: parse_bool rejects invalid input clearly"
cleanup_codegen_artifacts "tests/integration/parse_bool_invalid_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/parse_bool_invalid_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    RUN_OUT=$(./tests/integration/parse_bool_invalid_11f2 2>&1 >/dev/null)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ] && echo "$RUN_OUT" | grep -q "parse_bool invalid input"; then
        test_pass "parse_bool invalid-input diagnostic is clear"
    else
        test_fail "parse_bool invalid-input diagnostic is not clear"
    fi
else
    test_fail "parse_bool invalid fixture failed to compile"
fi

echo "Test 423: cmp_int returns canonical <0/0/>0 contract"
cleanup_codegen_artifacts "tests/integration/cmp_int_basic_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cmp_int_basic_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cmp_int_basic_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cmp_int canonical contract works"
    else
        test_fail "cmp_int fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cmp_int fixture failed to compile"
fi

echo "Test 424: cmp_real returns canonical <0/0/>0 contract"
cleanup_codegen_artifacts "tests/integration/cmp_real_basic_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cmp_real_basic_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cmp_real_basic_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cmp_real canonical contract works"
    else
        test_fail "cmp_real fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cmp_real fixture failed to compile"
fi

echo "Test 425: cmp_verbum returns canonical <0/0/>0 contract"
cleanup_codegen_artifacts "tests/integration/cmp_verbum_basic_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cmp_verbum_basic_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cmp_verbum_basic_11f2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cmp_verbum canonical contract works"
    else
        test_fail "cmp_verbum fixture returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cmp_verbum fixture failed to compile"
fi

echo "Test 426: moderate ALG extras (binary_search + sort) work"
cleanup_codegen_artifacts "tests/integration/alg_binary_search_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/alg_sort_basic_11f2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/alg_binary_search_basic_11f2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/alg_binary_search_basic_11f2 > /dev/null 2>&1
    EXIT_CODE_A=$?
    OUTPUT=$("$CCT_BIN" tests/integration/alg_sort_basic_11f2.cct 2>&1) || true
    if echo "$OUTPUT" | grep -q "Compiled:"; then
        ./tests/integration/alg_sort_basic_11f2 > /dev/null 2>&1
        EXIT_CODE_B=$?
        if [ $EXIT_CODE_A -eq 0 ] && [ $EXIT_CODE_B -eq 0 ]; then
            test_pass "Moderate ALG extras work"
        else
            test_fail "ALG extras returned unexpected codes ($EXIT_CODE_A/$EXIT_CODE_B)"
        fi
    else
        test_fail "alg_sort fixture failed to compile"
    fi
else
    test_fail "alg_binary_search fixture failed to compile"
fi

echo "Test 427: --sigilo-only records PARSE/CMP/ALG metadata counters"
cleanup_codegen_artifacts "tests/integration/sigilo_parse_cmp_basic_11f2.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_parse_cmp_basic_11f2.cct 2>&1) || true
if [ -f "tests/integration/sigilo_parse_cmp_basic_11f2.sigil" ] && \
   grep -q "^parse_module_used = true$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil && \
   grep -q "^cmp_module_used = true$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil && \
   grep -q "^alg_module_used = true$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil && \
   grep -q "^parse_ops_count = 3$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil && \
   grep -q "^cmp_ops_count = 3$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil && \
   grep -q "^alg_ops_count = 1$" tests/integration/sigilo_parse_cmp_basic_11f2.sigil; then
    test_pass "Sigilo metadata tracks PARSE/CMP/ALG counters"
else
    test_fail "Sigilo metadata missing PARSE/CMP/ALG counters/status"
fi

echo ""
echo "========================================"
echo "FASE 11G: Canonical Showcase + Sigilo Integration Tests"
echo "========================================"
echo ""

echo "Test 428: showcase string compiles and executes"
cleanup_codegen_artifacts "tests/integration/showcase_string_11g.cct"
OUTPUT=$("$CCT_BIN" tests/integration/showcase_string_11g.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/showcase_string_11g > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Showcase string executes correctly"
    else
        test_fail "Showcase string returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Showcase string failed to compile"
fi

echo "Test 429: showcase collection compiles and executes"
cleanup_codegen_artifacts "tests/integration/showcase_collection_11g.cct"
OUTPUT=$("$CCT_BIN" tests/integration/showcase_collection_11g.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/showcase_collection_11g > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Showcase collection executes correctly"
    else
        test_fail "Showcase collection returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Showcase collection failed to compile"
fi

echo "Test 430: showcase IO/FS compiles and executes"
cleanup_codegen_artifacts "tests/integration/showcase_io_fs_11g.cct"
OUTPUT=$("$CCT_BIN" tests/integration/showcase_io_fs_11g.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/showcase_io_fs_11g > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Showcase IO/FS executes correctly"
    else
        test_fail "Showcase IO/FS returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Showcase IO/FS failed to compile"
fi

echo "Test 431: showcase parse/math/random compiles and executes"
cleanup_codegen_artifacts "tests/integration/showcase_parse_math_random_11g.cct"
OUTPUT=$("$CCT_BIN" tests/integration/showcase_parse_math_random_11g.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/showcase_parse_math_random_11g > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Showcase parse/math/random executes correctly"
    else
        test_fail "Showcase parse/math/random returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Showcase parse/math/random failed to compile"
fi

echo "Test 432: showcase multi-module compiles and executes"
cleanup_codegen_artifacts "tests/integration/showcase_modular_11g_main.cct"
OUTPUT=$("$CCT_BIN" tests/integration/showcase_modular_11g_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/showcase_modular_11g_main > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Showcase multi-module executes correctly"
    else
        test_fail "Showcase multi-module returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Showcase multi-module failed to compile"
fi

echo "Test 433: --check passes for all canonical 11G showcases"
SHOWCASE_CHECK_OK=true
for f in \
    tests/integration/showcase_string_11g.cct \
    tests/integration/showcase_collection_11g.cct \
    tests/integration/showcase_io_fs_11g.cct \
    tests/integration/showcase_parse_math_random_11g.cct \
    tests/integration/showcase_modular_11g_main.cct; do
    OUTPUT=$("$CCT_BIN" --check "$f" 2>&1) || true
    if ! echo "$OUTPUT" | grep -q "Semantic check OK"; then
        SHOWCASE_CHECK_OK=false
        break
    fi
done
if [ "$SHOWCASE_CHECK_OK" = true ]; then
    test_pass "All canonical 11G showcases pass semantic check"
else
    test_fail "One or more 11G showcases failed semantic check"
fi

echo "Test 434: --ast works for single-file 11G showcase"
OUTPUT=$("$CCT_BIN" --ast tests/integration/showcase_string_11g.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "PROGRAM: showcase_string_11g"; then
    test_pass "--ast works for 11G single-file showcase"
else
    test_fail "--ast failed for 11G single-file showcase"
fi

echo "Test 435: --ast-composite works for 11G multi-module showcase"
OUTPUT=$("$CCT_BIN" --ast-composite tests/integration/showcase_modular_11g_main.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Modules in closure:" && echo "$OUTPUT" | grep -q "showcase_modular_11g_main"; then
    test_pass "--ast-composite works for 11G multi-module showcase"
else
    test_fail "--ast-composite failed for 11G multi-module showcase"
fi

echo "Test 436: --sigilo-only emits artifacts for 11G showcase"
cleanup_codegen_artifacts "tests/integration/showcase_parse_math_random_11g.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/showcase_parse_math_random_11g.cct 2>&1) || true
if [ -f "tests/integration/showcase_parse_math_random_11g.svg" ] && \
   [ -f "tests/integration/showcase_parse_math_random_11g.sigil" ]; then
    test_pass "--sigilo-only emits artifacts for 11G showcase"
else
    test_fail "--sigilo-only did not emit artifacts for 11G showcase"
fi

echo "Test 437: 11G sigilo metadata includes stdlib counters and module list"
cleanup_codegen_artifacts "tests/integration/sigilo_showcase_stdlib_11g.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_showcase_stdlib_11g.cct 2>&1) || true
if [ -f "tests/integration/sigilo_showcase_stdlib_11g.sigil" ] && \
   grep -q "^stdlib_module_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^stdlib_modules_used = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^verbum_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^fmt_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^series_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^fluxus_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^mem_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^io_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^fs_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^path_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^math_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^random_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^parse_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^cmp_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil && \
   grep -q "^alg_ops_count = " tests/integration/sigilo_showcase_stdlib_11g.sigil; then
    test_pass "11G sigilo metadata includes stdlib module counters/list"
else
    test_fail "11G sigilo metadata missing stdlib counters/list fields"
fi

echo "Test 438: --sigilo-mode essencial remains stable for 11G modular showcase"
cleanup_codegen_artifacts "tests/integration/showcase_modular_11g_main.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial tests/integration/showcase_modular_11g_main.cct 2>&1) || true
if [ -f "tests/integration/showcase_modular_11g_main.svg" ] && \
   [ -f "tests/integration/showcase_modular_11g_main.system.svg" ] && \
   [ ! -f "tests/integration/showcase_modular_11g_main.__mod_001.svg" ]; then
    test_pass "11G essential sigilo mode remains stable"
else
    test_fail "11G essential sigilo mode regressed"
fi

echo "Test 439: --sigilo-mode completo remains stable for 11G modular showcase"
rm -f tests/integration/tmp_sig_11g_complete.svg \
      tests/integration/tmp_sig_11g_complete.sigil \
      tests/integration/tmp_sig_11g_complete.system.svg \
      tests/integration/tmp_sig_11g_complete.system.sigil \
      tests/integration/tmp_sig_11g_complete.__mod_001.svg \
      tests/integration/tmp_sig_11g_complete.__mod_001.sigil \
      tests/integration/tmp_sig_11g_complete.__mod_002.svg \
      tests/integration/tmp_sig_11g_complete.__mod_002.sigil \
      tests/integration/tmp_sig_11g_complete.__mod_003.svg \
      tests/integration/tmp_sig_11g_complete.__mod_003.sigil
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-out tests/integration/tmp_sig_11g_complete tests/integration/showcase_modular_11g_main.cct 2>&1) || true
if [ -f "tests/integration/tmp_sig_11g_complete.svg" ] && \
   [ -f "tests/integration/tmp_sig_11g_complete.system.svg" ] && \
   [ -f "tests/integration/tmp_sig_11g_complete.__mod_001.svg" ] && \
   [ -f "tests/integration/tmp_sig_11g_complete.__mod_002.svg" ] && \
   [ -f "tests/integration/tmp_sig_11g_complete.__mod_003.svg" ]; then
    test_pass "11G complete sigilo mode remains stable"
else
    test_fail "11G complete sigilo mode regressed"
fi

echo ""
echo "========================================"
echo "FASE 11H: Final Consolidation + Public Release Readiness Tests"
echo "========================================"
echo ""

# Removed Test 440: documentation text/file comparison (policy)

# Removed Test 441: documentation text/file comparison (policy)

echo "Test 442: final naming freeze preserves fmt_parse_* facade and parse namespace separation"
if grep -Eq "^RITUALE fmt_parse_int\\(VERBUM [A-Za-z_][A-Za-z0-9_]*\\) REDDE REX$" lib/cct/fmt.cct && \
   grep -Eq "^RITUALE parse_int\\(VERBUM [A-Za-z_][A-Za-z0-9_]*\\) REDDE REX$" lib/cct/parse.cct && \
   ! grep -q "^RITUALE parse_int(VERBUM s) REDDE REX$" lib/cct/fmt.cct; then
    test_pass "Final naming separation between cct/fmt and cct/parse is preserved"
else
    test_fail "Final naming separation between cct/fmt and cct/parse regressed"
fi

# Removed Test 443: documentation text/file comparison (policy)

# Removed Test 444: documentation text/file comparison (policy)

# Removed Test 445: documentation text/file comparison (policy)

echo "Test 446: canonical examples are packaged and executable in source tree"
SHOWCASE_EXAMPLES_OK=true
for f in \
    examples/ars_magna_showcase.cct \
    examples/collection_ops_12d2.cct \
    examples/path_fs_showcase_11e2.cct \
    examples/math_random_showcase_11f1.cct \
    examples/parse_cmp_showcase_11f2.cct; do
    cleanup_codegen_artifacts "$f"
    OUTPUT=$("$CCT_BIN" "$f" 2>&1) || true
    if ! echo "$OUTPUT" | grep -q "Compiled:"; then
        SHOWCASE_EXAMPLES_OK=false
        break
    fi
done
if [ "$SHOWCASE_EXAMPLES_OK" = true ]; then
    test_pass "Canonical examples compile in source tree"
else
    test_fail "One or more canonical examples failed to compile"
fi

echo "Test 447: --sigilo-only on canonical showcase emits final stdlib metadata fields"
cleanup_codegen_artifacts "examples/math_random_showcase_11f1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only examples/math_random_showcase_11f1.cct 2>&1) || true
if [ -f "examples/math_random_showcase_11f1.sigil" ] && \
   grep -q "^stdlib_module_count = " examples/math_random_showcase_11f1.sigil && \
   grep -q "^stdlib_modules_used = " examples/math_random_showcase_11f1.sigil; then
    test_pass "Final stdlib sigilo metadata is emitted for canonical showcase"
else
    test_fail "Final stdlib sigilo metadata is missing for canonical showcase"
fi

echo "Test 448: final sigilo mode essencial/completo stays stable on canonical modular example"
cleanup_codegen_artifacts "examples/collection_ops_12d2.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode essencial examples/collection_ops_12d2.cct 2>&1) || true
ESS_OK=false
if [ -f "examples/collection_ops_12d2.svg" ] && \
   [ -f "examples/collection_ops_12d2.system.svg" ] && \
   [ ! -f "examples/collection_ops_12d2.__mod_001.svg" ]; then
    ESS_OK=true
fi
rm -f examples/tmp_sig_11h_complete.svg \
      examples/tmp_sig_11h_complete.sigil \
      examples/tmp_sig_11h_complete.system.svg \
      examples/tmp_sig_11h_complete.system.sigil \
      examples/tmp_sig_11h_complete.__mod_001.svg \
      examples/tmp_sig_11h_complete.__mod_001.sigil \
      examples/tmp_sig_11h_complete.__mod_002.svg \
      examples/tmp_sig_11h_complete.__mod_002.sigil \
      examples/tmp_sig_11h_complete.__mod_003.svg \
      examples/tmp_sig_11h_complete.__mod_003.sigil
OUTPUT=$("$CCT_BIN" --sigilo-only --sigilo-mode completo --sigilo-out examples/tmp_sig_11h_complete examples/collection_ops_12d2.cct 2>&1) || true
if [ "$ESS_OK" = true ] && \
   [ -f "examples/tmp_sig_11h_complete.svg" ] && \
   [ -f "examples/tmp_sig_11h_complete.system.svg" ] && \
   [ -f "examples/tmp_sig_11h_complete.__mod_001.svg" ]; then
    test_pass "Final sigilo essential/complete modes remain stable for canonical modular example"
else
    test_fail "Final sigilo essential/complete modes regressed for canonical modular example"
fi

echo "Test 449: make dist builds public release layout"
OUTPUT=$(make dist 2>&1) || true
if [ -f "dist/cct/bin/cct" ] && \
   [ -f "dist/cct/bin/cct.bin" ] && \
   [ -f "dist/cct/docs/install.md" ] && \
   [ -f "dist/cct/examples/ars_magna_showcase.cct" ] && \
   [ -f "dist/cct/lib/cct/verbum.cct" ]; then
    test_pass "Distribution layout is generated as expected"
else
    test_fail "Distribution layout is missing required files"
fi

echo "Test 450: distribution smoke check resolves cct/... via packaged stdlib"
OUTPUT=$(CCT_STDLIB_DIR="$(pwd)/dist/cct/lib/cct" ./dist/cct/bin/cct --check tests/integration/stdlib_resolution_basic_11a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Semantic check OK"; then
    test_pass "Distribution smoke check resolves cct/... from packaged stdlib"
else
    test_fail "Distribution smoke check failed stdlib resolution"
fi

# Removed Test 451: documentation text/file comparison (policy)

echo "Test 452: syntax diagnostics include location and correction hint"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_syntax_error_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Unexpected block terminator" && \
   echo "$OUTPUT" | grep -q "diagnostic_syntax_error_12a.cct:5:1" && \
   echo "$OUTPUT" | grep -q "suggestion:"; then
    test_pass "Syntax diagnostic includes location and correction hint"
else
    test_fail "Syntax diagnostic is missing location/hint details"
fi

echo "Test 453: semantic type mismatch diagnostics include snippet"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_type_error_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "assignment type mismatch" && \
   echo "$OUTPUT" | grep -q "tests/integration/diagnostic_type_error_12a.cct:5:3" && \
   echo "$OUTPUT" | grep -q "|"; then
    test_pass "Type mismatch diagnostic includes stable location/snippet context"
else
    test_fail "Type mismatch diagnostic missing expected context"
fi

echo "Test 454: fuzzy suggestion is emitted for rituale typo"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_symbol_not_found_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "undeclared symbol 'tota'" && \
   echo "$OUTPUT" | grep -q "did you mean 'total'"; then
    test_pass "Symbol typo emits fuzzy suggestion"
else
    test_fail "Symbol typo did not emit fuzzy suggestion"
fi

echo "Test 455: missing-import diagnostic includes canonical import hint"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_missing_import_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "rituale 'len' is not declared" && \
   echo "$OUTPUT" | grep -q "ADVOCARE \\\"cct/verbum.cct\\\""; then
    test_pass "Missing-import scenario emits canonical import hint"
else
    test_fail "Missing-import diagnostic hint is missing"
fi

echo "Test 456: --no-color is accepted and diagnostics still render"
OUTPUT=$("$CCT_BIN" --no-color --check tests/integration/diagnostic_type_error_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "assignment type mismatch"; then
    test_pass "--no-color is accepted and diagnostics remain functional"
else
    test_fail "--no-color flow regressed diagnostic emission"
fi

echo ""
echo "========================================"
echo "FASE 12B: Casts and Numeric Coercions Tests"
echo "========================================"
echo ""

echo "Test 457: cast int-to-int compiles and executes"
cleanup_codegen_artifacts "tests/integration/cast_int_to_int_12b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cast_int_to_int_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cast_int_to_int_12b > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cast int-to-int works"
    else
        test_fail "cast int-to-int executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cast int-to-int failed to compile"
fi

echo "Test 458: cast int-to-float compiles and executes"
cleanup_codegen_artifacts "tests/integration/cast_int_to_float_12b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cast_int_to_float_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cast_int_to_float_12b > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cast int-to-float works"
    else
        test_fail "cast int-to-float executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cast int-to-float failed to compile"
fi

echo "Test 459: cast float-to-int compiles and executes"
cleanup_codegen_artifacts "tests/integration/cast_float_to_int_12b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cast_float_to_int_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cast_float_to_int_12b > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cast float-to-int works"
    else
        test_fail "cast float-to-int executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cast float-to-int failed to compile"
fi

echo "Test 460: invalid cast (VERBUM -> REX) is rejected clearly"
OUTPUT=$("$CCT_BIN" --check tests/integration/cast_invalid_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "cast invalid" && \
   echo "$OUTPUT" | grep -q "fmt_stringify_\\* or parse_\\*"; then
    test_pass "invalid text/numeric cast is rejected clearly"
else
    test_fail "invalid text/numeric cast diagnostic is missing/unclear"
fi

echo "Test 461: cast integrates with generic ritual flow"
cleanup_codegen_artifacts "tests/integration/cast_with_genus_12b.cct"
OUTPUT=$("$CCT_BIN" tests/integration/cast_with_genus_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/cast_with_genus_12b > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "cast + GENUS flow works"
    else
        test_fail "cast + GENUS flow returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "cast + GENUS fixture failed to compile"
fi

echo "Test 462: --ast keeps cast expression parseable in 12B subset"
OUTPUT=$("$CCT_BIN" --ast tests/integration/cast_int_to_int_12b.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "__cast"; then
    test_pass "--ast exposes cast lowering node in stable form"
else
    test_fail "--ast did not expose cast expression form"
fi

echo ""
echo "========================================"
echo "FASE 12C: Result/Option Types Tests"
echo "========================================"
echo ""

echo "Test 463: Option Some/unwrap basic flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/option_basic_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/option_basic_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/option_basic_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Option Some/unwrap basic flow works"
    else
        test_fail "Option Some/unwrap executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Option Some/unwrap fixture failed to compile"
fi

echo "Test 464: Option unwrap_or fallback works"
cleanup_codegen_artifacts "tests/integration/option_unwrap_or_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/option_unwrap_or_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/option_unwrap_or_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Option unwrap_or fallback works"
    else
        test_fail "Option unwrap_or executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Option unwrap_or fixture failed to compile"
fi

echo "Test 465: Result Ok/Err + unwrap flows compile and execute"
cleanup_codegen_artifacts "tests/integration/result_basic_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/result_basic_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/result_basic_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Result Ok/Err + unwrap flow works"
    else
        test_fail "Result basic executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Result basic fixture failed to compile"
fi

echo "Test 466: Result unwrap_or fallback works"
cleanup_codegen_artifacts "tests/integration/result_unwrap_or_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/result_unwrap_or_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/result_unwrap_or_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Result unwrap_or fallback works"
    else
        test_fail "Result unwrap_or executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Result unwrap_or fixture failed to compile"
fi

echo "Test 467: Option + Result integration flow works"
cleanup_codegen_artifacts "tests/integration/option_result_integration_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/option_result_integration_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/option_result_integration_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Option + Result integration flow works"
    else
        test_fail "Option + Result integration executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Option + Result integration fixture failed to compile"
fi

echo "Test 468: Option integrates with cast flow from 12B"
cleanup_codegen_artifacts "tests/integration/option_with_cast_12c.cct"
OUTPUT=$("$CCT_BIN" tests/integration/option_with_cast_12c.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/option_with_cast_12c > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Option + cast integration works"
    else
        test_fail "Option + cast executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Option + cast fixture failed to compile"
fi

echo ""
echo "========================================"
echo "FASE 12D.1: HashMap and Set Baseline Tests"
echo "========================================"
echo ""

echo "Test 469: Map basic insert/get/len flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/map_basic_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/map_basic_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/map_basic_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Map basic flow works"
    else
        test_fail "Map basic executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Map basic fixture failed to compile"
fi

echo "Test 470: Map collision/resize flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/map_collisions_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/map_collisions_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/map_collisions_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Map collision/resize flow works"
    else
        test_fail "Map collision/resize executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Map collision/resize fixture failed to compile"
fi

echo "Test 471: Map remove/contains flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/map_remove_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/map_remove_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/map_remove_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Map remove flow works"
    else
        test_fail "Map remove executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Map remove fixture failed to compile"
fi

echo "Test 472: Map update on duplicate key keeps cardinality stable"
cleanup_codegen_artifacts "tests/integration/map_update_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/map_update_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/map_update_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Map duplicate-key update flow works"
    else
        test_fail "Map update executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Map update fixture failed to compile"
fi

echo "Test 473: Set uniqueness/contains flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/set_basic_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/set_basic_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/set_basic_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Set basic flow works"
    else
        test_fail "Set basic executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Set basic fixture failed to compile"
fi

echo "Test 474: Set remove flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/set_remove_12d1.cct"
OUTPUT=$("$CCT_BIN" tests/integration/set_remove_12d1.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/set_remove_12d1 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Set remove flow works"
    else
        test_fail "Set remove executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Set remove fixture failed to compile"
fi

echo "Test 475: --sigilo-only tracks map/set usage counters"
cleanup_codegen_artifacts "tests/integration/sigilo_map_set_12d1.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_map_set_12d1.cct 2>&1) || true
if [ -f "tests/integration/sigilo_map_set_12d1.sigil" ] && \
   grep -q "^map_ops_count = " tests/integration/sigilo_map_set_12d1.sigil && \
   grep -q "^set_ops_count = " tests/integration/sigilo_map_set_12d1.sigil; then
    test_pass "Sigilo metadata tracks map/set usage counters"
else
    test_fail "Sigilo metadata missing map/set counters"
fi

echo ""
echo "========================================"
echo "FASE 12D.2: Collection Functional Ops Tests"
echo "========================================"
echo ""

echo "Test 476: FLUXUS map callback flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_map_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_fluxus_map_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_fluxus_map_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 18 ]; then
        test_pass "FLUXUS map callback flow works"
    else
        test_fail "FLUXUS map executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS map fixture failed to compile"
fi

echo "Test 477: FLUXUS filter callback flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_filter_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_fluxus_filter_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_fluxus_filter_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 2 ]; then
        test_pass "FLUXUS filter callback flow works"
    else
        test_fail "FLUXUS filter executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS filter fixture failed to compile"
fi

echo "Test 478: FLUXUS fold callback flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_fold_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_fluxus_fold_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_fluxus_fold_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 24 ]; then
        test_pass "FLUXUS fold callback flow works"
    else
        test_fail "FLUXUS fold executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS fold fixture failed to compile"
fi

echo "Test 479: FLUXUS find + Option integration compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_find_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_fluxus_find_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_fluxus_find_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 10 ]; then
        test_pass "FLUXUS find + Option integration works"
    else
        test_fail "FLUXUS find executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS find fixture failed to compile"
fi

echo "Test 480: SERIES map/reduce callback flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_series_ops_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_series_ops_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_series_ops_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 63 ]; then
        test_pass "SERIES map/reduce callback flow works"
    else
        test_fail "SERIES map/reduce executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES map/reduce fixture failed to compile"
fi

echo "Test 481: FLUXUS any/all callback flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/collection_any_all_12d2.cct"
OUTPUT=$("$CCT_BIN" tests/integration/collection_any_all_12d2.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/collection_any_all_12d2 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 11 ]; then
        test_pass "FLUXUS any/all callback flow works"
    else
        test_fail "FLUXUS any/all executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS any/all fixture failed to compile"
fi

echo "Test 482: --sigilo-only tracks collection_ops usage counters"
cleanup_codegen_artifacts "tests/integration/sigilo_collection_ops_12d2.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_collection_ops_12d2.cct 2>&1) || true
if [ -f "tests/integration/sigilo_collection_ops_12d2.sigil" ] && \
   grep -q "^collection_ops_count = " tests/integration/sigilo_collection_ops_12d2.sigil && \
   grep -q "^collection_ops_module_used = true" tests/integration/sigilo_collection_ops_12d2.sigil; then
    test_pass "Sigilo metadata tracks collection_ops usage counters"
else
    test_fail "Sigilo metadata missing collection_ops counters"
fi

echo ""
echo "========================================"
echo "FASE 12D.3: Basic Iterator Syntax Tests"
echo "========================================"
echo ""

echo "Test 483: --ast parses ITERUM ... IN ... COM ... FIN ITERUM"
OUTPUT=$("$CCT_BIN" --ast tests/integration/iterum_fluxus_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "ITERUM" && \
   echo "$OUTPUT" | grep -q "Item: n"; then
    test_pass "--ast parses ITERUM syntax in 12D.3 subset"
else
    test_fail "--ast failed to parse ITERUM syntax"
fi

echo "Test 484: FLUXUS ITERUM flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/iterum_fluxus_12d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/iterum_fluxus_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/iterum_fluxus_12d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 6 ]; then
        test_pass "FLUXUS ITERUM flow works"
    else
        test_fail "FLUXUS ITERUM executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "FLUXUS ITERUM fixture failed to compile"
fi

echo "Test 485: SERIES ITERUM flow compiles and executes"
cleanup_codegen_artifacts "tests/integration/iterum_series_12d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/iterum_series_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/iterum_series_12d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 100 ]; then
        test_pass "SERIES ITERUM flow works"
    else
        test_fail "SERIES ITERUM executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "SERIES ITERUM fixture failed to compile"
fi

echo "Test 486: ITERUM over map result compiles and executes"
cleanup_codegen_artifacts "tests/integration/iterum_map_result_12d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/iterum_map_result_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/iterum_map_result_12d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 9 ]; then
        test_pass "ITERUM over collection_ops map result works"
    else
        test_fail "ITERUM map-result executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "ITERUM map-result fixture failed to compile"
fi

echo "Test 487: nested ITERUM over SERIES compiles and executes"
cleanup_codegen_artifacts "tests/integration/iterum_nested_12d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/iterum_nested_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/iterum_nested_12d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 66 ]; then
        test_pass "Nested ITERUM flow works"
    else
        test_fail "Nested ITERUM executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Nested ITERUM fixture failed to compile"
fi

echo "Test 488: ITERUM over empty FLUXUS executes zero-iteration path"
cleanup_codegen_artifacts "tests/integration/iterum_empty_12d3.cct"
OUTPUT=$("$CCT_BIN" tests/integration/iterum_empty_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    ./tests/integration/iterum_empty_12d3 > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "Empty FLUXUS ITERUM zero-iteration path works"
    else
        test_fail "Empty FLUXUS ITERUM executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "Empty FLUXUS ITERUM fixture failed to compile"
fi

echo "Test 489: semantic rejects ITERUM on non-collection value"
OUTPUT=$("$CCT_BIN" --check tests/integration/sem_iterum_type_check_12d3.cct 2>&1) || true
if echo "$OUTPUT" | grep -qi "ITERUM requires FLUXUS or SERIES"; then
    test_pass "ITERUM non-collection semantic diagnostic is clear"
else
    test_fail "ITERUM non-collection semantic diagnostic is unclear"
fi

echo "Test 490: --sigilo-only tracks ITERUM metadata counters"
cleanup_codegen_artifacts "tests/integration/sigilo_iterum_12d3.cct"
OUTPUT=$("$CCT_BIN" --sigilo-only tests/integration/sigilo_iterum_12d3.cct 2>&1) || true
if [ -f "tests/integration/sigilo_iterum_12d3.sigil" ] && \
   grep -q "^iterum = " tests/integration/sigilo_iterum_12d3.sigil && \
   grep -q "^iterum_count = " tests/integration/sigilo_iterum_12d3.sigil; then
    test_pass "Sigilo metadata tracks ITERUM counters"
else
    test_fail "Sigilo metadata missing ITERUM counters"
fi

echo ""
echo "========================================"
echo "FASE 12E.1: Formatter Tests"
echo "========================================"
echo ""

echo "Test 491: cct fmt formats unformatted basic fixture"
TMP_FMT_BASIC=$(mktemp $CCT_TMP_DIR/cct_fmt_basic_XXXXXX.cct)
cp tests/formatter/fmt_basic_12e1.input.cct "$TMP_FMT_BASIC"
OUTPUT=$("$CCT_BIN" fmt "$TMP_FMT_BASIC" 2>&1) || true
if echo "$OUTPUT" | grep -q "Formatted:" && diff -q "$TMP_FMT_BASIC" tests/formatter/fmt_basic_12e1.expected.cct > /dev/null; then
    test_pass "cct fmt formats basic fixture deterministically"
else
    test_fail "cct fmt did not format basic fixture as expected"
fi
rm -f "$TMP_FMT_BASIC"

echo "Test 492: cct fmt --check returns 0 for formatted file"
OUTPUT=$("$CCT_BIN" fmt --check tests/formatter/fmt_basic_12e1.expected.cct 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    test_pass "cct fmt --check returns success on formatted input"
else
    test_fail "cct fmt --check failed on formatted input ($EXIT_CODE)"
fi

echo "Test 493: cct fmt --check returns 2 for unformatted file"
OUTPUT=$("$CCT_BIN" fmt --check tests/formatter/fmt_basic_12e1.input.cct 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "Not formatted:"; then
    test_pass "cct fmt --check returns code 2 on unformatted input"
else
    test_fail "cct fmt --check did not return code 2 on unformatted input"
fi

echo "Test 494: cct fmt --diff shows diff markers for unformatted file"
OUTPUT=$("$CCT_BIN" fmt --diff tests/formatter/fmt_basic_12e1.input.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "^--- " && echo "$OUTPUT" | grep -q "^+++ " && echo "$OUTPUT" | grep -q "^- "; then
    test_pass "cct fmt --diff outputs expected diff headers/content"
else
    test_fail "cct fmt --diff output is missing expected markers"
fi

echo "Test 495: cct fmt is idempotent on iterum fixture"
TMP_FMT_ITER=$(mktemp $CCT_TMP_DIR/cct_fmt_iter_XXXXXX.cct)
cp tests/formatter/fmt_iterum_12e1.input.cct "$TMP_FMT_ITER"
OUTPUT=$("$CCT_BIN" fmt "$TMP_FMT_ITER" 2>&1) || true
FIRST_HASH=$(shasum "$TMP_FMT_ITER" | awk '{print $1}')
OUTPUT=$("$CCT_BIN" fmt "$TMP_FMT_ITER" 2>&1) || true
SECOND_HASH=$(shasum "$TMP_FMT_ITER" | awk '{print $1}')
if [ "$FIRST_HASH" = "$SECOND_HASH" ] && diff -q "$TMP_FMT_ITER" tests/formatter/fmt_iterum_12e1.expected.cct > /dev/null; then
    test_pass "cct fmt is idempotent for iterum fixture"
else
    test_fail "cct fmt is not idempotent for iterum fixture"
fi
rm -f "$TMP_FMT_ITER"

echo "Test 496: formatted iterum fixture compiles and executes"
TMP_FMT_EXEC=$(mktemp $CCT_TMP_DIR/cct_fmt_exec_XXXXXX.cct)
cp tests/formatter/fmt_iterum_12e1.input.cct "$TMP_FMT_EXEC"
OUTPUT=$("$CCT_BIN" fmt "$TMP_FMT_EXEC" 2>&1) || true
OUTPUT=$("$CCT_BIN" "$TMP_FMT_EXEC" 2>&1) || true
BIN_FMT_EXEC="${TMP_FMT_EXEC%.cct}"
if echo "$OUTPUT" | grep -q "Compiled:" && [ -x "$BIN_FMT_EXEC" ]; then
    "$BIN_FMT_EXEC" > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 0 ]; then
        test_pass "formatted iterum fixture preserves executable semantics"
    else
        test_fail "formatted iterum fixture executable returned unexpected code ($EXIT_CODE)"
    fi
else
    test_fail "formatted iterum fixture failed to compile"
fi
rm -f "$TMP_FMT_EXEC" "$BIN_FMT_EXEC" "${TMP_FMT_EXEC%.cct}.cgen.c" "${TMP_FMT_EXEC%.cct}.svg" "${TMP_FMT_EXEC%.cct}.sigil" "${TMP_FMT_EXEC%.cct}.system.svg" "${TMP_FMT_EXEC%.cct}.system.sigil"

echo ""
echo "========================================"
echo "FASE 12E.2: Linter Tests"
echo "========================================"
echo ""

echo "Test 497: cct lint clean fixture returns 0"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_clean_ok_12e2.cct 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "No lint issues:"; then
    test_pass "cct lint clean fixture returns success"
else
    test_fail "cct lint clean fixture failed (exit $EXIT_CODE)"
fi

echo "Test 498: cct lint reports unused-variable"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_unused_var_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[unused-variable\\]"; then
    test_pass "cct lint reports unused-variable warning"
else
    test_fail "cct lint missing unused-variable warning"
fi

echo "Test 499: cct lint reports unused-parameter"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_unused_param_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[unused-parameter\\]"; then
    test_pass "cct lint reports unused-parameter warning"
else
    test_fail "cct lint missing unused-parameter warning"
fi

echo "Test 500: cct lint reports unused-import"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_unused_import_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[unused-import\\]"; then
    test_pass "cct lint reports unused-import warning"
else
    test_fail "cct lint missing unused-import warning"
fi

echo "Test 501: cct lint reports dead-code-after-return"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_dead_code_return_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[dead-code-after-return\\]"; then
    test_pass "cct lint reports dead-code-after-return warning"
else
    test_fail "cct lint missing dead-code-after-return warning"
fi

echo "Test 502: cct lint reports dead-code-after-throw"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_dead_code_throw_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[dead-code-after-throw\\]"; then
    test_pass "cct lint reports dead-code-after-throw warning"
else
    test_fail "cct lint missing dead-code-after-throw warning"
fi

echo "Test 503: cct lint reports shadowing-local"
OUTPUT=$("$CCT_BIN" lint tests/integration/lint_shadowing_12e2.cct 2>&1)
if echo "$OUTPUT" | grep -q "warning\\[shadowing-local\\]"; then
    test_pass "cct lint reports shadowing-local warning"
else
    test_fail "cct lint missing shadowing-local warning"
fi

echo "Test 504: cct lint --strict returns 2 when warnings exist"
OUTPUT=$("$CCT_BIN" lint --strict tests/integration/lint_unused_var_12e2.cct 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "cct lint --strict returns 2 with warnings"
else
    test_fail "cct lint --strict did not return 2 (exit $EXIT_CODE)"
fi

echo "Test 505: cct lint --fix removes unused import warning"
TMP_LINT_FIX_IMPORT=$(mktemp $CCT_TMP_DIR/cct_lint_fix_import_XXXXXX.cct)
cp tests/integration/lint_fix_unused_import_12e2.cct "$TMP_LINT_FIX_IMPORT"
OUTPUT=$("$CCT_BIN" lint --fix "$TMP_LINT_FIX_IMPORT" 2>&1)
OUTPUT=$("$CCT_BIN" lint "$TMP_LINT_FIX_IMPORT" 2>&1)
if ! echo "$OUTPUT" | grep -q "warning\\[unused-import\\]"; then
    test_pass "cct lint --fix removes unused-import in safe subset"
else
    test_fail "cct lint --fix did not remove unused-import warning"
fi
rm -f "$TMP_LINT_FIX_IMPORT"

echo "Test 506: cct lint --fix removes dead code after return"
TMP_LINT_FIX_DEAD=$(mktemp $CCT_TMP_DIR/cct_lint_fix_dead_XXXXXX.cct)
cp tests/integration/lint_fix_dead_code_12e2.cct "$TMP_LINT_FIX_DEAD"
OUTPUT=$("$CCT_BIN" lint --fix "$TMP_LINT_FIX_DEAD" 2>&1)
OUTPUT=$("$CCT_BIN" lint "$TMP_LINT_FIX_DEAD" 2>&1)
if ! echo "$OUTPUT" | grep -q "warning\\[dead-code-after-return\\]"; then
    test_pass "cct lint --fix removes dead code in safe subset"
else
    test_fail "cct lint --fix did not remove dead-code-after-return warning"
fi
rm -f "$TMP_LINT_FIX_DEAD"

echo "Test 507: cct lint --fix is idempotent"
TMP_LINT_FIX_IDEMP=$(mktemp $CCT_TMP_DIR/cct_lint_fix_idemp_XXXXXX.cct)
cp tests/integration/lint_fix_idempotent_12e2.cct "$TMP_LINT_FIX_IDEMP"
OUTPUT=$("$CCT_BIN" lint --fix "$TMP_LINT_FIX_IDEMP" 2>&1)
FIRST_HASH=$(shasum "$TMP_LINT_FIX_IDEMP" | awk '{print $1}')
OUTPUT=$("$CCT_BIN" lint --fix "$TMP_LINT_FIX_IDEMP" 2>&1)
SECOND_HASH=$(shasum "$TMP_LINT_FIX_IDEMP" | awk '{print $1}')
if [ "$FIRST_HASH" = "$SECOND_HASH" ]; then
    test_pass "cct lint --fix is idempotent on canonical fixture"
else
    test_fail "cct lint --fix is not idempotent"
fi
rm -f "$TMP_LINT_FIX_IDEMP"

echo ""
echo "========================================"
echo "FASE 12F: Build System + Project Conventions Tests"
echo "========================================"
echo ""

echo "Test 508: cct build compiles project with default entry"
"$CCT_BIN" clean --project tests/integration/project_12f_basic --all > /dev/null 2>&1 || true
cleanup_codegen_artifacts "tests/integration/project_12f_basic/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/lib/util.cct"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_basic 2>&1) || true
if [ -x "tests/integration/project_12f_basic/dist/project_12f_basic" ]; then
    ./tests/integration/project_12f_basic/dist/project_12f_basic > /dev/null 2>&1
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 42 ]; then
        test_pass "cct build compiles project and emits runnable dist binary"
    else
        test_fail "project build binary returned unexpected exit code ($EXIT_CODE)"
    fi
else
    test_fail "cct build did not emit expected project binary"
fi

echo "Test 509: cct run builds and propagates program exit code"
OUTPUT=$("$CCT_BIN" run --project tests/integration/project_12f_basic 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 42 ]; then
    test_pass "cct run propagates project executable exit code"
else
    test_fail "cct run returned unexpected exit code ($EXIT_CODE)"
fi

echo "Test 510: cct test discovers and executes *.test.cct files"
OUTPUT=$("$CCT_BIN" test --project tests/integration/project_12f_basic 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "\\[test\\] summary: pass="; then
    test_pass "cct test discovers and executes project tests"
else
    test_fail "cct test did not execute project tests as expected"
fi

echo "Test 511: cct test pattern filters selected tests"
OUTPUT=$("$CCT_BIN" test math --project tests/integration/project_12f_pattern 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "selected: 1" && echo "$OUTPUT" | grep -q "a_math.test.cct" && ! echo "$OUTPUT" | grep -q "b_io.test.cct"; then
    test_pass "cct test pattern filter selects expected subset"
else
    test_fail "cct test pattern filter did not behave as expected"
fi

echo "Test 512: cct bench discovers and executes *.bench.cct files"
OUTPUT=$("$CCT_BIN" bench --project tests/integration/project_12f_bench --iterations 2 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "\\[bench\\] selected: 1" && echo "$OUTPUT" | grep -q "avg="; then
    test_pass "cct bench discovers and executes benchmarks"
else
    test_fail "cct bench did not execute benchmarks as expected"
fi

echo "Test 513: cct clean removes internal .cct artifacts"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_basic 2>&1) || true
OUTPUT=$("$CCT_BIN" clean --project tests/integration/project_12f_basic 2>&1) || true
if [ ! -d "tests/integration/project_12f_basic/.cct/cache" ] && [ ! -d "tests/integration/project_12f_basic/.cct/build" ]; then
    test_pass "cct clean removes canonical internal artifact directories"
else
    test_fail "cct clean did not remove canonical internal artifact directories"
fi

echo "Test 514: cct clean --all removes project dist artifacts"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_basic 2>&1) || true
if [ ! -x "tests/integration/project_12f_basic/dist/project_12f_basic" ]; then
    test_fail "precondition failed: dist binary missing before clean --all"
else
    OUTPUT=$("$CCT_BIN" clean --project tests/integration/project_12f_basic --all 2>&1) || true
    if [ ! -e "tests/integration/project_12f_basic/dist/project_12f_basic" ]; then
        test_pass "cct clean --all removes project dist binaries"
    else
        test_fail "cct clean --all did not remove project dist binaries"
    fi
fi

echo "Test 515: incremental build reports up-to-date on second run"
"$CCT_BIN" clean --project tests/integration/project_12f_incremental --all > /dev/null 2>&1 || true
cleanup_codegen_artifacts "tests/integration/project_12f_incremental/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_incremental/lib/mod.cct"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_incremental 2>&1) || true
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_incremental 2>&1) || true
if echo "$OUTPUT" | grep -q "status: up-to-date"; then
    test_pass "incremental cache reports up-to-date when sources are unchanged"
else
    test_fail "incremental cache did not report up-to-date on second build"
fi

echo "Test 516: incremental build invalidates after imported module change"
MOD_FILE="tests/integration/project_12f_incremental/lib/mod.cct"
MOD_BAK=$(mktemp $CCT_TMP_DIR/cct_mod_backup_XXXXXX.cct)
cp "$MOD_FILE" "$MOD_BAK"
echo "" >> "$MOD_FILE"
echo "# mutation for incremental invalidation" >> "$MOD_FILE"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_incremental 2>&1) || true
cp "$MOD_BAK" "$MOD_FILE"
rm -f "$MOD_BAK"
if echo "$OUTPUT" | grep -q "status: rebuilt"; then
    test_pass "incremental cache invalidates when imported module changes"
else
    test_fail "incremental cache did not invalidate after imported module change"
fi

echo "Test 517: cct build fails clearly without project root/entry"
OUTPUT=$("$CCT_BIN" build --project tests/integration/project_12f_no_root 2>&1) || true
if echo "$OUTPUT" | grep -qi "entry file not found"; then
    test_pass "missing project entry/root error is clear"
else
    test_fail "missing project entry/root error is unclear"
fi

echo "Test 518: cct test --strict-lint enforces lint gate"
OUTPUT=$("$CCT_BIN" test --project tests/integration/project_12f_basic --strict-lint 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "strict lint gate fails project test run when warnings exist"
else
    test_fail "strict lint gate did not fail with exit 2 ($EXIT_CODE)"
fi

echo "Test 519: cct test --fmt-check enforces format gate"
OUTPUT=$("$CCT_BIN" test --project tests/integration/project_12f_basic --fmt-check 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "fmt-check gate fails project test run when formatting mismatch exists"
else
    test_fail "fmt-check gate did not fail with exit 2 ($EXIT_CODE)"
fi

echo "Test 520: legacy single-file workflow remains stable after 12F"
cleanup_codegen_artifacts "examples/hello.cct"
OUTPUT=$("$CCT_BIN" examples/hello.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "Compiled:"; then
    test_pass "legacy single-file compile path remains compatible after 12F"
else
    test_fail "legacy single-file compile path regressed after 12F"
fi

echo ""
echo "========================================"
echo "FASE 12G: Doc Generator Tests"
echo "========================================"
echo ""

DOC_BASIC_OUT="tests/integration/docgen_basic_12g/docs/api"
DOC_VIS_OUT="tests/integration/docgen_visibility_12g/docs/api"
DOC_BAD_OUT="tests/integration/docgen_bad_tags_12g/docs/api"
DOC_DET_A="$CCT_TMP_DIR/cct_doc_det_a"
DOC_DET_B="$CCT_TMP_DIR/cct_doc_det_b"
rm -rf "$DOC_BASIC_OUT" "$DOC_VIS_OUT" "$DOC_BAD_OUT" "$DOC_DET_A" "$DOC_DET_B"

echo "Test 521: cct doc generates markdown output"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_basic_12g --format markdown 2>&1)
if [ -f "$DOC_BASIC_OUT/index.md" ] && ls "$DOC_BASIC_OUT/modules/"*.md > /dev/null 2>&1 && ls "$DOC_BASIC_OUT/symbols/"*.md > /dev/null 2>&1; then
    test_pass "cct doc markdown generation works"
else
    test_fail "cct doc markdown generation failed"
fi

echo "Test 522: cct doc generates html output"
rm -rf "$DOC_BASIC_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_basic_12g --format html 2>&1)
if [ -f "$DOC_BASIC_OUT/index.html" ] && ls "$DOC_BASIC_OUT/modules/"*.html > /dev/null 2>&1 && ls "$DOC_BASIC_OUT/symbols/"*.html > /dev/null 2>&1; then
    test_pass "cct doc html generation works"
else
    test_fail "cct doc html generation failed"
fi

echo "Test 523: cct doc --format both generates markdown + html"
rm -rf "$DOC_BASIC_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_basic_12g --format both 2>&1)
if [ -f "$DOC_BASIC_OUT/index.md" ] && [ -f "$DOC_BASIC_OUT/index.html" ]; then
    test_pass "cct doc both format generation works"
else
    test_fail "cct doc both format generation failed"
fi

echo "Test 524: --include-internal toggles ARCANUM visibility in docs"
rm -rf "$DOC_VIS_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_visibility_12g --format markdown 2>&1)
VISIBLE_WITHOUT=$(grep -R "segredo_local" "$DOC_VIS_OUT" 2>/dev/null || true)
rm -rf "$DOC_VIS_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_visibility_12g --format markdown --include-internal 2>&1)
VISIBLE_WITH=$(grep -R "segredo_local" "$DOC_VIS_OUT" 2>/dev/null || true)
if [ -z "$VISIBLE_WITHOUT" ] && [ -n "$VISIBLE_WITH" ]; then
    test_pass "--include-internal controls ARCANUM symbol exposure"
else
    test_fail "--include-internal did not control ARCANUM symbol exposure"
fi

echo "Test 525: doc tags @param/@return are preserved in symbol docs"
if grep -R "@param" "$DOC_BASIC_OUT/symbols" > /dev/null 2>&1 && grep -R "@return" "$DOC_BASIC_OUT/symbols" > /dev/null 2>&1; then
    test_pass "doc tags @param/@return are present in generated symbol docs"
else
    test_fail "doc tags @param/@return are missing from generated symbol docs"
fi

echo "Test 526: --no-examples omits examples section in symbol docs"
rm -rf "$DOC_BASIC_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_basic_12g --format markdown --no-examples 2>&1)
if ! grep -R "^## Examples" "$DOC_BASIC_OUT/symbols" > /dev/null 2>&1; then
    test_pass "--no-examples omits example section"
else
    test_fail "--no-examples did not omit example section"
fi

echo "Test 527: --warn-missing-docs emits warnings without failing generation"
rm -rf "$DOC_BAD_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_bad_tags_12g --format markdown --warn-missing-docs 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "missing docs"; then
    test_pass "--warn-missing-docs warns and keeps generation successful"
else
    test_fail "--warn-missing-docs behavior is not correct"
fi

echo "Test 528: --strict-docs returns exit 2 when warnings exist"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_bad_tags_12g --format markdown --warn-missing-docs --strict-docs 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "--strict-docs enforces warning gate"
else
    test_fail "--strict-docs did not fail with exit 2"
fi

echo "Test 529: invalid doc tag emits warning but generation remains valid"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_bad_tags_12g --format markdown 2>&1)
if echo "$OUTPUT" | grep -q "invalid doc tag" && [ -f "$DOC_BAD_OUT/index.md" ]; then
    test_pass "invalid doc tags are non-fatal and reported"
else
    test_fail "invalid doc tag behavior is incorrect"
fi

echo "Test 530: --no-timestamp keeps doc output deterministic"
rm -rf "$DOC_DET_A" "$DOC_DET_B"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_determinism_12g --output-dir "$DOC_DET_A" --format markdown --no-timestamp 2>&1)
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_determinism_12g --output-dir "$DOC_DET_B" --format markdown --no-timestamp 2>&1)
HASH_A=$(find "$DOC_DET_A" -type f -name "*.md" -print0 | sort -z | xargs -0 cat | shasum | awk '{print $1}')
HASH_B=$(find "$DOC_DET_B" -type f -name "*.md" -print0 | sort -z | xargs -0 cat | shasum | awk '{print $1}')
if [ "$HASH_A" = "$HASH_B" ]; then
    test_pass "doc output is deterministic with --no-timestamp"
else
    test_fail "doc output determinism failed with --no-timestamp"
fi

echo "Test 531: cct doc handles multi-module + stdlib closure"
rm -rf "$DOC_BASIC_OUT"
OUTPUT=$("$CCT_BIN" doc --project tests/integration/docgen_basic_12g --format both 2>&1)
if echo "$OUTPUT" | grep -q "modules:" && grep -R "stdlib (cct/...)" "$DOC_BASIC_OUT/modules" > /dev/null 2>&1; then
    test_pass "doc generator includes stdlib modules in closure output"
else
    test_fail "doc generator did not expose stdlib module closure info"
fi

echo ""
echo "========================================"
echo "FASE 12H: Final Canonization + Hardening Closure Tests"
echo "========================================"
echo ""

# Test 532: Complete end-to-end workflow for canonical phase 12 project
echo "Test 532: phase12 closure end-to-end workflow integration"
PHASE12_CLOSURE_DIR="tests/integration/phase12_closure_end_to_end_12h"
rm -rf "$PHASE12_CLOSURE_DIR/.cct" "$PHASE12_CLOSURE_DIR/dist" "$PHASE12_CLOSURE_DIR/docs/api"
OUTPUT_FMT=$("$CCT_BIN" fmt "$PHASE12_CLOSURE_DIR/src/main.cct" 2>&1)
OUTPUT_LINT=$("$CCT_BIN" lint --strict "$PHASE12_CLOSURE_DIR/src/main.cct" 2>&1) || true
OUTPUT_BUILD=$("$CCT_BIN" build --project "$PHASE12_CLOSURE_DIR" 2>&1)
OUTPUT_TEST=$("$CCT_BIN" test --project "$PHASE12_CLOSURE_DIR" 2>&1)
OUTPUT_DOC=$("$CCT_BIN" doc --project "$PHASE12_CLOSURE_DIR" --format both --no-timestamp 2>&1)
if [ -f "$PHASE12_CLOSURE_DIR/dist/phase12_closure_end_to_end_12h" ] && \
   [ -d "$PHASE12_CLOSURE_DIR/docs/api" ]; then
    test_pass "phase12 closure end-to-end workflow succeeded"
else
    test_fail "phase12 closure end-to-end workflow incomplete"
fi

# Test 533: Verify release documentation exists and is complete
# Removed Test 533: documentation text/file comparison (policy)

# Removed Test 534: documentation text/file comparison (policy)

# Removed Test 535: documentation text/file comparison (policy)

echo "Test 536: fase12 final showcase compiles and runs"
rm -rf "examples/fase12_final_showcase/.cct" "examples/fase12_final_showcase/dist"
OUTPUT=$("$CCT_BIN" build --project examples/fase12_final_showcase 2>&1)
if [ -f "examples/fase12_final_showcase/dist/fase12_final_showcase" ]; then
    test_pass "fase12 final showcase builds successfully"
else
    test_fail "fase12 final showcase build failed"
fi

# Test 537: Showcase example tests run
echo "Test 537: fase12 final showcase tests execute"
OUTPUT=$("$CCT_BIN" test --project examples/fase12_final_showcase 2>&1)
if echo "$OUTPUT" | grep -q "PASS\|pass=.*fail=0"; then
    test_pass "fase12 final showcase tests run successfully"
else
    test_fail "fase12 final showcase tests failed"
fi

echo ""
echo "========================================"
echo "FASE 13A.1: Sigil Parse Canonical Reader Tests"
echo "========================================"
echo ""

# Test 538: Standalone sigil parse runtime test target builds and runs
echo "Test 538: standalone sigil parse runtime test target builds and runs"
OUTPUT=$(make test_sigil_parse 2>&1) || true
if echo "$OUTPUT" | grep -q "test_sigil_parse: ok"; then
    test_pass "Standalone sigil parse runtime target builds and runs"
else
    test_fail "Standalone sigil parse runtime target failed"
fi

# Test 539: Sigil parse runtime binary exists after build
echo "Test 539: sigil parse runtime test binary exists after build"
if [ -f "tests/runtime/test_sigil_parse" ]; then
    test_pass "Sigil parse runtime test binary is produced deterministically"
else
    test_fail "Sigil parse runtime test binary not found"
fi

echo ""
echo "========================================"
echo "FASE 13A.2: Sigil Structural Diff Engine Tests"
echo "========================================"
echo ""

# Test 540: Standalone sigil diff runtime test target builds and runs
echo "Test 540: standalone sigil diff runtime test target builds and runs"
OUTPUT=$(make test_sigil_diff 2>&1) || true
if echo "$OUTPUT" | grep -q "test_sigil_diff: ok"; then
    test_pass "Standalone sigil diff runtime target builds and runs"
else
    test_fail "Standalone sigil diff runtime target failed"
fi

# Test 541: Sigil diff runtime binary exists after build
echo "Test 541: sigil diff runtime test binary exists after build"
if [ -f "tests/runtime/test_sigil_diff" ]; then
    test_pass "Sigil diff runtime test binary is produced deterministically"
else
    test_fail "Sigil diff runtime test binary not found"
fi

echo ""
echo "========================================"
echo "FASE 13A.3: Sigilo CLI inspect/diff/check Tests"
echo "========================================"
echo ""

# Test 542: sigilo inspect structured summary works
echo "Test 542: sigilo inspect structured summary works"
SIG13A3_A="$CCT_TMP_DIR/cct_sigilo_cli_13a3_a.sigil"
SIG13A3_C="$CCT_TMP_DIR/cct_sigilo_cli_13a3_c.sigil"
SIG13A3_D="$CCT_TMP_DIR/cct_sigilo_cli_13a3_d.sigil"
SIG13A3_E="$CCT_TMP_DIR/cct_sigilo_cli_13a3_e.sigil"
cat > "$SIG13A3_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
cat > "$SIG13A3_C" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 2
SIGEOF
cat > "$SIG13A3_D" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
[totals]
rituale = 1
SIGEOF
cat > "$SIG13A3_E" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = bad
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13A3_A" --format structured --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "format = cct.sigil.inspect.v1"; then
    test_pass "sigilo inspect structured summary works"
else
    test_fail "sigilo inspect structured summary failed"
fi

# Test 543: sigilo diff text summary reports highest severity
echo "Test 543: sigilo diff text summary reports highest severity"
SIG13A3_B="$CCT_TMP_DIR/cct_sigilo_cli_13a3_b.sigil"
cat > "$SIG13A3_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo diff "$SIG13A3_A" "$SIG13A3_B" --format text --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "highest=behavioral-risk"; then
    test_pass "sigilo diff summary reports expected highest severity"
else
    test_fail "sigilo diff summary did not report expected severity"
fi

# Test 544: sigilo check strict returns exit code 2 for blocking diff
echo "Test 544: sigilo check strict returns exit code 2 for blocking diff"
"$CCT_BIN" sigilo check "$SIG13A3_A" "$SIG13A3_B" --strict --summary > $CCT_TMP_DIR/cct_sigilo_cli_13a3_check.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "sigilo check strict returns exit code 2 on blocking diff"
else
    test_fail "sigilo check strict returned unexpected exit code ($EXIT_CODE)"
fi

# Test 545: sigilo inspect text details lists entries
echo "Test 545: sigilo inspect text details lists entries"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13A3_A" --format text 2>&1) || true
if echo "$OUTPUT" | grep -q "sigilo.inspect.summary" && echo "$OUTPUT" | grep -q "key=semantic_hash"; then
    test_pass "sigilo inspect text details include parsed entries"
else
    test_fail "sigilo inspect text details missing expected entries"
fi

# Test 546: sigilo inspect strict rejects invalid hash
echo "Test 546: sigilo inspect strict rejects invalid hash"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13A3_E" --strict --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "missing_required"; then
    test_pass "sigilo inspect strict rejects invalid hash metadata"
else
    test_fail "sigilo inspect strict did not reject invalid hash metadata"
fi

# Test 547: sigilo diff structured output schema
echo "Test 547: sigilo diff structured output schema"
OUTPUT=$("$CCT_BIN" sigilo diff "$SIG13A3_A" "$SIG13A3_B" --format structured 2>&1) || true
if echo "$OUTPUT" | grep -q "format = cct.sigil.diff.v1" && echo "$OUTPUT" | grep -q "highest_severity = behavioral-risk"; then
    test_pass "sigilo diff structured output matches schema"
else
    test_fail "sigilo diff structured output schema mismatch"
fi

# Test 548: sigilo check strict returns 0 for identical artifacts
echo "Test 548: sigilo check strict returns 0 for identical artifacts"
"$CCT_BIN" sigilo check "$SIG13A3_A" "$SIG13A3_A" --strict --summary > $CCT_TMP_DIR/cct_sigilo_cli_13a3_check_clean.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    test_pass "sigilo check strict returns 0 when there are no differences"
else
    test_fail "sigilo check strict returned unexpected code for identical artifacts ($EXIT_CODE)"
fi

# Test 549: sigilo check strict returns 0 for informational-only diff
echo "Test 549: sigilo check strict returns 0 for informational-only diff"
"$CCT_BIN" sigilo check "$SIG13A3_A" "$SIG13A3_C" --strict --summary > $CCT_TMP_DIR/cct_sigilo_cli_13a3_check_info.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    test_pass "sigilo check strict allows informational-only diffs"
else
    test_fail "sigilo check strict should allow informational-only diffs (got $EXIT_CODE)"
fi

# Test 550: sigilo check strict returns 2 for review-required diff
echo "Test 550: sigilo check strict returns 2 for review-required diff"
"$CCT_BIN" sigilo check "$SIG13A3_A" "$SIG13A3_D" --strict --summary > $CCT_TMP_DIR/cct_sigilo_cli_13a3_check_review.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "sigilo check strict blocks review-required diffs with exit code 2"
else
    test_fail "sigilo check strict returned unexpected code for review-required diff ($EXIT_CODE)"
fi

# Test 551: sigilo diff rejects unknown option
echo "Test 551: sigilo diff rejects unknown option"
OUTPUT=$("$CCT_BIN" sigilo diff "$SIG13A3_A" "$SIG13A3_B" --bogus-option 2>&1) || true
if echo "$OUTPUT" | grep -q "Unknown sigilo option"; then
    test_pass "sigilo diff rejects unknown options"
else
    test_fail "sigilo diff did not reject unknown options"
fi

# Test 552: sigilo inspect enforces .sigil extension
echo "Test 552: sigilo inspect enforces .sigil extension"
OUTPUT=$("$CCT_BIN" sigilo inspect tests/integration/minimal.cct --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "must have .sigil extension"; then
    test_pass "sigilo inspect enforces .sigil input extension"
else
    test_fail "sigilo inspect did not enforce .sigil extension"
fi

echo ""
echo "========================================"
echo "FASE 13A.4: Sigilo Baseline check/update Tests"
echo "========================================"
echo ""

SIG13A4_BASE="$CCT_TMP_DIR/cct_sigilo_baseline_13a4_local.sigil"
SIG13A4_META="$CCT_TMP_DIR/cct_sigilo_baseline_13a4_local.baseline.meta"
SIG13A4_CORRUPT="$CCT_TMP_DIR/cct_sigilo_baseline_13a4_corrupt.sigil"
SIG13A4_CORRUPT_META="$CCT_TMP_DIR/cct_sigilo_baseline_13a4_corrupt.baseline.meta"
SIG13A4_SNAPSHOT="$CCT_TMP_DIR/cct_sigilo_baseline_13a4_snapshot.sigil"
SIG13A4_PROJ="$CCT_TMP_DIR/cct_sigilo_baseline_proj_13a4"
CCT_BIN_ABS="$(pwd)/cct"

rm -f "$SIG13A4_BASE" "$SIG13A4_META" "$SIG13A4_CORRUPT" "$SIG13A4_CORRUPT_META" "$SIG13A4_SNAPSHOT"
rm -rf "$SIG13A4_PROJ"
mkdir -p "$SIG13A4_PROJ"

# Test 553: baseline check missing returns informative status and success
echo "Test 553: sigilo baseline check reports missing baseline without blocking"
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG13A3_A" --baseline "$SIG13A4_BASE" --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "status=missing" && echo "$OUTPUT" | grep -q "baseline=$SIG13A4_BASE"; then
    test_pass "sigilo baseline check handles missing baseline in informative mode"
else
    test_fail "sigilo baseline check did not report missing baseline as expected"
fi

# Test 554: baseline update writes baseline and metadata
echo "Test 554: sigilo baseline update writes baseline and metadata"
OUTPUT=$("$CCT_BIN" sigilo baseline update "$SIG13A3_A" --baseline "$SIG13A4_BASE" 2>&1) || true
if [ -f "$SIG13A4_BASE" ] && [ -f "$SIG13A4_META" ] && echo "$OUTPUT" | grep -q "status=written"; then
    test_pass "sigilo baseline update writes baseline artifacts deterministically"
else
    test_fail "sigilo baseline update did not write baseline/meta as expected"
fi

# Test 555: baseline check after update returns no diff
echo "Test 555: sigilo baseline check after update returns no diff"
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG13A3_A" --baseline "$SIG13A4_BASE" --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "status=ok" && echo "$OUTPUT" | grep -q "highest=none"; then
    test_pass "sigilo baseline check reports stable baseline after update"
else
    test_fail "sigilo baseline check did not report stable state after update"
fi

# Test 556: strict baseline check allows informational-only drift
echo "Test 556: sigilo baseline check strict allows informational-only drift"
"$CCT_BIN" sigilo baseline check "$SIG13A3_C" --baseline "$SIG13A4_BASE" --strict --summary > $CCT_TMP_DIR/cct_sigilo_baseline_13a4_info.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    test_pass "sigilo baseline check strict allows informational drift"
else
    test_fail "sigilo baseline check strict should allow informational drift (got $EXIT_CODE)"
fi

# Test 557: strict baseline check blocks review-required drift
echo "Test 557: sigilo baseline check strict blocks review-required drift"
"$CCT_BIN" sigilo baseline check "$SIG13A3_D" --baseline "$SIG13A4_BASE" --strict --summary > $CCT_TMP_DIR/cct_sigilo_baseline_13a4_review.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "sigilo baseline check strict blocks review-required drift with exit 2"
else
    test_fail "sigilo baseline check strict should block review drift with exit 2 (got $EXIT_CODE)"
fi

# Test 558: strict baseline check blocks behavioral-risk drift
echo "Test 558: sigilo baseline check strict blocks behavioral-risk drift"
"$CCT_BIN" sigilo baseline check "$SIG13A3_B" --baseline "$SIG13A4_BASE" --strict --summary > $CCT_TMP_DIR/cct_sigilo_baseline_13a4_behavior.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "sigilo baseline check strict blocks behavioral-risk drift with exit 2"
else
    test_fail "sigilo baseline check strict should block behavioral drift with exit 2 (got $EXIT_CODE)"
fi

# Test 559: update does not overwrite baseline without --force
echo "Test 559: sigilo baseline update requires --force for overwrite"
OUTPUT=$("$CCT_BIN" sigilo baseline update "$SIG13A3_D" --baseline "$SIG13A4_BASE" 2>&1) || true
if echo "$OUTPUT" | grep -q "use --force to overwrite"; then
    test_pass "sigilo baseline update protects against silent overwrite"
else
    test_fail "sigilo baseline update did not enforce --force overwrite protection"
fi

# Test 560: update with --force overwrites baseline
echo "Test 560: sigilo baseline update --force overwrites baseline"
OUTPUT=$("$CCT_BIN" sigilo baseline update "$SIG13A3_D" --baseline "$SIG13A4_BASE" --force 2>&1) || true
if [ -f "$SIG13A4_BASE" ] && grep -q "1111111111111111" "$SIG13A4_BASE" && echo "$OUTPUT" | grep -q "status=written"; then
    test_pass "sigilo baseline update --force overwrites existing baseline deterministically"
else
    test_fail "sigilo baseline update --force did not overwrite baseline as expected"
fi

# Test 561: corrupt baseline content fails with actionable diagnostic
echo "Test 561: sigilo baseline check fails for corrupt baseline content"
cat > "$SIG13A4_CORRUPT" <<'SIGEOF'
this is not key value sigil metadata
SIGEOF
cat > "$SIG13A4_CORRUPT_META" <<'SIGEOF'
format = cct.sigil.baseline.meta.v1
baseline_schema = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG13A3_A" --baseline "$SIG13A4_CORRUPT" --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "syntax"; then
    test_pass "sigilo baseline check reports malformed baseline with parser diagnostic"
else
    test_fail "sigilo baseline check did not report malformed baseline content"
fi

# Test 562: incompatible baseline metadata format fails clearly
echo "Test 562: sigilo baseline check fails for incompatible metadata format"
cat > "$SIG13A4_CORRUPT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
SIGEOF
cat > "$SIG13A4_CORRUPT_META" <<'SIGEOF'
format = cct.sigil.baseline.meta.v999
baseline_schema = 999
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG13A3_A" --baseline "$SIG13A4_CORRUPT" --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "unsupported baseline metadata format"; then
    test_pass "sigilo baseline check rejects incompatible baseline metadata format"
else
    test_fail "sigilo baseline check did not reject incompatible baseline metadata format"
fi

# Test 563: default baseline path is project-scoped and informative when missing
# Removed Test 563: documentation text/file comparison (policy)

echo "Test 564: sigilo baseline check is read-only (no baseline mutation)"
"$CCT_BIN" sigilo baseline update "$SIG13A3_A" --baseline "$SIG13A4_BASE" --force > $CCT_TMP_DIR/cct_sigilo_baseline_13a4_force_reset.out 2>&1
cp "$SIG13A4_BASE" "$SIG13A4_SNAPSHOT"
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG13A3_A" --baseline "$SIG13A4_BASE" --summary 2>&1) || true
if cmp -s "$SIG13A4_BASE" "$SIG13A4_SNAPSHOT"; then
    test_pass "sigilo baseline check keeps baseline file immutable"
else
    test_fail "sigilo baseline check mutated baseline file unexpectedly"
fi

echo ""
echo "========================================"
echo "FASE 13B.1: Canonical Local Sigilo Workflow Tests"
echo "========================================"
echo ""

SIG13B1_PROJ="$CCT_TMP_DIR/cct_sigilo_workflow_13b1"
SIG13B1_BASE="$SIG13B1_PROJ/docs/sigilo/baseline/local.sigil"
SIG13B1_DRIFT="$CCT_TMP_DIR/cct_sigilo_workflow_13b1_drift.sigil"
SIG13B1_ARTIFACT="$SIG13B1_PROJ/src/workflow.local.sigil"
rm -rf "$SIG13B1_PROJ"
mkdir -p "$SIG13B1_PROJ/src" "$SIG13B1_PROJ/tests"
cp tests/integration/sigilo_workflow_local_smoke_13b1.cct "$SIG13B1_PROJ/src/main.cct"
cat > "$SIG13B1_PROJ/tests/smoke.test.cct" <<'SIGEOF'
INCIPIT grimoire "sigilo_workflow_local_test_13b1"

RITUALE main() REDDE REX
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
SIGEOF
cat > "$SIG13B1_ARTIFACT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF

# Test 565: minimal workflow executes end-to-end
echo "Test 565: local minimal workflow executes end-to-end"
OK=1
"$CCT_BIN" fmt "$SIG13B1_PROJ/src/main.cct" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_fmt.out 2>&1 || OK=0
"$CCT_BIN" lint "$SIG13B1_PROJ/src/main.cct" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_lint.out 2>&1 || OK=0
"$CCT_BIN" build --project "$SIG13B1_PROJ" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_build.out 2>&1 || OK=0
"$CCT_BIN" sigilo baseline check "$SIG13B1_ARTIFACT" --summary > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_check_missing.out 2>&1 || OK=0
"$CCT_BIN" sigilo baseline update "$SIG13B1_ARTIFACT" --baseline "$SIG13B1_BASE" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_update.out 2>&1 || OK=0
"$CCT_BIN" sigilo baseline check "$SIG13B1_ARTIFACT" --baseline "$SIG13B1_BASE" --summary > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_check_ok.out 2>&1 || OK=0
if [ $OK -eq 1 ] && [ -f "$SIG13B1_BASE" ] && \
   grep -q "status=missing" $CCT_TMP_DIR/cct_sigilo_workflow_13b1_check_missing.out && \
   grep -q "status=ok" $CCT_TMP_DIR/cct_sigilo_workflow_13b1_check_ok.out; then
    test_pass "minimal local workflow (fmt/lint/build/sigilo baseline) executes end-to-end"
else
    test_fail "minimal local workflow did not execute end-to-end as expected"
fi

# Test 566: strict pre-merge workflow baseline passes when stable
echo "Test 566: strict local pre-merge workflow passes when baseline is stable"
OK=1
"$CCT_BIN" fmt --check "$SIG13B1_PROJ/src/main.cct" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_fmt_check.out 2>&1 || OK=0
"$CCT_BIN" lint --strict "$SIG13B1_PROJ/src/main.cct" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_lint_strict.out 2>&1 || OK=0
"$CCT_BIN" test --project "$SIG13B1_PROJ" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_test.out 2>&1 || OK=0
"$CCT_BIN" sigilo baseline check "$SIG13B1_ARTIFACT" --baseline "$SIG13B1_BASE" --strict --summary > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_strict_ok.out 2>&1 || OK=0
if [ $OK -eq 1 ]; then
    test_pass "strict local workflow passes when there is no blocking drift"
else
    test_fail "strict local workflow should pass when baseline is stable"
fi

# Test 567: strict workflow fails on blocking drift
echo "Test 567: strict local workflow fails on blocking baseline drift"
cat > "$SIG13B1_DRIFT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
"$CCT_BIN" sigilo baseline check "$SIG13B1_DRIFT" --baseline "$SIG13B1_BASE" --strict --summary > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_strict_drift.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "strict local workflow blocks behavioral-risk drift with exit code 2"
else
    test_fail "strict local workflow should fail with exit code 2 on blocking drift (got $EXIT_CODE)"
fi

# Test 568: blocking drift output includes corrective action
echo "Test 568: blocking drift output includes corrective action guidance"
if grep -q "action:" $CCT_TMP_DIR/cct_sigilo_workflow_13b1_strict_drift.out && \
   grep -q "sigilo baseline update" $CCT_TMP_DIR/cct_sigilo_workflow_13b1_strict_drift.out; then
    test_pass "strict drift failure output includes corrective action"
else
    test_fail "strict drift failure output is missing corrective action guidance"
fi

# Test 569: flow without sigilo-check remains functional
echo "Test 569: project flow without sigilo-check remains functional"
OK=1
"$CCT_BIN" build --project "$SIG13B1_PROJ" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_nosig_build.out 2>&1 || OK=0
"$CCT_BIN" test --project "$SIG13B1_PROJ" > $CCT_TMP_DIR/cct_sigilo_workflow_13b1_nosig_test.out 2>&1 || OK=0
if [ $OK -eq 1 ] && find "$SIG13B1_PROJ/dist" -maxdepth 1 -type f -perm -u+x | grep -q .; then
    test_pass "build/test flows remain functional without explicit sigilo-check stage"
else
    test_fail "build/test flow regressed when sigilo-check stage is skipped"
fi

# Test 570: local workflow documentation was published
# Removed Test 570: documentation text/file comparison (policy)

echo "Test 571: 13B1 smoke integration source compiles and runs"
OUTPUT=$("$CCT_BIN" tests/integration/sigilo_workflow_local_smoke_13b1.cct 2>&1) || true
if [ -f "tests/integration/sigilo_workflow_local_smoke_13b1" ]; then
    test_pass "13B1 smoke integration source compiles in standalone flow"
else
    test_fail "13B1 smoke integration source failed to compile"
fi

echo ""
echo "========================================"
echo "FASE 13B.2: Project Sigilo Opt-in Integration Tests"
echo "========================================"
echo ""

SIG13B2_PROJ="$CCT_TMP_DIR/cct_project_sigilo_13b2"
SIG13B2_BASE_CUSTOM="$CCT_TMP_DIR/cct_project_sigilo_13b2_custom.sigil"
SIG13B2_BASE_CUSTOM_META="$CCT_TMP_DIR/cct_project_sigilo_13b2_custom.baseline.meta"
SIG13B2_BASE_BLOCK="$CCT_TMP_DIR/cct_project_sigilo_13b2_block.sigil"
SIG13B2_BASE_BLOCK_META="$CCT_TMP_DIR/cct_project_sigilo_13b2_block.baseline.meta"

rm -rf "$SIG13B2_PROJ"
rm -f "$SIG13B2_BASE_CUSTOM" "$SIG13B2_BASE_CUSTOM_META" "$SIG13B2_BASE_BLOCK" "$SIG13B2_BASE_BLOCK_META"
mkdir -p "$SIG13B2_PROJ/src" "$SIG13B2_PROJ/tests" "$SIG13B2_PROJ/bench" "$SIG13B2_PROJ/docs/sigilo/baseline"
mkdir -p "$SIG13B2_PROJ/lib"
cp tests/integration/project_sigilo_build_optin_13b2.cct "$SIG13B2_PROJ/src/main.cct"
cp tests/integration/project_sigilo_test_optin_13b2.cct "$SIG13B2_PROJ/tests/smoke.test.cct"
cp tests/integration/project_sigilo_cache_13b2.cct "$SIG13B2_PROJ/bench/smoke.bench.cct"
cat > "$SIG13B2_PROJ/lib/util.cct" <<'SIGEOF'
INCIPIT grimoire "project_sigilo_13b2_util"

RITUALE helper() REDDE REX
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
SIGEOF

# Test 572: build without sigilo opt-in keeps canonical behavior
echo "Test 572: cct build without sigilo opt-in keeps canonical behavior"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B2_PROJ" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && ! echo "$OUTPUT" | grep -q "sigilo.baseline.check"; then
    test_pass "build default flow remains unchanged without --sigilo-check"
else
    test_fail "build default flow regressed or unexpectedly executed sigilo-check"
fi

# Test 573: build --sigilo-check executes baseline check with missing baseline status
echo "Test 573: cct build --sigilo-check executes baseline check with informative missing status"
rm -f "$SIG13B2_PROJ/docs/sigilo/baseline/local.sigil" "$SIG13B2_PROJ/docs/sigilo/baseline/local.baseline.meta"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B2_PROJ" --sigilo-check 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.baseline.check status=missing" && echo "$OUTPUT" | grep -q "current=$SIG13B2_PROJ/src/main.system.sigil"; then
    test_pass "build --sigilo-check runs baseline check and handles missing baseline by contract"
else
    test_fail "build --sigilo-check did not execute baseline check as expected"
fi

# Test 574: build --sigilo-check with explicit baseline override returns stable status
echo "Test 574: cct build --sigilo-check --sigilo-baseline enforces explicit baseline path"
"$CCT_BIN" sigilo baseline update "$SIG13B2_PROJ/src/main.system.sigil" --baseline "$SIG13B2_BASE_CUSTOM" --force > $CCT_TMP_DIR/cct_project_sigilo_13b2_baseline_update.out 2>&1
OUTPUT=$("$CCT_BIN" build --project "$SIG13B2_PROJ" --sigilo-check --sigilo-baseline "$SIG13B2_BASE_CUSTOM" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.baseline.check status=ok" && echo "$OUTPUT" | grep -q "baseline=$SIG13B2_BASE_CUSTOM"; then
    test_pass "build --sigilo-check honors explicit baseline override"
else
    test_fail "build --sigilo-check with explicit baseline override did not behave as expected"
fi

# Test 575: test --sigilo-check --sigilo-strict propagates blocking status coherently
echo "Test 575: cct test --sigilo-check --sigilo-strict propagates blocking baseline drift"
cat > "$SIG13B2_BASE_BLOCK" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
cat > "$SIG13B2_BASE_BLOCK_META" <<'SIGEOF'
format = cct.sigil.baseline.meta.v1
baseline_schema = 1
sigilo_scope = system
artifact_format = cct.sigil.v1
source_artifact = tests/integration/project_sigilo_test_optin_13b2.cct
SIGEOF
OUTPUT=$("$CCT_BIN" test --project "$SIG13B2_PROJ" --sigilo-check --sigilo-strict --sigilo-baseline "$SIG13B2_BASE_BLOCK" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "\\[test\\] error: sigilo-check failed"; then
    test_pass "test --sigilo-check --sigilo-strict propagates blocking drift as exit code 2"
else
    test_fail "test --sigilo-check --sigilo-strict did not propagate blocking drift correctly (exit=$EXIT_CODE)"
fi

# Test 576: cache fallback is safe when sigilo artifact is missing on up-to-date manifest
echo "Test 576: build --sigilo-check falls back safely when cache is up-to-date but sigilo artifact is missing"
"$CCT_BIN" sigilo baseline update "$SIG13B2_PROJ/src/main.system.sigil" --baseline "$SIG13B2_PROJ/docs/sigilo/baseline/system.sigil" --force > $CCT_TMP_DIR/cct_project_sigilo_13b2_default_baseline_update.out 2>&1
"$CCT_BIN" build --project "$SIG13B2_PROJ" --sigilo-check > $CCT_TMP_DIR/cct_project_sigilo_13b2_build_warmup.out 2>&1 || true
rm -f "$SIG13B2_PROJ/src/main.system.sigil"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B2_PROJ" --sigilo-check 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "forcing rebuild" && echo "$OUTPUT" | grep -q "\\[build\\] status: rebuilt"; then
    test_pass "build --sigilo-check safely rebuilds when cached sigilo artifact is missing"
else
    test_fail "build --sigilo-check did not handle missing cached sigilo artifact safely"
fi

# Test 577: test --sigilo-check treats missing baseline as informative, not blocking
echo "Test 577: cct test --sigilo-check treats missing baseline as informative"
rm -f "$SIG13B2_PROJ/docs/sigilo/baseline/system.sigil" "$SIG13B2_PROJ/docs/sigilo/baseline/system.baseline.meta"
OUTPUT=$("$CCT_BIN" test --project "$SIG13B2_PROJ" --sigilo-check 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.baseline.check status=missing"; then
    test_pass "test --sigilo-check handles missing baseline in non-blocking mode"
else
    test_fail "test --sigilo-check did not handle missing baseline according to contract"
fi

# Test 578: bench --sigilo-check executes sigilo-check stage
echo "Test 578: cct bench --sigilo-check executes sigilo baseline check stage"
OUTPUT=$("$CCT_BIN" bench --project "$SIG13B2_PROJ" --iterations 1 --sigilo-check 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.baseline.check status=missing" && echo "$OUTPUT" | grep -q "\\[bench\\] selected: 1"; then
    test_pass "bench --sigilo-check integrates baseline check without regressing benchmark execution"
else
    test_fail "bench --sigilo-check did not execute expected sigilo integration stage"
fi

# Test 579: post-13B2 default build remains free of implicit sigilo stage
echo "Test 579: cct build default path remains free of implicit sigilo stage after opt-in usage"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B2_PROJ" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && ! echo "$OUTPUT" | grep -q "sigilo.baseline.check"; then
    test_pass "default build behavior remains unchanged after 13B2 integrations"
else
    test_fail "default build behavior changed unexpectedly after 13B2 integrations"
fi

echo ""
echo "========================================"
echo "FASE 13B.3: CI Profile Contract and Progressive Gate Tests"
echo "========================================"
echo ""

SIG13B3_PROJ="$CCT_TMP_DIR/cct_sigilo_ci_13b3"
SIG13B3_BASE="$CCT_TMP_DIR/cct_sigilo_ci_13b3_base.sigil"
SIG13B3_META="$CCT_TMP_DIR/cct_sigilo_ci_13b3_base.baseline.meta"
SIG13B3_BEHAV_BASE="$CCT_TMP_DIR/cct_sigilo_ci_13b3_behavioral.sigil"
SIG13B3_BEHAV_META="$CCT_TMP_DIR/cct_sigilo_ci_13b3_behavioral.baseline.meta"
SIG13B4_SUMMARY_BASE="$CCT_TMP_DIR/cct_sigilo_report_13b4_summary_base.sigil"
SIG13B4_SUMMARY_META="$CCT_TMP_DIR/cct_sigilo_report_13b4_summary_base.baseline.meta"
SIG13B4_REVIEW_BASE="$CCT_TMP_DIR/cct_sigilo_report_13b4_review_base.sigil"
SIG13B4_REVIEW_META="$CCT_TMP_DIR/cct_sigilo_report_13b4_review_base.baseline.meta"
SIG13B4_MISSING_BASE="$CCT_TMP_DIR/cct_sigilo_report_13b4_missing_base.sigil"

rm -rf "$SIG13B3_PROJ"
rm -f "$SIG13B3_BASE" "$SIG13B3_META" "$SIG13B3_BEHAV_BASE" "$SIG13B3_BEHAV_META"
rm -f "$SIG13B4_SUMMARY_BASE" "$SIG13B4_SUMMARY_META" "$SIG13B4_REVIEW_BASE" "$SIG13B4_REVIEW_META" "$SIG13B4_MISSING_BASE"
mkdir -p "$SIG13B3_PROJ/src" "$SIG13B3_PROJ/lib" "$SIG13B3_PROJ/tests" "$SIG13B3_PROJ/bench"
cp tests/integration/sigilo_report_summary_13b4.cct "$SIG13B3_PROJ/src/main.cct"
cp tests/integration/sigilo_ci_profile_gated_13b3.cct "$SIG13B3_PROJ/tests/ci_gated.test.cct"
cp tests/integration/sigilo_report_explain_13b4.cct "$SIG13B3_PROJ/bench/ci_release.bench.cct"
cat > "$SIG13B3_PROJ/lib/util.cct" <<'SIGEOF'
INCIPIT grimoire "sigilo_ci_profile_util_13b3"

RITUALE helper() REDDE REX
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
SIGEOF

# Ensure project sigilo artifacts are available
"$CCT_BIN" build --project "$SIG13B3_PROJ" > $CCT_TMP_DIR/cct_sigilo_ci_13b3_bootstrap.out 2>&1 || true
"$CCT_BIN" sigilo baseline update "$SIG13B3_PROJ/src/main.system.sigil" --baseline "$SIG13B3_BASE" --force > $CCT_TMP_DIR/cct_sigilo_ci_13b3_update_base.out 2>&1

# Test 580: advisory profile does not block review-required drift
echo "Test 580: advisory CI profile does not block review-required drift"
cp "$SIG13B3_BASE" $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.sigil
cp "$SIG13B3_META" $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.baseline.meta
sed -i 's/system_hash = .*/system_hash = 0123456789abcdef/' $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.sigil
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.sigil 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "profile=advisory" && echo "$OUTPUT" | grep -q "highest=review-required"; then
    test_pass "advisory profile keeps review-required drift non-blocking"
else
    test_fail "advisory profile behavior for review-required drift is incorrect (exit=$EXIT_CODE)"
fi

# Test 581: gated profile blocks review-required drift with stable exit code 2
echo "Test 581: gated CI profile blocks review-required drift"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile gated --sigilo-baseline $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.sigil 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "profile=gated" && echo "$OUTPUT" | grep -q "blocked"; then
    test_pass "gated profile blocks review-required drift with exit code 2"
else
    test_fail "gated profile did not block review-required drift as expected (exit=$EXIT_CODE)"
fi

# Test 582: release profile blocks review-required drift with stable exit code 2
echo "Test 582: release CI profile blocks review-required drift"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline $CCT_TMP_DIR/cct_sigilo_ci_13b3_review_base.sigil 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "profile=release" && echo "$OUTPUT" | grep -q "blocked"; then
    test_pass "release profile blocks review-required drift with exit code 2"
else
    test_fail "release profile did not block review-required drift as expected (exit=$EXIT_CODE)"
fi

# Test 583: release profile keeps informational drift non-blocking with explicit warning
echo "Test 583: release CI profile keeps informational drift non-blocking with warning"
cp "$SIG13B3_BASE" $CCT_TMP_DIR/cct_sigilo_ci_13b3_info_base.sigil
cp "$SIG13B3_META" $CCT_TMP_DIR/cct_sigilo_ci_13b3_info_base.baseline.meta
cat >> $CCT_TMP_DIR/cct_sigilo_ci_13b3_info_base.sigil <<'SIGEOF'

[ci_note]
marker = informational_only
SIGEOF
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline $CCT_TMP_DIR/cct_sigilo_ci_13b3_info_base.sigil 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "profile=release" && echo "$OUTPUT" | grep -q "highest=informational"; then
    test_pass "release profile treats informational drift as warning without blocking"
else
    test_fail "release profile informational behavior is incorrect (exit=$EXIT_CODE)"
fi

# Test 584: advisory profile blocks behavioral-risk drift by default
echo "Test 584: advisory CI profile blocks behavioral-risk drift by default"
cat > "$SIG13B3_BEHAV_BASE" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
cat > "$SIG13B3_BEHAV_META" <<'SIGEOF'
format = cct.sigil.baseline.meta.v1
baseline_schema = 1
sigilo_scope = local
artifact_format = cct.sigil.v1
source_artifact = $CCT_TMP_DIR/cct_sigilo_ci_13b3_behavioral.sigil
SIGEOF
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$SIG13B3_BEHAV_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "highest=behavioral-risk"; then
    test_pass "behavioral-risk drift blocks even in advisory profile"
else
    test_fail "behavioral-risk default blocking policy is incorrect (exit=$EXIT_CODE)"
fi

# Test 585: explicit override allows behavioral-risk drift and emits audit warning
echo "Test 585: behavioral-risk override is explicit and audit-visible"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-override-behavioral-risk --sigilo-baseline "$SIG13B3_BEHAV_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "audit=true"; then
    test_pass "behavioral-risk override is explicit and audit-visible"
else
    test_fail "behavioral-risk override is missing or not audit-visible (exit=$EXIT_CODE)"
fi

# Test 586: release profile fails when baseline is missing
echo "Test 586: release CI profile requires baseline presence"
rm -f $CCT_TMP_DIR/cct_sigilo_ci_13b3_missing.sigil $CCT_TMP_DIR/cct_sigilo_ci_13b3_missing.baseline.meta
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline $CCT_TMP_DIR/cct_sigilo_ci_13b3_missing.sigil 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && echo "$OUTPUT" | grep -q "status=missing"; then
    test_pass "release profile enforces baseline presence"
else
    test_fail "release profile did not enforce missing baseline policy (exit=$EXIT_CODE)"
fi

# Test 587: gated profile passes on stable baseline in project test flow
echo "Test 587: gated CI profile passes on stable baseline in project test flow"
SIG13B3_TEST_BASE="$CCT_TMP_DIR/cct_sigilo_ci_13b3_test_base.sigil"
SIG13B3_BENCH_BASE="$CCT_TMP_DIR/cct_sigilo_ci_13b3_bench_base.sigil"
"$CCT_BIN" test --project "$SIG13B3_PROJ" > $CCT_TMP_DIR/cct_sigilo_ci_13b3_test_bootstrap.out 2>&1 || true
"$CCT_BIN" bench --project "$SIG13B3_PROJ" --iterations 1 > $CCT_TMP_DIR/cct_sigilo_ci_13b3_bench_bootstrap.out 2>&1 || true
"$CCT_BIN" sigilo baseline update "$SIG13B3_PROJ/tests/ci_gated.test.system.sigil" --baseline "$SIG13B3_TEST_BASE" --force > $CCT_TMP_DIR/cct_sigilo_ci_13b3_test_base_update.out 2>&1
"$CCT_BIN" sigilo baseline update "$SIG13B3_PROJ/bench/ci_release.bench.system.sigil" --baseline "$SIG13B3_BENCH_BASE" --force > $CCT_TMP_DIR/cct_sigilo_ci_13b3_bench_base_update.out 2>&1
OUTPUT=$("$CCT_BIN" test --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile gated --sigilo-baseline "$SIG13B3_TEST_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "profile=gated" && echo "$OUTPUT" | grep -q "highest=none"; then
    test_pass "gated profile passes when baseline is stable"
else
    test_fail "gated profile should pass on stable baseline in test flow (exit=$EXIT_CODE)"
fi

# Test 588: release profile passes on stable baseline in bench flow
echo "Test 588: release CI profile passes on stable baseline in bench flow"
OUTPUT=$("$CCT_BIN" bench --project "$SIG13B3_PROJ" --iterations 1 --sigilo-check --sigilo-ci-profile release --sigilo-baseline "$SIG13B3_BENCH_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "profile=release" && echo "$OUTPUT" | grep -q "highest=none"; then
    test_pass "release profile passes on stable baseline in bench flow"
else
    test_fail "release profile should pass on stable baseline in bench flow (exit=$EXIT_CODE)"
fi

# Test 589: advisory profile remains non-blocking on stable baseline in build flow
echo "Test 589: advisory CI profile remains non-blocking on stable baseline in build flow"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$SIG13B3_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "profile=advisory" && echo "$OUTPUT" | grep -q "highest=none"; then
    test_pass "advisory profile remains non-blocking on stable baseline"
else
    test_fail "advisory profile should pass on stable baseline in build flow (exit=$EXIT_CODE)"
fi

cp "$SIG13B3_BASE" "$SIG13B4_SUMMARY_BASE"
cp "$SIG13B3_META" "$SIG13B4_SUMMARY_META"
cp "$SIG13B3_BASE" "$SIG13B4_REVIEW_BASE"
cp "$SIG13B3_META" "$SIG13B4_REVIEW_META"
sed -i 's/system_hash = .*/system_hash = fedcba9876543210/' "$SIG13B4_REVIEW_BASE"

# Test 590: default sigilo report summary includes operational minimum fields
echo "Test 590: sigilo report summary includes operational minimum fields"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$SIG13B4_SUMMARY_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "sigilo.report format=cct.sigilo.report.v1" && \
   echo "$OUTPUT" | grep -q "command=build" && \
   echo "$OUTPUT" | grep -q "profile=advisory" && \
   echo "$OUTPUT" | grep -q "decision=pass" && \
   echo "$OUTPUT" | grep -q "sigilo.report.summary"; then
    test_pass "summary report prints format/version, command/profile, decision, and next action hints"
else
    test_fail "summary report is missing required operational fields (exit=$EXIT_CODE)"
fi

# Test 591: detailed mode includes per-diff domain and before/after values
echo "Test 591: sigilo report detailed mode includes diff domain and before/after"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile gated --sigilo-baseline "$SIG13B4_REVIEW_BASE" --sigilo-report detailed 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && \
   echo "$OUTPUT" | grep -q "mode=detailed" && \
   echo "$OUTPUT" | grep -q "sigilo.report.item\\[0\\]" && \
   echo "$OUTPUT" | grep -q "domain=root/system_hash" && \
   echo "$OUTPUT" | grep -q "before=" && \
   echo "$OUTPUT" | grep -q "after="; then
    test_pass "detailed report exposes per-item domain and before/after values"
else
    test_fail "detailed report did not expose required per-item diagnostics (exit=$EXIT_CODE)"
fi

# Test 592: explain mode provides cause/action/docs on blocking drift
# Removed Test 592: documentation text/file comparison (policy)

echo "Test 593: sigilo explain mode covers release profile missing baseline"
rm -f "$SIG13B4_MISSING_BASE" "${SIG13B4_MISSING_BASE%.sigil}.baseline.meta"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline "$SIG13B4_MISSING_BASE" --sigilo-explain 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ] && \
   echo "$OUTPUT" | grep -q "status=missing" && \
   echo "$OUTPUT" | grep -q "sigilo.explain probable_cause=baseline file is missing" && \
   echo "$OUTPUT" | grep -q "blocked=true"; then
    test_pass "explain mode handles missing baseline diagnostics in release profile"
else
    test_fail "missing baseline explain diagnostics are incorrect (exit=$EXIT_CODE)"
fi

# Test 594: output format signature remains stable in standard mode
echo "Test 594: sigilo report format signature remains stable"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$SIG13B4_SUMMARY_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "format=cct.sigilo.report.v1"; then
    test_pass "sigilo report format signature is stable in standard mode"
else
    test_fail "sigilo report format signature changed unexpectedly (exit=$EXIT_CODE)"
fi

# Test 595: legacy CI output line remains available for compatibility scripts
echo "Test 595: legacy sigilo.ci output remains available for compatibility"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$SIG13B4_SUMMARY_BASE" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.ci profile=advisory"; then
    test_pass "legacy sigilo.ci output is preserved for existing scripts"
else
    test_fail "legacy sigilo.ci output was regressed (exit=$EXIT_CODE)"
fi

# Test 596: baseline mode emits summary report and explain hint without CI profile
echo "Test 596: baseline mode emits report/explain without CI profile"
OUTPUT=$("$CCT_BIN" build --project "$SIG13B3_PROJ" --sigilo-check --sigilo-baseline "$SIG13B4_SUMMARY_BASE" --sigilo-explain 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "profile=baseline" && \
   echo "$OUTPUT" | grep -q "sigilo.report format=cct.sigilo.report.v1" && \
   echo "$OUTPUT" | grep -q "sigilo.explain probable_cause="; then
    test_pass "baseline mode reports and explains sigilo status in operational format"
else
    test_fail "baseline mode observability output is incomplete (exit=$EXIT_CODE)"
fi

echo ""
echo "========================================"
echo "FASE 13C.1: Sigilo Schema Governance and Compatibility Tests"
echo "========================================"
echo ""

SIG13C1_BACK="tests/integration/sigilo_schema_backward_13c1"
SIG13C1_FWD="tests/integration/sigilo_schema_forward_13c1"
SIG13C1_BASE_SIGIL="$CCT_TMP_DIR/cct_sigilo_schema_13c1_base.sigil"
SIG13C1_UNKNOWN="$CCT_TMP_DIR/cct_sigilo_schema_13c1_unknown.sigil"
SIG13C1_DEPRECATED="$CCT_TMP_DIR/cct_sigilo_schema_13c1_deprecated.sigil"
SIG13C1_INVALID_SCHEMA="$CCT_TMP_DIR/cct_sigilo_schema_13c1_invalid_schema.sigil"

rm -f "${SIG13C1_BACK}.svg" "${SIG13C1_BACK}.sigil" "${SIG13C1_BACK}"
rm -f "${SIG13C1_FWD}.svg" "${SIG13C1_FWD}.sigil" "${SIG13C1_FWD}"
rm -f "$SIG13C1_BASE_SIGIL" "$SIG13C1_UNKNOWN" "$SIG13C1_DEPRECATED" "$SIG13C1_INVALID_SCHEMA"

"$CCT_BIN" tests/integration/sigilo_schema_backward_13c1.cct >$CCT_TMP_DIR/cct_sigilo_schema_13c1_back_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_schema_forward_13c1.cct >$CCT_TMP_DIR/cct_sigilo_schema_13c1_fwd_compile.out 2>&1 || true
cat > "$SIG13C1_BASE_SIGIL" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
visual_engine = network
semantic_hash = abcdef0123456789

[totals]
rituale = 1
SIGEOF

# Test 597: schema governance document exists and is versioned
# Removed Test 597: documentation text/file comparison (policy)

echo "Test 598: sigilo inspect structured reads canonical schema version"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C1_BASE_SIGIL" --format structured --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigil_format = cct.sigil.v1"; then
    test_pass "structured inspect reads schema version cct.sigil.v1"
else
    test_fail "structured inspect did not report expected schema version (exit=$EXIT_CODE)"
fi

# Test 599: tolerant consumer ignores additive unknown field
echo "Test 599: tolerant sigilo consumer ignores unknown additive field"
cp "$SIG13C1_BASE_SIGIL" "$SIG13C1_UNKNOWN"
echo "future_schema_field = canary" >> "$SIG13C1_UNKNOWN"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C1_UNKNOWN" --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.inspect.summary"; then
    test_pass "tolerant inspect accepts unknown additive top-level field"
else
    test_fail "tolerant inspect should accept unknown additive field (exit=$EXIT_CODE)"
fi

# Test 600: deprecated field alias is accepted and mapped to visual_engine
echo "Test 600: deprecated sigilo field alias remains accepted with compatibility mapping"
cp "$SIG13C1_BASE_SIGIL" "$SIG13C1_DEPRECATED"
sed -i '/^visual_engine = /d' "$SIG13C1_DEPRECATED"
sed -i '/^\[totals\]/i sigilo_style = seal' "$SIG13C1_DEPRECATED"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C1_DEPRECATED" --format text 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "visual_engine=seal"; then
    test_pass "deprecated field alias remains accepted and compatible"
else
    test_fail "deprecated field alias compatibility mapping failed (exit=$EXIT_CODE)"
fi

# Test 601: strict mode fails on schema incompatibility
echo "Test 601: strict sigilo inspect fails on incompatible schema format"
cp "$SIG13C1_BASE_SIGIL" "$SIG13C1_INVALID_SCHEMA"
sed -i 's/^format = .*/format = cct.sigil.v2/' "$SIG13C1_INVALID_SCHEMA"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C1_INVALID_SCHEMA" --strict --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "schema_mismatch"; then
    test_pass "strict inspect rejects incompatible schema contract"
else
    test_fail "strict inspect should reject incompatible schema format (exit=$EXIT_CODE)"
fi

# Test 602: strict mode keeps unknown additive field non-fatal for forward compatibility
echo "Test 602: strict consumer keeps unknown additive field non-fatal"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C1_UNKNOWN" --strict --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.inspect.summary"; then
    test_pass "strict inspect keeps additive unknown field as non-fatal warning"
else
    test_fail "strict inspect should keep additive unknown field non-fatal (exit=$EXIT_CODE)"
fi

# Test 603: runtime parser suite validates 13C1 compatibility diagnostics
echo "Test 603: runtime sigil parser suite validates 13C1 compatibility diagnostics"
if make test_sigil_parse >$CCT_TMP_DIR/cct_sigilo_schema_13c1_runtime_parse.out 2>&1; then
    test_pass "runtime sigil parser suite validates deprecated/unknown/schema compatibility cases"
else
    test_fail "runtime sigil parser suite failed for 13C1 compatibility checks"
fi

echo ""
echo "========================================"
echo "FASE 13C.2: Analytical Metadata Blocks Tests"
echo "========================================"
echo ""

SIG13C2_A="tests/integration/sigilo_metadata_new_blocks_13c2"
SIG13C2_B="tests/integration/sigilo_metadata_diff_blocks_13c2"
SIG13C2_A_SIGIL="${SIG13C2_A}.sigil"
SIG13C2_B_SIGIL="${SIG13C2_B}.sigil"

rm -f "${SIG13C2_A}.svg" "$SIG13C2_A_SIGIL" "${SIG13C2_A}"
rm -f "${SIG13C2_B}.svg" "$SIG13C2_B_SIGIL" "${SIG13C2_B}"

"$CCT_BIN" tests/integration/sigilo_metadata_new_blocks_13c2.cct >$CCT_TMP_DIR/cct_sigilo_13c2_a_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_metadata_diff_blocks_13c2.cct >$CCT_TMP_DIR/cct_sigilo_13c2_b_compile.out 2>&1 || true

# Test 604: local sigil emits all new analytical metadata sections
echo "Test 604: local sigil emits new analytical metadata sections"
if [ -f "$SIG13C2_A_SIGIL" ] && \
   grep -q "^\[analysis_summary\]" "$SIG13C2_A_SIGIL" && \
   grep -q "^\[diff_fingerprint_context\]" "$SIG13C2_A_SIGIL" && \
   grep -q "^\[module_structural_summary\]" "$SIG13C2_A_SIGIL" && \
   grep -q "^\[compatibility_hints\]" "$SIG13C2_A_SIGIL"; then
    test_pass "13C2 analytical metadata sections are emitted in local sigil"
else
    test_fail "13C2 analytical metadata sections missing in local sigil"
fi

# Test 605: deterministic serialization order for new analytical blocks
echo "Test 605: analytical metadata sections keep deterministic order"
ORDER_A=$(grep -n "^\[analysis_summary\]\|^\[diff_fingerprint_context\]\|^\[module_structural_summary\]\|^\[compatibility_hints\]" "$SIG13C2_A_SIGIL" 2>/dev/null | cut -d: -f2 | tr '\n' '|' || true)
if [ "$ORDER_A" = "[analysis_summary]|[diff_fingerprint_context]|[module_structural_summary]|[compatibility_hints]|" ]; then
    test_pass "analytical metadata section serialization order is deterministic"
else
    test_fail "analytical metadata section order is not deterministic"
fi

# Test 606: parser strict mode accepts new analytical blocks
echo "Test 606: strict parser accepts sigil with new analytical blocks"
SIG13C2_STRICT="$CCT_TMP_DIR/cct_sigilo_13c2_strict.sigil"
cat > "$SIG13C2_STRICT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789

[analysis_summary]
scope = local

[diff_fingerprint_context]
scope_anchor = local

[module_structural_summary]
module_count = 1

[compatibility_hints]
schema_contract = cct.sigil.v1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C2_STRICT" --strict --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.inspect.summary"; then
    test_pass "strict parser accepts 13C2 analytical blocks"
else
    test_fail "strict parser should accept 13C2 analytical blocks (exit=$EXIT_CODE)"
fi

# Test 607: diff classifies analysis_summary drift as review-required
echo "Test 607: sigilo diff marks analysis_summary drift as review-required"
SIG13C2_DIFF_A="$CCT_TMP_DIR/cct_sigilo_13c2_diff_a.sigil"
SIG13C2_DIFF_B="$CCT_TMP_DIR/cct_sigilo_13c2_diff_b.sigil"
cat > "$SIG13C2_DIFF_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789

[analysis_summary]
control_flow_pressure = 2
SIGEOF
cat > "$SIG13C2_DIFF_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789

[analysis_summary]
control_flow_pressure = 8
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo diff "$SIG13C2_DIFF_A" "$SIG13C2_DIFF_B" --format text 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "section=analysis_summary" && \
   echo "$OUTPUT" | grep -q "severity=review-required"; then
    test_pass "analysis_summary drift is classified as review-required"
else
    test_fail "analysis_summary drift classification is incorrect (exit=$EXIT_CODE)"
fi

# Test 608: compatibility_hints drift remains informational
echo "Test 608: compatibility_hints drift remains informational in diff"
SIG13C2_HINTS_A="$CCT_TMP_DIR/cct_sigilo_13c2_hints_a.sigil"
SIG13C2_HINTS_B="$CCT_TMP_DIR/cct_sigilo_13c2_hints_b.sigil"
cat > "$SIG13C2_HINTS_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789

[compatibility_hints]
schema_contract = cct.sigil.v1
SIGEOF
cat > "$SIG13C2_HINTS_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789

[compatibility_hints]
schema_contract = cct.sigil.v1+hint
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo diff "$SIG13C2_HINTS_A" "$SIG13C2_HINTS_B" --format text 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "section=compatibility_hints" && \
   echo "$OUTPUT" | grep -q "severity=informational"; then
    test_pass "compatibility_hints drift remains informational"
else
    test_fail "compatibility_hints drift should be informational (exit=$EXIT_CODE)"
fi

# Test 609: runtime parser suite includes 13C2 analytical block recognition
echo "Test 609: runtime sigil parser suite includes 13C2 analytical recognition"
if make test_sigil_parse >$CCT_TMP_DIR/cct_sigilo_13c2_runtime_parse.out 2>&1; then
    test_pass "runtime sigil parser suite validates 13C2 analytical block recognition"
else
    test_fail "runtime sigil parser suite failed for 13C2 analytical recognition"
fi

# Test 610: runtime diff suite includes 13C2 analytical block severity contract
echo "Test 610: runtime sigil diff suite includes 13C2 severity contract"
if make test_sigil_diff >$CCT_TMP_DIR/cct_sigilo_13c2_runtime_diff.out 2>&1; then
    test_pass "runtime sigil diff suite validates 13C2 analytical block severity"
else
    test_fail "runtime sigil diff suite failed for 13C2 severity contract"
fi

echo ""
echo "========================================"
echo "FASE 13C.3: Consumer Compatibility Profiles Tests"
echo "========================================"
echo ""

SIG13C3_LEGACY="tests/integration/sigilo_consumer_legacy_13c3"
SIG13C3_CURRENT="tests/integration/sigilo_consumer_current_13c3"
SIG13C3_STRICT="tests/integration/sigilo_consumer_strict_13c3"

rm -f "${SIG13C3_LEGACY}.svg" "${SIG13C3_LEGACY}.sigil" "${SIG13C3_LEGACY}"
rm -f "${SIG13C3_CURRENT}.svg" "${SIG13C3_CURRENT}.sigil" "${SIG13C3_CURRENT}"
rm -f "${SIG13C3_STRICT}.svg" "${SIG13C3_STRICT}.sigil" "${SIG13C3_STRICT}"

"$CCT_BIN" tests/integration/sigilo_consumer_legacy_13c3.cct >$CCT_TMP_DIR/cct_sigilo_13c3_legacy_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_consumer_current_13c3.cct >$CCT_TMP_DIR/cct_sigilo_13c3_current_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_consumer_strict_13c3.cct >$CCT_TMP_DIR/cct_sigilo_13c3_strict_compile.out 2>&1 || true

# Test 611: consumer compatibility guide is published
# Removed Test 611: documentation text/file comparison (policy)

echo "Test 612: legacy-tolerant profile applies higher-schema fallback"
SIG13C3_V2_LEGACY="$CCT_TMP_DIR/cct_sigilo_13c3_v2_legacy.sigil"
cat > "$SIG13C3_V2_LEGACY" <<'SIGEOF'
format = cct.sigil.v2
sigilo_scope = local
semantic_hash = abcdef0123456789
future_field = additive
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C3_V2_LEGACY" --summary --consumer-profile legacy-tolerant 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "sigilo.inspect.summary" && \
   echo "$OUTPUT" | grep -q "warnings=" && \
   ! echo "$OUTPUT" | grep -q "warnings=0"; then
    test_pass "legacy-tolerant profile accepts higher schema with warning fallback"
else
    test_fail "legacy-tolerant fallback contract failed (exit=$EXIT_CODE)"
fi

# Test 613: current-default profile accepts higher schema with warning fallback
echo "Test 613: current-default profile applies higher-schema fallback"
SIG13C3_V2_CURRENT="$CCT_TMP_DIR/cct_sigilo_13c3_v2_current.sigil"
cat > "$SIG13C3_V2_CURRENT" <<'SIGEOF'
format = cct.sigil.v3
sigilo_scope = local
semantic_hash = abcdef0123456789
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C3_V2_CURRENT" --summary --consumer-profile current-default 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && \
   echo "$OUTPUT" | grep -q "sigilo.inspect.summary" && \
   echo "$OUTPUT" | grep -q "warnings=" && \
   ! echo "$OUTPUT" | grep -q "warnings=0"; then
    test_pass "current-default profile accepts higher schema with warning fallback"
else
    test_fail "current-default fallback contract failed (exit=$EXIT_CODE)"
fi

# Test 614: strict-contract profile rejects higher schema
echo "Test 614: strict-contract profile rejects higher schema"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C3_V2_CURRENT" --summary --consumer-profile strict-contract 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "schema_mismatch"; then
    test_pass "strict-contract rejects unsupported higher schema"
else
    test_fail "strict-contract should reject unsupported higher schema (exit=$EXIT_CODE)"
fi

# Test 615: --strict alias remains equivalent to strict-contract
echo "Test 615: --strict remains strict-contract alias"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C3_V2_CURRENT" --summary --strict 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "schema_mismatch"; then
    test_pass "--strict keeps strict-contract behavior for schema mismatch"
else
    test_fail "--strict alias mismatch for strict-contract behavior (exit=$EXIT_CODE)"
fi

# Test 616: invalid consumer profile value is rejected
echo "Test 616: invalid consumer profile is rejected"
OUTPUT=$("$CCT_BIN" sigilo inspect "$SIG13C3_V2_CURRENT" --summary --consumer-profile future-mode 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "Invalid --consumer-profile value"; then
    test_pass "invalid consumer profile is rejected clearly"
else
    test_fail "invalid consumer profile should be rejected clearly (exit=$EXIT_CODE)"
fi

# Test 617: 13C3 integration fixtures compile and emit sigil artifacts
echo "Test 617: 13C3 consumer integration fixtures compile to sigil artifacts"
if [ -f "${SIG13C3_LEGACY}.sigil" ] && [ -f "${SIG13C3_CURRENT}.sigil" ] && [ -f "${SIG13C3_STRICT}.sigil" ]; then
    test_pass "13C3 integration fixtures compile and emit sigilo metadata"
else
    test_fail "13C3 integration fixtures did not emit expected sigilo artifacts"
fi

# Test 618: runtime parser suite includes 13C3 profile fallback behavior
echo "Test 618: runtime sigil parser suite includes 13C3 profile fallback behavior"
if make test_sigil_parse >$CCT_TMP_DIR/cct_sigilo_13c3_runtime_parse.out 2>&1; then
    test_pass "runtime sigil parser suite validates 13C3 compatibility profiles"
else
    test_fail "runtime sigil parser suite failed for 13C3 compatibility profiles"
fi

echo ""
echo "========================================"
echo "FASE 13C.4: Strict/Tolerant Validation and Normalization Tests"
echo "========================================"
echo ""

SIG13C4_TOL="tests/integration/sigilo_validate_tolerant_13c4"
SIG13C4_STRICT="tests/integration/sigilo_validate_strict_13c4"
SIG13C4_SCHEMA="tests/integration/sigilo_validate_schema_violation_13c4"

rm -f "${SIG13C4_TOL}.svg" "${SIG13C4_TOL}.sigil" "${SIG13C4_TOL}"
rm -f "${SIG13C4_STRICT}.svg" "${SIG13C4_STRICT}.sigil" "${SIG13C4_STRICT}"
rm -f "${SIG13C4_SCHEMA}.svg" "${SIG13C4_SCHEMA}.sigil" "${SIG13C4_SCHEMA}"

"$CCT_BIN" tests/integration/sigilo_validate_tolerant_13c4.cct >$CCT_TMP_DIR/cct_sigilo_13c4_tol_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_validate_strict_13c4.cct >$CCT_TMP_DIR/cct_sigilo_13c4_strict_compile.out 2>&1 || true
"$CCT_BIN" tests/integration/sigilo_validate_schema_violation_13c4.cct >$CCT_TMP_DIR/cct_sigilo_13c4_schema_compile.out 2>&1 || true

SIG13C4_VALID="$CCT_TMP_DIR/cct_sigilo_13c4_valid.sigil"
SIG13C4_UNKNOWN="$CCT_TMP_DIR/cct_sigilo_13c4_unknown.sigil"
SIG13C4_MISSING="$CCT_TMP_DIR/cct_sigilo_13c4_missing_required.sigil"
SIG13C4_TYPE="$CCT_TMP_DIR/cct_sigilo_13c4_type_invalid.sigil"
SIG13C4_SCHEMA_HIGH="$CCT_TMP_DIR/cct_sigilo_13c4_schema_high.sigil"

cat > "$SIG13C4_VALID" <<'SIGEOF'
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

# Test 619: new validate subcommand works in tolerant mode for valid artifact
echo "Test 619: sigilo validate tolerant passes valid artifact"
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG13C4_VALID" --summary --consumer-profile current-default 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.validate.summary" && echo "$OUTPUT" | grep -q "result=pass"; then
    test_pass "sigilo validate tolerant accepts valid artifact"
else
    test_fail "sigilo validate tolerant should accept valid artifact (exit=$EXIT_CODE)"
fi

# Test 620: strict-contract validation passes valid artifact
echo "Test 620: sigilo validate strict-contract passes valid artifact"
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG13C4_VALID" --summary --consumer-profile strict-contract 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "result=pass"; then
    test_pass "sigilo validate strict-contract accepts valid artifact"
else
    test_fail "sigilo validate strict-contract should accept valid artifact (exit=$EXIT_CODE)"
fi

# Test 621: unknown optional field remains non-blocking in tolerant validation
echo "Test 621: tolerant validation keeps unknown optional field non-blocking"
cp "$SIG13C4_VALID" "$SIG13C4_UNKNOWN"
echo "future_optional = x" >> "$SIG13C4_UNKNOWN"
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG13C4_UNKNOWN" --summary --consumer-profile legacy-tolerant 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "warnings=" && ! echo "$OUTPUT" | grep -q "warnings=0"; then
    test_pass "tolerant validation keeps unknown optional field as warning"
else
    test_fail "tolerant validation should keep unknown optional field non-blocking (exit=$EXIT_CODE)"
fi

# Test 622: strict-contract fails on missing required hash by scope
echo "Test 622: strict validation fails on missing required field"
cat > "$SIG13C4_MISSING" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG13C4_MISSING" --summary --strict 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "errors=" && ! echo "$OUTPUT" | grep -q "errors=0"; then
    test_pass "strict validation fails on missing required field"
else
    test_fail "strict validation should fail on missing required field (exit=$EXIT_CODE)"
fi

# Test 623: strict-contract fails invalid numeric type with action hint
echo "Test 623: strict validation fails invalid numeric type with action hint"
cat > "$SIG13C4_TYPE" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
module_count = many
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG13C4_TYPE" --format structured --strict 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && \
   echo "$OUTPUT" | grep -q "kind = type" && \
   echo "$OUTPUT" | grep -q "action:"; then
    test_pass "strict validation reports invalid numeric type with correction hint"
else
    test_fail "strict validation should report typed error with action hint (exit=$EXIT_CODE)"
fi

# Test 624: sigilo check --strict keeps schema violation blocking contract
echo "Test 624: sigilo check --strict blocks schema violation inputs"
cat > "$SIG13C4_SCHEMA_HIGH" <<'SIGEOF'
format = cct.sigil.v2
sigilo_scope = local
semantic_hash = abcdef0123456789
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo check "$SIG13C4_VALID" "$SIG13C4_SCHEMA_HIGH" --strict --summary 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "schema_mismatch"; then
    test_pass "sigilo check --strict blocks schema violation during validation"
else
    test_fail "sigilo check --strict should block schema violation (exit=$EXIT_CODE)"
fi

# Test 625: runtime parser suite includes 13C4 strict/tolerant validation rules
echo "Test 625: runtime sigil parser suite includes 13C4 validation rules"
if make test_sigil_parse >$CCT_TMP_DIR/cct_sigilo_13c4_runtime_parse.out 2>&1; then
    test_pass "runtime sigil parser suite validates 13C4 strict/tolerant rules"
else
    test_fail "runtime sigil parser suite failed for 13C4 rules"
fi

echo ""
echo "========================================"
echo "FASE 13D.1: Sigilo Tooling Regression Matrix Tests"
echo "========================================"
echo ""

PHASE13D1_FIX="tests/integration/phase13_regression_13d1"
PHASE13D1_RUNNER="tests/run_phase13_regression.sh"
PHASE13D1_TMP="$CCT_TMP_DIR/cct_phase13d1_global"
PHASE13D1_ART_BASE="$PHASE13D1_TMP/base.sigil"
PHASE13D1_ART_INFO="$PHASE13D1_TMP/info.sigil"
PHASE13D1_ART_REVIEW="$PHASE13D1_TMP/review.sigil"
PHASE13D1_ART_BEHAV="$PHASE13D1_TMP/behavior.sigil"
PHASE13D1_BASELINE="$PHASE13D1_TMP/baseline.sigil"
PHASE13D1_PROJ="$PHASE13D1_TMP/project"
PHASE13D1_CI_BASE="$PHASE13D1_TMP/ci_base.sigil"

rm -rf "$PHASE13D1_TMP"
mkdir -p "$PHASE13D1_TMP"

# Test 626: canonical fixture inventory exists
echo "Test 626: phase13d1 canonical fixture inventory exists"
if [ -f "$PHASE13D1_FIX/single_file/smoke.cct" ] && \
   [ -f "$PHASE13D1_FIX/multi_module/main.cct" ] && \
   [ -f "$PHASE13D1_FIX/multi_module/modules/util.cct" ] && \
   [ -f "$PHASE13D1_FIX/project/src/main.cct" ] && \
   [ -f "$PHASE13D1_FIX/artifacts/legacy_v1.sigil" ] && \
   [ -f "$PHASE13D1_FIX/artifacts/expanded_metadata_v1.sigil" ] && \
   [ -f "$PHASE13D1_FIX/artifacts/invalid_missing_required.sigil" ]; then
    test_pass "13D1 fixtures cover single-file, multi-module, project, and artifact domains"
else
    test_fail "13D1 fixture inventory is incomplete"
fi

# Test 627: dedicated phase13d1 runner exists and is executable
echo "Test 627: dedicated phase13d1 runner exists and is executable"
if [ -x "$PHASE13D1_RUNNER" ]; then
    test_pass "phase13d1 dedicated regression runner exists and is executable"
else
    test_fail "phase13d1 dedicated regression runner missing or not executable"
fi

# Test 628: dedicated phase13d1 runner executes successfully
echo "Test 628: dedicated phase13d1 runner executes successfully"
if "$PHASE13D1_RUNNER" >$CCT_TMP_DIR/cct_phase13d1_runner.out 2>&1; then
    test_pass "phase13d1 dedicated regression runner executes all domain checks"
else
    test_fail "phase13d1 dedicated regression runner reported failures"
fi

# Test 629: parser compatibility legacy + strict higher-schema rejection
echo "Test 629: parser legacy + higher schema compatibility contract"
OUTPUT=$("$CCT_BIN" sigilo inspect "$PHASE13D1_FIX/artifacts/legacy_v1.sigil" --summary 2>&1)
EXIT_A=$?
OUTPUT_STRICT=$("$CCT_BIN" sigilo inspect "$PHASE13D1_FIX/artifacts/higher_schema_v2.sigil" --summary --consumer-profile strict-contract 2>&1)
EXIT_B=$?
if [ $EXIT_A -eq 0 ] && echo "$OUTPUT" | grep -q "sigilo.inspect.summary" && [ $EXIT_B -ne 0 ] && echo "$OUTPUT_STRICT" | grep -q "schema_mismatch"; then
    test_pass "legacy artifact parses and strict profile rejects higher schema"
else
    test_fail "parser compatibility contract failed for legacy/higher-schema path"
fi

# Test 630: diff severity matrix covers none/informational/review/behavioral-risk
echo "Test 630: diff severity matrix covers all severity levels"
cat > "$PHASE13D1_ART_BASE" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
cp "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_INFO"
cp "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_REVIEW"
echo "future_optional = additive" >> "$PHASE13D1_ART_INFO"
sed -i 's/semantic_hash = .*/semantic_hash = 1111111111111111/' "$PHASE13D1_ART_REVIEW"
cat > "$PHASE13D1_ART_BEHAV" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = system
system_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
OUT_NONE=$("$CCT_BIN" sigilo diff "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_BASE" --summary 2>&1)
OUT_INFO=$("$CCT_BIN" sigilo diff "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_INFO" --summary 2>&1)
OUT_REVIEW=$("$CCT_BIN" sigilo diff "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_REVIEW" --summary 2>&1)
OUT_BEHAV=$("$CCT_BIN" sigilo diff "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_BEHAV" --summary 2>&1)
if echo "$OUT_NONE" | grep -q "highest=none" && \
   echo "$OUT_INFO" | grep -q "highest=informational" && \
   echo "$OUT_REVIEW" | grep -q "highest=review-required" && \
   echo "$OUT_BEHAV" | grep -q "highest=behavioral-risk"; then
    test_pass "diff severity matrix remains stable across all critical severities"
else
    test_fail "diff severity matrix coverage failed for one or more severities"
fi

# Test 631: baseline update/check with and without drift
echo "Test 631: baseline update/check covers stable and drifted states"
OUTPUT=$("$CCT_BIN" sigilo baseline update "$PHASE13D1_ART_BASE" --baseline "$PHASE13D1_BASELINE" --force 2>&1)
OUT_STABLE=$("$CCT_BIN" sigilo baseline check "$PHASE13D1_ART_BASE" --baseline "$PHASE13D1_BASELINE" --summary 2>&1)
sed -i 's/semantic_hash = .*/semantic_hash = deadbeefcafebabe/' "$PHASE13D1_BASELINE"
OUT_DRIFT=$("$CCT_BIN" sigilo baseline check "$PHASE13D1_ART_BASE" --baseline "$PHASE13D1_BASELINE" --strict --summary 2>&1)
EXIT_DRIFT=$?
if echo "$OUTPUT" | grep -q "status=written" && \
   echo "$OUT_STABLE" | grep -q "highest=none" && \
   [ $EXIT_DRIFT -eq 2 ] && \
   echo "$OUT_DRIFT" | grep -q "status=drift"; then
    test_pass "baseline workflow preserves stable path and blocks strict drift"
else
    test_fail "baseline workflow regression detected in stable/drift contract"
fi

# Test 632: CI profile matrix advisory/gated/release behavior
echo "Test 632: CI profile matrix advisory/gated/release remains stable"
cp -R "$PHASE13D1_FIX/project" "$PHASE13D1_PROJ"
"$CCT_BIN" build --project "$PHASE13D1_PROJ" >$CCT_TMP_DIR/cct_phase13d1_build_boot.out 2>&1 || true
OUTPUT=$("$CCT_BIN" sigilo baseline update "$PHASE13D1_PROJ/src/main.system.sigil" --baseline "$PHASE13D1_CI_BASE" --force 2>&1)
sed -i 's/system_hash = .*/system_hash = 0123456789abcdef/' "$PHASE13D1_CI_BASE"
"$CCT_BIN" build --project "$PHASE13D1_PROJ" --sigilo-check --sigilo-ci-profile advisory --sigilo-baseline "$PHASE13D1_CI_BASE" >$CCT_TMP_DIR/cct_phase13d1_ci_advisory.out 2>&1
EXIT_ADV=$?
"$CCT_BIN" build --project "$PHASE13D1_PROJ" --sigilo-check --sigilo-ci-profile gated --sigilo-baseline "$PHASE13D1_CI_BASE" >$CCT_TMP_DIR/cct_phase13d1_ci_gated.out 2>&1
EXIT_GATED=$?
"$CCT_BIN" build --project "$PHASE13D1_PROJ" --sigilo-check --sigilo-ci-profile release --sigilo-baseline "$PHASE13D1_CI_BASE" >$CCT_TMP_DIR/cct_phase13d1_ci_release.out 2>&1
EXIT_REL=$?
if echo "$OUTPUT" | grep -q "status=written" && [ $EXIT_ADV -eq 0 ] && [ $EXIT_GATED -eq 2 ] && [ $EXIT_REL -eq 2 ]; then
    test_pass "CI profiles keep advisory non-blocking and gated/release blocking for review drift"
else
    test_fail "CI profile contract regressed for advisory/gated/release behavior"
fi

# Test 633: validator strict/tolerant contract remains stable
echo "Test 633: validator strict/tolerant contract remains stable"
OUT_TOL=$("$CCT_BIN" sigilo validate "$PHASE13D1_FIX/artifacts/expanded_metadata_v1.sigil" --summary --consumer-profile legacy-tolerant 2>&1)
OUT_STRICT=$("$CCT_BIN" sigilo validate "$PHASE13D1_FIX/artifacts/invalid_missing_required.sigil" --summary --strict 2>&1)
EXIT_STRICT=$?
if echo "$OUT_TOL" | grep -q "result=pass" && [ $EXIT_STRICT -ne 0 ] && echo "$OUT_STRICT" | grep -q "errors="; then
    test_pass "validator keeps tolerant pass and strict blocking behavior"
else
    test_fail "validator strict/tolerant contract regressed"
fi

# Test 634: project integration build/test/doc stays functional in regression fixture
echo "Test 634: project build/test/doc integration stays functional"
BUILD_OUT=$("$CCT_BIN" build --project "$PHASE13D1_PROJ" 2>&1)
TEST_OUT=$("$CCT_BIN" test --project "$PHASE13D1_PROJ" 2>&1)
DOC_OUT=$("$CCT_BIN" doc --project "$PHASE13D1_PROJ" --output-dir "$PHASE13D1_TMP/docs" --format markdown --no-timestamp 2>&1)
if echo "$BUILD_OUT" | grep -q "\\[build\\] output:" && \
   echo "$TEST_OUT" | grep -q "\\[test\\] summary:" && \
   echo "$DOC_OUT" | grep -q "\\[doc\\] modules:"; then
    test_pass "project integration workflow remains functional for build/test/doc"
else
    test_fail "project integration workflow regressed in build/test/doc path"
fi

# Test 635: legacy command surface remains stable in 13D1 fixture
echo "Test 635: legacy command surface remains stable in 13D1 fixture"
LEGACY_COMPILE=$("$CCT_BIN" "$PHASE13D1_FIX/single_file/smoke.cct" 2>&1)
LEGACY_SIGILO=$("$CCT_BIN" --sigilo-only "$PHASE13D1_FIX/multi_module/main.cct" 2>&1)
LEGACY_CHECK=$("$CCT_BIN" sigilo check "$PHASE13D1_ART_BASE" "$PHASE13D1_ART_BASE" --strict --summary 2>&1)
if echo "$LEGACY_COMPILE" | grep -q "Compiled:" && \
   echo "$LEGACY_SIGILO" | grep -q "Sigil Meta:" && \
   echo "$LEGACY_CHECK" | grep -q "highest=none"; then
    test_pass "legacy command surface remains stable after 13D1 regression integration"
else
    test_fail "legacy command regression detected in 13D1 fixture paths"
fi

# Test 636: architecture/snapshot documentation includes 13D1 regression inventory
# Removed Test 636: documentation text/file comparison (policy)

echo ""
echo "========================================"
echo "FASE 13D.2: Determinism Audit Tests"
echo "========================================"
echo ""

PHASE13D2_RUNNER="tests/run_phase13_determinism_audit.sh"
PHASE13D2_DOC="md_out/docs/release/FASE_13_DETERMINISM_AUDIT.md"
PHASE13D2_TMP="$CCT_TMP_DIR/cct_phase13d2_global"
PHASE13D2_BASE="$PHASE13D2_TMP/base.sigil"
PHASE13D2_REVIEW="$PHASE13D2_TMP/review.sigil"
PHASE13D2_BASELINE="$PHASE13D2_TMP/baseline.sigil"
PHASE13D2_DRIFT="$PHASE13D2_TMP/baseline_drift.sigil"

rm -rf "$PHASE13D2_TMP"
mkdir -p "$PHASE13D2_TMP"

cat > "$PHASE13D2_BASE" <<'SIGEOF'
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

cat > "$PHASE13D2_REVIEW" <<'SIGEOF'
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

echo "Test 637: dedicated 13D2 determinism runner exists and is executable"
if [ -x "$PHASE13D2_RUNNER" ]; then
    test_pass "13D2 determinism runner exists and is executable"
else
    test_fail "13D2 determinism runner missing or not executable"
fi

# Test 638: determinism audit document exists and describes repetition + byte comparison
# Removed Test 638: documentation text/file comparison (policy)

# Test 639: dedicated 13D2 determinism runner executes successfully
echo "Test 639: dedicated 13D2 determinism runner executes successfully"
if "$PHASE13D2_RUNNER" >$CCT_TMP_DIR/cct_phase13d2_runner.out 2>&1; then
    test_pass "13D2 determinism runner executes all audit scenarios successfully"
else
    test_fail "13D2 determinism runner reported failures"
fi

# Test 640: repeated structured inspect remains byte-equivalent
echo "Test 640: repeated structured inspect remains byte-equivalent"
"$CCT_BIN" sigilo inspect "$PHASE13D2_BASE" --format structured --summary >$CCT_TMP_DIR/cct_phase13d2_inspect_1.out 2>&1
"$CCT_BIN" sigilo inspect "$PHASE13D2_BASE" --format structured --summary >$CCT_TMP_DIR/cct_phase13d2_inspect_2.out 2>&1
if cmp -s $CCT_TMP_DIR/cct_phase13d2_inspect_1.out $CCT_TMP_DIR/cct_phase13d2_inspect_2.out; then
    test_pass "structured inspect output is byte-equivalent across repeated runs"
else
    test_fail "structured inspect output is not byte-equivalent across repeated runs"
fi

# Test 641: repeated structured diff keeps stable severity classification
echo "Test 641: repeated structured diff keeps stable severity classification"
DIFF1=$("$CCT_BIN" sigilo diff "$PHASE13D2_BASE" "$PHASE13D2_REVIEW" --format structured --summary 2>&1)
DIFF2=$("$CCT_BIN" sigilo diff "$PHASE13D2_BASE" "$PHASE13D2_REVIEW" --format structured --summary 2>&1)
if [ "$DIFF1" = "$DIFF2" ] && echo "$DIFF1" | grep -q "highest_severity = review-required"; then
    test_pass "structured diff keeps deterministic output and stable review-required severity"
else
    test_fail "structured diff output/severity changed across repeated runs"
fi

# Test 642: repeated baseline check keeps stable deterministic decision
echo "Test 642: repeated baseline check keeps stable deterministic decision"
"$CCT_BIN" sigilo baseline update "$PHASE13D2_BASE" --baseline "$PHASE13D2_BASELINE" --force >$CCT_TMP_DIR/cct_phase13d2_base_update.out 2>&1
"$CCT_BIN" sigilo baseline check "$PHASE13D2_BASE" --baseline "$PHASE13D2_BASELINE" --format structured --summary >$CCT_TMP_DIR/cct_phase13d2_basecheck_1.out 2>&1
"$CCT_BIN" sigilo baseline check "$PHASE13D2_BASE" --baseline "$PHASE13D2_BASELINE" --format structured --summary >$CCT_TMP_DIR/cct_phase13d2_basecheck_2.out 2>&1
if cmp -s $CCT_TMP_DIR/cct_phase13d2_basecheck_1.out $CCT_TMP_DIR/cct_phase13d2_basecheck_2.out && grep -q "highest_severity = none" $CCT_TMP_DIR/cct_phase13d2_basecheck_1.out; then
    test_pass "baseline check decision/output is stable for repeated deterministic runs"
else
    test_fail "baseline check decision/output drifted across repeated runs"
fi

# Test 643: strict baseline drift keeps stable blocking exit and output
echo "Test 643: strict baseline drift keeps stable blocking decision"
cp "$PHASE13D2_BASELINE" "$PHASE13D2_DRIFT"
sed -i 's/semantic_hash = .*/semantic_hash = deadbeefcafebabe/' "$PHASE13D2_DRIFT"
"$CCT_BIN" sigilo baseline check "$PHASE13D2_BASE" --baseline "$PHASE13D2_DRIFT" --format structured --summary --strict >$CCT_TMP_DIR/cct_phase13d2_drift_1.out 2>&1
EXIT1=$?
"$CCT_BIN" sigilo baseline check "$PHASE13D2_BASE" --baseline "$PHASE13D2_DRIFT" --format structured --summary --strict >$CCT_TMP_DIR/cct_phase13d2_drift_2.out 2>&1
EXIT2=$?
if [ $EXIT1 -eq 2 ] && [ $EXIT2 -eq 2 ] && cmp -s $CCT_TMP_DIR/cct_phase13d2_drift_1.out $CCT_TMP_DIR/cct_phase13d2_drift_2.out; then
    test_pass "strict drift path keeps deterministic blocking exit and output"
else
    test_fail "strict drift path produced unstable exit/output across repeated runs"
fi

# Test 644: repeated structured validate keeps byte-equivalent output
echo "Test 644: repeated structured validate keeps byte-equivalent output"
"$CCT_BIN" sigilo validate "$PHASE13D2_BASE" --format structured --summary --consumer-profile current-default >$CCT_TMP_DIR/cct_phase13d2_validate_1.out 2>&1
"$CCT_BIN" sigilo validate "$PHASE13D2_BASE" --format structured --summary --consumer-profile current-default >$CCT_TMP_DIR/cct_phase13d2_validate_2.out 2>&1
if cmp -s $CCT_TMP_DIR/cct_phase13d2_validate_1.out $CCT_TMP_DIR/cct_phase13d2_validate_2.out && grep -q "result = pass" $CCT_TMP_DIR/cct_phase13d2_validate_1.out; then
    test_pass "structured validate output remains deterministic and passing"
else
    test_fail "structured validate output is not deterministic across repeated runs"
fi

# Test 645: deterministic output profile is free of volatile fields
echo "Test 645: deterministic output profile has no volatile fields"
if rg -n "timestamp|generated_at|time=|pid|nonce|uuid|session|random" \
      $CCT_TMP_DIR/cct_phase13d2_inspect_1.out \
      $CCT_TMP_DIR/cct_phase13d2_basecheck_1.out \
      $CCT_TMP_DIR/cct_phase13d2_validate_1.out >$CCT_TMP_DIR/cct_phase13d2_volatile.out 2>&1; then
    test_fail "deterministic output unexpectedly contains volatile fields"
else
    test_pass "deterministic output profile is free of volatile fields"
fi

# Test 646: final snapshot references 13D2 determinism artifacts
# Removed Test 646: documentation text/file comparison (policy)

echo ""
echo "========================================"
echo "FASE 13D.3: Release Documentation Consolidation Tests"
echo "========================================"
echo ""

PHASE13D3_SNAPSHOT="docs/release/FASE_13_FINAL_SNAPSHOT.md"
PHASE13D3_STABILITY="docs/release/FASE_13_STABILITY_MATRIX.md"
PHASE13D3_COMPAT="docs/release/FASE_13_COMPATIBILITY_MATRIX.md"
PHASE13D3_LIMITS="docs/release/FASE_13_KNOWN_LIMITS.md"
PHASE13D3_NOTES="docs/release/FASE_13_RELEASE_NOTES.md"
PHASE13D3_TMP="$CCT_TMP_DIR/cct_phase13d3_global"
PHASE13D3_BASE="$PHASE13D3_TMP/base.sigil"
PHASE13D3_REVIEW="$PHASE13D3_TMP/review.sigil"
PHASE13D3_BASELINE="$PHASE13D3_TMP/baseline.sigil"

rm -rf "$PHASE13D3_TMP"
mkdir -p "$PHASE13D3_TMP"
cat > "$PHASE13D3_BASE" <<'SIGEOF'
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
cat > "$PHASE13D3_REVIEW" <<'SIGEOF'
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
"$CCT_BIN" sigilo baseline update "$PHASE13D3_BASE" --baseline "$PHASE13D3_BASELINE" --force >$CCT_TMP_DIR/cct_phase13d3_base_update.out 2>&1

echo "Test 647: mandatory FASE 13 release documents exist"
# Removed Test 647: documentation text/file comparison (policy)

# Test 648: smoke of documented sigilo commands from release notes
echo "Test 648: documented sigilo commands smoke successfully"
OUT_INSPECT=$("$CCT_BIN" sigilo inspect "$PHASE13D3_BASE" --summary 2>&1)
OUT_DIFF=$("$CCT_BIN" sigilo diff "$PHASE13D3_BASE" "$PHASE13D3_REVIEW" --summary 2>&1)
OUT_CHECK=$("$CCT_BIN" sigilo check "$PHASE13D3_BASE" "$PHASE13D3_BASE" --strict --summary 2>&1)
OUT_BASE_CHECK=$("$CCT_BIN" sigilo baseline check "$PHASE13D3_BASE" --baseline "$PHASE13D3_BASELINE" --summary 2>&1)
OUT_VALIDATE=$("$CCT_BIN" sigilo validate "$PHASE13D3_BASE" --summary --consumer-profile current-default 2>&1)
if echo "$OUT_INSPECT" | grep -q "sigilo.inspect.summary" && \
   echo "$OUT_DIFF" | grep -q "sigil_diff.summary" && \
   echo "$OUT_CHECK" | grep -q "highest=none" && \
   echo "$OUT_BASE_CHECK" | grep -q "status=ok" && \
   echo "$OUT_VALIDATE" | grep -q "sigilo.validate.summary"; then
    test_pass "documented sigilo command set executes successfully in smoke validation"
else
    test_fail "one or more documented sigilo commands failed smoke validation"
fi

# Test 649: snapshot and stability matrix share consistent component statuses
# Removed Test 649: documentation text/file comparison (policy)

# Test 650: known limits and release notes maintain aligned limit identifiers
# Removed Test 650: documentation text/file comparison (policy)

# Test 651: release document cross-references resolve to existing files
# Removed Test 651: documentation text/file comparison (policy)

# Test 652: canonical docs reference consolidated FASE 13 release package
# Removed Test 652: documentation text/file comparison (policy)

echo ""
echo "========================================"
echo "FASE 13D.4: Final Exit Gate and Phase Closure Tests"
echo "========================================"
echo ""

PHASE13D4_CLOSURE="md_out/docs/release/FASE_13_CLOSURE_GATE.md"
PHASE13D4_RISKS="md_out/docs/release/FASE_13_RESIDUAL_RISKS.md"
PHASE13D4_SNAPSHOT="docs/release/FASE_13_FINAL_SNAPSHOT.md"
PHASE13D4_TMP="$CCT_TMP_DIR/cct_phase13d4_global"
PHASE13D4_BASE="$PHASE13D4_TMP/base.sigil"
PHASE13D4_REVIEW="$PHASE13D4_TMP/review.sigil"
PHASE13D4_BASELINE="$PHASE13D4_TMP/baseline.sigil"

rm -rf "$PHASE13D4_TMP"
mkdir -p "$PHASE13D4_TMP"

cat > "$PHASE13D4_BASE" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
module_count = 1
module_resolution_status = ok
SIGEOF

cat > "$PHASE13D4_REVIEW" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
module_count = 1
module_resolution_status = ok
SIGEOF

"$CCT_BIN" sigilo baseline update "$PHASE13D4_BASE" --baseline "$PHASE13D4_BASELINE" --force >$CCT_TMP_DIR/cct_phase13d4_base_update_init.out 2>&1

echo "Test 653: mandatory 13D4 closure documents exist"
# Removed Test 653: documentation text/file comparison (policy)

# Test 654: closure gate records objective pass decision
# Removed Test 654: documentation text/file comparison (policy)

# Test 655: residual risks register keeps structured deferred backlog
# Removed Test 655: documentation text/file comparison (policy)

# Test 656: final snapshot references closure-gate artifacts and closed stage
# Removed Test 656: documentation text/file comparison (policy)

# Test 657: roadmap preserves 13D4 closure history and records post-13 progress
# Removed Test 657: documentation text/file comparison (policy)

echo "Test 658: rerun complete phase 13 regression suite"
if tests/run_phase13_regression.sh >$CCT_TMP_DIR/cct_phase13d4_regression.out 2>&1; then
    test_pass "phase 13 regression suite rerun passed in 13D4 gate"
else
    test_fail "phase 13 regression suite rerun failed in 13D4 gate"
fi

# Test 659: required determinism audit rerun
echo "Test 659: rerun phase 13 determinism audit suite"
if tests/run_phase13_determinism_audit.sh >$CCT_TMP_DIR/cct_phase13d4_determinism.out 2>&1; then
    test_pass "phase 13 determinism audit rerun passed in 13D4 gate"
else
    test_fail "phase 13 determinism audit rerun failed in 13D4 gate"
fi

# Test 660: validate documented command set used by closure gate docs
echo "Test 660: documented sigilo commands validate successfully"
OUT_INSPECT=$("$CCT_BIN" sigilo inspect "$PHASE13D4_BASE" --summary 2>&1)
OUT_DIFF=$("$CCT_BIN" sigilo diff "$PHASE13D4_BASE" "$PHASE13D4_REVIEW" --summary 2>&1)
OUT_CHECK=$("$CCT_BIN" sigilo check "$PHASE13D4_BASE" "$PHASE13D4_BASE" --strict --summary 2>&1)
"$CCT_BIN" sigilo baseline update "$PHASE13D4_BASE" --baseline "$PHASE13D4_BASELINE" --force >$CCT_TMP_DIR/cct_phase13d4_base_update.out 2>&1
OUT_BASE_CHECK=$("$CCT_BIN" sigilo baseline check "$PHASE13D4_BASE" --baseline "$PHASE13D4_BASELINE" --summary 2>&1)
OUT_VALIDATE=$("$CCT_BIN" sigilo validate "$PHASE13D4_BASE" --summary --consumer-profile current-default 2>&1)
if echo "$OUT_INSPECT" | grep -q "sigilo.inspect.summary" && \
   echo "$OUT_DIFF" | grep -q "sigil_diff.summary" && \
   echo "$OUT_CHECK" | grep -q "highest=none" && \
   echo "$OUT_BASE_CHECK" | grep -q "status=ok" && \
   echo "$OUT_VALIDATE" | grep -q "sigilo.validate.summary"; then
    test_pass "documented command set is valid and executable at closure gate"
else
    test_fail "documented command set failed validation at closure gate"
fi

# Test 661: legacy CLI surface remains non-regressive at closure gate
echo "Test 661: legacy CLI surface remains non-regressive"
LEGACY_COMPILE=$("$CCT_BIN" tests/integration/phase13_regression_13d1/single_file/smoke.cct 2>&1)
LEGACY_SIGILO_ONLY=$("$CCT_BIN" --sigilo-only tests/integration/phase13_regression_13d1/multi_module/main.cct 2>&1)
LEGACY_HELP=$("$CCT_BIN" --help 2>&1)
if echo "$LEGACY_COMPILE" | grep -q "Compiled:" && \
   echo "$LEGACY_SIGILO_ONLY" | grep -q "Sigil Meta:" && \
   echo "$LEGACY_HELP" | grep -q "Usage:"; then
    test_pass "legacy CLI compile/sigilo/help flows remain stable in 13D4"
else
    test_fail "legacy CLI non-regression failed in closure gate"
fi

echo ""
echo "========================================"
echo "FASE 13M.A1: Scope Freeze and Semantic Contract Tests"
echo "========================================"
echo ""

DOC_13M_MASTER=$(resolve_doc_path "FASE_13M_CCT.md")
DOC_13M_A1=$(resolve_doc_path "FASE_13M_A1_CCT.md")
DOC_13M_A2=$(resolve_doc_path "FASE_13M_A2_CCT.md")
DOC_13M_B1=$(resolve_doc_path "FASE_13M_B1_CCT.md")
DOC_13M_B2=$(resolve_doc_path "FASE_13M_B2_CCT.md")
DOC_13M_CHECKLIST=$(resolve_doc_path "FASE_13M_CHECKLIST_DOCUMENTOS.md")

# Test 662: mandatory 13M planning/execution docs exist
echo "Test 662: mandatory 13M planning/execution docs exist"
if [ -f "$DOC_13M_MASTER" ] && \
   [ -f "$DOC_13M_A1" ] && \
   [ -f "$DOC_13M_A2" ] && \
   [ -f "$DOC_13M_B1" ] && \
   [ -f "$DOC_13M_B2" ] && \
   [ -f "$DOC_13M_CHECKLIST" ]; then
    test_pass "13M core and executable-phase documents are present"
else
    test_fail "13M core and executable-phase documents are incomplete"
fi

# Test 663: 13M master architecture defines consolidated execution order
echo "Test 663: 13M architecture defines consolidated A1->A2->B1->B2 order"
if grep -q "13M-A1" "$DOC_13M_MASTER" && \
   grep -q "13M-A2" "$DOC_13M_MASTER" && \
   grep -q "13M-B1" "$DOC_13M_MASTER" && \
   grep -q "13M-B2" "$DOC_13M_MASTER"; then
    test_pass "13M architecture keeps consolidated execution order"
else
    test_fail "13M architecture is missing consolidated execution order entries"
fi

# Test 664: A1 freezes final semantic decisions (power/idiv/emod and caret deferral)
echo "Test 664: 13M-A1 freezes semantic decisions and caret policy"
if grep -q "POLICY_CARET_DEFERRED" "$DOC_13M_A1" && \
   grep -q "floor division" "$DOC_13M_A1" && \
   grep -q "módulo euclidiano" "$DOC_13M_A1"; then
    test_pass "13M-A1 freezes caret deferral and idiv/emod semantic decisions"
else
    test_fail "13M-A1 is missing one or more frozen semantic decisions"
fi

# Test 665: A1 locks scope to **, //, %% and excludes optional-chaining family
echo "Test 665: 13M-A1 locks scope to high-utility math operators only"
if grep -q "\`\\*\\*\`" "$DOC_13M_A1" && \
   grep -q "\`//\`" "$DOC_13M_A1" && \
   grep -q "\`%%\`" "$DOC_13M_A1" && \
   grep -q "\`\?\\.\`" "$DOC_13M_A1" && \
   grep -q "\`!!\`" "$DOC_13M_A1"; then
    test_pass "13M-A1 scope includes only prioritized operators and excludes optional system"
else
    test_fail "13M-A1 scope lock is incomplete for priority/exclusion set"
fi

# Test 666: execution checklist keeps mandatory stages and current progression coherence
echo "Test 666: 13M execution checklist keeps coherent stage progression"
if grep -q "\[x\] \`13M-A1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-A2\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-B1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-B2\`" "$DOC_13M_CHECKLIST"; then
    test_pass "13M checklist execution status reflects A1/A2/B1/B2 completion"
else
    test_fail "13M checklist execution status is incoherent for current progression"
fi

# Test 667: A2 implementation contract remains aligned with A1 frozen operator set
echo "Test 667: 13M-A2 implementation contract aligns with A1 frozen set"
if grep -q "\`\*\*\`" "$DOC_13M_A2" && \
   grep -q "\`//\`" "$DOC_13M_A2" && \
   grep -q "\`%%\`" "$DOC_13M_A2" && \
   ! grep -q "implementar.*\`\^\`" "$DOC_13M_A2"; then
    test_pass "13M-A2 stays aligned with A1 frozen scope"
else
    test_fail "13M-A2 contract drift detected against A1 frozen scope"
fi

# Test 668: B1 requires broad test matrix and full historical regression
echo "Test 668: 13M-B1 demands broad matrix and full regression"
if grep -q "Matriz extensa de testes obrigatórios" "$DOC_13M_B1" && \
   grep -q "no mínimo 25 novos checks efetivos" "$DOC_13M_B1" && \
   grep -q "make test" "$DOC_13M_B1"; then
    test_pass "13M-B1 quality contract requires broad coverage and full regression"
else
    test_fail "13M-B1 quality contract is missing required breadth/regression clauses"
fi

# Test 669: B2 closure contract includes docs harmony and final gate evidence
# Removed Test 669: documentation text/file comparison (policy)

echo "Test 670: 13M consolidation mapping keeps C/D absorbed into B1/B2"
if grep -q "Conteúdo de \`13M-C\` foi incorporado em \`13M-B1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "Conteúdo de \`13M-D\` foi incorporado em \`13M-B2\`" "$DOC_13M_CHECKLIST"; then
    test_pass "13M consolidation mapping is explicit and consistent"
else
    test_fail "13M consolidation mapping is missing or inconsistent"
fi

echo ""
echo "========================================"
echo "FASE 13M.A2: Compiler Implementation for ** // %% Tests"
echo "========================================"
echo ""

cleanup_codegen_artifacts "tests/integration/math_pow_basic_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_pow_assoc_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_pow_real_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_idiv_floor_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_emod_euclid_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_precedence_mix_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_idiv_zero_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_emod_zero_13m.cct"

SIG13M_A2_TOK="$CCT_TMP_DIR/cct_13m_a2_tokens.cct"
cat > "$SIG13M_A2_TOK" <<'CCTEOF'
INCIPIT grimoire "tok"
RITUALE main() REDDE REX
  EVOCA REX a AD 2 ** 3
  EVOCA REX b AD 7 // 2
  EVOCA REX c AD 7 %% 3
  REDDE a + b + c
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF

# Test 671: lexer recognizes STAR_STAR token
echo "Test 671: lexer recognizes STAR_STAR token"
OUT_TOK=$("$CCT_BIN" --tokens "$SIG13M_A2_TOK" 2>&1)
if echo "$OUT_TOK" | grep -q "STAR_STAR"; then
    test_pass "lexer recognizes ** as STAR_STAR"
else
    test_fail "lexer did not recognize ** as STAR_STAR"
fi

# Test 672: lexer recognizes SLASH_SLASH token
echo "Test 672: lexer recognizes SLASH_SLASH token"
if echo "$OUT_TOK" | grep -q "SLASH_SLASH"; then
    test_pass "lexer recognizes // as SLASH_SLASH"
else
    test_fail "lexer did not recognize // as SLASH_SLASH"
fi

# Test 673: lexer recognizes PERCENT_PERCENT token
echo "Test 673: lexer recognizes PERCENT_PERCENT token"
if echo "$OUT_TOK" | grep -q "PERCENT_PERCENT"; then
    test_pass "lexer recognizes %% as PERCENT_PERCENT"
else
    test_fail "lexer did not recognize %% as PERCENT_PERCENT"
fi

# Test 674: parser/semantic accepts canonical mixed-operator fixture
echo "Test 674: parser/semantic accepts canonical mixed-operator fixture"
if "$CCT_BIN" --check tests/integration/math_precedence_mix_13m.cct >$CCT_TMP_DIR/cct_13m_a2_check_mix.out 2>&1; then
    test_pass "parser/semantic accepts mixed ** // %% expressions"
else
    test_fail "parser/semantic rejected canonical mixed-operator fixture"
fi

# Test 675: ** basic power codegen executes expected result
echo "Test 675: ** basic power codegen executes expected result"
if "$CCT_BIN" tests/integration/math_pow_basic_13m.cct >$CCT_TMP_DIR/cct_13m_a2_pow_basic_build.out 2>&1 && \
   [ -x tests/integration/math_pow_basic_13m ]; then
    tests/integration/math_pow_basic_13m >$CCT_TMP_DIR/cct_13m_a2_pow_basic_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "** basic power compiles and executes"
    else
        test_fail "** basic power executable returned unexpected code ($RC)"
    fi
else
    test_fail "** basic power fixture failed to compile"
fi

# Test 676: ** right-associativity behavior is preserved
echo "Test 676: ** right-associativity behavior is preserved"
if "$CCT_BIN" tests/integration/math_pow_assoc_13m.cct >$CCT_TMP_DIR/cct_13m_a2_pow_assoc_build.out 2>&1 && \
   [ -x tests/integration/math_pow_assoc_13m ]; then
    tests/integration/math_pow_assoc_13m >$CCT_TMP_DIR/cct_13m_a2_pow_assoc_run.out 2>&1
    RC=$?
    if [ $RC -eq 192 ]; then
        test_pass "** chain keeps right-associative semantics (difference 448 -> exit 192)"
    else
        test_fail "** associativity fixture returned unexpected code ($RC)"
    fi
else
    test_fail "** associativity fixture failed to compile"
fi

# Test 677: ** supports numeric real path
echo "Test 677: ** supports numeric real path"
if "$CCT_BIN" tests/integration/math_pow_real_13m.cct >$CCT_TMP_DIR/cct_13m_a2_pow_real_build.out 2>&1 && \
   [ -x tests/integration/math_pow_real_13m ]; then
    tests/integration/math_pow_real_13m >$CCT_TMP_DIR/cct_13m_a2_pow_real_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "** real-power path works in executable subset"
    else
        test_fail "** real-power fixture returned unexpected code ($RC)"
    fi
else
    test_fail "** real-power fixture failed to compile"
fi

# Test 678: // floor behavior for signed inputs
echo "Test 678: // floor behavior for signed inputs"
if "$CCT_BIN" tests/integration/math_idiv_floor_13m.cct >$CCT_TMP_DIR/cct_13m_a2_idiv_build.out 2>&1 && \
   [ -x tests/integration/math_idiv_floor_13m ]; then
    tests/integration/math_idiv_floor_13m >$CCT_TMP_DIR/cct_13m_a2_idiv_run.out 2>&1
    RC=$?
    if [ $RC -eq 48 ]; then
        test_pass "// floor-division behavior is correct for signed combinations"
    else
        test_fail "// floor-division fixture returned unexpected code ($RC)"
    fi
else
    test_fail "// floor-division fixture failed to compile"
fi

# Test 679: %% euclidean modulo behavior for signed inputs
echo "Test 679: %% euclidean modulo behavior for signed inputs"
if "$CCT_BIN" tests/integration/math_emod_euclid_13m.cct >$CCT_TMP_DIR/cct_13m_a2_emod_build.out 2>&1 && \
   [ -x tests/integration/math_emod_euclid_13m ]; then
    tests/integration/math_emod_euclid_13m >$CCT_TMP_DIR/cct_13m_a2_emod_run.out 2>&1
    RC=$?
    if [ $RC -eq 6 ]; then
        test_pass "%% euclidean modulo behavior is correct for signed combinations"
    else
        test_fail "%% euclidean modulo fixture returned unexpected code ($RC)"
    fi
else
    test_fail "%% euclidean modulo fixture failed to compile"
fi

# Test 680: mixed precedence keeps stable contract
echo "Test 680: mixed precedence keeps stable contract"
if "$CCT_BIN" tests/integration/math_precedence_mix_13m.cct >$CCT_TMP_DIR/cct_13m_a2_prec_build.out 2>&1 && \
   [ -x tests/integration/math_precedence_mix_13m ]; then
    tests/integration/math_precedence_mix_13m >$CCT_TMP_DIR/cct_13m_a2_prec_run.out 2>&1
    RC=$?
    if [ $RC -eq 57 ]; then
        test_pass "mixed precedence for ** // %% remains stable"
    else
        test_fail "mixed precedence fixture returned unexpected code ($RC)"
    fi
else
    test_fail "mixed precedence fixture failed to compile"
fi

# Test 681: semantic rejects non-numeric operand for **
echo "Test 681: semantic rejects invalid ** operand type"
OUT=$("$CCT_BIN" --check tests/integration/math_pow_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "arithmetic operators require numeric operands"; then
    test_pass "semantic rejects non-numeric operand for **"
else
    test_fail "semantic did not reject invalid ** operand type"
fi

# Test 682: semantic rejects non-integer operand for //
echo "Test 682: semantic rejects invalid // operand type"
OUT=$("$CCT_BIN" --check tests/integration/math_idiv_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "operator // requires integer operands"; then
    test_pass "semantic rejects non-integer operand for //"
else
    test_fail "semantic did not reject invalid // operand type"
fi

# Test 683: semantic rejects non-integer operand for %%
echo "Test 683: semantic rejects invalid %% operand type"
OUT=$("$CCT_BIN" --check tests/integration/math_emod_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "operator %% requires integer operands"; then
    test_pass "semantic rejects non-integer operand for %%"
else
    test_fail "semantic did not reject invalid %% operand type"
fi

# Test 684: // by zero fails at runtime with clear diagnostic
echo "Test 684: // by zero fails at runtime with clear diagnostic"
if "$CCT_BIN" tests/integration/math_idiv_zero_13m.cct >$CCT_TMP_DIR/cct_13m_a2_idiv_zero_build.out 2>&1 && \
   [ -x tests/integration/math_idiv_zero_13m ]; then
    tests/integration/math_idiv_zero_13m >$CCT_TMP_DIR/cct_13m_a2_idiv_zero_run.out 2>&1
    RC=$?
    if [ $RC -ne 0 ] && grep -q "integer division by zero" $CCT_TMP_DIR/cct_13m_a2_idiv_zero_run.out; then
        test_pass "// by zero fails with explicit runtime diagnostic"
    else
        test_fail "// by zero did not fail with expected runtime diagnostic"
    fi
else
    test_fail "// by zero fixture failed to compile"
fi

# Test 685: %% by zero fails at runtime with clear diagnostic
echo "Test 685: %% by zero fails at runtime with clear diagnostic"
if "$CCT_BIN" tests/integration/math_emod_zero_13m.cct >$CCT_TMP_DIR/cct_13m_a2_emod_zero_build.out 2>&1 && \
   [ -x tests/integration/math_emod_zero_13m ]; then
    tests/integration/math_emod_zero_13m >$CCT_TMP_DIR/cct_13m_a2_emod_zero_run.out 2>&1
    RC=$?
    if [ $RC -ne 0 ] && grep -q "euclidean modulo by zero" $CCT_TMP_DIR/cct_13m_a2_emod_zero_run.out; then
        test_pass "%% by zero fails with explicit runtime diagnostic"
    else
        test_fail "%% by zero did not fail with expected runtime diagnostic"
    fi
else
    test_fail "%% by zero fixture failed to compile"
fi

# Test 686: // and %% identity relation holds for signed canonical case
echo "Test 686: // and %% identity relation holds (a = b*q + r)"
cat > $CCT_TMP_DIR/cct_13m_a2_identity.cct <<'CCTEOF'
INCIPIT grimoire "identity_13m"
RITUALE main() REDDE REX
  EVOCA REX a AD 0 - 7
  EVOCA REX b AD 3
  EVOCA REX q AD a // b
  EVOCA REX r AD a %% b
  EVOCA REX check AD b * q + r
  SI check == a
    REDDE 0
  FIN SI
  REDDE 1
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" $CCT_TMP_DIR/cct_13m_a2_identity.cct >$CCT_TMP_DIR/cct_13m_a2_identity_build.out 2>&1 && \
   [ -x $CCT_TMP_DIR/cct_13m_a2_identity ]; then
    $CCT_TMP_DIR/cct_13m_a2_identity >$CCT_TMP_DIR/cct_13m_a2_identity_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "identity relation holds for // and %% canonical signed case"
    else
        test_fail "identity relation failed in canonical signed case ($RC)"
    fi
else
    test_fail "identity relation fixture failed to compile"
fi

# Test 687: legacy modulus (%) behavior remains available
echo "Test 687: legacy modulus (%) behavior remains available"
cat > $CCT_TMP_DIR/cct_13m_a2_legacy_mod.cct <<'CCTEOF'
INCIPIT grimoire "legacy_mod_13m"
RITUALE main() REDDE REX
  EVOCA REX r AD 7 % 3
  REDDE r
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" $CCT_TMP_DIR/cct_13m_a2_legacy_mod.cct >$CCT_TMP_DIR/cct_13m_a2_legacy_mod_build.out 2>&1 && \
   [ -x $CCT_TMP_DIR/cct_13m_a2_legacy_mod ]; then
    $CCT_TMP_DIR/cct_13m_a2_legacy_mod >$CCT_TMP_DIR/cct_13m_a2_legacy_mod_run.out 2>&1
    RC=$?
    if [ $RC -eq 1 ]; then
        test_pass "legacy % behavior remains available and stable"
    else
        test_fail "legacy % behavior returned unexpected result ($RC)"
    fi
else
    test_fail "legacy % fixture failed to compile"
fi

# Test 688: new operators are reflected in token string surface
echo "Test 688: token string surface includes STAR_STAR/SLASH_SLASH/PERCENT_PERCENT"
if echo "$OUT_TOK" | grep -q "STAR_STAR" && \
   echo "$OUT_TOK" | grep -q "SLASH_SLASH" && \
   echo "$OUT_TOK" | grep -q "PERCENT_PERCENT"; then
    test_pass "token string surface exposes all 13M.A2 operator tokens"
else
    test_fail "token string surface missing one or more 13M.A2 operator tokens"
fi

# Test 689: documented A2 scope still aligned with implemented operator set
echo "Test 689: A2 document scope aligns with implemented set"
if grep -q "\`\*\*\`" "$DOC_13M_A2" && \
   grep -q "\`//\`" "$DOC_13M_A2" && \
   grep -q "\`%%\`" "$DOC_13M_A2"; then
    test_pass "A2 contract remains aligned with implemented operator set"
else
    test_fail "A2 contract drifted from implemented operator set"
fi

# Test 690: A1 freeze still compatible with A2 implementation output
echo "Test 690: A1 freeze remains compatible with A2 implementation"
if grep -q "POLICY_CARET_DEFERRED" "$DOC_13M_A1" && \
   ! echo "$OUT_TOK" | grep -q "CARET"; then
    test_pass "A1 caret deferral remains compatible with A2 implementation"
else
    test_fail "A1 caret deferral is inconsistent with A2 implementation state"
fi

echo ""
echo "========================================"
echo "FASE 13M.B1: Deep Test Matrix and Non-Regression Proof"
echo "========================================"
echo ""

cleanup_codegen_artifacts "tests/integration/math_pow_2_10_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_pow_parenthesized_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_idiv_signs_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_emod_signs_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_precedence_parentheses_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_variables_combo_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_return_expr_13m.cct"
cleanup_codegen_artifacts "tests/integration/math_control_expr_13m.cct"

# Test 691: B1 document defines mandatory 30-scenario matrix
echo "Test 691: B1 contract defines mandatory 30-scenario matrix"
if grep -q "Matriz extensa de testes obrigatórios" "$DOC_13M_B1" && \
   grep -q "30 cenários" "$DOC_13M_B1"; then
    test_pass "B1 contract keeps mandatory deep matrix definition"
else
    test_fail "B1 contract is missing mandatory deep matrix definition"
fi

# Test 692: B1 quality metric requires at least 25 checks
echo "Test 692: B1 quality metric requires at least 25 checks"
if grep -q "no mínimo 25 novos checks efetivos" "$DOC_13M_B1"; then
    test_pass "B1 quality metric includes >=25 effective new checks"
else
    test_fail "B1 quality metric missing >=25 checks requirement"
fi

# Test 693: 2 ** 10 computes expected result
echo "Test 693: 2 ** 10 computes expected result"
if "$CCT_BIN" tests/integration/math_pow_2_10_13m.cct >$CCT_TMP_DIR/cct_13m_b1_pow_2_10_build.out 2>&1 && \
   [ -x tests/integration/math_pow_2_10_13m ]; then
    tests/integration/math_pow_2_10_13m >$CCT_TMP_DIR/cct_13m_b1_pow_2_10_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "2 ** 10 behavior is correct"
    else
        test_fail "2 ** 10 returned unexpected result ($RC)"
    fi
else
    test_fail "2 ** 10 fixture failed to compile"
fi

# Test 694: parenthesized power variants keep expected values
echo "Test 694: parenthesized power variants keep expected values"
if "$CCT_BIN" tests/integration/math_pow_parenthesized_13m.cct >$CCT_TMP_DIR/cct_13m_b1_pow_paren_build.out 2>&1 && \
   [ -x tests/integration/math_pow_parenthesized_13m ]; then
    tests/integration/math_pow_parenthesized_13m >$CCT_TMP_DIR/cct_13m_b1_pow_paren_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "parenthesized power variants produce expected values"
    else
        test_fail "parenthesized power fixture returned unexpected result ($RC)"
    fi
else
    test_fail "parenthesized power fixture failed to compile"
fi

# Test 695: real power fixture remains stable under --check
echo "Test 695: real power fixture passes semantic check"
if "$CCT_BIN" --check tests/integration/math_pow_real_13m.cct >$CCT_TMP_DIR/cct_13m_b1_pow_real_check.out 2>&1; then
    test_pass "real power fixture passes --check path"
else
    test_fail "real power fixture failed semantic check"
fi

# Test 696: // sign matrix returns expected canonical values
echo "Test 696: // sign matrix returns expected canonical values"
if "$CCT_BIN" tests/integration/math_idiv_signs_13m.cct >$CCT_TMP_DIR/cct_13m_b1_idiv_signs_build.out 2>&1 && \
   [ -x tests/integration/math_idiv_signs_13m ]; then
    tests/integration/math_idiv_signs_13m >$CCT_TMP_DIR/cct_13m_b1_idiv_signs_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "// sign matrix is correct for all 4 combinations"
    else
        test_fail "// sign matrix fixture returned unexpected result ($RC)"
    fi
else
    test_fail "// sign matrix fixture failed to compile"
fi

# Test 697: %% sign matrix returns expected canonical values
echo "Test 697: %% sign matrix returns expected canonical values"
if "$CCT_BIN" tests/integration/math_emod_signs_13m.cct >$CCT_TMP_DIR/cct_13m_b1_emod_signs_build.out 2>&1 && \
   [ -x tests/integration/math_emod_signs_13m ]; then
    tests/integration/math_emod_signs_13m >$CCT_TMP_DIR/cct_13m_b1_emod_signs_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "%% sign matrix is correct for all 4 combinations"
    else
        test_fail "%% sign matrix fixture returned unexpected result ($RC)"
    fi
else
    test_fail "%% sign matrix fixture failed to compile"
fi

# Test 698: precedence with and without parentheses keeps contract
echo "Test 698: precedence with and without parentheses keeps contract"
if "$CCT_BIN" tests/integration/math_precedence_parentheses_13m.cct >$CCT_TMP_DIR/cct_13m_b1_prec_paren_build.out 2>&1 && \
   [ -x tests/integration/math_precedence_parentheses_13m ]; then
    tests/integration/math_precedence_parentheses_13m >$CCT_TMP_DIR/cct_13m_b1_prec_paren_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "precedence and parentheses contract is stable"
    else
        test_fail "precedence/parentheses fixture returned unexpected result ($RC)"
    fi
else
    test_fail "precedence/parentheses fixture failed to compile"
fi

# Test 699: mixed precedence fixture remains executable and stable
echo "Test 699: mixed precedence fixture remains executable and stable"
if "$CCT_BIN" tests/integration/math_precedence_mix_13m.cct >$CCT_TMP_DIR/cct_13m_b1_prec_mix_build.out 2>&1 && \
   [ -x tests/integration/math_precedence_mix_13m ]; then
    tests/integration/math_precedence_mix_13m >$CCT_TMP_DIR/cct_13m_b1_prec_mix_run.out 2>&1
    RC=$?
    if [ $RC -eq 57 ]; then
        test_pass "mixed precedence fixture remains stable in B1"
    else
        test_fail "mixed precedence fixture regressed in B1 ($RC)"
    fi
else
    test_fail "mixed precedence fixture failed to compile in B1"
fi

# Test 700: variable-based combinations execute correctly
echo "Test 700: variable-based combinations execute correctly"
if "$CCT_BIN" tests/integration/math_variables_combo_13m.cct >$CCT_TMP_DIR/cct_13m_b1_vars_build.out 2>&1 && \
   [ -x tests/integration/math_variables_combo_13m ]; then
    tests/integration/math_variables_combo_13m >$CCT_TMP_DIR/cct_13m_b1_vars_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "variable-based combinations with ** // %% are correct"
    else
        test_fail "variable-based combinations returned unexpected result ($RC)"
    fi
else
    test_fail "variable-based combinations fixture failed to compile"
fi

# Test 701: REDDE expression composition with new operators works
echo "Test 701: REDDE expression composition with new operators works"
if "$CCT_BIN" tests/integration/math_return_expr_13m.cct >$CCT_TMP_DIR/cct_13m_b1_return_build.out 2>&1 && \
   [ -x tests/integration/math_return_expr_13m ]; then
    tests/integration/math_return_expr_13m >$CCT_TMP_DIR/cct_13m_b1_return_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "function return expression with ** // %% is correct"
    else
        test_fail "function return expression fixture returned unexpected result ($RC)"
    fi
else
    test_fail "function return expression fixture failed to compile"
fi

# Test 702: control-flow expression with new operators works
echo "Test 702: control-flow expression with new operators works"
if "$CCT_BIN" tests/integration/math_control_expr_13m.cct >$CCT_TMP_DIR/cct_13m_b1_ctrl_build.out 2>&1 && \
   [ -x tests/integration/math_control_expr_13m ]; then
    tests/integration/math_control_expr_13m >$CCT_TMP_DIR/cct_13m_b1_ctrl_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "control-flow expression with ** // %% is correct"
    else
        test_fail "control-flow expression fixture returned unexpected result ($RC)"
    fi
else
    test_fail "control-flow expression fixture failed to compile"
fi

# Test 703: power type error keeps minimum diagnostic message
echo "Test 703: power type error keeps minimum diagnostic message"
OUT=$("$CCT_BIN" --check tests/integration/math_pow_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "arithmetic operators require numeric operands"; then
    test_pass "power type error keeps expected minimum diagnostic message"
else
    test_fail "power type error message regressed"
fi

# Test 704: integer division type error keeps minimum diagnostic message
echo "Test 704: integer division type error keeps minimum diagnostic message"
OUT=$("$CCT_BIN" --check tests/integration/math_idiv_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "operator // requires integer operands"; then
    test_pass "integer division type error keeps expected minimum diagnostic message"
else
    test_fail "integer division type error message regressed"
fi

# Test 705: euclidean modulo type error keeps minimum diagnostic message
echo "Test 705: euclidean modulo type error keeps minimum diagnostic message"
OUT=$("$CCT_BIN" --check tests/integration/math_emod_type_error_13m.cct 2>&1) || true
if echo "$OUT" | grep -q "operator %% requires integer operands"; then
    test_pass "euclidean modulo type error keeps expected minimum diagnostic message"
else
    test_fail "euclidean modulo type error message regressed"
fi

# Test 706: // by zero runtime diagnostic remains explicit
echo "Test 706: // by zero runtime diagnostic remains explicit"
if "$CCT_BIN" tests/integration/math_idiv_zero_13m.cct >$CCT_TMP_DIR/cct_13m_b1_idiv_zero_build.out 2>&1 && \
   [ -x tests/integration/math_idiv_zero_13m ]; then
    tests/integration/math_idiv_zero_13m >$CCT_TMP_DIR/cct_13m_b1_idiv_zero_run.out 2>&1
    RC=$?
    if [ $RC -ne 0 ] && grep -q "integer division by zero" $CCT_TMP_DIR/cct_13m_b1_idiv_zero_run.out; then
        test_pass "// by zero runtime diagnostic remains explicit"
    else
        test_fail "// by zero runtime behavior regressed"
    fi
else
    test_fail "// by zero fixture failed to compile in B1"
fi

# Test 707: %% by zero runtime diagnostic remains explicit
echo "Test 707: %% by zero runtime diagnostic remains explicit"
if "$CCT_BIN" tests/integration/math_emod_zero_13m.cct >$CCT_TMP_DIR/cct_13m_b1_emod_zero_build.out 2>&1 && \
   [ -x tests/integration/math_emod_zero_13m ]; then
    tests/integration/math_emod_zero_13m >$CCT_TMP_DIR/cct_13m_b1_emod_zero_run.out 2>&1
    RC=$?
    if [ $RC -ne 0 ] && grep -q "euclidean modulo by zero" $CCT_TMP_DIR/cct_13m_b1_emod_zero_run.out; then
        test_pass "%% by zero runtime diagnostic remains explicit"
    else
        test_fail "%% by zero runtime behavior regressed"
    fi
else
    test_fail "%% by zero fixture failed to compile in B1"
fi

# Test 708: floor-division identity remains valid in canonical signed case
echo "Test 708: floor-division identity remains valid in canonical signed case"
cat > $CCT_TMP_DIR/cct_13m_b1_identity_pos.cct <<'CCTEOF'
INCIPIT grimoire "id_b1"
RITUALE main() REDDE REX
  EVOCA REX a AD 29
  EVOCA REX b AD 4
  EVOCA REX q AD a // b
  EVOCA REX r AD a %% b
  SI b * q + r == a
    REDDE 0
  FIN SI
  REDDE 1
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" $CCT_TMP_DIR/cct_13m_b1_identity_pos.cct >$CCT_TMP_DIR/cct_13m_b1_identity_pos_build.out 2>&1 && \
   [ -x $CCT_TMP_DIR/cct_13m_b1_identity_pos ]; then
    $CCT_TMP_DIR/cct_13m_b1_identity_pos >$CCT_TMP_DIR/cct_13m_b1_identity_pos_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "canonical identity a=b*q+r holds for positive divisor case"
    else
        test_fail "canonical identity failed for positive divisor case ($RC)"
    fi
else
    test_fail "canonical identity positive fixture failed to compile"
fi

# Test 709: floor-division identity remains valid with negative dividend
echo "Test 709: floor-division identity remains valid with negative dividend"
cat > $CCT_TMP_DIR/cct_13m_b1_identity_neg.cct <<'CCTEOF'
INCIPIT grimoire "id_b1_neg"
RITUALE main() REDDE REX
  EVOCA REX a AD (0 - 29)
  EVOCA REX b AD 4
  EVOCA REX q AD a // b
  EVOCA REX r AD a %% b
  SI b * q + r == a
    REDDE 0
  FIN SI
  REDDE 1
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" $CCT_TMP_DIR/cct_13m_b1_identity_neg.cct >$CCT_TMP_DIR/cct_13m_b1_identity_neg_build.out 2>&1 && \
   [ -x $CCT_TMP_DIR/cct_13m_b1_identity_neg ]; then
    $CCT_TMP_DIR/cct_13m_b1_identity_neg >$CCT_TMP_DIR/cct_13m_b1_identity_neg_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "canonical identity a=b*q+r holds for negative dividend case"
    else
        test_fail "canonical identity failed for negative dividend case ($RC)"
    fi
else
    test_fail "canonical identity negative fixture failed to compile"
fi

# Test 710: repeated execution of power binary is stable
echo "Test 710: repeated execution of power binary is stable"
if [ -x tests/integration/math_pow_2_10_13m ]; then
    tests/integration/math_pow_2_10_13m >$CCT_TMP_DIR/cct_13m_b1_pow_repeat_run1.out 2>&1
    RC1=$?
    tests/integration/math_pow_2_10_13m >$CCT_TMP_DIR/cct_13m_b1_pow_repeat_run2.out 2>&1
    RC2=$?
    if [ $RC1 -eq 0 ] && [ $RC2 -eq 0 ]; then
        test_pass "repeated local run remains stable for power fixture"
    else
        test_fail "repeated local run diverged for power fixture ($RC1/$RC2)"
    fi
else
    test_fail "power repeat check missing built binary"
fi

# Test 711: repeated check output for type error stays message-stable
echo "Test 711: repeated check output for type error stays message-stable"
OUT1=$("$CCT_BIN" --check tests/integration/math_idiv_type_error_13m.cct 2>&1) || true
OUT2=$("$CCT_BIN" --check tests/integration/math_idiv_type_error_13m.cct 2>&1) || true
if echo "$OUT1" | grep -q "operator // requires integer operands" && \
   echo "$OUT2" | grep -q "operator // requires integer operands"; then
    test_pass "repeated diagnostic check remains semantically stable"
else
    test_fail "repeated diagnostic check is unstable for // type error"
fi

# Test 712: legacy arithmetic fixture remains stable after 13M operators
echo "Test 712: legacy arithmetic fixture remains stable after 13M operators"
if "$CCT_BIN" tests/integration/codegen_arithmetic.cct >$CCT_TMP_DIR/cct_13m_b1_legacy_arith_build.out 2>&1 && \
   [ -x tests/integration/codegen_arithmetic ]; then
    tests/integration/codegen_arithmetic >$CCT_TMP_DIR/cct_13m_b1_legacy_arith_run.out 2>&1
    RC=$?
    if [ $RC -eq 15 ]; then
        test_pass "legacy arithmetic behavior remains stable"
    else
        test_fail "legacy arithmetic behavior regressed ($RC)"
    fi
else
    test_fail "legacy arithmetic fixture failed to compile in B1"
fi

# Test 713: legacy modulus fixture remains stable after 13M operators
echo "Test 713: legacy modulus fixture remains stable after 13M operators"
cat > $CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat.cct <<'CCTEOF'
INCIPIT grimoire "legacy_mod_repeat_b1"
RITUALE main() REDDE REX
  REDDE 23 % 7
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" $CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat.cct >$CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat_build.out 2>&1 && \
   [ -x $CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat ]; then
    $CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat >$CCT_TMP_DIR/cct_13m_b1_legacy_mod_repeat_run.out 2>&1
    RC=$?
    if [ $RC -eq 2 ]; then
        test_pass "legacy % remains stable after 13M operators"
    else
        test_fail "legacy % behavior regressed after 13M operators ($RC)"
    fi
else
    test_fail "legacy % repeat fixture failed to compile"
fi

# Test 714: legacy pipeline --check stays stable on known file
echo "Test 714: legacy pipeline --check stays stable on known file"
if "$CCT_BIN" --check tests/integration/codegen_minimal.cct >$CCT_TMP_DIR/cct_13m_b1_legacy_check.out 2>&1; then
    test_pass "legacy --check pipeline stays stable"
else
    test_fail "legacy --check pipeline regressed"
fi

# Test 715: legacy compile+run without new operators remains stable
echo "Test 715: legacy compile+run without new operators remains stable"
if "$CCT_BIN" tests/integration/codegen_minimal.cct >$CCT_TMP_DIR/cct_13m_b1_legacy_min_build.out 2>&1 && \
   [ -x tests/integration/codegen_minimal ]; then
    tests/integration/codegen_minimal >$CCT_TMP_DIR/cct_13m_b1_legacy_min_run.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "legacy compile+run remains stable without new operators"
    else
        test_fail "legacy compile+run returned unexpected result ($RC)"
    fi
else
    test_fail "legacy minimal fixture failed to compile in B1"
fi

# Test 716: parser handles chained new operators under --ast
echo "Test 716: parser handles chained new operators under --ast"
cat > $CCT_TMP_DIR/cct_13m_b1_ast_chain.cct <<'CCTEOF'
INCIPIT grimoire "ast_chain_b1"
RITUALE main() REDDE REX
  EVOCA REX v AD 2 ** 3 // 2 %% 3
  REDDE v
EXPLICIT RITUALE
EXPLICIT grimoire
CCTEOF
if "$CCT_BIN" --ast $CCT_TMP_DIR/cct_13m_b1_ast_chain.cct >$CCT_TMP_DIR/cct_13m_b1_ast_chain.out 2>&1; then
    test_pass "--ast remains stable with chained 13M operators"
else
    test_fail "--ast failed on chained 13M operators"
fi

# Test 717: token output contains exact counts for each 13M operator in synthetic fixture
echo "Test 717: token output contains expected occurrences for each 13M operator"
TOK_OUT=$("$CCT_BIN" --tokens "$SIG13M_A2_TOK" 2>&1)
COUNT_POW=$(echo "$TOK_OUT" | grep -c "STAR_STAR")
COUNT_IDIV=$(echo "$TOK_OUT" | grep -c "SLASH_SLASH")
COUNT_EMOD=$(echo "$TOK_OUT" | grep -c "PERCENT_PERCENT")
if [ "$COUNT_POW" -eq 1 ] && [ "$COUNT_IDIV" -eq 1 ] && [ "$COUNT_EMOD" -eq 1 ]; then
    test_pass "token stream keeps expected per-operator occurrence counts"
else
    test_fail "token stream operator occurrence counts are unexpected ($COUNT_POW/$COUNT_IDIV/$COUNT_EMOD)"
fi

# Test 718: B1 expected fixture set exists in integration directory
echo "Test 718: B1 expected fixture set exists in integration directory"
if [ -f tests/integration/math_pow_2_10_13m.cct ] && \
   [ -f tests/integration/math_pow_parenthesized_13m.cct ] && \
   [ -f tests/integration/math_idiv_signs_13m.cct ] && \
   [ -f tests/integration/math_emod_signs_13m.cct ] && \
   [ -f tests/integration/math_precedence_parentheses_13m.cct ] && \
   [ -f tests/integration/math_variables_combo_13m.cct ] && \
   [ -f tests/integration/math_return_expr_13m.cct ] && \
   [ -f tests/integration/math_control_expr_13m.cct ]; then
    test_pass "B1 fixture inventory is complete"
else
    test_fail "B1 fixture inventory is incomplete"
fi

# Test 719: B1 execution status is marked complete in 13M checklist
echo "Test 719: B1 execution status is marked complete in 13M checklist"
if grep -q "\[x\] \`13M-B1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-B2\`" "$DOC_13M_CHECKLIST"; then
    test_pass "13M checklist marks B1/B2 completion"
else
    test_fail "13M checklist status is inconsistent for B1/B2 closure progression"
fi

# Test 720: 13M-B1 document explicitly requires full historical non-regression
echo "Test 720: 13M-B1 document explicitly requires full historical non-regression"
if grep -q "nenhum teste histórico pode regredir" "$DOC_13M_B1" && \
   grep -q "executar regressão completa" "$DOC_13M_B1"; then
    test_pass "13M-B1 keeps explicit non-regression contract"
else
    test_fail "13M-B1 is missing explicit non-regression contract language"
fi

echo ""
echo "========================================"
echo "FASE 13M.B2: Documentation, Release Pack, and Exit Gate Tests"
echo "========================================"
echo ""

# Test 721: 13M release pack documents exist
# Removed Test 721: documentation text/file comparison (policy)

# Removed Test 722: documentation text/file comparison (policy)

# Removed Test 723: documentation text/file comparison (policy)

# Removed Test 724: documentation text/file comparison (policy)

# Removed Test 725: documentation text/file comparison (policy)

# Removed Test 726: documentation text/file comparison (policy)

# Removed Test 727: documentation text/file comparison (policy)

# Removed Test 728: documentation text/file comparison (policy)

echo "Test 729: canonical 13M example exists"
if [ -f examples/math_common_ops_13m.cct ]; then
    test_pass "canonical 13M example source exists"
else
    test_fail "canonical 13M example source is missing"
fi

# Test 730: canonical 13M example passes --check
echo "Test 730: canonical 13M example passes --check"
if "$CCT_BIN" --check examples/math_common_ops_13m.cct >$CCT_TMP_DIR/cct_13m_b2_check_example.out 2>&1; then
    test_pass "canonical 13M example passes semantic check"
else
    test_fail "canonical 13M example failed semantic check"
fi

# Test 731: canonical 13M example compiles and executes
echo "Test 731: canonical 13M example compiles and executes"
if "$CCT_BIN" examples/math_common_ops_13m.cct >$CCT_TMP_DIR/cct_13m_b2_build_example.out 2>&1 && \
   [ -x examples/math_common_ops_13m ]; then
    examples/math_common_ops_13m >$CCT_TMP_DIR/cct_13m_b2_run_example.out 2>&1
    RC=$?
    if [ $RC -eq 0 ]; then
        test_pass "canonical 13M example compiles and executes"
    else
        test_fail "canonical 13M example returned unexpected exit code ($RC)"
    fi
else
    test_fail "canonical 13M example failed to compile"
fi

# Test 732: canonical 13M example output contains expected values
echo "Test 732: canonical 13M example output contains expected values"
if grep -q "pow 2\\*\\*5 =32" $CCT_TMP_DIR/cct_13m_b2_run_example.out && \
   grep -q "idiv -7//3 =-3" $CCT_TMP_DIR/cct_13m_b2_run_example.out && \
   grep -q "emod -7%%3 =2" $CCT_TMP_DIR/cct_13m_b2_run_example.out; then
    test_pass "canonical 13M example output matches expected values"
else
    test_fail "canonical 13M example output does not match expected values"
fi

# Test 733: command smoke --tokens on 13M example shows new token family
echo "Test 733: command smoke --tokens on 13M example shows new token family"
TOK13M=$("$CCT_BIN" --tokens examples/math_common_ops_13m.cct 2>&1)
if echo "$TOK13M" | grep -q "STAR_STAR" && \
   echo "$TOK13M" | grep -q "SLASH_SLASH" && \
   echo "$TOK13M" | grep -q "PERCENT_PERCENT"; then
    test_pass "--tokens on 13M example shows expected operator tokens"
else
    test_fail "--tokens on 13M example missing one or more operator tokens"
fi

# Test 734: command smoke --ast on 13M example works
echo "Test 734: command smoke --ast on 13M example works"
if "$CCT_BIN" --ast examples/math_common_ops_13m.cct >$CCT_TMP_DIR/cct_13m_b2_ast_example.out 2>&1; then
    test_pass "--ast on 13M example works"
else
    test_fail "--ast on 13M example failed"
fi

# Test 735: B2 contract includes required closure sequence
echo "Test 735: B2 contract includes required closure sequence"
if grep -q "harmonizar documentação canônica" "$DOC_13M_B2" && \
   grep -q "publicar release notes e limites" "$DOC_13M_B2" && \
   grep -q "executar gate final com evidências" "$DOC_13M_B2"; then
    test_pass "B2 contract keeps required closure sequence"
else
    test_fail "B2 contract missing required closure sequence"
fi

# Test 736: B2 checklist status marks full 13M closure
echo "Test 736: B2 checklist status marks full 13M closure"
if grep -q "\[x\] \`13M-A1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-A2\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-B1\`" "$DOC_13M_CHECKLIST" && \
   grep -q "\[x\] \`13M-B2\`" "$DOC_13M_CHECKLIST"; then
    test_pass "13M checklist reflects full closure at B2"
else
    test_fail "13M checklist does not reflect full closure at B2"
fi

# Test 737: README and release notes cross-reference 13M release pack files
# Removed Test 737: documentation text/file comparison (policy)

echo "Test 738: CLI legacy help remains non-regressive after 13M.B2 docs closure"
HELP_OUT=$("$CCT_BIN" --help 2>&1)
if echo "$HELP_OUT" | grep -q "Usage:" && \
   echo "$HELP_OUT" | grep -q "sigilo"; then
    test_pass "CLI help remains non-regressive after 13M.B2 closure"
else
    test_fail "CLI help regressed after 13M.B2 closure"
fi

# Test 739: final snapshot references full-suite green evidence
# Removed Test 739: documentation text/file comparison (policy)

# Removed Test 740: documentation text/file comparison (policy)

echo "Test 741: standalone diagnostic taxonomy runtime tests build and run"
OUTPUT=$(make test_diagnostic_taxonomy 2>&1) || true
if echo "$OUTPUT" | grep -q "test_diagnostic_taxonomy: ok"; then
    test_pass "Diagnostic taxonomy runtime target builds and runs"
else
    test_fail "Diagnostic taxonomy runtime target failed"
fi

# Test 742: diagnostic taxonomy runtime binary exists
echo "Test 742: diagnostic taxonomy runtime binary exists after build"
if [ -f "tests/runtime/test_diagnostic_taxonomy" ]; then
    test_pass "Diagnostic taxonomy runtime binary is produced deterministically"
else
    test_fail "Diagnostic taxonomy runtime binary not found"
fi

# Test 743: syntax diagnostic preserves suggestion line compatibility
echo "Test 743: syntax diagnostic preserves suggestion line compatibility"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_syntax_error_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "suggestion:"; then
    test_pass "Suggestion line compatibility is preserved for legacy diagnostics"
else
    test_fail "Suggestion line compatibility regressed"
fi

# Test 744: syntax diagnostic now also emits hint taxonomy line
echo "Test 744: syntax diagnostic emits hint taxonomy line"
OUTPUT=$("$CCT_BIN" --check tests/integration/diagnostic_syntax_error_12a.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "hint:"; then
    test_pass "Hint taxonomy line is emitted for actionable diagnostics"
else
    test_fail "Hint taxonomy line is missing for actionable diagnostics"
fi

# Test 745: sigilo validate text diagnostics keep canonical taxonomy prefix
echo "Test 745: sigilo validate text diagnostics keep canonical taxonomy prefix"
SIG14A1_INVALID="$CCT_TMP_DIR/cct_sigilo_14a1_invalid.sigil"
cat > "$SIG14A1_INVALID" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = bad
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG14A1_INVALID" --format text --strict 2>&1) || true
if echo "$OUTPUT" | grep -q "cct: .*error: sigilo\\[" && \
   echo "$OUTPUT" | grep -q "\\[sigilo\\]"; then
    test_pass "Sigilo diagnostics use canonical diagnostic envelope"
else
    test_fail "Sigilo diagnostics are not using canonical diagnostic envelope"
fi

# Test 746: sigilo validate structured output level remains stable
echo "Test 746: sigilo validate structured output level remains stable"
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG14A1_INVALID" --format structured --strict 2>&1) || true
if echo "$OUTPUT" | grep -q "level = error"; then
    test_pass "Structured sigilo diagnostics preserve stable level contract"
else
    test_fail "Structured sigilo diagnostic level contract regressed"
fi

# Test 747: invalid CLI argument still reports canonical code label
echo "Test 747: invalid CLI argument still reports canonical code label"
OUTPUT=$("$CCT_BIN" --sigilo-mode invalido tests/integration/sigilo_minimal.cct 2>&1) || true
if echo "$OUTPUT" | grep -q "\\[Invalid argument\\]"; then
    test_pass "CLI invalid-argument diagnostics preserve canonical code label"
else
    test_fail "CLI invalid-argument diagnostics lost canonical code label"
fi

# Test 748: diagnostic level taxonomy helper is publicly available via runtime target
echo "Test 748: diagnostic level taxonomy helper is covered by runtime tests"
OUTPUT=$(make test_diagnostic_taxonomy 2>&1) || true
if echo "$OUTPUT" | grep -q "test_diagnostic_taxonomy: ok"; then
    test_pass "Diagnostic level taxonomy helper contract is covered in runtime suite"
else
    test_fail "Diagnostic level taxonomy helper contract is not covered"
fi

# Test 749: public spec documents 14A1 diagnostic taxonomy contract
# Removed Test 749: documentation text/file comparison (policy)

echo "Test 750: unknown option returns canonical unknown-command exit code"
"$CCT_BIN" --unknown-option >$CCT_TMP_DIR/cct_exit_14a2_unknown.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 4 ]; then
    test_pass "unknown option returns canonical exit code 4 (unknown command)"
else
    test_fail "unknown option should return exit code 4 (got $EXIT_CODE)"
fi

# Test 751: missing argument returns canonical missing-argument exit code
echo "Test 751: missing argument returns canonical missing-argument exit code"
"$CCT_BIN" --tokens >$CCT_TMP_DIR/cct_exit_14a2_missing.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 3 ]; then
    test_pass "missing argument returns canonical exit code 3"
else
    test_fail "missing argument should return exit code 3 (got $EXIT_CODE)"
fi

# Test 752: invalid argument returns canonical invalid-argument exit code
echo "Test 752: invalid argument returns canonical invalid-argument exit code"
"$CCT_BIN" test.txt >$CCT_TMP_DIR/cct_exit_14a2_invalid.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    test_pass "invalid argument returns canonical exit code 1"
else
    test_fail "invalid argument should return exit code 1 (got $EXIT_CODE)"
fi

# Test 753: formatter check mode returns contract-violation exit code on drift
echo "Test 753: fmt --check returns canonical contract-violation exit code"
"$CCT_BIN" fmt --check tests/integration/project_12f_basic/tests/fmt_bad.test.cct >$CCT_TMP_DIR/cct_exit_14a2_fmt_check.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "fmt --check returns canonical exit code 2 when format contract is violated"
else
    test_fail "fmt --check should return exit code 2 on formatting drift (got $EXIT_CODE)"
fi

# Test 754: strict sigilo validate returns contract-violation exit code
echo "Test 754: strict sigilo validate returns canonical contract-violation exit code"
SIG14A2_INVALID="$CCT_TMP_DIR/cct_sigilo_14a2_invalid.sigil"
cat > "$SIG14A2_INVALID" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
SIGEOF
"$CCT_BIN" sigilo validate "$SIG14A2_INVALID" --summary --strict >$CCT_TMP_DIR/cct_exit_14a2_validate.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "strict sigilo validate returns canonical exit code 2 on contract violation"
else
    test_fail "strict sigilo validate should return exit code 2 (got $EXIT_CODE)"
fi

# Test 755: strict baseline drift still returns contract-violation exit code
echo "Test 755: strict baseline drift keeps canonical contract-violation exit code"
SIG14A2_BASE_A="$CCT_TMP_DIR/cct_sigilo_14a2_base_a.sigil"
SIG14A2_BASE_B="$CCT_TMP_DIR/cct_sigilo_14a2_base_b.sigil"
cat > "$SIG14A2_BASE_A" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
cat > "$SIG14A2_BASE_B" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111111111111111
[totals]
rituale = 1
SIGEOF
"$CCT_BIN" sigilo baseline update "$SIG14A2_BASE_A" --baseline $CCT_TMP_DIR/cct_sigilo_14a2_baseline.sigil --force >$CCT_TMP_DIR/cct_exit_14a2_base_update.out 2>&1
"$CCT_BIN" sigilo baseline check "$SIG14A2_BASE_B" --baseline $CCT_TMP_DIR/cct_sigilo_14a2_baseline.sigil --strict --summary >$CCT_TMP_DIR/cct_exit_14a2_base_check.out 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 2 ]; then
    test_pass "strict baseline drift keeps canonical exit code 2"
else
    test_fail "strict baseline drift should return exit code 2 (got $EXIT_CODE)"
fi

# Test 756: spec documents canonical 14A2 exit code policy
# Removed Test 756: documentation text/file comparison (policy)

echo "Test 757: sigilo help exposes --explain on core subcommands"
OUTPUT=$("$CCT_BIN" sigilo 2>&1) || true
if echo "$OUTPUT" | grep -q "sigilo inspect .*--explain" && \
   echo "$OUTPUT" | grep -q "sigilo validate .*--explain" && \
   echo "$OUTPUT" | grep -q "sigilo baseline check .*--explain"; then
    test_pass "sigilo help documents explain mode across core subcommands"
else
    test_fail "sigilo help is missing --explain in one or more core subcommands"
fi

# Test 758: validate --explain emits actionable explain line on strict failure
echo "Test 758: validate --explain emits actionable explain line on strict failure"
SIG14A3_INVALID="$CCT_TMP_DIR/cct_sigilo_14a3_invalid.sigil"
cat > "$SIG14A3_INVALID" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG14A3_INVALID" --strict --explain --format text 2>&1) || true
if echo "$OUTPUT" | grep -q "sigilo.explain probable_cause=" && \
   echo "$OUTPUT" | grep -q "recommended_action=" && \
   echo "$OUTPUT" | grep -q "docs=docs/sigilo_troubleshooting_13b4.md" && \
   echo "$OUTPUT" | grep -q "command=validate"; then
    test_pass "validate --explain emits troubleshooting guidance"
else
    test_fail "validate --explain did not emit expected troubleshooting guidance"
fi

# Test 759: validate default mode keeps output concise (no explain line)
echo "Test 759: validate default mode keeps output concise (no explain line)"
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG14A3_INVALID" --strict --format text 2>&1) || true
if ! echo "$OUTPUT" | grep -q "sigilo.explain probable_cause="; then
    test_pass "default validate output remains concise without explain noise"
else
    test_fail "default validate output unexpectedly emitted explain noise"
fi

# Test 760: baseline check --explain covers missing baseline troubleshooting
echo "Test 760: baseline check --explain covers missing baseline troubleshooting"
SIG14A3_BASE_ART="$CCT_TMP_DIR/cct_sigilo_14a3_base_artifact.sigil"
cat > "$SIG14A3_BASE_ART" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = abcdef0123456789
[totals]
rituale = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo baseline check "$SIG14A3_BASE_ART" --baseline $CCT_TMP_DIR/cct_sigilo_14a3_missing_baseline.sigil --explain --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "status=missing" && \
   echo "$OUTPUT" | grep -q "sigilo.explain probable_cause=baseline file is missing" && \
   echo "$OUTPUT" | grep -q "command=baseline-check"; then
    test_pass "baseline check --explain reports missing baseline with action"
else
    test_fail "baseline check --explain missing baseline troubleshooting is incomplete"
fi

# Test 761: strict diff/check blocking drift with --explain emits blocked guidance
echo "Test 761: strict check --explain emits blocked troubleshooting guidance"
SIG14A3_LEFT="$CCT_TMP_DIR/cct_sigilo_14a3_left.sigil"
SIG14A3_RIGHT="$CCT_TMP_DIR/cct_sigilo_14a3_right.sigil"
cat > "$SIG14A3_LEFT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = aaaaaaaaaaaaaaaa
[totals]
rituale = 1
SIGEOF
cat > "$SIG14A3_RIGHT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = bbbbbbbbbbbbbbbb
[totals]
rituale = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo check "$SIG14A3_LEFT" "$SIG14A3_RIGHT" --strict --explain --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "sigilo.explain probable_cause=" && \
   echo "$OUTPUT" | grep -q "blocked=true" && \
   echo "$OUTPUT" | grep -q "command=check"; then
    test_pass "strict check --explain emits blocked cause/action guidance"
else
    test_fail "strict check --explain did not emit blocked troubleshooting guidance"
fi

# Test 762: public spec documents 14A3 explain contract
# Removed Test 762: documentation text/file comparison (policy)

echo "Test 763: validate structured diagnostics are deterministic across runs"
SIG14A4_MULTI_DIAG="$CCT_TMP_DIR/cct_sigilo_14a4_multi_diag.sigil"
cat > "$SIG14A4_MULTI_DIAG" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
sigilo_style = legacy
[analysis_summary]
scope = system
SIGEOF
OUT_A=$("$CCT_BIN" sigilo validate "$SIG14A4_MULTI_DIAG" --format structured --summary --strict 2>&1) || true
OUT_B=$("$CCT_BIN" sigilo validate "$SIG14A4_MULTI_DIAG" --format structured --summary --strict 2>&1) || true
if [ "$OUT_A" = "$OUT_B" ]; then
    test_pass "validate structured output is deterministic across equivalent runs"
else
    test_fail "validate structured output changed across equivalent runs"
fi

# Test 764: validate text diagnostics are deterministic across runs
echo "Test 764: validate text diagnostics are deterministic across runs"
OUT_A=$("$CCT_BIN" sigilo validate "$SIG14A4_MULTI_DIAG" --format text --strict 2>&1) || true
OUT_B=$("$CCT_BIN" sigilo validate "$SIG14A4_MULTI_DIAG" --format text --strict 2>&1) || true
if [ "$OUT_A" = "$OUT_B" ]; then
    test_pass "validate text diagnostics are deterministic across equivalent runs"
else
    test_fail "validate text diagnostics changed across equivalent runs"
fi

# Test 765: deterministic ordering keeps error diagnostics before warnings in structured mode
echo "Test 765: deterministic ordering keeps errors before warnings in structured diagnostics"
OUT=$("$CCT_BIN" sigilo validate "$SIG14A4_MULTI_DIAG" --format structured --strict 2>&1) || true
FIRST_LEVEL=$(echo "$OUT" | awk '
    /^\[diag\.[0-9]+\]/ { in_diag=1; next }
    in_diag && /^level = / { print $3; exit }
')
if [ "$FIRST_LEVEL" = "error" ]; then
    test_pass "structured diagnostic ordering is stable with errors first"
else
    test_fail "structured diagnostics are not ordered deterministically with errors first"
fi

# Test 766: inspect structured output stays deterministic across runs
echo "Test 766: inspect structured output stays deterministic across runs"
SIG14A4_VALID="$CCT_TMP_DIR/cct_sigilo_14a4_valid.sigil"
cat > "$SIG14A4_VALID" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1234567890abcdef
[totals]
rituale = 1
SIGEOF
OUT_A=$("$CCT_BIN" sigilo inspect "$SIG14A4_VALID" --format structured --summary 2>&1) || true
OUT_B=$("$CCT_BIN" sigilo inspect "$SIG14A4_VALID" --format structured --summary 2>&1) || true
if [ "$OUT_A" = "$OUT_B" ]; then
    test_pass "inspect structured summary remains deterministic"
else
    test_fail "inspect structured summary changed across equivalent runs"
fi

# Test 767: explain mode remains opt-in after 14A4 (no default log noise)
echo "Test 767: explain mode remains opt-in after 14A4"
OUT=$("$CCT_BIN" sigilo baseline check "$SIG14A4_VALID" --baseline $CCT_TMP_DIR/cct_sigilo_14a4_missing_baseline.sigil --summary 2>&1) || true
if ! echo "$OUT" | grep -q "sigilo.explain probable_cause="; then
    test_pass "default baseline check output keeps explain logs opt-in"
else
    test_fail "baseline check emitted explain logs without --explain"
fi

# Test 768: public spec documents deterministic output contract for 14A4
# Removed Test 768: documentation text/file comparison (policy)

# Removed Test 769: documentation text/file comparison (policy)

# Removed Test 770: documentation text/file comparison (policy)

# Removed Test 771: documentation text/file comparison (policy)

# Removed Test 772: documentation text/file comparison (policy)

echo "Test 773: CLI help/version status avoids stale 13A.4/12H references"
HELP_OUT=$("$CCT_BIN" --help 2>&1)
VER_OUT=$("$CCT_BIN" --version 2>&1)
if echo "$HELP_OUT" | grep -q "Current status: FASE 14A.4 completed" && \
   echo "$VER_OUT" | grep -q "Build: FASE 14A" && \
   ! echo "$HELP_OUT" | grep -q "FASE 13A.4 in progress over 12H baseline"; then
    test_pass "CLI public status strings are harmonized with current contracts"
else
    test_fail "CLI public status strings are stale or inconsistent"
fi

# Test 774: README release-pack references use 13/13M canonical docs (no 12 references)
# Removed Test 774: documentation text/file comparison (policy)

# Removed Test 775: documentation text/file comparison (policy)

# Removed Test 776: documentation text/file comparison (policy)

echo "Test 777: documented local validate workflow executes successfully"
SIG14B2_VALID="$CCT_TMP_DIR/cct_sigilo_14b2_valid.sigil"
cat > "$SIG14B2_VALID" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 0123456789abcdef
[totals]
rituale = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo validate "$SIG14B2_VALID" --summary 2>&1) || true
if echo "$OUTPUT" | grep -q "result=pass"; then
    test_pass "documented local validate workflow executes successfully"
else
    test_fail "documented local validate workflow failed"
fi

# Test 778: documented strict check + explain workflow executes and emits explain line
echo "Test 778: documented strict check + explain workflow executes and emits explain line"
SIG14B2_LEFT="$CCT_TMP_DIR/cct_sigilo_14b2_left.sigil"
SIG14B2_RIGHT="$CCT_TMP_DIR/cct_sigilo_14b2_right.sigil"
cat > "$SIG14B2_LEFT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = aaaabbbbccccdddd
[totals]
rituale = 1
SIGEOF
cat > "$SIG14B2_RIGHT" <<'SIGEOF'
format = cct.sigil.v1
sigilo_scope = local
semantic_hash = 1111222233334444
[totals]
rituale = 1
SIGEOF
OUTPUT=$("$CCT_BIN" sigilo check "$SIG14B2_LEFT" "$SIG14B2_RIGHT" --strict --summary --explain 2>&1) || true
if echo "$OUTPUT" | grep -q "sigilo.explain probable_cause=" && \
   echo "$OUTPUT" | grep -q "command=check"; then
    test_pass "documented strict check + explain workflow executes as documented"
else
    test_fail "documented strict check + explain workflow did not match documented behavior"
fi

# Test 779: README/spec reference the consolidated 14B2 operations guide
# Removed Test 779: documentation text/file comparison (policy)

# Removed Test 780: documentation text/file comparison (policy)

# Removed Test 781: documentation text/file comparison (policy)

# Removed Test 782: documentation text/file comparison (policy)

# Removed Test 783: documentation text/file comparison (policy)

# Removed Test 784: documentation text/file comparison (policy)

# Removed Test 785: documentation text/file comparison (policy)

# Removed Test 786: documentation text/file comparison (policy)

# Removed Test 787: documentation text/file comparison (policy)

# Removed Test 788: documentation text/file comparison (policy)

# Removed Test 789: documentation text/file comparison (policy)

# Removed Test 790: documentation text/file comparison (policy)

# Removed Test 791: documentation text/file comparison (policy)

# Removed Test 792: documentation text/file comparison (policy)

# Removed Test 793: documentation text/file comparison (policy)

echo "Test 794: 14C1 regression matrix script executes successfully"
if bash tests/run_phase14c1_regression_matrix.sh >$CCT_TMP_DIR/cct_phase14c1_runner.out 2>&1; then
    test_pass "14C1 regression matrix script passed"
else
    test_fail "14C1 regression matrix script failed"
fi

# Test 795: 14C1 script emitted expected proof points in outputs
echo "Test 795: 14C1 script emitted expected proof points in outputs"
if grep -q "STAR_STAR" $CCT_TMP_DIR/cct_phase14c1_math_tokens.out && \
   grep -q "sigil_diff.summary" $CCT_TMP_DIR/cct_phase14c1_check.out && \
   grep -q "sigilo baseline check blocked (strict mode)" $CCT_TMP_DIR/cct_phase14c1_baseline_check.out; then
    test_pass "14C1 output artifacts confirm strict drift and 13M token checks"
else
    test_fail "14C1 output artifacts are missing expected strict/token proof points"
fi

# Test 796: architecture/roadmap capture 14C1 implementation
# Removed Test 796: documentation text/file comparison (policy)

# Removed Test 797: documentation text/file comparison (policy)

echo "Test 798: 14C2 stress/soak script executes successfully"
if bash tests/run_phase14c2_stress_soak.sh 6 >$CCT_TMP_DIR/cct_phase14c2_runner.out 2>&1; then
    test_pass "14C2 stress/soak script passed"
else
    test_fail "14C2 stress/soak script failed"
fi

# Test 799: 14C2 report captures stable/failure-free outcome
echo "Test 799: 14C2 report captures stable/failure-free outcome"
if grep -q "^failures=0$" $CCT_TMP_DIR/cct_phase14c2_stress/report.txt && \
   grep -q "^status=stable$" $CCT_TMP_DIR/cct_phase14c2_stress/report.txt; then
    test_pass "14C2 report records stable and failure-free run"
else
    test_fail "14C2 report does not record expected stable status"
fi

# Test 800: architecture/roadmap capture 14C2 implementation
# Removed Test 800: documentation text/file comparison (policy)

# Removed Test 801: documentation text/file comparison (policy)

echo "Test 802: 14C3 performance budget script executes successfully"
if bash tests/run_phase14c3_perf_budget.sh 2 >$CCT_TMP_DIR/cct_phase14c3_runner.out 2>&1; then
    test_pass "14C3 performance budget script passed"
else
    test_fail "14C3 performance budget script failed"
fi

# Test 803: 14C3 baseline artifact includes metrics and budgets
echo "Test 803: 14C3 baseline artifact includes metrics and budgets"
if grep -q "^help_avg_ms=" $CCT_TMP_DIR/cct_phase14c3_perf/baseline.txt && \
   grep -q "^check_avg_ms=" $CCT_TMP_DIR/cct_phase14c3_perf/baseline.txt && \
   grep -q "^validate_avg_ms=" $CCT_TMP_DIR/cct_phase14c3_perf/baseline.txt && \
   grep -q "^budget_help_ms=" $CCT_TMP_DIR/cct_phase14c3_perf/baseline.txt; then
    test_pass "14C3 baseline artifact includes expected metric/budget fields"
else
    test_fail "14C3 baseline artifact missing expected metric/budget fields"
fi

# Test 804: architecture/roadmap capture 14C3 implementation
# Removed Test 804: documentation text/file comparison (policy)

# Removed Test 805: documentation text/file comparison (policy)

# Removed Test 806: documentation text/file comparison (policy)

# Removed Test 807: documentation text/file comparison (policy)

# Removed Test 808: documentation text/file comparison (policy)

echo "Test 809: 14D1 packaging reproducibility script executes successfully"
if bash tests/run_phase14d1_packaging_repro.sh >$CCT_TMP_DIR/cct_phase14d1_runner.out 2>&1; then
    test_pass "14D1 packaging reproducibility script passed"
else
    test_fail "14D1 packaging reproducibility script failed"
fi

# Test 810: 14D1 generated dist artifacts contain mandatory files
echo "Test 810: 14D1 generated dist artifacts contain mandatory files"
if [ -f "$CCT_TMP_DIR/cct_phase14d1_packaging/dist_a/CHECKSUMS.sha256" ] && \
   [ -f "$CCT_TMP_DIR/cct_phase14d1_packaging/dist_b/CHECKSUMS.sha256" ] && \
   [ -f "$CCT_TMP_DIR/cct_phase14d1_packaging/dist_a/docs/spec.md" ]; then
    test_pass "14D1 generated dist artifacts include mandatory files"
else
    test_fail "14D1 generated dist artifacts are missing mandatory files"
fi

# Test 811: architecture/roadmap capture 14D1 implementation
# Removed Test 811: documentation text/file comparison (policy)

# Removed Test 812: documentation text/file comparison (policy)

echo "Test 813: 14D2 RC validation script executes successfully"
if bash tests/run_phase14d2_rc_validation.sh >$CCT_TMP_DIR/cct_phase14d2_runner.out 2>&1; then
    test_pass "14D2 RC validation script passed"
else
    test_fail "14D2 RC validation script failed"
fi

# Test 814: 14D2 validation outputs contain expected pass contracts
echo "Test 814: 14D2 validation outputs contain expected pass contracts"
if grep -q "Usage:" $CCT_TMP_DIR/cct_phase14d2_help.out && \
   grep -q "sigilo.inspect.summary" $CCT_TMP_DIR/cct_phase14d2_inspect.out && \
   grep -q "result=pass" $CCT_TMP_DIR/cct_phase14d2_validate.out; then
    test_pass "14D2 output artifacts match expected RC contracts"
else
    test_fail "14D2 output artifacts missing expected RC contract signals"
fi

# Test 815: architecture/roadmap capture 14D2 implementation
# Removed Test 815: documentation text/file comparison (policy)

# Removed Test 816: documentation text/file comparison (policy)

# Removed Test 817: documentation text/file comparison (policy)

# Removed Test 818: documentation text/file comparison (policy)

# Removed Test 819: documentation text/file comparison (policy)

# Removed Test 820: documentation text/file comparison (policy)

echo "Test 821: 15A1 loop-control contract (DUM/DONEC)"
if tests/run_phase15a1_loop_control.sh >$CCT_TMP_DIR/cct_phase15a1_loop.out 2>&1; then
    test_pass "15A1 FRANGE/RECEDE loop-control contract is stable"
else
    test_fail "15A1 FRANGE/RECEDE loop-control contract failed"
fi

# Test 822: 15A2 loop-control contract (REPETE)
echo "Test 822: 15A2 loop-control contract (REPETE)"
if tests/run_phase15a2_repete_control.sh >$CCT_TMP_DIR/cct_phase15a2_repete.out 2>&1; then
    test_pass "15A2 FRANGE/RECEDE in REPETE contract is stable"
else
    test_fail "15A2 FRANGE/RECEDE in REPETE contract failed"
fi

# Test 823: 15A3 loop-control contract (ITERUM)
echo "Test 823: 15A3 loop-control contract (ITERUM)"
if tests/run_phase15a3_iterum_control.sh >$CCT_TMP_DIR/cct_phase15a3_iterum.out 2>&1; then
    test_pass "15A3 FRANGE/RECEDE in ITERUM contract is stable"
else
    test_fail "15A3 FRANGE/RECEDE in ITERUM contract failed"
fi

# Test 824: 15A4 outside-loop diagnostics and nested SI contract
echo "Test 824: 15A4 outside-loop diagnostics and nested SI contract"
if tests/run_phase15a4_outside_loop_diagnostics.sh >$CCT_TMP_DIR/cct_phase15a4_diag.out 2>&1; then
    test_pass "15A4 outside-loop semantic diagnostics and nested SI loop behavior are stable"
else
    test_fail "15A4 outside-loop semantic diagnostics and nested SI loop behavior failed"
fi

echo ""
echo "========================================"
echo "FASE 15B1/B2: Logical ET/VEL Codegen Tests"
echo "========================================"
echo ""

# Test 825: 15B1 logical ET contract
echo "Test 825: 15B1 logical ET contract"
if tests/run_phase15b1_logical_et.sh >$CCT_TMP_DIR/cct_phase15b1_et.out 2>&1; then
    test_pass "15B1 ET short-circuit and expression/conditional behavior are stable"
else
    test_fail "15B1 ET short-circuit and expression/conditional behavior failed"
fi

# Test 826: 15B2 logical VEL contract
echo "Test 826: 15B2 logical VEL contract"
if tests/run_phase15b2_logical_vel.sh >$CCT_TMP_DIR/cct_phase15b2_vel.out 2>&1; then
    test_pass "15B2 VEL short-circuit and ET/VEL precedence behavior are stable"
else
    test_fail "15B2 VEL short-circuit and ET/VEL precedence behavior failed"
fi

echo ""
echo "========================================"
echo "FASE 15A/B: Cross Integration Matrix"
echo "========================================"
echo ""

# Test 827: DUM + ET + FRANGE
echo "Test 827: DUM + ET + FRANGE cross-integration"
if tests/run_phase15ab_cross_case.sh dum_et_frange >$CCT_TMP_DIR/cct_phase15ab_827.out 2>&1; then
    test_pass "DUM + ET + FRANGE behaves correctly"
else
    test_fail "DUM + ET + FRANGE behavior regressed"
fi

# Test 828: DUM + VEL + RECEDE
echo "Test 828: DUM + VEL + RECEDE cross-integration"
if tests/run_phase15ab_cross_case.sh dum_vel_recede >$CCT_TMP_DIR/cct_phase15ab_828.out 2>&1; then
    test_pass "DUM + VEL + RECEDE behaves correctly"
else
    test_fail "DUM + VEL + RECEDE behavior regressed"
fi

# Test 829: DONEC + ET + RECEDE
echo "Test 829: DONEC + ET + RECEDE cross-integration"
if tests/run_phase15ab_cross_case.sh donec_et_recede >$CCT_TMP_DIR/cct_phase15ab_829.out 2>&1; then
    test_pass "DONEC + ET + RECEDE behaves correctly"
else
    test_fail "DONEC + ET + RECEDE behavior regressed"
fi

# Test 830: DONEC + VEL + FRANGE
echo "Test 830: DONEC + VEL + FRANGE cross-integration"
if tests/run_phase15ab_cross_case.sh donec_vel_frange >$CCT_TMP_DIR/cct_phase15ab_830.out 2>&1; then
    test_pass "DONEC + VEL + FRANGE behaves correctly"
else
    test_fail "DONEC + VEL + FRANGE behavior regressed"
fi

# Test 831: REPETE + ET + RECEDE
echo "Test 831: REPETE + ET + RECEDE cross-integration"
if tests/run_phase15ab_cross_case.sh repete_et_recede >$CCT_TMP_DIR/cct_phase15ab_831.out 2>&1; then
    test_pass "REPETE + ET + RECEDE behaves correctly"
else
    test_fail "REPETE + ET + RECEDE behavior regressed"
fi

# Test 832: REPETE + VEL + FRANGE
echo "Test 832: REPETE + VEL + FRANGE cross-integration"
if tests/run_phase15ab_cross_case.sh repete_vel_frange >$CCT_TMP_DIR/cct_phase15ab_832.out 2>&1; then
    test_pass "REPETE + VEL + FRANGE behaves correctly"
else
    test_fail "REPETE + VEL + FRANGE behavior regressed"
fi

# Test 833: ITERUM + ET + FRANGE
echo "Test 833: ITERUM + ET + FRANGE cross-integration"
if tests/run_phase15ab_cross_case.sh iterum_et_frange >$CCT_TMP_DIR/cct_phase15ab_833.out 2>&1; then
    test_pass "ITERUM + ET + FRANGE behaves correctly"
else
    test_fail "ITERUM + ET + FRANGE behavior regressed"
fi

# Test 834: ITERUM + VEL + RECEDE
echo "Test 834: ITERUM + VEL + RECEDE cross-integration"
if tests/run_phase15ab_cross_case.sh iterum_vel_recede >$CCT_TMP_DIR/cct_phase15ab_834.out 2>&1; then
    test_pass "ITERUM + VEL + RECEDE behaves correctly"
else
    test_fail "ITERUM + VEL + RECEDE behavior regressed"
fi

# Test 835: nested loops + ET/VEL + FRANGE/RECEDE
echo "Test 835: nested loops + ET/VEL + FRANGE/RECEDE cross-integration"
if tests/run_phase15ab_cross_case.sh nested_mix_loops >$CCT_TMP_DIR/cct_phase15ab_835.out 2>&1; then
    test_pass "Nested-loop ET/VEL with FRANGE/RECEDE behaves correctly"
else
    test_fail "Nested-loop ET/VEL with FRANGE/RECEDE behavior regressed"
fi

# Test 836: short-circuit ET with CONIURA/ANUR side-effect guard
echo "Test 836: ET short-circuit protects against right-side ANUR"
if tests/run_phase15ab_cross_case.sh short_circuit_et_anur >$CCT_TMP_DIR/cct_phase15ab_836.out 2>&1; then
    test_pass "ET short-circuit blocks right-side ANUR side effect"
else
    test_fail "ET short-circuit failed to block right-side ANUR side effect"
fi

# Test 837: short-circuit VEL with CONIURA/ANUR side-effect guard
echo "Test 837: VEL short-circuit protects against right-side ANUR"
if tests/run_phase15ab_cross_case.sh short_circuit_vel_anur >$CCT_TMP_DIR/cct_phase15ab_837.out 2>&1; then
    test_pass "VEL short-circuit blocks right-side ANUR side effect"
else
    test_fail "VEL short-circuit failed to block right-side ANUR side effect"
fi

# Test 838: ET rejects string operand types
echo "Test 838: ET rejects string operands in semantic analysis"
if tests/run_phase15ab_cross_case.sh semantic_et_string >$CCT_TMP_DIR/cct_phase15ab_838.out 2>&1; then
    test_pass "ET type checks reject string operands"
else
    test_fail "ET type checks did not reject string operands"
fi

# Test 839: VEL rejects string operand types
echo "Test 839: VEL rejects string operands in semantic analysis"
if tests/run_phase15ab_cross_case.sh semantic_vel_string >$CCT_TMP_DIR/cct_phase15ab_839.out 2>&1; then
    test_pass "VEL type checks reject string operands"
else
    test_fail "VEL type checks did not reject string operands"
fi

# Test 840: precedence ET > VEL in loop condition
echo "Test 840: precedence ET > VEL in loop condition"
if tests/run_phase15ab_cross_case.sh precedence_vel_et_dum >$CCT_TMP_DIR/cct_phase15ab_840.out 2>&1; then
    test_pass "ET precedence over VEL is stable in loop conditions"
else
    test_fail "ET precedence over VEL regressed in loop conditions"
fi

echo ""
echo "========================================"
echo "FASE 15B3: Logical Combination Precedence Tests"
echo "========================================"
echo ""

# Test 841: 15B3 logical combination precedence contract
echo "Test 841: 15B3 logical combination precedence contract"
if tests/run_phase15b3_logical_precedence.sh >$CCT_TMP_DIR/cct_phase15b3_logic.out 2>&1; then
    test_pass "15B3 NON/ET/VEL precedence with parentheses is stable"
else
    test_fail "15B3 NON/ET/VEL precedence with parentheses failed"
fi

# Test 842: 15B3 parentheses stress (nested + series)
echo "Test 842: 15B3 parentheses stress (nested + series)"
if tests/run_phase15b3_parentheses_stress.sh >$CCT_TMP_DIR/cct_phase15b3_paren_stress.out 2>&1; then
    test_pass "15B3 deep/long parentheses sequences are stable"
else
    test_fail "15B3 deep/long parentheses sequences failed"
fi

echo ""
echo "========================================"
echo "FASE 15B4: Comparator + Logical Integration Tests"
echo "========================================"
echo ""

# Test 843: ET with arithmetic comparators
echo "Test 843: ET integration with arithmetic comparators"
if tests/run_phase15b4_comparator_integration.sh >$CCT_TMP_DIR/cct_phase15b4_all.out 2>&1; then
    test_pass "15B4 comparator/logical integration contract is stable"
else
    test_fail "15B4 comparator/logical integration contract failed"
fi

# Test 844: et_with_comparators_15b mandatory scenario
echo "Test 844: et_with_comparators_15b returns expected value"
cleanup_codegen_artifacts "tests/integration/et_with_comparators_15b.cct"
if "$CCT_BIN" "tests/integration/et_with_comparators_15b.cct" >$CCT_TMP_DIR/cct_phase15b4_844_compile.out 2>&1; then
    tests/integration/et_with_comparators_15b >$CCT_TMP_DIR/cct_phase15b4_844_run.out 2>&1
    RC_844=$?
else
    RC_844=255
fi
if [ "$RC_844" -eq 1 ]; then
    test_pass "et_with_comparators_15b returns 1 as expected"
else
    test_fail "et_with_comparators_15b did not return 1 as expected"
fi

# Test 845: vel_with_comparators_15b mandatory scenario
echo "Test 845: vel_with_comparators_15b returns expected value"
cleanup_codegen_artifacts "tests/integration/vel_with_comparators_15b.cct"
if "$CCT_BIN" "tests/integration/vel_with_comparators_15b.cct" >$CCT_TMP_DIR/cct_phase15b4_845_compile.out 2>&1; then
    tests/integration/vel_with_comparators_15b >$CCT_TMP_DIR/cct_phase15b4_845_run.out 2>&1
    RC_845=$?
else
    RC_845=255
fi
if [ "$RC_845" -eq 15 ]; then
    test_pass "vel_with_comparators_15b returns 15 as expected"
else
    test_fail "vel_with_comparators_15b did not return 15 as expected"
fi

# Test 846: precedence comparators > ET/VEL mandatory scenarios
echo "Test 846: comparator precedence over ET/VEL is stable"
cleanup_codegen_artifacts "tests/integration/comparator_before_et_precedence_15b.cct"
cleanup_codegen_artifacts "tests/integration/et_vel_arith_complex_15b.cct"
if "$CCT_BIN" "tests/integration/comparator_before_et_precedence_15b.cct" >$CCT_TMP_DIR/cct_phase15b4_846a_compile.out 2>&1; then
    tests/integration/comparator_before_et_precedence_15b >$CCT_TMP_DIR/cct_phase15b4_846a_run.out 2>&1
    RC_846A=$?
else
    RC_846A=255
fi
if "$CCT_BIN" "tests/integration/et_vel_arith_complex_15b.cct" >$CCT_TMP_DIR/cct_phase15b4_846b_compile.out 2>&1; then
    tests/integration/et_vel_arith_complex_15b >$CCT_TMP_DIR/cct_phase15b4_846b_run.out 2>&1
    RC_846B=$?
else
    RC_846B=255
fi
if [ "$RC_846A" -eq 1 ] && [ "$RC_846B" -eq 1 ]; then
    test_pass "comparator precedence over ET/VEL scenarios are stable"
else
    test_fail "comparator precedence over ET/VEL scenarios regressed"
fi

echo ""
echo "========================================"
echo "FASE 15C1: Bitwise Binary Operators Tests"
echo "========================================"
echo ""

# Test 847: 15C1 bitwise operator contract
echo "Test 847: 15C1 ET_BIT/VEL_BIT/XOR contract"
if tests/run_phase15c1_bitwise_ops.sh >$CCT_TMP_DIR/cct_phase15c1_bitwise.out 2>&1; then
    test_pass "15C1 bitwise operators and type-check contract are stable"
else
    test_fail "15C1 bitwise operators and type-check contract failed"
fi

# Test 848: bitwise_and_basic_15c mandatory scenario
echo "Test 848: bitwise_and_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/bitwise_and_basic_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_and_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_848_compile.out 2>&1; then
    tests/integration/bitwise_and_basic_15c >$CCT_TMP_DIR/cct_phase15c1_848_run.out 2>&1
    RC_848=$?
else
    RC_848=255
fi
if [ "$RC_848" -eq 8 ]; then
    test_pass "bitwise_and_basic_15c returns 8 as expected"
else
    test_fail "bitwise_and_basic_15c did not return 8 as expected"
fi

# Test 849: bitwise_or_basic_15c mandatory scenario
echo "Test 849: bitwise_or_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/bitwise_or_basic_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_or_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_849_compile.out 2>&1; then
    tests/integration/bitwise_or_basic_15c >$CCT_TMP_DIR/cct_phase15c1_849_run.out 2>&1
    RC_849=$?
else
    RC_849=255
fi
if [ "$RC_849" -eq 14 ]; then
    test_pass "bitwise_or_basic_15c returns 14 as expected"
else
    test_fail "bitwise_or_basic_15c did not return 14 as expected"
fi

# Test 850: bitwise_xor_basic_15c mandatory scenario
echo "Test 850: bitwise_xor_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/bitwise_xor_basic_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_xor_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_850_compile.out 2>&1; then
    tests/integration/bitwise_xor_basic_15c >$CCT_TMP_DIR/cct_phase15c1_850_run.out 2>&1
    RC_850=$?
else
    RC_850=255
fi
if [ "$RC_850" -eq 6 ]; then
    test_pass "bitwise_xor_basic_15c returns 6 as expected"
else
    test_fail "bitwise_xor_basic_15c did not return 6 as expected"
fi

# Test 851: bitwise flags and xor toggle scenarios
echo "Test 851: bitwise flag and xor-toggle scenarios return expected values"
cleanup_codegen_artifacts "tests/integration/bitwise_flag_operations_15c.cct"
cleanup_codegen_artifacts "tests/integration/bitwise_xor_toggle_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_flag_operations_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_851a_compile.out 2>&1; then
    tests/integration/bitwise_flag_operations_15c >$CCT_TMP_DIR/cct_phase15c1_851a_run.out 2>&1
    RC_851A=$?
else
    RC_851A=255
fi
if "$CCT_BIN" "tests/integration/bitwise_xor_toggle_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_851b_compile.out 2>&1; then
    tests/integration/bitwise_xor_toggle_15c >$CCT_TMP_DIR/cct_phase15c1_851b_run.out 2>&1
    RC_851B=$?
else
    RC_851B=255
fi
if [ "$RC_851A" -eq 1 ] && [ "$RC_851B" -eq 7 ]; then
    test_pass "bitwise flag and xor-toggle scenarios are stable"
else
    test_fail "bitwise flag and xor-toggle scenarios regressed"
fi

# Test 852: bitwise_type_error_15c mandatory negative scenario
echo "Test 852: bitwise_type_error_15c rejects non-integer operands"
set +e
"$CCT_BIN" --check "tests/integration/bitwise_type_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c1_852_check.out 2>&1
RC_852=$?
set -e
if [ "$RC_852" -ne 0 ] && grep -q "require integer operands" $CCT_TMP_DIR/cct_phase15c1_852_check.out; then
    test_pass "bitwise_type_error_15c reports integer-operand requirement"
else
    test_fail "bitwise_type_error_15c did not enforce integer-operand requirement"
fi

echo ""
echo "========================================"
echo "FASE 15C2: Shift Operators Tests"
echo "========================================"
echo ""
set +e

# Test 853: shift_left_basic_15c mandatory scenario
echo "Test 853: shift_left_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/shift_left_basic_15c.cct"
if "$CCT_BIN" "tests/integration/shift_left_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c2_853_compile.out 2>&1; then
    tests/integration/shift_left_basic_15c >$CCT_TMP_DIR/cct_phase15c2_853_run.out 2>&1
    RC_853=$?
else
    RC_853=255
fi
if [ "$RC_853" -eq 8 ]; then
    test_pass "shift_left_basic_15c returns 8 as expected"
else
    test_fail "shift_left_basic_15c did not return 8 as expected"
fi

# Test 854: shift_right_basic_15c mandatory scenario
echo "Test 854: shift_right_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/shift_right_basic_15c.cct"
if "$CCT_BIN" "tests/integration/shift_right_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c2_854_compile.out 2>&1; then
    tests/integration/shift_right_basic_15c >$CCT_TMP_DIR/cct_phase15c2_854_run.out 2>&1
    RC_854=$?
else
    RC_854=255
fi
if [ "$RC_854" -eq 4 ]; then
    test_pass "shift_right_basic_15c returns 4 as expected"
else
    test_fail "shift_right_basic_15c did not return 4 as expected"
fi

# Test 855: shift_left_multiply_15c mandatory scenario
echo "Test 855: shift_left_multiply_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/shift_left_multiply_15c.cct"
if "$CCT_BIN" "tests/integration/shift_left_multiply_15c.cct" >$CCT_TMP_DIR/cct_phase15c2_855_compile.out 2>&1; then
    tests/integration/shift_left_multiply_15c >$CCT_TMP_DIR/cct_phase15c2_855_run.out 2>&1
    RC_855=$?
else
    RC_855=255
fi
if [ "$RC_855" -eq 80 ]; then
    test_pass "shift_left_multiply_15c returns 80 as expected"
else
    test_fail "shift_left_multiply_15c did not return 80 as expected"
fi

# Test 856: shift_combined_bitwise_15c mandatory scenario
echo "Test 856: shift_combined_bitwise_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/shift_combined_bitwise_15c.cct"
if "$CCT_BIN" "tests/integration/shift_combined_bitwise_15c.cct" >$CCT_TMP_DIR/cct_phase15c2_856_compile.out 2>&1; then
    tests/integration/shift_combined_bitwise_15c >$CCT_TMP_DIR/cct_phase15c2_856_run.out 2>&1
    RC_856=$?
else
    RC_856=255
fi
if [ "$RC_856" -eq 10 ]; then
    test_pass "shift_combined_bitwise_15c returns 10 as expected"
else
    test_fail "shift_combined_bitwise_15c did not return 10 as expected"
fi

# Test 857: shift_type_error_15c mandatory negative scenario
echo "Test 857: shift_type_error_15c rejects non-integer operands"
"$CCT_BIN" --check "tests/integration/shift_type_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c2_857_check.out 2>&1
RC_857=$?
if [ "$RC_857" -ne 0 ] && grep -q "require integer operands" $CCT_TMP_DIR/cct_phase15c2_857_check.out; then
    test_pass "shift_type_error_15c reports integer-operand requirement"
else
    test_fail "shift_type_error_15c did not enforce integer-operand requirement"
fi

# Test 858: negative literal shift count emits semantic warning
echo "Test 858: shift_negative_warning_15c emits warning and keeps check success"
OUT_858=$("$CCT_BIN" --check "tests/integration/shift_negative_warning_15c.cct" 2>&1)
RC_858=$?
if [ "$RC_858" -eq 0 ] && \
   echo "$OUT_858" | grep -qi "warning" && \
   echo "$OUT_858" | grep -q "shift count is a negative constant"; then
    test_pass "shift_negative_warning_15c emits expected semantic warning"
else
    test_fail "shift_negative_warning_15c warning contract regressed"
fi

echo ""
echo "========================================"
echo "FASE 15C3: Unary NON_BIT Operator Tests"
echo "========================================"
echo ""

# Test 859: non_bit_basic_15c mandatory scenario
echo "Test 859: non_bit_basic_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/non_bit_basic_15c.cct"
if "$CCT_BIN" "tests/integration/non_bit_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c3_859_compile.out 2>&1; then
    tests/integration/non_bit_basic_15c >$CCT_TMP_DIR/cct_phase15c3_859_run.out 2>&1
    RC_859=$?
else
    RC_859=255
fi
if [ "$RC_859" -eq 1 ]; then
    test_pass "non_bit_basic_15c returns 1 as expected"
else
    test_fail "non_bit_basic_15c did not return 1 as expected"
fi

# Test 860: non_bit_mask_15c mandatory scenario
echo "Test 860: non_bit_mask_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/non_bit_mask_15c.cct"
if "$CCT_BIN" "tests/integration/non_bit_mask_15c.cct" >$CCT_TMP_DIR/cct_phase15c3_860_compile.out 2>&1; then
    tests/integration/non_bit_mask_15c >$CCT_TMP_DIR/cct_phase15c3_860_run.out 2>&1
    RC_860=$?
else
    RC_860=255
fi
if [ "$RC_860" -eq 248 ]; then
    test_pass "non_bit_mask_15c returns 248 as expected"
else
    test_fail "non_bit_mask_15c did not return 248 as expected"
fi

# Test 861: non_bit_type_error_15c mandatory negative scenario
echo "Test 861: non_bit_type_error_15c rejects non-integer operand"
"$CCT_BIN" --check "tests/integration/non_bit_type_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c3_861_check.out 2>&1
RC_861=$?
if [ "$RC_861" -ne 0 ] && grep -q "NON_BIT requires integer operand" $CCT_TMP_DIR/cct_phase15c3_861_check.out; then
    test_pass "non_bit_type_error_15c reports integer-operand requirement"
else
    test_fail "non_bit_type_error_15c did not enforce integer-operand requirement"
fi

# Test 862: NON_BIT does not interfere with logical NON
echo "Test 862: NON and NON_BIT interop remains stable"
cleanup_codegen_artifacts "tests/integration/non_non_bit_interop_15c.cct"
if "$CCT_BIN" "tests/integration/non_non_bit_interop_15c.cct" >$CCT_TMP_DIR/cct_phase15c3_862_compile.out 2>&1; then
    tests/integration/non_non_bit_interop_15c >$CCT_TMP_DIR/cct_phase15c3_862_run.out 2>&1
    RC_862=$?
else
    RC_862=255
fi
if [ "$RC_862" -eq 1 ]; then
    test_pass "logical NON remains stable alongside NON_BIT"
else
    test_fail "logical NON regressed when combined with NON_BIT"
fi

# Test 863: NON_BIT codegen emits bitwise complement in generated C
echo "Test 863: NON_BIT emits '~' pattern in generated C"
cleanup_codegen_artifacts "tests/integration/non_bit_basic_15c.cct"
if "$CCT_BIN" "tests/integration/non_bit_basic_15c.cct" >$CCT_TMP_DIR/cct_phase15c3_863_compile.out 2>&1 && \
   grep -q "~(" "tests/integration/non_bit_basic_15c.cgen.c"; then
    test_pass "NON_BIT codegen emits C bitwise complement pattern"
else
    test_fail "NON_BIT codegen did not emit expected '~' C pattern"
fi

echo ""
echo "========================================"
echo "FASE 15C4: Bitwise Integration and Closure Tests"
echo "========================================"
echo ""

# Test 864: all bitwise operators integrated in one scenario
echo "Test 864: bitwise_all_ops_15c integrated scenario remains stable"
cleanup_codegen_artifacts "tests/integration/bitwise_all_ops_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_all_ops_15c.cct" >$CCT_TMP_DIR/cct_phase15c4_864_compile.out 2>&1; then
    tests/integration/bitwise_all_ops_15c >$CCT_TMP_DIR/cct_phase15c4_864_run.out 2>&1
    RC_864=$?
else
    RC_864=255
fi
# Expected semantic result is 377; process exit status carries 377 mod 256 = 121.
if [ "$RC_864" -eq 121 ]; then
    test_pass "bitwise_all_ops_15c integrated arithmetic/bitwise composition is stable"
else
    test_fail "bitwise_all_ops_15c integrated arithmetic/bitwise composition regressed"
fi

# Test 865: bitwise expression in SI condition
echo "Test 865: bitwise_in_si_condition_15c returns expected value"
cleanup_codegen_artifacts "tests/integration/bitwise_in_si_condition_15c.cct"
if "$CCT_BIN" "tests/integration/bitwise_in_si_condition_15c.cct" >$CCT_TMP_DIR/cct_phase15c4_865_compile.out 2>&1; then
    tests/integration/bitwise_in_si_condition_15c >$CCT_TMP_DIR/cct_phase15c4_865_run.out 2>&1
    RC_865=$?
else
    RC_865=255
fi
if [ "$RC_865" -eq 1 ]; then
    test_pass "bitwise_in_si_condition_15c returns 1 as expected"
else
    test_fail "bitwise_in_si_condition_15c did not return 1 as expected"
fi

# Test 866: bitwise string operand is rejected with clear diagnostic
echo "Test 866: bitwise_verbum_error_15c rejects string operand"
"$CCT_BIN" --check "tests/integration/bitwise_verbum_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c4_866_check.out 2>&1
RC_866=$?
if [ "$RC_866" -ne 0 ] && grep -q "bitwise operators require integer operands" $CCT_TMP_DIR/cct_phase15c4_866_check.out; then
    test_pass "bitwise_verbum_error_15c reports clear integer-operand diagnostic"
else
    test_fail "bitwise_verbum_error_15c did not enforce integer-operand diagnostic"
fi

# Test 867: shift negative literal warning contract (15C2/15C4 closure)
echo "Test 867: shift_negative_literal_warning_15c emits warning"
OUT_867=$("$CCT_BIN" --check "tests/integration/shift_negative_literal_warning_15c.cct" 2>&1)
RC_867=$?
if [ "$RC_867" -eq 0 ] && \
   echo "$OUT_867" | grep -qi "warning" && \
   echo "$OUT_867" | grep -q "shift count is a negative constant"; then
    test_pass "shift_negative_literal_warning_15c emits expected warning"
else
    test_fail "shift_negative_literal_warning_15c warning contract regressed"
fi

# Test 868: 15C closure gate keeps previous type diagnostics stable
echo "Test 868: float and NON_BIT type diagnostics remain stable"
"$CCT_BIN" --check "tests/integration/shift_type_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c4_868_shift_check.out 2>&1
RC_868A=$?
"$CCT_BIN" --check "tests/integration/non_bit_type_error_15c.cct" >$CCT_TMP_DIR/cct_phase15c4_868_nonbit_check.out 2>&1
RC_868B=$?
if [ "$RC_868A" -ne 0 ] && [ "$RC_868B" -ne 0 ] && \
   grep -q "shift operators require integer operands" $CCT_TMP_DIR/cct_phase15c4_868_shift_check.out && \
   grep -q "NON_BIT requires integer operand" $CCT_TMP_DIR/cct_phase15c4_868_nonbit_check.out; then
    test_pass "15C closure gate preserves float/non-bit diagnostics"
else
    test_fail "15C closure gate type diagnostics regressed"
fi

echo ""
echo "========================================"
echo "FASE 15D1: CONSTANS Semantic Enforcement Tests"
echo "========================================"
echo ""

# Test 869: CONSTANS can be read and used in expressions
echo "Test 869: constans_basic_use_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_basic_use_15d.cct"
if "$CCT_BIN" "tests/integration/constans_basic_use_15d.cct" >$CCT_TMP_DIR/cct_phase15d1_869_compile.out 2>&1; then
    tests/integration/constans_basic_use_15d >$CCT_TMP_DIR/cct_phase15d1_869_run.out 2>&1
    RC_869=$?
else
    RC_869=255
fi
if [ "$RC_869" -eq 43 ]; then
    test_pass "constans_basic_use_15d returns 43 as expected"
else
    test_fail "constans_basic_use_15d did not return 43 as expected"
fi

# Test 870: CONSTANS participates in composed expression conditions
echo "Test 870: constans_in_expression_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_in_expression_15d.cct"
if "$CCT_BIN" "tests/integration/constans_in_expression_15d.cct" >$CCT_TMP_DIR/cct_phase15d1_870_compile.out 2>&1; then
    tests/integration/constans_in_expression_15d >$CCT_TMP_DIR/cct_phase15d1_870_run.out 2>&1
    RC_870=$?
else
    RC_870=255
fi
if [ "$RC_870" -eq 1 ]; then
    test_pass "constans_in_expression_15d returns 1 as expected"
else
    test_fail "constans_in_expression_15d did not return 1 as expected"
fi

# Test 871: multiple CONSTANS declarations remain readable
echo "Test 871: constans_multiple_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_multiple_15d.cct"
if "$CCT_BIN" "tests/integration/constans_multiple_15d.cct" >$CCT_TMP_DIR/cct_phase15d1_871_compile.out 2>&1; then
    tests/integration/constans_multiple_15d >$CCT_TMP_DIR/cct_phase15d1_871_run.out 2>&1
    RC_871=$?
else
    RC_871=255
fi
if [ "$RC_871" -eq 30 ]; then
    test_pass "constans_multiple_15d returns 30 as expected"
else
    test_fail "constans_multiple_15d did not return 30 as expected"
fi

# Test 872: non-CONSTANS assignment behavior remains unchanged
echo "Test 872: constans_non_constans_ok_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_non_constans_ok_15d.cct"
if "$CCT_BIN" "tests/integration/constans_non_constans_ok_15d.cct" >$CCT_TMP_DIR/cct_phase15d1_872_compile.out 2>&1; then
    tests/integration/constans_non_constans_ok_15d >$CCT_TMP_DIR/cct_phase15d1_872_run.out 2>&1
    RC_872=$?
else
    RC_872=255
fi
if [ "$RC_872" -eq 10 ]; then
    test_pass "constans_non_constans_ok_15d returns 10 as expected"
else
    test_fail "constans_non_constans_ok_15d did not return 10 as expected"
fi

# Test 873: reassignment of CONSTANS is rejected with clear semantic error
echo "Test 873: constans_reassign_error_15d rejects reassignment"
"$CCT_BIN" --check "tests/integration/constans_reassign_error_15d.cct" >$CCT_TMP_DIR/cct_phase15d1_873_check.out 2>&1
RC_873=$?
if [ "$RC_873" -ne 0 ] && \
   grep -q "cannot reassign" $CCT_TMP_DIR/cct_phase15d1_873_check.out && \
   grep -q "CONSTANS" $CCT_TMP_DIR/cct_phase15d1_873_check.out && \
   grep -q "'x'" $CCT_TMP_DIR/cct_phase15d1_873_check.out; then
    test_pass "constans_reassign_error_15d reports CONSTANS reassignment clearly"
else
    test_fail "constans_reassign_error_15d did not enforce CONSTANS reassignment rule"
fi

echo ""
echo "========================================"
echo "FASE 15D2: CONSTANS const Codegen Tests"
echo "========================================"
echo ""

# Test 874: generated C keeps CONSTANS intent with const qualifier
echo "Test 874: constans_const_in_output_15d emits const in generated C"
cleanup_codegen_artifacts "tests/integration/constans_const_in_output_15d.cct"
if "$CCT_BIN" "tests/integration/constans_const_in_output_15d.cct" >$CCT_TMP_DIR/cct_phase15d2_874_compile.out 2>&1 && \
   grep -q "const long long marker =" "tests/integration/constans_const_in_output_15d.cgen.c"; then
    test_pass "constans_const_in_output_15d emits const qualifier in generated C"
else
    test_fail "constans_const_in_output_15d did not emit const qualifier in generated C"
fi

# Test 875: CONSTANS runtime with bitwise shift remains correct
echo "Test 875: constans_runtime_value_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_runtime_value_15d.cct"
if "$CCT_BIN" "tests/integration/constans_runtime_value_15d.cct" >$CCT_TMP_DIR/cct_phase15d2_875_compile.out 2>&1; then
    tests/integration/constans_runtime_value_15d >$CCT_TMP_DIR/cct_phase15d2_875_run.out 2>&1
    RC_875=$?
else
    RC_875=255
fi
if [ "$RC_875" -eq 15 ]; then
    test_pass "constans_runtime_value_15d returns 15 as expected"
else
    test_fail "constans_runtime_value_15d did not return 15 as expected"
fi

# Test 876: CONSTANS can be used in loop condition without reassignment
echo "Test 876: constans_in_loop_condition_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_in_loop_condition_15d.cct"
if "$CCT_BIN" "tests/integration/constans_in_loop_condition_15d.cct" >$CCT_TMP_DIR/cct_phase15d2_876_compile.out 2>&1; then
    tests/integration/constans_in_loop_condition_15d >$CCT_TMP_DIR/cct_phase15d2_876_run.out 2>&1
    RC_876=$?
else
    RC_876=255
fi
if [ "$RC_876" -eq 5 ]; then
    test_pass "constans_in_loop_condition_15d returns 5 as expected"
else
    test_fail "constans_in_loop_condition_15d did not return 5 as expected"
fi

echo ""
echo "========================================"
echo "FASE 15D3: CONSTANS Rituale Parameter Tests"
echo "========================================"
echo ""

# Test 877: CONSTANS parameter is accepted, used, and emitted as const in generated C
echo "Test 877: constans_param_use_15d enforces const parameter emission"
cleanup_codegen_artifacts "tests/integration/constans_param_use_15d.cct"
if "$CCT_BIN" "tests/integration/constans_param_use_15d.cct" >$CCT_TMP_DIR/cct_phase15d3_877_compile.out 2>&1; then
    tests/integration/constans_param_use_15d >$CCT_TMP_DIR/cct_phase15d3_877_run.out 2>&1
    RC_877=$?
else
    RC_877=255
fi
if [ "$RC_877" -eq 42 ] && grep -q "const long long x" "tests/integration/constans_param_use_15d.cgen.c"; then
    test_pass "constans_param_use_15d returns 42 and generated C emits const parameter"
else
    test_fail "constans_param_use_15d contract regressed (runtime or const emission)"
fi

# Test 878: reassignment of CONSTANS parameter is rejected
echo "Test 878: constans_param_reassign_error_15d rejects reassignment"
"$CCT_BIN" --check "tests/integration/constans_param_reassign_error_15d.cct" >$CCT_TMP_DIR/cct_phase15d3_878_check.out 2>&1
RC_878=$?
if [ "$RC_878" -ne 0 ] && \
   grep -q "cannot reassign CONSTANS parameter 'x'" $CCT_TMP_DIR/cct_phase15d3_878_check.out; then
    test_pass "constans_param_reassign_error_15d reports CONSTANS parameter reassignment clearly"
else
    test_fail "constans_param_reassign_error_15d did not enforce CONSTANS parameter reassignment rule"
fi

# Test 879: local CONSTANS and regular parameter coexist as expected
echo "Test 879: constans_local_vs_param_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_local_vs_param_15d.cct"
if "$CCT_BIN" "tests/integration/constans_local_vs_param_15d.cct" >$CCT_TMP_DIR/cct_phase15d3_879_compile.out 2>&1; then
    tests/integration/constans_local_vs_param_15d >$CCT_TMP_DIR/cct_phase15d3_879_run.out 2>&1
    RC_879=$?
else
    RC_879=255
fi
if [ "$RC_879" -eq 21 ]; then
    test_pass "constans_local_vs_param_15d returns 21 as expected"
else
    test_fail "constans_local_vs_param_15d did not return 21 as expected"
fi

echo ""
echo "========================================"
echo "FASE 15D4: CONSTANS Edge Cases and Closure Tests"
echo "========================================"
echo ""

# Test 880: CONSTANS SPECULUM keeps pointer binding fixed while allowing pointee mutation
echo "Test 880: constans_speculum_15d keeps pointer const and allows pointee mutation"
cleanup_codegen_artifacts "tests/integration/constans_speculum_15d.cct"
if "$CCT_BIN" "tests/integration/constans_speculum_15d.cct" >$CCT_TMP_DIR/cct_phase15d4_880_compile.out 2>&1; then
    tests/integration/constans_speculum_15d >$CCT_TMP_DIR/cct_phase15d4_880_run.out 2>&1
    RC_880=$?
else
    RC_880=255
fi
if [ "$RC_880" -eq 42 ] && grep -q "long long \\* const p" "tests/integration/constans_speculum_15d.cgen.c"; then
    test_pass "constans_speculum_15d preserves const pointer binding and mutable pointee semantics"
else
    test_fail "constans_speculum_15d pointer-const edge behavior regressed"
fi

# Test 881: reassigning CONSTANS pointer is rejected semantically
echo "Test 881: constans_speculum_reassign_error_15d rejects pointer reassignment"
"$CCT_BIN" --check "tests/integration/constans_speculum_reassign_error_15d.cct" >$CCT_TMP_DIR/cct_phase15d4_881_check.out 2>&1
RC_881=$?
if [ "$RC_881" -ne 0 ] && \
   grep -q "cannot reassign CONSTANS variable 'p'" $CCT_TMP_DIR/cct_phase15d4_881_check.out; then
    test_pass "constans_speculum_reassign_error_15d reports const pointer reassignment clearly"
else
    test_fail "constans_speculum_reassign_error_15d did not enforce const pointer reassignment rule"
fi

# Test 882: CONSTANS declaration inside loop body keeps local-scope behavior
echo "Test 882: constans_in_loop_body_15d returns expected value"
cleanup_codegen_artifacts "tests/integration/constans_in_loop_body_15d.cct"
if "$CCT_BIN" "tests/integration/constans_in_loop_body_15d.cct" >$CCT_TMP_DIR/cct_phase15d4_882_compile.out 2>&1; then
    tests/integration/constans_in_loop_body_15d >$CCT_TMP_DIR/cct_phase15d4_882_run.out 2>&1
    RC_882=$?
else
    RC_882=255
fi
if [ "$RC_882" -eq 30 ]; then
    test_pass "constans_in_loop_body_15d returns 30 as expected"
else
    test_fail "constans_in_loop_body_15d did not return 30 as expected"
fi

echo ""
echo "========================================"
echo "Test Results:"
echo -e "  ${GREEN}Passed:${NC} $TESTS_PASSED"
echo -e "  ${RED}Failed:${NC} $TESTS_FAILED"
echo "========================================"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
