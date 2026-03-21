# CCT Build and Validation System

## Overview

CCT now ships with two families of workflows:
- host-compiler workflows
- bootstrap/self-host workflows

Both are part of the supported engineering surface.

## Host Compiler Targets

Core commands:

```bash
make
make test
make dist
```

## Validation Targets

Default runner:

```bash
make test
```

Legacy and compatibility runners:

```bash
make test-legacy-full
make test-legacy-rebased
make test-all-0-30
```

Focused modern runners:

```bash
make test-bootstrap
make test-bootstrap-selfhost
make test-phase30-final
```

## Bootstrap and Self-Host Targets

Compiler stages:

```bash
make bootstrap-stage0
make bootstrap-stage1
make bootstrap-stage2
make bootstrap-stage-identity
```

Operational self-hosting:

```bash
make bootstrap-selfhost-ready
make project-selfhost-build PROJECT=examples/phase30_data_app
make project-selfhost-run PROJECT=examples/phase30_data_app
make project-selfhost-test PROJECT=examples/phase30_data_app
make project-selfhost-package PROJECT=examples/phase30_data_app
```

## Design Rules

- historical validation is preserved separately from current compatibility validation
- the aggregated `0..30` runner must remain available
- bootstrap/self-host validation must remain runnable independently from legacy suites
- operational project flows must stay scriptable from `make`

## Artifact Areas

Common locations:
- `out/bootstrap/`
- `tests/.tmp/`
- project-local `.cct/`
- project-local `dist/`

Generated artifacts should remain reproducible and disposable.
