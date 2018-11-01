#include <vector>
#include <string>

extern std::string lexval;

struct dtype {
    std::string name;
    std::string friendly_name;
};

struct ctype {
    std::string name;
    std::string friendly_name;
    std::string ctype;
    std::string format_string;
};

struct struct_member {
    std::string name;
    std::string type;
};

struct sstruct {
    std::vector<struct_member> members;
    std::string name;
};

struct lang {
    std::vector<struct dtype> treeTypes;
    std::vector<struct ctype> treeCTypes;
    std::vector<struct sstruct> treeStructs;
    std::vector<struct struct_member> treeData;
};

enum token_modules
{
    DEFSTRUCT,
    DEFTYPE,
    DEFCTYPE,
    DEFDATA,
    IDENTIFIER,
    STRING
};

void lang_read(FILE *f, struct lang &lang);
