# Ars Magna Omniversal (Modular CCT Example)

This is a multi-module showcase designed to exercise a broad set of currently supported CCT features.

Status note:
- Valid on the current project baseline (`FASE 15D.4 completed`).

## Features Used

- Multi-file closure with `ADVOCARE`
- Visibility boundary with `ARCANUM`
- `SIGILLUM`, nested fields, and `ORDO`
- `PACTUM` conformance (`Numerabilis`) and constrained generics (`GENUS(T PACTUM C)`)
- Generic rituals and generic `SIGILLUM` instantiation
- `SI/ALITER`, `DUM`, `DONEC`, `REPETE`
- Static arrays (`SERIES`) and pointer-indexed dynamic memory (`SPECULUM`, `OBSECRO pete/libera`, `DIMITTE`, `MENSURA`)
- Failure-control flow (`TEMPTA`, `CAPE`, `SEMPER`, `IACE`) including propagated failure and bridge path
- Mixed `OBSECRO scribe(...)` output (string/int/real/bool)

## Entry Point

- `main.cct` imports `modules/engine.cct`
- `engine.cct` orchestrates all other modules and returns an integer exit code

## Run

```bash
./cct examples/ars_magna_omniversal/main.cct
./examples/ars_magna_omniversal/main
```

## Sigilo

```bash
./cct --sigilo-only --sigilo-mode essencial examples/ars_magna_omniversal/main.cct
./cct --sigilo-only --sigilo-mode completo examples/ars_magna_omniversal/main.cct
```
