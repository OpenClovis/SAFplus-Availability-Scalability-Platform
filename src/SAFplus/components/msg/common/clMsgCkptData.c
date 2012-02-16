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
 * ModuleName  : message
 * File        : clMsgQueueCkpt.c
 *******************************************************************************/

#include <clMsgCkptData.h>
#include <clLogApi.h>
#include <clDebugApi.h>

ClRcT clMsgQueueCkptDataMarshal(ClMsgQueueCkptDataT *qCkptData, ClCachedCkptDataT *outData)
{
    ClRcT rc = CL_OK;
    ClUint8T *copyData;
    ClUint32T network_byte_order;

    clNameCopy(&outData->sectionName, &qCkptData->qName);
    outData->sectionAddress.iocPhyAddress = qCkptData->qAddress;
    outData->dataSize = CL_MSG_QUEUE_DATA_SIZE;

    outData->data = (ClUint8T *) clHeapAllocate(outData->dataSize);
    copyData = outData->data;

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->qServerAddress.nodeAddress);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->qServerAddress.portId);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->state);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->creationFlags);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    return rc;
}

void clMsgQueueCkptDataUnmarshal(ClMsgQueueCkptDataT *qCkptData, const ClCachedCkptDataT *inData)
{
    ClUint8T *copyData;
    ClUint32T network_byte_order;

    clNameCopy(&qCkptData->qName, &inData->sectionName);
    qCkptData->qAddress = inData->sectionAddress.iocPhyAddress;

    copyData = inData->data;

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->qServerAddress.nodeAddress = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->qServerAddress.portId = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->state = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->creationFlags = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);
}

ClRcT clMsgQGroupCkptDataMarshal(ClMsgQGroupCkptDataT *qCkptData, ClCachedCkptDataT *outData)
{
    ClRcT rc = CL_OK;
    ClUint8T *copyData;
    ClUint32T network_byte_order;
    ClUint32T i, dataSize;

    clNameCopy(&outData->sectionName, &qCkptData->qGroupName);
    outData->sectionAddress.iocPhyAddress = qCkptData->qGroupAddress;
    dataSize = 2 * sizeof(ClUint32T);
    for (i = 0; i < qCkptData->numberOfQueues; i++)
    {
        ClNameT *queueName = (ClNameT *)(qCkptData->pQueueList + i);
        dataSize += sizeof(ClUint32T);
        dataSize += queueName->length;
    }
    outData->dataSize = dataSize;

    outData->data = (ClUint8T *) clHeapAllocate(outData->dataSize);
    copyData = outData->data;

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->policy);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    network_byte_order = (ClUint32T) htonl((ClUint32T)qCkptData->numberOfQueues);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);

    for (i = 0; i < qCkptData->numberOfQueues; i++)
    {
        ClNameT *queueName = (ClNameT *)(qCkptData->pQueueList + i);
        network_byte_order = (ClUint32T) htonl((ClUint32T)queueName->length);
        memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
        copyData = copyData + sizeof(ClUint32T);

        memcpy(copyData, &queueName->value, queueName->length);
        copyData = copyData + queueName->length;
    }

    return rc;
}

void clMsgQGroupCkptHeaderUnmarshal(ClMsgQGroupCkptDataT *qCkptData, const ClCachedCkptDataT *inData)
{
    ClUint8T *copyData;
    ClUint32T network_byte_order;

    clNameCopy(&qCkptData->qGroupName, &inData->sectionName);
    qCkptData->qGroupAddress = inData->sectionAddress.iocPhyAddress;

    copyData = inData->data;

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->policy = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->numberOfQueues = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    /* Get header only */
    qCkptData->pQueueList = NULL;

    return;
}

ClRcT clMsgQGroupCkptDataUnmarshal(ClMsgQGroupCkptDataT *qCkptData, const ClCachedCkptDataT *inData)
{
    ClRcT rc = CL_OK;
    ClUint8T *copyData;
    ClUint32T network_byte_order;
    ClUint32T i;

    clNameCopy(&qCkptData->qGroupName, &inData->sectionName);
    qCkptData->qGroupAddress = inData->sectionAddress.iocPhyAddress;

    copyData = inData->data;

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->policy = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
    qCkptData->numberOfQueues = (ClUint32T) ntohl((ClUint32T)network_byte_order);
    copyData = copyData + sizeof(ClUint32T);

    if (qCkptData->numberOfQueues > 0)
    {
        qCkptData->pQueueList = (ClNameT *) clHeapAllocate(qCkptData->numberOfQueues * sizeof(ClNameT));
        if (qCkptData->pQueueList == NULL)
        {
            rc = CL_ERR_NO_MEMORY;
            clLogError("MSG", "UMS", "Failed to allocate memory for %zd bytes.", qCkptData->numberOfQueues * sizeof(ClNameT));
            return rc;
        }
    }

    for (i = 0; i < qCkptData->numberOfQueues; i++)
    {
        ClNameT *queueName = (ClNameT *)(qCkptData->pQueueList + i);
        memset(queueName, 0, sizeof(ClNameT));

        memcpy(&network_byte_order,copyData, sizeof(ClUint32T));
        queueName->length = (ClUint32T) ntohl((ClUint32T)network_byte_order);
        copyData = copyData + sizeof(ClUint32T);

        memcpy(&queueName->value, copyData, queueName->length);
        copyData = copyData + queueName->length;
    }
    return rc;
}

ClBoolT clMsgQGroupCkptQueueExist(ClMsgQGroupCkptDataT *qCkptData, ClNameT *pQueueName, ClUint32T *pPos)
{
    ClUint32T i;

    if(qCkptData->pQueueList == NULL)
        return CL_FALSE;

    for (i = 0; i< qCkptData->numberOfQueues; i++)
    {
        ClNameT *pQueueTemp = (ClNameT *)(qCkptData->pQueueList + i);

        if ((pQueueTemp->length == pQueueName->length) 
          && (memcmp(pQueueTemp->value, pQueueName->value, pQueueTemp->length)==0) )
        {
            *pPos = i;
            return CL_TRUE;
        }
    }

    return CL_FALSE;
}
void clMsgQGroupCkptDataFree(ClMsgQGroupCkptDataT *qCkptData)
{
    if (qCkptData->numberOfQueues > 0)
    {
        if(qCkptData->pQueueList != NULL)
            clHeapFree(qCkptData->pQueueList);
        qCkptData->pQueueList = NULL;
    }
}


