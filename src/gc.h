#pragma once

#include "tree.h"
#include <stddef.h>

extern tree *top_of_stack;

#define GC_TREE_DECL(name)                                      \
    volatile tree *__tree_##name                                \
    __attribute__((__section__("static_trees"))) = &(name);

/* Return an uninitialised tree object that will be tracked through
 * GC. */
tree alloc_tree(void);
void collect(void);
