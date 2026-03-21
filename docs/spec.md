# CCT Language Specification

## Scope

This specification documents the practical language and toolchain surface as validated through FASE 30.

CCT is a compiled language with:
- ritual-themed syntax
- deterministic sigilo generation
- module-based compilation
- advanced typing subset
- operational project workflows
- validated bootstrap and self-host pipeline

## Compiler Commands

Primary host compiler commands:
- `./cct <file.cct>`
- `./cct --tokens <file.cct>`
- `./cct --ast <file.cct>`
- `./cct --ast-composite <file.cct>`
- `./cct --check <file.cct>`
- `./cct --sigilo-only <file.cct>`
- `./cct build`
- `./cct run`
- `./cct test`
- `./cct bench`
- `./cct clean`
- `./cct doc`

Related repository build and bridge commands:
- `make lbos-bridge`
- `make test-all-0-30`

## Current Language Surface

Validated language families include:
- scalar values and arithmetic
- structured control flow
- module imports via `ADVOCARE`
- `SIGILLUM`, `PACTUM`, `ORDO`
- `GENUS` with validated semantic and codegen support in the bootstrap track
- `ELIGE` / `CASUS` / `ALIOQUIN`
- `FORMA`
- failure control with `TEMPTA` / `CAPE` / `SEMPER` / `IACE`
- iteration and collection-oriented language growth through the phase-19 subset

Historical milestone markers that still matter operationally:
- **FASE 16**: `--profile freestanding`, `--emit-asm`, explicit `--entry`, and `cct/kernel`
- **FASE 17**: `cct/verbum_scan`, `cct/verbum_builder`, `cct/variant`, `cct/env`, `cct/time`, and `cct/bytes`
- **FASE 18 Canonical Library Expansion**: Bibliotheca Canonica 18 expanded the standard library with broader canonical modules, including `cct/process`

## Backend Model

The official executable backend remains generated C plus a host C compiler.

Bootstrap and self-host flows are validated against the same architectural model.

## Self-Hosting Status

The bootstrap compiler stack is complete through:
- lexer
- parser
- semantic analysis
- code generation
- stage0/stage1/stage2 convergence
- operational self-host project workflows

## Validation Contract

The language surface is only considered supported when it is covered by:
- legacy compatibility validation where applicable
- host compiler tests
- bootstrap/self-host validation where applicable
- operational project gates for self-host features

## Practical Boundary

CCT should now be treated as an operational compiler platform, not only a staged experiment.

Future work should preserve:
- deterministic behavior
- multi-stage convergence
- compatibility across host and bootstrap paths
