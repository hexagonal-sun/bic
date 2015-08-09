#ifndef __IDENTIFIER_H_
#define __IDENTIFIER_H_
#include "hash_table.h"

typedef struct identifier identifier;

struct identifier {
    const char *name;
    list hash_table_entry;
};

void identifier_init(void);
identifier *get_identifier(const char *name);

#endif
