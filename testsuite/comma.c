#include <stdio.h>

int main()
{
    int a, b, i, z;

    a = 20;
    b = 30;

    i = (a++, b + 4);

    z = a--, b + 20;

    printf("%d %d %d %d\n", a, b, i, z);

    return 0;
}
