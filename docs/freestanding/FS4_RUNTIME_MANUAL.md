# CCT Freestanding FS-4 Runtime Manual

## Scope

FS-4 closes the first freestanding networking block:

- FS-4A: PCI enumeration and `cct/pci_fs`
- FS-4B: RTL8139 driver bridge and `cct/net_fs`
- FS-4C: Ethernet, ARP, IPv4, ICMP, and `cct/net_proto_fs`
- FS-4D: minimal TCP listener and `cct/tcp_fs`

The concrete closure gate is `G-FS4`: the booted system exposes a live network stack, answers ICMP echo on `10.0.2.15`, and accepts a TCP connection on port `80`.

## Runtime Service Contract

The freestanding runtime shim in `src/runtime/cct_freestanding_rt.h` now exposes these FS-4 services:

- PCI and port-I/O services from FS-4A: `cct_svc_pci_*`, `cct_svc_inw`, `cct_svc_outw`, `cct_svc_inl`, `cct_svc_outl`
- NIC bridge services:
  - `cct_svc_net_init`
  - `cct_svc_net_send`
  - `cct_svc_net_recv`
  - `cct_svc_net_mac`
  - `cct_svc_net_poll`
- protocol dispatcher service:
  - `cct_svc_net_dispatch_init`
- TCP bridge services:
  - `cct_svc_tcp_init`
  - `cct_svc_tcp_accept`
  - `cct_svc_tcp_recv`
  - `cct_svc_tcp_send`
  - `cct_svc_tcp_close`
  - `cct_svc_tcp_state`

These symbols remain freestanding-only and are provided by `../grub-hello`.

## Canonical Modules

### `cct/pci_fs`

FS-4 continues to depend on the PCI bridge delivered in FS-4A:

- full device enumeration
- vendor and device lookup
- BAR and IRQ discovery
- bus mastering enable for the selected device
- helper path for locating the QEMU RTL8139 NIC

### `cct/net_fs`

Provides the CCT-facing RTL8139 surface:

- `net_fs_init(iobase)` initializes the NIC and returns success as `VERUM`
- `net_fs_mac()` returns a `NetMac` structure populated from the device
- `net_fs_is_ready()` checks for a non-zero MAC snapshot
- `net_fs_send()` and `net_fs_recv()` bridge raw Ethernet frames
- `net_fs_poll()` processes pending RX work without waiting for an IRQ
- `net_fs_print_mac()` renders the current MAC on the VGA console

Operational notes:

- `net_fs_mac()` allocates a 6-byte staging buffer through `mem_fs_alloc()`
- MAC printing is intentionally hex-based and console-oriented for boot diagnostics

### `cct/net_proto_fs`

Provides the first fixed network-configuration layer:

- `net_proto_fs_init()` installs the RX dispatcher
- `net_proto_fs_ip_text()` returns the current static guest IP as `10.0.2.15`
- `net_proto_fs_print_ip()` prints IP, gateway, and mask information

Current static network contract:

- guest IP: `10.0.2.15`
- gateway: `10.0.2.2`
- mask: `255.255.255.0`

### `cct/tcp_fs`

Provides the minimal TCP listener API used by the FS-4 gate:

- `tcp_fs_listen(port)`
- `tcp_fs_accept()`
- `tcp_fs_recv(buf, max_len)`
- `tcp_fs_send(data, len)`
- `tcp_fs_send_str(text)`
- `tcp_fs_close()`
- `tcp_fs_state()`
- `tcp_fs_is_connected()`
- `tcp_fs_peer_closed()`

Current state mapping:

- `0`: closed
- `1`: listen
- `2`: syn-received
- `3`: established
- `4`: close-wait

The module deliberately exposes only the narrow subset needed for the gate listener.

## Compiler Contracts

Host-profile gating now rejects these modules outside `--profile freestanding`:

- `cct/pci_fs`
- `cct/net_fs`
- `cct/net_proto_fs`
- `cct/tcp_fs`

Semantic validation and C generation now recognize the FS-4 builtins:

- NIC init, MAC, send, recv, and polling
- protocol-dispatch initialization
- TCP listen, accept, recv, send, close, and state

The generated `.cgen.c` objects compile cleanly with `i686-elf-gcc` and link into the bootable kernel.

## Boot Integration Contract

The active FS-4 entry is `../grub-hello/src/civitas_boot_fase4.cct`.

Actual boot flow:

1. `kernel_main()` prints the initial C banner
2. heap initialization still happens from the Multiboot memory map
3. low-level setup still includes GDT, PIC, IDT, keyboard, timer, and PCI support
4. `civitas_boot_fase4()` reuses the FS-2 presentation step through `civitas_boot_fase2()`
5. interrupts are enabled, the timer waits briefly, and the screen is cleared
6. the FS-4 screen prints PCI count, MAC, IP, gateway, mask, and network readiness markers
7. the TCP listener opens on port `80` and the boot loop continuously polls RX work

The integrated `../grub-hello/Makefile` now compiles:

- `src/pci.c`
- `src/rtl8139.c`
- `src/net/arp.c`
- `src/net/ip.c`
- `src/net/icmp.c`
- `src/net/net_dispatch.c`
- `src/net/tcp.c`
- `src/civitas_boot_fase4.cct`

The compatibility artifact names remain stable:

- `build/civitas_boot.cct`
- `build/civitas_boot.cgen.c`
- `build/civitas_boot.o`

## Validation

Filtered validation for this block:

```bash
CCT_TEST_PHASES=FS4B,FS4C,FS4D bash tests/run_tests.sh
```

Boot integration:

```bash
cd ../grub-hello && make iso
```

Concrete gate probe:

```bash
python3 tests/support/freestanding_fs4_gate_probe.py \
  --qemu-bin qemu-system-i386 \
  --iso ../grub-hello/grub-hello.iso
```

Expected evidence:

- VGA boot screen shows `=== FS-4: Rede Minima ===`
- ICMP echo reply is received from `10.0.2.15`
- TCP connection to port `80` returns the FS-4 banner text

## Limits

- the network configuration is static and hard-coded for the QEMU guest network
- only one NIC is expected and only the RTL8139 path is implemented
- the TCP implementation is intentionally minimal and single-connection oriented
- no retransmission, congestion control, or long-lived stream management is claimed in FS-4
