# CCT Installation Guide

## Scope

This guide describes how to build and validate the current CCT platform through FASE 30.

## Requirements

- POSIX shell
- `make`
- `gcc` or `clang`

## Build the Host Compiler

```bash
make
```

Expected result:
- `./cct` exists in the project root

## Recommended Validation Commands

Quick default suite:

```bash
make test
```

Full project-wide validation from phase 0 through phase 30:

```bash
make test-all-0-30
```

Legacy compatibility only:

```bash
make test-legacy-rebased
```

## Build Distribution Bundle

```bash
make dist
```

## Self-Hosting Validation

```bash
make bootstrap-stage-identity
make bootstrap-selfhost-ready
```

## Self-Hosted Project Smoke Test

```bash
make project-selfhost-build PROJECT=examples/phase30_data_app
make project-selfhost-test PROJECT=examples/phase30_data_app
```

## Installation Notes

CCT currently ships with:
- host compiler as the default user-facing compiler
- bootstrap/self-host tooling as validated operational paths inside the repository

The repository-local validation path is the canonical installation contract.
