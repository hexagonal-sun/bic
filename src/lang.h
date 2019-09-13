#include <vector>
#include <unordered_map>
#include <string>

extern std::string lexval;

// This represents a base type that is declared in the language file.
struct BaseType {
    std::string type;
    std::string name;
    bool isTree;
};

// This represents an 'instantiated' base type.
struct InstantiatedType
{
    std::string memberName;
    struct BaseType baseType;
};

typedef std::unordered_map<std::string, const struct InstantiatedType> typeMap_t;

struct TreeType {
    std::string name;
    std::string friendly_name;
    typeMap_t props;
};

struct CType : public TreeType {
    std::string ctype;
    std::string format_string;
    std::string ff_union_member_name;
};

struct TypeAllocator;

struct TypePool {
    explicit TypePool(struct BaseType bt) :
        baseType_(bt) {};
    void emitDeclarations(FILE *f) const;
    struct TypeAllocator getAllocator(void);
private:
    friend struct TypeAllocator;
    struct InstantiatedType alloc(void);
    struct BaseType baseType_;
    std::vector<struct InstantiatedType> pool_;
};

struct TypeAllocator {
    TypeAllocator(struct TypePool &pool);
    struct InstantiatedType alloc(void);
private:
    struct TypePool &typePool_;
    std::vector<struct InstantiatedType> currentPool_;
};

struct lang {
    std::vector<struct TreeType> treeTypes;
    std::vector<struct CType> treeCTypes;
    std::unordered_map<std::string, struct TypePool> baseTypePools;
    struct TypePool treePool;
    size_t trees_allocated;
    lang() : trees_allocated (0), treePool({"tree", "t", true}) {};
};

enum token_modules
{
    DEFTYPE,
    DEFCTYPE,
    DEFBASETYPES,
    IDENTIFIER,
    STRING
};

struct locus
{
    int line;
    int col;
};

void lang_read(FILE *f, struct lang &lang);
