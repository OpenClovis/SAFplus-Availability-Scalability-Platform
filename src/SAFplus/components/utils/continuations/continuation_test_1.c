#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include "continuation_player.h"

struct continuation_scope
{
#define WIND (0x1)
#define UNWIND (0x2)
    int continuation;
    int state;
};

static struct continuation_scope *make_scope(int continuation)
{
    struct continuation_scope *scope = calloc(1, sizeof(*scope));
    assert(scope);
    scope->continuation = continuation;
    scope->state |= WIND;
    return scope;
}

#define PRINT_CONTINUATION(cont) do {                                   \
    printf("Inside continuation id [%d] with state [%d] in function [%s]\n", \
           (cont)->continuation, (cont)->state, __FUNCTION__);          \
} while(0)

#ifdef APPEND_CONTINUATION

#define QUEUE_CONTINUATION(cont, nr, arg) do {                          \
    continuation_block_t continuation = {.block = continuation_##nr, .arg=arg }; \
    append_continuation(cont, &continuation, 1);                        \
    advance_continuation(cont);                                         \
}while(0)

#define REMOVE_CONTINUATION(cont) do {          \
    unwind_continuation(cont);                  \
}while(0)

#else

#define QUEUE_CONTINUATION(cont, nr, arg) do {                          \
    continuation_block_t continuation = {.block = continuation_##nr, .arg=arg}; \
    add_continuation(cont, &continuation, 1);                           \
}while(0)

#define REMOVE_CONTINUATION(cont) do {          \
    remove_continuation(cont);                  \
}while(0)

#endif

static void continuation_release(void *cont, int last_cont)
{
    if(last_cont) 
    {
        printf("Continuation [%d] being released\n", ((struct continuation_scope*)cont)->continuation);
        free(cont); 
    }
}

static void continuation_2(void *arg)
{
    struct continuation_scope *cont = arg;
    PRINT_CONTINUATION(cont);
    /*
     * unwind 
     */
    if(cont->state & WIND)
    {
        cont->state &= ~WIND;
        cont->state |= UNWIND;
        REMOVE_CONTINUATION(cont->continuation);
    }
}

static void continuation_1(void *arg)
{
    struct continuation_scope *cont = arg;
    PRINT_CONTINUATION(cont);
    if(cont->state & WIND)
    {
        QUEUE_CONTINUATION(cont->continuation, 2, arg);
    }
    else 
    {
        REMOVE_CONTINUATION(cont->continuation);
    }
}

static void continuation_0(void *arg)
{
    struct continuation_scope *cont = arg;
    PRINT_CONTINUATION(cont);
    if(cont->state & WIND)
    {
        QUEUE_CONTINUATION(cont->continuation, 1, arg);
    }
    else
    {
        REMOVE_CONTINUATION(cont->continuation);
    }
}

static void continuation_test(int continuations)
{
    int i;
    for(i = 0; i < continuations; ++i)
    {
        continuation_block_t continuation = {.block = continuation_0};
        int c = open_continuation(NULL, 0, continuation_release, 0);
        assert(c >= 0);
        printf("Continuation [%d] opened\n", c);
        mark_continuation(c);
        continuation.arg = (void*)make_scope(c);
        assert(add_continuation(c, &continuation, 1) == 0);
    }
    continuation_player();
}

int main(int argc, char **argv)
{
    int c = 10;
    if(argc > 1)
        c = atoi(argv[1]);
    if(!c) c = 10;
    if(c > CONTINUATIONS_MAX) c = CONTINUATIONS_MAX;
    continuation_test(c);
    return 0;
}
