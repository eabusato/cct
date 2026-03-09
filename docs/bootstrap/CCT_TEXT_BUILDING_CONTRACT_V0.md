# CCT Text Building Contract V0

This document formalizes the observable contracts of the textual track introduced in FASE 17B:
- `cct/verbum_builder.cct`
- `cct/code_writer.cct`

## 1) Builder Invariants

### `builder_len`
- `builder_len(b)` returns the current accumulated size in bytes.
- After each `builder_append`/`builder_append_char`, the length must reflect the exact applied growth.

### `builder_clear`
- `builder_clear(b)` resets the logical contents.
- After `builder_clear(b)`, `builder_len(b) == 0`.
- New appends after `clear` must work normally.

### `builder_to_verbum` (snapshot)
- `builder_to_verbum(b)` returns a textual snapshot of the current state.
- The snapshot must not corrupt the builder's internal state.
- Repeated calls without intermediate mutation must produce equivalent text.

## 2) Writer Invariants

### `writer_indent` / `writer_dedent`
- Each `writer_indent` increases the logical indentation level by 1.
- Each `writer_dedent` decreases it by 1.
- `writer_dedent` below zero is a contract error (underflow guard).

### `writer_write` / `writer_writeln`
- `writer_write` writes without automatic newline termination.
- `writer_writeln` writes content and terminates with `\n`.
- At the beginning of a line, the writer applies canonical indentation of 2 spaces per level.

### `writer_to_verbum`
- Returns the final textual snapshot of the writer's internal buffer.
- The result must be deterministic for the same sequence of calls.

## 3) Ownership and Lifecycle

- `builder_init` / `writer_init` allocate internal resources.
- `builder_free` / `writer_free` are mandatory to release resources.
- The `VERBUM` returned by `*_to_verbum` follows runtime string ownership; no separate manual `free` is required at the CCT level.

## 4) Definition of Textual Determinism

Determinism means:
- same input (source + arguments) and same sequence of operations;
- same output text byte-for-byte.

Phase gates validate determinism through repeated execution and `cmp -s` comparison.

## 5) Known Limits (Not Objectives of This Phase)

- No absolute throughput benchmark guarantee across machines.
- No latency SLA per append.
- This phase contract covers integrity, stability, and functional repeatability.
