#pragma once

#include "tree.h"
#include "list.h"

static inline void resolve_ptr_type(tree *ptr, tree *base_type)
{
    while (is_T_POINTER(*ptr)) {
        tree ptr_type = tree_make(D_T_PTR);
        ptr_type->data.exp = *base_type;
        *base_type = ptr_type;
        *ptr = (*ptr)->data.exp;
    }
}

void __attribute__((noreturn)) eval_die(tree t, const char *format, ...);
void evaluate(tree t, const char *filename);
void eval_init(void);
