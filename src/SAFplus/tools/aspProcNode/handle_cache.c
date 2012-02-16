/*
 * A handle cache prototype using a sparse map for fast lookups and much faster than a bitmap for lookup at the cost
 * of increased memory. The sparse map lookup algorithm is described by Russ Cox in a paper titled: 
 * "Using uninitialized memory for fun and profit"
 * The below prototype for faster handle lookups/add is based on that paper.
 *
 * gcc -g -Wall -o handle_cache handle_cache.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>

typedef struct ClLogStreamFilterMap
{
    unsigned long long handle;
    int severity_mask;
} ClLogStreamFilterMapT;

static ClLogStreamFilterMapT *filter_map;
static unsigned int *filter_sparse_map;
static unsigned int max_index;
static unsigned int num_slots;
#define CLUSTER_ID (1LL)
#define HANDLE_INDEX_MASK (~0U)
#define HANDLE_INDEX(handle) ( (unsigned int) ( (handle) & ~0U) )
#define MAKE_HANDLE(index) ( (CLUSTER_ID << 32) | ( (index) & HANDLE_INDEX_MASK ) )
#define SLOTS_EXTEND (32)
#define SLOTS_MASK (SLOTS_EXTEND - 1)

static __inline__ unsigned long long get_handle(unsigned int index)
{
    ++index;
    return MAKE_HANDLE(index);
}

static unsigned long long filter_map_add(int severity_mask)
{
    unsigned int index = num_slots;
    unsigned long long handle = get_handle(index);
    if(!(num_slots & SLOTS_MASK))
    {
        filter_map = realloc(filter_map, sizeof(*filter_map) * (num_slots + SLOTS_EXTEND));
        assert(filter_map);
        /* optional and would make it slow */
        //memset(filter_map + num_slots, 0, sizeof(*filter_map) * SLOTS_EXTEND);
    }
    filter_map[num_slots].handle = handle;
    filter_map[num_slots].severity_mask = severity_mask;
    /*
     * Add into the sparse map
     */
    if(index >= max_index)
    {
        filter_sparse_map = realloc(filter_sparse_map, sizeof(*filter_sparse_map) * (index+SLOTS_EXTEND));
        assert(filter_sparse_map);
        max_index = index + SLOTS_EXTEND;
    }
    filter_sparse_map[index] = num_slots++;
    printf("Added entry for handle [%#llx] with handle index [%#x] with sev mask [%#x]\n", 
           handle, index, severity_mask);
    return handle;
}

static __inline__ int filter_map_check(unsigned long long handle)
{
    unsigned int index = HANDLE_INDEX(handle);
    if(!index--) return 0;
    if(index >= max_index) return 0;
    return filter_sparse_map[index] < num_slots &&
        filter_map[filter_sparse_map[index]].handle == handle;
}

static __inline__ int filter_map_find(unsigned long long handle)
{
    if(filter_map_check(handle))
        return filter_map[ filter_sparse_map[HANDLE_INDEX(handle)-1] ].severity_mask;
    return -1;
}

static int filter_map_del(unsigned long long handle)
{
    unsigned int index = HANDLE_INDEX(handle)-1;
    unsigned long long last_handle;
    unsigned int slot_index, last_index;
    if(!filter_map_check(handle))
        return -1;
    last_handle = filter_map[num_slots-1].handle;
    last_index = HANDLE_INDEX(last_handle)-1;
    slot_index = filter_sparse_map[index];
    filter_sparse_map[last_index] = slot_index;
    filter_map[slot_index].handle = last_handle;
    --num_slots;
    return 0;
}

static int filter_map_clear(void)
{
    num_slots = 0;
    if(filter_map)
    {
        free(filter_map);
        filter_map = NULL;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int num_handles = 30;
    int i;
    int severity_mask = (1 << LOG_DEBUG) - 1;
    int severity_mask2 = (1 << LOG_CRIT) - 1;
    unsigned long long *handles;
    int repeat = 3, loop = 0;
    if(argc > 1 )
        num_handles = atoi(argv[1]);
    if(argc > 2)
        repeat = atoi(argv[2]);
    if(!num_handles) num_handles = 30;
    handles = calloc(num_handles, sizeof(*handles));
    assert(handles);
    do
    {
        for(i = 0; i < num_handles; ++i)
        {
            int mask;
            if(i&1)
                mask = severity_mask2;
            else 
                mask = severity_mask;
            handles[i] = filter_map_add(mask);
        }
        for(i = 0; i < num_handles; ++i)
        {
            int expected_mask;
            if(i&1)
                expected_mask = severity_mask2;
            else 
                expected_mask = severity_mask;
            assert(filter_map_find(handles[i]) == expected_mask);
        }
        for(i = 0; i < num_handles; ++i)
        {
            assert(filter_map_del(handles[i]) == 0);
        }
        for(i = 0; i < num_handles; ++i)
        {
            assert(filter_map_find(handles[i]) == -1);
        }
        printf("Test success with [%d] handles\n", num_handles);
        filter_map_clear();
    } while(++loop < repeat);

    return 0;
}
