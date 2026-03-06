# FASE 12 Final Showcase

This example demonstrates the complete capabilities of FASE 12:

Status note:
- This is a historical showcase introduced in FASE 12.
- It remains compatible with the current project baseline (`FASE 15D.4 completed`).

## Features Showcased

### Language Features
- Multi-module project structure
- `ADVOCARE` module imports
- `SIGILLUM` struct definitions
- `SPECULUM` pointers

### FASE 12 Additions
- **Collections (12D.1):** HashMap for key-value storage
- **Error Handling (12C):** Option type for safe null handling
- **Collection Ops (12D.2):** Functional operations on vectors
- **Doc Comments:** Triple-slash documentation for API generation

### Tooling Integration
- **Project Structure:** src/, lib/, tests/, bench/ organization
- **Build System (12F):** `cct build` support
- **Testing (12F):** `cct test` discovers smoke.test.cct
- **Benchmarks (12F):** `cct bench` discovers perf.bench.cct
- **Documentation (12G):** `cct doc` generates API docs from /// comments

## Build and Run

```bash
# Build the project
./cct build --project examples/fase12_final_showcase

# Run the executable
./cct run --project examples/fase12_final_showcase

# Run tests
./cct test --project examples/fase12_final_showcase

# Run benchmarks
./cct bench --project examples/fase12_final_showcase

# Generate documentation
./cct doc --project examples/fase12_final_showcase --format both
```

## Code Organization

- `src/main.cct`: Entry point, demonstrates high-level workflow
- `lib/data.cct`: Data storage module with HashMap wrapper
- `lib/ops.cct`: Operations module with collection processing
- `tests/smoke.test.cct`: Basic smoke test
- `bench/perf.bench.cct`: Simple performance benchmark

## Documentation

Run `cct doc` to generate API documentation in `docs/api/`.
