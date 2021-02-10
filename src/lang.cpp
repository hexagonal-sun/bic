#include <stdio.h>
#include <iostream>
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

void TypePool::emitDeclarations(FILE *f) const
{
    for (const auto &instantypatedType : pool_)
        fprintf(f, "    %s %s;\n", instantypatedType.baseType.type.c_str(),
                instantypatedType.memberName.c_str());
}

struct InstantiatedType TypePool::alloc(void)
{
    struct InstantiatedType ret;

    ret.baseType = baseType_;
    ret.memberName = baseType_.name + std::to_string(pool_.size());

    pool_.push_back(ret);

    return ret;
}

struct TypeAllocator TypePool::getAllocator(void)
{
    return TypeAllocator(*this);
}

TypeAllocator::TypeAllocator(struct TypePool &typePool)
    : typePool_(typePool), currentPool_(typePool_.pool_)
{
}

struct InstantiatedType TypeAllocator::alloc(void)
{
    if (currentPool_.empty())
        return typePool_.alloc();

    auto element = currentPool_.front();
    currentPool_.erase(currentPool_.begin());

    return element;
}

static void handle_defbasetype(struct lang &lang)
{
    while (1)
    {
        int token;

        struct BaseType baseType;
        token = yylex();

        if (token == ')')
            break;

        if (token != '(')
            perror("Expected '('");

        token = yylex();
        if (token != IDENTIFIER)
            perror("Expected IDENTIFIER");
        baseType.name = lexval;

        token = yylex();
        if (token != STRING)
            perror("Expected STRING");
        baseType.type = lexval;

        baseType.isTree = false;

        token = yylex();
        if (token != ')')
            perror("Expected ')'");

        lang.baseTypePools.insert({baseType.name, TypePool(baseType)});
    }
}

static struct BaseType getDataBaseType()
{
    int token = yylex();
    struct BaseType ret;

    if (token != STRING)
        perror("Expected STRING");
    ret.name = lexval;

    token = yylex();
    if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");
    ret.type = lexval;

    token = yylex();
    if (token != ')')
        perror("Expected ')'");

    return ret;
}

static void parsePropList(struct lang &lang,
                          struct TreeType &tree,
                          int token)
{
    bool haveProperties = true;
    std::string propNamePrefix;
    std::unordered_map<std::string, struct TypeAllocator> typeAllocators;
    struct TypeAllocator treeAllocator = lang.treePool.getAllocator();

    for (auto& i : lang.baseTypePools)
        typeAllocators.insert({i.first, i.second.getAllocator()});

    if (token != STRING)
        perror("Expected STRING or ')'");
    tree.propPrefix = lexval;

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
        {
            auto baseType = getDataBaseType();
            auto &allocator = typeAllocators.at(baseType.type);
            tree.props.insert({propNamePrefix + baseType.name, allocator.alloc()});
            break;
        }
        case STRING:
            tree.props.insert({lexval, treeAllocator.alloc()});
            break;
        default:
            perror("Expected '(' ')' or STRING");
        }
    }
}

static void handle_deftype(struct lang &lang)
{
    struct TreeType newType;
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
    if (token != STRING)
        perror("Expected STRING");
    newCType.ff_union_member_name = lexval;

    token = yylex();
    if (token != IDENTIFIER)
      perror("Expected IDENTIFIER");
    newCType.ffi_type = lexval;

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

static std::vector<std::string> handle_specifier_list()
{
    std::vector<std::string> ret;

    while (1)
    {
        int token = yylex();

        if (token == ')')
            break;

        if (token == IDENTIFIER)
            ret.push_back(lexval);
        else
            perror("Expected ')' or IDENTIFIER");
    }

    return ret;
}

static void handle_defspecifier(struct lang &lang)
{
    int token = yylex();
    struct SpecifierMap specifierMap;

    switch (token) {
    case IDENTIFIER:
    {
        specifierMap.returnTreeTypeName = lexval;

        token = yylex();
        if (token != '(')
          perror("Expected '('");

        while (1) {
          token = yylex();

          if (token == ')')
            break;

          if (token == '(')
            specifierMap.specifiers.push_back(handle_specifier_list());
          else
            perror("Expected ')' or '('");
        }

        lang.specMaps.push_back(specifierMap);
        break;
    }
    case IGNORE:
      token = yylex();
      if (token != IDENTIFIER)
        perror("Expected IDENTIFIER");

      lang.ignoredSpecifiers.push_back(lexval);
      break;
    case SELF:
        token = yylex();
        if (token != IDENTIFIER)
            perror("Expected IDENTIFIER");

        lang.selfSpecifiers.push_back(lexval);
        break;
    default:
      perror("Expected IDENTIFIER or IGNORE");
    }

    token = yylex();
    if (token != ')')
        perror("Expected ')'");
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
        case DEFBASETYPES:
            handle_defbasetype(lang);
            break;
        case DEFSPECIFIER:
            handle_defspecifier(lang);
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
