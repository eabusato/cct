# Implementation Plan: New Operators for CCT

## Context

This plan adds extra operators to the CCT language, grouped into three complexity tiers: TRIVIAL, EASY, and MEDIUM. The implementation follows the existing CCT compiler architecture, which has four main phases:

1. **Lexer** (`src/lexer/`) - tokenization
2. **Parser** (`src/parser/`) - syntactic analysis and AST construction
3. **Semantic** (`src/semantic/`) - semantic analysis and type checking
4. **Codegen** (`src/codegen/`) - C code emission

---

## PHASE 1: TRIVIAL OPERATORS (⭐☆☆☆☆)

### Operator 1: `^` (Exponentiation - R style)

#### 1.1 Lexer - Add Token
**File:** `src/lexer/lexer.h`

**Location:** After line 122 (after `TOKEN_PERCENT`)

**Action:** Add a new token to the enum:
```c
TOKEN_CARET,            /* ^ */
```

**File:** `src/lexer/lexer.c`

**Location:** Line 317 (scanner switch)

**Action:** Add a case to tokenize `^`:
```c
case '^': return make_token(lexer, TOKEN_CARET);
```

**Location:** Around line 357+ (function `cct_token_type_string`)

**Action:** Add string representation:
```c
case TOKEN_CARET: return "CARET";
```

#### 1.2 Parser - Define Precedence
**File:** `src/parser/parser.c`

**Location:** Function `get_precedence()` around lines 666-699

**Action:** Add precedence HIGHER than multiplication (precedence 12):
```c
case TOKEN_CARET:
    return 12;  /* Power has higher precedence than *, /, % */
```

**Critical note:** Exponentiation is **RIGHT-ASSOCIATIVE** (`2^3^2 = 2^(3^2) = 512`).

**Location:** Function `parse_precedence()` line 714

**Action:** Modify to support right associativity:
```c
/* Before (line 714): */
cct_ast_node_t *right = parse_precedence(parser, prec + 1);

/* After: */
cct_ast_node_t *right = parse_precedence(parser,
    (op == TOKEN_CARET) ? prec : prec + 1);  /* Right-assoc for ^ */
```

#### 1.3 Semantic - Type Checking
**File:** `src/semantic/semantic.c`

**Location:** Binary-op analysis, after line 2170 (inside the operator switch)

**Action:** Add a case for `^`:
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

#### 1.4 Codegen - Emit C Code
**File:** `src/codegen/codegen.c`

**Location:** Function `cg_emit_binary_expr()` around line 1644 (inside the switch)

**Action:** Add a case that emits a `pow()` call:
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

**Note:** `pow()` is already available through `<math.h>` included in `codegen_runtime_bridge.c` line 53.

---

### Operator 2: `//` (Integer Division - Python style)

#### 2.1 Lexer - Add Token
**File:** `src/lexer/lexer.h`

**Location:** After `TOKEN_CARET`

**Action:**
```c
TOKEN_SLASH_SLASH,      /* // */
```

**File:** `src/lexer/lexer.c`

**Location:** Line 315 (existing `/` case)

**Action:** Modify to detect `//`:
```c
case '/':
    if (match(lexer, '/')) return make_token(lexer, TOKEN_SLASH_SLASH);
    return make_token(lexer, TOKEN_SLASH);
```

**Location:** Function `cct_token_type_string`

**Action:**
```c
case TOKEN_SLASH_SLASH: return "SLASH_SLASH";
```

#### 2.2 Parser - Define Precedence
**File:** `src/parser/parser.c`

**Location:** `get_precedence()` around lines 668-673

**Action:** Add the same precedence as `*`, `/`, `%`:
```c
case TOKEN_STAR:
case TOKEN_SLASH:
case TOKEN_SLASH_SLASH:  /* Add here */
case TOKEN_PERCENT:
    return 11;
```

#### 2.3 Semantic - Type Checking
**File:** `src/semantic/semantic.c`

**Location:** After case `TOKEN_SLASH` (around line 2153)

**Action:**
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

#### 2.4 Codegen - Emit C Code
**File:** `src/codegen/codegen.c`

**Location:** Function `cg_emit_binary_expr()`

**Action:** Add a case that emits division with cast:
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

### Operator 3: `%%` (Positive Modulo - R style)

**Note:** In R, `%%` guarantees that the result has the sign of the divisor (Euclidean modulo).

#### 3.1 Lexer - Add Token
**File:** `src/lexer/lexer.h`

**Action:**
```c
TOKEN_PERCENT_PERCENT,  /* %% */
```

**File:** `src/lexer/lexer.c`

**Location:** Line 316 (`%` case)

**Action:**
```c
case '%':
    if (match(lexer, '%')) return make_token(lexer, TOKEN_PERCENT_PERCENT);
    return make_token(lexer, TOKEN_PERCENT);
```

**String representation:**
```c
case TOKEN_PERCENT_PERCENT: return "PERCENT_PERCENT";
```

#### 3.2 Parser - Precedence
**File:** `src/parser/parser.c`

**Action:** Same precedence as `%`:
```c
case TOKEN_STAR:
case TOKEN_SLASH:
case TOKEN_SLASH_SLASH:
case TOKEN_PERCENT:
case TOKEN_PERCENT_PERCENT:  /* Add here */
    return 11;
```

#### 3.3 Semantic - Type Checking
**File:** `src/semantic/semantic.c`

**Action:**
```c
case TOKEN_PERCENT_PERCENT:  /* Positive modulo %% */
    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
        sem_report_node(sem, expr, "operator %% requires integer operands");
        return &sem->type_error;
    }
    return &sem->type_rex;
```

#### 3.4 Codegen - Euclidean Modulo
**File:** `src/codegen/codegen.c`

**Action:** Emit formula `((a % b) + b) % b`
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

### Operators 4-8: Bitwise Operators (Add Codegen)

**Note:** These operators already exist as keywords (`ET_BIT`, `VEL_BIT`, `XOR`, `SINISTER`, `DEXTER`), are already tokenized, and already have semantic type checking. Only code generation is missing.

#### 4.1 Codegen - Add Support
**File:** `src/codegen/codegen.c`

**Location:** Function `cg_emit_binary_expr()` after line 1659

**Action:** Add cases for bitwise operators:
```c
case TOKEN_ET_BIT:      op_text = "&"; break;
case TOKEN_VEL_BIT:     op_text = "|"; break;
case TOKEN_XOR:         op_text = "^"; break;
case TOKEN_SINISTER:    op_text = "<<"; break;
case TOKEN_DEXTER:      op_text = ">>"; break;
```

**Location:** Operand validation around line 1690

**Action:** Add bitwise validation:
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

## PHASE 2: EASY OPERATORS (⭐⭐☆☆☆)

### Operator 9: `**` (Exponentiation - Python style)

**Implementation:** Identical to `^`, only the token changes.

#### 9.1 Lexer
**File:** `src/lexer/lexer.h`

**Action:**
```c
TOKEN_STAR_STAR,        /* ** */
```

**File:** `src/lexer/lexer.c`

**Location:** Line 314 (`*` case)

**Action:**
```c
case '*':
    if (match(lexer, '*')) return make_token(lexer, TOKEN_STAR_STAR);
    return make_token(lexer, TOKEN_STAR);
```

**String:**
```c
case TOKEN_STAR_STAR: return "STAR_STAR";
```

#### 9.2 Parser - Precedence
**File:** `src/parser/parser.c`

**Action:** Same precedence and associativity as `^`:
```c
case TOKEN_CARET:
case TOKEN_STAR_STAR:  /* Add here */
    return 12;
```

**Action:** Modify line 714 to include `TOKEN_STAR_STAR`:
```c
cct_ast_node_t *right = parse_precedence(parser,
    (op == TOKEN_CARET || op == TOKEN_STAR_STAR) ? prec : prec + 1);
```

#### 9.3 Semantic
**File:** `src/semantic/semantic.c`

**Action:**
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
**File:** `src/codegen/codegen.c`

**Action:** Add `TOKEN_STAR_STAR` to the existing `TOKEN_CARET` case:
```c
case TOKEN_CARET:
case TOKEN_STAR_STAR:  /* Same implementation as ^ */
    fputs("pow(", out);
    /* ... rest of implementation same as TOKEN_CARET ... */
```

---

### Operator 10: `..` (Range - Inclusive)

**Semantics:** `a..b` creates a range from `a` to `b` (inclusive). For now, only for iterator use.

#### 10.1 Lexer
**File:** `src/lexer/lexer.h`

**Action:**
```c
TOKEN_DOT_DOT,          /* .. */
```

**File:** `src/lexer/lexer.c`

**Location:** Line 310 (`.` case)

**Action:**
```c
case '.':
    if (match(lexer, '.')) return make_token(lexer, TOKEN_DOT_DOT);
    return make_token(lexer, TOKEN_DOT);
```

**String:**
```c
case TOKEN_DOT_DOT: return "DOT_DOT";
```

#### 10.2 Parser - Precedence
**File:** `src/parser/parser.c`

**Action:** Low precedence (below comparison):
```c
case TOKEN_DOT_DOT:
    return 6;  /* Between relational and equality */
```

#### 10.3 Semantic
**File:** `src/semantic/semantic.c`

**Action:**
```c
case TOKEN_DOT_DOT:  /* Range operator a..b */
    if (!sem_is_integer_type(left) || !sem_is_integer_type(right)) {
        sem_report_node(sem, expr, "range operator .. requires integer bounds");
        return &sem->type_error;
    }
    /* For now, treat as a special iterator type - expand in the future */
    /* Return a synthetic "range" type or error for now */
    sem_report_node(sem, expr, "range operator .. not yet fully implemented");
    return &sem->type_error;
```

**Note:** Full range support requires a dedicated type. For now, leave it as a placeholder.

#### 10.4 Codegen
**Skip** - Do not implement code generation until the full semantic model is ready.

---

### Operator 11: `?:` (Ternary)

**Syntax:** `condition ? true_value : false_value`

**Challenge:** This is not a binary operator, it is **ternary**. It requires deeper parser and AST changes.

#### 11.1 Lexer
**File:** `src/lexer/lexer.h`

**Note:** `TOKEN_QUESTION` already exists. `TOKEN_COLON` also already exists.

**No lexer changes required.**

#### 11.2 Parser - New Function
**File:** `src/parser/parser.c`

**Location:** Create a new function `parse_ternary()` between `parse_precedence()` and `parse_unary()`

**Action:** Add ternary parsing:
```c
static cct_ast_node_t* parse_ternary(cct_parser_t *parser) {
    /* Parse the condition (use binary expression precedence) */
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

**Location:** Modify `parse_expression()` line 721:
```c
static cct_ast_node_t* parse_expression(cct_parser_t *parser) {
    return parse_ternary(parser);  /* Changed from parse_precedence(parser, 1) */
}
```

#### 11.3 AST - New Node Type
**File:** `src/parser/ast.h`

**Location:** Add to enum `cct_ast_node_type_t` (after `AST_BINARY_OP`)

**Action:**
```c
AST_TERNARY_OP,
```

**Location:** Add structure to the union (after `binary_op`)

**Action:**
```c
struct {
    cct_ast_node_t *condition;
    cct_ast_node_t *true_branch;
    cct_ast_node_t *false_branch;
} ternary_op;
```

**Location:** Add constructor declaration in the header:
```c
cct_ast_node_t* cct_ast_create_ternary_op(cct_ast_node_t *condition,
                                          cct_ast_node_t *true_branch,
                                          cct_ast_node_t *false_branch,
                                          u32 line, u32 col);
```

**File:** `src/parser/ast.c`

**Action:** Implement the constructor:
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
**File:** `src/semantic/semantic.c`

**Location:** In `sem_analyze_expr()`, add a case:
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
**File:** `src/codegen/codegen.c`

**Location:** Add a case in `cg_emit_expr()`:
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

## PHASE 3: MEDIUM OPERATORS (⭐⭐⭐☆☆)

### Operator 12: `?.` (Optional Chaining)

**Semantics:** `obj?.field` returns the field if `obj` is not null; otherwise it returns null/none.

**Complexity:** Requires an optional-type system (`Option`/`Maybe`).

**Status:** **RECOMMENDATION - DO NOT IMPLEMENT YET**

**Reasoning:** CCT already has `Option<T>` patterns (FASE 12C) through `option_some`, `option_none`, and `option_unwrap_ptr`. Optional chaining requires deep integration with that system.

**Future implementation:** This would be a special postfix operator that:
1. Checks whether the left side is `Some` or `None`
2. Propagates `None` if the left side is `None`
3. Accesses the field if the value is `Some`

**Estimate:** 2-3 days of work; requires a type-system redesign.

---

### Operator 13: `!!` (Unwrap/Assert)

**Semantics:** `opt!!` unwraps an `Option<T>` or fails if it is `None`.

**Status:** **RECOMMENDATION - DO NOT IMPLEMENT YET**

**Reasoning:** It requires the same optional-type system support as `?.`.

**Future implementation:** Postfix operator that:
1. Checks for an optional type
2. Emits `option_unwrap_ptr()` or `option_expect_ptr()` in codegen
3. Fails at runtime when the value is `None`

**Estimate:** 1-2 days, after `?.` is ready.

---

## IMPLEMENTATION SUMMARY

### Operators to Implement (Recommended)

| Operator | Complexity | Files | Est. Lines | Est. Time |
|----------|------------|-------|------------|-----------|
| `^` | ⭐☆☆☆☆ | 4 | ~80 | 2h |
| `//` | ⭐☆☆☆☆ | 4 | ~60 | 1.5h |
| `%%` | ⭐☆☆☆☆ | 4 | ~70 | 2h |
| Bitwise codegen | ⭐☆☆☆☆ | 1 | ~20 | 30min |
| `**` | ⭐☆☆☆☆ | 4 | ~30 | 1h |
| `?:` | ⭐⭐☆☆☆ | 5 | ~150 | 4h |
| **TOTAL** | - | - | **~410** | **11h** |

### Operators to NOT Implement Yet

| Operator | Reason |
|----------|--------|
| `..` | Requires a full Range type |
| `?.` | Requires deep integration with `Option<T>` |
| `!!` | Requires deep integration with `Option<T>` |

---

## MODIFIED FILES

### Main Files
1. `src/lexer/lexer.h` - add 7 tokens
2. `src/lexer/lexer.c` - add tokenization for 7 operators
3. `src/parser/parser.c` - add precedences + function `parse_ternary()`
4. `src/parser/ast.h` - add `AST_TERNARY_OP` and structure
5. `src/parser/ast.c` - add function `cct_ast_create_ternary_op()`
6. `src/semantic/semantic.c` - add type checking for 7 operators
7. `src/codegen/codegen.c` - add code generation for 7 operators

### Change Totals
- **7 modified files**
- **~410 lines added**
- **0 new files**
- **0 breaking changes** (100% backward compatible)

---

## RECOMMENDED IMPLEMENTATION ORDER

### Phase 1 - Simple Operators (6h)
1. `^` - exponentiation (2h)
2. `**` - Python-style exponentiation (1h)
3. `//` - integer division (1.5h)
4. `%%` - positive modulo (2h)
5. Bitwise codegen (30min)

### Phase 2 - Ternary Operator (4h)
6. `?:` - ternary (4h)

### Phase 3 - Tests and Validation (2h)
7. Create tests for each operator
8. Validate precedence and associativity
9. Test edge cases (division by zero, overflow, etc.)

**Total time:** ~12 hours

---

## VALIDATION AND TESTING

### Minimum Tests per Operator

#### `^` Test (Exponentiation)
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

#### `//` Test (Integer Division)
```cct
EVOCA REX r1 AD 7 // 2      COMMENTARIUS 3
EVOCA REX r2 AD 0 - 7 // 2  COMMENTARIUS -4 (floor)
```

#### `%%` Test (Positive Modulo)
```cct
EVOCA REX r1 AD 7 %% 3      COMMENTARIUS 1
EVOCA REX r2 AD 0 - 7 %% 3  COMMENTARIUS 2 (always positive)
```

#### `?:` Test (Ternary)
```cct
EVOCA REX x AD 10
EVOCA REX max AD x > 5 ? x : 5  COMMENTARIUS 10
EVOCA REX y AD x > 20 ? 100 : 50  COMMENTARIUS 50
```

### Full Precedence Test
```cct
EVOCA REX result AD 2 + 3 ^ 2 * 4
COMMENTARIUS 2 + (3^2) * 4 = 2 + 9*4 = 2 + 36 = 38
```

### Test Command
```bash
./cct examples/test_new_operators.cct
./examples/test_new_operators
```

**Expected result:** All values are computed correctly without errors.

---

## KNOWN ISSUES AND MITIGATIONS

### Issue 1: `^` vs Bitwise XOR
- **Current CCT:** `^` is currently a keyword-backed XOR surface (`TOKEN_XOR`)
- **Solution:** Operator `^` gets scanner precedence before keyword resolution
- **Validation:** XOR remains available via keyword `XOR`, so existing code is unaffected

### Issue 2: Right Associativity
- **Change:** Line 714 of `parser.c` changes behavior
- **Risk:** It may affect other operators if not gated correctly
- **Mitigation:** Use `if (op == TOKEN_CARET || op == TOKEN_STAR_STAR)`

### Issue 3: Overflow in Power
- **`pow()` in C can produce Infinity or NaN**
- **Mitigation:** Document behavior; do not add runtime validation
- **Future:** Consider a wrapper that validates overflow

### Issue 4: Division by Zero
- **`//` and `%%` can divide by zero**
- **C behavior:** Undefined behavior
- **Mitigation:** Document it; do not add runtime checks yet (overhead)
- **Future:** Consider a `--safe-math` flag for runtime checks

---

## REQUIRED DOCUMENTATION UPDATES

1. **`docs/spec.md`** - add the operators to the formal specification
2. **`docs/bibliotheca_canonica.md`** - if applicable
3. **`README.md`** - add examples showing the new operators
4. **`examples/`** - add `examples/new_operators_showcase.cct`

### Documentation Example (`spec.md`)

```markdown
## Arithmetic Operators

| Operator | Name | Associativity | Precedence | Types |
|----------|------|---------------|------------|-------|
| `^`, `**` | Exponentiation | Right | 12 | numeric -> numeric |
| `*`, `/`, `//`, `%`, `%%` | Multiplication | Left | 11 | numeric -> numeric |
| `+`, `-` | Addition | Left | 10 | numeric -> numeric |

### Exponentiation (`^`, `**`)
- Right-associative: `2^3^2` = `2^(3^2)` = 512
- Returns `UMBRA` if either operand is `UMBRA`
- Returns `REX` if both operands are `REX`

### Integer Division (`//`)
- Always returns `REX` (floor division)
- Example: `7 // 2` = 3, `-7 // 2` = -4

### Positive Modulo (`%%`)
- Euclidean modulo (sign of the divisor)
- Example: `-7 %% 3` = 2
```

---

## FINAL NOTES

### Design Decisions

1. **`^` vs `**`**: implement both for R and Python compatibility
2. **Bitwise**: keep existing keywords (`ET_BIT`, etc.) and add codegen
3. **Ternary**: implement because it is useful in compact validation logic
4. **Optional chaining**: postpone to FASE 14 (requires a type-system upgrade)

### Future Extensibility

This implementation prepares the ground for:
- operator `..=` (inclusive range)
- operator `...` (exclusive range)
- operator `?.` (optional chaining) in FASE 14
- operator overloading (future phase)

### Compatibility

- **100% backward compatible** - existing CCT code keeps working
- **Zero breaking changes**
- **New operators are opt-in** - use them only when needed
