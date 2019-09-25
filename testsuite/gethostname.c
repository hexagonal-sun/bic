#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    char *name;

    name = malloc(1024);
    gethostname(name, 1024);
    printf("%s\n", name);
    free(name);

    return 0;
}
