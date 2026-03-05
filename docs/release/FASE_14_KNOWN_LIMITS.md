# FASE 14 Known Limits

- `LIMIT-14-001`
  - Scope: stress/soak rounds are finite and intentionally short in default CI profile.
  - Impact: rare long-tail intermittent failures may require manual extended soak reruns.
  - Workaround: run `tests/run_phase14c2_stress_soak.sh 25` before high-risk release.
  - Planned resolution phase: FASE 15 hardening automation.

- `LIMIT-14-002`
  - Scope: performance budgets are conservative and not platform-calibrated by percentile.
  - Impact: low-signal regressions may stay under threshold.
  - Workaround: inspect baseline deltas in `/tmp/cct_phase14c3_perf/baseline.txt`.
  - Planned resolution phase: FASE 15 metrics refinement.

- `LIMIT-14-003`
  - Scope: release template pack is generic and requires manual final curation.
  - Impact: incomplete manual fill can weaken release communication quality.
  - Workaround: enforce closure checklist before GO decision.
  - Planned resolution phase: maintain checklist discipline.
