#include <stdio.h>

int main()
{
    int foo;
    int a;

    foo = 24;

    for (a = 15; a; a--)
        printf("%d\n", a);

    while (foo) {
        printf("%d\n", foo--);
    }

    return 0;
}
