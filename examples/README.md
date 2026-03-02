# CCT Examples (FASE 12H)

Example programs showcasing Clavicula Turing language features and standard library usage.

## Basic Examples

### hello.cct
Simple "Hello World" program demonstrating basic syntax.
- INCIPIT/EXPLICIT grimoire structure
- RITUALE definition
- OBSECRO scribe statement

```bash
cct run hello.cct
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
cct run ars_magna_showcase.cct
```

### option_result.cct
**Option and Result types** (FASE 12D):
- Option\<T\> for nullable values (Some/None)
- Result\<T, E\> for error handling (Ok/Err)
- Safe error patterns

```bash
cct run option_result.cct
```

## Collections & Iterators

### fluxus_demo.cct
**Dynamic collections** basics:
- Creating collections with fluxus_init
- Adding elements with fluxus_push
- Accessing with fluxus_get
- Memory cleanup with fluxus_free

```bash
cct run fluxus_demo.cct
```

### collection_ops_12d2.cct
**Functional operations**:
- Map: transform collections
- Filter: select elements
- Fold: aggregate values
- Chaining operations

```bash
cct run collection_ops_12d2.cct
```

### iterators.cct
**Iterator patterns** with ITERUM:
- Iterating FLUXUS collections
- Iterating SERIES arrays
- Using with map operations

```bash
cct run iterators.cct
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
# Compile and run
cct run <example.cct>

# Just compile
cct compile <example.cct>

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
- Release notes: `docs/release/FASE_12_RELEASE_NOTES.md`
