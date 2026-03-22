# CCT Project Conventions

This document preserves the original project-layout conventions and extends them with the bootstrap, self-host, and aggregated-validation conventions that are now part of the operational repository baseline.

## Canonical Layout

```text
project/
├── src/
│   └── main.cct
├── lib/
├── tests/
├── bench/
├── examples/
├── docs/
└── cct.toml (optional in FASE 12F)
```

## Naming

- modules: `snake_case.cct`
- tests: `<feature>.test.cct`
- benchmarks: `<feature>.bench.cct`

## Modular Organization

- keep orchestration in `src/main.cct`
- keep reusable code in `lib/`
- keep tests isolated from import side effects
- keep benchmarks deterministic whenever practical

## Build Artifacts

Generated artifacts are local to project:

- `.cct/` for internal cache and runners
- `dist/` for final binaries

Never commit `.cct/` artifacts.

## Recommended Local Flow

```bash
cct fmt src/main.cct
cct lint --strict src/main.cct
cct test
cct build --release
```

## Bootstrap and Self-Host Layout Conventions

Additional repository-owned locations now matter operationally:
- `src/bootstrap/` for the self-hosted compiler implementation
- `out/bootstrap/` for stage artifacts, reports, and convergence metadata
- `tests/integration/` for both host-era and bootstrap/self-host integration fixtures

## Validation Conventions

Use the narrowest runner that matches the subsystem under active development, but use `make test-all-0-30` before treating documentation, release work, or large refactors as complete.

Recommended hierarchy:
1. subsystem runner
2. phase runner
3. aggregated whole-project runner
