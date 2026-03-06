# cct/kernel

Módulo canônico mínimo para primitivas de kernel bare-metal no perfil `freestanding`.

Superfície inicial:
- `kernel_halt()`
- `kernel_outb(REX porta, REX valor)`
- `kernel_inb(REX porta) REDDE REX`
- `kernel_memcpy(SPECULUM REX dst, SPECULUM REX src, REX n)`
- `kernel_memset(SPECULUM REX dst, REX valor, REX n)`

Regras:
- disponível apenas com `--profile freestanding`;
- em perfil host, `ADVOCARE "cct/kernel"` falha com diagnóstico semântico canônico.
