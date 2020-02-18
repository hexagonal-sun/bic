typedef int foo;

int main()
{
    int a;
    foo *bar;

    a = 0;
    bar = &a;

    return *bar;
}
