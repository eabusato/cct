# FASE 21E2 Benchmark

- Arquivo benchmark: `lib/cct/fluxus.cct`
- Iteracoes padrao: `100`
- Comando: `make benchmark_lexer`
- Resultado observado neste checkout: `Ratio aproximado 1.3x`
- Status: `PASS (< 10x)`

Observacoes:
- O benchmark compara `./cct --tokens` contra `./cct_lexer_bootstrap`.
- O script oficial da fase e [tests/benchmark_lexer_21e2.sh](/Users/eabusato/dev/cct/tests/benchmark_lexer_21e2.sh).
