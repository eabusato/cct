# CCT Freestanding Track

This directory is the operational index for the bare-metal freestanding track built on top of the CCT freestanding profile and the `../grub-hello/` bootable integration.

## Current Status

- FS-1A complete: `cct/console` delivered as the canonical VGA text-mode module for `--profile freestanding`
- FS-1B complete: integrated pipeline from `cct-host --profile freestanding` to `../grub-hello/grub-hello.iso`
- FS-1C complete: first CCT ritual visible on the booted system screen
- FS-2A complete: `cct/mem_fs` delivered with a freestanding bump allocator initialized from the Multiboot memory map
- FS-2B complete: `cct/verbum_fs` delivered with heap-backed dynamic strings and a freestanding builder surface
- FS-2C complete: `cct/fluxus_fs` delivered with dynamic arrays and a boot-visible `G-FS2` catalog/memory demo
- FS-3A complete: interrupt foundations (`GDT`, `IDT`, `PIC`) plus `cct/irq_fs`
- FS-3B complete: PS/2 keyboard input plus `cct/keyboard_fs`
- FS-3C complete: PIT uptime/sleep services plus `cct/timer_fs`
- FS-3D complete: local interactive shell plus `cct/shell_fs`
- Current concrete gate: `G-FS3` closed with an automated boot interaction check in `tests/run_tests.sh`

## Document Map

- `docs/freestanding/FS1_RUNTIME_MANUAL.md`
  Historical runtime, module, and codegen contracts for FS-1A through FS-1C.
- `docs/freestanding/FS2_RUNTIME_MANUAL.md`
  Runtime, module, heap, string, and collection contracts for FS-2A through FS-2C.
- `docs/freestanding/FS3_RUNTIME_MANUAL.md`
  Runtime, interrupt, keyboard, timer, and shell contracts for FS-3A through FS-3D.
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
  Build, heap-init, link, and boot integration with `../grub-hello/`.
- `docs/release/FREESTND_FS1_RELEASE_NOTES.md`
  Release summary for the first freestanding delivery block.
- `docs/release/FREESTND_FS2_RELEASE_NOTES.md`
  Release summary for the freestanding memory/string/collection delivery block.
- `docs/release/FREESTND_FS3_RELEASE_NOTES.md`
  Release summary for the freestanding interrupt/keyboard/timer/shell delivery block.
- `../grub-hello/README.md`
  Consumer-side notes for the bootable integration repository.

## Primary Commands

```bash
./cct-host --profile freestanding --check tests/integration/shell_fs_freestanding_smoke_fs3d.cct
CCT_TEST_PHASES=FS1A,FS1B,FS1C,FS2A,FS2B,FS2C,FS3A,FS3B,FS3C,FS3D bash tests/run_tests.sh
cd ../grub-hello && make clean && make iso
```

## Current Closure Criteria

- `cct/console` is rejected on host profile and accepted on freestanding profile.
- `cct/mem_fs`, `cct/verbum_fs`, `cct/fluxus_fs`, `cct/irq_fs`, `cct/keyboard_fs`, `cct/timer_fs`, and `cct/shell_fs` are rejected on host profile and accepted on freestanding profile.
- Freestanding codegen emits console, heap, string-builder, dynamic-array, IRQ, keyboard, timer, and shell-support service calls plus raw C ABI wrappers for exported `RITUALE`.
- `../grub-hello/Makefile` produces `build/civitas_boot.cgen.c`, `build/civitas_boot.o`, `isodir/boot/kernel.bin`, and `grub-hello.iso`.
- `kernel.bin` is recognized as Multiboot by GRUB.
- QEMU boot shows the C banner from `terminal.c`, transitions through the FS-2 CCT screen, and then reaches the interactive `G-FS3` shell produced by `civitas_boot_fase3()`.
