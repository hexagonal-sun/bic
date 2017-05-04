int printf(char *s, ...);
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

int main ()
{
    ftype *func;

    a = 10;
    b = 20;
    printf("%d %d\n", a, b);

    func = &doAdd;
    func(a, b);
    printf("%d %d\n", a, b);

    func = &doSub;
    func(a, b);

    printf("%d %d\n", a, b);
}
