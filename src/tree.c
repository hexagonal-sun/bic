#include <stdlib.h>
#include "tree.h"

tree tree_make(enum tree_type type)
{
    tree ret = calloc(sizeof(*ret), 1);
    ret->type = type;
    return ret;
}
