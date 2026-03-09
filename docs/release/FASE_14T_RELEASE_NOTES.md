# CCT — Release Notes: FASE 14T

**Date**: March 2026  
**Compiler version**: FASE 19D.4 + 14T  
**Tests**: 1120 passed / 0 failed (full suite)

---

## Summary

FASE 14T turns sigilo SVG into a semantic reading artifact without breaking the existing visual contract:

- native `<title>` on semantic elements already emitted by the SVG;
- deterministic additive `data-*` on local nodes and `call` edges;
- lightweight root semantics (`role`, `aria-label`, `desc`) when instrumentation is enabled;
- explicit toggles to generate instrumented SVG or plain pre-14T SVG;
- no required JavaScript.

In practice, sigilo components now support direct hover in the SVG itself: circles, lines, and edges reveal semantic context without a separate viewer.

The phase closed with deterministic output preserved and a full green regression gate.

---

## 1) Native Semantic Hover

The SVG can now be read directly in a browser or any tool that respects native SVG `<title>`.

Delivered coverage:
- `RITUALE` nodes
- structural nodes (`branch`, `loop`, `bind`, `term`)
- local edges (`primary`, `call`, `branch`, `loop`, `bind`, `term`)
- system sigilo nodes and edges

Hover payloads are derived from real program context: ritual name, statement kind, nesting depth, relevant counts, and normalized source excerpt.

---

## 2) Additive Metadata

Local SVG gained deterministic `data-*` without changing the `.sigil` contract:

- local nodes: `data-kind`, `data-ritual`, `data-line`, `data-col`, `data-depth`, `data-stmt`, and related fields
- `call` edges: `data-kind`, `data-from`, `data-to`, `data-weight`, `data-self-loop`

Scope intentionally remained closed in this phase:
- no metadata proliferation across every system-sigilo element
- no new `.sigil` schema

---

## 3) Instrumentation Toggling

New CLI options:

```bash
./cct --sigilo-only --sigilo-no-titles file.cct
./cct --sigilo-only --sigilo-no-data file.cct
./cct --sigilo-only --sigilo-no-titles --sigilo-no-data file.cct
```

Contract:
- `--sigilo-no-titles`: removes `<title>` and hover wrappers
- `--sigilo-no-data`: removes additive `data-*` and root `<desc>`
- both together: restore plain pre-14T SVG

This gives three practical modes:
- enriched human reading
- incremental tooling consumption
- minimal structural baseline preservation

---

## 4) Quality and Non-Regression

Quality work delivered in this phase:
- LF/CRLF normalization in source-context extraction
- deterministic tooltip escaping and clipping
- local and system regression coverage in the main runner
- cleanup of generated phase artifacts inside `tests/run_tests.sh`
- lean runner output showing failures only plus the final summary

Final gate:
- `make test`
- result: **1120 passed / 0 failed**

---

## 5) Out of Scope

Explicitly out of scope for FASE 14T:
- dedicated web viewer
- JavaScript-driven interactive navigation
- search, filters, semantic zoom, or animation
- new `.sigil` schema
- large visual redesign of the layout engine

---

## 6) Next Phase

With FASE 14T closed, the official next step returns to:
- **FASE 20 — Application Library Stack Expansion**
