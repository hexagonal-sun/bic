#include <stdio.h>

struct
  {
    union
    {
      unsigned long long int __wseq;
        unsigned int __high;
    }bar, baz;
      int a;
  }foo;


int main()
{
    foo.a = 10;
    foo.bar.__wseq = 1242;
    foo.baz.__high = 2424;


    printf("%d\n", foo.a);
    printf("%d\n", foo.bar.__wseq);
    printf("%d\n", foo.bar.__high);
    printf("%d\n", foo.baz.__wseq);
    printf("%d\n", foo.baz.__high);

    return 0;
}
