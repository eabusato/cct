# FASE 30 - Handoff Tecnico

**Data:** 2026-03-21
**Status:** Concluida
**Proxima etapa natural:** expansao pos-bootstrap sobre o compilador self-hosted operacional

## Executive Summary

FASE 30 fecha a trilha de bootstrap. O compilador self-hosted agora tem:
- convergencia multi-stage validada em FASE 29;
- subset operacional de stdlib/runtime;
- bibliotecas de aplicacao maduras;
- workflow real de projeto no caminho self-hosted;
- gate final e documentacao alinhados ao estado validado.

## Deliverables

### Toolchain
- `out/bootstrap/phase29/stage2/cct_stage2`
- `out/bootstrap/phase30/bin/cct_selfhost`
- manifests de `out/bootstrap/phase30/manifests/`

### Workflow Operacional
- `project-selfhost-build`
- `project-selfhost-run`
- `project-selfhost-test`
- `project-selfhost-clean`
- `project-selfhost-package`

### Libraries
- `lib/cct/csv.cct`
- `lib/cct/https.cct`
- `lib/cct/orm_lite.cct`

### Exemplos
- `examples/phase30_data_app`
- `examples/project_broken_30d`

## Estado Validado

### Verde
- gate de self-hosting multi-stage
- gate operacional de stdlib relevante
- gate operacional de bibliotecas de aplicacao
- gate operacional de workflow de projeto
- baseline agregado `make test`

### Nao Prometido Nesta Fase
- exposicao completa de todos os modulos `cct/...` no caminho self-hosted
- paridade interna total dos subcomandos `cct build/run/test` dentro do binario stage2
- distribuicao “production-ready” fora do ambiente e subset validados

## Como Operar

### Single-file
```bash
make bootstrap-selfhost-ready
make bootstrap-selfhost-build SRC=tests/integration/selfhost_csv_parse_30c_input.cct OUT=out/bootstrap/phase30/run/csv_demo
```

### Projeto
```bash
make project-selfhost-build PROJECT=examples/phase30_data_app
make project-selfhost-run PROJECT=examples/phase30_data_app
make project-selfhost-test PROJECT=examples/phase30_data_app
make project-selfhost-package PROJECT=examples/phase30_data_app
```

## Riscos Residuais

1. A escolha do caminho self-hosted para projetos ainda e feita por target de `Makefile`, nao por subcomando nativo do binario stage2.
2. O subset operacional de stdlib continua explicitamente limitado pela prelude/support layer.
3. Alguns exemplos historicos do repositório continuam fora do subset operacional self-hosted.

## Handoff Checklist

- [x] wrapper self-hosted operacional
- [x] subset de stdlib/runtime materializado
- [x] bibliotecas de aplicacao maduras
- [x] projeto real usando libs da fase
- [x] projeto negativo com falha clara
- [x] gate agregado de FASE 30
- [x] release notes e README atualizados

## Sign-off

O bootstrap esta concluido. A base correta a partir daqui e evoluir o compilador CCT self-hosted como ferramenta primaria, ampliando gradualmente o subset operacional ate cobertura plena da plataforma.
