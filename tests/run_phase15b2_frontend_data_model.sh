#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

B2_PLAN="md_out/docs/release/FASE_15B2_BOOTSTRAP_FRONTEND_PLAN.md"
B2_SCHEMA="md_out/docs/release/FASE_15B2_FRONTEND_DATA_SCHEMA.md"
B2_MAP="md_out/docs/release/FASE_15B2_CANONICAL_BOOTSTRAP_MAPPING.md"
B2_LIMITS="md_out/docs/release/FASE_15B2_STRUCTURAL_VALIDATION_LIMITS.md"
B2_MATRIX="md_out/docs/release/FASE_15B2_TEST_MATRIX.md"
B2_ACCEPT="md_out/docs/release/FASE_15B2_ACCEPTANCE_ROLLBACK.md"
B2_RISK="md_out/docs/release/FASE_15B2_RISK_OWNERSHIP_MAP.md"
B2_HANDOFF="md_out/docs/release/FASE_15B2_HANDOFF_15B3.md"

for f in "$B2_PLAN" "$B2_SCHEMA" "$B2_MAP" "$B2_LIMITS" "$B2_MATRIX" "$B2_ACCEPT" "$B2_RISK" "$B2_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "schema bootstrap para tokens e AST subset" "$B2_PLAN"
grep -q "trilha bootstrap é estritamente opt-in" "$B2_PLAN"
grep -q "fallback obrigatório" "$B2_PLAN"

grep -q "TokenNode" "$B2_SCHEMA"
grep -q "AstNode" "$B2_SCHEMA"
grep -q "RITUALE_DECL" "$B2_SCHEMA"

grep -q "MAP-15B2-001" "$B2_MAP"
grep -q "MAP-15B2-004" "$B2_MAP"
grep -q 'Divergência `BLOCKER`' "$B2_MAP"

grep -q "Validações Estruturais" "$B2_LIMITS"
grep -q "parser geral completo está fora de escopo" "$B2_LIMITS"

grep -q "10 validações objetivas" "$B2_MATRIX"
grep -q "coerência entre 15B1 e 15B2" "$B2_MATRIX"

grep -q 'divergência `BLOCKER` sem owner' "$B2_ACCEPT"
grep -q '`pass`, `watch` ou `block`' "$B2_ACCEPT"

grep -q "RISK-15B2-001" "$B2_RISK"
grep -q "Frontend Model Lead" "$B2_RISK"
grep -q "baseline de risco só atualiza com evidência" "$B2_RISK"

grep -q "Pronto para 15B3" "$B2_HANDOFF"
grep -q "parser subset em CCT" "$B2_HANDOFF"

echo "phase15b2 frontend data-model audit: PASS"
