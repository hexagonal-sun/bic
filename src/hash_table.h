#ifndef __HASH_TABLE_H_
#define __HASH_TABLE_H_
#include "list.h"

typedef struct hash_table *hash_table;

struct hash_table {
    list *table;
    int bits;
};

hash_table hash_table_alloc(int bits);
void hash_table_add(hash_table ht, const char *key, list *element);
list *hash_table_get_bin(hash_table ht, const char *key);

#endif
