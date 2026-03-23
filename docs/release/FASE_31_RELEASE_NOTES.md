# FASE 31 - Self-Hosted Compiler Promotion - Release Notes

**Date:** 2026-03-21
**Version:** 0.31.0
**Status:** Completed

## Executive Summary

FASE 31 promoted the validated self-hosted compiler from a bootstrap artifact into the primary operational compiler path, closing the transition from "self-hosted compiler exists" to "self-hosted compiler is the default tool developers use."

This phase delivered explicit promotion/demotion infrastructure, CLI contract parity, project workflow integration, and a formal parity validation matrix ensuring the bootstrap compiler meets production quality standards.

## Scope Closed

Delivered subphases:
- **31A**: Self-host wrapper parity (compile, output handling, stdlib resolution)
- **31B**: CLI contract parity (--check, --ast, --tokens, --sigilo-only fallback)
- **31C**: Project workflow parity (build, run, test, bench, clean, package)
- **31D**: Promotion and demotion infrastructure (explicit, reversible, testable)
- **31E**: Default switch and final validation gate

## Key Deliverables

### Infrastructure

- `cct` - default compiler wrapper (mode-switchable via state file)
- `cct-host` - explicit host compiler entrypoint
- `cct-selfhost` - explicit self-hosted compiler entrypoint
- `cct.bin` - renamed host binary (preserved as fallback)
- `scripts/cct_wrapper.sh` - unified wrapper logic (266 lines)

### Makefile Targets

- `make bootstrap-promote` - switch default to self-host mode
- `make bootstrap-demote` - switch default back to host mode
- `make test-bootstrap-parity` - validate parity against core language tests
- `make test-phase31-final` - run all FASE 31 tests (1724-1754)
- `make test-all-0-31` - run full aggregated suite (0-31)

### Documentation

- `docs/bootstrap_parity_matrix.txt` - formal parity expectations matrix
- `tests/run_bootstrap_parity.sh` - automated parity validation runner
- `md_out/FASE_31*.md` - phase planning documents (6 files)

### Test Suite

31 integration tests (1724-1754):
- 7 tests: wrapper parity (31A)
- 6 tests: CLI parity (31B)
- 6 tests: workflow parity (31C)
- 6 tests: promotion/demotion (31D)
- 6 tests: final gate (31E)

## Major Technical Decisions

### Wrapper Architecture

The promotion system uses a **mode-based wrapper architecture** instead of binary replacement:

**Rationale:**
- Safer: host binary never overwritten, always available as `cct.bin` or `cct-host`
- Reversible: `make bootstrap-demote` instantly rolls back
- Testable: `./cct --which-compiler` reports active mode
- Auditable: state tracked in `out/bootstrap/phase31/state/default_mode.txt`

**Alternatives considered:**
- Direct binary replacement: rejected (too risky, hard to revert)
- Symlink switching: rejected (platform-dependent, confusing in version control)
- Environment variable: rejected (easy to forget, not persistent)

### CLI Parity Strategy

The wrapper implements **selective parity with intelligent fallback**:

**Implemented in self-host:**
- Core compile flow (input.cct → binary)
- --check (semantic validation)
- --ast (parser output)
- --tokens (lexer output)
- Project commands (build, run, test, bench, clean, package)

**Delegated to host:**
- --sigilo-only (graph visualization, host-only feature)
- fmt (code formatter, tooling not compiler)
- lint (linter, tooling not compiler)
- doc (documentation generator, tooling not compiler)
- --version, --help (dispatch to host for consistency)

**Rationale:** Focus compiler parity on compilation contract, defer tooling parity to future phases.

### Parity Validation Approach

Created formal parity matrix (`docs/bootstrap_parity_matrix.txt`) defining expectations:

**Categories:**
- **MUST_PASS**: Core language functionality (phases 0-10, 21-29)
- **SHOULD_PASS**: Stdlib supported modules (verbum, fs, path, fluxus, parse)
- **PENDING**: Features not yet in bootstrap (config, json, db_sqlite, http)
- **DELEGATE_TO_HOST**: Tooling commands (fmt, lint, doc)
- **BEHAVIORAL_PARITY**: Different implementation OK if behavior correct (performance, error text)

**Minimum passing rates:**
- Language core: 95%+ of MUST_PASS tests
- Stdlib supported: 90%+ of tests
- Self-hosting (29-30): 100% of tests
- Promotion (31): 100% of tests

## Validation Summary

The phase closed with:
- ✅ 31 promotion tests passing (1724-1754)
- ✅ Bootstrap identity preserved (stage1 C == stage2 C)
- ✅ Wrapper trio operational (cct, cct-host, cct-selfhost)
- ✅ Promotion/demotion idempotent and reversible
- ✅ Project workflows functional via self-host path
- ✅ Parity matrix documented and testable

### Gate Results

```bash
make test-phase31-final     # 31 tests, all pass
make bootstrap-promote       # Switches to selfhost mode
./cct --which-compiler       # Reports: selfhost
make bootstrap-demote        # Switches back to host mode
./cct --which-compiler       # Reports: host
make test-bootstrap-parity   # Validates core language parity (new)
make test-all-0-31           # Full aggregated suite 0-31
```

## User-Facing Impact

### Before FASE 31

```bash
./cct program.cct            # Host C compiler (always)
# Bootstrap only for validation:
./out/bootstrap/phase29/stage2/cct_stage2 program.cct program.c
gcc -o program program.c ... # Manual C compilation
```

### After FASE 31

```bash
./cct program.cct            # Self-hosted compiler (default after promotion)
./cct-host program.cct       # Host compiler (explicit)
./cct-selfhost program.cct   # Self-hosted compiler (explicit)

# Workflows via promoted compiler:
./cct build --project myapp
./cct run --project myapp
./cct test --project myapp

# Promotion control:
make bootstrap-promote       # Switch default to self-host
make bootstrap-demote        # Switch default to host
```

This is the first phase where **CCT is operationally self-hosted** - the compiler you use day-to-day is the one written in CCT, not the C implementation.

## Residual Limits at Phase Close

### Known Gaps (Documented in Parity Matrix)

1. **Stdlib modules pending:**
   - config, json, db_sqlite, http not exported in selfhost_prelude
   - Workaround: Use host compiler for programs requiring these modules
   - Roadmap: Address in post-31 stdlib expansion

2. **Tooling commands host-only:**
   - fmt, lint, doc remain host-only features
   - Wrapper delegates these commands transparently
   - Roadmap: Tooling parity not required for v1.0

3. **Error message text may differ:**
   - Error **detection** is identical (same errors caught)
   - Error **text** can vary between host and bootstrap
   - Exit codes match for scripting compatibility

4. **Performance baseline:**
   - Bootstrap ~2-3x slower than host (CCT vs C implementation)
   - Correctness prioritized over performance
   - Roadmap: Performance optimization in FASE 32+

### Not Gaps (Intentional Design)

- Host compiler preserved as `cct-host` (not removed, intentional fallback)
- Promotion not automatic (requires explicit `make bootstrap-promote`)
- Some tests may fail with selfhost (subset defined by parity matrix, not bugs)

## Migration Guide

### For Existing Projects

No migration required. After FASE 31:

**Default behavior (after promotion):**
```bash
make bootstrap-promote       # One-time switch
./cct build --project myapp  # Now uses selfhost
```

**Explicit fallback to host:**
```bash
./cct-host build --project myapp  # Force host compiler
```

**Temporary override:**
```bash
CCT_WRAPPER_MODE=host ./cct build --project myapp
```

### For CI/CD Pipelines

```yaml
# Option 1: Stay on host (safest for now)
script:
  - make bootstrap-demote  # Ensure host mode
  - ./cct build --project myapp

# Option 2: Use promoted selfhost
script:
  - make bootstrap-promote  # Switch to selfhost
  - ./cct build --project myapp

# Option 3: Explicit paths (most explicit)
script:
  - ./cct-host build --project myapp     # Always use host
  # or
  - ./cct-selfhost build --project myapp # Always use selfhost
```

## Release Outcome

FASE 31 closes the bootstrap era transition:

**Phases 0-20:** Language and stdlib foundation
**Phases 21-29:** Bootstrap compiler implementation and self-hosting convergence
**Phase 30:** Operational self-hosted platform
**Phase 31:** Self-hosted compiler promoted to default path ← **YOU ARE HERE**

The project now has:
- ✅ A production-quality self-hosted compiler
- ✅ Operational workflows via self-host path
- ✅ Explicit fallback to host compiler
- ✅ Formal parity validation infrastructure
- ✅ Documented subset boundaries (parity matrix)

Next phases (32+) should focus on:
- Platform maturity (diagnostics, performance, tooling parity)
- Stdlib expansion (pending modules)
- Advanced features (concurrency, optimization, multi-backend)

## Breaking Changes

None. FASE 31 is purely additive:
- `./cct` behavior unchanged by default (requires explicit promotion)
- All existing Makefile targets preserved
- All existing workflows continue to work
- Host compiler remains available

## Acknowledgments

This phase demonstrates the culmination of the bootstrap effort:
- Stage0, stage1, stage2 identity validation (FASE 29)
- Operational workflows (FASE 30)
- Production promotion infrastructure (FASE 31)

The self-hosted compiler is no longer just a validation artifact - it is now the **default recommended compiler path** for CCT development.

---

**Validation checkpoint:** `make test-phase31-final && make test-all-0-31`
**Parity validation:** `make test-bootstrap-parity` (requires promotion)
**Promotion status:** `./cct --which-compiler`
