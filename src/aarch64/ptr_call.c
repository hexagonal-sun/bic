#include <stdio.h>
#include <stdlib.h>

#include "../evaluate.h"
#include "../spec-resolver.h"
#include "ptr_call.h"

extern void entry_point_0(void);
extern void entry_point_1(void);
extern void entry_point_end(void);

struct ptr_map {
    void *entry_addr;
    tree fndef;
    struct ptr_map *next;
};

static struct ptr_map *ptr_call_map = NULL;
static void *next_entry_point = &entry_point_0;

void *get_entry_point_for_fn(tree fndef)
{
    ptrdiff_t entry_point_delta = &entry_point_1 - &entry_point_0;
    void *entry_point_end = &entry_point_end;
    struct ptr_map *new_map = malloc(sizeof(*new_map));
    void *entry_addr = next_entry_point;

    if (next_entry_point > entry_point_end) {
        eval_die(fndef, "can not take address of function,"
                 "entry points exhausted.\n");
    }

    new_map->entry_addr = entry_addr;
    new_map->fndef = fndef;
    new_map->next = ptr_call_map;
    ptr_call_map = new_map;

    next_entry_point += entry_point_delta;

    return entry_addr;
}

tree resolve_fn_call(void *entry_addr)
{
    struct ptr_map *cur_map = ptr_call_map;

    while (cur_map) {
        if (cur_map->entry_addr == entry_addr)
            return cur_map->fndef;

        cur_map = cur_map->next;
    }

    fprintf(stderr, "Error: could not resolve function for entry point %p\n",
            entry_addr);

    exit(1);
}

static tree get_argument_chain(tree fndef, struct argregs *regs)
{
    int integer_idx = 0;
    tree ret, arg, args = tFN_ARGS(fndef);

    if (!args)
        return NULL;

    ret = tree_make(CHAIN_HEAD);

    for_each_tree(arg, args) {
        tree argType = resolve_decl_specs_to_type(tDECL_SPECS(arg)),
            decl = tDECL_DECLS(arg),
            new_arg;

        resolve_ptr_type(&decl, &argType);

        switch (TYPE(argType)) {
#define CREATE_INT_ARG(type, mpz_func, ctype)                           \
            case type:                                                  \
                new_arg = tree_make(T_INTEGER);                         \
                mpz_func (tINT_VAL(new_arg),                                \
                          (ctype) regs->iarg[integer_idx]);             \
                integer_idx++;                                          \
                break;
            CREATE_INT_ARG(D_T_CHAR, mpz_init_set_si, char);
            CREATE_INT_ARG(D_T_SHORT, mpz_init_set_si, short);
            CREATE_INT_ARG(D_T_INT, mpz_init_set_si, int);
            CREATE_INT_ARG(D_T_LONG, mpz_init_set_si, long);
            CREATE_INT_ARG(D_T_LONGLONG, mpz_init_set_si, long long);
            CREATE_INT_ARG(D_T_UCHAR, mpz_init_set_ui, unsigned char);
            CREATE_INT_ARG(D_T_USHORT, mpz_init_set_ui, unsigned short);
            CREATE_INT_ARG(D_T_UINT, mpz_init_set_ui, unsigned int);
            CREATE_INT_ARG(D_T_ULONG, mpz_init_set_ui, unsigned long);
            CREATE_INT_ARG(D_T_ULONGLONG, mpz_init_set_ui, unsigned long long);
            CREATE_INT_ARG(D_T_PTR, mpz_init_set_ui, ptrdiff_t);
        default:
            eval_die(arg, "unknown parameter type in pointer call\n");
        }

        tree_chain(new_arg, ret);
    }

    return ret;
}

ptrdiff_t handle_ptr_call(struct argregs *regs, void *pc)
{
    tree fndef = resolve_fn_call(pc),
        fncall = tree_make(T_FN_CALL);

    tFNCALL_ID(fncall) = tFN_DECL(fndef);
    tFNCALL_ARGS(fncall) = get_argument_chain(fndef, regs);

    evaluate(fncall, "<PTR>");

    return 0;
}
