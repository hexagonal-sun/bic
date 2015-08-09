#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

hash_table hash_table_alloc(int bits)
{
    int i;

    hash_table ret = calloc(sizeof(*ret), 1);
    ret->bits = bits;

    ret->table = calloc(sizeof(*(ret->table)), 1 << bits);
    for (i = 0; i < 1 << bits; i++)
        INIT_LIST(&(ret->table[i]));

    return ret;
}


/* Taken from: http://www.azillionmonkeys.com/qed/hash.html. This is
 * licensed under the LGPL v2.1 and according to
 * https://www.gnu.org/licenses/gpl-faq.html#AllCompatibility this
 * code can be relicensed under the GPLv2 (the license of this
 * project).
 */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)    \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif
#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)    \
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t super_fast_hash (const char * data)
{
    uint32_t len = strlen(data);
    uint32_t hash = len, tmp;
    int rem;
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3: hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
        hash += hash >> 11;
        break;
    case 2: hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += (signed char)*data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

void hash_table_add(hash_table ht, const char *key, list *element)
{
    int bin = super_fast_hash(key) & ((1 << ht->bits) - 1);
    list_add(element, &(ht->table[bin]));
}

list *hash_table_get_bin(hash_table ht, const char *key)
{
    int bin = super_fast_hash(key) & ((1 << ht->bits) - 1);
    return &(ht->table[bin]);
}
