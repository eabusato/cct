#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

D3_PLAN="md_out/docs/release/FASE_15D3_FINAL_SNAPSHOT_PLAN.md"
D3_EVID="md_out/docs/release/FASE_15D3_EVIDENCE_PACKAGE_INDEX.md"
D3_MAT="md_out/docs/release/FASE_15D3_CONSOLIDATED_MATRICES.md"
D3_RL="md_out/docs/release/FASE_15D3_RESIDUAL_RISK_LIMITS.md"
D3_NOTES="md_out/docs/release/FASE_15D3_TECHNICAL_RELEASE_NOTES.md"
D3_TEST="md_out/docs/release/FASE_15D3_TEST_MATRIX.md"
D3_ACCEPT="md_out/docs/release/FASE_15D3_ACCEPTANCE_ROLLBACK.md"
D3_RISK="md_out/docs/release/FASE_15D3_RISK_OWNERSHIP_MAP.md"
D3_HANDOFF="md_out/docs/release/FASE_15D3_HANDOFF_15D4.md"

for f in "$D3_PLAN" "$D3_EVID" "$D3_MAT" "$D3_RL" "$D3_NOTES" "$D3_TEST" "$D3_ACCEPT" "$D3_RISK" "$D3_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "snapshot final" "$D3_PLAN"
grep -q "pacote de evidências" "$D3_PLAN"
grep -q "handoff" "$D3_PLAN"

grep -q "EV-15D3-001" "$D3_EVID"
grep -q "EV-15D3-004" "$D3_EVID"
grep -q "rastreável" "$D3_EVID"

grep -q "MAT-15D3-001" "$D3_MAT"
grep -q "MAT-15D3-004" "$D3_MAT"
grep -q "pass/watch/block" "$D3_MAT"

grep -q "RL-15D3-001" "$D3_RL"
grep -q "RL-15D3-004" "$D3_RL"
grep -q "risco residual" "$D3_RL"

grep -q "trilha bootstrap dual" "$D3_NOTES"
grep -q "15D3" "$D3_NOTES"
grep -q "gate final 15D4" "$D3_NOTES"

grep -q "10 validações objetivas" "$D3_TEST"
grep -q "coerência 15D2 -> 15D3" "$D3_TEST"

grep -q '`pass`: pacote consolidado completo e coerente' "$D3_ACCEPT"
grep -q '`block`: inconsistência crítica' "$D3_ACCEPT"

grep -q "RISK-15D3-001" "$D3_RISK"
grep -q "Release Consolidation Lead" "$D3_RISK"
grep -q "baseline de risco só atualiza com evidência" "$D3_RISK"

grep -q "Pronto para 15D4" "$D3_HANDOFF"
grep -q "closure gate oficial" "$D3_HANDOFF"

echo "phase15d3 artifact-consolidation audit: PASS"
