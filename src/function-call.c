#include "evaluate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ffi.h>

#include "ffi-types.h"
#include "function-call.h"

static ffi_type *type_stack[32];
static void *val_stack[32];
static int n = 0;

static void push_arg(ffi_type *type, void* val)
{
    if (n == 32) {
        fprintf(stderr , "Max num args reached.\n");
        exit(1);
    }

    type_stack[n] = type;
    val_stack[n] = val;

    n++;
}

void do_ext_call(void *function_address, tree args, tree ret_lv,
                 int *variadic_pos)
{
    tree fn_arg;
    ffi_cif cif;
    n = 0;
    ffi_type *ret_type = &ffi_type_void;

    if (args)
        for_each_tree(fn_arg, args) {
            tree arg = tFNARG_EXP(fn_arg);

            switch(TYPE(arg)) {
            case T_STRING:
                push_arg(&ffi_type_pointer, &tSTRING_VAL(arg));
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM)                     \
                    case TNAME:                                         \
                        push_arg(get_ffi_type(TNAME), &tLV_VAL(arg)->TNAME); \
                        break;
#include "ctypes.def"
#undef DEFCTYPE
                default:
                    eval_die(arg, "cannot marshall live var type\n");
                }
                break;
            case T_INTEGER:
                {
                    int *buf = alloca(sizeof(int));
                    *buf = mpz_get_si(tINT_VAL(arg));
                    push_arg(&ffi_type_sint, buf);
                    break;
                }
            case T_FLOAT:
                {
                    double *buf = alloca(sizeof(double));
                    *buf = mpf_get_d(tFLOAT_VAL(arg));
                    push_arg(&ffi_type_double, buf);
                    break;
                }
            default:
                eval_die(arg, "Unknown tree type to marshall.\n");
                exit(1);
            }
        }

    if (ret_lv)
        ret_type = get_ffi_type(TYPE(tLV_TYPE(ret_lv)));

    if (variadic_pos)
        ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, *variadic_pos, n, ret_type, &type_stack);
    else
        ffi_prep_cif(&cif, FFI_DEFAULT_ABI, n, ret_type, &type_stack);

    ffi_call(&cif, function_address, ret_lv ? (void *)tLV_VAL(ret_lv) : NULL, val_stack);
}
