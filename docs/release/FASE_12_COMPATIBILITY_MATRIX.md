# FASE 12 — Compatibility Matrix

> **Platform support and compatibility guarantees**
> **Updated:** March 2026 (FASE 12H freeze)

---

## Purpose

This document defines the **compatibility contract** for FASE 12:

- Supported host platforms and C compilers
- Command compatibility guarantees
- Cross-version compatibility
- Legacy workflow preservation
- Known platform-specific limitations

---

## Host Platform Support

### Supported Platforms

| Platform  | Architecture | C Compiler       | Status        | Notes                          |
|-----------|--------------|------------------|---------------|--------------------------------|
| macOS     | x86_64       | clang (Xcode)    | **Supported** | Primary development platform   |
| macOS     | arm64        | clang (Xcode)    | **Supported** | Apple Silicon native           |
| Linux     | x86_64       | gcc 7+           | **Supported** | Tested on Ubuntu 20.04+        |
| Linux     | x86_64       | clang 10+        | **Supported** | Tested on Ubuntu 20.04+        |
| Linux     | arm64        | gcc 7+           | **Expected**  | Not regularly tested           |

### Minimum Requirements

- **C Compiler:** C99-compatible compiler (gcc 7+, clang 10+)
- **Build System:** `make` (GNU Make 3.81+)
- **Shell:** POSIX-compatible shell for test scripts
- **Disk Space:** ~50MB for full build + stdlib + tests

### Not Supported (Out of Scope for FASE 12)

- Windows native (no MSVC/MinGW support yet)
- Cross-compilation to different architectures
- Embedded platforms (bare metal, microcontrollers)
- WebAssembly compilation

---

## Command Compatibility

### Single-File Workflow (Legacy)

The original single-file compilation workflow is **fully preserved**:

```bash
./cct file.cct                    # Compile to executable + sigil
./cct --tokens file.cct           # Show token stream
./cct --ast file.cct              # Show AST
./cct --check file.cct            # Syntax + semantic checks only
./cct --sigilo-only file.cct      # Generate sigil artifacts only
```

**Guarantee:** This workflow will remain supported indefinitely. FASE 12 additions do not break single-file usage.

---

### Project Workflow (FASE 12F+)

Project-based commands introduced in FASE 12F:

```bash
./cct build [--project DIR]       # Build project
./cct run [--project DIR]         # Build and run project
./cct test [--project DIR]        # Run project tests
./cct bench [--project DIR]       # Run project benchmarks
./cct clean [--project DIR]       # Clean build artifacts
```

**Guarantee:** Project commands are additive and do not interfere with single-file workflow.

---

### Tooling Commands (FASE 12E+)

Standalone tooling commands:

```bash
./cct fmt <file.cct> [...]        # Format files
./cct lint <file.cct> [...]       # Lint files
./cct doc [--project DIR]         # Generate documentation
```

**Guarantee:** Tooling commands work on both single files and project structures.

---

### Inspection Commands (Stable Since FASE 9)

Multi-module inspection:

```bash
./cct --ast-composite entry.cct   # Show composed multi-module AST
```

**Guarantee:** Composite AST inspection remains stable and deterministic.

---

## Exit Code Compatibility

All commands follow a **consistent exit code contract**:

| Exit Code | Meaning                                   | Commands                          |
|-----------|-------------------------------------------|-----------------------------------|
| `0`       | Success (no errors, no warnings in normal mode) | All commands                  |
| `1`       | Fatal error (compilation, runtime, I/O)   | All commands                      |
| `2`       | Warnings/mismatch in strict/check mode    | `fmt --check`, `lint --strict`, `doc --strict-docs` |

**Guarantee:** Exit codes will remain consistent across FASE 12+ releases.

---

## Sigilo Compatibility

### Deterministic Generation

Sigilo generation is **fully deterministic**:

- Same source + same mode → **identical sigil artifacts**
- No timestamps or random elements in sigil content (unless explicitly requested via metadata)

**Modes:**

| Mode         | Alias       | Purpose                          | Status   |
|--------------|-------------|----------------------------------|----------|
| `essencial`  | `essential` | Minimal sigil (structure only)   | Stable   |
| `completo`   | `complete`  | Full sigil (structure + metadata)| Stable   |

**Guarantee:** Both modes produce deterministic output. Aliases are permanent.

---

### Sigil Artifact Compatibility

| Artifact Type       | File Extension   | Format              | Stability |
|---------------------|------------------|---------------------|-----------|
| Local Sigil SVG     | `.svg`           | SVG 1.1             | Stable    |
| Local Sigil Meta    | `.sigil`         | JSON                | Stable    |
| System Sigil SVG    | `.system.svg`    | SVG 1.1 (composite) | Stable    |
| System Sigil Meta   | `.system.sigil`  | JSON                | Stable    |

**Guarantee:** File formats and naming conventions will not change.

---

## Module System Compatibility

### Import Resolution

Module resolution is **deterministic and stable**:

- `ADVOCARE "relative/path.cct"` resolves relative to current module
- `ADVOCARE "cct/module.cct"` resolves via stdlib wrapper
- Cycle detection and error reporting is consistent

**Guarantee:** Import resolution rules will not change.

---

### Visibility Rules

- **Direct imports only:** No implicit transitive visibility
- **ARCANUM:** Top-level internal visibility marker

**Guarantee:** Visibility semantics are frozen and will not change.

---

## Backward Compatibility with Previous Phases

FASE 12 **preserves full compatibility** with all prior phases:

| Phase   | Feature                          | Compatibility      |
|---------|----------------------------------|--------------------|
| 0–4     | Core language (parser/semantic)  | ✅ Fully preserved |
| 5       | Enums (ORDO)                     | ✅ Fully preserved |
| 6       | GENUS baseline                   | ✅ Fully preserved |
| 7       | Pointers, structs, allocation    | ✅ Fully preserved |
| 8       | Error handling (IACE/TEMPTA)     | ✅ Fully preserved |
| 9       | Modules (ADVOCARE), sigils       | ✅ Fully preserved |
| 10      | Advanced typing (GENUS/PACTUM)   | ✅ Fully preserved |
| 11      | Stdlib foundation (cct/...)      | ✅ Fully preserved |

**No breaking changes** were introduced in FASE 12.

---

## Forward Compatibility Considerations

### What FASE 12 Does NOT Guarantee

1. **Internal artifact formats:**
   - `.cgen.c` structure is internal and may change
   - `.cct/cache/` internal directory format may change
   - Compiled binary format is host-dependent

2. **Experimental features:**
   - `cct/option`, `cct/result`, `cct/map`, `cct/set` APIs may be refined
   - Linter rule set may expand (but existing rules won't break)
   - Doc generator tags may expand (but existing tags won't break)

3. **Performance characteristics:**
   - Compilation speed, memory usage, binary size are not guaranteed
   - Incremental compilation heuristics may change

### Cross-Phase Note (FASE 13C.1)

FASE 13 introduces formal schema governance for sigilo metadata while preserving FASE 12 compatibility guarantees:

- canonical schema stays `cct.sigil.v1` during FASE 13;
- schema evolution is additive by default;
- deprecated fields require coexistence period and explicit migration plan.

Reference: `docs/sigilo_schema_13c1.md`.

---

## Incremental Build Compatibility

### Cache Invalidation Contract

Incremental builds **correctly invalidate** when:

- Source file contents change
- Imported module contents change
- `cct.toml` configuration changes
- Build profile changes (e.g., `--release` when implemented)

**Guarantee:** Incremental builds produce **identical results** to clean builds.

---

### Cache Format

The `.cct/cache/` directory format is **internal**:

- Format may change between versions
- Cache is automatically rebuilt if format is incompatible
- No manual migration needed

---

## CLI Flag Compatibility

### Stable Flags (Will Not Change)

| Flag                  | Introduced | Status   |
|-----------------------|------------|----------|
| `--tokens`            | Phase 0    | Stable   |
| `--ast`               | Phase 0    | Stable   |
| `--ast-composite`     | Phase 9    | Stable   |
| `--check`             | Phase 1    | Stable   |
| `--sigilo-only`       | Phase 9    | Stable   |
| `--no-color`          | Phase 12A  | Stable   |
| `--sigilo-style`      | Phase 9    | Stable   |
| `--sigilo-mode`       | Phase 10   | Stable   |
| `--sigilo-out`        | Phase 9    | Stable   |
| `--sigilo-no-meta`    | Phase 9    | Stable   |
| `--sigilo-no-svg`     | Phase 9    | Stable   |

### Experimental Flags (May Change)

| Flag                  | Introduced | Status   | Notes                          |
|-----------------------|------------|----------|--------------------------------|
| `--release`           | Phase 12F  | Placeholder | Not yet functional, reserved  |

---

## Stdlib Compatibility

### Stable Modules (API Frozen)

The following stdlib modules have **frozen APIs**:

- `cct/verbum`, `cct/fmt`, `cct/series`, `cct/alg`
- `cct/mem`, `cct/fluxus`, `cct/io`, `cct/fs`
- `cct/path`, `cct/math`, `cct/random`, `cct/parse`, `cct/cmp`

**Guarantee:** These APIs will not break in future phases.

---

### Experimental Modules (API May Change)

The following stdlib modules are **functional but may be refined**:

- `cct/option`, `cct/result` (minor ergonomic improvements possible)
- `cct/map`, `cct/set` (API may be optimized or extended)
- `cct/collection_ops` (more combinators may be added)

**Guarantee:** Changes will be documented and migration path provided.

---

## Known Platform-Specific Limitations

### macOS

- **File paths:** Case-insensitive by default (APFS defaults)
  - May cause issues if two modules differ only in case
  - Workaround: Use case-sensitive APFS volume for CCT projects

### Linux

- **File descriptor limits:** Very large projects may hit `ulimit -n` limit
  - Workaround: Increase limit with `ulimit -n 4096` or higher

### General POSIX Assumptions

CCT assumes a **POSIX-like environment** for:

- File path separators (`/`)
- Line endings (accepts both `\n` and `\r\n`, outputs `\n`)
- Shell execution (`tests/run_tests.sh` requires POSIX shell)

**Windows native support** is planned for future phases but not included in FASE 12.

---

## Cross-Version Migration

### Upgrading from FASE 11 to FASE 12

**No breaking changes:**

- All FASE 11 code compiles and runs identically in FASE 12
- New features (formatter, linter, build system, doc generator) are **additive**

**Migration steps:**

1. Rebuild project with FASE 12 compiler
2. Optionally adopt new tooling (`cct fmt`, `cct lint`, `cct build`)
3. Optionally migrate to project structure if desired

**No code changes required.**

---

### Future Upgrades (FASE 13+)

**Commitment:**

- Breaking changes to `stable` features will be **extremely rare**
- Deprecation cycle will be provided (warning → error → removal)
- Migration guides will be provided for all breaking changes

---

## Tooling Compatibility

### Formatter Idempotency

**Guarantee:**

- Running `cct fmt` multiple times produces **identical output**
- Formatted code **compiles and behaves identically** to original

---

### Linter Fix Safety

**Guarantee:**

- `cct lint --fix` only applies **semantically safe** transformations
- Fixed code **compiles and behaves identically** to original
- If fix cannot be safely automated, warning remains

---

### Doc Generator Determinism

**Guarantee:**

- `cct doc --no-timestamp` produces **deterministic output**
- Re-running with same source produces **bit-identical docs** (excluding timestamps)

---

## Summary of Guarantees

| Area                     | Guarantee                                                      |
|--------------------------|----------------------------------------------------------------|
| Single-file workflow     | ✅ Preserved indefinitely                                      |
| Project workflow         | ✅ Additive, does not break single-file usage                 |
| Exit codes               | ✅ Consistent across all commands                             |
| Sigilo generation        | ✅ Fully deterministic                                        |
| Module resolution        | ✅ Stable and deterministic                                   |
| Backward compatibility   | ✅ No breaking changes from FASE 0–11                         |
| Stable stdlib API        | ✅ Will not break in future phases                            |
| Formatter idempotency    | ✅ Guaranteed                                                 |
| Linter fix safety        | ✅ Guaranteed                                                 |
| Doc determinism          | ✅ Guaranteed with `--no-timestamp`                           |

---

## Contact and Support

For compatibility issues or questions:

- **Issue tracker:** (to be determined)
- **Documentation:** See `docs/` directory

---

**Document version:** 1.0
**Last updated:** March 2026 (FASE 12H)
**Maintainer:** Erick Andrade Busato
