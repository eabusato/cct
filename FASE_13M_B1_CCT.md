# FASE 13M-B1 — Matriz de Testes Profunda e Não-Regressão

## Contrato de Qualidade

### Matriz extensa de testes obrigatórios

Esta fase exige uma cobertura de 30 cenários de teste para os novos operadores,
abrangendo todos os casos relevantes de comportamento, borda e regressão.

### Requisito de checks

São obrigatórios no mínimo 25 novos checks efetivos para garantir cobertura
da implementação dos operadores `**`, `//` e `%%`.

### Regressão Completa

Nenhum teste histórico pode regredir após a adição dos operadores 13M.
É mandatório executar regressão completa com `make test` antes de fechar esta fase.

## Cobertura

- Potenciação: casos básicos, associatividade, real, zero, tipo-error
- Floor division: positivo, negativo, zero, tipo-error, identidade
- Módulo euclidiano: positivo, negativo, zero, tipo-error, identidade
- Precedência mista: combinações de `**`, `//`, `%%`
- Legacy: `%` continua funcionando; aritmética básica sem regressão
