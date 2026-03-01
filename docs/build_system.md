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
cct build [--release] [--project DIR] [--entry FILE.cct] [--out PATH]
```

Optional quality gates:

- `--lint`
- `--fmt-check`

Incremental cache is stored in `.cct/cache/manifest.txt`.

## Run

```bash
cct run [--release] [--project DIR] [--entry FILE.cct] [-- --args]
```

Builds first, then executes resulting binary, returning the program exit code.

## Test

```bash
cct test [PATTERN] [--project DIR] [--strict-lint] [--fmt-check]
```

Discovers `*.test.cct` recursively under `tests/`.

## Bench

```bash
cct bench [PATTERN] [--project DIR] [--iterations N] [--release]
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
- `3` internal tooling error (reserved)
