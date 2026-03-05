# Bibliotheca Canonica Stability Matrix (FASE 11H Freeze)

## Legend

- **Canonical Stable**: public API committed for production-like use in current subset
- **Canonical Experimental**: public API available but may receive controlled shape/naming adjustments in a future major phase
- **Runtime Internal**: not a public stdlib API; implementation support layer

## Stability Matrix

| Module / Surface | Stability | Notes | Freeze Phase |
|---|---|---|---|
| `cct/verbum` | Canonical Stable | Text core used across examples and tests | 11H |
| `cct/fmt` | Canonical Stable | Formatting + explicit `fmt_parse_*` façade | 11H |
| `cct/series` | Canonical Stable | Static collection helpers | 11H |
| `cct/fluxus` | Canonical Stable | Dynamic vector API over runtime storage core | 11H |
| `cct/mem` | Canonical Stable | Explicit ownership memory primitives | 11H |
| `cct/io` | Canonical Stable | Minimal IO façade (`print`, `println`, `print_int`, `read_line`) | 11H |
| `cct/fs` | Canonical Stable | Whole-file operations subset | 11H |
| `cct/path` | Canonical Stable | Minimal path helpers for text/file integration | 11H |
| `cct/math` | Canonical Stable | Deterministic numeric helper baseline | 11H |
| `cct/parse` | Canonical Stable | Strict primitive parsing | 11H |
| `cct/cmp` | Canonical Stable | Canonical comparator contract (`<0/0/>0`) | 11H |
| `cct/random` | Canonical Experimental | PRNG baseline is deterministic/tested, not cryptographic | 11H |
| `cct/alg` | Canonical Experimental | Moderate algorithm surface, expected controlled expansion in future | 11H |
| Runtime helper C APIs (`cct_rt_*`, `fluxus_runtime`, `mem_runtime`) | Runtime Internal | Not imported from `cct/...`; generated/runtime-only | 11H |
| Builtins (`OBSECRO ...`, `MENSURA`) | Runtime Internal (language intrinsic) | Language-level intrinsic layer, not stdlib module API | 11H |

## Naming Freeze Notes

- `cct/fmt` parse façade is frozen as `fmt_parse_int`, `fmt_parse_real`, `fmt_parse_bool`
- `cct/parse` remains the canonical generic parse namespace (`parse_int`, `parse_real`, `parse_bool`)
- Sigilo modes remain `essencial|completo` with aliases `essential|complete`
