void free(void *ptr);
void *malloc(int len);
int printf(char *s, ...);
int gethostname(char *name, unsigned int namelen);

int main()
{
    char *name;

    name = malloc(1024);
    gethostname(name, 1024);
    printf("%s\n", name);
    free(name);
}
