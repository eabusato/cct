# CCT — Release Notes: FASE 19

**Date**: March 2026  
**Compiler version**: FASE 19D (post-18D4)  
**Tests**: 1069 passed / 0 failed (full suite)

---

## Summary

FASE 19 adds four major improvements for application-level coding:

- **ELIGE**: pattern-style selection for integers, `VERBUM`, and `ORDO` (`CUM` remains accepted as a legacy alias).
- **FORMA**: string interpolation with format specifiers.
- **ORDO with payload**: sum types (ADTs) with typed fields.
- **Expanded ITERUM**: native iteration over `map` and `set`.

No known blocking bugs remained open for the delivered feature set at phase closure.

---

## 1) ELIGE — Pattern-Style Selection

### What You Gain

- Decision code that is clearer than long `SI`/`ALITER` chains.
- Mandatory exhaustiveness for `ORDO` (no silently missed branch).
- Direct integration with `ORDO` payload via `CASUS Variant(bindings)`.

### Supported Types

- Integers (`REX`, `DUX`, `COMES`, `MILES`)
- `VERBUM`
- `ORDO` (with and without payload)

### Example

```cct
ORDO State
  Active,
  Inactive
FIN ORDO

RITUALE main() REDDE NIHIL
  EVOCA REX code AD 404
  EVOCA State state AD Active

  ELIGE code
    CASUS 200:
      OBSECRO scribe("ok\n")
    CASUS 404:
      OBSECRO scribe("not found\n")
    ALIOQUIN:
      OBSECRO scribe("other\n")
  FIN ELIGE

  ELIGE state
    CASUS Active:
      OBSECRO scribe("active\n")
    CASUS Inactive:
      OBSECRO scribe("inactive\n")
  FIN ELIGE

  REDDE
EXPLICIT RITUALE
```

### Important Rules

- For `ORDO`, omitting `ALIOQUIN` requires full coverage or compilation fails.
- For integer/`VERBUM`, omitting `ALIOQUIN` is accepted with a warning.
- `FRANGE` inside `ELIGE` within a loop exits the outer loop.

---

## 2) FORMA — String Interpolation

### What You Gain

- Message/text construction without large manual concatenation chains.
- Numeric formatting and alignment directly in the literal.
- Natural integration with `OBSECRO scribe(...)`.

### Availability

- **Host-only** in this phase.
- In freestanding, `FORMA` is a compile-time error.

### Common Specifiers

| Spec | Effect |
|---|---|
| `5d` | integer with minimum width 5 |
| `.2f` | real with 2 decimal places |
| `>10` | right alignment |
| `<10` | left alignment |
| `^10` | centered alignment |

### Example

```cct
RITUALE main() REDDE NIHIL
  EVOCA VERBUM name AD "Carlos"
  EVOCA REX age AD 30
  EVOCA UMBRA height AD 1.75

  OBSECRO scribe(FORMA "Name: {name}, Age: {age}\n")
  OBSECRO scribe(FORMA "Height: {height:.2f}m\n")
  OBSECRO scribe(FORMA "Aligned name: {name:>10}\n")

  EVOCA VERBUM msg AD FORMA "Adult status: {age >= 18}"
  OBSECRO scribe(msg, "\n")

  REDDE
EXPLICIT RITUALE
```

---

## 3) ORDO with Payload — Typed ADTs

### What You Gain

- Native typed modeling of result/option-like structures.
- Constructor validation at compile time.
- Direct destructuring through `ELIGE`.

### Supported Payload Types

`REX`, `DUX`, `COMES`, `MILES`, `UMBRA`, `FLAMMA`, `VERUM`, `VERBUM`.

### Example (Result-like Pattern)

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO

RITUALE dividir(REX a, REX b) REDDE Resultado
  SI b == 0
    REDDE Err("division by zero")
  FIN SI
  REDDE Ok(a // b)
EXPLICIT RITUALE

RITUALE main() REDDE NIHIL
  EVOCA Resultado r AD CONIURA dividir(10, 2)

  ELIGE r
    CASUS Ok(v):
      OBSECRO scribe(FORMA "resultado: {v}\n")
    CASUS Err(m):
      OBSECRO scribe(FORMA "error: {m}\n")
  FIN ELIGE

  REDDE
EXPLICIT RITUALE
```

### Idiomatic Reference

- `lib/cct/ordo_samples.cct` includes `Resultado` and `Opcao` as baseline patterns.

---

## 4) Expanded ITERUM — `map` and `set`

### What You Gain

- Direct iteration over collections commonly used in application code.
- Insertion-order iteration for both `map` and `set`.

### Arity Rules

- `map`: 2 bindings (`key`, `value`)
- `set`: 1 binding (`item`)

### Example

```cct
ADVOCARE "cct/map.cct"
ADVOCARE "cct/set.cct"

RITUALE main() REDDE REX
  EVOCA SPECULUM NIHIL mapa AD CONIURA map_init GENUS(REX, REX)()
  EVOCA SPECULUM NIHIL conjunto AD CONIURA set_init GENUS(REX)()

  CONIURA map_insert GENUS(REX, REX)(mapa, 1, 10)
  CONIURA map_insert GENUS(REX, REX)(mapa, 2, 20)
  CONIURA set_insert GENUS(REX)(conjunto, 7)
  CONIURA set_insert GENUS(REX)(conjunto, 9)

  ITERUM chave, valor IN mapa COM
    OBSECRO scribe(FORMA "{chave} -> {valor}\n")
  FIN ITERUM

  ITERUM elem IN conjunto COM
    OBSECRO scribe(FORMA "set: {elem}\n")
  FIN ITERUM

  CONIURA map_free GENUS(REX, REX)(mapa)
  CONIURA set_free GENUS(REX)(conjunto)
  REDDE 0
EXPLICIT RITUALE
```

---

## 5) Quality and Regression

- Full regression executed: `make test`.
- Final result: **1069 passed / 0 failed**.
- No regressions across phases 1 through 18.

---

## 6) Out of Scope (Planned for FASE 20)

- Recursive `ORDO`.
