#include "util.h"
#include <gc.h>
#include <string.h>

char *concat_strings(const char *s1, const char *s2)
{
    char *ret = GC_MALLOC(strlen(s1) + strlen(s2));

    ret[0] = '\0';

    strcat(ret, s1);
    strcat(ret, s2);

    return ret;
}
