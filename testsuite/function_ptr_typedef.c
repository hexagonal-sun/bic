#include <stdio.h>

typedef void* ( *foobar)(int a);

typedef struct m{
    foobar baz;
} tan;

int main()
{
    printf("%d\n", sizeof(tan));

    return 0;
}
