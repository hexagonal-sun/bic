#include <stdio.h>

#include "pretty-printer.h"

static void __pp_1(tree t, int depth);

static void pp_print_indent(int depth)
{
    int i;

    for (i = 0; i < depth; i ++)
        printf("    ");
}

static void pp_live_var(tree t, int depth)
{
    tree type = tLV_TYPE(t);

    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT, FFMEM)        \
        case TNAME:                                     \
            printf("%" #FMT, tLV_VAL(t)->TNAME);     \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        printf("<Unprintable>");
        break;
    }

    /* Handle a special case for char * live variables. */
    if (is_D_T_PTR(type) &&
        is_D_T_CHAR(tDTPTR_EXP(type))) {
        char *str = tLV_VAL(t)->D_T_PTR;

        if (str)
            printf(" (\"%s\")", str);

        return;
    }
}

static void pp_live_compound(tree t, int depth)
{
    tree member;

    printf("{\n");

    for_each_tree(member, tLV_COMP_MEMBERS(t)) {
        tree left = tEMAP_LEFT(member),
            right = tEMAP_RIGHT(member);

        pp_print_indent(depth + 1);
        printf(".%s", tID_STR(left));

        printf(" = ");
        __pp_1(right, depth + 1);
        printf("\n");
    }

    printf("}");
}

static void __pp_1(tree t, int depth)
{
    switch (t->type) {
    case T_INTEGER:
        gmp_printf("%Zd", tINT_VAL(t));
        break;
    case T_FLOAT:
        gmp_printf("%Ff", tFLOAT_VAL(t));
        break;
    case T_STRING:
        printf("\"%s\"", tSTRING_VAL(t));
        break;
    case T_IDENTIFIER:
        printf("%s", tID_STR(t));
        break;
    case T_LIVE_VAR:
        pp_live_var(t, depth);
        break;
    case T_LIVE_COMPOUND:
        pp_live_compound(t, depth);
        break;
    default:
        break;
    }
}

static void __pp(tree head, int depth)
{
    tree i;

    if (!head)
        return;

    if (is_CHAIN_HEAD(head)) {
        pp_print_indent(depth);

        for_each_tree(i, head)
            __pp_1(i, depth+1);

    }
    else
        __pp_1(head, depth);
}

void pretty_print(tree head)
{
    __pp(head, 0);
}
