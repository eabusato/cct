# FASE 21E3 Valgrind

- Arquivo verificado: `tests/integration/codegen_minimal.cct`
- Comando: `make valgrind_lexer`
- Log padrao: `tests/.tmp/valgrind_lexer_21e3.log`
- Status neste checkout: `SKIP`

Observacoes:
- O script oficial da fase e [tests/valgrind_lexer_21e3.sh](/Users/eabusato/dev/cct/tests/valgrind_lexer_21e3.sh).
- Neste ambiente local, `valgrind` nao esta instalado, entao o script registra `SKIP: valgrind indisponivel neste ambiente`.
- Em ambiente com `valgrind`, o criterio continua sendo `0 leaks`, `0 invalid reads/writes` e `0 uninitialized values`.
