#include <vector>
#include <unordered_map>
#include <string>

extern std::string lexval;

struct tree_data {
    std::string type;
    std::string name;
};

struct treeType {
    std::string name;
    std::string friendly_name;
    std::unordered_map<std::string, const struct tree_data> props;
};

struct CType : public treeType {
    std::string ctype;
    std::string format_string;
};

struct lang {
    std::vector<struct treeType> treeTypes;
    std::vector<struct CType> treeCTypes;
    std::vector<struct tree_data> treeData;
    size_t trees_allocated;
    lang() : trees_allocated (0) { };
};

enum token_modules
{
    DEFTYPE,
    DEFCTYPE,
    DEFDATA,
    IDENTIFIER,
    STRING
};

struct locus
{
    int line;
    int col;
};

void lang_read(FILE *f, struct lang &lang);
