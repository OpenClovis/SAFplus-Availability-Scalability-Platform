#ifndef _LIST_H_
#define _LIST_H_

#include <assert.h>

#ifdef __cplusplus
extern "C "{
#endif

struct list_head
{
    struct list_head *next;
    struct list_head *prev;
};

#define INIT_LIST_HEAD(name) { &(name), &(name) }

#define DECLARE_LIST_HEAD(name)                     \
    struct list_head name = INIT_LIST_HEAD(name)


#define LIST_HEAD_INIT(ptr) do {                \
    (ptr)->next = (ptr)->prev = ptr;        \
} while(0)

#define LIST_EMPTY(list)  ( (list) == (list)->next )

#define __list_add(element,a,b) do {            \
    (element)->next = b;                        \
    (element)->prev = a;                        \
    (a)->next = element;                        \
    (b)->prev = element;                        \
} while(0)

#define __list_del(a,b) do {                    \
    (a)->next = b;                              \
    (b)->prev = a;                              \
} while(0)

static __inline__ void list_head_init(struct list_head *head) {
    LIST_HEAD_INIT(head);
}

static __inline__ void list_add_tail(struct list_head *element,struct list_head *head) {
    struct list_head *tail = head->prev;
    assert(!element->next && !element->prev);
    __list_add(element,tail,head);
}

static __inline__ void list_add(struct list_head *element,struct list_head *head) {
    struct list_head *first = head->next;
    assert(!element->next && !element->prev);
    __list_add(element,head,first);
}

static __inline__ void list_del(struct list_head *element) {
    struct list_head *before = element->prev;
    struct list_head *after  = element->next;
    assert(before && after);
    __list_del(before,after);
    element->next = element->prev = NULL;
}

static __inline__ void list_del_init(struct list_head  *element) { 
    list_del(element); 
    LIST_HEAD_INIT(element);
} 

static __inline__ void list_splice(struct list_head *src,struct list_head *dst) {
    if(!LIST_EMPTY(src) ) {
        struct list_head *dst_first = dst->next;
        struct list_head *src_last  = src->prev;
        dst->next = src->next;
        src->next->prev = dst;   
        src_last->next = dst_first;
        dst_first->prev = src_last;
        LIST_HEAD_INIT(src);
    }
}

static __inline__ void list_concat(struct list_head *src, struct list_head *dst)
{
    if(!LIST_EMPTY(src))
    {
        struct list_head *dst_last = dst->prev;
        dst_last->next = src->next;
        src->next->prev = dst_last;
        src->prev->next = dst;
        dst->prev = src->prev;
        LIST_HEAD_INIT(src);
    }
}

#define list_entry(ptr,cast,field)                                      \
    (cast*) ( (unsigned char*)(ptr) - (unsigned long)(&((cast*)0)->field) )

#define list_for_each(list,head)                            \
    for(list = (head)->next; list != (head); list = list->next)

#define list_sort_add(element,head,cast,field, cmp_fn) do { \
    struct list_head *__tmp = NULL;                         \
    __typeof__ (cmp_fn) *fn_ptr = &(cmp_fn);                \
    cast *__reference = list_entry(element,cast,field);     \
    list_for_each(__tmp,head) {                             \
        cast *__node = list_entry(__tmp,cast,field);        \
        if(fn_ptr(__reference,__node) < 0) {                \
            list_add_tail(element,__tmp);                   \
            goto __out;                                     \
        }                                                   \
    }                                                       \
    list_add_tail(element,head);                            \
    __out:                                                  \
    (void)"list";                                           \
}while(0)

#ifdef __cplusplus
}
#endif

#endif
