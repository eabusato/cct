# FASE 12 — Known Limits

> **Explicit boundaries of what is NOT in FASE 12**
> **Updated:** March 2026 (FASE 12H freeze)

---

## Purpose

This document **explicitly lists** what is **not included** in FASE 12. It provides:

- Clear boundaries for the current release
- Expectations management for users
- Roadmap context for future phases

---

## Language Features NOT in FASE 12

### Advanced Pattern Matching

**What's missing:**
- Exhaustiveness checking for `ORDO` beyond basic cases
- Pattern guards (e.g., `SI x = Some(y) ESTI y > 10`)
- Destructuring in function parameters
- Nested pattern matching

**Current state:**
- Basic `ORDO` matching works
- Manual case-by-case handling required

**Planned for:** FASE 13+

---

### Type Inference for Generics

**What's missing:**
- Automatic type parameter inference from usage
- `EVOCA result AD Some(42)` without explicit `OPTION(REX)`

**Current state:**
- Explicit instantiation required: `GENUS(ConcreteType)`

**Planned for:** FASE 13+

**Rationale:** Explicit instantiation ensures deterministic monomorphization and clear error messages

---

### Advanced Iterator Protocol

**What's missing:**
- Custom iterator implementation for user-defined types
- Iterator composition (e.g., chaining `map().filter().fold()`)
- Lazy evaluation (iterators that compute on-demand)
- Infinite iterators

**Current state:**
- `ITERUM ... IN ... COM` works for `FLUXUS`, `SERIES`, and collection-op results
- No way to implement custom iterators

**Planned for:** FASE 13+

---

### Trait System Beyond PACTUM

**What's missing:**
- Associated types in `PACTUM`
- Default implementations in `PACTUM`
- Multiple `PACTUM` conformance for single `SIGILLUM`
- Trait bounds beyond single-constraint `GENUS(T PACTUM C)`

**Current state:**
- Basic `PACTUM` declaration and conformance works
- Single-constraint generics supported

**Planned for:** FASE 14+

---

### Async/Await and Concurrency

**What's missing:**
- Async functions and `await` syntax
- Green threads or async runtime
- Channel-based message passing
- Thread primitives (spawn, join)
- Mutex, atomic operations

**Current state:**
- No concurrency support

**Planned for:** FASE 15+

**Rationale:** Concurrency requires careful design and runtime support

---

### Foreign Function Interface (FFI)

**What's missing:**
- Direct C function calls from CCT
- Binding to native libraries
- Interop with other languages

**Current state:**
- No FFI support (beyond internal C runtime)

**Planned for:** FASE 14+

---

### Macros or Metaprogramming

**What's missing:**
- Macro system (template expansion, code generation)
- Compile-time evaluation beyond constants
- Reflection or runtime introspection

**Current state:**
- No metaprogramming facilities

**Planned for:** Far future (no specific phase planned)

---

## Standard Library Gaps

### Network I/O

**What's missing:**
- TCP/UDP sockets
- HTTP client/server
- DNS resolution

**Current state:**
- Filesystem I/O only (`cct/fs`, `cct/io`)

**Planned for:** FASE 13+

---

### Regular Expressions

**What's missing:**
- Regex parsing and matching
- Pattern-based string operations

**Current state:**
- Manual string operations only (`cct/verbum`)

**Planned for:** FASE 14+

---

### JSON/XML Parsing

**What's missing:**
- Structured data format parsing
- Serialization/deserialization

**Current state:**
- Manual parsing only

**Planned for:** FASE 14+

---

### Compression and Cryptography

**What's missing:**
- Compression algorithms (gzip, zlib, etc.)
- Cryptographic primitives (hash, encrypt, sign)

**Current state:**
- None

**Planned for:** FASE 15+

---

### Date and Time

**What's missing:**
- Date/time types
- Timezone handling
- Duration arithmetic

**Current state:**
- None (can use host C `time()` via internal helpers)

**Planned for:** FASE 14+

---

### Advanced Collections

**What's missing:**
- Persistent data structures
- Immutable collections
- Specialized collections (priority queue, deque, trie)

**Current state:**
- Basic `FLUXUS`, `SERIES`, `HASH_MAP`, `HASH_SET`

**Planned for:** FASE 14+

---

## Tooling Limitations

### Linter (FASE 12E.2)

**What's missing:**
- Advanced dataflow analysis (e.g., "variable may be uninitialized")
- Complexity metrics (cyclomatic complexity, nesting depth)
- Security-focused rules (e.g., "potential SQL injection")
- Custom rule definitions
- Rule configuration (enable/disable per file or block)

**Current state:**
- Basic lint rules (`unused-*`, `dead-code-*`, `shadowing-*`)
- No dataflow analysis

**Planned for:** FASE 13 (more rules), FASE 14+ (advanced analysis)

---

### Formatter (FASE 12E.1)

**What's missing:**
- User-configurable formatting rules (indent size, line width, etc.)
- Multiple formatting profiles
- Per-file or per-block formatting overrides

**Current state:**
- Fixed formatting rules (deterministic, idempotent)

**Planned for:** Far future (formatting consistency is a feature, not a bug)

**Rationale:** One true style reduces bikeshedding

---

### Build System (FASE 12F)

**What's missing:**
- Remote package manager (fetch dependencies from registry)
- Dependency version resolution (semver, lock files)
- Parallel compilation (build multiple modules concurrently)
- Cross-compilation (target different architectures)
- Build graph visualization
- Custom build scripts or plugins

**Current state:**
- Local incremental compilation
- No remote dependencies
- Single-threaded builds

**Planned for:** FASE 13 (package manager), FASE 14 (parallel builds)

---

### Doc Generator (FASE 12G)

**What's missing:**
- Runtime documentation introspection
- Interactive documentation browser
- Doc testing (run code examples in docs as tests)
- Inheritance/conformance diagrams
- Search functionality in generated docs
- More doc tags (e.g., `@since`, `@deprecated`, `@see`)

**Current state:**
- Static documentation generation
- Basic tags: `@param`, `@return`, `@example`

**Planned for:** FASE 13 (more tags), FASE 14+ (doc testing)

---

### Language Server Protocol (LSP)

**What's missing:**
- LSP server for editor integration
- Autocomplete, go-to-definition, hover info
- Real-time diagnostics in editors
- Refactoring tools (rename, extract function, etc.)

**Current state:**
- None (CLI-only tooling)

**Planned for:** FASE 14+

**Rationale:** Requires stable language foundation (achieved in FASE 12)

---

### Debugger

**What's missing:**
- Interactive debugger (breakpoints, step, inspect)
- Core dump analysis
- Debug symbol generation

**Current state:**
- None (can use host C debugger on `.cgen.c` output)

**Planned for:** FASE 15+

---

### Profiler

**What's missing:**
- CPU profiling (time per function)
- Memory profiling (heap allocation tracking)
- Flamegraph generation

**Current state:**
- None

**Planned for:** FASE 14+

---

### Code Coverage

**What's missing:**
- Line/branch coverage tracking
- Coverage reports (HTML, lcov)
- Integration with CI pipelines

**Current state:**
- None

**Planned for:** FASE 14+

---

## Platform and Ecosystem Gaps

### Windows Native Support

**What's missing:**
- Native Windows builds (MSVC, MinGW)
- Windows path handling (`\` separators)
- Windows-specific APIs

**Current state:**
- macOS/Linux only (POSIX assumed)

**Planned for:** FASE 13

**Workaround:** Use WSL (Windows Subsystem for Linux) for now

---

### Package Registry

**What's missing:**
- Central package registry (like npm, crates.io)
- Package publishing workflow
- Package discovery and search

**Current state:**
- None (local packages only)

**Planned for:** FASE 13 (local package manager), FASE 14+ (remote registry)

---

### IDE Integration

**What's missing:**
- Official IDE plugins (VSCode, IntelliJ, etc.)
- Syntax highlighting (beyond basic TextMate grammars)
- Integrated build/run/debug

**Current state:**
- CLI-only workflow

**Planned for:** FASE 14+ (after LSP server)

---

### Web/Mobile Targets

**What's missing:**
- WebAssembly compilation target
- Mobile platform support (iOS, Android)

**Current state:**
- Native binary only (via host C compiler)

**Planned for:** Far future (no specific phase planned)

---

## Performance Limitations

### Compilation Speed

**Current state:**
- Single-threaded compilation
- No distributed compilation

**Future improvements:**
- Parallel module compilation (FASE 14)
- Build cache sharing across machines (far future)

---

### Incremental Compilation

**Current limitations:**
- Full module recompilation on change (no sub-module incremental)
- Cache invalidation is conservative (may rebuild more than needed)

**Future improvements:**
- Finer-grained incremental compilation (FASE 14)
- Better cache invalidation heuristics

---

### Runtime Performance

**Current limitations:**
- No whole-program optimization (LTO)
- No profile-guided optimization (PGO)
- No runtime JIT compilation

**Future improvements:**
- `--release` optimization flags (FASE 13)
- Link-time optimization (FASE 14)

---

### Memory Usage

**Current limitations:**
- No automatic memory pooling
- No arena allocators
- No reference counting (manual `OBSECRO pete/libera`)

**Future improvements:**
- Optional GC or reference counting (far future)
- Lifetime analysis for automatic deallocation (far future)

---

## Comparison with Other Languages

### vs. Rust

**CCT lacks:**
- Borrow checker (no lifetime analysis)
- Zero-cost abstractions (no guaranteed no-overhead)
- Cargo ecosystem

**CCT has:**
- Deterministic sigil generation (unique to CCT)
- Ritual-themed syntax (aesthetic choice)

---

### vs. Go

**CCT lacks:**
- Goroutines and channels
- Built-in concurrency runtime
- Large standard library

**CCT has:**
- Stronger type system (`GENUS`, `PACTUM`)
- Explicit memory management (no GC overhead)

---

### vs. Python

**CCT lacks:**
- Dynamic typing
- REPL (interactive mode)
- Massive ecosystem

**CCT has:**
- Compiled performance
- Static type safety
- Deterministic sigil metadata

---

## Workarounds and Alternatives

### For Network I/O

**Workaround:** Use host C bindings manually (requires editing `.cgen.c` output)

**Alternative:** Wait for FASE 13 network stdlib

---

### For Package Management

**Workaround:** Copy `.cct` files into `lib/` manually

**Alternative:** Wait for FASE 13 local package manager

---

### For Concurrency

**Workaround:** Launch multiple processes via host shell

**Alternative:** Wait for FASE 15 concurrency primitives

---

### For LSP/IDE Support

**Workaround:** Use basic syntax highlighting + CLI workflow

**Alternative:** Wait for FASE 14 LSP server

---

## Philosophy: What FASE 12 Optimizes For

FASE 12 optimizes for:

1. **Correctness:** Zero regressions, stable contracts
2. **Developer experience:** Complete toolchain, consistent CLI
3. **Foundation:** Stable base for future ecosystem growth

FASE 12 does NOT optimize for:

1. **Feature completeness:** Many language/stdlib features deferred
2. **Performance:** No advanced optimizations yet
3. **Ecosystem size:** No package registry or large stdlib

**Trade-off:** FASE 12 delivers a **small, stable, usable foundation** rather than a **large, unstable, incomplete system**.

---

## Summary: What's NOT Here

| Category                  | Missing Features                                      |
|---------------------------|-------------------------------------------------------|
| **Language**              | Advanced pattern matching, type inference, async/await, FFI |
| **Stdlib**                | Network I/O, regex, JSON/XML, crypto, date/time       |
| **Tooling**               | LSP, debugger, profiler, code coverage                |
| **Build System**          | Package manager, remote dependencies, parallel builds |
| **Platforms**             | Windows native, cross-compilation, WebAssembly        |
| **Performance**           | Parallel compilation, whole-program optimization      |

**Total deferred features:** ~40+

**Planned delivery:** FASE 13–15+

---

## Expectations for FASE 13

Based on FASE 12 foundation, FASE 13 will likely focus on:

1. **Local package manager** (no remote registry yet)
2. **Windows native support**
3. **More lint rules and doc tags**
4. **Promotion of experimental features to stable**
5. **Performance improvements** (parallel builds, `--release` mode)

See `docs/roadmap.md` for official roadmap updates.

---

## Conclusion

FASE 12 is **not feature-complete**—and that's intentional. It delivers:

- A **complete developer workflow** (format, lint, build, test, doc)
- A **stable language foundation** (types, modules, sigils)
- A **usable standard library** (core operations covered)

Everything else is **explicitly deferred** to future phases. This document ensures users know what to expect and what to wait for.

**FASE 12 is ready for real projects**—within its defined scope.

---

**Document version:** 1.0
**Last updated:** March 2026 (FASE 12H)
**Maintainer:** Erick Andrade Busato
