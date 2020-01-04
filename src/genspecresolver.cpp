#include <stdio.h>
#include "lang.h"
#include <map>
#include <vector>
#include <iostream>

static std::map<std::string, int> noBitsPerSpecifier;

struct SanitisiedSpecifier {
    std::string name;
    int bitPos;
};

static void outputPreamble(FILE *output)
{
    fputs("#include \"spec-resolver.h\"\n"
          "#include \"evaluate.h\"\n"
          "\n"
          "tree resolve_decl_specs_to_type(tree decl_specs)\n"
          "{\n"
          "    tree i;\n"
          "    unsigned int spec_bitstring = 0;\n"
          "\n", output);
}

static std::map<std::string, int> outputBitStirngSetLoop(const lang &lang, FILE *output)
{
    fputs("    for_each_tree(i, decl_specs) {\n"
          "        switch (TYPE(i))\n"
          "        {\n", output);

    // Allocate bits in spec_bitstring to each type specifier; first count them.
    for (auto specMap : lang.specMaps) {
        for (auto specifierList : specMap.specifiers) {
            // Count the number of times each specifier is used in this list.
            std::map<std::string, int> specifierCount;
            for (auto specifier : specifierList)
                specifierCount[specifier]++;

            // Now update the global noBitsPerSpecifier.
            for (auto specCount : specifierCount)
                noBitsPerSpecifier[specCount.first] =
                    std::max(noBitsPerSpecifier[specCount.first], specCount.second);
        }
    }

    // Now allocate bit positions, outputting code to set the appropriate bit.
    int currentBitPosition = 0;
    std::map<std::string, int> bitAllocations;

    for (auto count : noBitsPerSpecifier) {
      fprintf(output, "        case %s:\n", count.first.c_str());
      bitAllocations[count.first] = currentBitPosition;
      if (count.second > 1) {
        for (int i = 0; i < count.second; i++) {
          fprintf(output, "            if (!(spec_bitstring & (1 << %d))) {\n", currentBitPosition);
          fprintf(output, "                spec_bitstring |= (1 << %d);\n", currentBitPosition);
          fprintf(output, "                break;\n");
          fprintf(output, "            }\n");
          currentBitPosition++;
        }
        fprintf(output, "            eval_die(i, \"too many `%%s`s in declaration specifier\\n\", tree_description_string(i));\n");
      } else {
          fprintf(output, "            if (spec_bitstring & (1 << %d))\n", currentBitPosition);
          fprintf(output, "                eval_die(i, \"duplicate `%%s` in declaration specifier\\n\", tree_description_string(i));\n");
          fprintf(output, "            spec_bitstring |= (1 << %d);\n",
                  currentBitPosition);
          fprintf(output, "            break;\n");
          currentBitPosition += count.second;
      }
    }

    // Output any self specifiers.
    if (!lang.selfSpecifiers.empty()) {
        for (auto selfSpecifier : lang.selfSpecifiers ) {
            fprintf(output, "        case %s:\n", selfSpecifier.c_str());
        }
        fputs("            return i;\n", output);
    }

    // Output any ignored specifiers.
    if (!lang.ignoredSpecifiers.empty()) {
        fputs("        /* Ignore these specifiers as they don't have any bearing on type information. */\n", output);
        for (auto ignoredSpec : lang.ignoredSpecifiers)
            fprintf(output, "        case %s:\n", ignoredSpec.c_str());
        fputs("            break;\n", output);
    }

    fputs("        default:\n"
          "            eval_die(i, \"unknown type specifier found in specifier list: %s\\n\", tree_type_string(TYPE(i)));\n"
          "        }\n"
          "    }\n"
          "\n", output);

    return bitAllocations;
}

static void outputTypeReturn(const lang &lang, FILE *output,
                             std::map<std::string, int> bitAllocs)
{
    fputs("    switch(spec_bitstring)\n"
          "    {\n", output);

    for (auto specMap : lang.specMaps) {
        for (auto specifierList : specMap.specifiers) {
            std::map<std::string, int> specCount;

            bool isFirst = true;
            fprintf(output, "        case ");
            for (auto specifier : specifierList) {
                fprintf(output, "%s(1 << %d)", isFirst ? "" : " | ",
                        bitAllocs[specifier] + specCount[specifier]);
                specCount[specifier]++;
                isFirst = false;
            }
            fprintf(output, ":\n");
        }
        fprintf(output, "            return tree_make(%s);\n",
                specMap.returnTreeTypeName.c_str());
    }

    fputs("    default:\n"
          "        eval_die(decl_specs, \"Unknown combination of declaration specifiers\\n\");\n"
          "    }\n", output);
}

static void outputSpecResolver(const lang &lang, FILE *output)
{
    auto bitPositions = outputBitStirngSetLoop(lang, output);
    outputTypeReturn(lang, output, bitPositions);
}

static void outputEpilogue(FILE *output)
{
    fputs("}\n", output);
}

static void usage(char *progname)
{
    fprintf(stderr, "%s - Generate the `spec-resolver.c' file\n", 
            progname);
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

    FILE *output = fopen("spec-resolver.c", "w");

    fputs("/*\n"
          " * spec-resolver.c - Define the resolve_decl_specs_to_type function.\n"
          " *\n"
          " * Automatically generated by genspecresolver. DO NOT EDIT.\n"
          " */\n\n", output);

    outputPreamble(output);
    outputSpecResolver(lang, output);
    outputEpilogue(output);

    fclose(output);

    return 0;
}
