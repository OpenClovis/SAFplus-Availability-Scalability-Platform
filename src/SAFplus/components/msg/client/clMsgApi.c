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
 *//*
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
#include <clMsgCkptClient.h>
#include <clMsgIdl.h>
#include <clMsgReceiver.h>
#include <clMsgSender.h>
#include <clMsgDebugInternal.h>
#include <clMsgGroupRR.h>

#include <msgIdlClient.h>
#include <msgCltServer.h>
#include <msgCltClient.h>
#include <msgCltSrvServer.h>
#include <msgCltSrvClient.h>
#include <msgIdlClientCallsFromClientClient.h>
#include <clMsgConsistentHash.h>


#define MSG_RECEIVE_MAX_RETRY 10
#define MSG_DISPATCH_QUEUE_HASH_BITS (8)
#define MSG_DISPATCH_QUEUE_HASH_BUCKETS (1<<MSG_DISPATCH_QUEUE_HASH_BITS)
#define MSG_DISPATCH_QUEUE_HASH_MASK (MSG_DISPATCH_QUEUE_HASH_BUCKETS-1)
#define MSG_CLI_LOCK() do { pthread_mutex_lock(&gClMsgCliLock); } while(0)
#define MSG_CLI_UNLOCK() do { pthread_mutex_unlock(&gClMsgCliLock); } while(0)

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

static pthread_mutex_t mutexMsgId = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gClMsgCliLock = PTHREAD_MUTEX_INITIALIZER;
static ClInt32T gClMsgCliRefCnt;

typedef struct {
    SaMsgHandleT msgHandle;
    SaInvocationT invocation;
    SaAisErrorT rc;
}ClMsgAppMessageSendCallbackParamsT;

typedef struct {
    SaMsgQueueHandleT qHandle;
    ClBoolT asyncReceive;
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
    SaMsgHandleT msgHandle; /* the msg handle reference for the queue */
    SaMsgQueueHandleT queueHandle; /*the msg queue reference*/
    SaMsgMessageReceivedCallbackT msgReceivedCallback; /*msg receive callback to invoke*/
}ClMsgDispatchQueueT;

static ClMsgDispatchQueueCtrlT gClMsgDispatchQueueCtrl;
static ClUint32T currentMessageId = 0;

ClHandleDatabaseHandleT gMsgHandleDatabase;
static SaMsgHandleT gMsgHandle;

static ClMsgDispatchQueueT *msgDispatchQueueFind(SaMsgQueueHandleT queueHandle);
static void msgDispatchQueueDestroy(void);

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
        clDbgCodeError(rc,("Failed to checkout the queue handle. error code [0x%x].",rc));
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
                if(!pParam->asyncReceive || 
                   !gClMsgDispatchQueueCtrl.dispatchQueueSize || 
                   !gClMsgDispatchQueueCtrl.dispatchTable)
                {
                    goto recvCallback;
                }
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

/*
 * Should be called with gClMsgCliLock held
 */
static ClRcT clMsgInitialize(void)
{
    ClRcT rc = CL_OK, retCode;
    ClEoExecutionObjT *pThis = NULL;

    ++gClMsgCliRefCnt;
    /*
     * gClMsgInit is shared by msg server and extern'ed.
     * So it goes unaffected even with ref cnt
     */
    if(gClMsgInit == CL_TRUE)
        goto out;

    rc = clASPInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the ASP. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleDatabaseCreate(NULL, &gMsgHandleDatabase);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to create a handle database for message-client-init-handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clMsgCommIdlInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Initializing IDL failed. error code [0x%x].", rc);
        goto error_out_2;
    }

    /* Initializing the IDL generated code. */
    rc = clEoMyEoObjectGet(&pThis);
    CL_ASSERT(rc == CL_OK);

    gLocalAddress = clIocLocalAddressGet();
    gLocalPortId = pThis->eoPort;

    rc = clMsgIdlClientTableRegister(pThis->eoPort);
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

    rc = clMsgCltClientTableRegister(pThis->eoPort);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register the Client Table. error code [0x%x].", rc);
        goto error_out_5;
    }

    rc = clMsgCltSrvClientInstall();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install the Client Server Table. error code [0x%x].", rc);
        goto error_out_6;
    }

    rc = clMsgCltSrvClientTableRegister(pThis->eoPort);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register the Client Server Table. error code [0x%x].", rc);
        goto error_out_7;
    }

    /* Initialize cached ckpt for MSG queue & MSG queue group */
    rc = clMsgQCkptInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoints. error code [0x%x].", rc);
        goto error_out_8;
    }

    /* Initializing a database for maintaining queues. */
    rc = clMsgQueueInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the queue databases. error code [0x%x].", rc);
        goto error_out_9;
    }

    rc = clMsgSenderDatabaseInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the \"sender database\". error code [0x%x].", rc);
        goto error_out_10;
    }

    rc = clMsgReceiverDatabaseInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize \"receiver database\". error code [0x%x].", rc);
        goto error_out_11;
    }

    rc = clMsgQueueGroupHashInit(__MSG_QUEUE_GROUP_NODES, __MSG_QUEUE_GROUP_HASHES_PER_NODE);
    if(rc != CL_OK)
    {
        goto error_out_12;
    }

    rc = clOsalMutexInit(&gClGroupRRLock);
    CL_ASSERT(rc == CL_OK);

    gClMsgDispatchQueueCtrl.dispatchQueueSize = 0;
    if(!gClMsgDispatchQueueCtrl.dispatchTable)
    {
        rc = clOsalMutexInit(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
        CL_ASSERT(rc == CL_OK);
        gClMsgDispatchQueueCtrl.dispatchTable = clHeapCalloc(MSG_DISPATCH_QUEUE_HASH_BUCKETS, 
                                                             sizeof(*gClMsgDispatchQueueCtrl.dispatchTable));
        CL_ASSERT(gClMsgDispatchQueueCtrl.dispatchTable != NULL);
    }
    rc = clJobQueueInit(&gClMsgDispatchQueueCtrl.dispatchQueue, 0, CL_MSG_DISPATCH_QUEUE_THREADS);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize dispatch job queue [%#x]", rc);
        goto error_out_12;
    }
        
    return rc;

    error_out_12:
    clMsgReceiverDatabaseFin();
    error_out_11:
    clMsgSenderDatabaseFin();
    error_out_10:
    retCode = clMsgQueueFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize queue databases. error code [0x%x].", retCode);
    error_out_9:
    retCode = clMsgQCkptFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "clMsgQCkptFinalize(): error code [0x%x].", retCode);
    error_out_8:

    retCode = clMsgCltSrvClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Client Server Table. error code [0x%x].",retCode);
    error_out_7:
    retCode = clMsgCltSrvClientUninstall();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
    error_out_6:
    retCode = clMsgCltClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Client Table. error code [0x%x].",retCode);
    error_out_5:
    retCode = clMsgCltClientUninstall();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
    error_out_4:
    retCode = clMsgIdlClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Server Table. error code [0x%x].",retCode);
    error_out_3:
    clMsgCommIdlFinalize();
    error_out_2:
    retCode = clHandleDatabaseDestroy(gMsgHandleDatabase);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
    error_out_1:
    retCode = clASPFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to Finalize the ASP. error code [0x%x].", retCode);
    error_out:
    --gClMsgCliRefCnt;

    out:
    return rc;
}

/*
 * Should be called with msg cli lock held
 */
static ClRcT clMsgFinalize(void)
{
    ClRcT rc = CL_OK;
    if(!gClMsgCliRefCnt || --gClMsgCliRefCnt > 0)
        return rc;
    /*
     * Destroy the msg. dispatch queue.
     */
    msgDispatchQueueDestroy();

    clMsgCommIdlFinalize();

    /* Finalize the IDL generated code. */
    rc = clMsgIdlClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Server Table. error code [0x%x].",rc);

    rc = clMsgCltClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Client Table. error code [0x%x].",rc);

    rc = clMsgCltClientUninstall();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the just opened handle database. error code [0x%x].", rc);

    rc = clMsgCltSrvClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Client Server Table. error code [0x%x].",rc);

    rc = clMsgCltSrvClientUninstall();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the just opened handle database. error code [0x%x].", rc);

    /* Finalize cached ckpt for MSG queue & MSG queue group */
    rc = clMsgQCkptFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "clMsgQCkptFinalize(): error code [0x%x].", rc);    

    clMsgQueueGroupHashFinalize();

    /* Finalize database for maintaining queues. */
    clMsgReceiverDatabaseFin();

    clMsgSenderDatabaseFin();

    rc = clMsgQueueFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize queue databases. error code [0x%x].", rc);

    rc = clOsalMutexDestroy(&gClGroupRRLock);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the group round robin lock. error code [0x%x].", rc);

    rc = clHandleDatabaseDestroy(gMsgHandleDatabase);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the just opened handle database. error code [0x%x].", rc);

    rc = clASPFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to Finalize the ASP. error code [0x%x].", rc);

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
    
    MSG_CLI_LOCK();
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
        rc = VDECL_VER(clMsgInitClientSync, 4, 0, 0)(gIdlUcastHandle, (ClUint32T*)pVersion, tempMsgHandle, &pMsgLibInfo->handle);
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
    gMsgHandle = msgHandle;

    gClMsgInit = CL_TRUE;
    goto out;

    error_out_5:
    retCode = clDispatchDeregister(pMsgLibInfo->dispatchHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister the dispatch handle. error code [0x%x].", retCode);
    error_out_4:
    retCode = VDECL_VER(clMsgFinClientSync, 4, 0, 0)(gIdlUcastHandle, pMsgLibInfo->handle);
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
    MSG_CLI_UNLOCK();
    return CL_MSG_SA_RC(rc);
}



SaAisErrorT saMsgFinalize(
        SaMsgHandleT msgHandle
        )
{
    ClRcT rc;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_INIT_CHECK;

    MSG_CLI_LOCK();
    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "FIN", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clDispatchDeregister(pMsgLibInfo->dispatchHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister with dispatch library. error code [0x%x].", rc);

    rc = VDECL_VER(clMsgFinClientSync, 4, 0, 0)(gIdlUcastHandle, pMsgLibInfo->handle); 
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
    
    if(!gClMsgCliRefCnt)
    {
        gMsgHandle = 0;
        gClMsgInit = CL_FALSE;
    }

error_out:
    MSG_CLI_UNLOCK();

    return CL_MSG_SA_RC(rc);
}


static void clMsgAppMessageDeliveredCallbackFunc(ClIdlHandleT idlHandle,
        ClUint32T  sendType,
        ClNameT *pDest,
        ClMsgMessageIovecT *pMessage,
        SaTimeT sendTime,
        ClHandleT  senderHandle,
        SaTimeT timeout,
        ClRcT rc,
        ClPtrT pCallbackParams)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppMessageSendCallbackParamsT *pParam = (ClMsgAppMessageSendCallbackParamsT*)pCallbackParams;
    SaMsgHandleT msgHandle = pParam->msgHandle;

    retCode = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out;
    }

    ((ClMsgAppMessageSendCallbackParamsT*)pCallbackParams)->rc = CL_MSG_SA_RC(rc);
    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE, pCallbackParams);
    if(retCode != CL_OK)
        clLogError("MSG", "MDCb", "Failed to enqueue the callback into dispatcher. error code [0x%x].", retCode);

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "MDCb", "Failed to checkin the message handle. error code [0x%x].",retCode);

error_out:

    if(pMessage)
    {
        if(pMessage->pIovec)
        {
            for (ClUint32T i = 0; i < pMessage->numIovecs; i++)
            {
                if (pMessage->pIovec[i].iov_base)
                {
                    clHeapFree(pMessage->pIovec[i].iov_base);
                }
            }
            clHeapFree(pMessage->pIovec);
        }
        if(pMessage->senderName)
            clHeapFree(pMessage->senderName);
    }
}

static ClRcT clMsgQueueDestAddrGet(SaNameT *pDestination, ClIocAddressT *pQueueAddr, ClIocAddressT *pQServerAddr)
{
    ClRcT rc = CL_ERR_NOT_EXIST;
    ClBoolT retVal = CL_FALSE;
    ClMsgQueueCkptDataT queueData = {{0}};
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    ClMsgGroupRoundRobinT *pGroupRR;
    ClNameT *pTempQName;
    ClUint32T rrIndex = 0;
    ClUint32T startIndex = 0;

    retVal = clMsgQCkptExists((ClNameT*)pDestination, &queueData);

    if (retVal)
    {
        /* Check if MSG queue is being copied */
        if ((queueData.qAddress.nodeAddress == 0) 
                && (queueData.creationFlags != SA_MSG_QUEUE_PERSISTENT))
        {
            rc = CL_ERR_TRY_AGAIN;
            goto out;
        }
        else if ((queueData.qServerAddress.nodeAddress == 0) 
                && (queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT))
        {
            rc = CL_ERR_TRY_AGAIN;
            goto out;
        }

        pQueueAddr->iocPhyAddress = queueData.qAddress;
        pQServerAddr->iocPhyAddress = queueData.qServerAddress;
        rc = CL_OK;
    }
    else
    {
        if (clMsgQGroupCkptDataGet((ClNameT*)pDestination, &qGroupData) == CL_OK)
        {
            if (qGroupData.numberOfQueues == 0)
            {
                clLogWarning("MSG", "SND", "Message sent to [%.*s] -- but nobody is listening",pDestination->length, pDestination->value);
                goto out;
            }

            CL_OSAL_MUTEX_LOCK(&gClGroupRRLock);

            if(qGroupData.policy != SA_MSG_QUEUE_GROUP_BROADCAST)
            {
                if(clMsgGroupRRExists((ClNameT*)pDestination, &pGroupRR) == CL_FALSE)
                {
                    ClUint32T randIndex = rand() % qGroupData.numberOfQueues;
                    clMsgGroupRRAdd((ClNameT*)pDestination, randIndex, &pGroupRR);
                }

                startIndex = pGroupRR->rrIndex;
                if (startIndex >= qGroupData.numberOfQueues)
                {
                    startIndex = 0;
                    pGroupRR->rrIndex = 0;
                }
            }

            switch(qGroupData.policy)
            {
                case SA_MSG_QUEUE_GROUP_ROUND_ROBIN:
                    do
                    {
                        rrIndex = pGroupRR->rrIndex;

                        /* Look up the destination queue in cached checkpoint */
                        pTempQName = &qGroupData.pQueueList[rrIndex];
                        if (clMsgQCkptExists(pTempQName, &queueData) == CL_TRUE)
                        {
                            /* Skip MSG queue that is being moved */
                            if (   ((queueData.qAddress.nodeAddress != 0) 
                                 && (queueData.creationFlags != SA_MSG_QUEUE_PERSISTENT))
                               ||  ((queueData.qServerAddress.nodeAddress != 0) 
                                 && (queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT))  )
                            {
                                pQueueAddr->iocPhyAddress = queueData.qAddress;
                                pQServerAddr->iocPhyAddress = queueData.qServerAddress;
                                clNameCopy((ClNameT *)pDestination, &queueData.qName);
                                rc = CL_OK;
                                retVal = CL_TRUE;
                            }
                        }

                        rrIndex++;
                        if (rrIndex == qGroupData.numberOfQueues)
                            rrIndex = 0;
                        pGroupRR->rrIndex = rrIndex;

                    }while (rrIndex != startIndex && retVal == CL_FALSE);
                    if (retVal == CL_FALSE)
                    {
                      clLogWarning("MSG", "SND", "Message sent to [%.*s] using round robin send semantics -- but nobody is listening",pDestination->length, pDestination->value);
                    }                    

                    break;
                case SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE:
                    clLogWarning("MSG", "SND", "SAFmsg Specification: Local best queue is implemented as local round robin for now.");
                case SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN:
                    do
                    {
                        rrIndex = pGroupRR->rrIndex;

                        /* Look up the destination queue in cached checkpoint */
                        pTempQName = &qGroupData.pQueueList[rrIndex];
                        if (clMsgQCkptExists(pTempQName, &queueData) == CL_TRUE)
                        {
                            /* Check for local queue */
                            if (queueData.qAddress.nodeAddress == qGroupData.qGroupAddress.nodeAddress)
                            {
                                /* Skip MSG queue that is being moved */
                                if (   ((queueData.qAddress.nodeAddress != 0) 
                                     && (queueData.creationFlags != SA_MSG_QUEUE_PERSISTENT))
                                   ||  ((queueData.qServerAddress.nodeAddress != 0) 
                                     && (queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT))  )
                                {
                                    pQueueAddr->iocPhyAddress = queueData.qAddress;
                                    pQServerAddr->iocPhyAddress = queueData.qServerAddress;
                                    clNameCopy((ClNameT *)pDestination, &queueData.qName);
                                    rc = CL_OK;
                                    retVal = CL_TRUE;
                                }
                            }
                        }

                        rrIndex++;
                        if (rrIndex == qGroupData.numberOfQueues)
                            rrIndex = 0;

                        pGroupRR->rrIndex = rrIndex;
                    }while (rrIndex != startIndex && retVal == CL_FALSE);
                    if (retVal == CL_FALSE)
                    {
                      clLogWarning("MSG", "SND", "Message sent to [%.*s] using local round robin send semantics -- but nobody local is listening",pDestination->length, pDestination->value);
                    }                    

                    break;
                case SA_MSG_QUEUE_GROUP_BROADCAST:
                    pQueueAddr->iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
                    pQServerAddr->iocPhyAddress.nodeAddress = 0;
                    retVal = CL_TRUE;
                    rc = CL_OK;
                    break;
                default:
                    retVal = CL_FALSE;
                    clLogError("MSG", "SND", "Queue [%.*s] Unknown send semantics",pDestination->length, pDestination->value);
            }

            CL_OSAL_MUTEX_UNLOCK(&gClGroupRRLock);
            clMsgQGroupCkptDataFree(&qGroupData);
        }
        else
        {
            clLogWarning("MSG", "SND", "Message queue [%.*s] has no checkpoint entry",pDestination->length, pDestination->value);
        }        

    }
    
out:
    return rc;
}

static SaAisErrorT clMsgMessageSendInternal(
        ClBoolT isSync,
        SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pDestination,
        const ClMsgMessageIovecT *pMessage,
        SaTimeT timeout,
        SaMsgAckFlagsT ackFlags)
{ 
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgMessageIovecT tempMessage;
    SaNameT senderName = {0,{0}};
    SaTimeT sendTime = 0;
    SaNameT tempDest;
    ClIocAddressT queueAddr;
    ClIocAddressT queueServerAddr;

    CL_MSG_INIT_CHECK;

    clNameCopy((ClNameT *) &tempDest, (ClNameT *)pDestination);

    if(pDestination == NULL || pMessage == NULL || pMessage->pIovec == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "SND", "NULL parameter passed. error code [0x%x].", rc);
        goto error_out;
    }

    if(pMessage->numIovecs == 0)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "SND", "Number of iovec is 0. error code [0x%x].", rc);
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
        clLogError("MSG", "SND", "Failed to checkout the message handle, destination [%.*s]. error code [0x%x].", pDestination->length,pDestination->value,rc);
        goto error_out;
    }

    rc = clMsgQueueDestAddrGet(&tempDest, &queueAddr, &queueServerAddr);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SND", "Failed to discover destination [%.*s].  error code [0x%x].", pDestination->length,pDestination->value,rc);
        goto error_out;
    }

    /* FIXME : BUG : If NULL is passed to IDL the program crashes. Following lines are work-around. Can be removed once the IDL fix is in.*/ 
    memcpy(&tempMessage, pMessage, sizeof(*pMessage));
    if(tempMessage.senderName == NULL)
        tempMessage.senderName = &senderName;
    /***** Work around ends here. *****/

    if(ackFlags == SA_MSG_MESSAGE_DELIVERED_ACK)
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
        rc = clMsgClientMessageSend(&queueAddr, &queueServerAddr, (ClNameT*)&tempDest, &tempMessage, sendTime, timeout, isSync, ackFlags, &clMsgAppMessageDeliveredCallbackFunc, pCallbackParam);
    }
    else
    {
        rc = clMsgClientMessageSend(&queueAddr, &queueServerAddr, (ClNameT*)&tempDest, &tempMessage, sendTime, timeout, isSync, ackFlags, NULL, NULL);
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

ClUint32T getNextMsgId()
{
  pthread_mutex_lock(&mutexMsgId);
  currentMessageId++;
  pthread_mutex_unlock(&mutexMsgId);
  return currentMessageId;
}

SaAisErrorT saMsgMessageSend(
        SaMsgHandleT msgHandle,
        const SaNameT *pDestination,
        const SaMsgMessageT *pMessage,
        SaTimeT timeout)
{
    ClMsgMessageIovecT msgVector;
    struct iovec iovec = {0};
    iovec.iov_base = (void*)pMessage->data;
    iovec.iov_len = pMessage->size;

    msgVector.type = pMessage->type;
    msgVector.version = pMessage->version;
    msgVector.senderName = pMessage->senderName;
    msgVector.priority = pMessage->priority;
    msgVector.pIovec = &iovec;
    msgVector.numIovecs = 1;
    msgVector.messageId=getNextMsgId();
    return clMsgMessageSendIovec(msgHandle, pDestination, &msgVector, timeout);
}


SaAisErrorT saMsgMessageSendAsync(SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pDestination,
        const SaMsgMessageT *pMessage,
        SaMsgAckFlagsT ackFlags)
{
    ClMsgMessageIovecT msgVector;
    struct iovec iovec = {0};
    iovec.iov_base = (void*)pMessage->data;
    iovec.iov_len = pMessage->size;

    msgVector.type = pMessage->type;
    msgVector.version = pMessage->version;
    msgVector.senderName = pMessage->senderName;
    msgVector.priority = pMessage->priority;
    msgVector.pIovec = &iovec;
    msgVector.numIovecs = 1;
    msgVector.messageId=getNextMsgId();
    return clMsgMessageSendAsyncIovec(msgHandle, invocation, pDestination, &msgVector, ackFlags);
}



SaAisErrorT clMsgMessageSendIovec(
        SaMsgHandleT msgHandle,
        const SaNameT *pDestination,
        const ClMsgMessageIovecT *pMessage,
        SaTimeT timeout)
{
    return clMsgMessageSendInternal(CL_TRUE, msgHandle, 0, pDestination, pMessage, timeout, 0);
}


SaAisErrorT clMsgMessageSendAsyncIovec(SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pDestination,
        const ClMsgMessageIovecT *pMessage,
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
        clLogError("MSG", "GET", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(SaMsgMessageT), rc);
        goto error_out;
    }

    pTempSenderName = (SaNameT*)clHeapAllocate(sizeof(SaNameT));
    if(pTempSenderName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "GET", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(SaNameT), rc);
        goto error_out_1;
    }

    /*IDL will allocate memory depending on the size. This one byte is IDL's pray, as it frees the this memory before allocating new.*/
    if(!(size = pMessage->size))
        size = 1;

    pTempData = (ClUint8T*)clHeapAllocate(size);
    if(pTempData == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "GET", "Failed to allocate memory of 1 byte. error code [0x%x].", rc);
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
    *pTempMessage = NULL; // Clear it out so misbehaving apps will crash.
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
    ClRcT rc = CL_OK;
    ClRcT retCode;
    ClCntNodeHandleT nodeHandle = NULL;
    SaMsgMessageT *pRecvMessage;
    ClUint32T i=0;
    ClBoolT condVarFlag = CL_FALSE;
    ClTimerTimeOutT tempTimeout;
    ClMsgQueueInfoT *pQInfo = NULL;
    ClMsgReceivedMessageDetailsT *pRecvInfo;
    ClNameT qName;
    ClMsgQueueCkptDataT queueData = {{0}};
    CL_MSG_INIT_CHECK;
#ifdef MESSAGE_ORDER
    ClUint8T tryAgain=0;
try_again:
#endif
    if(pMessage == NULL || pSenderId == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clDbgCodeError(rc,("Parameter verification: NULL pointer passed or message or sender id."));
        goto error_out;
    }

    if(pMessage->data == NULL)
        pMessage->size = 0;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clDbgCodeError(rc,("Failed to checkout the queue handle. error code [0x%x].",rc));
        goto error_out;
    }
    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    clNameCopy(&qName, &pQInfo->pQueueEntry->qName);

get_message:
    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; i++)
    {
        rc = clCntFirstNodeGet(pQInfo->pPriorityContainer[i], &nodeHandle);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            continue;
        if(rc != CL_OK)
        {
            clLogError("MSG", "GET", "Failed to get the first node from the message queue of priority %d. error code [0x%x].", i, rc);
            continue;
        }

        rc = clCntNodeUserDataGet(pQInfo->pPriorityContainer[i], nodeHandle, (ClCntDataHandleT*)&pRecvInfo);
        if(rc != CL_OK)
        {
            clLogError("MSG", "GET", "Failed to get the user-data from the first node. error code 0x%x", rc);
            continue;
        }

#ifdef MESSAGE_ORDER
        // check the messageID . try again if wrong order
        ClUint32T *currentMsgId;
        ClHandleT *sendHandle;
        currentMsgId =getCurrentId(&pQInfo->currentIdTable,&pRecvInfo->pMessage->senderHandle);
        if(currentMsgId == NULL)
        {
            currentMsgId=clHeapAllocate(sizeof(ClUint32T));
            sendHandle = clHeapAllocate(sizeof(ClHandleT)); ;
            *currentMsgId=0;
            *sendHandle=pRecvInfo->pMessage->senderHandle;

        }
        else
        {
            sendHandle=&pRecvInfo->pMessage->senderHandle;
            clLogError("MSG", "GET", "handle [%d] : get the user-data from the first node with message Id [%d] current id [%d]",(int)queueHandle,(int)pRecvInfo->pMessage->messageId,*currentMsgId);

        }
        if(pRecvInfo->pMessage->messageId - *currentMsgId !=1)
        {
          if(pRecvInfo->pMessage->messageId!=1)
          {
              if(tryAgain < MSG_RECEIVE_MAX_RETRY)
              {
                CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
                rc = clHandleCheckin(gClMsgQDatabase, queueHandle);
                usleep(1000);
                pQInfo = NULL;
                nodeHandle = NULL;
                tryAgain++;
                goto try_again;
              }
          }
        }
        *currentMsgId=pRecvInfo->pMessage->messageId;
        insertCurrentId(&pQInfo->currentIdTable,&pRecvInfo->pMessage->senderHandle,currentMsgId);
#endif
        pRecvMessage = pRecvInfo->pMessage;
        if(pMessage->size != 0 && pMessage->size < pRecvMessage->size)
        {
            pMessage->size = pRecvMessage->size;
            rc = CL_MSG_RC(CL_ERR_NO_SPACE);
            clLogError("MSG", "GET", "The buffer provided by the client is not enough to hold the received message. error code [0x%x].",rc);
            goto error_out_1;
        }

        /* GAS:  This code copies the data buffer from pTempMessage to pReceiveMsg and then frees the pTempMessage data buffer.  Instead, why not just move the pointer "pReceiveMsg->data = pTempMessage->data".  If pRecvMessage->data is then set to NULL, clMsgMessageFree will not delete the data buffer. 
         * SAI-AIS-MSG-B.03.01 - Section 3.8.3: if data as an in parameter is not NULL, it points to memory allocated by the invoking process for the buffer.
         */
        if(pMessage->data == NULL)
        {
            ClUint8T *pTemp;
            pTemp = (ClUint8T*)clHeapAllocate(pRecvMessage->size);
            if(pTemp == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
                clDbgResourceLimitExceeded(clDbgMemoryResource,0,("Failed to allocate [%llu] bytes of memory.",pRecvMessage->size));
                goto error_out_1;
            }
            pMessage->data = pTemp;
        }

        pMessage->type = pRecvMessage->type;
        pMessage->version = pRecvMessage->version;
        pMessage->size = pRecvMessage->size;
        pMessage->priority = pRecvMessage->priority;
        if(pMessage->senderName != NULL)
            memcpy(pMessage->senderName, pRecvMessage->senderName, sizeof(ClNameT));
        memcpy(pMessage->data, pRecvMessage->data, pRecvMessage->size);

        /*
         * Just adding an inconsistent queue size report armor which shouldn't be anyway hit.
         */
        if(pQInfo->usedSize[i] >= pRecvMessage->size)
            pQInfo->usedSize[i] =  pQInfo->usedSize[i] - pRecvMessage->size;
        else
        {
            clLogWarning("QUE", "GET", "MSG queue used size [%lld] is lesser than message size [%lld] "
                         "for queue handle [%llx]", pQInfo->usedSize[i], pRecvMessage->size, queueHandle);
            pQInfo->usedSize[i] = 0;
        }

        if(pQInfo->numberOfMessages[i] > 0)
            --pQInfo->numberOfMessages[i];

        clMsgMessageFree(pRecvMessage);

        retCode = clCntNodeDelete(pQInfo->pPriorityContainer[i], nodeHandle);
        if(retCode != CL_OK)
            clLogError("MSG", "GET", "Failed to delete a node from container. error code [0x%x].", retCode);

        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        if(pSendTime)
            *pSendTime = pRecvInfo->sendTime;
        *pSenderId = pRecvInfo->replyId;

        clHeapFree(pRecvInfo);

        goto out;
    }

    /*
     * Bail out immediately if timeout is 0 and queue is empty. with timeout
     */
    if(!timeout)
    {
        rc = CL_MSG_RC(CL_ERR_TIMEOUT);
        clLogDebug("MSG", "GET", "No messages found in the queue for [%.*s]",
                   pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value);
        goto error_out_1;
    }

    if(condVarFlag == CL_FALSE)
    {
        ClBoolT monitorDisabled = CL_FALSE;

        condVarFlag = CL_TRUE;

        clMsgTimeConvert(&tempTimeout, timeout);

        pQInfo->numThreadsBlocked++;

        /* If the task pool monitor's timeout is bigger then the user timeout then turn off the task pool monitor temporarily */
        if((!tempTimeout.tsSec && !tempTimeout.tsMilliSec)
           || 
           (tempTimeout.tsSec*2 >= CL_TASKPOOL_MONITOR_INTERVAL))
        {
            monitorDisabled = CL_TRUE;
            clTaskPoolMonitorDisable();
        }
        /* Now wait for the message */
        rc = clOsalCondWait(pQInfo->qCondVar, &pQInfo->qLock, tempTimeout);
        if(monitorDisabled)
        {
            monitorDisabled = CL_FALSE;
            clTaskPoolMonitorEnable();
        }
        if(pQInfo->numThreadsBlocked == 0)
        {
            rc = CL_MSG_RC(CL_ERR_INTERRUPT);
            clLogInfo("MSG", "RCV", "[%s] is unblocked/cancled by the application. error code [0x%x].",__FUNCTION__, rc);
            goto error_out_1;
        }
        pQInfo->numThreadsBlocked--;

        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {
            clLogInfo("MSG", "RCV", "Message get timed-out, while waiting for message in [%.*s]. timeout [%lld ns]. error code [0x%x].", 
                    pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, timeout, rc);
            goto error_out_1;
        }
        if(rc != CL_OK)
        {
            clLogError("MSG", "RCV", "Failed at Cond Wait for getting a message. error code [0x%x].", rc);
            goto error_out_1;
        }
        /* GAS: I think this "goto" logic causes the loop to restart if any message is handled.  This ensures that high priority messages
           are handled first.  But please change the goto into a loop construct.  */
        goto get_message;
    }

    rc = CL_MSG_RC(CL_ERR_NOT_EXIST);
    
error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
out:
    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "RCV", "Failed to checkin the queue handle. error code [0x%x].", retCode);

    /* GAS: What is the purpose of this code? Is it to clear one message from the redundant queue? */
    if(rc == CL_OK)
    {
        if (clMsgQCkptExists((ClNameT *)&qName, &queueData) == CL_TRUE)
        {
            if ((queueData.qAddress.nodeAddress == gLocalAddress)
                 && (queueData.qAddress.portId == gLocalPortId)
                 && (queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT)
                 && (queueData.qServerAddress.nodeAddress != 0))
            {
                clMsgMessageGet_Idl(queueData.qServerAddress, (ClNameT *)&qName, timeout);
            }
        }
    }

error_out:
    return CL_MSG_SA_RC(rc);
}


ClRcT clMsgMessageReceiveCallback(SaMsgQueueHandleT qHandle)
{
    ClRcT rc;
    ClMsgMessageReceiveCallbackParamsT *pParam;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgDispatchQueueT *pMsgDispatch = NULL;
    SaMsgHandleT msgHandle = 0;
    ClBoolT asyncReceive = CL_FALSE;

    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    pMsgDispatch = msgDispatchQueueFind(qHandle);
    if(pMsgDispatch)
    {
        msgHandle = pMsgDispatch->msgHandle;
    }
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    if(!msgHandle)
    {
        msgHandle = gMsgHandle;
        asyncReceive = CL_TRUE;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
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
    pParam->asyncReceive = asyncReceive;
    rc = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_MESSAGE_RECEIVED_CALLBACK_TYPE, pParam); 
    if(rc != CL_OK)
    {
        clLogError("MSG", "RCVcb", "Failed to enqueue a callback to Dispatch queue. error code [0x%x].", rc);
    }

error_out_1:
    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
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

    CL_MSG_INIT_CHECK;

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
    SaNameT tempDest;
    ClIocAddressT queueAddr;
    ClIocAddressT queueServerAddr;

    CL_MSG_INIT_CHECK;

    clNameCopy((ClNameT *)&tempDest, (ClNameT *)pDestAddress);

    if(pDestAddress == NULL || pSendMsg  == NULL || pReceiveMsg  == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clDbgCodeError(rc,("Parameter verification: NULL pointer passed"));
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
        clDbgCodeError(rc,("Failed to checkout the queue handle. error code [0x%x].",rc));
        goto error_out;
    }

    /* GAS:  Why is pTempMessage created instead of using pReceiveMsg directly?
     * Idl will free the pTempMessage->data and then re-allocate new buffer. 
     * However, in SAI-AIS-MSG-B.03.01 - Section 3.9.1: if data as an in parameter is not NULL, it points to memory allocated by the invoking process for the buffer.
     */
    rc = clMsgReceiveMessageForm(&pTempMessage, pReceiveMsg);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create a receive message. error code [0x%x].",rc);
        goto error_out_1;
    }

    if(pReceiveMsg->data == NULL)
        pTempMessage->size = 0;

    ClInt32T tries = 0, maxTry=3;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 1000 };
    do 
    {
       rc = clMsgQueueDestAddrGet(&tempDest, &queueAddr, &queueServerAddr);
       if (rc != CL_OK)
       {
          clLogNotice("MSG", "SendRecv", "Failed at send and receive to [%.*s]. error code [0x%x]. Try [%d] of [%d]",
               pDestAddress->length, pDestAddress->value, rc, tries+1, maxTry);		  
       }
     } while ((rc!=CL_OK) && (++tries<maxTry) && (clOsalTaskDelay(delay)==CL_OK));

     if (rc!=CL_OK)
     {
         clLogError("MSG", "SendRecv", "Queue address resolution failed with [%s] after trying [%d] times", clErrorToString(rc), tries);
         goto error_out_2; 
     }
    
    rc = clMsgQueueDestAddrGet(&tempDest, &queueAddr, &queueServerAddr);
    if (rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed at send and receive to [%.*s]. error code [0x%x].",
               pDestAddress->length, pDestAddress->value, rc);
        goto error_out_2;
    }

    rc = clMsgClientMessageSendReceive(&queueAddr, &queueServerAddr, (ClNameT*)&tempDest,
            &tempSendMsg, sendTime, pTempMessage, &replySentTime, timeout);
    if(rc != CL_OK)
    {
        /* Note : The above call would have freed pTempMessage->senderName and pTempMessage->data. 
         * so just pTempMessage needs to be freed.*/
        clLogCritical("MSG", "SendRecv", "Failed at send and receive to [%.*s]. error code [0x%x].",
                pDestAddress->length, pDestAddress->value, rc);
        /* Note : Packet might have have got dropped on the server side.*/
        goto error_out_2;
    }

    if(pReplySendTime != NULL)
        *pReplySendTime = replySentTime;


    /* GAS:  This code copies the data buffer from pTempMessage to pReceiveMsg and then frees the pTempMessage data buffer.  Instead, why not just move the pointer "pReceiveMsg->data = pTempMessage->data" 
     * SAI-AIS-MSG-B.03.01 - Section 3.9.1: if data as an in parameter is not NULL, it points to memory allocated by the invoking process for the buffer.
     */
    if(pReceiveMsg->data == NULL)
    {
        pTemp = (ClUint8T*)clHeapAllocate(pTempMessage->size);
        if(pTemp == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clDbgResourceLimitExceeded(clDbgMemoryResource,0,("Failed to allocate [%llu] bytes of memory.  Dropping the received packet.",pTempMessage->size));
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
        ClUint32T  sendType,
        ClNameT *pDest,
        ClMsgMessageIovecT *pMessage,
        SaTimeT sendTime,
        ClHandleT  senderHandle,
        SaTimeT timeout,
        ClRcT rc,
        ClPtrT pCallbackParams)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppMessageSendCallbackParamsT *pParam = (ClMsgAppMessageSendCallbackParamsT*)pCallbackParams;
    SaMsgHandleT msgHandle = pParam->msgHandle;

    retCode = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
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

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "MDCb", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    if(pMessage)
    {
        if(pMessage->pIovec)
        {
            for (ClUint32T i = 0; i < pMessage->numIovecs; i++)
            {
                if (pMessage->pIovec[i].iov_base)
                {
                    clHeapFree(pMessage->pIovec[i].iov_base);
                }
            }
            clHeapFree(pMessage->pIovec);
        }
        if(pMessage->senderName)
            clHeapFree(pMessage->senderName);
    }
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

    CL_MSG_INIT_CHECK;

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
        clLogError("MSG", "REPLY", "Invalid message priority passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    if(ackFlags == SA_MSG_MESSAGE_DELIVERED_ACK)
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
        rc = clMsgClientMessageReply((SaMsgMessageT*)pReplyMessage, sendTime, *pSenderId, timeout, isSync, ackFlags, &clMsgAppMessageReplyDeliveredCallbackFunc , pCallbackParam);
    }
    else
    {
        rc = clMsgClientMessageReply((SaMsgMessageT*)pReplyMessage, sendTime, *pSenderId, timeout, isSync, ackFlags, NULL, NULL);
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

    CL_MSG_INIT_CHECK;

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

    CL_MSG_INIT_CHECK;

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
    if(!gClMsgDispatchQueueCtrl.dispatchTable ||
       !gClMsgDispatchQueueCtrl.dispatchQueueSize) 
        return;
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
    gClMsgDispatchQueueCtrl.dispatchQueueSize = 0;
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    clJobQueueDelete(&gClMsgDispatchQueueCtrl.dispatchQueue);
    return;
}

static void msgDispatchQueueAdd(SaMsgHandleT msgHandle,
                                SaMsgQueueHandleT queueHandle,
                                SaMsgMessageReceivedCallbackT callback)
{
    ClMsgDispatchQueueT *pDispatchQueue = NULL;
    ClUint32T key = (ClUint32T)(ClUint64T)queueHandle & MSG_DISPATCH_QUEUE_HASH_MASK;
    if(!(pDispatchQueue = msgDispatchQueueFind(queueHandle)))
    {
        pDispatchQueue = clHeapCalloc(1, sizeof(*pDispatchQueue));
        CL_ASSERT(pDispatchQueue != NULL);
        pDispatchQueue->msgHandle = msgHandle;
        pDispatchQueue->queueHandle = queueHandle;
        pDispatchQueue->msgReceivedCallback = callback;
        hashAdd(gClMsgDispatchQueueCtrl.dispatchTable, key, &pDispatchQueue->hash);
        ++gClMsgDispatchQueueCtrl.dispatchQueueSize;
    }
    else
    {
        /*
         * Update the msg handle and callback for the queue
         */
        pDispatchQueue->msgHandle = msgHandle;
        pDispatchQueue->msgReceivedCallback = callback;
    }
}

ClRcT clMsgDispatchQueueRegisterInternal(SaMsgHandleT msgHandle,
                                         SaMsgQueueHandleT queueHandle,
                                         SaMsgMessageReceivedCallbackT callback)
{
    if(!gClMsgDispatchQueueCtrl.dispatchTable) 
        return CL_MSG_RC(CL_ERR_NOT_INITIALIZED);

    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    msgDispatchQueueAdd(msgHandle, queueHandle, callback);
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    return CL_OK;
}

ClRcT clMsgDispatchQueueRegister(SaMsgQueueHandleT queueHandle,
                                 SaMsgMessageReceivedCallbackT callback)
{
    CL_MSG_INIT_CHECK;

    if(!callback)
        return CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
    
    return clMsgDispatchQueueRegisterInternal(0, queueHandle, callback);
}

ClRcT clMsgDispatchQueueDeregisterInternal(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc = CL_OK;

    if(!gClMsgDispatchQueueCtrl.dispatchTable)
        return CL_MSG_RC(CL_ERR_NOT_INITIALIZED);

    clOsalMutexLock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);
    rc = msgDispatchQueueDel(queueHandle);
    clOsalMutexUnlock(&gClMsgDispatchQueueCtrl.dispatchQueueLock);

    return rc;
}

ClRcT clMsgDispatchQueueDeregister(SaMsgQueueHandleT queueHandle)
{
    CL_MSG_INIT_CHECK;
    return clMsgDispatchQueueDeregisterInternal(queueHandle);
}
