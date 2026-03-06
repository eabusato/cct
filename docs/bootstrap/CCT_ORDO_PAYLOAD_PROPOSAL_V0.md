# CCT ORDO Payload Proposal V0

## Motivacao

O modelo atual baseado em `cct/variant` resolveu o bloqueio pratico de modelagem de tipo soma,
mas ainda exige disciplina manual:
- uso recorrente de `tag` numerica;
- payload opaco com casts/convencoes no consumidor;
- menor seguranca de tipo para acessores e construtores.

Objetivo desta proposta:
evoluir a linguagem para `ORDO` com payload nativo, preservando compatibilidade e rollout incremental.

## Sintaxe Candidata

```cct
ORDO Expr
  IDENT(VERBUM nome),
  LITERAL(REX valor),
  BINARY(SPECULUM NIHIL lhs, VERBUM op, SPECULUM NIHIL rhs)
FIN ORDO
```

Uso candidato:

```cct
EVOCA Expr a AD IDENT("x")
EVOCA Expr b AD LITERAL(42)
EVOCA Expr c AD BINARY(SPECULUM NIHIL a, "+", SPECULUM NIHIL b)
```

## Semantica Proposta

- Cada item de `ORDO` pode ter zero ou mais campos de payload.
- Construtores passam a ser nativos do item (`IDENT("x")`, `LITERAL(42)` etc.).
- Leitura de valor deve checar variante ativa.
- Itens sem payload continuam representando apenas tag.
- Itens com payload exigem aridade e tipos exatos.

## Impacto No Parser

- Estender gramatica de `ORDO` para aceitar lista de campos opcionais por item.
- Criar nos AST especificos para:
  - item de ORDO sem payload;
  - item de ORDO com payload tipado.
- Validar delimitadores e aridade sintatica ja no parser para reduzir erro tardio.

## Impacto No Semantico

- Resolver tipos dos campos por item e verificar referencias.
- Validar chamadas de construtor nativo:
  - item existe;
  - aridade correta;
  - tipos compativeis.
- Emitir diagnosticos canonicos para mismatch de item/payload.

## Impacto No Codegen

Lowering proposto:
- `struct` com campo `tag`;
- armazenamento de payload por `union` ou por bloco estruturado equivalente;
- helpers gerados para construcao e leitura segura por item.

Requisitos de codegen:
- manter layout deterministico;
- manter compatibilidade com backend C atual;
- preservar mensagens de erro previsiveis para acesso invalido.

## Backward Compatibility

- `ORDO` sem payload permanece valido sem mudanca de codigo.
- Codigo existente com `cct/variant` continua funcionando durante coexistencia.
- Nenhuma quebra imediata em APIs antigas.

## Plano De Migracao

### Fase 1: coexistencia
- introduzir sintaxe nova de `ORDO` com payload;
- manter `cct/variant` como caminho suportado.

### Fase 2: wrappers
- disponibilizar wrappers de compatibilidade `variant <-> ORDO payload`;
- facilitar migracao de consumidores gradualmente.

### Fase 3: deprecacao progressiva
- marcar APIs manuais de tag/payload como legadas;
- remover apenas apos janela de migracao documentada.

## Riscos E Mitigacoes

1. Complexidade no parser.
- Mitigacao: fasear mudanca por nos AST claros e erros pontuais.

2. Regressao na qualidade de diagnosticos.
- Mitigacao: manter codigos de erro e envelopes canonicos.

3. Crescimento de binario por suporte de payload.
- Mitigacao: strategy de lowering compacta e testes de impacto.

## Tradeoffs

| Aspecto | Vantagem | Custo |
|---|---|---|
| Ergonomia | Menos boilerplate manual | Parser/semantico mais complexos |
| Seguranca de tipo | Menos cast opaco em payload | Mais regras de validacao |
| Evolucao | Caminho nativo para AST/IR | Implementacao backend adicional |
| Compatibilidade | Coexistencia com `cct/variant` | Janela de migracao mais longa |

## Exemplo Completo (Declaracao E Uso)

```cct
INCIPIT grimoire "expr_demo"

ORDO Expr
  IDENT(VERBUM nome),
  LITERAL(REX valor),
  ADD(SPECULUM NIHIL lhs, SPECULUM NIHIL rhs)
FIN ORDO

RITUALE main() REDDE REX
  EVOCA Expr a AD IDENT("x")
  EVOCA Expr b AD LITERAL(1)
  EVOCA Expr c AD ADD(SPECULUM NIHIL a, SPECULUM NIHIL b)
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

## Conclusao

Esta proposta define direcao tecnica objetiva para sair do modelo manual de `Variant`
e chegar a um `ORDO` com payload nativo, com rollout incremental e sem breaking change abrupto.
