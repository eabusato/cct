# CCT Freestanding + `grub-hello` Integration

## Integration Target

The bootable consumer for the current freestanding track is the sibling repository:

```text
../grub-hello/
```

It is treated as a legitimate integration surface for:

- GRUB ISO creation
- Multiboot entry
- Multiboot memory-map handoff
- heap initialization for the freestanding runtime
- freestanding object linking
- runtime-visible VGA output
- interrupt, keyboard, and timer device bring-up
- interactive shell validation on the booted system

## Build Flow

The integrated `../grub-hello/Makefile` performs this pipeline:

1. compile `src/boot.s`, `src/gdt_flush.s`, `src/isr_stubs.s`, `src/kernel.c`, `src/terminal.c`, `src/gdt.c`, `src/pic.c`, `src/idt.c`, `src/keyboard.c`, and `src/timer.c`
2. compile `cct/src/runtime/cct_freestanding_rt.c`
3. copy `src/civitas_boot_fase3.cct` into `build/civitas_boot.cct`
4. run `cct-host --profile freestanding --entry civitas_boot_fase3 --sigilo-no-svg build/civitas_boot.cct`
5. compile `build/civitas_boot.cgen.c` into `build/civitas_boot.o`
6. link all freestanding objects into `isodir/boot/kernel.bin`
7. produce `grub-hello.iso` with `grub-mkrescue`

Key generated artifacts:

- `../grub-hello/build/civitas_boot.cgen.c`
- `../grub-hello/build/civitas_boot.o`
- `../grub-hello/build/cct_freestanding_rt.o`
- `../grub-hello/isodir/boot/kernel.bin`
- `../grub-hello/grub-hello.iso`

The compatibility artifact names remain stable even though the boot source is now `civitas_boot_fase3.cct`:

- `build/civitas_boot.cgen.c`
- `build/civitas_boot.o`

This avoids breaking the existing freestanding runner while allowing the active boot demo to evolve by phase.

## ABI and Symbol Contract

The C kernel consumes:

- `civitas_boot_fase3`

`src/kernel.c` declares it as `extern void civitas_boot_fase3(void);` and invokes it after the initial C-only banner, heap initialization, and low-level interrupt-device setup.

The generated CCT object still exports the compatibility support symbols from earlier phases, but the concrete phase gate is driven by `civitas_boot_fase3()`.

## Multiboot and Heap Requirement

`src/boot.s` must keep the Multiboot header in an allocatable section:

- `.section .multiboot, "a"`

`src/boot.s` also preserves the Multiboot info pointer and passes `%ebx` through to `kernel_main`.

This prevents GRUB from missing the header because of a non-loadable section placement.

The runner validates this with:

```bash
i686-elf-grub-file --is-x86-multiboot ../grub-hello/isodir/boot/kernel.bin
```

`src/kernel.c` then:

- reads the memory map from the Multiboot structure
- skips memory below 1 MiB and below `_kernel_end`
- chooses a heap span for the bump allocator
- calls `cct_svc_heap_init(base, size)`
- initializes `gdt`, `pic`, `idt`, `keyboard`, and `timer`
- enters `civitas_boot_fase3()`

## Boot-Screen Contract

The boot flow now has two visible CCT stages:

- rows `0-2`: C banner from `terminal.c`
- row `3`: blank transition line
- rows `4-24`: FS-2 CCT screen from `civitas_boot_fase2()`
- after a short timer-backed delay: screen clear, FS-3 shell banner, and interactive prompt from `civitas_boot_fase3()`

Operational choices that matter:

- the CCT entry path does not call `console_init()`, preserving the C banner
- the FS-2 screen uses `console_set_cursor(4, 0)` to start below the C banner
- the FS-3 transition clears the screen intentionally before opening the shell
- `shell_fs_init()` flushes the keyboard buffer and enables interrupts before the interactive loop

## Automated Boot Validation

`tests/run_tests.sh` boots the ISO in QEMU with the monitor on stdio, executes:

```text
pmemsave 0xb8000 4000 <relative-path>
```

and then decodes the VGA text buffer to assert the boot screen.

Expected visible markers for the current gate:

- `=== CCT OS Lab ===`
- `Transferindo para runtime CCT...`
- `CCT FREESTANDING SHELL`
- `Type 'help' for commands.`
- `cct>`

The FS-3 runner also drives the QEMU monitor and injects keyboard input to validate command execution. The interaction capture is decoded from VGA memory and currently proves:

- `help` lists the delivered shell commands
- `echo ok` echoes typed payload
- `hist` shows retained command history
- backspace editing changes the final command line before dispatch
