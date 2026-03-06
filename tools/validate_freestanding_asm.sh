#!/usr/bin/env bash

set -u

PREFIX="[validate-fs-asm]"

fail() {
    echo "$PREFIX FAIL: $1" >&2
    exit 1
}

ok() {
    echo "$PREFIX OK: $1"
}

if [ "$#" -ne 1 ]; then
    fail "usage: tools/validate_freestanding_asm.sh <path/to/file.cgen.s>"
fi

ASM_FILE="$1"
if [ ! -f "$ASM_FILE" ]; then
    fail "input file not found: $ASM_FILE"
fi

if ! command -v as >/dev/null 2>&1; then
    fail "required tool not found: as"
fi
if ! command -v nm >/dev/null 2>&1; then
    fail "required tool not found: nm"
fi
if ! command -v objdump >/dev/null 2>&1; then
    fail "required tool not found: objdump"
fi

if ! grep -Eq '^[[:space:]]*\.intel_syntax[[:space:]]+noprefix' "$ASM_FILE"; then
    fail "missing .intel_syntax noprefix header"
fi

FORBIDDEN_LIBC_REGEX='printf|malloc|free|memcpy|memset|strlen|puts|fopen|fclose|sprintf|snprintf|abort|exit'
if grep -E -w "$FORBIDDEN_LIBC_REGEX" "$ASM_FILE" >/dev/null 2>&1; then
    bad_sym="$(grep -E -o -w "$FORBIDDEN_LIBC_REGEX" "$ASM_FILE" | head -n 1)"
    fail "forbidden libc symbol in asm: ${bad_sym:-unknown}"
fi

FORBIDDEN_HELPER_REGEX='__stack_chk_fail|__stack_chk_guard|__udivdi3|__divdi3|__umoddi3|__moddi3|__muldi3|__fixunsdfsi|__fixdfsi|__floatsidf|__floatdidf|__adddf3|__subdf3'
if grep -E -w "$FORBIDDEN_HELPER_REGEX" "$ASM_FILE" >/dev/null 2>&1; then
    bad_helper="$(grep -E -o -w "$FORBIDDEN_HELPER_REGEX" "$ASM_FILE" | head -n 1)"
    fail "forbidden compiler helper in asm: ${bad_helper:-unknown}"
fi

invalid_calls="$(awk '
    /^[[:space:]]*call[[:space:]]+/ {
        target = $2;
        gsub(/,/, "", target);
        gsub(/\*/, "", target);
        if (target ~ /^\./) next;
        if (target ~ /^cct_(fn|svc|fs|rt)_/) next;
        if (target ~ /^[0-9]+$/) next;
        print target;
    }
' "$ASM_FILE" | sort -u)"
if [ -n "$invalid_calls" ]; then
    fail "invalid call target namespace(s): $(echo "$invalid_calls" | tr '\n' ' ')"
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_BASE="$ROOT_DIR/tests/.tmp"
mkdir -p "$TMP_BASE"
TMP_DIR="$(mktemp -d "$TMP_BASE/validate_fs_asm.XXXXXX")"
OBJ_FILE="$TMP_DIR/validated.o"

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

if ! as --32 "$ASM_FILE" -o "$OBJ_FILE" >/dev/null 2>&1; then
    fail "assembler failed for input ASM"
fi

UNDEF_SYMS="$(nm "$OBJ_FILE" | awk '$2 == "U" { print $3 }')"
if [ -n "$UNDEF_SYMS" ]; then
    FORBIDDEN_UNDEF="$(echo "$UNDEF_SYMS" | grep -E -w "$FORBIDDEN_LIBC_REGEX|$FORBIDDEN_HELPER_REGEX" || true)"
    if [ -n "$FORBIDDEN_UNDEF" ]; then
        fail "forbidden undefined symbol(s): $(echo "$FORBIDDEN_UNDEF" | tr '\n' ' ')"
    fi
fi

if objdump -h "$OBJ_FILE" | grep -q '\.eh_frame'; then
    fail "section .eh_frame found in object"
fi

DISASM="$(objdump -d "$OBJ_FILE")"
HAS_RET=0
HAS_LEAVE_OR_POP_EBP=0

echo "$DISASM" | grep -Eq '[[:space:]]ret[[:space:]]*$' && HAS_RET=1
echo "$DISASM" | grep -Eq '[[:space:]]leave[[:space:]]*$|[[:space:]]pop[[:space:]]+%ebp[[:space:]]*$' && HAS_LEAVE_OR_POP_EBP=1

if [ "$HAS_RET" -ne 1 ] || [ "$HAS_LEAVE_OR_POP_EBP" -ne 1 ]; then
    fail "missing plausible function epilogue pattern (leave/ret or pop ebp/ret)"
fi

ok "$ASM_FILE"
