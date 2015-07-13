#include <stdlib.h>
#include <stdio.h>
#include "tree.h"

#define eprintf(args...) fprintf (stderr, args)

tree tree_make(enum tree_type type)
{
    tree ret = calloc(sizeof(*ret), 1);
    ret->type = type;
    return ret;
}

static const char *tree_type_string(enum tree_type t)
 {
    switch (t) {
        #define DEFTYPE(ETYPE, DESC)    \
            case ETYPE:                 \
                return #ETYPE;
        #include "tree.def"
        #undef DEFTYPE
        default:
            return "unknown";
    }
}

void tree_dump(tree tree)
{
    while (tree) {
        eprintf("<tree at %p, next %p, type %s,",
                tree, tree->next, tree_type_string(tree->type));
        switch (tree->type) {
        case T_INTEGER:
            eprintf(" number %d", tree->data.integer);
            break;
        }
        eprintf(">\n");
        tree = tree->next;
    }
}
