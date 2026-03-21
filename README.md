# CCT — Clavicula Turing

> "To name is to invoke. To invoke is to bind. To bind is to compute."

CCT is a compiled, ritual-themed programming language with a deterministic sigil system, a C-hosted production backend, and a fully validated self-hosting bootstrap pipeline.

<div align="center">
  <img src="docs/example_sigil_ars_magna.svg" alt="CCT system sigil example" width="600"/>
  <p><em>Deterministic system sigil generated from a multi-module CCT program.</em></p>
</div>

## Status

Current status: FASE 30 completed.

Implemented scope:
- Phases **0 through 30**
- Bootstrap compiler stack completed through lexer, parser, semantic analysis, code generation, multi-stage self-hosting, and operational self-hosted workflows
- Full aggregated validation available from **phase 0 to phase 30**

Current platform highlights:
- End-to-end compiler pipeline: `.cct -> parse -> semantic -> codegen -> .cgen.c -> host C compiler -> executable`
- Deterministic sigil generation (`.svg` + `.sigil`)
- Multi-module compilation with `ADVOCARE`, visibility control, and module closure
- Stable standard-library foundation under `lib/cct/`
- Bootstrap implementation in CCT under `src/bootstrap/`
- Stage0 / stage1 / stage2 self-host convergence
- Operational self-hosted workflows for building, running, testing, packaging, and validating projects

## Build

Requirements:
- POSIX shell
- `make`
- `gcc` or `clang`

Build the host compiler:

```bash
make
```

## Test Matrix

Run the current default runner:

```bash
make test
```

Run the restored historical legacy suite (phases 0-20, frozen reference):

```bash
make test-legacy-full
```

Run the rebased legacy suite against the current compiler:

```bash
make test-legacy-rebased
```

Run the complete aggregated validation from phase 0 through phase 30:

```bash
make test-all-0-30
```

Focused runners:

```bash
make test-bootstrap
make test-bootstrap-selfhost
make test-phase30-final
```

## Bootstrap and Self-Hosting

Important locations:
- `src/bootstrap/lexer/`
- `src/bootstrap/parser/`
- `src/bootstrap/semantic/`
- `src/bootstrap/codegen/`
- `out/bootstrap/phase29/`
- `out/bootstrap/phase30/`

Useful targets:

```bash
make bootstrap-stage0
make bootstrap-stage1
make bootstrap-stage2
make bootstrap-stage-identity
make bootstrap-selfhost-ready
make project-selfhost-build PROJECT=examples/phase30_data_app
```

## Documentation Map

Core documents:
- [Installation](docs/install.md)
- [Build System](docs/build_system.md)
- [Architecture](docs/architecture.md)
- [Language Specification](docs/spec.md)
- [Testing and Validation](docs/testing.md)
- [Self-Hosting Pipeline](docs/self_hosting.md)
- [Roadmap](docs/roadmap.md)

Release and handoff artifacts:
- `docs/release/`
- `docs/bootstrap/`

Implementation phase prompts:
- `md_out/`

## Repository Layout

```text
src/        host compiler in C
src/bootstrap/  bootstrap compiler in CCT
lib/cct/    standard library modules
examples/   public examples and operational project fixtures
tests/      integration runners and validation tooling
docs/       project-facing documentation
md_out/     phase implementation prompts and archival planning docs
```

## Current Direction

The bootstrap is complete. The next engineering track is no longer “make it self-host once”, but “evolve the self-hosted compiler as the primary development toolchain while preserving determinism, compatibility gates, and release discipline”.
