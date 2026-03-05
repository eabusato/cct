# Sigilo Schema Governance (FASE 13C.1)

This document defines the schema/versioning policy for `.sigil` artifacts during FASE 13.

## Scope

- Applies to `.sigil` metadata consumed by `cct sigilo inspect|diff|check` and project sigilo gates.
- Defines compatibility rules for producers and consumers.
- Establishes deprecation lifecycle and field status classes.

## Canonical schema version

Current schema identifier:

- `format = cct.sigil.v1`

Compatibility rule:

- FASE 13 keeps `cct.sigil.v1` as the only accepted stable format.
- Incompatible format changes are not allowed in FASE 13.
- New metadata must be additive and compatible with tolerant readers.

## Canonical top-level fields

| Field | Type | Cardinality | Status | Introduced | Notes |
|---|---|---|---|---|---|
| `format` | string | single | stable | 6A | Must be `cct.sigil.v1` |
| `sigilo_scope` | enum(`local`,`system`) | single | stable | 9D | Scope contract for hash requirement |
| `visual_engine` | string | single | stable | 6A | Visual metadata |
| `semantic_hash` | 16-hex string | single | stable | 5B | Required for local scope |
| `system_hash` | 16-hex string | single | stable | 9D | Required for system scope |
| `sigilo_style` | string | single | deprecated | 13C.1 | Legacy alias; use `visual_engine` |

## Section model

- Section header syntax: `[section_name]`.
- Keys are `key = value` pairs.
- Cardinality:
  - Top-level: keys are expected single-value.
  - Sections: repeatable keys depend on producer contract; parser remains tolerant by default.

Analytical section extensions introduced in FASE 13C.2:

| Section | Status | Determinism | Compatibility |
|---|---|---|---|
| `analysis_summary` | experimental | stable serialization order | additive; optional |
| `diff_fingerprint_context` | experimental | no volatile timestamp fields | additive; optional |
| `module_structural_summary` | experimental | derived from deterministic metrics | additive; optional |
| `compatibility_hints` | experimental | static policy hints | additive; optional |

## Field status classes

- `stable`: public-compatible and expected by tooling.
- `experimental`: additive, optional, must not break tolerant consumers.
- `deprecated`: accepted with warning; replacement field must be documented.
- `internal`: reserved for internal producer/consumer use; not part of public compatibility promise.

## Evolution policy (FASE 13)

1. Additive-only by default.
2. No incompatible format bump in this phase.
3. Unknown optional fields must remain warning-only in tolerant mode.
4. Strict mode may enforce semantic/schema contracts, but no silent schema break is allowed.
5. Removals require deprecation lifecycle and explicit migration in a future phase.

## Deprecation policy

For a deprecated field:

1. Keep parser acceptance in tolerant and strict modes.
2. Emit warning-level diagnostic (`deprecated_field`).
3. Keep documented replacement field.
4. Remove only in a future phase with migration guide and release note.

Current deprecated field:

- `sigilo_style` -> replacement: `visual_engine`

## Compatibility matrix

| Producer | Consumer mode | Expected behavior |
|---|---|---|
| v1 with stable fields | tolerant | success |
| v1 with additive unknown field | tolerant | success + `unknown_field` warning |
| v1 with additive unknown field | strict | success + warning (non-fatal) |
| v1 with deprecated field | tolerant | success + `deprecated_field` warning |
| v1 with deprecated field | strict | success + warning (non-fatal) |
| incompatible format (`cct.sigil.v2`) | tolerant/strict | error (`schema_mismatch`) |

## Producer/consumer cross-version guidance

- New producer -> old tolerant consumer:
  - allowed when additions are optional and format remains `cct.sigil.v1`.
- Old producer -> new consumer:
  - allowed; missing additive fields should not break parsing.
- Deprecated fields:
  - accepted during coexistence period.

## Operational recommendations

- Use strict mode for contract enforcement near release gates.
- Use tolerant mode for exploratory analysis and forward-compatible reads.
- Prefer `visual_engine`; avoid new usage of `sigilo_style`.
