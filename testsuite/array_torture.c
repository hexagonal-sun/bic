#include <stdio.h>

typedef long int arrtype[8];

struct foo {
    int a;
    int b;
    int bazza[15];
};

void print_foo(struct foo *p)
{
    int i;

    for (i = 0; i < 15; i++)
        printf("%d %d %d\n", p->a, p->b, p->bazza[i]);
}

int main()
{
    int i;
    long int double_array[20];
    struct foo ff[20];
    arrtype bb;

    bb[0] = 1024;
    printf("%ld\n", sizeof(bb));
    printf("%ld\n", sizeof(arrtype));

    for (i = 0; i < 20; i++) {
        int j;
        double_array[i] = i * 10;

        ff[i].a = i;
        ff[i].b = i + 1;

        for (j = 0; j < 15; j++) {
            ff[i].bazza[j] = j + i;
        }
    }

    for (i = 0; i < 20; i++)
        printf("%ld\n", double_array[i]);

    for (i = 0; i < 20; i++)
        print_foo(&ff[i]);

    return 0;
}
