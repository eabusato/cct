# CCT Examples (Baseline: FASE 15D.4)

Example programs showcasing Clavicula Turing language features and standard library usage.

Status note:
- This examples catalog remains valid on the current baseline (`FASE 15D.4 completed`).
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
**Option and Result types** (introduced in FASE 12D, still supported):
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
7. **lint_showcase_*.cct** - Code quality

## Documentation

- Language spec: `docs/spec.md`
- Standard library: `docs/bibliotheca_canonica.md`
- Architecture: `docs/architecture.md`
- Current phase status and next-step planning: `docs/roadmap.md`
- Historical release packages: `docs/release/`
