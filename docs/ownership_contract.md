# Ownership Contract (FASE 11D.1)

## 1. Core Rule

If code allocates memory through `cct/mem`, that same ownership chain is responsible for releasing it.

## 2. Canonical API Surface (`cct/mem`)

- `alloc(size)` -> raw pointer
- `realloc(ptr, new_size)` -> resized pointer (use returned pointer)
- `free(ptr)` -> releases owned pointer
- `copy(dest, src, size)` -> raw memory copy
- `set(ptr, value, size)` -> raw memory set
- `zero(ptr, size)` -> raw memory zero
- `mem_compare(a, b, size)` -> raw memory compare (`-1`, `0`, `1`)

## 3. Ownership and Lifecycle

- `alloc` transfers ownership to caller.
- `realloc` keeps ownership with caller and may return a different pointer.
- `free` consumes ownership.
- After `free`, pointer usage is invalid (use-after-free).
- Freeing the same pointer twice is invalid (double free).

## 4. Initialization Policy

- Allocated memory is not automatically initialized.
- Use `zero` or `set` when deterministic initial state is required.

## 5. Correct Usage Patterns

```cct
EVOCA SPECULUM REX p
VINCIRE p AD CONIURA alloc(4 * MENSURA(REX))
CONIURA zero(p, 4 * MENSURA(REX))
VINCIRE p AD CONIURA realloc(p, 8 * MENSURA(REX))
CONIURA free(p)
```

## 6. Incorrect Usage Patterns

```cct
# leak
EVOCA SPECULUM REX p
VINCIRE p AD CONIURA alloc(4 * MENSURA(REX))
# missing free(p)

# double free
CONIURA free(p)
CONIURA free(p)

# use-after-free
CONIURA free(p)
VINCIRE p[0] AD 7
```

## 7. Phase Boundary

This contract is the memory foundation for FASE 11D.2/11D.3 (`FLUXUS` storage + API).
