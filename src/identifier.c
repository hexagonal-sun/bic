#include <string.h>
#include "identifier.h"
#define IDENTIFIER_HT_BITS 5

static hash_table identifiers;

void identifier_init(void)
{
    identifiers = hash_table_alloc(IDENTIFIER_HT_BITS);
}

identifier *get_identifier(const char *name)
{
    identifier *i;

    /* See if the identifier already exists. */
    list *bin = hash_table_get_bin(identifiers, name);
    if (!list_empty(bin)) {
        list_for_each(i, bin, hash_table_entry)
            if (strcmp(name, i->name) == 0)
                return i;
    }

    /* The identifier doesn't exist.  Create it and return it. */
    i = calloc(sizeof(*i), 1);
    i->name = name;
    hash_table_add(identifiers, name, &i->hash_table_entry);
    return i;
}
