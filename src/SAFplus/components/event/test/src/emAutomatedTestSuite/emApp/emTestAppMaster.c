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
/*******************************************************************************
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emApp/emTestAppMaster.c $
 * $Author: bkpavan $
 * $Date: 2006/12/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include "string.h"
#include "clDebugApi.h"
#include "clCpmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include "clEventApi.h"
#include "clEventExtApi.h"
#include "../common/emAutoCommon.h"
#include "../common/emTestPatterns.h"
#include <clEventUtilsIpi.h>
#include <clEventClientIpi.h>

ClCntHandleT gChanHandleInitInfo;
extern ClRcT clEventChannelHandleGet(ClEventInitHandleT evtHandle,
        const ClNameT *pChannelName,
        ClEventChannelOpenFlagsT
        evtChannelOpenFlag,
        ClEventChannelHandleT *pEvtChannelHandle);

ClEvtContResultT gEvtResult;

void clEvtTestAppDeliverCallback(ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle,
        ClSizeT eventDataSize)
{
    ClRcT rc = CL_OK;

    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    ClNameT publisherName = { 0 };
    ClEventIdT eventId = 0;

    ClUint8T payLoad[50];
    ClSizeT payLoadLen = sizeof(payLoad);

    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    void *pCookie = NULL;

    rc = clEventCookieGet(eventHandle, &pCookie);

    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
            &retentionTime, &publisherName, &publishTime,
            &eventId);

    rc = clEventDataGet(eventHandle, (void *) payLoad, &payLoadLen);

    if (0 == gEvtResult.noOfSubs)
    {
        gEvtResult.priority = priority;
        memcpy(gEvtResult.pubName.value, publisherName.value,
                publisherName.length);
        gEvtResult.pubName.length = publisherName.length;
    }

    gEvtResult.noOfSubs++;

    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n");
    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("             Subscription ID   : 0x%X\n", subscriptionId);
    clOsalPrintf("             Event Priority    : 0x%X\n", priority);
    clOsalPrintf("             Retention Time    : 0x%X\n", retentionTime);
    clOsalPrintf("             Publisher Name    : %.*s\n",
            publisherName.length, publisherName.value);
    clOsalPrintf("             EventID           : 0x%X\n", eventId);
    clOsalPrintf("             Payload           : %s\n", payLoad);
    clOsalPrintf("             Payload Len       : 0x%X\n",
            (ClUint32T) payLoadLen);
    clOsalPrintf("             Event Data Size   : 0x%X\n",
            (ClUint32T) eventDataSize);
    clOsalPrintf("             Cookie            : 0x%X\n", pCookie);
    clOsalPrintf("-------------------------------------------------------\n");

    return;
}

ClEventInitHandleT gEvtTestInitHandle;
ClCntHandleT gEvtTestInitInfo;
ClOsalMutexIdT gEvtTestInitMutex;
ClOsalMutexIdT gChanHandleDBMutex;
ClEventHandleT gEvtTestEventHandle = 0;

ClEventCallbacksT gEvtTestCallbacks = {
    NULL,
    clEvtTestAppDeliverCallback
};

#if 0
typedef struct ClEvtTestInitInfo
{
    ClEventInitHandleT evtHandle;
    ClCntHandleT initInfoContHdl;
    ClOsalMutexIdT initInfoMutex;

} ClEvtTestInitInfoT;

ClEvtTestInitInfoT gEvtTestInitInfo;
#endif


ClRcT clEvtTestAppInit(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClVersionT version = CL_EVENT_VERSION;

    ClNameT *pInitKey = NULL;
    ClUint32T initDataLen = sizeof(ClNameT);    /* Since we know the size */

    pInitKey = clHeapAllocate(sizeof(*pInitKey)); /* Allocate memory as it's used 
                                                 * as key for the container */

    rc = clBufferNBytesRead(inMsg, (ClUint8T *) pInitKey,
            (ClUint32T *) &initDataLen);

    rc = clEventInitialize(&gEvtTestInitHandle, &gEvtTestCallbacks, &version);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event Initialize Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    ClCntDataHandleT initHandle = (ClCntDataHandleT)(ClWordT)gEvtTestInitHandle;
    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntNodeAdd(gEvtTestInitInfo, (ClCntKeyHandleT) pInitKey,
             initHandle, NULL);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n\r\nFailed to add the node into Init List, rc[0x%X]\n\r\n",
                 rc));
        return rc;
    }

    return CL_OK;
}

ClRcT clEvtTestAppFin(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;

    ClNameT *pInitKey = NULL;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pInitKey);
    rc = clBufferDelete(&inMsg);

    ClCntDataHandleT initHandle = NULL;
    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo, (ClCntKeyHandleT) pInitKey,
            (ClCntDataHandleT *) &initHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    } 
    gEvtTestInitHandle = (ClEventInitHandleT)(ClWordT)initHandle;

    rc = clEventFinalize(gEvtTestInitHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Event Finalize Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));
    rc = clCntAllNodesForKeyDelete(gEvtTestInitInfo,
            (ClCntKeyHandleT) pInitKey);
    clOsalMutexUnlock(gEvtTestInitMutex);
    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntAllNodesDelete(gChanHandleInitInfo);
    clOsalMutexUnlock(gChanHandleDBMutex);
    return CL_OK;
}

ClRcT clEvtTestAppOpen(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClChanKeyT * pChanKey = NULL;
    ClEventChannelHandleT evtChannelHandle;
    ClEvtContChOpenT *pOpenReq;
    ClCntDataHandleT initHandle = NULL;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pOpenReq);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pOpenReq->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }
    gEvtTestInitHandle = (ClEventInitHandleT)(ClWordT)initHandle;

    rc = clEventChannelOpen(gEvtTestInitHandle, &pOpenReq->channelName,
            pOpenReq->openFlag, pOpenReq->timeOut,
            &evtChannelHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event Channel Open Failed, rc[0x%X]", rc));
    }
    pChanKey = clHeapAllocate(sizeof(*pChanKey));
    pChanKey->channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pOpenReq->openFlag);
    pChanKey->channelName = pOpenReq->channelName; 
    clOsalMutexLock(gChanHandleDBMutex);

    ClCntDataHandleT channelHdl = (ClCntDataHandleT)(ClWordT)evtChannelHandle;
    rc = clCntNodeAdd(gChanHandleInitInfo,
            (ClCntKeyHandleT)pChanKey,
            channelHdl, NULL);
    clOsalMutexUnlock(gChanHandleDBMutex);
    if(rc!=CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Adding to the Channel handle list failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppClose(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClEventChannelHandleT channelHandle;
    ClEvtContChCloseT *pCloseReq;
    ClChanKeyT ChanKey;
    rc = clBufferFlatten(inMsg, (ClUint8T **) &pCloseReq);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pCloseReq->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pCloseReq->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pCloseReq->openFlag);

    ClCntDataHandleT channelHdl = NULL;
    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            &channelHdl);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet Failed, rc[0x%X]",rc));
        return rc;
    }
    channelHandle = (ClEventChannelHandleT)(ClWordT)channelHdl;

    rc = clEventChannelClose(channelHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event Channel Close Failed, rc[0x%X]", rc));
    }
    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntAllNodesForKeyDelete(gChanHandleInitInfo,
            (ClCntKeyHandleT) &ChanKey);
    clOsalMutexUnlock(gChanHandleDBMutex);                                   
    if(rc!=CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Node Deletion for Key Failed, rc[0x%X]",rc));
        return rc;
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppFilterGet(ClUint32T filterNo, ClEventFilterArrayT **ppArray)
{
    if (gEmTestAppFilterDbSize < filterNo)
    {
        clOsalPrintf("Filter is not present in filter Db :: %d \r\n", filterNo);
        return CL_EVENT_ERR_INIT_NOT_DONE;
    }
    *ppArray = (ClEventFilterArrayT *) gEmTestAppFilterDb[filterNo];

    return CL_OK;
}

ClRcT clEvtTestAppPatternGet(ClUint32T patternNo,
        ClEventPatternArrayT **ppArray)
{
    if (gEmTestAppPatternDbSize < patternNo)
    {
        return CL_EVENT_ERR_INIT_NOT_DONE;
    }
    *ppArray = (ClEventPatternArrayT *) gEmTestAppPatternDb[patternNo];

    return CL_OK;
}

ClRcT clEvtTestAppSub(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle;
    ClEvtContSubT *pSubReq = NULL;
    ClEventFilterArrayT *pFilters;
    ClChanKeyT ChanKey;
    rc = clBufferFlatten(inMsg, (ClUint8T **) &pSubReq);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pSubReq->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nclCntDataForKeyGet Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pSubReq->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pSubReq->openFlag);

    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            (ClCntDataHandleT *)&channelHandle);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntNodeFind Failed, rc[0x%X]",rc));
        return rc;
    }

    rc = clEvtTestAppFilterGet(pSubReq->filterNo, &pFilters);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Filter Get Failed, rc[0x%X]", rc));
        return rc;
    }

    rc = clEventSubscribe(channelHandle, pFilters, pSubReq->subId,
            pSubReq->pCookie);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event Subscribe Failed, rc[0x%X]", rc));
    }

    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppUnsub(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClEvtContUnsubT *pUnsubReq = NULL;
    ClEventChannelHandleT channelHandle;
    ClChanKeyT ChanKey;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pUnsubReq);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pUnsubReq->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pUnsubReq->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pUnsubReq->openFlag);

    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            (ClCntDataHandleT *)&channelHandle);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet Failed, rc[0x%X]",rc));
        return rc;
    }

    rc = clEventUnsubscribe(channelHandle, pUnsubReq->subId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event Unsubscribe Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppPub(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClEvtContPubT *pPubReq;
    ClEventChannelHandleT channelHandle;
    ClEventIdT eventId;
    ClChanKeyT ChanKey;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pPubReq);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pPubReq->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pPubReq->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pPubReq->openFlag);

    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            (ClCntDataHandleT *)&channelHandle);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet Failed, rc[0x%X]",rc));
        return rc;
    }

    rc = clEventPublish(gEvtTestEventHandle, pPubReq->payLoad, pPubReq->dataLen,
            &eventId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Event Publish Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));
    return CL_OK;
}

ClRcT clEvtTestAppAttSet(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClEvtContAttrSetT *pAttr = NULL;
    ClEventChannelHandleT channelHandle;
    ClEventPatternArrayT *pPattern = NULL;
    ClChanKeyT ChanKey;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pAttr);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pAttr->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pAttr->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pAttr->openFlag);

    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            (ClCntDataHandleT *)&channelHandle);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet Failed, rc[0x%X]",rc));
        return rc;
    }

    rc = clEvtTestAppPatternGet(pAttr->pattNo, &pPattern);
    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clEventAttributesSet(gEvtTestEventHandle, pPattern, pAttr->priorityNo,
            pAttr->retentionTime, &pAttr->publisherName);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Event AttributesSet Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppEvtAlloc(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClEvtContAllocT *pAlloc = NULL;
    ClEventChannelHandleT channelHandle;
    ClChanKeyT ChanKey;

    rc = clBufferFlatten(inMsg, (ClUint8T **) &pAlloc);
    rc = clBufferDelete(&inMsg);

    clOsalMutexLock(gEvtTestInitMutex);
    rc = clCntDataForKeyGet(gEvtTestInitInfo,
            (ClCntKeyHandleT) &pAlloc->initName,
            (ClCntDataHandleT *) &gEvtTestInitHandle);
    clOsalMutexUnlock(gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nNode Find in Init List Failed, rc[0x%X]\n\r\n", rc));
        return rc;
    }

    clEvtUtilsNameCpy(&ChanKey.channelName,  &pAlloc->channelName);

    ChanKey.channelscope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(pAlloc->openFlag);

    clOsalMutexLock(gChanHandleDBMutex);
    rc = clCntDataForKeyGet(gChanHandleInitInfo, (ClCntKeyHandleT)&ChanKey, 
            (ClCntDataHandleT*)&channelHandle);
    clOsalMutexUnlock(gChanHandleDBMutex);                      
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet Failed, rc[0x%X]",rc));
        return rc;
    }

    rc = clEventAllocate(channelHandle, &gEvtTestEventHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Event Allocate Failed, rc[0x%X]", rc));
    }
    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &rc, sizeof(ClRcT));

    return CL_OK;
}

ClRcT clEvtTestAppResultGet(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;

    rc = clBufferNBytesWrite(outMsg, (ClUint8T *) &gEvtResult,
            sizeof(ClEvtContResultT));

    return CL_OK;
}

ClRcT clEvtTestAppReset(ClUint32T cData, ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    gEvtResult.noOfSubs = 0;

    return CL_OK;
}
