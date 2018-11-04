#include <stdio.h>
#include <stdlib.h>
#include "lang_lexer.h"
#include "lang.h"

#define perror(STR)                                                    \
    {                                                                  \
        fprintf(stderr, "%d:%d: Parse error in %s: " STR "\n",         \
                token_loc.line, token_loc.col, __func__);              \
        exit(1);                                                       \
    }

std::string lexval;

extern struct locus token_loc;

int yywrap(void)
{
    return 1;
}

static void handle_defdata(struct lang &lang)
{
    while (1)
    {
        int token;

        struct tree_data member;
        token = yylex();

        if (token == ')')
            break;

        if (token != '(')
            perror("Expected '('");

        token = yylex();
        if (token != STRING)
            perror("Expected STRING");
        member.type = lexval;

        token = yylex();
        if (token != IDENTIFIER)
            perror("Expected IDENTIFIER");
        member.name = lexval;

        token = yylex();
        if (token != ')')
            perror("Expected ')'");

        lang.treeData.push_back(member);
    }
}


static void genTreeProp(std::string pName, struct treeType &newType,
                        std::vector<struct tree_data> &dataPool,
                        struct lang &lang)
{
    for (auto i = dataPool.begin(); i != dataPool.end(); ++i)
        if ((*i).type == "tree")
        {
            newType.props.insert({pName, *i});
            dataPool.erase(i);
            return;
        }

    struct tree_data newTreeData;
    newTreeData.type = "tree";
    newTreeData.name = "exp" + std::to_string(lang.trees_allocated++);

    lang.treeData.push_back(newTreeData);
    newType.props.insert({pName, newTreeData});
}

static void genDataProp(std::string propNamePrefix, struct treeType &newType,
                        std::vector<struct tree_data> &dataPool,
                        struct lang &lang)
{
    std::string propName;
    std::string dataMemberName;
    int token = yylex();

    if (token != STRING)
        perror("Expected STRING");
    propName = propNamePrefix + lexval;

    token = yylex();
    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    dataMemberName = lexval;

    token = yylex();
    if (token != ')')
        perror("Expected ')'");

    struct tree_data newTreeData;

    for (auto i = dataPool.begin(); i != dataPool.end(); i++)
    {
        if ((*i).name == dataMemberName) {
            newTreeData = *i;
            newType.props.insert({propName, newTreeData});
            return;
        }
    }

    fprintf(stderr, "Error: could not data member name `%s' for `%s' "
            "property of `%s' tree type.\n", dataMemberName.c_str(),
            propName.c_str(), newType.name.c_str());

    exit(1);
}

static void parsePropList(struct lang &lang,
                          struct treeType &tree,
                          int token)
{
    bool haveProperties = true;
    std::vector<struct tree_data> treeDataPool = lang.treeData;
    std::string propNamePrefix;

    if (token != STRING)
        perror("Expected STRING or ')'");
    propNamePrefix = lexval + "_";

    token = yylex();
    if (token != '(')
        perror("Expected '('");

    while (haveProperties)
    {
        token = yylex();
        switch (token)
        {
        case ')':
            haveProperties = false;
            break;
        case '(':
            genDataProp(propNamePrefix, tree, treeDataPool, lang);
            break;
        case STRING:
            genTreeProp(propNamePrefix + lexval, tree,
                        treeDataPool, lang);
            break;
        default:
            perror("Expected '(' ')' or STRING");
        }
    }
}

static void handle_deftype(struct lang &lang)
{
    struct treeType newType;
    int token = yylex();

    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    newType.name = lexval;

    token = yylex();
    if (token != STRING)
        perror("Expected STRING");
    newType.friendly_name = lexval;

    token = yylex();
    if (token == ')')
        goto out;

    parsePropList(lang, newType, token);

    token = yylex();
    if (token != ')')
        perror("Expected ')'");
out:
    lang.treeTypes.push_back(newType);
}

static void handle_defctype(struct lang &lang)
{
    struct CType newCType;
    int token = yylex();

    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    newCType.name = lexval;

    token = yylex();
    if (token != STRING)
        perror("Expected STRING");
    newCType.friendly_name = lexval;

    token = yylex();
    if (token != STRING)
        perror("Expected STRING");
    newCType.ctype = lexval;

    token = yylex();
    if (token != STRING)
        perror("Expected STRING");
    newCType.format_string = lexval;

    token = yylex();
    if (token == ')')
        goto out;

    parsePropList(lang, newCType, token);

    token = yylex();
    if (token != ')')
        perror("Expected ')'");
out:
    lang.treeCTypes.push_back(newCType);
}

static void lang_parse(struct lang &lang)
{
    token_loc.line = 1;
    token_loc.col = 1;

    while (1) {
        int token = yylex();

        if (!token)
            return;

        if (token != '(')
            perror("Expected '('");

        token = yylex();

        switch (token)
        {
        case DEFTYPE:
            handle_deftype(lang);
            break;
        case DEFCTYPE:
            handle_defctype(lang);
            break;
        case DEFDATA:
            handle_defdata(lang);
            break;
        default:
            perror("Unexpected toplevel construct");
        }
    }
}

void lang_read(FILE *f, struct lang &lang)
{
    yyin = f;

    lang_parse(lang);
}
