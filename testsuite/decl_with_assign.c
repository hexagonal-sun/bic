int printf(char *fmt, int n);

int foobar(int b)
{
    printf("%d\n", b);
}

int main()
{
    int a = 10;
    foobar(a);
}
