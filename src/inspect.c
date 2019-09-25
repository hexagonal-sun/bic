#include "inspect.h"
#include "evaluate.h"
#include "pretty-printer.h"

static void print_object_value(tree inspect_id,
                               tree resolved_object)
{
    printf("value of %s is ", tID_STR(inspect_id));

    pretty_print(resolved_object);

    printf("\n");
}

static void print_sizeof_object(tree inspect_id,
                                tree resolved_object)
{
    tree sizeof_obj = tree_make(T_SIZEOF),
        sizeof_result;

    tSZOF_EXP(sizeof_obj) = resolved_object;

    sizeof_result = evaluate(sizeof_obj, "<inspector>");

    printf("sizeof(%s) = ", tID_STR(inspect_id));
    pretty_print(sizeof_result);
    printf(" bytes.\n");
}

static tree handle_pointer_type(tree ptr_type)
{
    if (!is_D_T_PTR(ptr_type))
        return ptr_type;

    printf("%s to a ", tree_description_string(ptr_type));

    return handle_pointer_type(tDTPTR_EXP(ptr_type));
}

static void print_object_type(tree inspect_id, tree resolved_object)
{
    tree object_type = tLV_TYPE(resolved_object),
        sizeof_obj = tree_make(T_SIZEOF);
    const char *object_name;

    if (!is_T_IDENTIFIER(inspect_id))
        return;

    printf("%s is a ", tID_STR(inspect_id));

    object_type = handle_pointer_type(object_type);

    if (is_T_DECL_COMPOUND(object_type))
        object_name = tID_STR(tCOMP_DECL_ID(object_type));
    else
        object_name = tree_description_string(object_type);

    tSZOF_EXP(sizeof_obj) = inspect_id;
    printf("%s.\n", object_name);
}

static void print_object_locus(tree inspect_id, tree resolved_object)
{
    printf("%s was declared at: %s:%d\n", tID_STR(inspect_id),
           tID_STR(resolved_object->locus.file),
           resolved_object->locus.line_no);
}

void inspect(tree inspect_id, tree resolved_object)
{
    print_object_type(inspect_id, resolved_object);
    print_object_value(inspect_id, resolved_object);
    print_sizeof_object(inspect_id, resolved_object);
    print_object_locus(inspect_id, resolved_object);
}
