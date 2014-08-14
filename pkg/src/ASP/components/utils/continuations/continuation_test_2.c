/*
 * A factorial example using continuations. Probably one of the most complicated implementation 
 * of a 3 line inline routine to demonstrate a complicated concept.
 * Maybe the below implementation makes you a responsible coder as continuations explain how the stack is played
 * when the function is recursive. 
 * But in closure implementations, there is ideally no stack usage as things are managed on the heap
 * through a state machine that plays the stacked functions/continuations.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "continuation_player.h"

struct scope_fact
{
#define WIND (0x1)
#define UNWIND (0x2)
    int *g_fact;
    int *g_acc;
    int *g_cont;
    int *g_state;
    int l_fact;
};

static void continuation_fact_release(void *arg, int last_cont)
{
    if(last_cont)
    {
        free( ( (struct scope_fact*)arg)->g_fact);
    }
    free(arg);
}

#ifdef TAIL_RECURSIVE

static void do_fact(void *arg)
{
    struct scope_fact *scope = arg;
    if(*scope->g_state & WIND)
    {
        if(scope->l_fact <= 1) 
        {
            /*
             * So-called optimization with continuations :)
             * Just throw away the remaining of the continuation at this stage 
             * since the accumulator has the result. 
             */
            printf("Factorial of [%d] = [%d]\n", *scope->g_fact, *scope->g_acc);
            unmark_continuation(*scope->g_cont);
            close_continuation(*scope->g_cont); /* scope is invalid or freed after this point */
        }
        else
        {
            struct scope_fact *next_scope = calloc(1, sizeof(*next_scope));
            continuation_block_t continuation = {.block = do_fact, .arg = next_scope };
            assert(next_scope);
            memcpy(next_scope, scope, sizeof(*next_scope));
            *scope->g_acc *= scope->l_fact;
            next_scope->l_fact = scope->l_fact - 1;
            add_continuation(*next_scope->g_cont, &continuation, 1);
        }
    }
}

#else

static void do_fact(void *arg)
{
    struct scope_fact *scope = arg;
    if(*scope->g_state & WIND)
    {
        if(scope->l_fact <= 1) 
        {
            *scope->g_acc = 1;
            *scope->g_state &= ~WIND;
            *scope->g_state |= UNWIND;
        }
        else
        {
            struct scope_fact *next_scope = calloc(1, sizeof(*next_scope));
            continuation_block_t continuation = {.block = do_fact, .arg = next_scope };
            assert(next_scope);
            memcpy(next_scope, scope, sizeof(*next_scope));
            next_scope->l_fact = scope->l_fact - 1;
            add_continuation(*next_scope->g_cont, &continuation, 1);
        }
    }
    else
    {
        *scope->g_acc *= scope->l_fact;
    }

    if(*scope->g_state & UNWIND)
    {
        if(scope->l_fact == *scope->g_fact)
        {
            printf("Factorial of [%d] = [%d]\n", scope->l_fact, *scope->g_acc);
        }
        remove_continuation(*scope->g_cont); /* the scope is invalid/freed after this point*/
    }
}

#endif

static void continuation_fact(int fact)
{
    struct scope_fact *scope = calloc(1, sizeof(*scope));
    continuation_block_t continuation = {.block = do_fact, .arg = scope };
    int *fact_globals;
    int c;

    assert(scope);

    fact_globals = calloc(4, sizeof(*fact_globals));
    assert(fact_globals);

    /*
     * globals to be shared across scopes.
     */
    scope->g_fact = fact_globals;
    scope->g_acc = fact_globals+1;
    scope->g_cont = fact_globals+2;
    scope->g_state = fact_globals+3;

    *scope->g_acc = 1;
    *scope->g_fact = fact;
    *scope->g_state = WIND;
    scope->l_fact = fact;

    c = open_continuation(&continuation, 1, continuation_fact_release, 0);
    assert(c >= 0);
    *scope->g_cont = c;
    mark_continuation(c);
    /*
     * Just play the marked continuations.
     */
    continuation_player();
}

int main(int argc, char **argv)
{
    int fact = 0;
    if(argc > 1)
        fact = atoi(argv[1]);
    continuation_fact(fact);
    return 0;
}
