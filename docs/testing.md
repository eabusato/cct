# CCT Testing and Validation Model

## Overview

CCT uses layered validation rather than a single flat runner.

This is deliberate. The project carries:
- historical validation from early phases
- rebased compatibility validation against the current compiler
- bootstrap validation
- self-host validation
- operational self-host validation

## Runners

### Historical Legacy Runner

File:
- `tests/run_tests_legacy_0_20.sh`

Purpose:
- frozen historical reference for phases 0 through 20
- documents original expectations at the time those phases were closed

### Rebased Legacy Runner

File:
- `tests/run_tests_legacy_0_20_rebased.sh`

Purpose:
- validates phases 0 through 20 against the current compiler behavior
- keeps old coverage while accepting intentional later compiler changes

### Modern Runner

File:
- `tests/run_tests.sh`

Purpose:
- validates bootstrap, self-host, and operational phases in focused groups

### Full Aggregated Runner

File:
- `tests/run_tests_all_0_30.sh`

Purpose:
- runs the rebased legacy runner
- runs modern bootstrap phases
- runs self-host phases
- runs operational phase-30 gates

## Recommended Commands

```bash
make test-legacy-full
make test-legacy-rebased
make test
make test-all-0-30
```

## Acceptance Rules

A platform-wide green baseline means:
- rebased legacy is green
- bootstrap is green
- self-host is green
- operational phase-30 is green

That is the practical definition of a healthy repository state.

## Post-FASE-31 Validation Model

FASE 31 changes the meaning of the main validation entrypoints.

### Default Product Validation

```bash
make test
```

This validates the promoted/default compiler path exposed through `./cct`.

### Host Fallback Validation

```bash
make test-host-legacy
```

This validates the preserved host fallback path through the rebased historical suite.

### Structural Bootstrap Validation

```bash
make bootstrap-stage-identity
```

This does not replace functional tests. It proves stage convergence and identity in the bootstrap chain.

### Whole-Project Aggregated Validation

```bash
make test-all-0-30
make test-all-0-31
```

Practical usage:
- `make test-all-0-30`: authoritative whole-project gate through the FASE 30 closure
- `make test-all-0-31`: extended aggregated gate including compiler-promotion validation

### Phase Gates

```bash
make test-phase30-final
make test-phase31-final
```

Use these when you need to certify the closed post-bootstrap platform layers specifically.

## Recommended Command Sequences

### Daily Work

```bash
make bootstrap-stage-identity
make test
make test-host-legacy
```

### Release Candidate Validation

```bash
make bootstrap-stage-identity
make test
make test-host-legacy
make test-all-0-31
make test-phase30-final
make test-phase31-final
```

### Debugging Host vs Self-Host Parity

```bash
./cct --which-compiler
./cct-host --which-compiler
./cct-selfhost --which-compiler
make bootstrap-stage-identity
make test
make test-host-legacy
make test-bootstrap-parity
```

## Operational Caution

Do not parallelize long bootstrap-generating suites with other bootstrap or promotion commands. The repository still contains shared generated artifacts in the bootstrap track, so overlapping long runs can produce false regressions that are not real compiler bugs.
