#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../evaluate.h"

#include "function_call.h"

static void push_arg(struct arg **head, struct arg arg)
{
    struct arg *new_arg = malloc(sizeof(*new_arg));

    memcpy(new_arg, &arg, sizeof(arg));

    /* Arguments are passed in right-to-left order. */
    new_arg->next = *head;
    *head = new_arg;
}

static void handle_double(struct arg **head, double d)
{
    struct arg new_arg;

    union {
        struct {
            ptrdiff_t a, b;
        };
        double d;
    } x;

    x.d = d;

    new_arg.val.i = x.a;
    push_arg(head, new_arg);

    new_arg.val.i = x.b;
    push_arg(head, new_arg);
}

/* Here we simplify the interface for accessing arguments so that
 * it is reasonable to be done in assembly. */
union function_return do_call(void *function_address, tree args, tree ret_type,
                              bool is_variadic)
{
    tree fn_arg;
    struct arg *arg_head = NULL;
    union function_return ret;

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = tFNARG_EXP(fn_arg);
            struct arg new_arg;

            switch(arg->type) {
            case T_STRING:
                new_arg.val.i = (ptrdiff_t)tSTRING_VAL(arg);
                push_arg(&arg_head, new_arg);
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define SETINT(type)                                                    \
                    case type:                                          \
                        new_arg.val.i = (ptrdiff_t)tLV_VAL(arg)->type;   \
                        push_arg(&arg_head, new_arg);                   \
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
                    if (is_variadic)
                        handle_double(&arg_head, (double)tLV_VAL(arg)->D_T_FLOAT);
                    else {
                        new_arg.val.f = (float)tLV_VAL(arg)->D_T_FLOAT;
                        push_arg(&arg_head, new_arg);
                    }
                    break;
                case D_T_DOUBLE:
                    handle_double(&arg_head, tLV_VAL(arg)->D_T_DOUBLE);
                    break;
                case D_T_PTR:
                    new_arg.val.i = (ptrdiff_t)tLV_VAL(arg)->D_T_PTR;
                    push_arg(&arg_head, new_arg);
                    break;
                default:
                    eval_die(arg, "cannot marshall live var type\n");
                }
                break;
            case T_INTEGER:
                new_arg.val.i = (ptrdiff_t)mpz_get_si(tINT_VAL(arg));
                push_arg(&arg_head, new_arg);
                break;
            case T_FLOAT:
                handle_double(&arg_head, mpf_get_d(tFLOAT_VAL(arg)));
                break;
            default:
                eval_die(arg, "Unknown tree type to marshall.\n");
                exit(1);
            }
        }

    switch (TYPE(ret_type))
    {
    case D_T_DOUBLE:
    case D_T_FLOAT:
    {
        /* HACK ALERT */
        typedef double (*fntype)(void *, struct arg *);
        fntype fnptr = (fntype)&__do_call;
        ret.dval = fnptr(function_address, arg_head);
        break;
    }
    default:
        ret.ival = __do_call(function_address, arg_head);
    }

    return ret;
}
