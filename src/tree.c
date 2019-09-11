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
