#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

D4_PLAN="md_out/docs/release/FASE_15D4_CLOSURE_GATE_PLAN.md"
D4_DECISION="md_out/docs/release/FASE_15D4_OFFICIAL_DECISION_RECORD.md"
D4_BACKLOG="md_out/docs/release/FASE_15D4_TRANSITION_BACKLOG.md"
D4_CONT="md_out/docs/release/FASE_15D4_CONTINUITY_ROLLBACK_PLAN.md"
D4_SUMMARY="md_out/docs/release/FASE_15D4_PHASE15_CLOSURE_SUMMARY.md"
D4_TEST="md_out/docs/release/FASE_15D4_TEST_MATRIX.md"
D4_ACCEPT="md_out/docs/release/FASE_15D4_ACCEPTANCE_ROLLBACK.md"
D4_RISK="md_out/docs/release/FASE_15D4_RISK_OWNERSHIP_MAP.md"
D4_HANDOFF="md_out/docs/release/FASE_15D4_HANDOFF_FASE16.md"

for f in "$D4_PLAN" "$D4_DECISION" "$D4_BACKLOG" "$D4_CONT" "$D4_SUMMARY" "$D4_TEST" "$D4_ACCEPT" "$D4_RISK" "$D4_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "closure gate" "$D4_PLAN"
grep -q "fechamento da FASE 15" "$D4_PLAN"

grep -q "DEC-15D4-001" "$D4_DECISION"
grep -q "DEC-15D4-004" "$D4_DECISION"
grep -q "decisão oficial" "$D4_DECISION"

grep -q "TB-15D4-001" "$D4_BACKLOG"
grep -q "TB-15D4-004" "$D4_BACKLOG"
grep -q "não bloqueantes" "$D4_BACKLOG"

grep -q "RB-15D4-001" "$D4_CONT"
grep -q "RB-15D4-004" "$D4_CONT"
grep -q "continuidade" "$D4_CONT"

grep -q "Sumário de Fechamento da FASE 15" "$D4_SUMMARY"
grep -q "trilha pronta para transição controlada para FASE 16" "$D4_SUMMARY"

grep -q "10 validações objetivas" "$D4_TEST"
grep -q "coerência 15D3 -> 15D4" "$D4_TEST"

grep -q '`pass`: closure gate final aprovado com decisão oficial e backlog de transição controlado' "$D4_ACCEPT"
grep -q '`block`: regressão crítica, evidência inconsistente ou decisão sem rastreabilidade' "$D4_ACCEPT"

grep -q "RISK-15D4-001" "$D4_RISK"
grep -q "Phase Closure Lead" "$D4_RISK"
grep -q "baseline de risco só atualiza com evidência" "$D4_RISK"

grep -q "Pronto para FASE 16" "$D4_HANDOFF"
grep -q "arquitetura executável da FASE 16" "$D4_HANDOFF"

echo "phase15d4 phase-closure audit: PASS"
