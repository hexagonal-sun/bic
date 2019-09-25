#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char *s1, *s2, *s3;

    s1 = "Hello, ";
    s2 = "World!";
    s3 = malloc(strlen(s1) + strlen(s2) + 2);
    puts(s1);
    puts(s2);
    strcpy(s3, s1);
    strcat(s3, s2);
    puts(s3);

    return 0;
}
