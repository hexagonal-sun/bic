#pragma once

#include "tree.h"
#include "config.h"
#include <stddef.h>

extern tree *top_of_stack;

#if defined(BUILD_LINUX)
# define GC_TREE_DECL(name)                                     \
    volatile tree *__tree_##name                                \
    __attribute__((__section__("static_trees"))) = &(name);
#elif defined(BUILD_DARWIN)
# define GC_TREE_DECL(name)                                             \
    volatile tree *__tree_##name                                        \
    __attribute__((__section__("__DATA,static_trees"))) = &(name);
#else
# error "Unknown build OS"
#endif

/* Return an uninitialised tree object that will be tracked through
 * GC. */
tree alloc_tree(void);
void collect(void);
