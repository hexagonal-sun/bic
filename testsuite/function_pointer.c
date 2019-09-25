#include <stdio.h>
int a;
int b;

typedef void ftype(int first, int second);

void doAdd(int ia, int ib)
{
    a = ia + ib;
    b = a + ib;
}

void doSub(int ia, int ib)
{
    a = ia - ib;
    b = b - ia;
}

void add_ptr_func(int *a)
{
    (*a)++;
}

void sub_ptr_func(int *b)
{
    (*b)--;
}

int main ()
{
    ftype *func;

    void (* fptr)(int *p);

    a = 10;
    b = 20;
    fptr = &add_ptr_func;
    fptr(&a);
    fptr(&b);
    printf("%d %d\n", a, b);

    func = &doAdd;
    func(a, b);
    fptr(&a);
    fptr(&a);
    printf("%d %d\n", a, b);

    func = &doSub;
    fptr = &sub_ptr_func;
    fptr(&a);
    fptr(&b);
    func(a, b);

    printf("%d %d\n", a, b);

    return 0;
}
