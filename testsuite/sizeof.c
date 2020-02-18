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

    printf("%ld\n", sizeof(a));
    printf("%ld\n", sizeof(b));
    printf("%ld\n", sizeof(c));
    printf("%ld\n", sizeof(d));
    printf("%ld\n", sizeof(e));
    printf("%ld\n", sizeof(ua));
    printf("%ld\n", sizeof(ub));
    printf("%ld\n", sizeof(uc));
    printf("%ld\n", sizeof(ud));
    printf("%ld\n", sizeof(ue));
    printf("%ld\n", sizeof(pa));
    printf("%ld\n", sizeof(pb));
    printf("%ld\n", sizeof(pc));
    printf("%ld\n", sizeof(char));
    printf("%ld\n", sizeof(short));
    printf("%ld\n", sizeof(int));
    printf("%ld\n", sizeof(long));
    printf("%ld\n", sizeof(long long));
    printf("%ld\n", sizeof(unsigned char));
    printf("%ld\n", sizeof(unsigned short));
    printf("%ld\n", sizeof(unsigned int));
    printf("%ld\n", sizeof(unsigned long));
    printf("%ld\n", sizeof(unsigned long long));
    printf("%ld\n", sizeof(void *));
    printf("%ld\n", sizeof(foob));
    printf("%ld\n", sizeof(pstruct));
    printf("%ld\n", sizeof(array));
    printf("%ld\n", sizeof(struct s_with_array));

    return 0;
}
