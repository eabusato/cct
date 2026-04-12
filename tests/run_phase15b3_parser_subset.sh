#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

B3_PLAN="md_out/docs/release/FASE_15B3_BOOTSTRAP_PARSER_PLAN.md"
B3_MATRIX="md_out/docs/release/FASE_15B3_PARSER_SUPPORT_MATRIX.md"
B3_PREC="md_out/docs/release/FASE_15B3_PRECEDENCE_CONTRACT.md"
B3_DIFF="md_out/docs/release/FASE_15B3_DIFFERENTIAL_INTEGRATION.md"
B3_TEST="md_out/docs/release/FASE_15B3_TEST_MATRIX.md"
B3_ACCEPT="md_out/docs/release/FASE_15B3_ACCEPTANCE_ROLLBACK.md"
B3_RISK="md_out/docs/release/FASE_15B3_RISK_OWNERSHIP_MAP.md"
B3_HANDOFF="md_out/docs/release/FASE_15B3_HANDOFF_15B4.md"

for f in "$B3_PLAN" "$B3_MATRIX" "$B3_PREC" "$B3_DIFF" "$B3_TEST" "$B3_ACCEPT" "$B3_RISK" "$B3_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "parser subset em CCT" "$B3_PLAN"
grep -q "fallback canônico obrigatório" "$B3_PLAN"
grep -q "opt-in" "$B3_PLAN"

grep -q "declaração de ritual básico" "$B3_MATRIX"
grep -qi "regras de precedência" "$B3_MATRIX"
grep -q "fora de escopo" "$B3_MATRIX"

grep -q "PC-15B3-001" "$B3_PREC"
grep -q "PC-15B3-004" "$B3_PREC"
grep -q 'Divergência de precedência' "$B3_PREC"

grep -q "classificar diferença" "$B3_DIFF"
grep -q 'divergência `BLOCKER` aciona fallback imediato' "$B3_DIFF"

grep -q "10 validações objetivas" "$B3_TEST"
grep -q "continuidade 15B2 -> 15B3" "$B3_TEST"

grep -q 'divergência `BLOCKER` sem owner' "$B3_ACCEPT"
grep -q '`pass`, `watch` ou `block`' "$B3_ACCEPT"

grep -q "RISK-15B3-001" "$B3_RISK"
grep -q "Parser Subset Lead" "$B3_RISK"
grep -q "baseline de risco só atualiza com evidência" "$B3_RISK"

grep -q "Pronto para 15B4" "$B3_HANDOFF"
grep -q "semântica subset" "$B3_HANDOFF"

echo "phase15b3 parser-subset audit: PASS"
