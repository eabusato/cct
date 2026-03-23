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

## Promotion Layer (FASE 31)

FASE 31 adds a promotion layer above the stage model.

This layer separates three things that were previously easy to conflate:
- bootstrap stage artifacts
- the operational self-hosted wrapper
- the default compiler entrypoint seen by users

### Artifact Roles

- `out/bootstrap/phase29/stage0/...`: host-produced bootstrap seed
- `out/bootstrap/phase29/stage1/...`: first self-hosted compiler artifact
- `out/bootstrap/phase29/stage2/...`: converged compiler artifact used for identity validation and self-host preparation
- `./cct-selfhost`: explicit operational wrapper for the self-hosted compiler path
- `./cct`: default wrapper exposed to users
- `./cct-host`: explicit host fallback wrapper

### Promotion Commands

```bash
make bootstrap-promote
make bootstrap-demote
```

Practical meaning:
- promotion changes the default compiler mode used by `./cct`
- demotion restores the host compiler as the default wrapper mode
- neither operation deletes the host compiler or the self-hosted compiler wrapper

### What Is Self-Hosted Today

Self-hosted in the current operational sense:
- the compiler front-end and bootstrap code generation path
- direct compile workflows through `./cct-selfhost`
- promoted/default compile workflows through `./cct` when the wrapper mode is `selfhost`
- operational project workflows validated in phases 30 and 31

### What Still Uses Host Fallback

Host fallback may still be used for:
- tooling-oriented commands whose self-host implementation is not yet promoted end to end
- compatibility-preserving wrapper paths where the repository intentionally reuses host orchestration logic while keeping the promoted compiler path visible to users
- explicit recovery or regression comparison via `./cct-host`

This distinction is part of the documented system design after FASE 31.
