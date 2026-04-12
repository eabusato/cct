#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

C2_PLAN="md_out/docs/release/FASE_15C2_CONVERGENCE_BASELINE_PLAN.md"
C2_MATRIX="md_out/docs/release/FASE_15C2_FUNCTIONAL_CONVERGENCE_MATRIX.md"
C2_TARGETS="md_out/docs/release/FASE_15C2_TARGETS_AND_THRESHOLDS.md"
C2_GAPS="md_out/docs/release/FASE_15C2_GAP_ANALYSIS.md"
C2_BLOCK="md_out/docs/release/FASE_15C2_BLOCKING_CRITERIA.md"
C2_TEST="md_out/docs/release/FASE_15C2_TEST_MATRIX.md"
C2_ACCEPT="md_out/docs/release/FASE_15C2_ACCEPTANCE_ROLLBACK.md"
C2_RISK="md_out/docs/release/FASE_15C2_RISK_OWNERSHIP_MAP.md"
C2_HANDOFF="md_out/docs/release/FASE_15C2_HANDOFF_15C3.md"

for f in "$C2_PLAN" "$C2_MATRIX" "$C2_TARGETS" "$C2_GAPS" "$C2_BLOCK" "$C2_TEST" "$C2_ACCEPT" "$C2_RISK" "$C2_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "baseline quantitativo" "$C2_PLAN"
grep -q "D1" "$C2_PLAN"
grep -q "D4" "$C2_PLAN"

grep -q "FC-15C2-001" "$C2_MATRIX"
grep -q "FC-15C2-004" "$C2_MATRIX"
grep -q "blocker" "$C2_MATRIX"

grep -q "META-15C2-D1" "$C2_TARGETS"
grep -q "META-15C2-D4" "$C2_TARGETS"
grep -q "pass" "$C2_TARGETS"

grep -q "GAP-15C2-001" "$C2_GAPS"
grep -q "GAP-15C2-004" "$C2_GAPS"
grep -q "BLOCKER" "$C2_GAPS"

grep -q "BC-15C2-001" "$C2_BLOCK"
grep -q "BC-15C2-005" "$C2_BLOCK"
grep -q "Desbloqueio" "$C2_BLOCK"

grep -q "10 validações objetivas" "$C2_TEST"
grep -q "coesão 15C1 -> 15C2" "$C2_TEST"

grep -q 'pass`: convergência mínima' "$C2_ACCEPT"
grep -q '`block`: blocker ativo' "$C2_ACCEPT"

grep -q "RISK-15C2-001" "$C2_RISK"
grep -q "Functional Convergence Lead" "$C2_RISK"
grep -q "baseline de risco só atualiza com evidência" "$C2_RISK"

grep -q "Pronto para 15C3" "$C2_HANDOFF"
grep -q "estabilidade operacional" "$C2_HANDOFF"

echo "phase15c2 convergence-matrix audit: PASS"
