/*
 * A test player that just plays the continuations opened.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "continuation.h"
#include "continuation_player.h"

static struct continuation_map
{
    unsigned int *map;
    int bits;
} continuation_map = { 
    .map = NULL, .bits = 0 
};

#define CONTINUATION_MAP_BITS ( continuation_map.bits )
#define CONTINUATION_MAP_BITS_EXTEND (64)
#define __ALIGN(n, align) ( ( (n) + (align) - 1 ) & ~( (align) - 1 ) )
#define CONTINUATION_MAP_WORDS (__ALIGN(continuation_map.bits, 32) >> 5 )

static void extend_map(int c)
{
    int last_words;
    if(c < CONTINUATION_MAP_BITS) return;
    last_words = CONTINUATION_MAP_WORDS;
    CONTINUATION_MAP_BITS = c + CONTINUATION_MAP_BITS_EXTEND;
    continuation_map.map = realloc(continuation_map.map, 
                                   sizeof(*continuation_map.map) * CONTINUATION_MAP_WORDS);
    assert(continuation_map.map != NULL);
    memset(continuation_map.map + last_words, 0, sizeof(*continuation_map.map) * (CONTINUATION_MAP_WORDS - last_words));
}

void mark_continuation(int c)
{
    if(c >= CONTINUATION_MAP_BITS)
    {
        extend_map(c);
    }
    continuation_map.map[c >> 5] |= ( 1 << (c & 31) );
}

void unmark_continuation(int c)
{
    if(c >= CONTINUATION_MAP_BITS) return;
    continuation_map.map[c >> 5] &= ~( 1 << (c & 31) );
}

static void *continuation_loop(void *unused)
{
    register int i;
    for(;;)
    {
        int run = 0;
        /*
         * Walk the bitmap and run the set continuations.  
         * Break when all are cleared.
         */
        for(i = 0; i < CONTINUATION_MAP_WORDS; ++i)
        {
            unsigned int mask = continuation_map.map[i];
            register int j;
            if(!mask) continue;
            for(j = 0; j < 32; ++j)
            {
                if( mask & ( 1 << j) )
                {
                    int cont;
                    run = 1;
                    cont = (i << 5) + j;
                    if(play_continuation(cont) < 0)
                    {
                        close_continuation(cont);
                        unmark_continuation(cont);
                    }
                }
                
            }
        }
        /*
         * Reclaim closures if any.
         */
        reclaim_closures();
        if(!run) 
        {
            fprintf(stderr, "All continuations have been run\n");
            break;
        }
    }
    return NULL;
}

void continuation_player(void)
{
    pthread_attr_t attr;
    pthread_t tid;
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, continuation_loop, NULL);
    pthread_join(tid, NULL);
}
