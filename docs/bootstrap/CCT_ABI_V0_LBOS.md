# CCT ABI V0 — Convenção de Interface CCT ↔ LBOS

- Versão: V0
- Status: Ativo (FASE 16A.1)
- Dependência: FASE 15D.4 fechada; LBOS F7.CCT Bridge ABI V0 especificada.
- Próxima revisão: V1 (quando FASE 17 iniciar auto-bootstrap).

## 1. Escopo e Propósito

Este documento formaliza a **Application Binary Interface (ABI)** entre código CCT compilado com
`--profile freestanding` e o ambiente de execução do LBOS (Labyrinthine Boot Operating Substrate),
um sistema operacional bare-metal x86-32.

Este contrato é unilateral do lado CCT: o LBOS define suas expectativas (F7.CCT), e o CCT
compromete-se a produzi-las. Qualquer mudança neste documento exige revisão de coerência com
`third_party/cct-boot/PROMPT_IMPLEMENTACAO_F7CCT_BRIDGE_ABI_V0.md`.

## 2. Ambiente de Execução do Target

| Propriedade              | Valor                                            |
|--------------------------|--------------------------------------------------|
| Arquitetura              | x86 32-bit (modo protegido, sem paginação init)  |
| Modo de operação         | Near 32-bit (CS:EIP com base 0)                  |
| Sistema operacional      | Nenhum (bare-metal)                              |
| Runtime C                | Ausente (sem libc, sem crt0, sem crt1)           |
| Heap dinâmico            | Ausente (sem malloc/free)                        |
| Floating point           | FPU x87 disponível (não-SSE garantido)           |
| Endereço máximo          | 0x000A0000 (limite hard do LBOS)                 |
| Endianness               | Little-endian (padrão x86)                       |

## 3. Convenção de Chamada (cdecl x86-32)

### 3.1 Modelo

O CCT adota **cdecl** como convenção de chamada universal no perfil freestanding:

- Argumentos passados via stack, **da direita para a esquerda** (último argumento empilhado primeiro).
- O **chamador (caller)** é responsável por limpar a stack após a chamada.
- Valor de retorno: registrador **EAX** (32-bit inteiro) ou **EAX:EDX** (64-bit estendido, se necessário).
- Ponteiros retornados: **EAX** (32-bit, endereço no espaço x86-32).

### 3.2 Registradores

| Categoria        | Registradores          | Responsabilidade        |
|------------------|------------------------|-------------------------|
| Callee-saved     | EBX, ESI, EDI, EBP     | Função chamada preserva |
| Caller-saved     | EAX, ECX, EDX          | Chamador não pode confiar após call |
| Stack pointer    | ESP                     | Mantido pelo caller/callee conforme cdecl |
| Segmento         | CS, DS, SS, ES          | Mantidos pelo LBOS; CCT não muda |

### 3.3 Alinhamento de Stack

- Stack deve estar alinhada a **4 bytes** imediatamente antes de uma instrução `CALL`.
- O compilador CCT (via GCC host com `-m32`) garante este alinhamento no `.cgen.c` gerado.
- Aviso: versões do GCC com `-m32 -mpreferred-stack-boundary=4` requerem alinhamento de 16 bytes
  para SSE; no perfil freestanding CCT sem SSE, 4 bytes é suficiente e é o contrato.

### 3.4 Prólogo e Epílogo Padrão de Função

Toda função CCT gerada em perfil freestanding deve ter o padrão:

```asm
; Prólogo
push ebp
mov  ebp, esp
sub  esp, <tamanho_locals>

; ... corpo ...

; Epílogo
mov  esp, ebp
pop  ebp
ret
```

Alternativa equivalente (GCC pode emitir):
```asm
push ebp
mov  ebp, esp
; ...
leave
ret
```

Ambas as formas são aceitáveis. O script de validação 16C.2 verifica a presença de `ret` no epílogo.

## 4. Sinalização de Erros

Em ambiente bare-metal, não há mecanismo de exceção de sistema operacional. O CCT usa:

| Mecanismo         | Uso                                                              |
|-------------------|------------------------------------------------------------------|
| `EAX`             | Valor de retorno de funções `cct_fn_*` (código de erro ou resultado) |
| `CF` (carry flag) | Sinalização de erro **exclusivamente em wrappers `cct_svc_*`** — nunca em `cct_fn_*` direto |
| `kernel_halt()`   | Parada definitiva em falha irrecuperável (instrução `hlt` em loop)|

> **Limitação crítica — CF não é configurável via C compilado com GCC:**
> GCC gerando código C para x86-32 não emite `stc`/`clc` como parte do contrato de retorno
> de função. Portanto:
> - Funções `cct_fn_*` (RITUALEs CCT compilados): retornam resultado/erro **somente em EAX**,
>   nunca manipulam CF.
> - Funções `cct_svc_*` (wrappers de borda LBOS): **devem ser escritas com `__asm__ volatile`**
>   e explicitamente emitem `clc` (sucesso) ou `stc` (erro) antes de `ret`.
>   São a única camada que traduz o modelo C (EAX) para o modelo LBOS (CF+EAX).

### 4.1 Manifest de ABI por Símbolo de Serviço

Para cada `cct_svc_*` exposto ao LBOS, o manifest completo é:

| Símbolo           | Argumentos (stack, dir→esq)             | Retorno EAX         | Clobber adicional    | CF no retorno           | Política de erro            |
|-------------------|-----------------------------------------|---------------------|----------------------|-------------------------|-----------------------------|
| `cct_svc_outb`    | porta `uint16` (4B), val `uint8` (4B)   | sem significado     | nenhum               | sempre CF=0             | sem falha possível          |
| `cct_svc_inb`     | porta `uint16` (4B)                     | byte lido (bits 7:0)| nenhum               | sempre CF=0             | sem falha possível          |
| `cct_svc_halt`    | nenhum                                  | nunca retorna       | todos                | não retorna             | loop `hlt` (fail-stop)      |
| `cct_svc_memcpy`  | dst `ptr32`, src `ptr32`, n `int32`     | sem significado     | ESI, EDI, ECX        | sempre CF=0 se retorna  | ptr nulo → `cct_svc_halt()` (fail-stop; não retorna) |
| `cct_svc_memset`  | dst `ptr32`, val `int32`, n `int32`     | sem significado     | EDI, ECX             | sempre CF=0 se retorna  | ptr nulo → `cct_svc_halt()` (fail-stop; não retorna) |
| `cct_svc_panic`   | msg `ptr32` (rodata)                    | nunca retorna       | todos                | não retorna             | loop `hlt` (fail-stop)      |

> **Política de erro explícita para `cct_svc_memcpy` e `cct_svc_memset`**: estas funções são
> **fail-stop** — em caso de ponteiro nulo chamam `cct_svc_halt()` e nunca retornam. Portanto
> CF=1 nunca é emitido; se a função retorna, CF é sempre 0. Não há código de tratamento de erro
> do chamador: ou a operação termina com sucesso (CF=0), ou o sistema para (halt).

Implementação obrigatória dos wrappers com inline ASM:

```c
/* cct_svc_outb: CF=0 explícito (clc), sem erro possível */
static inline void cct_svc_outb(uint16_t port, uint8_t val) {
    __asm__ volatile (
        "outb %b0, %w1\n\t"
        "clc\n\t"
        : : "a"(val), "Nd"(port) : "memory"
    );
}

/* cct_svc_inb: CF=0 explícito, retorno em AL (EAX bits 7:0) */
static inline uint8_t cct_svc_inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile (
        "inb %w1, %b0\n\t"
        "clc\n\t"
        : "=a"(result) : "Nd"(port) : "memory"
    );
    return result;
}

/* cct_svc_halt: nunca retorna */
static inline void __attribute__((noreturn)) cct_svc_halt(void) {
    __asm__ volatile ("1: hlt\n\t jmp 1b\n\t" : : : "memory");
    __builtin_unreachable();
}

/* cct_svc_memcpy: fail-stop — ptr nulo = halt (nunca retorna); se retorna, CF=0 sempre */
static inline void cct_svc_memcpy(void *dst, const void *src, int32_t n) {
    if (!dst || !src) { cct_svc_halt(); } /* halt; não retorna */
    __asm__ volatile (
        "rep movsb\n\t"
        "clc\n\t"   /* CF=0: sucesso garantido se chegou aqui */
        : : "D"(dst), "S"(src), "c"(n) : "memory", "cc"
    );
}
```

O CCT **não** usa `IACE`/`TEMPTA`/`CAPE`/`SEMPER` em perfil freestanding — estes construtos
requerem stack unwinding e runtime de exceção, ausentes no target. Tentar usar estes construtos
com `--profile freestanding` resulta em erro semântico (implementado em 16A.3).

## 5. Mapeamento de Tipos CCT → x86-32

| Tipo CCT          | Tipo C gerado          | Tamanho   | Representação no target             |
|-------------------|------------------------|-----------|-------------------------------------|
| `REX`             | `int32_t`              | 4 bytes   | Inteiro com sinal 32-bit            |
| `DUX`             | `uint32_t`             | 4 bytes   | Inteiro sem sinal 32-bit            |
| `COMES`           | `float`                | 4 bytes   | IEEE 754 single via FPU x87         |
| `MILES`           | `double`               | 8 bytes   | IEEE 754 double via FPU x87         |
| `VERUM`/`FALSUM`  | `int32_t`              | 4 bytes   | 1 (verdadeiro) / 0 (falso)         |
| `VERBUM`          | `uint32_t` (ptr)       | 4 bytes   | Ponteiro 32-bit para literal rodata |
| `SPECULUM(T)`     | `T*` (ptr 32-bit)      | 4 bytes   | Endereço 32-bit                     |
| `SERIES T[N]`     | `T[N]` (array estático)| N * sz(T) | Bloco contíguo na stack/rodata      |
| `NIHIL`           | `void`                 | 0 bytes   | Sem representação                   |
| `CONSTANS T`      | `const T`              | sz(T)     | Emitido em `.rodata` quando global  |

### 5.1 Notas sobre `REX` no Target

No host x86_64, `REX` é `int64_t` (8 bytes). **No perfil freestanding x86-32, `REX` é mapeado
para `int32_t` (4 bytes)**. Este é o único caso de mudança de semântica entre perfis.

O codegen deve respeitar este mapeamento: quando `--profile freestanding`, o preamble do `.cgen.c`
define `typedef int32_t cct_rex_t;` em vez de `typedef int64_t cct_rex_t;`.

Consequência: valores `REX` maiores que `2^31 - 1` ou menores que `-2^31` são indefinidos no target.
Esta limitação deve ser documentada nas restrições do perfil freestanding em `docs/spec.md`.

### 5.2 Tipos Não Suportados no Perfil Freestanding

| Tipo/Construto    | Motivo da Ausência                                |
|-------------------|---------------------------------------------------|
| `FLUXUS`          | Requer `malloc`/`free` — heap indisponível        |
| `IACE`/`TEMPTA`   | Requer runtime de exceção — ausente no target     |
| `SERIES T[n]` (n dinâmico) | VLA não garantido; stack limit no target |

## 6. Passagem de Argumentos — Exemplos Concretos

### Função com 3 argumentos REX

```cct
RITUALE soma_tres(REX a, REX b, REX c) REDDE REX
  REDDE a + b + c
EXPLICIT RITUALE
```

Stack no momento do `CALL cct_fn_modulo_soma_tres`:
```
[ESP+0]  → endereço de retorno
[ESP+4]  → a  (primeiro argumento, 4 bytes)
[ESP+8]  → b  (segundo argumento, 4 bytes)
[ESP+12] → c  (terceiro argumento, 4 bytes)
```

O caller empilha c, depois b, depois a (direita → esquerda), depois faz `call`.

### Função com ponteiro e REX

```cct
RITUALE kernel_memset(SPECULUM REX dst, REX valor, REX n) REDDE NIHIL
```

Stack:
```
[ESP+4]  → dst (ponteiro 32-bit)
[ESP+8]  → valor (int32_t)
[ESP+12] → n (int32_t)
```

## 7. Invariantes Absolutos do LBOS

As seguintes regras são impostas pelo LBOS e nunca podem ser violadas pelo CCT:

1. **Sem libc**: nenhum símbolo de libc pode aparecer como referência undefined no `.o` final.
   Verificação: `nm <obj> | grep 'U '` deve retornar vazio ou apenas símbolos CCT internos.

2. **Sem binário CCT no target**: o compilador CCT roda exclusivamente no host. O output (`.o`)
   é copiado para o LBOS e linkado; o compilador em si nunca executa no target.

3. **Ponteiros abaixo de 0x000A0000**: nenhum ponteiro CCT pode referenciar ou cruzar este
   endereço no target. O controle do layout de memória é do LBOS; o CCT respeita sem questionar.

4. **Formato de output aceito**: apenas ASM (`.cgen.s` em Intel syntax GAS) ou objeto ELF32
   montado por `as --32`. Outros formatos não são consumidos pelo LBOS.

## 8. Verificação de Coerência com F7.CCT do LBOS

Este documento foi criado com base em:
- `FASE_16_CCT.md`, seção 4 (Contrato ABI V0)
- `third_party/cct-boot/PROMPT_IMPLEMENTACAO_F7CCT_BRIDGE_ABI_V0.md`

Divergências conhecidas: nenhuma na V0.

Qualquer atualização futura deste documento deve incluir a verificação de coerência com
`PROMPT_IMPLEMENTACAO_F7CCT_BRIDGE_ABI_V0.md` e registrar divergências ou confirmações
na seção 8.
