/*
 * CCT — Clavicula Turing
 * AST Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_AST_H
#define CCT_AST_H

#include "../common/types.h"
#include "../lexer/lexer.h"

/* Forward declarations */
typedef struct cct_ast_node cct_ast_node_t;
typedef struct cct_ast_program cct_ast_program_t;
typedef struct cct_ast_type cct_ast_type_t;
typedef struct cct_ast_type_param cct_ast_type_param_t;
typedef struct cct_ast_type_list cct_ast_type_list_t;
typedef struct cct_ast_case_node cct_ast_case_node_t;
typedef struct cct_ast_molde_part cct_ast_molde_part_t;
typedef struct cct_ast_ordo_field cct_ast_ordo_field_t;
typedef struct cct_ast_ordo_variant cct_ast_ordo_variant_t;
typedef struct cct_ast_ordo_variant_list cct_ast_ordo_variant_list_t;

/*
 * AST Node Types
 *
 * Categorized by purpose for clarity
 */
typedef enum {
    /* Program structure */
    AST_PROGRAM,
    AST_IMPORT,              /* ADVOCARE */

    /* Top-level declarations */
    AST_RITUALE,             /* Function definition */
    AST_CODEX,               /* Namespace/module */
    AST_SIGILLUM,            /* Struct/class */
    AST_ORDO,                /* Enum */
    AST_PACTUM,              /* Interface */

    /* Statements */
    AST_BLOCK,               /* Block of statements */
    AST_EVOCA,               /* Variable declaration */
    AST_VINCIRE,             /* Assignment */
    AST_REDDE,               /* Return */
    AST_SI,                  /* If statement */
    AST_QUANDO,              /* Switch-like statement */
    AST_DUM,                 /* While loop */
    AST_DONEC,               /* Do-while loop */
    AST_REPETE,              /* For loop */
    AST_ITERUM,              /* Collection iterator loop */
    AST_TEMPTA,              /* Try/catch block (FASE 8A subset) */
    AST_IACE,                /* Throw statement */
    AST_FRANGE,              /* Break */
    AST_RECEDE,              /* Continue */
    AST_TRANSITUS,           /* Goto */
    AST_ANUR,                /* Exit */
    AST_DIMITTE,             /* Free/release pointer */
    AST_EXPR_STMT,           /* Expression as statement */

    /* Expressions */
    AST_LITERAL_INT,
    AST_LITERAL_REAL,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOL,
    AST_LITERAL_NIHIL,
    AST_MOLDE,               /* interpolated string literal */
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,                /* Function call */
    AST_CONIURA,             /* Explicit ritual call */
    AST_OBSECRO,             /* Builtin/syscall */
    AST_FIELD_ACCESS,        /* obj.field */
    AST_INDEX_ACCESS,        /* arr[i] */
    AST_MENSURA,             /* sizeof */

    /* Auxiliaries */
    AST_TYPE,                /* Type specification */
    AST_PARAM,               /* Function parameter */
    AST_FIELD,               /* Struct field */
    AST_ENUM_ITEM,           /* Enum item */

} cct_ast_node_type_t;

/*
 * Type specification node
 */
struct cct_ast_type {
    char *name;              /* Type name */
    bool is_pointer;         /* SPECULUM modifier */
    bool is_array;           /* SERIES modifier */
    u32 array_size;          /* Array size (0 if dynamic) */
    cct_ast_type_t *element_type;  /* For pointer/array */
    cct_ast_type_list_t *generic_args; /* Optional GENUS(T1, ...) type args */
};

/*
 * Parameter node
 */
typedef struct {
    char *name;
    cct_ast_type_t *type;
    bool is_constans;
    u32 line;
    u32 column;
} cct_ast_param_t;

/*
 * Field node (for SIGILLUM)
 */
typedef struct {
    char *name;
    cct_ast_type_t *type;
    u32 line;
    u32 column;
} cct_ast_field_t;

/*
 * Enum item node
 */
typedef struct {
    char *name;
    i64 value;
    bool has_value;
    u32 line;
    u32 column;
} cct_ast_enum_item_t;

/*
 * ORDO payload field node
 */
struct cct_ast_ordo_field {
    char *name;
    cct_ast_type_t *type;
    u32 line;
    u32 column;
};

/*
 * ORDO variant node (optionally with payload fields)
 */
struct cct_ast_ordo_variant {
    char *name;
    cct_ast_ordo_field_t **fields;
    size_t field_count;
    size_t field_capacity;
    bool has_value;
    i64 value;
    i64 tag_value;
    u32 line;
    u32 column;
};

/*
 * Type parameter node (for GENUS)
 */
struct cct_ast_type_param {
    char *name;
    char *constraint_pactum_name; /* Optional FASE 10D constraint: T PACTUM C */
    u32 line;
    u32 column;
};

/*
 * Node list (for storing multiple nodes)
 */
typedef struct {
    cct_ast_node_t **nodes;
    size_t count;
    size_t capacity;
} cct_ast_node_list_t;

/*
 * Parameter list
 */
typedef struct {
    cct_ast_param_t **params;
    size_t count;
    size_t capacity;
} cct_ast_param_list_t;

/*
 * Field list
 */
typedef struct {
    cct_ast_field_t **fields;
    size_t count;
    size_t capacity;
} cct_ast_field_list_t;

/*
 * Enum item list
 */
typedef struct {
    cct_ast_enum_item_t **items;
    size_t count;
    size_t capacity;
} cct_ast_enum_item_list_t;

/*
 * ORDO variant list
 */
struct cct_ast_ordo_variant_list {
    cct_ast_ordo_variant_t **variants;
    size_t count;
    size_t capacity;
};

/*
 * Type parameter list
 */
typedef struct {
    cct_ast_type_param_t **params;
    size_t count;
    size_t capacity;
} cct_ast_type_param_list_t;

/*
 * Type list (for generic applications)
 */
struct cct_ast_type_list {
    cct_ast_type_t **types;
    size_t count;
    size_t capacity;
};

/*
 * QUANDO case node
 */
struct cct_ast_case_node {
    cct_ast_node_t **literals;
    size_t literal_count;
    char **binding_names;            /* optional CASO payload bindings (ORDO) */
    size_t binding_count;
    cct_ast_ordo_variant_t *resolved_ordo_variant; /* semantic-resolved variant (non-owning) */
    cct_ast_node_t *body;    /* AST_BLOCK */
};

typedef enum {
    CCT_AST_MOLDE_PART_LITERAL = 0,
    CCT_AST_MOLDE_PART_EXPR,
} cct_ast_molde_part_kind_t;

struct cct_ast_molde_part {
    cct_ast_molde_part_kind_t kind;
    char *literal_text;      /* for CCT_AST_MOLDE_PART_LITERAL */
    cct_ast_node_t *expr;    /* for CCT_AST_MOLDE_PART_EXPR */
    char *fmt_spec;          /* optional for CCT_AST_MOLDE_PART_EXPR ({expr:spec}) */
};

/*
 * Main AST Node
 *
 * Using tagged union pattern for type-safe node handling
 */
struct cct_ast_node {
    cct_ast_node_type_t type;
    u32 line;
    u32 column;
    bool is_internal; /* FASE 9C: ARCANUM top-level visibility marker */

    union {
        /* Program */
        struct {
            char *name;
            cct_ast_node_list_t *declarations;
        } program;

        /* Import */
        struct {
            char *filename;
        } import;

        /* Rituale (function) */
        struct {
            char *name;
            cct_ast_type_param_list_t *type_params; /* GENUS(...) */
            cct_ast_param_list_t *params;
            cct_ast_type_t *return_type;
            cct_ast_node_t *body;  /* Block */
        } rituale;

        /* Codex (namespace) */
        struct {
            char *name;
            cct_ast_node_list_t *declarations;
        } codex;

        /* Sigillum (struct) */
        struct {
            char *name;
            cct_ast_type_param_list_t *type_params; /* GENUS(...) */
            char *pactum_name; /* Optional FASE 10C explicit conformance clause */
            cct_ast_field_list_t *fields;
            cct_ast_node_list_t *methods;  /* Rituales */
        } sigillum;

        /* Ordo (enum) */
        struct {
            char *name;
            cct_ast_ordo_variant_list_t *variants;
            cct_ast_enum_item_list_t *items;
            bool has_payload;
        } ordo;

        /* Pactum (interface) */
        struct {
            char *name;
            cct_ast_node_list_t *signatures;  /* Rituales without body */
        } pactum;

        /* Block */
        struct {
            cct_ast_node_list_t *statements;
        } block;

        /* Evoca (declaration) */
        struct {
            cct_ast_type_t *var_type;
            char *name;
            cct_ast_node_t *initializer;  /* Optional */
            bool is_constans;
        } evoca;

        /* Vincire (assignment) */
        struct {
            cct_ast_node_t *target;
            cct_ast_node_t *value;
        } vincire;

        /* Redde (return) */
        struct {
            cct_ast_node_t *value;  /* Optional */
        } redde;

        /* Si (if) */
        struct {
            cct_ast_node_t *condition;
            cct_ast_node_t *then_branch;
            cct_ast_node_t *else_branch;  /* Optional */
        } si;

        /* Quando (switch) */
        struct {
            cct_ast_node_t *expression;
            cct_ast_case_node_t *cases;
            size_t case_count;
            cct_ast_node_t *else_body; /* Optional */
        } quando;

        /* Dum (while) */
        struct {
            cct_ast_node_t *condition;
            cct_ast_node_t *body;
        } dum;

        /* Donec (do-while) */
        struct {
            cct_ast_node_t *body;
            cct_ast_node_t *condition;
        } donec;

        /* Repete (for) */
        struct {
            char *iterator;
            cct_ast_node_t *start;
            cct_ast_node_t *end;
            cct_ast_node_t *step;  /* Optional */
            cct_ast_node_t *body;
        } repete;

        /* Iterum (collection iterator) */
        struct {
            char *item_name;
            char *value_name; /* Optional second binding (map key/value) */
            cct_ast_node_t *collection;
            cct_ast_node_t *body;
        } iterum;

        /* Tempta/Cape (try/catch; SEMPER reserved for later phases) */
        struct {
            cct_ast_node_t *try_block;
            cct_ast_type_t *cape_type;   /* FASE 8A requires FRACTUM */
            char *cape_name;
            cct_ast_node_t *cape_block;
            cct_ast_node_t *semper_block; /* NULL in FASE 8A */
        } tempta;

        /* Iace (throw) */
        struct {
            cct_ast_node_t *value;
        } iace;

        /* Transitus (goto) */
        struct {
            char *label;
        } transitus;

        /* Anur (exit) */
        struct {
            cct_ast_node_t *value;  /* Optional */
        } anur;

        /* Dimitte (release/free) */
        struct {
            cct_ast_node_t *target;  /* Usually identifier */
        } dimitte;

        /* Expression statement */
        struct {
            cct_ast_node_t *expression;
        } expr_stmt;

        /* Literals */
        struct {
            i64 int_value;
        } literal_int;

        struct {
            f64 real_value;
        } literal_real;

        struct {
            char *string_value;
        } literal_string;

        struct {
            bool bool_value;
        } literal_bool;

        /* MOLDE (interpolated string) */
        struct {
            cct_ast_molde_part_t *parts;
            size_t part_count;
        } molde;

        /* Identifier */
        struct {
            char *name;
            bool is_ordo_construct;   /* semantic: identifier used as ORDO constructor (no payload) */
            char *ordo_name;          /* semantic: target ORDO name */
            char *variant_name;       /* semantic: target variant */
            i64 ordo_tag_value;       /* semantic: resolved tag value */
        } identifier;

        /* Binary operation */
        struct {
            cct_token_type_t operator;
            cct_ast_node_t *left;
            cct_ast_node_t *right;
        } binary_op;

        /* Unary operation */
        struct {
            cct_token_type_t operator;
            cct_ast_node_t *operand;
        } unary_op;

        /* Function call */
        struct {
            cct_ast_node_t *callee;
            cct_ast_node_list_t *arguments;
            bool is_ordo_construct;   /* semantic: call is ORDO variant constructor */
            char *ordo_name;          /* semantic: target ORDO name */
            char *variant_name;       /* semantic: target variant */
            i64 ordo_tag_value;       /* semantic: resolved tag value */
        } call;

        /* Coniura (explicit ritual call) */
        struct {
            char *name;
            cct_ast_type_list_t *type_args;
            cct_ast_node_list_t *arguments;
        } coniura;

        /* Obsecro (builtin/syscall) */
        struct {
            char *name;
            cct_ast_node_list_t *arguments;
        } obsecro;

        /* Field access */
        struct {
            cct_ast_node_t *object;
            char *field;
        } field_access;

        /* Index access */
        struct {
            cct_ast_node_t *array;
            cct_ast_node_t *index;
        } index_access;

        /* MENSURA (sizeof) */
        struct {
            cct_ast_type_t *type;
        } mensura;

    } as;
};

/*
 * Program node (root of AST)
 */
struct cct_ast_program {
    char *name;
    cct_ast_node_list_t *declarations;
};

/* ========================================================================
 * AST Construction Functions
 * ======================================================================== */

/* Create program */
cct_ast_program_t* cct_ast_create_program(const char *name);

/* Create node list */
cct_ast_node_list_t* cct_ast_create_node_list(void);
void cct_ast_node_list_append(cct_ast_node_list_t *list, cct_ast_node_t *node);

/* Create parameter list */
cct_ast_param_list_t* cct_ast_create_param_list(void);
void cct_ast_param_list_append(cct_ast_param_list_t *list, cct_ast_param_t *param);

/* Create field list */
cct_ast_field_list_t* cct_ast_create_field_list(void);
void cct_ast_field_list_append(cct_ast_field_list_t *list, cct_ast_field_t *field);

/* Create enum item list */
cct_ast_enum_item_list_t* cct_ast_create_enum_item_list(void);
void cct_ast_enum_item_list_append(cct_ast_enum_item_list_t *list, cct_ast_enum_item_t *item);
cct_ast_ordo_variant_list_t* cct_ast_create_ordo_variant_list(void);
void cct_ast_ordo_variant_list_append(cct_ast_ordo_variant_list_t *list, cct_ast_ordo_variant_t *variant);
void cct_ast_ordo_variant_add_field(cct_ast_ordo_variant_t *variant, cct_ast_ordo_field_t *field);
/* Create type parameter list */
cct_ast_type_param_list_t* cct_ast_create_type_param_list(void);
void cct_ast_type_param_list_append(cct_ast_type_param_list_t *list, cct_ast_type_param_t *param);
/* Create type list */
cct_ast_type_list_t* cct_ast_create_type_list(void);
void cct_ast_type_list_append(cct_ast_type_list_t *list, cct_ast_type_t *type);

/* Create type */
cct_ast_type_t* cct_ast_create_type(const char *name);
cct_ast_type_t* cct_ast_create_pointer_type(cct_ast_type_t *base);
cct_ast_type_t* cct_ast_create_array_type(cct_ast_type_t *base, u32 size);

/* Create parameter */
cct_ast_param_t* cct_ast_create_param(const char *name, cct_ast_type_t *type, bool is_constans, u32 line, u32 col);

/* Create field */
cct_ast_field_t* cct_ast_create_field(const char *name, cct_ast_type_t *type, u32 line, u32 col);

/* Create enum item */
cct_ast_enum_item_t* cct_ast_create_enum_item(const char *name, i64 value, bool has_value, u32 line, u32 col);
cct_ast_ordo_field_t* cct_ast_create_ordo_field(const char *name, cct_ast_type_t *type, u32 line, u32 col);
cct_ast_ordo_variant_t* cct_ast_create_ordo_variant(const char *name, bool has_value, i64 value, u32 line, u32 col);
/* Create type parameter */
cct_ast_type_param_t* cct_ast_create_type_param(const char *name, const char *constraint_pactum_name, u32 line, u32 col);

/* Create nodes */
cct_ast_node_t* cct_ast_create_import(const char *filename, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_rituale(const char *name, cct_ast_type_param_list_t *type_params,
                                       cct_ast_param_list_t *params, cct_ast_type_t *return_type,
                                       cct_ast_node_t *body, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_codex(const char *name, cct_ast_node_list_t *decls, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_sigillum(const char *name, cct_ast_type_param_list_t *type_params,
                                        const char *pactum_name,
                                        cct_ast_field_list_t *fields, cct_ast_node_list_t *methods,
                                        u32 line, u32 col);
cct_ast_node_t* cct_ast_create_ordo(const char *name, cct_ast_enum_item_list_t *items, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_ordo_with_variants(const char *name, cct_ast_ordo_variant_list_t *variants,
                                                  cct_ast_enum_item_list_t *items, bool has_payload,
                                                  u32 line, u32 col);
cct_ast_node_t* cct_ast_create_pactum(const char *name, cct_ast_node_list_t *sigs, u32 line, u32 col);

cct_ast_node_t* cct_ast_create_block(cct_ast_node_list_t *stmts, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_evoca(cct_ast_type_t *type, const char *name, cct_ast_node_t *init, bool is_constans, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_vincire(cct_ast_node_t *target, cct_ast_node_t *value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_redde(cct_ast_node_t *value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_si(cct_ast_node_t *cond, cct_ast_node_t *then_br, cct_ast_node_t *else_br, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_quando(cct_ast_node_t *expr, cct_ast_case_node_t *cases, size_t case_count,
                                      cct_ast_node_t *else_body, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_dum(cct_ast_node_t *cond, cct_ast_node_t *body, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_donec(cct_ast_node_t *body, cct_ast_node_t *cond, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_repete(const char *iter, cct_ast_node_t *start, cct_ast_node_t *end,
                                      cct_ast_node_t *step, cct_ast_node_t *body, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_iterum(const char *item_name, const char *value_name,
                                      cct_ast_node_t *collection,
                                      cct_ast_node_t *body, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_tempta(cct_ast_node_t *try_block, cct_ast_type_t *cape_type,
                                      const char *cape_name, cct_ast_node_t *cape_block,
                                      cct_ast_node_t *semper_block, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_iace(cct_ast_node_t *value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_frange(u32 line, u32 col);
cct_ast_node_t* cct_ast_create_recede(u32 line, u32 col);
cct_ast_node_t* cct_ast_create_transitus(const char *label, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_anur(cct_ast_node_t *value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_dimitte(cct_ast_node_t *target, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_expr_stmt(cct_ast_node_t *expr, u32 line, u32 col);

cct_ast_node_t* cct_ast_create_literal_int(i64 value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_literal_real(f64 value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_literal_string(const char *value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_literal_bool(bool value, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_literal_nihil(u32 line, u32 col);
cct_ast_node_t* cct_ast_create_molde(cct_ast_molde_part_t *parts, size_t part_count, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_identifier(const char *name, u32 line, u32 col);

cct_ast_node_t* cct_ast_create_binary_op(cct_token_type_t op, cct_ast_node_t *left, cct_ast_node_t *right, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_unary_op(cct_token_type_t op, cct_ast_node_t *operand, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_call(cct_ast_node_t *callee, cct_ast_node_list_t *args, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_coniura(const char *name, cct_ast_type_list_t *type_args,
                                        cct_ast_node_list_t *args, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_obsecro(const char *name, cct_ast_node_list_t *args, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_field_access(cct_ast_node_t *obj, const char *field, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_index_access(cct_ast_node_t *arr, cct_ast_node_t *index, u32 line, u32 col);
cct_ast_node_t* cct_ast_create_mensura(cct_ast_type_t *type, u32 line, u32 col);

/* ========================================================================
 * AST Destruction Functions
 * ======================================================================== */

void cct_ast_free_program(cct_ast_program_t *program);
void cct_ast_free_node(cct_ast_node_t *node);
void cct_ast_free_type(cct_ast_type_t *type);
void cct_ast_free_node_list(cct_ast_node_list_t *list);
void cct_ast_free_param_list(cct_ast_param_list_t *list);
void cct_ast_free_field_list(cct_ast_field_list_t *list);
void cct_ast_free_enum_item_list(cct_ast_enum_item_list_t *list);
void cct_ast_free_ordo_variant_list(cct_ast_ordo_variant_list_t *list);
void cct_ast_free_type_param_list(cct_ast_type_param_list_t *list);
void cct_ast_free_type_list(cct_ast_type_list_t *list);

/* ========================================================================
 * AST Printing Functions
 * ======================================================================== */

void cct_ast_print_program(const cct_ast_program_t *program);
void cct_ast_print_node(const cct_ast_node_t *node, int indent);

/* Get node type as string */
const char* cct_ast_node_type_string(cct_ast_node_type_t type);

#endif /* CCT_AST_H */
