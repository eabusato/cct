# Freestanding FS-1 Release Notes

## Summary

FS-1 closes the first freestanding delivery block on the CCT side and on the `../grub-hello/` boot integration side.

Gate closed:

- `G-FS1`: the booted system screen now shows text written by a CCT ritual

## Highlights

### FS-1A

- `cct/console` delivered as the canonical freestanding VGA module
- host-profile rejection enforced in module resolution and semantic builtin analysis
- freestanding runtime shim extended with `cct_svc_console_*`

### FS-1B

- `../grub-hello/Makefile` now compiles CCT code as part of the ISO build
- generated freestanding C is compiled and linked with the kernel objects
- `kernel.bin` is validated as Multiboot-compatible

### FS-1C

- `src/civitas_boot.cct` exports `civitas_boot_banner()` and `civitas_status_info()`
- `kernel_main()` calls the CCT ritual after the initial C banner
- QEMU boot is validated automatically through VGA-memory capture

## Compatibility Notes

- default host compiler behavior remains unchanged
- `cct/console` remains freestanding-only by contract
- FS-1 still assumes VGA text mode and a 32-bit Multiboot boot path

## Verification

- filtered block: `CCT_TEST_PHASES=FS1A,FS1B,FS1C bash tests/run_tests.sh`
- full suite: `bash tests/run_tests.sh`
- bootable consumer: `cd ../grub-hello && make clean && make iso`

## References

- `docs/freestanding/README.md`
- `docs/freestanding/FS1_RUNTIME_MANUAL.md`
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
- `docs/release/FREESTND_FS2_RELEASE_NOTES.md`
- `../grub-hello/README.md`
