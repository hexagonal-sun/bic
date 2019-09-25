#include <stdio.h>

int main() {
    char *a;
    int *b;
    printf("%d %d\n", sizeof(*a), sizeof(*b));
    return 0;
}
