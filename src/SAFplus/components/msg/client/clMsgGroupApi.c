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
#include <clIdlApi.h>
#include <clIocApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clDispatchApi.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgApi.h>
#include <clMsgGroupApi.h>
#include <clCpmApi.h>
#include <clVersion.h>
#include <clMsgIdl.h>
#include <clMsgCkptClient.h>

#include <msgIdlClientGroupCallsFromClientClient.h>
#include <msgCltClientCallsFromServerToClientClient.h>

SaAisErrorT saMsgQueueGroupCreate(SaMsgHandleT msgHandle,
        const SaNameT *pQueueGroupName,
        SaMsgQueueGroupPolicyT groupPolicy)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgQGroupCkptDataT qGroupData;

    CL_MSG_INIT_CHECK;

    if(pQueueGroupName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGC", "NULL pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    if (pQueueGroupName->length > CL_MAX_NAME_LENGTH)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGC", "Invalid ClNameT structure. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptExists((ClNameT*)pQueueGroupName, &qGroupData) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogWarning("MSG", "QGC", "Message queue group [%.*s] already exists. error code [0x%x]."
                        , pQueueGroupName->length, pQueueGroupName->value
                        , rc);
        goto error_out;
    }

    if(groupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN && groupPolicy != SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN && 
            groupPolicy != SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE && groupPolicy != SA_MSG_QUEUE_GROUP_BROADCAST)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGC", "Invalid group policy passed. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGC", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = VDECL_VER(clMsgQueueGroupCreateClientSync, 4, 0, 0)(gIdlUcastHandle, (ClNameT*)pQueueGroupName, groupPolicy);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGC", "Failed to create message queue group [%.*s]. error code [0x%x].", pQueueGroupName->length, pQueueGroupName->value, rc);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGC", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueGroupInsert(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueGroupName,
        const SaNameT *pQueueName
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgQueueCkptDataT queueData = {{0}};
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlQGroupHandle = 0;
    ClUint32T pos;

    CL_MSG_INIT_CHECK;

    if(pQueueGroupName == NULL || pQueueName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGI", "NULL pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptDataGet((ClNameT*)pQueueGroupName, &qGroupData) != CL_OK)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGI", "Message queue group [%.*s] does not exists. error code [0x%x]."
                        , pQueueGroupName->length, pQueueGroupName->value
                        , rc);
        goto error_out;
    }

    if (clMsgQGroupCkptQueueExist(&qGroupData, (ClNameT *)pQueueName, &pos) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogError("MSG", "QGI", "Message queue [%.*s] already exists in message queue group [%.*s]. error code [0x%x]."
                            , pQueueName->length, pQueueName->value
                            , pQueueGroupName->length, pQueueGroupName->value
                            , rc);
        goto error_out;
    }

    clMsgQGroupCkptDataFree(&qGroupData);

    /* Look up msg queue in the cached checkpoint */
    if(clMsgQCkptExists((ClNameT *)pQueueName, &queueData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGI", "Failed to get the message queue information from the cached ckpt. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGI", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue group*/
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    rc = clIdlHandleInitialize(&idlObj, &idlQGroupHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGI", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = VDECL_VER(clMsgQueueGroupInsertClientSync, 4, 0, 0)(idlQGroupHandle, (ClNameT*)pQueueGroupName, (ClNameT*)pQueueName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGR", "Failed to add message queue [%.*s] to message queue group [%.*s]. error code [0x%x]."
                                 , pQueueName->length, pQueueName->value
                                 , pQueueGroupName->length, pQueueGroupName->value
                                 , rc);
        goto error_out_2;
    }

error_out_2:
    clIdlHandleFinalize(idlQGroupHandle);
error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGI", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }
error_out:
    return CL_MSG_SA_RC(rc);
}

SaAisErrorT saMsgQueueGroupRemove(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueGroupName,
        const SaNameT *pQueueName
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgQueueCkptDataT queueData = {{0}};
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlQGroupHandle = 0;
    ClUint32T pos;

    CL_MSG_INIT_CHECK;

    if(pQueueGroupName == NULL || pQueueName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGR", "NULL pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptDataGet((ClNameT*)pQueueGroupName, &qGroupData) != CL_OK)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGR", "Message queue group [%.*s] does not exist. error code [0x%x]."
                        , pQueueGroupName->length, pQueueGroupName->value
                        , rc);
        goto error_out;
    }

    if (clMsgQGroupCkptQueueExist(&qGroupData, (ClNameT *)pQueueName, &pos) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGR", "Message queue [%.*s] does not exist in message queue group [%.*s]. error code [0x%x]."
                            , pQueueName->length, pQueueName->value
                            , pQueueGroupName->length, pQueueGroupName->value
                            , rc);
        goto error_out;
    }

    clMsgQGroupCkptDataFree(&qGroupData);

    /* Look up msg queue in the cached checkpoint */
    if(clMsgQCkptExists((ClNameT *)pQueueName, &queueData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGR", "Failed to get the message queue information from the cached ckpt. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGR", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue group*/
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    rc = clIdlHandleInitialize(&idlObj, &idlQGroupHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGR", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = VDECL_VER(clMsgQueueGroupRemoveClientSync, 4, 0, 0)(idlQGroupHandle, (ClNameT*)pQueueGroupName, (ClNameT*)pQueueName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGR", "Failed to remove message queue [%.*s] from message queue group [%.*s]. error code [0x%x]."
                                 , pQueueName->length, pQueueName->value
                                 , pQueueGroupName->length, pQueueGroupName->value
                                 , rc);
        goto error_out_2;
    }

error_out_2:
    clIdlHandleFinalize(idlQGroupHandle);
error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGR", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }
error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueGroupDelete(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueGroupName
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgQGroupCkptDataT qGroupData;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlQGroupHandle = 0;

    CL_MSG_INIT_CHECK;

    if(pQueueGroupName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "QGD", "NULL pointer passed as parameter. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptExists((ClNameT*)pQueueGroupName, &qGroupData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGD", "Message queue group [%.*s] does not exist. error code [0x%x]."
                        , pQueueGroupName->length, pQueueGroupName->value
                        , rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGD", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue group*/
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    rc = clIdlHandleInitialize(&idlObj, &idlQGroupHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGD", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = VDECL_VER(clMsgQueueGroupDeleteClientSync, 4, 0, 0)(idlQGroupHandle, (ClNameT*)pQueueGroupName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGD", "Failed to delete [%.*s] queue group. error code [0x%x].", pQueueGroupName->length, pQueueGroupName->value, rc);
        goto error_out_2;
    }

error_out_2:
    clIdlHandleFinalize(idlQGroupHandle);
error_out_1:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGD", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }
error_out:
    return CL_MSG_SA_RC(rc);
}


static void clMsgQGroupTrackCallbackInternal(SaMsgHandleT clientHandle, ClMsgAppQGroupTrackCallbakParamsT *pParam) 
{
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    retCode = clHandleCheckout(gMsgHandleDatabase, clientHandle, (void**)&pMsgLibInfo);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGT", "Failed to checkout the message handle. error code [0x%x].", retCode);
        goto error_out;
    }

    retCode = clDispatchCbEnqueue(pMsgLibInfo->dispatchHandle, CL_MSG_QUEUE_GROUP_TRACK_CALLBACK_TYPE, pParam); 

    retCode = clHandleCheckin(gMsgHandleDatabase, clientHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGT", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }

error_out:
    return;
}


ClRcT VDECL_VER(clMsgClientsTrackCallback, 4, 0, 0)(SaMsgHandleT clientHandle, ClNameT *pGroupName, SaMsgQueueGroupNotificationBufferT *pData)
{
    ClRcT rc = CL_OK;
    ClMsgAppQGroupTrackCallbakParamsT *pParam;
    ClUint32T dataSize;

    clLogDebug("MSG", "TCb", "Client Handle [0x%llx], Group [%.*s], pData [%p]", clientHandle, pGroupName->length, pGroupName->value, (void*)pData);
    pParam = (ClMsgAppQGroupTrackCallbakParamsT*)clHeapAllocate(sizeof(ClMsgAppQGroupTrackCallbakParamsT));
    if(pParam == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "TCb", "Failed to allocate %zd bytes of memory. error code [0x%x].", sizeof(ClMsgAppQGroupTrackCallbakParamsT), rc);
        goto error_out;
    }

    pParam->pNotificationBuffer = (SaMsgQueueGroupNotificationBufferT*)clHeapAllocate(sizeof(SaMsgQueueGroupNotificationBufferT));
    if(pParam->pNotificationBuffer == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "TCb", "Failed to allocate %zd bytes of memory. error code [0x%x].", sizeof(SaMsgQueueGroupNotificationBufferT), rc);
        goto error_out_1;
    }

    dataSize = pData->numberOfItems * sizeof(SaMsgQueueGroupNotificationT);
    pParam->pNotificationBuffer->notification = (SaMsgQueueGroupNotificationT*)clHeapAllocate(dataSize);
    if(pParam->pNotificationBuffer->notification == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "TCb", "Failed to allocate %u bytes of memory. error code [0x%x].", dataSize, rc);
        goto error_out_2;
    }

    pParam->pNotificationBuffer->numberOfItems = pData->numberOfItems;
    pParam->pNotificationBuffer->queueGroupPolicy = pData->queueGroupPolicy;
    memcpy(pParam->pNotificationBuffer->notification, pData->notification, dataSize);
    memcpy(&pParam->groupName, pGroupName, sizeof(SaNameT));
    pParam->rc = CL_MSG_SA_RC(CL_OK);

    clMsgQGroupTrackCallbackInternal(clientHandle, pParam);

    goto out;
error_out_2:
    clHeapFree(pParam->pNotificationBuffer);
error_out_1:
    clHeapFree(pParam);
error_out:
out:
    return rc;
}


static void clMsgQGroupTrackCallback(ClIdlHandleT idlHandle, SaMsgHandleT clientHandle, SaMsgHandleT msgHandle, ClNameT *pQueueGroupName, ClUint8T trackFlags, SaMsgQueueGroupNotificationBufferT *pNotificationBuffer, ClRcT rc, ClPtrT pCallbackParam)
{
    ClMsgAppQGroupTrackCallbakParamsT *pParam;

    pParam = (ClMsgAppQGroupTrackCallbakParamsT*)clHeapAllocate(sizeof(ClMsgAppQGroupTrackCallbakParamsT));
    if(pParam == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "TCb", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(ClMsgAppQGroupTrackCallbakParamsT), rc);
        goto error_out;
    }

    memcpy(&pParam->groupName, pQueueGroupName, sizeof(pParam->groupName));
    pParam->rc = CL_MSG_SA_RC(rc);
    pParam->pNotificationBuffer = clHeapCalloc(1, sizeof(*pParam->pNotificationBuffer));
    if(pParam->pNotificationBuffer == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "TCb", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(*pParam->pNotificationBuffer), rc);
        goto error_out_1;
    }
    memcpy(pParam->pNotificationBuffer, pNotificationBuffer, sizeof(*pParam->pNotificationBuffer));

    clMsgQGroupTrackCallbackInternal(clientHandle, pParam);

    goto out;

error_out_1:
    clHeapFree(pParam);
error_out:
out:
    return;
}


SaAisErrorT saMsgQueueGroupTrack(
        SaMsgHandleT msgHandle,
        const SaNameT *pQueueGroupName,
        SaUint8T trackFlags,
        SaMsgQueueGroupNotificationBufferT *pNotificationBuffer
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    SaMsgQueueGroupNotificationBufferT *pTempNotif;
    ClMsgQGroupCkptDataT qGroupData;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlQGroupHandle = 0;

    CL_MSG_INIT_CHECK;

    if(pQueueGroupName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QGT", "NULL passed for Group Name. error code [0x%x].", rc);
        goto error_out;
    }

    if(!(trackFlags & (SA_TRACK_CURRENT | SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY)) || 
            ((trackFlags & SA_TRACK_CHANGES) && (trackFlags & SA_TRACK_CHANGES_ONLY)))
    {
        rc = CL_MSG_RC(CL_ERR_BAD_FLAG);
        clLogError("MSG", "QGT", "Invalid track flags passed. error code [0x%x].", rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptExists((ClNameT*)pQueueGroupName, &qGroupData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGT", "Failed to get the message queue group information from the cached ckpt. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue group*/
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    rc = clIdlHandleInitialize(&idlObj, &idlQGroupHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGT", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGT", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    if((trackFlags & SA_TRACK_CHANGES || trackFlags & SA_TRACK_CHANGES_ONLY) && pMsgLibInfo->callbacks.saMsgQueueGroupTrackCallback == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogError("MSG", "QGT", "Invalid track flags passed. error code [0x%x].", rc);
        goto error_out_2;
    }

    if(pNotificationBuffer == NULL && pMsgLibInfo->callbacks.saMsgQueueGroupTrackCallback == NULL) 
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("MSG", "QGT", "NULL pointer passed as paramter and callback is also NULL. "
                "So failed to set the track the message queue group. error code [0x%x].",rc);
        goto error_out_2;
    }


    pTempNotif = (SaMsgQueueGroupNotificationBufferT*)clHeapAllocate(sizeof(SaMsgQueueGroupNotificationBufferT));
    if(pTempNotif == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "QGT", "Failed to allocate %zd bytes. error code [0x%x].",sizeof(SaMsgQueueGroupNotificationBufferT), rc);
        goto error_out_2;
    }

    /* This is for IDL to free, as it frees an CL_INOUT parameter for new allocation */
    pTempNotif->notification = (SaMsgQueueGroupNotificationT*)clHeapAllocate(1);
    if(pTempNotif->notification == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("MSG", "QGT", "Failed to allocate 1 bytes. error code [0x%x].", rc);
        goto error_out_3;
    }

    pTempNotif->numberOfItems = 0;

    if((trackFlags & SA_TRACK_CURRENT) && pNotificationBuffer != NULL)
    {
        if(pNotificationBuffer->notification != NULL)
        {
            pTempNotif->numberOfItems = pNotificationBuffer->numberOfItems;
            pTempNotif->notification = clHeapRealloc(pTempNotif->notification, 
                                                     sizeof(*pTempNotif->notification) * pNotificationBuffer->numberOfItems);
            if(pTempNotif->notification == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
                clLogError("MSG", "QGT", "Failed to re-allocate [%zd] bytes. error code [0x%x].", 
                        (sizeof(*pTempNotif->notification) * pNotificationBuffer->numberOfItems), rc);
                goto error_out_3;
            }
        }

        rc = VDECL_VER(clMsgQueueGroupTrackClientSync, 4, 0, 0)(idlQGroupHandle, msgHandle, 
                pMsgLibInfo->handle, (ClNameT*)pQueueGroupName, trackFlags, pTempNotif);
        if(rc != CL_OK)
        {
            /*Here I am not freeing the memory of pTempNotif->notification as it would have been freed by IDL */
            clLogError("MSG", "QGT", "Failed to get/set the Group Tracking. error code [0x%x].", rc);
            goto error_out_3;
        }
        if(pNotificationBuffer->numberOfItems != 0 && pNotificationBuffer->numberOfItems < pTempNotif->numberOfItems)
        {
            /* Note : This is error case. But server side returned CL_OK because we want the
             * pTempNotif->numerOfItems value put by the server. The RMD call in IDL will not
             * copy the value passed by the server if the server returns error there. */
            pNotificationBuffer->numberOfItems = pTempNotif->numberOfItems;
            rc = CL_MSG_RC(CL_ERR_NO_SPACE);
            goto error_out_4;
        }
        pNotificationBuffer->numberOfItems = pTempNotif->numberOfItems;
        pNotificationBuffer->queueGroupPolicy = pTempNotif->queueGroupPolicy;
        if(pNotificationBuffer->notification == NULL)
        {
            pNotificationBuffer->notification = pTempNotif->notification;
            goto error_out_3;
        }
        else
        {
            memcpy(pNotificationBuffer->notification, pTempNotif->notification, pTempNotif->numberOfItems * sizeof(SaMsgQueueGroupNotificationT));
            goto sync_success_out;
        }
    }
    else 
    {
        if(pMsgLibInfo->callbacks.saMsgQueueGroupTrackCallback == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
            clLogError("MSG", "QGT", "The message track callback is not registered. error code [0x%x].", rc);
            goto error_out_4;
        }

        rc = VDECL_VER(clMsgQueueGroupTrackClientAsync, 4, 0, 0)(idlQGroupHandle, msgHandle, pMsgLibInfo->handle, (ClNameT*)pQueueGroupName, trackFlags, pTempNotif, clMsgQGroupTrackCallback, NULL);
        if(rc != CL_OK)
        {
            /* pTempNotif->notification is not freed, as it would have been freed by IDL */
            clLogError("MSG", "QGT", "Failed to get/set the Group Tracking. error code [0x%x].", rc);
        }
        goto error_out_3;
    }


error_out_4:
sync_success_out:
    if(pTempNotif->notification)
        clHeapFree(pTempNotif->notification);
error_out_3:
    if(pTempNotif)
        clHeapFree(pTempNotif);
error_out_2:
    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
    {
        clLogError("MSG", "QGT", "Failed to checkin the message handle. error code [0x%x].",retCode);
    }
error_out_1:
    clIdlHandleFinalize(idlQGroupHandle);
error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT
saMsgQueueGroupTrackStop (
	SaMsgHandleT msgHandle,
	const SaNameT *pGroupName)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgLibInfoT *pMsgLibInfo = NULL;
    ClMsgQGroupCkptDataT qGroupData;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlQGroupHandle = 0;

    CL_MSG_INIT_CHECK;

    if(pGroupName == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "GTS", "NULL Queue Group Name passed. error code [0x%x].",rc);
        goto error_out;
    }

    /* Look up msg queue group in the cached checkpoint */
    if(clMsgQGroupCkptExists((ClNameT*)pGroupName, &qGroupData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "QGS", "Failed to get the message queue group information from the cached ckpt. error code [0x%x].", rc);
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue group*/
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    rc = clIdlHandleInitialize(&idlObj, &idlQGroupHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "QGS", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "GTS", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = VDECL_VER(clMsgQueueGroupTrackStopClientSync, 4, 0, 0)(idlQGroupHandle, pMsgLibInfo->handle, (ClNameT *)pGroupName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "GTS", "Failed to stop the tracking at the server. error code [0x%x].",rc);
    }

    retCode = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "GTS", "Failed to checkin the message handle. error code [0x%x].", retCode);

error_out_1:
    clIdlHandleFinalize(idlQGroupHandle);
error_out:
    return CL_MSG_SA_RC(rc);
}


SaAisErrorT saMsgQueueGroupNotificationFree(
        SaMsgHandleT msgHandle,
        SaMsgQueueGroupNotificationT *pNotification
        )
{
    ClRcT rc;
    ClMsgLibInfoT *pMsgLibInfo = NULL;

    CL_MSG_INIT_CHECK;

    rc = clHandleCheckout(gMsgHandleDatabase, msgHandle, (void**)&pMsgLibInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "NF", "Failed to checkout the message handle. error code [0x%x].", rc);
        goto error_out;
    }

    clHeapFree(pNotification);

    rc = clHandleCheckin(gMsgHandleDatabase, msgHandle);
    if(rc != CL_OK)
        clLogError("MSG", "NF", "Failed to checkin the message handle. error code [0x%x].", rc);

error_out:
    return CL_MSG_SA_RC(rc);
}

