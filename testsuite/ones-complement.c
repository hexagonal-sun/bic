#include <stdio.h>

int main()
{
    int a = 0;
    int b = 24252;
    unsigned int c = 0xdeadbeef;
    int d = ~b;

    printf("%d\n", ~a);
    printf("%d\n", ~b);
    printf("%d\n", ~c);
    printf("%d\n", d);
}
