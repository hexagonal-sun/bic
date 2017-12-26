int printf(char *s, ...);

int main()
{
    int a = 1024;
    int b = 345875;

    printf("%d\n", a >> 2);
    printf("%d\n", a >> 32);
    printf("%d\n", b >> 1);
    printf("%d\n", 2458 >> 3);
    printf("%d\n", b >> 9);
    printf("%d\n", 0xff << 4);
    printf("%d\n", b << 324);
    printf("%d\n", b >> 0);

    return 0;
}
