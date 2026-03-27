# FASE 38 - Runtime Instrumentation - Release Notes

**Date:** 2026-03-26
**Version:** 0.38.0
**Status:** Completed

## Executive Summary

FASE 38 delivered the runtime instrumentation layer of the CCT standard library. Two modules provide span emission by operational category and request/task-scoped context storage.

These modules are the low-level foundation for Sigilo Vivo's live trace correlation: `cct/instrument` emits spans that feed the `.ctrace` format defined in FASE 35, while `cct/context_local` makes trace and request identity available across call boundaries without explicit parameter threading.

## Scope Closed

Delivered subphases:

- **38A**: `cct/instrument` — span emission by operational category with mode control
- **38B**: `cct/context_local` — request/task-scoped key-value context store

## Key Deliverables

### 38A — `cct/instrument`

Span emission for runtime instrumentation:

```cct
ADVOCARE "cct/instrument.cct"

CONIURA instrument_set_mode("active")      -- enable instrumentation

EVOCA REX span AD CONIURA instrument_open("db.query", INSTRUMENT_DB)
-- ... work ...
CONIURA instrument_close(span)
```

Span categories (`InstrumentKind`):

| Constant              | Semantic                        |
|-----------------------|---------------------------------|
| `INSTRUMENT_CALL`     | Generic function call           |
| `INSTRUMENT_DB`       | Database query                  |
| `INSTRUMENT_CACHE`    | Cache read/write                |
| `INSTRUMENT_MAIL`     | Email send/queue                |
| `INSTRUMENT_STORAGE`  | Object/file storage operation   |
| `INSTRUMENT_TASK`     | Background task execution       |
| `INSTRUMENT_HTTP`     | Outbound HTTP request           |
| `INSTRUMENT_CUSTOM`   | Application-defined category    |

Full API:

- `instrument_open(VERBUM name, REX kind) REDDE REX` — returns span_id; 0 if off
- `instrument_close(REX span_id)` — records end time; no-op if span_id is 0
- `instrument_attr(REX span_id, VERBUM key, VERBUM value)` — annotate span; no-op if 0
- `instrument_set_mode(VERBUM mode)` — `"active"` | `"off"`
- `instrument_flush(VERBUM path)` — write buffered spans to `.ctrace` file
- `instrument_reset()` — clear span buffer (for tests)

Mode is **off by default**. Instrumentation only activates when `instrument_set_mode("active")` is called, or when the environment variable `CCT_INSTRUMENT=1` is set at startup. This ensures zero overhead in non-instrumented deployments.

Span IDs are monotonic integers within a process lifetime. Span 0 is the null span (used as a no-op sentinel).

### 38B — `cct/context_local`

Request and task-scoped key-value context:

```cct
ADVOCARE "cct/context_local.cct"

CONIURA ctx_set("request_id", request_id)
CONIURA ctx_set("trace_id", trace_id)
CONIURA ctx_set("user_id", user_id)

-- anywhere in the call stack, without passing parameters:
EVOCA VERBUM rid AD CONIURA ctx_get("request_id")
```

Well-known keys (documented contract):

| Key           | Semantic                              |
|---------------|---------------------------------------|
| `request_id`  | Current HTTP request identifier       |
| `trace_id`    | Active trace identifier               |
| `user_id`     | Authenticated user identifier         |
| `locale`      | Active locale for i18n                |
| `route_id`    | Matched route from sigil              |
| `task_id`     | Background task identifier            |

Full API:

- `ctx_set(VERBUM key, VERBUM value)` — set value in current context
- `ctx_get(VERBUM key) REDDE VERBUM` — get value; empty string if not set
- `ctx_has(VERBUM key) REDDE VERUM` — check presence without default
- `ctx_clear(VERBUM key)` — remove key
- `ctx_reset()` — clear entire context (typically at request boundary)

Context storage is per-thread (OS threads / cooperative tasks within the same thread share one context store). Applications that use multiple threads must call `ctx_reset()` at the start of each thread's request handler.

## Design Decisions

### Instrument Mode Is Off by Default

Instrumentation has a real cost: every `instrument_open`/`instrument_close` pair allocates span state and records timestamps. For production deployments where tracing is not needed, the cost must be zero. The mode check is a single branch on a global flag — effectively free when off.

### Span ID Zero as No-Op Sentinel

When `instrument_set_mode("off")`, `instrument_open` returns 0. All downstream calls (`instrument_close`, `instrument_attr`) treat span_id 0 as a no-op. This allows callers to write instrumentation code unconditionally without `SI` guards around every call:

```cct
EVOCA REX span AD CONIURA instrument_open("op", INSTRUMENT_DB)
-- (mode is off: span == 0, everything below is a no-op)
CONIURA instrument_attr(span, "sql", query)
CONIURA instrument_close(span)
```

### `context_local` Uses Well-Known Keys, Not a Typed Struct

The context store uses string keys rather than a typed struct. This allows middleware layers to set context values without knowing the full schema of downstream consumers, and avoids requiring a shared type definition across modules.

## Integration with Sigilo Vivo

`cct/instrument` emits spans compatible with the `.ctrace` format defined in FASE 35C. A program can:

1. Set `instrument_set_mode("active")` at startup
2. Call `instrument_open`/`instrument_close` around operations
3. Call `instrument_flush("request.ctrace")` at the end of a request
4. View the trace with `cct sigilo trace view request.ctrace`

`cct/context_local` allows `cct/instrument` (and other modules like `cct/log` and `cct/audit`) to automatically attach `request_id` and `trace_id` to spans and log lines without requiring explicit parameter threading through every function call.

## Validation Summary

- ✅ `instrument`: open/close span produces correct start_us/end_us ordering
- ✅ `instrument`: attr recorded on span
- ✅ `instrument`: mode off → span_id 0 → all operations no-op
- ✅ `instrument`: flush writes valid `.ctrace` JSON Lines
- ✅ `instrument`: nested spans produce correct parent-child IDs
- ✅ `instrument`: buffer clear between test cases
- ✅ `context_local`: set/get/has/clear correctness
- ✅ `context_local`: reset clears all keys
- ✅ `context_local`: get on absent key returns empty string
- ✅ Freestanding rejection tests for each module

## Breaking Changes

None. FASE 38 is purely additive.

---

**Next:** FASE 39 — Trace Visualization (animated SVG renderer, operational category overlays)
