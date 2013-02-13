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
 * File        : clMsgEo.c
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
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clIdlApi.h>
#include <clDispatchApi.h>
#include <clMsgApi.h>
#include <clMsgCommon.h>
#include <clMsgApiExt.h>
#include <clMsgCkptClient.h>
#include <clMsgQueue.h>
#include <clMsgDebugInternal.h>
#include <clMsgIdl.h>
#include <clCpmExtApi.h>

#include <msgIdlClientQueueCallsFromClientClient.h>
#include <msgIdlClientCallsFromServerClient.h>
#include <msgCltSrvClientCallsFromClientToClientServerClient.h>
#include <msgCltSrvClientCallsFromClientToClientServerServer.h>

static void clMsgAppQueueOpenCallbackFunc(ClHandleT msgHandle,
                                          SaInvocationT invocation,
                                          ClHandleT qHandle, ClRcT rc)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppQueueOpenCallbackParamsT *pParam;

    pParam = (ClMsgAppQueueOpenCallbackParamsT*)clHeapAllocate(sizeof(ClMsgAppQueueOpenCallbackParamsT));
    if(pParam == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "QOCb", "Failed to allocate memory for %zd bytes. error code [0x%x].", sizeof(ClMsgAppQueueOpenCallbackParamsT), rc);
        goto error_out;
    }

    retCode = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOCb", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out1;
    }

    pParam->msgHandle = msgHandle;
    pParam->invocation = invocation;
    pParam->rc = CL_MSG_SA_RC(rc);
    pParam->queueHandle = qHandle;
    if(qHandle && rc == CL_OK && pMsgLibInfo->callbacks.saMsgMessageReceivedCallback)
    {
        clMsgDispatchQueueRegisterInternal(msgHandle, qHandle,
                                           pMsgLibInfo->callbacks.saMsgMessageReceivedCallback);
    }

    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_QUEUE_OPEN_CALLBACK_TYPE, (void *)pParam);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOCb", "Failed to enqueue the callback into dispatcher. error code [0x%x].", retCode);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOC", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }
    goto out;

error_out1:
    clHeapFree(pParam);
error_out:
out:
    return;
}

static ClRcT clMsgQueueOpenNew(     SaMsgHandleT msgHandle,
                                          SaInvocationT invocation,
                                          const SaNameT *pQueueName,
                                          const SaMsgQueueCreationAttributesT *pCreationAttributes,
                                          SaMsgQueueOpenFlagsT openFlags,
                                          SaTimeT timeout,
                                          SaMsgQueueHandleT *pQueueHandle
                                          )
{
    ClRcT rc = CL_OK;
    ClRcT retCode;

    /* Allocate a new msg queue */
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clMsgQueueAllocate((ClNameT *)pQueueName, /* openFlags unused */ 0, (SaMsgQueueCreationAttributesT *)pCreationAttributes, pQueueHandle);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to allocate queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out;
    }

    rc = clMsgQueueOpen(*pQueueHandle, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to initialize queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out1;
    }

    /* Register queue with server*/
    rc = VDECL_VER(clMsgQueueOpenClientSync, 4, 0, 0)(gIdlUcastHandle, (ClNameT*)pQueueName, 
                                                            (SaMsgQueueCreationAttributesT *)pCreationAttributes, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to register queue with server. error code [0x%x].", rc);
        goto error_out1;
    }

    goto out;

error_out1:
    retCode = clMsgQueueFreeByHandle(*pQueueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "OPEN", "Failed to free the queue. error code [0x%x].", retCode);
    *pQueueHandle = 0;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    clMsgQEntryDel((ClNameT *)pQueueName);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
out:
    return rc;
}

static ClRcT clMsgQueueOpenRemote(  SaMsgHandleT msgHandle,
                                          SaInvocationT invocation,
                                          const SaNameT *pQueueName,
                                          const SaMsgQueueCreationAttributesT *pCreationAttributes,
                                          SaMsgQueueOpenFlagsT openFlags,
                                          SaTimeT timeout,
                                          SaMsgQueueHandleT *pQueueHandle,
                                          ClIocPhysicalAddressT qAddress,
                                          ClBoolT qDelete
                                          )
{
    ClRcT rc;
    ClRcT retCode;
    SaMsgQueueCreationAttributesT qAttrs;
    ClMsgQueueCkptDataT queueData = {{0}};

    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    /* Set MSG queue address = 0 before moving queue */
    if (qDelete)
    {
        /* Look up msg queue in the cached checkpoint */
        if(clMsgQCkptExists((ClNameT *)pQueueName, &queueData) == CL_FALSE)
        {
            rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
            clLogError("QUE", "OPEN", "Failed to get the message queue information from the cached ckpt. error code [0x%x].", rc);
            goto error_out;
        }

        queueData.qAddress.nodeAddress = 0;
        rc = VDECL_VER(clMsgQDatabaseUpdateClientSync, 4, 0, 0)(gIdlUcastHandle, CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(CL_OK != rc)
        {
            clLogError("QUE", "OPEN", "Failed to update message queue cache checkpoints. error code [0x%x].", rc);
        }
    }

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = qAddress;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Copy creation attributes from remote queue */
    rc = VDECL_VER(clMsgQueueInfoGetClientSync, 4, 0, 0)(idlHandle, (ClNameT *) pQueueName, &qAttrs);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to get queue [%.*s]'s information. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out1;
    }

    /* Allocate a new msg queue */
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clMsgQueueAllocate((ClNameT *)pQueueName, /* openFlags unused */ 0, &qAttrs, pQueueHandle);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to allocate queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out1;
    }

    /* Copy messages from remote queue */
    if(!(openFlags & SA_MSG_QUEUE_EMPTY))
    {
        rc = VDECL_VER(clMsgQueueMoveMessagesClientSync, 4, 0, 0)(idlHandle, (ClNameT *) pQueueName, openFlags, qDelete);
        if(rc != CL_OK)
        {
            clLogError("QUE", "OPEN", "Failed to allocate queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            goto error_out2;
        }
    }

    rc = clMsgQueueOpen(*pQueueHandle, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to initialize queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out2;
    }

    /* Register queue with server*/
    rc = VDECL_VER(clMsgQueueOpenClientSync, 4, 0, 0)(gIdlUcastHandle, (ClNameT*)pQueueName, 
                                                            (SaMsgQueueCreationAttributesT *)pCreationAttributes, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to register queue with server. error code [0x%x].", rc);
        goto error_out2;
    }

    clIdlHandleFinalize(idlHandle);
    goto out;

error_out2:
    retCode = clMsgQueueFreeByHandle(*pQueueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "OPEN", "Failed to free the queue. error code [0x%x].", retCode);
    *pQueueHandle = 0;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    clMsgQEntryDel((ClNameT *)pQueueName);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out1:
    clIdlHandleFinalize(idlHandle);
error_out:
out:
    return rc;
}

static ClRcT clMsgQueueOpenLocal(   SaMsgHandleT msgHandle,
                                          SaInvocationT invocation,
                                          const SaNameT *pQueueName,
                                          const SaMsgQueueCreationAttributesT *pCreationAttributes,
                                          SaMsgQueueOpenFlagsT openFlags,
                                          SaTimeT timeout,
                                          SaMsgQueueHandleT *pQueueHandle
                                          )
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pQEntry;
    SaMsgQueueHandleT qHandle;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists((ClNameT *)pQueueName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "OPEN", "Queue [%.*s] doesn't exist. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out;
    }

    qHandle = pQEntry->qHandle;
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clMsgQueueOpen(qHandle, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to initialize queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out;
    }

    *pQueueHandle = qHandle;

    /* Register queue with server*/
    rc = VDECL_VER(clMsgQueueOpenClientSync, 4, 0, 0)(gIdlUcastHandle, (ClNameT*)pQueueName, 
                                                            (SaMsgQueueCreationAttributesT *)pCreationAttributes, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to register queue with server. error code [0x%x].", rc);
        goto error_out;
    }

    goto out;
error_out:
out:
    return rc;
}

typedef struct {
    SaMsgHandleT msgHandle;
    SaInvocationT invocation;
    SaNameT *pQueueName;
    SaMsgQueueCreationAttributesT *pCreationAttributes;
    SaMsgQueueOpenFlagsT openFlags;
    ClIocPhysicalAddressT qAddress;
    ClBoolT qDelete;
}ClMsgOpenAsyncParamsT;

static void *clMsgQueueOpenNewAsync(void *pParam)
{
    ClRcT rc;
    SaMsgHandleT msgHandle = ((ClMsgOpenAsyncParamsT*)pParam)->msgHandle;
    SaInvocationT invocation = ((ClMsgOpenAsyncParamsT*)pParam)->invocation;
    SaNameT *pQueueName = ((ClMsgOpenAsyncParamsT*)pParam)->pQueueName;
    SaMsgQueueCreationAttributesT *pCreationAttributes = ((ClMsgOpenAsyncParamsT*)pParam)->pCreationAttributes;
    SaMsgQueueOpenFlagsT openFlags = ((ClMsgOpenAsyncParamsT*)pParam)->openFlags;
    SaMsgQueueHandleT queueHandle;
    
    rc = clMsgQueueOpenNew(msgHandle, invocation, pQueueName, pCreationAttributes,
                          openFlags, 0, &queueHandle);
    if (CL_GET_ERROR_CODE(rc) != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to open a message queue. error code [0x%x].", rc);
    }

    /* Invoke Msg queue open callback function for async case */
    clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, queueHandle, rc);
    clHeapFree(pQueueName);
    clHeapFree(pParam);

    return NULL;
}

static void *clMsgQueueOpenRemoteAsync(void *pParam)
{
    ClRcT rc;
    SaMsgHandleT msgHandle = ((ClMsgOpenAsyncParamsT*)pParam)->msgHandle;
    SaInvocationT invocation = ((ClMsgOpenAsyncParamsT*)pParam)->invocation;
    SaNameT *pQueueName = ((ClMsgOpenAsyncParamsT*)pParam)->pQueueName;
    SaMsgQueueCreationAttributesT *pCreationAttributes = ((ClMsgOpenAsyncParamsT*)pParam)->pCreationAttributes;
    SaMsgQueueOpenFlagsT openFlags = ((ClMsgOpenAsyncParamsT*)pParam)->openFlags;
    SaMsgQueueHandleT queueHandle;
    ClIocPhysicalAddressT qAddress = ((ClMsgOpenAsyncParamsT*)pParam)->qAddress;
    ClBoolT qDelete = ((ClMsgOpenAsyncParamsT*)pParam)->qDelete;

    rc = clMsgQueueOpenRemote(msgHandle, invocation, pQueueName, pCreationAttributes,
                          openFlags, 0, &queueHandle, qAddress, qDelete);
    if (CL_GET_ERROR_CODE(rc) != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to open a message queue. error code [0x%x].", rc);
    }

    /* Invoke Msg queue open callback function for async case */
    clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, queueHandle, rc);

    clHeapFree(pQueueName);
    clHeapFree(pParam);

    return NULL;
}

static void *clMsgQueueOpenLocalAsync(void *pParam)
{
    ClRcT rc;
    SaMsgHandleT msgHandle = ((ClMsgOpenAsyncParamsT*)pParam)->msgHandle;
    SaInvocationT invocation = ((ClMsgOpenAsyncParamsT*)pParam)->invocation;
    SaNameT *pQueueName = ((ClMsgOpenAsyncParamsT*)pParam)->pQueueName;
    SaMsgQueueCreationAttributesT *pCreationAttributes = ((ClMsgOpenAsyncParamsT*)pParam)->pCreationAttributes;
    SaMsgQueueOpenFlagsT openFlags = ((ClMsgOpenAsyncParamsT*)pParam)->openFlags;
    SaMsgQueueHandleT queueHandle;

    rc = clMsgQueueOpenLocal(msgHandle, invocation, pQueueName, pCreationAttributes,
                          openFlags, 0, &queueHandle);
    if (CL_GET_ERROR_CODE(rc) != CL_OK)
    {
        clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
    }

    /* Invoke Msg queue open callback function for async case */
    clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, queueHandle, rc);
    clHeapFree(pQueueName);
    clHeapFree(pParam);

    return NULL;
}

static SaAisErrorT clMsgQueueOpenInternal(
                                          ClBoolT syncCall,
                                          SaMsgHandleT msgHandle,
                                          SaInvocationT invocation,
                                          const SaNameT *pQueueName,
                                          const SaMsgQueueCreationAttributesT *pCreationAttributes,
                                          SaMsgQueueOpenFlagsT openFlags,
                                          SaTimeT timeout,
                                          SaMsgQueueHandleT *pQueueHandle
                                          )
{
    SaAisErrorT                      rc = CL_OK;
    ClRcT                            retCode;
    ClMsgLibInfoT                    *pMsgLibInfo = NULL;
    ClBoolT                          qExists;
    ClMsgQueueCkptDataT              queueData;
    ClInt32T                         tries = 0;
    ClTimerTimeOutT                  delay = {.tsSec = 0, .tsMilliSec = 500 };

    CL_MSG_INIT_CHECK;

    /* Data input validation */
    if((syncCall == CL_TRUE && pQueueHandle == NULL) || pQueueName == NULL || 
       ((openFlags & SA_MSG_QUEUE_CREATE) && (pCreationAttributes == NULL)))
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clDbgCodeError(rc, ("NULL parameter passed."));
        goto error_out;
    }

    if((openFlags != 0 && !(openFlags & (SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY | SA_MSG_QUEUE_RECEIVE_CALLBACK))) ||
       (pCreationAttributes != NULL && pCreationAttributes->creationFlags != SA_MSG_QUEUE_PERSISTENT && pCreationAttributes->creationFlags != 0))
    {
        rc = CL_MSG_RC(CL_ERR_BAD_FLAG);
        clDbgCodeError(rc, ("Invalid open flags passed."));
        goto error_out;
    }

    if( !(openFlags & SA_MSG_QUEUE_CREATE) && (pCreationAttributes != NULL))
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(rc, ("Creation attributes should be NULL, if an existing queue is to be opened. Or, the SA_MSG_QUEUE_CREATE openflag must be passed to create a queue."));
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clDbgCodeError(rc, ("Failed to checkout the message handle."));
        goto error_out;
    }

    if(syncCall == CL_FALSE && pMsgLibInfo->callbacks.saMsgQueueOpenCallback == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clDbgCodeError(rc, ("The message-queue-open callback is not registered."));
        goto error_out_1;
    }

    if(openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK && pMsgLibInfo->callbacks.saMsgMessageReceivedCallback == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clDbgCodeError(rc, ("The message received callback is not registered."));
        goto error_out_1;
    }

retry:
    qExists = clMsgQCkptExists((ClNameT *)pQueueName, &queueData);

    if ((qExists == CL_FALSE) && !(openFlags & SA_MSG_QUEUE_CREATE))
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "OPEN", "Queue [%.*s] does not exist. Open needs [SA_MSG_QUEUE_CREATE] flag passed. error code [0x%x].",
                pQueueName->length, pQueueName->value, rc);
        goto error_out_1;
    }

    if ((qExists == CL_TRUE) 
            && (queueData.state == CL_MSG_QUEUE_OPEN)
            && (queueData.creationFlags != SA_MSG_QUEUE_PERSISTENT))
    {
        if (syncCall)
        {
            if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
            {
                goto retry;
            }

            rc = CL_MSG_RC(CL_ERR_INUSE);
            clLogError("QUE", "OPEN", "Queue [%.*s] is in use. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            goto error_out_1;
        }
        else
        {
            retCode = CL_MSG_RC(CL_ERR_INUSE);
            clLogError("QUE", "OPEN", "Queue [%.*s] is in use. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, 0, retCode);
            goto error_out_1;
        }
    }

    if ((qExists == CL_TRUE) 
            && (queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT)
            && (queueData.qAddress.nodeAddress != 0)
            && (queueData.qServerAddress.nodeAddress != 0))
    {
        if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
        {
            goto retry;
        }

        if (syncCall)
        {
            rc = CL_MSG_RC(CL_ERR_INUSE);
            clLogError("QUE", "OPEN", "Persistent queue [%.*s] is in use. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            goto error_out_1;
        }
        else
        {
            retCode = CL_MSG_RC(CL_ERR_INUSE);
            clLogError("QUE", "OPEN", "Persistent queue [%.*s] is in use. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, 0, retCode);
            goto error_out_1;
        }
    }

    ClMsgOpenAsyncParamsT *pMsgOpenParams = NULL;
    if(syncCall == CL_FALSE)
    {
        pMsgOpenParams = (ClMsgOpenAsyncParamsT *) clHeapAllocate(sizeof(ClMsgOpenAsyncParamsT));
        pMsgOpenParams->msgHandle = msgHandle;
        pMsgOpenParams->invocation = invocation;
        pMsgOpenParams->pQueueName = clHeapCalloc(1, sizeof(*pMsgOpenParams->pQueueName));
        CL_ASSERT(pMsgOpenParams->pQueueName != NULL);
        memcpy(pMsgOpenParams->pQueueName, pQueueName, sizeof(*pMsgOpenParams->pQueueName));
        pMsgOpenParams->pCreationAttributes = (SaMsgQueueCreationAttributesT *)pCreationAttributes;
        pMsgOpenParams->openFlags = openFlags;
    }

    /* Open an existing msg queue located on this process */
    if ((qExists == CL_TRUE)
            && (queueData.qAddress.nodeAddress == gLocalAddress)
            && (queueData.qAddress.portId == gLocalPortId))
    {
        if (syncCall)
        {
            rc = clMsgQueueOpenLocal(msgHandle, invocation, pQueueName, pCreationAttributes, openFlags, timeout, pQueueHandle);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
        else
        {
            rc = clOsalTaskCreateDetached("MsgQueueOpenAsync", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                          clMsgQueueOpenLocalAsync, pMsgOpenParams);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
    }
    /* Open an existing msg queue located remotely on another process */
    else if ((qExists == CL_TRUE) 
            && (queueData.qAddress.nodeAddress != 0)
            && ((queueData.qAddress.nodeAddress != gLocalAddress)
            || (queueData.qAddress.portId != gLocalPortId)))
    {
        if (syncCall)
        {
            rc = clMsgQueueOpenRemote(msgHandle, invocation, pQueueName, pCreationAttributes, openFlags, timeout, pQueueHandle, queueData.qAddress, CL_TRUE);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
        else
        {
            pMsgOpenParams->qAddress = queueData.qAddress;
            pMsgOpenParams->qDelete = CL_TRUE;
            rc = clOsalTaskCreateDetached("MsgQueueOpenAsync", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                          clMsgQueueOpenRemoteAsync, pMsgOpenParams);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
    }
    /* Open an existing persistent msg queue located on the MSG server*/
    else if ((qExists == CL_TRUE) 
            && (queueData.qServerAddress.nodeAddress != 0))
    {
        if (syncCall)
        {
            rc = clMsgQueueOpenRemote(msgHandle, invocation, pQueueName, pCreationAttributes, openFlags, timeout, pQueueHandle, queueData.qServerAddress, CL_FALSE);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
        else
        {
            pMsgOpenParams->qAddress = queueData.qServerAddress;
            pMsgOpenParams->qDelete = CL_FALSE;
            rc = clOsalTaskCreateDetached("MsgQueueOpenAsync", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
            clMsgQueueOpenRemoteAsync, pMsgOpenParams);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
    }
    /* Try again if the queue is being moved
     * queueData.qAddress.nodeAddress == 0 && queueData.qServerAddress.nodeAddress != 0 
     */
    else if (qExists == CL_TRUE)
    {
        if (syncCall)
        {
            if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
            {
                goto retry;
            }

            rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
            clLogError("QUE", "OPEN", "Queue [%.*s] is being moved to another node. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            goto error_out_1;
        }
        else
        {
            retCode = CL_MSG_RC(CL_ERR_TRY_AGAIN);
            clLogError("QUE", "OPEN", "Queue [%.*s] is being moved to another node. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
            clMsgAppQueueOpenCallbackFunc(msgHandle, invocation, 0, retCode);
            goto error_out_1;
        }
    }
    /* Create a new queue */
    else
    {
        if (syncCall)
        {                
            rc = clMsgQueueOpenNew(msgHandle,invocation, pQueueName, pCreationAttributes, openFlags, timeout, pQueueHandle);
                    if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
        else
        {
            rc = clOsalTaskCreateDetached("MsgQueueOpenAsync", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                          clMsgQueueOpenNewAsync, pMsgOpenParams);
            if (CL_OK != CL_GET_ERROR_CODE(rc))
            {
                clLogError("QUE", "OPEN", "Fail to open queue [%.*s]. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
                goto error_out_1;
            }
        }
    }
 
    if(syncCall && pMsgLibInfo->callbacks.saMsgMessageReceivedCallback && *pQueueHandle)
    {
        clMsgDispatchQueueRegisterInternal(msgHandle, *pQueueHandle, 
                                           pMsgLibInfo->callbacks.saMsgMessageReceivedCallback);
    }

error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to checkin the message handle. error code [0x%x].",retCode);
        goto error_out;
    }

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueOpen(SaMsgHandleT msgHandle,
        const SaNameT *pQueueName,
        const SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueOpenFlagsT openFlags,
        SaTimeT timeout,
        SaMsgQueueHandleT *pQueueHandle)
{
    return clMsgQueueOpenInternal(CL_TRUE, msgHandle,0, pQueueName, pCreationAttributes, openFlags, timeout, pQueueHandle); 
}

SaAisErrorT saMsgQueueOpenAsync(SaMsgHandleT msgHandle,
        SaInvocationT invocation,
        const SaNameT *pQueueName,
        const SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueOpenFlagsT openFlags
        )
{
    return clMsgQueueOpenInternal(CL_FALSE, msgHandle, invocation, pQueueName, pCreationAttributes, openFlags, 0, NULL);
}

SaAisErrorT saMsgQueueClose(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc, retCode;
    ClMsgQueueInfoT *pQInfo;
    ClBoolT qDeleteFlag = CL_FALSE;
    ClBoolT qPersistencyFlag = CL_FALSE;
    ClBoolT qUnlinkFlag = CL_FALSE;
    ClNameT qName;
    ClMsgQueueCkptDataT queueData = {{0}};
    ClBoolT isExist = CL_FALSE;

    CL_MSG_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (ClPtrT *)&pQInfo);
    if(rc != CL_OK)
    {
        clLogError("QUE", "CLOS", "Failed at checkout the passed queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    clNameCopy(&qName, &pQInfo->pQueueEntry->qName);
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    isExist = clMsgQCkptExists((ClNameT *)&qName, &queueData);

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_HANDLE);
        clLogError("QUE", "CLOS", "Queue [%.*s] is already closed. error code [0x%x].",
                   pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, rc);
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        goto qHdl_chkin;
    }

    pQInfo->state = CL_MSG_QUEUE_CLOSED;
    pQInfo->closeTime = clOsalStopWatchTimeGet();

    if(pQInfo->unlinkFlag == CL_TRUE || (pQInfo->creationFlags == 0 && pQInfo->retentionTime == 0))
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        clMsgQueueFree(pQInfo);

        clLogDebug("QUE", "CLOS", "Unlinked/zero-retention-time queue is freed.");
        qDeleteFlag = CL_TRUE;
        qUnlinkFlag = CL_TRUE;

        goto qHdl_chkin;
    }
    else if(pQInfo->creationFlags == 0)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

        clLogDebug("QUE", "CLOS", "Starting [%.*s]'s retention timer of [%lld ns].", 
                pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, pQInfo->retentionTime);

        qPersistencyFlag = CL_TRUE;
        goto qHdl_chkin;
    }

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    /* Persistent queue */
    if ((isExist) 
            && (queueData.qAddress.nodeAddress == gLocalAddress)
            && (queueData.qAddress.portId == gLocalPortId))
    {
        queueData.qAddress.nodeAddress = 0;
        queueData.state = CL_MSG_QUEUE_CLOSED;
        rc = VDECL_VER(clMsgQDatabaseUpdateClientSync, 4, 0, 0)(gIdlUcastHandle, CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
            clLogError("QUE", "CLOS", "Failed to update the cached checkpoint. error code [0x%x].", rc);

        clMsgQueueFree(pQInfo);
        qDeleteFlag = CL_TRUE;
    }

qHdl_chkin:
    if(rc == CL_OK)
    {
        clMsgDispatchQueueDeregisterInternal(queueHandle);
    }

    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CLOS", "Failed to checkin the queue handle. error code [0x%x].", retCode);
error_out:
    if ((qPersistencyFlag)
            && (isExist)
            && (queueData.qAddress.nodeAddress == gLocalAddress)
            && (queueData.qAddress.portId == gLocalPortId))
    {
        /* Move message to server */
        queueData.qAddress.nodeAddress = 0;
        rc = VDECL_VER(clMsgQDatabaseUpdateClientSync, 4, 0, 0)(gIdlUcastHandle, CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to update the cached checkpoint. error code [0x%x].", rc);
            goto persistency_out;
        }

        clMsgToDestQueueMove(gLocalAddress, (ClNameT *)&qName);
        queueData.qAddress.nodeAddress = gLocalAddress;
        queueData.qAddress.portId = CL_IOC_MSG_PORT;
        queueData.state = CL_MSG_QUEUE_CLOSED;
        rc = VDECL_VER(clMsgQDatabaseUpdateClientSync, 4, 0, 0)(gIdlUcastHandle, CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to update the cached checkpoint. error code [0x%x].", rc);
            goto persistency_out;
        }

        rc = VDECL_VER(clMsgQueueRetentionCloseClientSync, 4, 0, 0)(gIdlUcastHandle, (ClNameT *)&qName);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to start queue retention timer. error code [0x%x].", rc);
            goto persistency_out;
        }
    }

persistency_out:

    if(qDeleteFlag == CL_TRUE)
    {
        retCode = clHandleDestroy(gClMsgQDatabase, queueHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "CLOS", "Failed to destroy the queue handle. error code [0x%x].", retCode);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

        /* Remove the message queue out of the database */
        CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
        clMsgQEntryDel((ClNameT *)&qName);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

        if(qUnlinkFlag)
            clMsgQueueUnlinkToServer((ClNameT *)&qName);
    }
    else
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    return CL_MSG_SA_RC(rc);
}

ClRcT VDECL_VER(clMsgQueueUnlink, 4, 0, 0)(ClNameT *pQName)
{
    ClRcT rc;
    ClRcT retCode;
    ClBoolT qDeleteFlag = CL_FALSE;
    ClMsgQueueRecordT *pQEntry;
    SaMsgQueueHandleT qHandle;
    ClMsgQueueInfoT *pQInfo;

    CL_MSG_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "UNL", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    qHandle = pQEntry->qHandle;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "UNL", "Failed to checkout queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        clMsgQueueFree(pQInfo);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

        clLogDebug("QUE", "UNL", "Closed queue [%.*s] is unlinked and freed.", pQName->length, pQName->value);
        qDeleteFlag = CL_TRUE;

    }
    else
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        pQInfo->unlinkFlag = CL_TRUE;
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    }

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "UNL", "Failed to checkin the queue handle. error code [0x%x].", retCode);

    if(qDeleteFlag == CL_TRUE)
    {
        retCode = clHandleDestroy(gClMsgQDatabase, qHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "UNL", "Failed to destroy the queue handle. error code [0x%x].", retCode);
    }

    /* Remove the message queue out of the database */
    clMsgQueueUnlinkToServer(pQName);

error_out:
    return rc;
}

SaAisErrorT saMsgQueueUnlink(SaMsgHandleT msgHandle, const SaNameT *pQueueName)
{
    ClRcT rc;

    CL_MSG_INIT_CHECK;

    if(pQueueName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QUL", "NULL queue name passed. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue in the cached checkpoint */
    ClMsgQueueCkptDataT queueData;
    if(clMsgQCkptExists((ClNameT *)pQueueName, &queueData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QUL", "Failed to get the message queue information. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue */
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    if (queueData.qAddress.nodeAddress != 0)
    {
        if((queueData.qAddress.nodeAddress == gLocalAddress)
            && (queueData.qAddress.portId == gLocalPortId))
        {
            rc = VDECL_VER(clMsgQueueUnlink, 4, 0, 0)((ClNameT *)pQueueName);
            if(rc != CL_OK)
            {
                clLogError("MSG", "QUL", "Failed to close the message queue. error code [0x%x].", rc);
            }
            goto out;
        }

        idlObj.address.address.iocAddress.iocPhyAddress = queueData.qAddress;
    }
    else if ((queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT) &&
            (queueData.qServerAddress.nodeAddress != 0))
    {
        idlObj.address.address.iocAddress.iocPhyAddress = queueData.qServerAddress;
    }
    else
    {
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("MSG", "QUL", "Queue [%.*s] is being moved to another node. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out;
    }

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QUL", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Send request to the process where the MSG queue is located */
    rc = VDECL_VER(clMsgQueueUnlinkClientSync, 4, 0, 0)(idlHandle, (ClNameT *)pQueueName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QUL", "Failed to close the message queue. error code [0x%x].", rc);
    }

    clIdlHandleFinalize(idlHandle);

error_out:
out:
    return CL_MSG_SA_RC(rc);
}

SaAisErrorT saMsgQueueStatusGet(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueName,
        SaMsgQueueStatusT *pQueueStatus
        )
{
    ClRcT rc = CL_OK;

    CL_MSG_INIT_CHECK;

    if(pQueueName == NULL || pQueueStatus == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QSG", "NULL parameter passed. error code [0x%x].",rc);
        goto error_out;
    }

    /* Look up msg queue in the cached checkpoint */
    ClMsgQueueCkptDataT queueData;
    if(clMsgQCkptExists((ClNameT *)pQueueName, &queueData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QSG", "Message queue [%.*s] does not exist in the cached checkpoint. error code [0x%x]."
                , pQueueName->length, pQueueName->value
                , rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue */
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    if (queueData.qAddress.nodeAddress != 0)
    {
        if((queueData.qAddress.nodeAddress == gLocalAddress)
            && (queueData.qAddress.portId == gLocalPortId))
        {
            rc = VDECL_VER(clMsgQueueStatusGet, 4, 0, 0)
                    ((ClNameT*)pQueueName, pQueueStatus);
            if(rc != CL_OK)
            {
                clLogError("MSG", "QSG", "Failed to get the message queue status from message server. error code [0x%x].", rc);
            }
            goto out;
        }
        idlObj.address.address.iocAddress.iocPhyAddress = queueData.qAddress;
    }
    else if ((queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT) &&
            (queueData.qServerAddress.nodeAddress != 0))
    {
        idlObj.address.address.iocAddress.iocPhyAddress = queueData.qServerAddress;
    }
    else
    {
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("MSG", "QSG", "Queue [%.*s] is being moved to another node. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out;
    }
    
    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QSG", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Send request to the process where the MSG queue is located */
    rc = VDECL_VER(clMsgQueueStatusGetClientSync, 4, 0, 0)
            (idlHandle, (ClNameT*)pQueueName, pQueueStatus);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QSG", "Failed to get the message queue status from message server. error code [0x%x].", rc);
    }

    clIdlHandleFinalize(idlHandle);

error_out:
out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueRetentionTimeSet(
        SaMsgQueueHandleT qHandle,
        SaTimeT *retentionTime)
{
    ClRcT rc;

    CL_MSG_INIT_CHECK;

    if(retentionTime == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "RET", "NULL passed for retention-time parameter. error code [0x%x].",rc);
        goto error_out;
    }
    
    rc = clMsgQueueRetentionTimeSet(qHandle, retentionTime);
    if(rc != CL_OK)
    {
        clLogError("MSG", "RET", "Failed to set the retention time. error code [0x%x].",rc);
    }

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgMessageCancel(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc;

    CL_MSG_INIT_CHECK;

    rc = clMsgMessageCancel(queueHandle);
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_DOESNT_EXIST)
    {
        clLogError("MSG", "CNCL", "Failed to unblock the blocking calls at the server. error code [0x%x].",rc);
    }

    return CL_MSG_SA_RC(rc);
}

ClRcT clMsgQueuePersistRedundancy(const SaNameT *queue, const SaNameT *node)
{
    ClRcT rc;
    ClIocAddressT iocAddress;
    ClNameT *pNodeName = (ClNameT *) node;

    CL_MSG_INIT_CHECK;

    /* Get IOC address for a node */
    rc = clCpmIocAddressForNodeGet(*pNodeName, &iocAddress);
    if (rc != CL_OK)
    {
        rc = CL_ERR_DOESNT_EXIST;
        clLogError("QUE", "REDUN", "Failed to get the IOC address for a node. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue in the cached checkpoint */
    ClMsgQueueCkptDataT queueData;
    if(clMsgQCkptExists((ClNameT *)queue, &queueData) == CL_FALSE)
    {
        rc = CL_ERR_DOESNT_EXIST;
        clLogError("MSG", "QSG", "Failed to get the message queue information. error code [0x%x].", rc);
        goto error_out;
    }

    if(queueData.creationFlags != SA_MSG_QUEUE_PERSISTENT)
    {
        rc = CL_ERR_INVALID_PARAMETER;
        clLogError("QUE", "REDUN", "Unable to designate the backup node for non-persistent queue.");
        goto error_out;
    }

    if(queueData.qServerAddress.nodeAddress == iocAddress.iocPhyAddress.nodeAddress)
    {
        rc = CL_OK;
        goto out;
    }

    /* Get Ioc address of the given MSG queue */
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = iocAddress.iocPhyAddress.nodeAddress;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QSG", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    if(queueData.qServerAddress.nodeAddress != 0)
        rc = VDECL_VER(clMsgQueuePersistRedundancyClientSync, 4, 0, 0)(idlHandle, (ClNameT*) queue, queueData.qServerAddress, CL_TRUE);
    else
        rc = VDECL_VER(clMsgQueuePersistRedundancyClientSync, 4, 0, 0)(idlHandle, (ClNameT*) queue, queueData.qAddress, CL_FALSE);

    if (rc != CL_OK)
    {
        clLogError("MSG", "REDUN", "Failed to designate the backup node. error code [0x%x].",rc);
    }

    clIdlHandleFinalize(idlHandle);

error_out:
out:
    return rc;
}
