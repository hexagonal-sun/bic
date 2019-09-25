#include <stdio.h>

int main()
{
    unsigned int a = 1024;
    unsigned int b = 345875;

    printf("%u\n", a & 2);
    printf("%u\n", a & 31);
    printf("%u\n", b & 1);
    printf("%u\n", 2458 & 3);
    printf("%u\n", b & 9);
    printf("%u\n", 0xff & 4);
    printf("%u\n", b & 31);
    printf("%u\n", b & 0);

    return 0;
}
