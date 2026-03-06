# FASE 12 — Release Notes

> **CCT Structural Maturity Milestone**
> **Release Date:** March 2026
> **Version:** FASE 12H (final)

---

## Overview

**FASE 12** marks the **structural maturity milestone** of the CCT (Clavicula Turing) language project. This release transforms CCT from a language implementation into a **complete development toolchain** with:

- End-to-end developer workflow
- High-quality diagnostics
- Standalone tooling (formatter, linter, doc generator)
- Project-based build system
- Expanded standard library

FASE 12 was delivered across **eight sub-phases** (12A–12H), each adding critical infrastructure:

- **12A:** High-quality diagnostics
- **12B:** Numeric casts and coercions
- **12C:** Option and Result types
- **12D.1:** HashMap and HashSet
- **12D.2:** Functional collection operations
- **12D.3:** Iterator syntax
- **12E.1:** Standalone formatter
- **12E.2:** Canonical linter
- **12F:** Project-based build system
- **12G:** API documentation generator
- **12H:** Canonization and hardening

---

## Highlights

### 🎯 Complete Developer Workflow

FASE 12 delivers a **unified CLI experience** for all development tasks:

```bash
# Format code
./cct fmt src/main.cct lib/*.cct

# Lint code
./cct lint --strict src/main.cct

# Build project
./cct build

# Run tests
./cct test

# Generate docs
./cct doc --format both
```

No external tools required—everything is built into the `cct` binary.

---

### 📊 High-Quality Diagnostics (12A)

**Before FASE 12A:**
```
Error: type mismatch at line 42
```

**After FASE 12A:**
```
Error: type mismatch in assignment
  --> src/main.cct:42:8
   |
42 |   VINCIRE x AD "hello"
   |          ^    ^^^^^^^
   |          |    |
   |          |    found: VERBUM
   |          expected: REX (from declaration at line 40)
   |
   | Suggestion: Use cast GENUS(REX)(parse_int(value)) to convert VERBUM to REX
```

- Source snippets with precise position markers
- Actionable suggestions for common errors
- Consistent format across all compiler phases

---

### 🔢 Explicit Numeric Casts (12B)

Type-safe numeric conversions with explicit syntax:

```cct
EVOCA REX x AD 42
EVOCA FLAMMA y AD cast GENUS(FLAMMA)(x)  -- explicit cast required
```

- Prevents silent precision loss
- Clear error messages for invalid casts
- Controlled coercion rules for literals

---

### ✨ Ergonomic Error Handling (12C)

Nullable-free error handling with `Option` and `Result`:

```cct
ADVOCARE "cct/option.cct"
ADVOCARE "cct/result.cct"

RITUALE parse_safe(input VERBUM) REDDE OPTION(REX)
  EVOCA RESULT(REX) result AD parse_int(input)
  SI result.is_ok() COM
    REDDE Some(result.unwrap())
  ALITER
    REDDE None()
  EXPLICIT SI
EXPLICIT RITUALE
```

- `Some`/`None` for optional values
- `Ok`/`Err` for fallible operations
- `unwrap`, `unwrap_or`, `expect` for ergonomic access

---

### 🗂️ Hash-Based Collections (12D.1)

HashMap and HashSet for efficient key-value storage:

```cct
ADVOCARE "cct/map.cct"
ADVOCARE "cct/set.cct"

EVOCA HASH_MAP(REX, VERBUM) user_map
map_insert(user_map, 42, "Alice")
map_insert(user_map, 99, "Bob")

SI map_contains(user_map, 42) COM
  EVOCA VERBUM name AD map_get(user_map, 42)
  print(name)  -- "Alice"
EXPLICIT SI
```

- Deterministic iteration order (insertion-order preservation)
- Core operations: `insert`, `get`, `contains`, `remove`, `len`, `clear`

---

### 🔄 Functional Collection Operations (12D.2)

Functional programming patterns for data transformation:

```cct
ADVOCARE "cct/collection_ops.cct"

EVOCA FLUXUS(REX) numbers AD fluxus_init()
fluxus_push(numbers, 1)
fluxus_push(numbers, 2)
fluxus_push(numbers, 3)

-- Double all numbers
EVOCA FLUXUS(REX) doubled AD fluxus_map(numbers, double_fn)

-- Filter even numbers
EVOCA FLUXUS(REX) evens AD fluxus_filter(numbers, is_even_fn)

-- Sum all numbers
EVOCA REX sum AD fluxus_fold(numbers, 0, add_fn)
```

- Operations on `FLUXUS` and `SERIES`
- `map`, `filter`, `fold`, `find`, `any`, `all`

---

### 🔁 Iterator Syntax (12D.3)

Clean iteration over collections:

```cct
EVOCA FLUXUS(REX) numbers AD fluxus_init()
fluxus_push(numbers, 1)
fluxus_push(numbers, 2)
fluxus_push(numbers, 3)

ITERUM num IN numbers COM
  print_int(num)
FIN ITERUM
```

- Works with `FLUXUS`, `SERIES`, and collection-op results
- Syntax sugar for common iteration patterns

---

### 🎨 Standalone Formatter (12E.1)

Automatic code formatting with idempotency guarantee:

```bash
# Format in place
./cct fmt src/main.cct lib/*.cct

# Check formatting (CI mode)
./cct fmt --check src/main.cct  # exit 2 if unformatted

# Show diff without writing
./cct fmt --diff src/main.cct
```

- Deterministic and idempotent
- Semantic-preserving transformations only

---

### 🔍 Canonical Linter (12E.2)

Catch common mistakes with actionable warnings:

```bash
# Run lint rules
./cct lint src/main.cct

# Treat warnings as CI failure
./cct lint --strict src/main.cct  # exit 2 on warnings

# Apply safe automatic fixes
./cct lint --fix src/main.cct
```

**Rules:**
- `unused-variable`, `unused-parameter`, `unused-import`
- `dead-code-after-return`, `dead-code-after-throw`
- `shadowing-local`

---

### 🏗️ Project-Based Build System (12F)

Scalable project workflow with incremental compilation:

```bash
# Build project
./cct build

# Build and run
./cct run -- --my-args

# Run tests
./cct test

# Run benchmarks
./cct bench

# Clean artifacts
./cct clean --all
```

**Project structure:**
```
my-project/
├── src/main.cct        # Entry point
├── lib/*.cct           # Library modules
├── tests/*.test.cct    # Test files
├── bench/*.bench.cct   # Benchmark files
└── cct.toml            # Configuration
```

**Features:**
- Incremental compilation with cache invalidation
- Automatic test/benchmark discovery
- Clean separation of source and build artifacts

---

### 📚 API Documentation Generator (12G)

Generate professional API documentation:

```bash
# Generate markdown docs
./cct doc --format markdown

# Generate HTML docs
./cct doc --format html

# Generate both formats
./cct doc --format both

# Include internal symbols
./cct doc --include-internal

# Warn on missing docs
./cct doc --warn-missing-docs --strict-docs
```

**Doc comment format:**
```cct
-- /// Calculates the factorial of a number.
-- /// @param n The input number (must be non-negative).
-- /// @return The factorial of n.
RITUALE factorial(n REX) REDDE REX
  SI n <= 1 COM REDDE 1 EXPLICIT SI
  REDDE n * factorial(n - 1)
EXPLICIT RITUALE
```

- Parses `@param`, `@return`, `@example` tags
- Deterministic output with `--no-timestamp`
- Respects visibility (`ARCANUM` symbols hidden by default)

---

## Breaking Changes

**None.** FASE 12 is fully backward compatible with all prior phases (0–11).

---

## Deprecations

**None.** No features were deprecated in FASE 12.

---

## Migration Guide

### From FASE 11 to FASE 12

**No code changes required.** FASE 12 is a purely additive release.

**Optional migrations:**

1. **Adopt project structure:**
   - Move `main.cct` to `src/main.cct`
   - Move library modules to `lib/`
   - Create `cct.toml` for configuration

2. **Adopt new tooling:**
   - Run `./cct fmt` on existing code
   - Run `./cct lint --fix` to auto-fix warnings
   - Generate docs with `./cct doc`

3. **Adopt new stdlib modules:**
   - Use `cct/option` and `cct/result` for error handling
   - Use `cct/map` and `cct/set` for hash-based collections
   - Use `cct/collection_ops` for functional data transformations

---

## Recommended Workflow

FASE 12 establishes a **canonical daily workflow**:

```bash
# 1. Format code
./cct fmt src/main.cct lib/*.cct

# 2. Lint code
./cct lint --strict src/main.cct

# 3. Build project
./cct build

# 4. Run tests
./cct test

# 5. Generate documentation
./cct doc --format both --no-timestamp

# 6. Build release binary (when ready)
./cct build --release  # (future: optimization flags)
```

This workflow is **recommended but not enforced**. Use what makes sense for your project.

---

## Performance Notes

### Incremental Compilation

FASE 12F introduces **incremental compilation**:

- Cache stored in `.cct/cache/`
- Invalidates on source/import changes
- Typical rebuild after small change: **<100ms**

### Formatter Performance

- Typical formatting speed: **~10,000 lines/second**
- Idempotent (re-formatting is instant)

### Linter Performance

- Typical linting speed: **~5,000 lines/second**
- Scales linearly with file size

### Doc Generator Performance

- Typical doc generation: **~1,000 symbols/second**
- Deterministic output cached efficiently

---

## Known Issues

### General

- **Windows native support:** Implemented via MSYS2 UCRT64 / MinGW-w64.
  When running from Windows CMD/PowerShell, set `CC=C:\msys64\ucrt64\bin\gcc.exe`.
  No setup needed inside the MSYS2 UCRT64 terminal.
- **Cross-compilation:** Not yet supported (planned for future phase)

### Linter (12E.2)

- No advanced dataflow analysis (e.g., no "value may be uninitialized" warnings)
- `--fix` is limited to simple safe cases (more coverage planned)

### Doc Generator (12G)

- No runtime introspection (static analysis only)
- No doc testing (code examples in docs that run as tests)

### Build System (12F)

- No remote package manager (planned for FASE 13+)
- No parallel compilation (planned for future phase)

---

## Stability Classification Summary

The classification below is the preserved public summary for FASE 12.
Detailed matrix/snapshot artifacts are maintained in archived internal release records.

| Component               | Status        |
|-------------------------|---------------|
| Core Compiler           | `stable`      |
| Module System           | `stable`      |
| Sigilo Engine           | `stable`      |
| Advanced Typing         | `stable`      |
| Diagnostics (12A)       | `stable`      |
| Casts (12B)             | `stable`      |
| Formatter (12E.1)       | `stable`      |
| Build System (12F)      | `stable`      |
| Option/Result (12C)     | `experimental`|
| HashMap/Set (12D.1)     | `experimental`|
| Collection Ops (12D.2)  | `experimental`|
| Iterators (12D.3)       | `experimental`|
| Linter (12E.2)          | `experimental`|
| Doc Generator (12G)     | `experimental`|

---

## What's Next: FASE 13 Preview

Planned focus areas for FASE 13:

1. **Tooling maturity:**
   - Promote linter and doc generator to `stable`
   - Add more lint rules and doc tags

2. **Stdlib refinement:**
   - Promote `cct/option`, `cct/result` to `stable`
   - Optimize `cct/map`, `cct/set` performance

3. **Package management:**
   - Local package manager design
   - Dependency resolution (local first)

4. **Performance:**
   - Parallel incremental compilation
   - Build graph optimization

5. **Developer experience:**
   - LSP server exploration
   - Editor integration improvements

---

## Credits

FASE 12 was delivered as a coordinated effort across eight sub-phases, each building on the previous foundation. Special thanks to the rigorous phase discipline that ensured:

- Zero regressions
- Full backward compatibility
- Stable CLI experience
- Complete test coverage (531 tests green)

---

## Resources

- **FASE 12 Release Notes:** `docs/release/FASE_12_RELEASE_NOTES.md`
- **Release Notes Index:** `docs/release/`
- **Main README:** `README.md`
- **Spec:** `docs/spec.md`
- **Architecture:** `docs/architecture.md`

---

## Get Started

```bash
# Clone and build
git clone <repo-url>
cd cct
make

# Run tests
make test

# Try the formatter
./cct fmt examples/hello_world.cct

# Build a project
./cct build --project examples/fase12_final_showcase

# Generate docs
./cct doc --project examples/fase12_final_showcase --format both
```

**Welcome to FASE 12—the structural maturity milestone of CCT!**

---

**Document version:** 1.0
**Last updated:** March 2026 (FASE 12H)
**Maintainer:** Erick Andrade Busato
