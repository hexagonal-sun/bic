#include "typename.h"
#include "gc.h"

static tree type_names;
GC_STATIC_TREE(type_names);

/* Typenames that are located within #include files. */
static tree include_type_names;
GC_STATIC_TREE(include_type_names);

static int in_include_file = 0;

void add_typename(tree typename)
{
    tree new_typename = get_identifier(tID_STR(typename));
    tree_chain(new_typename,
               in_include_file ? include_type_names : type_names);
}

void typename_set_include_file(void)
{
    in_include_file = 1;
}

void typename_unset_include_file(void)
{
    in_include_file = 0;
}

void reset_include_typenames(void)
{
    include_type_names = tree_make(CHAIN_HEAD);
}

void typename_init(void)
{
    type_names = tree_make(CHAIN_HEAD);

    reset_include_typenames();

    /* Add all builtin types here so the parser knows about them. */
    add_typename(get_identifier("__builtin_va_list"));
    add_typename(get_identifier("_Float128"));
}

int is_typename(tree typename)
{
    tree i;

    for_each_tree(i, type_names) {
        if (strcmp(tID_STR(typename), tID_STR(i)) == 0)
            return 1;
    }

    for_each_tree(i, include_type_names) {
        if (strcmp(tID_STR(typename), tID_STR(i)) == 0)
            return 1;
    }

    return 0;
}
