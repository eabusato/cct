# CCT Doc Generator

## Overview

FASE 12G introduces `cct doc`, a canonical API documentation generator for CCT projects.

It reuses project discovery (`FASE 12F`) and module closure resolution (`ADVOCARE`) to produce deterministic API docs.

## Command

```bash
cct doc [--project DIR] [--entry FILE] [--output-dir DIR] [--format markdown|html|both] \
        [--include-internal] [--no-examples] [--warn-missing-docs] [--strict-docs] [--no-timestamp]
```

## Defaults

- project root discovered like `cct build`
- output directory: `<project>/docs/api`
- format: `both`
- internal symbols excluded unless `--include-internal`

## Warnings and Strict Mode

- invalid doc tags produce warnings
- missing docs warnings require `--warn-missing-docs`
- `--strict-docs` returns exit code `2` when warnings exist

## Output Layout

```text
docs/api/
├── index.md / index.html
├── modules/
├── symbols/
└── assets/style.css
```

## Determinism

Use `--no-timestamp` for reproducible content hashing across repeated runs.
