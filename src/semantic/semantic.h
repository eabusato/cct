/*
 * CCT — Clavicula Turing
 * Semantic Analyzer Definitions
 *
 * FASE 12H: Structural maturity milestone
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_SEMANTIC_H
#define CCT_SEMANTIC_H

#include "../common/types.h"
#include "../common/errors.h"
#include "../parser/ast.h"

typedef struct cct_sem_type cct_sem_type_t;
typedef struct cct_sem_symbol cct_sem_symbol_t;
typedef struct cct_sem_scope cct_sem_scope_t;
typedef struct cct_sem_type_alloc cct_sem_type_alloc_t;
typedef struct cct_sem_scope_alloc cct_sem_scope_alloc_t;
typedef struct cct_sem_symbol_alloc cct_sem_symbol_alloc_t;
typedef struct cct_sem_generic_instance cct_sem_generic_instance_t;

typedef enum {
    CCT_SEM_SCOPE_GLOBAL = 0,
    CCT_SEM_SCOPE_RITUALE,
    CCT_SEM_SCOPE_BLOCK,
    CCT_SEM_SCOPE_LOOP,
} cct_sem_scope_kind_t;

typedef enum {
    CCT_SEM_SYMBOL_VARIABLE = 0,
    CCT_SEM_SYMBOL_PARAMETER,
    CCT_SEM_SYMBOL_RITUALE,
    CCT_SEM_SYMBOL_TYPE,
    CCT_SEM_SYMBOL_PACTUM,
} cct_sem_symbol_kind_t;

typedef enum {
    CCT_SEM_ITER_COLLECTION_NONE = 0,
    CCT_SEM_ITER_COLLECTION_FLUXUS,
    CCT_SEM_ITER_COLLECTION_MAP,
    CCT_SEM_ITER_COLLECTION_SET,
} cct_sem_iter_collection_kind_t;

typedef enum {
    CCT_SEM_TYPE_ERROR = 0,
    CCT_SEM_TYPE_NIHIL,
    CCT_SEM_TYPE_VERUM,
    CCT_SEM_TYPE_VERBUM,
    CCT_SEM_TYPE_FRACTUM,
    CCT_SEM_TYPE_REX,
    CCT_SEM_TYPE_DUX,
    CCT_SEM_TYPE_COMES,
    CCT_SEM_TYPE_MILES,
    CCT_SEM_TYPE_UMBRA,
    CCT_SEM_TYPE_FLAMMA,
    CCT_SEM_TYPE_POINTER,
    CCT_SEM_TYPE_ARRAY,
    CCT_SEM_TYPE_NAMED,
    CCT_SEM_TYPE_TYPE_PARAM,
} cct_sem_type_kind_t;

struct cct_sem_type {
    cct_sem_type_kind_t kind;
    const char *name;           /* Used by named/builtin display */
    cct_sem_type_t *element;    /* pointer/array element type */
    u32 array_size;             /* 0 means unsized/dynamic */
};

struct cct_sem_symbol {
    char *name;
    cct_sem_symbol_kind_t kind;
    cct_sem_type_t *type;       /* variable/param/type symbol payload */
    const cct_ast_node_t *type_decl; /* For CCT_SEM_SYMBOL_TYPE (SIGILLUM/ORDO AST node) */
    const cct_ast_node_t *pactum_decl; /* For CCT_SEM_SYMBOL_PACTUM */
    u32 line;
    u32 column;

    /* Function signature payload for CCT_SEM_SYMBOL_RITUALE */
    cct_sem_type_t *return_type;
    cct_sem_type_t **param_types;
    size_t param_count;
    bool is_constans;
    size_t type_param_count;
    char **type_param_names;          /* GENUS parameter names for rituale symbols */
    char **type_param_constraint_pactum_names; /* Optional FASE 10D constraints per GENUS parameter */
    const cct_ast_node_t *rituale_decl; /* Back-reference for generic rituale diagnostics */

    cct_sem_iter_collection_kind_t iter_collection_kind;
    cct_sem_type_t *iter_key_type;
    cct_sem_type_t *iter_value_type;

    cct_sem_symbol_t *next;
};

struct cct_sem_scope {
    cct_sem_scope_kind_t kind;
    cct_sem_scope_t *parent;
    cct_sem_symbol_t *symbols;
};

typedef struct {
    const char *name;
    size_t min_args;
    bool variadic;
    cct_sem_type_t *return_type;
} cct_sem_builtin_spec_t;

typedef struct {
    const char *filename;
    bool had_error;
    u32 error_count;

    cct_sem_scope_t *global_scope;
    cct_sem_scope_t *current_scope;
    cct_sem_symbol_t *current_rituale;
    u32 loop_depth;
    cct_profile_t profile;          /* FASE 16A.2: compilation profile */

    /* Builtin semantic types (interned for pointer equality) */
    cct_sem_type_t type_error;
    cct_sem_type_t type_nihil;
    cct_sem_type_t type_verum;
    cct_sem_type_t type_verbum;
    cct_sem_type_t type_fractum;
    cct_sem_type_t type_rex;
    cct_sem_type_t type_dux;
    cct_sem_type_t type_comes;
    cct_sem_type_t type_miles;
    cct_sem_type_t type_umbra;
    cct_sem_type_t type_flamma;

    /* Allocations owned by analyzer */
    cct_sem_scope_alloc_t *scope_alloc_list;
    cct_sem_symbol_alloc_t *symbol_alloc_list;
    cct_sem_type_alloc_t *type_alloc_list;

    /* Active generic type-parameter scope (FASE 10A) */
    char **active_type_param_names;
    size_t active_type_param_count;
    size_t active_type_param_capacity;

    /* Registered generic type instantiations (FASE 10B) */
    cct_sem_generic_instance_t *generic_instances;
} cct_semantic_analyzer_t;

void cct_semantic_init(cct_semantic_analyzer_t *sem, const char *filename, cct_profile_t profile);
bool cct_semantic_analyze_program(cct_semantic_analyzer_t *sem, const cct_ast_program_t *program);
bool cct_semantic_had_error(const cct_semantic_analyzer_t *sem);
u32 cct_semantic_error_count(const cct_semantic_analyzer_t *sem);
void cct_semantic_dispose(cct_semantic_analyzer_t *sem);

const char* cct_sem_type_string(const cct_sem_type_t *type);
const char* cct_sem_symbol_kind_string(cct_sem_symbol_kind_t kind);

#endif /* CCT_SEMANTIC_H */
