int printf(char *fmt, ...);

int foobar(int a)
{
    printf("%d\n", a);
}

int main()
{
    foobar(20);

    return 0;
}
