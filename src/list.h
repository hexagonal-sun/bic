#ifndef __LIST_H_
#define __LIST_H_

/*
 * This file is mostly based on ideas from the Linux kernel list
 * implementation (include/linux/list.h).
 */

typedef struct list list;

struct list {
    struct list *next, *prev;
};

/* Creation and initialisation helper macros. */
#define LIST_INIT(name) \
    { &(name), &(name) }

#define LIST(name) \
    list name = LIST_INIT(name)

#define INIT_LIST(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/* List manipulation functions. */
void list_add(list *new, list *head);
void list_add_tail(list *new, list *head);
void list_del(list *entry);
void list_splice(list *from, list *head);
int list_empty(list *list);

/* List accessor and iteration functions. */
#define list_entry(list, type, list_member_name) \
	((type *)((char *)(list)-(unsigned long)(&((type *)0)->list_member_name)))

#define list_for_each(pos, head, member)        \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
         &pos->member != (head);                                 \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
