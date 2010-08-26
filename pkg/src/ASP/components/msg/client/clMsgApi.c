/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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
 * File        : clMsgApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description : This file contains SAF APIs of Messaging except Queue and Queue
 *               Group related APIs. These APIs make call to the Message service
 *               intern to get the work done.
 *****************************************************************************/



#include <clArchHeaders.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clTimerApi.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clDispatchApi.h>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clRmdErrors.h>
#include <clEoErrors.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <saAis.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgApi.h>
#include <clMsgApiExt.h>
#include <clMsgGroupApi.h>
#include <clVersionApi.h>
#include <clEoIpi.h>
#include <msgIdlClient.h>
#include <msgCltServer.h>
#include <msgIdlClientCallsFromClientClient.h>
#include <msgCltClient.h>

#define MSG_DISPATCH_QUEUE_HASH_BITS (8)
#define MSG_DISPATCH_QUEUE_HASH_BUCKETS (1<<MSG_DISPATCH_QUEUE_HASH_BITS)
#define MSG_DISPATCH_QUEUE_HASH_MASK (MSG_DISPATCH_QUEUE_HASH_BUCKETS-1)

/*
 * Supported version.
 */
static ClVersionT clVersionSupported[]=
{
    {'B', 0x01, 0x01},
    {'B', 0x02, 0x01},
};

/*
 * Version Database.
 */
static ClVersionDatabaseT versionDatabase=
{
    sizeof(clVersionSupported)/sizeof(ClVersionT),
    clVersionSupported
}; 


typedef struct {
    SaMsgHandleT msgHandle;
    SaInvocationT invocation;
    SaAisErrorT rc;
}ClMsgAppMessageSendCallbackParamsT;

typedef struct {
    SaMsgQueueHandleT qHandle;
}ClMsgMessageReceiveCallbackParamsT;

typedef struct ClMsgDispatchQueueCtrl
{
    struct hashStruct **dispatchTable;
    ClJobQueueT dispatchQueue;
    ClUint32T dispatchQueueSize;
    ClOsalMutexT dispatchQueueLock;
}ClMsgDispatchQueueCtrlT;

typedef struct ClMsgDispatchQueue
{
#define CL_MSG_DISPATCH_QUEUE_THREADS (0x8)
    struct hashStruct hash; /*hash index*/
    SaMsgQueueHandleT queueHandle; /*the msg queue reference*/
    SaMsgMessageReceivedCallbackT msgReceivedCallback; /*msg receive callback to invoke*/
}ClMsgDispatchQueueT;

static ClMsgDispatchQueueCtrlT gClMsgDispatchQueueCtrl;

ClHandleDatabaseHandleT gMsgHandleDatabase;
ClIdlHandleT gClMsgIdlHandle;

ClUint32T gMsgNumInits;
static ClIdlHandleObjT gIdlObj;
static ClMsgDispatchQueueT *msgDispatchQueueFind(SaMsgQueueHandleT queueHandle);
static void msgDispatchQueueDestroy(void);

static ClRcT clMsgCltIdlInitialize(void)
{
    ClRcT rc;

    gIdlObj.objId =  clObjId_ClIdlHandleObjT;

    gIdlObj.address.addressType = CL_IDL_ADDRESSTYPE_IOC;
    gIdlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    gIdlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;
    gIdlObj.flags = 0;
    gIdlObj.options.timeout = CL_RMD_TIMEOUT_FOREVER;
    gIdlObj.options.retries = 3;
    gIdlObj.options.priority = CL_IOC_DEFAULT_PRIORITY;

    rc = clIdlHandleInitialize(&gIdlObj, &gClMsgIdlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "IDL", "Failed to initialize the the IDL. error code [0x%x].", rc);
    }

    return rc;
}


static void clMsgCltIdlFinalize(void)
{
    ClRcT rc;

    rc = clIdlHandleFinalize(gClMsgIdlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "IDL", "Failed to do the IDL cleanup. error code [0x%x].", rc);
    }

    gClMsgIdlHandle = 0;
}

static ClRcT msgDispatchThread(ClPtrT dispatchArg)
{
    ClMsgDispatchQueueT *pDispatchQueue = dispatchArg;
    SaMsgQueueHandleT queueHandle;
    SaMsgMessageReceivedCallbackT callback = NULL;
    CL_ASSERT(dispatchArg);
    queueHandle = pDispatchQueue->queueHandle;
    callback = pDispatchQueue->msgReceivedCallback;
    CL_ASSERT(callback != NULL);
    clHeapFree(dispatchArg);
    callback(queueHandle);
    return CL_OK;
}

static void clMsgDispatchCallback(SaMsgHandleT msgHandle, ClUint32T callbackType, void *pCallbackParam)
{
    ClRcT rc;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    
    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "DCb", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    switch(callbackType)
    {
        case CL_MSG_QUEUE_OPEN_CALLBACK_TYPE:
            {
                ClMsgAppQueueOpenCallbackParamsT *pParam = (ClMsgAppQueueOpenCallbackParamsT*)pCallbackParam;
                pMsgLibInfo->callbacks.saMsgQueueOpenCallback(pParam->invocation, pParam->queueHandle, pParam->rc);
            }
            break;
        case CL_MSG_QUEUE_GROUP_TRACK_CALLBACK_TYPE:
            {
                ClMsgAppQGroupTrackCallbakParamsT *pParam = (ClMsgAppQGroupTrackCallbakParamsT*)pCallbackParam;
                pMsgLibInfo->callbacks.saMsgQueueGroupTrackCallback((SaNameT*)&pParam->groupName,
                        pParam->pNotificationBuffer,
                        pParam->pNotificationBuffer->numberOfItems,
                        pParam->rc);
                /*
                 * We free the notification as well as the buffer. Because with synchronous track callbacks
                 * the app writer himself would free the notification with saMsgQueueGroupNotificationFree.
                 * But in async callbacks where the message service allocates both the buffer and the notification,
                 * the onus is on the msg service to free both. as SAF doesnt say anything about it.
                 */
                if(pParam->pNotificationBuffer)
                {
                    if(pParam->pNotificationBuffer->notification)
                        clHeapFree(pParam->pNotificationBuffer->notification);
                    clHeapFree(pParam->pNotificationBuffer);
                }
            }
            break;
        case CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE:
            {
                ClMsgAppMessageSendCallbackParamsT *pParam = (ClMsgAppMessageSendCallbackParamsT*)pCallbackParam;
                pMsgLibInfo->callbacks.saMsgMessageDeliveredCallback(pParam->invocation, pParam->rc);
            }
            break;
        case CL_MSG_MESSAGE_RECEIVED_CALLBACK_TYPE:
            {
                ClMsgMessageReceiveCallbackParamsT *pParam = (ClMsgMessageReceiveCallbackParamsT*)pCallbackParam;
                ClMsgDispatchQueueT *pDispatchQueue = NULL;
                ClMsgDispatchQueueT *pTempDispatchQueue = NULL;
                if(!gClMsgDispatchQueueCtrl.dispatchQueueSize || !gClMsgDispatchQueueCtrl.dispatchTable)
                    goto recvCallback;
                clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
                if(!(pDispatchQueue = msgDispatchQueueFind(pParam->qHandle)))
                {
                    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
                    recvCallback:
                    pMsgLibInfo->callbacks.saMsgMessageReceivedCallback(pParam->qHandle);
                }
                else
                {
                    /*
                     * Make a copy just to prevent in-flight msg finalize ripping us off.
                     */
                    pTempDispatchQueue = clHeapCalloc(1, sizeof(*pTempDispatchQueue));
                    CL_ASSERT(pTempDispatchQueue);
                    memcpy(pTempDispatchQueue, pDispatchQueue, sizeof(*pTempDispatchQueue));
                    clJobQueuePush(&gClMsgDispatchQueueCtrl.dispatchQueue, 
                                   msgDispatchThread, 
                                   (ClPtrT)pTempDispatchQueue);
                    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
                }
            }
            break;
    }

    if(pCallbackParam)
        clHeapFree(pCallbackParam);

    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "DCb", "Failed to check in the message handle the Handle Database. error code [0x%x].", rc);
    }

error_out:
    return; 
}


static void clMsgDispatchDestroyCallback(ClUint32T callbackType, ClPtrT pCallbackParam)
{
    switch(callbackType)
    {
        case CL_MSG_QUEUE_OPEN_CALLBACK_TYPE:
            break;
        case CL_MSG_QUEUE_GROUP_TRACK_CALLBACK_TYPE:
            {
                ClMsgAppQGroupTrackCallbakParamsT *pParam = pCallbackParam;
                if(pParam->pNotificationBuffer)
                {
                    if(pParam->pNotificationBuffer->notification)
                        clHeapFree(pParam->pNotificationBuffer->notification);
                    clHeapFree(pParam->pNotificationBuffer);
                }
            }
            break;
        case CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE:
            break;
        case CL_MSG_MESSAGE_RECEIVED_CALLBACK_TYPE:
            break;
    }

    clHeapFree(pCallbackParam);

    return;
}

static ClRcT clMsgInitialize(void)
{
    ClRcT rc = CL_OK;
    ClRcT retCode;
    ClEoExecutionObjT *pThis = NULL;

    if(gMsgNumInits != 0)
        goto out;

    rc = clASPInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initalize the ASP. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleDatabaseCreate(NULL, &gMsgHandleDatabase);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to create a handle database for message-client-init-handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clMsgCltIdlInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Initializing IDL failed. error code [0x%x].", rc);
        goto error_out_2;
    }

    rc = clMsgIdlClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register the Server Table. error code [0x%x].", rc);
        goto error_out_3;
    }

    rc = clMsgCltClientInstall();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install the Client Table. error code [0x%x].", rc);
        goto error_out_4;
    }

    rc = clEoMyEoObjectGet(&pThis);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to get my eo object handle. error code [0x%x].", rc);
        goto error_out_4;
    }

    rc = clMsgCltClientTableRegister(pThis->eoPort);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install the another Client Table. error code [0x%x].", rc);
        goto error_out_4;
    }

    goto out;

error_out_4:
    retCode = clMsgIdlClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Server Table. error code [0x%x].",retCode);
error_out_3:
    clMsgCltIdlFinalize();
error_out_2:
    retCode = clHandleDatabaseDestroy(gMsgHandleDatabase);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
error_out_1:
    retCode = clASPFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to Finalize the ASP. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}

static ClRcT clMsgFinalize(void)
{
    ClRcT rc = CL_OK;

    if(gMsgNumInits != 0)
        goto out;

    /*
     * Destroy the mesg. dispatch queue.
     */
    msgDispatchQueueDestroy();

    rc = clMsgCltClientUninstall();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the just opened handle database. error code [0x%x].", rc);

    rc = clMsgIdlClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Server Table. error code [0x%x].",rc);

    clMsgCltIdlFinalize();

    rc = clHandleDatabaseDestroy(gMsgHandleDatabase);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the just opened handle database. error code [0x%x].", rc);

    rc = clASPFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to Finalize the ASP. error code [0x%x].", rc);

out: 
    return rc;
}


SaAisErrorT saMsgInitialize(
                            SaMsgHandleT *pMsgHandle,
                            const SaMsgCallbacksT *pMsgCallbacks,
                            SaVersionT *pVersion
                            )
{
    ClRcT rc = CL_OK;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    SaMsgHandleT msgHandle;
    SaMsgHandleT tempMsgHandle = 0;
    ClTimerTimeOutT delay = CL_MSG_DEFAULT_DELAY;
    ClInt32T retries = 0;

    if(pVersion == NULL || pMsgHandle == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "INI", "NULL parameter passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clVersionVerify(&versionDatabase, (ClVersionT*)pVersion);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed with version mismatch. error code [0x%x]",rc);
        goto error_out;
    }
    else
    {
        /* FIXME : BUG : The following line must be added in clVersionVerify() of clVersionApi.c file*/
        pVersion->majorVersion = versionDatabase.versionsSupported[0].majorVersion;
    }

    rc = clMsgInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to do the basic initializations of message client. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clHandleCreate(gMsgHandleDatabase, sizeof(ClMsgLibInfoT), &msgHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to create a handle in Handle Database. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out_2;
    }

    if( pMsgCallbacks != NULL)
    {
        memcpy(&pMsgLibInfo->callbacks, pMsgCallbacks, sizeof(pMsgLibInfo->callbacks));
        if(pMsgCallbacks->saMsgMessageReceivedCallback != NULL)
            tempMsgHandle = msgHandle;
    }

    do {
        rc = VDECL_VER(clMsgClientInitClientSync, 4, 0, 0)(gClMsgIdlHandle, (ClUint32T*)pVersion, tempMsgHandle, &pMsgLibInfo->handle);
    }while((rc == CL_MSG_RC(CL_ERR_NOT_INITIALIZED) || 
            rc == CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED) || 
            rc == CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE) || 
            rc == CL_RMD_RC(CL_IOC_ERR_COMP_UNREACHABLE)) 
           && ++retries < CL_MSG_DEFAULT_RETRIES
           && clOsalTaskDelay(delay) == CL_OK);
    if(rc != CL_OK)
    {
        if (rc == CL_MSG_RC(CL_ERR_NOT_INITIALIZED) || 
            rc == CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED) || 
            rc == CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE) || 
            rc == CL_RMD_RC(CL_IOC_ERR_COMP_UNREACHABLE))
            rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("MSG", "INI", "Failed to register the client with server. error code [0x%x].", rc);
        goto error_out_3;
    }

    if(!gClMsgDispatchQueueCtrl.dispatchTable)
    {
        rc = clJobQueueInit(&gClMsgDispatchQueueCtrl.dispatchQueue, 0, CL_MSG_DISPATCH_QUEUE_THREADS);
        if(rc != CL_OK)
        {
            clLogError("MSG", "INI", "Failed to initialize dispatch job queue [%#x]", rc);
            goto error_out_3;
        }

        rc = clOsalMutexInit(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
        CL_ASSERT(rc == CL_OK);

        gClMsgDispatchQueueCtrl.dispatchTable = clHeapCalloc(MSG_DISPATCH_QUEUE_HASH_BUCKETS, 
                                                             sizeof(*gClMsgDispatchQueueCtrl.dispatchTable));
        CL_ASSERT(gClMsgDispatchQueueCtrl.dispatchTable != NULL);

        gClMsgDispatchQueueCtrl.dispatchQueueSize = 0;
    }

    rc = clDispatchRegister(&pMsgLibInfo->dispatchHandle, msgHandle, &clMsgDispatchCallback, &clMsgDispatchDestroyCallback );
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register with dispatch library. error code [0x%x].", rc);
        goto error_out_4;
    }

    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to check in the message handle the Handle Database. error code [0x%x].", rc);
        goto error_out_5;
    }

    *pMsgHandle = msgHandle;

    gMsgNumInits++;
    goto out;

    error_out_5:
    retCode = clDispatchDeregister(pMsgLibInfo->dispatchHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister the dispatch handle. error code [0x%x].", retCode);
    error_out_4:
    retCode = VDECL_VER(clMsgClientFinClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister the message client with message server. error code [0x%x].", retCode);
    error_out_3:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to checkin the message handle. error code [0x%x].", retCode);
    error_out_2:
    retCode = clHandleDestroy(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the message handle. error code [0x%x].", retCode);
    error_out_1:
    retCode = clMsgFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to do the cleanup of message client. error code [0x%x].", retCode);
    error_out:
    out:
    return CL_MSG_SA_RC(rc);
}



SaAisErrorT saMsgFinalize(
        SaMsgHandleT msgHandle
        )
{
    ClRcT rc;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "FIN", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    gMsgNumInits--;

    rc = clDispatchDeregister(pMsgLibInfo->dispatchHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister with dispatch library. error code [0x%x].", rc);

    rc = VDECL_VER(clMsgClientFinClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle); 
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister the client with server. error code [0x%x].", rc);

    memset(&pMsgLibInfo->callbacks, 0, sizeof(pMsgLibInfo->callbacks));

    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to checkin the message handle. error code [0x%x].", rc);

    rc = clHandleDestroy(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the message handle. error code [0x%x].", rc);

    rc = clMsgFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to dot the cleanup of the message client. error code [0x%x].", rc);

error_out:
    return CL_MSG_SA_RC(rc);
}


static void clMsgAppMessageDeliveredCallbackFunc(ClIdlHandleT idlHandle,
        SaMsgHandleT msgHandle,
        ClNameT *pDest,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        SaTimeT timeout,
        ClRcT rc,
        ClPtrT pCallbackParams)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppMessageSendCallbackParamsT *pParam = (ClMsgAppMessageSendCallbackParamsT*)pCallbackParams;

    retCode = clHandleCheckout(gMsgHandleDatabase, pParam->msgHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out;
    }

    ((ClMsgAppMessageSendCallbackParamsT*)pCallbackParams)->rc = CL_MSG_SA_RC(rc);
    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE, pCallbackParams);
    if(retCode != CL_OK)
        clLogError("MSG", "MDCb", "Failed to enqueue the callback into dispatcher. error code [0x%x].", retCode);

    retCode = clHandleCheckin(gMsgHandleDatabase, pParam->msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "MDCb", "Failed to checkin the message handle. error code [0x%x].",retCode);

error_out:
    return;
}


static SaAisErrorT clMsgMessageSendInternal(
        ClBoolT isSync,
        SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pDestination,
        const SaMsgMessageT *pMessage,
        SaTimeT timeout,
        SaMsgAckFlagsT ackFlags)
{ 
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    SaMsgMessageT tempMessage;
    SaNameT senderName = {0,{0}};
    SaTimeT sendTime = 0;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pDestination == NULL || pMessage == NULL || pMessage->data == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "SND", "NULL paramter passed. error code [0x%x].", rc);
        goto error_out;
    }

    if(pMessage->priority > SA_MSG_MESSAGE_LOWEST_PRIORITY)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "SND", "Invalid priority for the message is passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SND", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* FIXME : BUG : If NULL is passed to IDL the program crashes. Following lines are work-around. Can be removed once the IDL fix is in.*/ 
    memcpy(&tempMessage, pMessage, sizeof(*pMessage));
    if(tempMessage.senderName == NULL)
        tempMessage.senderName = &senderName;
    /***** Work around ends here. *****/

    if(isSync == CL_TRUE)
    {
        rc = VDECL_VER(clMsgClientMessageSendClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pDestination, &tempMessage, sendTime, timeout);
    }
    else
    {
        if(ackFlags == SA_MSG_MESSAGE_DELIVERED_ACK )
        {
            ClMsgAppMessageSendCallbackParamsT *pCallbackParam; 

            if(pMsgLibInfo->callbacks.saMsgMessageDeliveredCallback == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED); 
                clLogError("MSG", "SND", "Callback function not registered for acknowledgement. error code [0x%x].", rc);
                goto error_out_1;
            }

            pCallbackParam = (ClMsgAppMessageSendCallbackParamsT*) clHeapAllocate(sizeof(ClMsgAppMessageSendCallbackParamsT));
            if(pCallbackParam == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
                clLogError("MSG", "SND", "Failed to allocate memory for %zd bytes. error code [0x%x].", sizeof(ClMsgAppMessageSendCallbackParamsT), rc);
                goto error_out_1;
            }

            pCallbackParam->msgHandle = msgHandle;
            pCallbackParam->invocation = invocation;
            rc = VDECL_VER(clMsgClientMessageSendClientAsync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pDestination, &tempMessage, sendTime, 0, &clMsgAppMessageDeliveredCallbackFunc, pCallbackParam);
        }
        else
        {
            rc = VDECL_VER(clMsgClientMessageSendClientAsync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pDestination, &tempMessage, sendTime, 0, NULL, NULL);
        }
    }

    if(rc != CL_OK)
    {
        clLogError("MSG", "SND", "Failed to send the message. error code [0x%x].", rc);
    }

error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "SND", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgMessageSend(
        SaMsgHandleT msgHandle,
        const SaNameT *pDestination,
        const SaMsgMessageT *pMessage,
        SaTimeT timeout)
{
    return clMsgMessageSendInternal(CL_TRUE, msgHandle, 0, pDestination, pMessage, timeout, 0);  
}


SaAisErrorT saMsgMessageSendAsync(SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pDestination,
        const SaMsgMessageT *pMessage,
        SaMsgAckFlagsT ackFlags)
{ 
    return clMsgMessageSendInternal(CL_FALSE, msgHandle, invocation, pDestination, pMessage, 0, ackFlags);  
}


static ClRcT clMsgReceiveMessageForm(SaMsgMessageT **pTempMessage, SaMsgMessageT *pMessage)
{
    ClRcT rc = CL_OK;
    ClUint8T *pTempData;
    SaNameT *pTempSenderName;
    SaSizeT size = 0;

    *pTempMessage = (SaMsgMessageT *)clHeapAllocate(sizeof(SaMsgMessageT));
    if(*pTempMessage == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "GET", "Failed to allcoate memory of %zd bytes. error code [0x%x].", sizeof(SaMsgMessageT), rc);
        goto error_out;
    }

    pTempSenderName = (SaNameT*)clHeapAllocate(sizeof(SaNameT));
    if(pTempSenderName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "GET", "Failed to allcoate memory of %zd bytes. error code [0x%x].", sizeof(SaNameT), rc);
        goto error_out_1;
    }
    
    /*IDL will allocate memory depending on the size. This one byte is IDL's pray, as it frees the this memory before allocating new.*/
    if(!(size = pMessage->size))
        size = 1;

    pTempData = (ClUint8T*)clHeapAllocate(size);
    if(pTempData == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "GET", "Failed to allcoate memory of 1 byte. error code [0x%x].", rc);
        goto error_out_2;
    }

    memcpy(*pTempMessage, pMessage, sizeof(**pTempMessage));
    (*pTempMessage)->senderName = pTempSenderName;
    (*pTempMessage)->data = pTempData;

    goto out;

error_out_2:
    clHeapFree(pTempSenderName);
error_out_1:
    clHeapFree(*pTempMessage);
error_out:
out:
    return rc;
}
        

SaAisErrorT saMsgMessageGet(SaMsgQueueHandleT queueHandle,
        SaMsgMessageT *pMessage,
        SaTimeT *pSendTime,
        SaMsgSenderIdT *pSenderId,
        SaTimeT timeout)
{
    ClRcT rc;
    SaTimeT sendTime;
    SaMsgMessageT *pTempMessage;
    ClUint8T *pTemp;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pMessage == NULL || pSenderId == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "GET", "NULL pointer passed for message/sender-id. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clMsgReceiveMessageForm(&pTempMessage, pMessage);
    if(rc != CL_OK)
    {
        clLogError("MSG", "GET", "Failed to create a receive message. error code [0x%x].",rc);
        goto error_out;
    }

    if(pMessage->data == NULL)
        pTempMessage->size = 0;

    rc = VDECL_VER(clMsgClientMessageGetClientSync, 4, 0, 0)(gClMsgIdlHandle, queueHandle, pTempMessage, &sendTime, pSenderId, timeout);
    if(rc != CL_OK)
    {
        /* Note : The above call would have freed pTempMessage->senderName and pTempMessage->data. 
         * so just pTempMessage needs to be freed.*/
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {
            clLogInfo("MSG", "GET", "The message get request has timed-out. timeout [%lld ns] . error code [0x%x].", timeout, rc);
        }
        else
        {
            clLogCritical("MSG", "GET", "Failed to get the message from queue. error code [0x%x].", rc);
            clLogCritical("MSG", "GET", "A received packet might have got DROPPED.");
        }
        /* I am not sure if the packet has got dropped, because I dont know where it has returned from. */
        goto error_out_1;
    }
    
    if(pSendTime != NULL)
        *pSendTime = sendTime;
    
    if(pMessage->data == NULL)
    {
        pTemp = (ClUint8T*)clHeapAllocate(pTempMessage->size);
        if(pTemp == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("MSG", "GET", "Failed to allocate %llu bytes of memory. error code [0x%x].", pTempMessage->size, rc);
            clLogCritical("MSG", "GET", "Dropping the received packet.");
            goto error_out_2;
        }
        pMessage->data = pTemp;
    }

    pMessage->type = pTempMessage->type;
    pMessage->version = pTempMessage->version;
    pMessage->size = pTempMessage->size;
    pMessage->priority = pTempMessage->priority;
    if(pMessage->senderName != NULL)
        memcpy(pMessage->senderName, pTempMessage->senderName, sizeof(SaNameT));
    memcpy(pMessage->data, pTempMessage->data, pTempMessage->size);


error_out_2:
    clHeapFree(pTempMessage->senderName);
    clHeapFree(pTempMessage->data);
error_out_1:
    clHeapFree(pTempMessage);
error_out:
    return CL_MSG_SA_RC(rc);
}


ClRcT VDECL_VER(clMsgClientsMessageReceiveCallback, 4, 0, 0)(SaMsgHandleT clientHandle, SaMsgQueueHandleT qHandle)
{
    ClRcT rc;
    ClMsgMessageReceiveCallbackParamsT *pParam;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    rc = clHandleCheckout(gMsgHandleDatabase, clientHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "RCVcb", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    pParam = (ClMsgMessageReceiveCallbackParamsT *)clHeapAllocate(sizeof(ClMsgMessageReceiveCallbackParamsT));
    if(pParam == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "RCVcb", "Failed to allocate memory of size %zd. error code [0x%x].", sizeof(ClMsgMessageReceiveCallbackParamsT), rc);
        goto error_out_1;
    }
    pParam->qHandle = qHandle;
    rc = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_MESSAGE_RECEIVED_CALLBACK_TYPE, pParam); 
    if(rc != CL_OK)
    {
        clLogError("MSG", "RCVcb", "Failed to enqueue a callback to Dispatch queue. error code [0x%x].", rc);
    }

error_out_1:
    rc = clHandleCheckin(gMsgHandleDatabase, clientHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "RCVcb", "Failed to checkin the message handle. error code [0x%x].", rc);
    }

error_out:
    return rc;
}


SaAisErrorT saMsgMessageDataFree(SaMsgHandleT msgHandle, ClPtrT pData)
{
    ClRcT rc;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pData == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "DATA-FREE", "Null pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "DATA-FREE", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "DATA-FREE", "Failed to checkin the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    clHeapFree(pData);

error_out:
    return CL_MSG_SA_RC(rc);
}



SaAisErrorT saMsgMessageSendReceive(
        SaMsgHandleT msgHandle,
        const SaNameT *pDestAddress,
        const SaMsgMessageT *pSendMsg,
        SaMsgMessageT *pReceiveMsg,
        SaTimeT *pReplySendTime,
        SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    SaNameT dummyName = {0,{0}};
    SaMsgMessageT tempSendMsg;
    SaMsgMessageT *pTempMessage;
    SaTimeT sendTime = 0;
    SaTimeT replySentTime; 
    ClUint8T *pTemp;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pDestAddress == NULL || pSendMsg  == NULL || pReceiveMsg  == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "SendRecv", "NULL pointer passed for senderName. error code [0x%x].",rc);
        goto error_out;
    }

    /* FIXME : BUG : If NULL is passed to IDL the program crashes. Following lines are work-around. Can be removed once the IDL fix is in.*/ 
    memcpy(&tempSendMsg, pSendMsg, sizeof(*pSendMsg));
    if(tempSendMsg.senderName == NULL)
        tempSendMsg.senderName = &dummyName;
    /***** Work around ends here. *****/

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clMsgReceiveMessageForm(&pTempMessage, pReceiveMsg);
    if(rc != CL_OK)
    {
        clLogError("MSG", "GET", "Failed to create a receive message. error code [0x%x].",rc);
        goto error_out_1;
    }

    if(pReceiveMsg->data == NULL)
        pTempMessage->size = 0;

    rc = VDECL_VER(clMsgClientMessageSendReceiveClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pDestAddress,
            &tempSendMsg, sendTime, pTempMessage, &replySentTime, timeout);
    if(rc != CL_OK)
    {
        /* Note : The above call would have freed pTempMessage->senderName and pTempMessage->data. 
         * so just pTempMessage needs to be freed.*/
        clLogCritical("MSG", "GET", "Failed at send and receive to [%.*s]. error code [0x%x].",
                pDestAddress->length, pDestAddress->value, rc);
        /* Note : Packet might have have got dropped on the server side.*/
        goto error_out_2;
    }

    if(pReplySendTime != NULL)
        *pReplySendTime = replySentTime;

    if(pReceiveMsg->data == NULL)
    {
        pTemp = (ClUint8T*)clHeapAllocate(pTempMessage->size);
        if(pTemp == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("MSG", "GET", "Failed to allocate [%llu] bytes of memory. error code [0x%x].", pTempMessage->size, rc);
            clLogCritical("MSG", "GET", "Dropping the received packet.");
            goto error_out_3;
        }
        pReceiveMsg->data = pTemp;
    }

    pReceiveMsg->type = pTempMessage->type;
    pReceiveMsg->version = pTempMessage->version;
    pReceiveMsg->size = pTempMessage->size;
    pReceiveMsg->priority = pTempMessage->priority;
    if(pReceiveMsg->senderName != NULL)
        memcpy(pReceiveMsg->senderName, pTempMessage->senderName, sizeof(SaNameT));
    memcpy(pReceiveMsg->data, pTempMessage->data, pTempMessage->size);


error_out_3:
    clHeapFree(pTempMessage->senderName);
    clHeapFree(pTempMessage->data);
error_out_2:
    clHeapFree(pTempMessage);
error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to checkin the message handle. error code [0x%x].",retCode);
error_out:
    return CL_MSG_SA_RC(rc);
}


static void clMsgAppMessageReplyDeliveredCallbackFunc(ClIdlHandleT idlHandle,
        SaMsgHandleT msgHandle,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        SaMsgSenderIdT senderId,
        SaTimeT timeout,
        ClRcT rc,
        ClPtrT pCallbackParams)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppMessageSendCallbackParamsT *pParam = (ClMsgAppMessageSendCallbackParamsT*)pCallbackParams;

    retCode = clHandleCheckout(gMsgHandleDatabase, pParam->msgHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out;
    }

    ((ClMsgAppMessageSendCallbackParamsT*)pCallbackParams)->rc = CL_MSG_SA_RC(rc);
    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE, pCallbackParams);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to enqueue the callback into dispatcher. error code [0x%x].", retCode);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, pParam->msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    return;
}



static SaAisErrorT clMsgMessageReplyInternal (
        ClBoolT isSync,
        SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaMsgMessageT *pReplyMessage,
        const SaMsgSenderIdT *pSenderId,
        SaTimeT timeout,
        SaMsgAckFlagsT ackFlags)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    SaTimeT sendTime;
    ClMsgAppMessageSendCallbackParamsT *pCallbackParam;

    CL_MSG_CLIENT_INIT_CHECK;

    CL_MSG_SEND_TIME_GET(sendTime);

    if(pReplyMessage == NULL || pSenderId == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "REPLY", "NULL passed for message/sender-id. error code [0x%x].", rc);
        goto error_out;
    }

    if(pReplyMessage->priority > SA_MSG_MESSAGE_LOWEST_PRIORITY)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "REPLY", "Invalide message priority passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    if(isSync == CL_TRUE)
    {
        rc = VDECL_VER(clMsgClientMessageReplyClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (SaMsgMessageT*)pReplyMessage, sendTime, *pSenderId, timeout);
    }
    else
    {
        if(ackFlags == SA_MSG_MESSAGE_DELIVERED_ACK )
        {
            if(pMsgLibInfo->callbacks.saMsgMessageDeliveredCallback == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED); 
                clLogError("MSG", "REPLY", "Callback function not registered for acknowledgement. error code [0x%x].", rc);
                goto error_out_1;
            }

            pCallbackParam = (ClMsgAppMessageSendCallbackParamsT*) clHeapAllocate(sizeof(ClMsgAppMessageSendCallbackParamsT));
            if(pCallbackParam == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
                clLogError("MSG", "REPLY", "Failed to allocate memory for %zd bytes. error code [0x%x].", sizeof(ClMsgAppMessageSendCallbackParamsT), rc);
                goto error_out_1;
            }

            pCallbackParam->msgHandle = msgHandle;
            pCallbackParam->invocation = invocation;
            rc = VDECL_VER(clMsgClientMessageReplyClientAsync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (SaMsgMessageT*)pReplyMessage, sendTime, *pSenderId, 0, &clMsgAppMessageReplyDeliveredCallbackFunc , pCallbackParam);
        }
        else
        {
            rc = VDECL_VER(clMsgClientMessageReplyClientAsync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (SaMsgMessageT*)pReplyMessage, sendTime, *pSenderId, 0, NULL, NULL);
        }
    }
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to reply. error code [0x%x].",rc);
    }

error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to checkin the message handle. error code [0x%x].", retCode);

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgMessageReply (
        SaMsgHandleT msgHandle,
        const SaMsgMessageT *pReplyMessage,
        const SaMsgSenderIdT *pSenderId,
        SaTimeT timeout)
{
    return clMsgMessageReplyInternal(CL_TRUE, msgHandle, 0, pReplyMessage, pSenderId, timeout, 0); 
}


SaAisErrorT saMsgMessageReplyAsync (
        SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaMsgMessageT *pReplyMessage,
        const SaMsgSenderIdT *pSenderId,
        SaMsgAckFlagsT ackFlags)
{
    return clMsgMessageReplyInternal(CL_FALSE, msgHandle, invocation, pReplyMessage, pSenderId, 0, ackFlags); 
}


SaAisErrorT saMsgSelectionObjectGet (
        SaMsgHandleT msgHandle,
        SaSelectionObjectT *pSelectionObject)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pSelectionObject == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "SOG", "NULL pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SOG", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clDispatchSelectionObjectGet(pMsgLibInfo->dispatchHandle, pSelectionObject);
    
error_out:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "SOG", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgDispatch(SaMsgHandleT msgHandle, SaDispatchFlagsT dispatchFlags)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "MD", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clDispatchCbDispatch(pMsgLibInfo->dispatchHandle, dispatchFlags);

error_out:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MD", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

    return CL_MSG_SA_RC(rc);
}

static ClMsgDispatchQueueT *msgDispatchQueueFind(SaMsgQueueHandleT queueHandle)
{
    struct hashStruct *iter ;
    ClUint32T key = (ClUint32T)(ClUint64T)queueHandle & MSG_DISPATCH_QUEUE_HASH_MASK;
    if(!gClMsgDispatchQueueCtrl.dispatchTable) return NULL;
    for(iter = gClMsgDispatchQueueCtrl.dispatchTable[key]; iter; iter = iter->pNext)
    {
        ClMsgDispatchQueueT *pDispatchQueue = hashEntry(iter, ClMsgDispatchQueueT, hash);
        if(pDispatchQueue->queueHandle == queueHandle)
            return pDispatchQueue;
    }
    return NULL;
}

static ClRcT msgDispatchQueueDel(SaMsgQueueHandleT queueHandle)
{
    ClMsgDispatchQueueT *pDispatchQueue = NULL;
    ClRcT rc = CL_OK;
    pDispatchQueue = msgDispatchQueueFind(queueHandle);
    if(pDispatchQueue)
    {
        hashDel(&pDispatchQueue->hash);
        clHeapFree(pDispatchQueue);
        --gClMsgDispatchQueueCtrl.dispatchQueueSize;
    }
    else rc = CL_MSG_RC(CL_ERR_NOT_EXIST);
    return rc;
}

static void msgDispatchQueueDestroy(void)
{
    register ClInt32T i;
    if(!gClMsgDispatchQueueCtrl.dispatchTable) return;
    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    if(gClMsgDispatchQueueCtrl.dispatchQueueSize > 0)
    {
        for(i = 0; i < MSG_DISPATCH_QUEUE_HASH_BUCKETS; ++i)
        {
            struct hashStruct *next = NULL;
            struct hashStruct *iter = NULL;
            if(!(iter = gClMsgDispatchQueueCtrl.dispatchTable[i])) continue;
            for(; iter; iter = next)
            {
                ClMsgDispatchQueueT *pDispatchQueue;
                next = iter->pNext;
                pDispatchQueue = hashEntry(iter, ClMsgDispatchQueueT, hash);
                clHeapFree(pDispatchQueue);
            }
            gClMsgDispatchQueueCtrl.dispatchTable[i] = NULL;
        }
    }
    clHeapFree(gClMsgDispatchQueueCtrl.dispatchTable);
    gClMsgDispatchQueueCtrl.dispatchTable = NULL;
    gClMsgDispatchQueueCtrl.dispatchQueueSize = 0;
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    clJobQueueDelete(&gClMsgDispatchQueueCtrl.dispatchQueue);
    clOsalMutexDestroy(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    return;
}
                                     
static void msgDispatchQueueAdd(SaMsgQueueHandleT queueHandle,
                                SaMsgMessageReceivedCallbackT callback)
{
    ClMsgDispatchQueueT *pDispatchQueue = NULL;
    ClUint32T key = (ClUint32T)(ClUint64T)queueHandle & MSG_DISPATCH_QUEUE_HASH_MASK;
    if(!(pDispatchQueue = msgDispatchQueueFind(queueHandle)))
    {
        pDispatchQueue = clHeapCalloc(1, sizeof(*pDispatchQueue));
        CL_ASSERT(pDispatchQueue != NULL);
        pDispatchQueue->queueHandle = queueHandle;
        pDispatchQueue->msgReceivedCallback = callback;
        hashAdd(gClMsgDispatchQueueCtrl.dispatchTable, key, &pDispatchQueue->hash);
        ++gClMsgDispatchQueueCtrl.dispatchQueueSize;
    }
    else
        pDispatchQueue->msgReceivedCallback = callback;
}

ClRcT clMsgDispatchQueueRegister(SaMsgQueueHandleT queueHandle,
                                 SaMsgMessageReceivedCallbackT callback)
{
    CL_MSG_CLIENT_INIT_CHECK;

    if(!gClMsgDispatchQueueCtrl.dispatchTable) 
        return CL_MSG_RC(CL_ERR_NOT_INITIALIZED);

    if(!callback)
        return CL_MSG_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    msgDispatchQueueAdd(queueHandle, callback);
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    return CL_OK;
}

ClRcT clMsgDispatchQueueDeregister(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc = CL_OK;

    CL_MSG_CLIENT_INIT_CHECK;

    if(!gClMsgDispatchQueueCtrl.dispatchTable)
        return CL_MSG_RC(CL_ERR_NOT_INITIALIZED);

    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    rc = msgDispatchQueueDel(queueHandle);
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    return rc;
}
