#include <clRadixTree.h>
#include <clDebugApi.h>
#include <clHeapApi.h>

/*
 * An example spatial distribution of the tree with 4 bits per slot:
 * The index distribution would be:
 * height 0 -> 0
 * height 1 -> 2^0 through 2 ^ 4
 * height 2 -> 2^4 through 2 ^ 8
 * height 3 -> 2^8 through 2 ^ 12
 * height 4 -> 2^12 through 2 ^ 16
 * height 5 -> 2^16 through 2 ^ 20
 * height 6 -> 2^20 through 2^24
 * height 7 -> 2^24 through 2 ^28
 * height 8 -> 2^28 through 2^32
 */

#define RADIX_TREE_INDEX_BITS ((ClUint32T)sizeof(ClUint32T) * 8)
#define RADIX_TREE_MAP_SHIFT (4)
#define RADIX_TREE_MAP_SIZE (1 << RADIX_TREE_MAP_SHIFT)
#define RADIX_TREE_MAP_MASK (RADIX_TREE_MAP_SIZE - 1)
#define RADIX_TREE_MAX_PATH_ALIGN(v) ( (RADIX_TREE_INDEX_BITS+(v)-1)/(v) * (v) )
#define RADIX_TREE_MAX_PATH  (RADIX_TREE_MAX_PATH_ALIGN(RADIX_TREE_MAP_SHIFT)/RADIX_TREE_MAP_SHIFT)

typedef struct ClRadixTreeNode
{
    ClUint32T height;
    ClUint32T count;
    ClPtrT slots[RADIX_TREE_MAP_SIZE];
} ClRadixTreeNodeT;

typedef struct ClRadixTreeRoot
{
    ClRadixTreeNodeT *node;
    ClUint32T height;
    ClPtrT firstSlot; /* for index 0*/
} ClRadixTreeRootT;

typedef struct ClRadixTreePath
{
    ClRadixTreeNodeT *node;
    ClUint32T offset;
} ClRadixTreePathT;

static ClUint32T heightIndexMap[RADIX_TREE_MAX_PATH+1];

static __inline__ ClUint32T maxindex(ClUint32T height)
{
    ClUint32T width = height * RADIX_TREE_MAP_SHIFT;
    ClInt32T shift = RADIX_TREE_INDEX_BITS - width;
    if(shift < 0 || shift >= RADIX_TREE_INDEX_BITS) return 0;
    return ~0U >> shift;
}

static __inline__ void heightIndexMapSet(void)
{
    ClInt32T i;
    for(i = 0; i < sizeof(heightIndexMap)/sizeof(heightIndexMap[0]); ++i)
        heightIndexMap[i] = maxindex(i);
}

static __inline__ ClUint32T radixTreeMaxIndex(ClUint32T height)
{
    return heightIndexMap[height];
}

static ClRcT radixTreeExtend(ClRadixTreeRootT *root, ClUint32T index)
{
    ClRadixTreeNodeT *node = NULL;
    ClUint32T height;
    ClRcT rc = CL_OK;

    height = root->height + 1;
    while(index > radixTreeMaxIndex(height))
        ++height;
    
    if(!root->node)
    {
        root->height = height;
        goto out;
    }

    do
    {
        node = clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        node->slots[0] = root->node;
        node->height = root->height + 1;
        node->count = 1;
        root->height = node->height;
        root->node = node;
    } while(height > root->height);

    out:
    return rc;
}

ClRcT clRadixTreeInsert(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT item, ClPtrT *lastItem)
{
    ClRadixTreeRootT *root = NULL;
    ClRadixTreeNodeT *node = NULL;
    ClRadixTreeNodeT *slot = NULL;
    ClUint32T height;
    ClUint32T shift;
    ClUint32T offset = 0;
    ClRcT rc = CL_OK;

    if(!handle) 
    {
        rc = CL_ERR_INVALID_PARAMETER;
        goto out;
    }

    root = (ClRadixTreeRootT*)handle;
    if(!index)
    {
        if(!lastItem && root->firstSlot)
        {
            rc = CL_ERR_ALREADY_EXIST;
            goto out;
        }
        if(lastItem)
            *lastItem = root->firstSlot;
        root->firstSlot = item;
        goto out;
    }
    if(index > radixTreeMaxIndex(root->height)
       && (rc = radixTreeExtend(root, index)) != CL_OK )
        goto out;
    
    height = root->height;
    shift = (height - 1) * RADIX_TREE_MAP_SHIFT;
    slot = root->node;

    while(height > 0)
    {
        if(!slot)
        {
            slot = clHeapCalloc(1, sizeof(*slot));
            CL_ASSERT(slot != NULL);
            slot->height = height;
            /*
             * mark this as a child
             */
            if(node)
            {
                node->slots[offset] = slot;
                ++node->count;
            }
            else
            {
                root->node = slot;
            }
        }
        offset = (index >> shift) & RADIX_TREE_MAP_MASK;
        node = slot;
        slot = node->slots[offset];
        shift -= RADIX_TREE_MAP_SHIFT;
        --height;
    }

    if(slot && !lastItem)
    {
        rc = CL_ERR_ALREADY_EXIST;
        goto out;
    }

    if(lastItem)
    {
        *lastItem = slot;
    }
    if(node)
    {
        if(!slot)
            ++node->count;
        node->slots[offset] = item;
    }
    else
    {
        root->node = item;
    }

    out:
    return rc;
}

static ClRcT __clRadixTreeLookup(ClRadixTreeRootT *root, ClUint32T index, ClPtrT *item)
{
    ClUint32T height;
    ClUint32T shift;
    ClRadixTreeNodeT **slot = NULL;
    ClRadixTreeNodeT *node = NULL;
    ClRcT rc = CL_ERR_NOT_EXIST;

    if(!root || !item) 
    {
        rc = CL_ERR_INVALID_PARAMETER;
        goto out;
    }

    *item = NULL;

    if(!index)
    {
        if(!root->firstSlot)
            goto out;
        *item = root->firstSlot;
        rc = CL_OK;
        goto out;
    }

    if(index > radixTreeMaxIndex(root->height))
    {
        goto out;
    }
    
    if(!root->node) 
    {
        goto out;
    }

    node = root->node;
    height = root->height;
    shift = (height - 1) * RADIX_TREE_MAP_SHIFT;
    do
    {
        slot = (ClRadixTreeNodeT**)node->slots + ((index >> shift) & RADIX_TREE_MAP_MASK);
        node = *slot;
        if(!node) goto out;
        shift -= RADIX_TREE_MAP_SHIFT;
        --height;
    } while(height > 0);

    *item = node;
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clRadixTreeLookup(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT *item)
{
    return __clRadixTreeLookup((ClRadixTreeRootT*)handle, index, item);
}

static void radixTreeShrink(ClRadixTreeRootT *root)
{
    while(root->height > 0 )
    {
        ClRadixTreeNodeT *node = root->node;
        void *nextEntry = NULL;
        if(node->count != 1) break;
        if(!(nextEntry = node->slots[0])) break;
        root->node = nextEntry;
        --root->height;
        node->slots[0] = NULL;
        node->count = 0;
        clHeapFree(node);
    }
}

ClRcT clRadixTreeDelete(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT *item)
{
    ClRadixTreeRootT *root = (ClRadixTreeRootT*)handle;
    ClRadixTreePathT path[RADIX_TREE_MAX_PATH+1];
    ClRadixTreePathT *pathp = path;
    ClUint32T height;
    ClUint32T shift;
    ClUint32T offset = 0;
    ClRadixTreeNodeT *slot = NULL;
    ClRadixTreeNodeT *toFree = NULL;
    ClRcT rc = CL_ERR_NOT_EXIST;

    if(!root || !item)
    {
        rc = CL_ERR_INVALID_PARAMETER;
        goto out;
    }

    if(index > radixTreeMaxIndex(root->height))
    {
        goto out;
    }

    *item = NULL;

    if(!index)
    {
        if(!root->firstSlot)
            goto out;
        *item = root->firstSlot;
        root->firstSlot = NULL;
        rc = CL_OK;
        goto out;
    }

    slot = root->node;
    height = root->height;
    shift = (height-1)*RADIX_TREE_MAP_SHIFT;
    pathp->node = NULL;
    do
    {
        if(!slot) goto out;
        ++pathp;
        offset = (index >> shift) & RADIX_TREE_MAP_MASK;
        pathp->offset = offset;
        pathp->node = slot;
        slot = slot->slots[offset];
        shift -= RADIX_TREE_MAP_SHIFT;
        --height ;
    } while(height > 0);

    rc = CL_OK;
    *item = slot;
    toFree = NULL;

    while(pathp->node)
    {
        pathp->node->slots[pathp->offset] = NULL;
        --pathp->node->count;
        
        if(toFree) clHeapFree(toFree);

        if(pathp->node->count)
        {
            if(pathp->node == root->node) 
                radixTreeShrink(root);
            goto out;
        }

        toFree = pathp->node;
        --pathp;
    }

    root->height = 0;
    root->node = NULL;
    if(toFree) clHeapFree(toFree);
    
    out:
    return rc;
}

ClRcT clRadixTreeInit(ClRadixTreeHandleT *handle)
{
    ClRadixTreeRootT *root = NULL;
    if(!handle) return CL_ERR_INVALID_PARAMETER;
    root = clHeapCalloc(1, sizeof(*root));
    CL_ASSERT(root != NULL);
    root->height = 0;
    *handle = (ClRadixTreeHandleT)root;
    heightIndexMapSet();
    return CL_OK;
}
