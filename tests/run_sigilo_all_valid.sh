#!/bin/bash
#
# CCT — Clavicula Turing
# Sigilo Batch Generator / Validation (FASE 5)
#
# Generates sigils for every semantic-valid .cct fixture in tests/integration
# (and selected examples) using `--sigilo-only`.
#
# Useful for:
# - visual regression spot checks
# - validating sigilo coverage on the test corpus
# - generating a local gallery of `.svg`/`.sigil` artifacts
#
# Copyright (c) Erick Andrade Busato. Todos os direitos reservados.

set -u

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ -z "${CCT_BIN:-}" ]; then
    if [ -x ./cct-host ]; then
        CCT_BIN="./cct-host"
    else
        CCT_BIN="./cct"
    fi
fi

GENERATED_OK=0
SKIPPED_INVALID=0
FAILED=0

if [ ! -x "$CCT_BIN" ]; then
    echo -e "${RED}Error:${NC} CCT binary not found/executable at $CCT_BIN"
    echo "Run 'make' first."
    exit 1
fi

echo "CCT — Sigilo Batch (semantic-valid fixtures)"
echo "Binary: $CCT_BIN"
echo ""

cleanup_sigilo_artifacts() {
    local src="$1"
    local base="${src%.cct}"
    rm -f "$base.svg" "$base.sigil"
}

process_file() {
    local src="$1"
    local base="${src%.cct}"

    cleanup_sigilo_artifacts "$src"

    # Only semantic-valid programs should produce sigils in this batch.
    if ! "$CCT_BIN" --check "$src" > /dev/null 2>&1; then
        echo -e "${YELLOW}SKIP${NC} $src (not semantic-valid in current subset)"
        ((SKIPPED_INVALID++))
        return 0
    fi

    if "$CCT_BIN" --sigilo-only "$src" > /dev/null 2>&1 && \
       [ -f "$base.svg" ] && [ -f "$base.sigil" ]; then
        echo -e "${GREEN}OK${NC}   $src -> $base.svg, $base.sigil"
        ((GENERATED_OK++))
        return 0
    fi

    echo -e "${RED}FAIL${NC} $src (sigilo generation failed)"
    ((FAILED++))
    return 0
}

while IFS= read -r src; do
    process_file "$src"
done < <(find tests/integration -maxdepth 1 -type f -name '*.cct' | sort)

# Include canonical example(s) as part of the batch gallery.
if [ -f "examples/hello.cct" ]; then
    process_file "examples/hello.cct"
fi

echo ""
echo "Summary:"
echo -e "  ${GREEN}Generated:${NC} $GENERATED_OK"
echo -e "  ${YELLOW}Skipped:${NC}   $SKIPPED_INVALID"
echo -e "  ${RED}Failed:${NC}    $FAILED"

if [ "$FAILED" -eq 0 ]; then
    echo -e "${GREEN}Sigilo batch completed without generation failures.${NC}"
    exit 0
fi

echo -e "${RED}Sigilo batch completed with failures.${NC}"
exit 1
