# FASE 18 Release Notes

## Summary

FASE 18 consolidates and expands Bibliotheca Canonica into a broad, practical host-oriented standard library layer.
The phase closes at **18D.4** with major text/format/parse growth, IO/FS/path expansion, richer collections/algorithms, and new low-level utility modules.

## Highlights

### 18A — Text, Formatting, and Parsing Expansion

- `cct/verbum`: major API growth (search/replace, case/trim/pad/slice/reverse, split/join, lines/words).
- `cct/fmt`: radix/precision formatting, template helpers (`format_1..4`), and table helpers.
- `cct/parse`: safe parsing surface (`try_*`) plus radix and CSV helpers.

### 18B — Host IO/FS/Path Expansion

- `cct/fs`: mutation, inspection, listing, temp resources, and metadata helpers.
- `cct/io`: stderr variants, flush controls, stdin aggregate reads, tty detection.
- `cct/path`: normalize/resolve/relative, stem/ext transforms, and split utilities.

### 18C — Collections and Algorithms Expansion

- `cct/fluxus`: peek/set/remove/insert/slice/copy/reverse/sort utilities.
- `cct/set` and `cct/map`: richer set algebra and map convenience APIs.
- `cct/alg` and `cct/series`: reductions, min/max, sorting, rotate/reverse, counting, and related helpers.

### 18D — New Modules and Closure

- New module: `cct/process` (run/capture/input/env/timeout host process helpers).
- New module: `cct/hash` (djb2/fnv1a/crc32/murmur3/combine).
- New module: `cct/bit` (bit access/manipulation, rotate/swap/power-of-two utilities).
- `cct/random` expanded with bool/range/string/bytes/shuffle helpers.
- Documentation and closure artifacts finalized at 18D.4.

## Compatibility Notes

- No breaking changes to existing core language contracts.
- FASE 18 focuses on library-surface growth and keeps host profile as primary target for new operational modules.
- `cct/process` remains host-oriented and out of freestanding scope in this phase.

## Verification Status

- FASE 18 closure gate executed on full suite.
- Final phase gate: **1014 passed / 0 failed** on `make test`.

## References

- `docs/bootstrap/FASE_18_HANDOFF.md`
- `md_out/FASE_18_CCT.md`
- `docs/spec.md`
- `docs/bibliotheca_canonica.md`
