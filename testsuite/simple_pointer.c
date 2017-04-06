int printf(char *s, int d, int e);

int main()
{
    int a, b;

    int *foo;
    foo = &a;
    *foo = 20;
    foo = &b;
    *foo = 30;
    printf("%d %d\n", a, b);
}
