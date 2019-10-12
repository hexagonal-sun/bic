#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int a, b;
} s1;

int main()
{
    int i;
    for (i = 0; i < 3000; i++) {
        struct foobar {
            int a;
            int b;
            void *bacl;
            char *str, ch;
            s1 pan;
        };

        struct funcptr
        {
            int * (*funcptr)(void *arg);
            int arg;
        };

        struct {
            void *m1;
            struct {
                int foo;
                int baz;
            } m2;
        } news;

        struct {
            int allocated;
            s1 *ps;
        } news2;

        struct foobar fab;

        fab.a = 10;
        fab.pan.a = 1024;

        news.m1 = 0;
        news.m2.foo = 10;
        news.m2.baz = 24242;

        news2.ps = malloc(sizeof(s1));
        news2.allocated = 1;
        news2.ps->a = 10;
        news2.ps->b = 3434;

        printf("%d %d\n", fab.a, fab.pan.a);
        printf("%p %d %d\n", news.m1, news.m2.foo, news.m2.baz);
        printf("%d %d %d\n", news2.allocated, news2.ps->a, news2.ps->b);

        free(news2.ps);
    }

    return 0;
}
