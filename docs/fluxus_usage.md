# cct/fluxus Usage Guide

`cct/fluxus` is the canonical dynamic-vector module delivered in FASE 11D.3.

Import:

```cct
ADVOCARE "cct/fluxus.cct"
```

## Ownership Model

- `fluxus_init(...)` allocates a vector instance.
- `fluxus_free(...)` must be called exactly once by the caller.
- `fluxus_clear(...)` resets length but keeps allocated capacity.

## API Surface

- `fluxus_init(REX elem_size) -> SPECULUM NIHIL`
- `fluxus_free(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_push(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_pop(SPECULUM NIHIL flux, SPECULUM NIHIL out) -> NIHIL`
- `fluxus_len(SPECULUM NIHIL flux) -> REX`
- `fluxus_get(SPECULUM NIHIL flux, REX idx) -> SPECULUM NIHIL`
- `fluxus_clear(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_reserve(SPECULUM NIHIL flux, REX cap) -> NIHIL`
- `fluxus_capacity(SPECULUM NIHIL flux) -> REX`

## Basic Pattern

```cct
ADVOCARE "cct/fluxus.cct"

RITUALE main() REDDE REX
  EVOCA SPECULUM NIHIL flux
  EVOCA REX x AD 42

  VINCIRE flux AD CONIURA fluxus_init(MENSURA(REX))
  CONIURA fluxus_push(flux, SPECULUM x)

  EVOCA SPECULUM REX p
  VINCIRE p AD CONIURA fluxus_get(flux, 0)

  EVOCA REX out AD *p
  CONIURA fluxus_free(flux)
  REDDE out
EXPLICIT RITUALE
```

## Error Behavior

Runtime failure is explicit for invalid operations:

- `fluxus_init` with non-positive `elem_size`
- `fluxus_pop` on empty vector
- `fluxus_get` out of bounds
- null vector pointer in API calls

## Notes

- Growth strategy is deterministic (`2x`, starting from capacity `8`).
- This subset is intentionally minimal and does not include insert/remove/sort.
- For practical usage see `examples/fluxus_demo.cct`.
