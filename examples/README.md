# CCT Examples (Baseline: FASE 39)

Example programs showcasing Clavicula Turing language features and standard library usage.

Status note:
- This examples catalog is current through `FASE 39 completed`.
- Some filenames keep historical phase tags to preserve traceability.
- SVG artifacts (sigils and trace renders) are described in the **Visual Gallery** section below.

## Basic Examples

### hello.cct
Simple "Hello World" program demonstrating basic syntax.
- INCIPIT/EXPLICIT grimoire structure
- RITUALE definition
- OBSECRO scribe statement

```bash
cct hello.cct
./hello
```

## Language Showcase

### ars_magna_showcase.cct
**Comprehensive language tour** demonstrating:
- ORDO (enums) and SIGILLUM (structs)
- SERIES (arrays) and SPECULUM (pointers)
- Memory management (pete/libera/DIMITTE)
- Control flow (SI/ALITER, DUM, DONEC, REPETE)
- Exception handling (TEMPTA/CAPE/SEMPER, IACE)
- Function calls (CONIURA)

```bash
cct ars_magna_showcase.cct
./ars_magna_showcase
```

### option_result.cct
**Option and Result types** (introduced in FASE 12C, still supported):
- Option\<T\> for nullable values (Some/None)
- Result\<T, E\> for error handling (Ok/Err)
- Safe error patterns

```bash
cct option_result.cct
./option_result
```

## Collections & Iterators

### fluxus_demo.cct
**Dynamic collections** basics:
- Creating collections with fluxus_init
- Adding elements with fluxus_push
- Accessing with fluxus_get
- Memory cleanup with fluxus_free

```bash
cct fluxus_demo.cct
./fluxus_demo
```

### collection_ops_12d2.cct
**Functional operations**:
- Map: transform collections
- Filter: select elements
- Fold: aggregate values
- Chaining operations

```bash
cct collection_ops_12d2.cct
./collection_ops_12d2
```

### iterators.cct
**Iterator patterns** with ITERUM:
- Iterating FLUXUS collections
- Iterating SERIES arrays
- Using with map operations

```bash
cct iterators.cct
./iterators
```

## Tooling Examples

### lint_showcase_before_12e2.cct
Code with **linter warnings**:
- Unused variables
- Unreachable code

```bash
cct lint lint_showcase_before_12e2.cct
```

### lint_showcase_after_12e2.cct
**Clean version** after fixing warnings.

```bash
cct lint lint_showcase_after_12e2.cct
```

## Sigilo Examples

### sigilo_web_system_35
Stable modular Sigilo showcase with a composed system view and a realistic media upload trace.
- route-composed system sigil already present
- animated operational trace over the composed `.system.sigil`
- explicit `--sigil-view routes` option to project the same trace on pure route view
- interactive `--step` SVG with scrubber

```bash
./cct --sigilo-only \
  --sigilo-style routes \
  --sigilo-out examples/sigilo_web_system_35/routes_view \
  examples/sigilo_web_system_35/main.cct

./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --out examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg

./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --sigil-view routes \
  --out examples/sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg
```

### sigilo_creator_platform_39
Larger creator-platform Sigilo showcase with studio, auth, media, billing, moderation, analytics, notifications, admin, webhooks, and internal task routes.
- denser composed system sigil than `sigilo_web_system_35`
- deeper publish trace crossing several modules
- explicit route-only trace rendering via `--sigil-view routes`
- animated and `--step` renders over the real composed `.system.sigil`

```bash
./cct --sigilo-only \
  --sigilo-style routes \
  --sigilo-out examples/sigilo_creator_platform_39/routes_view \
  examples/sigilo_creator_platform_39/main.cct

./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace \
  --sigil examples/sigilo_creator_platform_39/routes_view.system.sigil \
  --out examples/sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg

./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace \
  --sigil examples/sigilo_creator_platform_39/routes_view.system.sigil \
  --sigil-view routes \
  --out examples/sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg
```

## Visual Gallery — SVG Artifacts

Every time the compiler runs with sigilo mode active, it emits a `.svg` artifact alongside the binary. Open any of these directly in a browser with `file://` — no server, no extension, no JavaScript framework. The animated ones use pure CSS `@keyframes`; they just play.

---

### Trace Visualization (FASE 39) — Animated

These are the richest artifacts in the repo. They overlay a real `.ctrace` execution trace onto the route sigil, coloring each span by operational category and animating the signal flow through the system.

#### `sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg`

The flagship animated trace. **1420×1344 px**, 2359 lines of SVG.

A creator-platform publish pipeline — auth check, SQL lookups, cache hits, storage upload, transcode queue, email confirmation, task dispatch — playing out as a CSS animation over the full composed system sigil. Each span arrives at its node in temporal order, color-coded: green for SQL, orange for cache, violet for storage, pink for transcode, blue for handlers. Below the sigil, a synchronized timeline lane shows the relative duration of every span at its depth.

Open it, watch it once, then hover over any node — the `data-category` tooltip appears. Slow spans (> 2× the median for their depth) glow with a drop-shadow.

```bash
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg
```

#### `sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg`

The same creator-platform trace, projected onto the **routes-only view** — 960×720 px, much more compact. The system structure compresses into a constellation of route nodes; spans animate along the arcs between them. Useful for focusing on the routing topology rather than module internals.

```bash
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg
```

#### `sigilo_creator_platform_39/creator_release_pipeline_39_step.svg`

**Step-by-step mode.** Same 1420×1370 px canvas, but with an interactive scrubber rail at the bottom. Each dot on the rail corresponds to one span in execution order. Clicking a dot (or dragging the knob) advances the trace one span at a time — nodes light up as active, previously completed spans dim. The timeline lanes update in sync. Ideal for reading a trace from beginning to end without the animation racing past.

```bash
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_step.svg
```

#### `sigilo_web_system_35/media_upload_pipeline_39_animated.svg`

**Media upload pipeline**, animated on the `sigilo_web_system_35` composed system sigil — 1383×1401 px, 1458 lines. This is the earlier, smaller system (web server + upload + processing modules) so the span graph is less dense, making it easier to read each individual span category as it arrives. A good starting point if the creator-platform trace feels like a lot at once.

```bash
open examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg
```

#### `sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg`

Routes-only view of the media upload trace — 960×720 px. The upload flow collapses to a clean arc sequence: request → auth → upload → transcode → callback. Category colors on the arcs tell the whole story in one frame.

```bash
open examples/sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg
```

#### `sigilo_web_system_35/media_upload_pipeline_39_step.svg`

Step mode for the media upload pipeline — 1383×1427 px, scrubber included. Slower system than the creator platform, so stepping through it is a good way to understand how the step scrubber interacts with the timeline lanes.

```bash
open examples/sigilo_web_system_35/media_upload_pipeline_39_step.svg
```

---

### Route and System Sigils

The static structural view — the topology of the program before any trace is overlaid.

#### `sigilo_creator_platform_39/routes_view.svg`

The bare route sigil for the creator platform — 1100×900 px. Ten module groups (studio, auth, media, billing, moderation, analytics, notifications, admin, webhooks, internal tasks) arranged as a constellation around the core. Each arc is a declared route; the density here is what makes the animated traces above look so complex. This is also the base canvas that the animated SVGs render on top of.

```bash
open examples/sigilo_creator_platform_39/routes_view.svg
```

#### `sigilo_web_system_35/routes_view.svg` and `sigilo_web_system_35/system_view.svg`

The `sigilo_web_system_35` system has two complementary views. `routes_view.svg` (1100×900 px) shows the pure route topology. `system_view.svg` (512×512 px, 354 lines) is the composed `.system.sigil` — a multi-ring structure where each ring represents a module in the closure. The system view is denser and shows cross-module dependencies as arcs between rings.

```bash
open examples/sigilo_web_system_35/routes_view.svg
open examples/sigilo_web_system_35/system_view.svg
```

#### `sigilo_rotas_constelacao_35.svg`

Standalone constellation-style routes showcase — 1208×1080 px, 508 lines. This is the largest pure-topology sigil in the repo. The layout algorithm places routes radially around a common core, producing a star-map appearance. No trace overlay; just the structure at rest. A good reference for what a real production-scale route set looks like in sigil form.

```bash
open examples/sigilo_rotas_constelacao_35.svg
```

---

### Showcase and Historical Sigils

#### `ars_magna_showcase.svg`

Sigil emitted by the comprehensive language tour program — 512×512 px, **591 lines** (the most detailed standard sigil in the examples). `ars_magna_showcase.cct` exercises nearly every language surface: ORDO, SIGILLUM, SERIES, SPECULUM, exception handling, all control flow forms. The resulting sigil is correspondingly rich, with many distinct node types visible in the ring structure.

```bash
open examples/ars_magna_showcase.svg
```

#### `tmp_sig_11h_complete.svg` and module companions

The FASE 11H stdlib closure sigil — 512×512 px, 280 lines — plus **seven individual module sigils**:
- `tmp_sig_11h_complete.__mod_001.svg` through `__mod_007.svg`

The `__mod_001` file is by far the most detailed, at **1396 lines**. Each module sigil captures the internal structure of one stdlib module. Together they show how the complete Bibliotheca Canonica looks when decomposed module by module. The parent `tmp_sig_11h_complete.svg` is the composed view of all seven.

```bash
open examples/tmp_sig_11h_complete.svg
open examples/tmp_sig_11h_complete.__mod_001.svg  # most detailed
```

#### `fase13_omniversal_13m.svg` and `fase13_omniversal_13m_variant.svg`

Two variants of the FASE 13M "omniversal" sigil — both 512×512 px. These were the first sigils generated with the full multi-ring omniversal layout, which places the primary module at center and arranges dependencies as concentric outer rings. The variant uses a slightly different arc-routing algorithm; comparing them side by side is a good illustration of how sigil layout parameters affect the visual output.

```bash
open examples/fase13_omniversal_13m.svg
open examples/fase13_omniversal_13m_variant.svg
```

#### `analisador_log_18_19.svg`

Sigil for the log-analyzer example — 512×512 px, **349 lines**. One of the richer single-module sigils: the log analyzer uses `cct/fs`, `cct/io`, `cct/verbum`, `cct/fmt`, and `cct/parse`, so the dependency arc structure is noticeably busier than simple examples.

```bash
open examples/analisador_log_18_19.svg
```

---

### Application Sigils

Smaller but instructive — each one is the structural fingerprint of a real example program.

| File | Size | What you see |
|------|------|-------------|
| `option_result.svg` | 512×512, 311 lines | Option/Result pattern — two distinct flow paths (Some vs None, Ok vs Err) visible as branching arcs |
| `cadastro_sqlite_app/src/main.svg` | 512×512, 106 lines | SQLite registration app — clean three-node structure: config → db → main |
| `phase30_data_app/src/main.svg` | 512×512, 125 lines | Phase 30 data app — slightly denser than cadastro; shows JSON + SQLite + HTTP in one sigil |
| `config_sqlite_app_20f2.svg` | 512×512 | Config-driven SQLite app — INI + SQLite dependency arc clearly visible |
| `http_simple_server_20f2.svg` | 512×512 | Single-request HTTP server — minimal arc structure, very readable as a first sigil |
| `hello.svg` | 512×512, 109 lines | The simplest possible sigil. One module, one ritual, one node. A clean baseline for comparison. |

```bash
open examples/option_result.svg
open examples/hello.svg
```

---

### Test Suite Sigils

The `cadastro_sqlite_app/tests/` directory contains sigils for the test files themselves:
- `app_flow.test.svg` — end-to-end flow test topology
- `db_smoke.test.svg` — database smoke test
- `query_filters.test.svg` — query filter test

These demonstrate that the sigilo system captures test modules as first-class artifacts, not just production code.

---

### A Note on `.system.svg`

For most examples you will also find a `.system.svg` companion alongside the main `.svg`. The `.system.svg` is the composed multi-module view — it includes the full transitive closure of imported modules, rendered as concentric rings. The plain `.svg` shows only the primary module. For small programs they look similar; for complex apps like `cadastro_sqlite_app` or `sigilo_web_system_35`, the difference is dramatic.

## Application Stack Examples (FASE 20)

### http_simple_server_20f2.cct
Single-request HTTP server returning JSON on `127.0.0.1:8091`.
- `cct/http` request accept/reply flow
- `cct/json` body construction
- explicit `Content-Type` header

```bash
./cct examples/http_simple_server_20f2.cct
./examples/http_simple_server_20f2
```

### http_json_client_20f2.cct
HTTP client consuming JSON from the local server.
- `http_get_json`
- `json_get_key` / `json_expect_*`
- local loopback workflow

Run after starting `http_simple_server_20f2.cct` in another terminal.

```bash
./cct examples/http_json_client_20f2.cct
./examples/http_json_client_20f2
```

### config_sqlite_app_20f2.cct
Configuration-driven SQLite mini app.
- INI parsing with `cct/config`
- local persistence with `cct/db_sqlite`
- explicit project-local file paths under `examples/.tmp`

```bash
./cct examples/config_sqlite_app_20f2.cct
./examples/config_sqlite_app_20f2
```

### udp_loopback_demo_20f2.cct
UDP loopback demo using only project-local host networking.
- `cct/net` high-level UDP helpers
- `cct/socket` timeout control
- `cct/option` unwrap flow for datagrams

```bash
./cct examples/udp_loopback_demo_20f2.cct
./examples/udp_loopback_demo_20f2
```

## Quick Start

```bash
# Compile
cct <example.cct>

# Run the compiled executable
./<example>

# Format code
cct fmt <example.cct>

# Lint code
cct lint <example.cct>
```

## Learning Path

1. **hello.cct** - Start here
2. **fluxus_demo.cct** - Collections basics
3. **iterators.cct** - Iteration patterns
4. **option_result.cct** - Error handling
5. **collection_ops_12d2.cct** - Functional programming
6. **ars_magna_showcase.cct** - Complete tour
7. **http_simple_server_20f2.cct** + **http_json_client_20f2.cct** - Local application protocol flow
8. **config_sqlite_app_20f2.cct** - Config + persistence integration
9. **udp_loopback_demo_20f2.cct** - Host networking basics
10. **lint_showcase_*.cct** - Code quality

## Documentation

- Language spec: `docs/spec.md`
- Standard library: `docs/bibliotheca_canonica.md`
- Architecture: `docs/architecture.md`
- Current phase status and next-step planning: `docs/roadmap.md`
- Historical release packages: `docs/release/`
