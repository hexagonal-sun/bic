#include <stdio.h>

int main()
{
    int a[20];
    int *b;
    int *c;
    int *d;

    b = &a[0];
    c = a;
    d = (int *)&a;

    printf("%d\n", b == c);
    printf("%d\n", c == d);

    return 0;
}
