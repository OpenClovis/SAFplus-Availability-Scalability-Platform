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

#include <msgIdlClientQueueCallsFromClientClient.h>



static void clMsgAppQueueOpenCallbackFunc(ClIdlHandleT idlHandle, ClHandleT msgHandle, ClNameT *pQName, 
        SaMsgQueueCreationAttributesT* pAttributes, SaMsgQueueOpenFlagsT  openFlags, 
        ClInt64T  timeout, ClHandleT* pQHandle, ClRcT rc, ClPtrT pCallbackParam)
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgAppQueueOpenCallbackParamsT *pParam = (ClMsgAppQueueOpenCallbackParamsT*)pCallbackParam;

    retCode = clHandleCheckout(gMsgHandleDatabase, pParam->msgHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOCb", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out;
    }

    ((ClMsgAppQueueOpenCallbackParamsT*)pCallbackParam)->rc = CL_MSG_SA_RC(rc);
    ((ClMsgAppQueueOpenCallbackParamsT*)pCallbackParam)->queueHandle = *pQHandle;

    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_QUEUE_OPEN_CALLBACK_TYPE, pCallbackParam);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOCb", "Failed to enqueue the callback into dispatcher. error code [0x%x].", retCode);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, pParam->msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOC", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    return;
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
    SaAisErrorT rc = CL_OK;
    ClRcT retCode;
    ClMsgAppQueueOpenCallbackParamsT *pParam;
    SaMsgQueueHandleT queueHandle;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    if((syncCall == CL_TRUE && pQueueHandle == NULL) || pQueueName == NULL || 
       ((openFlags & SA_MSG_QUEUE_CREATE) && (pCreationAttributes == NULL)))
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QOP", "NULL parameter passed. error code [0x%x].", rc);
        goto error_out;
    }

    if((openFlags != 0 && !(openFlags & (SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY | SA_MSG_QUEUE_RECEIVE_CALLBACK))) ||
       (pCreationAttributes != NULL && pCreationAttributes->creationFlags != SA_MSG_QUEUE_PERSISTENT && pCreationAttributes->creationFlags != 0))
    {
        rc = CL_MSG_RC(CL_ERR_BAD_FLAG);
        clLogError("MSG", "QOP", "Invalid open flags passed. error code [0x%x].", rc);
        goto error_out;
    }

    if( !(openFlags & SA_MSG_QUEUE_CREATE) && (pCreationAttributes != NULL))
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QOP", "Creation attributes should be NULL, if an existing queue is to be opened.");
        clLogError("MSG", "QOP", "Or, the SA_MSG_QUEUE_CREATE openflag must be passed to create a queue. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QOP", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    if(syncCall == CL_FALSE && pMsgLibInfo->callbacks.saMsgQueueOpenCallback == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogError("MSG", "QOP", "The message-queue-open callback is not registered. error code [0x%x].", rc);
        goto error_out_1;
    }

    if(openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK && pMsgLibInfo->callbacks.saMsgMessageReceivedCallback == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogError("MSG", "QOP", "The message received callback is not registered. error code [0x%x].", rc);
        goto error_out_1;
    }


    if(syncCall == CL_TRUE)
    {
        ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };
        ClInt32T tries = 0;
        do
        {
            rc = VDECL_VER(clMsgClientQueueOpenClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pQueueName, 
                                                                    (SaMsgQueueCreationAttributesT *)pCreationAttributes, openFlags, timeout, &queueHandle);
        } while(
                (CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE ||
                CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN)
                && 
                ++tries < 5
                && 
                clOsalTaskDelay(delay) == CL_OK);

        if(rc != CL_OK)
        {
            clLogError("MSG", "QOP", "Failed to register queue with server. error code [0x%x].", rc);
            goto error_out_1;
        }
        *pQueueHandle = queueHandle;
    }
    else
    {
        pParam = (ClMsgAppQueueOpenCallbackParamsT*)clHeapAllocate(sizeof(ClMsgAppQueueOpenCallbackParamsT));
        if(pParam == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogError("MSG", "QOP", "Failed to allocate memory for %zd bytes. error code [0x%x].", sizeof(ClMsgAppQueueOpenCallbackParamsT), rc);
            goto error_out_1;
        }

        pParam->msgHandle = msgHandle;
        pParam->invocation = invocation;

        rc = VDECL_VER(clMsgClientQueueOpenClientAsync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, 
                                                                 (ClNameT*)pQueueName, (SaMsgQueueCreationAttributesT *)pCreationAttributes,
                                                                 openFlags, 0, NULL, &clMsgAppQueueOpenCallbackFunc, (ClPtrT)pParam );
        if(rc != CL_OK)
        {
            clLogError("MSG", "QOP", "Failed to open the queue at server. error code [0x%x].", rc);
        }
    }

    error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QOP", "Failed to checkin the message handle. error code [0x%x].",retCode);
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
    ClRcT rc = CL_MSG_RC(CL_ERR_INVALID_HANDLE);

    CL_MSG_CLIENT_INIT_CHECK;

    rc = VDECL_VER(clMsgClientQueueCloseClientSync, 4, 0, 0)(gClMsgIdlHandle, queueHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QCL", "Failed to close the message queue at message server. error code [0x%x].", rc);
    }
    /*
     * deregister from the dispatch queue table.
     */
    clMsgDispatchQueueDeregister(queueHandle);

    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueUnlink(SaMsgHandleT msgHandle, const SaNameT *pQueueName)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pQueueName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QUL", "NULL queue name passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QUL", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = VDECL_VER(clMsgClientQueueUnlinkClientSync, 4, 0, 0)(gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT *)pQueueName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QUL", "Failed to close the message queue at message server. error code [0x%x].", rc);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QSG", "Failed to checkin the message handle. error code [0x%x].", retCode);

error_out:
    return CL_MSG_SA_RC(rc);
}

SaAisErrorT saMsgQueueStatusGet(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueName,
        SaMsgQueueStatusT *pQueueStatus
        )
{
    ClRcT rc = CL_OK;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClTimerTimeOutT delay = CL_MSG_DEFAULT_DELAY;
    ClInt32T tries = 0;

    CL_MSG_CLIENT_INIT_CHECK;

    if(pQueueName == NULL || pQueueStatus == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QSG", "NULL parameter passed. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QSG", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    do
    {
        rc = VDECL_VER(clMsgClientQueueStatusGetClientSync, 4, 0, 0)
            (gClMsgIdlHandle, pMsgLibInfo->handle, (ClNameT*)pQueueName, pQueueStatus);
    }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
           &&
           ++tries < CL_MSG_DEFAULT_RETRIES
           &&
           clOsalTaskDelay(delay) == CL_OK);

    if(rc != CL_OK)
    {
        clLogError("MSG", "QSG", "Failed to get the message queue status from message server. error code [0x%x].", rc);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QSG", "Failed to checkin the message handle. error code [0x%x].", retCode);

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueRetentionTimeSet(
        SaMsgQueueHandleT qHandle,
        SaTimeT *retentionTime)
{
    ClRcT rc;

    CL_MSG_CLIENT_INIT_CHECK;

    if(retentionTime == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "RET", "NULL passed for retention-time parameter. error code [0x%x].",rc);
        goto error_out;
    }
    
    rc = VDECL_VER(clMsgClientQueueRetentionTimeSetClientSync, 4, 0, 0)(gClMsgIdlHandle, qHandle, retentionTime);
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

    CL_MSG_CLIENT_INIT_CHECK;

    rc = VDECL_VER(clMsgClientMessageCancelClientSync, 4, 0, 0)(gClMsgIdlHandle, queueHandle);
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_DOESNT_EXIST)
    {
        clLogError("MSG", "CNCL", "Failed to unblock the blocking calls at the server. error code [0x%x].",rc);
    }

    return CL_MSG_SA_RC(rc);
}

