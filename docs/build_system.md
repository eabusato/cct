# CCT Build System

## Overview

FASE 12F introduces canonical project workflow commands:

- `cct build`
- `cct run`
- `cct test`
- `cct bench`
- `cct clean`

FASE 12G adds:

- `cct doc`

The legacy single-file flow remains fully supported.

## Project Discovery

Root resolution order:

1. `--project <dir>`
2. current directory containing `cct.toml`
3. current directory containing `src/main.cct`
4. upward traversal until `cct.toml` or `src/main.cct`

Default entry is `src/main.cct`, unless overridden with `--entry`.

## Build

```bash
cct build [--release] [--project DIR] [--entry FILE.cct] [--out PATH] \
          [--sigilo-check] [--sigilo-strict] [--sigilo-baseline PATH]
```

Optional quality gates:

- `--lint`
- `--fmt-check`

Optional sigilo baseline gate:

- `--sigilo-check`: runs `cct sigilo baseline check` for the built artifact sigilo
- `--sigilo-strict`: enables strict baseline check behavior (exit `2` on blocking drift)
- `--sigilo-baseline PATH`: baseline path override (relative paths are resolved from project root)
- default baseline path without override is `docs/sigilo/baseline/local.sigil` or `system.sigil` based on artifact scope

Incremental cache is stored in `.cct/cache/manifest.txt`.

## Run

```bash
cct run [--release] [--project DIR] [--entry FILE.cct] \
        [--sigilo-check] [--sigilo-strict] [--sigilo-baseline PATH] [-- --args]
```

Builds first, then executes resulting binary, returning the program exit code.

## Test

```bash
cct test [PATTERN] [--project DIR] [--strict-lint] [--fmt-check] \
         [--sigilo-check] [--sigilo-strict] [--sigilo-baseline PATH]
```

Discovers `*.test.cct` recursively under `tests/`.

## Bench

```bash
cct bench [PATTERN] [--project DIR] [--iterations N] [--release] \
          [--sigilo-check] [--sigilo-strict] [--sigilo-baseline PATH]
```

Discovers `*.bench.cct` recursively under `bench/` and reports average/total runtime.

## Clean

```bash
cct clean [--project DIR] [--all]
```

- default: removes `.cct/build`, `.cct/cache`, `.cct/test-bin`, `.cct/bench-bin`
- `--all`: also removes generated project binaries under `dist/`

## Exit Codes

- `0` success
- `1` command/build/run/test failure
- `2` quality gate failure (`--strict-lint`, `--fmt-check`)
- `2` also for strict sigilo baseline gate blocking drift
- `3` internal tooling error (reserved)
