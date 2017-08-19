#include "typename.h"
#include "gc.h"

static tree type_names;

void add_typename(char *s)
{
    tree_chain(get_identifier(s), type_names);
}

void typename_init(void)
{
    type_names = tree_make(CHAIN_HEAD);

    /* Add all builtin types here so the parser knows about them. */
    add_typename("__builtin_va_list");
}

int is_typename(char *identifier)
{
    tree i;

    for_each_tree(i, type_names) {
        if (strcmp(identifier, tID_STR(i)) == 0)
            return 1;
    }

    return 0;
}
