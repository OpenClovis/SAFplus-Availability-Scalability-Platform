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
 * ModuleName  : txn                                                           
 * File        : clTxnCommIfc.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module inmplements communication layer for transaction processing
 *
 *
 *****************************************************************************/

#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>

#include <xdrClTxnCmdT.h>
#include <xdrClTxnMessageHeaderIDLT.h>
#include <clTxnCommonDefn.h>

#include <clTxnApi.h>
#include <clTxnDb.h>
#include <clTxnStreamIpi.h>
#include <clTxnErrors.h>
#include <clTxnXdrCommon.h>

static ClInt32T _clTxnCommHandleCmp(
        CL_IN   ClCntKeyHandleT key1, 
        CL_IN   ClCntKeyHandleT key2);

static ClRcT _clTxnMarshallUnmarshall(
        CL_IN   ClBufferHandleT         *inMsg,
        CL_IN   ClBufferHandleT         *outMsg);
#if 0
static ClUint32T _clTxnCommHandleHashFn(
        CL_IN   ClCntKeyHandleT key);
#endif

static void _clTxnCommIfcEntryDel(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

static ClRcT _clTxnCommIfcSendMessage(
        CL_IN   ClTxnCommHandleT    commHandle,
        CL_IN   ClUint8T            syncFlag);

ClTxnCommIfcT   *clTxnCommInterface=NULL;

/**
 * Initializes communication interface for exchanging transaction-messages
 */
ClRcT clTxnCommIfcInit(
        CL_IN   ClVersionT  *pTxnSupportVersion)
{
    ClRcT   rc  =   CL_OK;

    CL_FUNC_ENTER();

    /* Initialize communication interface for caching payloads and
       maintaining multiple sessions
    */

    if (clTxnCommInterface != NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Txn Comm Interface already intialized"));
        CL_FUNC_EXIT();
        return (rc);
    }

    clTxnCommInterface = clHeapAllocate(sizeof(ClTxnCommIfcT));

    if (clTxnCommInterface == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }
    memset(clTxnCommInterface, 0, sizeof(ClTxnCommIfcT));

    memcpy(&(clTxnCommInterface->txnSupportedVersion), 
             pTxnSupportVersion, sizeof(ClVersionT));
    rc = clEoMyEoIocPortGet(&(clTxnCommInterface->srcPortNo));

    if (CL_OK == rc)
    {
        rc = clCntLlistCreate(_clTxnCommHandleCmp, 
                                _clTxnCommIfcEntryDel, NULL, 
                                CL_CNT_UNIQUE_KEY, 
                                &(clTxnCommInterface->txnCommSessionList));
    }

    if (CL_OK == rc)
    {
        rc = clOsalMutexCreate (&(clTxnCommInterface->txnCommMutex));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize communication interface. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Finalizes communication interface and releases all allocated memory.
 */
ClRcT clTxnCommIfcFini()
{
    CL_FUNC_ENTER();

    if (clTxnCommInterface == NULL)
    {
        CL_FUNC_EXIT();
        return (CL_OK);
    }
    /* Finalize all active sessions and release allocated memory */
    /* LOCK */
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK)
    { 
        clCntAllNodesDelete(clTxnCommInterface->txnCommSessionList);
        clCntDelete(clTxnCommInterface->txnCommSessionList);
        /* UNLOCK */
        clOsalMutexUnlock (clTxnCommInterface->txnCommMutex);
    }

    clOsalMutexDelete ( clTxnCommInterface->txnCommMutex );

    clHeapFree(clTxnCommInterface);

    clTxnCommInterface = NULL;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Creates new communication session with a given dest component 
 * (txn-client, agent or server)
 */
ClRcT clTxnCommIfcNewSessionCreate(
        CL_IN   ClTxnMessageTypeT       msgType,
        CL_IN   ClIocPhysicalAddressT   destAddr,
        CL_IN   ClUint32T               funcId,
        CL_IN   ClRmdAsyncCallbackT     fpCallback,
        CL_IN   ClUint32T               timeOut,
        CL_IN   ClUint32T               tId,
        CL_OUT  ClTxnCommHandleT        *pHandle)
{
    ClRcT                   rc  = CL_OK;
    ClTxnMessageHeaderT     *pMsgHdr = NULL;
    ClTxnCommSessionKeyT    *pKey    = NULL;
    ClCntNodeHandleT        cntNodeHandle;
    ClTxnCommSessionKeyT    userKey = {
        .msgType  = msgType,
        .funcId   = funcId
    };

    CL_FUNC_ENTER();

    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    clLogDebug(NULL, NULL, "FuncId : %d", funcId);
    memcpy( &(userKey.destAddr), &destAddr, sizeof(ClIocPhysicalAddressT));
    userKey.tId = tId;
    userKey.funcId = funcId;
    userKey.msgType = msgType;

    /* LOCK */
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK)
    {
        rc = clCntNodeFind(clTxnCommInterface->txnCommSessionList, 
                                (ClCntKeyHandleT) &userKey, 
                                (ClCntNodeHandleT *)&cntNodeHandle);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock. Aborting"));
        CL_FUNC_EXIT();
        return CL_ERR_UNSPECIFIED;
    }
    if (CL_OK == rc)
    {
        /* Aleady exists. Do msg-type verification */
        rc = clCntNodeUserKeyGet(clTxnCommInterface->txnCommSessionList, 
                                cntNodeHandle, (ClCntKeyHandleT *) &pKey);
        if(CL_OK != rc)
        {
            clLogError("CMN", NULL,
                    "User key get failed with error [0x%x]", rc);
        }

        rc = clCntNodeUserDataGet(clTxnCommInterface->txnCommSessionList, 
                                cntNodeHandle, (ClCntDataHandleT *) &pMsgHdr);
        if (CL_OK == rc)
        {
            pMsgHdr->fpCallback = fpCallback;
            *pHandle = (ClTxnCommHandleT) pKey;
            pMsgHdr->sessionCount++;
            clLogDebug("CMN", NULL,
                    "Using existing session with key[0x%x:0x%x] msgType[%d] funcId[%d] tid[0x%x]",
                    pKey->destAddr.nodeAddress,
                    pKey->destAddr.portId,
                    pKey->msgType,
                    pKey->funcId, 
                    pKey->tId);
        }
        else
        {
            *pHandle = 0x0;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while retreiving transaction-comm key. rc:0x%x", rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
        /* UNLOCK */
        if (clOsalMutexUnlock (clTxnCommInterface->txnCommMutex) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock"));
            rc = CL_ERR_UNSPECIFIED;
        }
        CL_FUNC_EXIT();
        return (rc);
    }

    pMsgHdr = (ClTxnMessageHeaderT *) clHeapAllocate(sizeof (ClTxnMessageHeaderT));
    pKey = (ClTxnCommSessionKeyT *) clHeapAllocate (sizeof (ClTxnCommSessionKeyT));
    if ( (pMsgHdr == NULL) || (pKey == NULL) )
    {
        /* UNLOCK */
        clOsalMutexUnlock (clTxnCommInterface->txnCommMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory."));
        CL_FUNC_EXIT();
        return CL_ERR_NO_MEMORY;
    }

    /* Create a session, with header and message-buffer for payload */
    memset(pMsgHdr, 0, sizeof(ClTxnMessageHeaderT));
    memset(pKey, 0, sizeof(ClTxnCommSessionKeyT));

    rc = clBufferCreate( &(pMsgHdr->payload));

    if (CL_OK == rc)
    {
        pKey->msgType = msgType;
        pMsgHdr->msgType = msgType;
        pMsgHdr->srcAddr.nodeAddress = clIocLocalAddressGet();
        pMsgHdr->srcAddr.portId = clTxnCommInterface->srcPortNo;
        pMsgHdr->srcStatus = CL_OK;
        pMsgHdr->funcId = funcId;
        pMsgHdr->sessionCount = 0x1;
        pMsgHdr->timeOut = timeOut;
        pKey->funcId = funcId;
        if(fpCallback != NULL)
            pMsgHdr->fpCallback = fpCallback;
        else
            pMsgHdr->fpCallback = NULL;

        memcpy( &(pMsgHdr->destAddr), &destAddr, sizeof(ClIocPhysicalAddressT));
        memcpy( &(pKey->destAddr), &destAddr, sizeof(ClIocPhysicalAddressT));
        pKey->tId = tId; /* Channel is unique per transaction at client*/
        memcpy(&pMsgHdr->version,
               &(clTxnCommInterface->txnSupportedVersion), 
               sizeof(ClVersionT));

        rc = clCntNodeAdd(clTxnCommInterface->txnCommSessionList, 
                          (ClCntKeyHandleT) pKey, 
                          (ClCntDataHandleT) pMsgHdr, NULL);
        if (CL_OK == rc)
        {
            *pHandle = (ClTxnCommHandleT)pKey;
            clLogDebug("CMN", NULL,
                    "New session creation successfull with key[0x%x:0x%x] msgType[%d] funcId[%d], tid[0x%x]",
                    pKey->destAddr.nodeAddress,
                    pKey->destAddr.portId,
                    pKey->msgType,
                    pKey->funcId, pKey->tId);
        }
        else
        {
            clLogError("CMN", "CCI", "Failed to add node, rc=[0x%x]", rc);
            *pHandle = 0x0;
            clHeapFree(pKey);
            clBufferDelete( &(pMsgHdr->payload));
            clHeapFree(pMsgHdr);
        }
    }
    /* UNLOCK */
    clTxnMutexUnlock (clTxnCommInterface->txnCommMutex); 

    if (CL_OK != rc)
    {
        clLogError("CMN", "CCI",
                "Failed to create session for [dest:0x%x:0x%x msg:0x%x tid:0x%x], rc:0x%x", 
                 destAddr.nodeAddress, destAddr.portId, msgType, tId, rc);
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Releases opened session (used at server end only
 */
ClRcT clTxnCommIfcSessionRelease(
        CL_IN   ClTxnCommHandleT    commHandle)
{
    ClRcT                   rc  = CL_OK;
    ClTxnMessageHeaderT     *pMsgHdr;

    CL_FUNC_ENTER();
    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    /* LOCK */
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK ) 
    {
        rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                            (ClCntKeyHandleT) commHandle, 
                            (ClCntDataHandleT *)&pMsgHdr);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock"));
        CL_FUNC_EXIT();
        return (rc);
    }

    if (CL_OK == rc)
    {
        pMsgHdr->sessionCount--;
    }

    /* UNLOCK */
    if (clOsalMutexUnlock (clTxnCommInterface->txnCommMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock, aborting"));
        rc = CL_ERR_UNSPECIFIED;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Specialized API to append transaction cmd/response to output buffer
 */
ClRcT clTxnCommIfcSessionAppendTxnCmd(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClTxnCmdT               *pTxnCmd)
{
    ClRcT                rc = CL_OK;
    ClUint32T            size = 0;
    ClTxnMessageHeaderT *pMsgHdr = NULL;

    CL_FUNC_ENTER();
    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }

    /* LOCK */
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK ) 
    {
        rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                            (ClCntKeyHandleT) commHandle, 
                            (ClCntDataHandleT *)&pMsgHdr);
        if(CL_OK != rc)
        {
            ClCntNodeHandleT    nhdl;
            ClTxnCommSessionKeyT key;
            clCntSizeGet(clTxnCommInterface->txnCommSessionList, &size);
            clLogError("CMN", NULL,
                    "Failed to get msgHeader using key[0x%x:0x%x] msgType[%d] funcId[%d]. Size of container [%d]",
                    ((ClTxnCommSessionKeyT *)commHandle)->destAddr.nodeAddress,
                    ((ClTxnCommSessionKeyT *)commHandle)->destAddr.portId,
                    ((ClTxnCommSessionKeyT *)commHandle)->msgType,
                    ((ClTxnCommSessionKeyT *)commHandle)->funcId,
                    size);
            if(size) /* For debugging and logging */
            {
                rc = clCntFirstNodeGet(clTxnCommInterface->txnCommSessionList,
                        &nhdl);
                if(CL_OK != rc)
                {
                    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
                    return rc;
                }

                rc = clCntNodeUserKeyGet(clTxnCommInterface->txnCommSessionList,
                        nhdl,
                        (ClCntKeyHandleT *)&key);
                if(CL_OK != rc)
                {
                    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
                    return rc;
                }
                clLogDebug("CMN", NULL,
                        "User key [0x%x:0x%x] msgType [%d] funcId[%d]",
                        key.destAddr.nodeAddress,
                        key.destAddr.portId,
                        key.msgType,
                        key.funcId);
            }


        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock"));
        CL_FUNC_EXIT();
        return (rc);
    }

    if (CL_OK == rc)
    {
        /* Marshall ClTxnCmdT into the payload initialized during session-creation */
        pMsgHdr->tid = pTxnCmd->txnId; 
        rc = VDECL_VER(clXdrMarshallClTxnCmdT, 4, 0, 0)(pTxnCmd, pMsgHdr->payload, 0);
    }

    if (CL_OK == rc)
    {
        pMsgHdr->msgCount++;
    }
    /* UNLOCK */
    if (clOsalMutexUnlock (clTxnCommInterface->txnCommMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock, aborting"));
        rc = CL_ERR_UNSPECIFIED;
    }

    if (CL_OK != rc)
    {
        if (NULL != pMsgHdr)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to append txn-cmd to payload for dest 0x%x:0x%x, rc:0x%x", 
                     pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, rc));
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid session or comm-handle. rc:0x%x", rc));
        }
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Add this message to the pay-load of the message being formed.
 */
ClRcT clTxnCommIfcSessionAppendPayload(
        CL_IN   ClTxnCommHandleT            commHandle,
        CL_IN   ClTxnCmdT                   *pTxnCmd,
        CL_IN   ClUint8T                    *pMsg,
        CL_IN   ClUint32T                   len)
{
    ClRcT                   rc          = CL_OK;
    ClTxnMessageHeaderT     *pMsgHdr    = NULL;
    
    CL_FUNC_ENTER();

    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    /* LOCK */
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK ) 
    {
        rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                            (ClCntKeyHandleT) commHandle, 
                            (ClCntDataHandleT *)&pMsgHdr);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock"));
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Append cmd followed by payload */
    if (CL_OK == rc)
    {
        /* Marshall ClTxnCmdT into thoe payload initialized during session-creation */
        pMsgHdr->tid = pTxnCmd->txnId;
        rc = VDECL_VER(clXdrMarshallClTxnCmdT, 4, 0, 0)(pTxnCmd, pMsgHdr->payload, 0);
    }

    if (CL_OK == rc)
    {
        pMsgHdr->msgCount++;
        rc = clBufferNBytesWrite(pMsgHdr->payload, pMsg, len);
    }

    /* UNLOCK */
    if (clOsalMutexUnlock (clTxnCommInterface->txnCommMutex) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock, aborting")); 
        rc = CL_ERR_UNSPECIFIED;
    }

    if (CL_OK != rc)
    {
        if ( NULL != pMsgHdr )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to append data to payload for dest 0x%x:0x%x, rc:0x%x", 
                     pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, rc));
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid handle or session. rc:0x%x", rc));
        }
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Cancel the communication session and release all allocated
 * memory
 */
ClRcT clTxnCommIfcSessionCancel(
        CL_IN   ClTxnCommHandleT    commHandle)
{

    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    /* Cancel this transaction-comm-session */
    /* LOCK */
    clTxnMutexLock(clTxnCommInterface->txnCommMutex);
    rc = clCntAllNodesForKeyDelete(clTxnCommInterface->txnCommSessionList, 
                                   (ClCntKeyHandleT)commHandle);
    if (CL_OK != rc)
    {
        clLogError("CMN", "CCI",  
                "Invalid session or comm-handle, rc[0x%x]", rc);
        rc = CL_GET_ERROR_CODE(rc);
    }
    /* UNLOCK */
    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Close this session and send the data to the intended destination
 */
ClRcT clTxnCommIfcSessionClose(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClUint8T                syncFlag)
{
    ClRcT                   rc  =   CL_OK;

    CL_FUNC_ENTER();

    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    /* LOCK */
    clTxnMutexLock(clTxnCommInterface->txnCommMutex);
    rc = _clTxnCommIfcSendMessage(commHandle, syncFlag);
    if(CL_OK != rc)
    {
        clLogError("CMN", "CCI",
                "Sending message failed with error [0x%x]", rc);
    }

    /* UNLOCK */
    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);

    /* Delete all entries
    clCntAllNodesForKeyDelete(clTxnCommInterface->txnCommSessionList, 
                              (ClCntKeyHandleT) commHandle);
    */
    return (rc);
}
/**
 * Internal Function
 */
ClRcT clTxnCommIfcReadMessage(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClBufferHandleT         outMsg)
{
    ClRcT                   rc  = CL_OK;
    ClTxnMessageHeaderT    *pMsgHdr;
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}};
    
    CL_FUNC_ENTER();

    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    if ( clOsalMutexLock (clTxnCommInterface->txnCommMutex) == CL_OK )
    { 
        rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                (ClCntKeyHandleT) commHandle, 
                (ClCntDataHandleT *)&pMsgHdr);

        if (CL_OK != rc)
        {
            clLogError("SER", NULL, 
                    "Unknown session, or invalid handle %p, rc:0x%x", 
                     (ClPtrT) commHandle, rc);
            CL_FUNC_EXIT();
            clTxnMutexUnlock (clTxnCommInterface->txnCommMutex);
            return CL_GET_ERROR_CODE(rc);
        }

        clLogDebug("CMN", "CCI",
                   "Closing session-handle destAddr[0x%x:0x%x] msg [0x%x]", 
                    ((ClTxnCommSessionKeyT *)commHandle)->destAddr.nodeAddress, 
                    ((ClTxnCommSessionKeyT *)commHandle)->destAddr.portId, 
                    ((ClTxnCommSessionKeyT *)commHandle)->msgType);
        if (pMsgHdr->sessionCount > 0)
        {
            clLogDebug("CMN", "CCI", 
                    "Incomplete session, count greater than 0, skipping now. Session handle destAddr[0x%x:0x%x] msgType[0x%x] funcId[0x%x]  session-count[0x%x]", 
                     pMsgHdr->destAddr.nodeAddress, 
                     pMsgHdr->destAddr.portId, pMsgHdr->msgType, 
                     pMsgHdr->funcId, pMsgHdr->sessionCount);
            CL_FUNC_EXIT();
            clTxnMutexUnlock (clTxnCommInterface->txnCommMutex);
            return CL_OK;
        }

        if (pMsgHdr->msgCount == 0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Ignorable session[0x%x:0x%x,0x%x,0x%x]. Skipping now",
                        pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, pMsgHdr->msgType, pMsgHdr->funcId));
            CL_FUNC_EXIT();
            clTxnMutexUnlock (clTxnCommInterface->txnCommMutex);
            return CL_OK;
        }

        _clTxnCopyMsgHeadToIDL(pMsgHdr, &msgHeadIDL);
        CL_ASSERT(msgHeadIDL.msgCount);

        if (CL_OK == rc)
        {
            rc = VDECL_VER(clXdrMarshallClTxnMessageHeaderIDLT, 4, 0, 0)(&msgHeadIDL, outMsg, 0);
        }

        if (CL_OK == rc)
        {
            ClUint32T   payloadSize;
            ClUint8T    *data;

            rc = clBufferLengthGet(pMsgHdr->payload, &payloadSize);

            if (CL_OK == rc)
            {
                rc = clBufferFlatten(pMsgHdr->payload, &data);
            }

            if (CL_OK == rc)
            {
                rc = clBufferNBytesWrite(outMsg, data, payloadSize);
            }
            clHeapFree(data);
        }
        /* Prepare for future sessions */
        /* This is not required now. Just use clBufferClear() 

        clBufferDelete ( &(pMsgHdr->payload) );
        clBufferCreate ( &(pMsgHdr->payload) );
        */
        clBufferClear(pMsgHdr->payload);
        pMsgHdr->msgCount = 0x0;

        clTxnMutexUnlock (clTxnCommInterface->txnCommMutex);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock"));
        rc = CL_ERR_UNSPECIFIED;
    }

    if (CL_OK != rc)
    {
       clLogError("CMN", "CCI", 
               "Error in reading the messages, rc[0x%x]", rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function. Lock txnCommSessionList before calling this function
 */
static ClRcT _clTxnCommIfcSendMessage(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClUint8T                syncFlag)
{
    ClRcT                   rc  = CL_OK;
    ClTxnMessageHeaderT     *pMsgHdr;
    ClTxnMessageHeaderIDLT    msgHeadIDL = {{0}};
    ClBufferHandleT  outMsg;
    
    CL_FUNC_ENTER();

    rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                            (ClCntKeyHandleT) commHandle, 
                            (ClCntDataHandleT *)&pMsgHdr);

    /* Close this session and send data to destination-address */
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Unknown session, or invalid handle %p, rc:0x%x", 
                 (ClPtrT) commHandle, rc));
        CL_FUNC_EXIT();
        return CL_GET_ERROR_CODE(rc);
    }

    clLogDebug("CMN", "CCI", "Closing session-handle [destAddr:0x%x:0x%x msg:0x%x]", 
                ((ClTxnCommSessionKeyT *)commHandle)->destAddr.nodeAddress, 
                ((ClTxnCommSessionKeyT *)commHandle)->destAddr.portId, 
                ((ClTxnCommSessionKeyT *)commHandle)->msgType);
    if (pMsgHdr->sessionCount > 0)
    {
        clLogDebug("CMN", "CCI", 
           "Incomplete session, skipping now. session-handle[0x%x:0x%x,0x%x,0x%x], session-count[0x%x]", 
             pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, 
             pMsgHdr->msgType, pMsgHdr->funcId, pMsgHdr->sessionCount);
        CL_FUNC_EXIT();
        return CL_OK;
    }

    if (pMsgHdr->msgCount == 0)
    {
        clLogDebug("CMN", "CCI", "Ignorable session[0x%x:0x%x,0x%x,0x%x]. Skipping now",
             pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, 
             pMsgHdr->msgType, pMsgHdr->funcId);
        CL_FUNC_EXIT();
        return CL_OK;
    }

    _clTxnCopyMsgHeadToIDL(pMsgHdr, &msgHeadIDL);
    CL_ASSERT(msgHeadIDL.msgCount);

    rc = clBufferCreate(&outMsg);
    if (CL_OK == rc)
    {
        rc = VDECL_VER(clXdrMarshallClTxnMessageHeaderIDLT, 4, 0, 0)(&msgHeadIDL, outMsg, 0);
    }

    if (CL_OK == rc)
    {
        ClUint32T   payloadSize = 0;
        ClUint8T    *data = NULL;

        rc = clBufferLengthGet(pMsgHdr->payload, &payloadSize);

        if (CL_OK == rc)
        {
            rc = clBufferFlatten(pMsgHdr->payload, &data);
        }

        if (CL_OK == rc)
        {
            rc = clBufferNBytesWrite(outMsg, data, payloadSize);
        }
        clHeapFree(data);

        /* Cleaning payload and resetting msgCount for the next transaction*/
        rc = clBufferClear(pMsgHdr->payload);
        if(CL_OK != rc)
            clLogError("CMN", "CCI",
                    "Error in clearing the buffer, rc=[0x%x]. This payload cannot be reused", rc);

        pMsgHdr->msgCount = 0x0;
    }

    if (CL_OK == rc)
    {
        ClIocAddressT           destAddr = {
            .iocPhyAddress      = pMsgHdr->destAddr
        };
        ClRmdOptionsT           rmdOptions = CL_RMD_DEFAULT_OPTIONS;
        ClUint32T               rmdFlag = CL_RMD_CALL_ATMOST_ONCE;

        rmdOptions.timeout = pMsgHdr->timeOut;
        rmdOptions.retries = CL_TXN_RMD_DFLT_RETRIES;
        rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Making RMD Call to dest 0x%x:0x%x funcId:0x%x", 
                       pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, pMsgHdr->funcId));

        if (syncFlag == CL_FALSE)
            rmdFlag |= CL_RMD_CALL_ASYNC;
#if 1
        if(pMsgHdr->fpCallback != NULL
                &&
            pMsgHdr->msgType == CL_TXN_MSG_CLIENT_REQ_TO_AGNT)
        {
            ClRmdAsyncOptionsT readAsyncOpt = {
                .fpCallback = pMsgHdr->fpCallback
            };
            ClBufferHandleT  incomingMsg;
            ClBufferHandleT  inMsg;
            rc = clBufferCreate(&incomingMsg);
            rc = clBufferCreate(&inMsg);
            
            rmdFlag |= CL_RMD_CALL_NEED_REPLY;

            _clTxnMarshallUnmarshall(&outMsg,&inMsg);     
            readAsyncOpt.pCookie = (void*)inMsg;
            
            rc = clRmdWithMsg(destAddr, pMsgHdr->funcId,
                    (ClBufferHandleT) outMsg, 
                    (ClBufferHandleT) incomingMsg, 
                    rmdFlag,
                    &rmdOptions, 
                    &readAsyncOpt);
        }
        else if (CL_TXN_MSG_MGR_CMD == pMsgHdr->msgType
                            &&
                 pMsgHdr->fpCallback != NULL )
        {
            ClRmdAsyncOptionsT readAsyncOpt = {
                .fpCallback = pMsgHdr->fpCallback
            };
            ClBufferHandleT  incomingMsg = NULL; /* Should be deleting this in the callback function which is clTxnAgentAsyncResponseRecv() */
            ClTxnAppCookieT  *pCookie = NULL;

            /* Free this in the callback function */
            pCookie = (ClTxnAppCookieT *)clHeapAllocate(sizeof(ClTxnAppCookieT));
            if(pCookie)
            {
                pCookie->addr = pMsgHdr->destAddr;
                pCookie->tid = pMsgHdr->tid;
            }
            rc = clBufferCreate(&incomingMsg);
            
            rmdFlag |= CL_RMD_CALL_NEED_REPLY;

            readAsyncOpt.pCookie = (ClPtrT)pCookie;
            
            rc = clRmdWithMsg(destAddr, pMsgHdr->funcId,
                    (ClBufferHandleT) outMsg, 
                    (ClBufferHandleT) incomingMsg, 
                    rmdFlag,
                    &rmdOptions, 
                    &readAsyncOpt);

        }
        else if (CL_TXN_MSG_CLIENT_REQ == pMsgHdr->msgType)
        {
            ClRmdAsyncOptionsT readAsyncOpt = {
                .fpCallback = pMsgHdr->fpCallback
            };
            clLogDebug("CMN", "CCI",
                    "Client request to server");
            ClBufferHandleT  incomingMsg = NULL; /* Should be deleting this in the callback function which is clTxnAgentAsyncResponseRecv() */
            if(pMsgHdr->fpCallback)
            {
                ClTxnTransactionIdT *pTid = NULL;

                pTid = (ClTxnTransactionIdT *)clHeapAllocate(sizeof(ClTxnTransactionIdT));
                if(!pTid)
                {
                    clLogError("CMN", "CCI",
                            "Memory allocation failed with error [0x%x]", rc);
                    return rc;
                }
                /* Setting up cookie to be used at the client side for txn failures */
                *pTid = pMsgHdr->tid;

                readAsyncOpt.pCookie = (ClPtrT)pTid;
            }
            rc = clBufferCreate(&incomingMsg);
            
            rmdFlag |= CL_RMD_CALL_NEED_REPLY;

            rc = clRmdWithMsg(destAddr, pMsgHdr->funcId,
                    (ClBufferHandleT) outMsg, 
                    (ClBufferHandleT) incomingMsg, 
                    rmdFlag,
                    &rmdOptions, 
                    &readAsyncOpt);
            if(CL_OK != rc)
            {
                clLogError("CMN", "CCI",
                        "Rmd failed with error [0x%x]", rc);
                return rc;
            }
            /*
            if(syncFlag == CL_TRUE)
            {
                if(pOutMsg)
                {
                    *pOutMsg = incomingMsg;
                    
                }
                else
                    clLogError("CMN", "CCI",
                            "Message handle passed is NULL");
            }
            */

        }
        else
        {
            clLogDebug("CMN", NULL,
                    "Sending RMD without response to [0x%x:0x%x]",
                    pMsgHdr->destAddr.nodeAddress,
                    pMsgHdr->destAddr.portId);
                    
            rc = clRmdWithMsg(destAddr, pMsgHdr->funcId,
                    (ClBufferHandleT) outMsg, 
                    0,
                    rmdFlag,
                    &rmdOptions, 
                    NULL);

        }
#endif
    }

    
    if(pMsgHdr->fpCallback) 
        pMsgHdr->fpCallback = NULL;
    rc = clBufferDelete (&outMsg);
    if (CL_OK != rc)
    {
        clLogError("CMN", "CCI",  
                "Error in deleting buffer, rc=[0x%x]", rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Close all sessions
 */
ClRcT clTxnCommIfcAllSessionClose()
{
    ClRcT               rc  = CL_OK;
    ClCntNodeHandleT    currNode;

    CL_FUNC_ENTER();

    /* Return from here if not initialized or already finalized */
    if(!clTxnCommInterface)
    {
        clLogInfo("CMN", "CCI", "Comm interface not initialized");
        return CL_ERR_NULL_POINTER;
    }
    clTxnMutexLock (clTxnCommInterface->txnCommMutex); 

    rc = clCntFirstNodeGet(clTxnCommInterface->txnCommSessionList, &currNode);

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClCntKeyHandleT         key;

        rc = clCntNodeUserKeyGet(clTxnCommInterface->txnCommSessionList, 
                                  currNode, &key);
        if (CL_OK == rc)
        {

            rc = _clTxnCommIfcSendMessage((ClTxnCommHandleT) key, CL_FALSE);
        }

        tNode = currNode;
        rc = clCntNextNodeGet(clTxnCommInterface->txnCommSessionList, tNode, &currNode);
    }

    clTxnMutexUnlock (clTxnCommInterface->txnCommMutex);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        rc = CL_OK;

    /* Delete all nodes
    clCntAllNodesDelete(clTxnCommInterface->txnCommSessionList);
    */

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnCommIfcAllSessionCancel()
{
    ClRcT   rc = CL_OK;

    CL_FUNC_ENTER();

    rc = clCntAllNodesDelete(clTxnCommInterface->txnCommSessionList);

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete all sessions. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**************************************************************************
 *                      Private Functions                                 *
 **************************************************************************/

static ClInt32T 
_clTxnCommHandleCmp(
        CL_IN   ClCntKeyHandleT key1, 
        CL_IN   ClCntKeyHandleT key2)
{
    ClInt32T    cmp = 0x0;
    ClTxnCommSessionKeyT    *pKey1 = (ClTxnCommSessionKeyT *) key1;
    ClTxnCommSessionKeyT    *pKey2 = (ClTxnCommSessionKeyT *) key2;


    cmp = memcmp(pKey1, pKey2, sizeof(ClTxnCommSessionKeyT));

    clLogTrace("CMN", "CCI", 
        "Comparing key1[destAddr:0x%x:0x%x msg:0x%x funcId:0x%x tid:0x%x] with key2[destAddr:0x%x:0x%x msg:0x%x funcId:0x%x tid:0x%x] cmp:0x%x", 
         pKey1->destAddr.nodeAddress, pKey1->destAddr.portId, pKey1->msgType, pKey1->funcId, pKey1->tId,
         pKey2->destAddr.nodeAddress, pKey2->destAddr.portId, pKey2->msgType, pKey2->funcId, pKey1->tId, 
         cmp);
    return cmp;
}

#if 0
static ClUint32T
_clTxnCommHandleHashFn(
        CL_IN   ClCntKeyHandleT key)
{
    return (key % 10);
}
#endif

static void 
_clTxnCommIfcEntryDel(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnMessageHeaderT     *pMsgHdr = (ClTxnMessageHeaderT *) userData;

    if ( pMsgHdr->payload != 0 ) 
        clBufferDelete ( &(pMsgHdr->payload) );

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting session-handle [destAddr:0x%x:0x%x msg:0x%x]", 
                ((ClTxnCommSessionKeyT *)userKey)->destAddr.nodeAddress, 
                ((ClTxnCommSessionKeyT *)userKey)->destAddr.portId, 
                ((ClTxnCommSessionKeyT *)userKey)->msgType));


    clHeapFree (pMsgHdr);
    clHeapFree ((ClPtrT) userKey);
}

static ClRcT _clTxnMarshallUnmarshall(
        CL_IN   ClBufferHandleT         *pInMsg,
        CL_OUT   ClBufferHandleT         *pOutMsg)
{
    ClTxnMessageHeaderIDLT     msgHeader = {{0}};
    ClRcT                   rc  = CL_OK;

    rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(*pInMsg, &msgHeader);
    if(CL_OK != rc)
    {
        clLogError("CMN", NULL, 
                "Error in unmarshalling message header, rc [0x%x]", rc);
        return rc;
    }
    rc = VDECL_VER(clXdrMarshallClTxnMessageHeaderIDLT, 4, 0, 0)(&msgHeader, *pOutMsg, 0);
    if(CL_OK != rc)
    {
        clLogError("CMN", NULL, 
                "Error in marshalling message header, rc [0x%x]", rc);
        return rc;
    }

    while ( (CL_OK == rc) && (msgHeader.msgCount > 0) )
    {
        ClTxnCmdT   tCmd;

        msgHeader.msgCount--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(*pInMsg, &tCmd);
        switch (tCmd.cmd)
        {
            case CL_TXN_CMD_READ_JOB:
                {
                    ClTxnDefnT          *pTxnDefn;
                    ClTxnAppJobDefnT    *pTxnAppJob;

                    rc = clTxnStreamTxnDataUnpack(*pInMsg, &pTxnDefn, &pTxnAppJob);
                    rc = VDECL_VER(clXdrMarshallClTxnCmdT, 4, 0, 0)(&tCmd,*pOutMsg, 0);
                    clTxnDefnDelete(pTxnDefn);
                    clTxnAppJobDelete(pTxnAppJob);
                }
                break;

            default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid comamnd received 0x%x", tCmd.cmd));
                rc = CL_ERR_INVALID_PARAMETER;
                break;
        }
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Result of marshalling and unmarsalling rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return rc;

}
void clTxnCommCookieFree(ClPtrT cookie)
{
    clHeapFree(cookie);
}

ClRcT  clTxnCommIfcDeferSend(
        ClTxnCommHandleT commHandle,
        ClRmdResponseContextHandleT rmdDeferHdl,
        ClBufferHandleT             outMsg)
{
    ClRcT                   rc  = CL_OK;
    ClTxnMessageHeaderT     *pMsgHdr;
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}};
    
    CL_FUNC_ENTER();

    clTxnMutexLock(clTxnCommInterface->txnCommMutex);

    rc = clCntDataForKeyGet(clTxnCommInterface->txnCommSessionList, 
                            (ClCntKeyHandleT) commHandle, 
                            (ClCntDataHandleT *)&pMsgHdr);

    /* Close this session and send data to destination-address */
    if (CL_OK != rc)
    {
        clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
        clLogError("CMN", "CCI", 
                "Unknown session, or invalid handle [%p], rc[0x%x]", 
                 (ClPtrT) commHandle, rc);
        CL_FUNC_EXIT();
        return CL_GET_ERROR_CODE(rc);
    }

    if (pMsgHdr->sessionCount > 0)
    {
        clLogDebug("CMN", "CCI",
            "Incomplete session, skipping now. session-handle[0x%x:0x%x,0x%x,0x%x], session-count[0x%x]", 
             pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, pMsgHdr->msgType, pMsgHdr->funcId, 
             pMsgHdr->sessionCount);
        CL_FUNC_EXIT();
        clTxnMutexUnlock(clTxnCommInterface->txnCommMutex); 
        return CL_OK;
    }

    if (pMsgHdr->msgCount == 0)
    {
        clLogDebug("CMN", "CCI", 
                "Ignorable session[0x%x:0x%x,0x%x,0x%x]. Skipping now",
                 pMsgHdr->destAddr.nodeAddress, pMsgHdr->destAddr.portId, 
                 pMsgHdr->msgType, pMsgHdr->funcId);
        CL_FUNC_EXIT();
        clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
        return CL_OK;
    }

    if(!outMsg)
    {
        rc = clBufferCreate(&outMsg);
        if(CL_OK != rc)
        {
            clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
            clLogError("CMN", "CCI",
                    "Buffer creation failed with error [0x%x]", rc);
            return rc;
        }
    }

    _clTxnCopyMsgHeadToIDL(pMsgHdr, &msgHeadIDL);
    CL_ASSERT(msgHeadIDL.msgCount);

    rc = VDECL_VER(clXdrMarshallClTxnMessageHeaderIDLT, 4, 0, 0)(&msgHeadIDL, outMsg, 0);

    if (CL_OK == rc)
    {
        ClUint32T   payloadSize = 0;
        ClUint8T    *data = NULL;

        rc = clBufferLengthGet(pMsgHdr->payload, &payloadSize);

        if (CL_OK == rc)
        {
            rc = clBufferFlatten(pMsgHdr->payload, &data);
        }

        if (CL_OK == rc)
        {
            rc = clBufferNBytesWrite(outMsg, data, payloadSize);
        }

        clHeapFree(data);
    }

    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);
    if (CL_OK == rc)
    {
        rc = clRmdSyncResponseSend(rmdDeferHdl, outMsg, CL_OK);
        if(CL_OK != rc)
        {
            clLogError("CMN", "CCI",
                    "Sending response to client failed with error [0x%x]",
                    rc);
        }
        else
            clLogDebug("CMN", "CCI",
                    "Successfully sent response to client");
    }
    clTxnMutexLock(clTxnCommInterface->txnCommMutex);
    rc = clBufferClear(pMsgHdr->payload);
    if(CL_OK != rc)
        clLogError("CMN", "CCI",
                "Error in clearing the buffer, rc=[0x%x]. This payload cannot be reused", rc);

    pMsgHdr->msgCount = 0x0;
    /* Make the callback NULL. This is necessary as the same message type can
       have different protocols. Ex: From server to agent, for REMOVE cmd, it 
       is async WITHOUT reply and for others, it is asyn WITH reply.
       However, during session create, the func callback has to be assigned
       everytime
    */
    if(pMsgHdr->fpCallback) 
        pMsgHdr->fpCallback = NULL;
    clTxnMutexUnlock(clTxnCommInterface->txnCommMutex);

    CL_FUNC_EXIT();
    return (rc);
}
        
/* Functions to copy back and forth IDL structure */
void 
_clTxnCopyMsgHeadToIDL(ClTxnMessageHeaderT *pMsgHead, 
                       ClTxnMessageHeaderIDLT *pMsgHeadIDL)
{
    memcpy(&pMsgHeadIDL->version, &pMsgHead->version, sizeof(pMsgHeadIDL->version));
    pMsgHeadIDL->msgType = pMsgHead->msgType;
    memcpy(&pMsgHeadIDL->srcAddr, &pMsgHead->srcAddr, sizeof(pMsgHeadIDL->srcAddr));
    pMsgHeadIDL->srcStatus = pMsgHead->srcStatus;
    pMsgHeadIDL->msgCount = pMsgHead->msgCount;
}

void
_clTxnCopyIDLToMsgHead(ClTxnMessageHeaderT *pMsgHead, 
                       ClTxnMessageHeaderIDLT *pMsgHeadIDL)
{
    memcpy(&pMsgHead->version, &pMsgHeadIDL->version, sizeof(pMsgHead->version));
    pMsgHead->msgType = pMsgHeadIDL->msgType;
    memcpy(&pMsgHead->srcAddr, &pMsgHeadIDL->srcAddr, sizeof(pMsgHead->srcAddr));
    pMsgHead->srcStatus = pMsgHeadIDL->srcStatus;
    pMsgHead->msgCount = pMsgHeadIDL->msgCount;
}
