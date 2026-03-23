# CCT Release Status Through FASE 31

## Summary

CCT completed the self-hosted compiler promotion phase, making the bootstrap compiler the operational default.

Closed phase groups:
- FASE 0-10: compiler core and language foundation
- FASE 11-20: standard library, tooling, language growth, and application libraries
- FASE 21-29: bootstrap compiler and self-host convergence
- FASE 30: operational self-hosted platform
- FASE 31: self-hosted compiler promoted to default path

## Current Release Position

- Host compiler: operational, preserved as explicit fallback (`cct-host`)
- Bootstrap compiler: complete and promoted to default (`cct-selfhost`)
- Self-hosting: converged, validated, and operational
- Default compiler: **self-hosted** (switchable via promotion/demotion)
- Parity validation: formalized via matrix and automated testing
- Aggregated validation path: available from phase 0 through phase 31

## Canonical Green Gates

```bash
# Legacy and foundation
make test-legacy-rebased

# Full aggregated suite
make test-all-0-31

# Bootstrap identity (deterministic C generation)
make bootstrap-stage-identity

# Operational platform
make test-phase30-final

# Promotion and parity
make test-phase31-final
make test-bootstrap-parity  # Requires promotion

# Promotion control
make bootstrap-promote      # Switch default to selfhost
make bootstrap-demote       # Switch default to host

# Check active compiler
./cct --which-compiler      # Reports: selfhost or host
```

## Operational Compiler Status

### Default Compiler (Promoted)

After `make bootstrap-promote`, the default `./cct` wrapper uses the self-hosted compiler:

```bash
./cct program.cct           # Self-hosted compiler (stage2 backend)
./cct build --project app   # Self-hosted compilation
./cct run --project app     # Self-hosted workflow
./cct test --project app    # Self-hosted test runner
```

### Explicit Fallback Paths

Host compiler remains available:

```bash
./cct-host program.cct      # Host C compiler (explicit)
./cct-selfhost program.cct  # Self-hosted compiler (explicit)
```

### Mode Management

```bash
# Promote to selfhost default
make bootstrap-promote

# Demote to host default
make bootstrap-demote

# Query active mode
./cct --which-compiler

# Temporary override via environment
CCT_WRAPPER_MODE=host ./cct build --project app
```

## Parity Status

### Core Language (MUST_PASS: 95%+)

Bootstrap compiler is expected to pass 95%+ of core language tests:
- Lexer, parser, semantic analysis
- Type system (including generics)
- Code generation (expressions, statements, functions)
- Structs (SIGILLUM), enums (ORDO), pattern matching (ELIGE)
- Advanced features (TEMPTA/CAPE, FRANGE/RECEDE, FORMA)

Validation: `make test-bootstrap-parity`

### Stdlib Modules (Categorized)

**SUPPORTED** (exported in `selfhost_prelude.cct`):
- verbum (string operations)
- fmt (formatting)
- fs (file system operations)
- path (path manipulation)
- fluxus (dynamic arrays)
- parse (parsing utilities)

**PENDING** (not yet exported):
- config (configuration parsing)
- json (JSON encoding/decoding)
- db_sqlite (database operations)
- http (HTTP client)

**Workaround:** Use `./cct-host` for programs requiring pending modules.

### Tooling Commands (Host-Delegated)

The self-hosted wrapper delegates these to host:
- `fmt` - code formatter
- `lint` - linter
- `doc` - documentation generator
- `--sigilo-only` - graph visualization

These are tooling features, not compiler features. Wrapper transparently delegates to host implementation.

## Test Coverage Summary

| Phase Group | Tests | Status |
|-------------|-------|--------|
| Legacy (0-10) | ~1,696 | Host: ✅ / Bootstrap: Parity subset |
| Bootstrap (21-23) | ~1,932 | ✅ 100% (defines bootstrap itself) |
| Semantic (24-25) | ~2,025 | ✅ 100% (core language) |
| Codegen (26-28) | ~2,796 | ✅ 100% (code generation) |
| Self-hosting (29) | ~303 | ✅ 100% (identity validation) |
| Operational (30) | ~334 | ✅ 100% (workflows) |
| Promotion (31) | 31 | ✅ 100% (wrapper, parity, workflows) |
| **Total** | **~9,117+** | ✅ All relevant tests passing |

## Known Limitations (Documented)

1. **Performance:** Bootstrap ~2-3x slower than host (CCT vs C implementation)
2. **Stdlib pending modules:** config, json, db_sqlite, http require host compiler
3. **Error message text:** May differ from host (detection is identical, text varies)
4. **Tooling commands:** fmt, lint, doc delegated to host

See `docs/bootstrap_parity_matrix.txt` for complete expectations.

## Post-31 Roadmap

Future phases should focus on:

1. **Parity improvements:**
   - Export pending stdlib modules in selfhost_prelude
   - Implement tooling commands in bootstrap (or formalize delegation)
   - Error message harmonization

2. **Performance:**
   - Profiling and optimization of bootstrap compiler
   - Compilation speed improvements
   - Memory footprint reduction

3. **Platform maturity:**
   - Better diagnostics (source highlighting, suggestions)
   - Warning system (unused vars, dead code)
   - IDE integration (LSP, formatter, linter)

4. **Language expansion:**
   - Advanced type system features
   - Concurrency primitives
   - Multi-backend support (LLVM, WASM, native)

## Version Targets

### Current: v0.31 (Bootstrap Promoted)

- Self-hosted compiler operational and promoted
- Formal parity validation infrastructure
- Explicit promotion/demotion control
- Host fallback preserved

### Next: v0.32+

Post-bootstrap platform maturation:
- Diagnostics and error quality
- Performance optimization
- Stdlib expansion (pending modules)
- Developer experience improvements

### Future: v1.0

Production-ready self-hosted platform:
- Performance competitive with host
- Full stdlib parity
- Comprehensive tooling suite
- Security audit complete
- Formal language specification

## Bootstrap Era Completion

FASE 31 marks the **operational completion** of the bootstrap trajectory:

✅ **Phase 0-20:** Language and stdlib implemented in C
✅ **Phase 21-29:** Compiler reimplemented in CCT, self-hosting achieved
✅ **Phase 30:** Self-hosted workflows operational
✅ **Phase 31:** Self-hosted compiler promoted to default ← **CURRENT**

The project is now **operationally self-hosted** - developers use the CCT compiler written in CCT, not the C implementation, for day-to-day work.

The C host compiler (`cct.bin` / `cct-host`) is preserved as:
- Emergency fallback
- Regression baseline
- Performance comparison target
- Initial bootstrap seed

Future work focuses on **platform maturation**, not bootstrap enablement.

---

**Current active compiler:** Run `./cct --which-compiler` to check (selfhost or host)
**Promotion status:** Controlled via `make bootstrap-promote` / `make bootstrap-demote`
**Parity validation:** `make test-bootstrap-parity` (requires promotion)
**Full validation:** `make test-all-0-31`
