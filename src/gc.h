#pragma once
#include "tree.h"

#if defined(BUILD_LINUX)
# define GC_STATIC_TREE(name)                                   \
    volatile tree *__tree_##name                                \
    __attribute__((__section__("static_trees"))) = &(name);
#elif defined(BUILD_DARWIN)
# define GC_STATIC_TREE(name)                                           \
    volatile tree *__tree_##name                                        \
    __attribute__((__section__("__DATA,static_trees"))) = &(name);
#else
# error "Unknown build OS"
#endif

struct gc_obj;
typedef struct gc_obj *gc_obj;

void enable_gc();
void inhibit_gc();

tree alloc_tree();
