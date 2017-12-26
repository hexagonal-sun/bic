int printf(char *fmt, ...);

int main()
{
    struct foobar *a;

    printf("%d\n", sizeof(a));

    return 0;
}
