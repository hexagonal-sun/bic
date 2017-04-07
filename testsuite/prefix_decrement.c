int printf(char *fmt, ...);

int foobar(int a)
{
    printf("%d\n", a);
}

int main()
{
    int a = 10;
    foobar(a);
    foobar(--a);
    foobar(a);
}
