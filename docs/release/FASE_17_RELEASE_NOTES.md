# FASE 17 Release Notes

## Summary

FASE 17 expands the canonical standard library for host-first bootstrap work.
The phase closes at **17D.4** with lexer/CLI foundations, efficient text-building primitives, pragmatic variant/AST support, host utility libraries, and closure documentation.

## Highlights

### 17A — Lexer and CLI Foundation

- `cct/verbum`: `char_at`, `from_char`.
- `cct/char`: `is_digit`, `is_alpha`, `is_whitespace`.
- `cct/args`: `argc`, `arg`.
- `cct/verbum_scan`: cursor API (`init`, `pos`, `eof`, `peek`, `next`, `free`).

### 17B — Efficient Text Construction

- `cct/verbum_builder` core API stabilized.
- `cct/code_writer` with deterministic indentation/newline semantics.
- `cct/fmt` integration for append/write of int, bool, and real values.

### 17C — Pragmatic Sum-Type Base

- `cct/variant` and `cct/variant_helpers` delivered.
- `cct/ast_node` host-side AST kit delivered.
- ORDO payload native syntax remains proposal-only (`CCT_ORDO_PAYLOAD_PROPOSAL_V0.md`).

### 17D — Host Utility Libraries and Closure

- `cct/env`: `getenv`, `has_env`, `cwd`.
- `cct/time`: `now_ms`, `now_ns`, `sleep_ms`.
- `cct/bytes`: mutable byte buffer (`bytes_new`, `bytes_len`, `bytes_get`, `bytes_set`, `bytes_free`).
- Spec consolidation + phase handoff publication completed.

## Compatibility Notes

- Host profile remains primary target for the new 17D utility modules.
- Freestanding restrictions continue to block host-only modules (`env_*`, `time_*`, `bytes_*`, `args_*`, and related host surfaces).
- No backend strategy change: C-hosted compilation flow remains the baseline.

## Verification Status

- Regression blocks added for 17A-17D (`tests/run_tests.sh`, tests 957-1012).
- Final phase gate: **930 passed / 0 failed** on full `make test`.

## References

- `docs/bootstrap/FASE_17_HANDOFF.md`
- `md_out/FASE_17_CCT.md`
- `docs/bootstrap/CCT_LEXER_TOOLKIT_V0.md`
- `docs/bootstrap/CCT_TEXT_BUILDING_CONTRACT_V0.md`
- `docs/bootstrap/CCT_ORDO_PAYLOAD_PROPOSAL_V0.md`
