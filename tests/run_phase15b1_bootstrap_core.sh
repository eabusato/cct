#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

B1_PLAN="md_out/docs/release/FASE_15B1_BOOTSTRAP_CORE_PLAN.md"
B1_MATRIX="md_out/docs/release/FASE_15B1_TEST_MATRIX.md"
B1_AR="md_out/docs/release/FASE_15B1_ACCEPTANCE_ROLLBACK.md"
B1_RISK="md_out/docs/release/FASE_15B1_RISK_OWNERSHIP_MAP.md"
B1_HANDOFF="md_out/docs/release/FASE_15B1_HANDOFF_15B2.md"

for f in "$B1_PLAN" "$B1_MATRIX" "$B1_AR" "$B1_RISK" "$B1_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "portabilidade ASM" "$B1_PLAN"
grep -q "caminho canônico permanece padrão" "$B1_PLAN"
grep -q "trilha bootstrap é opt-in" "$B1_PLAN"

grep -q "10 verificações objetivas" "$B1_MATRIX"
grep -q "runner dedicado" "$B1_MATRIX"
grep -q "coesão 15A4 -> 15B1" "$B1_MATRIX"

grep -q "A 15B1 é considerada concluída" "$B1_AR"
grep -q 'divergência `BLOCKER` sem owner' "$B1_AR"
grep -q 'decisão final por execução: `pass`, `watch` ou `block`' "$B1_AR"

grep -q "RISK-15B1-001" "$B1_RISK"
grep -q "Compiler Core Lead" "$B1_RISK"
grep -q "Bootstrap Governance Lead" "$B1_RISK"

grep -q "Pronto para 15B2" "$B1_HANDOFF"
grep -q "modelo de dados de frontend" "$B1_HANDOFF"

echo "phase15b1 bootstrap core audit: PASS"
