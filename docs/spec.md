# CCT Language Specification

## Document Purpose

This document is the practical language manual for CCT.

It is written to help you:
- understand the current language surface
- compile and run programs with the current toolchain
- know which constructs are stable, restricted, or reserved

## Status

Specification baseline: **FASE 19D.4** (FASE 13/13M closures, FASE 14 hardening contracts, full FASE 15 closure set, FASE 16 freestanding bridge closure, FASE 17 bootstrap-library expansion, FASE 18 canonical-library expansion, and FASE 19 language-surface expansion/closure).

The language is fully usable in its current subset, with explicit boundaries documented below.

## 1. Compiler Command Reference

### 1.1 Primary Commands

- `./cct --help`
Behavior: print command usage and available options.

- `./cct --version`
Behavior: print compiler version and build status.

- `./cct <file.cct>`
Behavior: parse, run semantic checks, generate sigils, generate `.cgen.c`, invoke host C compiler, produce executable.

- `./cct --profile host|freestanding <file.cct>`
Behavior: select explicit compilation profile (`host` by default, `freestanding` for no-libc target flow).

- `./cct --profile freestanding --emit-asm [--entry <rituale>] <file.cct>`
Behavior: generate freestanding `.cgen.s` using Intel GAS syntax for x86 32-bit flow.

- `./cct --tokens <file.cct>`
Behavior: print token stream.

- `./cct --ast <file.cct>`
Behavior: print AST for the entry file only.

- `./cct --ast-composite <entry.cct>`
Behavior: print composed AST for the full module closure.

- `./cct --check <file.cct>`
Behavior: syntax + semantic validation only.

- `./cct --sigilo-only <file.cct>`
Behavior: emit sigilo artifacts without executable generation.

- `./cct sigilo inspect <artifact.sigil>`
Behavior: inspect parsed sigilo metadata in text or structured formats.

- `./cct sigilo validate <artifact.sigil> [--strict] [--consumer-profile legacy-tolerant|current-default|strict-contract]`
Behavior: run canonical strict/tolerant validation engine and emit normalized violation report.

- `./cct sigilo diff <left.sigil> <right.sigil>`
Behavior: structural/semantic diff with severity classification.

- `./cct sigilo check <left.sigil> <right.sigil> [--strict]`
Behavior: same as diff with CI-friendly exit code policy (`2` for strict blocking severities).

- `./cct sigilo baseline check <artifact.sigil> [--baseline PATH] [--strict]`
Behavior: compare current artifact against persisted project baseline without mutating files.

- `./cct sigilo baseline update <artifact.sigil> [--baseline PATH] [--force]`
Behavior: explicitly write/update baseline artifacts and metadata (never implicit).

- `./cct fmt <file1.cct> [file2.cct ...]`
Behavior: format one or more CCT files in place.

- `./cct fmt --check <file1.cct> [file2.cct ...]`
Behavior: formatting check mode only; returns exit code `2` when any file is not formatted.

- `./cct fmt --diff <file1.cct> [file2.cct ...]`
Behavior: print formatter diff without rewriting files.

- `./cct lint <file.cct>`
Behavior: run canonical lint rule set and emit warnings.

- `./cct lint --strict <file.cct>`
Behavior: return exit code `2` when warnings are present.

- `./cct lint --fix <file.cct>`
Behavior: apply only deterministic safe fixes for selected lint rules.

- `./cct build [--project DIR] [--entry FILE.cct] [--release] [--out PATH]`
Behavior: build a project using canonical discovery (`cct.toml` or `src/main.cct`) and produce dist artifact.

- `./cct run [--project DIR] [--entry FILE.cct] [--release] [-- --args]`
Behavior: build project then execute binary, forwarding exit code.

- `./cct test [PATTERN] [--project DIR] [--strict-lint] [--fmt-check]`
Behavior: discover and execute `*.test.cct` under `tests/`.

- `./cct bench [PATTERN] [--project DIR] [--iterations N] [--release]`
Behavior: discover and execute `*.bench.cct` under `bench/`, reporting average and total runtime.

Exit code policy (FASE 14A.2):
- `0`: success
- `1`: invalid argument
- `2`: contract violation gate (strict checks, strict lint/docs, strict sigilo gates)
- `3`: missing required argument
- `4`: unknown command/option

- `./cct clean [--project DIR] [--all]`
Behavior: clean `.cct` project artifacts; `--all` also removes generated project binaries in `dist/`.

- `./cct doc [--project DIR] [--entry FILE] [--output-dir DIR] [--format markdown|html|both]`
Behavior: generate deterministic API docs for module closure (`docs/api` by default).

### 1.2 Sigilo Options

- `--sigilo-style network|seal|scriptum`
- `--sigilo-mode essencial|completo`
- aliases: `essential|complete`
- `--sigilo-out <basepath>`
- `--sigilo-no-meta`
- `--sigilo-no-svg`

### 1.3 Input Constraints

- input must be a `.cct` file
- one positional input file per invocation

### 1.4 Freestanding Profile (FASE 16)

The `freestanding` profile enables the CCT-to-LBOS bridge flow and keeps target code independent from host libc/runtime assumptions.

Canonical flags:
- `--profile freestanding`
- `--emit-asm`
- `--entry <rituale>` (freestanding only)

Profile contract:
- `host` remains default and preserves legacy behavior from phases 0-15.
- `freestanding` enforces semantic/codegen restrictions required by ABI V0.
- `--emit-asm` operates in freestanding mode and emits `.cgen.s`.
- `--entry` is valid only in freestanding and chooses exported `cct_fn_<mod>_<entry>`.

Freestanding module policy:
- `cct/kernel` is supported only in freestanding.
- `cct/io`, `cct/fs`, and dynamic runtime-heavy modules remain blocked in freestanding.
- `cct/kernel` must be rejected in host profile.

Supported freestanding surface (summary):
- scalar arithmetic and control flow subset used by the bridge;
- bitwise operators and static data forms validated in FASE 16;
- kernel service lowering via `cct/kernel` wrappers.

Restricted/prohibited freestanding surface (summary):
- host I/O/filesystem modules (`cct/io`, `cct/fs`);
- heap/dynamic structures outside validated subset;
- dynamic `GENUS/PACTUM` paths;
- unsupported soft-float target paths (`COMES`/`MILES` keep warning-only diagnostics where applicable).

Bridge command reference:
- `make lbos-bridge`
Behavior: runs the canonical bridge pipeline and publishes `build/lbos-bridge/cct_kernel.o` for LBOS-side consumption.

### 1.5 FASE 17 Library Expansion

FASE 17 expands the canonical standard library to unlock bootstrap-oriented compiler/tooling workflows in CCT.

17A - Lexer and CLI foundation:
- `cct/verbum`: `char_at`, `from_char`
- `cct/char`: ASCII classification helpers
- `cct/args`: process argument access
- `cct/verbum_scan`: cursor-based text scanning

17B - Efficient textual construction:
- `cct/verbum_builder`: mutable text builder API
- `cct/code_writer`: deterministic writer with indentation/newline control
- `cct/fmt` integration paths for formatted append/write flows

17C - Variant and host-side AST toolkit:
- `cct/variant`
- `cct/variant_helpers`
- `cct/ast_node`
- ORDO payload language support is stable in FASE 19 (`QUANDO` destructuring + payload constructors)

17D - Host utility libraries:
- `cct/env`: environment variable and cwd access
- `cct/time`: monotonic timing and sleep primitives
- `cct/bytes`: mutable byte buffer primitives with bounds/range contracts

### 1.6 FASE 18 Canonical Library Expansion

FASE 18 expands and consolidates Bibliotheca Canonica with production-ready host utilities, richer text/parsing APIs, collection helpers, and low-level modules for process/hash/bit flows.

18A - text/format/parse expansion:
- `cct/verbum`: starts/ends, strip, replace, case conversion, trim variants, padding, slicing, split/join, lines/words.
- `cct/fmt`: radix formatting, precision controls, templating (`format_1..4`), table helpers.
- `cct/parse`: safe parsers (`try_*`) and CSV/radix helpers.

18B - host I/O and filesystem/path expansion:
- `cct/fs`: mutation, inspection, directory listing, temp resources, metadata helpers.
- `cct/io`: stderr variants, flush, stdin aggregate read, tty detection.
- `cct/path`: normalize/resolve/relative, stem/ext transforms, split utilities.

18C - collection and algorithm expansion:
- `cct/fluxus`, `cct/set`, `cct/map`: richer mutation/iteration/composition helpers.
- `cct/alg`, `cct/series`: sorting, min/max, reductions, rotate/reverse and counting.

18D - new low-level/capability modules:
- `cct/process` (new): host command execution and capture/timeout variants.
- `cct/hash` (new): djb2/fnv1a/crc32/murmur3 + combine.
- `cct/bit` (new): bit access/manipulation, popcount/zeros, rotate/swap/power-of-2 helpers.
- `cct/random`: bool/range/string/bytes/shuffle helpers.

### 1.7 FASE 19 Language Surface Expansion

FASE 19 closes a major expressiveness gap in core language syntax and semantics.

19A - selection/pattern statement:
- `QUANDO`/`CASO`/`SENAO` for integer, `VERBUM`, and `ORDO` dispatch.
- ORDO exhaustiveness enforcement when `SENAO` is absent.

19B - interpolated string expression:
- `MOLDE "..."` with `{expr}` interpolation and formatting specifiers.
- host-only contract in current subset.

19C - payload-capable sum types:
- `ORDO` supports payload variants with compile-time constructor validation.
- payload destructuring through `QUANDO ... CASO Variante(bindings): ...`.

19D - collection iteration expansion:
- `ITERUM key, value IN map COM ... FIN ITERUM`
- `ITERUM item IN set COM ... FIN ITERUM`
- insertion-order iteration contract for map/set.

## 2. Source File Structure

A valid CCT source file uses `INCIPIT ... EXPLICIT grimoire`.

Example:

```cct
INCIPIT grimoire "hello"

RITUALE main() REDDE REX
  OBSECRO scribe("Ave Mundi!\n")
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

## 3. Top-Level Declarations

### 3.1 `ADVOCARE` (module import)

Syntax:

```cct
ADVOCARE "relative/path/to/module.cct"
```

Rules:
- path is resolved relative to the importer file
- import graph is deduplicated by canonical path
- import cycles are rejected
- visibility and direct-import rules apply (see Module section)

### 3.2 `ARCANUM` (internal visibility)

Syntax:

```cct
ARCANUM RITUALE ...
ARCANUM SIGILLUM ...
ARCANUM ORDO ...
```

Rules:
- only valid on top-level `RITUALE`, `SIGILLUM`, or `ORDO`
- marks symbol as internal to its module

### 3.3 `RITUALE` (function)

Syntax:

```cct
RITUALE name(<params>) REDDE <type>
  ...
EXPLICIT RITUALE
```

Notes:
- parameter list is optional
- return type is optional; if omitted, semantic defaults apply
- entry ritual can be `main` or `principium` with zero parameters in the executable subset

### 3.4 `SIGILLUM` (struct-like type)

Syntax:

```cct
SIGILLUM Name
  field: Type
  ...
FIN SIGILLUM
```

With contract conformance:

```cct
SIGILLUM Name PACTUM ContractName
  ...
FIN SIGILLUM
```

### 3.5 `ORDO` (enum-like type with optional payload)

Status:
- Stable (FASE 19C payload model)

Syntax (simple variants):

```cct
ORDO Status
  QUIETUS,
  ACTUS = 10
FIN ORDO
```

Syntax (payload variants):

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO
```

Normative grammar (payload extension):

```text
ordo_decl    := ORDO IDENT variant_list FIN ORDO
variant_list := variant (',' variant)*
variant      := IDENT
              | IDENT '(' field_list ')'
field_list   := field (',' field)*
field        := type IDENT
```

Semantics:
- payload-capable ORDO is a tagged-sum type lowered to a C tagged union form.
- payload fields are variant-local and are only available after matching a variant in `QUANDO`.
- constructors are resolved by target-context type and validated by variant arity and field types.

Payload contract (current stable subset):
- allowed payload field types: `REX`, `DUX`, `COMES`, `MILES`, `UMBRA`, `FLAMMA`, `VERUM`, `VERBUM`
- mixed variants (with and without payload) are supported
- constructor arity/type is validated at compile time
- payload destructuring is supported only via `QUANDO ... CASO Variant(bindings): ...`

Current restrictions:
- recursive ORDO payloads are not part of the stable subset
- `GENUS` over payload ORDO declarations is not part of the stable subset
- shared-body OR-cases with payload bindings (`CASO A:` followed by `CASO B:`) are not supported

Example:

```cct
ORDO Resultado
  Ok(REX valor),
  Err(VERBUM msg)
FIN ORDO

RITUALE dividir(REX a, REX b) REDDE Resultado
  SI b == 0
    REDDE Err("divisao por zero")
  FIN SI
  REDDE Ok(a // b)
EXPLICIT RITUALE
```

### 3.6 `PACTUM` (contract/interface)

Syntax:

```cct
PACTUM Numerabilis
  RITUALE valor(REX x) REDDE REX
FIN PACTUM
```

Current conformance model:
- `SIGILLUM X PACTUM C` declares explicit conformance
- implementation is validated through top-level rituals matching required signatures under the current subset contract

### 3.7 `CODEX` (namespace block)

Syntax:

```cct
CODEX Name
  ...
FIN CODEX
```

Status:
- parsed and semantically traversed
- not part of the stable executable subset for direct codegen workflows

## 4. Type System (Current Subset)

### 4.1 Primitive Types

- `REX` (int64)
- `DUX` (int32)
- `COMES` (int16)
- `MILES` (uint8)
- `UMBRA` (double)
- `FLAMMA` (float)
- `VERBUM` (string)
- `VERUM` (boolean type/literal family)
- `NIHIL` (void/null family)
- `FRACTUM` (failure payload type in failure-control subset)

### 4.2 Pointer and Array Forms

- pointer form: `SPECULUM T`
- dereference operator: `*p`
- address-of operator: `SPECULUM x`
- static array form: `SERIES T [N]`

### 4.3 Size Query

- `MENSURA(Type)` returns size as integer

### 4.4 Type Modifiers: `CONSTANS`

Syntax:

```cct
EVOCA CONSTANS T nome AD expr
```

Semantic contract:
- a `CONSTANS` binding cannot be reassigned through `VINCIRE`
- applies to local variables and `RITUALE` parameters
- reassignment attempts are semantic errors with source location

Code generation contract:
- `CONSTANS` bindings are emitted with `const` in generated C
- this provides defense-in-depth with host C compiler diagnostics

`CONSTANS SPECULUM T` contract:
- the pointer binding is constant (cannot be rebound to another address)
- the pointed value can still be mutated through dereference when allowed by type/rules

### 4.5 Generic Types and Params (`GENUS`)

Type parameter declaration:

```cct
RITUALE identitas GENUS(T)(T x) REDDE T
```

Generic type instantiation:

```cct
EVOCA Capsa GENUS(REX) c
```

Generic call instantiation (explicit only):

```cct
CONIURA identitas GENUS(REX)(7)
```

## 5. Statement Reference

### 5.1 `EVOCA` (declaration)

Syntax:

```cct
EVOCA <Type> name
EVOCA <Type> name AD <expr>
```

### 5.2 `VINCIRE` (assignment)

Syntax:

```cct
VINCIRE <target> AD <expr>
```

Targets supported in the current subset:
- identifiers
- field access
- index access
- pointer dereference

### 5.3 `REDDE` (return)

Syntax:

```cct
REDDE
REDDE <expr>
```

### 5.4 `ANUR` (process exit)

Syntax:

```cct
ANUR <int-expr>
```

### 5.5 `SI / ALITER` (if/else)

Syntax:

```cct
SI <condition>
  ...
ALITER
  ...
FIN SI
```

`ALITER` is optional.

### 5.6 `DUM` (while)

Syntax:

```cct
DUM <condition>
  ...
FIN DUM
```

### 5.7 `DONEC` (do-while)

Syntax:

```cct
DONEC
  ...
DUM <condition>
```

### 5.8 `REPETE` (for-range)

Syntax:

```cct
REPETE i DE <start> AD <end>
  ...
FIN REPETE
```

Optional step:

```cct
REPETE i DE <start> AD <end> GRADUS <step>
  ...
FIN REPETE
```

### 5.9 `ITERUM` (collection iterator)

#### 5.9.1 Core Form

Syntax:

```cct
ITERUM item IN collection COM
  ...
FIN ITERUM
```

Rules:
- `COM` is part of the canonical surface syntax.
- iterator variable scope is local to the `ITERUM` body.
- lazy iterator protocols are outside the current stable subset.

#### 5.9.2 `ITERUM` over `FLUXUS` and `SERIES`

Status:
- Stable

Collection coverage:
- `FLUXUS` values
- `SERIES T[N]` static arrays
- collection-operation results that materialize as `FLUXUS`/`SERIES`

Iteration semantics:
- exactly one binding symbol is required.
- traversal order is index order.

#### 5.9.3 `ITERUM` over `map` and `set` (FASE 19D)

Status:
- Stable (FASE 19D)

`ITERUM` supports canonical `map` and `set` values from Bibliotheca Canonica.

Map form (requires 2 bindings):

```cct
ITERUM chave, valor IN mapa COM
  OBSECRO scribe(MOLDE "{chave} -> {valor}\n")
FIN ITERUM
```

Set form (requires 1 binding):

```cct
ITERUM item IN conjunto COM
  OBSECRO scribe(MOLDE "{item}\n")
FIN ITERUM
```

Arity and ordering rules:
- `map`: exactly 2 bindings (`key`, `value`)
- `set`: exactly 1 binding
- map/set iteration order is insertion order
- arity mismatch is a compile-time error

Iterable collections summary:

| Collection | Arity | Ordering semantics |
|---|---:|---|
| `FLUXUS` | 1 | index order |
| `SERIES` | 1 | index order |
| `map` | 2 | insertion order (`key`, `value`) |
| `set` | 1 | insertion order |

### 5.10 `DIMITTE` (explicit release)

Syntax:

```cct
DIMITTE pointer_symbol
```

### 5.11 Failure Control: `TEMPTA`, `CAPE`, `SEMPER`, `IACE`

Throw:

```cct
IACE "message"
```

Try/catch:

```cct
TEMPTA
  ...
CAPE FRACTUM erro
  ...
FIN TEMPTA
```

Try/catch/finally:

```cct
TEMPTA
  ...
CAPE FRACTUM erro
  ...
SEMPER
  ...
FIN TEMPTA
```

Subset constraints:
- exactly one `CAPE` in current official subset
- `SEMPER` must come after `CAPE`

### 5.12 `QUANDO` (pattern-style selection)

Status:
- Stable (FASE 19A/C payload integration)

Normative grammar:

```text
quando_stmt   := QUANDO expr caso+ (SENAO ':' stmt_list)? FIN QUANDO
caso          := CASO literal ':' stmt_list
               | CASO variant_name '(' binding_list ')' ':' stmt_list
binding_list  := IDENT (',' IDENT)*
```

Semantics:
- `QUANDO` evaluates `expr` once and executes the first matching `CASO`.
- when no `CASO` matches:
  - if `SENAO` exists, `SENAO` executes;
  - if `SENAO` is absent and scrutinee type is `ORDO`, exhaustiveness diagnostics apply.
- OR-cases are represented by multiple consecutive `CASO` labels sharing one body.
- for `ORDO` with payload, bindings are local to the matched `CASO` block.

Exhaustiveness rules:
- `QUANDO` over `ORDO` (with or without payload): exhaustive coverage is required unless `SENAO` is present.
- `QUANDO` over integer or `VERBUM`: missing `SENAO` is accepted with warning-oriented diagnostics.

Restrictions:
- `QUANDO` is a statement, not an expression.
- OR-cases with payload bindings in a shared body are not supported.
- nested payload destructuring patterns are not supported in the stable subset.
- `FRANGE` inside `QUANDO` nested in a loop exits the enclosing loop, not the `QUANDO`.

Examples:

```cct
QUANDO x
  CASO 1:
  CASO 2:
    OBSECRO scribe("pequeno\n")
  CASO 3:
    OBSECRO scribe("medio\n")
  SENAO:
    OBSECRO scribe("grande\n")
FIN QUANDO
```

```cct
QUANDO resultado
  CASO Ok(v):
    OBSECRO scribe(MOLDE "valor: {v}\n")
  CASO Err(msg):
    OBSECRO scribe(MOLDE "erro: {msg}\n")
FIN QUANDO
```

### 5.13 `FRANGE`, `RECEDE`, `TRANSITUS`

Status:
- `FRANGE` and `RECEDE`: Stable; valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM`
- `TRANSITUS`: parsed and represented in AST; currently outside the stable executable subset in codegen

## 6. Expression Reference

### 6.1 Literals

- integer: `42`
- real: `3.14`
- string: `"text"`
- boolean: `VERUM`, `FALSUM`
- null-like: `NIHIL`

### 6.2 Calls

- preferred ritual call form for executable subset:

```cct
CONIURA nome(...)
```

- generic call form:

```cct
CONIURA nome GENUS(T1, T2)(...)
```

- direct call expressions are not the recommended executable path; use `CONIURA`

### 6.3 Field and Index Access

- field: `obj.field`
- index: `arr[i]`, `ptr[i]` (with subset rules)

### 6.4 Unary Operators

- `+x`, `-x`
- `NON x`
- `NON_BIT x`
- `*p`
- `SPECULUM x` (address-of)

### 6.5 Binary Operators

Arithmetic and comparison are stable in executable subset:
- `+`, `-`, `*`, `/`, `%`, `**`, `//`, `%%`
- `==`, `!=`, `<`, `<=`, `>`, `>=`

Logical/bitwise/shift forms:
- `ET`, `VEL` (stable in executable subset)
- `ET_BIT`, `VEL_BIT`, `XOR`
- `SINISTER`, `DEXTER`

Executable support note:
- arithmetic/comparison, logical `ET`/`VEL`, and bitwise/shift forms are stable codegen paths
- bitwise/shift operators require integer operands

13M arithmetic addendum:
- `**` is exponentiation and is right-associative (`2 ** 3 ** 2` = `2 ** (3 ** 2)`).
- `//` is floor integer division (requires integer operands).
- `%%` is Euclidean modulo (requires integer operands), coherent with `//`.
- `%` remains available with historical behavior for compatibility.

### 6.6 Precedence and Associativity (Stable Executable Core)

From higher to lower priority in arithmetic/comparison core:

1. unary: `+x`, `-x`, `NON x`, `*p`, `SPECULUM x`
2. power: `**` (right-associative)
3. multiplicative: `*`, `/`, `%`, `//`, `%%`
4. additive: `+`, `-`
5. comparison/equality: `==`, `!=`, `<`, `<=`, `>`, `>=`

Examples:
- `2 + 3 ** 2 * 4` => `2 + ((3 ** 2) * 4)`
- `20 // 3 + 1` => `(20 // 3) + 1`
- `20 // (3 + 1)` => `20 // 4`

### 6.8 `MOLDE` (string interpolation)

Status:
- Stable (FASE 19B)
- Host-only (rejected in freestanding profile)

Normative grammar:

```text
molde_expr      := MOLDE string_literal
interpolation   := '{' expr (':' format_spec)? '}'
format_spec     := [width] ['.' precision] ['d'|'f'|'s'|'<'|'>'|'^']
width           := DIGIT+
precision       := DIGIT+
```

Canonical form:

```cct
MOLDE "texto {expr} mais {expr:spec}"
```

Supported interpolation payload types:
- `REX`, `DUX`, `COMES`, `MILES`
- `UMBRA`, `FLAMMA`
- `VERUM`
- `VERBUM`

Formatting notes:
- default `{expr}` formatting is type-aware
- optional `:spec` supports numeric and alignment forms in the current subset
- escaped braces use `{{` and `}}`
- representative specs: `5d`, `.2f`, `<10`, `>10`, `^10`

Constraints:
- `OBSECRO` calls are not allowed inside `{...}` interpolation expressions
- `MOLDE` returns a `VERBUM` value
- as direct argument to output builtins, runtime ownership is automatically handled by generated code

Examples:

```cct
EVOCA VERBUM nome AD "Maria"
EVOCA REX pontos AD 95
EVOCA UMBRA media AD 8.75

OBSECRO scribe(MOLDE "Aluno: {nome}, Pontos: {pontos}\n")
OBSECRO scribe(MOLDE "Media: {media:.2f}\n")

EVOCA VERBUM relatorio AD MOLDE "Aluno: {nome} ({pontos:3d} pts)\n"
OBSECRO scribe(relatorio)
```

## 7. `OBSECRO` Builtins

### 7.1 Stable in current executable subset

- `OBSECRO scribe(...)`
Prints values. Supports mixed scalar/string/real forms in current subset.

- `OBSECRO pete(size_expr)`
Allocates memory and returns pointer expression value.

- `OBSECRO libera(ptr_expr)`
Releases memory when used as a statement.

### 7.2 Recognized but currently restricted in executable subset

- `OBSECRO lege(...)`
- `OBSECRO aperi(...)`
- `OBSECRO claude(...)`

These names are recognized in semantic builtin tables, but executable support is not fully part of the stable subset.

## 8. Modules and Visibility

### 8.1 Import and Resolution

- imports are declared with `ADVOCARE "path.cct"`
- no implicit transitive symbol visibility
- duplicate global symbol declarations across module closure are rejected
- import cycles are rejected
- reserved `cct/...` imports are resolved from the canonical Bibliotheca Canonica root
- stdlib root resolution order for `cct/...` imports:
  1. `CCT_STDLIB_DIR` environment variable (when set)
  2. build-time canonical stdlib path

### 8.2 Bibliotheca Canonica Namespace

Canonical stdlib modules use the reserved namespace:

```cct
ADVOCARE "cct/verbum.cct"
ADVOCARE "cct/io.cct"
```

FASE 11A policy:
- `cct/...` is reserved for Bibliotheca Canonica
- canonical modules resolve from compiler-distributed stdlib path
- user-local modules do not override canonical `cct/...` targets

### 8.2.1 Bibliotheca Canonica 18 (Delta from 17D.4)

The table below lists FASE 18 additions by module (`+N` = functions added in the phase).

| Module | +N | Added API surface (FASE 18) |
|---|---:|---|
| `cct/verbum` | +28 | `starts_with`, `ends_with`, `strip_prefix`, `strip_suffix`, `replace`, `replace_all`, `to_upper`, `to_lower`, `trim_left`, `trim_right`, `trim_char`, `repeat`, `pad_left`, `pad_right`, `center`, `last_find`, `find_from`, `count_occurrences`, `reverse`, `is_empty`, `equals_ignore_case`, `slice`, `is_ascii`, `split`, `split_char`, `join`, `lines`, `words` |
| `cct/fmt` | +16 | `stringify_int_hex`, `stringify_int_hex_upper`, `stringify_int_oct`, `stringify_int_bin`, `stringify_uint`, `stringify_int_padded`, `stringify_real_prec`, `stringify_real_sci`, `stringify_real_fixed`, `stringify_char`, `format_1`, `format_2`, `format_3`, `format_4`, `repeat_char`, `table_row` |
| `cct/parse` | +12 | `try_int`, `try_real`, `try_bool`, `parse_int_hex`, `try_int_hex`, `parse_int_radix`, `try_int_radix`, `is_int`, `is_real`, `parse_lines`, `parse_csv_line`, `parse_csv_line_sep` |
| `cct/fs` | +21 | `mkdir`, `mkdir_all`, `delete_file`, `delete_dir`, `rename`, `copy`, `move`, `is_file`, `is_dir`, `is_symlink`, `is_readable`, `is_writable`, `modified_time`, `chmod`, `list_dir`, `read_lines`, `create_temp_file`, `create_temp_dir`, `truncate`, `symlink`, `same_file` |
| `cct/io` | +14 | `print_bool`, `print_char`, `print_hex`, `eprint`, `eprintln`, `eprint_int`, `eprint_real`, `eprint_bool`, `flush`, `flush_err`, `read_char`, `read_all_stdin`, `read_line_prompt`, `is_tty` |
| `cct/path` | +12 | `stem`, `normalize`, `is_absolute`, `is_relative`, `resolve`, `relative_to`, `with_ext`, `without_ext`, `parent`, `home_dir`, `temp_dir`, `split_path` |
| `cct/fluxus` | +13 | `fluxus_peek`, `fluxus_set`, `fluxus_remove`, `fluxus_insert`, `fluxus_contains`, `fluxus_is_empty`, `fluxus_concat`, `fluxus_slice`, `fluxus_copy`, `fluxus_reverse`, `fluxus_sort_int`, `fluxus_sort_verbum`, `fluxus_to_ptr` |
| `cct/set` | +11 | `set_union`, `set_intersection`, `set_difference`, `set_symmetric_difference`, `set_is_subset`, `set_is_superset`, `set_equals`, `set_copy`, `set_to_fluxus`, `set_reserve`, `set_capacity` |
| `cct/map` | +6 | `map_get_or_default`, `map_update_or_insert`, `map_copy`, `map_keys`, `map_values`, `map_merge` |
| `cct/alg` | +19 | `alg_sum`, `alg_sum_real`, `alg_min`, `alg_max`, `alg_min_real`, `alg_max_real`, `alg_reverse_range`, `alg_reverse`, `alg_fill`, `alg_fill_real`, `alg_rotate`, `alg_quicksort`, `alg_mergesort`, `alg_sort_verbum`, `alg_is_sorted`, `alg_count`, `alg_deduplicate_sorted`, `alg_dot_product`, `alg_dot_product_real` |
| `cct/series` | +7 | `series_sum`, `series_sum_real`, `series_min`, `series_max`, `series_is_sorted`, `series_sort`, `series_count_val` |
| `cct/random` | +8 | `random_real_unit`, `random_bool`, `random_real_range`, `random_verbum`, `random_verbum_from`, `random_choice_int`, `shuffle_int`, `random_bytes` |
| `cct/process` | +6 (new) | `run`, `run_capture`, `run_capture_err`, `run_with_input`, `run_env`, `run_timeout` |
| `cct/hash` | +6 (new) | `djb2`, `fnv1a`, `fnv1a_bytes`, `crc32`, `murmur3`, `combine` |
| `cct/bit` | +14 (new) | `bit_get`, `bit_set`, `bit_clear`, `bit_toggle`, `popcount`, `leading_zeros`, `trailing_zeros`, `rotate_left`, `rotate_right`, `next_power_of_2`, `is_power_of_2`, `byte_swap`, `parity`, `bit_extract` |

Notes:
- `cct/parse` safe functions (`try_*`) return Option-style opaque pointers to avoid hard-fail paths in host tooling flows.
- Values above are phase deltas, not total historical module size.

### 8.2.2 New Modules Introduced in FASE 18

- `cct/process`:
  - wraps host process execution with explicit contracts for capture/input/env/timeout.
  - restricted to host profile; not available in freestanding.

- `cct/hash`:
  - provides deterministic hashes for strings/bytes and composition (`combine`) for structural fingerprints.
  - `combine` follows the canonical mix strategy used by the phase architecture.

- `cct/bit`:
  - offers canonical bit-level helpers for AST/IR/tooling workflows that need compact flags and bitfield manipulation.
  - index-sensitive operations enforce `0..63` bounds with canonical diagnostics.

### 8.2.3 Full Canonical API Index (FASE 19D.4)

- This specification includes a complete canonical API inventory in section `16`.
- The same inventory is mirrored in `docs/bibliotheca_canonica.md` (section `32`) for library-first navigation.
- Inventory generation source: declarations `RITUALE` from `lib/cct/*.cct` and `lib/cct/kernel/*.cct`.

### 8.3 Visibility

- top-level declarations are public by default
- `ARCANUM` marks declarations as internal to their module
- external access to internal symbols is rejected

### 8.4 Canonical Text Module (`cct/verbum`)

Import:

```cct
ADVOCARE "cct/verbum.cct"
```

Available operations in the 11B.1 subset:
- `len(s)` -> `REX`
- `concat(a, b)` -> `VERBUM`
- `compare(a, b)` -> `REX` (`-1`, `0`, `1`)
- `substring(s, inicio, fim)` -> `VERBUM` (strict bounds)
- `trim(s)` -> `VERBUM`
- `contains(s, sub)` -> `VERUM`
- `find(s, sub)` -> `REX` (`-1` when missing)

### 8.5 Canonical Formatting Module (`cct/fmt`)

Import:

```cct
ADVOCARE "cct/fmt.cct"
```

Available operations in the 11B.2 subset:
- stringify: `stringify_int`, `stringify_real`, `stringify_float`, `stringify_bool`
- parse façade: `fmt_parse_int`, `fmt_parse_real`, `fmt_parse_bool`
- composition: `format_pair`

Subset behavior:
- stringify produces textual values suitable for display/logging
- `parse_int` / `parse_real` are strict and fail on invalid textual input

### 8.6 Canonical Static-Collection Module (`cct/series`)

Import:

```cct
ADVOCARE "cct/series.cct"
```

Available operations in the 11C subset:
- `series_len GENUS(T)(SPECULUM T arr, REX tamanho) -> REX`
- `series_fill GENUS(T)(SPECULUM T arr, T valor, REX tamanho) -> NIHIL`
- `series_copy GENUS(T)(SPECULUM T dest, SPECULUM T src, REX tamanho) -> NIHIL`
- `series_reverse GENUS(T)(SPECULUM T arr, REX tamanho) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX tamanho) -> VERUM`

Subset behavior:
- `series_fill`, `series_copy`, and `series_reverse` are generic mutation helpers
- `series_contains` is integer-focused in this subset (`REX`)
- caller provides explicit `tamanho` and is responsible for shape correctness

### 8.7 Canonical Baseline Algorithms Module (`cct/alg`)

Import:

```cct
ADVOCARE "cct/alg.cct"
```

Available operations in the 11F.2 subset:
- `alg_linear_search(SPECULUM REX arr, REX valor, REX tamanho) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX tamanho) -> VERUM`
- `alg_binary_search(SPECULUM REX arr, REX tamanho, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX tamanho) -> NIHIL`

Subset behavior:
- algorithms are intentionally small and deterministic
- both routines target integer arrays in this initial subset
- binary search expects sorted input
- insertion sort is in-place and intentionally minimal

### 8.8 Canonical Memory Utility Module (`cct/mem`)

Import:

```cct
ADVOCARE "cct/mem.cct"
```

Available operations in the 11D.1 subset:
- `alloc(size)` -> pointer
- `free(ptr)` -> `NIHIL`
- `realloc(ptr, new_size)` -> pointer
- `copy(dest, src, size)` -> `NIHIL`
- `set(ptr, value, size)` -> `NIHIL`
- `zero(ptr, size)` -> `NIHIL`
- `mem_compare(a, b, size)` -> `REX` (`-1`, `0`, `1`)

Subset behavior:
- ownership is explicit: allocator side must release with `free`
- memory API is runtime-backed and deterministic
- this is the foundation for dynamic collections in the 11D block

### 8.9 Canonical Dynamic-Vector Module (`cct/fluxus`)

Import:

```cct
ADVOCARE "cct/fluxus.cct"
```

Available operations in the 11D.3 subset:
- `fluxus_init(elem_size)` -> pointer
- `fluxus_free(flux)` -> `NIHIL`
- `fluxus_push(flux, elem_ptr)` -> `NIHIL`
- `fluxus_pop(flux, out_ptr)` -> `NIHIL`
- `fluxus_len(flux)` -> `REX`
- `fluxus_get(flux, index)` -> pointer
- `fluxus_clear(flux)` -> `NIHIL`
- `fluxus_reserve(flux, capacity)` -> `NIHIL`
- `fluxus_capacity(flux)` -> `REX`

Subset behavior:
- explicit ownership: caller must release instances with `fluxus_free`
- deterministic growth strategy (x2, initial growth bucket of 8)
- strict runtime failure for invalid operations (null instance, pop empty, out-of-bounds get)

### 8.10 Canonical IO Module (`cct/io`)

Import:

```cct
ADVOCARE "cct/io.cct"
```

Available operations in the 11E.1 subset:
- `print(s)` -> `NIHIL`
- `println(s)` -> `NIHIL`
- `print_int(n)` -> `NIHIL`
- `read_line()` -> `VERBUM`

Subset behavior:
- runtime-backed stdout/stderr bridge with deterministic textual output semantics
- `read_line` reads one stdin line and returns a caller-owned string
- subset intentionally small and terminal-focused

### 8.11 Canonical Filesystem Module (`cct/fs`)

Import:

```cct
ADVOCARE "cct/fs.cct"
```

Available operations in the 11E.1 subset:
- `read_all(path)` -> `VERBUM`
- `write_all(path, content)` -> `NIHIL`
- `append_all(path, content)` -> `NIHIL`
- `exists(path)` -> `VERUM`
- `size(path)` -> `REX`

Subset behavior:
- strict read/size error paths with clear diagnostics
- ownership is explicit for text returned by `read_all`
- API is whole-file focused (no directory listing/streaming in this subset)

### 8.12 Canonical Path Module (`cct/path`)

Import:

```cct
ADVOCARE "cct/path.cct"
```

Available operations in the 11E.2 subset:
- `path_join(a, b)` -> `VERBUM`
- `path_basename(p)` -> `VERBUM`
- `path_dirname(p)` -> `VERBUM`
- `path_ext(p)` -> `VERBUM`

Subset behavior:
- canonical separator in produced output is `/`
- both `/` and `\\` are accepted as input separators in parsing logic
- API is intentionally minimal and text-semantic (not a full OS path framework)

### 8.13 Canonical Math Module (`cct/math`)

Import:

```cct
ADVOCARE "cct/math.cct"
```

Available operations in the 11F.1 subset:
- `abs(x)` -> `REX`
- `min(a, b)` -> `REX`
- `max(a, b)` -> `REX`
- `clamp(x, lo, hi)` -> `REX`

Subset behavior:
- deterministic integer-oriented helpers for everyday numeric logic
- `clamp` is strict and fails when `lo > hi`

### 8.14 Canonical Random Module (`cct/random`)

Import:

```cct
ADVOCARE "cct/random.cct"
```

Available operations in the 11F.1 subset:
- `seed(s)` -> `NIHIL`
- `random_int(lo, hi)` -> `REX`
- `random_real()` -> `FLAMMA`

Subset behavior:
- deterministic seeded PRNG baseline for reproducible workflows in this environment
- `random_int` is inclusive on both bounds and fails when `lo > hi`
- `random_real` returns values in `[0, 1)`

### 8.15 Canonical Parse Module (`cct/parse`)

Import:

```cct
ADVOCARE "cct/parse.cct"
```

Available operations in the 11F.2 subset:
- `parse_int(text)` -> `REX`
- `parse_real(text)` -> `UMBRA`
- `parse_bool(text)` -> `VERUM`

Subset behavior:
- strict conversion behavior for primitive textual parsing
- invalid integer/real text fails clearly
- invalid boolean text fails clearly (`parse_bool invalid input`)

### 8.16 Canonical Compare Module (`cct/cmp`)

Import:

```cct
ADVOCARE "cct/cmp.cct"
```

Available operations in the 11F.2 subset:
- `cmp_int(a, b)` -> `REX`
- `cmp_real(a, b)` -> `REX`
- `cmp_bool(a, b)` -> `REX`
- `cmp_verbum(a, b)` -> `REX`

Subset behavior:
- canonical comparator contract: `<0` (left < right), `0` (equal), `>0` (left > right)
- stable cross-module comparison API for ordering/equality workflows

### 8.17 Canonical Option Module (`cct/option`)

Import:

```cct
ADVOCARE "cct/option.cct"
```

Available operations in the 12C subset:
- constructors: `Some GENUS(T)(value)`, `None GENUS(T)()`
- checks: `option_is_some GENUS(T)(opt)`, `option_is_none GENUS(T)(opt)`
- extraction: `option_unwrap GENUS(T)(opt)`, `option_unwrap_or GENUS(T)(opt, default)`, `option_expect GENUS(T)(opt, message)`
- lifecycle: `option_free GENUS(T)(opt)`

Subset behavior:
- Option values are opaque pointer-backed instances
- callers must release instances with `option_free`
- `option_unwrap`/`option_expect` fail clearly on `None`

### 8.18 Canonical Result Module (`cct/result`)

Import:

```cct
ADVOCARE "cct/result.cct"
```

Available operations in the 12C subset:
- constructors: `Ok GENUS(T,E)(value)`, `Err GENUS(T,E)(error)`
- checks: `result_is_ok GENUS(T,E)(res)`, `result_is_err GENUS(T,E)(res)`
- extraction: `result_unwrap GENUS(T,E)(res)`, `result_unwrap_or GENUS(T,E)(res, default)`, `result_unwrap_err GENUS(T,E)(res)`, `result_expect GENUS(T,E)(res, message)`
- lifecycle: `result_free GENUS(T,E)(res)`

Subset behavior:
- Result values are opaque pointer-backed instances
- callers must release instances with `result_free`
- `result_unwrap` and `result_expect` fail clearly on `Err`
- `result_unwrap_err` fails clearly on `Ok`

### 8.19 Canonical Map Module (`cct/map`)

Import:

```cct
ADVOCARE "cct/map.cct"
```

Available operations in the 12D.1 subset:
- lifecycle: `map_init GENUS(K,V)()`, `map_free GENUS(K,V)(map)`
- mutation: `map_insert GENUS(K,V)(map, key, value)`, `map_remove GENUS(K,V)(map, key)`, `map_clear GENUS(K,V)(map)`, `map_reserve GENUS(K,V)(map, additional)`
- query: `map_get GENUS(K,V)(map, key)`, `map_contains GENUS(K,V)(map, key)`, `map_len GENUS(K,V)(map)`, `map_is_empty GENUS(K,V)(map)`, `map_capacity GENUS(K,V)(map)`

Subset behavior:
- map instances are opaque pointer-backed runtime structures
- `map_get` returns a pointer-backed Option payload (`Some`/`None` model from `cct/option`)
- `map_insert` updates existing keys in-place and does not duplicate key cardinality
- callers must release instances with `map_free`

### 8.20 Canonical Set Module (`cct/set`)

Import:

```cct
ADVOCARE "cct/set.cct"
```

Available operations in the 12D.1 subset:
- lifecycle: `set_init GENUS(T)()`, `set_free GENUS(T)(set)`
- mutation: `set_insert GENUS(T)(set, item)`, `set_remove GENUS(T)(set, item)`, `set_clear GENUS(T)(set)`
- query: `set_contains GENUS(T)(set, item)`, `set_len GENUS(T)(set)`, `set_is_empty GENUS(T)(set)`

Subset behavior:
- set instances are opaque pointer-backed runtime structures
- duplicates are ignored (`set_insert` returns `FALSUM` when item already exists)
- callers must release instances with `set_free`

### 8.21 Canonical Collection Ops Module (`cct/collection_ops`)

Import:

```cct
ADVOCARE "cct/collection_ops.cct"
```

Available operations in the 12D.2 subset:
- FLUXUS: `fluxus_map`, `fluxus_filter`, `fluxus_fold`, `fluxus_find`, `fluxus_any`, `fluxus_all`
- SERIES: `series_map`, `series_filter`, `series_reduce`, `series_find`, `series_any`, `series_all`

Current 12D.2 callback model:
- callbacks are passed as rituale pointers (`SPECULUM NIHIL`) and must be named rituales
- runtime bridge currently supports word-sized callback payloads (canonical `REX`/`VERUM`/pointer-sized flows)
- no closures in this subset (closure model remains for later phases)

### 8.22 Canonical Showcase Usage Patterns (11G)

Canonical showcase programs are part of the public stdlib contract:
- `examples/showcase_stdlib_string_11g.cct`
- `examples/showcase_stdlib_collection_11g.cct`
- `examples/showcase_stdlib_io_fs_11g.cct`
- `examples/showcase_stdlib_parse_math_random_11g.cct`
- `examples/showcase_stdlib_modular_11g_main.cct`

These examples must remain compatible with:
- `--check`
- `--ast` (single-file showcases)
- `--ast-composite` (multi-module showcase)
- `--sigilo-only`
- `--sigilo-mode essencial|completo`

Sigilo metadata in 11G+12D.1 includes stdlib-usage fields such as:
- `stdlib_module_count`
- `stdlib_modules_used`
- module operation counters (`verbum_ops_count`, `fmt_ops_count`, `series_ops_count`, `fluxus_ops_count`, `mem_ops_count`, `io_ops_count`, `fs_ops_count`, `path_ops_count`, `math_ops_count`, `random_ops_count`, `parse_ops_count`, `cmp_ops_count`, `alg_ops_count`, `option_ops_count`, `result_ops_count`, `map_ops_count`, `set_ops_count`)

### 8.23 Final Subset and Stability References (11H)

The frozen final stdlib subset and stability classes are documented in:
- `docs/stdlib_subset_11h.md`
- `docs/stdlib_stability_matrix_11h.md`

## 9. Advanced Typing (FASE 10E Contract)

### 9.1 `GENUS`

- explicit generic parameters in declarations
- explicit type arguments at use sites
- executable materialization through pragmatic monomorphization

### 9.2 `PACTUM`

- explicit contract declarations
- explicit conformance on `SIGILLUM`
- semantic conformance validation enabled in current subset

### 9.3 Basic Constraints

Supported form:

```cct
GENUS(T PACTUM Contract)
```

Current subset limits:
- one contract per type parameter
- constrained usage in supported generic ritual contexts
- no advanced inference or solver logic

## 10. Sigilo Specification (Operational)

For valid programs, CCT can emit:
- local sigilo: `<base>.svg` and `<base>.sigil`
- system sigilo for module closure: `<entry>.system.svg` and `<entry>.system.sigil`

Emission modes for multi-module workflows:
- essential (`essencial`): entry local + system
- complete (`completo`): entry local + imported module locals + system

System sigilo model:
- vector inline sigil-of-sigils composition
- deterministic metadata and hash behavior by structure

Baseline contract (FASE 13A.4):
- default baseline location: `docs/sigilo/baseline/local.sigil` or `docs/sigilo/baseline/system.sigil`
- baseline metadata sidecar: `<baseline>.baseline.meta`
- `check` is read-only (no implicit updates)
- `update` is explicit, and overwrite requires `--force`

Project sigilo operational report contract (FASE 13B.4):
- output signature: `format=cct.sigilo.report.v1`
- default mode: summary (`--sigilo-report summary`)
- detailed mode: `--sigilo-report detailed` (includes per-diff domain and before/after when CI profile gate computes diff)
- explain mode: `--sigilo-explain` (probable cause + recommended action + troubleshooting reference)
- compatibility note: legacy `sigilo.ci ...` line is preserved for existing script consumers
- consolidated operations guide: `docs/sigilo_operations_14b2.md`

Sigilo schema governance contract (FASE 13C.1):
- canonical schema identifier remains `format = cct.sigil.v1` during FASE 13
- evolution is additive by default (no incompatible format break in this phase)
- unknown additive top-level fields are warning-only in tolerant/strict parser modes
- deprecated field support is explicit (`sigilo_style` accepted with warning; replacement: `visual_engine`)
- normative schema governance remains enforced by the current validator/parser contract (historical 13C1 governance notes are archived privately)

Sigilo analytical metadata expansion (FASE 13C.2):
- additive analytical sections: `analysis_summary`, `diff_fingerprint_context`, `module_structural_summary`, `compatibility_hints`
- deterministic serialization order for analytical blocks in local/system `.sigil`
- no volatile timestamp fields in deterministic profile
- diff classification treats analytical blocks as review-required, except `compatibility_hints` (informational)

Sigilo consumer compatibility contract (FASE 13C.3):
- consumer profiles: `legacy-tolerant`, `current-default`, `strict-contract`
- CLI selection: `--consumer-profile legacy-tolerant|current-default|strict-contract` (`--strict` maps to `strict-contract`)
- higher same-family schema (`cct.sigil.v2+`) uses warning + v1-compatible fallback in non-strict profiles
- strict-contract keeps schema mismatch as blocking error
- migration/fallback behavior is covered by current operational guidance and `--consumer-profile` validation contracts

Sigilo strict/tolerant validator contract (FASE 13C.4):
- canonical validator entrypoint: `cct sigilo validate <artifact.sigil>`
- validation domains: required fields, type/cardinality checks, and cross-section consistency
- tolerant profiles classify recoverable issues as warnings and keep processing when safe
- strict-contract profile blocks contractual violations with deterministic non-zero exit status
- diagnostics are harmonized with objective message + corrective action hint (`action: ...`)

Diagnostic taxonomy contract (FASE 14A.1):
- canonical diagnostic levels: `error`, `warning`, `note`, `hint`
- actionable diagnostics preserve legacy `suggestion:` line and additionally emit `hint:` line
- sigilo text diagnostics are emitted through the canonical diagnostic envelope (`[sigilo]` code label)

Explain and troubleshooting contract (FASE 14A.3):
- `cct sigilo inspect|validate|diff|check|baseline check` accepts `--explain`
- explain mode appends `sigilo.explain probable_cause=... recommended_action=... docs=... blocked=... command=...`
- default mode remains concise and does not emit explain lines unless explicitly requested

Deterministic output/log hygiene contract (FASE 14A.4):
- sigilo diagnostics are emitted in deterministic order (`level`, `kind`, `line`, `column`, `message`)
- structured validation output (`[diag.N]`) follows the same deterministic ordering
- deterministic ordering is guaranteed for equivalent artifact content/profile inputs

FASE 13 release references (13D.3):
- release notes: `docs/release/FASE_13_RELEASE_NOTES.md`
- detailed 13D closure matrices/snapshots are preserved in archived/internal release governance records

Publication boundary policy reference (FASE 14B.3):

Release documentation template pack (FASE 14B.4):

## 11. Keyword Catalog

Legend:
- **Stable**: part of the documented usable subset
- **Partial**: parsed and/or semantically recognized, with executable or contextual restrictions
- **Reserved**: tokenized keyword reserved for future phases

### 11.1 Structure, Module, Declaration Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `INCIPIT` | file/block start | Stable | mandatory file opener |
| `EXPLICIT` | file/ritual end | Stable | file closes with `EXPLICIT grimoire` |
| `FIN` | block/declaration terminator | Stable | used in `SI`, `DUM`, `REPETE`, `ITERUM`, `SIGILLUM`, `ORDO`, `PACTUM`, `CODEX` |
| `ADVOCARE` | import module | Stable | multi-module closure |
| `ARCANUM` | internal visibility marker | Stable | top-level `RITUALE`, `SIGILLUM`, `ORDO` only |
| `RITUALE` | ritual/function declaration | Stable | core executable declaration |
| `SIGILLUM` | struct-like type | Stable | advanced subset supported |
| `ORDO` | enum-like type | Stable | pragmatic subset |
| `PACTUM` | contract/interface | Stable | explicit conformance model |
| `GENUS` | generic params and explicit type args | Stable | explicit use only |
| `CODEX` | namespace-like block | Partial | parsed/semantic; not primary executable path |
| `CIRCULUS` | reserved scope keyword | Reserved | tokenized only |
| `CONSTANS` | type modifier | Stable | reassignment blocked in semantic analysis; emitted as `const` in generated C (locals + rituale parameters) |
| `VOLATILE` | type modifier | Partial | parsed as modifier; no full semantic enforcement |
| `FLUXUS` | dynamic-array modifier keyword | Partial | tokenized/parsed in type syntax with restrictions |

### 11.2 Control and Flow Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `SI` | if | Stable | supports `ALITER` |
| `ALITER` | else | Stable | optional |
| `QUANDO` | pattern/switch selection | Stable | statement-only selection over literals and ORDO variants |
| `CASO` | case arm in `QUANDO` | Stable | supports OR-case literals and ORDO payload bindings |
| `SENAO` | default arm in `QUANDO` | Stable | optional fallback branch |
| `DUM` | while / do-while trailer | Stable | used by both `DUM` and `DONEC ... DUM` |
| `DONEC` | do-while block start | Stable | post-condition form |
| `REPETE` | for-range loop | Stable | `DE`, `AD`, optional `GRADUS` |
| `ITERUM` | collection iteration loop | Stable | arity depends on collection (`map`: 2 bindings; others: 1) |
| `DE` | from | Stable | `REPETE` header |
| `AD` | to/assign delimiter | Stable | in `REPETE` and `VINCIRE`/`EVOCA` initializer syntax |
| `GRADUS` | step | Stable | optional in `REPETE` |
| `PRO` | foreach-like keyword | Reserved | tokenized; not active grammar |
| `IN` | iterator membership keyword | Stable | used in `ITERUM` headers |
| `FRANGE` | break | Stable | valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM` |
| `RECEDE` | continue | Stable | valid only inside `DUM`, `DONEC`, `REPETE`, `ITERUM` |
| `TRANSITUS` | goto | Partial | parsed; currently outside stable codegen path |

### 11.3 Execution and Memory Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `EVOCA` | variable declaration | Stable | optional initializer with `AD` |
| `VINCIRE` | assignment | Stable | `VINCIRE target AD expr` |
| `REDDE` | return | Stable | expression optional depending on ritual type |
| `ANUR` | process exit | Stable | integer expression expected |
| `CONIURA` | explicit ritual call | Stable | preferred executable call form |
| `OBSECRO` | builtin call namespace | Partial | stable for `scribe`, `pete`, `libera` subset |
| `DIMITTE` | pointer release statement | Stable | explicit discard path |
| `MENSURA` | size-of by type | Stable | returns integer size |

### 11.4 Failure-Control Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `TEMPTA` | try | Stable | current failure subset |
| `CAPE` | catch | Stable | one handler in final phase-8 subset |
| `SEMPER` | finally | Stable | must follow `CAPE` |
| `IACE` | throw | Stable | payload subset restrictions apply |
| `FRACTUM` | failure payload type | Stable | failure-control subset type |

### 11.5 Type and Literal Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `REX` | int64 | Stable | |
| `DUX` | int32 | Stable | |
| `COMES` | int16 | Stable | |
| `MILES` | uint8 | Stable | |
| `UMBRA` | double | Stable | |
| `FLAMMA` | float | Stable | |
| `VERBUM` | string | Stable | |
| `MOLDE` | interpolated string expression | Stable | host-only in current subset |
| `VERUM` | boolean true / bool family | Stable | literal and type family |
| `FALSUM` | boolean false | Stable | literal |
| `NIHIL` | null/void family | Stable | |
| `SPECULUM` | pointer modifier / address-of unary | Stable | subset-limited pointer system |
| `SERIES` | static array form | Stable | subset-limited array system |

### 11.6 Logical, Bitwise, and Shift Keywords

| Keyword | Meaning | Status | Notes |
|---|---|---|---|
| `ET` | logical and | Stable | short-circuit in conditionals and value expressions |
| `VEL` | logical or | Stable | short-circuit in conditionals and value expressions |
| `NON` | logical not | Stable | unary |
| `ET_BIT` | bitwise and | Stable | integer operands required |
| `VEL_BIT` | bitwise or | Stable | integer operands required |
| `XOR` | bitwise xor | Stable | integer operands required |
| `NON_BIT` | bitwise not | Stable | integer operand required |
| `SINISTER` | shift left | Stable | integer operands required |
| `DEXTER` | shift right | Stable | integer operands required |

## 12. Basic Programming Examples

### 12.1 Hello World

```cct
INCIPIT grimoire "hello"

RITUALE main() REDDE REX
  OBSECRO scribe("Hello from CCT\n")
  REDDE 0
EXPLICIT RITUALE

EXPLICIT grimoire
```

### 12.2 If/Loop/Assignment

```cct
INCIPIT grimoire "flow"

RITUALE main() REDDE REX
  EVOCA REX i AD 0
  DUM i < 3
    VINCIRE i AD i + 1
  FIN DUM

  SI i == 3
    REDDE 42
  ALITER
    REDDE 1
  FIN SI
EXPLICIT RITUALE

EXPLICIT grimoire
```

### 12.3 Module Import + Internal Visibility

```cct
INCIPIT grimoire "main"

ADVOCARE "lib.cct"

RITUALE main() REDDE REX
  REDDE CONIURA public_value()
EXPLICIT RITUALE

EXPLICIT grimoire
```

## 13. Known Subset Boundaries (Important)

- Some tokenized keywords are reserved and not active grammar yet.
- Some parsed/semantic constructs are outside current stable executable codegen subset.
- Generic use is explicit by design in current phases.
- Contract and constraint models are intentionally conservative in this baseline.

Always rely on:
- `--check` for semantic diagnostics
- integration tests in `tests/` for canonical supported patterns

## 14. Project Workflow (FASE 12F)

Canonical project layout:

```text
project/
├── src/main.cct
├── lib/
├── tests/
├── bench/
└── cct.toml (optional)
```

Conventions:
- test files end with `.test.cct`
- benchmark files end with `.bench.cct`
- project-local artifacts are stored under `.cct/`
- distribution binaries are emitted under `dist/`

Incremental contract:
- `cct build` fingerprints entry + module closure + profile
- unchanged fingerprint and existing output => `up-to-date`
- cache is stored at `.cct/cache/manifest.txt`

Quality-gate contract:
- `cct test --strict-lint` makes lint warnings fail the run (exit `2`)
- `cct test --fmt-check` makes formatting mismatches fail the run (exit `2`)

## 15. API Documentation Generation (FASE 12G)

`cct doc` generates API pages from project module closure.

Default output:

```text
<project>/docs/api/
```

Key options:
- `--format markdown|html|both`
- `--include-internal`
- `--warn-missing-docs`
- `--strict-docs`
- `--no-timestamp`

Strict contract:
- warnings are non-fatal by default
- with `--strict-docs`, warnings return exit code `2`

## 16. Bibliotheca Canonica — Inventario Completo De Funcoes (FASE 19D.4)

- Esta secao e normativa para cobertura de superficie de API canonica da biblioteca distribuida com o compilador.
- Toda funcao declarada em `lib/cct/*.cct` e `lib/cct/kernel/*.cct` deve aparecer neste inventario.
- O inventario abaixo e gerado a partir das declaracoes `RITUALE` e serve como indice de referencia rapida por modulo.

<!-- BEGIN AUTO API INVENTORY 19D4 -->
<!-- AUTO-GENERATED from lib/cct/*.cct and lib/cct/kernel/*.cct -->
**Cobertura**: inventario completo de funcoes canonicas em `lib/cct` (FASE 19D.4).

### `cct/alg`

- `alg_linear_search(SPECULUM REX arr, REX valor, REX tamanho) -> REX`
- `alg_compare_arrays(SPECULUM REX a, SPECULUM REX b, REX tamanho) -> VERUM`
- `alg_binary_search(SPECULUM REX arr, REX tamanho, REX alvo) -> REX`
- `alg_sort_insertion(SPECULUM REX arr, REX tamanho) -> NIHIL`
- `alg_sum(SPECULUM REX arr, REX n) -> REX`
- `alg_sum_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_min(SPECULUM REX arr, REX n) -> REX`
- `alg_max(SPECULUM REX arr, REX n) -> REX`
- `alg_min_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_max_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `alg_reverse_range(SPECULUM REX arr, REX begin, REX end_exclusive) -> NIHIL`
- `alg_reverse(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_fill(SPECULUM REX arr, REX n, REX value) -> NIHIL`
- `alg_fill_real(SPECULUM UMBRA arr, REX n, UMBRA value) -> NIHIL`
- `alg_rotate(SPECULUM REX arr, REX n, REX k) -> NIHIL`
- `alg_quicksort(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_mergesort(SPECULUM REX arr, REX n) -> NIHIL`
- `alg_sort_verbum(SPECULUM NIHIL arr, REX n) -> NIHIL`
- `alg_is_sorted(SPECULUM REX arr, REX n) -> VERUM`
- `alg_count(SPECULUM REX arr, REX n, REX value) -> REX`
- `alg_deduplicate_sorted(SPECULUM REX arr, REX n) -> REX`
- `alg_dot_product(SPECULUM REX a, SPECULUM REX b, REX n) -> REX`
- `alg_dot_product_real(SPECULUM UMBRA a, SPECULUM UMBRA b, REX n) -> UMBRA`

Total modulo: **23**

### `cct/args`

- `argc() -> REX`
- `arg(REX i) -> VERBUM`

Total modulo: **2**

### `cct/ast_node`

- `ast_make_ident(SPECULUM AstIdent p) -> Variant`
- `ast_make_literal_int(SPECULUM AstLiteralInt p) -> Variant`
- `ast_make_binary(SPECULUM AstBinary p) -> Variant`
- `ast_is_ident(Variant n) -> VERUM`
- `ast_is_literal_int(Variant n) -> VERUM`
- `ast_is_binary(Variant n) -> VERUM`
- `ast_as_ident(Variant n) -> SPECULUM AstIdent`
- `ast_as_literal_int(Variant n) -> SPECULUM AstLiteralInt`
- `ast_as_binary(Variant n) -> SPECULUM AstBinary`

Total modulo: **9**

### `cct/bit`

- `bit_get(REX n, REX pos) -> VERUM`
- `bit_set(REX n, REX pos) -> REX`
- `bit_clear(REX n, REX pos) -> REX`
- `bit_toggle(REX n, REX pos) -> REX`
- `popcount(REX n) -> REX`
- `leading_zeros(REX n) -> REX`
- `trailing_zeros(REX n) -> REX`
- `rotate_left(REX n, REX k) -> REX`
- `rotate_right(REX n, REX k) -> REX`
- `next_power_of_2(REX n) -> REX`
- `is_power_of_2(REX n) -> VERUM`
- `byte_swap(REX n) -> REX`
- `parity(REX n) -> VERUM`
- `bit_extract(REX n, REX start, REX len) -> REX`

Total modulo: **14**

### `cct/bytes`

- `bytes_new(REX size) -> SPECULUM NIHIL`
- `bytes_len(SPECULUM NIHIL b) -> REX`
- `bytes_get(SPECULUM NIHIL b, REX i) -> MILES`
- `bytes_set(SPECULUM NIHIL b, REX i, MILES v) -> NIHIL`
- `bytes_free(SPECULUM NIHIL b) -> NIHIL`

Total modulo: **5**

### `cct/char`

- `is_digit(MILES c) -> VERUM`
- `is_alpha(MILES c) -> VERUM`
- `is_whitespace(MILES c) -> VERUM`

Total modulo: **3**

### `cct/cmp`

- `cmp_int(REX a, REX b) -> REX`
- `cmp_real(UMBRA a, UMBRA b) -> REX`
- `cmp_bool(VERUM a, VERUM b) -> REX`
- `cmp_verbum(VERBUM a, VERBUM b) -> REX`

Total modulo: **4**

### `cct/code_writer`

- `writer_init() -> SPECULUM NIHIL`
- `writer_indent(SPECULUM NIHIL w) -> NIHIL`
- `writer_dedent(SPECULUM NIHIL w) -> NIHIL`
- `writer_write(SPECULUM NIHIL w, VERBUM s) -> NIHIL`
- `writer_writeln(SPECULUM NIHIL w, VERBUM s) -> NIHIL`
- `writer_write_int(SPECULUM NIHIL w, REX v) -> NIHIL`
- `writer_writeln_int(SPECULUM NIHIL w, REX v) -> NIHIL`
- `writer_write_bool(SPECULUM NIHIL w, VERUM v) -> NIHIL`
- `writer_to_verbum(SPECULUM NIHIL w) -> VERBUM`
- `writer_free(SPECULUM NIHIL w) -> NIHIL`

Total modulo: **10**

### `cct/collection_ops`

- `fluxus_map GENUS(T, R) -> SPECULUM NIHIL`
- `fluxus_filter GENUS(T) -> SPECULUM NIHIL`
- `fluxus_fold GENUS(T, Acc) -> Acc`
- `fluxus_find GENUS(T) -> SPECULUM NIHIL`
- `fluxus_any GENUS(T) -> VERUM`
- `fluxus_all GENUS(T) -> VERUM`
- `series_map GENUS(T, R) -> SPECULUM NIHIL`
- `series_filter GENUS(T) -> SPECULUM NIHIL`
- `series_reduce GENUS(T, Acc) -> Acc`
- `series_find GENUS(T) -> SPECULUM NIHIL`
- `series_any GENUS(T) -> VERUM`
- `series_all GENUS(T) -> VERUM`

Total modulo: **12**

### `cct/env`

- `getenv(VERBUM name) -> VERBUM`
- `has_env(VERBUM name) -> VERUM`
- `cwd() -> VERBUM`

Total modulo: **3**

### `cct/fluxus`

- `fluxus_init(REX elem_size) -> SPECULUM NIHIL`
- `fluxus_free(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_push(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_pop(SPECULUM NIHIL flux, SPECULUM NIHIL out) -> NIHIL`
- `fluxus_len(SPECULUM NIHIL flux) -> REX`
- `fluxus_get(SPECULUM NIHIL flux, REX idx) -> SPECULUM NIHIL`
- `fluxus_clear(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_reserve(SPECULUM NIHIL flux, REX cap) -> NIHIL`
- `fluxus_capacity(SPECULUM NIHIL flux) -> REX`
- `fluxus_peek(SPECULUM NIHIL flux) -> SPECULUM NIHIL`
- `fluxus_set(SPECULUM NIHIL flux, REX idx, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_remove(SPECULUM NIHIL flux, REX idx) -> NIHIL`
- `fluxus_insert(SPECULUM NIHIL flux, REX idx, SPECULUM NIHIL elem) -> NIHIL`
- `fluxus_contains(SPECULUM NIHIL flux, SPECULUM NIHIL elem) -> VERUM`
- `fluxus_is_empty(SPECULUM NIHIL flux) -> VERUM`
- `fluxus_concat GENUS(T) -> SPECULUM NIHIL`
- `fluxus_slice GENUS(T) -> SPECULUM NIHIL`
- `fluxus_copy GENUS(T) -> SPECULUM NIHIL`
- `fluxus_reverse(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_sort_int(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_sort_verbum(SPECULUM NIHIL flux) -> NIHIL`
- `fluxus_to_ptr(SPECULUM NIHIL flux) -> SPECULUM NIHIL`

Total modulo: **22**

### `cct/fmt`

- `stringify_int(REX x) -> VERBUM`
- `stringify_real(UMBRA x) -> VERBUM`
- `stringify_float(FLAMMA x) -> VERBUM`
- `stringify_bool(VERUM b) -> VERBUM`
- `fmt_parse_int(VERBUM s) -> REX`
- `fmt_parse_real(VERBUM s) -> UMBRA`
- `fmt_parse_bool(VERBUM s) -> VERUM`
- `format_pair(VERBUM label, VERBUM value) -> VERBUM`
- `stringify_int_hex(REX n) -> VERBUM`
- `stringify_int_hex_upper(REX n) -> VERBUM`
- `stringify_int_oct(REX n) -> VERBUM`
- `stringify_int_bin(REX n) -> VERBUM`
- `stringify_uint(DUX n) -> VERBUM`
- `stringify_int_padded(REX n, REX width, MILES fill) -> VERBUM`
- `stringify_real_prec(UMBRA x, REX prec) -> VERBUM`
- `stringify_real_sci(UMBRA x) -> VERBUM`
- `stringify_real_fixed(UMBRA x, REX prec) -> VERBUM`
- `stringify_char(MILES c) -> VERBUM`
- `format_1(VERBUM tmpl, VERBUM a) -> VERBUM`
- `format_2(VERBUM tmpl, VERBUM a, VERBUM b) -> VERBUM`
- `format_3(VERBUM tmpl, VERBUM a, VERBUM b, VERBUM c) -> VERBUM`
- `format_4(VERBUM tmpl, VERBUM a, VERBUM b, VERBUM c, VERBUM d) -> VERBUM`
- `repeat_char(MILES c, REX n) -> VERBUM`
- `table_row(SPECULUM NIHIL parts, SPECULUM REX widths, REX ncols) -> VERBUM`

Total modulo: **24**

### `cct/fs`

- `read_all(VERBUM path) -> VERBUM`
- `write_all(VERBUM path, VERBUM content) -> NIHIL`
- `append_all(VERBUM path, VERBUM content) -> NIHIL`
- `mkdir(VERBUM path) -> NIHIL`
- `mkdir_all(VERBUM path) -> NIHIL`
- `delete_file(VERBUM path) -> NIHIL`
- `delete_dir(VERBUM path) -> NIHIL`
- `rename(VERBUM de, VERBUM para) -> NIHIL`
- `copy(VERBUM de, VERBUM para) -> NIHIL`
- `move(VERBUM de, VERBUM para) -> NIHIL`
- `is_file(VERBUM path) -> VERUM`
- `is_dir(VERBUM path) -> VERUM`
- `is_symlink(VERBUM path) -> VERUM`
- `is_readable(VERBUM path) -> VERUM`
- `is_writable(VERBUM path) -> VERUM`
- `modified_time(VERBUM path) -> REX`
- `chmod(VERBUM path, REX mode) -> NIHIL`
- `list_dir(VERBUM path) -> SPECULUM NIHIL`
- `read_lines(VERBUM path) -> SPECULUM NIHIL`
- `create_temp_file() -> VERBUM`
- `create_temp_dir() -> VERBUM`
- `truncate(VERBUM path, REX size) -> NIHIL`
- `symlink(VERBUM alvo, VERBUM link) -> NIHIL`
- `same_file(VERBUM a, VERBUM b) -> VERUM`
- `exists(VERBUM path) -> VERUM`
- `size(VERBUM path) -> REX`

Total modulo: **26**

### `cct/hash`

- `djb2(VERBUM s) -> REX`
- `fnv1a(VERBUM s) -> REX`
- `fnv1a_bytes(SPECULUM NIHIL data, REX len) -> REX`
- `crc32(VERBUM s) -> REX`
- `murmur3(VERBUM s, REX seed) -> REX`
- `combine(REX h1, REX h2) -> REX`

Total modulo: **6**

### `cct/io`

- `print(VERBUM s) -> NIHIL`
- `println(VERBUM s) -> NIHIL`
- `print_int(REX n) -> NIHIL`
- `print_real(UMBRA x) -> NIHIL`
- `print_bool(VERUM b) -> NIHIL`
- `print_char(MILES c) -> NIHIL`
- `print_hex(REX n) -> NIHIL`
- `eprint(VERBUM s) -> NIHIL`
- `eprintln(VERBUM s) -> NIHIL`
- `eprint_int(REX n) -> NIHIL`
- `eprint_real(UMBRA x) -> NIHIL`
- `eprint_bool(VERUM b) -> NIHIL`
- `flush() -> NIHIL`
- `flush_err() -> NIHIL`
- `read_char() -> MILES`
- `read_line() -> VERBUM`
- `read_all_stdin() -> VERBUM`
- `read_line_prompt(VERBUM prompt) -> VERBUM`
- `is_tty() -> VERUM`

Total modulo: **19**

### `cct/map`

- `map_init GENUS(K, V) -> SPECULUM NIHIL`
- `map_free GENUS(K, V) -> NIHIL`
- `map_insert GENUS(K, V) -> NIHIL`
- `map_remove GENUS(K, V) -> VERUM`
- `map_get GENUS(K, V) -> SPECULUM NIHIL`
- `map_contains GENUS(K, V) -> VERUM`
- `map_len GENUS(K, V) -> REX`
- `map_is_empty GENUS(K, V) -> VERUM`
- `map_capacity GENUS(K, V) -> REX`
- `map_clear GENUS(K, V) -> NIHIL`
- `map_reserve GENUS(K, V) -> NIHIL`
- `map_get_or_default GENUS(K, V) -> V`
- `map_update_or_insert GENUS(K, V) -> VERUM`
- `map_copy GENUS(K, V) -> SPECULUM NIHIL`
- `map_keys GENUS(K, V) -> SPECULUM NIHIL`
- `map_values GENUS(K, V) -> SPECULUM NIHIL`
- `map_merge GENUS(K, V) -> NIHIL`

Total modulo: **17**

### `cct/math`

- `pi() -> UMBRA`
- `e() -> UMBRA`
- `tau() -> UMBRA`
- `phi() -> UMBRA`
- `sqrt2() -> UMBRA`
- `abs(REX x) -> REX`
- `min(REX a, REX b) -> REX`
- `max(REX a, REX b) -> REX`
- `clamp(REX x, REX lo, REX hi) -> REX`
- `sign(REX x) -> REX`
- `is_even(REX x) -> VERUM`
- `is_odd(REX x) -> VERUM`
- `abs_real(UMBRA x) -> UMBRA`
- `sign_real(UMBRA x) -> REX`
- `min_real(UMBRA a, UMBRA b) -> UMBRA`
- `max_real(UMBRA a, UMBRA b) -> UMBRA`
- `clamp_real(UMBRA x, UMBRA lo, UMBRA hi) -> UMBRA`
- `floor_real(UMBRA x) -> REX`
- `ceil_real(UMBRA x) -> REX`
- `round_real(UMBRA x) -> REX`
- `trunc_real(UMBRA x) -> REX`
- `pow_int(REX base, REX exponent) -> REX`
- `gcd(REX a, REX b) -> REX`
- `lcm(REX a, REX b) -> REX`
- `factorial(REX n) -> REX`
- `is_prime(REX n) -> VERUM`
- `sqrt_real(UMBRA x) -> UMBRA`
- `cbrt_real(UMBRA x) -> UMBRA`
- `pow_real(UMBRA base, UMBRA exponent) -> UMBRA`
- `hypot(UMBRA x, UMBRA y) -> UMBRA`
- `sin(UMBRA radians) -> UMBRA`
- `cos(UMBRA radians) -> UMBRA`
- `tan(UMBRA radians) -> UMBRA`
- `asin(UMBRA x) -> UMBRA`
- `acos(UMBRA x) -> UMBRA`
- `atan(UMBRA x) -> UMBRA`
- `atan2(UMBRA y, UMBRA x) -> UMBRA`
- `deg_to_rad(UMBRA degrees) -> UMBRA`
- `rad_to_deg(UMBRA radians) -> UMBRA`
- `exp(UMBRA x) -> UMBRA`
- `log_natural(UMBRA x) -> UMBRA`
- `log10(UMBRA x) -> UMBRA`
- `log2(UMBRA x) -> UMBRA`

Total modulo: **43**

### `cct/mem`

- `alloc(REX tamanho) -> SPECULUM REX`
- `free(SPECULUM REX ptr) -> NIHIL`
- `realloc(SPECULUM REX ptr, REX novo_tamanho) -> SPECULUM REX`
- `copy(SPECULUM REX dest, SPECULUM REX src, REX tamanho) -> NIHIL`
- `set(SPECULUM REX ptr, REX valor, REX tamanho) -> NIHIL`
- `zero(SPECULUM REX ptr, REX tamanho) -> NIHIL`
- `mem_compare(SPECULUM REX a, SPECULUM REX b, REX tamanho) -> REX`

Total modulo: **7**

### `cct/option`

- `Some GENUS(T) -> SPECULUM NIHIL`
- `None GENUS(T) -> SPECULUM NIHIL`
- `option_is_some GENUS(T) -> VERUM`
- `option_is_none GENUS(T) -> VERUM`
- `option_unwrap GENUS(T) -> T`
- `option_unwrap_or GENUS(T) -> T`
- `option_expect GENUS(T) -> T`
- `option_free GENUS(T) -> NIHIL`

Total modulo: **8**

### `cct/ordo_samples`

- `resultado_ok(REX v) -> Resultado`
- `resultado_err(VERBUM m) -> Resultado`
- `resultado_e_ok(Resultado r) -> VERUM`
- `resultado_valor(Resultado r) -> REX`
- `opcao_algum(REX v) -> Opcao`
- `opcao_nenhum() -> Opcao`
- `opcao_ou_padrao(Opcao o, REX padrao) -> REX`
- `token_descrever(Token t) -> VERBUM`

Total modulo: **8**

### `cct/parse`

- `parse_int(VERBUM s) -> REX`
- `parse_real(VERBUM s) -> UMBRA`
- `parse_bool(VERBUM s) -> VERUM`
- `try_int(VERBUM s) -> SPECULUM NIHIL`
- `try_real(VERBUM s) -> SPECULUM NIHIL`
- `try_bool(VERBUM s) -> SPECULUM NIHIL`
- `parse_int_hex(VERBUM s) -> REX`
- `try_int_hex(VERBUM s) -> SPECULUM NIHIL`
- `parse_int_radix(VERBUM s, REX radix) -> REX`
- `try_int_radix(VERBUM s, REX radix) -> SPECULUM NIHIL`
- `is_int(VERBUM s) -> VERUM`
- `is_real(VERBUM s) -> VERUM`
- `parse_lines(VERBUM s) -> SPECULUM NIHIL`
- `parse_csv_line(VERBUM s) -> SPECULUM NIHIL`
- `parse_csv_line_sep(VERBUM s, MILES sep) -> SPECULUM NIHIL`

Total modulo: **15**

### `cct/path`

- `path_join(VERBUM a, VERBUM b) -> VERBUM`
- `path_basename(VERBUM p) -> VERBUM`
- `path_dirname(VERBUM p) -> VERBUM`
- `path_ext(VERBUM p) -> VERBUM`
- `stem(VERBUM p) -> VERBUM`
- `normalize(VERBUM p) -> VERBUM`
- `is_absolute(VERBUM p) -> VERUM`
- `is_relative(VERBUM p) -> VERUM`
- `resolve(VERBUM p) -> VERBUM`
- `relative_to(VERBUM path, VERBUM base) -> VERBUM`
- `with_ext(VERBUM p, VERBUM extension) -> VERBUM`
- `without_ext(VERBUM p) -> VERBUM`
- `parent(VERBUM p) -> VERBUM`
- `home_dir() -> VERBUM`
- `temp_dir() -> VERBUM`
- `split_path(VERBUM p) -> SPECULUM NIHIL`

Total modulo: **16**

### `cct/process`

- `run(VERBUM cmd) -> REX`
- `run_capture(VERBUM cmd) -> VERBUM`
- `run_capture_err(VERBUM cmd) -> VERBUM`
- `run_with_input(VERBUM cmd, VERBUM input) -> VERBUM`
- `run_env(VERBUM cmd, SPECULUM NIHIL env_pairs) -> REX`
- `run_timeout(VERBUM cmd, REX timeout_ms) -> REX`

Total modulo: **6**

### `cct/random`

- `seed(REX s) -> NIHIL`
- `random_int(REX lo, REX hi) -> REX`
- `random_real() -> FLAMMA`
- `random_real_unit() -> FLAMMA`
- `random_bool() -> VERUM`
- `random_real_range(UMBRA lo, UMBRA hi) -> UMBRA`
- `random_verbum(REX comprimento) -> VERBUM`
- `random_verbum_from(REX comprimento, VERBUM alphabet) -> VERBUM`
- `random_choice_int(SPECULUM REX arr, REX n) -> REX`
- `shuffle_int(SPECULUM REX arr, REX n) -> NIHIL`
- `random_bytes(REX n) -> SPECULUM NIHIL`

Total modulo: **11**

### `cct/result`

- `Ok GENUS(T, E) -> SPECULUM NIHIL`
- `Err GENUS(T, E) -> SPECULUM NIHIL`
- `result_is_ok GENUS(T, E) -> VERUM`
- `result_is_err GENUS(T, E) -> VERUM`
- `result_unwrap GENUS(T, E) -> T`
- `result_unwrap_or GENUS(T, E) -> T`
- `result_unwrap_err GENUS(T, E) -> E`
- `result_expect GENUS(T, E) -> T`
- `result_free GENUS(T, E) -> NIHIL`

Total modulo: **9**

### `cct/series`

- `series_len GENUS(T) -> REX`
- `series_fill GENUS(T) -> NIHIL`
- `series_copy GENUS(T) -> NIHIL`
- `series_reverse GENUS(T) -> NIHIL`
- `series_contains(SPECULUM REX arr, REX valor, REX tamanho) -> VERUM`
- `series_sum(SPECULUM REX arr, REX n) -> REX`
- `series_sum_real(SPECULUM UMBRA arr, REX n) -> UMBRA`
- `series_min(SPECULUM REX arr, REX n) -> REX`
- `series_max(SPECULUM REX arr, REX n) -> REX`
- `series_is_sorted(SPECULUM REX arr, REX n) -> VERUM`
- `series_sort(SPECULUM REX arr, REX n) -> NIHIL`
- `series_count_val(SPECULUM REX arr, REX n, REX val) -> REX`

Total modulo: **12**

### `cct/set`

- `set_init GENUS(T) -> SPECULUM NIHIL`
- `set_free GENUS(T) -> NIHIL`
- `set_insert GENUS(T) -> VERUM`
- `set_remove GENUS(T) -> VERUM`
- `set_contains GENUS(T) -> VERUM`
- `set_len GENUS(T) -> REX`
- `set_is_empty GENUS(T) -> VERUM`
- `set_clear GENUS(T) -> NIHIL`
- `set_union GENUS(T) -> SPECULUM NIHIL`
- `set_intersection GENUS(T) -> SPECULUM NIHIL`
- `set_difference GENUS(T) -> SPECULUM NIHIL`
- `set_symmetric_difference GENUS(T) -> SPECULUM NIHIL`
- `set_is_subset GENUS(T) -> VERUM`
- `set_is_superset GENUS(T) -> VERUM`
- `set_equals GENUS(T) -> VERUM`
- `set_copy GENUS(T) -> SPECULUM NIHIL`
- `set_to_fluxus GENUS(T) -> SPECULUM NIHIL`
- `set_reserve GENUS(T) -> NIHIL`
- `set_capacity GENUS(T) -> REX`

Total modulo: **19**

### `cct/stub_test`

- `test_value() -> REX`

Total modulo: **1**

### `cct/time`

- `now_ms() -> REX`
- `now_ns() -> REX`
- `sleep_ms(REX ms) -> NIHIL`

Total modulo: **3**

### `cct/variant`

- `variant_make(REX tag, SPECULUM NIHIL payload) -> Variant`
- `variant_tag(Variant v) -> REX`
- `variant_payload(Variant v) -> SPECULUM NIHIL`

Total modulo: **3**

### `cct/variant_helpers`

- `variant_is_tag(Variant v, REX tag) -> VERUM`
- `variant_expect_tag(Variant v, REX tag, VERBUM message) -> NIHIL`
- `variant_payload_if(Variant v, REX tag) -> SPECULUM NIHIL`
- `variant_payload_expect(Variant v, REX tag, VERBUM message) -> SPECULUM NIHIL`

Total modulo: **4**

### `cct/verbum`

- `len(VERBUM s) -> REX`
- `concat(VERBUM a, VERBUM b) -> VERBUM`
- `compare(VERBUM a, VERBUM b) -> REX`
- `substring(VERBUM s, REX inicio, REX fim) -> VERBUM`
- `trim(VERBUM s) -> VERBUM`
- `find(VERBUM s, VERBUM sub) -> REX`
- `char_at(VERBUM s, REX i) -> MILES`
- `from_char(MILES c) -> VERBUM`
- `contains(VERBUM s, VERBUM sub) -> VERUM`
- `starts_with(VERBUM s, VERBUM prefix) -> VERUM`
- `ends_with(VERBUM s, VERBUM suffix) -> VERUM`
- `strip_prefix(VERBUM s, VERBUM prefix) -> VERBUM`
- `strip_suffix(VERBUM s, VERBUM suffix) -> VERBUM`
- `replace(VERBUM s, VERBUM de, VERBUM para) -> VERBUM`
- `replace_all(VERBUM s, VERBUM de, VERBUM para) -> VERBUM`
- `to_upper(VERBUM s) -> VERBUM`
- `to_lower(VERBUM s) -> VERBUM`
- `trim_left(VERBUM s) -> VERBUM`
- `trim_right(VERBUM s) -> VERBUM`
- `trim_char(VERBUM s, MILES c) -> VERBUM`
- `repeat(VERBUM s, REX n) -> VERBUM`
- `pad_left(VERBUM s, REX width, MILES fill) -> VERBUM`
- `pad_right(VERBUM s, REX width, MILES fill) -> VERBUM`
- `center(VERBUM s, REX width, MILES fill) -> VERBUM`
- `last_find(VERBUM s, VERBUM sub) -> REX`
- `find_from(VERBUM s, VERBUM sub, REX offset) -> REX`
- `count_occurrences(VERBUM s, VERBUM sub) -> REX`
- `reverse(VERBUM s) -> VERBUM`
- `is_empty(VERBUM s) -> VERUM`
- `equals_ignore_case(VERBUM a, VERBUM b) -> VERUM`
- `slice(VERBUM s, REX inicio, REX comprimento) -> VERBUM`
- `is_ascii(VERBUM s) -> VERUM`
- `split(VERBUM s, VERBUM sep) -> SPECULUM NIHIL`
- `split_char(VERBUM s, MILES c) -> SPECULUM NIHIL`
- `join(SPECULUM NIHIL parts, VERBUM sep) -> VERBUM`
- `lines(VERBUM s) -> SPECULUM NIHIL`
- `words(VERBUM s) -> SPECULUM NIHIL`

Total modulo: **37**

### `cct/verbum_builder`

- `builder_init() -> SPECULUM NIHIL`
- `builder_append(SPECULUM NIHIL b, VERBUM s) -> NIHIL`
- `builder_append_char(SPECULUM NIHIL b, MILES c) -> NIHIL`
- `builder_append_int(SPECULUM NIHIL b, REX v) -> NIHIL`
- `builder_append_bool(SPECULUM NIHIL b, VERUM v) -> NIHIL`
- `builder_append_real(SPECULUM NIHIL b, UMBRA v) -> NIHIL`
- `builder_len(SPECULUM NIHIL b) -> REX`
- `builder_to_verbum(SPECULUM NIHIL b) -> VERBUM`
- `builder_clear(SPECULUM NIHIL b) -> NIHIL`
- `builder_free(SPECULUM NIHIL b) -> NIHIL`

Total modulo: **10**

### `cct/verbum_scan`

- `cursor_init(VERBUM s) -> SPECULUM NIHIL`
- `cursor_pos(SPECULUM NIHIL c) -> REX`
- `cursor_eof(SPECULUM NIHIL c) -> VERUM`
- `cursor_peek(SPECULUM NIHIL c) -> MILES`
- `cursor_next(SPECULUM NIHIL c) -> MILES`
- `cursor_free(SPECULUM NIHIL c) -> NIHIL`

Total modulo: **6**

### `cct/kernel`

- `kernel_halt() -> NIHIL`
- `kernel_outb(REX porta, REX valor) -> NIHIL`
- `kernel_inb(REX porta) -> REX`
- `kernel_memcpy(SPECULUM REX dst, SPECULUM REX src, REX n) -> NIHIL`
- `kernel_memset(SPECULUM REX dst, REX valor, REX n) -> NIHIL`

Total modulo: **5**

**Total geral de funcoes inventariadas**: **422**
<!-- END AUTO API INVENTORY 19D4 -->
