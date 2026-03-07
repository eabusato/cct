# FASE 18 HANDOFF

## 1. Status Final Da Fase

- Status: PASS
- Data: 2026-03-06
- Escopo fechado: 18A1 ate 18D4
- Gate final: `make test` verde (1014 passed / 0 failed)

## 2. Entregas Por Subfase

- 18A1: expansao core de `cct/verbum` (busca, replace, case, trim/pad/repeat, slice/reverse).
- 18A2: operacoes de colecao de texto em `cct/verbum` (`split`, `split_char`, `join`, `lines`, `words`).
- 18A3: expansao de `cct/fmt` (radix, precisao, templates `format_1..4`, `table_row`).
- 18A4: expansao de `cct/parse` com `try_*`, radix e CSV.
- 18B1: mutacoes de `cct/fs` (`mkdir/delete/rename/copy/move`).
- 18B2: inspecao/listagem/temp em `cct/fs`.
- 18B3: expansao de `cct/io` para stderr/flush/stdin/tty.
- 18B4: expansao de `cct/path` para normalize/resolve/relative/split/ext.
- 18C1: expansao de `cct/fluxus` (peek/set/remove/insert/slice/copy/sort/to_ptr).
- 18C2: expansao de `cct/set` e `cct/map` (operacoes de conjunto e merge/keys/values).
- 18C3: expansao de `cct/alg` (min/max/soma/sort/rotate/fill/dot-product).
- 18C4: expansao de `cct/series` (sum/min/max/sort/is_sorted/count).
- 18D1: novo modulo `cct/process` + bridge runtime host para exec/capture/input/env/timeout.
- 18D2: novo modulo `cct/hash` + runtime para fnv1a bytes, crc32, murmur3.
- 18D3: novo modulo `cct/bit` + expansao de `cct/random` (bool/range/verbum/shuffle/bytes).
- 18D4: consolidacao documental da fase em `docs/spec.md` e este handoff.

## 3. Inventario De Modulos (Antes/Depois)

`Antes` abaixo segue o baseline de planejamento da FASE 18 (`md_out/FASE_18_CCT.md`).
`Depois` e a contagem observada no repositorio ao final da fase (numero de `RITUALE` por modulo).

| Modulo | Antes | Depois | Delta |
|---|---:|---:|---:|
| `cct/verbum` | 9 | 37 | +28 |
| `cct/fmt` | 8 | 24 | +16 |
| `cct/parse` | 3 | 15 | +12 |
| `cct/fs` | 5 | 26 | +21 |
| `cct/io` | 4 | 19 | +15 |
| `cct/path` | 4 | 16 | +12 |
| `cct/fluxus` | 9 | 22 | +13 |
| `cct/set` | 8 | 19 | +11 |
| `cct/map` | 11 | 17 | +6 |
| `cct/alg` | 4 | 23 | +19 |
| `cct/series` | 5 | 12 | +7 |
| `cct/random` | 3 | 11 | +8 |
| `cct/process` | 0 | 6 | +6 |
| `cct/hash` | 0 | 6 | +6 |
| `cct/bit` | 0 | 14 | +14 |
| **Total** | **73** | **267** | **+194** |

## 4. Decisoes Arquiteturais Relevantes

- `cct/bit` foi implementado prioritariamente em CCT (sem depender de uma familia extensa de builtins host), mantendo validacao canonica de faixa `0..63` para APIs de indice.
- `cct/random` manteve wrappers CCT e recebeu helpers host dedicados para `random_verbum`, `random_verbum_from`, `random_shuffle_int` e `random_bytes`.
- `cct/process` foi fechado como modulo host-only com contratos claros de retorno para `run`, `capture`, `input`, `env` e `timeout`.
- `cct/hash::combine` segue a estrategia de mistura canonica baseada em constante FNV para composicao deterministica.
- `cct/parse` consolidou caminho seguro (`try_*`) via retorno Option-like opaco, reduzindo hard-fail em fluxos de tooling.

## 5. Itens Fora De Escopo (Mantidos)

- RNG criptografico (fase atual usa PRNG host `rand()` para utilitarios nao-criptograficos).
- Suporte `cct/process` em freestanding.
- Tipo soma nativo (`ORDO` com payload tipado no core da linguagem) alem da proposta documental da fase 17.
- Buffer/string builder de performance com estrategia de capacidade customizada alem do escopo definido para 18.

## 6. Evidencias De Teste

- Blocos de teste FASE 18A: 1013-1043.
- Blocos de teste FASE 18B: 1044-1062.
- Blocos de teste FASE 18C: 1063-1077.
- Blocos de teste FASE 18D: 1078-1096.
- Resultado de regressao global final: `Passed: 1014`, `Failed: 0`.

## 7. Preparacao Para FASE 19

Backlog recomendado de entrada:

1. Consolidar bibliotecas para o toolchain self-hosted (lexer/parser/AST toolkits sobre APIs de 17+18).
2. Revisar contratos host-vs-freestanding dos modulos novos (`process/hash/random`) para evitar leaks de perfil.
3. Definir evolucao de performance para construcao textual (builder dedicado para codegen pesado).
4. Planejar proposta executavel para tipo soma/variant nativo de linguagem com migracao do toolkit host atual.

Checklist de entrada:

- [x] `make test` verde na regressao completa.
- [x] `docs/spec.md` atualizado com Bibliotheca Canonica 18.
- [x] Handoff da fase publicado.
- [ ] Congelar baseline inicial da FASE 19 apos prompts/subfases.
