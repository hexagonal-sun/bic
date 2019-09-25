#include <stdio.h>
#include <stdlib.h>

union foobar {
    int a;
    float b;
    void *ptr;
    unsigned char array[4];
};

int main()
{
    union foobar foob;

    foob.ptr = 0;
    foob.a = 0;
    foob.b = 2.42;
    printf("%d %f %p\n", foob.a, foob.b, foob.ptr);
    printf("%u %u %u %u\n", foob.array[0], foob.array[1],
           foob.array[2], foob.array[3]);

    return 0;
}
