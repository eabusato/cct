# CCT Lowering Matrix V0 — CCT to x86-32 ASM Mapping

- Version: V0
- Status: Active (FASE 16A.1)
- Dependency: `CCT_ABI_V0_LBOS.md` V0 and `CCT_SYMBOL_NAMING_V0.md` V0
- Next revision: V1 (when 16C confirms the final implementation path)

## 1. Purpose

This document maps core CCT constructs to the expected x86-32 lowering behavior in the freestanding profile.

It is the implementation reference for:
- runtime-shim expectations in 16B.1;
- ASM emission driver behavior in 16C.1;
- validator checks in 16C.2;
- ELF/link evidence in 16D.2.

## 2. Primitive Types

| CCT type | Host C | Freestanding C | Target size | ELF placement |
|---|---|---|---:|---|
| `REX` | `int64_t` | `int32_t` | 4 | stack / `.text` |
| `DUX` | `uint32_t` | `uint32_t` | 4 | stack / `.text` |
| `COMES` | `float` | `float` | 4 | stack / `.text` |
| `MILES` | `double` | `double` | 8 | stack / `.text` |
| `VERUM` / `FALSUM` | `int` | `int32_t` | 4 | stack / `.text` |
| `VERBUM` literal | `const char*` | `const char*` | 4 | `.rodata` |
| `SPECULUM(T)` | `T*` | `T*` | 4 | stack / `.text` |
| `SERIES T[N]` | `T[N]` | `T[N]` | `N * sizeof(T)` | stack / `.data` |
| `CONSTANS T` | `const T` | `const T` | `sizeof(T)` | `.rodata` |
| `NIHIL` | `void` | `void` | 0 | — |

Critical rule:
- in freestanding x86-32, `REX` lowers to `int32_t`.

## 3. Control Flow

### 3.1 `SI` / `ALITER`

Lowering shape:
```c
if (condition) { ... } else { ... }
```

Expected assembly structure:
```asm
test eax, eax
jz   .L_else_N
; then
jmp  .L_end_si_N
.L_else_N:
; else
.L_end_si_N:
```

### 3.2 `DUM`

Lowering shape:
```c
while (condition) { ... }
```

Expected assembly structure:
```asm
.L_dum_top_N:
  test eax, eax
  jz   .L_dum_end_N
  ; body
  jmp  .L_dum_top_N
.L_dum_end_N:
```

### 3.3 `DONEC ... DUM`

Lowering shape:
```c
do { ... } while (condition);
```

Expected assembly structure:
```asm
.L_donec_top_N:
  ; body
  test eax, eax
  jnz  .L_donec_top_N
```

### 3.4 `REPETE`

Baseline lowering shape:
```c
for (cct_rex_t i = begin; i < end; i++) { ... }
```

Expected assembly pattern:
```asm
mov  [ebp-N], begin
.L_repete_top_M:
  mov  eax, [ebp-N]
  cmp  eax, end
  jge  .L_repete_end_M
  ; body
  inc  dword [ebp-N]
  jmp  .L_repete_top_M
.L_repete_end_M:
```

With explicit step, the increment becomes `i += step`.

### 3.5 `ITERUM`

For fixed-size series/array-like iteration, lowering is equivalent to an indexed loop.

Freestanding restriction:
- `ITERUM` over `FLUXUS` is not supported.

### 3.6 `FRANGE` / `RECEDE`

- `FRANGE` lowers to a jump to the loop exit label.
- `RECEDE` lowers to a jump to the loop continue/increment path.

## 4. Function Calls and Returns

### Simple call

CCT:
```cct
CONIURA soma(a, b)
```

Expected freestanding lowering:
```asm
push b
push a
call cct_fn_modulo_soma
add  esp, 8
```

### Return value

CCT:
```cct
REDDE valor
```

Expected lowering:
```asm
mov eax, valor
ret
```

## 5. Memory and Pointer Operations

### Address-of / dereference

Representative patterns:
- `&x` lowers through `lea` where appropriate;
- `*p` read lowers through load from pointed address;
- `*p = v` lowers through store to pointed address.

Representative assembly:
```asm
lea eax, [ebp-N]   ; address of local x
mov eax, [eax]     ; dereference read
mov [eax], 42      ; dereference write
```

## 6. Global Storage Rules

### Global constants
- module-level `CONSTANS` lowers to `.rodata`.

### String literals
- `VERBUM` literals lower to `.rodata` with deterministic naming such as `cct_str_<mod>_<idx>`.

### Global variables
- freestanding globals follow the `cct_g_<mod>_<name>` convention.

## 7. Supported vs Forbidden Freestanding Surface

### Supported

| Construct | Notes |
|---|---|
| `RITUALE` | lowered with `cdecl` and `cct_fn_*` naming |
| scalar arithmetic | within the supported numeric matrix |
| structured control flow | `SI`, `DUM`, `DONEC`, `REPETE`, bounded iteration |
| simple `SIGILLUM` data | no exception/runtime dependency |
| freestanding-safe imports | only modules compatible with the profile |

### Forbidden

| Construct / module | Reason |
|---|---|
| `TEMPTA`, `IACE`, related exception flow | requires exception runtime |
| host-only modules such as `cct/io`, `cct/fs` | not available on target |
| heap-backed structures outside validated subset | target has no general heap contract |
| unsupported dynamic/runtime-heavy typing paths | outside the validated freestanding subset |

## 8. Forbidden Undefined Symbols

The validator must reject freestanding objects that depend on forbidden undefined symbols, including typical libc/runtime helpers such as:

```text
printf
malloc
free
memcpy
memset
puts
fopen
fclose
__stack_chk_fail
__udivdi3
__divdi3
__muldi3
```

The exact block list is enforced by the phase validator and bridge target.

## 9. Section/Layout Expectations

| Artifact kind | Expected location |
|---|---|
| local variables | stack |
| string literals | `.rodata` |
| global constants | `.rodata` |
| executable code | `.text` |
| mutable global/static data | `.data` / `.bss` where allowed |

## 10. Required Freestanding Toolchain Shape

The canonical freestanding flow is:
1. `./cct --profile freestanding --emit-asm --entry <rituale> file.cct`
2. validate generated `.cgen.s`
3. assemble with `as --32`
4. audit with `nm` / `objdump`
5. link-check with `ld -m elf_i386 -r` when required

## 11. Validation Checklist

A compliant freestanding lowering path must satisfy all of the following:
- emits Intel-syntax GAS assembly with `.intel_syntax noprefix`;
- exports expected `cct_fn_*` entry symbols;
- preserves the V0 type and calling contracts;
- does not introduce forbidden undefined symbols;
- remains consistent with the ABI and symbol-naming contracts.
