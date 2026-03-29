# cct/console

Módulo canônico de console VGA 80x25 para o perfil `freestanding`.

Superfície inicial:
- `console_init()`
- `console_clear()`
- `console_putc(REX c)`
- `console_write(VERBUM texto)`
- `console_write_line(VERBUM texto)`
- `console_write_centered(VERBUM texto)`
- `console_set_color(REX fg, REX bg)`
- `console_set_cursor(REX linha, REX col)`
- `console_get_linha() REDDE REX`
- `console_get_coluna() REDDE REX`

Regras:
- disponível apenas com `--profile freestanding`;
- em perfil host, `ADVOCARE "cct/console"` falha com diagnóstico semântico canônico.
