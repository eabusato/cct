# CCT Symbol Naming V0

- Version: V0
- Status: Active (FASE 16A.1)
- Dependency: `CCT_ABI_V0_LBOS.md` V0 and the LBOS F7.CCT Bridge ABI V0
- Next revision: V1 (when FASE 17 begins auto-bootstrap)

## 1. Purpose

This document defines the deterministic symbol-naming contract for all CCT code compiled with `--profile freestanding`.

Goals:
1. avoid collision with LBOS namespaces;
2. preserve determinism for the same module/ritual pair;
3. keep symbol origin traceable;
4. remain compatible with C identifiers and ELF32 symbol rules.

## 2. LBOS-Reserved Prefixes

CCT must never emit freestanding symbols with these prefixes:

| Prefix | Owner | Examples |
|---|---|---|
| `k0_` | LBOS kernel0 | `k0_init`, `k0_page_alloc` |
| `k1_` | LBOS kernel1 | `k1_sched`, `k1_ipc` |
| `svc_` | LBOS services | `svc_video`, `svc_serial` |
| `cct_rt_` | host-only CCT runtime | `cct_rt_fluxus_init` |

## 3. Canonical Naming Patterns

### 3.1 Functions (`RITUALE`)

Pattern:
```text
cct_fn_<mod>_<rituale>
```

Examples:
- `RITUALE kernel_halt()` in module `kernel` -> `cct_fn_kernel_kernel_halt`
- `RITUALE soma(REX a, REX b)` in `math_utils` -> `cct_fn_math_utils_soma`
- `RITUALE main()` in `hello` -> `cct_fn_hello_main`

### 3.2 Compiled modules

Pattern:
```text
cct_mod_<hash8>_<name>
```

Where:
- `hash8` is the first 8 lowercase hex chars of the SHA-256 of the canonical module path;
- `name` is the normalized module name.

Canonical-path rules:
- path is relative to the root of the CCT repository;
- separator is always `/`;
- no absolute path;
- no leading `./`.

Example:
```text
sha256("lib/cct/kernel/kernel.cct") -> a3f9d21b...
cct_mod_a3f9d21b_kernel
```

This symbol may be emitted as optional read-only module metadata in the generated object.

### 3.3 Service shims

Pattern:
```text
cct_svc_<name>
```

These are low-level glue functions with no specific module identity, for example:
- `cct_svc_outb`
- `cct_svc_inb`
- `cct_svc_halt`

### 3.4 Global variables

Pattern:
```text
cct_g_<mod>_<name>
```

Examples:
- `cct_g_kernel_estado_atual`
- `cct_g_boot_cct_contador`

### 3.5 String literals

Pattern:
```text
cct_str_<mod>_<idx>
```

Examples:
- `cct_str_kernel_0`
- `cct_str_kernel_1`

### 3.6 Global constants

Pattern:
```text
cct_const_<mod>_<name>
```

Example:
- `CONSTANS REX VERSAO AD 1` in module `boot_cct` -> `cct_const_boot_cct_VERSAO`

## 4. Normalization Rules

Input: arbitrary module or ritual name.  
Output: valid C identifier using only `[a-z0-9_]`, never starting with a digit.

Algorithm:
1. convert to lowercase;
2. replace any sequence outside `[a-z0-9]` with a single underscore;
3. remove leading/trailing underscores;
4. if the first character is a digit, prefix with `n_`;
5. if the result is empty, use `anon`.

Examples:

| Original | Normalized |
|---|---|
| `kernel` | `kernel` |
| `math_utils` | `math_utils` |
| `boot-cct` | `boot_cct` |
| `MyModule` | `mymodule` |
| `cct/kernel` | `cct_kernel` |
| `my module` | `my_module` |
| `123abc` | `n_123abc` |
| `99` | `n_99` |

Collision rule:
- if two different CCT names normalize to the same identifier, the compiler must warn and disambiguate with a short canonical-path hash suffix:
```text
cct_fn_<mod>_<rituale>__<hash4>
```

## 5. Entry-Point Rules

### 5.1 Explicit freestanding entry (`--entry`)

When the user specifies `--entry <rituale>`:
- that ritual is emitted with its canonical symbol `cct_fn_<mod>_<rituale>`;
- it is marked `.globl` in the generated assembly;
- no `int main(void)` wrapper is generated in freestanding mode.

Example:
```bash
cct compile --profile freestanding --entry kernel_halt lib/cct/kernel/kernel.cct
```

Expected global symbol:
```text
cct_fn_kernel_kernel_halt
```

### 5.2 No explicit entry (pure library mode)

When no `--entry` is specified in freestanding mode:
- all public rituals (without `ARCANUM`) are emitted as `.globl`;
- `ARCANUM` rituals remain object-local.

## 6. Non-Collision Validation

`tools/validate_freestanding_asm.sh` must reject emitted symbols that use LBOS-reserved prefixes.

Manual check:
```bash
nm output.o | awk '$2 == "T" {print $3}' | grep -E '^(k0_|k1_|svc_|cct_rt_)'
```
Expected result: empty output.

## 7. ELF32 Compatibility

All symbols generated under this contract are:
- ASCII-only names using `[a-z0-9_]`;
- valid ELF symbol names for the System V i386 ABI;
- unique within the scope of one compiled module.

## 8. Coherence Checks Against LBOS F7.CCT

Verification points:
- [x] `cct_fn_` does not collide with LBOS namespaces.
- [x] `cct_mod_` does not collide with LBOS namespaces.
- [x] `cct_svc_` does not collide with raw `svc_`.
- [ ] Confirm that the LBOS entry expectation remains `cct_fn_<mod>_<rituale>`.
