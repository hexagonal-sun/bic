#include <stdio.h>

void foobar(int b)
{
    printf("%d\n", b);
}

int main()
{
    int a = 10;
    foobar(a);

    return 0;
}
