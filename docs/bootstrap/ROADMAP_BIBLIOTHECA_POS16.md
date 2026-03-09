# ROADMAP_BIBLIOTHECA_POS16

## Objective

Define the expansion of Bibliotheca Canonica after the closure of FASE 16, with the explicit goal of enabling host-first CCT compiler work written in CCT itself, without compatibility regressions.

This roadmap prioritizes the real blockers in the compiler pipeline:
1. character access over `VERBUM`;
2. command-line argument access;
3. efficient text construction for code generation;
4. robust tagged-node representation for AST work.

## Principles

- Incremental delivery by subphase, with `make test` green at every gate.
- Host-first strategy over the current C backend, without breaking the freestanding track established in FASE 16.
- Avoid scope creep: unlock lexer/CLI/codegen first, then expand the ecosystem.
- New APIs must have explicit ownership and error contracts.

---

## FASE 17A — Critical Lexer and CLI Blockers

### 17A.1 — `VERBUM <-> MILES` bridge

#### Deliveries
- Extend `cct/verbum` with minimum APIs:
  - `char_at(VERBUM s, REX i) -> MILES`
  - `from_char(MILES c) -> VERBUM`
- Add a character-classification layer, preferably in `cct/char.cct`:
  - `is_digit(MILES c) -> VERUM`
  - `is_alpha(MILES c) -> VERUM`
  - `is_whitespace(MILES c) -> VERUM`

#### Contract
- `char_at` uses a strict invalid-index policy with predictable, testable failure behavior.
- `from_char` returns a single-character string with documented ownership.
- Character classification is ASCII-only in the first cut; full Unicode remains out of scope.

#### Acceptance Criteria
- A functional lexer can be written in CCT by iterating character-by-character.
- Edge cases are covered: negative index, index >= len, boundary bytes, classic whitespace (`' '`, `\t`, `\n`, `\r`).

### 17A.2 — `argv` / CLI access

#### Deliveries
- New module `cct/args.cct` (or `cct/cli.cct`) with minimum API:
  - `argc() -> REX`
  - `arg(REX i) -> VERBUM`
  - `has(VERBUM flag) -> VERUM` (optional in this subphase)
- Host runtime bridge exposing `argc/argv` to the generated CCT program.

#### Contract
- `arg(i)` uses a strict invalid-index policy.
- Compatible with the current execution flow (`./cct <program.cct>` and project commands).

#### Acceptance Criteria
- A CCT program can read its input path from `argv` and validate missing arguments.
- Success and failure fixtures exist for host execution.

---

## FASE 17B — Efficient Text Construction

### 17B.1 — Canonical `StringBuilder`

#### Deliveries
- New module `cct/verbum_builder.cct` with baseline API:
  - `builder_init() -> SPECULUM NIHIL`
  - `builder_append(SPECULUM NIHIL b, VERBUM s) -> NIHIL`
  - `builder_append_char(SPECULUM NIHIL b, MILES c) -> NIHIL`
  - `builder_len(SPECULUM NIHIL b) -> REX`
  - `builder_to_verbum(SPECULUM NIHIL b) -> VERBUM`
  - `builder_clear(SPECULUM NIHIL b) -> NIHIL`
  - `builder_free(SPECULUM NIHIL b) -> NIHIL`

#### Contract
- Amortized growth to avoid `O(n^2)` behavior during incremental concatenation.
- Explicit ownership of both the builder and the final string snapshot.

#### Acceptance Criteria
- Textual codegen workloads with thousands of appends do not degrade quadratically.
- Stability tests and integration with `cct/fmt` and `cct/verbum` are in place.

---

## FASE 17C — Tagged-Node Modeling

### 17C.1 — Pragmatic library path (short term)

#### Deliveries
- New module `cct/variant.cct` for opaque tag + payload handling:
  - `variant_make(REX tag, SPECULUM NIHIL payload) -> SIGILLUM Variant`
  - `variant_tag(SIGILLUM Variant v) -> REX`
  - `variant_payload(SIGILLUM Variant v) -> SPECULUM NIHIL`
- Tag conventions driven by `ORDO` in consuming code.

#### Limitation
- This does not replace a native sum type yet; it is a pragmatic bridge for early AST work.

### 17C.2 — Language path (medium term)

#### Deliveries
- Formal language-extension proposal for payload-capable `ORDO`.
- Design document covering semantic impact, codegen impact, and migration path from `variant`.

#### Acceptance Criteria
- Architectural decision recorded (approved or rejected), with proof-of-concept tests if approved.

---

## FASE 17D — Base Library for Compiler Work

After lexer/CLI/textual-codegen are unlocked, expand with productivity modules:

### 17D.1 — `cct/verbum_scan`
- `cursor_init`, `cursor_peek`, `cursor_next`, `cursor_eof`, `cursor_pos`
- Reduces repeated lexer boilerplate in compiler/tooling projects.

### 17D.2 — `cct/bytes`
- Byte read/write, slice, copy, fill.
- Useful for future binary backend and tooling paths.

### 17D.3 — `cct/env`
- `getenv`, `setenv` (if supported), `cwd`, environment-variable checks.
- Focus on local toolchain/automation flows.

### 17D.4 — `cct/time`
- Monotonic clock and simple wall-clock access for benchmarks and tooling.

---

## Recommended Execution Order

1. **17A.1** (`char_at` + classification) — without this, there is no lexer in CCT.
2. **17A.2** (`argv`) — without this, there is no usable compiler CLI in CCT.
3. **17B.1** (`StringBuilder`) — removes the critical textual-codegen bottleneck.
4. **17C.1** (pragmatic `variant`) — enables larger AST work with discipline.
5. **17D.x** (additional libraries) — ecosystem and productivity gains.
6. **17C.2** (native sum type) — language decision once the stack is mature enough.

---

## Success Metrics

- `make test` stays green in every subphase.
- A functional CCT lexer written in CCT exists by the end of 17A.
- A minimal compiler CLI in CCT exists by the end of 17A.2.
- Textual codegen in CCT avoids quadratic degradation by the end of 17B.1.
- A deterministic, documented, host-first library set exists by 17D.

---

## Immediate Out of Scope

- Full Unicode / grapheme-cluster character classification.
- GAS -> NASM conversion.
- Rewriting the entire compiler in a single big-bang self-hosting phase.
- Any modification under `third_party/cct-boot` (remains read-only).

---

## Next Objective Action

Start **17A.1** with the design and implementation of:
- `verbum.char_at`
- `verbum.from_char`
- `cct/char` (`is_digit`, `is_alpha`, `is_whitespace`)

with dedicated integration tests following the current runner style.
