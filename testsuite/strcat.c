typedef unsigned long int size_t;
void *malloc(size_t len);
size_t strlen(char *s);
int puts(char *s);
char *strcpy(char *dest, char *src);
char *strcat(char *s1, char *s2);

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
}
