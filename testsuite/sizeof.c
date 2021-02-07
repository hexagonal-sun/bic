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

    printf("%zu\n", sizeof(a));
    printf("%zu\n", sizeof(b));
    printf("%zu\n", sizeof(c));
    printf("%zu\n", sizeof(d));
    printf("%zu\n", sizeof(e));
    printf("%zu\n", sizeof(ua));
    printf("%zu\n", sizeof(ub));
    printf("%zu\n", sizeof(uc));
    printf("%zu\n", sizeof(ud));
    printf("%zu\n", sizeof(ue));
    printf("%zu\n", sizeof(pa));
    printf("%zu\n", sizeof(pb));
    printf("%zu\n", sizeof(pc));
    printf("%zu\n", sizeof(char));
    printf("%zu\n", sizeof(short));
    printf("%zu\n", sizeof(int));
    printf("%zu\n", sizeof(long));
    printf("%zu\n", sizeof(long long));
    printf("%zu\n", sizeof(unsigned char));
    printf("%zu\n", sizeof(unsigned short));
    printf("%zu\n", sizeof(unsigned int));
    printf("%zu\n", sizeof(unsigned long));
    printf("%zu\n", sizeof(unsigned long long));
    printf("%zu\n", sizeof(void *));
    printf("%zu\n", sizeof(foob));
    printf("%zu\n", sizeof(pstruct));
    printf("%zu\n", sizeof(array));
    printf("%zu\n", sizeof(struct s_with_array));

    return 0;
}
