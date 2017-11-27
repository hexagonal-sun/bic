#include "util.h"
#include <stdlib.h>
#include <string.h>

char *concat_strings(const char *s1, const char *s2)
{
    char *ret = malloc(strlen(s1) + strlen(s2) + 1);

    ret[0] = '\0';

    strcat(ret, s1);
    strcat(ret, s2);

    return ret;
}
