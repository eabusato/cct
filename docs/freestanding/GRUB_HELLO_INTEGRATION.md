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
- PCI, NIC, Ethernet, IP, ICMP, TCP, and HTTP bring-up
- live HTTP-gate validation on the booted system

## Build Flow

The integrated `../grub-hello/Makefile` performs this pipeline:

1. compile `src/boot.s`, `src/gdt_flush.s`, `src/isr_stubs.s`, `src/kernel.c`, `src/terminal.c`, `src/gdt.c`, `src/pic.c`, `src/idt.c`, `src/keyboard.c`, `src/timer.c`, `src/pci.c`, `src/rtl8139.c`, `src/http_server.c`, `src/http_parser.c`, `src/http_response.c`, `src/http_router.c`, `src/net/arp.c`, `src/net/ip.c`, `src/net/icmp.c`, `src/net/net_dispatch.c`, and `src/net/tcp.c`
2. compile `cct/src/runtime/cct_freestanding_rt.c`
3. copy `src/civitas_boot_fase5.cct` into `build/civitas_boot.cct`
4. run `cct-host --profile freestanding --entry civitas_boot_fase5 --sigilo-no-svg build/civitas_boot.cct`
5. compile `build/civitas_boot.cgen.c` into `build/civitas_boot.o`
6. link all freestanding objects into `isodir/boot/kernel.bin`
7. produce `grub-hello.iso` with `grub-mkrescue`

Key generated artifacts:

- `../grub-hello/build/civitas_boot.cgen.c`
- `../grub-hello/build/civitas_boot.o`
- `../grub-hello/build/cct_freestanding_rt.o`
- `../grub-hello/isodir/boot/kernel.bin`
- `../grub-hello/grub-hello.iso`

The compatibility artifact names remain stable even though the boot source is now `civitas_boot_fase5.cct`:

- `build/civitas_boot.cgen.c`
- `build/civitas_boot.o`

This avoids breaking the existing freestanding runner while allowing the active boot demo to evolve by phase.

## ABI and Symbol Contract

The C kernel consumes:

- `civitas_boot_fase5`

`src/kernel.c` declares it as `extern void civitas_boot_fase5(void);` and invokes it after the initial C-only banner, heap initialization, and low-level interrupt-device setup.

The generated CCT object still exports the compatibility support symbols from earlier phases, but the concrete phase gate is driven by `civitas_boot_fase5()`.

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
- initializes `gdt`, `pic`, `idt`, `keyboard`, `timer`, and the PCI-ready base required by the network stack
- enters `civitas_boot_fase4()`

## Boot-Screen Contract

The boot flow now has three visible stages:

- rows `0-2`: C banner from `terminal.c`
- rows below the banner: FS-2 CCT screen from `civitas_boot_fase2()`
- after a short timer-backed delay: screen clear and FS-5 HTTP screen from `civitas_boot_fase5()`

Operational choices that matter:

- the CCT entry path still preserves the initial C banner until the explicit FS-4 clear
- the FS-5 transition enables IRQs before the timer delay so the NIC and timer path are live when the HTTP screen opens
- the FS-5 screen shows the HTTP readiness banner and concrete route list before the listener loop starts
- the steady-state loop accepts one TCP session at a time, parses the request, dispatches to a CCT route handler, responds, closes, and relistens automatically

## Automated Boot Validation

`tests/run_tests.sh` boots the ISO in QEMU with the monitor on stdio, executes:

```text
pmemsave 0xb8000 4000 <relative-path>
```

and then decodes the VGA text buffer to assert the boot screen.

Expected visible markers for the current gate:

- `=== FS-5: HTTP Freestanding ===`
- `[OK] Parser, response builder e router ativos`
- `[OK] HTTP listen :80`
- `Gate G-FS5:`
- `curl http://10.0.2.15/`

For manual host-side access with `cd ../grub-hello && make run`, use the forwarded host endpoints instead of the guest IP directly:

- `http://127.0.0.1:8080/`
- `http://127.0.0.1:8080/status`
- `http://127.0.0.1:8080/api/info`
- `curl -X POST http://127.0.0.1:8080/echo -d baremetal`

For the live gate, the validation path is split:

- `tests/run_tests.sh` asserts the boot-visible FS-5 screen and the integrated object pipeline
- `tests/support/freestanding_fs5_gate_probe.py` boots the ISO with a socket-backed RTL8139 peer and proves HTML, JSON, prefix-routing, echo, and 404 exchanges over raw TCP/HTTP
