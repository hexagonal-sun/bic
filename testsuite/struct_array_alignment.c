#include <stdio.h>

typedef struct a
{
    short m0;
    char m1[4];
}a;

int main()
{
    a b;
    void *base;
    void *member;
    base = &b;
    member = &b.m1;
    printf("%d\n", member - base);

    return 0;
}
