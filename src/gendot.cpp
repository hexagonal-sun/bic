#include "lang.h"

static const char * const OUTPUT_FILE = "tree-dump-dot.c";

static void outputPreamble(FILE *f)
{
    fputs("#include \"tree-dump-dot.h\"\n"
          "#include \"tree-dump-primitives.h\"\n"
          "\n"
          "static void __tree_dump_dot(tree t, int depth);\n"
          "\n"
          "static void tree_print_indent(int depth)\n"
          "{\n"
          "    int i;\n"
          "    for (i = 0; i < depth *2; i++)\n"
          "        fprintf(stderr, \" \");\n"
          "}\n"
          "\n"
          "static void __tree_dump_dot_1(tree head, int depth)\n"
          "{\n"
          "    switch(TYPE(head)) {\n", f);
}

static void outputTreeType(FILE *f, const struct TreeType &type)
{
    fprintf(f, "    case %s:\n", type.name.c_str());
    fprintf(f, "        tree_print_indent(depth);\n");
    fprintf(f, "        fprintf(stderr, \"tree_%%p [label=\\\"%s\\\"];\\n\", head);\n", type.name.c_str());

    for (const auto &prop : type.props) {
        std::string propAccessor = type.getPropAccessor(prop.first);
        if (prop.second.baseType.isTree) {
            fprintf(f, "        if (%s(head)) {\n", propAccessor.c_str());
            fprintf(f, "            tree_print_indent(depth);\n");
            fprintf(f, "            if (is_CHAIN_HEAD(%s(head))) {\n", propAccessor.c_str());
            fprintf(f, "                tree first_member = NULL;\n");
            fprintf(f, "                for_each_tree(first_member, %s(head)) break;\n", propAccessor.c_str());
            fprintf(f, "                fprintf(stderr, \"tree_%%p -> tree_%%p [label=\\\"%s\\\", color=red, style=dashed];\\n\", head, first_member);\n",
                    prop.first.c_str());
            fprintf(f, "            } else\n");
            fprintf(f, "                fprintf(stderr, \"tree_%%p -> tree_%%p[label=\\\"%s\\\"];\\n\", head, %s(head));\n",
                    prop.first.c_str(), propAccessor.c_str());
            fprintf(f, "            __tree_dump_dot(%s(head), depth);\n", type.getPropAccessor(prop.first).c_str());
            fprintf(f, "        }\n");
        } else {
            fprintf(f, "        tree_print_indent(depth);\n");
            fprintf(f, "        fprintf(stderr, \"tree_%%p_val [label=\\\"\", head);\n");
            fprintf(f, "        tree_dump_%s(head, %s(head), DOT);\n",
                    prop.second.baseType.name.c_str(), type.getPropAccessor(prop.first).c_str());
            fprintf(f, "        fprintf(stderr, \"\\\"];\\n\");\n");
            fprintf(f, "        tree_print_indent(depth);\n");
            fprintf(f, "        fprintf(stderr, \"tree_%%p -> tree_%%p_val [label=\\\"%s\\\"];\\n\", head, head);\n",
                    prop.first.c_str());
        }
    }

    fprintf(f, "        break;\n");
}

static void outputDumpTypes(FILE *f, const struct lang &lang)
{
    for (const auto &tt : lang.treeTypes)
        outputTreeType(f, tt);

    for (const auto &ct : lang.treeCTypes)
        outputTreeType(f, ct);
}

static void outputEpilogue(FILE *f)
{
    fputs("    }\n"
          "}\n"
          "\n"
          "static void __tree_dump_dot(tree head, int depth)\n"
          "{\n"
          "    tree i, last_chain = NULL;\n"
          "    int subgraph_no = 0;\n"
          "\n"
          "    if (!head) {\n"
          "        return;\n"
          "    }\n"
          "\n"
          "    if (is_CHAIN_HEAD(head)) {\n"
          "        for_each_tree(i, head) {\n"
          "            tree_print_indent(depth);\n"
          "            fprintf(stderr, \"subgraph cluster_%p_%d {\\n\", head, subgraph_no);\n"
          "            __tree_dump_dot_1(i, depth + 1);\n"
          "            tree_print_indent(depth);\n"
          "            fputs(\"}\\n\", stderr);\n"
          "\n"
          "            if (last_chain){\n"
          "                tree_print_indent(depth);\n"
          "                fprintf(stderr, \"tree_%p -> tree_%p [ltail=cluster_%p_%d, lhead=cluster_%p_%d, style=dashed, color=blue];\\n\",\n"
          "                                last_chain, i, head, subgraph_no - 1, head, subgraph_no);\n"
          "            }\n"
          "\n"
          "            last_chain = i;\n"
          "            subgraph_no++;\n"
          "        }\n"
          "    }\n"
          "    else\n"
          "        __tree_dump_dot_1(head, depth);\n"
          "}\n"
          "\n"
          "void tree_dump_dot(tree head)\n"
          "{\n"
          "    fputs(\"digraph tree_dump {\\n\", stderr);\n"
          "    tree_print_indent(1);\n"
          "    fputs(\"compound=true\\n\", stderr);\n"
          "    __tree_dump_dot(head, 1);\n"
          "    fprintf(stderr, \"}\\n\");\n"
          "}\n", f);
}


static void usage(char *progname)
{
    fprintf(stderr, "%s - Generate the `%s' file from a language description\n",
            progname, OUTPUT_FILE);
    fprintf(stderr, "Usage: %s LANG_DESC_FILE\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Where LANG_DESC_FILE is a language description file.\n");
}

int main(int argc, char *argv[])
{
    struct lang lang;
    FILE *f;

    if (argc != 2) {
        fprintf(stderr, "Error: %s expects exactly one argument\n", argv[0]);
        usage(argv[0]);
        return 1;
    }

    f = fopen(argv[1], "r");

    if (!f) {
        perror("Could not open lang file");
        usage(argv[0]);
        exit(2);
    }

    lang_read(f, lang);

    FILE *output = fopen(OUTPUT_FILE, "w");

    fprintf(output,
            "/*\n"
            " * %s - Define the tree_dump_dot function.\n"
            " *\n"
            " * Automatically generated by gendot. DO NOT EDIT.\n"
            " */\n\n",
            OUTPUT_FILE);

    outputPreamble(output);
    outputDumpTypes(output, lang);
    outputEpilogue(output);

    fclose(output);

    return 0;
}
