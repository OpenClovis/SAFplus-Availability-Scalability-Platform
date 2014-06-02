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
#ifndef __CL_IOC_RELIABLE_LOSS_LIST_H__
#define __CL_IOC_RELIABLE_LOSS_LIST_H__

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clList.h>
#include <clHash.h>
#include <clIocApi.h>
#include <clIocParseConfig.h>
#include <clParserApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clVersionApi.h>
#include <clIocErrors.h>
#include <clIocIpi.h>
#include <clNodeCache.h>
#include <clTransport.h>
#include <clIocSetup.h>
#include <clIocGeneral.h>
#include <clRbTree.h>
#include <clIocManagementApi.h>
#include <clIocNeighComps.h>
#include <clIocUserApi.h>
#include <clClmTmsCommon.h>
#include <clCpmExtApi.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClFragmentListHeadT
{
    ClUint32T fragmentID;
    struct ClFragmentListHeadT *pNext;
}ClFragmentListHeadT;

ClFragmentListHeadT         *allocateNode       (void *data);
ClBoolT        appendNodeSorted    (ClFragmentListHeadT **head,ClFragmentListHeadT **new);
void        appendNode          (ClFragmentListHeadT **list,ClFragmentListHeadT **new);
void        delNode             (ClFragmentListHeadT **list,ClFragmentListHeadT *node);
void        destroyNode         (ClFragmentListHeadT **list,ClFragmentListHeadT *node);
void        destroyNodes        (ClFragmentListHeadT **head);
ClBoolT        emptyList           (ClFragmentListHeadT *list);
void        freeNode            (ClFragmentListHeadT **list);
ClFragmentListHeadT         *getNthNode         (ClFragmentListHeadT *list,int n);
void        initList            (ClFragmentListHeadT **list);
void        insertNode          (ClFragmentListHeadT **list,ClFragmentListHeadT **new);
int         numNodes            (ClFragmentListHeadT **head);

//void lossListAppend(ClFragmentListHeadT **pHead,ClUint32T num);
//void lossListAdd(ClFragmentListHeadT **pHead,ClUint32T num);
//void lossListAddAfter(ClFragmentListHeadT **pHead,ClUint32T num, ClUint32T loc);
ClUint32T lossListCount(ClFragmentListHeadT **head);
//void lossListInsert(ClFragmentListHeadT **pHead,ClUint32T num);
ClBoolT lossListDelete(ClFragmentListHeadT **pHead,ClUint32T num);
ClUint32T lossListInsertRange(ClFragmentListHeadT **lossList, ClUint32T seqno1, ClUint32T seqno2);
//void sendLossListRemoveRange(ClFragmentListHeadT *lossList,ClUint32T seqno1, ClUint32T seqno2);
//ClUint32T senderBufferLossListGetFirst(ClIocAddressT *destAddress,ClUint32T messageId,ClIocPortT portId);
ClUint32T lossListGetFirst(ClFragmentListHeadT *pHead);


#ifdef __cplusplus
}
#endif

#endif
