# FASE 21 - Bootstrap Foundations and Lexer - Release Notes

**Date:** 2026-03-21
**Version:** 0.21.0
**Status:** Completed

## Executive Summary

FASE 21 opened the bootstrap era by moving the first compiler subsystem from the host implementation into CCT. The phase delivered the support-library foundations required by the bootstrap track, a complete token model and keyword table in CCT, and a validated bootstrap lexer able to match the host lexer across focused fixtures, edge cases, and real-world files.

## Scope Closed

Delivered subphases:
- library foundations for bootstrap (`char`, `verbum`, `diagnostic` support required by the compiler)
- token-kind model and token ownership contract in CCT
- keyword lookup and lexer state infrastructure
- character navigation, whitespace/comment skipping, literal scanning, and token creation helpers
- host-vs-bootstrap token stream gates on synthetic and real project inputs

Explicitly not part of this phase:
- parser or AST porting
- semantic analysis
- code generation
- self-host stage pipeline

## Key Deliverables

Implementation delivered in:
- `lib/cct/char.cct`
- `lib/cct/verbum.cct`
- `lib/cct/diagnostic.cct`
- `src/bootstrap/lexer/token_type.cct`
- `src/bootstrap/lexer/token.cct`
- `src/bootstrap/lexer/keywords.cct`
- `src/bootstrap/lexer/lexer_state.cct`
- `src/bootstrap/lexer/lexer_helpers.cct`
- `src/bootstrap/lexer/lexer.cct`
- `src/bootstrap/main_lexer.cct`

Validation assets delivered in:
- `tests/integration/lexer_*.cct`
- `tests/run_tests.sh` bootstrap lexer gates

## Major Technical Decisions

- `Token` owns its `lexeme`; ownership is transferred into the token and released by `token_free`.
- `lexer_error_token` duplicates incoming error text before constructing an invalid token, preventing accidental release of string literals or borrowed pointers.
- token kind parity is validated by exact `1:1` comparison against the host enum rather than by approximate token counts.
- bootstrap lexer validation compares behavior against host token output, not against hand-maintained textual expectations only.

## Validation Summary

The phase closed with:
- dedicated lexer integration fixtures for identifiers, numbers, strings, comments, invalid input, and recovery
- differential token-stream checks against `./cct --tokens`
- real-file tokenization checks across canonical library and integration fixtures
- phase-local gates integrated into the repository runner

This phase established the first practical rule for the bootstrap era: a bootstrap component is only considered closed when it can be compared against the host implementation on realistic inputs.

## User-Facing Impact

No end-user language surface was introduced. The impact of FASE 21 is architectural:
- the compiler is no longer host-only
- bootstrap support libraries became part of the long-term language/platform baseline
- the repository gained bootstrap-aware testing strategy and compatibility gates

## Residual Limits at Phase Close

At the end of FASE 21, the bootstrap compiler still did not parse, type-check, or generate code. The bootstrap stack stopped at tokenization and support infrastructure.

## Transition to FASE 22

FASE 22 could assume all of the following were stable:
- bootstrap token model
- keyword classification
- line/column/file tracking
- deterministic error-token behavior
- reusable support-library pieces for parser and later phases
