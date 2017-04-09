#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "function_call.h"

struct arg *arg_head = NULL;

/* Arguments destined for the integer registers: rdi, rsi, etc. */
static struct arg *int_args = NULL;

/* Arguments destined for the vector registers: xmm0, xmm1, etc. */
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
ptrdiff_t do_call(void *function_address, tree args)
{
    tree fn_arg;
    ptrdiff_t ret;

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = fn_arg->data.exp;
            struct arg *new_arg = malloc(sizeof(*new_arg));

            switch(arg->type) {
            case T_STRING:
                new_arg->val.i = (ptrdiff_t)arg->data.string;
                new_arg->dest = INTEGER;
                push_arg(&int_args, new_arg);
                break;
            case T_LIVE_VAR:
                new_arg->val.i = (ptrdiff_t)arg->data.var.val.D_T_PTR;
                new_arg->dest = INTEGER;
                push_arg(&int_args, new_arg);
                break;
            case T_INTEGER:
                new_arg->val.i = (ptrdiff_t)mpz_get_si(arg->data.integer);
                new_arg->dest = INTEGER;
                push_arg(&int_args, new_arg);
                break;
            case T_FLOAT:
                new_arg->val.d = mpf_get_d(arg->data.ffloat);
                new_arg->dest = SSE;
                push_arg(&vec_args, new_arg);
                break;
            default:
                fprintf(stderr, "Error: Unknown tree type to marshall.\n");
                exit(1);
            }
        }

    /* Construct the list of arguments that we pass into the assembly
     * function.  This needs to be done in the order that the assembly
     * moves the arguments. */
    push_to_main_arg_head(vec_args);
    push_to_main_arg_head(int_args);

    ret = __do_call(function_address, arg_head);

    /* Cleanup consumed arguments. */
    while (arg_head) {
        struct arg *old_arg = arg_head;
        arg_head = arg_head->next;
        free(old_arg);
    }

    int_args = NULL;
    vec_args = NULL;

    return ret;
}
