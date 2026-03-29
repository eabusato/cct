# FS-2 Runtime Manual

## Scope

FS-2 closes the first dynamic freestanding runtime slice on top of the existing FS-1 boot path:

- FS-2A: heap initialization plus `cct/mem_fs`
- FS-2B: dynamic strings plus `cct/verbum_fs`
- FS-2C: dynamic arrays plus `cct/fluxus_fs`

The operational gate is `G-FS2`: a booted bare-metal screen showing heap-backed `SIGILLUM` data, dynamically built `VERBUM` fields, and `fluxus_fs` collections.

## Freestanding Modules

### `cct/mem_fs`

`lib/cct/mem_fs/mem_fs.cct` is the canonical FS-2A heap surface.

Available rituals:

- `mem_fs_alloc(REX)`
- `mem_fs_alloc_zero(REX)`
- `mem_fs_realloc(SPECULUM NIHIL, REX, REX)`
- `mem_fs_free(SPECULUM NIHIL)`
- `mem_fs_copy(SPECULUM NIHIL, SPECULUM NIHIL, REX)`
- `mem_fs_set(SPECULUM NIHIL, REX, REX)`
- `mem_fs_zero(SPECULUM NIHIL, REX)`
- `mem_fs_compare(SPECULUM NIHIL, SPECULUM NIHIL, REX)`
- `mem_fs_available()`
- `mem_fs_allocated()`
- `mem_fs_stats()`

`HeapStats` fields:

- `base_addr`
- `total_bytes`
- `allocated_bytes`
- `available_bytes`
- `alloc_count`

### `cct/verbum_fs`

`lib/cct/verbum_fs/verbum_fs.cct` is the canonical FS-2B string surface.

Available rituals:

- `verbum_fs_len(VERBUM)`
- `verbum_fs_char_at(VERBUM, REX)`
- `verbum_fs_compare(VERBUM, VERBUM)`
- `verbum_fs_equals(VERBUM, VERBUM)`
- `verbum_fs_substring(VERBUM, REX, REX)`
- `verbum_fs_dup(VERBUM)`
- `verbum_fs_concat(VERBUM, VERBUM)`
- `verbum_fs_find(VERBUM, VERBUM)`
- `verbum_fs_starts_with(VERBUM, VERBUM)`
- `verbum_fs_ends_with(VERBUM, VERBUM)`
- `verbum_fs_contains(VERBUM, VERBUM)`
- `verbum_fs_from_uint(REX)`
- `verbum_fs_from_int(REX)`
- `verbum_fs_from_hex(REX)`
- `verbum_fs_builder_new()`
- `verbum_fs_builder_append(SPECULUM NIHIL, VERBUM)`
- `verbum_fs_builder_append_char(SPECULUM NIHIL, REX)`
- `verbum_fs_builder_append_int(SPECULUM NIHIL, REX)`
- `verbum_fs_builder_build(SPECULUM NIHIL)`
- `verbum_fs_builder_len(SPECULUM NIHIL)`
- `verbum_fs_builder_clear(SPECULUM NIHIL)`

### `cct/fluxus_fs`

`lib/cct/fluxus_fs/fluxus_fs.cct` is the canonical FS-2C collection surface.

Available rituals:

- `fluxus_fs_new(REX elem_size)`
- `fluxus_fs_free(SPECULUM NIHIL)`
- `fluxus_fs_push(SPECULUM NIHIL, SPECULUM NIHIL)`
- `fluxus_fs_pop(SPECULUM NIHIL, SPECULUM NIHIL)`
- `fluxus_fs_len(SPECULUM NIHIL)`
- `fluxus_fs_get(SPECULUM NIHIL, REX)`
- `fluxus_fs_set(SPECULUM NIHIL, REX, SPECULUM NIHIL)`
- `fluxus_fs_clear(SPECULUM NIHIL)`
- `fluxus_fs_reserve(SPECULUM NIHIL, REX)`
- `fluxus_fs_cap(SPECULUM NIHIL)`
- `fluxus_fs_is_empty(SPECULUM NIHIL)`
- `fluxus_fs_peek(SPECULUM NIHIL)`

Operational contract:

- elements are stored by value, not by pointer identity
- capacity grows from `8` and doubles on demand
- `clear()` keeps the allocated capacity
- `free()` is a no-op under the current bump allocator

## Profile Gating

These modules are freestanding-only:

- `cct/console`
- `cct/mem_fs`
- `cct/verbum_fs`
- `cct/fluxus_fs`

Host-profile imports are rejected during module resolution.
Freestanding-only builtin services are also rejected semantically on host profile with the generic `cct_svc_*` diagnostic.

## Runtime Service Layer

`src/runtime/cct_freestanding_rt.h` now carries three freestanding service families on top of the FS-1 console layer:

- heap: `cct_svc_heap_init`, `cct_svc_alloc`, `cct_svc_realloc`, `cct_svc_free`, `cct_svc_heap_*`, `cct_svc_byte_at`
- dynamic string: `cct_svc_verbum_*`, `cct_svc_builder_*`
- dynamic array: `cct_svc_fluxus_*`

Internal runtime structures exposed to generated C:

- `cct_bump_heap_t`
- `cct_verbum_builder_t`
- `cct_fluxus_fs_t`

## Heap Initialization Contract

Heap readiness is no longer implicit in FS-2.

Boot integration responsibilities:

- `src/boot.s` passes the Multiboot info pointer in `%ebx` through to `kernel_main`
- `../grub-hello/src/kernel.c` parses the Multiboot memory map
- the kernel selects a heap region above `_kernel_end` and above the 1 MiB floor
- `cct_svc_heap_init(base, size)` is called before `civitas_boot_fase2()`

Allocator policy in this block:

- bump allocator only
- 8-byte alignment
- no reclamation
- `realloc` copies into newly allocated storage
- `free` is currently a compatibility no-op

## Codegen Contract

Freestanding C lowering now covers:

- `cct_svc_alloc`, `cct_svc_realloc`, `cct_svc_free`
- `cct_svc_memcpy`, `cct_svc_memset`, `cct_svc_byte_at`
- `cct_svc_heap_available`, `cct_svc_heap_allocated`, `cct_svc_heap_total`, `cct_svc_heap_base_addr`, `cct_svc_heap_alloc_count`
- `cct_svc_verbum_byte`, `cct_svc_verbum_len`, `cct_svc_verbum_copy_slice`
- `cct_svc_builder_new`, `cct_svc_builder_append`, `cct_svc_builder_append_char`, `cct_svc_builder_build`, `cct_svc_builder_clear`, `cct_svc_builder_len`
- `cct_svc_fluxus_new`, `cct_svc_fluxus_push`, `cct_svc_fluxus_pop`, `cct_svc_fluxus_get`, `cct_svc_fluxus_set`, `cct_svc_fluxus_clear`, `cct_svc_fluxus_reserve`, `cct_svc_fluxus_free`, `cct_svc_fluxus_len`, `cct_svc_fluxus_cap`, `cct_svc_fluxus_peek`

## ABI Notes

The current generated C ABI uses 64-bit storage for `REX` values even in the 32-bit freestanding target.
For raw `REX` payloads stored in `fluxus_fs`, callers must therefore use `elem_size = 8` unless they are intentionally packing a narrower representation.

For structured values, use `MENSURA(<SIGILLUM>)`.

## Boot Gate `G-FS2`

`../grub-hello/src/civitas_boot_fase2.cct` is the canonical demonstrator.

It proves, on the booted VGA screen, that the runtime can:

- allocate a `SIGILLUM Produto` on the freestanding heap
- build `VERBUM` fields dynamically
- populate a `fluxus_fs` of `REX`
- populate a `fluxus_fs` of `Produto`
- report live heap statistics and a usage bar

Observed visible markers:

- `CCT FREESTANDING RUNTIME`
- `FASE 2 - Memoria e Strings Freestanding`
- `[DEMO 1] SIGILLUM + VERBUM dinamico`
- `[DEMO 3] fluxus_fs de SIGILLUM Produto`
- `[DEMO 4] Relatorio de Memoria`
- `>>> G-FS2: GATE CONCLUIDO <<<`

## Validation Path

Filtered validation for the FS-2 block:

```bash
CCT_TEST_PHASES=FS2A,FS2B,FS2C bash tests/run_tests.sh
```

Concrete boot validation:

```bash
cd ../grub-hello && make iso
```

The main runner then reboots the ISO in QEMU, dumps the VGA buffer, and asserts the visible `G-FS2` markers.
