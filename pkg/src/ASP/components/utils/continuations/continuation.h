#ifndef _CONTINUATION_H_
#define _CONTINUATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONT_REPEATER (0x1)

typedef void (continuation_fun_t)(void *arg);
typedef struct continuation_block
{
    continuation_fun_t *block;
    void *arg;
} continuation_block_t;

typedef void (continuation_release_block_t)(void *arg, int last_cont);

int open_continuation(continuation_block_t *closures, int num_closures, 
                      continuation_release_block_t *release_block, int flags);
int close_continuation(int cont);
int remove_continuation(int cont);
/*
 * Removes and reverses the direction of the continuation traversal.
 */
int unwind_continuation(int cont);
int play_continuation(int cont);
int peek_continuation(int cont);
int advance_continuation(int cont);
int add_continuation(int cont, continuation_block_t *closures, int num_closures);
int append_continuation(int cont, continuation_block_t *closures, int num_closures);
void reclaim_closures(void);

#ifdef __cplusplus
}
#endif

#endif
