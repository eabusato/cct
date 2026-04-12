#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

A1_SURF="md_out/docs/release/FASE_15A1_SUPERFICIES_CRITICAS.md"
A1_DEPS="md_out/docs/release/FASE_15A1_DEPENDENCIAS_BOOTSTRAP.md"
A1_RISK="md_out/docs/release/FASE_15A1_RISK_TAXONOMY.md"
A1_ORDER="md_out/docs/release/FASE_15A1_MIGRATION_ORDER.md"

for f in "$A1_SURF" "$A1_DEPS" "$A1_RISK" "$A1_ORDER"; do
  [ -f "$f" ]
done

grep -q "SURF-15A1-004" "$A1_SURF"
grep -q "codegen_runtime_bridge" "$A1_SURF"
grep -q "W0" "$A1_SURF"

grep -q "não acoplado" "$A1_DEPS"
grep -q "fora do escopo" "$A1_DEPS"

grep -q "RISK-15A1-001" "$A1_RISK"
grep -q "Owner:" "$A1_RISK"
grep -q "SLA inicial:" "$A1_RISK"

grep -q "Onda W0" "$A1_ORDER"
grep -q "Onda W1" "$A1_ORDER"
grep -q "Onda W2" "$A1_ORDER"
grep -q "Handoff obrigatório para 15A2" "$A1_ORDER"

echo "phase15a1 inventory audit: PASS"
