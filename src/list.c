#include <stdlib.h>
#include "list.h"

static void _list_add(list *new, list *prev, list *next)
{
    next->prev = new;
    new->prev = prev;
    new->next = next;
    prev->next = new;
}

void list_add(list *new, list *head)
{
    _list_add(new, head, head->next);
}

void list_add_tail(list *new, list *head)
{
    _list_add(new, head->prev, head);
}

void list_del(list *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = NULL;
    entry->prev = NULL;
}

void list_splice(list *from, list *head)
{
    list *first = from->next;
    list *end = from->prev;

    first->prev = head;
    head->next = first;

    end->next = head->next;
    head->next->prev = end;
}

int list_empty(list *head)
{
    return head->next == head;
}
