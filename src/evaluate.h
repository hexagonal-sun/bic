#pragma once

#include "tree.h"
#include "list.h"

static inline void resolve_ptr_type(tree *ptr, tree *base_type)
{
    while (is_T_POINTER(*ptr)) {
        tree ptr_type = tree_make(D_T_PTR);
        tDTPTR_EXP(ptr_type) = *base_type;
        *base_type = ptr_type;
        *ptr = tPTR_EXP(*ptr);
    }
}

void __attribute__((noreturn)) eval_die(tree t, const char *format, ...);
tree find_global_identifiers(const char *prefix);
int get_c_main_return_value(tree t);
tree evaluate(tree t, const char *filename);
void eval_init(void);
