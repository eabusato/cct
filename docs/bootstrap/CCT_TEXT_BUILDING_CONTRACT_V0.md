# CCT Text Building Contract V0

Este documento formaliza os contratos observaveis da trilha textual da FASE 17B:
- `cct/verbum_builder.cct`
- `cct/code_writer.cct`

## 1) Invariantes do Builder

### `builder_len`
- `builder_len(b)` retorna o tamanho atual acumulado em bytes.
- Apos cada `builder_append`/`builder_append_char`, o tamanho deve refletir exatamente o crescimento aplicado.

### `builder_clear`
- `builder_clear(b)` zera o conteudo logico.
- Apos `builder_clear(b)`, `builder_len(b) == 0`.
- Novos appends apos `clear` devem funcionar normalmente.

### `builder_to_verbum` (snapshot)
- `builder_to_verbum(b)` retorna snapshot textual do estado atual.
- O snapshot nao pode corromper o estado interno do builder.
- Chamadas repetidas sem mutacao intermediaria devem produzir texto equivalente.

## 2) Invariantes do Writer

### `writer_indent` / `writer_dedent`
- Cada `writer_indent` aumenta 1 nivel de indentacao logica.
- Cada `writer_dedent` reduz 1 nivel.
- `writer_dedent` abaixo de zero e erro de contrato (underflow guard).

### `writer_write` / `writer_writeln`
- `writer_write` escreve sem quebra automatica de linha.
- `writer_writeln` escreve conteudo e finaliza com `\n`.
- Em inicio de linha, o writer aplica indentacao canonica de 2 espacos por nivel.

### `writer_to_verbum`
- Retorna snapshot textual final do buffer interno do writer.
- O resultado deve ser deterministico para a mesma sequencia de chamadas.

## 3) Ownership e ciclo de vida

- `builder_init` / `writer_init` alocam recursos internos.
- `builder_free` / `writer_free` sao obrigatorios para liberar recursos.
- O `VERBUM` retornado por `*_to_verbum` segue ownership da runtime de strings; nao exige `free` manual separado no nivel CCT.

## 4) Definicao de determinismo textual

Determinismo significa:
- mesma entrada (fonte + argumentos) e mesma sequencia de operacoes;
- mesmo output textual byte-a-byte.

Os gates desta fase validam determinismo com execucoes repetidas e comparacao por `cmp -s`.

## 5) Limites conhecidos (nao objetivos desta fase)

- Nao ha garantia de benchmark absoluto de throughput entre maquinas.
- Nao ha SLA de latencia por append.
- O contrato desta fase cobre integridade, estabilidade e repetibilidade funcional.
