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
 * ModuleName  : msg                                                          
 * File        : clMsgCkptServer.c
 *******************************************************************************/
#include <sys/mman.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clMsgIdl.h>
#include <clMsgCkptServer.h>
#include <clMsgGroupDatabase.h>
#include <clMsgQueue.h>
#include <clMsgDebugInternal.h>

#include <msgIdlClientCallsFromServerClient.h>

/* 
 * In SAI-AIS-MSG-B.03.01.pdf section 3.1.2, 
 * "Message queues and message queue groups have distinct name spaces" 
 */
ClCachedCkptSvcInfoT gMsgQCkptServer;
ClCachedCkptSvcInfoT gMsgQGroupCkptServer;

ClRcT clMsgQCkptInitialize(void)
{
    ClRcT rc = CL_OK; 
    ClRcT retCode;

    /* Initialize MSG queue cached ckpt */
    const ClNameT msgQueueCkptName  = {
                     sizeof("CL_MsgQueueCkpt") - 1,
                     "CL_MsgQueueCkpt"
                    };

    SaCkptCheckpointOpenFlagsT	msgQueueOpenFlags = SA_CKPT_CHECKPOINT_CREATE | SA_CKPT_CHECKPOINT_READ | SA_CKPT_CHECKPOINT_WRITE;
    SaCkptCheckpointCreationAttributesT msgQueueAttributes =
    {
        SA_CKPT_WR_ALL_REPLICAS | CL_CKPT_DISTRIBUTED,
        CL_MSG_QUEUE_CKPT_SIZE,
        CL_MSG_QUEUE_RETENTION_DURATION,
        CL_MSG_QUEUE_MAX_SECTIONS,
        CL_MSG_QUEUE_MAX_SECTION_SIZE,
        CL_MSG_QUEUE_MAX_SECTION_ID_SIZE
    };

    gMsgQCkptServer.ckptSvcHandle = CL_HANDLE_INVALID_VALUE;
    gMsgQCkptServer.ckptHandle = CL_HANDLE_INVALID_VALUE;

    rc = clCachedCkptInitialize(&gMsgQCkptServer, (SaNameT *)&msgQueueCkptName, &msgQueueAttributes, msgQueueOpenFlags, CL_MSG_QUEUE_CKPT_SIZE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoint server service for MSG queue: error code [0x%x].", rc);
        goto out;
    }

    /* Initialize MSG group queue cached ckpt */
    const ClNameT msgQGroupCkptName  = {
                     sizeof("CL_MsgQueueGroupCkpt") - 1,
                     "CL_MsgQueueGroupCkpt"
                    };

    SaCkptCheckpointOpenFlagsT	msgQGroupOpenFlags = SA_CKPT_CHECKPOINT_CREATE | SA_CKPT_CHECKPOINT_READ | SA_CKPT_CHECKPOINT_WRITE;
    SaCkptCheckpointCreationAttributesT msgQGroupAttributes =
    {
        SA_CKPT_WR_ALL_REPLICAS | CL_CKPT_DISTRIBUTED,
        CL_MSG_QGROUP_CKPT_SIZE,
        CL_MSG_QGROUP_RETENTION_DURATION,
        CL_MSG_QGROUP_MAX_SECTIONS,
        CL_MSG_QGROUP_MAX_SECTION_SIZE,
        CL_MSG_QGROUP_MAX_SECTION_ID_SIZE
    };

    gMsgQGroupCkptServer.ckptSvcHandle = CL_HANDLE_INVALID_VALUE;
    gMsgQGroupCkptServer.ckptHandle = CL_HANDLE_INVALID_VALUE;

    rc = clCachedCkptInitialize(&gMsgQGroupCkptServer, (SaNameT *)&msgQGroupCkptName, &msgQGroupAttributes, msgQGroupOpenFlags, CL_MSG_QGROUP_CKPT_SIZE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoint server service for MSG queue group: error code [0x%x].", rc);
        goto error_out1;
    }

    goto out;
error_out1:
    retCode = clCachedCkptFinalize(&gMsgQCkptServer); 
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize the CachedCkpt for MSG queue group. error code [0x%x].", retCode);
out:
    return rc;
}

ClRcT clMsgQCkptFinalize(void)
{
    ClRcT rc = CL_OK; 

    rc = clCachedCkptFinalize(&gMsgQCkptServer);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize the CachedCkpt for MSG queue. error code [0x%x].", rc);

    rc = clCachedCkptFinalize(&gMsgQGroupCkptServer);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize the CachedCkpt for MSG queue group. error code [0x%x].", rc);
    return rc;
}

static void clMsgGroupDatabaseSynch()
{
    ClCachedCkptDataT *sectionData = NULL;
    ClUint32T        sectionOffset = 0;

    clCachedCkptSectionGetFirst(&gMsgQGroupCkptServer, &sectionData, &sectionOffset);
    while (sectionData != NULL)
    {
        ClMsgQGroupCkptDataT qGroupData;
        clMsgQGroupCkptDataUnmarshal(&qGroupData, sectionData);
        clMsgGroupInfoUpdate(CL_MSG_DATA_ADD, &qGroupData.qGroupName, qGroupData.policy);

        for (int i = 0; i < qGroupData.numberOfQueues; i++)
        {
            ClNameT *qName = (ClNameT *) (qGroupData.pQueueList + i);
            clMsgGroupMembershipInfoSend(CL_MSG_DATA_ADD, &qGroupData.qGroupName, qName);
        }

        clCachedCkptSectionGetNext(&gMsgQGroupCkptServer, &sectionData, &sectionOffset);
    }
}

ClRcT clMsgQCkptSynch(void)
{
    ClRcT rc = CL_OK;

    rc = clCachedCkptSynch(&gMsgQCkptServer, CL_TRUE);
    if (rc != CL_OK)
    {
        clLogError("MSG", "SYNNC", "Failed to synchronize the MSG queue cached checkpoint. error code [0x%x].", rc);
        goto error;
    }

    rc = clCachedCkptSynch(&gMsgQGroupCkptServer, CL_TRUE);
    if (rc != CL_OK)
    {
        clLogError("MSG", "SYNNC", "Failed to synchronize the MSG queue group cached checkpoint. error code [0x%x].", rc);
        goto error;
    }

    clMsgGroupDatabaseSynch();

error:
    return rc;
}

ClBoolT clMsgQCkptExists(const ClNameT *pQName, ClMsgQueueCkptDataT *pQueueData)
{
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQCkptServer.cacheSem);
    clCachedCkptSectionRead(&gMsgQCkptServer, pQName, &sectionData);
    if (sectionData == NULL)
    {
        clOsalSemUnlock(gMsgQCkptServer.cacheSem);
        return CL_FALSE;
    }

    clMsgQueueCkptDataUnmarshal(pQueueData, sectionData);

    clOsalSemUnlock(gMsgQCkptServer.cacheSem);

    return CL_TRUE;

}

ClBoolT clMsgQGroupCkptExists(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData)
{
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);
    clCachedCkptSectionRead(&gMsgQGroupCkptServer, pQGroupName, &sectionData);
    if (sectionData == NULL)
    {
        clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
        return CL_FALSE;
    }

    clMsgQGroupCkptHeaderUnmarshal(pQGroupData, sectionData);

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);

    return CL_TRUE;
}

ClRcT clMsgQGroupCkptDataGet(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData)
{
    ClRcT rc = CL_OK;
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);
    clCachedCkptSectionRead(&gMsgQGroupCkptServer, pQGroupName, &sectionData);
    if (sectionData == NULL)
    {
        rc = CL_ERR_DOESNT_EXIST;
        clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
        return rc;
    }

    rc = clMsgQGroupCkptDataUnmarshal(pQGroupData, sectionData);

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);

    return rc;
}

static ClRcT clMsgQGroupCkptDataGet_Nolock(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData)
{
    ClRcT rc = CL_OK;
    ClCachedCkptDataT *sectionData = NULL;

    clCachedCkptSectionRead(&gMsgQGroupCkptServer, pQGroupName, &sectionData);
    if (sectionData == NULL)
    {
        rc = CL_ERR_DOESNT_EXIST;
        return rc;
    }

    rc = clMsgQGroupCkptDataUnmarshal(pQGroupData, sectionData);

    return rc;
}

static void clMsgQueueGroupsRemove(ClNameT *pQName)
{
    ClRcT			rc = CL_OK;
    ClUint32T                   i = 0;
    ClUint32T                   pos;
    ClMsgQGroupCkptDataT        qGroupData;
    ClCachedCkptDataT           ckptData;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);

    ClUint32T        *numberOfSections = (ClUint32T *) gMsgQGroupCkptServer.cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;

    while(i < *numberOfSections)
    {
        ClBoolT isUpdate = CL_FALSE;
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);
        pTemp->data = (ClUint8T *)(pTemp + 1);

        memset(&qGroupData, 0, sizeof(qGroupData));
        memset(&ckptData, 0, sizeof(ckptData));

        rc = clMsgQGroupCkptDataUnmarshal(&qGroupData, pTemp);
        if (rc != CL_OK)
        {
            clLogError("MSG", "QGsR", "clMsgQGroupCkptDataUnmarshal(): error code [0x%x].", rc);
            clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
            return;
        }

        if (clMsgQGroupCkptQueueExist(&qGroupData, pQName, &pos) == CL_TRUE)
        {
            qGroupData.numberOfQueues--;
            if (qGroupData.numberOfQueues == 0)
            {
                clHeapFree(qGroupData.pQueueList);
                qGroupData.pQueueList = NULL;
            }
            else
            {
                ClNameT *pNameTemp = (ClNameT *) clHeapAllocate(qGroupData.numberOfQueues * sizeof(ClNameT));
                memcpy(pNameTemp, qGroupData.pQueueList, pos * sizeof(ClNameT));

                ClNameT *pNameDes = pNameTemp + pos;
                ClNameT *pNameSrc = qGroupData.pQueueList + pos + 1;
                memcpy(pNameDes, pNameSrc, (qGroupData.numberOfQueues - pos) * sizeof(ClNameT));
                clHeapFree(qGroupData.pQueueList);
                qGroupData.pQueueList = pNameTemp;
            }

            rc = clMsgQGroupCkptDataMarshal(&qGroupData, &ckptData);
            if (CL_OK != rc)
            {
                clLogError("MSG", "GroupCkpt_UPD", "Failed to marshal message queue group [%.*s]. error code [0x%x]."
                                        , qGroupData.qGroupName.length,  qGroupData.qGroupName.value
                                        , rc);
                clMsgQGroupCkptDataFree(&qGroupData);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                return;
            }

            if (qGroupData.qGroupAddress.nodeAddress == gLocalAddress)
            {
                clCachedCkptSectionUpdate(&gMsgQGroupCkptServer, &ckptData);
            }
            else
            {
                clCacheEntryUpdate(&gMsgQGroupCkptServer, &ckptData);
            }

            clHeapFree(ckptData.data);
            clMsgGroupMembershipInfoSend(CL_MSG_DATA_DEL, &qGroupData.qGroupName, pQName);
            isUpdate = CL_TRUE;
        }

        clMsgQGroupCkptDataFree(&qGroupData);

        if(isUpdate == CL_FALSE)
        {
            sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
            i++;
        }
    }

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
}

ClRcT clMsgQCkptDataUpdate(ClMsgSyncActionT syncupType, ClMsgQueueCkptDataT *pQueueData, ClBoolT updateCkpt)
{
    ClRcT rc = CL_OK, retCode;

    ClCachedCkptDataT ckptData;

    clOsalSemLock(gMsgQCkptServer.cacheSem);

    rc = clMsgQueueCkptDataMarshal(pQueueData, &ckptData);
    if (CL_OK != rc)
    {
        clLogError("MSG", "QueCkpt_UPD", "Failed to marshal message queue [%.*s]. error code [0x%x]."
                                        , pQueueData->qName.length,  pQueueData->qName.value
                                        , rc);
        clOsalSemUnlock(gMsgQCkptServer.cacheSem);
        goto error_out;
    }

    /* Updating msg queue cached ckpt */
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            if (updateCkpt)
                rc = clCachedCkptSectionCreate(&gMsgQCkptServer, &ckptData);
            else
                rc = clCacheEntryAdd(&gMsgQCkptServer, &ckptData);
            break;
        case CL_MSG_DATA_DEL:
            if (updateCkpt)
                rc = clCachedCkptSectionDelete(&gMsgQCkptServer, &ckptData.sectionName);
            else
                rc = clCacheEntryDelete(&gMsgQCkptServer, &ckptData.sectionName);

            clMsgQueueGroupsRemove((ClNameT *)&ckptData.sectionName);
            break;
        case CL_MSG_DATA_UPD:
            if (updateCkpt)
                rc = clCachedCkptSectionUpdate(&gMsgQCkptServer, &ckptData);
            else
                rc = clCacheEntryUpdate(&gMsgQCkptServer, &ckptData);
            break;
    }

    if(CL_OK != rc)
    {
        clLogError("MSG", "QueCkpt_UPD", "Failed to update message queue [%.*s] to the cached checkpoint. error code [0x%x]."
                                        , pQueueData->qName.length,  pQueueData->qName.value
                                        , rc);
        clOsalSemUnlock(gMsgQCkptServer.cacheSem);
        goto error_out_1;
    }

    clOsalSemUnlock(gMsgQCkptServer.cacheSem);

    if (updateCkpt)
    {
        /* Updating all the message server's cache except my own. */
        retCode = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncupType, pQueueData, CL_FALSE, NULL, NULL);
        if(CL_OK != retCode)
        {
            clLogError("MSG", "QueCkpt_UPD", "Failed to synchronize all message queue cache checkpoints. error code [0x%x].", retCode);
        }
    }

error_out_1:
    clHeapFree(ckptData.data);
error_out:
    return rc;
}

ClRcT clMsgQGroupCkptDataUpdate(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy, ClIocPhysicalAddressT qGroupAddress, ClBoolT updateCkpt)
{
    ClRcT rc = CL_OK, retCode;

    ClMsgQGroupCkptDataT qGroupData;
    ClCachedCkptDataT ckptData;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            clNameCopy(&qGroupData.qGroupName,pGroupName);
            qGroupData.policy = policy;
            qGroupData.qGroupAddress = qGroupAddress;
            qGroupData.numberOfQueues = 0;
            qGroupData.pQueueList = NULL;

            rc = clMsgQGroupCkptDataMarshal(&qGroupData, &ckptData);
            if (CL_OK != rc)
            {
                clLogError("MSG", "GroupCkpt_UPD", "Failed to marshal message queue group [%.*s]. error code [0x%x]."
                                        , pGroupName->length,  pGroupName->value
                                        , rc);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                goto error_out;
            }

            if (updateCkpt)
                rc = clCachedCkptSectionCreate(&gMsgQGroupCkptServer, &ckptData);
            else
                rc = clCacheEntryAdd(&gMsgQGroupCkptServer, &ckptData);

            clHeapFree(ckptData.data);
            break;
        case CL_MSG_DATA_UPD:
            break;
        case CL_MSG_DATA_DEL:
            if (updateCkpt)
                rc = clCachedCkptSectionDelete(&gMsgQGroupCkptServer, pGroupName);
            else
                rc = clCacheEntryDelete(&gMsgQGroupCkptServer, pGroupName);
            break;
    }

    if(CL_OK != rc)
    {
        clLogError("MSG", "GroupCkpt_UPD", "Failed to update message queue group [%.*s] to the cached checkpoint. error code [0x%x]."
                                        , pGroupName->length,  pGroupName->value
                                        , rc);
        clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
        goto error_out;
    }

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
    if (updateCkpt)
    {
        /* Updating all the message server's cache except my own. */
        retCode = VDECL_VER(clMsgGroupDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncupType, pGroupName, policy, qGroupAddress, CL_FALSE, NULL, NULL);
        if(CL_OK != retCode)
        {
            clLogError("MSG", "GroupCkpt_UPD", "Failed to synchronize all message queue group cache checkpoints. error code [0x%x].", retCode);
        }
    }
    else
    {
        clMsgGroupInfoUpdate(syncupType, pGroupName, policy);
    }

error_out:
    return rc;
}

ClRcT clMsgQGroupMembershipCkptDataUpdate(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName, ClBoolT updateCkpt)
{
    ClRcT rc = CL_OK, retCode;

    ClMsgQGroupCkptDataT qGroupData = {{0}};
    ClCachedCkptDataT ckptData;
    ClUint32T pos;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);

    if(clMsgQGroupCkptDataGet_Nolock((ClNameT*)pGroupName, &qGroupData) != CL_OK)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "GroupCkpt_UPD", "Message queue group [%.*s] does not exist. error code [0x%x]."
                        , pGroupName->length, pGroupName->value
                        , rc);
        clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
        goto error_out;
    }

    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            if (clMsgQGroupCkptQueueExist(&qGroupData, (ClNameT *)pQueueName, &pos) == CL_TRUE)
            {
                clMsgQGroupCkptDataFree(&qGroupData);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                goto out;
            }

            qGroupData.numberOfQueues++;
            ClNameT *pNameTemp = (ClNameT *) clHeapAllocate(qGroupData.numberOfQueues * sizeof(ClNameT));

            if (qGroupData.pQueueList != NULL)
            {
                memcpy(pNameTemp, qGroupData.pQueueList, (qGroupData.numberOfQueues - 1) * sizeof(ClNameT));
                clHeapFree(qGroupData.pQueueList);
            }
            ClNameT *pNameCopy = pNameTemp + qGroupData.numberOfQueues -1;
            memcpy(pNameCopy, pQueueName, sizeof(ClNameT));
            qGroupData.pQueueList = pNameTemp;

            rc = clMsgQGroupCkptDataMarshal(&qGroupData, &ckptData);
            if (CL_OK != rc)
            {
                clLogError("MSG", "GroupCkpt_UPD", "Failed to marshal message queue group [%.*s]. error code [0x%x]."
                                        , pGroupName->length,  pGroupName->value
                                        , rc);
                clMsgQGroupCkptDataFree(&qGroupData);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                goto error_out;
            }

            if (updateCkpt)
                rc = clCachedCkptSectionUpdate(&gMsgQGroupCkptServer, &ckptData);
            else
                rc = clCacheEntryUpdate(&gMsgQGroupCkptServer, &ckptData);

            clHeapFree(ckptData.data);
            break;
        case CL_MSG_DATA_UPD:
            break;
        case CL_MSG_DATA_DEL:
            if (clMsgQGroupCkptQueueExist(&qGroupData, (ClNameT *)pQueueName, &pos) == CL_FALSE)
            {
                clMsgQGroupCkptDataFree(&qGroupData);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                goto out;
            }

            qGroupData.numberOfQueues--;
            if (qGroupData.numberOfQueues == 0)
            {
                clHeapFree(qGroupData.pQueueList);
                qGroupData.pQueueList = NULL;
            }
            else
            {
                ClNameT *pNameTemp = (ClNameT *) clHeapAllocate(qGroupData.numberOfQueues * sizeof(ClNameT));
                memcpy(pNameTemp, qGroupData.pQueueList, pos * sizeof(ClNameT));

                ClNameT *pNameDes = pNameTemp + pos;
                ClNameT *pNameSrc = qGroupData.pQueueList + pos + 1;
                memcpy(pNameDes, pNameSrc, (qGroupData.numberOfQueues - pos) * sizeof(ClNameT));
                clHeapFree(qGroupData.pQueueList);
                qGroupData.pQueueList = pNameTemp;
            }

            rc = clMsgQGroupCkptDataMarshal(&qGroupData, &ckptData);
            if (CL_OK != rc)
            {
                clLogError("MSG", "GroupCkpt_UPD", "Failed to marshal message queue group [%.*s]. error code [0x%x]."
                                        , pGroupName->length,  pGroupName->value
                                        , rc);
                clMsgQGroupCkptDataFree(&qGroupData);
                clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
                goto error_out;
            }

            if (updateCkpt)
                rc = clCachedCkptSectionUpdate(&gMsgQGroupCkptServer, &ckptData);
            else
                rc = clCacheEntryUpdate(&gMsgQGroupCkptServer, &ckptData);
            clHeapFree(ckptData.data);

            break;
    }

    clMsgQGroupCkptDataFree(&qGroupData);
    if(CL_OK != rc)
    {
        clLogError("MSG", "GroupCkpt_UPD", "Failed to update message queue group [%.*s] to the cached checkpoint. error code [0x%x]."
                                        , pGroupName->length,  pGroupName->value
                                        , rc);
        clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
        goto out;
    }

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);

    if (updateCkpt)
    {
        /* Updating all the message server's cache except my own. */
        retCode = VDECL_VER(clMsgGroupMembershipUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncupType, pGroupName, pQueueName, CL_FALSE, NULL, NULL);
        if(CL_OK != retCode)
        {
            clLogError("MSG", "GroupCkpt_UPD", "Failed to synchronize all message queue group cache checkpoints. error code [0x%x].", retCode);
        }
    }
    else
    {
        clMsgGroupMembershipInfoSend(syncupType, pGroupName, pQueueName);
    }

error_out:
out:
    return rc;
}

void clMsgQCkptCompDown(ClIocAddressT *pAddr)
{
    ClRcT			rc = CL_OK;
    ClUint32T                   i = 0;
    ClUint32T                   shmSize = gMsgQCkptServer.cachSize;

    clOsalSemLock(gMsgQCkptServer.cacheSem);

    ClUint32T        *numberOfSections = (ClUint32T *) gMsgQCkptServer.cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;
    ClBoolT          isModify = CL_FALSE;

    while(i < *numberOfSections)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);
        pTemp->data = (ClUint8T *)(pTemp + 1);
        ClUint32T dataSize = pTemp->dataSize;
        ClUint32T nextSectionOffset = sectionOffset + sizeof(ClCachedCkptDataT) + dataSize; 
        ClCachedCkptDataT *pNextRecord = (ClCachedCkptDataT *)(data + nextSectionOffset);
        ClMsgQueueCkptDataT qCkptData;

        ClBoolT delCkpt = CL_FALSE;
        ClBoolT modCkpt = CL_FALSE;
        memset(&qCkptData, 0, sizeof(qCkptData));
        clMsgQueueCkptDataUnmarshal(&qCkptData, (ClCachedCkptDataT *)pTemp);

        if ((qCkptData.qAddress.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
           && (qCkptData.qAddress.portId == pAddr->iocPhyAddress.portId) )
        {
            qCkptData.qAddress.nodeAddress = 0;
            pTemp->sectionAddress.iocPhyAddress.nodeAddress = 0;
            if ((qCkptData.creationFlags == 0 ) || (qCkptData.qServerAddress.nodeAddress == 0))
                delCkpt = CL_TRUE;
            modCkpt = CL_TRUE;
        }

        if ((qCkptData.qServerAddress.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
           && (qCkptData.qServerAddress.portId == pAddr->iocPhyAddress.portId) )
        {
            qCkptData.qServerAddress.nodeAddress = 0;
            ClUint32T *pQServerNodeAddr = (ClUint32T *) (pTemp->data);
            *pQServerNodeAddr = 0;
            if (qCkptData.qAddress.nodeAddress == 0)
                delCkpt = CL_TRUE;
            modCkpt = CL_TRUE;
        }

        if (delCkpt)
        {
            /* Remove MSG queue from all MSG queue groups */
            clMsgQueueGroupsRemove(&pTemp->sectionName);

            clCkptEntryDelete(&gMsgQCkptServer, &pTemp->sectionName);

            memcpy (pTemp, pNextRecord, *sizeOfCache - nextSectionOffset);
            (*numberOfSections)--;
            *sizeOfCache -= sizeof(ClCachedCkptDataT) + dataSize;
            isModify = CL_TRUE;
        }
        else
        {
            if(modCkpt)
            {
                clCkptEntryUpdate(&gMsgQCkptServer, (ClCachedCkptDataT *) pTemp);
                isModify = CL_TRUE;
            }
            sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
            i++;
        }
    }

    if(isModify)
    {
        rc = clOsalMsync(gMsgQCkptServer.cache, shmSize, MS_SYNC);
        if (rc != CL_OK)
        {
            clLogError("CCK", "DEL", "clOsalMsync(): error code [0x%x].", rc);
        }
    }

    clOsalSemUnlock(gMsgQCkptServer.cacheSem);
}

void clMsgQCkptNodeDown(ClIocAddressT *pAddr)
{
    ClRcT			rc = CL_OK;
    ClUint32T                   i = 0;
    ClUint32T                   shmSize = gMsgQCkptServer.cachSize;

    clOsalSemLock(gMsgQCkptServer.cacheSem);

    ClUint32T        *numberOfSections = (ClUint32T *) gMsgQCkptServer.cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;
    ClBoolT          isModify = CL_FALSE;

    while(i < *numberOfSections)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);
        pTemp->data = (ClUint8T *)(pTemp + 1);
        ClUint32T dataSize = pTemp->dataSize;
        ClUint32T nextSectionOffset = sectionOffset + sizeof(ClCachedCkptDataT) + dataSize; 
        ClCachedCkptDataT *pNextRecord = (ClCachedCkptDataT *)(data + nextSectionOffset);
        ClMsgQueueCkptDataT qCkptData;

        ClBoolT delCkpt = CL_FALSE;
        ClBoolT modCkpt = CL_FALSE;

        clMsgQueueCkptDataUnmarshal(&qCkptData, (ClCachedCkptDataT *)pTemp);

        if ((qCkptData.qAddress.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
                && (qCkptData.state != CL_MSG_QUEUE_CLOSED))
        {
            qCkptData.qAddress.nodeAddress = 0;
            pTemp->sectionAddress.iocPhyAddress.nodeAddress = 0;
            if ((qCkptData.creationFlags == 0 ) || (qCkptData.qServerAddress.nodeAddress == 0))
                delCkpt = CL_TRUE;
            modCkpt = CL_TRUE;
        }

        if (qCkptData.qServerAddress.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
        {
            qCkptData.qServerAddress.nodeAddress = 0;
            ClUint32T *pQServerNodeAddr = (ClUint32T *) (pTemp->data);
            *pQServerNodeAddr = 0;
            if (qCkptData.qAddress.nodeAddress == 0)
                delCkpt = CL_TRUE;
            modCkpt = CL_TRUE;
        }

        if (delCkpt)
        {
            /* Remove MSG queue from all MSG queue groups */
            clMsgQueueGroupsRemove(&pTemp->sectionName);

            clCkptEntryDelete(&gMsgQCkptServer, &pTemp->sectionName);

            memcpy (pTemp, pNextRecord, *sizeOfCache - nextSectionOffset);
            (*numberOfSections)--;
            *sizeOfCache -= sizeof(ClCachedCkptDataT) + dataSize;
            isModify = CL_TRUE;
        }
        else
        {
            if(modCkpt)
            {
                clCkptEntryUpdate(&gMsgQCkptServer, (ClCachedCkptDataT *) pTemp);
                isModify = CL_TRUE;
            }

            sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
            i++;
        }
    }

    if(isModify)
    {
        rc = clOsalMsync(gMsgQCkptServer.cache, shmSize, MS_SYNC);
        if (rc != CL_OK)
        {
            clLogError("CCK", "DEL", "clOsalMsync(): error code [0x%x].", rc);
        }
    }

    clOsalSemUnlock(gMsgQCkptServer.cacheSem);
}

void clMsgQGroupCkptNodeDown(ClIocAddressT *pAddr)
{
    ClRcT			rc = CL_OK;
    ClUint32T                   i = 0;
    ClUint32T                   shmSize = gMsgQGroupCkptServer.cachSize;

    clOsalSemLock(gMsgQGroupCkptServer.cacheSem);

    ClUint32T        *numberOfSections = (ClUint32T *) gMsgQGroupCkptServer.cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;
    ClBoolT          isDelete = CL_FALSE;

    while(i < *numberOfSections)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);
        ClUint32T dataSize = pTemp->dataSize;
        ClUint32T nextSectionOffset = sectionOffset + sizeof(ClCachedCkptDataT) + dataSize; 
        ClCachedCkptDataT *pNextRecord = (ClCachedCkptDataT *)(data + nextSectionOffset);

        if (pTemp->sectionAddress.iocPhyAddress.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
        {
            clCkptEntryDelete(&gMsgQGroupCkptServer, &pTemp->sectionName);

            memcpy (pTemp, pNextRecord, *sizeOfCache - nextSectionOffset);
            (*numberOfSections)--;
            *sizeOfCache -= sizeof(ClCachedCkptDataT) + dataSize;
            isDelete = CL_TRUE;
        }
        else
        {
            sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
            i++;
        }
    }

    if(isDelete)
    {
        rc = clOsalMsync(gMsgQGroupCkptServer.cache, shmSize, MS_SYNC);
        if (rc != CL_OK)
        {
            clLogError("CCK", "DEL", "clOsalMsync(): error code [0x%x].", rc);
        }
    }

    clOsalSemUnlock(gMsgQGroupCkptServer.cacheSem);
}

void clMsgFailoverQueuesMove(ClIocNodeAddressT destNode, ClUint32T *pNumOfOpenQs)
{
    ClRcT			rc = CL_OK, retCode;
    ClUint32T                   i = 0;
    ClUint32T                   shmSize = gMsgQCkptServer.cachSize;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    clOsalSemLock(gMsgQCkptServer.cacheSem);

    if (gMsgQCkptServer.cache == NULL)
        goto out;

    ClUint32T        *numberOfSections = (ClUint32T *) gMsgQCkptServer.cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;
    ClBoolT          isModify = CL_FALSE;

    while(i < *numberOfSections)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);
        pTemp->data = (ClUint8T *)(pTemp + 1);
        ClMsgQueueCkptDataT qCkptData;

        ClBoolT modCkpt = CL_FALSE;

        clMsgQueueCkptDataUnmarshal(&qCkptData, (ClCachedCkptDataT *)pTemp);

        if ((qCkptData.qAddress.nodeAddress == gLocalAddress)
            && (qCkptData.qAddress.portId == CL_IOC_MSG_PORT))
        {
            qCkptData.qAddress.nodeAddress = 0;
            pTemp->sectionAddress.iocPhyAddress.nodeAddress = 0;

            retCode = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, CL_MSG_DATA_UPD, &qCkptData, CL_FALSE, NULL, NULL);
            if(CL_OK != retCode)
            {
                clLogError("MSG", "QueCkpt_UPD", "Failed to synchronize all message queue cache checkpoints. error code [0x%x].", retCode);
            }

            clMsgToDestQueueMove(destNode, &qCkptData.qName);

            qCkptData.qAddress.nodeAddress = destNode;
            pTemp->sectionAddress.iocPhyAddress.nodeAddress = destNode;

            clCkptEntryUpdate(&gMsgQCkptServer, (ClCachedCkptDataT *) pTemp);

            retCode = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, CL_MSG_DATA_UPD, &qCkptData, CL_FALSE, NULL, NULL);
            if(CL_OK != retCode)
            {
                clLogError("MSG", "QueCkpt_UPD", "Failed to synchronize all message queue cache checkpoints. error code [0x%x].", retCode);
            }
            modCkpt = CL_TRUE;
        }
        else if ((qCkptData.qServerAddress.nodeAddress == gLocalAddress)
            && (qCkptData.qServerAddress.portId == CL_IOC_MSG_PORT))
        {
            ClUint32T *pQServerNodeAddr = (ClUint32T *) (pTemp->data);

            qCkptData.qServerAddress.nodeAddress = 0;
            *pQServerNodeAddr = 0;

            retCode = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, CL_MSG_DATA_UPD, &qCkptData, CL_FALSE, NULL, NULL);
            if(CL_OK != retCode)
            {
                clLogError("MSG", "QueCkpt_UPD", "Failed to synchronize all message queue cache checkpoints. error code [0x%x].", retCode);
            }

            clMsgToDestQueueMove(destNode, &qCkptData.qName);

            qCkptData.qServerAddress.nodeAddress = destNode;
            *pQServerNodeAddr = (ClUint32T) htonl((ClUint32T)destNode);

            clCkptEntryUpdate(&gMsgQCkptServer, (ClCachedCkptDataT *) pTemp);

            retCode = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, CL_MSG_DATA_UPD, &qCkptData, CL_FALSE, NULL, NULL);
            if(CL_OK != retCode)
            {
                clLogError("MSG", "QueCkpt_UPD", "Failed to synchronize all message queue cache checkpoints. error code [0x%x].", retCode);
            }
            modCkpt = CL_TRUE;
        }
        else
            (*pNumOfOpenQs)++;

        if(modCkpt)
        {
            isModify = CL_TRUE;
        }
        else
        {
            sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
            i++;
        }
    }

    if(isModify)
    {
        rc = clOsalMsync(gMsgQCkptServer.cache, shmSize, MS_SYNC);
        if (rc != CL_OK)
        {
            clLogError("CCK", "DEL", "clOsalMsync(): error code [0x%x].", rc);
        }
    }

out:
    clOsalSemUnlock(gMsgQCkptServer.cacheSem);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

}
