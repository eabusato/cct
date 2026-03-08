# FASE 13M Known Limits

Status: published at 13M closure.

## Explicit Non-Scope Items

- no optional chaining (`?.`) in this addendum.
- no unwrap operator (`!!`) in this addendum.
- no range system expansion in this addendum.
- no caret alias (`^`) for power in this addendum.

## Operator Constraints

- `//` and `%%` require integer operands.
- `//` and `%%` by zero are runtime failures by contract.
- `**` currently uses numeric path and does not add a new bigint model.

## Rationale

The addendum intentionally prioritizes high-frequency operations and avoids widening scope into low-demand syntax families, preserving regression safety and release stability.
