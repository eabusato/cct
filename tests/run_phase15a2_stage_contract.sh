#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

A2_STAGE="md_out/docs/release/FASE_15A2_STAGE_CONTRACT.md"
A2_GATES="md_out/docs/release/FASE_15A2_STAGE_GATES.md"
A2_POLICY="md_out/docs/release/FASE_15A2_PROMOTION_REVERSAL_POLICY.md"
A2_FALLBACK="md_out/docs/release/FASE_15A2_FALLBACK_MATRIX.md"
A2_EXEC_DOC="md_out/FASE_15A2_CCT.md"

for f in "$A2_EXEC_DOC" "$A2_STAGE" "$A2_GATES" "$A2_POLICY" "$A2_FALLBACK"; do
  [ -f "$f" ]
done

grep -q "EXECUTION CONTRACT — FASE 15A2" "$A2_EXEC_DOC"
grep -q "Prepara para: 15A3" "$A2_EXEC_DOC"
grep -q "REPETE" "$A2_EXEC_DOC"

grep -q "S0" "$A2_STAGE"
grep -q "S1" "$A2_STAGE"
grep -q "S2" "$A2_STAGE"
grep -q "S3" "$A2_STAGE"
grep -q "trilha canônica C tem precedência" "$A2_STAGE"

grep -q "GATE-15A2-S0-EXIT" "$A2_GATES"
grep -q "GATE-15A2-S1-ENTRY" "$A2_GATES"
grep -q "GATE-15A2-S1-EXIT" "$A2_GATES"
grep -q "GATE-15A2-S2-ENTRY" "$A2_GATES"
grep -q "GATE-15A2-S3-ENTRY" "$A2_GATES"

grep -q "promoção exige evidência" "$A2_POLICY"
grep -q "reversão é imediata" "$A2_POLICY"
grep -q "waiver" "$A2_POLICY"

grep -q "Trigger de fallback" "$A2_FALLBACK"
grep -q "S1" "$A2_FALLBACK"
grep -q "S2" "$A2_FALLBACK"
grep -q "S3" "$A2_FALLBACK"
grep -q "fallback é parte do sucesso da fase" "$A2_FALLBACK"

echo "phase15a2 stage-contract audit: PASS"
