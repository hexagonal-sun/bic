#include <stdio.h>

struct foobar {
    int a;
    int b;
    void (*foo)(int ba);
};

int main()
{
    struct foobar baz;
    baz.a = 10;
    baz.b = 1249;
    printf("%d\n", baz.a);
    printf("%d\n", baz.b);

    return 0;
}
