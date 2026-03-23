# CCT Installation Guide

## Scope

This guide defines the supported installation and validation paths for the current CCT toolchain, including the historical host-compiler installation flow and the post-bootstrap validation targets available through FASE 30.

## Prerequisites

- POSIX shell environment
- C compiler (`gcc` or `clang`)
- `make`

Platform note:
- Linux and macOS are the supported validation targets for the current toolchain.
- Native Windows usage is not fully addressed and should be treated as experimental.
- Known Windows issues remain in shell/process behavior, path semantics, packaging, and some runtime-backed workflows.
- If you need a stable installation path, use Linux, macOS, or WSL2 rather than native Windows.

## 1. Build

```bash
make
```

Expected result: `./cct` binary available in project root.

## 2. Run Full Regression (Recommended)

```bash
make test
```

Expected result: all tests pass.

## 3. Build Relocatable Distribution Bundle

```bash
make dist
```

Expected layout:

```text
dist/cct/
├── bin/
│   ├── cct
│   └── cct.bin
├── lib/
│   └── cct/
├── docs/
└── examples/
```

`dist/cct/bin/cct` is a wrapper that sets `CCT_STDLIB_DIR` to the bundle-local stdlib path by default.

## 4. Smoke Test the Bundle

```bash
./dist/cct/bin/cct --version
./dist/cct/bin/cct --check tests/integration/stdlib_resolution_basic_11a.cct
./dist/cct/bin/cct tests/integration/showcase_string_11g.cct && ./tests/integration/showcase_string_11g
```

## 5. System Installation (Optional)

Install under default prefix (`/usr/local`):

```bash
make install
```

Install under custom prefix:

```bash
make install PREFIX="$HOME/.local"
```

This installs:
- wrapper: `<prefix>/bin/cct`
- binary: `<prefix>/bin/cct.bin`
- stdlib: `<prefix>/lib/cct/`

## 6. Uninstall

```bash
make uninstall
```

Or with custom prefix:

```bash
make uninstall PREFIX="$HOME/.local"
```

## 7. Environment Override

If needed, stdlib resolution can be overridden explicitly:

```bash
export CCT_STDLIB_DIR=/path/to/lib/cct
./cct --check file.cct
```

Resolver order:
1. `CCT_STDLIB_DIR` environment variable (if set)
2. build-time canonical stdlib path

## 8. Troubleshooting

- `ADVOCARE target not found in Bibliotheca Canonica`:
  - check `CCT_STDLIB_DIR`
  - verify `<stdlib>/cct/<module>.cct` exists
- `Input file must have .cct extension`:
  - ensure positional file ends with `.cct`
- packaging smoke fails:
  - rebuild with `make` then `make dist`
- Windows-specific failures:
  - do not assume parity with Linux/macOS
  - prefer WSL2 first
  - if native Windows is required, expect unsupported gaps and partial failures in some workflows

## 9. Bootstrap and Self-Host Validation Targets

Additional repository-level validation targets now exist beyond the historical baseline:

```bash
make test-legacy-full
make test-legacy-rebased
make test-all-0-30
make test-bootstrap
make test-bootstrap-selfhost
make test-phase30-final
```

For publication or release validation, use:

```bash
make test-all-0-30
```

## 10. Multi-Stage Self-Host Build Targets

The repository also exposes explicit self-host stages:

```bash
make bootstrap-stage0
make bootstrap-stage1
make bootstrap-stage2
make bootstrap-stage-identity
```

These targets are part of the supported operational toolchain as of FASE 29 and FASE 30.

## 11. FASE 31 First-Run Verification

After a fresh build, verify the wrapper state explicitly:

```bash
./cct --which-compiler
./cct-host --which-compiler
./cct-selfhost --which-compiler
```

Current repository baseline:
- `./cct` is the default user-facing compiler entrypoint
- `./cct-host` is the explicit host fallback
- `./cct-selfhost` is the explicit self-hosted entrypoint

If you want to force the default wrapper mode explicitly after install:

```bash
make bootstrap-promote
# or
make bootstrap-demote
```

## 12. Bundle and Installed Wrapper Roles

In bundled or installed layouts, the user should still call `cct` first. FASE 31 changes the compiler mode behind that wrapper, not the top-level command name users are expected to memorize.

Wrapper roles in installed layouts:
- `cct`: default operational wrapper
- `cct-host`: explicit host path where installed or packaged
- `cct-selfhost`: explicit self-host path where installed or packaged

## 13. Recommended Validation After Installation

Daily validation after installation:

```bash
./cct --which-compiler
make bootstrap-stage-identity
make test
```

Release-oriented validation after installation:

```bash
make test-host-legacy
make test-all-0-31
make test-phase30-final
make test-phase31-final
```
