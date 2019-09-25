#include <stdio.h>

int main()
{
    int a, b, c;

    a = 24;
    b = 45;
    c = -241;

    printf("%d\n", 2 < 3);
    printf("%d\n", 2 < 2);
    printf("%d\n", a < b);
    printf("%d\n", b < a);
    printf("%d\n", c < a);
    printf("%d\n", c < b);
    printf("%d\n", b < c);
    printf("%d\n", a < c);
    printf("%d\n", c < -242);
    printf("%d\n", a < 25);

    return 0;
}
