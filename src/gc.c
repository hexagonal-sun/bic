#include "gc.h"
#include "list.h"
#include <stdlib.h>

static LIST(all_trees);

tree *top_of_stack;

#define COLLECT_TREE_ALLOC 1000
static unsigned int collect_counter = 0;

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
    case T_SIZEOF:
    case T_FN_ARG:
    case T_POINTER:
    case T_ADDR:
    case T_DEREF:
    case T_EXTERN:
    case T_STATIC:
    case T_RETURN:
    case T_STRUCT:
    case T_UNION:
    case T_TYPEDEF:
    case D_T_PTR:
        mark_tree(t->data.exp);
        break;
    case T_MUL:
    case T_DIV:
    case T_MOD:
    case T_ADD:
    case T_SUB:
    case T_LT:
    case T_GT:
    case T_LTEQ:
    case T_GTEQ:
    case T_ARRAY:
    case T_ARRAY_ACCESS:
    case T_COMP_ACCESS:
    case T_ASSIGN:
    case E_MAP:
        mark_tree(t->data.bin.left);
        mark_tree(t->data.bin.right);
        break;
    case T_DECL:
        mark_tree(t->data.decl.type);
        mark_tree(t->data.decl.decls);
        break;
    case T_DECL_COMPOUND:
        mark_tree(t->data.comp_decl.id);
        mark_tree(t->data.comp_decl.decls);
        break;
    case T_DECL_FN:
    case T_FN_DEF:
        mark_tree(t->data.function.id);
        mark_tree(t->data.function.return_type);
        mark_tree(t->data.function.arguments);
        mark_tree(t->data.function.stmts);
        break;
    case T_LOOP_FOR:
        mark_tree(t->data.floop.initialization);
        mark_tree(t->data.floop.condition);
        mark_tree(t->data.floop.afterthrought);
        mark_tree(t->data.floop.stmts);
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
            break;
    case E_CTX:
        mark_tree(t->data.ectx.id_map);
        mark_tree(t->data.ectx.alloc_chain);
        mark_tree(t->data.ectx.parent_ctx);
        break;
    case T_LIVE_COMPOUND:
        mark_tree(t->data.comp.members);
        mark_tree(t->data.comp.decl);
        break;
    case T_ENUMERATOR:
        mark_tree(t->data.enumerator.id);
        mark_tree(t->data.enumerator.enums);
        break;
    default:
        /* All other types don't need to recurse as they don't contain
         * any other objects. */
        break;
    }
}

static void unmark_all_trees(void)
{
    tree i;
    list_for_each(i, &all_trees, alloc)
        i->reachable = 0;
}

static void __attribute__((noinline)) mark_stack(void)
{
    tree bottom;
    tree *bottom_of_stack = &bottom, *ptr;

    for (ptr = top_of_stack; ptr > bottom_of_stack; ptr--)
    {
        tree i;
        list_for_each(i, &all_trees, alloc)
            if (*ptr == i)
                mark_tree(i);
    }
}

#if defined(BUILD_LINUX)
extern tree *__start_static_trees;
extern tree *__stop_static_trees;
#elif defined(BUILD_DARWIN)
extern tree *__start_static_trees __asm("section$start$__DATA$static_trees");
extern tree *__stop_static_trees __asm("section$end$__DATA$static_trees");
#else
#error "build OS"
#endif

static void mark_static(void)
{
    tree **i;
    for (i = &__start_static_trees; i < &__stop_static_trees; i++)
        mark_tree(**i);
}

static void dealloc_tree(tree t)
{
    switch (t->type)
    {
    case T_INTEGER:
        mpz_clear(t->data.integer);
        break;
    case T_IDENTIFIER:
        free(t->data.id.name);
        break;
    case T_STRING:
        free(t->data.string);
        break;
    case E_ALLOC:
        free(t->data.ptr);
        break;
    default:
        /* All other types don't contain any other referencies to
         * dynamic memory. */
        break;
    }

    list_del(&t->alloc);
    free (t);
}

static void sweep(void)
{
    list *i, *n;

    list_for_each_safe(i, n, &all_trees) {
        tree t = list_entry(i, typeof(*t), alloc);

        if (!t->reachable)
            dealloc_tree(t);
    }
}

static void collect(void)
{
    /* This undocumented intrinsic should force all callee-saved
     * registers on the stack.  This will allow us to find objects
     * that only have a reference kept in registers. */
    __builtin_unwind_init();

    unmark_all_trees();
    mark_stack();
    mark_static();
    sweep();
}

static void maybe_collect()
{
    if (collect_counter++ > COLLECT_TREE_ALLOC) {
        collect();
        collect_counter = 0;
    }
}

tree alloc_tree(void)
{
    maybe_collect();
    tree ret = calloc(sizeof(*ret), 1);
    list_add(&ret->alloc, &all_trees);
    return ret;
}
