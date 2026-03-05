#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

D1_PLAN="md_out/docs/release/FASE_15D1_PROMOTION_REVERSAL_POLICY_PLAN.md"
D1_PROM="md_out/docs/release/FASE_15D1_PROMOTION_TRIGGERS.md"
D1_REV="md_out/docs/release/FASE_15D1_REVERSAL_TRIGGERS.md"
D1_SAFE="md_out/docs/release/FASE_15D1_SAFETY_CHECKLIST.md"
D1_RUN="md_out/docs/release/FASE_15D1_FAST_ROLLBACK_RUNBOOK.md"
D1_TEST="md_out/docs/release/FASE_15D1_TEST_MATRIX.md"
D1_ACCEPT="md_out/docs/release/FASE_15D1_ACCEPTANCE_ROLLBACK.md"
D1_RISK="md_out/docs/release/FASE_15D1_RISK_OWNERSHIP_MAP.md"
D1_HANDOFF="md_out/docs/release/FASE_15D1_HANDOFF_15D2.md"

for f in "$D1_PLAN" "$D1_PROM" "$D1_REV" "$D1_SAFE" "$D1_RUN" "$D1_TEST" "$D1_ACCEPT" "$D1_RISK" "$D1_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "promoção/reversão" "$D1_PLAN"
grep -q "rollback rápido" "$D1_PLAN"
grep -q "fallback canônico" "$D1_PLAN"

grep -q "PRM-15D1-001" "$D1_PROM"
grep -q "PRM-15D1-004" "$D1_PROM"
grep -q "promoção" "$D1_PROM"

grep -q "REV-15D1-001" "$D1_REV"
grep -q "REV-15D1-004" "$D1_REV"
grep -q "reversão" "$D1_REV"

grep -q "SC-15D1-001" "$D1_SAFE"
grep -q "SC-15D1-005" "$D1_SAFE"
grep -q "Checklist" "$D1_SAFE"

grep -q "RB-15D1" "$D1_RUN"
grep -q "pass\`/\`watch\`/\`block" "$D1_RUN"
grep -q "SLA" "$D1_RUN"

grep -q "10 validações objetivas" "$D1_TEST"
grep -q "coerência 15C4 -> 15D1" "$D1_TEST"

grep -q '`pass`: promoção autorizada' "$D1_ACCEPT"
grep -q '`block`: gatilho de reversão ativo' "$D1_ACCEPT"

grep -q "RISK-15D1-001" "$D1_RISK"
grep -q "Release Governance Lead" "$D1_RISK"
grep -q "baseline de risco só atualiza com evidência" "$D1_RISK"

grep -q "Pronto para 15D2" "$D1_HANDOFF"
grep -q "matriz RC da trilha dual" "$D1_HANDOFF"

echo "phase15d1 promotion-reversal policy audit: PASS"
