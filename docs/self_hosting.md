# CCT Self-Hosting Pipeline

## Purpose

This document describes the self-hosting pipeline that became operational through phases 29 and 30.

## Stage Model

### Stage 0
- produced by the host compiler
- emits the bootstrap compiler artifacts

### Stage 1
- compiled using stage 0
- first self-hosted compiler artifact in the chain

### Stage 2
- compiled using stage 1
- convergence validation target

## Validation Contract

The self-hosting pipeline is accepted only when:
- stage 1 builds successfully
- stage 2 builds successfully
- stable outputs converge
- operational self-host workflows run on top of the converged compiler

## Commands

```bash
make bootstrap-stage0
make bootstrap-stage1
make bootstrap-stage2
make bootstrap-stage-identity
make bootstrap-selfhost-ready
```

## Operational Layer

Once convergence is established, the repository exposes:
- `project-selfhost-build`
- `project-selfhost-run`
- `project-selfhost-test`
- `project-selfhost-package`

These targets define the supported self-hosted workflow surface today.
