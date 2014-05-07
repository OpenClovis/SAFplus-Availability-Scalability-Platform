/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.
 */


#include <clIocReliableLossList.h>


void lossListAppend(ClFragmentListHeadT *pHead,ClUint32T num)
{
    struct ClFragmentListHeadT *temp,*right;
    temp= (struct ClFragmentListHeadT *)malloc(sizeof(ClFragmentListHeadT));
    temp->fragmentID = num;
    right=(struct ClFragmentListHeadT*)pHead;
    while(right->pNext != NULL)
        right=right->pNext;
    right->pNext =temp;
    right=temp;
    right->pNext=NULL;
}

void lossListAdd(ClFragmentListHeadT *pHead,ClUint32T num)
{
    struct ClFragmentListHeadT *temp;
    temp=(struct ClFragmentListHeadT *)malloc(sizeof(ClFragmentListHeadT));
    temp->fragmentID=num;
    //temp->messageKey=key;
    if (pHead==NULL)
    {
        pHead=temp;
        pHead->pNext=NULL;
    }
    else
    {
        temp->pNext=pHead;
        pHead=temp;
    }
}

void lossListAddAfter(ClFragmentListHeadT *pHead,ClUint32T num, ClUint32T loc)
{
    ClUint32T i;
    struct ClFragmentListHeadT *temp,*right = NULL;
    struct ClFragmentListHeadT *left = NULL;        
    right=pHead;
    for(i=1;i<loc;i++)
    {
        left=right;
        right=right->pNext;
    }
    temp=(struct ClFragmentListHeadT *)malloc(sizeof( ClFragmentListHeadT));
    temp->fragmentID=num;
    left->pNext=temp;
    left=temp;
    left->pNext=right;
    return;
}

ClUint32T lossListCount(ClFragmentListHeadT *head)
{
    struct ClFragmentListHeadT *n;
    ClUint32T c=0;
    n=head;
    while(n!=NULL)
    {
        n=n->pNext;
        c++;
    }
    return c;
}

ClUint32T lossListGetFirst(ClFragmentListHeadT *head)
{
    return head->fragmentID;
}

/*
 *Functionality : Insert a  loss elements into the loss  list.
 */
void lossListInsert(ClFragmentListHeadT *pHead,ClUint32T num)
{
    ClUint32T c=0;
    struct ClFragmentListHeadT *temp;
    temp=pHead;
    if(temp==NULL)
    {
        lossListAdd(pHead,num);
    }
    else
    {
        while(temp!=NULL)
        {
            if(temp->fragmentID<=num)
            {      
                c++;
                if(temp->fragmentID==num)// && temp->messageKey==key)
                {
                    return;
                }
            }
            temp=temp->pNext;            
        }
        if(c==0)
            lossListAdd(pHead,num);
        else if(c < lossListCount(pHead))
            lossListAddAfter(pHead,num,++c);
        else
            lossListAppend(pHead,num);
    }
}

ClUint32T lossListDelete(ClFragmentListHeadT *pHead,ClUint32T num)
{
    struct ClFragmentListHeadT *temp, *prev = NULL;
    temp=pHead;
    while(temp!=NULL)
    {
        if(temp->fragmentID==num)
        {
            if(temp==pHead)
            {
                pHead=temp->pNext;
                free(temp);
                return 1;
            }
            else
            {
                prev->pNext=temp->pNext;
                free(temp);
                return 1;
            }
        }
        else
        {
            prev=temp;
            temp= temp->pNext;
        }
    }
    return 0;
}

void lossListInsertRange(ClFragmentListHeadT *lossList, ClUint32T seqno1, ClUint32T seqno2)
{
    ClUint32T i;
    if(seqno1==seqno2)
    {
        lossListInsert(lossList,seqno1);
        return;
    }
    for(i = seqno1; i< seqno2; i++)
    {
        lossListInsert(lossList,i);
    }
}

void sendLossListRemoveRange(ClFragmentListHeadT *lossList,ClUint32T seqno1, ClUint32T seqno2)
{
    if(seqno1==seqno2)
    {
        lossListDelete(lossList,seqno1);
        return;
    }
    ClUint32T i;
    for(i = seqno1; i< seqno2; i++)
    {
        lossListDelete(lossList,i);
    }
}

