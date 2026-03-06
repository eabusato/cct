# EVIDENCIA_LINK_16D2

## Contexto
- Data/hora (UTC): 2026-03-06 15:38:05 UTC
- Workspace: `/home/eabusato/dev/cct`
- Artefato auditado: `build/lbos-bridge/cct_kernel.o`
- Objeto de linkcheck: `build/lbos-bridge/cct_kernel_linkcheck.o`
- Toolchain:
  - `gcc (GCC) 15.2.1 20260209`
  - `GNU ld (GNU Binutils) 2.46`
  - `GNU objdump (GNU Binutils) 2.46`
  - `GNU nm (GNU Binutils) 2.46`

## 1) Geração do artefato de bridge
Comando:
```bash
make lbos-bridge
```
Saída relevante:
```text
[CCT] lbos-bridge: emitindo ASM freestanding...
ASM emitted: lib/cct/kernel/kernel.cgen.s
Cross compiler: gcc
[CCT] lbos-bridge: montando objeto ELF32...
[CCT] lbos-bridge: auditando símbolos undefined proibidos...
[CCT] lbos-bridge: validando presença de símbolo cct_fn_...
[CCT] lbos-bridge: build/lbos-bridge/cct_kernel.o pronto
```
Status: PASS

## 2) Auditoria de undefined symbols gerais
Comando:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "U" {print $3}'
```
Saída:
```text
(vazio)
```
Status: PASS (sem undefined symbols)

## 3) Auditoria de símbolos proibidos (libc + helpers)
Comando:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "U" {print $3}' | \
  grep -E '^(printf|malloc|free|memcpy|memset|puts|fopen|fclose|__stack_chk_fail|__udivdi3|__divdi3|__muldi3|__floatsidf|__fixdfsi)$'
```
Saída:
```text
(vazio)
```
Exit code do `grep`: `1` (nenhuma ocorrência)
Status: PASS

## 4) Confirmação do símbolo entry
Comando:
```bash
nm build/lbos-bridge/cct_kernel.o | awk '$2 == "T" {print $3}' | \
  grep 'cct_fn_kernel_kernel_halt'
```
Saída:
```text
cct_fn_kernel_kernel_halt
```
Status: PASS

## 5) Link parcial ELF32 (relocatable)
Comando:
```bash
ld -m elf_i386 -r build/lbos-bridge/cct_kernel.o -o build/lbos-bridge/cct_kernel_linkcheck.o
```
Saída:
```text
(exit 0)
```
Status: PASS

## 6) Símbolos `cct_fn_` no objeto linkcheck
Comando:
```bash
objdump -t build/lbos-bridge/cct_kernel_linkcheck.o | grep cct_fn
```
Saída:
```text
000000fe l     F .text  00000044 cct_fn_kernel_memset
00000142 l     F .text  00000032 cct_fn_kernel_memcpy
00000174 l     F .text  00000022 cct_fn_kernel_inb
00000196 l     F .text  0000003e cct_fn_kernel_outb
000001d4 g     F .text  00000008 cct_fn_kernel_kernel_halt
```
Status: PASS

## 7) Gate CF+EAX (`clc`/`stc`)
Comando:
```bash
objdump -d build/lbos-bridge/cct_kernel.o | grep -E '\\b(clc|stc)\\b'
```
Saída:
```text
39: f8                    clc
64: f8                    clc
ae: f8                    clc
fa: f8                    clc
```
Status: PASS

## 8) Gate `.eh_frame`
Comando:
```bash
objdump -h build/lbos-bridge/cct_kernel.o | grep -i eh_frame
```
Saída:
```text
(vazio)
```
Exit code do `grep`: `1` (seção ausente)
Status: PASS

## Conclusão 16D.2
Validação de linkabilidade concluída com sucesso no lado CCT, sem tocar `third_party/cct-boot`:
- sem undefined proibidos de libc/helpers;
- símbolo de entry esperado presente;
- link parcial ELF32 (`ld -m elf_i386 -r`) bem-sucedido;
- contrato CF+EAX presente (`clc` detectado);
- `.eh_frame` ausente no objeto.
