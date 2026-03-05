#!/bin/bash
#
# CCT — FASE 13A.1 test runner
#

set -euo pipefail

echo "FASE 13A.1 — Sigil Parse Tests"
echo "========================================"

make test_sigil_parse

echo ""
echo "FASE 13A.1 tests: ok"
