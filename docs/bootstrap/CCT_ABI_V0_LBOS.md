# CCT ABI V0 — CCT ↔ LBOS Interface Contract

- Version: V0
- Status: Active (FASE 16A.1)
- Dependency: FASE 15D.4 closed; LBOS F7.CCT Bridge ABI V0 specified
- Next revision: V1 (when FASE 17 begins auto-bootstrap)

## 1. Scope and Purpose

This document defines the Application Binary Interface (ABI) between CCT code compiled with `--profile freestanding` and the LBOS runtime environment (bare-metal x86-32).

The contract is one-sided from the CCT side: LBOS defines expectations, and CCT must emit code that satisfies them.

## 2. Target Execution Environment

| Property | Value |
|---|---|
| Architecture | x86 32-bit |
| Operating mode | protected-mode near 32-bit |
| Operating system | none (bare-metal) |
| C runtime | absent |
| Dynamic heap | absent |
| Floating point | x87 available; SSE not guaranteed |
| Maximum address | `0x000A0000` |
| Endianness | little-endian |

## 3. Calling Convention

CCT uses `cdecl` as the universal freestanding calling convention:
- arguments are passed on the stack from right to left;
- the caller cleans the stack after the call;
- integer/pointer return values use `EAX`;
- extended 64-bit returns may use `EAX:EDX` if required.

### Register policy

| Class | Registers | Responsibility |
|---|---|---|
| callee-saved | `EBX`, `ESI`, `EDI`, `EBP` | preserved by callee |
| caller-saved | `EAX`, `ECX`, `EDX` | caller must not rely on after call |
| stack pointer | `ESP` | maintained according to cdecl |
| segments | `CS`, `DS`, `SS`, `ES` | left untouched by CCT |

### Stack alignment

- stack must be aligned to 4 bytes immediately before `CALL`;
- generated freestanding C must preserve that contract;
- no SSE-based 16-byte contract is assumed in this profile.

### Standard function shape

Accepted prologue/epilogue forms:

```asm
push ebp
mov  ebp, esp
sub  esp, <locals>
...
mov  esp, ebp
pop  ebp
ret
```

or:

```asm
push ebp
mov  ebp, esp
...
leave
ret
```

## 4. Error Signaling

There is no OS exception runtime in the target environment.

CCT-side rules:
- `cct_fn_*` returns result or error code in `EAX` only;
- `cct_fn_*` does not use CF (`carry flag`) as part of its return contract;
- only low-level `cct_svc_*` wrappers may set/clear CF explicitly.

### Critical limitation

GCC-generated C for x86-32 does not expose a reliable high-level function-return contract based on `stc` / `clc`.

Therefore:
- CCT rituals compiled to `cct_fn_*` use `EAX`-based result/error signaling only;
- `cct_svc_*` wrappers must be written with inline assembly if they need explicit `CF` semantics.

### Service-manifest summary

| Symbol | Arguments | EAX | CF | Failure policy |
|---|---|---|---|---|
| `cct_svc_outb` | port, value | unspecified | always 0 | no failure path |
| `cct_svc_inb` | port | read byte | always 0 | no failure path |
| `cct_svc_halt` | none | no return | no return | fail-stop |
| `cct_svc_memcpy` | dst, src, n | unspecified | 0 if returns | null pointer halts |
| `cct_svc_memset` | dst, val, n | unspecified | 0 if returns | null pointer halts |
| `cct_svc_panic` | msg | no return | no return | fail-stop |

## 5. Type Mapping

| CCT type | Freestanding C type | Size | Target representation |
|---|---|---:|---|
| `REX` | `int32_t` | 4 | signed 32-bit integer |
| `DUX` | `uint32_t` | 4 | unsigned 32-bit integer |
| `COMES` | `float` | 4 | x87-backed single precision |
| `MILES` | `double` | 8 | x87-backed double precision |
| `VERUM` / `FALSUM` | `int32_t` | 4 | `1` / `0` |
| `VERBUM` literal | `const char*` | 4 | 32-bit pointer to `.rodata` |
| `SPECULUM(T)` | `T*` | 4 | 32-bit pointer |
| `SERIES T[N]` | `T[N]` | `N * sizeof(T)` | contiguous block |
| `NIHIL` | `void` | 0 | no representation |
| `CONSTANS T` | `const T` | `sizeof(T)` | `.rodata` when global |

### Critical note on `REX`

In freestanding x86-32, `REX` is `int32_t`, not the wider host representation used elsewhere.

Consequences:
- generated freestanding code must typedef `REX` accordingly;
- values outside the signed 32-bit range are outside the supported target contract.

### Unsupported constructs in freestanding

| Construct | Reason |
|---|---|
| `FLUXUS` | requires heap allocation |
| `IACE` / `TEMPTA` / related exception flow | requires exception runtime / unwinding |
| runtime-dependent dynamic arrays | not part of the target contract |

## 6. Concrete Argument-Passing Example

Example ritual:

```cct
RITUALE soma_tres(REX a, REX b, REX c) REDDE REX
  REDDE a + b + c
EXPLICIT RITUALE
```

Stack at the moment of `CALL cct_fn_modulo_soma_tres`:

```text
[ESP+0]  return address
[ESP+4]  a
[ESP+8]  b
[ESP+12] c
```

The caller pushes `c`, then `b`, then `a`, then performs `call`.

## 7. Hard Constraints Imposed by LBOS

The following rules must never be violated by the CCT freestanding path:
1. no libc symbol may remain undefined in the final object;
2. no CCT compiler binary runs on the target; only emitted artifacts are linked there;
3. the target memory layout is controlled by LBOS, not by CCT;
4. emitted objects must remain compatible with the expected ELF32 toolchain.

## 8. Validation Expectations

At minimum, the freestanding path must be auditable through:
- `nm` undefined-symbol scan;
- `objdump` symbol/section inspection;
- `ld -m elf_i386 -r` link-check step;
- entry-symbol verification against the naming contract.

## 9. References

- `docs/bootstrap/CCT_SYMBOL_NAMING_V0.md`
- `docs/bootstrap/CCT_LOWERING_MATRIX_V0.md`
- `docs/bootstrap/CCT_ASM_SYNTAX_DECISION.md`
- `docs/bootstrap/EVIDENCIA_LINK_16D2.md`
