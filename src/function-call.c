#include "evaluate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ffi.h>

#include "ffi-types.h"
#include "function-call.h"
#include "evaluate.h"
#include "gc.h"

static ffi_type *type_stack[32];
static void *val_stack[32];
static int n = 0;

static void push_arg(ffi_type *type, void* val, bool should_promote)
{
    /* Acording to the 'default argument promotions' floats should be promotoed
       to doubles and <int should be promoted to int when being passed as a
       varadic parameter. */
#define PROMOTE(FFI_TYPE, CTYPE, TARGET_T_TYPE, TARGET_C_TYPE, TARGET_FFI_TYPE) \
    if (should_promote && type == &(FFI_TYPE)) {                        \
        tree dlv = make_live_var(tree_make(TARGET_T_TYPE));             \
        CTYPE *cval = (CTYPE *)val;                                     \
        tLV_VAL(dlv)->TARGET_T_TYPE = *cval;                            \
        val = &tLV_VAL(dlv)->TARGET_T_TYPE;                             \
        type = &(TARGET_FFI_TYPE);                                      \
    }
    PROMOTE(ffi_type_float, float, D_T_DOUBLE, double, ffi_type_double);
    PROMOTE(ffi_type_schar, char, D_T_INT, int, ffi_type_sint);
    PROMOTE(ffi_type_sshort, short, D_T_INT, int, ffi_type_sint);
    PROMOTE(ffi_type_uchar, unsigned char, D_T_UINT, unsigned int, ffi_type_uint);
    PROMOTE(ffi_type_ushort, unsigned short, D_T_UINT, unsigned int, ffi_type_uint);
#undef PROMOTE

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

    // Prevent GC here, we could potentially create temporary LV for the default
    // argument promotions. FFI would have a reference to the LVs but the trees
    // would be dangling, making them a candidate for GC. Prevent that until
    // after function calling is complete.
    inhibit_gc();

    if (args)
        for_each_tree(fn_arg, args) {
            bool should_promote = variadic_pos ? n >= *variadic_pos : false;
            tree arg = tFNARG_EXP(fn_arg);

            switch(TYPE(arg)) {
            case T_STRING:
                push_arg(&ffi_type_pointer, &tSTRING_VAL(arg), should_promote);
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM)                     \
                    case TNAME:                                         \
                        push_arg(get_ffi_type(TNAME), &tLV_VAL(arg)->TNAME, should_promote); \
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
                    push_arg(&ffi_type_sint, buf, should_promote);
                    break;
                }
            case T_FLOAT:
                {
                    double *buf = alloca(sizeof(double));
                    *buf = mpf_get_d(tFLOAT_VAL(arg));
                    push_arg(&ffi_type_double, buf, should_promote);
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

    enable_gc();
}
