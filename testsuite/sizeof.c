#include <stdio.h>

int main()
{
    char a;
    short b;
    int c;
    long d;
    long long e;
    unsigned char ua;
    unsigned short ub;
    unsigned int uc;
    unsigned long ud;
    unsigned long long ue;
    void *pa;
    char *pb;
    unsigned long long *pc;

    int array[10];

    struct newstruct {
        int a;
        unsigned int b;
        long c;
        long long d;
        void *pa;
        void *pb;
    } foob;

    struct s_with_array {
        void *ptr;
        int this_array[2444];
        short that_array[244];
    };

    struct newstruct *pstruct;

    printf("%d\n", sizeof(a));
    printf("%d\n", sizeof(b));
    printf("%d\n", sizeof(c));
    printf("%d\n", sizeof(d));
    printf("%d\n", sizeof(e));
    printf("%d\n", sizeof(ua));
    printf("%d\n", sizeof(ub));
    printf("%d\n", sizeof(uc));
    printf("%d\n", sizeof(ud));
    printf("%d\n", sizeof(ue));
    printf("%d\n", sizeof(pa));
    printf("%d\n", sizeof(pb));
    printf("%d\n", sizeof(pc));
    printf("%d\n", sizeof(char));
    printf("%d\n", sizeof(short));
    printf("%d\n", sizeof(int));
    printf("%d\n", sizeof(long));
    printf("%d\n", sizeof(long long));
    printf("%d\n", sizeof(unsigned char));
    printf("%d\n", sizeof(unsigned short));
    printf("%d\n", sizeof(unsigned int));
    printf("%d\n", sizeof(unsigned long));
    printf("%d\n", sizeof(unsigned long long));
    printf("%d\n", sizeof(void *));
    printf("%d\n", sizeof(foob));
    printf("%d\n", sizeof(pstruct));
    printf("%d\n", sizeof(array));
    printf("%d\n", sizeof(struct s_with_array));

    return 0;
}
