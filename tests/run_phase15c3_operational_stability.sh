#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

C3_PLAN="md_out/docs/release/FASE_15C3_OPERATIONAL_STABILITY_PLAN.md"
C3_CAMPAIGNS="md_out/docs/release/FASE_15C3_REPETITION_CAMPAIGNS.md"
C3_FLAKY="md_out/docs/release/FASE_15C3_FLAKINESS_METRICS.md"
C3_INC="md_out/docs/release/FASE_15C3_INCIDENT_RESPONSE_AND_MITIGATION.md"
C3_ASM="md_out/docs/release/FASE_15C3_ASM_OUTPUT_EQUIVALENCE_CONTRACT.md"
C3_TEST="md_out/docs/release/FASE_15C3_TEST_MATRIX.md"
C3_ACCEPT="md_out/docs/release/FASE_15C3_ACCEPTANCE_ROLLBACK.md"
C3_RISK="md_out/docs/release/FASE_15C3_RISK_OWNERSHIP_MAP.md"
C3_HANDOFF="md_out/docs/release/FASE_15C3_HANDOFF_15C4.md"

for f in "$C3_PLAN" "$C3_CAMPAIGNS" "$C3_FLAKY" "$C3_INC" "$C3_ASM" "$C3_TEST" "$C3_ACCEPT" "$C3_RISK" "$C3_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "estabilidade operacional" "$C3_PLAN"
grep -q "fallback explícito" "$C3_PLAN"
grep -q "trilha bootstrap" "$C3_PLAN"

grep -q "CAMP-15C3-001" "$C3_CAMPAIGNS"
grep -q "CAMP-15C3-004" "$C3_CAMPAIGNS"
grep -q "3 repetições" "$C3_CAMPAIGNS"

grep -q "FM-15C3-001" "$C3_FLAKY"
grep -q "FM-15C3-004" "$C3_FLAKY"
grep -q "block" "$C3_FLAKY"

grep -q "INC-15C3-001" "$C3_INC"
grep -q "INC-15C3-004" "$C3_INC"
grep -q "pós-mitigação" "$C3_INC"

grep -q "ASM-15C3-001" "$C3_ASM"
grep -q "ASM-15C3-005" "$C3_ASM"
grep -q "pass\`, \`watch\` ou \`block\`" "$C3_ASM"

grep -q "10 validações objetivas" "$C3_TEST"
grep -q "coerência 15C2 -> 15C3" "$C3_TEST"

grep -q '`pass`: estabilidade operacional confirmada' "$C3_ACCEPT"
grep -q '`block`: blocker ativo' "$C3_ACCEPT"

grep -q "RISK-15C3-001" "$C3_RISK"
grep -q "Operational Stability Lead" "$C3_RISK"
grep -q "baseline de risco só atualiza com evidência" "$C3_RISK"

grep -q "Pronto para 15C4" "$C3_HANDOFF"
grep -q "custo de convivência dual" "$C3_HANDOFF"

echo "phase15c3 operational-stability audit: PASS"
