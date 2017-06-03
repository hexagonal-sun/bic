#ifndef __TREE_H_
#define __TREE_H_

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "list.h"
#include "tree-access.h"

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

    /* Used for compound types such as arrays, structs and unions. */
    size_t offset;
};

typedef struct identifier identifier;

struct identifier {
    char *name;
};

struct compound_decl {
    tree id;
    tree decls;
    size_t length;
    int expanded;
    enum {
        sstruct,
        uunion
    } type;
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

struct live_var {
    tree type;
    union value *val;
    int is_array;
    size_t array_length;
};

struct live_compound {
    tree decl;
    void *base;
    tree members;
};

/* The underlying type that is used to represent a live enumeration
 * value. */
#define ENUMTYPE D_T_UINT

struct enum_type {
    tree id;
    tree enums;
};

struct for_loop {
    tree initialization;
    tree condition;
    tree afterthrought;
    tree stmts;
};

typedef struct eval_ctx {
    tree id_map;
    tree parent_ctx;
    tree alloc_chain;
    const char *name;
    int is_compound;
    int is_static;
} eval_ctx;

union tree_data {
    /* T_INTEGER */
    mpz_t integer;

    /* T_FLOAT */
    mpf_t ffloat;

    /* T_STRING */
    char *string;

    /* T_P_INC, T_P_DEC, T_INC, T_DEC, T_FN_ARG, T_POINTER */
    tree exp;

    /* T_IDENTIFIER */
    identifier id;

    /* E_ALLOC */
    void *ptr;

    /* T_MULTIPLY */
    struct binary_exp bin;

    struct declaration decl;

    /* T_DECL_COMPOUND */
    struct compound_decl comp_decl;

    /* T_FN_DEF, T_DECL_FN */
    struct function_data function;

    /* T_FN_CALL */
    struct function_call fncall;

    /* T_LIVE_VAR */
    struct live_var var;

    /* T_LIVE_COMPOUND */
    struct live_compound comp;

    /* T_ENUMERATOR */
    struct enum_type enumerator;

    /* T_LOOP_FOR */
    struct for_loop floop;

    /* E_CTX */
    struct eval_ctx ectx;
};

struct tree_locus {
    size_t line_no;
    size_t column_no;
};

struct tree {
    enum tree_type type;
    union tree_data data;
    struct tree_locus locus;
    list chain;

    /* GC members. */
    list alloc;
    int reachable;
};

/* These macros access the members of the `tree' struct. */
#define TYPE(obj) (obj)->type
#define _DATA(obj) (obj)->data

static inline tree tree_check(tree obj, enum tree_type type,
                              const char *file,
                              int line,
                              const char *function)
{
    if (TYPE (obj) != type) {
        fprintf(stderr, "Fatal error: %s:%d %s: Tree type miss-match",
                file, line, function);
        exit(1);
    }

    return obj;
}

tree tree_make(enum tree_type);
tree get_identifier(char *name);

static inline tree tree_chain_head(tree head)
{
    tree head_marker = tree_make(CHAIN_HEAD);
    list_add(&head->chain, &head_marker->chain);
    return head_marker;
}

static inline tree make_pointer_type(tree ptr, tree type)
{
    tree ret = ptr;

    /* If there is no pointer, the type is just type. */
    if (!ptr)
        return type;

    while (ptr->data.exp)
        ptr = ptr->data.exp;

    ptr->data.exp = type;

    return ret;
}

static inline void tree_chain(tree new, tree chain)
{
    list_add_tail(&new->chain, &chain->chain);
}

#define for_each_tree(pos, head)                \
    list_for_each((pos), &(head)->chain, chain)

static inline void tree_splice_chains(tree chain_dest, tree chain_src)
{
    list *i, *n;

    list_for_each_safe(i, n, &chain_src->chain) {
        tree elm = list_entry(i, struct tree, chain);
        list_del(&elm->chain);
        tree_chain(elm, chain_dest);
    }
}

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

static inline int is_CTYPE(tree t)
{
    if (!t)
        return 0;

    switch (t->type)
    {
#define DEFCTYPE(ETYPE, DESC, FMT, CTYPE)       \
        case ETYPE:
#include "ctypes.def"
#undef DEFCTYPE
        return 1;
    default:
        return 0;
    }
}

static inline int is_LIVE(tree exp)
{
    if (is_T_LIVE_VAR(exp) || is_T_LIVE_COMPOUND(exp))
        return 1;
    else
        return 0;
}

/* Copy the contents of one tree to another and overwrite.  NOTE this
 * does not do a deep copy! */
static inline void tree_copy(tree dest, tree src)
{
    memcpy(dest, src, sizeof(*dest));
}

#endif
