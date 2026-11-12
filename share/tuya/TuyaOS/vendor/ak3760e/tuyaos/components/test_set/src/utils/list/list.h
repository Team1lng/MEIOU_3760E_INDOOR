#ifndef _LIST_H_
#define _LIST_H_

#ifndef NULL 
#define NULL (void *)0
#endif 

struct list_head
{
   struct list_head *prev;
   struct list_head *next;
};

static inline void list_prefetch(const void *x) {;}

#define list_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define list_container_of(ptr, type, member) ({                     \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);        \
        (type *)( (char *)__mptr - list_offsetof(type,member) );})

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)


void PRODUCT_TEST_INIT_LIST_HEAD(struct list_head *list);
void product_test_list_add(struct list_head *new,struct list_head *head);
void product_test_list_add_tail(struct list_head *new,struct list_head *head);
void product_test_list_del(struct list_head *entry);
void product_test_list_replace(struct list_head *old,struct list_head *new);
void product_test_list_replace_init(struct list_head *old,struct list_head *new);
void product_test_list_del_init(struct list_head *entry);
void product_test_list_move(struct list_head *list,struct list_head *head);
void product_test_list_move_tail(struct list_head *list,struct list_head *head);
int  product_test_list_is_last(const struct list_head *list,const struct list_head *head);
int  product_test_list_empty(const struct list_head *head);
int  product_test_list_empty_careful(const struct list_head *head);
void product_test_list_splice(struct list_head *list,struct list_head *head);
void product_test_list_splice_init(struct list_head *list,struct list_head *head);


#define list_entry(ptr, type, member) \
    list_container_of(ptr, type, member)

#define list_for_each(pos, head)                                        \
    for (pos = (head)->next; list_prefetch(pos->next), pos != (head);   \
            pos = pos->next)

#define __list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head)                                   \
    for (pos = (head)->prev; list_prefetch(pos->prev), pos != (head);   \
            pos = pos->prev)

#define list_for_each_safe(pos, n, head)                        \
    for (pos = (head)->next, n = pos->next; pos != (head);      \
        pos = n, n = pos->next)


#define list_for_each_entry(pos, head, member)                      \
    for (pos = list_entry((head)->next, typeof(*pos), member);      \
         list_prefetch(pos->member.next), &pos->member != (head);   \
         pos = list_entry(pos->member.next, typeof(*pos), member))


#define list_for_each_entry_reverse(pos, head, member)              \
    for (pos = list_entry((head)->prev, typeof(*pos), member);      \
         list_prefetch(pos->member.prev), &pos->member != (head);   \
         pos = list_entry(pos->member.prev, typeof(*pos), member))


#define list_prepare_entry(pos, head, member) \
    ((pos) ? : list_entry(head, typeof(*pos), member))


#define list_for_each_entry_continue(pos, head, member)             \
    for (pos = list_entry(pos->member.next, typeof(*pos), member);  \
         list_prefetch(pos->member.next), &pos->member != (head);   \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_from(pos, head, member)                 \
    for (; list_prefetch(pos->member.next), &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member),      \
        n = list_entry(pos->member.next, typeof(*pos), member);     \
         &pos->member != (head);                                    \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))


#define list_for_each_entry_safe_continue(pos, n, head, member)         \
    for (pos = list_entry(pos->member.next, typeof(*pos), member),      \
        n = list_entry(pos->member.next, typeof(*pos), member);         \
         &pos->member != (head);                                        \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_from(pos, n, head, member)             \
    for (n = list_entry(pos->member.next, typeof(*pos), member);        \
         &pos->member != (head);                                        \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)      \
    for (pos = list_entry((head)->prev, typeof(*pos), member),      \
        n = list_entry(pos->member.prev, typeof(*pos), member);     \
         &pos->member != (head);                                    \
         pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#endif
