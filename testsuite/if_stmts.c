#include <stdio.h>

void fun(int a)
{
   if (a) {
        printf("True branch 1\n");
        printf("True branch 2\n");
    } else
        printf("False branch\n");
    printf("Done\n");
}

void fun1(unsigned int a)
{

    if (a)
        printf("A is zero!");

    if (a <= 0)
        printf("Zero branch\n");
    else if (a > 1)
        printf("Anything other branch\n");
    else
        printf("One branch\n");

}

int main()
{
    fun(0);
    fun(1);

    fun1(0);
    fun1(1);
    fun1(2);
    fun1(242);

    return 0;
}
