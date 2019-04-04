void printf(const char *s, ...);

int main()
{
    int a[20];
    int *b;
    int *c;
    int *d;

    b = &a[0];
    c = a;
    d = &a;

    printf("%d\n", b == c);
    printf("%d\n", c == d);
}
