# CCT Examples (Baseline: FASE 20F)

Example programs showcasing Clavicula Turing language features and standard library usage.

Status note:
- This examples catalog remains valid on the current baseline (`FASE 20F completed`).
- Some filenames keep historical phase tags to preserve traceability.

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
```

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
