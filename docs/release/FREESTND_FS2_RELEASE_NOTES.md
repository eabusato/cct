# Freestanding FS-2 Release Notes

## Summary

FS-2 closes the first dynamic-runtime freestanding delivery block on the CCT side and on the `../grub-hello/` boot integration side.

Gate closed:

- `G-FS2`: the booted system screen now shows heap-backed `SIGILLUM` data, dynamic `VERBUM` values, `fluxus_fs` collections, and live heap reporting

## Highlights

### FS-2A

- `cct/mem_fs` delivered as the canonical freestanding heap module
- bump allocator initialized from the Multiboot memory map in `../grub-hello/src/kernel.c`
- freestanding runtime shim extended with `cct_svc_heap_*`, `cct_svc_alloc`, `cct_svc_realloc`, and memory-copy helpers

### FS-2B

- `cct/verbum_fs` delivered with freestanding string slicing, duplication, comparison, and concatenation
- heap-backed builder service added through `cct_svc_builder_*`
- numeric formatting helpers (`from_uint`, `from_int`, `from_hex`) delivered without libc dependencies

### FS-2C

- `cct/fluxus_fs` delivered as the canonical freestanding dynamic-array module
- runtime shim extended with `cct_svc_fluxus_*`
- `../grub-hello/src/civitas_boot_fase2.cct` now renders the `G-FS2` product catalog and heap report on the booted VGA screen

## Compatibility Notes

- default host compiler behavior remains unchanged
- `cct/console`, `cct/mem_fs`, `cct/verbum_fs`, and `cct/fluxus_fs` remain freestanding-only by contract
- the current allocator remains bump-only; `free()` is a compatibility no-op
- raw `REX` elements stored in `fluxus_fs` currently require `elem_size = 8` under the generated C ABI
- the `../grub-hello` build keeps compatibility artifact names (`build/civitas_boot.*`) while compiling `src/civitas_boot_fase2.cct`

## Verification

- filtered block: `CCT_TEST_PHASES=FS2A,FS2B,FS2C bash tests/run_tests.sh`
- boot integration: `cd ../grub-hello && make iso`
- full suite: `bash tests/run_tests.sh`

## References

- `docs/freestanding/README.md`
- `docs/freestanding/FS2_RUNTIME_MANUAL.md`
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
- `../grub-hello/README.md`
