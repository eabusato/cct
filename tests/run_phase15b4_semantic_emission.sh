#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

B4_PLAN="md_out/docs/release/FASE_15B4_BOOTSTRAP_SEMANTIC_EMISSION_PLAN.md"
B4_SEM="md_out/docs/release/FASE_15B4_SEMANTIC_SUBSET_CONTRACT.md"
B4_IR="md_out/docs/release/FASE_15B4_INTERMEDIATE_EMISSION_CONTRACT.md"
B4_ASM="md_out/docs/release/FASE_15B4_ASM_OUTPUT_CONTRACT.md"
B4_TEST="md_out/docs/release/FASE_15B4_TEST_MATRIX.md"
B4_ACCEPT="md_out/docs/release/FASE_15B4_ACCEPTANCE_ROLLBACK.md"
B4_RISK="md_out/docs/release/FASE_15B4_RISK_OWNERSHIP_MAP.md"
B4_HANDOFF="md_out/docs/release/FASE_15B4_HANDOFF_15C1.md"

for f in "$B4_PLAN" "$B4_SEM" "$B4_IR" "$B4_ASM" "$B4_TEST" "$B4_ACCEPT" "$B4_RISK" "$B4_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "semântica subset" "$B4_PLAN"
grep -q "fallback canônico obrigatório" "$B4_PLAN"
grep -q "portabilidade ASM" "$B4_PLAN"

grep -q "SS-15B4-001" "$B4_SEM"
grep -q "SS-15B4-005" "$B4_SEM"
grep -q "Fora de escopo" "$B4_SEM"

grep -q "IE-15B4-001" "$B4_IR"
grep -q "IE-15B4-005" "$B4_IR"
grep -q "fallback imediato" "$B4_IR"

grep -q "ASM-15B4-001" "$B4_ASM"
grep -q "ASM-15B4-005" "$B4_ASM"
grep -q "shape" "$B4_ASM"

grep -q "10 validações objetivas" "$B4_TEST"
grep -q "continuidade 15B3 -> 15B4" "$B4_TEST"

grep -q 'divergência `BLOCKER` sem owner' "$B4_ACCEPT"
grep -q '`pass`: sem bloqueios' "$B4_ACCEPT"

grep -q "RISK-15B4-001" "$B4_RISK"
grep -q "Semantic Subset Lead" "$B4_RISK"
grep -q "baseline de risco só atualiza com evidência" "$B4_RISK"

grep -q "Pronto para 15C1" "$B4_HANDOFF"
grep -q "harness diferencial" "$B4_HANDOFF"

echo "phase15b4 semantic-emission audit: PASS"
