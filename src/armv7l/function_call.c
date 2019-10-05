#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "../evaluate.h"

#include "function_call.h"

struct arg *arg_head = NULL;

/* Arguments destined for the integer registers: r0, r1, r2, etc. */
static struct arg *int_args = NULL;

/* Arguments destined for the vector registers: d0, d1, etc. */
static struct arg *vec_args = NULL;

/* Arguments destined for the the stack. */
static struct arg *stacked_args = NULL;

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

static void push_to_main_arg_head_reverse(struct arg *head)
{
    if (!head)
        return;

    push_to_main_arg_head_reverse(head->next);

    push_arg(&arg_head, head);
}

static void handle_word_arg(ptrdiff_t val, int *ncrn, int *nssn)
{
    struct arg *new_arg = malloc(sizeof(*new_arg));

    if (*ncrn > 3)
    {
        new_arg->dest = STACKED;
        new_arg->val.ival = val;
        push_arg(&stacked_args, new_arg);
        (*nssn)++;
    } else {
        new_arg->dest = CORE_REG;
        new_arg->val.ival = val;
        push_arg(&int_args, new_arg);
        (*ncrn)++;
    }
}

static void handle_wide_arg(uint64_t arg, int *ncrn, int *nssn)
{
    union {
        struct {
            uint32_t hi;
            uint32_t lo;
        };
        uint64_t val;
    } a;

    a.val = arg;

    if (*ncrn == 1)
    {
        /* According to the APCS Rule C.3:
         *
         * If the argument requires double-word alignment (8-byte), the NCRN
         * is rounded up to the next even register number. */
        handle_word_arg(0, ncrn, nssn);
    }

    handle_word_arg(a.hi, ncrn, nssn);
    handle_word_arg(a.lo, ncrn, nssn);
}

static void handle_double_arg(double val, int *ncrn, int *nvrn, int *nssn,
                              bool is_variadic)
{
    if (is_variadic)
    {
        union {
            uint64_t ival;
            double dval;
        } dunion;

        dunion.dval = val;
        handle_wide_arg(dunion.ival, ncrn, nssn);
        return;
    }

    struct arg *new_arg = malloc(sizeof(*new_arg));

    new_arg->val.dval = val;
    new_arg->dest = VECTOR_REG;
    push_arg(&vec_args, new_arg);
    (*nvrn)++;
}

/* Here we simplify the interface for accessing arguments so that
 * it is reasonable to be done in assembly. */
union function_return do_call(void *function_address, tree args, tree ret_type,
                              bool is_variadic)
{
    tree fn_arg;
    union function_return ret;

    int ncrn = 0;               /* Next core register number */
    int nvrn = 0;               /* Next vector register number */
    int nssn = 0;               /* Next stack slot number */

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = tFNARG_EXP(fn_arg);

            switch(arg->type) {
            case T_STRING:
                handle_word_arg((ptrdiff_t)tSTRING_VAL(arg), &ncrn, &nssn);
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define SETINT(type)                                                    \
                    case type:                                          \
                        handle_word_arg((ptrdiff_t)tLV_VAL(arg)->type, &ncrn, &nssn); \
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
                {
                    if (is_variadic)
                        handle_double_arg(tLV_VAL(arg)->D_T_FLOAT, &ncrn, &nvrn, &nssn, is_variadic);
                    else {
                        union {ptrdiff_t ival; float fval;} a;
                        a.fval = tLV_VAL(arg)->D_T_FLOAT;
                        handle_word_arg(a.ival, &ncrn, &nssn);
                    }
                    break;
                }
                case D_T_DOUBLE:
                    handle_double_arg((double)tLV_VAL(arg)->D_T_DOUBLE, &ncrn, &nvrn, &nssn,
                                  is_variadic);
                    break;
                case D_T_PTR:
                    handle_word_arg((ptrdiff_t)tLV_VAL(arg)->D_T_PTR, &ncrn, &nssn);
                    break;
                default:
                    eval_die(arg, "cannot marshall live var type\n");
                }
                break;
            case T_INTEGER:
                if (mpz_sgn(tINT_VAL(arg)) == -1)
                    handle_word_arg(mpz_get_si(tINT_VAL(arg)), &ncrn, &nssn);
                else
                    handle_word_arg(mpz_get_ui(tINT_VAL(arg)), &ncrn, &nssn);
                break;
            case T_FLOAT:
                handle_double_arg(mpf_get_d(tFLOAT_VAL(arg)), &ncrn, &nvrn, &nssn,
                                  is_variadic);
                break;
            default:
                eval_die(arg, "Unknown tree type to marshall.\n");
                exit(1);
            }
        }

    if (nssn & 0x1) {
        /* The ARM calling convention states that the stack *must* be 8-byte
         * aligned. Therefore, if we have pushed an odd number of arguments onto
         * the stack, it will not be.
         *
         * If that is the case, push a bogus argument to maintain stack
         * alignment. We don't have to worry about the order here as the order
         * of stack arguments is reversed, so this will become the 'last'
         * argument and should never be accessed by the caller. */
        struct arg *new_arg = malloc(sizeof(*new_arg));
        new_arg->dest = STACKED;
        new_arg->val.ival = 0;
        push_arg(&stacked_args, new_arg);
        nssn++;
    }

    /* Construct the list of arguments that we pass into the assembly function.
     * This needs to be done in reverse order that the assembly moves the
     * arguments. Also note that stacked args are pushed in reverse order. */
    push_to_main_arg_head_reverse(stacked_args);
    push_to_main_arg_head(vec_args);
    push_to_main_arg_head(int_args);

    ret.ival = __do_call(function_address, arg_head);

    int_args = NULL;
    vec_args = NULL;
    arg_head = NULL;
    stacked_args = NULL;

    return ret;
}
