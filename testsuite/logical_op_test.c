#include <stdio.h>

int one()
{
    printf("One called\n");
    return 1;
}

int zero()
{
    printf("Zero called\n");
    return 0;
}

int main()
{
    printf("%d\n", 0 && 0);
    printf("%d\n", 0 && 1);
    printf("%d\n", 1 && 0);
    printf("%d\n", 1 && 1);

    printf("%d\n", 0 || 0);
    printf("%d\n", 0 || 1);
    printf("%d\n", 1 || 0);
    printf("%d\n", 1 || 1);

    printf("%d\n", zero() && zero());
    printf("%d\n", zero() && one());
    printf("%d\n", one() && zero());
    printf("%d\n", one() && one());

    printf("%d\n", zero() || zero());
    printf("%d\n", zero() || one());
    printf("%d\n", one() || zero());
    printf("%d\n", one() || one());

    return 0;
}
