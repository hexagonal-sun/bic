typedef unsigned int footype;
typedef unsigned int * intptr;

#include <stdio.h>

void foobar(footype a)
{
    printf("%d\n", a);
}

int main()
{
    footype a = 10;
    intptr p = &a;
    foobar(a);
    foobar(*p);

    return 0;
}
