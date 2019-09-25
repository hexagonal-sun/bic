#include <stdio.h>

int a1 = 1;
int a2 = 1;

int main()
{
    int i;

    for (i = 0; i < 20; i++) {
        int next;
        next = a1 + a2;
        a1 = a2;
        a2 = next;
        printf("%d\n", next);
    }

    return 0;
}
