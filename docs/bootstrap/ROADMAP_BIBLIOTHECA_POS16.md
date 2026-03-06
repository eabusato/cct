# ROADMAP_BIBLIOTHECA_POS16

## Objetivo
Definir a expansão da Bibliotheca Canonica após o fechamento da FASE 16, com foco em
habilitar a escrita do compilador CCT em CCT (host-first), sem regressão de compatibilidade.

Este roadmap prioriza bloqueadores reais do pipeline de compilador:
1. leitura de caractere em `VERBUM`;
2. acesso a argumentos de linha de comando;
3. construção eficiente de texto (codegen);
4. representação robusta de nós discriminados (AST).

## Princípios
- Entrega incremental por subfase, com `make test` verde em cada gate.
- Host-first (backend C atual), sem quebrar trilha freestanding da FASE 16.
- Evitar scope creep: primeiro viabilizar lexer/CLI/codegen; depois ampliar ecossistema.
- APIs novas com contrato explícito de ownership e erro.

---

## FASE 17A — Bloqueadores Críticos de Lexing e CLI

### 17A.1 — Ponte `VERBUM <-> MILES` (bloqueador crítico #1)

#### Entregas
- Extensão de `cct/verbum` com APIs mínimas:
  - `char_at(VERBUM s, REX i) -> MILES`
  - `from_char(MILES c) -> VERBUM`
- Nova camada de classificação de caractere (preferencialmente em `cct/char.cct`):
  - `is_digit(MILES c) -> VERUM`
  - `is_alpha(MILES c) -> VERUM`
  - `is_whitespace(MILES c) -> VERUM`

#### Contrato
- `char_at`: política estrita para índice inválido (erro previsível e testável).
- `from_char`: string de tamanho 1, ownership documentado.
- Classificação limitada a ASCII no primeiro corte (Unicode fora de escopo inicial).

#### Critério de aceite
- É possível escrever um lexer funcional em CCT que itera caractere por caractere.
- Testes de borda: índice negativo, índice >= len, bytes limite, whitespace clássico (`' '`, `\t`, `\n`, `\r`).

### 17A.2 — `argv`/CLI (bloqueador crítico #2)

#### Entregas
- Novo módulo `cct/args.cct` (ou `cct/cli.cct`) com API mínima:
  - `argc() -> REX`
  - `arg(REX i) -> VERBUM`
  - `has(VERBUM flag) -> VERUM` (opcional nesta subfase)
- Bridge runtime host para expor `argc/argv` ao programa CCT.

#### Contrato
- `arg(i)` com política estrita para índice inválido.
- Compatível com execução atual (`./cct <programa.cct>` e projetos).

#### Critério de aceite
- Programa CCT consegue ler caminho de entrada de `argv` e validar falta de argumentos.
- Cobertura com fixtures host de sucesso/erro.

---

## FASE 17B — Eficiência de Construção de Texto (gap significativo)

### 17B.1 — `StringBuilder` canônico

#### Entregas
- Novo módulo `cct/verbum_builder.cct` com API base:
  - `builder_init() -> SPECULUM NIHIL`
  - `builder_append(SPECULUM NIHIL b, VERBUM s) -> NIHIL`
  - `builder_append_char(SPECULUM NIHIL b, MILES c) -> NIHIL`
  - `builder_len(SPECULUM NIHIL b) -> REX`
  - `builder_to_verbum(SPECULUM NIHIL b) -> VERBUM`
  - `builder_clear(SPECULUM NIHIL b) -> NIHIL`
  - `builder_free(SPECULUM NIHIL b) -> NIHIL`

#### Contrato
- Crescimento amortizado (evitar `O(n^2)` de concat incremental).
- Ownership explícito de builder e string final.

#### Critério de aceite
- Cenário de codegen textual com milhares de append sem degradação quadrática.
- Testes de estabilidade + integração com `cct/fmt` e `cct/verbum`.

---

## FASE 17C — Modelagem de Nó Discriminado (gap significativo)

### 17C.1 — Caminho pragmático de biblioteca (curto prazo)

#### Entregas
- Novo módulo `cct/variant.cct` (nome provisório) para tag + payload opaco:
  - `variant_make(REX tag, SPECULUM NIHIL payload) -> SIGILLUM Variant`
  - `variant_tag(SIGILLUM Variant v) -> REX`
  - `variant_payload(SIGILLUM Variant v) -> SPECULUM NIHIL`
- Convenção de tags via `ORDO` nos consumidores.

#### Limitação
- Ainda não substitui tipo soma nativo; é ponte pragmática para AST inicial.

### 17C.2 — Trilha de linguagem (médio prazo)

#### Entregas
- Proposta formal de extensão de linguagem para ORDO com payload (sum type).
- Documento de design + impacto semântico/codegen + plano de migração do `variant`.

#### Critério de aceite
- Decisão arquitetural registrada (aprovado/rejeitado) com testes de prova de conceito se aprovado.

---

## FASE 17D — Biblioteca Base para Compiladores (expansão adicional)

Após desbloquear lexer/CLI/codegen textual, expandir com módulos de produtividade:

### 17D.1 — `cct/verbum_scan` (cursor/tokenização)
- `cursor_init`, `cursor_peek`, `cursor_next`, `cursor_eof`, `cursor_pos`.
- Facilita lexer sem repetir boilerplate em projetos.

### 17D.2 — `cct/bytes` (buffer binário)
- leitura/escrita de bytes, slice, copy, fill.
- útil para futuro backend binário e ferramentas.

### 17D.3 — `cct/env` (ambiente host)
- `getenv`, `setenv` (se suportado), `cwd`, `exists` de variáveis.
- foco em toolchain/automação local.

### 17D.4 — `cct/time` (medição simples)
- relógio monotônico e wall-clock básico para benchmark/ferramentas.

---

## Ordem Recomendada de Execução
1. **17A.1** (`char_at` + classificação) — sem isso, não existe lexer em CCT.
2. **17A.2** (`argv`) — sem isso, não existe compilador CLI utilizável.
3. **17B.1** (`StringBuilder`) — remove gargalo crítico de codegen textual.
4. **17C.1** (`variant` pragmático) — viabiliza AST grande com disciplina.
5. **17D.x** (bibliotecas adicionais) — ganho de produtividade/ecossistema.
6. **17C.2** (tipo soma nativo) — decisão de linguagem com maturidade.

---

## Métricas de Sucesso
- `make test` sempre verde por subfase.
- Existe lexer CCT funcional escrito em CCT até o fim de 17A.
- Existe CLI mínima de compilador em CCT até o fim de 17A.2.
- Codegen textual em CCT sem degradação quadrática até o fim de 17B.1.
- Pacote de bibliotecas utilitárias host-first com documentação e testes determinísticos até 17D.

---

## Fora de Escopo Imediato
- Unicode completo/grapheme cluster em classificação de caracteres.
- Conversor GAS -> NASM.
- Reescrever todo o compilador em uma única fase (big-bang self-host).
- Alterar `third_party/cct-boot` (permanece somente-leitura).

---

## Próxima Ação Objetiva
Iniciar **17A.1** com design e implementação de:
- `verbum.char_at`
- `verbum.from_char`
- `cct/char` (`is_digit`, `is_alpha`, `is_whitespace`)

com testes de integração dedicados no padrão do runner atual.
