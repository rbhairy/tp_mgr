#ifndef LIST_H__
#define LIST_H__

struct list_head {
        struct list_head *next;
        struct list_head *prev;
};

#define INIT_LIST_HEAD(head) do {               \
        (head)->next = (head)->prev = head;     \
} while (0)


static inline void
list_add_tail (struct list_head *new, struct list_head *head)
{
        new->next = head;
        new->prev = head->prev;
        new->prev->next = new;
        new->next->prev = new;
}

static inline void
list_del (struct list_head *old)
{
        old->prev->next = old->next;
        old->next->prev = old->prev;
        old->next = (void *)0xDEADBEEF;
        old->prev = (void *)0xDEADBEEF;
}


static inline void
list_del_init (struct list_head *old)
{
        old->prev->next = old->next;
        old->next->prev = old->prev;

        old->next = old;
        old->prev = old;
}

static inline int
list_empty (struct list_head *head)
{
        return (head->next == head);
}

#define list_entry(ptr, type, member)                                   \
        ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member);      \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.next, typeof(*pos), member))

#endif /*LIST_H__ */

