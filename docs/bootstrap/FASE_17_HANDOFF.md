# FASE 17 HANDOFF

## 1. Status Final Da Fase

- Status: PASS
- Data: 2026-03-06
- Gate final: `make test` verde na branch base (927 passed / 0 failed)
- Escopo fechado: 17A1 ate 17D4

## 2. Entregas Por Subfase

- 17A1: ponte `VERBUM <-> MILES` (`char_at`, `from_char`) e `cct/char`.
- 17A2: `cct/args` (`argc`, `arg`) com contrato de bounds.
- 17A3: `cct/verbum_scan` com cursor (`init`, `pos`, `eof`, `peek`, `next`, `free`).
- 17A4: consolidacao de toolkit de lexer e regressao integrada.
- 17B1: `cct/verbum_builder` (append, append_char, len, clear, free).
- 17B2: `cct/code_writer` com identacao deterministica.
- 17B3: integracao builder/writer com `cct/fmt` para int/bool/real.
- 17B4: fechamento de determinismo/carga textual e contrato de regressao.
- 17C1: `cct/variant` (tag + payload opaco) funcional no host.
- 17C2: `cct/variant_helpers` para predicado/tag/expect/payload_if.
- 17C3: `cct/ast_node` para nos AST host-side.
- 17C4: proposta documental ORDO com payload (`CCT_ORDO_PAYLOAD_PROPOSAL_V0.md`).
- 17D1: `cct/env` (`getenv`, `has_env`, `cwd`).
- 17D2: `cct/time` (`now_ms`, `now_ns`, `sleep_ms`).
- 17D3: `cct/bytes` (`bytes_new`, `bytes_len`, `bytes_get`, `bytes_set`, `bytes_free`).
- 17D4: consolidacao de `docs/spec.md`, handoff e gate documental final.

## 3. Evidencias Tecnicas

- Blocos de teste FASE 17A: 957-971.
- Blocos de teste FASE 17B: 972-987.
- Blocos de teste FASE 17C: 988-999.
- Blocos de teste FASE 17D: 1000-1012.
- Resultado final da regressao completa: 927 passed / 0 failed.

## 4. Riscos Residuais E Pendencias

- ORDO com payload continua como proposta de linguagem (nao implementado no parser/semantico/codegen).
- Modulos host-only de 17D dependem do perfil host e devem continuar bloqueados em freestanding.
- Pendencia de roadmap: decidir migracao de `variant` pragmatico para suporte nativo de soma/tag em fase futura.

## 5. Fora De Escopo Confirmado

- Self-hosting completo do compilador em uma unica fase.
- Implementacao da sintaxe nativa ORDO com payload.
- Novos backends alem do fluxo C-hosted atual.

## 6. Checklist De Entrada Para FASE 18

- [x] `make test` 0 failed na branch base.
- [x] docs de contrato 17A/17B/17C atualizados.
- [x] decisoes abertas de ORDO payload registradas.
- [ ] Definir backlog executavel da FASE 18 com criterios de aceite por subfase.
- [ ] Congelar baseline inicial da FASE 18 apos criacao dos prompts de implementacao.
