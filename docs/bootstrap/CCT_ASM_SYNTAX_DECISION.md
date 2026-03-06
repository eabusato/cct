# CCT ASM Syntax Decision — FASE 16C.3

- Status: Normativo (ativo)
- Escopo: pipeline `--emit-asm` do CCT em perfil freestanding
- Fonte de verdade: `md_out/FASE_16_CCT.md` (16C.3), `docs/bootstrap/CCT_LOWERING_MATRIX_V0.md`

## Decisão Oficial

O formato oficial de assembly gerado pelo CCT na fase 16 é:

- sintaxe Intel para GAS, com header obrigatório `.intel_syntax noprefix`;
- montagem oficial via `as --32` (binutils).

A linha `.intel_syntax noprefix` deve aparecer no topo do arquivo `.cgen.s`.

## Racional Técnico

1. O pipeline atual usa cross-GCC com `-m32 -S -masm=intel`, que gera ASM compatível com GAS.
2. O fluxo reduz risco de regressão ao evitar conversão adicional de dialeto nesta fase.
3. A integração com validação automática (16C.2) já garante invariantes de ABI/lowering sobre esse formato.

## Requisitos de Ambiente

- `as` (GNU assembler) com suporte a `--32`.
- `nm` e `objdump` para auditoria de artefatos.
- Recomendação mínima de binutils: compatível com `.intel_syntax noprefix` (prática comum em versões modernas; referência histórica >= 2.18).

## Comando Canônico

```bash
as --32 <arquivo>.cgen.s -o <arquivo>.o
```

## Não-Objetivos da Fase 16

- Não implementar conversor GAS -> NASM nesta fase.
- Não mudar backend para emissão NASM nativa.

Conversão para NASM fica explicitamente para fase futura, caso haja demanda de integração direta com toolchain NASM puro.

## Implicações para LBOS

- O bridge CCT-LBOS na fase 16 usa `.cgen.s` em Intel syntax GAS e montagem com `as --32`.
- Contratos de símbolos e ABI permanecem regidos por:
  - `docs/bootstrap/CCT_SYMBOL_NAMING_V0.md`
  - `docs/bootstrap/CCT_ABI_V0_LBOS.md`
  - `docs/bootstrap/CCT_LOWERING_MATRIX_V0.md`

## Evidência Esperada em Teste

1. Arquivo `.cgen.s` contém `.intel_syntax noprefix` no topo.
2. `as --32` monta `.cgen.s` com exit 0.
