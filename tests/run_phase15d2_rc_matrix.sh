#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

D2_PLAN="md_out/docs/release/FASE_15D2_DUAL_RC_VALIDATION_PLAN.md"
D2_MATRIX="md_out/docs/release/FASE_15D2_RC_MATRIX.md"
D2_SMOKE="md_out/docs/release/FASE_15D2_CRITICAL_SMOKE_SET.md"
D2_BW="md_out/docs/release/FASE_15D2_BLOCKER_WAIVER_POLICY.md"
D2_DEC="md_out/docs/release/FASE_15D2_DECISION_RECOMMENDATION.md"
D2_TEST="md_out/docs/release/FASE_15D2_TEST_MATRIX.md"
D2_ACCEPT="md_out/docs/release/FASE_15D2_ACCEPTANCE_ROLLBACK.md"
D2_RISK="md_out/docs/release/FASE_15D2_RISK_OWNERSHIP_MAP.md"
D2_HANDOFF="md_out/docs/release/FASE_15D2_HANDOFF_15D3.md"

for f in "$D2_PLAN" "$D2_MATRIX" "$D2_SMOKE" "$D2_BW" "$D2_DEC" "$D2_TEST" "$D2_ACCEPT" "$D2_RISK" "$D2_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "validação de RC" "$D2_PLAN"
grep -q "matriz RC dual" "$D2_PLAN"
grep -q "decisão recomendada" "$D2_PLAN"

grep -q "RC-15D2-001" "$D2_MATRIX"
grep -q "RC-15D2-004" "$D2_MATRIX"
grep -q "blocker" "$D2_MATRIX"

grep -q "SMK-15D2-001" "$D2_SMOKE"
grep -q "SMK-15D2-004" "$D2_SMOKE"
grep -q "smoke crítico" "$D2_SMOKE"

grep -q "BW-15D2-001" "$D2_BW"
grep -q "BW-15D2-004" "$D2_BW"
grep -q "Waiver" "$D2_BW"

grep -q "pass" "$D2_DEC"
grep -q "watch" "$D2_DEC"
grep -q "block" "$D2_DEC"

grep -q "10 validações objetivas" "$D2_TEST"
grep -q "coerência 15D1 -> 15D2" "$D2_TEST"

grep -q '`pass`: RC validado sem blocker' "$D2_ACCEPT"
grep -q '`block`: blocker ativo' "$D2_ACCEPT"

grep -q "RISK-15D2-001" "$D2_RISK"
grep -q "RC Validation Lead" "$D2_RISK"
grep -q "baseline de risco só atualiza com evidência" "$D2_RISK"

grep -q "Pronto para 15D3" "$D2_HANDOFF"
grep -q "artefatos finais da fase" "$D2_HANDOFF"

echo "phase15d2 rc-matrix audit: PASS"
