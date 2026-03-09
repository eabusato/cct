# CCT — Handoff FASE 17

## 1. Final Phase Status

- Status: PASS
- Date: 2026-03-06
- Final gate: `make test` green on the base branch (927 passed / 0 failed)
- Closed scope: 17A1 through 17D4

## 2. Deliveries by Subphase

- 17A1: `VERBUM <-> MILES` bridge (`char_at`, `from_char`) plus `cct/char`.
- 17A2: `cct/args` (`argc`, `arg`) with bounds contract.
- 17A3: `cct/verbum_scan` cursor API (`init`, `pos`, `eof`, `peek`, `next`, `free`).
- 17A4: lexer-toolkit consolidation and integrated regression.
- 17B1: `cct/verbum_builder` (append, append_char, len, clear, free).
- 17B2: `cct/code_writer` with deterministic indentation.
- 17B3: builder/writer integration with `cct/fmt` for int/bool/real.
- 17B4: text-load determinism closure and regression contract.
- 17C1: host-functional `cct/variant` (tag + opaque payload).
- 17C2: `cct/variant_helpers` for predicate/tag/expect/payload_if.
- 17C3: `cct/ast_node` for host-side AST nodes.
- 17C4: documentary proposal for payload ORDO (`CCT_ORDO_PAYLOAD_PROPOSAL_V0.md`).
- 17D1: `cct/env` (`getenv`, `has_env`, `cwd`).
- 17D2: `cct/time` (`now_ms`, `now_ns`, `sleep_ms`).
- 17D3: `cct/bytes` (`bytes_new`, `bytes_len`, `bytes_get`, `bytes_set`, `bytes_free`).
- 17D4: `docs/spec.md` consolidation, handoff, and final documentation gate.

## 3. Technical Evidence

- FASE 17A test blocks: 957-971.
- FASE 17B test blocks: 972-987.
- FASE 17C test blocks: 988-999.
- FASE 17D test blocks: 1000-1012.
- Final full-regression result: 927 passed / 0 failed.

## 4. Residual Risks and Pending Items

- Payload ORDO remains a language proposal only (not implemented in parser/semantic/codegen).
- 17D host-only modules still depend on host profile and must remain blocked in freestanding.
- Roadmap pending item: decide the migration path from pragmatic `variant` to native sum/tag support in a future phase.

## 5. Confirmed Out-of-Scope Items

- Full compiler self-hosting in a single phase.
- Native payload ORDO syntax implementation.
- New backends beyond the current C-hosted flow.

## 6. Entry Checklist for FASE 18

- [x] `make test` with 0 failed on the base branch.
- [x] 17A/17B/17C contract docs updated.
- [x] open payload-ORDO decisions recorded.
- [ ] Define executable FASE 18 backlog with acceptance criteria per subphase.
- [ ] Freeze the initial FASE 18 baseline after prompt creation.
