# CCT Lowering Matrix V0 — Mapeamento CCT → ASM x86-32

- Versão: V0
- Status: Ativo (FASE 16A.1)
- Dependência: CCT_ABI_V0_LBOS.md V0; CCT_SYMBOL_NAMING_V0.md V0.
- Próxima revisão: V1 (quando 16C confirmar implementação real).

## 1. Propósito

Este documento tabula a **correspondência entre construtos CCT e código assembly x86-32** gerado
pelo compilador CCT em perfil freestanding. É a referência de implementação para:
- 16B.1: shim de runtime (quais símbolos o `.cgen.c` emite)
- 16C.1: driver de emissão ASM (parâmetros corretos para o cross-GCC)
- 16C.2: validador de ASM (o que verificar no `.cgen.s`)
- 16D.2: evidência de linkagem (quais símbolos esperar no ELF)

## 2. Tipos Primitivos — Mapeamento Completo

| Tipo CCT          | C gerado (perfil host) | C gerado (perfil freestanding) | Tamanho target | Seção no ELF   |
|-------------------|------------------------|--------------------------------|----------------|----------------|
| `REX`             | `int64_t`              | `int32_t`                      | 4 bytes        | `.text`/stack  |
| `DUX`             | `uint32_t`             | `uint32_t`                     | 4 bytes        | `.text`/stack  |
| `COMES`           | `float`                | `float`                        | 4 bytes        | `.text`/stack  |
| `MILES`           | `double`               | `double`                       | 8 bytes        | `.text`/stack  |
| `VERUM`           | `int` (1)              | `int32_t` (1)                  | 4 bytes        | `.text`/stack  |
| `FALSUM`          | `int` (0)              | `int32_t` (0)                  | 4 bytes        | `.text`/stack  |
| `VERBUM` (literal)| `const char*`          | `const char*` (ptr 32-bit)     | 4 bytes (ptr)  | `.rodata`      |
| `NIHIL`           | `void`                 | `void`                         | 0 bytes        | —              |
| `SPECULUM(T)`     | `T*` (64-bit)          | `T*` (32-bit)                  | 4 bytes        | `.text`/stack  |
| `SERIES T[N]`     | `T[N]`                 | `T[N]`                         | N * sz(T)      | stack/`.data`  |
| `CONSTANS T`      | `const T`              | `const T`                      | sz(T)          | `.rodata`      |

### 2.1 Diferença Crítica: REX no Target

No perfil freestanding, `REX` é `int32_t`. O preamble do `.cgen.c` deve incluir:

```c
/* Gerado por CCT --profile freestanding */
#ifndef CCT_FREESTANDING_TYPES_H
#define CCT_FREESTANDING_TYPES_H
typedef int32_t       cct_rex_t;
typedef uint32_t      cct_dux_t;
/* ... demais typedefs ... */
#endif
```

Obs: os nomes dos tipos internos do preamble são detalhes de implementação; o que importa para
a ABI é o tamanho e alinhamento na stack/rodata.

## 3. Controle de Fluxo — Mapeamento para x86-32

### 3.1 Condicional: `SI` / `ALITER` / `FIN SI`

**CCT:**
```cct
SI condicao
  -- bloco then
ALITER
  -- bloco else
FIN SI
```

**C gerado:**
```c
if (condicao) {
    /* bloco then */
} else {
    /* bloco else */
}
```

**ASM gerado (pelo cross-GCC):**
```asm
    ; Avaliar condicao → resultado em EAX
    test  eax, eax
    jz    .L_else_N       ; jump se falso
    ; bloco then
    jmp   .L_end_si_N
.L_else_N:
    ; bloco else
.L_end_si_N:
```

Labels: `.L_else_N` e `.L_end_si_N` onde N é um índice gerado pelo GCC.

### 3.2 Loop com Condição de Entrada: `DUM`

**CCT:**
```cct
DUM condicao
  -- corpo
FIN DUM
```

**C gerado:**
```c
while (condicao) {
    /* corpo */
}
```

**ASM gerado:**
```asm
.L_dum_top_N:
    ; Avaliar condicao → resultado em EAX
    test  eax, eax
    jz    .L_dum_end_N    ; sai se falso
    ; corpo
    jmp   .L_dum_top_N
.L_dum_end_N:
```

### 3.3 Loop com Condição de Saída: `DONEC ... DUM`

**CCT:**
```cct
DONEC
  -- corpo
DUM condicao
```

**C gerado:**
```c
do {
    /* corpo */
} while (condicao);
```

**ASM gerado:**
```asm
.L_donec_top_N:
    ; corpo
    ; Avaliar condicao → resultado em EAX
    test  eax, eax
    jnz   .L_donec_top_N  ; repete se verdadeiro
```

### 3.4 Loop Rangeado: `REPETE ... DE ... AD`

**CCT:**
```cct
REPETE i DE inicio AD fim
  -- corpo
FIN REPETE
```

**C gerado:**
```c
for (cct_rex_t i = inicio; i < fim; i++) {
    /* corpo */
}
```

**ASM gerado:**
```asm
    mov   dword [ebp-N], inicio    ; i = inicio
.L_repete_top_M:
    mov   eax, dword [ebp-N]
    cmp   eax, fim
    jge   .L_repete_end_M          ; sai se i >= fim
    ; corpo
    inc   dword [ebp-N]            ; i++
    jmp   .L_repete_top_M
.L_repete_end_M:
```

Com passo explícito `GRADUS s`:
```c
for (cct_rex_t i = inicio; i < fim; i += s) { /* corpo */ }
```

### 3.5 Iterador de Coleção: `ITERUM`

**CCT:**
```cct
ITERUM item IN arr COM
  -- corpo
FIN ITERUM
```

**C gerado (SERIES T[N]):**
```c
for (cct_rex_t _idx_K = 0; _idx_K < N; _idx_K++) {
    T item = arr[_idx_K];
    /* corpo */
}
```

**ASM:** similar ao REPETE com indexação de array.

**Nota**: FLUXUS não é suportado em perfil freestanding. ITERUM sobre FLUXUS resulta em erro
semântico.

### 3.6 Break: `FRANGE`

**CCT:**
```cct
DUM condicao
  SI outra_condicao
    FRANGE
  FIN SI
  -- resto do corpo
FIN DUM
```

**C gerado:** `break;`

**ASM:**
```asm
    jmp   .L_dum_end_N   ; jump direto para label de saída do loop
```

### 3.7 Continue: `RECEDE`

**C gerado:** `continue;`

**ASM:**
```asm
    jmp   .L_dum_top_N   ; jump para topo do loop (reavaliar condição)
```

Para REPETE: `jmp` para o incremento antes do topo.

## 4. Chamadas de Função — Mapeamento

### 4.1 Chamada Simples

**CCT:**
```cct
CONIURA soma(a, b)
```

**C gerado:**
```c
cct_fn_modulo_soma(a, b);
```

**ASM gerado (cdecl):**
```asm
    push  b              ; segundo argumento
    push  a              ; primeiro argumento
    call  cct_fn_modulo_soma
    add   esp, 8         ; caller limpa stack (2 args * 4 bytes)
```

### 4.2 Retorno de Valor

**CCT:**
```cct
REDDE valor
```

**C gerado:**
```c
return valor;
```

**ASM gerado:**
```asm
    mov   eax, valor     ; valor de retorno em EAX
    leave                ; restaura EBP e ESP
    ret
```

### 4.3 Chamada com Resultado Capturado

**CCT:**
```cct
EVOCA REX resultado AD CONIURA funcao(arg1, arg2)
```

**C gerado:**
```c
cct_rex_t resultado = cct_fn_mod_funcao(arg1, arg2);
```

**ASM:**
```asm
    push  arg2
    push  arg1
    call  cct_fn_mod_funcao
    add   esp, 8
    mov   dword [ebp-N], eax   ; salva resultado (EAX) na variável local
```

## 5. Operadores — Mapeamento

### 5.1 Aritméticos

| Operador CCT | C gerado | Instrução x86 principal      | Restrições                    |
|--------------|----------|------------------------------|-------------------------------|
| `+`          | `+`      | `add`                        | —                             |
| `-`          | `-`      | `sub`                        | —                             |
| `*`          | `*`      | `imul`                       | —                             |
| `/`          | `/`      | `idiv`                       | Requer EDX:EAX; divisão por 0 é UB |
| `%`          | `%`      | `idiv` (resto em EDX)        | Mesmas regras de `/`          |
| `**`         | `cct_rt_pow_i32(a,b)` | shim loop ou chamada | Shim freestanding obrigatório |
| `//`         | C expr   | `idiv` + ajuste              | Shim freestanding             |
| `%%`         | C expr   | `idiv` + ajuste              | Shim freestanding             |

**Nota sobre `**`, `//`, `%%` em freestanding**: estes operadores em host usam `cct_rt_*` que
pode chamar `pow()` de libm. Em freestanding, o shim `cct_freestanding_rt` deve implementar
`cct_fs_pow_i32`, `cct_fs_idiv_floor`, `cct_fs_emod` sem libc.

### 5.2 Comparação

| Operador CCT | C gerado | Instrução x86 (resultado int32_t: 1 ou 0) |
|--------------|----------|-------------------------------------------|
| `==`         | `==`     | `cmp` + `sete` al                         |
| `!=`         | `!=`     | `cmp` + `setne` al                        |
| `<`          | `<`      | `cmp` + `setl` al                         |
| `<=`         | `<=`     | `cmp` + `setle` al                        |
| `>`          | `>`      | `cmp` + `setg` al                         |
| `>=`         | `>=`     | `cmp` + `setge` al                        |

### 5.3 Lógicos (com short-circuit)

| Operador CCT | C gerado | Comportamento                           |
|--------------|----------|-----------------------------------------|
| `ET`         | `&&`     | Short-circuit: direito não avaliado se esquerdo falso |
| `VEL`        | `\|\|`   | Short-circuit: direito não avaliado se esquerdo verdadeiro |
| `NON`        | `!`      | Negação booleana                        |

Emissão ASM com short-circuit (ET):
```asm
    ; Avaliar esquerdo
    test  eax, eax
    jz    .L_et_false_N      ; short-circuit
    ; Avaliar direito
    test  eax, eax
    jz    .L_et_false_N
    mov   eax, 1
    jmp   .L_et_end_N
.L_et_false_N:
    xor   eax, eax
.L_et_end_N:
```

### 5.4 Bitwise e Shift

| Operador CCT | C gerado | Instrução x86                  |
|--------------|----------|--------------------------------|
| `ET_BIT`     | `&`      | `and`                          |
| `VEL_BIT`    | `\|`     | `or`                           |
| `XOR`        | `^`      | `xor`                          |
| `NON_BIT`    | `~`      | `not`                          |
| `SINISTER`   | `<<`     | `sal` / `shl`                  |
| `DEXTER`     | `>>`     | `sar` (signed) / `shr` (unsigned) |

Todos exigem operandos inteiros (`REX`, `DUX`, `COMES`, `MILES`). Erro semântico para outros tipos.

## 6. Acesso a Memória — Mapeamento

### 6.1 Ponteiros (SPECULUM)

**CCT:**
```cct
EVOCA SPECULUM REX p AD SPECULUM x     -- address-of
EVOCA REX valor AD *p                  -- dereference leitura
VINCIRE *p AD 42                       -- dereference escrita
```

**C gerado:**
```c
int32_t* p = &x;
int32_t valor = *p;
*p = 42;
```

**ASM:**
```asm
    lea   eax, [ebp-N]      ; &x (endereço de x na stack)
    mov   dword [ebp-M], eax ; p = &x

    mov   eax, dword [ebp-M] ; eax = p
    mov   eax, dword [eax]   ; valor = *p

    mov   eax, dword [ebp-M] ; eax = p
    mov   dword [eax], 42    ; *p = 42
```

### 6.2 Arrays Estáticos (SERIES)

**CCT:**
```cct
EVOCA SERIES REX[5] arr AD {1, 2, 3, 4, 5}
EVOCA REX terceiro AD arr[2]
VINCIRE arr[0] AD 99
```

**C gerado:**
```c
int32_t arr[5] = {1, 2, 3, 4, 5};
int32_t terceiro = arr[2];
arr[0] = 99;
```

**ASM:**
```asm
    ; Inicialização (via .data ou stack init)
    mov   dword [ebp-20], 1
    mov   dword [ebp-16], 2
    mov   dword [ebp-12], 3
    mov   dword [ebp-8],  4
    mov   dword [ebp-4],  5

    ; arr[2] → base + 2 * sizeof(int32_t) = base + 8
    mov   eax, dword [ebp-12]

    ; arr[0] = 99
    mov   dword [ebp-20], 99
```

## 7. `CONSTANS` — Mapeamento

**CCT:**
```cct
EVOCA CONSTANS REX LIMITE AD 100
```

**C gerado:**
```c
const int32_t LIMITE = 100;
```

**No context de variável local**: emitido como `const` local na stack (o GCC pode otimizar para
imediato direto no assembly).

**No contexto de global do módulo**: emitido em `.rodata`.

```asm
section .rodata
cct_const_modulo_LIMITE:
    dd 100
```

## 8. Superfície Suportada vs Proibida no Perfil Freestanding

### 8.1 Suportado (compilação normal, sem erro)

| Construto CCT                   | Notas                                   |
|---------------------------------|-----------------------------------------|
| `REX`, `DUX`, `COMES`, `MILES` | Com mapeamento de tamanho (REX = 32-bit)|
| `VERUM`, `FALSUM`, `NIHIL`      | —                                       |
| `VERBUM` (literal)              | Ponteiro para `.rodata`                 |
| `SPECULUM(T)`                   | Ponteiro 32-bit                         |
| `SERIES T[N]` (N constante)     | Apenas tamanho estático                 |
| `CONSTANS T`                    | Emitido como `const` / `.rodata`        |
| `SI`/`ALITER`/`FIN SI`          | —                                       |
| `DUM`, `DONEC`                  | —                                       |
| `REPETE` (com passo constante)  | —                                       |
| `ITERUM item IN SERIES`         | Apenas sobre SERIES estático            |
| `FRANGE`, `RECEDE`              | Dentro dos loops suportados             |
| `CONIURA` (chamada de função)   | cdecl 32-bit                            |
| `REDDE`                         | Valor em EAX                            |
| `ET`, `VEL`, `NON`              | Com short-circuit                       |
| `ET_BIT`, `VEL_BIT`, `XOR`, `NON_BIT`, `SINISTER`, `DEXTER` | Inteiros |
| `+`, `-`, `*`, `/`, `%`         | —                                       |
| `**`, `//`, `%%`                | Via shim freestanding (cct_fs_*)        |
| `RITUALE` (sem exceção)         | cdecl, naming cct_fn_*                  |
| `ADVOCARE` (módulo válido)      | Apenas módulos freestanding-compatíveis |
| `cct/kernel`                    | Módulo exclusivo freestanding           |
| `SIGILLUM` (struct simples)     | Sem contratos de exceção                |
| `GENUS` (genérico estático)     | Monomorphização estática OK             |

### 8.2 Proibido (erro semântico com `--profile freestanding`)

| Construto/Módulo               | Código de erro semântico        | Motivo                               |
|--------------------------------|---------------------------------|--------------------------------------|
| `IACE`                         | `FS_ERR_IACE_FORBIDDEN`         | Requer stack unwinding               |
| `TEMPTA`                       | `FS_ERR_TEMPTA_FORBIDDEN`       | Requer runtime de exceção            |
| `CAPE`                         | `FS_ERR_CAPE_FORBIDDEN`         | Idem                                 |
| `SEMPER`                       | `FS_ERR_SEMPER_FORBIDDEN`       | Idem                                 |
| `FLUXUS`                       | `FS_ERR_FLUXUS_FORBIDDEN`       | Requer malloc/free                   |
| `ADVOCARE "cct/io"`            | `FS_ERR_MODULE_FORBIDDEN`       | Requer syscalls de OS                |
| `ADVOCARE "cct/fs"`            | `FS_ERR_MODULE_FORBIDDEN`       | Requer syscalls de OS                |
| `ADVOCARE "cct/fluxus"`        | `FS_ERR_MODULE_FORBIDDEN`       | Heap dinâmico                        |
| `ADVOCARE "cct/map"`           | `FS_ERR_MODULE_FORBIDDEN`       | Heap dinâmico                        |
| `ADVOCARE "cct/set"`           | `FS_ERR_MODULE_FORBIDDEN`       | Heap dinâmico                        |
| `ADVOCARE "cct/random"`        | `FS_ERR_MODULE_FORBIDDEN`       | Depende de libc (srand/rand)         |
| `SERIES T[n]` (n dinâmico)     | `FS_ERR_DYNAMIC_ARRAY`          | VLA não garantido no target          |
| `ITERUM item IN FLUXUS`        | `FS_ERR_FLUXUS_FORBIDDEN`       | FLUXUS proibido                      |
| `OBSECRO pete(...)` / `OBSECRO libera(...)` | `FS_ERR_HEAP_FORBIDDEN` | Requer malloc/free           |

### 8.3 Símbolos de Runtime Proibidos no Objeto Final

Além dos construtos CCT, o validador 16C.2 deve rejeitar qualquer símbolo undefined no `.o`
que pertença às seguintes categorias:

**Símbolos de libc:**
```
printf  fprintf  sprintf  snprintf  puts  fputs  fwrite  fread
malloc  calloc   realloc  free
memcpy  memset   memmove  memcmp  strlen  strcmp  strncmp
abort   exit     _exit    atexit
fopen   fclose   fseek    ftell   rewind  feof
```

**Helpers de compilador (gerados automaticamente pelo GCC) — igualmente proibidos:**
```
__stack_chk_fail        # proteção de stack (eliminada por -fno-stack-protector)
__stack_chk_guard       # idem
__udivdi3               # divisão uint64 em x86-32 (evitar operações REX/int64 em freestanding)
__divdi3                # divisão int64 em x86-32
__umoddi3               # módulo uint64 em x86-32
__moddi3                # módulo int64 em x86-32
__muldi3                # multiplicação int64 em x86-32
__fixunsdfsi            # conversão double→uint32
__fixdfsi               # conversão double→int32
__floatsidf             # conversão int32→double
__floatsisf             # conversão int32→float
__divdf3 __muldf3       # aritmética de double (soft-float)
__adddf3 __subdf3       # idem
__eqdf2 __nedf2         # comparação de double (soft-float)
__ledf2 __gedf2 __ltdf2 __gtdf2  # idem
```

**Política de tratamento:**
- Se `__stack_chk_fail` aparecer: build error — flag `-fno-stack-protector` não foi aplicada.
- Se `__udivdi3` / `__divdi3` aparecer: erro de design — operação int64 (`REX` host) usada em
  contexto freestanding onde `REX` deve ser int32. Revisar o código CCT gerado.
- Se outros helpers soft-float aparecerem: aviso se `COMES`/`MILES` estiver em uso; erro se
  o código não usa tipos float — indica regressão no codegen.
- Shim explícito em `cct_freestanding_rt.c` é aceito para helpers inevitáveis, desde que
  a implementação não referencie libc.

### 8.4 Aviso (warning, não erro)

| Construto         | Mensagem de aviso                                             |
|-------------------|---------------------------------------------------------------|
| `MILES` (double)  | `aviso: MILES usa FPU x87; confirmar disponibilidade no target` |
| `COMES` (float)   | `aviso: COMES usa FPU x87; confirmar disponibilidade no target` |

## 9. Seções ELF32 — Destino de Cada Construto

| Construto CCT                     | Seção ELF32  |
|-----------------------------------|--------------|
| Código de função (`RITUALE`)      | `.text`      |
| Literal string (`VERBUM`)         | `.rodata`    |
| `CONSTANS` global                 | `.rodata`    |
| Variável global mutável           | `.data`      |
| Variável global não inicializada  | `.bss`       |
| Variável local (escopo de função) | Stack        |
| Parâmetros de função              | Stack        |

## 10. Flags de Compilação Freestanding Obrigatórias

Quando o CCT invoca o cross-GCC para gerar o `.cgen.s`, o conjunto de flags abaixo é
**obrigatório e não negociável**. Omitir qualquer uma delas pode introduzir símbolos
proibidos ou seções indesejadas no objeto final.

```
<cross_cc> -m32 -S -masm=intel \
    -ffreestanding -nostdlib -fno-pic \
    -fno-stack-protector \
    -fno-builtin \
    -fno-asynchronous-unwind-tables \
    -fno-unwind-tables \
    -I <cct_runtime_dir> <module>.cgen.c -o <module>.cgen.s
```

| Flag                              | Símbolo/Seção eliminada               | Risco se omitida                          |
|-----------------------------------|---------------------------------------|-------------------------------------------|
| `-fno-stack-protector`            | `__stack_chk_fail`, `__stack_chk_guard` | Undefined symbol ao montar no LBOS      |
| `-fno-builtin`                    | Substituições de `memcpy`/`memset` por versões de libc | Referência a libc inesperada |
| `-fno-asynchronous-unwind-tables` | Seção `.eh_frame`                     | Aumenta tamanho do objeto; quebra linker LBOS |
| `-fno-unwind-tables`              | Tabelas de unwind residuais           | Idem                                      |
| `-ffreestanding`                  | Presunção de `main` e crt0            | GCC assume ambiente hosted                |
| `-nostdlib`                       | Linkagem automática de libc e libgcc  | Undefined symbols de libc ao link         |
| `-fno-pic`                        | PLT/GOT (position-independent calls)  | Ponteiros quebrados no modelo de memória flat do LBOS |

### 10.1 Verificação de Flags Aplicadas

O script `tools/validate_freestanding_asm.sh` (16C.2) deve verificar como etapa zero
que as flags foram de fato aplicadas, inspecionando o `.cgen.s` por ausência de
sintomas conhecidos:

```bash
# Nenhuma referência a __stack_chk_fail
grep -q "__stack_chk_fail" "$ASM_FILE" && echo "FAIL: -fno-stack-protector ausente" && exit 1

# Nenhuma seção .eh_frame
grep -q "\.eh_frame" "$ASM_FILE" && echo "FAIL: -fno-unwind-tables ausente" && exit 1

# Confirmação de Intel syntax
head -3 "$ASM_FILE" | grep -q "\.intel_syntax noprefix" || \
    { echo "FAIL: -masm=intel ausente ou sintaxe incorreta"; exit 1; }
```

## 11. Verificação de Coerência com F7.CCT do LBOS

Este documento foi criado com base em:
- `FASE_16_CCT.md`, seções 4 e 5
- `third_party/cct-boot/PROMPT_IMPLEMENTACAO_F7CCT_BRIDGE_ABI_V0.md`

Verificações a realizar ao criar este documento:

- [ ] Mapeamento de tipos coerente com o que F7.CCT espera (especialmente REX = int32_t).
- [ ] Controle de fluxo lowered corretamente (labels de loop, jcc correto).
- [ ] Chamada cdecl corretamente descrita (direita-esquerda, caller limpa).
- [ ] Superfície proibida coerente com invariantes do LBOS (sem malloc, sem syscall, sem unwinding).

Divergências conhecidas: nenhuma na V0.
