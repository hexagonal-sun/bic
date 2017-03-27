#ifndef __TREE_H_
#define __TREE_H_

#include <gmp.h>
#include <stdint.h>
#include "list.h"

typedef struct tree *tree;

enum tree_type {
    #define DEFTYPE(TNAME, DESC) TNAME ,
    #include "tree.def"
    #undef DEFTYPE
    #define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT) TNAME ,
    #include "ctypes.def"
    #undef DEFCTYPE
};

struct binary_exp {
    tree left;
    tree right;
};

struct declaration {
    tree type;
    tree decls;
};

typedef struct identifier identifier;

struct identifier {
    char *name;
};

struct struct_data {
    tree id;
    tree decls;
};

struct function_data {
    tree id;
    tree return_type;
    tree arguments;
    tree stmts;
};

struct function_call {
    tree identifier;
    tree arguments;
};

union value {
    #define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT) \
        STDINTSZ TNAME;
    #include "ctypes.def"
    #undef DEFCTYPE
};

typedef struct {
    tree id;
    tree t;
    list mappings;
} identifier_mapping;

struct pointer {
    tree id;
};

struct pointer_type {
    tree type;
};

struct live_var {
    tree type;
    union value val;
};

typedef struct eval_ctx {
    identifier_mapping id_map;
    tree parent_ctx;
    const char *name;
} eval_ctx;


union tree_data {
    /* T_INTEGER */
    mpz_t integer;

    /* T_STRING */
    char *string;

    /* T_P_INC, T_P_DEC, T_INC, T_DEC */
    tree exp;

    /* T_IDENTIFIER */
    identifier id;

    /* T_MULTIPLY */
    struct binary_exp bin;

    struct declaration decl;

    /* T_POINTER */
    struct pointer ptr;

    /* D_T_PTR */
    struct pointer_type ptr_type;

    /* T_STRUCT_DECL */
    struct struct_data structure;

    /* T_FN_DEF */
    struct function_data function;

    /* T_FN_CALL */
    struct function_call fncall;

    /* T_LIVE_VAR */
    struct live_var var;

    /* E_CTX */
    struct eval_ctx ectx;
};

struct tree {
    enum tree_type type;
    union tree_data data;
    list chain;

    /* GC members. */
    list alloc;
    int reachable;
};

tree tree_make(enum tree_type);
tree get_identifier(char *name);

static inline tree tree_chain_head(tree head)
{
    tree head_marker = tree_make(CHAIN_HEAD);
    list_add(&head->chain, &head_marker->chain);
    return head_marker;
}

static inline void tree_chain(tree new, tree chain)
{
    list_add_tail(&new->chain, &chain->chain);
}

#define for_each_tree(pos, head)                \
    list_for_each((i), &(head)->chain, chain)

tree tree_build_bin(enum tree_type, tree left, tree right);
void tree_dump(tree tree);

#define DEFTYPE(ETYPE, DESC)                    \
    static inline int is_##ETYPE (tree t)       \
    {                                           \
        if (!t)                                 \
            return 0;                           \
                                                \
        if (t->type == ETYPE)                   \
            return 1;                           \
        else                                    \
            return 0;                           \
    }
#include "tree.def"
#undef DEFTYPE

#define DEFCTYPE(ETYPE, DESC, FMT, CTYPE)       \
    static inline int is_##ETYPE (tree t)       \
    {                                           \
        if (!t)                                 \
            return 0;                           \
                                                \
        if (t->type == ETYPE)                   \
            return 1;                           \
        else                                    \
            return 0;                           \
    }
#include "ctypes.def"
#undef DEFCTYPE

#endif
