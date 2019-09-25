#include <stdio.h>

int main()
{
    int a, b, c, d, e;

    a = 24;
    b = 45;
    c = -241;
    d = 24;
    e = -24;

    printf("%d\n", 2 == 3);
    printf("%d\n", 2 == 2);
    printf("%d\n", 2 != 3);
    printf("%d\n", 2 != 2);
    printf("%d\n", a == b);
    printf("%d\n", a == a);
    printf("%d\n", a == 24);
    printf("%d\n", a == 23);
    printf("%d\n", a == 25);
    printf("%d\n", b != a);
    printf("%d\n", c == a);
    printf("%d\n", a == d);
    printf("%d\n", a == e);
    printf("%d\n", a != d);
    printf("%d\n", a != e);

    return 0;
}
