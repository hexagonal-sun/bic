#pragma once

#include "tree.h"
#include "list.h"

static inline void resolve_ptr_type(tree *ptr, tree *base_type)
{
    while (*ptr != NULL && is_T_POINTER(*ptr)) {
        tree ptr_type = tree_make(D_T_PTR);
        tDTPTR_EXP(ptr_type) = *base_type;
        *base_type = ptr_type;
        *ptr = tPTR_EXP(*ptr);
    }
}

const tree get_include_chain(void);
void __attribute__((noreturn)) eval_die(tree t, const char *format, ...);
tree find_global_identifiers(const char *prefix);
tree make_live_var(tree type);
int get_c_main_return_value(tree t);
void evaluate(tree t, const char *filename);
tree evaluate_expr(tree t);
void eval_init(void);
