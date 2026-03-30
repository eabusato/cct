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
- FS-4A complete: PCI enumeration plus `cct/pci_fs`
- FS-4B complete: RTL8139 NIC bridge plus `cct/net_fs`
- FS-4C complete: Ethernet, ARP, IPv4, ICMP, and `cct/net_proto_fs`
- FS-4D complete: TCP listener plus `cct/tcp_fs`
- FS-5A complete: HTTP accept/read loop plus `cct/http_server_fs`
- FS-5B complete: HTTP/1.1 parser plus `cct/http_parser_fs`
- FS-5C complete: HTTP response builder plus `cct/http_response_fs`
- FS-5D complete: HTTP router plus CCT handlers through `cct/http_router_fs`
- Current concrete gate: `G-FS5` closed with boot-visible HTTP status plus live HTML/JSON validation

## Document Map

- `docs/freestanding/FS1_RUNTIME_MANUAL.md`
  Historical runtime, module, and codegen contracts for FS-1A through FS-1C.
- `docs/freestanding/FS2_RUNTIME_MANUAL.md`
  Runtime, module, heap, string, and collection contracts for FS-2A through FS-2C.
- `docs/freestanding/FS3_RUNTIME_MANUAL.md`
  Runtime, interrupt, keyboard, timer, and shell contracts for FS-3A through FS-3D.
- `docs/freestanding/FS4_RUNTIME_MANUAL.md`
  Runtime, NIC, protocol, TCP, and gate-validation contracts for FS-4A through FS-4D.
- `docs/freestanding/FS5_RUNTIME_MANUAL.md`
  Runtime, parser, response, router, and HTTP gate contracts for FS-5A through FS-5D.
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
  Build, heap-init, link, and boot integration with `../grub-hello/`.
- `docs/release/FREESTND_FS1_RELEASE_NOTES.md`
  Release summary for the first freestanding delivery block.
- `docs/release/FREESTND_FS2_RELEASE_NOTES.md`
  Release summary for the freestanding memory/string/collection delivery block.
- `docs/release/FREESTND_FS3_RELEASE_NOTES.md`
  Release summary for the freestanding interrupt/keyboard/timer/shell delivery block.
- `docs/release/FREESTND_FS4_RELEASE_NOTES.md`
  Release summary for the freestanding PCI/network/TCP delivery block.
- `docs/release/FREESTND_FS5_RELEASE_NOTES.md`
  Release summary for the freestanding HTTP-server delivery block.
- `../grub-hello/README.md`
  Consumer-side notes for the bootable integration repository.

## Primary Commands

```bash
./cct-host --profile freestanding --check tests/integration/http_router_fs_freestanding_smoke_fs5d.cct
CCT_TEST_PHASES=FS1A,FS1B,FS1C,FS2A,FS2B,FS2C,FS3A,FS3B,FS3C,FS3D,FS4A,FS4B,FS4C,FS4D,FS5A,FS5B,FS5C,FS5D bash tests/run_tests.sh
cd ../grub-hello && make clean && make iso
python3 tests/support/freestanding_fs5_gate_probe.py --qemu-bin qemu-system-i386 --iso ../grub-hello/grub-hello.iso
```

## Current Closure Criteria

- `cct/console` is rejected on host profile and accepted on freestanding profile.
- `cct/mem_fs`, `cct/verbum_fs`, `cct/fluxus_fs`, `cct/irq_fs`, `cct/keyboard_fs`, `cct/timer_fs`, `cct/shell_fs`, `cct/pci_fs`, `cct/net_fs`, `cct/net_proto_fs`, `cct/tcp_fs`, `cct/http_server_fs`, `cct/http_parser_fs`, `cct/http_response_fs`, and `cct/http_router_fs` are rejected on host profile and accepted on freestanding profile.
- Freestanding codegen emits console, heap, string-builder, dynamic-array, IRQ, keyboard, timer, PCI, NIC, protocol-dispatch, TCP, and HTTP service calls plus raw C ABI wrappers for exported `RITUALE`.
- `../grub-hello/Makefile` produces `build/civitas_boot.cgen.c`, `build/civitas_boot.o`, `isodir/boot/kernel.bin`, and `grub-hello.iso`.
- `kernel.bin` is recognized as Multiboot by GRUB.
- QEMU boot shows the C banner from `terminal.c`, transitions through the FS-2 CCT screen, and then reaches the FS-5 HTTP screen produced by `civitas_boot_fase5()`.
- The active gate proves that the booted kernel serves `GET /`, `GET /status`, `GET /api/info`, `POST /echo`, and default `404` responses on port `80`.
