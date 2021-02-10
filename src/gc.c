#include "tree.h"
#include "gc.h"
#include "gc-internal.h"

static int gc_inhibited = 0;
static size_t alloc_array_sz = 10000;
static size_t alloc_ptr = 0;
static gc_obj *allocs = NULL;
static const int COLLECT_TREE_ALLOC = 2000;

void mark_tree(tree obj)
{
    if (!obj)
        return;

    tree locus_file = obj->locus.file;

    if (locus_file)
        mark_object((gc_obj)locus_file);

    mark_object((gc_obj)obj);
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
        mpz_clear(tINT_VAL(t));
        break;
    case T_IDENTIFIER:
        free(tID_STR(t));
        break;
    case T_STRING:
        free(tSTRING_VAL(t));
        break;
    case E_ALLOC:
        free(tALLOC_PTR(t));
        break;
    case T_CLOSURE:
        ffi_closure_free(tCLOSURE_CLOSURE(t));
        free(tCLOSURE_CIF(t));
        free(tCLOSURE_TYPES(t));
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
        } else
            allocs[i]->reachable = 0;
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
    size_t old_alloc_array_sz = alloc_array_sz;
    alloc_array_sz *= 2;

    allocs = realloc(allocs, sizeof(*allocs) * alloc_array_sz);
    memset(&allocs[old_alloc_array_sz], 0, old_alloc_array_sz);
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
