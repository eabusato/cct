# CCT Project Conventions

## Canonical Project Layout

```text
project/
├── src/
│   └── main.cct
├── lib/
├── tests/
├── bench/
├── docs/
├── dist/
├── .cct/
└── cct.toml
```

## Naming

- modules: `snake_case.cct`
- tests: `*.test.cct`
- benchmarks: `*.bench.cct`
- generated C: `*.cgen.c`

## Project Rules

- keep entry orchestration in `src/main.cct`
- keep reusable code in `lib/`
- keep tests deterministic whenever possible
- keep benchmark fixtures explicit and isolated
- do not commit transient `.cct/` artifacts

## Self-Hosted Project Rules

When using the self-hosted toolchain:
- keep the canonical layout unchanged
- use `project-selfhost-*` targets from the repository root
- treat support/prelude constraints as explicit product boundaries

## Validation Rules

Recommended local workflow:

```bash
cct fmt src/main.cct
cct lint --strict src/main.cct
cct test
cct build --release
```

Repository-level validation:

```bash
make test-all-0-30
```
