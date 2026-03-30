# Freestanding FS-4 Release Notes

## Summary

FS-4 closes the first bare-metal network block of the CCT freestanding track.

Gate closed:

- `G-FS4`: the booted system now answers ICMP echo on `10.0.2.15` and accepts a TCP connection on port `80`

## Highlights

### FS-4A

- PCI enumeration was delivered as the freestanding discovery layer
- `cct/pci_fs` became the canonical CCT surface for locating the RTL8139 and enabling bus mastering

### FS-4B

- `../grub-hello` now contains an RTL8139 driver with RX/TX support and IRQ-backed receive processing
- `cct/net_fs` was delivered as the raw NIC bridge for MAC discovery, frame send/recv, and polling

### FS-4C

- Ethernet dispatch plus ARP, IPv4, and ICMP handling landed in `../grub-hello/src/net/`
- `cct/net_proto_fs` was delivered with the current static guest network contract for `10.0.2.15`

### FS-4D

- a minimal TCP listener was added in `../grub-hello/src/net/tcp.c`
- `cct/tcp_fs` was delivered as the canonical freestanding TCP bridge
- `../grub-hello/src/civitas_boot_fase4.cct` now boots into the FS-4 network screen and keeps a live listener on port `80`

## Compatibility Notes

- host-profile behavior remains unchanged; `cct/net_fs`, `cct/net_proto_fs`, and `cct/tcp_fs` are freestanding-only
- `../grub-hello` still preserves the compatibility build artifacts under `build/civitas_boot.*`
- the active freestanding entry in the integration repo is now `civitas_boot_fase4`

## Verification

- filtered block: `CCT_TEST_PHASES=FS4B,FS4C,FS4D bash tests/run_tests.sh`
- boot integration: `cd ../grub-hello && make iso`
- live gate probe: `python3 tests/support/freestanding_fs4_gate_probe.py --qemu-bin qemu-system-i386 --iso ../grub-hello/grub-hello.iso`
- full suite: `bash tests/run_tests.sh`

## References

- `docs/freestanding/README.md`
- `docs/freestanding/FS4_RUNTIME_MANUAL.md`
- `docs/freestanding/GRUB_HELLO_INTEGRATION.md`
- `../grub-hello/README.md`
