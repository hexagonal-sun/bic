#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../evaluate.h"

#include "function_call.h"

struct arg *arg_head = NULL;

/* Arguments destined for the integer registers: r0, r1, r2, etc. */
static struct arg *int_args = NULL;

/* Arguments destined for the vector registers: v0, v1, etc. */
static struct arg *vec_args = NULL;

extern ptrdiff_t __do_call(void *fn, struct arg *args);

static void push_arg(struct arg **head, struct arg *new_arg)
{
    new_arg->next = *head;
    *head = new_arg;
}

/* Push all of the arguments specified in one the type specific lists
 * (int_head, for example) onto the main list of arguments,
 * arg_head.  This function will reverse the  */
static void push_to_main_arg_head(struct arg *head)
{
    struct arg *next;

    while (head) {
        next = head->next;
        push_arg(&arg_head, head);
        head = next;
    }
}

/* Here we simplify the interface for accessing arguments so that
 * it is reasonable to be done in assembly. */
union function_return do_call(void *function_address, tree args, tree ret_type,
                         bool is_variadic)
{
    tree fn_arg;
    union function_return ret;

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = tFNARG_EXP(fn_arg);
            struct arg *new_arg = malloc(sizeof(*new_arg));

            switch(arg->type) {
            case T_STRING:
                new_arg->val.i = (ptrdiff_t)tSTRING_VAL(arg);
                new_arg->dest = INTEGRAL;
                push_arg(&int_args, new_arg);
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define SETINT(type)                                                    \
                    case type:                                          \
                        new_arg->val.i = (ptrdiff_t)tLV_VAL(arg)->type; \
                        new_arg->dest = INTEGRAL;                       \
                        push_arg(&int_args, new_arg);                   \
                        break;
                    SETINT(D_T_CHAR);
                    SETINT(D_T_SHORT);
                    SETINT(D_T_INT);
                    SETINT(D_T_LONG);
                    SETINT(D_T_LONGLONG);
                    SETINT(D_T_UCHAR);
                    SETINT(D_T_USHORT);
                    SETINT(D_T_UINT);
                    SETINT(D_T_ULONG);
                    SETINT(D_T_ULONGLONG);
#undef SETINT
                case D_T_FLOAT:
                    new_arg->val.d = (double)tLV_VAL(arg)->D_T_FLOAT;
                    new_arg->dest = VECTOR;
                    push_arg(&vec_args, new_arg);
                    break;
                case D_T_DOUBLE:
                    new_arg->val.d = tLV_VAL(arg)->D_T_DOUBLE;
                    new_arg->dest = VECTOR;
                    push_arg(&vec_args, new_arg);
                    break;
                case D_T_PTR:
                    new_arg->val.i = (ptrdiff_t)tLV_VAL(arg)->D_T_PTR;
                    new_arg->dest = INTEGRAL;
                    push_arg(&int_args, new_arg);
                    break;
                default:
                    eval_die(arg, "cannot marshall live var type\n");
                }
                break;
            case T_INTEGER:
                new_arg->val.i = (ptrdiff_t)mpz_get_si(tINT_VAL(arg));
                new_arg->dest = INTEGRAL;
                push_arg(&int_args, new_arg);
                break;
            case T_FLOAT:
                new_arg->val.d = mpf_get_d(tFLOAT_VAL(arg));
                new_arg->dest = VECTOR;
                push_arg(&vec_args, new_arg);
                break;
            default:
                eval_die(arg, "Unknown tree type to marshall.\n");
                exit(1);
            }
        }

    /* Construct the list of arguments that we pass into the assembly
     * function.  This needs to be done in the order that the assembly
     * moves the arguments. */
    push_to_main_arg_head(vec_args);
    push_to_main_arg_head(int_args);

    ret.ival = __do_call(function_address, arg_head);

    int_args = NULL;
    vec_args = NULL;

    return ret;
}
