#ifndef _CL_RBTREE_H_
#define _CL_RBTREE_H_

#include <stdio.h>
#include <clDebugApi.h>
#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClRbTree
{
#define CL_RBTREE_RED (1)
#define CL_RBTREE_BLACK (2)

#define CL_RBTREE_INSERTED (0x1)
#define CL_RBTREE_DELETED  (0x2)

    struct ClRbTree *left;
    struct ClRbTree *right;
    struct ClRbTree *parent;
    ClUint32T colour;
    ClUint32T flags;
}ClRbTreeT;

typedef struct ClRbTreeRoot
{
    ClRbTreeT *root;
    ClUint32T nodes;
    ClInt32T (*compare)(ClRbTreeT *, ClRbTreeT *);
}ClRbTreeRootT;

typedef ClInt32T (*ClRbTreeCmpT)(ClRbTreeT *, ClRbTreeT *);

#define CL_RBTREE_INITIALIZER(tree, cmp) { .root = NULL, .nodes = 0, .compare = cmp }

#define CL_RBTREE_DECLARE(tree, cmp)                                    \
    ClRbTreeRootT tree = CL_RBTREE_INITIALIZER(tree, cmp)

extern void clRbTreeInsertColour(ClRbTreeRootT *root, ClRbTreeT *node);
extern void __clRbTreeDelete(ClRbTreeRootT *root, ClRbTreeT *node);

static __inline__ void clRbTreeInit(ClRbTreeRootT *root, ClInt32T (*cmp)(ClRbTreeT *, ClRbTreeT *))
{
    root->root = NULL;
    root->nodes = 0;
    root->compare = cmp;
}

static __inline__ ClRbTreeCmpT clRbTreeSetCompare(ClRbTreeRootT *root,
                                                  ClInt32T (*newCmp)(ClRbTreeT *, ClRbTreeT*))
{
    ClInt32T (*oldCmp)(ClRbTreeT*, ClRbTreeT*) = root->compare;
    root->compare = newCmp;
    return oldCmp;
}


static __inline__ void clRbTreeInsert(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT **pos = &root->root;
    ClRbTreeT *parent = NULL;
    ClInt32T cmp = 0;

    CL_ASSERT(root->compare != NULL);

    if( (node->flags & CL_RBTREE_INSERTED) ) return;

    while(*pos)
    {
        parent = *pos;
        if ( (cmp = root->compare(node, parent) ) <= 0 )
        {
            pos = &parent->left;
        }
        else
        {
            pos = &parent->right;
        }
    }

    *pos = node;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    node->colour = CL_RBTREE_RED;
    ++root->nodes;
    clRbTreeInsertColour(root, node);
    node->flags &= ~CL_RBTREE_DELETED;
    node->flags |= CL_RBTREE_INSERTED;
}

static __inline__ void clRbTreeDelete(ClRbTreeRootT *root, ClRbTreeT *node)
{
    if( !(node->flags & CL_RBTREE_INSERTED) ) return;
    if( (node->flags & CL_RBTREE_DELETED) ) return;
     --root->nodes;
    __clRbTreeDelete(root, node);
    node->flags &= ~CL_RBTREE_INSERTED;
    node->flags |= CL_RBTREE_DELETED;
}

static __inline__ ClRbTreeT *clRbTreeMinMax(ClRbTreeRootT *root, ClInt32T cmp)
{
    ClRbTreeT *entry = root->root;
    ClRbTreeT *entryLast = NULL;
    while(entry)
    {
        entryLast = entry;
        if(cmp > 0 )
        {
            entry = entry->right;
        }
        else
        {
            entry = entry->left;
        }
    }
    return entryLast;
}

/*
 * Get the next node for the given node.
 */
static __inline__ ClRbTreeT *clRbTreeNext(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *next = NULL;
    next = node->right;
    if(next)
    {
        ClRbTreeT *left;
        while ((left = next->left)) next = left;
    }
    else
    {
        while(node->parent && node->parent->right == node)
            node = node->parent;
        next = node->parent; 
    }
    return next;
}

static __inline__ ClRbTreeT *clRbTreeFind(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *entry = root->root;
    ClInt32T cmp = 0;;
    CL_ASSERT(root->compare != NULL);
    while(entry)
    {
        if ( (cmp = root->compare(node, entry)) < 0 )
        {
            entry = entry->left;
        }
        else if(cmp > 0)
        {
            entry = entry->right;
        }
        else 
        {
            return entry;
        }
    }
    return NULL;
}

#define clRbTreeMin(root) clRbTreeMinMax(root, 0)

#define clRbTreeMax(root) clRbTreeMinMax(root, 1)

#define CL_RBTREE_ENTRY(element, cast, field)                           \
    (cast *) ( (ClUint8T*)(element) - (ClWordT)(&((cast*)0)->field) )

#define CL_RBTREE_FOR_EACH(iter, root)                                  \
    for( iter = clRbTreeMin(root); iter; iter = clRbTreeNext(root, iter)) 


#ifdef __cplusplus
}
#endif

#endif
