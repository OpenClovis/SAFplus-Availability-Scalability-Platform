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
#include <clLogCommon.h>
#include <clLogDebug.h>
#include <clLogClient.h>
#include <clLogClientHandle.h>
#include <clLogClientHandler.h>
#include <clLogClientStream.h>

/*FIXME: These declaration will go */
extern ClRcT
clLogStreamClose(ClLogStreamHandleT hStream);

ClRcT
clLogClntHandleTypeGet(ClLogHandleT      hLog,
                       ClLogHandleTypeT  *pType)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInfo        = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter: %#llX", hLog));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    *pType = pInfo->type;

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit: Got type: %d", *pType));
    return rc;
}

ClRcT
clLogInitHandleCheckNCbValidate(ClLogHandleT hLog)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInfo        = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_LOG_INIT_HANDLE != pInfo->type )
    {
        CL_LOG_DEBUG_ERROR(("HandleType Mismatch"));

        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                       CL_OK);
        return CL_LOG_RC(CL_ERR_INVALID_HANDLE);
    }

    if( (NULL == pInfo->pCallbacks) || 
        (NULL == pInfo->pCallbacks->clLogRecordDeliveryCb) )
    {
        CL_LOG_DEBUG_ERROR(("record delivery callback is NULL"));
        return CL_LOG_RC(CL_ERR_NO_CALLBACK);
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit: Matched"));
    return rc;
}

ClRcT
clLogHandleCheck(ClLogHandleT      hLog,
                 ClLogHandleTypeT  type)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInfo        = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter: %d", type));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    if( type != pInfo->type )
    {
        CL_LOG_DEBUG_ERROR(("HandleType Mismatch"));

        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                       CL_OK);
        return CL_LOG_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit: Matched"));
    return rc;
}

ClRcT
clLogHandleDBGet(ClLogClntEoDataT  **ppClntEoEntry,
                 ClBoolT           *pAddedEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pAddedEntry = CL_FALSE;
    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_CLIENT_EO_ENTRY_KEY,
                            (void **) ppClntEoEntry);
    if( (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) )
    {
        /*
         * nn - Its a potential race condition. Problem might come if multiple
         * threads simultaneously access this code.
         * A clean solution need to be found.
         */
        rc = clLogClntEoEntryInstall(pEoObj, ppClntEoEntry);
        *pAddedEntry = (CL_OK == rc) ? CL_TRUE : CL_FALSE;
    }

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x], *pAddedEntry: %d", rc, *pAddedEntry));
    return rc;
}

ClRcT
clLogHandleInitHandleCreate(const ClLogCallbacksT  *pCallbacks,
                            ClLogHandleT           *phLog,
                            ClBoolT                *pAddedEntry)
{
    ClRcT               rc            = CL_OK;
    ClLogClntEoDataT    *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogHandleDBGet(&pClntEoEntry, pAddedEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCreate(pClntEoEntry->hClntHandleDB,
                        sizeof(ClLogInitHandleDataT), phLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCreate: rc[0x %x]", rc));
        if( CL_TRUE == *pAddedEntry )
        {
            CL_LOG_CLEANUP(clLogClntEoUninstall(&pClntEoEntry), CL_OK);
        }
        return rc;
    }

    rc = clLogHandleInitHandleInitialize(pClntEoEntry->hClntHandleDB,
                                         *phLog, pCallbacks);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB,
                                       *phLog), CL_OK);
        *phLog = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == *pAddedEntry )
        {
            CL_LOG_CLEANUP(clLogClntEoUninstall(&pClntEoEntry), CL_OK);
        }
        return rc;
    }

    pClntEoEntry->initCount++;
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleInitHandleInitialize(ClHandleDatabaseHandleT  hClntHandleDB,
                                ClLogHandleT             hLog,
                                const ClLogCallbacksT    *pCallbacks)
{
    ClRcT                 rc        = CL_OK;
    ClLogInitHandleDataT  *pInitData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clHandleCheckout(hClntHandleDB, hLog, (void **) (&pInitData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }
    pInitData->type = CL_LOG_INIT_HANDLE;

#if 0
    /* FIXME: Will come with SAF wrapper */
    rc = clDispatchRegister(&(pInitData->hDispatch), *phLog,
                            clLogDispatchWrapperCb, clLogDispatchQDestroyCb);
    if (rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clDispatchRegister(): rc[0x %x]",rc));
        return rc;
    }
#endif

    rc = clBitmapCreate(&(pInitData->hStreamBitmap), 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(hClntHandleDB, hLog), CL_OK);
        return rc;
    }

    if( NULL != pCallbacks )
    {
        pInitData->pCallbacks = clHeapCalloc(1, sizeof(ClLogCallbacksT));
        if( NULL == pInitData->pCallbacks)
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            CL_LOG_CLEANUP(clBitmapDestroy(pInitData->hStreamBitmap), CL_OK);
            CL_LOG_CLEANUP(clHandleCheckin(hClntHandleDB, hLog), CL_OK);
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        pInitData->pCallbacks->clLogFilterSetCb
            = pCallbacks->clLogFilterSetCb;
        pInitData->pCallbacks->clLogStreamOpenCb
            = pCallbacks->clLogStreamOpenCb;
        pInitData->pCallbacks->clLogRecordDeliveryCb
            = pCallbacks->clLogRecordDeliveryCb;
#if 0
        /* FIXME: Right now this call is not supported */
        pInitData->pCallbacks->clLogWriteCallback
            = pCallbacks->clLogWriteCallback;
#endif
    }

    rc = clHandleCheckin(hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pInitData->hStreamBitmap), CL_OK);
        CL_LOG_CLEANUP(clHandleCheckin(hClntHandleDB, hLog), CL_OK);
        clHeapFree(pInitData->pCallbacks);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogHandleCleanupCb(void  *pData)
{
    ClLogInitHandleDataT *pInit = pData;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_INIT_HANDLE == pInit->type )
    {
        CL_LOG_CLEANUP(clBitmapDestroy(pInit->hStreamBitmap), CL_OK);
        if( NULL != pInit->pCallbacks)
        {
            clHeapFree(pInit->pCallbacks);
            pInit->pCallbacks = NULL;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
}

ClRcT
clLogHandleStreamHandleCreate(ClHandleT         hLog,
                              ClCntNodeHandleT  hClntStreamNode,
                              ClLogHandleT     *phStream)
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK( (NULL == phStream), CL_LOG_RC(CL_ERR_NULL_POINTER) );

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCreate(pClntEoEntry->hClntHandleDB,
                        sizeof(ClLogStreamHandleDataT), phStream);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogHandleStreamDataInitialize(pClntEoEntry->hClntHandleDB,
                                         hLog, *phStream, hClntStreamNode);
    if( CL_OK != rc )
    {
        clHandleDestroy(pClntEoEntry->hClntHandleDB, *phStream);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleStreamDataInitialize(ClHandleDatabaseHandleT  hClntHandleDB,
                                ClLogHandleT             hLog,
                                ClLogStreamHandleT       hStream,
                                ClCntNodeHandleT         hClntStreamNode)
{
    ClRcT                   rc     = CL_OK;
    ClLogStreamHandleDataT  *pData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter:"));

    rc = clHandleCheckout(hClntHandleDB, hStream, (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    pData->type            = CL_LOG_STREAM_HANDLE;
    pData->hLog            = hLog;
    pData->hClntStreamNode = hClntStreamNode;

    rc = clHandleCheckin(hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleInitHandleStreamAdd(ClLogHandleT        hLog,
                               ClLogStreamHandleT  hStream)
{
    ClRcT                 rc             = CL_OK;
    ClLogInitHandleDataT  *pInfo         = NULL;
    ClLogClntEoDataT      *pClntEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBitmapBitSet(pInfo->hStreamBitmap, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleStreamHandleClose(ClLogStreamHandleT  hStream,
                             ClBoolT             notifySvr)
{
    ClRcT                   rc              = CL_OK;
    ClLogClntEoDataT        *pClntEoEntry   = NULL;
    ClLogStreamHandleDataT  *pData          = NULL;
    ClLogHandleT            hLog            = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT        hClntStreamNode = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pClntEoEntry->clntStreamTblLock);

    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream,
                          (void **) (&pData));
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    hLog            = pData->hLog;
    hClntStreamNode = pData->hClntStreamNode;

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleDestroy(pClntEoEntry->hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);
        CL_LOG_DEBUG_ERROR(("clHandleDestroy(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_TRUE == notifySvr )
    {
        rc = clLogHandleInitHandleStreamRemove(hLog, hStream);
        if( CL_OK != rc )
        {
            clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);
            return rc;
        }
    }

    CL_LOG_CLEANUP(clLogClntStreamCloseLocked(pClntEoEntry, hClntStreamNode, hStream, notifySvr),
                   CL_OK);

    clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleInitHandleStreamRemove(ClLogHandleT        hLog,
                                  ClLogStreamHandleT  hStream)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInfo        = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBitmapBitClear(pInfo->hStreamBitmap, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB,
                                       hLog), CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleOpenStreamCloseCb(ClBitmapHandleT  hBitmap,
                             ClUint32T        bitNum,
                             void             *pCookie)
{
    ClRcT             rc         = CL_OK;
    ClLogHandleTypeT  handleType = 0;

    CL_LOG_DEBUG_TRACE(("Enter: %d", bitNum));

    rc = clLogClntHandleTypeGet(bitNum, &handleType);
    if( CL_OK != rc )
    {
        return rc;
    }

    switch(handleType)
    {
        case CL_LOG_STREAM_HANDLE:
            {
                rc = clLogStreamClose(bitNum);
                break;
            }
        case CL_LOG_HANDLER_HANDLE:
            {
                clLogHandlerDeregister(bitNum);
                break;
            }
        case CL_LOG_INIT_HANDLE:
        case CL_LOG_FILE_HANDLE:
            {
                CL_LOG_DEBUG_ERROR(("Init handleType: Invalid here"));
                break;
            }
    }
    if(CL_OK != rc)
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogHandleInitHandleDestroy(ClLogHandleT  hLog)
{
    ClRcT                 rc            = CL_OK;
    ClLogClntEoDataT      *pClntEoEntry = NULL;
    ClLogInitHandleDataT  *pData        = NULL;
    ClPtrT                pCookie       = NULL;
    ClIocPortT            myPort        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBitmapWalkUnlocked(pData->hStreamBitmap,
                              clLogHandleOpenStreamCloseCb, pCookie);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapWalkUnlocked(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog)
                       , CL_OK);
        return rc;
    }

    if( NULL != pData->pCallbacks )
    {
        clHeapFree(pData->pCallbacks);
        pData->pCallbacks = NULL;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleDestroy(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleDestroy(): rc[0x %x]", rc));
        return rc;
    }

    pClntEoEntry->initCount--;
    if( 0 == pClntEoEntry->initCount )
    {
        /* Last finalize */
        clEoMyEoIocPortGet(&myPort);
        if( myPort != CL_IOC_LOG_PORT )
        {
            CL_LOG_CLEANUP(clLogClntStdStreamClose(nStdStream), CL_OK);
            CL_LOG_HANDLE_APP = CL_HANDLE_INVALID_VALUE;
            CL_LOG_HANDLE_SYS = CL_HANDLE_INVALID_VALUE;
        }
        CL_LOG_CLEANUP(clLogClntEoUninstall(&pClntEoEntry), CL_OK);
        //CL_LOG_CLEANUP(clLogDbgFileClose(), CL_OK);
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHandleHandlerHandleInit(ClHandleDatabaseHandleT  hClntHandleDB,
                                 ClLogStreamHandleT       hHandler, 
                                 ClLogHandleT             hLog,
                                 ClCntNodeHandleT         hClntHandlerNode, 
                                 ClIocMulticastAddressT   streamMcastAddr, 
                                 ClLogStreamHandlerFlagsT handlerFlag)
{
    ClRcT                          rc = CL_OK;
    ClLogStreamHandlerHandleDataT  *pData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clHandleCheckout(hClntHandleDB, hHandler, (void *) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout: rc[0x %x]", rc));
        return rc;
    }

    pData->type             = CL_LOG_HANDLER_HANDLE;
    pData->hLog             = hLog;
    pData->hClntHandlerNode = hClntHandlerNode;
    pData->handlerFlag      = handlerFlag;
    pData->status           = CL_TRUE;
    pData->streamMcastAddr  = streamMcastAddr;

    rc = clHandleCheckin(hClntHandleDB, hHandler);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin: rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
/*
 * clLogClntHandleHandlerHandleCreate()
 *  - Create the Stream Handler handle
 *  - Stores the bit map
 */

ClRcT
clLogClntHandleHandlerHandleCreate(ClLogHandleT              hLog,
                                   ClCntNodeHandleT          hClntHandlerNode,
                                   ClIocMulticastAddressT    streamMcastAddr, 
                                   ClLogStreamHandlerFlagsT  handlerFlag,
                                   ClLogStreamHandleT        *phStream)
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCreate(pClntEoEntry->hClntHandleDB,
                        sizeof(ClLogStreamHandlerHandleDataT), phStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCreate: rc[0x %x]", rc));
        return rc;
    }

    rc = clLogClntHandleHandlerHandleInit(pClntEoEntry->hClntHandleDB,
                                          *phStream, hLog, hClntHandlerNode,
                                          streamMcastAddr, handlerFlag);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB,
                                       *phStream), CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHandleHandlerHandleRemove(ClLogStreamHandleT  hHandler)
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleDestroy(pClntEoEntry->hClntHandleDB, hHandler); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleDestroy(): rc[0x %x]", rc));
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandleInitHandleHandlerRemove(ClLogHandleT        hLog,
                                   ClLogStreamHandleT  hStream)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pData        = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBitmapBitClear(pData->hStreamBitmap, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                       CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

#if 0
void
clLogDispatchWrapperCb(ClLogHandleT        hLog,
                       ClLogCallbackTypeT  cbType,
                       void*               cbData)
{
    ClLogClntEoDataT      *pClntEoEntry = NULL;
    ClLogInitHandleDataT  *pInitData    = NULL;
    ClRcT                 rc            = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pInitData)); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return;
    }

    switch(cbType)
    {
    case CL_LOG_STREAM_OPEN_CB:
        pInitData->pCallbacks->clLogStreamOpenCb(
            ((ClLogStreamOpenCbDataT *) cbData)->invocation,
            ((ClLogStreamOpenCbDataT *) cbData)->hStream,
            ((ClLogStreamOpenCbDataT *) cbData)->rc);
        break;

    case CL_LOG_FILTER_SET_CB:
        pInitData->pCallbacks->clLogFilterSetCb(
            ((ClLogFilterSetCbDataT *) cbData)->hStream,
            ((ClLogFilterSetCbDataT *) cbData)->filter);
        break;
#if 0
    case CL_LOG_WRITE_CB:
        pInitData->pCallbacks->clLogStreamOpenCb(
            (ClLogWriteCbDataT *) (cbData)->
            (ClLogWriteCbDataT *) (cbData)->
            (ClLogWriteCbDataT *) (cbData)-> );
        break;

    case CL_LOG_RECORD_DELIVERY_CB:
        pInitData->pCallbacks->clLogStreamOpenCb(
            (ClLogRecordDeliveryCbDataT *) (cbData)->
            (ClLogrecordDeliveryCbDataT *) (cbData)->
            (ClLogRecordDeliveryCbDataT *) (cbData)-> );
        break;
#endif
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

void
clLogDispatchQDestroyCb(ClLogCallbackTypeT  cbType,
                        void*               cbData)
{
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

ClRcT
clLogSelectionObjectGet(ClLogHandleT        hLog,
                        ClSelectionObjectT  *selectionObject)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInitData    = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Entry"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog ,
                          (void*) &pInitData);
    if (rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogClntHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = clDispatchSelectionObjectGet(pInitData->hDispatch, selectionObject);
    if (rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clDispatchSelectionObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog); 
    if (rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogDispatch (ClLogHandleT      hLog,
               ClDispatchFlagsT  dispatchFlags)
{
    ClRcT                 rc            = CL_OK;
    ClLogInitHandleDataT  *pInitData    = NULL;
    ClLogClntEoDataT      *pClntEoEntry = NULL;
    ClHandleT             hDispatch     = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog ,
                          (void*) &pInitData);
    if (rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    hDispatch = pInitData->hDispatch;
    
    /* 
     * Since dispatch can be blocking and it would return only after
     * finalize, we will do the checkin before getting into dispatch
     */

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if ( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }
    
    rc = clDispatchCbDispatch(hDispatch, dispatchFlags);
    if ( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clDispatchCbDispatch(): rc[0x %x]", rc));
        return rc;
    }

    return rc;
}

#endif
