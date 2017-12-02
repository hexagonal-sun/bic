#include "tree.h"
#include "gc.h"

struct gc_obj {
    struct tree t;
    uint8_t reachable;
};

static int gc_inhibited = 0;
static size_t alloc_array_sz = 10000;
static size_t alloc_ptr = 0;
static gc_obj *allocs = NULL;
static const int COLLECT_TREE_ALLOC = 2000;

static void mark_object(gc_obj obj);

static void mark_tree(tree obj)
{
    mark_object((gc_obj)obj);
}

static void mark_object(gc_obj obj)
{
    tree t;

    if (!obj)
        return;

    obj->reachable = 1;

    t = &obj->t;

    switch (TYPE(t)) {
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
    case T_LSHIFT:
    case T_RSHIFT:
    case T_LT:
    case T_GT:
    case T_LTEQ:
    case T_GTEQ:
    case T_EQ:
    case T_CAST:
    case T_ARRAY:
    case T_ARRAY_TYPE:
    case T_N_EQ:
    case T_L_OR:
    case T_L_AND:
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
        mark_tree(t->data.ectx.parent_ctx);
        mark_tree(t->data.ectx.alloc_chain);
        break;
    case T_LIVE_COMPOUND:
        mark_tree(t->data.comp.members);
        mark_tree(t->data.comp.decl);
        break;
    case T_ENUMERATOR:
        mark_tree(t->data.enumerator.id);
        mark_tree(t->data.enumerator.enums);
        break;
    case T_IF:
        mark_tree(t->data.ifstmt.condition);
        mark_tree(t->data.ifstmt.true_stmts);
        mark_tree(t->data.ifstmt.else_stmts);
        break;
    case T_INTEGER:
    case T_FLOAT:
    case T_STRING:
    case T_IDENTIFIER:
    case T_VARIADIC:
    case E_ALLOC:
    case E_INCOMP_TYPE:
    case CPP_INCLUDE:
    case D_T_VOID:
    case D_T_CHAR:
    case D_T_SHORT:
    case D_T_INT:
    case D_T_LONG:
    case D_T_LONGLONG:
    case D_T_UCHAR:
    case D_T_USHORT:
    case D_T_UINT:
    case D_T_ULONG:
    case D_T_ULONGLONG:
    case D_T_FLOAT:
    case D_T_DOUBLE:
    case D_T_LONGDOUBLE:
        /* All other types don't need to recurse as they don't contain
         * any other objects. */
        break;
    }
}

static int compare_allocs(const void *a1, const void *a2)
{
    gc_obj p1 = *(gc_obj *)a1;
    gc_obj p2 = *(gc_obj *)a2;

    if (p1 < p2)
        return 1;
    else if (p1 > p2)
        return -1;
    else
        return 0;
}

static int is_object(void *addr)
{
    size_t i;

    if (bsearch(&addr, allocs, alloc_ptr,
                sizeof(*allocs), compare_allocs))
        return 1;

    return 0;
}

static void mark_potential_root(void *addr)
{
    gc_obj obj;

    if (!is_object(addr))
        return;

    obj = (gc_obj)addr;
    mark_object(obj);
}

void *top_of_stack;

static void __attribute__((noinline)) mark_stack(void)
{
    gc_obj bottom;
    gc_obj *bottom_of_stack = &bottom, *ptr;

    for (ptr = top_of_stack; ptr > bottom_of_stack; ptr--)
        mark_potential_root(*ptr);
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
        if (is_object(**i))
            mark_tree(**i);
}

static void dealloc_object(gc_obj obj)
{
    tree t = &obj->t;

    switch (TYPE(t))
    {
    case T_INTEGER:
        mpz_clear(tINT(t));
        break;
    case T_IDENTIFIER:
        free(tID_STR(t));
        break;
    case T_STRING:
        free(tSTRING(t));
        break;
    case E_ALLOC:
        free(tALLOC_PTR(t));
        break;
    default:
        /* All other types don't contain any other referencies to
         * dynamic memory. */
        break;
    }

    free(obj);
}

static void sweep(void)
{
    size_t i;

    for (i = 0; i < alloc_ptr; i++)
        if (!allocs[i]->reachable) {
            dealloc_object(allocs[i]);
            allocs[i] = 0;
        }
}

static void fixup_alloc_array()
{
    qsort(allocs, alloc_ptr, sizeof(*allocs),
          compare_allocs);

    /* Run the alloc pointer backward to see when the first allocation
     * is. */
    while (alloc_ptr > 0 && !allocs[alloc_ptr])
        alloc_ptr--;

    alloc_ptr++;
}

static void collect(void)
{
    /* This undocumented intrinsic should force all callee-saved
     * registers on the stack.  This will allow us to find objects
     * that only have a reference kept in registers. */
    __builtin_unwind_init();

    fixup_alloc_array();
    mark_stack();
    mark_static();
    sweep();
}

static void maybe_collect()
{
    static int collect_counter = 0;

    if (gc_inhibited)
        return;

    if (collect_counter++ > COLLECT_TREE_ALLOC) {
        collect();
        collect_counter = 0;
    }
}

static void extend_allocs()
{
    alloc_array_sz *= 2;

    allocs = realloc(allocs, sizeof(*allocs) * alloc_array_sz);
}

static void track_new_alloc(gc_obj obj)
{
    if (!allocs)
        allocs = calloc(sizeof(obj), alloc_array_sz);

    if (alloc_ptr >= alloc_array_sz)
        extend_allocs();

    allocs[alloc_ptr++] = obj;
}

void inhibit_gc()
{
    gc_inhibited++;
}

void enable_gc()
{
    if (!gc_inhibited)
        return;

    gc_inhibited--;
}

tree alloc_tree()
{
    maybe_collect();
    gc_obj ret = calloc(1, sizeof(*ret));
    track_new_alloc(ret);
    return &ret->t;
}
