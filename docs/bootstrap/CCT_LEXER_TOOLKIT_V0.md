# CCT Lexer Toolkit V0 (FASE 17A)

## 1. Superficie Publica

### cct/verbum.cct
```cct
RITUALE char_at(VERBUM s, REX i) REDDE MILES
RITUALE from_char(MILES c) REDDE VERBUM
```

### cct/char.cct
```cct
RITUALE is_digit(MILES c) REDDE VERUM
RITUALE is_alpha(MILES c) REDDE VERUM
RITUALE is_whitespace(MILES c) REDDE VERUM
```

### cct/args.cct
```cct
RITUALE argc() REDDE REX
RITUALE arg(REX i) REDDE VERBUM
```

### cct/verbum_scan.cct
```cct
RITUALE cursor_init(VERBUM s) REDDE SPECULUM NIHIL
RITUALE cursor_pos(SPECULUM NIHIL c) REDDE REX
RITUALE cursor_eof(SPECULUM NIHIL c) REDDE VERUM
RITUALE cursor_peek(SPECULUM NIHIL c) REDDE MILES
RITUALE cursor_next(SPECULUM NIHIL c) REDDE MILES
RITUALE cursor_free(SPECULUM NIHIL c) REDDE NIHIL
```

## 2. Semantica de Borda

- `char_at(s, i)` falha em indice invalido com: `verbum char_at index out of bounds`
- `arg(i)` falha em indice invalido com: `args arg index out of bounds`
- `cursor_peek(c)` em EOF falha com: `scan peek at eof`
- `cursor_next(c)` em EOF falha com: `scan next at eof`
- `cursor_free(c)` aceita `NULL` (idempotente)

## 3. Escopo ASCII

- `is_digit`: bytes `0-9` (`48..57`)
- `is_alpha`: bytes `A-Z` (`65..90`) ou `a-z` (`97..122`)
- `is_whitespace`: `space (32)`, `tab (9)`, `line feed (10)`, `carriage return (13)`

## 4. Perfil de Execucao

- Host: toolkit completo suportado (`verbum`, `char`, `args`, `verbum_scan`).
- Freestanding (status em 17A):
  - `cct/args` bloqueado (`módulo 'cct/args' não disponível em perfil freestanding`).
  - `cct/verbum_scan` bloqueado (`módulo 'cct/verbum_scan' não disponível em perfil freestanding`).

## 5. Exemplo Minimo de Scanner

```cct
ADVOCARE "cct/verbum_scan.cct"
ADVOCARE "cct/char.cct"

EVOCA SPECULUM NIHIL c AD CONIURA cursor_init("x1")
SI CONIURA is_alpha(CONIURA cursor_peek(c))
  CONIURA cursor_next(c)
FIN SI
CONIURA cursor_free(c)
```
