/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgFailover.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clTimerApi.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clIocConfig.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clIdlApi.h>
#include <clRmdIpi.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>

#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgDatabase.h>
#include <clMsgQueue.h>
#include <clMsgEo.h>
#include <clMsgFailover.h>
#include <clMsgDebugInternal.h>
#include <clMsgIdl.h>


#define CL_MSG_FIN_BLOCK_TIME           (10)  /*seconds*/
#define CL_MSG_EVT_UP_CHECK_PERIODE     (3)  /*seconds*/
#define CL_MSG_EVT_UP_CHECK_RETRIES     (3)


ClMsgMoveStatusT gMsgMoveStatus;
ClIocNodeAddressT gQMoveDestNode;
static ClUint32T gMsgNumOfOpenQs;
ClOsalMutexT gFinBlockMutex;
static ClOsalCondIdT gFinBlockCond;
static ClEventInitHandleT gCpmChHdl;
static ClEventHandleT gMsgEvtHdl;
static ClTimerHandleT gMsgEvtTimerHdl;


static ClRcT clMsgNextNodeGet(ClIocNodeAddressT node, ClIocNodeAddressT *pNextNode)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT i;
    ClStatusT status;

    for(i = node - 1; i >= CL_IOC_MIN_NODE_ADDRESS ; i--)
    {
        rc = clCpmNodeStatusGet(i, &status);
        if(rc != CL_OK)
        {
            clLogError("NOD", "STA", "Failed to get node status for node [0x%x]. error code [0x%x].", i, rc);
            continue;
        }
        else if(status == CL_STATUS_DOWN)
        {
            continue;
        }

        *pNextNode = i;
        goto out;
    }

    for(i = node + 1; i <= CL_IOC_MAX_NODE_ADDRESS ; i++)
    {
        rc = clCpmNodeStatusGet(i, &status);
        if(rc != CL_OK)
        {
            clLogError("NOD", "STA", "Failed to get node status for node [0x%x]. error code [0x%x].", i, rc);
            continue;
        }
        else if(status == CL_STATUS_DOWN)
        {
            continue;
        }

        *pNextNode = i;
        goto out;
    }

    *pNextNode = node;
    clLogTrace("NOD", "STA", "This node is going down. Message queue failover node is [0x%x].", node);

out:
    return rc;
}


ClRcT clMsgFinBlockStatusSet(ClMsgMoveStatusT status)
{
    ClRcT rc;
    register struct hashStruct *pTemp;
    ClUint32T key;
    ClMsgQueueRecordT *pMsgQEntry;
    ClMsgQueueInfoT *pQInfo;
    ClBoolT qDelete;
    ClIocNodeAddressT node;


    switch(status)
    {
        case MSG_MOVE_FIN_BLOCKED :

            if(gMsgMoveStatus == MSG_MOVE_FIN_BLOCKED)
            {
                goto out;
            }

            rc = clMsgNextNodeGet(gClMyAspAddress, &node);
            if(rc != CL_OK)
            {
                clLogError("QUE", "FAI", "Failed to get the next node's address. error code [0x%x].", rc);
                goto error_out;
            }
            else if(node == gClMyAspAddress)
            {
                clLogDebug("QUE", "FAI", "Only one/this node is present in the cluster.");
                gMsgMoveStatus = MSG_MOVE_DONE;
                clOsalCondSignal(gFinBlockCond);
                goto error_out;
            }

            gQMoveDestNode = node;
            gMsgMoveStatus = MSG_MOVE_FIN_BLOCKED;

            key = clMsgNodeQHash(gClMyAspAddress);

            CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

            for(pTemp = ppMsgNodeQHashTable[key]; pTemp ; )
            {
                pMsgQEntry = hashEntry(pTemp, ClMsgQueueRecordT, qNodeHash);
                if(pMsgQEntry->compAddr.nodeAddress == gClMyAspAddress)
                {
                    qDelete = CL_FALSE;

                    rc = clHandleCheckout(gClMsgQDatabase, pMsgQEntry->qHandle, (void**)&pQInfo);
                    if(rc != CL_OK)
                    {
                        clLogError("QUE", "FAI", "Handle checkout for Queue [%.*s] failed. error code [0x%x].", 
                                pMsgQEntry->qName.length, pMsgQEntry->qName.value, rc);
                        continue; 
                    }

                    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock); 
                    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

                    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
                    {
                        clMsgFailoverClosedQMove(pQInfo, gQMoveDestNode);
                        clLogDebug("QUE", "FAI", "Queue [%.*s] has failed over to [0x%x] node.", pMsgQEntry->qName.length, pMsgQEntry->qName.value, gQMoveDestNode);
                        qDelete = CL_TRUE;
                    }
                    else
                    {
                        gMsgNumOfOpenQs++;
                    }

                    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
                    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock); 

                    if(qDelete)
                    {
                        clMsgQueueFree(CL_TRUE, pQInfo);
                        clLogDebug("QUE", "FAI", "Queue [%.*s] is failed over and freed.\n", pMsgQEntry->qName.length, pMsgQEntry->qName.value);
                    }

                    rc = clHandleCheckin(gClMsgQDatabase, pMsgQEntry->qHandle);
                    if(rc != CL_OK)
                        clLogError("QUE", "FAI", "Handle checkout for Queue [%.*s] failed. error code [0x%x].", 
                                pMsgQEntry->qName.length, pMsgQEntry->qName.value, rc);

                    if(qDelete)
                    {
                        rc = clHandleDestroy(gClMsgQDatabase, pMsgQEntry->qHandle);
                        if(rc != CL_OK)
                            clLogError("QUE", "FAI", "Handle destroy for Queue [%.*s] failed. error code [0x%x].", 
                                    pMsgQEntry->qName.length, pMsgQEntry->qName.value, rc);

                        pTemp = ppMsgNodeQHashTable[key];
                        continue;
                    }
                }
                pTemp = pTemp->pNext;
            }

            CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

error_out:
out:
            if(gMsgNumOfOpenQs != 0)
                break;

        case MSG_MOVE_DONE:
            if(gMsgNumOfOpenQs != 0)
                gMsgNumOfOpenQs--;
            if(gMsgNumOfOpenQs == 0)
            {
                gMsgMoveStatus = MSG_MOVE_DONE;
                clOsalCondSignal(gFinBlockCond);
            }
            break;

        default:
            break;
    }

    return CL_OK;
}


void * clMsgClosedQueueMoveThread(void *pParam)
{
    ClRcT rc;
    CL_OSAL_MUTEX_LOCK(&gFinBlockMutex);
    rc = clMsgFinBlockStatusSet(MSG_MOVE_FIN_BLOCKED);
    CL_OSAL_MUTEX_UNLOCK(&gFinBlockMutex);
    if(rc != CL_OK)
        clLogError("MOV", "Thd", "Failed at block the message finalize on this node. error code [0x%x].", rc);

    return NULL;
}
    

ClRcT clMsgFinalizeBlocker(void)
{
    ClRcT rc = CL_OK;
    SaTimeT timeout = (SaTimeT)(CL_MSG_FIN_BLOCK_TIME * 1000000000LL);
    ClTimerTimeOutT tempTime = {0};

    CL_OSAL_MUTEX_LOCK(&gFinBlockMutex);

    if(gMsgMoveStatus == MSG_MOVE_DONE)
        goto out;

    clLogDebug("FIN", "BLOCK", "Message service finalize will be blocked for [%llu ns].", timeout);

    if(gMsgMoveStatus == MSG_MOVE_FIN_UNINIT)
    {
        rc = clOsalTaskCreateDetached("allClosedQueueMoverThread", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, clMsgClosedQueueMoveThread, NULL);
        if(rc != CL_OK)
        {
            clLogError("FIN", "BLOCK", "Failed to create a Queue mover thread. error code [0x%x].", rc);
            goto error_out;
        }
        clMsgTimeConvert(&tempTime, timeout);

        rc = clOsalCondWait(gFinBlockCond, &gFinBlockMutex, tempTime);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {   
            clLogError("FIN", "BLOCK", "Finalize blocking timed out. Timeout is [%lld ns]. error code [0x%x].", timeout, rc);
        }   
        else if(rc != CL_OK)
        {
            clLogError("FIN", "BLOCK", "Failed at Conditional Wait. error code [0x%x].", rc);
        }
        else
        {
            clLogDebug("FIN", "BLOCK", "Message queues are failed over to [0x%x] node.", gQMoveDestNode);
        }
    }

    gMsgMoveStatus = MSG_MOVE_DONE;

    goto out;

    error_out:
    out:
    CL_OSAL_MUTEX_UNLOCK(&gFinBlockMutex);
     
    return rc;
}


static void clMsgEventCallbackFunc(ClEventSubscriptionIdT subscriptionId, ClEventHandleT eventHandle, ClSizeT eventDataSize)
{
    ClRcT rc;
    ClCpmEventNodePayLoadT nodePayload = {{0}};

    rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, CL_CPM_NODE_EVENT, (void *)&nodePayload);
    if(rc != CL_OK)
    {
        clLogError("EVT", "Cbk", "Failed to get event payload data. error code [0x%x].", rc);
        goto error_out;
    }

    if(nodePayload.operation != CL_CPM_NODE_DEPARTURE)
        goto out;

    clLogDebug("EVT", "Cbk", "Node [0x%x] is going down. Message queues are going to be failed over.", nodePayload.nodeIocAddress);

    if(nodePayload.nodeIocAddress == gClMyAspAddress)
    {
        CL_OSAL_MUTEX_LOCK(&gFinBlockMutex);        
        rc = clMsgFinBlockStatusSet(MSG_MOVE_FIN_BLOCKED);
        CL_OSAL_MUTEX_UNLOCK(&gFinBlockMutex);
        if(rc != CL_OK)
            clLogError("EVT", "Cbk", "Failed at block the message finalize on this node. error code [0x%x].", rc);
    }

error_out:
out:
    clEventFree(eventHandle);
    return;
}


static ClRcT clMsgEventInitialize(void)
{
    ClRcT rc, retCode;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventCallbacksT msgEvtCallbacks = { NULL, clMsgEventCallbackFunc };

    ClNameT cpmEvtCh = {0};
    ClEventChannelOpenFlagsT cpmChOpenFlags = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL;

    ClUint32T nodeDepPattern = CL_CPM_NODE_DEPART_PATTERN;
    ClEventFilterT nodeDepFilter[] = {{CL_EVENT_EXACT_FILTER, {0, (ClSizeT)sizeof(nodeDepPattern), (ClUint8T*)&nodeDepPattern}}};
    ClEventFilterArrayT nodeDepFltArray = {sizeof(nodeDepFilter)/sizeof(nodeDepFilter[0]), nodeDepFilter};

    rc = clEventInitialize(&gMsgEvtHdl, &msgEvtCallbacks, &version);
    if(rc != CL_OK)
    {
        clLogError("EVT", "INI", "Failed to initialize the event client. error code [0x%x].", rc);
        goto error_out;
    }

    clNameSet(&cpmEvtCh, CL_CPM_NODE_EVENT_CHANNEL_NAME);

    rc = clEventChannelOpen(gMsgEvtHdl, &cpmEvtCh, cpmChOpenFlags, SA_TIME_MAX, &gCpmChHdl);
    if(rc != CL_OK)
    {
        clLogError("EVT", "INI", "Failed to open event channel. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clEventSubscribe(gCpmChHdl, &nodeDepFltArray, 0, NULL);
    if(rc != CL_OK)
    {
        clLogError("EVT", "INI", "Failed to subscribe for node departure event. error code [0x%x].", rc);
        goto error_out_2;
    }

    goto out;

error_out_2:
    retCode = clEventChannelClose(gCpmChHdl);
    if(retCode != CL_OK)
        clLogError("EVT", "INI", "Failed to close CPM event channel. error code [0x%x].", retCode);
error_out_1:
    retCode = clEventFinalize(gMsgEvtHdl);
    if(retCode != CL_OK)
        clLogError("EVT", "INI", "Failed to finalize event library. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}


static ClRcT clMsgEventFinalize(void)
{
    ClRcT rc;

    rc = clEventChannelClose(gCpmChHdl);
    if(rc != CL_OK)
        clLogError("EVT", "FIN", "Failed to close CPM event channel. error code [0x%x]", rc);

    rc = clEventFinalize(gMsgEvtHdl);
    if(rc != CL_OK)
        clLogError("EVT", "FIN", "Failed to finalize event library. error code [0x%x].", rc);

    return rc;
}


static ClRcT clMsgEventInitTimerThread(void *pParam)
{
    ClRcT rc;

    rc = clMsgEventInitialize();
    if(rc != CL_OK)
    {
        clLogError("FIN", "BLOCK", "Failed to initialize event client. error code [0x%x].", rc);
        goto error_out;
    }

    clLogDebug("FIN", "BLOCK", "Event library is initialized.");

    rc = clTimerDelete(&gMsgEvtTimerHdl);
    if(rc != CL_OK)
        clLogError("FIN", "BLOCK", "Failed to delete event timer. error code [0x%x].", rc);

error_out:
    return rc;
}

static ClRcT clMsgEventInitTimerStart(void)
{
    ClRcT rc;
    ClTimerTimeOutT timeout = {CL_MSG_EVT_UP_CHECK_PERIODE, 0};

    rc = clTimerCreateAndStart(timeout, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT, clMsgEventInitTimerThread, NULL, &gMsgEvtTimerHdl);
    if(rc != CL_OK)
        clLogError("FIN", "BLOCK", "Failed to create and start event inti timer. error code [0x%x].", rc);

    return rc;
}


ClRcT clMsgFinalizeBlockInit(void)
{
    ClRcT rc;
    ClRcT retCode;

    rc = clOsalCondCreate(&gFinBlockCond);
    if(rc != CL_OK)
    {   
        clLogError("FIN", "BLOCK", "Failed to create a condtional variable. error code [0x%x].", rc);
        goto error_out;
    } 

    rc = clOsalMutexInit(&gFinBlockMutex);
    if(rc != CL_OK)
    {
        clLogError("FIN", "BLOCK", "Failed to create a mutex. error code [0x%x].", rc);
        goto error_out_1;
    } 

    rc = clMsgEventInitTimerStart();
    if(rc != CL_OK)
    {
        clLogError("FIN", "BLOCK", "Failed to start event initialize timer. error code [0x%x].", rc);
        goto error_out_2;
    }

    goto out;

error_out_2:
    retCode = clOsalMutexDestroy(&gFinBlockMutex);
    if(retCode != CL_OK)
        clLogError("MSG", "BLOCK", "Failed to destroy mutex. error code [0x%x].", retCode);
error_out_1:
    retCode = clOsalCondDelete(gFinBlockCond);
    if(retCode != CL_OK)
        clLogError("MSG", "BLOCK", "Failed to delete a conditional variable. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}


ClRcT clMsgFinalizeBlockFin(void)
{
    ClRcT rc;
    
    rc = clMsgEventFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "BLOCK", "Failed to finalize event library. error code [0x%x].", rc);

    rc = clOsalMutexDestroy(&gFinBlockMutex);
    if(rc != CL_OK)
        clLogError("MSG", "BLOCK", "Failed to delete a mutex. error code [0x%x].", rc);

    rc = clOsalCondDelete(gFinBlockCond);
    if(rc != CL_OK)
        clLogError("MSG", "BLOCK", "Failed to delete a conditional variable. error code [0x%x].", rc);

    return rc;
}
