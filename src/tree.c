#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "gc.h"

#define eprintf(args...) fprintf (stderr, args)

static const char *tree_desc_string[] = {
    #define DEFTYPE(ETYPE, DESC) [ETYPE] = DESC ,
    #include "tree.def"
    #undef DEFTYPE
    #define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT) [ETYPE] = DESC ,
    #include "ctypes.def"
    #undef DEFCTYPE
};

tree tree_make(enum tree_type type)
{
    tree ret = alloc_tree();
    ret->type = type;
    INIT_LIST(&ret->chain);
    return ret;
}

tree get_identifier(char *name)
{
    tree ret = tree_make(T_IDENTIFIER);
    tID_STR(ret) = name;
    return ret;
}

tree tree_build_bin(enum tree_type type, tree left, tree right)
{
    tree ret = tree_make(type);
    ret->data.binary_exp.left = left;
    ret->data.binary_exp.right = right;
    return ret;
}

static void __tree_dump(tree head, int depth);

static void tree_print_indent(int depth)
{
    int i;
    for (i = 0; i < depth *2; i++)
        eprintf(" ");
}

void tree_dump(tree head)
{
    return;
}
