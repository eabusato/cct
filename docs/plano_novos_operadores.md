# Plano de Implementação: Novos Operadores para CCT

## Contexto

Este plano implementa operadores adicionais para a linguagem CCT, divididos em três níveis de complexidade: TRIVIAIS, FÁCEIS e MÉDIOS. A implementação segue o padrão arquitetural existente no compilador CCT, que possui 4 fases principais:

1. **Lexer** (`src/lexer/`) - Tokenização
2. **Parser** (`src/parser/`) - Análise sintática e construção da AST
3. **Semantic** (`src/semantic/`) - Análise semântica e verificação de tipos
4. **Codegen** (`src/codegen/`) - Geração de código C

---

## FASE 1: OPERADORES TRIVIAIS (⭐☆☆☆☆)

### Operador 1: `^` (Potenciação - estilo R)

#### 1.1 Lexer - Adicionar Token
**Arquivo:** `src/lexer/lexer.h`

**Localização:** Após linha 122 (depois de `TOKEN_PERCENT`)

**Ação:** Adicionar novo token ao enum:
```c
TOKEN_CARET,            /* ^ */
```

**Arquivo:** `src/lexer/lexer.c`

**Localização:** Linha 317 (no switch do scanner)

**Ação:** Adicionar case para tokenizar `^`:
```c
case '^': return make_token(lexer, TOKEN_CARET);
```

**Localização:** Linha ~357+ (função `cct_token_type_string`)

**Ação:** Adicionar string representation:
```c
case TOKEN_CARET: return "CARET";
```

#### 1.2 Parser - Definir Precedência
**Arquivo:** `src/parser/parser.c`

**Localização:** Função `get_precedence()` linha 666-699

**Ação:** Adicionar precedência MAIOR que multiplicação (precedência 12):
```c
case TOKEN_CARET:
    return 12;  /* Power has higher precedence than *, /, % */
```

**Nota Crítica:** Potenciação é **RIGHT-ASSOCIATIVE** (2^3^2 = 2^(3^2) = 512).

**Localização:** Função `parse_precedence()` linha 714

**Ação:** Modificar para suportar associatividade à direita:
```c
/* Before (line 714): */
cct_ast_node_t *right = parse_precedence(parser, prec + 1);

/* After: */
cct_ast_node_t *right = parse_precedence(parser,
    (op == TOKEN_CARET) ? prec : prec + 1);  /* Right-assoc for ^ */
```

#### 1.3 Semantic - Type Checking
**Arquivo:** `src/semantic/semantic.c`

**Localização:** Função de análise de binary_op, após linha 2170 (dentro do switch de operadores)

**Ação:** Adicionar case para `^`:
```c
case TOKEN_CARET:  /* Power operator ^ */
    /* Both operands must be numeric */
    if (!sem_is_numeric_type(left) || !sem_is_numeric_type(right)) {
        sem_report_node(sem, expr, "operator ^ requires numeric operands");
        return &sem->type_error;
    }
    /* If either is real, result is real; otherwise integer */
    return sem_arithmetic_result_type(sem, left, right);
```

#### 1.4 Codegen - Emissão de Código C
**Arquivo:** `src/codegen/codegen.c`

**Localização:** Função `cg_emit_binary_expr()` linha ~1644 (dentro do switch)

**Ação:** Adicionar case para gerar chamada a `pow()`:
```c
case TOKEN_CARET: {
    /* Emit parenthesized pow() call */
    fputs("pow(", out);
    cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
    fputs(", ", out);
    cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
    fputs(")", out);

    /* Validate numeric operands */
    if (!cct_cg_value_kind_is_numeric(lhs_kind) ||
        !cct_cg_value_kind_is_numeric(rhs_kind)) {
        cg_report_node(cg, expr, "operator ^ requires numeric operands");
        return false;
    }

    /* Result is real if either operand is real */
    if (out_kind) {
        *out_kind = (lhs_kind == CCT_CODEGEN_VALUE_REAL ||
                     rhs_kind == CCT_CODEGEN_VALUE_REAL)
            ? CCT_CODEGEN_VALUE_REAL
            : CCT_CODEGEN_VALUE_INT;
    }
    return true;
}
```

**Observação:** O `pow()` já está disponível via `<math.h>` incluído em `codegen_runtime_bridge.c` linha 53.

---

### Operador 2: `//` (Divisão Inteira - estilo Python)

#### 2.1 Lexer - Adicionar Token
**Arquivo:** `src/lexer/lexer.h`

**Localização:** Após `TOKEN_CARET`

**Ação:**
```c
TOKEN_SLASH_SLASH,      /* // */
```

**Arquivo:** `src/lexer/lexer.c`

**Localização:** Linha 315 (case '/' existente)

**Ação:** Modificar para detectar `//`:
```c
case '/':
    if (match(lexer, '/')) return make_token(lexer, TOKEN_SLASH_SLASH);
    return make_token(lexer, TOKEN_SLASH);
```

**Localização:** Função `cct_token_type_string`

**Ação:**
```c
case TOKEN_SLASH_SLASH: return "SLASH_SLASH";
```

#### 2.2 Parser - Definir Precedência
**Arquivo:** `src/parser/parser.c`

**Localização:** `get_precedence()` linha 668-673

**Ação:** Adicionar mesma precedência que `*`, `/`, `%`:
```c
case TOKEN_STAR:
case TOKEN_SLASH:
case TOKEN_SLASH_SLASH:  /* Add here */
case TOKEN_PERCENT:
    return 11;
```

#### 2.3 Semantic - Type Checking
**Arquivo:** `src/semantic/semantic.c`

**Localização:** Após case `TOKEN_SLASH` (~linha 2153)

**Ação:**
```c
case TOKEN_SLASH_SLASH:  /* Integer division // */
    /* Both operands must be numeric */
    if (!sem_is_numeric_type(left) || !sem_is_numeric_type(right)) {
        sem_report_node(sem, expr, "operator // requires numeric operands");
        return &sem->type_error;
    }
    /* Always returns integer type (floor division) */
    return &sem->type_rex;
```

#### 2.4 Codegen - Emissão de Código C
**Arquivo:** `src/codegen/codegen.c`

**Localização:** Função `cg_emit_binary_expr()`

**Ação:** Adicionar case que gera divisão com cast:
```c
case TOKEN_SLASH_SLASH: {
    /* Emit floor division: (long long)(lhs / rhs) */
    fputs("((long long)(", out);
    cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
    fputs(" / ", out);
    cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
    fputs("))", out);

    if (!cct_cg_value_kind_is_numeric(lhs_kind) ||
        !cct_cg_value_kind_is_numeric(rhs_kind)) {
        cg_report_node(cg, expr, "operator // requires numeric operands");
        return false;
    }

    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
    return true;
}
```

---

### Operador 3: `%%` (Módulo Positivo - estilo R)

**Observação:** Em R, `%%` garante que o resultado tem o sinal do divisor (módulo euclidiano).

#### 3.1 Lexer - Adicionar Token
**Arquivo:** `src/lexer/lexer.h`

**Ação:**
```c
TOKEN_PERCENT_PERCENT,  /* %% */
```

**Arquivo:** `src/lexer/lexer.c`

**Localização:** Linha 316 (case '%')

**Ação:**
```c
case '%':
    if (match(lexer, '%')) return make_token(lexer, TOKEN_PERCENT_PERCENT);
    return make_token(lexer, TOKEN_PERCENT);
```

**String representation:**
```c
case TOKEN_PERCENT_PERCENT: return "PERCENT_PERCENT";
```

#### 3.2 Parser - Precedência
**Arquivo:** `src/parser/parser.c`

**Ação:** Mesma precedência que `%`:
```c
case TOKEN_STAR:
case TOKEN_SLASH:
case TOKEN_SLASH_SLASH:
case TOKEN_PERCENT:
case TOKEN_PERCENT_PERCENT:  /* Add here */
    return 11;
```

#### 3.3 Semantic - Type Checking
**Arquivo:** `src/semantic/semantic.c`

**Ação:**
```c
case TOKEN_PERCENT_PERCENT:  /* Positive modulo %% */
    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
        sem_report_node(sem, expr, "operator %% requires integer operands");
        return &sem->type_error;
    }
    return &sem->type_rex;
```

#### 3.4 Codegen - Módulo Euclidiano
**Arquivo:** `src/codegen/codegen.c`

**Ação:** Gerar fórmula: `((a % b) + b) % b`
```c
case TOKEN_PERCENT_PERCENT: {
    /* Euclidean modulo: ((a % b) + b) % b */
    fputs("((", out);
    cct_codegen_value_kind_t lhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.left, &lhs_kind)) return false;
    fputs(" % ", out);
    cct_codegen_value_kind_t rhs_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, &rhs_kind)) return false;
    fputs(") + ", out);
    /* Emit divisor again */
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, NULL)) return false;
    fputs(") % ", out);
    /* Emit divisor third time */
    if (!cg_emit_expr(out, cg, expr->as.binary_op.right, NULL)) return false;

    if (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT) {
        cg_report_node(cg, expr, "operator %% requires integer operands");
        return false;
    }

    if (out_kind) *out_kind = CCT_CODEGEN_VALUE_INT;
    return true;
}
```

---

### Operador 4-8: Bitwise Operators (Adicionar Codegen)

**Nota:** Estes operadores JÁ EXISTEM como keywords (`ET_BIT`, `VEL_BIT`, `XOR`, `SINISTER`, `DEXTER`), já são tokenizados, já têm type-checking semântico (linha 2232-2236 em `semantic.c`). **Falta apenas codegen.**

#### 4.1 Codegen - Adicionar Suporte
**Arquivo:** `src/codegen/codegen.c`

**Localização:** Função `cg_emit_binary_expr()` após linha 1659

**Ação:** Adicionar cases para bitwise operators:
```c
case TOKEN_ET_BIT:      op_text = "&"; break;
case TOKEN_VEL_BIT:     op_text = "|"; break;
case TOKEN_XOR:         op_text = "^"; break;
case TOKEN_SINISTER:    op_text = "<<"; break;
case TOKEN_DEXTER:      op_text = ">>"; break;
```

**Localização:** Validação de operandos (linha ~1690)

**Ação:** Adicionar validação para bitwise:
```c
/* After modulo check, add: */
if ((op == TOKEN_ET_BIT || op == TOKEN_VEL_BIT || op == TOKEN_XOR ||
     op == TOKEN_SINISTER || op == TOKEN_DEXTER) &&
    (lhs_kind != CCT_CODEGEN_VALUE_INT || rhs_kind != CCT_CODEGEN_VALUE_INT)) {
    cg_report_node(cg, expr, "bitwise operators require integer operands");
    return false;
}
```

---

## FASE 2: OPERADORES FÁCEIS (⭐⭐☆☆☆)

### Operador 9: `**` (Potenciação - estilo Python)

**Implementação:** Idêntica a `^`, apenas token diferente.

#### 9.1 Lexer
**Arquivo:** `src/lexer/lexer.h`

**Ação:**
```c
TOKEN_STAR_STAR,        /* ** */
```

**Arquivo:** `src/lexer/lexer.c`

**Localização:** Linha 314 (case '*')

**Ação:**
```c
case '*':
    if (match(lexer, '*')) return make_token(lexer, TOKEN_STAR_STAR);
    return make_token(lexer, TOKEN_STAR);
```

**String:**
```c
case TOKEN_STAR_STAR: return "STAR_STAR";
```

#### 9.2 Parser - Precedência
**Arquivo:** `src/parser/parser.c`

**Ação:** Mesma precedência e associatividade que `^`:
```c
case TOKEN_CARET:
case TOKEN_STAR_STAR:  /* Add here */
    return 12;
```

**Ação:** Modificar linha 714 para incluir `TOKEN_STAR_STAR`:
```c
cct_ast_node_t *right = parse_precedence(parser,
    (op == TOKEN_CARET || op == TOKEN_STAR_STAR) ? prec : prec + 1);
```

#### 9.3 Semantic
**Arquivo:** `src/semantic/semantic.c`

**Ação:**
```c
case TOKEN_CARET:
case TOKEN_STAR_STAR:  /* Add here */
    if (!sem_is_numeric_type(left) || !sem_is_numeric_type(right)) {
        sem_report_node(sem, expr, "power operator requires numeric operands");
        return &sem->type_error;
    }
    return sem_arithmetic_result_type(sem, left, right);
```

#### 9.4 Codegen
**Arquivo:** `src/codegen/codegen.c`

**Ação:** Adicionar `TOKEN_STAR_STAR` ao case existente de `TOKEN_CARET`:
```c
case TOKEN_CARET:
case TOKEN_STAR_STAR:  /* Same implementation as ^ */
    fputs("pow(", out);
    /* ... rest of implementation same as TOKEN_CARET ... */
```

---

### Operador 10: `..` (Range - Inclusivo)

**Semântica:** `a..b` cria um range de `a` até `b` (inclusivo). Por enquanto, apenas para uso em iteradores.

#### 10.1 Lexer
**Arquivo:** `src/lexer/lexer.h`

**Ação:**
```c
TOKEN_DOT_DOT,          /* .. */
```

**Arquivo:** `src/lexer/lexer.c`

**Localização:** Linha 310 (case '.')

**Ação:**
```c
case '.':
    if (match(lexer, '.')) return make_token(lexer, TOKEN_DOT_DOT);
    return make_token(lexer, TOKEN_DOT);
```

**String:**
```c
case TOKEN_DOT_DOT: return "DOT_DOT";
```

#### 10.2 Parser - Precedência
**Arquivo:** `src/parser/parser.c`

**Ação:** Baixa precedência (menor que comparação):
```c
case TOKEN_DOT_DOT:
    return 6;  /* Between relational and equality */
```

#### 10.3 Semantic
**Arquivo:** `src/semantic/semantic.c`

**Ação:**
```c
case TOKEN_DOT_DOT:  /* Range operator a..b */
    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
        sem_report_node(sem, expr, "range operator .. requires integer bounds");
        return &sem->type_error;
    }
    /* For now, treat as special iterator type - to be expanded in future */
    /* Return a synthetic "range" type or error for now */
    sem_report_node(sem, expr, "range operator .. not yet fully implemented");
    return &sem->type_error;
```

**Nota:** Range completo requer implementação de tipo especial. Por ora, deixar como placeholder.

#### 10.4 Codegen
**Skip** - Não implementar codegen até que semântica completa esteja pronta.

---

### Operador 11: `?:` (Ternário)

**Sintaxe:** `condição ? valor_true : valor_false`

**Desafio:** Não é um operador binário, é **ternário**. Requer mudanças mais profundas.

#### 11.1 Lexer
**Arquivo:** `src/lexer/lexer.h`

**Nota:** `TOKEN_QUESTION` já existe (linha 133). Precisamos de `TOKEN_COLON` (já existe na linha 109).

**Nenhuma mudança necessária no lexer.**

#### 11.2 Parser - Nova Função
**Arquivo:** `src/parser/parser.c`

**Localização:** Criar nova função `parse_ternary()` entre `parse_precedence()` e `parse_unary()`

**Ação:** Adicionar parsing de ternário:
```c
static cct_ast_node_t* parse_ternary(cct_parser_t *parser) {
    /* Parse the condition (use binary expression with precedence) */
    cct_ast_node_t *condition = parse_precedence(parser, 2);  /* Above || */
    if (!condition) return NULL;

    /* Check for ? token */
    if (!match(parser, TOKEN_QUESTION)) {
        return condition;  /* Not a ternary, return condition as-is */
    }

    u32 line = parser->previous.line;
    u32 col = parser->previous.column;

    /* Parse true branch */
    cct_ast_node_t *true_branch = parse_ternary(parser);  /* Recursive for nesting */
    if (!true_branch) {
        cct_parser_error(parser, "expected expression after '?'");
        return NULL;
    }

    /* Expect : token */
    if (!match(parser, TOKEN_COLON)) {
        cct_parser_error(parser, "expected ':' in ternary operator");
        return NULL;
    }

    /* Parse false branch */
    cct_ast_node_t *false_branch = parse_ternary(parser);
    if (!false_branch) {
        cct_parser_error(parser, "expected expression after ':'");
        return NULL;
    }

    return cct_ast_create_ternary_op(condition, true_branch, false_branch, line, col);
}
```

**Localização:** Modificar `parse_expression()` linha 721:
```c
static cct_ast_node_t* parse_expression(cct_parser_t *parser) {
    return parse_ternary(parser);  /* Changed from parse_precedence(parser, 1) */
}
```

#### 11.3 AST - Novo Node Type
**Arquivo:** `src/parser/ast.h`

**Localização:** Adicionar ao enum `cct_ast_node_type_t` (após `AST_BINARY_OP`)

**Ação:**
```c
AST_TERNARY_OP,
```

**Localização:** Adicionar estrutura ao union (após `binary_op`)

**Ação:**
```c
struct {
    cct_ast_node_t *condition;
    cct_ast_node_t *true_branch;
    cct_ast_node_t *false_branch;
} ternary_op;
```

**Localização:** Adicionar função de criação no header:
```c
cct_ast_node_t* cct_ast_create_ternary_op(cct_ast_node_t *condition,
                                          cct_ast_node_t *true_branch,
                                          cct_ast_node_t *false_branch,
                                          u32 line, u32 col);
```

**Arquivo:** `src/parser/ast.c`

**Ação:** Implementar função de criação:
```c
cct_ast_node_t* cct_ast_create_ternary_op(cct_ast_node_t *condition,
                                          cct_ast_node_t *true_branch,
                                          cct_ast_node_t *false_branch,
                                          u32 line, u32 col) {
    cct_ast_node_t *node = alloc_node(AST_TERNARY_OP, line, col);
    if (!node) return NULL;
    node->as.ternary_op.condition = condition;
    node->as.ternary_op.true_branch = true_branch;
    node->as.ternary_op.false_branch = false_branch;
    return node;
}
```

#### 11.4 Semantic
**Arquivo:** `src/semantic/semantic.c`

**Localização:** Na função `sem_analyze_expr()`, adicionar case:
```c
case AST_TERNARY_OP: {
    cct_sem_type_t *cond = sem_analyze_expr(sem, expr->as.ternary_op.condition);
    cct_sem_type_t *true_type = sem_analyze_expr(sem, expr->as.ternary_op.true_branch);
    cct_sem_type_t *false_type = sem_analyze_expr(sem, expr->as.ternary_op.false_branch);

    /* Condition must be boolean */
    if (!sem_is_bool_type(cond) && !sem_is_error_type(cond)) {
        sem_report_node(sem, expr->as.ternary_op.condition,
                       "ternary operator condition must be boolean");
    }

    /* Both branches must have compatible types */
    if (!sem_type_equal(true_type, false_type)) {
        /* Try numeric promotion */
        if (sem_is_numeric_type(true_type) && sem_is_numeric_type(false_type)) {
            return sem_arithmetic_result_type(sem, true_type, false_type);
        }
        sem_report_node(sem, expr, "ternary branches have incompatible types");
        return &sem->type_error;
    }

    return true_type;
}
```

#### 11.5 Codegen
**Arquivo:** `src/codegen/codegen.c`

**Localização:** Adicionar case em `cg_emit_expr()`:
```c
case AST_TERNARY_OP: {
    fputs("(", out);
    cct_codegen_value_kind_t cond_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.ternary_op.condition, &cond_kind)) return false;
    fputs(" ? ", out);
    cct_codegen_value_kind_t true_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.ternary_op.true_branch, &true_kind)) return false;
    fputs(" : ", out);
    cct_codegen_value_kind_t false_kind = CCT_CODEGEN_VALUE_UNKNOWN;
    if (!cg_emit_expr(out, cg, expr->as.ternary_op.false_branch, &false_kind)) return false;
    fputs(")", out);

    /* Result type is the widest of the two branches */
    if (out_kind) {
        *out_kind = (true_kind == CCT_CODEGEN_VALUE_REAL ||
                     false_kind == CCT_CODEGEN_VALUE_REAL)
            ? CCT_CODEGEN_VALUE_REAL
            : (true_kind == CCT_CODEGEN_VALUE_BOOL && false_kind == CCT_CODEGEN_VALUE_BOOL)
                ? CCT_CODEGEN_VALUE_BOOL
                : CCT_CODEGEN_VALUE_INT;
    }
    return true;
}
```

---

## FASE 3: OPERADORES MÉDIOS (⭐⭐⭐☆☆)

### Operador 12: `?.` (Optional Chaining)

**Semântica:** `obj?.field` retorna o campo se `obj` não é null, senão retorna null/none.

**Complexidade:** Requer sistema de tipos opcional (Option/Maybe type).

**Status:** **RECOMENDAÇÃO - NÃO IMPLEMENTAR AINDA**

**Justificativa:** CCT já tem tipos `Option<T>` (FASE 12C) via `option_some`, `option_none`, `option_unwrap_ptr`. Optional chaining requer integração profunda com esse sistema.

**Implementação futura:** Seria um operador postfix especial que:
1. Checa se o lado esquerdo é `Some` ou `None`
2. Propaga `None` se o lado esquerdo for `None`
3. Acessa o campo se for `Some`

**Estimativa:** 2-3 dias de trabalho, requer redesign de type system.

---

### Operador 13: `!!` (Unwrap/Assert)

**Semântica:** `opt!!` desempacota um `Option<T>` ou falha se for `None`.

**Status:** **RECOMENDAÇÃO - NÃO IMPLEMENTAR AINDA**

**Justificativa:** Requer mesmo sistema de tipos optional que `?.`.

**Implementação futura:** Operador postfix que:
1. Checa tipo opcional
2. Gera `option_unwrap_ptr()` ou `option_expect_ptr()` em codegen
3. Falha em runtime se for `None`

**Estimativa:** 1-2 dias, depois de `?.` estar pronto.

---

## RESUMO DE IMPLEMENTAÇÃO

### Operadores a Implementar (Recomendados)

| Operador | Complexidade | Arquivos | Linhas Est. | Tempo Est. |
|----------|--------------|----------|-------------|------------|
| `^` | ⭐☆☆☆☆ | 4 | ~80 | 2h |
| `//` | ⭐☆☆☆☆ | 4 | ~60 | 1.5h |
| `%%` | ⭐☆☆☆☆ | 4 | ~70 | 2h |
| Bitwise codegen | ⭐☆☆☆☆ | 1 | ~20 | 30min |
| `**` | ⭐☆☆☆☆ | 4 | ~30 | 1h |
| `?:` | ⭐⭐☆☆☆ | 5 | ~150 | 4h |
| **TOTAL** | - | - | **~410** | **11h** |

### Operadores a NÃO Implementar (Por Ora)

| Operador | Motivo |
|----------|--------|
| `..` | Requer tipo Range completo |
| `?.` | Requer integração profunda com Option<T> |
| `!!` | Requer integração profunda com Option<T> |

---

## ARQUIVOS MODIFICADOS

### Arquivos Principais
1. `src/lexer/lexer.h` - Adicionar 7 tokens
2. `src/lexer/lexer.c` - Adicionar tokenização para 7 operadores
3. `src/parser/parser.c` - Adicionar precedências + função `parse_ternary()`
4. `src/parser/ast.h` - Adicionar `AST_TERNARY_OP` e estrutura
5. `src/parser/ast.c` - Adicionar função `cct_ast_create_ternary_op()`
6. `src/semantic/semantic.c` - Adicionar type-checking para 7 operadores
7. `src/codegen/codegen.c` - Adicionar code generation para 7 operadores

### Total de Mudanças
- **7 arquivos modificados**
- **~410 linhas adicionadas**
- **0 arquivos novos**
- **0 breaking changes** (100% retrocompatível)

---

## ORDEM DE IMPLEMENTAÇÃO RECOMENDADA

### Fase 1 - Operadores Simples (6h)
1. `^` - Potenciação (2h)
2. `**` - Potenciação Python-style (1h)
3. `//` - Divisão inteira (1.5h)
4. `%%` - Módulo positivo (2h)
5. Bitwise codegen (30min)

### Fase 2 - Operador Ternário (4h)
6. `?:` - Ternário (4h)

### Fase 3 - Testes e Validação (2h)
7. Criar testes para cada operador
8. Validar precedência e associatividade
9. Testar casos edge (divisão por zero, overflow, etc)

**Tempo Total:** ~12 horas

---

## VALIDAÇÃO E TESTES

### Testes Mínimos por Operador

#### Teste `^` (Potenciação)
```cct
INCIPIT grimoire "test_power"
ADVOCARE "cct/io.cct"

RITUALE main() REDDE REX
  EVOCA REX r1 AD 2 ^ 10
  OBSECRO io_print_int(r1)  COMMENTARIUS 1024

  EVOCA UMBRA r2 AD 2.0 ^ 0.5
  COMMENTARIUS r2 = 1.414...

  EVOCA REX r3 AD 2 ^ 3 ^ 2
  OBSECRO io_print_int(r3)  COMMENTARIUS 512 (right-assoc)

  REDDE 0
EXPLICIT RITUALE
EXPLICIT grimoire
```

#### Teste `//` (Divisão Inteira)
```cct
EVOCA REX r1 AD 7 // 2      COMMENTARIUS 3
EVOCA REX r2 AD 0 - 7 // 2  COMMENTARIUS -4 (floor)
```

#### Teste `%%` (Módulo Positivo)
```cct
EVOCA REX r1 AD 7 %% 3      COMMENTARIUS 1
EVOCA REX r2 AD 0 - 7 %% 3  COMMENTARIUS 2 (sempre positivo)
```

#### Teste `?:` (Ternário)
```cct
EVOCA REX x AD 10
EVOCA REX max AD x > 5 ? x : 5  COMMENTARIUS 10
EVOCA REX y AD x > 20 ? 100 : 50  COMMENTARIUS 50
```

### Teste de Precedência Completo
```cct
EVOCA REX result AD 2 + 3 ^ 2 * 4
COMMENTARIUS 2 + (3^2) * 4 = 2 + 9*4 = 2 + 36 = 38
```

### Comando de Teste
```bash
./cct examples/test_new_operators.cct
./examples/test_new_operators
```

**Resultado esperado:** Todos os valores calculados corretamente sem erros.

---

## PROBLEMAS CONHECIDOS E MITIGAÇÕES

### Problema 1: `^` vs XOR Bitwise
- **CCT atual:** `^` é keyword para XOR (`TOKEN_XOR`)
- **Solução:** Operador `^` tem precedência em scanner antes de keywords
- **Validação:** XOR permanece via keyword `XOR`, não afeta código existente

### Problema 2: Associatividade à Direita
- **Modificação:** Linha 714 de `parser.c` muda comportamento
- **Risco:** Pode afetar outros operadores se não condicionado corretamente
- **Mitigação:** Usar `if (op == TOKEN_CARET || op == TOKEN_STAR_STAR)`

### Problema 3: Overflow em Potência
- **`pow()` em C pode gerar Infinity ou NaN**
- **Mitigação:** Documentar comportamento, não adicionar validação runtime
- **Futuro:** Considerar wrapper que valida overflow

### Problema 4: Divisão por Zero
- **`//` e `%%` podem dividir por zero**
- **Comportamento C:** Undefined behavior
- **Mitigação:** Documentar, não adicionar check runtime (overhead)
- **Futuro:** Considerar flag `--safe-math` para runtime checks

---

## DOCUMENTAÇÃO NECESSÁRIA

### Atualizar Arquivos de Documentação

1. **`docs/spec.md`** - Adicionar operadores à especificação formal
2. **`docs/bibliotheca_canonica.md`** - Se aplicável
3. **`README.md`** - Adicionar exemplo de uso dos novos operadores
4. **`examples/`** - Criar `examples/new_operators_showcase.cct`

### Exemplo de Documentação (spec.md)

```markdown
## Operadores Aritméticos

| Operador | Nome | Associatividade | Precedência | Tipos |
|----------|------|-----------------|-------------|-------|
| `^`, `**` | Potenciação | Direita | 12 | numeric → numeric |
| `*`, `/`, `//`, `%`, `%%` | Multiplicação | Esquerda | 11 | numeric → numeric |
| `+`, `-` | Adição | Esquerda | 10 | numeric → numeric |

### Potenciação (`^`, `**`)
- Associatividade à direita: `2^3^2` = `2^(3^2)` = 512
- Retorna UMBRA se qualquer operando for UMBRA
- Retorna REX se ambos forem REX

### Divisão Inteira (`//`)
- Sempre retorna REX (floor division)
- Exemplo: `7 // 2` = 3, `-7 // 2` = -4

### Módulo Positivo (`%%`)
- Módulo euclidiano (sinal do divisor)
- Exemplo: `-7 %% 3` = 2
```

---

## NOTAS FINAIS

### Decisões de Design

1. **`^` vs `**`**: Implementar ambos para compatibilidade com R e Python
2. **Bitwise**: Manter keywords existentes (`ET_BIT`, etc) + adicionar codegen
3. **Ternário**: Implementar como CCT pode usar muito em validações
4. **Optional chaining**: Postergar para FASE 14 (requer type system upgrade)

### Extensibilidade Futura

Esta implementação prepara o terreno para:
- Operador `..=` (range inclusivo)
- Operador `...` (range exclusivo)
- Operador `?.` (optional chaining) na FASE 14
- Sobrecarga de operadores (FASE futura)

### Compatibilidade

- **100% retrocompatível** - código CCT existente continua funcionando
- **Zero breaking changes**
- **Novos operadores são opt-in** - usar apenas se necessário
