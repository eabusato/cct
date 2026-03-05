# FASE 14 Compatibility Matrix

## Platform Surface

- Linux x86_64: validated in current CI/local baseline.
- Other targets: compatibility inherited from existing project support model, subject to RC validation.

## Contract Compatibility

- Existing public CLI command surface remains backward-compatible.
- Exit-code semantics are now explicitly canonicalized (`0/1/2/3/4`).
- Sigilo operational commands retain previous defaults; `--explain` is opt-in only.
- Historical phase-13/13M regression contracts remain mandatory and green.
