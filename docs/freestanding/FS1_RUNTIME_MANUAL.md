# FS-1 Runtime Manual

## Scope

FS-1 covers the first functional bare-metal slice on top of the existing freestanding compiler foundation:

- FS-1A: console runtime and canonical module
- FS-1B: integrated build pipeline with `../grub-hello/`
- FS-1C: first boot-visible CCT ritual

## Canonical Module: `cct/console`

`lib/cct/console/console.cct` is the canonical FS-1 console surface.

Available rituals:

- `console_init()`
- `console_clear()`
- `console_putc(REX)`
- `console_write(VERBUM)`
- `console_write_line(VERBUM)`
- `console_write_centered(VERBUM)`
- `console_set_color(ConsoleColor fg, ConsoleColor bg)`
- `console_set_cursor(REX row, REX col)`
- `console_get_linha()`
- `console_get_coluna()`

Available color payload:

- `COR_PRETO` through `COR_BRANCO`

Profile gate:

- accepted only with `--profile freestanding`
- rejected on host profile with `cct/console disponível apenas em perfil freestanding`

## Runtime Services

`src/runtime/cct_freestanding_rt.h` now provides the VGA-backed console service layer:

- `cct_svc_console_clear`
- `cct_svc_console_putc`
- `cct_svc_console_write`
- `cct_svc_console_write_centered`
- `cct_svc_console_set_attr`
- `cct_svc_console_init`
- `cct_svc_console_set_cursor`
- `cct_svc_console_get_row`
- `cct_svc_console_get_col`

Operational details:

- VGA text mode is fixed at `80x25` over `0xB8000`
- cursor updates use CRT ports `0x3D4` and `0x3D5`
- runtime state is tracked through `cct_vga_row`, `cct_vga_col`, and `cct_vga_attr`
- scrolling is implemented inside the runtime shim and must remain inactive for the FS-1 boot banner layout

## Codegen Contract

Freestanding codegen now closes two contracts at once:

- builtin lowering maps `cct/console` rituals to the `cct_svc_console_*` runtime service calls
- exported `RITUALE` also emit raw C ABI wrappers so a C kernel can call `civitas_boot_banner()` directly

Wrapper policy:

- generated internal implementation keeps the existing `cct_fn_<module>_<rituale>` naming
- a freestanding ABI wrapper with the raw ritual name is emitted for exported rituals
- `main` is excluded from this raw-wrapper path to avoid breaking the host executable contract

## FS-1 Boot Layout Rules

The FS-1C screen has strict operational limits:

- the initial C banner from `src/kernel.c` must remain visible
- the CCT banner starts at VGA row `4`
- full-width `80`-column separator lines must not emit an extra newline, or the screen scrolls and the C banner is lost
- the final gate line must stay on the last screen row without a trailing newline

## Validation Path

FS-1 is validated through the main runner:

```bash
CCT_TEST_PHASES=FS1A,FS1B,FS1C bash tests/run_tests.sh
```

The runner asserts:

- host rejection and freestanding acceptance for `cct/console`
- `.cgen.c` emission with real console service calls
- cross-compilation of generated freestanding C with `i686-elf-gcc`
- integrated `grub-hello` build artifacts
- Multiboot compliance
- automated QEMU boot plus VGA-memory capture

## Current Limits

- console output is VGA text-mode only
- no heap allocator is introduced in FS-1
- no keyboard, timer, shell, or networking support is part of this block
- `console_write_centered()` is line-oriented and appends a newline by design
