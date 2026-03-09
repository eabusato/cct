# CCT ORDO Payload Proposal V0

## Motivation

The current `cct/variant`-based model solved the practical blocking issue for sum-type modeling,
but it still requires manual discipline:
- repeated use of numeric `tag` values;
- opaque payload with consumer-side casts/conventions;
- weaker type safety for constructors and accessors.

Objective of this proposal:
evolve the language toward native payload-capable `ORDO`, while preserving compatibility and enabling incremental rollout.

## Candidate Syntax

```cct
ORDO Expr
  IDENT(VERBUM nome),
  LITERAL(REX valor),
  BINARY(SPECULUM NIHIL lhs, VERBUM op, SPECULUM NIHIL rhs)
FIN ORDO
```

Candidate usage:

```cct
EVOCA Expr a AD IDENT("x")
EVOCA Expr b AD LITERAL(42)
EVOCA Expr c AD BINARY(SPECULUM NIHIL a, "+", SPECULUM NIHIL b)
```

## Proposed Semantics

- Each `ORDO` item may have zero or more payload fields.
- Constructors become native to the item (`IDENT("x")`, `LITERAL(42)`, etc.).
- Value reads must check the active variant.
- Items without payload continue to represent tag-only variants.
- Items with payload require exact arity and types.

## Parser Impact

- Extend `ORDO` grammar to accept an optional field list per item.
- Create specific AST nodes for:
  - payload-free `ORDO` item;
  - typed-payload `ORDO` item.
- Validate delimiters and syntactic arity in the parser to reduce late errors.

## Semantic Impact

- Resolve per-item field types and verify references.
- Validate native constructor calls:
  - item exists;
  - correct arity;
  - compatible types.
- Emit canonical diagnostics for item/payload mismatch.

## Codegen Impact

Proposed lowering:
- `struct` with a `tag` field;
- payload storage through `union` or an equivalent structured block;
- generated helpers for safe per-item construction and access.

Codegen requirements:
- preserve deterministic layout;
- preserve compatibility with the current C backend;
- preserve predictable error messages for invalid access.

## Backward Compatibility

- Payload-free `ORDO` remains valid with no source change.
- Existing `cct/variant` code continues to work during coexistence.
- No immediate breaking changes to older APIs.

## Migration Plan

### Phase 1: coexistence
- introduce new payload `ORDO` syntax;
- keep `cct/variant` as a supported path.

### Phase 2: wrappers
- provide compatibility wrappers `variant <-> payload ORDO`;
- support gradual consumer migration.

### Phase 3: progressive deprecation
- mark manual tag/payload APIs as legacy;
- remove only after a documented migration window.

## Risks and Mitigations

1. Parser complexity.
- Mitigation: phase the change through clear AST nodes and targeted errors.

2. Diagnostic-quality regression.
- Mitigation: preserve canonical error codes and envelopes.

3. Binary growth due to payload support.
- Mitigation: compact lowering strategy and impact tests.

## Tradeoffs

| Aspect | Benefit | Cost |
|---|---|---|
| Ergonomics | Less manual boilerplate | More parser/semantic complexity |
| Type safety | Less opaque payload casting | More validation rules |
| Evolution | Native path for AST/IR | Additional backend implementation |
| Compatibility | Coexistence with `cct/variant` | Longer migration window |

## Full Example (Declaration and Use)

```cct
INCIPIT grimoire "expr_demo"

ORDO Expr
  IDENT(VERBUM nome),
  LITERAL(REX valor),
  ADD(SPECULUM NIHIL lhs, SPECULUM NIHIL rhs)
FIN ORDO

RITUALE main() REDDE REX
  EVOCA Expr a AD IDENT("x")
  EVOCA Expr b AD LITERAL(1)
  EVOCA Expr c AD ADD(SPECULUM NIHIL a, SPECULUM NIHIL b)
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

## Conclusion

This proposal defines a concrete technical direction to move from the manual `Variant` model
to native payload-capable `ORDO`, with incremental rollout and no abrupt breaking change.
