# FASE 21E2 Benchmark

- Benchmark file: `lib/cct/fluxus.cct`
- Default iterations: `100`
- Command: `make benchmark_lexer`
- Observed result in this checkout: `approximate ratio 1.3x`
- Status: `PASS (< 10x)`

Notes:
- The benchmark compares `./cct --tokens` against `./cct_lexer_bootstrap`.
- The phase benchmark script is [tests/benchmark_lexer_21e2.sh](/Users/eabusato/dev/cct/tests/benchmark_lexer_21e2.sh).
