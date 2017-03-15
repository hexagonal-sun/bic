#include "gc.h"
#include "list.h"
#include <stdlib.h>

static LIST(all_trees);

ptrdiff_t top_of_stack;

tree alloc_tree(void)
{
    tree ret = calloc(sizeof(*ret), 1);
    list_add(&ret->alloc, &all_trees);
    return ret;
}

static void mark_tree(tree t)
{
    if (!t)
        return;

    t->reachable = 1;

    switch (t->type) {
    case T_P_INC:
    case T_P_DEC:
    case T_INC:
    case T_DEC:
        mark_tree(t->data.exp);
        break;
    case T_MUL:
    case T_DIV:
    case T_MOD:
    case T_ADD:
    case T_SUB:
    case T_ASSIGN:
        mark_tree(t->data.bin.left);
        mark_tree(t->data.bin.right);
        break;
    case T_DECL:
        mark_tree(t->data.decl.type);
        mark_tree(t->data.decl.decls);
        break;
    case T_DECL_STRUCT:
        mark_tree(t->data.structure.decls);
        break;
    case T_FN_DEF:
        mark_tree(t->data.function.return_type);
        mark_tree(t->data.function.arguments);
        mark_tree(t->data.function.stmts);
        break;
    case T_FN_CALL:
        mark_tree(t->data.fncall.identifier);
        mark_tree(t->data.fncall.arguments);
        break;
    case T_LIVE_VAR:
        mark_tree(t->data.var.type);
        break;
    case CHAIN_HEAD:
            {
                tree i;
                for_each_tree(i, t)
                    mark_tree(i);
            }
    }
}

static void unmark_all_trees(void)
{
    tree i;
    list_for_each(i, &all_trees, alloc)
        i->reachable = 0;
}

static void mark_stack(void)
{
    int bottom;
    ptrdiff_t bottom_of_stack = (ptrdiff_t)&bottom,
        ptr;

     for (ptr = top_of_stack; ptr > bottom_of_stack; ptr--)
    {
        tree i;
        list_for_each(i, &all_trees, alloc)
            if (ptr == (ptrdiff_t)i)
                mark_tree(i);
    }
}

extern tree *__start_static_trees;
extern tree *__stop_static_trees;

static void mark_static(void)
{
    tree **i;
    for (i = &__start_static_trees; i < &__stop_static_trees; i++)
        mark_tree(**i);
}

void collect(void)
{
       unmark_all_trees();
       mark_stack();
       mark_static();
}
