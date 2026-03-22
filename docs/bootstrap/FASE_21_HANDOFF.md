# FASE 21 - Handoff

**Date:** 2026-03-21
**Status:** Completed
**Next Phase:** 22

## What Is Stable

- bootstrap support-library pieces needed by compiler code (`char`, `verbum`, `diagnostic`)
- token kinds and token ownership contract
- keyword lookup and lexer state management
- tokenization for identifiers, literals, strings, comments, recovery, and EOF behavior
- host-vs-bootstrap token stream comparison flow

## Contracts the Next Phase Can Rely On

- `Token` owns `lexeme`
- invalid/error tokens duplicate borrowed/literal messages before ownership transfer
- token kind parity is exact, not approximate
- lexer position tracking is stable enough for parser diagnostics and later semantic work

## Open Boundaries Carried Forward

- no AST yet
- no parser recovery beyond lexical recovery
- no semantic or backend behavior

## Expected Use by FASE 22

FASE 22 should build directly on the bootstrap token stream and must not invent parser-side token conventions that diverge from the host lexer contract.
