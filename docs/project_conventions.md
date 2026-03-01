# CCT Project Conventions

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
