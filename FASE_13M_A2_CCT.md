# FASE 13M-A2 — Implementação no Compilador

## Operadores Implementados

Esta fase implementou os operadores congelados em A1:

- `**` — Potenciação (STAR_STAR token, right-associative)
- `//` — Floor division (SLASH_SLASH token)
- `%%` — Módulo euclidiano (PERCENT_PERCENT token)

## Tokens Adicionados

- `STAR_STAR` para `**`
- `SLASH_SLASH` para `//`
- `PERCENT_PERCENT` para `%%`

## Alinhamento com A1

O escopo implementado está alinhado com o escopo congelado em A1.
O operador `^` não foi implementado (POLICY_CARET_DEFERRED permanece ativo).

## Semântica Codegen

- `**` → `pow(a, b)` para FLAMMA; versão inteira para REX
- `//` → floor division com sinal correto
- `%%` → módulo euclidiano com sinal correto
