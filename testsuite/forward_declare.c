#include <stdio.h>
#include <stdlib.h>

struct foobar;

typedef struct foobar foostruct;

struct baz {
    struct foobar *fab;
};

struct a {
    foostruct *fooelm;
};

void other()
{
    struct foobar {
        float d;
    } ff;

    ff.d = 20;

    printf("%f\n", ff.d);
}

struct foobar {
    int a;
};

int main()
{
    struct baz bag;
    struct a bat;

    bag.fab = malloc(sizeof(struct foobar));
    bat.fooelm = malloc(sizeof(foostruct));

    other();

    bag.fab->a = 10;
    bat.fooelm->a = 1024;

    printf("%d\n", bag.fab->a);
    printf("%d\n", bat.fooelm->a);

    free(bag.fab);

    return 0;
}
