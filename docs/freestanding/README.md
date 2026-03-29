# CCT Freestanding Track

This directory is the operational index for the bare-metal freestanding track built on top of the CCT freestanding profile and the `../grub-hello/` bootable integration.

## Current Status

- FS-1A complete: `cct/console` delivered as the canonical VGA text-mode module for `--profile freestanding`
- FS-1B complete: integrated pipeline from `cct-host --profile freestanding` to `../grub-hello/grub-hello.iso`
- FS-1C complete: first CCT ritual visible on the booted system screen
- FS-2A complete: `cct/mem_fs` delivered with a freestanding bump allocator initialized from the Multiboot memory map
- FS-2B complete: `cct/verbum_fs` delivered with heap-backed dynamic strings and a freestanding builder surface
- FS-2C complete: `cct/fluxus_fs` delivered with dynamic arrays and a boot-visible `G-FS2` catalog/memory demo
- Current concrete gate: `G-FS2` closed with an automated VGA dump check in `tests/run_tests.sh`

## Document Map

- `docs/freestanding/FS1_RUNTIME_MANUAL.md`
  Historical runtime, module, and codegen contracts for FS-1A through FS-1C.
- `docs/freestanding/FS2_RUNTIME_MANUAL.md`
  Runtime, module, heap, string, and collection contracts for FS-2A through FS-2C.
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
  Build, heap-init, link, and boot integration with `../grub-hello/`.
- `docs/release/FREESTND_FS1_RELEASE_NOTES.md`
  Release summary for the first freestanding delivery block.
- `docs/release/FREESTND_FS2_RELEASE_NOTES.md`
  Release summary for the freestanding memory/string/collection delivery block.
- `../grub-hello/README.md`
  Consumer-side notes for the bootable integration repository.

## Primary Commands

```bash
./cct-host --profile freestanding --check tests/integration/fluxus_fs_freestanding_smoke_fs2c.cct
CCT_TEST_PHASES=FS1A,FS1B,FS1C,FS2A,FS2B,FS2C bash tests/run_tests.sh
cd ../grub-hello && make clean && make iso
```

## Current Closure Criteria

- `cct/console` is rejected on host profile and accepted on freestanding profile.
- `cct/mem_fs`, `cct/verbum_fs`, and `cct/fluxus_fs` are rejected on host profile and accepted on freestanding profile.
- Freestanding codegen emits console, heap, string-builder, and dynamic-array service calls plus raw C ABI wrappers for exported `RITUALE`.
- `../grub-hello/Makefile` produces `build/civitas_boot.cgen.c`, `build/civitas_boot.o`, `isodir/boot/kernel.bin`, and `grub-hello.iso`.
- `kernel.bin` is recognized as Multiboot by GRUB.
- QEMU boot shows the C banner from `terminal.c` followed by the CCT `G-FS2` screen produced by `civitas_boot_fase2()`.
