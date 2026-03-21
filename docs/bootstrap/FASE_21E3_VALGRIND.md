# FASE 21E3 Valgrind

- Checked file: `tests/integration/codegen_minimal.cct`
- Command: `make valgrind_lexer`
- Default log: `tests/.tmp/valgrind_lexer_21e3.log`
- Status in this checkout: `SKIP`

Notes:
- The phase valgrind script is [tests/valgrind_lexer_21e3.sh](/Users/eabusato/dev/cct/tests/valgrind_lexer_21e3.sh).
- In this local environment, `valgrind` is not installed, so the script records `SKIP`.
- In an environment with `valgrind`, the acceptance criteria remain `0 leaks`, `0 invalid reads/writes`, and `0 uninitialized values`.
