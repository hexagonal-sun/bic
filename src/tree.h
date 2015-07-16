#ifndef __TREE_H_
#define __TREE_H_

#include <gmp.h>

enum tree_type {
    #define DEFTYPE(TNAME, DESC) TNAME ,
    #include "tree.def"
    #undef DEFTYPE
};

union tree_data {
    /* T_INTEGER */
    mpz_t integer;
};

typedef struct tree {
    struct tree *next;
    enum tree_type type;
    union tree_data data;
} *tree;

tree tree_make(enum tree_type);
void tree_dump(tree tree);

#endif
