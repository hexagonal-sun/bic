#ifndef __TREE_H_
#define __TREE_H_

#include <gmp.h>
#include "identifier.h"

typedef struct tree *tree;

enum tree_type {
    #define DEFTYPE(TNAME, DESC) TNAME ,
    #include "tree.def"
    #undef DEFTYPE
};

struct binary_exp {
    tree left;
    tree right;
};

struct declaration {
    tree type;
    tree decls;
};

struct struct_data {
    identifier *id;
    tree decls;
};

struct function_data {
    identifier *id;
    tree return_type;
    tree arguments;
    tree stmts;
};

union tree_data {
    /* T_INTEGER */
    mpz_t integer;

    /* T_P_INC, T_P_DEC, T_INC, T_DEC */
    tree exp;

    /* T_IDENTIFIER */
    identifier *id;

    /* T_MULTIPLY */
    struct binary_exp bin;

    struct declaration decl;

    /* T_STRUCT_DECL */
    struct struct_data structure;

    /* T_FN_DEF */
    struct function_data function;
};

struct tree {
    struct tree *next;
    enum tree_type type;
    union tree_data data;
};

tree tree_make(enum tree_type);
tree tree_build_bin(enum tree_type, tree left, tree right);
void tree_dump(tree tree, int depth);

#endif
