#include "clIocReliableLossList.h"



/*
** private protos
static void freeList(ClFragmentListHeadT **list);
*/

/*
**  initList()
**  initialize a list
**
**  Parameters:
**  ClFragmentListHeadT **list      list to initialize
**
**  Return Values:
**  none
**
**  Limitations and Comments:
**  none
**
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/
void initList(ClFragmentListHeadT **list)
{
    (*list)=NULL;
}


/*
**  allocateNode()
**  allocate a new node.
**
**  Parameters:
**  void    *fragmentID       a generic pointer to object fragmentID
**
**  Return Values:
**  pointer to ClFragmentListHeadT if succeeds
**  NULL otherwise
**
**  Limitations and Comments:
**  the caller must pass valid pointer to fragmentID.
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/

//ClFragmentListHeadT *allocateNode(ClUint32T fragmentID)
//{
//    ClFragmentListHeadT  *ClFragmentListHeadT;
//    ClFragmentListHeadT=(ClFragmentListHeadT *) malloc(sizeof(ClFragmentListHeadT));
//    if (ClFragmentListHeadT == (ClFragmentListHeadT *) NULL)
//    {
//        //(void) fprintf(stderr,"malloc failed at: %s\n",func);
//        return ((ClFragmentListHeadT *) NULL);
//    }
//
//    ClFragmentListHeadT->fragmentID=fragmentID;
//    ClFragmentListHeadT->pNext=NULL;
//
//    return (ClFragmentListHeadT);
//}

void appendNode(ClFragmentListHeadT **head,ClFragmentListHeadT **new)
{
    ClFragmentListHeadT
        *tmp;
    if (emptyList(*head) == CL_TRUE)
    {
        (*head)=(*new);
    }
    else
    {
        for (tmp=(*head); tmp->pNext != NULL; tmp=tmp->pNext)
            ;
        tmp->pNext=(*new);
    }
}

/*
**  appendNodeSorted()
**  appends a node to the end of a list sorting by a user defined function
**
**  Parameters:
**  ClFragmentListHeadT **head      - append at the ends of this node
**  ClFragmentListHeadT **new       - appends this node
**
**  Return Values:
**  None
**
**  Limitations and Comments:
**  new node must be allocated and initialized before passing it here
**  the function takes two arguments, void * each
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/

ClBoolT appendNodeSorted(ClFragmentListHeadT **head,ClFragmentListHeadT **new)
{
    ClFragmentListHeadT *tmp;

    if (emptyList(*head) == CL_TRUE)
    {
        (*head)=(*new);
        return CL_TRUE;
    }
    else
    {
        if ((*head)->fragmentID == (*new)->fragmentID)
        {
            return CL_FALSE;
        }
        if ((*head)->fragmentID > (*new)->fragmentID)
        {
            (*new)->pNext=(*head);
            (*head)=(*new);
            return CL_TRUE;
        }
        else
        {
            for(tmp=(*head); tmp->pNext; tmp=tmp->pNext)
            {
                if (tmp->pNext->fragmentID == (*new)->fragmentID)
                {
                    return CL_FALSE;
                }
                if (tmp->pNext->fragmentID > (*new)->fragmentID)
                    break;
            }
            (*new)->pNext=tmp->pNext;
            tmp->pNext=(*new);
        }
        return CL_TRUE;

    }
}

/*
**  insertNode()
**  insert a node at the beginning of a list
**
**  Parameters:
**  ClFragmentListHeadT **head      - modify this list
**  ClFragmentListHeadT **new       - appends this node
**
**  Return Values:
**  None
**
**  Limitations and Comments:
**  new node must be allocated and initialized before passing it here
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/

void insertNode(ClFragmentListHeadT **head,ClFragmentListHeadT **new)
{

    (*new)->pNext=(*head);
    (*head)=(*new);
}

/*
**  emptyList()
**  check if a list variable is NULL
**
**  Parameters:
**  ClFragmentListHeadT *list      list
**
**  Return Values:
**  TRUE    if empty
**  FALSE   if not empty
**
**  Limitations and Comments:
**  list must be allocated/initialized or initialized before calling
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/

ClBoolT emptyList(ClFragmentListHeadT *list)
{
    return ((list == NULL) ? CL_TRUE : CL_FALSE);
}

/*
**  delNode()
**  remove a node from a list
**
**  Parameters:
**  ClFragmentListHeadT **head      - list to modify
**  ClFragmentListHeadT *node       - node to remove
**
**  Return Values:
**  none
**
**  Limitations and Comments:
**  list is modified
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/
void delNode(ClFragmentListHeadT **head,ClFragmentListHeadT *node)
{
    if (emptyList(*head) == CL_TRUE)
        return;

    if ((*head) == node)
        (*head)=(*head)->pNext;
    else
    {
        ClFragmentListHeadT  *l;
        for (l=(*head); l != NULL && l->pNext != node; l=l->pNext);
        if (l == NULL)
            return;
        else
            l->pNext=node->pNext;
    }
    freeNode(&node);}

/*
**  freeNode()
**  frees a node
**
**  Parameters:
**  ClFragmentListHeadT **list  node to free
**
**  Return Values:
**  none
**
**  Limitations and Comments:
**  if list is not null, it wil be freed. so list better point to a valid
**  location
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-07-1998    first cut
*/

void freeNode(ClFragmentListHeadT **list)
{
    if (*list)
    {
        (void) free ((char *) (*list));
        (*list)=NULL;
    }
}


/*
**  getNthNode()
**  get nth node in a list
**
**  Parameters:
**  ClFragmentListHeadT *list       - the head list
**  int n           - return the node
**  Return Values:
**  a pointer to the list at position n
**  NULL if there's no such node at posion n
**
**  Limitations and Comments:
**  position starts at 1
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-08-1998    frist cut
*/

ClFragmentListHeadT *getNthNode(ClFragmentListHeadT *list,int n)
{
    ClFragmentListHeadT
        *lp=NULL;
    int
        j=0;

    for (lp=list; lp; lp=lp->pNext)
    {
        j++;
        if (j == n)
        {
            return (lp);
        }
    }

    return ((ClFragmentListHeadT *) NULL);
}

ClFragmentListHeadT *getNodebyValue(ClFragmentListHeadT *list,ClUint32T n)
{
    ClFragmentListHeadT
        *lp=NULL;
    if(list->fragmentID==n)
    {
        return list;
    }
    for (lp=list; lp; lp=lp->pNext)
    {
        ClUint32T j= lp->fragmentID;
        if (j == n)
        {
            return (lp);
        }
    }
    return ((ClFragmentListHeadT *) NULL);
}

void destroyNode(ClFragmentListHeadT **list,ClFragmentListHeadT *node)
{
    if (emptyList(node) == CL_FALSE)
    {
        /*
        ** destroy the fragmentID
        */
        delNode(list,node);
    }
}

/*
**  destroyNodes()
**  destroy the entire linked list and the fragmentID
**
**  Parameters:
**  ClFragmentListHeadT **head      - head node of the list
**  freeFunc        - function to free fragmentID
**
**  Return Values:
**  none
**
**  Limitations and Comments:
**  whole list and the fragmentID associated are freed
**
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-08-1998    first cut
*/

/*
void destroyNodes(ClFragmentListHeadT **head,void (*freeFunc)(void **))
*/
void destroyNodes(ClFragmentListHeadT **head)
{
    ClFragmentListHeadT *lp;
    while (*head)
    {
        lp=(*head);
        (*head)=(*head)->pNext;
        (void) free((char *) lp);
    }
}

/*
**  numNodes()
**  returns number of nodes in the list
**
**  Parameters:
**  ClFragmentListHeadT **head      - the head node of the list
**
**  Return Values:
**  number of node/s
**
**  Limitations and Comments:
**  traverse the whole list, so not efficient
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Aug-09-1998    first cut
*/

int numNodes(ClFragmentListHeadT **head)
{
    int n=0;

    ClFragmentListHeadT *lp;

    for (lp=(*head); lp; lp=lp->pNext)
    {
        n++;
    }

    return (n);
}

ClUint32T lossListInsertRange(ClFragmentListHeadT **lossList, ClUint32T seqno1, ClUint32T seqno2)
{
    ClUint32T i;
    if(seqno1==seqno2)
    {
        struct ClFragmentListHeadT *temp;
        temp=(struct ClFragmentListHeadT *)malloc(sizeof(ClFragmentListHeadT));
        temp->fragmentID=seqno2;
        temp->pNext=NULL;
        ClBoolT ret;
        ret = appendNodeSorted(lossList,&temp);
        if (ret==CL_TRUE)
            return 1;
        return 0;
    }
    ClUint32T count=0;
    for(i = seqno1; i<= seqno2; i++)
    {

        struct ClFragmentListHeadT *temp;
        temp=(struct ClFragmentListHeadT *)malloc(sizeof(ClFragmentListHeadT));
        temp->fragmentID=i;
        temp->pNext=NULL;
        ClBoolT ret;
        ret = appendNodeSorted(lossList,&temp);
        if (ret==CL_TRUE)
            count++;
    }
    return count;
}

ClBoolT lossListDelete(ClFragmentListHeadT **pHead,ClUint32T num)
{
    if (*pHead == NULL)
    {
        return CL_FALSE;
    }
    ClFragmentListHeadT *node=getNodebyValue(*pHead,num);
    if (node != NULL)
    {
        destroyNode(pHead,node);
        return CL_TRUE;
    }
    return CL_FALSE;

}

ClUint32T lossListGetFirst(ClFragmentListHeadT *pHead)
{
    if(pHead)
    {
        return pHead->fragmentID;
    }
    else
    {
        return 0;
    }

}

ClUint32T lossListCount(ClFragmentListHeadT **head)
{
    return numNodes(head);
}
