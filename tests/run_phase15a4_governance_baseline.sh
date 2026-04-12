#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

A4_RISK="md_out/docs/release/FASE_15A4_BOOTSTRAP_RISK_BASELINE.md"
A4_OWN="md_out/docs/release/FASE_15A4_OWNERSHIP_SLA_MATRIX.md"
A4_AUDIT="md_out/docs/release/FASE_15A4_AUDIT_CADENCE.md"
A4_ESC="md_out/docs/release/FASE_15A4_ESCALATION_POLICY.md"

for f in "$A4_RISK" "$A4_OWN" "$A4_AUDIT" "$A4_ESC"; do
  [ -f "$f" ]
done

grep -q "RISK-15A4-001" "$A4_RISK"
grep -q "RISK-15A4-002" "$A4_RISK"
grep -q "Owner:" "$A4_RISK"

grep -q "SLA Triagem" "$A4_OWN"
grep -q "Compiler Core Lead" "$A4_OWN"
grep -q "Bootstrap Governance Lead" "$A4_OWN"

grep -q "auditoria local por subsubfase" "$A4_AUDIT"
grep -q 'decisão: `pass`, `watch`, `block`' "$A4_AUDIT"

grep -q "Nível 1" "$A4_ESC"
grep -q "Nível 2" "$A4_ESC"
grep -q "Nível 3" "$A4_ESC"
grep -q "baseline só atualiza com evidência" "$A4_ESC"

echo "phase15a4 governance baseline audit: PASS"
