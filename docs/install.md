# CCT Installation Guide

## Scope

This guide defines the supported installation paths for the CCT compiler and Bibliotheca Canonica in the FASE 11 final subset.

## Prerequisites

- POSIX shell environment
- C compiler (`gcc` or `clang`)
- `make`

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
