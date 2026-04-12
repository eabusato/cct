#!/bin/bash
#
# CCT — FASE 13A.2 test runner
#

set -euo pipefail

echo "FASE 13A.2 — Sigil Diff Tests"
echo "========================================"

make test_sigil_diff

echo ""
echo "FASE 13A.2 tests: ok"
