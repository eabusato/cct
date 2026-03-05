# Bibliotheca Canonica Final Subset (FASE 11H)

## Status

This document freezes the official FASE 11 stdlib subset.

## Reserved Namespace and Import Contract

- Canonical imports use `ADVOCARE "cct/..."`
- `cct/...` is reserved for Bibliotheca Canonica
- Resolver uses `CCT_STDLIB_DIR` when defined, otherwise the build-time canonical stdlib root

## Final Module Inventory

### Text and Formatting
- `cct/verbum`:
  - `len`, `concat`, `compare`, `substring`, `trim`, `contains`, `find`
- `cct/fmt`:
  - `stringify_int`, `stringify_real`, `stringify_float`, `stringify_bool`
  - `fmt_parse_int`, `fmt_parse_real`, `fmt_parse_bool`
  - `format_pair`

### Collections and Memory
- `cct/series`:
  - `series_len`, `series_fill`, `series_copy`, `series_reverse`, `series_contains`
- `cct/alg`:
  - `alg_linear_search`, `alg_compare_arrays`, `alg_binary_search`, `alg_sort_insertion`
- `cct/mem`:
  - `alloc`, `free`, `realloc`, `copy`, `set`, `zero`, `mem_compare`
- `cct/fluxus`:
  - `fluxus_init`, `fluxus_free`, `fluxus_push`, `fluxus_pop`, `fluxus_len`, `fluxus_get`, `fluxus_clear`, `fluxus_reserve`, `fluxus_capacity`

### IO / File / Path
- `cct/io`:
  - `print`, `println`, `print_int`, `read_line`
- `cct/fs`:
  - `read_all`, `write_all`, `append_all`, `exists`, `size`
- `cct/path`:
  - `path_join`, `path_basename`, `path_dirname`, `path_ext`

### Utilities
- `cct/math`:
  - `abs`, `min`, `max`, `clamp`
- `cct/random`:
  - `seed`, `random_int`, `random_real`
- `cct/parse`:
  - `parse_int`, `parse_real`, `parse_bool`
- `cct/cmp`:
  - `cmp_int`, `cmp_real`, `cmp_bool`, `cmp_verbum`

## Final Policy Summary

### Error Policy
- Strict APIs fail explicitly (runtime failure or `FRACTUM`-compatible failure flows where applicable)
- Parse/validation APIs reject invalid input clearly
- Silent failure is not an accepted default policy

### Import Policy
- `cct/...` namespace cannot be shadowed by user-local modules
- Direct-import visibility rules remain in force
- `--ast`, `--ast-composite`, and `--check` remain compatible with stdlib modules

### Memory / Ownership Policy
- Ownership remains explicit in memory-returning APIs
- Caller-side release responsibilities are documented for allocated resources
- No implicit ownership transfer semantics are introduced in this phase

## Out of Scope in Final FASE 11 Subset

- independent stdlib package manager/version graph
- advanced statistics/crypto libraries
- broad OS abstraction layer
- stdlib import side effects as default behavior
