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
#include <clHash.h>
#include <clHandleApi.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clIocConfig.h>
#include <clRmdIpi.h>
#include <clLogApi.h>
#include <clVersion.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgEo.h>
#include <clMsgDatabase.h>
#include <clMsgGroupDatabase.h>
#include <clMsgQueue.h>
#include <clMsgIdl.h>
#include <clMsgReceiver.h>
#include <clMsgSender.h>


/**************************************************************************************************/
#define CL_MSG_CLIENT_BUCKET_BITS       (6)
#define CL_MSG_CLIENT_BUCKETS           (1 << CL_MSG_CLIENT_BUCKET_BITS)
#define CL_MSG_CLIENT_MASK              (CL_MSG_CLIENT_BUCKETS - 1)


static struct hashStruct *ppMsgClientHashTable[CL_MSG_CLIENT_BUCKETS];


static __inline__ ClUint32T clMsgClientHash(ClIocPortT port)
{
    return ((ClUint32T) (port & CL_MSG_CLIENT_MASK));
}

static ClRcT clMsgClientEntryAdd(ClMsgClientDetailsT *pClientEntry)
{
    ClUint32T key = clMsgClientHash(pClientEntry->address.portId);
    return hashAdd(ppMsgClientHashTable, key, &pClientEntry->hash);
}

static void clMsgClientEntryDel(ClMsgClientDetailsT *pClientEntry)
{
    hashDel(&pClientEntry->hash);
}

static ClBoolT clMsgClientEntryExistsByPort(ClIocPortT port, ClMsgClientDetailsT **ppClientEntry)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgClientHash(port);

    for(pTemp = ppMsgClientHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClMsgClientDetailsT *pClientEntry = hashEntry(pTemp, ClMsgClientDetailsT, hash);
        if(pClientEntry->address.portId == port)
        {
            if(ppClientEntry != NULL)
                *ppClientEntry = pClientEntry;
            return CL_TRUE;
        }
    }
    return CL_FALSE;
}


/**************************************************************************************************/



ClRcT VDECL_VER(clMsgClientInit, 4, 0, 0)(ClUint32T *pVersion, SaMsgHandleT cltHandle, SaMsgHandleT *pClientHandle)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgClientDetailsT *pClient;
    ClIocPhysicalAddressT srcAddress;
    SaMsgHandleT msgHandle;

    CL_MSG_SERVER_INIT_CHECK;

    if(pVersion == NULL || pClientHandle == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogError("CLT", "INI", "NULL parameter passed as an argument. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clRmdSourceAddressGet(&srcAddress);
    if(rc != CL_OK)
    {
        clLogError("CLT", "INI", "Failed to get the source address of client. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCreate(gMsgClientHandleDb, sizeof(ClMsgClientDetailsT), &msgHandle);
    if(rc != CL_OK)
    {
        clLogError("CLT", "INI", "Failed to create a handle with the handle database. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("CLT", "INI", "Failed at handle checkout for the client. error code [0x%x].", rc);
        goto error_out_1;
    }

    CL_LIST_HEAD_INIT(&pClient->qList);
    CL_LIST_HEAD_INIT(&pClient->trackList);
    pClient->address = srcAddress;
    pClient->svrHandle = msgHandle;
    pClient->cltHandle = cltHandle;

    clMsgClientEntryAdd(pClient);

    rc = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(rc != CL_OK) {
        clLogError("CLT", "INI", "Failed to checkin the client handle. error code [0x%x].", rc);
        goto error_out_2;
    }

    *pClientHandle = msgHandle;

    clLogDebug("CLT", "INI", "Message Service for Client [0x%x:0x%x] is initialized. Handle is [0x%llx].",
            srcAddress.nodeAddress, srcAddress.portId, msgHandle);

    goto out;

error_out_2:
    clMsgClientEntryDel(pClient);
error_out_1:
    retCode = clHandleDestroy(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("CLT", "INI", "Failed to destroy the client handle. error code [0x%x].", retCode);


error_out:
out:
    return rc;
}


static void clMsgStopAllTracksOfClient(SaMsgHandleT msgHandle, ClMsgClientDetailsT *pClient)
{
    ClListHeadT *pTrackList;
    ClMsgGroupTrackListEntryT *pTrackEntry;
    register ClListHeadT *pTemp;

    pTrackList = &pClient->trackList;
    for(pTemp = pTrackList->pNext; !CL_LIST_HEAD_EMPTY(pTrackList); pTemp = pTrackList->pNext)
    {
        pTrackEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupTrackListEntryT, clientList);
        clMsgQueueGroupTrackStopInternal(pClient, msgHandle, &pTrackEntry->pMsgGroup->name);
    }
}



static void clMsgCloseAllQueuesOfClient(ClMsgClientDetailsT *pClient)
{
    ClListHeadT *pClientQList;
    register ClListHeadT *pTemp;
    ClMsgClientQueueDetailsT *pQDetails;

    pClientQList = &pClient->qList;
    for(pTemp = pClientQList->pNext; !CL_LIST_HEAD_EMPTY(pClientQList); pTemp = pClientQList->pNext)
    {
        pQDetails = CL_LIST_ENTRY(pTemp, ClMsgClientQueueDetailsT, list);
        VDECL_VER(clMsgClientQueueClose, 4, 0, 0)(pQDetails->qHandle);
    }

    return;
}

ClRcT VDECL_VER(clMsgClientFin, 4, 0, 0)(SaMsgHandleT msgHandle)
{
    ClRcT rc  = CL_OK;
    ClMsgClientDetailsT *pClient;
    ClIocPhysicalAddressT dbgAddr;

    clOsalMutexLock(&gClMsgFinalizeLock);

    if(gClMsgSrvInit == CL_FALSE)
    {
        rc = CL_OK;
        clLogError("SRV", "ERR", "Message server is not initialized. error code is [0x%x], but returning [0x%x].", CL_MSG_RC(CL_ERR_NOT_INITIALIZED), rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("CLT", "FIN", "Passed an invalid handle. error code [0x%x].",rc);
        goto error_out;
    }
    
    dbgAddr = pClient->address;
    clMsgCloseAllQueuesOfClient(pClient);
    clMsgStopAllTracksOfClient(msgHandle, pClient);

    clMsgClientEntryDel(pClient);

    rc = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(rc != CL_OK)
        clLogError("CLT", "FIN", "Failed to do message handle checkin. error code [0x%x].",rc);

    rc = clHandleDestroy(gMsgClientHandleDb, msgHandle);
    if(rc != CL_OK)
        clLogError("CLT", "FIN", "Failed to deregister the Client. error code [0x%x].", rc);

    clLogDebug("CLT", "FIN", "Message Service is finalized for Client [0x%x:0x%x].",
            dbgAddr.nodeAddress, dbgAddr.portId);

error_out:
    clOsalMutexUnlock(&gClMsgFinalizeLock);

    return rc;
}

/**************************************************************************************/
/* Cleanup of a component which just died */
 
void clMsgCompLeftCleanup(ClIocAddressT *pAddr) 
{
    ClMsgClientDetailsT *pClientEntry = NULL;
    /* Doing the cleanup of the clientDb as the just-died component was of this node. */
    if(pAddr->iocPhyAddress.nodeAddress == gClMyAspAddress)
    {
        while(clMsgClientEntryExistsByPort(pAddr->iocPhyAddress.portId, &pClientEntry) == CL_TRUE)
        {
            clLogDebug("CLT", "LEFT", "Component [0x%x:0x%x] left. Calling Message Service Finalize for it.", 
                    pAddr->iocPhyAddress.nodeAddress, pAddr->iocPhyAddress.portId);
            VDECL_VER(clMsgClientFin, 4, 0, 0)(pClientEntry->svrHandle);
        }
    }
}
