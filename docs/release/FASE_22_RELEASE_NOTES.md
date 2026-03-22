# FASE 22 - Bootstrap Parser Core - Release Notes

**Date:** 2026-03-21
**Version:** 0.22.0
**Status:** Completed

## Executive Summary

FASE 22 delivered the bootstrap parser baseline. The phase introduced the AST backbone, parser state/helpers, expression parsing, basic statement parsing, declaration parsing for the core subset, recovery behavior, and a stable AST dump format that could be compared against the host parser.

## Scope Closed

Delivered subphases:
- AST kind model and owned AST node infrastructure
- parser state, token navigation, and parser helper routines
- expression parsing for literals, unary/binary expressions, calls, field/index access, and assignment shapes
- statement parsing for blocks, declarations, conditionals, loops, returns, and expression statements
- declaration parsing for functions, `SIGILLUM`, `ORDO`, and imports in the bootstrap baseline
- panic-mode recovery and AST dump validation against the host parser

## Key Deliverables

Implementation delivered in:
- `src/bootstrap/parser/ast_kind.cct`
- `src/bootstrap/parser/ast.cct`
- `src/bootstrap/parser/parser_state.cct`
- `src/bootstrap/parser/parser_helpers.cct`
- `src/bootstrap/parser/parse_expr.cct`
- `src/bootstrap/parser/parse_stmt.cct`
- `src/bootstrap/parser/parse_decl.cct`
- `src/bootstrap/parser/parser.cct`

Validation delivered through:
- parser AST fixtures in `tests/integration/`
- bootstrap parser dump runners in `tests/run_tests.sh`

## Major Technical Decisions

- the bootstrap parser uses an explicit AST contract rather than reusing host AST assumptions informally
- AST ownership is explicit and destructor-driven, allowing later semantic/codegen phases to consume the tree safely
- parser recovery was treated as part of the phase scope, not as deferred cleanup work
- compatibility is checked structurally via AST dump normalization, not by source-level pretty-printing

## Validation Summary

The phase closed with dedicated tests for:
- AST construction and ownership
- precedence and expression-tree formation
- basic statements and declarations
- parser recovery
- AST dump compatibility with the host compiler

This phase established the second bootstrap rule: parser completion requires structural comparability with the host parser, not just successful parse acceptance.

## User-Facing Impact

No new end-user syntax was introduced. The user-visible effect is indirect: the bootstrap track gained a real parser/AST layer, making a self-hosted front-end feasible.

## Residual Limits at Phase Close

At the end of FASE 22, the bootstrap parser handled the core executable subset but not the full advanced syntax surface. Advanced control flow, richer generic syntax, and remaining modular/parser-surface details were still left for FASE 23.

## Transition to FASE 23

FASE 23 could rely on:
- stable AST node ownership and dump format
- parser state/helpers already validated on the core subset
- declaration and statement parsing infrastructure ready for advanced syntax expansion
