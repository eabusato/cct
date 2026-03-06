# CCT Symbol Naming V0 — Contrato de Nomenclatura de Símbolos

- Versão: V0
- Status: Ativo (FASE 16A.1)
- Dependência: CCT_ABI_V0_LBOS.md V0; LBOS F7.CCT Bridge ABI V0.
- Próxima revisão: V1 (quando FASE 17 iniciar auto-bootstrap).

## 1. Propósito

Este documento define o **padrão determinístico de nomenclatura de símbolos** para todo código
CCT compilado com `--profile freestanding`. O objetivo é garantir:

1. **Não-colisão com namespaces do LBOS**: os símbolos CCT não conflitam com `k0_*`, `k1_*`,
   `svc_*`, `cct_rt_*` (host-only), nem com qualquer namespace interno do LBOS.
2. **Determinismo**: dado o mesmo módulo e rituale, o nome do símbolo é sempre o mesmo.
3. **Rastreabilidade**: o nome do símbolo permite identificar sua origem (módulo + nome original).
4. **Compatibilidade com linker ELF32**: os nomes são válidos como identificadores C e símbolos ELF.

## 2. Namespaces Reservados pelo LBOS (Não Usar)

| Prefixo      | Dono          | Exemplos                     |
|--------------|---------------|------------------------------|
| `k0_`        | LBOS kernel0  | `k0_init`, `k0_page_alloc`   |
| `k1_`        | LBOS kernel1  | `k1_sched`, `k1_ipc`         |
| `svc_`       | LBOS services | `svc_video`, `svc_serial`    |
| `cct_rt_`    | CCT runtime host-only | `cct_rt_fluxus_init` |

O CCT **nunca** emite símbolos com estes prefixos em perfil freestanding.

## 3. Padrões de Nomenclatura CCT

### 3.1 Funções (RITUALE)

Padrão: `cct_fn_<mod>_<rituale>`

| Elemento    | Regra                                                        |
|-------------|--------------------------------------------------------------|
| `cct_fn_`   | Prefixo fixo; identifica função CCT freestanding             |
| `<mod>`     | Nome do módulo CCT, normalizado (veja seção 4)               |
| `<rituale>` | Nome da função CCT, normalizado (veja seção 4)               |

Exemplos:

| Declaração CCT                               | Símbolo gerado                     |
|----------------------------------------------|------------------------------------|
| `RITUALE kernel_halt()` em módulo `kernel`   | `cct_fn_kernel_kernel_halt`        |
| `RITUALE soma(REX a, REX b)` em `math_utils` | `cct_fn_math_utils_soma`          |
| `RITUALE main()` em módulo `hello`           | `cct_fn_hello_main`                |
| `RITUALE init_stage()` em módulo `boot_cct`  | `cct_fn_boot_cct_init_stage`       |

### 3.2 Módulos (identificador de módulo compilado)

Padrão: `cct_mod_<hash8>_<name>`

| Elemento    | Regra                                                           |
|-------------|-----------------------------------------------------------------|
| `cct_mod_`  | Prefixo fixo; identifica descriptor de módulo CCT              |
| `<hash8>`   | Primeiros 8 caracteres hex (minúsculo) do SHA-256 do caminho canônico |
| `<name>`    | Nome normalizado do módulo (veja seção 4)                       |

**Definição de caminho canônico** (determinismo entre hosts):
- Caminho **relativo à raiz do repositório CCT**, separador sempre `/` (Unix), sem `./` inicial.
- Exemplo: `lib/cct/kernel/kernel.cct` — nunca caminho absoluto, nunca `.\` Windows.
- O compilador resolve o caminho canônico antes de calcular o hash; dois hosts com o mesmo
  repositório sempre geram o mesmo `<hash8>` para o mesmo módulo.

```
sha256("lib/cct/kernel/kernel.cct") → "a3f9d21b..."
hash8 = "a3f9d21b"
```

Este símbolo é emitido como variável de metadata de módulo no `.o` (somente-leitura, opcional).

Exemplo:
- Módulo `lib/cct/kernel/kernel.cct` → `cct_mod_a3f9d21b_kernel`

### 3.3 Shims de Serviço

Padrão: `cct_svc_<name>`

Shims de serviço são funções de glue que o CCT emite para operações de baixo nível sem nome
de módulo específico (ex: operações de I/O de hardware).

| Elemento    | Regra                                         |
|-------------|-----------------------------------------------|
| `cct_svc_`  | Prefixo fixo; identifica shim de serviço CCT  |
| `<name>`    | Nome descritivo da operação                   |

Exemplos:
- `cct_svc_outb` — wrapper de `outb` para I/O de porta
- `cct_svc_inb` — wrapper de `inb`
- `cct_svc_halt` — wrapper de `hlt`

### 3.4 Variáveis Globais

Padrão: `cct_g_<mod>_<nome>`

| Elemento   | Regra                                                   |
|------------|---------------------------------------------------------|
| `cct_g_`   | Prefixo fixo; identifica variável global CCT freestanding|
| `<mod>`    | Nome normalizado do módulo de origem                    |
| `<nome>`   | Nome normalizado da variável                            |

Exemplos:
- `cct_g_kernel_estado_atual` para variável global `estado_atual` no módulo `kernel`
- `cct_g_boot_cct_contador` para `contador` em `boot_cct`

### 3.5 Literais de String (rodata)

Padrão: `cct_str_<mod>_<idx>`

Strings literais `VERBUM` em perfil freestanding são emitidas em `.rodata`.

| Elemento   | Regra                                              |
|------------|----------------------------------------------------|
| `cct_str_` | Prefixo fixo; identifica literal string CCT        |
| `<mod>`    | Nome normalizado do módulo de origem               |
| `<idx>`    | Índice sequencial de string dentro do módulo (0-based) |

Exemplo: `cct_str_kernel_0`, `cct_str_kernel_1`, etc.

### 3.6 Constantes (`CONSTANS`) em rodata

Padrão: `cct_const_<mod>_<nome>`

Constantes com escopo global (declaradas no topo de módulo) são emitidas em `.rodata`.

Exemplo: `CONSTANS REX VERSAO AD 1` em módulo `boot_cct` → `cct_const_boot_cct_VERSAO`

## 4. Regras de Normalização de Nomes

A normalização converte nomes CCT (que podem ter caracteres não-ASCII ou separadores especiais)
para identificadores C válidos.

### 4.1 Algoritmo de Normalização

```
input:  string arbitrária (nome de módulo ou rituale)
output: identificador C válido com apenas [a-z0-9_], nunca começando com dígito

1. Converter para minúsculas.
2. Substituir qualquer sequência de caracteres fora de [a-z0-9] por underscore único.
3. Remover underscores iniciais e finais.
4. Se o primeiro caractere for dígito [0-9]: prefixar com "n_"
   (ex: "123abc" → "n_123abc"; garante identificador C válido).
5. Se string vazia após normalização: usar "anon".
```

### 4.2 Exemplos de Normalização

| Nome original      | Normalizado       | Observação                          |
|--------------------|-------------------|-------------------------------------|
| `kernel`           | `kernel`          | —                                   |
| `math_utils`       | `math_utils`      | —                                   |
| `boot-cct`         | `boot_cct`        | hífen → underscore                  |
| `MyModule`         | `mymodule`        | caixa alta → minúscula              |
| `cct/kernel`       | `cct_kernel`      | `/` → underscore                    |
| `meu módulo`       | `meu_m_dulo`      | espaço e não-ASCII → underscore     |
| `123abc`           | `n_123abc`        | dígito inicial → prefixo `n_`       |
| `99`               | `n_99`            | idem; sem letras restantes          |

### 4.3 Colisões de Normalização

Se dois nomes CCT diferentes normalizarem para o mesmo identificador, o compilador deve:
1. Emitir um aviso de naming collision.
2. Desambiguar pelo hash do caminho canônico: `cct_fn_<mod>_<rituale>__<hash4>`.

## 5. Entry Points Especiais

### 5.1 Entry Point Definido pelo Usuário (`--entry`)

Quando o usuário especifica `--entry <nome_rituale>`:
- O RITUALE `<nome_rituale>` é emitido com seu símbolo canônico `cct_fn_<mod>_<nome_rituale>`
  marcado como `.globl` no assembly gerado.
- Nenhum wrapper `int main(void)` é gerado em perfil freestanding.
- O LBOS faz `CALL cct_fn_<mod>_<nome_rituale>` diretamente após o handoff.

Exemplo:
```
cct compile --profile freestanding --entry kernel_halt lib/cct/kernel/kernel.cct
```
Gera símbolo global: `cct_fn_kernel_kernel_halt`

### 5.2 Sem Entry Point (biblioteca pura)

Se nenhum `--entry` é especificado em perfil freestanding:
- Todos os RITUALEs públicos (sem `ARCANUM`) são emitidos como `.globl`.
- RITUALEs com `ARCANUM` são emitidos sem `.globl` (visibilidade local ao objeto).

## 6. Verificação de Não-Colisão

O script `tools/validate_freestanding_asm.sh` (criado em 16C.2) verifica automaticamente que
nenhum símbolo CCT emitido usa prefixo reservado do LBOS.

Verificação manual:
```bash
# awk '{print $3}' extrai a coluna do nome do símbolo (formato nm: <addr> <type> <name>)
nm output.o | awk '$2 == "T" {print $3}' | grep -E '^(k0_|k1_|svc_|cct_rt_)'
# Deve retornar vazio
```

## 7. Compatibilidade com Linker ELF32

Todos os símbolos gerados segundo este contrato são:
- Strings ASCII puras ([a-z0-9_], no máximo 255 caracteres).
- Válidos como nomes de símbolo ELF conforme System V ABI i386.
- Únicos dentro do escopo de um único módulo compilado.

## 8. Verificação de Coerência com F7.CCT do LBOS

Este documento foi criado com base em:
- `FASE_16_CCT.md`, seção 4.3 (Naming de Símbolos)
- `third_party/cct-boot/PROMPT_IMPLEMENTACAO_F7CCT_BRIDGE_ABI_V0.md`

Pontos de verificação:
- [ ] Prefixo `cct_fn_` não conflita com namespaces do LBOS.
- [ ] Prefixo `cct_mod_` não conflita com namespaces do LBOS.
- [ ] Prefixo `cct_svc_` não conflita com `svc_` do LBOS — `svc_` ≠ `cct_svc_`; distinção
      confirmada: `cct_svc_` tem prefixo mais longo e não é substring de `svc_`.
- [ ] Entry point do LBOS espera símbolo `cct_fn_<mod>_<rituale>` — confirmar.

Divergências conhecidas: nenhuma na V0.
