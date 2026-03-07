# CCT — Handoff FASE 19

**Status**: PASS  
**Data**: 2026-03-07  
**Testes**: 1069 passou / 0 falhou

---

## Resumo executivo

A FASE 19 fechou com sucesso e adicionou quatro ganhos de ergonomia/seguranca:

| Construto | Subfases | Status |
|---|---|---|
| QUANDO | 19A1-19A4 | PASS |
| MOLDE | 19B1-19B4 | PASS |
| ORDO com payload | 19C1-19C4 | PASS |
| ITERUM map/set | 19D1 | PASS |
| Documentacao e release | 19D2-19D3 | PASS |

---

## Tabela de features (antes/depois)

| Feature | Antes (18D4) | Depois (19D4) |
|---|---|---|
| QUANDO sobre inteiros | Nao | Sim |
| QUANDO sobre VERBUM | Nao | Sim |
| QUANDO sobre ORDO simples | Nao | Sim |
| QUANDO sobre ORDO com payload | Nao | Sim |
| Exaustividade de QUANDO sobre ORDO | Nao | Sim (erro de compilacao sem SENAO) |
| MOLDE basico | Nao | Sim |
| MOLDE com especificadores | Nao | Sim |
| MOLDE com expressoes inline | Nao | Sim |
| ORDO com payload (declaracao/construcao) | Nao | Sim |
| ORDO payload + binding em QUANDO | Nao | Sim |
| ITERUM sobre map | Nao | Sim (2 bindings) |
| ITERUM sobre set | Nao | Sim (1 binding) |
| Ordem de iteracao map/set | Nao definida | Insercao |

---

## Contagem de testes por subfase

| Subfase | Fixtures | Status |
|---|---:|---|
| 19A1 | 5 | PASS |
| 19A2 | 3 | PASS |
| 19A3 | 4 | PASS |
| 19A4 | 4 | PASS |
| 19B1 | 6 | PASS |
| 19B2 | 4 | PASS |
| 19B3 | 3 | PASS |
| 19B4 | 4 | PASS |
| 19C1 | 3 | PASS |
| 19C2 | 4 | PASS |
| 19C3 | 6 | PASS |
| 19C4 | 4 | PASS |
| 19D1 | 5 | PASS |
| **Total FASE 19** | **55** | **PASS** |
| **Acumulado global** | **1069** | **PASS** |

---

## Decisoes arquiteturais

1. `QUANDO` usa lowering por tipo: `switch` para inteiros/ORDO e cadeia `strcmp` para `VERBUM`, mantendo C gerado idiomatico e previsivel.
2. Exaustividade de `QUANDO` sobre ORDO e erro (nao warning) para bloquear estados nao tratados em runtime.
3. `FRANGE` dentro de `QUANDO` em loop usa salto para rotulo de break do loop (`goto`), evitando semantica incorreta de `break` no `switch`.
4. `MOLDE` foi implementado com builder dinamico de runtime para evitar truncamento e suportar tamanho final desconhecido.
5. `MOLDE` permanece host-only nesta fase por dependencia de alocacao dinamica.
6. ORDO com payload foi reduzido para tagged union em C, com construcao por compound literal.
7. Bindings de payload em `CASO Variante(x, y)` tem escopo local ao bloco do caso.
8. `ITERUM` foi expandido para `map`/`set` com contratos de aridade validados no semantico.
9. Runtime de `map`/`set` preserva ordem de insercao para iteracao deterministica.

---

## Estado do compilador pos-FASE 19

- Linguagem: QUANDO/CASO/SENAO, MOLDE, ORDO payload e ITERUM map/set estaveis.
- Documentacao normativa atualizada: `docs/spec.md`, `docs/bibliotheca_canonica.md`, `docs/architecture.md`.
- Release notes publicados: `docs/release/FASE_19_RELEASE_NOTES.md`.
- Suite global completa verde no fechamento da fase.

---

## Backlog FASE 20 (priorizado)

### Prioridade alta

| Item | Racional |
|---|---|
| `cct/result` e `cct/option` genericos (`GENUS`) | Impacto direto em APIs seguras e ergonomicas |
| Guards em `CASO` (`CASO x SE cond`) | Reduz `QUANDO` aninhado e melhora legibilidade |
| OR-cases com binding de payload | Cobertura de fluxos `Ok(v)` / `Algum(v)` no mesmo corpo |
| ORDO generico mais expressivo | Libera ADTs de dominio com menos boilerplate |

### Prioridade media

| Item | Racional |
|---|---|
| Desestruturacao aninhada em `CASO` | Amplia modelagem de ADTs compostos |
| ORDO recursivo | Estruturas como listas/arvores nativas |
| ITERUM para tipos customizados | Ergonomia para colecoes de dominio |
| Inferencia parcial de `GENUS` | Reduz anotacao explicita repetitiva |

### Prioridade baixa

| Item | Racional |
|---|---|
| `QUANDO` como expressao | Conveniencia sintatica |
| Novos especificadores de `MOLDE` | Cobertura extra de formato |
| Wildcard explicito em `CASO` | Alias adicional para fallback |

---

## Evidencias de qualidade

- Gate final executado: `make test`.
- Resultado registrado: `Passed: 1069`, `Failed: 0`.
- Snippets de docs/release da FASE 19 compilados em `tests/.tmp`.
- Nenhum `TODO` pendente em `src/` identificado no fechamento.

---

## Notas para a proxima sessao

1. Iniciar a FASE 20 pelo documento mestre (`md_out/FASE_20_CCT.md`).
2. Atacar primeiro itens de alta prioridade (GENUS em result/option e guards em CASO).
3. Manter gate obrigatorio: `make test` verde ao final de cada subfase.
