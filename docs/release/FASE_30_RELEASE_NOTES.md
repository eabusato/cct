# FASE 30 - Operational Self-Hosted Platform - Release Notes

**Data:** 2026-03-21
**Versao:** 0.30.0
**Status:** Concluida

## Resumo

FASE 30 fecha a transicao do bootstrap validado para plataforma operacional. O compilador self-hosted deixa de ser apenas artefato de convergencia e passa a sustentar workflows reais de projeto, subset relevante da stdlib/runtime e bibliotecas de aplicacao.

## Escopo Fechado

### 30A - Self-Hosted Toolchain
- wrapper operacional do compilador self-hosted
- parser, semantic e codegen bootstrap recompilados com o proprio pipeline
- targets operacionais no `Makefile`

### 30B - Stdlib/Runtime Compatibility
- subset operacional explicitado para o caminho self-hosted
- wrappers/bootstrap support para texto, FS, path, parse, process e SQLite
- matriz materializada de compatibilidade self-hosted

### 30C - Application Libraries
- `cct/csv.cct`
- `cct/https.cct`
- `cct/orm_lite.cct`

### 30D - Project Workflows
- `project-selfhost-build`
- `project-selfhost-run`
- `project-selfhost-test`
- `project-selfhost-clean`
- `project-selfhost-package`
- projeto real de exemplo em `examples/phase30_data_app`

### 30E - Final Gate and Handoff
- gate agregado da plataforma operacional
- atualizacao do `README`
- release notes e handoff tecnico da fase

## Principais Entregas

### Make Targets
- `make bootstrap-selfhost-ready`
- `make bootstrap-selfhost-parser`
- `make bootstrap-selfhost-semantic`
- `make bootstrap-selfhost-codegen`
- `make bootstrap-selfhost-build SRC=... OUT=...`
- `make bootstrap-selfhost-run SRC=... OUT=...`
- `make bootstrap-selfhost-stdlib-matrix`
- `make project-selfhost-build PROJECT=...`
- `make project-selfhost-run PROJECT=...`
- `make project-selfhost-test PROJECT=...`
- `make project-selfhost-clean PROJECT=...`
- `make project-selfhost-package PROJECT=...`
- `make test-operational-platform`
- `make test-phase30-final`

### Self-Hosted Compiler Artifact
- `out/bootstrap/phase29/stage2/cct_stage2`
- `out/bootstrap/phase30/bin/cct_selfhost`

### Libraries
- `lib/cct/csv.cct`
- `lib/cct/https.cct`
- `lib/cct/orm_lite.cct`

### Example Projects
- `examples/phase30_data_app`
- `examples/project_broken_30d`

## Operational Contract

O caminho self-hosted validado nesta fase cobre:
- compilacao single-file via stage2
- projetos no layout canonico (`cct.toml`, `src/main.cct`, `tests/*.test.cct`)
- bibliotecas maduras da fase (`csv`, `https`, `orm_lite`)
- empacotamento com manifest refletindo o compilador usado

## Validation Snapshot

- `make test-bootstrap-selfhost`: PASS
- `make test-operational-selfhost`: PASS
- `make test-operational-platform`: PASS
- `make test-phase30-final`: PASS
- `make test`: PASS

## Limitacoes Conhecidas

1. O binario self-hosted ainda nao implementa internamente os subcomandos canonicos `cct build/run/test`; o workflow operacional desta fase e exposto via targets `project-selfhost-*`.
2. O subset self-hosted de modulos `cct/...` permanece explicito; modulos fora da matriz suportada falham com erro claro.
3. O exemplo historico `examples/fase12_final_showcase` nao e parte do subset self-hosted operacional porque depende de `cct/math.cct`, ainda fora da exposicao operacional do self-hosted prelude.

## Fechamento 21-30

Com a FASE 30:
- o bootstrap lexer/parser/semantic/codegen esta concluido;
- o self-hosting multi-stage converge (`stage1 == stage2`);
- o compilador self-hosted possui caminho operacional real para programas e projetos;
- a plataforma passa a ter base tecnica para evolucao pos-bootstrap.
