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

echo "Test 440: final stdlib subset manifest exists and lists canonical modules"
if [ -f "docs/release/FASE_12_STABILITY_MATRIX.md" ] && \
   grep -q "cct/verbum" docs/release/FASE_12_STABILITY_MATRIX.md && \
   grep -q "cct/fluxus" docs/release/FASE_12_STABILITY_MATRIX.md && \
   grep -q "cct/path" docs/release/FASE_12_STABILITY_MATRIX.md && \
   grep -q "cct/parse" docs/release/FASE_12_STABILITY_MATRIX.md; then
    test_pass "Final stdlib subset manifest is present and complete"
else
    test_fail "Final stdlib subset manifest is missing or incomplete"
fi

echo "Test 441: final stability matrix exists with stable/experimental/internal classes"
if [ -f "docs/bibliotheca_canonica.md" ] && \
   grep -q "Canonical Stable" docs/bibliotheca_canonica.md && \
   grep -q "Canonical Experimental" docs/bibliotheca_canonica.md && \
   grep -q "Runtime Internal" docs/release/FASE_12_FINAL_SNAPSHOT.md; then
    test_pass "Final stability matrix is present with required classes"
else
    test_fail "Final stability matrix missing required classes"
fi

echo "Test 442: final naming freeze preserves fmt_parse_* facade and parse namespace separation"
if grep -Eq "^RITUALE fmt_parse_int\\(VERBUM [A-Za-z_][A-Za-z0-9_]*\\) REDDE REX$" lib/cct/fmt.cct && \
   grep -Eq "^RITUALE parse_int\\(VERBUM [A-Za-z_][A-Za-z0-9_]*\\) REDDE REX$" lib/cct/parse.cct && \
   ! grep -q "^RITUALE parse_int(VERBUM s) REDDE REX$" lib/cct/fmt.cct; then
    test_pass "Final naming separation between cct/fmt and cct/parse is preserved"
else
    test_fail "Final naming separation between cct/fmt and cct/parse regressed"
fi

echo "Test 443: final error policy is documented in release subset docs"
if grep -q "Strict API" docs/bibliotheca_canonica.md && \
   grep -q "silent errors" docs/bibliotheca_canonica.md; then
    test_pass "Final error policy is documented"
else
    test_fail "Final error policy documentation is missing"
fi

echo "Test 444: final import policy keeps cct/... reserved with env override support"
if grep -q "CCT_STDLIB_DIR" src/module/module.c && \
   grep -q "Resolver precedence" docs/bibliotheca_canonica.md; then
    test_pass "Final import policy and resolver override support are consistent"
else
    test_fail "Final import policy/override support is inconsistent"
fi

echo "Test 445: final ownership policy is documented"
if grep -q "explicit ownership semantics" docs/bibliotheca_canonica.md; then
    test_pass "Final ownership policy is documented and linked"
else
    test_fail "Final ownership policy documentation is missing"
fi

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

echo "Test 451: installation/release docs are present and coherent"
if [ -f "docs/install.md" ] && \
   [ -f "docs/release/FASE_12_RELEASE_NOTES.md" ] && \
   [ -f "examples/README.md" ] && \
   grep -q "make dist" docs/install.md && \
   grep -q "FASE 12" docs/release/FASE_12_RELEASE_NOTES.md; then
    test_pass "Installation and release documentation is present and coherent"
else
    test_fail "Installation/release documentation is missing or inconsistent"
fi

echo ""
echo "========================================"
echo "FASE 12A: Diagnostic Infrastructure Tests"
echo "========================================"
echo ""

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
TMP_FMT_BASIC=$(mktemp /tmp/cct_fmt_basic_XXXXXX.cct)
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
TMP_FMT_ITER=$(mktemp /tmp/cct_fmt_iter_XXXXXX.cct)
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
TMP_FMT_EXEC=$(mktemp /tmp/cct_fmt_exec_XXXXXX.cct)
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
TMP_LINT_FIX_IMPORT=$(mktemp /tmp/cct_lint_fix_import_XXXXXX.cct)
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
TMP_LINT_FIX_DEAD=$(mktemp /tmp/cct_lint_fix_dead_XXXXXX.cct)
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
TMP_LINT_FIX_IDEMP=$(mktemp /tmp/cct_lint_fix_idemp_XXXXXX.cct)
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
MOD_BAK=$(mktemp /tmp/cct_mod_backup_XXXXXX.cct)
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
DOC_DET_A="/tmp/cct_doc_det_a"
DOC_DET_B="/tmp/cct_doc_det_b"
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
echo "Test 533: FASE 12 release documentation completeness"
if [ -f "docs/release/FASE_12_FINAL_SNAPSHOT.md" ] && \
   [ -f "docs/release/FASE_12_STABILITY_MATRIX.md" ] && \
   [ -f "docs/release/FASE_12_COMPATIBILITY_MATRIX.md" ] && \
   [ -f "docs/release/FASE_12_RELEASE_NOTES.md" ] && \
   [ -f "docs/release/FASE_12_KNOWN_LIMITS.md" ]; then
    test_pass "All FASE 12 release documents exist"
else
    test_fail "Some FASE 12 release documents are missing"
fi

# Test 534: Verify release documents contain required sections
echo "Test 534: FASE 12 snapshot document contains required sections"
if grep -q "Official Scope" docs/release/FASE_12_FINAL_SNAPSHOT.md && \
   grep -q "Stability Classification" docs/release/FASE_12_FINAL_SNAPSHOT.md && \
   grep -q "CLI Commands" docs/release/FASE_12_FINAL_SNAPSHOT.md; then
    test_pass "FASE 12 snapshot contains required sections"
else
    test_fail "FASE 12 snapshot missing required sections"
fi

# Test 535: Verify stability matrix contains classification table
echo "Test 535: Stability matrix contains component classification"
if grep -q "stable" docs/release/FASE_12_STABILITY_MATRIX.md && \
   grep -q "experimental" docs/release/FASE_12_STABILITY_MATRIX.md && \
   grep -q "internal" docs/release/FASE_12_STABILITY_MATRIX.md; then
    test_pass "Stability matrix contains all stability levels"
else
    test_fail "Stability matrix missing stability classifications"
fi

# Test 536: Showcase example compiles and runs
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

cleanup_codegen_artifacts "tests/integration/codegen_minimal.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_minimal.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_if.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_while.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_repete.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_call.cct"
cleanup_codegen_artifacts "tests/integration/parser_test.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_structural_variant_a.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_structural_variant_b.cct"
cleanup_codegen_artifacts "tests/integration/codegen_type_not_supported_real.cct"
cleanup_codegen_artifacts "tests/integration/codegen_real_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_real_return.cct"
cleanup_codegen_artifacts "tests/integration/codegen_real_call.cct"
cleanup_codegen_artifacts "tests/integration/codegen_real_scribe.cct"
cleanup_codegen_artifacts "tests/integration/codegen_verbum_var.cct"
cleanup_codegen_artifacts "tests/integration/codegen_verbum_scribe_mixed.cct"
cleanup_codegen_artifacts "tests/integration/codegen_series_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_series_read_write.cct"
cleanup_codegen_artifacts "tests/integration/codegen_series_index_expr.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fields.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_scribe.cct"
cleanup_codegen_artifacts "tests/integration/codegen_ordo_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_repete_gradus_zero.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_deref_write.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_call_param.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_pass_by_ref.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_alloc_free.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_dimitte_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_series_ref.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_unsupported_verbum_ptr.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fields_multi.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_nested_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_field_expr.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_scribe_rich.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_param_ref.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_sigillum_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_param_value_unsupported.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_return_unsupported.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_read_write.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_scribe.cct"
cleanup_codegen_artifacts "tests/integration/codegen_memory_indexed_free.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_fill_by_ref.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_mutate_nested_by_ref.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_copy_shallow_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_speculum_sigillum_deep_unsupported.cct"
cleanup_codegen_artifacts "tests/integration/codegen_sigillum_copy_with_pointer_field_unsupported.cct"
cleanup_codegen_artifacts "tests/integration/codegen_tempta_local_catch.cct"
cleanup_codegen_artifacts "tests/integration/codegen_tempta_no_throw.cct"
cleanup_codegen_artifacts "tests/integration/codegen_iace_uncaught.cct"
cleanup_codegen_artifacts "tests/integration/codegen_iace_rethrow_uncaught.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_tempta_basic.cct"
cleanup_codegen_artifacts "tests/integration/diagnostic_type_error_12a.cct"
cleanup_codegen_artifacts "tests/integration/diagnostic_symbol_not_found_12a.cct"
cleanup_codegen_artifacts "tests/integration/diagnostic_syntax_error_12a.cct"
cleanup_codegen_artifacts "tests/integration/diagnostic_missing_import_12a.cct"
cleanup_codegen_artifacts "tests/integration/cast_int_to_int_12b.cct"
cleanup_codegen_artifacts "tests/integration/cast_int_to_float_12b.cct"
cleanup_codegen_artifacts "tests/integration/cast_float_to_int_12b.cct"
cleanup_codegen_artifacts "tests/integration/cast_invalid_12b.cct"
cleanup_codegen_artifacts "tests/integration/cast_with_genus_12b.cct"
cleanup_codegen_artifacts "tests/integration/option_basic_12c.cct"
cleanup_codegen_artifacts "tests/integration/option_unwrap_or_12c.cct"
cleanup_codegen_artifacts "tests/integration/result_basic_12c.cct"
cleanup_codegen_artifacts "tests/integration/result_unwrap_or_12c.cct"
cleanup_codegen_artifacts "tests/integration/option_result_integration_12c.cct"
cleanup_codegen_artifacts "tests/integration/option_with_cast_12c.cct"
cleanup_codegen_artifacts "tests/integration/map_basic_12d1.cct"
cleanup_codegen_artifacts "tests/integration/map_collisions_12d1.cct"
cleanup_codegen_artifacts "tests/integration/map_remove_12d1.cct"
cleanup_codegen_artifacts "tests/integration/map_update_12d1.cct"
cleanup_codegen_artifacts "tests/integration/set_basic_12d1.cct"
cleanup_codegen_artifacts "tests/integration/set_remove_12d1.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_map_set_12d1.cct"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_map_12d2.cct"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_filter_12d2.cct"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_fold_12d2.cct"
cleanup_codegen_artifacts "tests/integration/collection_fluxus_find_12d2.cct"
cleanup_codegen_artifacts "tests/integration/collection_series_ops_12d2.cct"
cleanup_codegen_artifacts "tests/integration/collection_any_all_12d2.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_collection_ops_12d2.cct"
cleanup_codegen_artifacts "tests/integration/iterum_fluxus_12d3.cct"
cleanup_codegen_artifacts "tests/integration/iterum_series_12d3.cct"
cleanup_codegen_artifacts "tests/integration/iterum_map_result_12d3.cct"
cleanup_codegen_artifacts "tests/integration/iterum_nested_12d3.cct"
cleanup_codegen_artifacts "tests/integration/iterum_empty_12d3.cct"
cleanup_codegen_artifacts "tests/integration/sem_iterum_type_check_12d3.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_iterum_12d3.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_propagation_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_propagation_uncaught.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_expr_propagation.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_rethrow_chain.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_no_throw.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_propagation_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_semper_no_throw.cct"
cleanup_codegen_artifacts "tests/integration/codegen_semper_after_catch.cct"
cleanup_codegen_artifacts "tests/integration/codegen_semper_rethrow.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_libera_semper.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_dimitte_semper.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_coniura_capture_semper.cct"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_bridge_semper.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_semper_basic.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_fullstack_local_propagate_semper.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_rethrow_semper_outer_catch.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_uncaught_after_semper.cct"
cleanup_codegen_artifacts "tests/integration/codegen_failure_cleanup_libera_dimitte_combined.cct"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_integrated_final.cct"
cleanup_codegen_artifacts "tests/integration/codegen_runtime_fail_nonintegrated_final.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_failure_final_subset.cct"
cleanup_codegen_artifacts "tests/integration/module_main_basic.cct"
cleanup_codegen_artifacts "tests/integration/module_main_multi_import.cct"
cleanup_codegen_artifacts "tests/integration/module_main_sigillum.cct"
cleanup_codegen_artifacts "tests/integration/module_main_ordo.cct"
cleanup_codegen_artifacts "tests/integration/module_import_duplicate.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_module_basic.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_call_ok_main.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_type_ok_main.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_missing_symbol.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_duplicate_rituale_entry.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_duplicate_type_entry.cct"
cleanup_codegen_artifacts "tests/integration/module_resolve_transitive_denied_main.cct"
cleanup_codegen_artifacts "tests/integration/module_linking_pragmatic_main.cct"
cleanup_codegen_artifacts "tests/integration/module_visibility_public_default_main.cct"
cleanup_codegen_artifacts "tests/integration/module_visibility_internal_rituale_main_fail.cct"
cleanup_codegen_artifacts "tests/integration/module_visibility_internal_sigillum_main_fail.cct"
cleanup_codegen_artifacts "tests/integration/module_visibility_internal_ordo_main_fail.cct"
cleanup_codegen_artifacts "tests/integration/module_visibility_arcanum_invalid_context.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_module_visibility_basic.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_entry.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_import_variant.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_call_graph_variant.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mod_type_ref_variant.cct"
cleanup_codegen_artifacts "tests/integration/modules_9d/sigilo_mod_types.cct"
cleanup_codegen_artifacts "tests/integration/modules_9d/sigilo_mod_ops.cct"
cleanup_codegen_artifacts "tests/integration/module_final_entry.cct"
cleanup_codegen_artifacts "tests/integration/module_final_visibility_fail.cct"
cleanup_codegen_artifacts "tests/integration/module_final_cycle_a.cct"
cleanup_codegen_artifacts "tests/integration/module_final_cycle_b.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_final_modular_entry.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_a.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_b.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_cycle_a.cct"
cleanup_codegen_artifacts "tests/integration/modules_9e/module_final_cycle_b.cct"
cleanup_codegen_artifacts "tests/integration/ast_genus_rituale_basic.cct"
cleanup_codegen_artifacts "tests/integration/ast_genus_sigillum_basic.cct"
cleanup_codegen_artifacts "tests/integration/syntax_genus_empty_invalid.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_duplicate_param_invalid.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_scope_invalid.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_signature_ok.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_sigillum_field_ok.cct"
cleanup_codegen_artifacts "tests/integration/syntax_genus_invalid_context_ordo.cct"
cleanup_codegen_artifacts "tests/integration/codegen_genus_not_executable_10a.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_genus_basic.cct"
cleanup_codegen_artifacts "tests/integration/module_genus_ast_composite_basic.cct"
cleanup_codegen_artifacts "tests/integration/modules_10a/module_genus_ast_composite_lib.cct"
cleanup_codegen_artifacts "tests/integration/ast_genus_call_instantiation_basic.cct"
cleanup_codegen_artifacts "tests/integration/ast_genus_type_instantiation_basic.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_call_missing_type_args_10b.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_call_non_generic_with_type_args_10b.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_arity_mismatch_10b.cct"
cleanup_codegen_artifacts "tests/integration/sem_genus_type_arg_invalid_10b.cct"
cleanup_codegen_artifacts "tests/integration/codegen_genus_rituale_rex_basic_10b.cct"
cleanup_codegen_artifacts "tests/integration/codegen_genus_rituale_verbum_basic_10b.cct"
cleanup_codegen_artifacts "tests/integration/codegen_genus_sigillum_basic_10b.cct"
cleanup_codegen_artifacts "tests/integration/codegen_genus_dedup_10b.cct"
cleanup_codegen_artifacts "tests/integration/module_genus_cross_instantiation_main_10b.cct"
cleanup_codegen_artifacts "tests/integration/modules_10b/module_genus_cross_instantiation_lib_10b.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_genus_instantiation_basic_10b.cct"
cleanup_codegen_artifacts "tests/integration/ast_sigillum_pactum_basic_10c.cct"
cleanup_codegen_artifacts "tests/integration/ast_pactum_signature_basic_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_pactum_conformance_ok_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_pactum_missing_contract_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_pactum_missing_method_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_pactum_param_mismatch_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_pactum_return_mismatch_10c.cct"
cleanup_codegen_artifacts "tests/integration/sem_sigillum_multi_pactum_invalid_10c.cct"
cleanup_codegen_artifacts "tests/integration/codegen_pactum_conformance_ok_10c.cct"
cleanup_codegen_artifacts "tests/integration/module_pactum_cross_ok_main_10c.cct"
cleanup_codegen_artifacts "tests/integration/module_pactum_transitive_denied_main_10c.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_pactum_basic_10c.cct"
cleanup_codegen_artifacts "tests/integration/modules_10c/module_pactum_contract_10c.cct"
cleanup_codegen_artifacts "tests/integration/modules_10c/module_pactum_impl_10c.cct"
cleanup_codegen_artifacts "tests/integration/modules_10c/module_pactum_transitive_mid_10c.cct"
cleanup_codegen_artifacts "tests/integration/modules_10c/module_pactum_transitive_contract_10c.cct"
cleanup_codegen_artifacts "tests/integration/ast_genus_constraint_basic_10d.cct"
cleanup_codegen_artifacts "tests/integration/sem_constraint_ok_10d.cct"
cleanup_codegen_artifacts "tests/integration/sem_constraint_missing_pactum_10d.cct"
cleanup_codegen_artifacts "tests/integration/sem_constraint_non_sigillum_arg_10d.cct"
cleanup_codegen_artifacts "tests/integration/sem_constraint_not_conforming_sigillum_10d.cct"
cleanup_codegen_artifacts "tests/integration/sem_constraint_multi_pactum_invalid_10d.cct"
cleanup_codegen_artifacts "tests/integration/codegen_constraint_ok_10d.cct"
cleanup_codegen_artifacts "tests/integration/module_constraint_ok_main_10d.cct"
cleanup_codegen_artifacts "tests/integration/module_constraint_transitive_denied_main_10d.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_constraint_basic_10d.cct"
cleanup_codegen_artifacts "tests/integration/typing_final_happy_path_10e.cct"
cleanup_codegen_artifacts "tests/integration/typing_final_constraint_happy_10e.cct"
cleanup_codegen_artifacts "tests/integration/typing_final_nonconforming_10e.cct"
cleanup_codegen_artifacts "tests/integration/typing_final_subset_boundary_10e.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_typing_final_10e.cct"
cleanup_codegen_artifacts "tests/integration/module_typing_final_10e_main.cct"
cleanup_codegen_artifacts "tests/integration/modules_10e/module_typing_contract_10e.cct"
cleanup_codegen_artifacts "tests/integration/modules_10e/module_typing_impl_10e.cct"
cleanup_codegen_artifacts "tests/integration/stdlib_resolution_basic_11a.cct"
cleanup_codegen_artifacts "tests/integration/stdlib_resolution_missing_11a.cct"
cleanup_codegen_artifacts "tests/integration/stdlib_resolution_no_collision_11a.cct"
cleanup_codegen_artifacts "tests/integration/verbum_len_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_concat_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_compare_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_substring_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_substring_oob_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_trim_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_contains_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_find_11b1.cct"
cleanup_codegen_artifacts "tests/integration/verbum_scribe_11b1.cct"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_int_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_int_neg_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_real_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_stringify_bool_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_neg_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_roundtrip_int_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_parse_int_invalid_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_format_pair_11b2.cct"
cleanup_codegen_artifacts "tests/integration/fmt_scribe_11b2.cct"
cleanup_codegen_artifacts "tests/integration/series_fill_11c.cct"
cleanup_codegen_artifacts "tests/integration/series_copy_11c.cct"
cleanup_codegen_artifacts "tests/integration/series_reverse_11c.cct"
cleanup_codegen_artifacts "tests/integration/series_contains_11c.cct"
cleanup_codegen_artifacts "tests/integration/alg_linear_search_11c.cct"
cleanup_codegen_artifacts "tests/integration/alg_compare_arrays_11c.cct"
cleanup_codegen_artifacts "tests/integration/series_generic_real_11c.cct"
cleanup_codegen_artifacts "tests/integration/series_fmt_integration_11c.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_series_alg_11c.cct"
cleanup_codegen_artifacts "tests/integration/mem_alloc_free_11d1.cct"
cleanup_codegen_artifacts "tests/integration/mem_realloc_11d1.cct"
cleanup_codegen_artifacts "tests/integration/mem_copy_11d1.cct"
cleanup_codegen_artifacts "tests/integration/mem_set_zero_11d1.cct"
cleanup_codegen_artifacts "tests/integration/mem_with_verbum_11d1.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_mem_basic_11d1.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_init_free_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_push_len_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_get_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_pop_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_clear_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_reserve_capacity_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_growth_11d3.cct"
cleanup_codegen_artifacts "tests/integration/fluxus_with_verbum_11d3.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_fluxus_basic_11d3.cct"
cleanup_codegen_artifacts "tests/integration/io_print_11e1.cct"
cleanup_codegen_artifacts "tests/integration/io_print_int_11e1.cct"
cleanup_codegen_artifacts "tests/integration/io_read_line_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_write_read_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_append_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_exists_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_size_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_with_verbum_11e1.cct"
cleanup_codegen_artifacts "tests/integration/fs_read_error_11e1.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_io_fs_basic_11e1.cct"
cleanup_codegen_artifacts "tests/integration/path_join_basic_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_join_double_sep_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_basename_basic_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_dirname_basic_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_ext_basic_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_ext_noext_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_fs_integration_read_11e2.cct"
cleanup_codegen_artifacts "tests/integration/path_fs_integration_write_read_11e2.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_path_basic_11e2.cct"
cleanup_codegen_artifacts "tests/integration/math_abs_basic_11f1.cct"
cleanup_codegen_artifacts "tests/integration/math_min_max_basic_11f1.cct"
cleanup_codegen_artifacts "tests/integration/math_clamp_basic_11f1.cct"
cleanup_codegen_artifacts "tests/integration/math_clamp_invalid_range_11f1.cct"
cleanup_codegen_artifacts "tests/integration/random_seed_repro_11f1.cct"
cleanup_codegen_artifacts "tests/integration/random_int_range_11f1.cct"
cleanup_codegen_artifacts "tests/integration/random_int_invalid_range_11f1.cct"
cleanup_codegen_artifacts "tests/integration/random_real_basic_11f1.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_math_random_basic_11f1.cct"
cleanup_codegen_artifacts "tests/integration/parse_int_valid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/parse_int_invalid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/parse_real_valid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/parse_real_invalid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/parse_bool_valid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/parse_bool_invalid_11f2.cct"
cleanup_codegen_artifacts "tests/integration/cmp_int_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/cmp_real_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/cmp_verbum_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/alg_binary_search_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/alg_sort_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_parse_cmp_basic_11f2.cct"
cleanup_codegen_artifacts "tests/integration/showcase_string_11g.cct"
cleanup_codegen_artifacts "tests/integration/showcase_collection_11g.cct"
cleanup_codegen_artifacts "tests/integration/showcase_io_fs_11g.cct"
cleanup_codegen_artifacts "tests/integration/showcase_parse_math_random_11g.cct"
cleanup_codegen_artifacts "tests/integration/showcase_modular_11g_main.cct"
cleanup_codegen_artifacts "tests/integration/sigilo_showcase_stdlib_11g.cct"
cleanup_codegen_artifacts "tests/integration/modules_11g/showcase_mod_core_11g.cct"
cleanup_codegen_artifacts "tests/integration/modules_11g/showcase_mod_stats_11g.cct"
cleanup_codegen_artifacts "tests/integration/modules_11g/showcase_mod_io_11g.cct"
cleanup_codegen_artifacts "examples/showcase_stdlib_string_11g.cct"
cleanup_codegen_artifacts "examples/showcase_stdlib_collection_11g.cct"
cleanup_codegen_artifacts "examples/showcase_stdlib_io_fs_11g.cct"
cleanup_codegen_artifacts "examples/showcase_stdlib_parse_math_random_11g.cct"
cleanup_codegen_artifacts "examples/showcase_stdlib_modular_11g_main.cct"
cleanup_codegen_artifacts "examples/modules_11g/showcase_mod_core_11g.cct"
cleanup_codegen_artifacts "examples/modules_11g/showcase_mod_stats_11g.cct"
cleanup_codegen_artifacts "examples/modules_11g/showcase_mod_io_11g.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/lib/util.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/tests/basic.test.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/tests/lint_warn.test.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/tests/fmt_bad.test.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_basic/bench/basic.bench.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_incremental/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_incremental/lib/mod.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_pattern/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_pattern/tests/a_math.test.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_pattern/tests/b_io.test.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_bench/src/main.cct"
cleanup_codegen_artifacts "tests/integration/project_12f_bench/bench/calc.bench.cct"
cleanup_codegen_artifacts "examples/project_minimal_12f/src/main.cct"
cleanup_codegen_artifacts "examples/project_minimal_12f/tests/smoke.test.cct"
cleanup_codegen_artifacts "examples/project_minimal_12f/bench/smoke.bench.cct"
cleanup_codegen_artifacts "examples/project_modular_12f/src/main.cct"
cleanup_codegen_artifacts "examples/project_modular_12f/lib/math.cct"
cleanup_codegen_artifacts "examples/project_modular_12f/tests/math.test.cct"
cleanup_codegen_artifacts "tests/integration/modules_10d/module_constraint_contract_10d.cct"
cleanup_codegen_artifacts "tests/integration/modules_10d/module_constraint_impl_10d.cct"
cleanup_codegen_artifacts "tests/integration/modules_10d/module_constraint_transitive_mid_10d.cct"
cleanup_codegen_artifacts "tests/integration/modules_10d/module_constraint_transitive_contract_10d.cct"
rm -f tests/fixtures/io_fs/output_test.txt
rm -f tests/fixtures/io_fs/append_test.txt
rm -f tests/fixtures/io_fs/size_test.txt
rm -f tests/fixtures/io_fs/verbum_test.txt
rm -f tests/fixtures/io_fs/sigilo_io_fs.txt
rm -f tests/fixtures/io_fs/path_write_11e2.txt
rm -f tests/fixtures/io_fs/showcase_path_11e2.log
rm -f tests/fixtures/io_fs/showcase_io_fs_11g.txt
rm -f tests/tmp_sigilo_11g.txt
rm -f examples/showcase_io_fs_11g.tmp
rm -rf tests/integration/docgen_basic_12g/docs/api
rm -rf tests/integration/docgen_visibility_12g/docs/api
rm -rf tests/integration/docgen_bad_tags_12g/docs/api
rm -rf tests/integration/docgen_determinism_12g/docs/api
rm -rf /tmp/cct_doc_det_a /tmp/cct_doc_det_b
rm -rf tests/integration/project_12f_basic/.cct tests/integration/project_12f_basic/dist
rm -rf tests/integration/project_12f_incremental/.cct tests/integration/project_12f_incremental/dist
rm -rf tests/integration/project_12f_pattern/.cct tests/integration/project_12f_pattern/dist
rm -rf tests/integration/project_12f_bench/.cct tests/integration/project_12f_bench/dist
rm -f tests/integration/sigilo_minimal.svg.ref tests/integration/sigilo_minimal.sigil.ref
rm -f tests/integration/sigilo_mod_entry.system.ref.sigil
rm -f examples/hello.svg examples/hello.sigil
rm -f tests/integration/tmp_sig_style_network.svg tests/integration/tmp_sig_style_network.sigil
rm -f tests/integration/tmp_sig_style_seal.svg tests/integration/tmp_sig_style_seal.sigil
rm -f tests/integration/tmp_sig_style_scriptum.svg tests/integration/tmp_sig_style_scriptum.sigil
rm -f tests/integration/tmp_sig_style_seal.ref.svg tests/integration/tmp_sig_style_seal.ref.sigil
rm -f tests/integration/tmp_sig_out_custom.svg tests/integration/tmp_sig_out_custom.sigil
rm -f tests/integration/tmp_sig_9d_complete.svg tests/integration/tmp_sig_9d_complete.sigil
rm -f tests/integration/tmp_sig_9d_complete.system.svg tests/integration/tmp_sig_9d_complete.system.sigil
rm -f tests/integration/tmp_sig_9d_complete.__mod_001.svg tests/integration/tmp_sig_9d_complete.__mod_001.sigil
rm -f tests/integration/tmp_sig_9d_complete.__mod_002.svg tests/integration/tmp_sig_9d_complete.__mod_002.sigil
rm -f tests/integration/tmp_sig_9d_complete_compile.svg tests/integration/tmp_sig_9d_complete_compile.sigil
rm -f tests/integration/tmp_sig_9d_complete_compile.system.svg tests/integration/tmp_sig_9d_complete_compile.system.sigil
rm -f tests/integration/tmp_sig_9d_complete_compile.__mod_001.svg tests/integration/tmp_sig_9d_complete_compile.__mod_001.sigil
rm -f tests/integration/tmp_sig_9d_complete_compile.__mod_002.svg tests/integration/tmp_sig_9d_complete_compile.__mod_002.sigil
rm -f tests/integration/tmp_sig_9d_essential.svg tests/integration/tmp_sig_9d_essential.sigil
rm -f tests/integration/tmp_sig_9d_essential.system.svg tests/integration/tmp_sig_9d_essential.system.sigil
rm -f tests/integration/tmp_sig_9d_essential.__mod_001.svg tests/integration/tmp_sig_9d_essential.__mod_001.sigil
rm -f tests/integration/tmp_sig_9d_nometa.svg tests/integration/tmp_sig_9d_nometa.sigil
rm -f tests/integration/tmp_sig_9d_nometa.system.svg tests/integration/tmp_sig_9d_nometa.system.sigil
rm -f tests/integration/tmp_sig_9d_nometa.__mod_001.svg tests/integration/tmp_sig_9d_nometa.__mod_001.sigil
rm -f tests/integration/tmp_sig_9d_nometa.__mod_002.svg tests/integration/tmp_sig_9d_nometa.__mod_002.sigil
rm -f tests/integration/tmp_sig_9d_nosvg.svg tests/integration/tmp_sig_9d_nosvg.sigil
rm -f tests/integration/tmp_sig_9d_nosvg.system.svg tests/integration/tmp_sig_9d_nosvg.system.sigil
rm -f tests/integration/tmp_sig_9d_nosvg.__mod_001.svg tests/integration/tmp_sig_9d_nosvg.__mod_001.sigil
rm -f tests/integration/tmp_sig_9d_nosvg.__mod_002.svg tests/integration/tmp_sig_9d_nosvg.__mod_002.sigil
rm -f tests/integration/tmp_sig_9e_complete.svg tests/integration/tmp_sig_9e_complete.sigil
rm -f tests/integration/tmp_sig_9e_complete.system.svg tests/integration/tmp_sig_9e_complete.system.sigil
rm -f tests/integration/tmp_sig_9e_complete.__mod_001.svg tests/integration/tmp_sig_9e_complete.__mod_001.sigil
rm -f tests/integration/tmp_sig_9e_complete.__mod_002.svg tests/integration/tmp_sig_9e_complete.__mod_002.sigil
rm -f tests/integration/tmp_sig_9e_nometa.svg tests/integration/tmp_sig_9e_nometa.sigil
rm -f tests/integration/tmp_sig_9e_nometa.system.svg tests/integration/tmp_sig_9e_nometa.system.sigil
rm -f tests/integration/tmp_sig_9e_nometa.__mod_001.svg tests/integration/tmp_sig_9e_nometa.__mod_001.sigil
rm -f tests/integration/tmp_sig_9e_nometa.__mod_002.svg tests/integration/tmp_sig_9e_nometa.__mod_002.sigil
rm -f tests/integration/tmp_sig_10a_complete.svg tests/integration/tmp_sig_10a_complete.sigil
rm -f tests/integration/tmp_sig_10a_complete.system.svg tests/integration/tmp_sig_10a_complete.system.sigil
rm -f tests/integration/tmp_sig_10a_complete.__mod_001.svg tests/integration/tmp_sig_10a_complete.__mod_001.sigil
rm -f tests/integration/tmp_sig_10a_complete.__mod_002.svg tests/integration/tmp_sig_10a_complete.__mod_002.sigil
rm -f tests/integration/tmp_sig_10e_complete.svg tests/integration/tmp_sig_10e_complete.sigil
rm -f tests/integration/tmp_sig_10e_complete.system.svg tests/integration/tmp_sig_10e_complete.system.sigil
rm -f tests/integration/tmp_sig_10e_complete.__mod_001.svg tests/integration/tmp_sig_10e_complete.__mod_001.sigil
rm -f tests/integration/tmp_sig_10e_complete.__mod_002.svg tests/integration/tmp_sig_10e_complete.__mod_002.sigil
rm -f tests/integration/tmp_sig_10e_nometa.svg tests/integration/tmp_sig_10e_nometa.sigil
rm -f tests/integration/tmp_sig_10e_nometa.system.svg tests/integration/tmp_sig_10e_nometa.system.sigil
rm -f tests/integration/tmp_sig_10e_nometa.__mod_001.svg tests/integration/tmp_sig_10e_nometa.__mod_001.sigil
rm -f tests/integration/tmp_sig_10e_nometa.__mod_002.svg tests/integration/tmp_sig_10e_nometa.__mod_002.sigil
rm -f tests/integration/tmp_sig_10e_nosvg.svg tests/integration/tmp_sig_10e_nosvg.sigil
rm -f tests/integration/tmp_sig_10e_nosvg.system.svg tests/integration/tmp_sig_10e_nosvg.system.sigil
rm -f tests/integration/tmp_sig_10e_nosvg.__mod_001.svg tests/integration/tmp_sig_10e_nosvg.__mod_001.sigil
rm -f tests/integration/tmp_sig_10e_nosvg.__mod_002.svg tests/integration/tmp_sig_10e_nosvg.__mod_002.sigil
rm -f tests/integration/tmp_sig_11g_complete.svg tests/integration/tmp_sig_11g_complete.sigil
rm -f tests/integration/tmp_sig_11g_complete.system.svg tests/integration/tmp_sig_11g_complete.system.sigil
rm -f tests/integration/tmp_sig_11g_complete.__mod_001.svg tests/integration/tmp_sig_11g_complete.__mod_001.sigil
rm -f tests/integration/tmp_sig_11g_complete.__mod_002.svg tests/integration/tmp_sig_11g_complete.__mod_002.sigil
rm -f tests/integration/tmp_sig_11g_complete.__mod_003.svg tests/integration/tmp_sig_11g_complete.__mod_003.sigil
rm -f tests/runtime/test_fluxus_storage

# Summary
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
