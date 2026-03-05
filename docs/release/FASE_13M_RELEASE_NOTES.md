# FASE 13M Release Notes

Status: closed in the final 13M release gate.

## Scope Delivered

- `**` exponentiation operator.
- `//` floor integer division operator.
- `%%` euclidean modulo operator.
- preserved compatibility for legacy `%`.

## Behavioral Contract

- `**` is right-associative.
- `//` accepts integer operands and follows floor semantics.
- `%%` accepts integer operands and follows euclidean semantics.
- division/modulo by zero keeps explicit runtime diagnostics.

## Compatibility

- no operator removals.
- no behavioral change for legacy `%`.
- historical parser/semantic/codegen flows remain green in full regression suite.

## Documentation and Example

- spec updated with new operators and precedence table.
- README updated with practical usage and command smoke.
- executable example: `examples/math_common_ops_13m.cct`.

## Closure Evidence

- global test suite includes dedicated 13M scope/implementation/quality/release blocks.

## Deferred and Out-of-Scope

- deferred: additional low-frequency operators and extended mathematical notation remain outside this addendum.
- out-of-scope: new DSL constructs unrelated to common arithmetic workflows.
