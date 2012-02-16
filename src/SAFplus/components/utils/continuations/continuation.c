/*
 * A continuation interface with closures.
 * These principles could be used to create a single threaded async-only request server.
 */

#if !defined (__linux__) && !defined(__APPLE__)
#error "fuck off as your arch. is not supported"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include "continuation.h"
#include "list.h"
#define _CONTINUATION_C_
#include "thunk.h"

#ifdef __APPLE__
#ifndef MAP_32BIT
#define MAP_32BIT (0)
#endif
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#if !defined(MAP_32BIT) && !defined(__i386__)
#error "MAP_32BIT not defined for 64 bit"
#endif

#define CONT_UNWIND (0x2) 

typedef void (*closure_t)(void);

struct continuation
{
    struct list_head list;
    closure_t thunk;
    void *scope;
};

struct continuation_list
{
    int flags;
    continuation_release_block_t *release_block;
    struct list_head continuations;
    struct list_head deferred_continuations;
};

#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)

#define __ALIGN(v, a) ( ( (v) + (a) - 1) & ~( (a) - 1 ) )
#define CONTINUATION_MAP_BITS (1024) /* 1024 simultaneous requests/continuations */
#define CONTINUATION_MAP_BYTES ( __ALIGN(CONTINUATION_MAP_BITS, 8) >>  3 )
#define CONTINUATION_MAP_SIZE  CONTINUATION_MAP_BYTES 
#define CONTINUATION_MAP_WORDS (__ALIGN(CONTINUATION_MAP_SIZE, sizeof(unsigned int)) >> 2 )

struct continuation_map
{
    struct continuation_list *continuation_list;
    unsigned int map[CONTINUATION_MAP_WORDS];
};

static struct continuation_map continuation_map = {
    .map = { [0 ... CONTINUATION_MAP_WORDS-1] = 0 },
};

static __inline__ int __clear_bit(unsigned int *map, int bit)
{
    if(unlikely(bit >= CONTINUATION_MAP_BITS)) return -1;
    map[bit >> 5] &= ~(1 << (bit & 31));
    return 0;
}

static __inline__ int __test_bit(unsigned int *map, int bit)
{
    if(unlikely(bit >= CONTINUATION_MAP_BITS)) return 1;
    return map[bit>>5] & ( 1 << (bit & 31) );
}

static int __ffz(unsigned int *map, unsigned int size)
{
    int i;
    static int ffz_map[16] = { 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, -1 };
    for(i = 0; i < size; ++i)
    {
        unsigned int offset = 0;
        unsigned int mask = map[i] & 0xffffffffU;
        if(mask == 0xffffffffU) continue;
        if( ( mask & 0xffff ) == 0xffff )
        {
            offset += 16;
            mask >>= 16;
        }
        if( (mask & 0xff) == 0xff)
        {
            offset += 8;
            mask >>= 8;
        }
        if( (mask & 0xf) == 0xf)
        {
            offset += 4;
            mask >>= 4;
        }
        
        offset += ffz_map[ mask & 0xf ];
        map[i] |= ( 1 << offset );
        i = (i << 5) + offset;
        if( unlikely(i >= CONTINUATION_MAP_BITS))
            return -1;
        return i;
    }
    return -1;
}

static void __attribute__((constructor)) init_continuation_map(void)
{
    continuation_map.continuation_list = calloc(CONTINUATION_MAP_BITS, sizeof(*continuation_map.continuation_list));
    assert(continuation_map.continuation_list);
}

static int get_continuation(struct continuation_map *map)
{
    return  __ffz(map->map, sizeof(map->map)/sizeof(map->map[0]));
}

static int clear_continuation(struct continuation_map *map, int continuation)
{
    return __clear_bit(map->map, continuation);
}

static __inline__ struct continuation_list *get_continuation_queue(struct continuation_map *map, int cont)
{
    if(unlikely(cont >= CONTINUATION_MAP_BITS)) return NULL;
    return map->continuation_list + cont;
}

/*
 * Take it from the recycle or reclaim list if any.
 */
static struct thunk *reclaim_list;

static void *alloc_closure(void)
{
    struct thunk *thunk = (struct thunk *)reclaim_list;
    if(thunk)
    {
        reclaim_list = *(struct thunk **)thunk;
        --thunk;
    }
    else
    {
        thunk = (struct thunk*)mmap(0, sizeof(*thunk) + sizeof(thunk), PROT_READ | PROT_WRITE | PROT_EXEC,
                                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
        if(thunk == MAP_FAILED)
            thunk = NULL;
    }
    return (void *)thunk;
}

/*
 * Take a scope and the function block to be bound to the scope.
 */
static closure_t make_closure(void *block, void *arg)
{
    struct thunk *thunk = (struct thunk*)alloc_closure();
    if(!thunk) return NULL;
    *thunk = initialize_thunk;
    thunk->scope = arg;
    thunk->call_offset = (long)block - (long)&thunk->UNWIND_OP;
    return (closure_t)thunk;
}

static void free_closure(closure_t closure)
{
    struct thunk *thunk = (struct thunk*)closure;
    int err = 0;
    err = munmap((void*)closure, sizeof(*thunk) + sizeof(thunk));
    assert(err == 0);
}

/*
 * Stack it into a reclaim list since it could be released in the context of the closure itself.
 * So let it be released async.
 * We cannot write to the header of the thunk for the reclaim list as that contains the closure code.
 * So cannot overwrite ourselves.
 */

static void release_closure(closure_t closure)
{
    struct thunk *thunk = (struct thunk *)closure + 1;
    *(struct thunk**)thunk = reclaim_list;
    reclaim_list = thunk;
}

void reclaim_closures(void)
{
    struct thunk *iter = reclaim_list;
    struct thunk *next = NULL;
    while(iter)
    {
        struct thunk *thunk = iter - 1;
        next = *(struct thunk**)iter;
        free_closure((closure_t)thunk);
        iter = next;
    }
    reclaim_list = NULL;
}

int open_continuation(continuation_block_t *closures, int num_closures, 
                      continuation_release_block_t *release_block, int flags)
{
    int cont;
    int i;
    struct continuation_list *continuation_list;

    cont = get_continuation(&continuation_map);
    if(cont < 0)
    {
        errno = ENFILE;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    LIST_HEAD_INIT(&continuation_list->continuations);
    LIST_HEAD_INIT(&continuation_list->deferred_continuations);
    continuation_list->flags = flags;
    continuation_list->release_block = release_block;
    if(unlikely(!closures || !num_closures))
    {
        return cont;
    }
    for(i = 0; i < num_closures; ++i)
    {
        struct continuation *continuation;
        if(!closures[i].block) continue;
        continuation = calloc(1, sizeof(*continuation));
        if(!continuation) 
        {
            errno = ENOMEM;
            goto out_free;
        }
        continuation->thunk = make_closure((void*)closures[i].block, closures[i].arg);
        if(!continuation->thunk) 
        {
            errno = ENOMEM;
            goto out_free;
        }
        continuation->scope = closures[i].arg;
        list_add_tail(&continuation->list, &continuation_list->continuations);
    }
    return cont;

    out_free:
    while(!LIST_EMPTY(&continuation_list->continuations))
    {
        struct continuation *continuation = list_entry(continuation_list->continuations.next, 
                                                       struct continuation, list);
        list_del(&continuation->list);
        free_closure(continuation->thunk);
        free(continuation);
    }

    clear_continuation(&continuation_map, cont);
    return -1;
}

static __inline__ struct continuation *pop_continuation(struct continuation_list *continuation_list, 
                                                        int removed)
{
    struct continuation *continuation;
    if(LIST_EMPTY(&continuation_list->continuations))
    {
        if(LIST_EMPTY(&continuation_list->deferred_continuations))
            return NULL;
        list_splice(&continuation_list->deferred_continuations, &continuation_list->continuations);
    }
    if( (continuation_list->flags & CONT_UNWIND) || 
        (removed && (continuation_list->flags & CONT_REPEATER)))
    {
        continuation = list_entry(continuation_list->continuations.prev, struct continuation, list);
    }
    else
    {
        continuation = list_entry(continuation_list->continuations.next, struct continuation, list);
    }
    list_del(&continuation->list);
    if(LIST_EMPTY(&continuation_list->continuations))
    {
        list_splice(&continuation_list->deferred_continuations, &continuation_list->continuations);
    }
    return continuation;
}

static int __remove_continuation(int cont, int unwind)
{
    struct continuation_list *continuation_list;
    struct continuation *continuation;
    if(unlikely(cont >= CONTINUATION_MAP_BITS))
    {
        errno = EINVAL;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list)) 
    {
        errno = ENOENT;
        return -1;
    }
    continuation = pop_continuation(continuation_list, 1);
    if(unlikely(!continuation)) 
    {
        errno = ESRCH;
        return -1;
    }
    if(continuation_list->release_block)
    {
        continuation_list->release_block(continuation->scope, 
                                         !!LIST_EMPTY(&continuation_list->continuations));
    }
    release_closure(continuation->thunk);
    free(continuation);
    /*
     * In unwind mode, we reverse the continuation list traversal
     */
    if(unwind)
    {
        if(!(continuation_list->flags & CONT_UNWIND))
            continuation_list->flags |= CONT_UNWIND;
    }
    return 0;
}

int remove_continuation(int cont)
{
    return __remove_continuation(cont, 0);
}

int unwind_continuation(int cont)
{
    return __remove_continuation(cont, 1);
}

static __inline__ struct continuation *__peek_continuation(struct continuation_list *continuation_list)
{
    struct continuation *continuation;
    if(LIST_EMPTY(&continuation_list->continuations))
    {
        if(LIST_EMPTY(&continuation_list->deferred_continuations))
            return NULL;
        list_splice(&continuation_list->deferred_continuations, &continuation_list->continuations);
    }
    if(continuation_list->flags & CONT_UNWIND)
    {
        continuation = list_entry(continuation_list->continuations.prev, struct continuation, list);
    }
    else
    {
        continuation = list_entry(continuation_list->continuations.next, struct continuation, list);
    }
    return continuation;
}

/*
 * Just run the continuation at the top of the list without de/re-queueing
 */
int peek_continuation(int cont)
{
    struct continuation_list *continuation_list;
    struct continuation *continuation;
    if(unlikely(cont >= CONTINUATION_MAP_BITS)) 
    {
        errno = EINVAL;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list)) 
    {
        errno = ENOENT;
        return -1;
    }
    continuation = __peek_continuation(continuation_list);
    if(unlikely(!continuation))
    {
        errno = ESRCH;
        return -1;
    }
    continuation->thunk();
    return 0;
}

int close_continuation(int cont)
{
    struct continuation_list *continuation_list;
    if(unlikely(cont >= CONTINUATION_MAP_BITS)) 
    {
        errno = EINVAL;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list))
    {
        errno = ENOENT;
        return -1;
    }
    list_splice(&continuation_list->deferred_continuations, &continuation_list->continuations);
    while(!LIST_EMPTY(&continuation_list->continuations))
    {
        struct continuation *continuation = list_entry(continuation_list->continuations.next, struct continuation, list);
        list_del(&continuation->list);
        if(continuation_list->release_block)
        {
            continuation_list->release_block(continuation->scope, 
                                             !!LIST_EMPTY(&continuation_list->continuations));
        }
        release_closure(continuation->thunk);
        free(continuation);
    }
    clear_continuation(&continuation_map, cont);
    return 0;
}

int play_continuation(int cont)
{
    struct continuation_list *continuation_list;
    struct continuation *continuation;
    int repeater;
    if(unlikely(cont >= CONTINUATION_MAP_BITS))
    {
        errno = EINVAL;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list))
    {
        errno = ENOENT;
        return -1;
    }
    repeater = continuation_list->flags & CONT_REPEATER;
    if(repeater)
    {
        continuation = pop_continuation(continuation_list, 0);
    }
    else
    {
        continuation = __peek_continuation(continuation_list);
    }
    if(unlikely(!continuation))
    {
        errno = ESRCH;
        return -1;
    }
    /*
     * Add the continuation back to the end of the list if its a repeater.
     */
    if(repeater)
    {
        list_add_tail(&continuation->list, &continuation_list->continuations);
    }
    continuation->thunk(); /* run the continuation block*/
    return 0;
}

/*  
 * Advance a continuation to prepare to run the next continuation in the stack
 */
int advance_continuation(int cont)
{
    struct continuation_list *continuation_list;
    struct list_head *head;
    if(unlikely(cont >= CONTINUATION_MAP_BITS))
    {
        errno = EINVAL;
        return -1;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list)) 
    {
        errno = ENOENT;
        return -1;
    }
    if(LIST_EMPTY(&continuation_list->continuations))
    {
        errno = ESRCH;
        return -1; /*nothing to advance*/
    }
    head = continuation_list->continuations.next;
    list_del(head);
    if(continuation_list->flags & CONT_REPEATER)
    {
        list_add_tail(head, &continuation_list->continuations);
    }
    else
    {
        list_add_tail(head, &continuation_list->deferred_continuations);
        if(LIST_EMPTY(&continuation_list->continuations))
        {
            list_splice(&continuation_list->deferred_continuations, &continuation_list->continuations);
        }
    }
    return 0;
}

static int __extend_continuation(int cont, continuation_block_t *closures, int num_closures, int prepend)
{
    struct continuation_list *continuation_list;
    struct continuation *continuation;
    DECLARE_LIST_HEAD(closure_list);
    int i;
    int err = -1;
    if(unlikely(!closures || !num_closures))
    {
        errno = EINVAL;
        goto out;
    }
    if(unlikely(cont >= CONTINUATION_MAP_BITS))
    {
        errno = EINVAL;
        goto out;
    }
    continuation_list = get_continuation_queue(&continuation_map, cont);
    if(unlikely(!continuation_list))
    {
        errno = ENOENT;
        goto out;
    }
    for(i = 0; i < num_closures; ++i)
    {
        if(!closures[i].block) continue;
        continuation = calloc(1, sizeof(*continuation));
        if(!continuation) 
        {
            errno = ENOMEM;
            goto out_free;
        }
        continuation->thunk = make_closure((void*)closures[i].block, closures[i].arg);
        if(!continuation->thunk)
        {
            errno = ENOMEM;
            goto out_free;
        }
        continuation->scope = closures[i].arg;
        list_add_tail(&continuation->list, &closure_list);
    }
    if(prepend)
    {
        list_splice(&closure_list, &continuation_list->continuations);
    }
    else
    {
        list_concat(&closure_list, &continuation_list->continuations);
    }
    err = 0;
    goto out;

    out_free:
    while(!LIST_EMPTY(&closure_list))
    {
        continuation = list_entry(closure_list.next, struct continuation, list);
        list_del(&continuation->list);
        free_closure(continuation->thunk);
        free(continuation);
    }

    out:
    return err;
}

int add_continuation(int cont, continuation_block_t *closures, int num_closures)
{
    return __extend_continuation(cont, closures, num_closures, 1);
}

int append_continuation(int cont, continuation_block_t *closures, int num_closures)
{
    return __extend_continuation(cont, closures, num_closures, 0);
}
