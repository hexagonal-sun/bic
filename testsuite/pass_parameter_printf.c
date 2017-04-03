int printf(char *fmt, int n);

int foobar(int a)
{
    printf("%d\n", a);
}

int main()
{
    foobar(20);
}
