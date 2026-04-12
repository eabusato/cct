#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

C4_PLAN="md_out/docs/release/FASE_15C4_PERFORMANCE_COST_PLAN.md"
C4_BASE="md_out/docs/release/FASE_15C4_COST_BASELINE.md"
C4_BUDGET="md_out/docs/release/FASE_15C4_BUDGET_BY_SCENARIO.md"
C4_GUARD="md_out/docs/release/FASE_15C4_GUARDRAILS.md"
C4_LIMITS="md_out/docs/release/FASE_15C4_KNOWN_LIMITS.md"
C4_TEST="md_out/docs/release/FASE_15C4_TEST_MATRIX.md"
C4_ACCEPT="md_out/docs/release/FASE_15C4_ACCEPTANCE_ROLLBACK.md"
C4_RISK="md_out/docs/release/FASE_15C4_RISK_OWNERSHIP_MAP.md"
C4_HANDOFF="md_out/docs/release/FASE_15C4_HANDOFF_15D1.md"

for f in "$C4_PLAN" "$C4_BASE" "$C4_BUDGET" "$C4_GUARD" "$C4_LIMITS" "$C4_TEST" "$C4_ACCEPT" "$C4_RISK" "$C4_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "custo de convivência" "$C4_PLAN"
grep -q "guardrails" "$C4_PLAN"
grep -q "trilha bootstrap" "$C4_PLAN"

grep -q "CB-15C4-001" "$C4_BASE"
grep -q "CB-15C4-004" "$C4_BASE"
grep -q "baseline" "$C4_BASE"

grep -q "BG-15C4-001" "$C4_BUDGET"
grep -q "BG-15C4-004" "$C4_BUDGET"
grep -q "pass" "$C4_BUDGET"

grep -q "GR-15C4-001" "$C4_GUARD"
grep -q "GR-15C4-005" "$C4_GUARD"
grep -q "fallback explícito" "$C4_GUARD"

grep -q "LIM-15C4-001" "$C4_LIMITS"
grep -q "LIM-15C4-004" "$C4_LIMITS"
grep -q "Mitigação" "$C4_LIMITS"

grep -q "10 validações objetivas" "$C4_TEST"
grep -q "coerência 15C3 -> 15C4" "$C4_TEST"

grep -q '`pass`: orçamento respeitado' "$C4_ACCEPT"
grep -q '`block`: estouro severo de custo' "$C4_ACCEPT"

grep -q "RISK-15C4-001" "$C4_RISK"
grep -q "Performance Budget Lead" "$C4_RISK"
grep -q "baseline de risco só atualiza com evidência" "$C4_RISK"

grep -q "Pronto para 15D1" "$C4_HANDOFF"
grep -q "promoção/reversão" "$C4_HANDOFF"

echo "phase15c4 performance-cost guardrails audit: PASS"
