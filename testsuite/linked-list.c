#include <stdio.h>
#include <stdlib.h>

typedef int footype;

struct list {
    struct list *next;
    footype val;
};

struct list *head;

void push_val(int val)
{
    struct list *new;

    new = malloc(sizeof(struct list));

    new->next = head;
    new->val = val;
    head = new;
}

int main()
{
    int i;

    head = 0;

    for (i = 0; i < 1000; i++)
        push_val(i);

    printf("%d\n", head->next->next->next->next->val);

    for (i = 0; i < 1000; i++) {
        struct list *tmp;
        tmp = head->next;
        free(head);
        head = tmp;
    }

    return 0;
}
