#include <stdio.h>

int factorial(int n)
{
    if (n == 0)
    {
        return 1;
    }
    return n * factorial(n - 1);
}

int main()
{
    printf("Factorial of 4 is: %d\n", factorial(4));
    return 0;
}
