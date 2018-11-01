#include <stdio.h>
#include <stdlib.h>
#include "lang_lexer.h"
#include "lang.h"

#define perror(STR) {fprintf(stderr, "Parse error in %s: " STR "\n", __func__); exit(1);}

std::string lexval;

int yywrap(void)
{
    return 1;
}

static void getStructMembers(std::vector<struct struct_member> &members)
{
    while (1)
    {
        int token;

        struct struct_member member;
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

        members.push_back(member);
    }
}

static void handle_defdata(struct lang &lang)
{
    getStructMembers(lang.treeData);
}

static void handle_defstruct(struct lang &lang)
{
    struct sstruct newStruct;
    int token = yylex();

    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    newStruct.name = lexval;

    getStructMembers(newStruct.members);

    lang.treeStructs.push_back(newStruct);
}

static void handle_deftype(struct lang &lang)
{
    struct dtype newType;
    int token = yylex();

    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    newType.name = lexval;

    token = yylex();
    if (token != STRING)
        perror("Expected STRING");
    newType.friendly_name = lexval;

    token = yylex();
    if (token != ')')
        perror("Expected ')'");

    lang.treeTypes.push_back(newType);
}

static void handle_defctype(struct lang &lang)
{
    struct ctype newCType;
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
    if (token != ')')
        perror("Expected ')'");

    lang.treeCTypes.push_back(newCType);
}

static void lang_parse(struct lang &lang)
{
    while (1) {
        int token = yylex();

        if (!token)
            return;

        if (token != '(')
            perror("Expected '('");

        token = yylex();

        switch (token)
        {
        case DEFSTRUCT:
            handle_defstruct(lang);
            break;
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
