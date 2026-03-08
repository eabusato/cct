# CCT — Release Notes: FASE 14T

**Data**: Marco 2026  
**Versao do compilador**: FASE 19D.4 + 14T  
**Testes**: 1120 passou / 0 falhou (suite completa)

---

## Resumo

A FASE 14T transforma o sigilo SVG em um artefato de leitura semantica sem quebrar o contrato visual existente:

- `<title>` nativo em elementos semanticos ja emitidos pelo SVG;
- `data-*` deterministico e aditivo em nos locais e arestas de `call`;
- semantica leve no root (`role`, `aria-label`, `desc`) quando a instrumentacao esta ativa;
- toggles explicitos para gerar SVG instrumentado ou SVG plain pre-14T;
- nenhum JavaScript obrigatorio.

Na pratica, os componentes do sigilo agora aceitam hover diretamente no SVG: circulos, linhas e arestas revelam contexto semantico sem viewer separado.

O fechamento da fase manteve o pipeline deterministico e encerrou com regressao global verde.

---

## 1) Hover Semantico Nativo

O SVG agora pode ser lido diretamente pelo navegador ou por ferramentas que respeitam `<title>`.

Cobertura entregue:
- nos de `RITUALE`
- nos estruturais (`branch`, `loop`, `bind`, `term`)
- arestas locais (`primary`, `call`, `branch`, `loop`, `bind`, `term`)
- nos e arestas do system sigilo

Os payloads de hover sao derivados de contexto real do programa: nome do rituale, tipo do statement, profundidade, contagens relevantes e trecho normalizado de fonte.

---

## 2) Metadados Aditivos

O SVG local ganhou `data-*` deterministicos, sem alterar o contrato `.sigil`:

- nos locais: `data-kind`, `data-ritual`, `data-line`, `data-col`, `data-depth`, `data-stmt`, e campos relacionados
- arestas de `call`: `data-kind`, `data-from`, `data-to`, `data-weight`, `data-self-loop`

Escopo deliberadamente fechado nesta fase:
- sem proliferação de metadados em todos os elementos do system sigilo
- sem schema novo em `.sigil`

---

## 3) Toggling de Instrumentacao

Novas opcoes de CLI:

```bash
./cct --sigilo-only --sigilo-no-titles arquivo.cct
./cct --sigilo-only --sigilo-no-data arquivo.cct
./cct --sigilo-only --sigilo-no-titles --sigilo-no-data arquivo.cct
```

Contrato:
- `--sigilo-no-titles`: remove `<title>` e wrappers de hover
- `--sigilo-no-data`: remove `data-*` aditivos e `<desc>`
- ambos juntos: restauram o SVG plain pre-14T

Isso permite escolher entre:
- leitura humana enriquecida
- consumo incremental por tooling
- preservacao de baseline estrutural minimalista

---

## 4) Qualidade e Nao-Regressao

Entregas de qualidade desta fase:
- normalizacao LF/CRLF no source-context
- escape/clipping deterministico de tooltip
- regressao local e system exercitada no runner principal
- cleanup dos artefatos desses testes no proprio `tests/run_tests.sh`
- runner enxuto mostrando apenas falhas e o sumario final

Gate final:
- `make test`
- resultado: **1120 passed / 0 failed**

---

## 5) Fora de Escopo

Ficaram fora da FASE 14T:
- viewer web dedicado
- navegacao interativa com JS
- busca, filtros, zoom semantico ou animacao
- novo schema `.sigil`
- redesign visual amplo do motor de layout

---

## 6) Proxima Fase

Com a FASE 14T encerrada, o proximo passo oficial volta para:
- **FASE 20 — Application Library Stack Expansion**
