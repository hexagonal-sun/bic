#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "function_call.h"

struct arg *arg_head = NULL;
extern ptrdiff_t __do_call(void *fn, struct arg *args);


/* Here we simplify the interface for accessing arguments so that
 * it is reasonable to be done in assembly. */
ptrdiff_t do_call(void *function_address, tree args)
{
    tree fn_arg;
    ptrdiff_t ret;
    struct arg *arg_rear;

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = fn_arg->data.exp;
            struct arg *new_arg = malloc(sizeof(*new_arg));

            switch(arg->type) {
            case T_STRING:
                new_arg->val = (ptrdiff_t)arg->data.string;
                new_arg->class = POINTER;
                break;
            case T_LIVE_VAR:
                new_arg->val = (ptrdiff_t)arg->data.var.val.D_T_PTR;
                new_arg->class = POINTER;
                break;
            case T_INTEGER:
                new_arg->val = (ptrdiff_t)mpz_get_si(arg->data.integer);
                new_arg->class = INTEGER;
                break;
            default:
                fprintf(stderr, "Error: Unknown tree type to marshall.\n");
                exit(1);
            }

            new_arg->next = NULL;

            if (!arg_head)
                arg_head = arg_rear = new_arg;
            else
                arg_rear->next = new_arg;

            arg_rear = new_arg;
        }

    ret = __do_call(function_address, arg_head);

    /* Cleanup consumed arguments. */
    while (arg_head) {
        struct arg *old_arg = arg_head;
        arg_head = arg_head->next;
        free(old_arg);
    }

    return ret;
}
