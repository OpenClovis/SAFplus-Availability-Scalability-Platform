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
 * ModuleName  : event                                                         
 * File        : clEvtClientMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains the library support for the EM API.
 *          It provides an Interface to the EM server.
 *****************************************************************************/

#define __CLIENT__
#ifdef SOLARIS_BUILD
#include <strings.h>
#endif 
#include <clCksmApi.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clIocApi.h>
#include <clCpmApi.h>
#include <clEventClientIpi.h>
#include <clEventUtilsIpi.h>
#include <ipi/clHandleIpi.h>
#include <ipi/clUtilsIpi.h>
#include "clEventServerToClientFuncTable.h"

#include "xdrClEvtInitRequestT.h"
#include "xdrClEvtChannelOpenRequestT.h"
#include "xdrClEvtSubscribeEventRequestT.h"
#include "xdrClEvtUnsubscribeEventRequestT.h"

/* 
 * Added for getting local system time
 */
#include <sys/time.h>


/*
 * Added for the Selection Object support 
 */
#include <unistd.h>

#define  CL_EVT_MAX_SVR_RETRY     10

#undef __SERVER__
#include <clEventClientFuncTable.h>
#include <clEventServerFuncTable.h>

#ifdef VERSION_READY
# include <clVersionApi.h>

static ClVersionT gEvtAppToClientVersionsSupported[] = {
    CL_EVENT_VERSION,
    /*
     * Add further Versions
     */
};
static ClVersionDatabaseT gEvtAppToClientVersionDb = {
    sizeof(gEvtAppToClientVersionsSupported) / sizeof(ClVersionT),
    gEvtAppToClientVersionsSupported
};

static ClOsalMutexT gEvtReceiveMutex;
/*
 * Really need pthread_once abstraction in osal to avoid this shit but thats for another day
 */
static ClBoolT gEvtReceiveMutexInitialized; 

# define clEvtClientToServerVersionSet(header) (header).releaseCode = 'B', \
                                                                      (header).majorVersion = 0x1,\
(header).minorVersion = 0x1

# define clEvtVerifyAppToClientVersion(pHeader) \
    do {\
        ClRcT rc = CL_OK; \
        ClVersionT version = { (pHeader)->releaseCode, (pHeader)->majorVersion, 0 }; \
        rc = clVersionVerify(&gEvtAppToClientVersionDb, &version); \
        if(CL_OK != rc) \
        { \
            clLogError("EVT", "VER", \
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE, "App-Client", \
                    (pHeader)->releaseCode, (pHeader)->majorVersion, \
                    (pHeader)->minorVersion, version.releaseCode, \
                    version.majorVersion, version.minorVersion); \
            memcpy(pHeader, &version, sizeof(version)); \
            CL_FUNC_EXIT(); \
            return CL_EVENTS_RC(CL_EVENT_ERR_VERSION);\
        } \
        memcpy(pHeader, &version, sizeof(version)); \
    }while(0);

#endif

#define CL_EVT_INIT_MAGIC_NUMBER 0xABCD

/*
 * To Check if the EM Initialization has been done 
 */

ClUint32T gEvtInitCount = 0;

#define CL_EVT_INIT_COUNT_INC() gEvtInitCount++
#define CL_EVT_INIT_COUNT_DEC() gEvtInitCount--
#define CL_EVT_CHECK_INIT_COUNT()\
    do{\
        if(0 == gEvtInitCount)\
        {\
            clLogError("EVT", "VER", CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);\
            CL_FUNC_EXIT();\
            rc = CL_EVENT_ERR_INIT_NOT_DONE;\
            goto failure;\
        }\
    }while(0);


#define isInitDone() ((0 == gEvtInitCount)? CL_FALSE : CL_TRUE)
#define isLastFinalize() ((0 == gEvtInitCount)? CL_TRUE : CL_FALSE)


ClRcT clEvtInitHandleValidate(ClEventInitHandleT evtHandle,
        ClEvtClientHeadT *pEvtClientHead)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    /* 
     * Lookup Handle Database for this initialization Handle:evtHandle
     */
    rc = clHandleValidate(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    CL_FUNC_EXIT();
    return rc;

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "SOG", 
            "Event Initialization handle validation failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEvtInitValidate(ClEoExecutionObjT **pEoObj,
        ClEvtClientHeadT **ppEvtClientHead)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();


    rc = clEoMyEoObjectGet(pEoObj);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);
        goto failure;
    }

    rc = clEoPrivateDataGet(*pEoObj, CL_EO_EVT_EVENT_DELIVERY_COOKIE_ID,
            (void **) ppEvtClientHead);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        clLogError("EVT", "INI", 
                "EO is not initialized with EM Service [%#X]", rc);
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);
        rc = CL_EVENT_ERR_INIT_NOT_DONE;
        goto failure;
    }
    else if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                "Failed to get EO private data [%#X]", rc);
        goto failure;
    }

    CL_FUNC_EXIT();
    return rc;

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "INI", 
            "Library Initialize Validation failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

/*
 ** Function to compare the evtHandle returned by clEventInitialize.
 ** Used in creation of the container holding InitInformation.
 */
ClInt32T clEvtClientEvtHandleCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    CL_FUNC_ENTER();

    CL_FUNC_EXIT();

    return (ClWordT)key1 - (ClWordT)key2;
}

void clEvtClientInitInfoDelete(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    CL_FUNC_ENTER();
    clHeapFree(userData);
    CL_FUNC_EXIT();
    return;
}

ClInt32T clEvtClientECHUserKeyCompare(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    ClEvtClientChannelInfoT *pEvtCliChannelInfo1 =
        (ClEvtClientChannelInfoT *) key1;
    ClEvtClientChannelInfoT *pEvtCliChannelInfo2 =
        (ClEvtClientChannelInfoT *) key2;


    CL_FUNC_ENTER();

    cmpResult = pEvtCliChannelInfo1->evtHandle - pEvtCliChannelInfo2->evtHandle;

    /*
     ** When evtChannelKey is 0 it implies that we need to clean up channel information
     ** for a particular evtHandle. Hence, compare only evtHandle.
     */
    if (0 == pEvtCliChannelInfo1->evtChannelKey ||
            0 == pEvtCliChannelInfo2->evtChannelKey || 0 != cmpResult)
    {
        CL_FUNC_EXIT();
        return cmpResult;
    }
    else if (0 !=
            (cmpResult =
             pEvtCliChannelInfo1->evtChannelKey -
             pEvtCliChannelInfo2->evtChannelKey))
    {
        CL_FUNC_EXIT();
        return cmpResult;
    }

    CL_FUNC_EXIT();
    return clEvtUtilsNameCmp((&pEvtCliChannelInfo1->evtChannelName),
            (&pEvtCliChannelInfo2->evtChannelName));
}

void clEvtClientECHUserUserDelete(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    CL_FUNC_ENTER();
    clHeapFree((void *) userKey);
    CL_FUNC_EXIT();
    return;
}


/*
 * Callback related functionality 
 */

static void clEvtCbDequeueCallBack(ClQueueDataT userData)
{
    /*
     * Do Nothing 
     */
}

static void clEvtCbDestroyCallback(ClQueueDataT userData)
{
    ClEvtCbQueueDataT *pQueueData = (ClEvtCbQueueDataT *) userData;

    CL_FUNC_ENTER();

    clHeapFree(pQueueData->cbArg);
    clHeapFree(pQueueData);

    CL_FUNC_EXIT();
}


ClRcT clEvtQueueCallback(ClEvtInitInfoT *pInitInfo, ClEvtCallbackIdT cbId,
        void *cbArg)
{
    ClRcT rc = CL_OK;
    ClInt32T bytes;
    ClEvtCbQueueDataT *pQueueData = NULL;
    ClCharT chr = 'c';

    CL_FUNC_ENTER();


    pQueueData = clHeapAllocate(sizeof(ClEvtCbQueueDataT));
    if (NULL == pQueueData)
    {
        clLogError("EVT", "CBQ", 
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto failure;
    }


    /*
     * Populate the Queue with Callback Data 
     */

    pQueueData->cbId = cbId;
    pQueueData->cbArg = cbArg;

    clOsalMutexLock(pInitInfo->cbMutex);
    rc = clQueueNodeInsert(pInitInfo->cbQueue, (ClQueueDataT) pQueueData);
    clOsalMutexUnlock(pInitInfo->cbMutex);
    if (CL_OK != rc)
    {
        clLogError("EVT", "CBQ", 
                "Unable to Insert node into the Queue, rc=[0x%x]", rc);
        goto queueDataAllocated;
    }

    bytes = write(pInitInfo->writeFd, (void *) &chr, 1);
    if(bytes != 1)
        clLogError("EVT", "CBQ", "Write to dispatch pipe failed with error [%s]",
                   strerror(errno));
    CL_FUNC_EXIT();
    return rc;

queueDataAllocated:
    clHeapFree(pQueueData);

failure:
    CL_FUNC_EXIT();
    return CL_EVENTS_RC(rc);
}

static void evtEventDeliverCallbackDispatch(ClEvtInitInfoT *pInitInfo,
                                            ClUint8T version,
                                            ClEventSubscriptionIdT subscriptionId,
                                            ClEventHandleT eventHandle,
                                            ClSizeT dataSize)
                                              
{
    ClUint32T i;
    ClBoolT found = CL_FALSE;

    for(i = 0; i < pInitInfo->numCallbacks; ++i)
    {
        /*
         * If you have multiple deliver callbacks per version, invoke
         * all of them - FEATURE:-)
         */
        if(version == pInitInfo->pEvtCallbackTable[i].version 
           &&
           pInitInfo->pEvtCallbackTable[i].callbacks.clEvtEventDeliverCallback)
        {
            found = CL_TRUE;

            pInitInfo->pEvtCallbackTable[i].callbacks.clEvtEventDeliverCallback(
                                                                                subscriptionId,
                                                                                eventHandle,
                                                                                dataSize);
        }
    }

    if(!found)
    {
        clLogWarning("EVT", "DELIVER", "No event deliver callback found for version [%d]", version);
    }
}

void clEvtCallbackDispatcher(ClEvtCbQueueDataT *pQueueData,
        ClEvtInitInfoT *pInitInfo)
{
    CL_FUNC_ENTER();

    switch (pQueueData->cbId)
    {
        case CL_EVT_PUBLISH_CALLBACK:
            {
                ClEvtEventPublishInfoT *pPublishInfo = pQueueData->cbArg;

                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL,
                        CL_EVENT_LIB_NAME,
                        CL_EVENT_LOG_MSG_0_EVENT_DELIVER_CALLBACK);
                evtEventDeliverCallbackDispatch(pInitInfo,
                                                pPublishInfo->version,
                                                pPublishInfo->subscriptionId,
                                                (ClEventHandleT)pPublishInfo->eventHandle,
                                                pPublishInfo->eventDataSize);
                clHeapFree(pPublishInfo);

                break;
            }
        case CL_EVT_CHANNEL_CALLBACK:
            {
                ClEvtClientAsyncChanOpenCbArgT *pCallbackArg = pQueueData->cbArg;

                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL,
                        CL_EVENT_LIB_NAME,
                        CL_EVENT_LOG_MSG_0_ASYNC_CHAN_OPEN_CALLBACK);
                
                pInitInfo->pEvtCallbackTable[0].callbacks.clEvtChannelOpenCallback(pCallbackArg->
                        invocation,
                        pCallbackArg->
                        channelHandle,
                        pCallbackArg->
                        apiResult);
                clHeapFree(pCallbackArg);

                break;
            }

        default:
            {
                clLogError("EVT", "CBD", "Invalid Callback ID");
            }
    }

    CL_FUNC_EXIT();
}

ClRcT VDECL(clEvtEventReceive)(ClEoDataT data, ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint8T *pInData = NULL;
    ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader = NULL;

    ClEvtInitInfoT *pInitInfo = NULL;
    ClEoExecutionObjT *pEoObj = { 0 };
    ClEvtClientHeadT *pEvtClientHead = { 0 };
    ClUint32T len = 0;
    ClEvtEventHandleT *pEventInfo = NULL;

    ClEvtEventSecondaryHeaderT *pEvtSecHeader = NULL;
    ClEvtEventSecondaryHeaderT evtSecHeader = {0};
    ClUint32T headerLen = 0;
    ClEventHandleT eventHandle = 0;
    ClUint32T i;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();


    rc = clBufferLengthGet(inMsgHandle, &len);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR",
                   "Failed to get Buffer length [%#X]", rc);
        goto failure;
    }

    rc = clBufferFlatten(inMsgHandle, &pInData);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR",
                   "Failed to Flatten Buffer [%#X]", rc);
        goto failure;
    }

    pEvtPrimaryHeader = (ClEvtEventPrimaryHeaderT *) pInData;

    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);

    if (CL_OK != rc)            /* If given EO is not initalized with EM
                                 * serivce */
    {
        rc = CL_EVENT_ERR_INIT_NOT_DONE;
        goto inDataAllocated;
    }

    /*
    ** Obtain the Event on lines of clEventAllocate sans the details
    ** unecessary here. Fields like channel handle, etc. can be initialized
    ** to 0.
    */
    clOsalMutexLock(&gEvtReceiveMutex);
    /*
     * Recheck again as it could have been finalized behind our back.
     */
    if(!isInitDone())
    {
        clOsalMutexUnlock(&gEvtReceiveMutex);
        clLogError("EVT", "VER", CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED); 
        rc = CL_EVENT_ERR_INIT_NOT_DONE;                                
        goto inDataAllocated;
    }

    rc = clHandleCreate(pEvtClientHead->evtClientHandleDatabase, sizeof(ClEvtEventHandleT), &eventHandle);
    if (CL_OK != rc)
    {
        clOsalMutexUnlock(&gEvtReceiveMutex);
        clLogError("EVT", "EVR", 
                   CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED, rc);
        rc = CL_EVENT_ERR_NO_RESOURCE;
        goto inDataAllocated;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void **)&pEventInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlAllocated;
    }

    /*
     * Initialize the structure 
     */
    memset(pEventInfo, 0, sizeof(*pEventInfo));
    pEventInfo->handleType = CL_EVENT_HANDLE;
    rc = clBufferCreate(&pEventInfo->msgHandle); /* Create the mesg
                                                  * handle */
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   "Message Buffer creation Falied, rc[%#X]", rc);

        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
        goto eventHdlAllocated;
    }

    rc = clBufferNBytesWrite(pEventInfo->msgHandle, pInData, len);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   "Message Buffer Write Failed, rc[%#X]", rc);

        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
        goto eventBufferCreated;
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto eventBufferCreated;
    }
    /*
    ** Using the evtHandle present in the Secondary Header we now obtain the
    ** callback registered and invoke the same.
    */

    pEvtSecHeader =
        (ClEvtEventSecondaryHeaderT *) (pInData + sizeof(*pEvtPrimaryHeader) +
                                        pEvtPrimaryHeader->channelNameLen +
                                        pEvtPrimaryHeader->publisherNameLen +
                                        pEvtPrimaryHeader->patternSectionLen +
                                        pEvtPrimaryHeader->eventDataSize);

#ifdef SOLARIS_BUILD
    bcopy(pEvtSecHeader, &evtSecHeader,  sizeof(evtSecHeader));
#else
    memcpy(&evtSecHeader, pEvtSecHeader, sizeof(evtSecHeader));
#endif

    headerLen = sizeof(*pEvtPrimaryHeader) + pEvtPrimaryHeader->channelNameLen + 
        pEvtPrimaryHeader->publisherNameLen + pEvtPrimaryHeader->patternSectionLen +
        pEvtPrimaryHeader->eventDataSize + sizeof(*pEvtSecHeader);

    if(len > headerLen && 
       len == headerLen + sizeof(ClEvtEventTertiaryHeaderT))
    {
        ClEvtEventTertiaryHeaderT *pEvtTertiaryHeader = (ClEvtEventTertiaryHeaderT*)(pEvtSecHeader + 1);
        ClEvtEventTertiaryHeaderT evtTertiaryHeader = {0};
        memcpy(&evtTertiaryHeader, pEvtTertiaryHeader, sizeof(evtTertiaryHeader));
        if(evtTertiaryHeader.eoId != pEoObj->eoID)
        {
            clLogInfo("EVT", "RECEIVE", 
                      "Event publish received for EO id [%#llx] is not valid any more "
                      "as EO id is now [%#llx]", evtTertiaryHeader.eoId, pEoObj->eoID);
            rc = CL_EVENT_ERR_INTERNAL;
            goto eventBufferCreated;
        }
    }

    /* 
     * Check out the InitInfo of this client from the Handle Database
     */

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtSecHeader.evtHandle, (void*)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventBufferCreated;
    }
    
    if(!pInitInfo->numCallbacks 
       ||
       !pInitInfo->pEvtCallbackTable)
    {
        /*
         * No callbacks defined
         */
        goto eventBufferCreated;
    }

    pInitInfo->receiveStatus = CL_TRUE;

    clOsalMutexUnlock(&gEvtReceiveMutex);

    for(i = 0; i < pInitInfo->numCallbacks 
            &&
            pInitInfo->pEvtCallbackTable[i].callbacks.clEvtEventDeliverCallback == NULL;
        ++i);

    if(i != pInitInfo->numCallbacks)
    {
        if (CL_TRUE == pInitInfo->queueFlag)
        {
            ClEvtEventPublishInfoT *pPublishInfo =
                clHeapAllocate(sizeof(ClEvtEventPublishInfoT));
            if (NULL == pPublishInfo)
            {
                clLogError("EVT", "EVR", 
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);

                clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtSecHeader.evtHandle);
                rc = CL_EVENT_ERR_NO_MEM;
                goto resetReceiveStatus;
            }
            pPublishInfo->version = pEvtPrimaryHeader->version;
            pPublishInfo->subscriptionId = evtSecHeader.subscriptionId;
            pPublishInfo->eventHandle = eventHandle;
            pPublishInfo->eventDataSize = pEvtPrimaryHeader->eventDataSize;

            rc = clEvtQueueCallback(pInitInfo, CL_EVT_PUBLISH_CALLBACK,
                                    pPublishInfo);
            if (CL_OK != rc)
            {
                clLogError("EVT", "EVR", 
                           "Queuing Event Callback Failed, rc[%#X]", rc);

                clHeapFree(pPublishInfo);
                clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtSecHeader.evtHandle);
                goto resetReceiveStatus;
            }

            clLog(CL_LOG_TRACE, "EVT", "EVR", 
                  CL_EVENT_LOG_MSG_0_EVENT_QUEUED);
        }
        else
        {
            clLog(CL_LOG_TRACE, "EVT", "EVR", 
                  CL_EVENT_LOG_MSG_0_EVENT_DELIVER_CALLBACK);
            evtEventDeliverCallbackDispatch(pInitInfo, 
                                            pEvtPrimaryHeader->version,
                                            evtSecHeader.subscriptionId,
                                            eventHandle,
                                            pEvtPrimaryHeader->eventDataSize);
        }
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtSecHeader.evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EVR", 
                   CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto resetReceiveStatus;
    }

    // success:
    clOsalMutexLock(&gEvtReceiveMutex);
    pInitInfo->receiveStatus = CL_FALSE;
    if(pInitInfo->numReceiveWaiters)
        clOsalCondBroadcast(pInitInfo->receiveCond);
    clOsalMutexUnlock(&gEvtReceiveMutex);

    clLogTrace("EVT", "REC",
               "Event was received successfully");

    clHeapFree(pInData); // Free the buffer from clBufferFlatten()

    CL_FUNC_EXIT();
    return rc;

    resetReceiveStatus:

    clOsalMutexLock(&gEvtReceiveMutex);

    pInitInfo->receiveStatus = CL_FALSE;
    if(pInitInfo->numReceiveWaiters)
        clOsalCondBroadcast(pInitInfo->receiveCond);

    eventBufferCreated:
    clBufferDelete(&pEventInfo->msgHandle);

    eventHdlAllocated:
    clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, eventHandle);

    clOsalMutexUnlock(&gEvtReceiveMutex);

    inDataAllocated:
    rc = CL_EVENTS_RC(rc);
    if(rc != CL_EVENT_ERR_INTERNAL)
    {
        clLogError("EVT", "REC",
                   "Receive of Event failed, rc[0x%x]", rc);
    }

    clHeapFree(pInData); // Free the buffer from clBufferFlatten()

    failure:

    CL_FUNC_EXIT();
    return rc;
}

void handleDestroyCallback(void * pInstance)
{   
    ClRcT rc = CL_OK;
    ClEvtHdlDbDataT * evtHdlDbData;
    ClEvtInitInfoT *pInitInfo = NULL;
    ClEvtEventHandleT *pEventInfo = NULL;

    evtHdlDbData = (ClEvtHdlDbDataT *)pInstance;

    switch(evtHdlDbData->handleType)
    {
    case CL_CHANNEL_HANDLE:
        break;

    case CL_EVENT_HANDLE:

        pEventInfo = (ClEvtEventHandleT *)pInstance;
        if(pEventInfo->msgHandle)
        {
            rc = clBufferDelete(&pEventInfo->msgHandle);
            if(CL_OK != rc)
            {
                clLogError("EVT", "HDC", 
                           "clBufferDelete Failed, rc[%#X]", rc);
            }
        }
        break;

    case CL_EVENT_INIT_HANDLE:

        pInitInfo = (ClEvtInitInfoT *)pInstance;
        if (CL_TRUE == pInitInfo->queueFlag)
        {
            clOsalMutexLock(pInitInfo->cbMutex);
            clQueueDelete(&pInitInfo->cbQueue);
            clOsalMutexUnlock(pInitInfo->cbMutex);

            clOsalMutexDelete(pInitInfo->cbMutex);
            clOsalCondDelete(pInitInfo->receiveCond);
            close(pInitInfo->writeFd);
            close(pInitInfo->readFd);
        }
        break;
    }
}

ClRcT clEvtClientInit(ClEoExecutionObjT **ppEoObj,
                      ClEvtClientHeadT **ppEvtClientHead)
{
    ClRcT rc = CL_OK;

    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;

    CL_FUNC_ENTER();

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                   "Failed to get EO object [%#X]", rc);
        goto failure;
    }
    *ppEoObj = pEoObj;          /* Return the EO */

    pEvtClientHead = clHeapAllocate(sizeof(ClEvtClientHeadT));
    if (NULL == pEvtClientHead)
    {
        clLogError("EVT", "INI", 
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto failure;
    }
    memset(pEvtClientHead, 0, sizeof(ClEvtClientHeadT));
    *ppEvtClientHead = pEvtClientHead;  /* Return the Client Head */

    /* Installing my own functions, so that any other process can make an RMD call to me.*/
    rc = clEventClientInstallTables(pEoObj);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                   "Installing EM client failed  [%#X]", rc);
        goto clientHeadAllocated;
    }
    
    /* Registering client table to make RMDs from event server to event client. */
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspCltFuncTable, EVT), pEoObj->eoPort);
    if(CL_OK != rc)
    {
        clLogError("EVT", "INT", "Event EO servert-to-client client table register failed with [%#x]", rc);
        goto clientInstalled;
    }

    /* Registering the counterpart of server's table, so that I can make RMD calls to event server */
	rc = clEventLibTableInit();
	if(CL_OK != rc)
	{
		clLogError("EVT", "INI", "Registering server table failed, rc [%#x]", rc);
		goto clientInstalled;
	}

    rc = clEoPrivateDataSet(pEoObj, CL_EO_EVT_EVENT_DELIVERY_COOKIE_ID,
                            (void *) pEvtClientHead);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                   "Unable to set EO private data [%#X]", rc);
        goto clientInstalled;
    }

    /*
     * Create Handle Database to store the user information 
     */
    rc =  clHandleDatabaseCreate(handleDestroyCallback, &pEvtClientHead->evtClientHandleDatabase);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                   CL_LOG_MESSAGE_1_HANDLE_DB_CREATION_FAILED, rc);
        rc = CL_EVENT_ERR_NO_RESOURCE;    
        goto clientInstalled;
    }

    if(!gEvtReceiveMutexInitialized)
    {
        rc = clOsalMutexInit(&gEvtReceiveMutex);
        if(CL_OK != rc)
        {
            clLogError("EVT", "INI", "Evt receive mutex init returned [%#x]", rc);
            goto clientInstalled;
        }
        gEvtReceiveMutexInitialized = CL_TRUE;
    }
    
    // success:
    CL_FUNC_EXIT();
    return rc;


    clientInstalled:
    clEventClientUninstallTables(pEoObj);

    clientHeadAllocated:
    clHeapFree(pEvtClientHead);

    failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "INI", 
               "Event Client Init failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

#ifdef CL_EVT_SAF_DEBUG

static ClSelectionObjectT gEvtSelectionObject;

static void *clEvtDispatchHandleAllCallBacks(void *userArg)
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT *pEvtHandle = userArg;

    fd_set rfds;

    FD_ZERO(&rfds);

    while (gEvtInitCount != 0)
    {
        FD_SET(gEvtSelectionObject, &rfds);

        rc = select(gEvtSelectionObject + 1, &rfds, NULL, NULL, NULL);
        if (rc <= 0)
        {
            break;
        }

        if (FD_ISSET(gEvtSelectionObject, &rfds))
        {
            rc = clEventDispatch(*pEvtHandle, CL_DISPATCH_ONE);
            if (CL_OK != rc)
            {
                clLogError("EVT", "EVD", 
                        "Failed rc[0x%x]", rc);
                goto failure;
            }
        }
    }

failure:
    return NULL;
}
#endif

ClRcT clEventInitializeWithVersion(ClEventInitHandleT *pEvtHandle,
                                   const ClEventVersionCallbacksT *pEvtCallbackTable,
                                   ClUint32T numCallbacks,
                                   ClVersionT *pVersion)
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT  tempHdl = CL_HANDLE_INVALID_VALUE;
    ClEvtInitRequestT evtInitReq = { 0 };
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtInitInfoT *pInitInfo = NULL;
    ClRmdOptionsT rmdOptions = { 0 };
    ClIocNodeAddressT localIocAddress;
    ClIocAddressT destAddr;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec=500};
    ClInt32T tries = 0;
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;

    CL_FUNC_ENTER();

    /*
     * Validate the parameter(s) 
     */
    if (NULL == pEvtHandle)
    {
        clLogError("EVT", "INI", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pEvtHandle");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }
    if (NULL == pVersion)
    {
        clLogError("EVT", "INI", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pVersion");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }

#ifdef VERSION_READY
    /*
     * Verify if the version specified is supported 
     */
    clEvtVerifyAppToClientVersion(pVersion);
#endif

    /*
     ** Check if the Initialization is being done for the first time. If so
     ** create the the Containers for User and Channel Info and finish the
     ** housekeeping chores. If not then fetch the EO & Client Head & proceed.
     */
    if (CL_FALSE == isInitDone())
    {
        rc = clEvtClientInit(&pEoObj, &pEvtClientHead);
        if (CL_OK != rc)
        {
            goto failure;
        }
    }
    else
    {
        rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
        if (CL_OK != rc)
        {
            goto failure;
        }
    }


    localIocAddress = clIocLocalAddressGet();
    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    {    
        ClUint32T i = 0;
        ClStatusT status;

        for(i = 0; i < CL_EVT_MAX_SVR_RETRY; i++) 
        {
            rc = clCpmCompStatusGet(destAddr, &status);
            if( rc != CL_OK )
                return CL_EVENTS_RC(rc);
            if(status == CL_STATUS_UP)
                break;

            clLogWarning("EVT", "INI", "Event server status is %d...waiting for it to come up.", status);

            sleep(1);
        }
        if(i == CL_EVT_MAX_SVR_RETRY) {
            rc = CL_ERR_TRY_AGAIN;
            goto failure;
        }
    } 


    evtInitReq.userId.eoIocPort = pEoObj->eoID;

    /*
     * Add the event handler function into corresponding container 
     */

    /*
     * Pack the initialize info 
     */

    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(evtInitReq);

    /*
     * Generate a client Handle and pass to the server
     */
    rc = clHandleCreate(pEvtClientHead->evtClientHandleDatabase, sizeof(ClEvtInitInfoT), &evtInitReq.clientHdl);
    if(CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED, rc);
        rc = CL_EVENT_ERR_NO_RESOURCE;
        goto failure;
    }

    rc = clBufferCreate(&inMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtInitRequestT, 4, 0, 0)(&evtInitReq,
                                                            inMsgHandle,
                                                            0);

    /*
     * To receive the Event Handle returne by server 
     */
    rc = clBufferCreate(&outMsgHandle);

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_INTIALIZE, inMsgHandle, outMsgHandle,
                          CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_ATMOST_ONCE,
                          &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN 
            && 
            ++tries < 5 
            &&
            clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion1 = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion1);
            if (CL_OK != rc)
            {
                clLogError("EVT", "INI", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto inMsgCreated;
            }

            clLogError("EVT", "INI", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Initialize", evtInitReq.releaseCode,
                    evtInitReq.majorVersion, evtInitReq.minorVersion,
                    pVersion1->releaseCode, pVersion1->majorVersion,
                    pVersion1->minorVersion);

            clHeapFree(pVersion1); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "INI", 
                    "RMD to [node:%d port:%d]:: Initialization failed for EO [IOC Port=0x%llx, "
                    "evtHandle=%#llX], rc=[0x%x]",
                    destAddr.iocPhyAddress.nodeAddress, destAddr.iocPhyAddress.portId,
                    evtInitReq.userId.eoIocPort,
                    evtInitReq.userId.evtHandle, rc);
        }

        clBufferDelete(&outMsgHandle);
        goto inMsgCreated;
    }
    else
    {
        ClUint32T size = sizeof(tempHdl);

        rc = clBufferNBytesRead(outMsgHandle, (ClUint8T *) &tempHdl, &size);
        clBufferDelete(&outMsgHandle);
        if (CL_OK != rc)
        {
            clLogError("EVT", "INI", 
                    "Reading evtHandle from outMessage Failed [%#X]",
                    rc);
            goto inMsgCreated;
        }
    }
    /*
     * Store the client specific information in the Handle Database against
     * the client Handle
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtInitReq.clientHdl, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto inMsgCreated;
    }

    if (NULL != pEvtCallbackTable)
    {
        if(pInitInfo->numCallbacks != numCallbacks)
        {
            if(pInitInfo->pEvtCallbackTable)
                clHeapFree(pInitInfo->pEvtCallbackTable);
            pInitInfo->pEvtCallbackTable = clHeapCalloc(numCallbacks, 
                                                        sizeof(*pInitInfo->pEvtCallbackTable));
            CL_ASSERT(pInitInfo->pEvtCallbackTable != NULL);
        }
        memcpy(pInitInfo->pEvtCallbackTable, pEvtCallbackTable, 
               sizeof(*pInitInfo->pEvtCallbackTable) * numCallbacks);
        pInitInfo->numCallbacks = numCallbacks;
    }

    rc = clOsalCondCreate(&pInitInfo->receiveCond);
    if(rc != CL_OK)
    {
        clLogError("EVT", "INI", "Event initialize conditional variable create returned with error [%#x]", rc);
        goto inMsgCreated;
    }
    pInitInfo->handleType = CL_EVENT_INIT_HANDLE;
    pInitInfo->queueFlag = CL_FALSE;
    pInitInfo->servHdl =  tempHdl;
    *pEvtHandle = evtInitReq.clientHdl; 
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtInitReq.clientHdl);
    if (CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto inMsgCreated;
    }

    /*
     * Inc the Init Reference Count 
     */
    CL_EVT_INIT_COUNT_INC();

#ifdef CL_EVT_SAF_DEBUG
    /*
     * Added for the purpose of testing Selection Object functionality.
     */
    {
        clEventSelectionObjectGet(*pEvtHandle, &gEvtSelectionObject);

        clOsalTaskCreateDetached("Dispatcher", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                clEvtDispatchHandleAllCallBacks, pEvtHandle);
    }
#endif
// success:

    clLogTrace("EVT", "INI", 
            "Event Library Initialized with evtHandle[%#llX]", *pEvtHandle);

    CL_FUNC_EXIT();
    return CL_OK;

inMsgCreated:
    clBufferDelete(&inMsgHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "INI", 
            "Event Library Initialization failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventInitialize(ClEventInitHandleT *pEvtHandle,
                        const ClEventCallbacksT *pEvtCallbacks,
                        ClVersionT *pVersion)
{
    ClEventVersionCallbacksT callbackTable[1] = { {0} };
    callbackTable[0].version = CL_EVT_DATA_VERSION_BASE;
    memset(&callbackTable[0].callbacks, 0, sizeof(callbackTable[0].callbacks));
    if(pEvtCallbacks)
        callbackTable[0].callbacks = *pEvtCallbacks;
    return clEventInitializeWithVersion(pEvtHandle, callbackTable, 1, pVersion);
}


/*
 * Added the following API for SAF conformance 
 */

ClRcT clEventSelectionObjectGet(ClEventInitHandleT evtHandle,
        ClSelectionObjectT * pSelectionObject)
{
    ClRcT rc = CL_OK;

    ClInt32T fds[2];

    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtInitInfoT *pInitInfo = NULL;


    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Validate the parameter(s) 
     */

    if (pSelectionObject == NULL)
    {
        clLogError("EVT", "SOG", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pSelectionObject");

        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Check the validity of evtHandle supplied 
     */
    rc = clEvtInitHandleValidate(evtHandle, pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }
    /*
     * Get the pInitInfo from the Handle Database
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "SOG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    /*
     * If already called Simply return the Selection Object 
     */
    if (CL_TRUE == pInitInfo->queueFlag)
    {
        *pSelectionObject = pInitInfo->readFd;
        goto success;
    }

    if (pipe(fds) != 0)
    {
        clLogError("EVT", "SOG", "Pipe failed");
        rc = CL_EVENT_ERR_NO_RESOURCE;
        goto eventHdlCheckedOut;
    }

    rc = clOsalMutexCreate(&pInitInfo->cbMutex);
    if (CL_OK != rc)
    {
        clLogError("EVT", "SOG", 
                "Mutex Creation Failed, rc[%#X]", rc);
        goto eventHdlCheckedOut;
    }

    rc = clQueueCreate(0, clEvtCbDequeueCallBack, clEvtCbDestroyCallback,
            &pInitInfo->cbQueue);
    if (CL_OK != rc)
    {
        clLogError("EVT", "SOG", 
                "Queue Creation Failed, rc[%#X]", rc);
        goto mutexCreated;
    }

    pInitInfo->readFd = fds[0];
    pInitInfo->writeFd = fds[1];

    pInitInfo->queueFlag = CL_TRUE;

    *pSelectionObject = fds[0]; /* Read FD */

success:
    clLogTrace("EVT", "SOG", 
            "Selection Object Get succeeded with object[%#llX]", *pSelectionObject);

    /*
     * Checkin the data before exiting 
     */
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "SOG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto mutexCreated;
    }

    CL_FUNC_EXIT();
    return CL_OK;


    mutexCreated:
    clOsalMutexDelete(pInitInfo->cbMutex);

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "SOG", 
            "Selection Object Get failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventDispatch(ClEventInitHandleT evtHandle,
        ClDispatchFlagsT dispatchFlags)
{
    ClRcT rc = CL_OK;

    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtInitInfoT *pInitInfo = NULL;

    ClEvtCbQueueDataT *pQueueData = NULL;

    ClCharT chr1;
    ClUint32T queueSize = 0;


    CL_FUNC_ENTER();


    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Validate the parameter(s) 
     */

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Check the validity of evtHandle supplied 
     */
    rc = clEvtInitHandleValidate(evtHandle, pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "DPT", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    }

    switch (dispatchFlags)
    {
        case CL_DISPATCH_ONE:
            {
                /*
                 * Dequeue the first pending request
                 */
                rc = clOsalMutexLock(pInitInfo->cbMutex);
                rc = clQueueSizeGet(pInitInfo->cbQueue, &queueSize);
                rc = clOsalMutexUnlock(pInitInfo->cbMutex);
                if (CL_OK != rc)
                {
                    /*
                     * Queue Error 
                     */
                    clLogError("EVT", "DPT", 
                            "Unable to get queue Size %x", rc);
                    goto eventHdlCheckedOut;
                }

                /*
                 *  If there is nothing in the queue return 
                 */
                if (0 == queueSize)
                {
                    /*
                     * Nothing to do Simply return 
                     */
                    goto success;
                }

                /*
                 *  read the pipe and proceed with the callback 
                 */
                if(read(pInitInfo->readFd, (void *) &chr1, 1) != 1)
                    clLogError("EVT", "DIS", "Read from dispatch pipe returned [%s]",
                               strerror(errno));

                /*
                 * Obtain the pQueueData while deleting the node 
                 */
                clOsalMutexLock(pInitInfo->cbMutex);
                rc = clQueueNodeDelete(pInitInfo->cbQueue,
                        (ClQueueDataT *) &pQueueData);
                clOsalMutexUnlock(pInitInfo->cbMutex);
                if (CL_OK != rc)
                {
                    clLogError("EVT", "DPT", 
                            "Unable to delete Queue node, rc=[0x%x]", rc);
                    goto eventHdlCheckedOut;
                }

                /*
                 * process the CB 
                 */
                clEvtCallbackDispatcher(pQueueData, pInitInfo);

                clHeapFree(pQueueData);

                break;
            }
        case CL_DISPATCH_ALL:
            {
                /*
                 * dequeue the first pending request, process it dequeue the next 
                 * pending request, process it drain the whole queue 
                 */
                while (1)
                {
                    queueSize = 0;
                    clOsalMutexLock(pInitInfo->cbMutex);
                    rc = clQueueSizeGet(pInitInfo->cbQueue, &queueSize);
                    clOsalMutexUnlock(pInitInfo->cbMutex);
                    if (CL_OK != rc)
                    {
                        clLogError("EVT", "DPT", 
                                "Unable to get queue Size %x", rc);
                        goto eventHdlCheckedOut;
                    }
                    /*
                     *  If there is nothing in the queue return 
                     */
                    if (queueSize == 0)
                    {
                        /*
                         * Done the Job so return 
                         */
                        goto success;
                    }

                    while (queueSize > 0)
                    {
                        clOsalMutexLock(pInitInfo->cbMutex);
                        rc = clQueueNodeDelete(pInitInfo->cbQueue,
                                (ClQueueDataT *) &pQueueData);
                        clOsalMutexUnlock(pInitInfo->cbMutex);
                        if (CL_OK != rc)
                        {
                            clLogError("EVT", "DPT", 
                                    "Unable to delete Queue node 0x%x", rc);
                            goto eventHdlCheckedOut;
                        }

                        if(read(pInitInfo->readFd, (void *) &chr1, 1) != 1)
                            clLogError("EVT", "DIS", "Read from dispatch pipe returned with [%s]",
                                       strerror(errno));
                      
                        /*
                         * process the CB 
                         */
                        clEvtCallbackDispatcher(pQueueData, pInitInfo);

                        clHeapFree(pQueueData);

                        queueSize--;
                    }
                }
                break;
            }
        case CL_DISPATCH_BLOCKING:
            {
                /*
                 * Block here, and keep processing the request as it comes in 
                 */
                while (1)
                {
                    if(read(pInitInfo->readFd, (void *) &chr1, 1) != 1)
                        clLogError("EVT", "DIS", "Read from dispatch pipe returned [%s]",
                                   strerror(errno));

                    clOsalMutexLock(pInitInfo->cbMutex);
                    rc = clQueueNodeDelete(pInitInfo->cbQueue,
                            (ClQueueDataT *) &pQueueData);
                    clOsalMutexUnlock(pInitInfo->cbMutex);
                    if (CL_OK != rc)
                    {
                        clLogError("EVT", "DPT", 
                                "Unable to delete Queue node 0x%x", rc);
                        goto eventHdlCheckedOut;
                    }

                    /*
                     * process the CB 
                     */
                    clEvtCallbackDispatcher(pQueueData, pInitInfo);

                    clHeapFree(pQueueData);
                }
                break;
            }
        default:
            {
                rc = CL_ERR_INVALID_PARAMETER;
                goto eventHdlCheckedOut;
            }

    }

success:
    clLogTrace("EVT", "DPT", 
            "Event Dispatch succeeded for evtHandle[%#llX]", evtHandle);
    /*
     * Checkin the data before exiting 
     */
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "DPT", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

    CL_FUNC_EXIT();
    return CL_OK;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "DPT", 
            "Event Dispatch failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT channelHdlWalk(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, void * pCookie)
{
    ClRcT rc = CL_OK;

    ClEvtEventHandleT *pEventInfo = NULL;
    ClEvtHdlDbDataT * evtHdlDbData = NULL;
    ClEventChannelHandleT * pChannelHandle = NULL;    

    pChannelHandle = (ClEventChannelHandleT *)pCookie;

    rc = clHandleCheckout(databaseHandle, handle, (void **)&evtHdlDbData);
    if (CL_OK != rc)
    {
        clLogError("EVT", "CHW", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    }

    if(evtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventInfo = (ClEvtEventHandleT *)evtHdlDbData;
    }
    else
    {
        goto success;
    }
    if(pEventInfo->evtChannelHandle == *pChannelHandle)
    {
        rc = clHandleCheckin(databaseHandle, handle);
        /* 
         * Destroy the event 
         */
        rc = clHandleDestroy(databaseHandle, handle);
        return CL_OK;
    }

success:
    clLogTrace("EVT", "CHW", 
            "Channel Handle Walk succeeded");
    rc = clHandleCheckin(databaseHandle, handle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "CHW", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }
    return CL_OK; 

failure:
    clLogError("EVT", "CHW", 
            "Channel Handle Walk failed, rc[%#X]", rc);
    return rc;
}

ClRcT eventFinalizeHandleDBWalk(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, void *pCookie)
{
    ClRcT rc = CL_OK;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtHdlDbDataT * evtHdlDbData = NULL;
    ClEventInitHandleT * pEvtHandle = NULL;

    pEvtHandle = (ClEventInitHandleT *)pCookie; 

    rc = clHandleCheckout(databaseHandle, handle, (void**)&evtHdlDbData);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FHW", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    }

    if(evtHdlDbData->handleType == CL_CHANNEL_HANDLE)
    {
        pEvtChannelInfo = (ClEvtClientChannelInfoT *)evtHdlDbData;
    }
    else
    {
        goto success;
    }

    if(pEvtChannelInfo->evtHandle == *pEvtHandle)
    {
        /*
         * Walk with the channelHdlWalk to cleanup events allocated on this 
         * Channel
         */

        rc = clHandleWalk(databaseHandle, channelHdlWalk, (void * )&handle); 

        rc = clHandleCheckin(databaseHandle, handle);

        /*
         * Destroy the Handle for this Channel
         */
        rc = clHandleDestroy(databaseHandle, handle);
        return CL_OK;
    }

success:
    clLogTrace("EVT", "FHW", 
            "Finalize Handle Walk succeeded");
    rc = clHandleCheckin(databaseHandle, handle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FHW", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }
    return CL_OK;

failure:
    clLogError("EVT", "FHW", 
            "Finalize Handle Walk failed, rc[%#X]", rc);
    return rc;
}

ClRcT clEventFinalize(ClEventInitHandleT evtHandle)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClRmdOptionsT rmdOptions = { 0 };
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClIocNodeAddressT localIocAddress = {0};
    ClIocPortT eoIocPort = 0;
    ClEvtUnsubscribeEventRequestT unsubscribeRequest = { 0 };
    ClIocAddressT destAddr = {{0}};
    ClEvtClientChannelInfoT channelInfoKey = { 0 };
    ClEvtInitInfoT *pInitInfo = NULL;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * validate the parameter(s) 
     */

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Check the validity of evtHandle supplied 
     */
    rc = clEvtInitHandleValidate(evtHandle, pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Get user information 
     */
    rc = clEoMyEoIocPortGet(&eoIocPort);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Pack the unsubscription information 
     */

    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(unsubscribeRequest);

    unsubscribeRequest.evtChannelHandle = 0;

    unsubscribeRequest.subscriptionId = 0;
    unsubscribeRequest.userId.eoIocPort = pEoObj->eoID;

    clOsalMutexLock(&gEvtReceiveMutex);

    /*
     *Get the servHdl corresponding to this initialization, pass it in 
     *unsubscription request
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto out_unlock;
    }

    CL_ASSERT(pInitInfo != NULL);

    unsubscribeRequest.userId.evtHandle = pInitInfo->servHdl;
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto out_unlock;
    }
    
    /*
     * Check for in-flight event receives.
     */
    while(pInitInfo->receiveStatus == CL_TRUE)
    {
        ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0 };
        ++pInitInfo->numReceiveWaiters;
        clOsalCondWait(pInitInfo->receiveCond, &gEvtReceiveMutex, delay);
        --pInitInfo->numReceiveWaiters;
    }

    unsubscribeRequest.reqFlag = CL_EVT_FINALIZE;
    clLogTrace("EVT", "FIN", 
            "EM Client: Finalizing->{eoIocPort[%#llX], evtHandle[%#llX]}",
            unsubscribeRequest.userId.eoIocPort,
            unsubscribeRequest.userId.evtHandle);

    /*
     * Create message handle and pack the user input 
     */
    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferCreate(&outMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtUnsubscribeEventRequestT, 4, 0, 0)(&unsubscribeRequest,
                                                                        inMsgHandle,
                                                                        0);

    localIocAddress = clIocLocalAddressGet();

    /*
     * Invoke RMD call provided by EM/S for finalizing EM, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = 0;
    rmdOptions.timeout = CL_RMD_DEFAULT_TIMEOUT;

    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_UNSUBSCRIBE, inMsgHandle,
                          outMsgHandle, CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN 
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);
    clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "FIN", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            clLogError("EVT", "FIN", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Finalize", unsubscribeRequest.releaseCode,
                    unsubscribeRequest.majorVersion,
                    unsubscribeRequest.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "FIN", 
                       CL_EVENT_LOG_MSG_3_FINALIZE_FAILED, eoIocPort, 
                       evtHandle, rc);
            if(CL_RMD_TIMEOUT_UNREACHABLE_CHECK(rc)) // NTC ???
            {
                clLogInfo("EVT", "FIN", 
                          "Returning rc as CL_OK in case of CL_ERR_TIMEOUT");
                rc = CL_OK;
            }

        }
        goto outMsgHdlAllocated;
    }

    /*
     ** To delete all channel related information for this evtHandle, mark
     ** evtChannelKey as 0 and invoke delete with this special key.
     */
    channelInfoKey.evtChannelKey = 0;
    channelInfoKey.evtHandle = evtHandle;   /* Match only evtHandle in Channel
                                             * Info Container */

    /*
     * Obtain the info for evtHandle 
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto outMsgHdlAllocated;
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto outMsgHdlAllocated;
    }

    if(pInitInfo && pInitInfo->pEvtCallbackTable)
    {
        clHeapFree(pInitInfo->pEvtCallbackTable);
        pInitInfo->pEvtCallbackTable = NULL;
        pInitInfo->numCallbacks = 0;
    }

    /*
     * Walk the HandleDatabase and delete all the entries for channel
     * which have been opened with this evtHandle:
     */
    rc = clHandleWalk(pEvtClientHead->evtClientHandleDatabase, eventFinalizeHandleDBWalk, (void*)&evtHandle); 
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                "eventFinalizeHandleDBWalk failed, rc[%#X]", rc);
        goto outMsgHdlAllocated;
    }


    /*
     * Cleanup the information related to this Initialization handle
     */
    rc = clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "FIN", 
                "clHandleDestroy Failed, rc[%#X]", rc);
        goto outMsgHdlAllocated;
    }

    /*
     * Decrement Init Counter 
     */
    CL_EVT_INIT_COUNT_DEC();


    /*
     ** Finalize only once when the Init count reaches 0.
     */

    if (isLastFinalize())       /* Init Count is 0 */
    {
        rc = clHandleDatabaseDestroy(pEvtClientHead->evtClientHandleDatabase); 
        if(CL_OK != rc)
        {
            clLogError("EVT", "FIN", 
                    CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
            goto outMsgHdlAllocated;
        }

        clHeapFree(pEvtClientHead);

        /*
         ** Set the EO private data as NULL. so that next time after init/finalize
         ** we will get NULL. and we can figure out this is not alreay finalized once.
         */
        rc = clEventClientUninstallTables(pEoObj);
    }

    clOsalMutexUnlock(&gEvtReceiveMutex);

// success:
    clBufferDelete(&outMsgHandle);

    clLogTrace("EVT", "FIN", 
            "Event Finalize succeeded for evtHandle[%#llX]", evtHandle);

    CL_FUNC_EXIT();
    return CL_OK;

    outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

    out_unlock:
    clOsalMutexUnlock(&gEvtReceiveMutex);

    failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "FIN", 
            "Event Finalize failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEvtChannelOpenPrologue(ClEventInitHandleT evtHandle,
        const ClNameT *pChannelName,
        ClEventChannelOpenFlagsT evtChannelOpenFlag,
        ClEventChannelHandleT *pEvtChannelHandle,
        ClEvtClientHeadT **ppEvtClientHead,
        ClBufferHandleT *pInMsgHandle,
        ClBufferHandleT *pOutMsgHandle)
{
    ClRcT rc = CL_OK;
    ClEvtChannelOpenRequestT evtChannelOpenRequest = { 0 };
    ClEvtClientChannelInfoT *pEvtCliChannelInfo = NULL;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClUint16T evtChannelID = 0;
    ClUint8T channelScope = 0;
    ClEvtInitInfoT *pInitInfo = NULL;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();


    /*
     * Validate the Parameters 
     */

    rc = clEvtInitValidate(&pEoObj, ppEvtClientHead);    /* Fetch the client
                                                          * head */
    if (CL_OK != rc)
    {
        goto failure;
    }
    pEvtClientHead = *ppEvtClientHead;  /* Update the local place holder */

    /*
     * Check the validity of evtHandle supplied 
     */
    rc = clEvtInitHandleValidate(evtHandle, pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    CL_EVT_CHANNEL_NAME_VALIDATE(pChannelName);
    if (CL_EVT_FALSE == CL_EVT_VALIDATE_OPEN_CHANNEL_FLAG(evtChannelOpenFlag))
    {
        clLogError("EVT", "COP", 
                CL_EVENT_LOG_MSG_1_BAD_FLAGS, evtChannelOpenFlag);
        rc = CL_EVENT_ERR_BAD_FLAGS;
        goto failure;
    }

    /*
     * Generate channelId from channel Name by using 16 bit checksum 
     */
    rc = clNetworkCksm16bitCompute((ClUint8T *) pChannelName->value,
            (ClUint32T) pChannelName->length, &evtChannelID);
    if (CL_OK != rc)
    {
        goto failure;
    }

    /*
     * Create a Handle for the Channel Information
     */
    rc = clHandleCreate(pEvtClientHead->evtClientHandleDatabase,
            sizeof(ClEvtClientChannelInfoT ),
            pEvtChannelHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED, rc);
        rc = CL_EVENT_ERR_NO_RESOURCE;
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, 
            *pEvtChannelHandle, (void**)&pEvtCliChannelInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, *pEvtChannelHandle);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    channelScope = CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(evtChannelOpenFlag);
    pEvtCliChannelInfo->handleType = CL_CHANNEL_HANDLE;
    pEvtCliChannelInfo->evtHandle = evtHandle;
    pEvtCliChannelInfo->evtChannelKey =
        CL_EVT_CHANNEL_KEY_GEN(channelScope, evtChannelID);
    pEvtCliChannelInfo->flag = evtChannelOpenFlag;
    clEvtUtilsNameCpy(&pEvtCliChannelInfo->evtChannelName, pChannelName);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, *pEvtChannelHandle); 
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }
    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(evtChannelOpenRequest);

    /*
     * Checkout the pInitInfo to get the servHdl obtained from the server
     * during initialization
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }
    evtChannelOpenRequest.userId.evtHandle = pInitInfo->servHdl;

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    evtChannelOpenRequest.userId.eoIocPort = pEoObj->eoID;
    evtChannelOpenRequest.evtChannelHandle =
        CL_EVT_CHANNEL_HANDLE_FORM(evtChannelOpenFlag, evtChannelID);
    clEvtUtilsNameCpy(&evtChannelOpenRequest.evtChannelName, pChannelName);

    /*
     * Pack the channel information to pass it to the EM/S 
     */
    rc = clBufferCreate(pInMsgHandle);
    rc = clBufferCreate(pOutMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtChannelOpenRequestT, 4, 0, 0)(&evtChannelOpenRequest,
                                                                   *pInMsgHandle,
                                                                   0);
// success:
    clLogTrace("EVT", "COP", 
            "Event Channel Open Prologue succeeded");

    CL_FUNC_EXIT();
    return CL_OK;

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "COP", 
            "Event Channel Open Prologue failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEvtChannelOpenEpilogue(ClRcT rc, ClEvtClientHeadT *pEvtClientHead,
        ClEventChannelHandleT *pEvtChannelHandle,
        ClBufferHandleT *pInMsgHandle,
        ClBufferHandleT *pOutMsgHandle)
{
    CL_FUNC_ENTER();

    /*
     ** Release the resources. The inMsgHandle allocated in the Sync Call needs to be released.
     ** For the async open's callback the inMsgHandle shouldn't be touched. NULL is an indicator
     ** of the async call.
     */
    if (NULL != pInMsgHandle)
    {
        clBufferDelete(pInMsgHandle);    /* The message needs to be
                                          * deleted only for the Sync
                                          * Call */
    }

    /*
     ** Validate the result obtained from either sync or async calls.
     */
    if (CL_OK != rc && CL_EVENT_ERR_EXIST != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(*pOutMsgHandle,
                    (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "COP", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto failure;
            }

            clLogError("EVT", "COP", 
                    "Chan Open Version Nack Received"
                    "Supported Version is {'%c', 0x%x, 0x%x}",
                    pVersion->releaseCode, pVersion->majorVersion,
                    pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "COP", 
                    "Open Channel failed, rc=[%#X]", rc);
        }

        clBufferDelete(pOutMsgHandle);
        clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, *pEvtChannelHandle);
        goto failure;
    }

// success:
    clLogTrace("EVT", "COP", 
            "Event Channel Open succeeded");
    clBufferDelete(pOutMsgHandle);

    CL_FUNC_EXIT();
    return rc;                  /* Could be either CL_OK or CL_EVENT_ERR_EXIST */

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "COP", 
            "Event Channel Open Epilogue failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

/*
 * This function is wrapper over RMD call which is provided by EM/S 1. Verify
 * the information which is passed by the application. 2. Pack the information
 * which is passed by application. 3. Send the packed information to EM/S by
 * invoking RMD call. 
 */
ClRcT clEventChannelOpen(ClEventInitHandleT evtHandle,
        const ClNameT *pChannelName,
        ClEventChannelOpenFlagsT evtChannelOpenFlag,
        ClTimeT timeout, ClEventChannelHandleT *pChannelHandle)
{
    ClRcT rc = CL_OK;

    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClIocAddressT destAddr;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T tries = 0;

    CL_FUNC_ENTER();

    if (NULL == pChannelHandle)
    {
        clLogError("EVT", "COP", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pChannelHandle");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }

    rc = clEvtChannelOpenPrologue(evtHandle, pChannelName, evtChannelOpenFlag,
            pChannelHandle, &pEvtClientHead, &inMsgHandle,
            &outMsgHandle);
    if (CL_OK != rc)
    {
        goto failure;
    }


    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = timeout;
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_CHANNEL_OPEN, inMsgHandle,
                          outMsgHandle, CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);

    rc = clEvtChannelOpenEpilogue(rc, pEvtClientHead, pChannelHandle,
            &inMsgHandle, &outMsgHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_EVENT_LOG_MSG_3_CHANNEL_OPEN_FAILED, pChannelName->length,
                pChannelName->value, rc);
        goto failure;
    }
// success:
    clLogTrace("EVT", "COP", 
            "Event Channel Open succeeded with channelHdl[%#llX]",
            *pChannelHandle);

    CL_FUNC_EXIT();
    return CL_OK;

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "COP", 
            "Event Channel Open failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

void clEvtAsyncChanOpenCbReceive(ClRcT apiResult, void *pCookie,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;

    ClEvtInitInfoT *pInitInfo = NULL;

    ClEoExecutionObjT *pEoObj = { 0 };
    ClEvtClientHeadT *pEvtClientHead = NULL;

    ClEvtClientAsyncChanOpenCbArgT *pCallbackArg = pCookie;


    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO CL_EVT_CHECK_INIT_COUNT();
     */

    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);    /* Fetch the Client
                                                          * Head */
    if (CL_OK != rc)            /* If given EO is not initalized with EM
                                 * serivce */
    {
        goto failure;
    }

    /*
     ** Pass the inMsgHandle as NULL as we needn't free it for Async Call.
     */
    rc = clEvtChannelOpenEpilogue(apiResult, pEvtClientHead,
            &pCallbackArg->channelHandle, NULL,
            &outMsgHandle);
    if (CL_OK != rc)            /* rc is same as apiResult */
    { 
        clLogError("EVT", "COP", 
                CL_EVENT_LOG_MSG_1_ASYNC_CHANNEL_OPEN_FAILED, apiResult);
    }
    else
    {
        clLogTrace("EVT", "COP", 
                CL_EVENT_LOG_MSG_0_ASYNC_CHANNEL_OPEN_SUCCESS);
    }

    /*
     ** Using the evtHandle present in the pCallbackArg we now obtain the
     ** callback registered and invoke the same.
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, pCallbackArg->evtHandle, (void**)&pInitInfo);
    if (CL_OK != rc)
    { 
        clLogError("EVT", "COP", 
                "Finding the Channel Open Callback Failed, rc[%#X]", rc);
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    }

    if (NULL != pInitInfo->pEvtCallbackTable[0].callbacks.clEvtChannelOpenCallback)
    {
        /*
         * If the queue flag is set - Queue the callback with the arguments &
         * callback ID 
         */
        if (CL_TRUE == pInitInfo->queueFlag)
        {
            pCallbackArg->apiResult = apiResult;
            rc = clEvtQueueCallback(pInitInfo, CL_EVT_CHANNEL_CALLBACK, pCallbackArg);
            if (CL_OK != rc)
            {
                rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pCallbackArg->evtHandle);
                goto failure;
            }

            clLogTrace("EVT", "COP", CL_EVENT_LOG_MSG_0_EVENT_QUEUED);

            rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pCallbackArg->evtHandle);
            if (CL_OK != rc)
            { 
                clLogError("EVT", "COP", 
                        CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
                goto failure;
            }

            goto success;
        }

        clLogTrace("EVT", "COP", 
                CL_EVENT_LOG_MSG_0_ASYNC_CHAN_OPEN_CALLBACK);

        pInitInfo->pEvtCallbackTable[0].callbacks.clEvtChannelOpenCallback(pCallbackArg->
                invocation, pCallbackArg->channelHandle, apiResult);

        rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pCallbackArg->evtHandle);
        if (CL_OK != rc)
        { 
            clLogError("EVT", "COP", 
                    CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
            goto failure;
        }

    }

success:
failure:
    clHeapFree(pCallbackArg);   /* Release the allocated memory */

    CL_FUNC_EXIT();
    return;
}


ClRcT clEventChannelOpenAsync(ClEventInitHandleT evtHandle,
        ClInvocationT invocation,
        const ClNameT *pChannelName,
        ClEventChannelOpenFlagsT channelOpenFlags)
{
    ClRcT rc = CL_OK;

    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClIocAddressT destAddr;
    ClEvtClientAsyncChanOpenCbArgT *pCallbackArg = NULL;
    ClRmdAsyncOptionsT asyncRmdOptions = { 0 };
    ClEvtInitInfoT *pInitInfo = NULL;
    ClEoExecutionObjT *pEoObj;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};

    CL_FUNC_ENTER();

    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);    /* Fetch the client
                                                          * head */
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, evtHandle, (void **)&pInitInfo);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(NULL == pInitInfo->pEvtCallbackTable[0].callbacks.clEvtChannelOpenCallback)
    {
        clLogError("EVT", "COP", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "clEvtChannelOpenCallback");
        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
        rc = CL_EVENT_ERR_INIT;  
        goto failure;
    }
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }
    /*
     * Populate the argument to be passed to the Callback.
     */
    pCallbackArg = clHeapAllocate(sizeof(*pCallbackArg));
    if (NULL == pCallbackArg)
    {
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto failure;
    }

    /*
     **Channel Handle field of the Callback Argument will be obtained from the prologue
     */
    rc = clEvtChannelOpenPrologue(evtHandle, pChannelName, channelOpenFlags,
            &pCallbackArg->channelHandle, &pEvtClientHead,
            &inMsgHandle, &outMsgHandle);
    if (CL_OK != rc)
    {
        goto cbArgAllocated;
    }

    /*
     ** Fill in the other fields of callback Argument.
     */
    pCallbackArg->evtHandle = evtHandle;
    pCallbackArg->invocation = invocation;

    /*
     ** Populate the Async RMD options.
     */
    asyncRmdOptions.fpCallback = clEvtAsyncChanOpenCbReceive;
    asyncRmdOptions.pCookie = pCallbackArg;


    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_RMD_DEFAULT_TIMEOUT;

    /*
     * Update the destination address 
     */
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    /*
     ** The RMD call made for the Asyn Open differs from the Sync Open. It requires the
     ** async options with callback and cookie. Need Reply flag has to be set for the
     ** callback to execute. This implies that outMsgHandle must be specified. This out
     ** msg handle must be freed in the callback. To relieve the freeing of inMsgHandle
     ** the Non Persistent flag has been used. We no longer need to free inMsgHandle.
     */
    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_CHANNEL_OPEN, inMsgHandle,
                          outMsgHandle,
                          CL_RMD_CALL_ASYNC | CL_RMD_CALL_NEED_REPLY |
                          CL_RMD_CALL_NON_PERSISTENT, &rmdOptions,
                          &asyncRmdOptions);
    }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
           &&
           ++tries < 5
           &&
           clOsalTaskDelay(delay) == CL_OK);

    if (CL_OK != rc)
    { 
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_RMD_CALL_FAILED, rc);

        clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, 
                pCallbackArg->channelHandle);
        goto cbArgAllocated;
    }

// success:
    clLogTrace("EVT", "COP", 
            "Event Channel Open Async succeeded");
    CL_FUNC_EXIT();
    return CL_OK;

cbArgAllocated:
    clHeapFree(pCallbackArg);   /* Release the allocated memory */

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "COP", 
            "Event Channel Open Async failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEventChannelClose(ClEventChannelHandleT channelHandle)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT localIocAddress = 0;
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClEvtUnsubscribeEventRequestT unsubscribeRequest = { 0 };
    ClIocPortT eoIocPort = 0;
    ClEvtClientHeadT *pEvtClientHead;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtInitInfoT *pInitInfo = NULL;
    ClIocAddressT destAddr = {{0}};
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClEventInitHandleT servEvtHandle = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T tries = 0;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, channelHandle, (void **)&pEvtHdlDbData);
    if (CL_OK != rc)
    { 
        clLogError("EVT", "COP", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_CHANNEL_HANDLE)
    {
        pEvtChannelInfo = (ClEvtClientChannelInfoT *)pEvtHdlDbData;
    }
    else
    {
        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }
    /*
     * Pack the version Info 
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, pEvtChannelInfo->evtHandle, (void **)&pInitInfo);
    if(rc != CL_OK)
    {
        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }
    servEvtHandle = pInitInfo->servHdl;
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pEvtChannelInfo->evtHandle);
    if(rc != CL_OK)
    {
        clLogError("EVT", "CCL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

    clEvtClientToServerVersionSet(unsubscribeRequest);

    /*
     * Get user information 
     */
    rc = clEoMyEoIocPortGet(&eoIocPort);
    if (CL_OK != rc)
    {
        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
        goto failure;
    }

    /*
     * Pack the unsubscription information 
     */
    unsubscribeRequest.evtChannelHandle =
        CL_EVT_CHANNEL_HANDLE_FROM_KEY_FLAG(pEvtChannelInfo->flag,
                pEvtChannelInfo->evtChannelKey);
    unsubscribeRequest.subscriptionId = 0;
    unsubscribeRequest.userId.eoIocPort = pEoObj->eoID;
    unsubscribeRequest.userId.evtHandle = servEvtHandle;
    unsubscribeRequest.reqFlag = CL_EVT_CHANNEL_CLOSE;
    clEvtUtilsNameCpy(&unsubscribeRequest.evtChannelName,
            &pEvtChannelInfo->evtChannelName);
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
    if (CL_OK != rc)
    { 
        clLogError("EVT", "CCL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }
    /*
     * Create message handle and pack the user input 
     */
    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferCreate(&outMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtUnsubscribeEventRequestT, 4, 0, 0)(&unsubscribeRequest,
                                                                        inMsgHandle,
                                                                        0);

    /*
     * Get local IOC address 
     */
    localIocAddress = clIocLocalAddressGet();

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;
    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_UNSUBSCRIBE, inMsgHandle,
                          outMsgHandle, CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    { 
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "CCL", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            clLogError("EVT", "CCL", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Channel Close",
                    unsubscribeRequest.releaseCode,
                    unsubscribeRequest.majorVersion,
                    unsubscribeRequest.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "CCL", 
                    CL_EVENT_LOG_MSG_5_CHANNEL_CLOSE_FAILED,
                    pEvtChannelInfo->evtChannelName.length,
                    pEvtChannelInfo->evtChannelName.value, eoIocPort,
                    pEvtChannelInfo->evtHandle, rc);
        }
        goto outMsgHdlAllocated;
    }

    rc = clHandleWalk(pEvtClientHead->evtClientHandleDatabase, channelHdlWalk, (void * )&channelHandle); 
    rc = clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, channelHandle);
    if (CL_OK != rc)
    { 
        clLogError("EVT", "CCL", 
                CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
        goto outMsgHdlAllocated;
    }
    
// success:
    clLogTrace("EVT", "CCL", 
            "Event Channel Close succeeded for channelHandle[%#llX]",
            channelHandle);
    clBufferDelete(&outMsgHandle);
    CL_FUNC_EXIT();
    return CL_OK;

outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "CCL", 
            "Event Channel Close failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEventExtWithRbeSubscribe(const ClEventChannelHandleT channelHandle,
        ClRuleExprT *pRbeExpr,
        ClEventSubscriptionIdT subscriptionId,
        void *pCookie)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT localIocAddress = 0;
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClIocPortT subscriberCommPort = 0;
    ClEvtSubscribeEventRequestT subscribeRequest = {0};
    ClIocPortT eoIocPort = 0;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClIocAddressT destAddr = {{0}};
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClEvtInitInfoT *pInitInfo = NULL;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T tries = 0;
    ClUint32T i = 0;
    
    CL_FUNC_ENTER();
    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, channelHandle, (void**)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "SUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_CHANNEL_HANDLE)
    {
        pEvtChannelInfo = (ClEvtClientChannelInfoT *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto chanHdlCheckedOut;
    }

    CL_EVT_SUBSCRIBE_FLAG_VALIDATE(pEvtChannelInfo->flag);

    /*
     * Check if the clEvtEventDeliverCallback passed to the 
     * clEventInitialize API was NULL: Checkout the channel
     * information using the channelHandle and then checkout
     * Initialize information using the evtHandle obtained.
     * Also this gives the servHdl needed to send to the 
     * server.
     */
    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, pEvtChannelInfo->evtHandle, (void**)&pInitInfo); 
    if(CL_OK != rc)
    {
        clLogError("EVT", "SUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto chanHdlCheckedOut;
    }

    for(i = 0; i < pInitInfo->numCallbacks && 
            pInitInfo->pEvtCallbackTable[i].callbacks.clEvtEventDeliverCallback == NULL;
        ++i);

    if(i == pInitInfo->numCallbacks)
    {
        clLogError("EVT", "SUB", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "clEvtEventDeliverCallback");
        clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pEvtChannelInfo->evtHandle);
        rc = CL_EVENT_ERR_INIT;  
        goto chanHdlCheckedOut;
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pEvtChannelInfo->evtHandle);
    if (CL_OK != rc)
    { 
        clLogError("EVT", "SUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto chanHdlCheckedOut;
    }

    clEoMyEoIocPortGet(&subscriberCommPort);
    eoIocPort = subscriberCommPort;

    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(subscribeRequest);

    /*
     * Create message handle and pack the user input 
     */
    subscribeRequest.evtChannelHandle =
        CL_EVT_CHANNEL_HANDLE_FROM_KEY_FLAG(pEvtChannelInfo->flag,
                pEvtChannelInfo->evtChannelKey);

    subscribeRequest.subscriberCommPort = subscriberCommPort;
    subscribeRequest.userId.eoIocPort = pEoObj->eoID;
    subscribeRequest.userId.evtHandle = pEvtChannelInfo->evtHandle;
    // subscribeRequest.userId.evtHandle = pInitInfo->servHdl;  //NTC: Pass server handle to the server
    subscribeRequest.subscriptionId = subscriptionId;
    subscribeRequest.pCookie = (ClUint64T)(ClWordT)pCookie;

    clEvtUtilsNameCpy(&subscribeRequest.evtChannelName,
            &pEvtChannelInfo->evtChannelName);

    if (pRbeExpr)
    {
        rc = clRuleExprPack(pRbeExpr,
                            &subscribeRequest.packedRbe,
                            &subscribeRequest.packedRbeLen);
        if (rc != CL_OK)
            goto chanHdlCheckedOut;
    }
    
    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferCreate(&outMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtSubscribeEventRequestT, 4, 0, 0)(&subscribeRequest,
                                                                      inMsgHandle,
                                                                      0);
    
    if (subscribeRequest.packedRbe)
    {
        clHeapFree(subscribeRequest.packedRbe);
    }
    
    /*
     * Get local IOC address 
     */
    localIocAddress = clIocLocalAddressGet();

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;

    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_SUBSCRIBE, inMsgHandle, outMsgHandle,
                          CL_RMD_CALL_ATMOST_ONCE | CL_RMD_CALL_NEED_REPLY,
                          &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&inMsgHandle);
    if(CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "SUB", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            clLogError("EVT", "SUB", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Subscription",
                    subscribeRequest.releaseCode,
                    subscribeRequest.majorVersion,
                    subscribeRequest.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "SUB", 
                    CL_EVENT_LOG_MSG_6_SUBSCRIBE_FAILED, subscriptionId,
                    pEvtChannelInfo->evtChannelName.length,
                    pEvtChannelInfo->evtChannelName.value, eoIocPort,
                    pEvtChannelInfo->evtHandle, rc);
        }

        goto outMsgHdlAllocated;
    }

// success:
    clBufferDelete(&outMsgHandle);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "SUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "SUB", 
            "Event Subscribe with RBE succeeded");

    CL_FUNC_EXIT();
    return CL_OK;

outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

chanHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "SUB", 
            "Event Subscribe with RBE failed, rc[%#X]", rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventExtSubscribe(const ClEventChannelHandleT channelHandle,
        ClUint32T eventType,
        ClEventSubscriptionIdT subscriptionId, void *pCookie)
{
    ClRcT rc = CL_OK;

    ClEventFilterT filter = { CL_EVENT_EXACT_FILTER, {0, sizeof(ClUint32T),
        (ClUint8T *) &eventType}
    };
    ClEventFilterArrayT filterArray =
    { sizeof(filter) / sizeof(ClEventFilterT), &filter };

    CL_FUNC_ENTER();
    rc = clEventSubscribe(channelHandle, &filterArray, subscriptionId, pCookie);

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventSubscribe(const ClEventChannelHandleT channelHandle,
        const ClEventFilterArrayT *pFilters,
        ClEventSubscriptionIdT subscriptionId, ClPtrT pCookie)
{
    ClRcT rc = CL_OK;

    ClRuleExprT *pRbeExpr = NULL;

    rc = clEvtUtilsFilter2Rbe(pFilters, &pRbeExpr);
    if(CL_OK != rc)
    {
        clLogError("EVT", "SUB", 
                "clEvtUtilsFilter2Rbe failed, rc[%#X]", rc);
        goto failure;
    }

    rc = clEventExtWithRbeSubscribe(channelHandle, pRbeExpr, subscriptionId, pCookie);
    if(CL_OK != rc)
    {
        goto rbeAllocated;
    }

    if (NULL != pRbeExpr)
    {
        clRuleExprDeallocate(pRbeExpr);
    }

// success:
    clLogTrace("EVT", "SUB", 
            "Event Subscribe succeeded for channelHandle[%#llX]"
            "subscriptionId[%d]", 
            channelHandle, subscriptionId);
    CL_FUNC_EXIT();

    return CL_OK;

rbeAllocated:
    if (NULL != pRbeExpr)
    {
        clRuleExprDeallocate(pRbeExpr);
    }

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "SUB", 
            "Event Subscribe failed for channelHandle[%#llX]"
            "subscriptionId[%d] with rc[%#X]", 
            channelHandle, subscriptionId, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventUnsubscribe(ClEventChannelHandleT channelHandle,
        ClEventSubscriptionIdT subscriptionId)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT localIocAddress = 0;
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClEvtUnsubscribeEventRequestT unsubscribeRequest = { 0 };
    ClRmdOptionsT rmdOptions = { 0 };
    ClIocPortT eoIocPort = 0;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClIocAddressT destAddr = {{0}};
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, channelHandle, (void **)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "USB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_CHANNEL_HANDLE)
    {
        pEvtChannelInfo = (ClEvtClientChannelInfoT *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto chanHdlCheckedOut;
    }

    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(unsubscribeRequest);

    rc = clEoMyEoIocPortGet(&eoIocPort);

    /*
     * Pack the unsubscription information 
     */
    unsubscribeRequest.evtChannelHandle =
        CL_EVT_CHANNEL_HANDLE_FROM_KEY_FLAG(pEvtChannelInfo->flag,
                pEvtChannelInfo->evtChannelKey);

    unsubscribeRequest.subscriptionId = subscriptionId;
    unsubscribeRequest.userId.eoIocPort = pEoObj->eoID; 
    unsubscribeRequest.userId.evtHandle = pEvtChannelInfo->evtHandle;
    // unsubscribeRequest.userId.evtHandle = pInitInfo->servHdl;  //NTC Pass server handle to the server
    unsubscribeRequest.reqFlag = CL_EVT_UNSUBSCRIBE;

    clEvtUtilsNameCpy(&unsubscribeRequest.evtChannelName,
            &pEvtChannelInfo->evtChannelName);

    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferCreate(&outMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtUnsubscribeEventRequestT, 4, 0, 0)(&unsubscribeRequest,
                                                                        inMsgHandle,
                                                                        0);

    /*
     * Get local IOC address 
     */
    localIocAddress = clIocLocalAddressGet();

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;
    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_UNSUBSCRIBE, inMsgHandle,
                          outMsgHandle, CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
           &&
           ++tries < 5
           &&
           clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "USB", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            clLogError("EVT", "USB", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Unsubscription",
                    unsubscribeRequest.releaseCode,
                    unsubscribeRequest.majorVersion,
                    unsubscribeRequest.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "USB", 
                    CL_EVENT_LOG_MSG_6_UNSUBSCRIBE_FAILED, subscriptionId,
                    pEvtChannelInfo->evtChannelName.length,
                    pEvtChannelInfo->evtChannelName.value, eoIocPort,
                    pEvtChannelInfo->evtHandle, rc);
        }
        goto outMsgHdlAllocated;
    }

// success:
    clBufferDelete(&outMsgHandle);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "USB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "USB", 
            "Event Unsubscribe succeeded for channelHandle[%#llX]"
            "subscriptionId[%d]", channelHandle, subscriptionId);
    CL_FUNC_EXIT();
    return CL_OK;

outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

chanHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "USB", 
            "Event Unsubscribe failed for channelHandle[%#llX]"
            "subscriptionId[%d] with rc[%#X]", 
            channelHandle, subscriptionId, rc);
    CL_FUNC_EXIT();
    return rc;
}

/*
 * This function will be allocate message for event header.
 * We have a 8 bit version which could be used to interpret publishes across different
 * versions in a safe way
 */

ClRcT clEventAllocateWithVersion(ClEventChannelHandleT channelHandle,
                                 ClUint8T version,
                                 ClEventHandleT *pEventHandle)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT *pEventInfo = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };

    /*
     * Added for Bug 3129 
     */
    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();


    /*
     * Validate the parameter(s) 
     */
    if (NULL == pEventHandle)
    {
        clLogError("EVT", "EAL", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pEventHandle");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase,channelHandle, (void**)&pEvtHdlDbData);
    if (CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_CHANNEL_HANDLE)
    {
        pEvtChannelInfo = (ClEvtClientChannelInfoT *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto chanHdlCheckedOut;
    }
    /*
     ** Verify if publisher since only a publisher can alter the attributes
     ** of an Event. If not publisher return error.
     */
    CL_EVT_PUBLISH_FLAG_VALIDATE(pEvtChannelInfo->flag);

    rc = clHandleCreate(pEvtClientHead->evtClientHandleDatabase, sizeof(*pEventInfo), (ClHandleT *)pEventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED, rc);
        rc = CL_EVENT_ERR_NO_RESOURCE;
        goto chanHdlCheckedOut;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, *pEventHandle, (void **)&pEventInfo);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto eventHdlAllocated;
    }

    pEventInfo->handleType = CL_EVENT_HANDLE; 
    pEventInfo->evtChannelHandle = channelHandle;
    rc = clBufferCreate(&pEventInfo->msgHandle);

    /*
     * Set the default values for the Event by making a call to AttributesSet()
     * with the default values after successfullly allocatint the Event.
     * For no patterns one has to specify a non-null array with 0 patterns.
     * Refer bug 3129.
     */

    clEvtClientToServerVersionSet(evtPrimaryHeader);
    evtPrimaryHeader.version = version;
    evtPrimaryHeader.eventPriority = CL_EVENT_LOWEST_PRIORITY;
    evtPrimaryHeader.channelNameLen = pEvtChannelInfo->evtChannelName.length;
    CL_EVT_CHANNEL_SCOPE_GET(pEvtChannelInfo->evtChannelKey,
            evtPrimaryHeader.channelScope);

    CL_EVT_CHANNEL_ID_GET(pEvtChannelInfo->evtChannelKey, evtPrimaryHeader.channelId);  

    rc = clBufferClear(pEventInfo->msgHandle);

    rc = clBufferNBytesWrite(pEventInfo->msgHandle,
            (ClUint8T *) &evtPrimaryHeader,
            sizeof(ClEvtEventPrimaryHeaderT));
    /*
     * Pack the channel name information 
     */
    rc = clBufferNBytesWrite(pEventInfo->msgHandle,
            (ClUint8T *) pEvtChannelInfo->evtChannelName.value,
            pEvtChannelInfo->evtChannelName.length);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, *pEventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        clBufferDelete(&pEventInfo->msgHandle);
        goto eventHdlAllocated;
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EAL", 
            "Event Allocate succeeded for channelHandle[%#llX],"
            "with eventHandle[%#llX]",
            channelHandle, *pEventHandle);
    CL_FUNC_EXIT();
    return rc;

eventHdlAllocated:
    clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, *pEventHandle);

chanHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, channelHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EAL", 
            "Event Allocate failed for channelHandle[%#llX], rc[%#X]",
            channelHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventAllocate(ClEventChannelHandleT channelHandle,
                      ClEventHandleT *pEventHandle)
{
    return clEventAllocateWithVersion(channelHandle, CL_EVT_DATA_VERSION_BASE, pEventHandle);
}

ClRcT clEventExtAttributesSet(ClEventHandleT eventHandle, ClUint32T eventType,
        ClEventPriorityT priority, ClTimeT retentionTime,
        const ClNameT *pPublisherName)
{
    ClRcT rc = CL_OK;
    ClEventPatternT pattern = { 0, sizeof(ClUint32T), (ClUint8T *) &eventType };
    ClEventPatternArrayT patternArray =
    { 0, sizeof(pattern) / sizeof(ClEventPatternT), &pattern };

    CL_FUNC_ENTER();
    rc = clEventAttributesSet(eventHandle, &patternArray, priority,
            retentionTime, pPublisherName);

    CL_FUNC_EXIT();
    return rc;
}


/*
 * User can construct event related information and pass this information while 
 * publishing the event. 
 */

ClRcT clEventAttributesSet(ClEventHandleT eventHandle,
        const ClEventPatternArrayT *pPatternArray,
        ClEventPriorityT priority, ClTimeT retentionTime,
        const ClNameT *pPublisherName)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT *pEventHandle = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };
    ClUint32T noOfBytesToRead = 0;
    ClUint32T i = 0;
    ClUint32T patternSize = 0;
    ClEoExecutionObjT *pEoObj;
    ClUint32T messageLength = 0;
    ClUint32T readOffset = 0;
    ClBufferHandleT tempPatternArray = 0;   
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;

    CL_FUNC_ENTER();


    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();


    /*
     * Validate the parameter(s) 
     */


    if (CL_EVENT_LOWEST_PRIORITY < priority)
    {
        clLogError("EVT", "EAL", 
                CL_EVENT_LOG_MSG_1_INVALID_PARAMETER,
                "Priority is less than CL_EVENT_LOWEST_PRIORITY");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }



    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void **)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }
    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventHandle = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }


    noOfBytesToRead = sizeof(ClEvtEventPrimaryHeaderT);
    rc = clBufferReadOffsetGet (pEventHandle->msgHandle, &readOffset);
    rc = clBufferLengthGet (pEventHandle->msgHandle, &messageLength);
    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, 0,
            CL_BUFFER_SEEK_SET);
    rc = clBufferNBytesRead(pEventHandle->msgHandle, (ClUint8T *)&evtPrimaryHeader, &noOfBytesToRead);

    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, readOffset, CL_BUFFER_SEEK_SET);

    evtPrimaryHeader.eventPriority = priority;
    evtPrimaryHeader.retentionTime = retentionTime;
    /*
     * Case 1: 
     * When clEventAttributesSet is called for the first time
     * the evtPrimary Header is already in place and has emptyPublisher
     * Name and patterns initialized when allocated. 
     * So, the length of the buffer should be sizeof primaryHeader + the
     * channelName Length 
     */
    if((sizeof(evtPrimaryHeader) + evtPrimaryHeader.channelNameLen
       ) == (messageLength))
    {
        /*
         * We are calling Attributes Set for the first Time after 
         * Event Allocation
         */

        if(pPublisherName != NULL)
        {
            evtPrimaryHeader.publisherNameLen = pPublisherName->length;
        }

        if (NULL != pPatternArray)
        {
            evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;

            for (i = 0; i < pPatternArray->patternsNumber; i++)
            {
                patternSize =
                    pPatternArray->pPatterns[i].patternSize + sizeof(ClSizeT);
                evtPrimaryHeader.patternSectionLen += patternSize;
            }
        }

        rc = clBufferWriteOffsetSet(pEventHandle->msgHandle,
                0, CL_BUFFER_SEEK_SET);

        rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                (ClUint8T *) &evtPrimaryHeader,
                sizeof(ClEvtEventPrimaryHeaderT));

        /*
         * Move the Write Offset So as to set it after the Channel
         * Name
         */
        rc = clBufferWriteOffsetSet(pEventHandle->msgHandle,
                evtPrimaryHeader.channelNameLen, CL_BUFFER_SEEK_CUR);
        /*
         * if pPublisherName is non null, then write it to the buffer
         * else its already empty
         */
        if(NULL != pPublisherName)
        {
            rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                    (ClUint8T *) pPublisherName->value,
                    pPublisherName->length);
        }
        /*
         * Pack the Pattern Information if any 
         */
        if (NULL != pPatternArray)
        {
            /*
             * Pack the event pattern 
             */
            evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;
            for (i = 0; i < pPatternArray->patternsNumber; i++)
            {
                /*
                 * Fillin pattern length 
                 */
                rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                        (ClUint8T *) &pPatternArray->
                        pPatterns[i].patternSize,
                        sizeof(ClSizeT));

                /*
                 * Fillin pattern data 
                 */
                patternSize = pPatternArray->pPatterns[i].patternSize;
                rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                        (ClUint8T *) pPatternArray->
                        pPatterns[i].pPattern, patternSize);
            }
        }
    }

    /*
     * Case 2: 
     * This is the Case when we are setting the values of an allocated 
     * event for the second/or more time. Here if the pPublisherName or
     * pPatternArray are NULL, then we need to retain the older Value 
     * for these attributes of the event.
     */
    else
    {
        /*
         * We have four cases here: 
         * 1. we pass both pPublisherName & pPatternArray as NULL
         * 2. pPublisherName is NULL and pPatternArray is valid new val
         * 3. pPatternArray is NULL and pPublisherName is valid.
         * 4. both are valid and Non Null.
         */
        if((NULL == pPatternArray) && (NULL == pPublisherName))
        {
            /*
             * Both are NULL so we need to preserve both 
             * Copy the evtPrimaryHeader with modified 
             * priority , retention time etc. to the buffer 
             */

            rc = clBufferWriteOffsetSet (pEventHandle->msgHandle, 0, 
                    CL_BUFFER_SEEK_SET);

            rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                    (ClUint8T *) &evtPrimaryHeader,
                    sizeof(ClEvtEventPrimaryHeaderT));

        }

        if((NULL == pPublisherName) && (pPatternArray != NULL))
        {
            /*
             * Publisher Name is Null, we need to retain the publisher 
             * Name and populate the New pPatterArray into the buffer
             * Here PatternSectionLength may get modified 
             */

            /*
             * Get pattern length: Here the pattern Section length needs
             * to be reinitialized else it will grow with every attrib. 
             * set 
             */

            if (NULL != pPatternArray)
            {
                evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;
                evtPrimaryHeader.patternSectionLen = 0;
                for (i = 0; i < pPatternArray->patternsNumber; i++)
                {
                    patternSize =
                        pPatternArray->pPatterns[i].patternSize + sizeof(ClSizeT);
                    evtPrimaryHeader.patternSectionLen += patternSize;
                }
            }

            rc = clBufferWriteOffsetSet (pEventHandle->msgHandle, 0, 
                    CL_BUFFER_SEEK_SET);

            rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                    (ClUint8T *) &evtPrimaryHeader,
                    sizeof(ClEvtEventPrimaryHeaderT));

            rc = clBufferWriteOffsetSet(pEventHandle->msgHandle,
                    evtPrimaryHeader.channelNameLen, CL_BUFFER_SEEK_CUR);
            /* 
             * Set the offset ahead by the size of previous publisherName
             * so that its not read as size in attributesGet api
             */
            rc = clBufferWriteOffsetSet(pEventHandle->msgHandle, 
                    evtPrimaryHeader.publisherNameLen, 
                    CL_BUFFER_SEEK_CUR);
            /*
             * Pack the Pattern Information if any 
             */
            if (NULL != pPatternArray)
            {
                /*
                 * Pack the event pattern 
                 */
                evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;
                for (i = 0; i < pPatternArray->patternsNumber; i++)
                {
                    /*
                     * Fillin pattern length 
                     */
                    rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                            (ClUint8T *) &pPatternArray->
                            pPatterns[i].patternSize,
                            sizeof(ClSizeT));

                    /*
                     * Fillin pattern data 
                     */
                    patternSize = pPatternArray->pPatterns[i].patternSize;
                    rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                            (ClUint8T *) pPatternArray->
                            pPatterns[i].pPattern, patternSize);
                }
            }


        }

        if((NULL == pPatternArray) && (pPublisherName != NULL))
        {
            /*
             * Here we have a publisherName which is Non Null 
             * and can be of a different length than the prev 
             * publisherName, So to preserve the Patterns , we
             * need to first read the patterns into a buffer 
             * , write the new publisher Name and then copy the 
             * patterns back
             */

            if(pPublisherName->length != evtPrimaryHeader.publisherNameLen)
            {
                /*
                 * read the patternArray into a temp buffer
                 */
                rc = clBufferCreate(&tempPatternArray);
                if(CL_OK == rc)
                {
                    /*
                     * Copy the pattern into the tempPatternArray 
                     * Move the Read offset by evtPrimaryHeader 
                     * + channelNameLen + publisherNamelen
                     */

                    rc = clBufferToBufferCopy(pEventHandle->msgHandle, 
                            (sizeof(evtPrimaryHeader) + evtPrimaryHeader.channelNameLen + evtPrimaryHeader.publisherNameLen), 

                            tempPatternArray, 
                            evtPrimaryHeader.patternSectionLen);

                    /*
                     * Now update the evtPrimaryHeader with the new 
                     * publisher Name Len 
                     */
                    evtPrimaryHeader.publisherNameLen = pPublisherName->length;
                    /*
                     * Move the Write offset back to 0 
                     */
                    rc = clBufferWriteOffsetSet(
                            pEventHandle->msgHandle, 0,
                            CL_BUFFER_SEEK_SET);

                    rc = clBufferNBytesWrite(
                            pEventHandle->msgHandle,
                            (ClUint8T *) &evtPrimaryHeader,
                            sizeof(ClEvtEventPrimaryHeaderT));
                    /*
                     * Copy the Publisher Name 
                     */
                    rc = clBufferNBytesWrite(
                            pEventHandle->msgHandle,
                            (ClUint8T *) pPublisherName->value,
                            pPublisherName->length);

                    /*
                     * Set the write offset back to 0 for tempArr.
                     */
                    rc = clBufferWriteOffsetSet(
                            tempPatternArray, 0,
                            CL_BUFFER_SEEK_SET);

                    /*
                     * Copy the Pattern back to the buffer
                     */
                    rc = clBufferToBufferCopy(tempPatternArray,
                            0, 
                            pEventHandle->msgHandle,
                            evtPrimaryHeader.patternSectionLen);
                    /*
                     * Delete the tempBuffer 
                     */
                    rc = clBufferDelete(&tempPatternArray); 
                }
            }
            else
            {
                /*
                 * PublisherName->length is same as the previous one
                 */

                /*
                 * Now update the evtPrimaryHeader with the new 
                 * publisher Name Len 
                 */
                evtPrimaryHeader.publisherNameLen = pPublisherName->length;
                /*
                 * Move the Write offset back to 0 
                 */
                rc = clBufferWriteOffsetSet(
                        pEventHandle->msgHandle, 0,
                        CL_BUFFER_SEEK_SET);

                rc = clBufferNBytesWrite(
                        pEventHandle->msgHandle,
                        (ClUint8T *) &evtPrimaryHeader,
                        sizeof(ClEvtEventPrimaryHeaderT));
                /*
                 * Copy the Publisher Name 
                 */
                rc = clBufferNBytesWrite(
                        pEventHandle->msgHandle,
                        (ClUint8T *) pPublisherName->value,
                        pPublisherName->length);
            }
        }

        if((NULL != pPublisherName) && (NULL != pPatternArray))
        {
            /*
             * Neither the PublisherName nor the PatternArray are 
             * NULL so we need to rewrite them in case of Multiple
             * AttributeSets. Dont Need to preserve any of them. 
             */
            evtPrimaryHeader.publisherNameLen = pPublisherName->length;

            rc = clBufferWriteOffsetSet (pEventHandle->msgHandle, 0, 
                    CL_BUFFER_SEEK_SET);

            /*
             * Get pattern length: Here the pattern Section length needs
             * to be reinitialized else it will grow with every attrib. 
             * set 
             */

            if (NULL != pPatternArray)
            {
                evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;
                evtPrimaryHeader.patternSectionLen = 0;
                for (i = 0; i < pPatternArray->patternsNumber; i++)
                {
                    patternSize =
                        pPatternArray->pPatterns[i].patternSize + sizeof(ClSizeT);
                    evtPrimaryHeader.patternSectionLen += patternSize;
                }
            }
            /*
             * Copy the evtPrimaryHeader to the buffer
             */
            rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                    (ClUint8T *) &evtPrimaryHeader,
                    sizeof(ClEvtEventPrimaryHeaderT));


            /*
             * Move the Write Offset So as to set it after the Channel
             * Name
             */
            rc = clBufferWriteOffsetSet(pEventHandle->msgHandle,
                    evtPrimaryHeader.channelNameLen, CL_BUFFER_SEEK_CUR);


            /*
             * Copy the publisher Name 
             */
            rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                    (ClUint8T *) pPublisherName->value,
                    pPublisherName->length);


            /*
             * Pack the Pattern Information  
             */
            if (NULL != pPatternArray)
            {
                /*
                 * Pack the event pattern 
                 */
                evtPrimaryHeader.noOfPatterns = pPatternArray->patternsNumber;
                for (i = 0; i < pPatternArray->patternsNumber; i++)
                {
                    /*
                     * Fillin pattern length 
                     */
                    rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                            (ClUint8T *) &pPatternArray->
                            pPatterns[i].patternSize,
                            sizeof(ClSizeT));

                    /*
                     * Fillin pattern data 
                     */
                    patternSize = pPatternArray->pPatterns[i].patternSize;
                    rc = clBufferNBytesWrite(pEventHandle->msgHandle,
                            (ClUint8T *) pPatternArray->
                            pPatterns[i].pPattern, patternSize);
                }
            }

        }

    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAL", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EAS", 
            "Event Attribute Set succeeded for eventHandle[%#llX]",
            eventHandle);

    CL_FUNC_EXIT();
    return rc;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EAS", 
            "Event Attribute Set failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEventCookieGet(ClEventHandleT eventHandle, void **ppCookie)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT *pEventHandle = NULL;
    ClUint32T noOfBytesToRead = 0;
    ClUint32T readOffset = 0;
    ClUint32T offset = 0;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;

    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };
    ClEvtEventSecondaryHeaderT evtSecondaryHeader = { 0 };


    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Validate prameters 
     */
    if (NULL == ppCookie)
    {
        clLogError("EVT", "ECG", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "ppCookie");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }
    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void**)&pEvtHdlDbData);  
    if(CL_OK != rc)
    {
        clLogError("EVT", "ECG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventHandle = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    /*
     * Need to get the marker of message before read any content and reset to
     * the same marker posion this will get to user can read more than once on
     * the data 
     */
    rc = clBufferReadOffsetGet(pEventHandle->msgHandle, &readOffset);

    noOfBytesToRead = sizeof(ClEvtEventPrimaryHeaderT);
    rc = clBufferNBytesRead(pEventHandle->msgHandle,
            (ClUint8T *) &evtPrimaryHeader,
            &noOfBytesToRead);

    offset =
        sizeof(ClEvtEventPrimaryHeaderT) + evtPrimaryHeader.channelNameLen +
        evtPrimaryHeader.publisherNameLen + evtPrimaryHeader.patternSectionLen +
        evtPrimaryHeader.eventDataSize;

    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, offset,
            CL_BUFFER_SEEK_SET);

    noOfBytesToRead = sizeof(ClEvtEventSecondaryHeaderT);
    rc = clBufferNBytesRead(pEventHandle->msgHandle,
            (ClUint8T *) &evtSecondaryHeader,
            &noOfBytesToRead);

    *ppCookie = (ClPtrT)(ClWordT)evtSecondaryHeader.pCookie;

    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, readOffset,
            CL_BUFFER_SEEK_SET);
    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "ECG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EAS", 
            "Event Cookie Get succeeded for eventHandle[%#llX]", eventHandle);
    CL_FUNC_EXIT();
    return CL_OK;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EAS", 
            "Event Cookie Get failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventExtAttributesGet(ClEventHandleT eventHandle, ClUint32T *pEventType,
        ClEventPriorityT * pPriority,
        ClTimeT *pRetentionTime, ClNameT *pPublisherName,
        ClTimeT *pPublishTime, ClEventIdT * pEventId)
{
    ClRcT rc = CL_OK;
    ClEventPatternArrayT patternArray = { 0 };

    CL_FUNC_ENTER();

    rc = clEventAttributesGet(eventHandle, &patternArray, pPriority,
            pRetentionTime, pPublisherName, pPublishTime,
            pEventId);

    memcpy(pEventType, patternArray.pPatterns->pPattern, sizeof(ClUint32T));
    
    clHeapFree(patternArray.pPatterns->pPattern);
    clHeapFree(patternArray.pPatterns);

    CL_FUNC_EXIT();
    return rc;
}

/*
 * This function will get the event attributes 
 */
ClRcT clEventAttributesGet(ClEventHandleT eventHandle,
        ClEventPatternArrayT *pPatternArray,
        ClEventPriorityT * pPriority,
        ClTimeT *pRetentionTime, ClNameT *pPublisherName,
        ClTimeT *pPublishTime, ClEventIdT * pEventId)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT *pEventHandle = NULL;
    ClUint32T noOfBytesToRead = 0;
    ClUint32T readOffset = 0;
    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };
    ClUint32T i = 0;
    ClBoolT isPatternAllocated = CL_FALSE;
    ClBoolT isErrorNoSpace = CL_FALSE;
    ClEvtClientHeadT *pEvtClientHead;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClEoExecutionObjT *pEoObj = NULL;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();


    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void**)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventHandle = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    /*
     * Need to get the marker of message before read any content and reset to
     * the same marker posion this will get to user can read more than once on
     * the data 
     */

    noOfBytesToRead = sizeof(ClEvtEventPrimaryHeaderT);

    rc = clBufferReadOffsetGet(pEventHandle->msgHandle, &readOffset);
    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, 
            0, CL_BUFFER_SEEK_SET); 

    rc = clBufferNBytesRead(pEventHandle->msgHandle,
            (ClUint8T *) &evtPrimaryHeader,
            &noOfBytesToRead);

    /*
     * Fill the information 
     */
    if (NULL != pPriority)
    {
        *pPriority = evtPrimaryHeader.eventPriority;
    }

    if (NULL != pRetentionTime)
    {
        *pRetentionTime = evtPrimaryHeader.retentionTime;
    }

    if (NULL != pPublishTime)
    {
        *pPublishTime = evtPrimaryHeader.publishTime;
    }

    if (NULL != pPublisherName)
    {
        ClUint32T len = evtPrimaryHeader.publisherNameLen;

        rc = clBufferReadOffsetSet(pEventHandle->msgHandle,
                evtPrimaryHeader.channelNameLen,
                CL_BUFFER_SEEK_CUR);
        rc = clBufferNBytesRead(pEventHandle->msgHandle,
                (ClUint8T *) &pPublisherName->value,
                &len);
        pPublisherName->length = len;
    }
    else
    {
        /*
         * Update the offset explicity 
         */

        rc = clBufferReadOffsetSet(pEventHandle->msgHandle,
                evtPrimaryHeader.channelNameLen +
                evtPrimaryHeader.publisherNameLen,
                CL_BUFFER_SEEK_CUR);
    }

    if (NULL != pEventId)
    {
        *pEventId = evtPrimaryHeader.eventId;
    }

    /*
     * Unpack the pattern information 
     */
    if (NULL != pPatternArray)
    {
        if (NULL == pPatternArray->pPatterns)
        {
            if (0 != evtPrimaryHeader.noOfPatterns)
                pPatternArray->pPatterns =
                    clHeapAllocate(sizeof(ClEventPatternT) *
                            evtPrimaryHeader.noOfPatterns);
            isPatternAllocated = CL_FALSE;
        }
        else
        {
            if (pPatternArray->allocatedNumber < evtPrimaryHeader.noOfPatterns)
            {
                isErrorNoSpace = CL_TRUE;
            }
            isPatternAllocated = CL_TRUE;
        }

        pPatternArray->patternsNumber = evtPrimaryHeader.noOfPatterns;

        if (CL_FALSE == isPatternAllocated)
        {
            for (i = 0; i < evtPrimaryHeader.noOfPatterns; i++)
            {
                /*
                 * Get pattern length 
                 */
                noOfBytesToRead = sizeof(ClSizeT);
                rc = clBufferNBytesRead(pEventHandle->msgHandle,
                        (ClUint8T *) &(pPatternArray->
                            pPatterns[i].
                            patternSize),
                        &noOfBytesToRead);

                /*
                 * NOTE - noOfBytesToRead 32 bits while patternSize
                 * 64 bits 
                 */
                noOfBytesToRead = pPatternArray->pPatterns[i].patternSize;
                pPatternArray->pPatterns[i].pPattern =
                    clHeapAllocate(noOfBytesToRead);

                /*
                 * Get the pattern 
                 */
                rc = clBufferNBytesRead(pEventHandle->msgHandle,
                        (ClUint8T *) pPatternArray->
                        pPatterns[i].pPattern,
                        &noOfBytesToRead);
            }
        }
        else
        {
            for (i = 0;
                    i < pPatternArray->allocatedNumber &&
                    i < evtPrimaryHeader.noOfPatterns; i++)
            {
                /*
                 * Get pattern length 
                 */
                noOfBytesToRead = sizeof(ClSizeT);
                rc = clBufferNBytesRead(pEventHandle->msgHandle,
                        (ClUint8T *) &(pPatternArray->
                            pPatterns[i].
                            patternSize),
                        &noOfBytesToRead);

                noOfBytesToRead = pPatternArray->pPatterns[i].patternSize;
                if (pPatternArray->pPatterns[i].allocatedSize < noOfBytesToRead)
                {
                    isErrorNoSpace = CL_TRUE;
                }

                if (CL_FALSE == isErrorNoSpace)
                {
                    /*
                     * Validate pPattern 
                     */
                    if (NULL == pPatternArray->pPatterns[i].pPattern)
                    {
                        rc = clBufferReadOffsetSet(pEventHandle->
                                msgHandle, readOffset,
                                CL_BUFFER_SEEK_SET);
                        clLogError("EVT", "EAG", 
                                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT,
                                "pPattern in pPatternArray");

                        rc = CL_EVENT_ERR_INVALID_PARAM;
                        goto eventHdlCheckedOut;
                    }

                    /*
                     * Get the pattern 
                     */
                    rc = clBufferNBytesRead(pEventHandle->msgHandle,
                            (ClUint8T *) pPatternArray->
                            pPatterns[i].pPattern,
                            &noOfBytesToRead);
                }
            }
        }

        if (CL_TRUE == isErrorNoSpace)
        {
            rc = clBufferReadOffsetSet(pEventHandle->msgHandle,
                    readOffset, CL_BUFFER_SEEK_SET);
            clLogError("EVT", "EAG", 
                    "Insufficient space allocated for pattern, rc[%#X]",
                    CL_EVENT_ERR_NO_SPACE);

            rc = CL_EVENT_ERR_NO_SPACE;
            goto eventHdlCheckedOut;
        }

    }                           /* End of Pattern Get */

    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, readOffset,
            CL_BUFFER_SEEK_SET);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EAG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EAG", 
            "Event Attribute Get succeeded for eventHandle[%#llX]", 
            eventHandle);
    CL_FUNC_EXIT();
    return rc;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EAG", 
            "Event Attribute Get failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventDataGet(ClEventHandleT eventHandle, void *pEventData,
        ClSizeT *pEventDataSize)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT * pEventHandle = NULL;
    ClUint32T noOfBytesToRead = sizeof(ClEvtEventPrimaryHeaderT);
    ClUint32T readOffset = 0;
    ClUint32T newDataOffset = 0;
    ClUint32T msgLen = 0;
    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void **)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EDG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventHandle = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    /*
     * Need to get the marker of message before read any content and reset to
     * the same marker posion this will get to user can read more than once on
     * the data 
     */
    rc = clBufferReadOffsetGet(pEventHandle->msgHandle, &readOffset);

    rc = clBufferLengthGet(pEventHandle->msgHandle, &msgLen);

    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, 0,
            CL_BUFFER_SEEK_SET);

    /*
     * read primary event header 
     */
    rc = clBufferNBytesRead(pEventHandle->msgHandle,
            (ClUint8T *) &evtPrimaryHeader,
            (ClUint32T *) &noOfBytesToRead);

    /*
     * Move the offset to get the event pay load data lenght 
     */
    newDataOffset =
        evtPrimaryHeader.patternSectionLen + evtPrimaryHeader.channelNameLen +
        evtPrimaryHeader.publisherNameLen;

    if (msgLen > sizeof(ClEvtEventPrimaryHeaderT) + newDataOffset)
    {
        rc = clBufferReadOffsetSet(pEventHandle->msgHandle,
                newDataOffset, CL_BUFFER_SEEK_CUR);

        /*
         * Set the no. of bytes to be read 
         */
        noOfBytesToRead = evtPrimaryHeader.eventDataSize;

        /*
         * This won't help as it's NULL and user won't get anything back.
         * Keeping as is to comply with SAF. This bug has been fixed in
         * in SAF - B12 spec.
         */
        if (NULL == pEventData)
        {
            pEventData = clHeapAllocate(noOfBytesToRead);
            if (NULL == pEventData)
            {
                clLogError("EVT", "EDG", 
                        CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);

                rc = CL_EVENT_ERR_NO_MEM;
                goto eventHdlCheckedOut;
            }
        }
        else if (noOfBytesToRead > *pEventDataSize)
        {
            /*
             * If user has allocated the memory check if it's sufficient 
             */
            *pEventDataSize = noOfBytesToRead; // NTC

            rc = CL_EVENT_ERR_NO_SPACE;
            goto eventHdlCheckedOut;
        }

        /*
         * Fetch the Event Data 
         */
        rc = clBufferNBytesRead(pEventHandle->msgHandle,
                (ClUint8T *) pEventData,
                &noOfBytesToRead);

        *pEventDataSize = noOfBytesToRead;  /* Update the Data Size */
    }

    /*
     * Reset the read offset to old postion 
     */
    rc = clBufferReadOffsetSet(pEventHandle->msgHandle, readOffset,
            CL_BUFFER_SEEK_SET);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EDG", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EDG", 
            "Event Data Get succeeded for eventHandle[%#llX]",
            eventHandle);

    CL_FUNC_EXIT();
    return CL_OK;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EDG", 
            "Event Data Get failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

/*
 * this function will free the message 
 */
ClRcT clEventFree(ClEventHandleT eventHandle)
{
    ClRcT rc = CL_OK;
    ClEvtEventHandleT *pEventInfo = NULL;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void **)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EFR", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventInfo = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EFR", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

    rc = clHandleDestroy(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "EFR", 
                CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "EFR", 
            "Event Free succeeded for eventHandle[%#llX]",
            eventHandle);

    CL_FUNC_EXIT();
    return rc;

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "EFR", 
            "Event Free failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventPublish(ClEventHandleT eventHandle, const void *pEventData,
        ClSizeT eventDataSize, ClEventIdT * pEventId)
{
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;
    ClEvtEventHandleT *pEventHandle = NULL;
    ClBufferHandleT newMsgHandle = 0, outMsgHandle = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClIocNodeAddressT localIocAddress = 0;
    ClEvtClientHeadT *pEvtClientHead = NULL;
    ClEvtClientChannelInfoT *pEvtChannelInfo = NULL;
    ClIocAddressT destAddr = {{0}};
    ClEvtEventPrimaryHeaderT evtPrimaryHeader = { 0 };
    ClUint32T noOfBytesToRead = sizeof(ClEvtEventPrimaryHeaderT);
    ClEoExecutionObjT *pEoObj = NULL;
    ClEvtHdlDbDataT * pEvtHdlDbData = NULL;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    struct timeval localtime = {0};

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    if (NULL == pEventId)
    {
        clLogError("EVT", "PUB", 
                CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "pEventId");
        rc = CL_EVENT_ERR_INVALID_PARAM;
        goto failure;
    }
    /*
     * Returning 0 for Event ID till the implementation for retention is in place.
     */
    *pEventId = 0;

    /*
     * Check EM library is intialized by given EO 
     */
    rc = clEvtInitValidate(&pEoObj, &pEvtClientHead);
    if (CL_OK != rc)
    {
        goto failure;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, eventHandle, (void **)&pEvtHdlDbData);
    if(CL_OK != rc)
    {
        clLogError("EVT", "PUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto failure;
    }

    if(pEvtHdlDbData->handleType == CL_EVENT_HANDLE)
    {
        pEventHandle = (ClEvtEventHandleT  *)pEvtHdlDbData;
    }
    else
    {
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    rc = clHandleCheckout(pEvtClientHead->evtClientHandleDatabase, pEventHandle->evtChannelHandle, 
            (void **)&pEvtChannelInfo);
    if(CL_OK != rc)
    {
        clLogError("EVT", "PUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        rc = CL_EVENT_ERR_BAD_HANDLE;
        goto eventHdlCheckedOut;
    }

    CL_EVT_PUBLISH_FLAG_VALIDATE(pEvtChannelInfo->flag);

    rc = clBufferDuplicate(pEventHandle->msgHandle, &newMsgHandle);
    rc = clBufferCreate(&outMsgHandle);

    if (NULL != pEventData)
    {
        ClUint32T writeOffset = 0;

        /*
         * Preserve the Read Offset
         */
        rc = clBufferWriteOffsetGet(newMsgHandle, &writeOffset);

        /*
         * Set the Read offset to the beginining 
         */
        rc = clBufferReadOffsetSet(newMsgHandle, 0, CL_BUFFER_SEEK_SET);

        rc = clBufferNBytesRead(newMsgHandle,
                (ClUint8T *) &evtPrimaryHeader,
                &noOfBytesToRead);

        /*
         * Set the Read offset original Offset
         */
        rc = clBufferWriteOffsetSet(newMsgHandle, 0, CL_BUFFER_SEEK_SET);

        /*
         * Update the eventDataSize "if" user has specified a payload 
         */
        evtPrimaryHeader.eventDataSize = eventDataSize;
        evtPrimaryHeader.publishTime = 0;
        /*
         * Update the publishTime of the event
         */
        rc = gettimeofday(&localtime,NULL);
        if(!rc)
        {
            evtPrimaryHeader.publishTime = (ClTimeT)localtime.tv_sec * 1000000 + localtime.tv_usec; 
        }
        rc = clBufferNBytesWrite(newMsgHandle,
                (ClUint8T *) &evtPrimaryHeader,
                noOfBytesToRead);

        rc = clBufferWriteOffsetSet(newMsgHandle, writeOffset,
                CL_BUFFER_SEEK_SET);
        /*
         * Add the eventData to the message handle 
         */
        rc = clBufferNBytesWrite(newMsgHandle, (ClUint8T *) pEventData,
                (ClUint32T) eventDataSize);
    }

    /*
     * Get local IOC address 
     */
    localIocAddress = clIocLocalAddressGet();

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;
    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_PUBLISH, newMsgHandle, outMsgHandle,
                          CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&newMsgHandle);
    if (CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "PUB", 
                        "Buffer Flatten failed, rc=[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            if (NULL == pEventData)
            {
                /*
                 * Set the Read offset to the beginining 
                 */
                rc = clBufferReadOffsetSet(newMsgHandle, 0,
                        CL_BUFFER_SEEK_SET);

                noOfBytesToRead = sizeof(ClVersionT);
                rc = clBufferNBytesRead(newMsgHandle,
                        (ClUint8T *) &evtPrimaryHeader,
                        &noOfBytesToRead);
            }

            clLogError("EVT", "PUB", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Event Publish",
                    evtPrimaryHeader.releaseCode,
                    evtPrimaryHeader.majorVersion,
                    evtPrimaryHeader.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            ClIocPortT eoIocPort = 0;

            clEoMyEoIocPortGet(&eoIocPort);
            clLogError("EVT", "PUB", 
                    CL_EVENT_LOG_MSG_5_PUBLISH_FAILED,
                    pEvtChannelInfo->evtChannelName.length,
                    pEvtChannelInfo->evtChannelName.value, eoIocPort,
                    pEvtChannelInfo->evtHandle, rc);
        }
        goto outMsgHdlAllocated;
    }

    clBufferDelete(&outMsgHandle);

    rc = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pEventHandle->evtChannelHandle);
    if(CL_OK != rc)
    {
        clLogError("EVT", "PUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto eventHdlCheckedOut;
    }

    retCode = clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);
    if(retCode!=CL_OK)
    {
        clLogError("EVT", "PUB", 
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }

// success:
    clLogTrace("EVT", "PUB", 
            "Event Publish succeeded for eventHandle[%#llX]", 
            eventHandle);

    CL_FUNC_EXIT();
    return CL_OK;

outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

// chanHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, pEventHandle->evtChannelHandle);

eventHdlCheckedOut:
    clHandleCheckin(pEvtClientHead->evtClientHandleDatabase, eventHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "PUB", 
            "Event Publish failed for eventHandle[%#llX]"
            "with rc[%#X]", eventHandle, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEventRetentionTimeClear(ClEventChannelHandleT channelHandle,
        const ClEventIdT eventId)
{
    ClRcT rc = CL_OK;
    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    clLogTrace("EVT", "RET", "clEventRetentionTimeClear() not supported yet");

failure:
    return CL_OK;
}

ClRcT clEventChannelUnlink(CL_IN ClEventInitHandleT evtHandle,
        CL_IN const ClNameT *pEvtChannelName)
{
    ClRcT rc = CL_OK;
    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    clLogTrace("EVT", "ULK", "clEventChannelUnlink() inovked");

failure:
    return CL_OK;
}


/*
 ** Cleanup the information related to this user via Finalize
 ** on behalf of that user by the server.
 */

ClRcT clEventCpmCleanup(ClCpmEventPayLoadT *pEvtCpmCleanupInfo)
{
    ClRcT rc = CL_OK;

    ClRmdOptionsT rmdOptions = { 0 };
    ClBufferHandleT inMsgHandle = 0, outMsgHandle = 0;
    ClIocNodeAddressT localIocAddress = 0;
    ClEvtUnsubscribeEventRequestT unsubscribeRequest = { 0 };
    ClIocAddressT destAddr = {{0}};
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T tries = 0;

    CL_FUNC_ENTER();

    /*
     * Check EM library is intialized by given EO 
     */
    CL_EVT_CHECK_INIT_COUNT();

    /*
     * Pack the version Info 
     */
    clEvtClientToServerVersionSet(unsubscribeRequest);

    /*
     * Pack the unsubscription information 
     */
    unsubscribeRequest.evtChannelHandle = 0;

    unsubscribeRequest.subscriptionId = 0;
    unsubscribeRequest.userId.eoIocPort = pEvtCpmCleanupInfo->eoId;

    unsubscribeRequest.userId.evtHandle = 0;
    clLogTrace("EVT", "CLP", 
            "Cleaning up {node[%.*s]:eoIocPort[%#X]} since dead",
            pEvtCpmCleanupInfo->nodeName.length, 
            pEvtCpmCleanupInfo->nodeName.value, 
            pEvtCpmCleanupInfo->eoIocPort);

    unsubscribeRequest.reqFlag = CL_EVT_FINALIZE;

    /*
     * Create message handle and pack the user input 
     */

    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferCreate(&outMsgHandle);
    rc = VDECL_VER(clXdrMarshallClEvtUnsubscribeEventRequestT, 4, 0, 0)(&unsubscribeRequest,
                                                                        inMsgHandle,
                                                                        0);

    localIocAddress = clIocLocalAddressGet();

    /*
     ** Invoke RMD call provided by EM/S for opening
     ** channel, with packed user data.
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;

    destAddr.iocPhyAddress.nodeAddress = localIocAddress;
    destAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;

    clLogInfo("EVT", "CLP", 
              CL_EVENT_LOG_MSG_2_CPM_CLEANUP, pEvtCpmCleanupInfo->eoIocPort,
              rc);

    do
    {
        rc = clRmdWithMsg(destAddr, EO_CL_EVT_UNSUBSCRIBE, inMsgHandle,
                          outMsgHandle, CL_RMD_CALL_NEED_REPLY, &rmdOptions, NULL);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
            &&
            ++tries < 5
            &&
            clOsalTaskDelay(delay) == CL_OK);

    clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    {
        if (CL_EVENT_ERR_VERSION == rc)
        {
            ClVersionT *pVersion = NULL;

            rc = clBufferFlatten(outMsgHandle, (ClUint8T **) &pVersion);
            if (CL_OK != rc)
            {
                clLogError("EVT", "CLP", 
                        "Buffer Fallten failed, rc[%#X]", rc);
                goto outMsgHdlAllocated;
            }

            clLogError("EVT", "CLP", 
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE,
                    "Client-Server Cpm Node Cleanup",
                    unsubscribeRequest.releaseCode,
                    unsubscribeRequest.majorVersion,
                    unsubscribeRequest.minorVersion, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clHeapFree(pVersion); // Free the buffer from clBufferFlatten()
        }
        else
        {
            clLogError("EVT", "CLP", 
                    CL_EVENT_LOG_MSG_2_CPM_CLEANUP_FAILED,
                    pEvtCpmCleanupInfo->eoIocPort, rc);
        }

        goto outMsgHdlAllocated;
    }

// success:
    clLogInfo("EVT", "CLP", 
              "Cleaned up {node[%.*s]:eoIocPort[%#X]} since dead",
              pEvtCpmCleanupInfo->nodeName.length, 
              pEvtCpmCleanupInfo->nodeName.value, 
              pEvtCpmCleanupInfo->eoIocPort);
    clBufferDelete(&outMsgHandle);

    CL_FUNC_EXIT();
    return CL_OK;

outMsgHdlAllocated:
    clBufferDelete(&outMsgHandle);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError("EVT", "CLP", 
            "Clean up of {node[%.*s]:eoIocPort[%#X]}"
            "failed, rc[%#X]",
            pEvtCpmCleanupInfo->nodeName.length, 
            pEvtCpmCleanupInfo->nodeName.value, 
            pEvtCpmCleanupInfo->eoIocPort, rc);
    CL_FUNC_EXIT();
    return rc;
}

ClRcT	clEventLibTableInit(void)
{
    ClRcT rc = CL_OK;

    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, EVT), CL_IOC_EVENT_PORT);
    if(CL_OK != rc)
    {
        clLogError("EVT", "INI", 
                   "Registering EM client failed  [%#X]", rc);
    }
    return rc;
}
