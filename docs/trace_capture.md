# Trace Capture Guide

`cct/trace_capture` exists for the exact workflow of a live CCT application that keeps running while you decide when to start and stop recording.

The model is:

- your program enables runtime instrumentation
- you open a capture session
- real users or you manually hit real endpoints
- the runtime accumulates closed spans in memory
- `snapshot()` or `stop()` exports one `.ctrace` file per completed root request
- you render any exported trace back on top of the route/system sigil

## What It Is

`cct/trace_capture` is not a screen recorder and not a synthetic animation engine.

The SVG animation is derived from real runtime spans collected by `cct/instrument`, serialized as `.ctrace`, and then rendered by `cct sigilo trace render`.

## Session API

Module: `lib/cct/trace_capture.cct`

Main surface:

- `trace_capture_start(root_dir)`
- `trace_capture_start_in(root_dir, capture_id, mode)`
- `trace_capture_snapshot(session)`
- `trace_capture_flush(session)`
- `trace_capture_stop(session)`
- `trace_capture_dir(session)`
- `trace_capture_is_active(session)`

Behavior:

- `start` creates a capture directory and clears the instrumentation buffer
- `snapshot` exports only completed root trees
- open requests are preserved across snapshots and exported only after they close
- `stop` performs one last snapshot and disables instrumentation

## Directory Layout

Example:

```text
captures/live-demo/
  20260327_153000/
    snapshot-001/
      request-001-get-posts.ctrace
      request-002-post-media-upload.ctrace
    snapshot-002/
      request-001-post-profile-avatar.ctrace
```

Each exported `.ctrace` contains one completed root request tree.

## Minimal Usage

```cct
ADVOCARE "cct/instrument.cct"
ADVOCARE "cct/trace_capture.cct"

RITUALE main() REDDE REX
  EVOCA TraceCaptureSession capture AD CONIURA trace_capture_start_in(
    "captures/live-demo",
    "manual-window",
    INSTR_MODE_FULL
  )

  -- your server keeps running here
  -- real traffic happens here

  CONIURA trace_capture_snapshot(SPECULUM capture)

  -- more real traffic

  CONIURA trace_capture_stop(SPECULUM capture)
  REDDE 0
EXPLICIT RITUALE
```

## Request Instrumentation

To split export files by request, each real request should start as a root span.

Typical pattern:

```cct
EVOCA REX req AD CONIURA instrument_http_span("POST", "/media/upload")
CONIURA instrument_span_attr(req, "route_id", "media.upload")

-- nested work

CONIURA instrument_span_end(req)
```

Nested operations can add richer anchors for the renderer:

- `route_id`
- `module`
- `handler`
- `middleware`
- category hints such as `sql`, `cache`, `task`, `email`, `storage`

The more precise the span metadata is, the more precisely Sigilo can anchor the trace in the composed system view.

## Rendering A Captured Trace

Generate the composed system sigil:

```bash
./cct --sigilo-only --sigilo-style routes \
  --sigilo-out examples/sigilo_web_system_35/routes_view \
  examples/sigilo_web_system_35/main.cct
```

Render one exported request:

```bash
./cct sigilo trace render \
  --animated \
  --trace captures/live-demo/manual-window/snapshot-001/request-001-get-posts.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --out captures/live-demo/manual-window/request-001-get-posts.svg
```

## Practical Notes

- `snapshot()` exports only closed root trees; if a request is still open, it will be kept for the next snapshot
- `stop()` disables instrumentation after exporting the final closed requests
- this is intended for live server windows such as “record the next 10 seconds while I click through the app”
- for exact overlay placement on the `system.svg`, attach `route_id`, `module`, `handler`, and middleware metadata whenever your runtime can provide them
