#include <stdio.h>

int foobar;

void set_ptr_to_foobar(int **pptr)
{
    *pptr = &foobar;
}

int main()
{
    int a;
    int *iptr;

    a = 10;
    foobar = 20;

    printf("%d\n", a);
    printf("%d\n", foobar);

    iptr = &a;
    *iptr = 2020;

    printf("%d\n", a);
    printf("%d\n", foobar);

    set_ptr_to_foobar(&iptr);
    *iptr = 2020;

    printf("%d\n", a);
    printf("%d\n", foobar);


    return 0;
}
