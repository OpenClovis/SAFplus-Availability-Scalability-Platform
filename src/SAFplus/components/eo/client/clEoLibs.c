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
 * ModuleName  : eolog
File        : clEoLibs.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provides for reading the Log related functionality 
 *
 *
 ****************************************************************************/

/*
 * Standard header files.
 */
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h>
/*
 * ASP header files.
 */
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clIocIpi.h>
#include <clEoLibs.h>
#include <clRmdIpi.h>


#define CL_EO_COMPONENTS   32


typedef struct {
    ClIocNodeAddressT      node;
    ClIocPortT             port;
    ClCpmNotificationFuncT pFunc;
    ClPtrT                 pArg;
} ClEoCallbackRecT;


typedef struct {
    ClOsalMutexT      lock;
    ClUint32T         numRecs;
    ClEoCallbackRecT  **pDb;
} ClEoCallbackDbT;

static ClEoCallbackDbT gpCallbackDb;

static ClRcT clEoClientNotification(ClIocNotificationT *notificationInfo);

ClRcT clEoWaterMarkHit(ClCompIdT compId, ClWaterMarkIdT wmId, ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList)
{
    ClRcT rc = CL_OK;
    ClUint64T wmValue = 0;
    ClEoLibIdT libId;

    CL_EO_LIB_VERIFY();

    libId =  eoCompId2LibId(compId);

    /*
     * Validate the parameter(s)
     */
    if(CL_OK != eoLibIdValidate(libId))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid Library Id Specified [%u], rc=[0x%x]\n",
                 compId, CL_EO_RC(CL_EO_ERR_LIB_ID_INVALID)));
        return CL_EO_RC(CL_EO_ERR_LIB_ID_INVALID);
    }

    if(CL_OK != eoWaterMarkIdValidate(libId, wmId))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid Water Mark Id Specified [%u], rc=[0x%x]\n", 
                 wmId, CL_EO_RC(CL_EO_ERR_WATER_MARK_ID_INVALID)));
        return CL_EO_RC(CL_EO_ERR_WATER_MARK_ID_INVALID);
    }

    wmValue = (wmType? pWaterMark->highLimit : pWaterMark->lowLimit);

    /*
     * Depending on the BitMap take the action
     */
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("EO[%s]:LIB[%s]:The Action being triggered for "
                "Water Mark[%d]->[%s] with the value [%lld]\n", 
                clEoConfig.EOname, LIB_NAME(libId), wmId, 
                (wmType == CL_WM_HIGH_LIMIT)? "HIGH_LIMIT" : "LOW_LIMIT", 
                wmValue));

    /*
     * Queue the Water Mark Info for asynchronous action generation.
     */ 
    rc = clEoQueueWaterMarkInfo(libId, wmId, pWaterMark, wmType, argList);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN, ("Failed to execute the action for:"
                    "EO[%s]:LIB[%s]:Water Mark[%d]->[%s] with the value [%lld]\n", 
                    clEoConfig.EOname, LIB_NAME(libId), wmId,
                    (wmType == CL_WM_HIGH_LIMIT)? "HIGH_LIMIT" : "LOW_LIMIT", 
                    wmValue));
        return rc;
    }

    return CL_OK;
}

static ClRcT clEoIocWMNotification(ClCompIdT compId,
        ClIocPortT port,
        ClIocNodeAddressT nodeId,
        ClIocNotificationT *pIocNotification
        )
{
    ClRcT rc = CL_OK;
    ClIocQueueNotificationT *pIocQueueNotification = NULL;
    ClIocQueueNotificationT queueNotification = {0};
    ClIocNotificationIdT id = ntohl(pIocNotification->id);
    ClWaterMarkIdT wmID;
    const ClCharT *msgStr = NULL;

    switch(id)
    {
        case CL_IOC_SENDQ_WM_NOTIFICATION:
            {
                msgStr = "node sendq";
                pIocQueueNotification = &pIocNotification->sendqWMNotification;
                wmID = CL_WM_SENDQ;
            }
            break;
        case CL_IOC_COMM_PORT_WM_NOTIFICATION:
            {
                msgStr = "commport recvq";
                pIocQueueNotification = &pIocNotification->commPortWMNotification;
                wmID = CL_WM_RECVQ;
            }
            break;
        default:
            rc = CL_EO_RC(CL_ERR_UNSPECIFIED);
            goto out;
    }

    clIocQueueNotificationUnpack(pIocQueueNotification,&queueNotification);

    clEoLibLog(compId,CL_LOG_INFORMATIONAL,
            "Port: %d, node: %d hit %s watermark event for %s." \
            "Watermark limits(low:%lld,high:%lld), queue size:%d bytes " \
            "message length: %d bytes\n",
            port,nodeId,queueNotification.wmID == CL_WM_LOW ? "Low" : "High",
            msgStr,
            queueNotification.wm.lowLimit,queueNotification.wm.highLimit,
            queueNotification.queueSize,queueNotification.messageLength);

    /*Trigger EO watermark hit to take the specific actions*/
    rc = clEoWaterMarkHit(compId,wmID,
            &queueNotification.wm,
            queueNotification.wmID == CL_WM_HIGH \
            ? CL_TRUE : CL_FALSE,NULL);

out:
    return rc;
}

/*
 * This function shall change when the CPM has a devoted queue to handle the death & leave 
 * Notifications. The pRecvParam shall be of use then for queuing.
 */
ClRcT clEoProcessIocRecvPortNotification(ClEoExecutionObjT* pThis, ClBufferHandleT eoRecvMsg, ClUint8T priority, ClUint8T protoType, ClUint32T length, ClIocPhysicalAddressT srcAddr)
{
    ClRcT rc = CL_OK;

    ClIocNotificationT notificationInfo = {0};
    ClUint32T msgLength = sizeof(ClIocNotificationT);
    ClIocNotificationIdT notificationId = 0;
    ClUint32T protoVersion = 0;

    CL_EO_LIB_VERIFY();

    rc = clBufferNBytesRead(eoRecvMsg, (ClUint8T *)&notificationInfo,
            &msgLength);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("EO: clBufferNBytesRead() Failed, rc=[0x%x]\n",
                 rc));
        goto out_delete;
    }
    
    protoVersion = ntohl(notificationInfo.protoVersion);
    if(protoVersion != CL_IOC_NOTIFICATION_VERSION)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("EO: Ioc recv notification received with version [%d]. Supported [%d]\n",
                        protoVersion, CL_IOC_NOTIFICATION_VERSION));
        goto out_delete;
    }

    notificationId = ntohl(notificationInfo.id); 

    switch(notificationId)
    {
        case CL_IOC_SENDQ_WM_NOTIFICATION:
            if(gIsNodeRepresentative == CL_FALSE)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Got sendq wm notification for non-node representative\n"));
                rc = CL_EO_RC(CL_ERR_BAD_OPERATION);
                goto out_delete;
            }
            /*fall through*/
        case CL_IOC_COMM_PORT_WM_NOTIFICATION:
            {
                ClIocNodeAddressT nodeId = 0;
                ClIocPortT portId = ntohl(notificationInfo.nodeAddress.iocPhyAddress.portId);

                ClIocNodeAddressT nodeAddress = ntohl(notificationInfo.nodeAddress.iocPhyAddress.nodeAddress);

                CL_CPM_IOC_ADDRESS_SLOT_GET(nodeAddress,nodeId); 

                rc = clEoIocWMNotification(CL_CID_IOC,portId,nodeId,&notificationInfo);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("EO: clEoIocWMNotification() Failed, rc=[0x%x]\n",
                             rc));
                    goto out_delete;
                }
            }
            break;
        case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
        case CL_IOC_NODE_LINK_UP_NOTIFICATION:
        case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
            /* Need to add functionality if the components wants a specific operation...*/
            clEoClientNotification(&notificationInfo);
            rc = CL_OK;
            break;
        case CL_IOC_NODE_LEAVE_NOTIFICATION:
        case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
        case CL_IOC_COMP_DEATH_NOTIFICATION:
            rc = clBufferDelete(&eoRecvMsg);
            CL_ASSERT(rc == CL_OK);
            rc = clRmdDatabaseCleanup(pThis->rmdObj, &notificationInfo);
            clEoClientNotification(&notificationInfo);
            return rc;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid notification id[%d]\n",
                        notificationId));
            rc = CL_EO_RC(CL_ERR_BAD_OPERATION);
            goto out_delete; 
    }

out_delete:
    clBufferDelete(&eoRecvMsg);

    return rc;
}

ClRcT clEoLibLog (ClUint32T compId,ClUint32T severity, const ClCharT *msg, ...)
{
    ClRcT rc = CL_OK;
    ClCharT clEoLogMsg[CL_EOLIB_MAX_LOG_MSG] = "";
    ClEoLibIdT libId = eoCompId2LibId(compId);
    va_list args;

    CL_EO_LIB_VERIFY();

    if((rc = eoLibIdValidate(libId)) != CL_OK)
    {
        return rc;
    }

    va_start(args, msg);
    vsnprintf(clEoLogMsg, CL_EOLIB_MAX_LOG_MSG, msg, args);
    va_end(args);
    /*Dont use the return val. as of now*/
    //if(clLogCheckLog(severity) == CL_TRUE)
    {
        clEoQueueLogInfo(libId,severity,(const ClCharT *)clEoLogMsg);
    }
    return CL_OK;
}

ClRcT clEoLogInitialize(void)
{
    ClRcT rc = CL_FALSE;
    rc = clLogLibInitialize();
    if ( rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Log Open Failed\n"));
        return rc;
    }
    return CL_OK;
}

void clEoLogFinalize(void)
{
    /* Make sure we call this only if open is successfull */
    clLogLibFinalize();
}


ClRcT clEoClientCallbackDbInitialize(void)
{
    ClRcT rc;

    gpCallbackDb.pDb = (ClEoCallbackRecT **)clHeapAllocate(sizeof(ClEoCallbackRecT*) * CL_EO_COMPONENTS);
    if(gpCallbackDb.pDb == NULL)
    {
        /* Print Error here */
        return CL_EO_RC( CL_ERR_NO_MEMORY);
    }

    memset(gpCallbackDb.pDb, 0, sizeof(ClEoCallbackRecT *) * CL_EO_COMPONENTS);

    gpCallbackDb.numRecs = CL_EO_COMPONENTS;

    rc = clOsalMutexInit(&gpCallbackDb.lock);
    CL_ASSERT(rc == CL_OK);

    return CL_OK;
}


ClRcT clEoClientCallbackDbFinalize(void)
{
    ClRcT rc;
    ClUint32T i;
    
    rc = clOsalMutexLock(&gpCallbackDb.lock);
    if(rc != CL_OK)
    {
        /* Print a message here */
        return rc;
    }

    for(i = 0 ; i < gpCallbackDb.numRecs; i++)
    {
        if(gpCallbackDb.pDb[i] != NULL)
            clHeapFree(gpCallbackDb.pDb[i]);
    }

    clHeapFree(gpCallbackDb.pDb);

    gpCallbackDb.numRecs = 0;

    clOsalMutexUnlock(&gpCallbackDb.lock);

    return CL_OK;
}


ClRcT clEoNotificationCallbackInstall(ClIocPhysicalAddressT compAddr, ClPtrT pFunc, ClPtrT pArg, ClHandleT *pHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClEoCallbackRecT **tempDb;
    
    if(pFunc == NULL || pHandle == NULL)
    {
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clOsalMutexLock(&gpCallbackDb.lock);
    if(rc != CL_OK)
    {
        /* Print an error message here */
        return rc;
    }

re_check:
    for(; i < gpCallbackDb.numRecs; i++)
    {
        if(gpCallbackDb.pDb[i] == NULL)
        {
            ClEoCallbackRecT *pRec = {0};

            pRec =(ClEoCallbackRecT *)clHeapAllocate(sizeof(ClEoCallbackRecT));

            pRec->node = compAddr.nodeAddress;
            pRec->port = compAddr.portId;
            pRec->pFunc = *((ClCpmNotificationFuncT*)&pFunc);
            pRec->pArg = pArg;

            *pHandle = i;

            gpCallbackDb.pDb[i] = pRec;
            goto out;
        }
    }
    
    tempDb = clHeapRealloc(gpCallbackDb.pDb, sizeof(ClEoCallbackRecT *) * gpCallbackDb.numRecs * 2);
    if(tempDb == NULL)
    {
        rc  = CL_EO_RC(CL_ERR_NO_MEMORY);
        goto out;
    }
    memset(tempDb + gpCallbackDb.numRecs, 0, sizeof(ClEoCallbackRecT *) * gpCallbackDb.numRecs);
    gpCallbackDb.pDb =  tempDb;
    gpCallbackDb.numRecs *= 2;
    goto re_check;

out :
    clOsalMutexUnlock(&gpCallbackDb.lock);
    return rc;
}


ClRcT clEoNotificationCallbackUninstall(ClHandleT *pHandle)
{
    if(pHandle == NULL || 
       *pHandle >= gpCallbackDb.numRecs)
    {
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    clOsalMutexLock(&gpCallbackDb.lock);
    if(gpCallbackDb.pDb[*pHandle])
    {
        clHeapFree(gpCallbackDb.pDb[*pHandle]);
        gpCallbackDb.pDb[*pHandle] = NULL;
        *pHandle = 0;
    }
    clOsalMutexUnlock(&gpCallbackDb.lock);

    return CL_OK;
}



static ClRcT clEoClientNotification(ClIocNotificationT *notification)
{
    ClUint32T i;
    ClIocNotificationIdT event;
    ClIocNodeAddressT node;
    ClIocPortT port;
    ClEoCallbackRecT *pMatchedRecords = NULL;
    ClUint32T numMatchedRecs = 0;
    ClUint32T numRecs = 0;
    ClIocAddressT address = {{0}};

    if(!notification) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    event = ntohl(notification->id);
    node = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    port = ntohl(notification->nodeAddress.iocPhyAddress.portId);

    if(event == CL_IOC_NODE_ARRIVAL_NOTIFICATION ||
       event == CL_IOC_NODE_LINK_UP_NOTIFICATION ||
       event == CL_IOC_NODE_LEAVE_NOTIFICATION ||
       event == CL_IOC_NODE_LINK_DOWN_NOTIFICATION) 
    {
        port = 0;
    }

    address.iocPhyAddress.nodeAddress = node;
    address.iocPhyAddress.portId = port;

    /*
     * db numRecs could change behind our back. Hence pre-fetching it
     * to have temp db allocations without the lock.
     */
    clOsalMutexLock(&gpCallbackDb.lock);
    numRecs = gpCallbackDb.numRecs;
    clOsalMutexUnlock(&gpCallbackDb.lock);

    /*
     * Better to allocate without the lock. to reduce lock
     * granularity and give scope for further client registrations
     * in this window.
     */
    pMatchedRecords = clHeapAllocate(sizeof(*pMatchedRecords) * numRecs);
    if(!pMatchedRecords)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation error\n"));
        return CL_EO_RC(CL_ERR_NO_MEMORY);
    }
    
    clOsalMutexLock(&gpCallbackDb.lock);
    
    numRecs = CL_MIN(gpCallbackDb.numRecs, numRecs);
    
    for(i = 0; i < numRecs; i++)
    {
        if(gpCallbackDb.pDb[i] != NULL &&
           (gpCallbackDb.pDb[i]->node == node || gpCallbackDb.pDb[i]->node == CL_IOC_BROADCAST_ADDRESS) &&
           (!port || gpCallbackDb.pDb[i]->port == port || !gpCallbackDb.pDb[i]->port)
           )
        {
            memcpy(&pMatchedRecords[numMatchedRecs++],
                   gpCallbackDb.pDb[i],
                   sizeof(*pMatchedRecords));
        }
    }

    clOsalMutexUnlock(&gpCallbackDb.lock);

    for(i = 0; i < numMatchedRecs; ++i)
    {
        pMatchedRecords[i].pFunc(event, pMatchedRecords[i].pArg, &address);
    }
    
    clHeapFree(pMatchedRecords);

    return CL_OK;
}
