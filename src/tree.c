#include <stdlib.h>
#include "tree.h"
#include "gc.h"

tree tree_make(enum tree_type type)
{
    tree ret = alloc_tree();
    ret->type = type;
    INIT_LIST(&ret->chain);
    return ret;
}

tree get_identifier(const char *name)
{
    tree ret = tree_make(T_IDENTIFIER);
    tID_STR(ret) = strdup(name);
    return ret;
}

tree get_integer(unsigned int n)
{
    tree ret = tree_make(T_INTEGER);
    mpz_init_set_ui(tINT_VAL(ret), n);
    return ret;
}

tree get_addr(void *p)
{
    tree ret = tree_make(T_INTEGER);
    mpz_init_set_ui(tINT_VAL(ret), (ptrdiff_t)p);
    return ret;
}
