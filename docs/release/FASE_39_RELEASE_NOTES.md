# FASE 39 - Trace Visualization - Release Notes

**Date:** 2026-03-27
**Version:** 0.39.0
**Status:** Completed

## Executive Summary

FASE 39 delivered the trace visualization layer of Sigilo Vivo. Two C-only subphases implement an animated SVG renderer that overlays `.ctrace` spans onto the route sigil, and a visual taxonomy system for operational span categories with a stable color palette and exportable CSS.

The result is a self-contained, offline-capable SVG artifact that turns a `.ctrace` file into an animated, inspectable, and comparable trace visualization — no browser extension, no external server, no JavaScript framework required.

## Scope Closed

Delivered subphases:

- **39A**: Animated trace SVG renderer — renders `.ctrace` over route sigil with timeline and animation
- **39B**: Operational category overlays — visual taxonomy, color palette, CSS, legend

Both subphases are implemented in C (`src/sigilo/`) and are not CCT library modules. They are part of the `cct` CLI toolchain.

## Key Deliverables

### 39A — Animated Trace SVG Renderer

C implementation in `src/sigilo/trace_render.h` / `trace_render.c`.

#### Core Types

```c
typedef enum {
    TRACE_RENDER_SINGLE  = 0,   /* animated or static single trace */
    TRACE_RENDER_COMPARE = 1,   /* side-by-side comparison of two traces */
    TRACE_RENDER_STEP    = 2,   /* single step frame */
    TRACE_RENDER_STATIC  = 3,   /* non-animated static render */
} TraceRenderMode;

typedef struct {
    int64_t     span_id;
    int64_t     parent_id;     /* 0 = root */
    const char *name;
    const char *category;
    int64_t     start_us;
    int64_t     end_us;
    int         depth;         /* 0 = root */
    TraceAttr  *attrs;
    int         attr_count;
} TraceRenderSpan;
```

#### Public API

- `trace_render_load_ctrace(path, &out)` — load `.ctrace` from disk
- `trace_render_free(&trace)` — release loaded trace
- `trace_render_compute_depths(&trace)` — populate `depth` field on all spans
- `trace_render_single(&input, out_file)` — render single trace to SVG
- `trace_render_compare(&input, out_file)` — render comparison SVG for two traces
- `trace_render_step(&input, out_file)` — render one step frame; returns 1 when step_index >= span_count
- `trace_render_emit_timeline(&trace, out_file)` — emit timeline SVG block

#### Animation

When `input.animated == 1`, the renderer emits CSS `@keyframes` with per-span delays derived from normalized `start_us`:

```
delay_s    = (span.start_us - trace_start_us) / total_us * ANIM_DURATION_S
duration_s = (span.end_us - span.start_us)    / total_us * ANIM_DURATION_S
```

- Nodes: `opacity 0→1` with delay matching the first span that activates the node
- Edges: `stroke-dashoffset` animates from full length to 0
- Timeline lanes: `width 0→final` with span delay

#### Timeline Layout

The timeline sits below the sigil canvas. Each tree depth occupies one lane:

```
y = SIGILO_HEIGHT + TIMELINE_TOP + depth * LANE_HEIGHT
```

Spans within a lane are positioned proportionally to `start_us` / `total_us`.

#### Span-to-Sigil Mapping

The renderer resolves which sigil node/edge a span activates using this priority:

1. Span attribute `route_id` → sigil route node
2. Span attribute `module` → sigil group/region
3. Span `category` → default visual category for unmapped spans
4. Fallback → unresolved-spans chamber (`<g id="unresolved-spans">`) — never silently omitted

#### Comparison Mode

Two traces are aligned by matching spans on `(name, category, depth, sibling_position)`. For each matched pair the renderer emits `data-delta-us` on the SVG element. Spans exclusive to one trace are marked with `data-only-a` or `data-only-b`.

#### CLI Integration

```bash
# Render by trace ID (from Civitas trace store)
cct sigilo trace render <trace_id>
cct sigilo trace render <trace_id> --mode=animated
cct sigilo trace render <trace_id> --mode=step

# Render from file
cct sigilo trace render --trace request.ctrace --sigil routes.sigil --out trace.svg
cct sigilo trace render --static --trace request.ctrace --sigil routes.sigil --out trace.svg
cct sigilo trace render --step 2 --trace request.ctrace --sigil routes.sigil --out step2.svg

# Compare two traces
cct sigilo trace compare before.ctrace after.ctrace --sigil routes.sigil --out diff.svg
```

Flags: `--animated` / `--static`, `--step N`, `--filter-kind <category>`, `--focus-route <route_id>`, `--hide-timeline`, `--out <file>`

### 39B — Operational Category Overlays

C implementation in `src/sigilo/trace_overlay.h` / `trace_overlay.c`.

#### Span Categories

```c
typedef enum {
    SPAN_CAT_UNKNOWN    = 0,
    SPAN_CAT_SQL        = 1,
    SPAN_CAT_CACHE      = 2,
    SPAN_CAT_STORAGE    = 3,
    SPAN_CAT_TRANSCODE  = 4,
    SPAN_CAT_MAIL       = 5,
    SPAN_CAT_I18N       = 6,
    SPAN_CAT_TASK       = 7,
    SPAN_CAT_HTTP       = 8,
    SPAN_CAT_AUTH       = 9,
    SPAN_CAT_ERROR      = 10,
} SpanCategory;
```

#### Color Palette

| Category   | CSS Class        | Color          |
|------------|------------------|----------------|
| SQL        | `span-sql`       | `#3B82F6` (blue)   |
| Cache      | `span-cache`     | `#10B981` (emerald)|
| Storage    | `span-storage`   | `#8B5CF6` (violet) |
| Transcode  | `span-transcode` | `#F59E0B` (amber)  |
| Mail       | `span-mail`      | `#EC4899` (pink)   |
| i18n       | `span-i18n`      | `#06B6D4` (cyan)   |
| Task       | `span-task`      | `#F97316` (orange) |
| HTTP       | `span-http`      | `#6366F1` (indigo) |
| Auth       | `span-auth`      | `#14B8A6` (teal)   |
| Error      | `span-error`     | `#EF4444` (red)    |
| Unknown    | `span-unknown`   | `#9CA3AF` (gray)   |

Slow spans (duration > 2× median in their depth layer) receive the additional class `span-slow`.
Error spans (`category == SPAN_CAT_ERROR`) receive `span-error` regardless of duration.

#### Public API

- `trace_overlay_resolve_category(VERBUM category_str) → SpanCategory`
- `trace_overlay_heuristic(VERBUM span_name) → SpanCategory` — name-prefix heuristics when category unset
- `trace_overlay_style(SpanCategory) → OverlayStyle` — returns color, class name, label
- `trace_overlay_compute_meta(TraceRenderTrace*, SpanOverlayMeta* out)` — computes slow-span threshold per depth
- `trace_overlay_emit_css(FILE* out)` — writes full `<style>` block
- `trace_overlay_emit_legend(FILE* out)` — writes `<g id="legend">` SVG block
- `trace_overlay_emit_attrs(SpanCategory, VERUM is_slow, FILE* out)` — writes `class` and `data-category` on a span element

#### Name-Based Heuristics

When `span.category` is absent, `trace_overlay_heuristic` infers from `span.name` prefix:

- `select:`, `insert:`, `update:`, `delete:`, `sql:` → `SPAN_CAT_SQL`
- `cache:`, `get:`, `set:`, `evict:` → `SPAN_CAT_CACHE`
- `upload:`, `download:`, `storage:` → `SPAN_CAT_STORAGE`
- `send_mail:`, `enqueue_mail:` → `SPAN_CAT_MAIL`
- `task:`, `job:`, `worker:` → `SPAN_CAT_TASK`
- `http:`, `fetch:`, `request:` → `SPAN_CAT_HTTP`
- `auth:`, `login:`, `verify:` → `SPAN_CAT_AUTH`

## Design Decisions

### C-Only Implementation

The trace renderer and overlay are `cct` CLI tools, not CCT library modules. They are not in the bootstrap path. Implementing them in C directly (rather than in CCT calling C bridges) avoids circular dependencies and keeps the visualization toolchain buildable even when the CCT runtime is under development.

### Span 0 as Null Sentinel in Animation

CSS `animation-delay: 0s` is valid and means "start immediately." To avoid ambiguity, the renderer normalizes all delays to `>= 0` and clamps negative values to 0. `animation-duration: 0s` is also valid but produces an invisible animation; the renderer enforces a minimum duration of `50ms` per span.

### Unresolved Spans Are Never Silent

Every span appears in the SVG output. Spans that cannot be mapped to a sigil node appear in a dedicated `<g id="unresolved-spans">` chamber rather than being omitted. This prevents the viewer from hiding coverage gaps in the route model.

### Palette Is Stable and Semantic

The color palette is intentionally fixed and not user-configurable in FASE 39. Stability makes the palette learnable: a blue span is always a SQL span. Future phases may add a theme system, but the semantic mapping is a constant.

## Validation Summary

- ✅ `trace_render_single`: SVG well-formed XML for ≥1 span
- ✅ `trace_render_nested`: 3-span trace produces correct depth assignments and 2 timeline lanes
- ✅ `trace_render_animated`: `@keyframes` present; child delay ≥ parent delay
- ✅ `trace_render_step`: step 0 activates span 1 only; step 2 marks spans 1-2 as done; step 3 returns exit code 1
- ✅ `trace_render_compare`: `data-delta-us` present for matched spans; regression correctly identified
- ✅ `trace_render_unmapped`: unresolved span appears in `<g id="unresolved-spans">`
- ✅ No `nan` or `inf` in any numeric SVG attribute across all test cases
- ✅ `trace_overlay_resolve_category`: all 11 category strings
- ✅ `trace_overlay_heuristic`: name-prefix inference for each category
- ✅ `trace_overlay_emit_css`: valid CSS block, all 11 classes present
- ✅ `trace_overlay_emit_legend`: legend entries match palette table
- ✅ Slow-span threshold: spans > 2× median receive `span-slow` class

## Breaking Changes

None. FASE 39 is purely additive. The `cct` CLI gains new subcommands; existing commands are unchanged.

## User-Facing Impact

### Before FASE 39

```bash
cct sigilo trace view request.ctrace    # terminal text viewer (FASE 35C)
```

### After FASE 39

```bash
# Animated SVG — open in any browser
cct sigilo trace render --trace request.ctrace --sigil routes.sigil --out trace.svg
open trace.svg

# Step-by-step inspection
cct sigilo trace render --step 0 --trace request.ctrace --sigil routes.sigil --out step0.svg

# Performance regression comparison
cct sigilo trace compare before.ctrace after.ctrace --sigil routes.sigil --out diff.svg
open diff.svg
```

The output SVG is self-contained: no JavaScript, no external CSS, no server. It opens directly with `file://` in any modern browser or SVG viewer.

---

**Phases 32–39 complete.** The CCT standard library now covers: cryptography, encodings, regex, date/time, TOML, compression, file types, media, images, language detection, advanced strings, lexer utilities, UUIDs, slugs, i18n, form codec, structured logging, distributed tracing, metrics, signal handling, filesystem watching, audit logging, route topology metadata, navigable SVG sigilization, trace file format, framework manifests, PostgreSQL, full-text search, Redis, advisory locks, transactional mail, mail spool, webhook normalization, runtime instrumentation, context locals, animated trace rendering, and operational category overlays.
