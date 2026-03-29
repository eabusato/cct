# Freestanding FS-3 Release Notes

## Summary

FS-3 closes the first interrupt-driven interactive block of the CCT freestanding track.

Gate closed:

- `G-FS3`: the booted system now exposes a local shell written in CCT and accepts commands from the PS/2 keyboard

## Highlights

### FS-3A

- flat protected-mode GDT and full IDT integration landed in `../grub-hello`
- PIC remap and IRQ registration hooks are now available to the freestanding runtime
- `cct/irq_fs` delivered as the canonical CCT bridge for interrupt control and reboot/panic operations

### FS-3B

- PS/2 keyboard driver with IRQ-backed ring buffer added in `../grub-hello`
- `cct/keyboard_fs` delivered with blocking read, polling, flush, and line-editing helpers
- raw builder support added to the runtime so backspace-aware line editing stays inside the freestanding profile

### FS-3C

- PIT timer driver added in `../grub-hello`
- `cct/timer_fs` delivered with uptime, tick, sleep, deadline, and formatting helpers
- freestanding uptime formatting avoids division-runtime dependencies during bare-metal linking

### FS-3D

- `cct/shell_fs` delivered as the first interactive shell fully written in CCT
- shell commands include `help`, `status`, `uptime`, `mem`, `clear`, `reboot`, `echo`, and `hist`
- `../grub-hello/src/civitas_boot_fase3.cct` now transitions from the FS-2 demo screen into the interactive shell

## Compatibility Notes

- host-profile behavior remains unchanged; `cct/irq_fs`, `cct/keyboard_fs`, `cct/timer_fs`, and `cct/shell_fs` are freestanding-only by contract
- `../grub-hello` still keeps compatibility artifact names under `build/civitas_boot.*`
- the freestanding integration now compiles with `--entry civitas_boot_fase3` so generated code is emitted for bare-metal object linking instead of host executable linking

## Verification

- filtered block: `CCT_TEST_PHASES=FS3A,FS3B,FS3C,FS3D bash tests/run_tests.sh`
- boot integration: `cd ../grub-hello && make iso`
- full suite: `bash tests/run_tests.sh`

## References

- `docs/freestanding/README.md`
- `docs/freestanding/FS3_RUNTIME_MANUAL.md`
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
- `../grub-hello/README.md`
