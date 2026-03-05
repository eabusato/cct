# FASE 13 Residual Risks

## Purpose

This document captures non-blocking residual risks after FASE 13 closure (13D.4),
including impact, current mitigation, and target phase for follow-up.

Status: tracked and accepted at closure gate.

## Risk Register

### RISK-13R-001 — Metadata breadth vs consumer heterogeneity

- Impact:
  - older downstream consumers can mis-handle newly added optional metadata blocks.
- Current mitigation:
  - strict/tolerant profiles and explicit schema validation (`sigilo validate`).
  - compatibility matrix maintained in release docs.
- Residual exposure:
  - medium in mixed-version ecosystems.
- Target resolution phase:
  - FASE 14 hardening (consumer guidance + tooling UX refinements).

### RISK-13R-002 — Operational misuse of CI profile strictness

- Impact:
  - teams may run `advisory` when `gated` or `release` should be enforced.
- Current mitigation:
  - explicit CI profile contract and examples in release notes.
  - baseline check returns severity-aligned exit codes.
- Residual exposure:
  - medium, process-dependent.
- Target resolution phase:
  - FASE 14 hardening (policy templates + stronger defaults guidance).

### RISK-13R-003 — Documentation drift between release artifacts and canonical docs

- Impact:
  - users can rely on stale commands/contracts if references diverge.
- Current mitigation:
  - cross-reference tests in consolidated suite.
  - final snapshot and closure gate with explicit inventory.
- Residual exposure:
  - low to medium over time.
- Target resolution phase:
  - FASE 14 hardening (automated docs consistency checks expansion).

### RISK-13R-004 — Performance overhead in high-volume sigilo workflows

- Impact:
  - larger projects can incur higher CI/runtime cost for repeated sigilo checks.
- Current mitigation:
  - incremental baseline workflows and scoped command usage.
  - deterministic behavior allows caching strategies.
- Residual exposure:
  - medium for large repositories.
- Target resolution phase:
  - FASE 14 hardening (profiling and optimization pass).

### RISK-13R-005 — Human review quality for `review-required` changes

- Impact:
  - semantic-impact changes classified as review-required may be accepted without deep analysis.
- Current mitigation:
  - severity classification and strict baseline options.
  - release docs include decision thresholds and policy examples.
- Residual exposure:
  - medium, human-process dependent.
- Target resolution phase:
  - FASE 14 hardening (playbooks/checklists for reviewer decisions).

## Accepted Deferred Backlog (14+)

- stronger policy scaffolding for CI profile adoption
- expanded documentation-consistency automation
- targeted performance optimization for sigilo-heavy pipelines
- reviewer playbooks for severity-driven acceptance criteria

## Closure Note

No residual risk above was classified as a FASE 13 closure blocker.
All residual items are explicitly deferred and tracked for 14+.
