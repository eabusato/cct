# FASE 16D2 — Linkability Evidence

## Context
- Timestamp (UTC): 2026-03-06 15:38:05 UTC
- Workspace: `/home/eabusato/dev/cct`
- Audited artifact: `build/lbos-bridge/cct_kernel.o`
- Linkcheck object: `build/lbos-bridge/cct_kernel_linkcheck.o`
- Toolchain:
  - `gcc (GCC) 15.2.1 20260209`
  - `GNU ld (GNU Binutils) 2.46`
  - `GNU objdump (GNU Binutils) 2.46`
  - `GNU nm (GNU Binutils) 2.46`

## 1) Bridge Artifact Generation
Command:
```bash
make lbos-bridge
```
Relevant output:
```text
[CCT] lbos-bridge: emitting freestanding ASM...
ASM emitted: lib/cct/kernel/kernel.cgen.s
Cross compiler: gcc
[CCT] lbos-bridge: assembling ELF32 object...
[CCT] lbos-bridge: auditing forbidden undefined symbols...
[CCT] lbos-bridge: validating presence of cct_fn_ symbol...
[CCT] lbos-bridge: build/lbos-bridge/cct_kernel.o ready
```
Status: PASS

## 2) General Undefined-Symbol Audit
Command:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "U" {print $3}'
```
Output:
```text
(empty)
```
Status: PASS (no undefined symbols)

## 3) Forbidden-Symbol Audit (libc + helpers)
Command:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "U" {print $3}' | \
  grep -E '^(printf|malloc|free|memcpy|memset|puts|fopen|fclose|__stack_chk_fail|__udivdi3|__divdi3|__muldi3|__floatsidf|__fixdfsi)$'
```
Output:
```text
(empty)
```
`grep` exit code: `1` (no match)
Status: PASS

## 4) Entry-Symbol Confirmation
Command:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "T" {print $3}' | \
  grep 'cct_fn_kernel_kernel_halt'
```
Output:
```text
cct_fn_kernel_kernel_halt
```
Status: PASS

## 5) ELF32 Partial Link (Relocatable)
Command:
```bash
ld -m elf_i386 -r build/lbos-bridge/cct_kernel.o -o build/lbos-bridge/cct_kernel_linkcheck.o
```
Output:
```text
(exit 0)
```
Status: PASS

## 6) `cct_fn_` Symbols in the Linkcheck Object
Command:
```bash
objdump -t build/lbos-bridge/cct_kernel_linkcheck.o | grep cct_fn
```
Output:
```text
000000fe l     F .text  00000044 cct_fn_kernel_memset
00000142 l     F .text  00000032 cct_fn_kernel_memcpy
00000174 l     F .text  00000022 cct_fn_kernel_inb
00000196 l     F .text  0000003e cct_fn_kernel_outb
000001d4 g     F .text  00000008 cct_fn_kernel_kernel_halt
```
Status: PASS

## 7) CF+EAX Gate (`clc`/`stc`)
Command:
```bash
objdump -d build/lbos-bridge/cct_kernel.o | grep -E '\\b(clc|stc)\\b'
```
Output:
```text
39: f8                    clc
64: f8                    clc
ae: f8                    clc
fa: f8                    clc
```
Status: PASS

## 8) `.eh_frame` Gate
Command:
```bash
objdump -h build/lbos-bridge/cct_kernel.o | grep -i eh_frame
```
Output:
```text
(empty)
```
`grep` exit code: `1` (section absent)
Status: PASS

## 16D.2 Conclusion
Linkability validation completed successfully on the CCT side without touching `third_party/cct-boot`:
- no forbidden undefined libc/helper symbols;
- expected entry symbol present;
- ELF32 partial link (`ld -m elf_i386 -r`) successful;
- CF+EAX contract present (`clc` detected);
- `.eh_frame` absent in the object.
