# CCT Freestanding FS-3 Runtime Manual

## Scope

FS-3 closes the interrupt-driven freestanding runtime block:

- FS-3A: GDT, IDT, PIC remap, and `cct/irq_fs`
- FS-3B: PS/2 keyboard IRQ path and `cct/keyboard_fs`
- FS-3C: PIT timer services and `cct/timer_fs`
- FS-3D: interactive local shell in `cct/shell_fs`

The concrete closure gate is `G-FS3`: the booted system accepts keyboard commands in a shell written in CCT.

## Runtime Service Contract

The freestanding runtime shim in `src/runtime/cct_freestanding_rt.h` now exposes these FS-3 services:

- IRQ and PIC: `cct_svc_irq_enable`, `cct_svc_irq_disable`, `cct_svc_irq_flags`, `cct_svc_irq_mask`, `cct_svc_irq_unmask`, `cct_svc_irq_register`, `cct_svc_irq_unregister`
- machine control: `cct_svc_reboot`
- keyboard: `cct_svc_keyboard_getc`, `cct_svc_keyboard_poll`, `cct_svc_keyboard_available`, `cct_svc_keyboard_flush`, `cct_svc_keyboard_self_test`
- builder helpers used by keyboard line editing: `cct_svc_builder_new_raw`, `cct_svc_builder_backspace`
- timer: `cct_svc_timer_ms`, `cct_svc_timer_ticks`, `cct_svc_timer_sleep`
- collection helper used by shell history: `cct_svc_fluxus_remove_first`

These services remain freestanding-only and depend on the bare-metal symbols exported by `../grub-hello`.

## Canonical Modules

### `cct/irq_fs`

Provides the CCT-side bridge for interrupt control:

- enable and disable interrupts
- inspect current interrupt-flag state
- mask and unmask PIC IRQ lines
- register and unregister IRQ handlers by raw pointer
- perform panic output through VGA + halt
- request reboot through the runtime shim

`irq_fs_panic()` is deliberately terminal: it disables interrupts, clears the screen, prints a panic banner, and halts the kernel.

### `cct/keyboard_fs`

Provides the PS/2 keyboard access layer:

- blocking `keyboard_fs_getc()`
- non-blocking `keyboard_fs_poll()`
- queue-depth inspection through `keyboard_fs_available()`
- explicit buffer reset with `keyboard_fs_flush()`
- `keyboard_fs_self_test()`
- `keyboard_fs_read_line(max_len)` with echo and backspace support

Operational details:

- Enter accepts both ASCII `10` and `13`
- backspace updates both the builder state and the VGA cursor state
- only printable ASCII `32..126` is appended to the line buffer

### `cct/timer_fs`

Provides PIT-backed time helpers:

- `timer_fs_uptime_ms()`
- `timer_fs_ticks()`
- `timer_fs_sleep_ms(ms)`
- `timer_fs_uptime_sec()`
- `timer_fs_deadline_ms(delta_ms)`
- `timer_fs_elapsed_ms(started_ms)`
- `timer_fs_deadline_expired(deadline_ms)`
- `timer_fs_format_uptime()`
- `timer_fs_format_uptime_ms()`

Implementation note:

- formatted uptime intentionally avoids integer division helpers so the generated freestanding object does not require compiler runtime symbols such as `__divdi3`

### `cct/shell_fs`

Provides the first interactive local shell fully written in CCT.

Delivered commands:

- `help`
- `status`
- `uptime`
- `mem`
- `clear`
- `reboot`
- `echo <text>`
- `hist`

Operational contracts:

- `shell_fs_init()` flushes stale keyboard input and enables interrupts
- `shell_fs_run()` loops forever on prompt, line-read, trim, history append, and dispatch
- shell history is stored in `fluxus_fs` and capped at 8 entries
- when the history is full, the oldest command is removed through `fluxus_fs_remove_first()`

## Profile Gating

The following modules are rejected outside `--profile freestanding`:

- `cct/irq_fs`
- `cct/keyboard_fs`
- `cct/timer_fs`
- `cct/shell_fs`

Host-profile behavior for the preexisting freestanding modules remains unchanged.

## Codegen and Semantic Contracts

The compiler now recognizes and lowers the FS-3 service family in both semantic validation and C generation:

- IRQ/PIC builtins
- keyboard builtins
- raw builder helpers for line editing
- timer builtins
- `fluxus` front-removal helper used by shell history

This allows freestanding `.cgen.c` emission to reference the runtime shim directly while host-profile gating continues to reject the freestanding modules.

## Boot Integration Contract

The boot entry for the current freestanding block is `../grub-hello/src/civitas_boot_fase3.cct`.

Actual boot sequence:

1. `kernel_main()` prints the initial C banner
2. heap is initialized from the Multiboot memory map
3. `gdt_init()`, `pic_init()`, `idt_init()`, `keyboard_init()`, and `timer_init()` run in C
4. `civitas_boot_fase3()` executes the FS-2 boot screen
5. interrupts are enabled, a short timer delay runs, and the screen is cleared
6. the FS-3 shell banner is printed and the shell loop starts

The `../grub-hello/Makefile` compiles the CCT entry with:

```bash
../cct/cct-host --profile freestanding --entry civitas_boot_fase3 --sigilo-no-svg
```

The compatibility artifact names stay unchanged:

- `build/civitas_boot.cct`
- `build/civitas_boot.cgen.c`
- `build/civitas_boot.o`

## Validation

Filtered validation for this block:

```bash
CCT_TEST_PHASES=FS3A,FS3B,FS3C,FS3D bash tests/run_tests.sh
```

Boot integration:

```bash
cd ../grub-hello && make iso
```

Concrete gate evidence:

- booted QEMU screen shows `CCT FREESTANDING SHELL`
- prompt `cct>` is visible
- typed commands are echoed and executed through the PS/2 keyboard path
- `hist` shows prior commands from shell-managed history

## Limits

- keyboard decoding is still the current simple PS/2 set-1 path, focused on printable ASCII and backspace/enter handling
- shell history is fixed to 8 retained entries
- `reboot` depends on the current low-level reboot path exposed by `cct_svc_reboot`
