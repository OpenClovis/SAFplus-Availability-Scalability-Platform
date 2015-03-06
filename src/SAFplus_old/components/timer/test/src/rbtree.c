/*
 * Faster than using the container rb trees.
 */
#include "clTimerTestRbtree.h"

/*
 * Left rotation of the node.
 */
static void clRbTreeRotateLeft(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *rightLeft = node->right->left;
    ClRbTreeT *right = node->right;
    if( (node->right = rightLeft) )
    {
        rightLeft->parent = node;
    }
    if((right->parent = node->parent))
    {
        if(node->parent->left == node)
            node->parent->left = right;
        else
            node->parent->right = right;
    }
    else root->root = right;

    node->parent = right;
    right->left = node;
}

static void clRbTreeRotateRight(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *leftRight = node->left->right;
    ClRbTreeT *left = node->left;
    
    if( (node->left = leftRight) )
        leftRight->parent = node;

    if( (left->parent = node->parent) )
    {
        if(node->parent->left == node)
            node->parent->left = left;
        else
            node->parent->right = left;
    }
    else root->root = left;

    left->right = node;
    node->parent = left;
}

void clRbTreeInsertColour(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *gparent = NULL;
    ClRbTreeT *parent = NULL;
    while(node != root->root && node->parent->colour == CL_RBTREE_RED)
    {
        parent = node->parent;
        gparent = parent->parent;
        if(parent == gparent->left)
        {
            if(gparent->right && gparent->right->colour == CL_RBTREE_RED)
            {
                gparent->colour = CL_RBTREE_RED;
                gparent->right->colour = CL_RBTREE_BLACK;
                parent->colour = CL_RBTREE_BLACK;
                node = gparent;
                continue;
            }
            if(node == parent->right)
            {
                ClRbTreeT *tmp = NULL;
                clRbTreeRotateLeft(root, parent);
                tmp = node;
                node = parent;
                parent = tmp;
            }
            parent->colour = CL_RBTREE_BLACK;
            gparent->colour = CL_RBTREE_RED;
            clRbTreeRotateRight(root, gparent);
        }
        else
        {
            /* reverse left/right*/
            if(gparent->left && gparent->left->colour == CL_RBTREE_RED)
            {
                gparent->colour = CL_RBTREE_RED;
                gparent->left->colour = CL_RBTREE_BLACK;
                parent->colour = CL_RBTREE_BLACK;
                node = gparent;
                continue;
            }
            if(node == parent->left)
            {
                ClRbTreeT *tmp = NULL;
                clRbTreeRotateRight(root, parent);
                tmp = node;
                node = parent;
                parent = tmp;
            }
            parent->colour = CL_RBTREE_BLACK;
            gparent->colour = CL_RBTREE_RED;
            clRbTreeRotateLeft(root, gparent);
        }
    }
    root->root->colour = CL_RBTREE_BLACK;
}

static void clRbTreeDeleteColour(ClRbTreeRootT *root, ClRbTreeT *node, ClRbTreeT *parent)
{
    while( (!node || node->colour == CL_RBTREE_BLACK) && node != root->root)
    {
        if(node == parent->left)
        {
            ClRbTreeT *brother = parent->right;
            if(brother->colour == CL_RBTREE_RED)
            {
                parent->colour = CL_RBTREE_RED;
                brother->colour = CL_RBTREE_BLACK;
                clRbTreeRotateLeft(root, parent);
                brother = parent->right;
            }
            if((!brother->left || brother->left->colour == CL_RBTREE_BLACK)
               &&
               (!brother->right || brother->right->colour == CL_RBTREE_BLACK))
            {
                brother->colour = CL_RBTREE_RED;
                node = parent;
                parent = node->parent;
            }
            else
            { 
                if(!brother->right || brother->right->colour == CL_RBTREE_BLACK)
                {
                    brother->colour = CL_RBTREE_RED;
                    brother->left->colour = CL_RBTREE_BLACK;
                    clRbTreeRotateRight(root, brother);
                    brother = parent->right;
                }
                brother->colour = parent->colour;
                parent->colour = CL_RBTREE_BLACK;
                brother->right->colour = CL_RBTREE_BLACK;
                clRbTreeRotateLeft(root, parent);
                node = root->root;
                break;
            }
        }
        else
        {
            ClRbTreeT *brother = parent->left;
            if(brother->colour == CL_RBTREE_RED)
            {
                parent->colour = CL_RBTREE_RED;
                brother->colour = CL_RBTREE_BLACK;
                clRbTreeRotateRight(root, parent);
                brother = parent->left;
            }
            if((!brother->left || brother->left->colour == CL_RBTREE_BLACK)
               &&
               (!brother->right || brother->right->colour == CL_RBTREE_BLACK))
            {
                brother->colour = CL_RBTREE_RED;
                node = parent;
                parent = node->parent;
            }
            else
            { 
                if(!brother->left || brother->left->colour == CL_RBTREE_BLACK)
                {
                    brother->colour = CL_RBTREE_RED;
                    brother->right->colour = CL_RBTREE_BLACK;
                    clRbTreeRotateLeft(root, brother);
                    brother = parent->left;
                }
                brother->colour = parent->colour;
                parent->colour = CL_RBTREE_BLACK;
                brother->left->colour = CL_RBTREE_BLACK;
                clRbTreeRotateRight(root, parent);
                node = root->root;
                break;
            }
        }
    }

    if(node) node->colour = CL_RBTREE_BLACK;
}

void __clRbTreeDelete(ClRbTreeRootT *root, ClRbTreeT *node)
{
    ClRbTreeT *child = NULL;
    ClRbTreeT *parent = NULL;
    ClUint32T colour = 0;

    if(!node->left) child = node->right;
    else if (!node->right) child = node->left;
    else
    {
        ClRbTreeT *oldNode = node;
        ClRbTreeT *left = NULL;
        node = node->right;
        while ( (left = node->left) ) node = left;
        child = node->right;
        parent = node->parent;
        colour = node->colour;
        if(child)
            child->parent = parent;
        if(parent)
        {
            if(node == parent->left)
                parent->left = child;
            else parent->right = child;
        }
        else root->root = child;

        if(node->parent == oldNode)
            parent = node;
        node->left = oldNode->left;
        node->right = oldNode->right;
        node->colour = oldNode->colour;
        if( (node->parent = oldNode->parent) )
        {
            if(oldNode->parent->left == oldNode)
                oldNode->parent->left = node;
            else oldNode->parent->right = node;
        }
        else root->root = node;
        oldNode->left->parent = node;
        if(oldNode->right) oldNode->right->parent = node;
        goto rebalance;
    }
    parent = node->parent;
    colour = node->colour;
    if(child) child->parent = parent;
    if(parent)
    {
        if(parent->left == node)
            parent->left = child;
        else parent->right = child;
    }
    else root->root = child;

    rebalance:
    if(colour == CL_RBTREE_BLACK)
        clRbTreeDeleteColour(root, child, parent);
}
