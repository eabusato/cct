# CCT Canonical Examples

## Quick Showcase Set (FASE 11G/11H)

- `showcase_stdlib_string_11g.cct`
- `showcase_stdlib_collection_11g.cct`
- `showcase_stdlib_io_fs_11g.cct`
- `showcase_stdlib_parse_math_random_11g.cct`
- `showcase_stdlib_modular_11g_main.cct`
- `showcase_realistic_ops.cct` (pragmatic day-to-day stdlib workflow)
- `ars_magna_canonica/main.cct` (advanced multi-module stdlib integration)

## Run

```bash
./cct examples/showcase_stdlib_string_11g.cct && ./examples/showcase_stdlib_string_11g
./cct examples/showcase_stdlib_collection_11g.cct && ./examples/showcase_stdlib_collection_11g
./cct examples/showcase_stdlib_io_fs_11g.cct && ./examples/showcase_stdlib_io_fs_11g
./cct examples/showcase_stdlib_parse_math_random_11g.cct && ./examples/showcase_stdlib_parse_math_random_11g
./cct examples/showcase_stdlib_modular_11g_main.cct && ./examples/showcase_stdlib_modular_11g_main
./cct examples/showcase_realistic_ops.cct && ./examples/showcase_realistic_ops
./cct examples/ars_magna_canonica/main.cct && ./examples/ars_magna_canonica/main
```

## Modular Introspection

```bash
./cct --ast-composite examples/showcase_stdlib_modular_11g_main.cct
./cct --sigilo-only --sigilo-mode essencial examples/showcase_stdlib_modular_11g_main.cct
./cct --sigilo-only --sigilo-mode completo examples/showcase_stdlib_modular_11g_main.cct
```
