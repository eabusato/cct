#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

A3_EQ="md_out/docs/release/FASE_15A3_EQUIVALENCE_MATRIX.md"
A3_DIV="md_out/docs/release/FASE_15A3_DIVERGENCE_POLICY.md"
A3_BLK="md_out/docs/release/FASE_15A3_BLOCKING_CRITERIA.md"
A3_DET="md_out/docs/release/FASE_15A3_DETERMINISM_CONTRACT.md"

for f in "$A3_EQ" "$A3_DIV" "$A3_BLK" "$A3_DET"; do
  [ -f "$f" ]
done

grep -q "EQ-15A3-001" "$A3_EQ"
grep -q "EQ-15A3-002" "$A3_EQ"
grep -q "EQ-15A3-003" "$A3_EQ"
grep -q "EQ-15A3-004" "$A3_EQ"

grep -q "BLOCKER" "$A3_DIV"
grep -q "REVIEW" "$A3_DIV"
grep -q "INFO" "$A3_DIV"
grep -q "Tolerâncias proibidas" "$A3_DIV"

grep -q "BLK-15A3-001" "$A3_BLK"
grep -q "BLK-15A3-002" "$A3_BLK"
grep -q "BLK-15A3-003" "$A3_BLK"

grep -q "mesma entrada + mesmo perfil -> mesma classe de resultado" "$A3_DET"
grep -q "runner dedicado de auditoria 15A3" "$A3_DET"

echo "phase15a3 equivalence contract audit: PASS"
