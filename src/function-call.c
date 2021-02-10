#include "evaluate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ffi.h>

#include "ffi-types.h"
#include "function-call.h"
#include "evaluate.h"
#include "gc.h"
#include "spec-resolver.h"

#define ARG_STACK_SZ 32

static void push_arg(ffi_type **type_stack,
                     void **val_stack,
                     int *n,
                     ffi_type *type,
                     void* val,
                     bool should_promote)
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

    if (*n == ARG_STACK_SZ) {
        fprintf(stderr , "Max num args reached.\n");
        exit(1);
    }

    type_stack[*n] = type;
    val_stack[*n] = val;

    (*n)++;
}

void do_ext_call(void *function_address, tree args, tree ret_lv,
                 int *variadic_pos)
{
    tree fn_arg;
    ffi_cif cif;
    int n = 0;
    ffi_type **types = calloc(sizeof(*types), ARG_STACK_SZ);
    void **vals = calloc(sizeof(*vals), ARG_STACK_SZ);
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
                push_arg(types, vals, &n,
                         &ffi_type_pointer, &tSTRING_VAL(arg), should_promote);
                break;
            case T_LIVE_VAR:
                switch(TYPE(tLV_TYPE(arg))) {
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM)                     \
                    case TNAME:                                         \
                        push_arg(types, vals, &n,                     \
                                 get_ffi_type(TNAME), &tLV_VAL(arg)->TNAME, should_promote); \
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
                    push_arg(types, vals, &n, &ffi_type_sint, buf, should_promote);
                    break;
                }
            case T_FLOAT:
                {
                    double *buf = alloca(sizeof(double));
                    *buf = mpf_get_d(tFLOAT_VAL(arg));
                    push_arg(types, vals, &n, &ffi_type_double, buf, should_promote);
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
        ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, *variadic_pos, n, ret_type, types);
    else
        ffi_prep_cif(&cif, FFI_DEFAULT_ABI, n, ret_type, types);

    ffi_call(&cif, function_address, ret_lv ? (void *)tLV_VAL(ret_lv) : NULL, vals);

    free(types);
    free(vals);

    enable_gc();
}

static void closure_entry(ffi_cif *cif, void *ret, void **ffi_args, void *user)
{
    tree closure = (tree)user;
    tree arg,
        args = tCLOSURE_ARG_TYPES(closure),
        arg_chain = tree_make(CHAIN_HEAD),
        fn_call = tree_make(T_FN_CALL);
    int n = 0;

    for_each_tree(arg, args) {
        tree new_arg;

        switch (TYPE(arg)){
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM)                     \
            case TNAME:                                                 \
                new_arg = make_live_var(arg);                         \
                tLV_VAL(new_arg)->TNAME = *((STDINTSZ *)(ffi_args[n])); \
                break;
#include "ctypes.def"
#undef DEFCTYPE
        default:
            eval_die(tCLOSURE_FN(closure), "Unknown argument type in closure entry");
        }

        n++;
        tree_chain(new_arg, arg_chain);
    }

    tFNCALL_ID(fn_call) = tCLOSURE_FN(closure);
    tFNCALL_ARGS(fn_call) = arg_chain;

    evaluate(fn_call, "<PTRCALL>");
}

void *get_entry_point_for_fn(tree fndef)
{
    void *ret;
    tree arg, args = tFN_ARGS(fndef),
        ret_type = tFN_RET_TYPE(fndef);
    tree arg_chain = tree_make(CHAIN_HEAD),
        closure = tree_make(T_CLOSURE);
    ffi_type *ffi_ret_type = &ffi_type_void;
    tCLOSURE_CIF(closure) = malloc(sizeof(*tCLOSURE_CIF(closure)));
    tCLOSURE_TYPES(closure) = calloc(sizeof(*tCLOSURE_TYPES(closure)), ARG_STACK_SZ);

    int n = 0;

    for_each_tree(arg, args) {
        tree arg_type;
        tree declarator;

        if (!is_T_DECL(arg))
            eval_die(arg, "Unknown type arg chain, expecting decl");

        arg_type = resolve_decl_specs_to_type(tDECL_SPECS(arg));
        declarator = tDECL_DECLS(arg);

        if (is_T_POINTER(tDECL_DECLS(arg)))
            resolve_ptr_type(&declarator, &arg_type);

        if (!is_T_IDENTIFIER(declarator))
            eval_die(arg, "Unknown declarator type in closure allocation");

        tCLOSURE_TYPES(closure)[n++] = get_ffi_type(TYPE(arg_type));
        tree_chain(arg_type, arg_chain);

        if (n >= ARG_STACK_SZ)
            eval_die(fndef, "Argument stack size exceeded");
    }

    if (!is_D_T_VOID(ret_type))
        ffi_ret_type = get_ffi_type(TYPE(ret_type));

    tCLOSURE_FN(closure) = fndef;
    tCLOSURE_ARG_TYPES(closure) = arg_chain;
    tCLOSURE_RET_TYPE(closure) = ret_type;

    tCLOSURE_CLOSURE(closure) = ffi_closure_alloc(sizeof(*closure), &ret);

    ffi_prep_cif(tCLOSURE_CIF(closure), FFI_DEFAULT_ABI, n, ffi_ret_type,
                 tCLOSURE_TYPES(closure));

    ffi_prep_closure_loc(tCLOSURE_CLOSURE(closure),
                         tCLOSURE_CIF(closure),
                         &closure_entry,
                         (void *)closure,
                         ret);

    return ret;
}
