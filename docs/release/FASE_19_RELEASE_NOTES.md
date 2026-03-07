# CCT — Release Notes: FASE 19

**Data**: Marco 2026  
**Versao do compilador**: FASE 19D (pos-18D4)  
**Testes**: 1069 passou / 0 falhou (suite completa)

---

## Resumo

A FASE 19 adiciona quatro melhorias centrais para escrita de codigo de aplicacao:

- **CUM**: selecao por padrao para inteiros, VERBUM e ORDO.
- **FORMA**: interpolacao de strings com especificadores de formato.
- **ORDO com payload**: tipos soma (ADT) com campos tipados.
- **ITERUM expandido**: iteracao nativa sobre `map` e `set`.

Nao ha bugs conhecidos em aberto para os recursos entregues nesta fase.

---

## 1) CUM — Selecao por Padrao

### O que voce ganha

- Codigo de decisao mais legivel do que cadeias longas de `SI`/`ALITER`.
- Exaustividade obrigatoria para ORDO (sem branch silencioso perdido).
- Integracao direta com ORDO payload em `CASUS Variante(bindings)`.

### Tipos suportados

- Inteiros (`REX`, `DUX`, `COMES`, `MILES`)
- `VERBUM`
- `ORDO` (com e sem payload)

### Exemplo

```cct
ORDO Estado
  Ativo,
  Inativo
FIN ORDO

RITUALE main() REDDE NIHIL
  EVOCA REX codigo AD 404
  EVOCA Estado est AD Ativo

  CUM codigo
    CASUS 200:
      OBSECRO scribe("ok\n")
    CASUS 404:
      OBSECRO scribe("nao encontrado\n")
    ALIOQUIN:
      OBSECRO scribe("outro\n")
  FIN CUM

  CUM est
    CASUS Ativo:
      OBSECRO scribe("ativo\n")
    CASUS Inativo:
      OBSECRO scribe("inativo\n")
  FIN CUM

  REDDE
EXPLICIT RITUALE
```

### Regras importantes

- Em ORDO, sem `ALIOQUIN`, cobertura incompleta e erro de compilacao.
- Em inteiro/VERBUM, sem `ALIOQUIN`, a compilacao e aceita com warning.
- `FRANGE` dentro de `CUM` em loop encerra o loop externo.

---

## 2) FORMA — Interpolacao de Strings

### O que voce ganha

- Criacao de mensagens e textos formatados sem concatenacao manual extensa.
- Formatos numericos e alinhamento no proprio literal.
- Integracao natural com `OBSECRO scribe(...)`.

### Disponibilidade

- **Host-only** nesta fase.
- Em freestanding, `FORMA` gera erro de compilacao.

### Especificadores comuns

| Spec | Efeito |
|---|---|
| `5d` | inteiro com largura minima 5 |
| `.2f` | real com 2 casas decimais |
| `>10` | alinhamento a direita |
| `<10` | alinhamento a esquerda |
| `^10` | alinhamento central |

### Exemplo

```cct
RITUALE main() REDDE NIHIL
  EVOCA VERBUM nome AD "Carlos"
  EVOCA REX idade AD 30
  EVOCA UMBRA altura AD 1.75

  OBSECRO scribe(FORMA "Nome: {nome}, Idade: {idade}\n")
  OBSECRO scribe(FORMA "Altura: {altura:.2f}m\n")
  OBSECRO scribe(FORMA "Nome alinhado: {nome:>10}\n")

  EVOCA VERBUM msg AD FORMA "Maioridade: {idade >= 18}"
  OBSECRO scribe(msg, "\n")

  REDDE
EXPLICIT RITUALE
```

---

## 3) ORDO com Payload — ADTs Tipados

### O que voce ganha

- Modelagem de resultado/opcao de forma nativa e tipada.
- Construtores validados em tempo de compilacao.
- Desestruturacao direta com `CUM`.

### Tipos de payload suportados

`REX`, `DUX`, `COMES`, `MILES`, `UMBRA`, `FLAMMA`, `VERUM`, `VERBUM`.

### Exemplo (padrao Resultado)

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO

RITUALE dividir(REX a, REX b) REDDE Resultado
  SI b == 0
    REDDE Err("divisao por zero")
  FIN SI
  REDDE Ok(a // b)
EXPLICIT RITUALE

RITUALE main() REDDE NIHIL
  EVOCA Resultado r AD CONIURA dividir(10, 2)

  CUM r
    CASUS Ok(v):
      OBSECRO scribe(FORMA "resultado: {v}\n")
    CASUS Err(m):
      OBSECRO scribe(FORMA "erro: {m}\n")
  FIN CUM

  REDDE
EXPLICIT RITUALE
```

### Referencia idiomatica

- `lib/cct/ordo_samples.cct` inclui `Resultado` e `Opcao` como baseline.

---

## 4) ITERUM Expandido — `map` e `set`

### O que voce ganha

- Iteracao direta sobre colecoes canonicamente usadas em codigo de aplicacao.
- Ordem de iteracao por insercao para `map` e `set`.

### Regras de aridade

- `map`: 2 bindings (`chave`, `valor`)
- `set`: 1 binding (`item`)

### Exemplo

```cct
ADVOCARE "cct/map.cct"
ADVOCARE "cct/set.cct"

RITUALE main() REDDE REX
  EVOCA SPECULUM NIHIL mapa AD CONIURA map_init GENUS(REX, REX)()
  EVOCA SPECULUM NIHIL conjunto AD CONIURA set_init GENUS(REX)()

  CONIURA map_insert GENUS(REX, REX)(mapa, 1, 10)
  CONIURA map_insert GENUS(REX, REX)(mapa, 2, 20)
  CONIURA set_insert GENUS(REX)(conjunto, 7)
  CONIURA set_insert GENUS(REX)(conjunto, 9)

  ITERUM chave, valor IN mapa COM
    OBSECRO scribe(FORMA "{chave} -> {valor}\n")
  FIN ITERUM

  ITERUM elem IN conjunto COM
    OBSECRO scribe(FORMA "set: {elem}\n")
  FIN ITERUM

  CONIURA map_free GENUS(REX, REX)(mapa)
  CONIURA set_free GENUS(REX)(conjunto)
  REDDE 0
EXPLICIT RITUALE
```

---

## 5) Qualidade e Regressao

- Regressao completa executada: `make test`.
- Resultado final: **1069 passed / 0 failed**.
- Sem regressao nas fases 1 a 18.

---

## 6) Fora de Escopo (planejado para FASE 20)

- ORDO recursivo.
- OR-cases com binding de payload compartilhando corpo.
- Desestruturacao aninhada em `CASUS`.
- Guards em `CASUS`.
- `FORMA` em freestanding.
- Iteracao `ITERUM` para tipos customizados iteraveis.

---

## 7) Notas de Migracao

- Sem quebra de compatibilidade para codigo existente.
- `CUM`, `CASUS`, `ALIOQUIN`, `FORMA` passaram a ser palavras reservadas.
- Se usados antes como identificadores, devem ser renomeados.
