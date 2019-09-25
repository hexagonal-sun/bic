#include <stdio.h>

int a = 2424;

void foofunc()
{
    int i;
    static int a = 10, b = 20;

    for (i = 0; i < 5; i++)
    {
        static int a = 4424;
        a++;

        printf("%d\n", a);
    }

    b--;

    printf("%d\n", a++);
    printf("%d\n", b);
}

int main ()
{
    foofunc();
    foofunc();
    foofunc();

    printf("%d\n", a);

    return 0;
}
