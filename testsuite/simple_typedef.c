typedef unsigned int footype;
typedef int * intptr;

int printf(char *fmt, ...);

int foobar(footype a)
{
    printf("%d\n", a);
}

int main()
{
    footype a = 10;
    intptr p = &a;
    foobar(a);
    foobar(*p);

    return 0;
}
