# FASE 13M-A1 — Congelamento de Escopo e Decisões Semânticas

## Escopo Congelado

Operadores priorizados para implementação:

- `**` — Potenciação com semântica de right-associativity
- `//` — floor division (divisão inteira com arredondamento para menos infinito)
- `%%` — módulo euclidiano (resultado sempre não-negativo)

Operadores excluídos do escopo 13M:

- `?.` — encadeamento opcional (deferido para fase posterior)
- `!!` — asserção não-nula (deferido para fase posterior)

## Decisões Semânticas Congeladas

### POLICY_CARET_DEFERRED

O operador `^` (caret / XOR bitwise / potenciação alternativa) foi explicitamente deferido.
Motivo: ambiguidade semântica entre XOR bitwise e potenciação; POLICY_CARET_DEFERRED permanece ativo.

### floor division (`//`)

A semântica de floor division foi adotada: `a // b = floor(a / b)`.
Para entradas negativas, o resultado é arredondado para menos infinito.

### módulo euclidiano (`%%`)

A semântica euclidiana foi adotada: `a %% b = a - b * floor(a / b)`.
O resultado é sempre não-negativo quando `b > 0`.

## Conclusão

Escopo congelado. Nenhuma adição será feita após A1 sem nova fase.
