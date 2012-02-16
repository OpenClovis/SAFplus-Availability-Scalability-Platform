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

#include <clIocErrors.h>
#include <clLogStreamOwner.h>
#include <LogPortSvrClient.h>
#include <AppclientPortclientClient.h>
#include <LogPortSvrServer.h>

/*
 * Function - clLogStreamOwnerSvrIntimate()
 *  - Initialize the Idl handle
 *  - Get the msgId bitmap pointer and length.
 *  - Get the compId bitmap pointer and length.
 *  - Make a call to specified node.
 */
ClRcT
clLogStreamOwnerSvrIntimate(ClCntKeyHandleT   key,
                            ClCntDataHandleT  data, 
                            ClCntArgHandleT   arg,
                            ClUint32T         size)
{
    ClRcT                 rc          = CL_OK;
    ClLogCompKeyT         *pCompKey   = (ClLogCompKeyT *) key;
    ClLogSOCommonCookieT  *pData      = (ClLogSOCommonCookieT *) arg;
    ClLogFilterT          filterInfo  = {0};
    ClLogFilterInfoT      *pFilter    = pData->pStreamFilter;    
    ClLogStreamKeyT       *pStreamKey = pData->pStreamKey;
    ClIocAddressT         nodeAddr    = {{0}};
    ClIdlHandleT          hLogIdl     = CL_HANDLE_INVALID_VALUE;
    ClLogStreamScopeT     streamScope = 0;  
    ClUint32T             length      = 0;
    ClLogSOCompDataT      *pCompData  = (ClLogSOCompDataT *) data;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    if( 0 == pCompData->refCount )
    {
        /*
         *  should not send info to streamHdlrs, this is only for the loggers
         */  
        return CL_OK;
    }
    nodeAddr.iocPhyAddress.nodeAddress = pCompKey->nodeAddr;
    nodeAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
    rc = clLogIdlHandleInitialize(nodeAddr, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamScopeGet(&pStreamKey->streamScopeNode, &streamScope);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }
    if( CL_TRUE == pData->sendFilter )
    {
        rc = clBitmap2BufferGet(pFilter->hMsgIdMap, &length, &filterInfo.pMsgIdSet);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            return rc;
        }
        filterInfo.msgIdSetLength = length;
        rc = clBitmap2BufferGet(pFilter->hCompIdMap, &length, &filterInfo.pCompIdSet);
        if( CL_OK != rc )
        {
            clHeapFree(filterInfo.pMsgIdSet);
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            return rc;
        }
        filterInfo.compIdSetLength = length;
        filterInfo.severityFilter  = pFilter->severityFilter;
        if(pCompKey->nodeAddr == clIocLocalAddressGet())
        {
            rc = VDECL_VER(clLogSvrFilterSet, 4, 0, 0)(&pStreamKey->streamName, streamScope,
                                                       &pStreamKey->streamScopeNode, &filterInfo);
        }
        else
        {
            rc = VDECL_VER(clLogSvrFilterSetClientAsync, 4, 0, 0)(hLogIdl, &pStreamKey->streamName,
                                                                  streamScope, &pStreamKey->streamScopeNode,
                                                                  &filterInfo, NULL, 0);
        }
        clHeapFree(filterInfo.pCompIdSet);
        clHeapFree(filterInfo.pMsgIdSet);
    }
    else
    {
        rc = VDECL_VER(clLogSvrStreamHandleFlagsUpdateClientAsync, 4, 0, 0)(hLogIdl, &pStreamKey->streamName,
                                                                            streamScope,
                                                                            &pStreamKey->streamScopeNode,
                                                                            pData->handlerFlags,
                                                                            pData->setFlags, NULL, 0);
    }
    if( CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE )
    {
        CL_LOG_DEBUG_TRACE(("Error code : %x  %d", rc, nodeAddr.iocPhyAddress.nodeAddress));
        pData->nodeAddr = nodeAddr.iocPhyAddress.nodeAddress;
        rc = CL_OK;
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerMulticastCloseNotify(ClIocMulticastAddressT  streamMcastAddr)
{
    ClRcT          rc           = CL_OK;
    ClIocAddressT  muticastAddr = {{0}};
    ClIdlHandleT   hIdlHdl      = CL_HANDLE_INVALID_VALUE;
    ClUint8T       charVar      = '\0';

    CL_LOG_DEBUG_TRACE(("Enter: %llx", streamMcastAddr)); 

    muticastAddr.iocMulticastAddress = streamMcastAddr;
    rc = clLogIdlHandleInitialize(muticastAddr, &hIdlHdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)(hIdlHdl, streamMcastAddr, 
                                                 0, CL_IOC_RESERVED_ADDRESS, 
                                                 0, 0, 1, &charVar, NULL, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)():"
                    "rc[0x %x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);
        return rc;
    } 
    CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
