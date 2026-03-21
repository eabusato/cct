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
