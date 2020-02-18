#include <stdio.h>

int main() {
    char *a;
    int *b;
    printf("%ld %ld\n", sizeof(*a), sizeof(*b));
    return 0;
}
