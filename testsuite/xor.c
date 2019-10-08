#include <stdio.h>

int main()
{
    int a = 0xdeadbeef;

    printf("%X\n", a ^ 0xd00dfeed);
    printf("%X\n", a ^ 0);
    printf("%X\n", a ^ 1);
    printf("%X\n", a ^ a);

    return 0;
}
