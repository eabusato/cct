#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

C1_PLAN="md_out/docs/release/FASE_15C1_BOOTSTRAP_DIFF_HARNESS_PLAN.md"
C1_RUNNER="md_out/docs/release/FASE_15C1_DIFF_RUNNER_CONTRACT.md"
C1_REPORT="md_out/docs/release/FASE_15C1_DIFF_REPORT_FORMAT.md"
C1_SEV="md_out/docs/release/FASE_15C1_DIFF_SEVERITY_POLICY.md"
C1_CI="md_out/docs/release/FASE_15C1_LOCAL_CI_INTEGRATION.md"
C1_TEST="md_out/docs/release/FASE_15C1_TEST_MATRIX.md"
C1_ACCEPT="md_out/docs/release/FASE_15C1_ACCEPTANCE_ROLLBACK.md"
C1_RISK="md_out/docs/release/FASE_15C1_RISK_OWNERSHIP_MAP.md"
C1_HANDOFF="md_out/docs/release/FASE_15C1_HANDOFF_15C2.md"

for f in "$C1_PLAN" "$C1_RUNNER" "$C1_REPORT" "$C1_SEV" "$C1_CI" "$C1_TEST" "$C1_ACCEPT" "$C1_RISK" "$C1_HANDOFF"; do
  [ -f "$f" ]
done

grep -q "harness diferencial unificado" "$C1_PLAN"
grep -q "fallback explícito" "$C1_PLAN"
grep -q "opt-in" "$C1_PLAN"

grep -q "DR-15C1-001" "$C1_RUNNER"
grep -q "DR-15C1-005" "$C1_RUNNER"
grep -q "pass\`, \`watch\` ou \`block\`" "$C1_RUNNER"

grep -q "RF-15C1-001" "$C1_REPORT"
grep -q "RF-15C1-004" "$C1_REPORT"
grep -q "severity" "$C1_REPORT"

grep -q "INFO" "$C1_SEV"
grep -q "REVIEW" "$C1_SEV"
grep -q "BLOCKER" "$C1_SEV"

grep -q "CI-15C1-001" "$C1_CI"
grep -q "CI-15C1-003" "$C1_CI"
grep -q "bloqueio automático" "$C1_CI"

grep -q "10 validações objetivas" "$C1_TEST"
grep -q "continuidade 15B4 -> 15C1" "$C1_TEST"

grep -q 'divergência `BLOCKER` sem owner' "$C1_ACCEPT"
grep -q '`pass`: sem bloqueios' "$C1_ACCEPT"

grep -q "RISK-15C1-001" "$C1_RISK"
grep -q "Differential Harness Lead" "$C1_RISK"
grep -q "baseline de risco só atualiza com evidência" "$C1_RISK"

grep -q "Pronto para 15C2" "$C1_HANDOFF"
grep -q "matriz de convergência funcional" "$C1_HANDOFF"

echo "phase15c1 diff-harness audit: PASS"
