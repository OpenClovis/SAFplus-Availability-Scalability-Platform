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
 * ModuleName  : rmd                                                           
 * File        : clRmd.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module implements the core functionality of RMD.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include "clEoApi.h"
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clRmdApi.h>
#include <clLogApi.h>
#include "clRmdMain.h"
#include "clDebugApi.h"
#include "clRmdConfigApi.h"
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <unistd.h>
#include <clEoIpi.h>
#include <clIocIpi.h>
#include <clList.h>
#include <clNodeCache.h>
#include <clHash.h>
#include <clNetwork.h>
#ifdef DMALLOC
# include "dmalloc.h"
#endif
#include <sys/time.h>

#ifdef MORE_CODE_COVERAGE
# include<clCodeCovStub.h>
#endif

#define CL_IOC_IS_ADDRESS_PHYSICAL(x)   CL_TRUE

ClUint32T clRmdPrivateDataKey;

extern ClRcT clRmdPrivateDataSet(void *data);
extern ClRcT clRmdPrivateDataGet(void **data);

extern ClRcT clEoGetRemoteObjectAndBlock(ClUint16T remoteObj,
                                         ClEoExecutionObjT **pRemoteEoObj);
extern ClRcT clEoRemoteObjectUnblock(ClEoExecutionObjT *remoteEoObj);
static ClRcT clRmdWithMessage(ClIocAddressT remoteObjAddr, 
                              ClVersionT *version,
                              ClUint32T funcId,
                              ClBufferHandleT inMsgHdl,
                              ClBufferHandleT outMsgHdl, ClUint32T flags,
                              ClRmdOptionsT *pOptions,
                              ClRmdAsyncOptionsT *pAsyncOptions);

extern ClRcT clRmdCreateRmdMessageToSend(ClVersionT *version,
                                         ClUint32T funcId,
                                         ClBufferHandleT inMsgHdl,
                                         ClUint32T flags, ClUint64T msgId,
                                         ClUint32T dataLen,
                                         ClUint32T payLoadLen,
                                         ClUint32T maxRetries,
                                         ClUint32T callTimeOut,
                                         ClUint32T *hdrLen);

static ClRcT clRmdSyncSendAndReplyReceive(ClEoExecutionObjT *pThis,
                                          ClRmdObjT *pRmdObject,
                                          ClUint32T hdrLen,
                                          ClBufferHandleT outMsgHdl,
                                          ClRmdOptionsT *pOptions,
                                          ClUint32T nwMsgId,
                                          ClBufferHandleT inMsg,
                                          ClIocAddressT destAddr,
                                          ClUint32T flags);

ClRmdLibConfigT _clRmdConfig;
static ClUint32T rmdInitDone = CL_FALSE;
static ClUint32T gPrintHeaders = 0;

/*
 * static ClUint32T timeDiff (ClTimerTimeOutT old, ClTimerTimeOutT new,
 * ClTimerTimeOutT *pTimeDiff);
 */

/*
 * Version Related Definitions
 */
static ClVersionT gRmdAppToClientVersionsSupported[] = {
    /*
     * Add further Versions
     */
    {'B', 0x1, 0x1},
};

static ClVersionDatabaseT gRmdAppToClientVersionDb = {
    sizeof(gRmdAppToClientVersionsSupported) / sizeof(ClVersionT),
    gRmdAppToClientVersionsSupported
};

/*
 * API to verify if the Version is compatible 
 */
ClRcT clRmdVersionVerify(ClVersionT *pVersion)
{
    ClRcT rc = CL_OK;

    if (NULL == pVersion)
    {
        RMD_DBG1(("NULL passed for Version\n"));
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }

    /*
     * Verify the version information 
     */
    rc = clVersionVerify(&gRmdAppToClientVersionDb, pVersion);
    if (rc != CL_OK)
    {
        RMD_DBG1(("Version Mismatch - Supported Version{'%c', 0x%x, 0x%x}\n",
                  pVersion->releaseCode, pVersion->majorVersion,
                  pVersion->minorVersion));
        return (CL_RMD_RC(CL_ERR_VERSION_MISMATCH));
    }

    return CL_OK;
}


ClRcT clRmdLibFinalize(void)
{
    if (rmdInitDone != CL_TRUE)
    {
        RMD_DBG1(("RMD is not initialized\n"));
        return (CL_RMD_RC(CL_ERR_NOT_INITIALIZED));
    }
    clOsalTaskKeyDelete(clRmdPrivateDataKey);
    rmdMetricFinalize();
    rmdInitDone = CL_FALSE;     /* Reset the Init Flag (Bug - 3380) */

    return CL_OK;
}

/*
 * Lightweight/fast RMD header marshall/unmarshalls.
 */
ClRcT clRmdMarshallRmdHdr(ClRmdHdrT *pRmdHdr, ClUint8T *buffer, ClUint32T *bufferLen)
{
    ClUint32T len = 0;
    ClUint32T val = 0;
    ClUint64T msgId = 0;
    buffer[len++] = pRmdHdr->protoVersion;
    buffer[len++] = pRmdHdr->clientVersion.releaseCode;
    buffer[len++] = pRmdHdr->clientVersion.majorVersion;
    buffer[len++] = pRmdHdr->clientVersion.minorVersion;

    val = htonl(pRmdHdr->flags);
    memcpy(&buffer[len], &val, sizeof(val));
    len += sizeof(val);

    val = htonl(pRmdHdr->maxNumTries);
    memcpy(&buffer[len], &val, sizeof(val));
    len += sizeof(val);

    val = htonl(pRmdHdr->callTimeout);
    memcpy(&buffer[len], &val, sizeof(val));
    len += sizeof(val);

    msgId = clHtonl64(pRmdHdr->msgId);
    memcpy(&buffer[len], &msgId, sizeof(msgId));
    len += sizeof(msgId);

    val = htonl(pRmdHdr->fnID);
    memcpy(&buffer[len], &val, sizeof(val));
    len += sizeof(val);
    
    val = htonl(pRmdHdr->rmdReqRepl.rc);
    memcpy(&buffer[len], &val, sizeof(val));
    len += sizeof(val);

    *bufferLen = len;

    return CL_OK;
}

ClRcT clRmdUnmarshallRmdHdr(ClBufferHandleT msg, ClRmdHdrT *pRmdHdr, ClUint32T *hdrLen)
{
    ClUint32T len = 1;
    ClUint32T nbytesRead = 0;
    ClRcT rc = CL_OK;

    rc = clBufferNBytesRead(msg,  (ClUint8T*)&pRmdHdr->protoVersion, &len);
    len = 1;
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->clientVersion.releaseCode, &len);
    len = 1;
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->clientVersion.majorVersion, &len);
    len = 1;
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->clientVersion.minorVersion, &len);

    if(rc != CL_OK)
    {
        return rc;
    }
    
    nbytesRead += 4;

    len = sizeof(ClUint32T);
    rc = clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->flags, &len);
    pRmdHdr->flags = ntohl(pRmdHdr->flags);
    nbytesRead += sizeof(ClUint32T);
    
    len = sizeof(ClUint32T);
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->maxNumTries, &len);
    pRmdHdr->maxNumTries = ntohl(pRmdHdr->maxNumTries);
    nbytesRead += sizeof(ClUint32T);

    len = sizeof(ClUint32T);
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->callTimeout, &len);
    pRmdHdr->callTimeout = ntohl(pRmdHdr->callTimeout);
    nbytesRead += sizeof(ClUint32T);

    len = sizeof(ClUint64T);
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->msgId, &len);
    pRmdHdr->msgId = clNtohl64(pRmdHdr->msgId);
    nbytesRead += sizeof(ClUint64T);

    len = sizeof(ClUint32T);
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->fnID, &len);
    pRmdHdr->fnID = ntohl(pRmdHdr->fnID);
    nbytesRead += sizeof(ClUint32T);

    len = sizeof(ClUint32T);
    rc |= clBufferNBytesRead(msg, (ClUint8T*)&pRmdHdr->rmdReqRepl.rc, &len);
    pRmdHdr->rmdReqRepl.rc = ntohl(pRmdHdr->rmdReqRepl.rc);
    nbytesRead += sizeof(ClUint32T);

    *hdrLen = nbytesRead;

    return rc;
}

ClRcT clRmdWithMsg(ClIocAddressT remoteObjAddr, /* remote Obj addr */
                   ClUint32T funcId,    /* Function ID to invoke */
                   ClBufferHandleT inMsgHdl, /* Input Message */
                   ClBufferHandleT outMsgHdl,    /* Output Message */
                   ClUint32T flags, /* Flags */
                   ClRmdOptionsT *pOptions, /* Optional Params for RMD Call */
                   ClRmdAsyncOptionsT *pAsyncOptions)   /* Optional Async
                                                         * Parameters for RMD
                                                         * Call */
{
    ClUint32T len;
    ClRcT rc = CL_OK;
    ClIocPortT srcPort = 0;
    ClIocPortT dstPort = 0;
    ClUint32T nodeVersion = 0;
    ClVersionT version = { CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION };
    static ClUint32T minVersion = CL_VERSION_CODE(CL_RELEASE_VERSION_BASE,
                                                  CL_MAJOR_VERSION_BASE,
                                                  CL_MINOR_VERSION_BASE);
    CL_FUNC_ENTER();

    if (rmdInitDone == CL_FALSE)
    {
        RMD_DBG1(("RMD is not initialized\n"));
        rc = CL_RMD_RC(CL_ERR_NOT_INITIALIZED);
        goto err_out;
    }

    clEoMyEoIocPortGet(&srcPort);

#ifdef RMD_DISABLE_HIGH_PRI
    if ((srcPort != CL_IOC_CPM_PORT) && (pOptions && CL_IOC_HIGH_PRIORITY == pOptions->priority))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, 
                       ("High priority RMD called by someone "
                        "other than CPM !!!, port = [%#x]\n", 
                        srcPort));
    }
#endif

    if (flags & RMD_CALL_MSG_ASYNC)
    {
        rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
        goto err_out;
    }

    if (flags & CL_RMD_CALL_NEED_REPLY)
    {
        if (outMsgHdl == 0)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
            goto err_out;
        }
        if ((clBufferLengthGet(outMsgHdl, &len)) != CL_OK)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_BUFFER);
            goto err_out;
        }
    }
    else
    {
        if (outMsgHdl)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_BUFFER);
            goto err_out;
        }
    }

    if ((flags & CL_RMD_CALL_ASYNC) && (flags & CL_RMD_CALL_NEED_REPLY))
    {
        flags = flags | RMD_CALL_MSG_ASYNC;
    }

    dstPort = srcPort;
    /*
     * FIXME: Can do nothing for LA/MA addresses other than taking our own
     */
    if(CL_IOC_ADDRESS_TYPE_GET(&remoteObjAddr) == CL_IOC_PHYSICAL_ADDRESS_TYPE)
    {
        dstPort = remoteObjAddr.iocPhyAddress.portId;
    }

    clEoClientGetFuncVersion(dstPort, funcId, &version);

    /*
     * Check node cache version only if function version is greater than minimum.
     */
    if(minVersion < CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion))
    {
        /*
         * Now get the min. version of ASP running in the cluster.
         * and use it if its lesser than the version of the function id
         */
        clNodeCacheMinVersionGet(NULL, &nodeVersion);
        if(nodeVersion 
           && 
           nodeVersion < CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion))
        {
            version.releaseCode = CL_VERSION_RELEASE(nodeVersion);
            version.majorVersion = CL_VERSION_MAJOR(nodeVersion);
            version.minorVersion = CL_VERSION_MINOR(nodeVersion);
        }
    }

    rc = clRmdWithMessage(remoteObjAddr, &version, funcId, inMsgHdl, outMsgHdl, flags,
                          pOptions, pAsyncOptions);

    goto out;

    err_out:

    if ((flags & CL_RMD_CALL_NON_PERSISTENT) && (inMsgHdl))
    {
        clBufferDelete(&inMsgHdl);
    }
    out :
    CL_FUNC_EXIT();

    return rc;
    
}

ClRcT clRmdWithMsgVer(ClIocAddressT remoteObjAddr,
                      ClVersionT *version,
                      ClUint32T funcId,
                      ClBufferHandleT inMsgHdl,
                      ClBufferHandleT outMsgHdl,
                      ClUint32T flags,
                      ClRmdOptionsT *pOptions,
                      ClRmdAsyncOptionsT *pAsyncOptions)
{
    ClUint32T len;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if (rmdInitDone == CL_FALSE)
    {
        RMD_DBG1(("RMD is not initialized\n"));
        rc = CL_RMD_RC(CL_ERR_NOT_INITIALIZED);
        goto err_out;
    }

    if (flags & RMD_CALL_MSG_ASYNC)
    {
        rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
        goto err_out;
    }

    if (flags & CL_RMD_CALL_NEED_REPLY)
    {
        if (outMsgHdl == 0)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
            goto err_out;
        }
        if ((clBufferLengthGet(outMsgHdl, &len)) != CL_OK)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_BUFFER);
            goto err_out;
        }
    }
    else
    {
        if (outMsgHdl)
        {
            rc = CL_RMD_RC(CL_ERR_INVALID_BUFFER);
            goto err_out;
        }
    }

    if ((flags & CL_RMD_CALL_ASYNC) && (flags & CL_RMD_CALL_NEED_REPLY))
    {
        flags = flags | RMD_CALL_MSG_ASYNC;
    }

    rc = clRmdWithMessage(remoteObjAddr, version, funcId, inMsgHdl, outMsgHdl, flags,
                          pOptions, pAsyncOptions);

    goto out;

err_out:

    if ((flags & CL_RMD_CALL_NON_PERSISTENT) && (inMsgHdl))
    {
        clBufferDelete(&inMsgHdl);
    }
out :
    CL_FUNC_EXIT();

    return rc;
    
}

static ClRcT clRmdWithMessage(ClIocAddressT remoteObjAddr,  /* remote OM addr */
                              ClVersionT *version,
                              ClUint32T funcId, /* Function ID to invoke */
                              ClBufferHandleT inMsgHdl,  /* Input
                                                                 * Message */
                              ClBufferHandleT outMsgHdl, /* Output
                                                                 * Message */
                              ClUint32T flags,  /* Flags */
                              ClRmdOptionsT *pOptions,  /* Optional Parameters
                                                         * for RMD Call */
                              ClRmdAsyncOptionsT *pAsyncOptions)    /* Optional 
                                                                     * Parameters 
                                                                     * for RMD
                                                                     * Call */
{

    ClRcT retVal = 0, retVal1 = 0;
    ClRcT retCode = 0;
    ClEoExecutionObjT *pThis = NULL;
    ClEoExecutionObjT *remoteEoObj = NULL;
    ClRmdObjT *pRmdObject = NULL;
    ClUint64T msgId = 0, nwOrderMsgId = 0;
    ClRmdOptionsT tempOptions = { 0 };
    ClRmdAsyncOptionsT tempAsyncOptions = { 0 };
    ClBufferHandleT message = 0;
    ClIocPortT locIocPort = 0;
    ClRmdRecordSendT *pSendRec = NULL;
    ClCntNodeHandleT nodeHandle = 0;
    ClUint32T payloadLen = 0;
    ClIocSendOptionT sndOption;
    ClUint32T hdrLen = 0;

    CL_FUNC_ENTER();

    retVal = clEoMyEoObjectGet(&pThis);
    if (retVal != CL_OK)
    {
        /*
         * either obj is not set or set to NULL
         */
        if ((flags & CL_RMD_CALL_NON_PERSISTENT) && (inMsgHdl))
            clBufferDelete(&inMsgHdl);
        RMD_DBG1(("EO_OBJ is not proper\n"));
        return retVal;
    }
    pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    if (pRmdObject == NULL)
    {
        if ((flags & CL_RMD_CALL_NON_PERSISTENT) && (inMsgHdl))
            clBufferDelete(&inMsgHdl);
        RMD_DBG1(("clRmdWithMessage: Unable to get RMD Object\n"));
        CL_FUNC_EXIT();
        return (CL_RMD_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * This is the only entry point of all calls so putting it here
     */
    RMD_STAT_INC(pRmdObject->rmdStats.nRmdCalls);
    retVal =
        clRmdParamSanityCheck(inMsgHdl, outMsgHdl, flags, pOptions,
                              pAsyncOptions);

    if (retVal != CL_OK)
    {
        if ((flags & CL_RMD_CALL_NON_PERSISTENT) && (inMsgHdl))
            clBufferDelete(&inMsgHdl);

        RMD_DBG1(("clRmdWithMessage: Invalid parameters Rc :0x%x\n", (retVal)));
        CL_FUNC_EXIT();
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
        return retVal;
    }

    if (inMsgHdl)
    {
        if (flags & CL_RMD_CALL_NON_PERSISTENT)
            message = inMsgHdl;
        else
        {
            retCode = clBufferClone(inMsgHdl, &message);
            if (retCode != CL_OK)
            {
                RMD_DBG1(("clBufferClone: Failed in duplicate  message Rc :0x%x\n", (retCode)));
                CL_FUNC_EXIT();
                RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
                return retCode;
            }
        }

        retCode = clBufferLengthGet(message, &payloadLen);
        if (retCode != CL_OK)
        {
            RMD_DBG1(("clBufferLengthGet: Failed in getting  message  length Rc :0x%x\n", (retCode)));
            CL_FUNC_EXIT();
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
            return retCode;
        }

    }
    else
    {
        retCode = clBufferCreate(&message);
        if (retCode != CL_OK)
        {
            RMD_DBG1(("clBufferCreate: Failed in create message Rc :0x%x\n", (retCode)));
            CL_FUNC_EXIT();
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
            return retCode;
        }
    }

    if (pOptions)
    {
        tempOptions.timeout = pOptions->timeout;    /* time to wait for the
                                                     * reply to come */
        tempOptions.priority = pOptions->priority;  /* associated priority */

        if (pOptions->retries <= _clRmdConfig.maxRetries)
        {
            tempOptions.retries = pOptions->retries;
        }
        else
        {
            tempOptions.retries = _clRmdConfig.maxRetries;
        }
        tempOptions.transportHandle = pOptions->transportHandle;
    }
    else
    {
        tempOptions.priority = CL_RMD_DEFAULT_PRIORITY;
        tempOptions.timeout = CL_RMD_DEFAULT_TIMEOUT;
        tempOptions.retries = CL_RMD_DEFAULT_RETRIES;
        tempOptions.transportHandle = CL_RMD_DEFAULT_TRANSPORT_HANDLE;
    }

    if (pAsyncOptions)
    {
        tempAsyncOptions.pCookie = pAsyncOptions->pCookie;  /* Application
                                                             * cookie */
        tempAsyncOptions.fpCallback = pAsyncOptions->fpCallback;    /* Application's
                                                                     * call back
                                                                     * function */
    }
    else
    {
        tempAsyncOptions.pCookie = NULL;
        tempAsyncOptions.fpCallback = NULL;
    }

    /*
     * create the message and pass it to the user
     */
    retVal = clOsalMutexLock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(retVal == CL_OK);
    msgId = pRmdObject->msgId++;
    retVal = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(retVal == CL_OK);
    nwOrderMsgId = msgId;

    retVal = clRmdCreateRmdMessageToSend(version,
                                         funcId,
                                         message,
                                         flags,
                                         msgId,
                                         payloadLen,
                                         0,
                                         tempOptions.retries,
                                         tempOptions.timeout,
                                         &hdrLen);
    if (retVal != CL_OK)
    {
        clBufferDelete(&message);
        RMD_DBG1(("clRmdWithMessage: Failed in Create Packet Rc :0x%x\n",
                  (retVal)));
        CL_FUNC_EXIT();
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
        return retVal;
    }

    if ((flags & CL_RMD_CALL_ASYNC) == 0)
    {

        if ((CL_IOC_IS_ADDRESS_PHYSICAL(remoteObjAddr)) &&
            ((flags & CL_RMD_CALL_DO_NOT_OPTIMIZE) == 0) &&
            (remoteObjAddr.iocPhyAddress.nodeAddress == clIocLocalAddressGet()))
        {
            retVal =
                clEoGetRemoteObjectAndBlock(remoteObjAddr.iocPhyAddress.portId,
                                            &remoteEoObj);

            if(retVal == CL_OK)
            {
                /*
                 * We disable optimization if its an app. port and reply is expected.
                 */
                if(remoteObjAddr.iocPhyAddress.portId >= CL_IOC_RESERVED_PORTS
                   &&
                   (flags & CL_RMD_CALL_NEED_REPLY))
                {
                    retVal = CL_RMD_RC(CL_ERR_NOT_SUPPORTED);
                }
            }

            if (retVal == CL_OK)
            {
                ClRmdResponeContextT responseContext  = {0};
                ClUint32T clientID = (funcId >> CL_EO_CLIENT_BIT_SHIFT) & CL_EO_CLIENT_ID_MASK;
                ClUint32T hdrLen = 0;
                RMD_STAT_INC(pRmdObject->rmdStats.nRmdCallOptimized);
                (void) clEoMyEoIocPortGet(&locIocPort);
                (void) clEoMyEoObjectSet(remoteEoObj);
                (void) clEoMyEoIocPortSet(remoteEoObj->eoPort);

                /*
                 * passing msg, so no need of memory allocation or flattening
                 * of buffer
                 */

                /*
                 * no need to worry about payloadLen anymore!!
                 */
                RMD_STAT_INC(((ClRmdObjT *) (remoteEoObj->rmdObj))->rmdStats.
                             nRmdRequests);
                retVal = clRmdUnmarshallRmdHdr(message, &responseContext.ClRmdHdr, &hdrLen);
                if(retVal != CL_OK)
                {
                    RMD_DBG1(("%s: Failed to unmarshall the RMD header. Rc :0x%x\n", __FUNCTION__, retVal));
                    return retVal; 
                }
                clBufferReadOffsetSet(message, 0, CL_BUFFER_SEEK_SET);
                responseContext.priority = tempOptions.priority;
                responseContext.srcAddr = remoteObjAddr.iocPhyAddress;
                responseContext.isInProgress = CL_FALSE;
                responseContext.replyType = CL_IOC_RMD_SYNC_REPLY_PROTO;
                clRmdPrivateDataSet((ClPtrT)&responseContext);

                if (!pThis->pServerTable
                    || 
                    !clEoClientTableFilter(pThis->eoPort, clientID))
                {
                    retVal =
                    clEoWalk(remoteEoObj, funcId, clRmdInvoke, message,
                             outMsgHdl);
                }
                else
                {
                    retVal =
                    clEoWalkWithVersion(remoteEoObj, funcId, version, clRmdInvoke, message,
                                        outMsgHdl);
                }
                
                RMD_STAT_INC(((ClRmdObjT *) (remoteEoObj->rmdObj))->rmdStats.
                             nReplySend);
                clBufferDelete(&message);
                (void) clEoMyEoIocPortSet(locIocPort);
                (void) clEoMyEoObjectSet(pThis);
                RMD_STAT_INC(pRmdObject->rmdStats.nRmdReplies);
                (void) clEoRemoteObjectUnblock(remoteEoObj);
                return retVal;
            }
        }

        RMD_DBG3(("  RMD Sync Path\n"));

        if (pOptions &&
            ((pOptions->timeout == 0) ||
             (pOptions->timeout == (ClUint32T) CL_RMD_TIMEOUT_FOREVER)))
            tempOptions.timeout = CL_RMD_TIMEOUT_FOREVER;

        if (outMsgHdl)
        {
            clBufferClear(outMsgHdl);
            retVal =
                clRmdSyncSendAndReplyReceive(pThis, pRmdObject, hdrLen, outMsgHdl,
                                             &tempOptions, nwOrderMsgId,
                                             message, remoteObjAddr, flags);
            clBufferDelete(&message);
            return retVal;
        }
        else
        {
            retVal = clBufferCreate(&outMsgHdl);
            if (retVal != CL_OK)
            {
                clBufferDelete(&message);
                return retVal;
            }
            retVal =
                clRmdSyncSendAndReplyReceive(pThis, pRmdObject, hdrLen, outMsgHdl,
                                             &tempOptions, nwOrderMsgId,
                                             message, remoteObjAddr, flags);

            clBufferDelete(&outMsgHdl);
            clBufferDelete(&message);
            return retVal;
        }
    }
    else
    {
        ClUint8T protoType = (flags & CL_RMD_CALL_ORDERED)? 
            CL_IOC_RMD_ORDERED_PROTO : CL_IOC_RMD_ASYNC_REQUEST_PROTO;

        RMD_DBG4((" RMD ASync Path\n"));
        /*
         * Async
         */

        sndOption.linkHandle = tempOptions.transportHandle;
        sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
        sndOption.timeout = tempOptions.timeout;
        sndOption.priority = tempOptions.priority;

        if (flags & CL_RMD_CALL_IN_SESSION)
        {
            sndOption.sendType = CL_IOC_SESSION_BASED;
        }
        else
        {
            sndOption.sendType = CL_IOC_NO_SESSION;
        }

        /*
         * there is no record return form here 
         */
        if ((flags & CL_RMD_CALL_NEED_REPLY) == 0)
        {
            clRmdDumpPkt("sending async req", message);
            retVal =
                clIocSend(pThis->commObj, message,
                          protoType, &remoteObjAddr,
                          &sndOption);

            retCode = clBufferDelete(&message);

            if (retVal != CL_OK)
                RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);

            CL_FUNC_EXIT();
            return retVal;
        }

        /*
         * need to lock the hash table
         */
        clOsalMutexLock(pRmdObject->semaForSendHashTable);

        retVal =
            clRmdCreateAndAddRmdSendRecord(pThis, remoteObjAddr, hdrLen, outMsgHdl,
                                           flags, &tempOptions,
                                           &tempAsyncOptions, message,
                                           nwOrderMsgId, payloadLen, &pSendRec);
        if (retVal != CL_OK)
        {
            clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
            clBufferDelete(&message);
            RMD_DBG1(("clRmdWithMessage: Failed in Adding Record Rc :0x%x\n",
                      (retVal)));
            CL_FUNC_EXIT();
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
            return retVal;
        }

        clRmdDumpPkt("sending async req", message);
        
        retVal1 =
            clIocSend(pThis->commObj, message, protoType,
                      &remoteObjAddr, &sndOption);

        if (CL_GET_ERROR_CODE(retVal1) == CL_IOC_ERR_HOST_UNREACHABLE || CL_GET_ERROR_CODE(retVal1) == CL_IOC_ERR_COMP_UNREACHABLE)
        {
            ClIocNotificationT notification = {0};
            ClIocRecvParamT recvParam;
            ClBufferHandleT notificationMsg=0;

            notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
            notification.id = htonl(CL_IOC_COMP_DEATH_NOTIFICATION);
            notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(remoteObjAddr.iocPhyAddress.nodeAddress);
            notification.nodeAddress.iocPhyAddress.portId = htonl(remoteObjAddr.iocPhyAddress.portId);

            recvParam.priority = 1;
            recvParam.protoType = CL_IOC_PORT_NOTIFICATION_PROTO;
            recvParam.length = sizeof(notification);
            recvParam.srcAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
            recvParam.srcAddr.iocPhyAddress.portId = 0;

            retVal = clBufferCreate(&notificationMsg);
            if(retVal != CL_OK)
            {
                goto error_out;
            }
            retVal = clBufferNBytesWrite(notificationMsg, (ClUint8T*)&notification, sizeof(notification));
            if(retVal != CL_OK)
            {
                clBufferDelete(&notificationMsg);
                goto error_out;
            }

            clEoSendErrorNotify(notificationMsg, &recvParam);
        } else if(retVal1 != CL_OK) {
            RMD_DBG1(("%s: Failed in IOC Rc :0x%x\n", __FUNCTION__,
                      CL_RMD_RC(retVal1)));
            goto error_out;
        }

        retVal1 = clTimerStart(pSendRec->recType.asyncRec.timerID);
        if (retVal1 != CL_OK)
        {
            RMD_DBG1(("%s: Failed in timer start. Rc :0x%x\n", __FUNCTION__,
                      CL_RMD_RC(retVal1)));
            goto error_out;
        }
        retVal = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
        CL_ASSERT(retVal == CL_OK);       
    }

    CL_FUNC_EXIT();
    return CL_OK;

error_out:
    clBufferDelete(&message);
    retVal =
        clCntNodeFind(((ClRmdObjT *) (pThis->rmdObj))->
                sndRecContainerHandle, (ClPtrT)(ClWordT)nwOrderMsgId,
                &nodeHandle);
    if (retVal == CL_OK)
    {
        retVal =
            clCntNodeDelete(((ClRmdObjT *) (pThis->rmdObj))->
                    sndRecContainerHandle, nodeHandle);
    }
    else
    {
        clHeapFree(pSendRec);   /* freeing it like this as hash table
                                 * has no entry for this */
    }


    retVal = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(retVal == CL_OK);
    
    RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);

    CL_FUNC_EXIT();
    return retVal1;
}

ClRcT clRmdLibInitialize(ClPtrT config)
{
    ClRcT retCode;

    if (rmdInitDone == CL_TRUE)
    {
        return (CL_RMD_RC(CL_ERR_INITIALIZED));
    }

    retCode = clRmdLibConfigGet(&_clRmdConfig);
    if (CL_OK != retCode)
    {
        return retCode;
    }
    retCode = clOsalTaskKeyCreate(&clRmdPrivateDataKey, NULL);
    if (CL_OK != retCode)
    {
        return retCode;
    }
    
    rmdMetricInitialize();

    rmdInitDone = CL_TRUE;
    return CL_OK;
}


ClRcT clRmdStatsReset(void)
{
    ClRcT retVal;
    ClEoExecutionObjT *pThis = NULL;
    ClRmdObjT *pRmdObject = NULL;

    CL_FUNC_ENTER();
    retVal = clEoMyEoObjectGet(&pThis);
    if (retVal != CL_OK)
    {
        /*
         * either obj is not set or set to NULL
         */
        RMD_DBG1(("EO_OBJ is not proper\n"));
        return retVal;
    }
    pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    if (pRmdObject == NULL)
    {
        RMD_DBG1(("clRmdStatsReset: Unable to get RMD Object\n"));
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }
    
    memset(&pRmdObject->rmdStats, 0, sizeof(pRmdObject->rmdStats));
    return CL_OK;
}


ClRcT clRmdStatsGet(ClRmdStatsT *pStats)
{
    ClRcT rc;
    ClEoExecutionObjT *pThis = NULL;
    ClRmdObjT *pRmdObject = NULL;

    if (!pStats)
    {
        return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clEoMyEoObjectGet(&pThis);
    if (rc != CL_OK)
    {
        /*
         * either obj is not set or set to NULL
         */
        RMD_DBG1(("EO_OBJ is not proper\n"));
        return rc;
    }

    pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    if (pRmdObject == NULL)
    {
        RMD_DBG1(("clRmdStatsGet: Unable to get RMD Object\n"));
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }
    memcpy(pStats, &pRmdObject->rmdStats, sizeof(*pStats));
    return CL_OK;
}



ClRcT clRmdMaxPayloadSizeGet(ClUint32T *pSize)
{
    ClRcT rc;

    if (pSize == NULL)
    {
        return (CL_RMD_RC(CL_ERR_NULL_POINTER));
    }
    rc = clIocMaxPayloadSizeGet(pSize);
    if (rc != CL_OK)
        return rc;

    *pSize = (*pSize) - sizeof(ClRmdHdrT);
    return CL_OK;

}

ClRcT clRmdMaxNumbOfRetriesGet(ClUint32T *pNumbOfRetries)
{

    if (pNumbOfRetries == NULL)
    {
        return (CL_RMD_RC(CL_ERR_NULL_POINTER));
    }

    if (rmdInitDone)
    {
        *pNumbOfRetries = _clRmdConfig.maxRetries;
        return CL_OK;
    }
    else
    {
        return (CL_RMD_RC(CL_ERR_NOT_INITIALIZED));
    }
}

/*
 * pass timeout retries etc
 */
static ClRcT clRmdSyncSendAndReplyReceive(ClEoExecutionObjT *pThis,
                                          ClRmdObjT *pRmdObject,
                                          ClUint32T hdrLen,
                                          ClBufferHandleT outMsgHdl,
                                          ClRmdOptionsT *pOptions,
                                          ClUint32T nwMsgId,
                                          ClBufferHandleT inMsg,
                                          ClIocAddressT destAddr,
                                          ClUint32T flags)
{
    ClRcT retVal = 0, retCode = 0;
    ClUint8T i = 0;
    ClRmdRecordSendT sendRecord = {0};
    ClRmdRecordSendT *pSendRecord = &sendRecord;

    ClUint32T condTimeout = pOptions->timeout;
    ClUint32T locTimeOut = condTimeout;
    ClUint32T retries = pOptions->retries;

    ClTimerTimeOutT condTsTimeout;

    ClIocSendOptionT sndOption = { 0 };
    
    ClUint8T protoType = (flags & CL_RMD_CALL_ORDERED)? 
        CL_IOC_RMD_ORDERED_PROTO : CL_IOC_RMD_SYNC_REQUEST_PROTO;

#if 0
    pSendRecord = clHeapAllocate(sizeof(ClRmdRecordSendT));
    if (pSendRecord == NULL)
    {
        CL_FUNC_EXIT();
        return (CL_RMD_RC(CL_ERR_NO_MEMORY));
    }
#endif

    pSendRecord->flags = flags;
    pSendRecord->hdrLen = hdrLen;
    pSendRecord->recType.syncRec.retVal = CL_OK;
    pSendRecord->recType.syncRec.outMsgHdl = outMsgHdl;
    pSendRecord->recType.syncRec.destAddr = destAddr;

    retVal = clOsalCondInit(&pSendRecord->recType.syncRec.syncCond);
    CL_ASSERT(retVal == CL_OK);
    
    /*
     * Make an entry in the Hash table for the Sync Call
     */
    retVal = clOsalMutexLock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(retVal == CL_OK);

    if (1) /* Make sure that the node does not exist in the container */
    {
        ClCntNodeHandleT nodeHdl = 0;        
        retVal = clCntNodeFind(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)nwMsgId,&nodeHdl);
        if (retVal == CL_OK)
        {
            ClRmdRecordSendT *currentRecord=NULL;
            clCntNodeUserDataGet(pRmdObject->sndRecContainerHandle, nodeHdl, (ClCntDataHandleT*) &currentRecord);
            clDbgCodeError(0,("Message Id [0x%x] already exists in the container", nwMsgId));
        }
    }
    
    retVal =
        clCntNodeAdd(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)nwMsgId,
                     (ClCntDataHandleT) pSendRecord, NULL);
    if (retVal != CL_OK)
    {
        clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
        RMD_DBG3(("RMD: unable to add the record 0x%x\n", retVal));
        return retVal;
    }

    if(gClIocTrafficShaper && locTimeOut && locTimeOut != CL_RMD_TIMEOUT_FOREVER)
    {
        ClUint32T msgLength = 0;
        retVal = clBufferLengthGet(inMsg, &msgLength);
        if(retVal != CL_OK)
        {
            clCntAllNodesForKeyDelete(pRmdObject->sndRecContainerHandle,
                                      (ClPtrT)(ClWordT)nwMsgId);
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);            
            clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
            clLogError("RMD", "SND", "Buffer length get failed with [%#x]", retVal);
            return retVal;
        }
        /*
         * If over the 200 mb limit, the below timeout might not be sufficient.
         * Set it to a bigger timeout based on msg length
         */
        if(msgLength >= (200 << 20U))
        {
            ClUint32T factor = 1;
            ClUint32T delay = msgLength/(20 << 20U);
            if(msgLength >= (1<<30L))
                factor <<= 1 ; /* double it for larger sizes */
            delay *= factor;
            delay += CL_RMD_DEFAULT_TIMEOUT;
            if(delay > 100) 
                delay = 100; /* this should be enough for worst case */
            if(locTimeOut < delay)
                locTimeOut = delay;
            if(retries > 1) retries = 1; /* retry such large transfers only once */
            condTimeout = locTimeOut;
        }
    }

    /*
     * Make retries+1 attempts to make the sync call.
     * '+1' is the orignial call and is at denoted by
     * i = 0 in the for loop.
     */
    for (i = 0; i <= retries; i++)
    {
        if (i > 0)
            RMD_STAT_INC(pRmdObject->rmdStats.nResendRequests);

        sndOption.linkHandle = 0;
        sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
        sndOption.timeout = condTimeout;
        sndOption.priority = pOptions->priority;

        if (flags & CL_RMD_CALL_IN_SESSION)
        {
            sndOption.sendType = CL_IOC_SESSION_BASED;
        }
        else
        {
            sndOption.sendType = CL_IOC_NO_SESSION;
        }

        clRmdDumpPkt("sending sync req", inMsg);
        retVal = clIocSend(pThis->commObj, inMsg, protoType, &destAddr, &sndOption);
        
        if (retVal != CL_OK)
        {
            clLogWarning("RMD","SND","In RMD, IOC call failed with error [%x].  MsgId [0x%x]", retVal, nwMsgId);
            clCntAllNodesForKeyDelete(pRmdObject->sndRecContainerHandle,
                                      (ClPtrT)(ClWordT)nwMsgId);
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);            
            if(i > 0)
            {
                RMD_STAT_INC(pRmdObject->rmdStats.nFailedResendRequests);
            }
            retCode = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
            CL_ASSERT(retCode == CL_OK);
            return retVal;
        }

        if (locTimeOut == CL_RMD_TIMEOUT_FOREVER)
        {
            condTsTimeout.tsSec = 0;
            condTsTimeout.tsMilliSec = 0;
        }
        else
        {
            condTsTimeout.tsSec = locTimeOut / 1000;
            condTsTimeout.tsMilliSec = locTimeOut % 1000;
        }

        retVal =
            clOsalCondWait(&pSendRecord->recType.syncRec.syncCond,
                           pRmdObject->semaForSendHashTable, condTsTimeout);
        if (retVal == CL_OK)
        {
            retVal = pSendRecord->recType.syncRec.retVal;
            clCntAllNodesForKeyDelete(pRmdObject->sndRecContainerHandle,
                                      (ClPtrT)(ClWordT)nwMsgId);
            retCode = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
            CL_ASSERT(retCode == CL_OK);
            return retVal;
        }
        else if (CL_GET_ERROR_CODE(retVal) == CL_ERR_TIMEOUT)
        {
            /*
             * timeout has happened go to sleep again
             */
            locTimeOut = condTimeout;
        }
        else
        {
            clDbgCodeError(retVal, ("Unhandled return code"));
        }
        
    }

    /*
     * All the retries have failed. Flag TIMEOUT error.
     */

    clCntAllNodesForKeyDelete(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)nwMsgId);
    retVal = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(retVal == CL_OK); 
    RMD_DBG4(("RMD-TIMEOUT at %d\n", __LINE__));
    RMD_STAT_INC(pRmdObject->rmdStats.nCallTimeouts);
    return (CL_RMD_RC(CL_ERR_TIMEOUT));
}


ClRcT clRmdDumpPacketStatus(ClUint32T isEnable)
{
    if (isEnable)
    {
        gPrintHeaders = 1;
    }
    else
    {
        gPrintHeaders = 0;
    }

    return CL_OK;
}

void clRmdDumpPkt(char *name, ClBufferHandleT msg)
{
    if (gPrintHeaders)
    {
        ClRmdHdrT inHdr = {{0}};
        ClUint32T hdrLen = 0;
        ClUint32T size = 0;
        ClRcT rc = CL_OK;
        clBufferReadOffsetSet(msg, 0, CL_BUFFER_SEEK_SET);
        rc = clRmdUnmarshallRmdHdr(msg, &inHdr, &hdrLen);
        if(rc != CL_OK) return;
        clBufferLengthGet(msg, &size);
        clLogDebug("RMD", "DUMP", ":::::::::::::::::::: RMD Pkt at %s ::::::::::::::::::::", name);
        clLogDebug("RMD", "DUMP", "-------------Header-------------");
        clLogDebug("RMD", "DUMP", "Protocol version %d", inHdr.protoVersion);
        clLogDebug("RMD", "DUMP", "Version: {'%c', 0x%x, 0x%x}",
                   inHdr.clientVersion.releaseCode, 
                   inHdr.clientVersion.majorVersion,
                   inHdr.clientVersion.minorVersion);

        clLogDebug("RMD", "DUMP", "Flags: %#x, MsgId: %#llx", inHdr.flags, inHdr.msgId);
        clLogDebug("RMD", "DUMP", "FnID: %#x, rc: %#x, msgLen: %#x", 
                   inHdr.fnID, inHdr.rmdReqRepl.rc, size - hdrLen);
        clLogDebug("RMD", "DUMP", "-------------END-------------");
        clBufferReadOffsetSet(msg, 0, CL_BUFFER_SEEK_SET);
    }

    return;
}

static void rmdAtmostOnceDatabaseCleanup(ClRmdObjT *pRmdObj, ClIocPhysicalAddressT *pAddress)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;
    ClCntNodeHandleT nextNodeHandle = 0;
    if(!pAddress) return;
    clOsalMutexLock(pRmdObj->semaForRecvHashTable);
    rc = clCntFirstNodeGet(pRmdObj->rcvRecContainerHandle, &nodeHandle);
    if(rc != CL_OK)
    {
        goto out_unlock;
    }
    while(nodeHandle)
    {
        ClRmdRecordRecvT *record = NULL;
        nextNodeHandle = 0;
        clCntNextNodeGet(pRmdObj->rcvRecContainerHandle, nodeHandle, &nextNodeHandle);
        clCntNodeUserDataGet(pRmdObj->rcvRecContainerHandle, nodeHandle,
                             (ClCntDataHandleT*)&record);
        if(record)
        {
            if(record->recvKey.srcAddr.nodeAddress == pAddress->nodeAddress)
            {
                if(!pAddress->portId 
                   || 
                   record->recvKey.srcAddr.portId == pAddress->portId)
                {
                    clCntNodeDelete(pRmdObj->rcvRecContainerHandle, nodeHandle);
                }
            }
        }
        nodeHandle = nextNodeHandle;
    }
    out_unlock:
    clOsalMutexUnlock(pRmdObj->semaForRecvHashTable);
}

ClRcT clRmdDatabaseCleanup(ClRmdObjHandleT rmdObj, ClIocNotificationT *pNotification)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0, nextNodeHandle = 0;
    ClRmdObjT *pRmdObj = (ClRmdObjT *)rmdObj;
    ClRmdRecordSendT *pRec = NULL;
    ClIocNotificationT notification = {0};
    CL_LIST_HEAD_DECLARE(rmdAsyncCallbackList);
    typedef struct ClRmdAsyncCallbackData
    {
        ClRcT rc;
        ClBufferHandleT inMsgHdl;
        ClBufferHandleT outMsgHdl;
        void *cookie;
        ClRmdAsyncCallbackT func;
        ClListHeadT list;
    }ClRmdAsyncCallbackDataT;

    if (rmdInitDone != CL_TRUE || !pNotification) {
        return CL_OK;
    }

    if(!pRmdObj)
    {
        ClEoExecutionObjT *pThis = NULL;
        rc = clEoMyEoObjectGet(&pThis);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get my EO object. error code [0x%x].\n", rc));
            return rc;
        }
        pRmdObj = pThis->rmdObj;
    }
    memcpy(&notification, pNotification, sizeof(notification));
    notification.id = ntohl(notification.id);
    notification.nodeAddress.iocPhyAddress.nodeAddress = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
    notification.nodeAddress.iocPhyAddress.portId = ntohl(notification.nodeAddress.iocPhyAddress.portId);

    clOsalMutexLock(pRmdObj->semaForSendHashTable);
    rc = clCntFirstNodeGet(pRmdObj->sndRecContainerHandle, &nodeHandle);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Trace : No records present in RMD's sendRecContainer. error code 0x%x\n",rc));
        rc = CL_OK;
        goto error_ret_1;
    }
    
    while(nodeHandle != NULL) {
        rc = clCntNodeUserDataGet(pRmdObj->sndRecContainerHandle, nodeHandle, (ClCntDataHandleT*)&pRec);
        if(rc != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to get the data of a record. error code 0x%x\n",rc));
            continue;
        }


        if( (pRec->flags & CL_RMD_CALL_ASYNC) )
        {
            ClUint32T addrType = 0;
            ClBoolT match = CL_FALSE;

            switch((addrType = 
                    CL_IOC_ADDRESS_TYPE_GET
                    (&(pRec->recType.asyncRec.destAddr))))
            {
            case CL_IOC_PHYSICAL_ADDRESS_TYPE:
            case CL_IOC_LOGICAL_ADDRESS_TYPE:
            case CL_IOC_MULTICAST_ADDRESS_TYPE:
                break;
            default:
                goto next_node;
            }

            switch(notification.id) 
            {

                case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
                case CL_IOC_NODE_LINK_UP_NOTIFICATION:
                case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
                    goto notif_not_handled;

                case CL_IOC_NODE_LEAVE_NOTIFICATION:
                case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
                    if(addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE)
                    {
                        match = 
                            (ClBoolT)(
                                      pRec->recType.asyncRec.destAddr.iocPhyAddress.nodeAddress == 
                                      notification.nodeAddress.iocPhyAddress.nodeAddress);
                            
                    }
                    else
                    {
                        if( clIocServerReady(&pRec->recType.asyncRec.destAddr)
                           != CL_OK)
                        {
                            match = CL_TRUE;
                        }
                    }

                    if(match == CL_TRUE)
                    {
                        ClRmdAsyncCallbackDataT *pCallbackData =  NULL;
                        clTimerDeleteAsync(&pRec->recType.asyncRec.timerID);
                        rc = clBufferHeaderTrim(pRec->recType.asyncRec.sndMsgHdl, pRec->hdrLen);
                        if(rc != CL_OK)
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to trim the buffer header. error code [0x%x]",rc));
                        
                        pCallbackData = clHeapCalloc(1, sizeof(*pCallbackData));
                        CL_ASSERT(pCallbackData != NULL);
                        pCallbackData->rc = CL_RMD_RC(CL_IOC_ERR_HOST_UNREACHABLE);
                        pCallbackData->cookie = pRec->recType.asyncRec.cookie;
                        pCallbackData->inMsgHdl = pRec->recType.asyncRec.sndMsgHdl;
                        pCallbackData->outMsgHdl = pRec->recType.asyncRec.outMsgHdl;
                        pCallbackData->func = pRec->recType.asyncRec.func;
                        pRec->recType.asyncRec.sndMsgHdl = 0;
                        pRec->recType.asyncRec.outMsgHdl = 0;
                        clListAddTail(&pCallbackData->list, &rmdAsyncCallbackList);
                        goto node_del;
                    }
                    break;

                case CL_IOC_COMP_DEATH_NOTIFICATION:

                    if(addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE)
                    {
                        match =
                            ( (pRec->recType.asyncRec.destAddr.iocPhyAddress.nodeAddress == 
                               notification.nodeAddress.iocPhyAddress.nodeAddress)
                              &&
                              (pRec->recType.asyncRec.destAddr.iocPhyAddress.portId == 
                               notification.nodeAddress.iocPhyAddress.portId) );
                    }
                    else
                    {
                        if(clIocServerReady(&pRec->recType.asyncRec.destAddr)
                           != CL_OK)
                        {
                            match = CL_TRUE;
                        }
                    }

                    if(match == CL_TRUE)
                    {
                        ClRmdAsyncCallbackDataT *pCallbackData = NULL;
                        pCallbackData = clHeapCalloc(1, sizeof(*pCallbackData));
                        CL_ASSERT(pCallbackData != NULL);
                        clTimerDeleteAsync(&pRec->recType.asyncRec.timerID);
                        rc = clBufferHeaderTrim(pRec->recType.asyncRec.sndMsgHdl, pRec->hdrLen);
                        if(rc != CL_OK)
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to trim the buffer header. error code [0x%x]",rc));
                        pCallbackData->rc = CL_RMD_RC(CL_IOC_ERR_COMP_UNREACHABLE);
                        pCallbackData->cookie = pRec->recType.asyncRec.cookie;
                        pCallbackData->inMsgHdl = pRec->recType.asyncRec.sndMsgHdl;
                        pCallbackData->outMsgHdl = pRec->recType.asyncRec.outMsgHdl;
                        pCallbackData->func = pRec->recType.asyncRec.func;
                        pRec->recType.asyncRec.sndMsgHdl = 0;
                        pRec->recType.asyncRec.outMsgHdl = 0;
                        clListAddTail(&pCallbackData->list, &rmdAsyncCallbackList);
                        goto node_del;
                    }
                    break;

            default: 
                break;
            }
        } 
        else 
        {
            ClUint32T addrType = 0;
            ClBoolT match = CL_FALSE;

            switch((addrType = 
                    CL_IOC_ADDRESS_TYPE_GET(
                                            &(pRec->recType.syncRec.destAddr))))
            {
            case CL_IOC_PHYSICAL_ADDRESS_TYPE:
            case CL_IOC_LOGICAL_ADDRESS_TYPE:
            case CL_IOC_MULTICAST_ADDRESS_TYPE:
                break;

            default:
                goto next_node;
            }

            switch(notification.id) 
            {
                case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
                case CL_IOC_NODE_LINK_UP_NOTIFICATION:
                case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
                    goto notif_not_handled;

                case CL_IOC_NODE_LEAVE_NOTIFICATION:
                case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
                    if(addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE)
                    {
                        match = 
                            (ClBoolT)(pRec->recType.syncRec.destAddr.iocPhyAddress.nodeAddress == 
                                      notification.nodeAddress.iocPhyAddress.nodeAddress);
                    }
                    else
                    {
                        if(clIocServerReady(&pRec->recType.syncRec.destAddr) 
                           != CL_OK)
                        {
                            match = CL_TRUE;
                        }
                    }

                    if(match == CL_TRUE)
                    {
                        pRec->recType.syncRec.retVal = CL_RMD_RC(CL_IOC_ERR_HOST_UNREACHABLE);
                        clOsalCondSignal(&pRec->recType.syncRec.syncCond);
                        goto next_node;  /* Synchronous RMDs delete their own nodes */
                    }
                    break;

                case CL_IOC_COMP_DEATH_NOTIFICATION:

                    if(addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE)
                    {
                        match =
                            (ClBoolT) ( ( pRec->recType.syncRec.destAddr.iocPhyAddress.nodeAddress == 
                                          notification.nodeAddress.iocPhyAddress.nodeAddress ) 
                                        &&
                                        ( pRec->recType.syncRec.destAddr.iocPhyAddress.portId == 
                                          notification.nodeAddress.iocPhyAddress.portId ) );
                    }
                    else
                    {
                        if(clIocServerReady(&pRec->recType.syncRec.destAddr) 
                           != CL_OK)
                        {
                            match = CL_TRUE;
                        }
                    }
                    
                    if(match == CL_TRUE)
                    {
                        pRec->recType.syncRec.retVal = CL_RMD_RC(CL_IOC_ERR_COMP_UNREACHABLE);
                        clOsalCondSignal(&pRec->recType.syncRec.syncCond);
                        goto next_node; /* Synchronous RMDs delete their own nodes */
                    }
                    break;
            default:
                break;
            }
        }

next_node:
       rc = clCntNextNodeGet(pRmdObj->sndRecContainerHandle,  nodeHandle, &nextNodeHandle);
       if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to get the next handle from sndRecHandler. error code 0x%x\n", rc));
           goto error_ret_1;
       }
       rc = CL_OK;
       nodeHandle = nextNodeHandle;
       continue;

node_del:
       rc = clCntNextNodeGet(pRmdObj->sndRecContainerHandle,  nodeHandle, &nextNodeHandle);
       if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to get the next handle from sndRecHandler. error code 0x%x\n", rc));
           goto error_ret_1;
       }
       rc = clCntNodeDelete(pRmdObj->sndRecContainerHandle, nodeHandle);
       if(rc != CL_OK) {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to delete a record form RMD hashtable. error code 0x%x\n", rc));
           goto error_ret_1;
       }
       nodeHandle = nextNodeHandle;
    }

notif_not_handled:
error_ret_1:
    clOsalMutexUnlock(pRmdObj->semaForSendHashTable);

    while(!CL_LIST_HEAD_EMPTY(&rmdAsyncCallbackList))
    {
        ClRmdAsyncCallbackDataT *data = NULL;
        ClListHeadT *node = rmdAsyncCallbackList.pNext;
        data = CL_LIST_ENTRY(node, ClRmdAsyncCallbackDataT, list);
        clListDel(node);
        if(data->func)
        {
            data->func(data->rc, data->cookie, data->inMsgHdl, data->outMsgHdl);
        }
        if(data->inMsgHdl) 
            clBufferDelete(&data->inMsgHdl);
        clHeapFree(data);
    }
    rmdAtmostOnceDatabaseCleanup(pRmdObj, &notification.nodeAddress.iocPhyAddress);
    return rc;
}

#define RMD_METRIC_HASH_TABLE_SIZE (1024)
#define RMD_METRIC_HASH_TABLE_MASK (RMD_METRIC_HASH_TABLE_SIZE - 1)
#define RMD_METRIC_HASH(clientId, funcId) (  ( (clientId) << 26 ) | ( (funcId) & 63 ) )

static ClInt32T rmdMetricStatus = -1;
static struct hashStruct *rmdMetricHashTable[RMD_METRIC_HASH_TABLE_SIZE];
static CL_LIST_HEAD_DECLARE(rmdMetricActiveList);
static ClOsalMutexT rmdMetricMutex;
static ClTimerHandleT rmdMetricTimer;

typedef struct ClRmdMetric
{
    struct hashStruct hash;
    ClListHeadT list;
    ClUint32T clientId;
    ClUint32T funcId;
    ClUint64T numErrors;
    ClTimeT   minResponseTime;
    ClTimeT   maxResponseTime;
    ClTimeT   totalResponseTime;
    ClTimeT   requestTime;
    ClUint64T numRequests;
    ClUint64T minRequestBytes;
    ClUint64T maxRequestBytes;
    ClUint64T totalRequestBytes;
    ClUint64T numResponses;
    ClUint64T minResponseBytes;
    ClUint64T maxResponseBytes;
    ClUint64T totalResponseBytes;
}ClRmdMetricT;

static __inline__ void rmdMetricAdd(ClRmdMetricT *entry, ClUint32T clientId, ClUint32T funcId)
{
    ClUint32T hash = RMD_METRIC_HASH(clientId, funcId) & RMD_METRIC_HASH_TABLE_MASK;
    hashAdd(rmdMetricHashTable, hash, &entry->hash);
}

static __inline__ void rmdMetricActiveListAdd(ClRmdMetricT *entry)
{
    if(!entry->list.pNext && !entry->list.pPrev)
        clListAddTail(&entry->list, &rmdMetricActiveList);
}

static __inline__ void rmdMetricActiveListClear(void)
{
    while(!CL_LIST_HEAD_EMPTY(&rmdMetricActiveList))
    {
        ClListHeadT *next = rmdMetricActiveList.pNext;
        clListDel(next);
    }
}

static ClRmdMetricT *rmdMetricFind(ClUint32T clientId, ClUint32T funcId)
{
    ClUint32T hash = RMD_METRIC_HASH(clientId, funcId) & RMD_METRIC_HASH_TABLE_MASK;
    struct hashStruct *iter = NULL;
    for(iter = rmdMetricHashTable[hash]; iter; iter = iter->pNext)
    {
        ClRmdMetricT *rmdMetric = hashEntry(iter, ClRmdMetricT, hash);
        if(rmdMetric->clientId == clientId && rmdMetric->funcId == funcId)
            return rmdMetric;
    }
    return NULL;
}

ClRcT rmdMetricUpdate(ClRcT status, ClUint32T clientId, ClUint32T funcId, ClBufferHandleT message, ClBoolT response)
{
    ClRmdMetricT *entry = NULL;
    ClUint32T messageLen = 0;
    if(!rmdMetricStatus) return CL_OK;
    clOsalMutexLock(&rmdMetricMutex);
    entry = rmdMetricFind(clientId, funcId);
    if(!entry)
    {
        entry = clHeapCalloc(1, sizeof(*entry));
        CL_ASSERT(entry != NULL);
        entry->clientId = clientId;
        entry->funcId = funcId;
        rmdMetricAdd(entry, clientId, funcId);
    }
    if(message)
    {
        clBufferLengthGet(message, &messageLen);
    }
    if(status != CL_OK) ++entry->numErrors;
    if(!response)
    {
        ++entry->numRequests;
        if(messageLen < entry->minRequestBytes
           ||
           !entry->minRequestBytes)
            entry->minRequestBytes = messageLen;

        if(messageLen > entry->maxRequestBytes)
            entry->maxRequestBytes = messageLen;

        entry->totalRequestBytes += messageLen;
        entry->requestTime = clOsalStopWatchTimeGet();
    }
    else
    {
        ClTimeT responseTime = 0;
        ++entry->numResponses;
        if(messageLen < entry->minResponseBytes
            ||
           !entry->minResponseBytes)
            entry->minResponseBytes = messageLen;

        if(messageLen > entry->maxResponseBytes)
            entry->maxResponseBytes = messageLen;

        entry->totalResponseBytes += messageLen;
        responseTime = clOsalStopWatchTimeGet() - entry->requestTime;
        entry->totalResponseTime += responseTime;
        if(responseTime < entry->minResponseTime
           ||
           !entry->minResponseTime)
            entry->minResponseTime = responseTime;

        if(responseTime > entry->maxResponseTime)
            entry->maxResponseTime = responseTime;

        rmdMetricActiveListAdd(entry);
    }
    clOsalMutexUnlock(&rmdMetricMutex);
    return CL_OK;
}

static void rmdMetricDisplay(ClRmdMetricT *entry, ClInt32T index)
{
    clLogNotice("RMD", "METRIC", "RMD Metric Entry [%d]", index);
    clLogMultiline(CL_LOG_NOTICE, "RMD", "METRIC", "Client [%d], Func [%d], Total request bytes [%lld], "
                   "Total response bytes [%lld], Total requests [%lld], Total responses [%lld]\n"
                   "Min request bytes [%lld], Max request bytes [%lld], Avg request bytes [%.2f]\n"
                   "Min response bytes [%lld], Max response bytes [%lld], Avg response bytes [%.2f]\n"
                   "Min response time [%lld] usecs, Max response time [%lld] usecs, "
                   "Average response time [%.2f] usecs, Total errors [%lld]",
                   entry->clientId, entry->funcId, entry->totalRequestBytes, entry->totalResponseBytes,
                   entry->numRequests, entry->numResponses,
                   entry->minRequestBytes, entry->maxRequestBytes, (entry->totalRequestBytes*1.0)/entry->numRequests,
                   entry->minResponseBytes, entry->maxResponseBytes, 
                   (entry->totalResponseBytes*1.0)/entry->numResponses,
                   entry->minResponseTime, entry->maxResponseTime, 
                   (entry->totalResponseTime*1.0)/( (entry->numRequests + entry->numResponses) >> 1 ),
                   entry->numErrors);
}

void rmdMetricDBDump(void)
{
    struct hashStruct *iter = NULL;
    ClInt32T i;
    ClInt32T index = 1;
    if(!rmdMetricStatus) return;
    for(i = 0; i < RMD_METRIC_HASH_TABLE_SIZE; ++i)
    {
        for(iter = rmdMetricHashTable[i]; iter; iter = iter->pNext)
        {
            ClRmdMetricT *rmdMetric = hashEntry(iter, ClRmdMetricT, hash);
            rmdMetricDisplay(rmdMetric, index);
            ++index;
        }
    }
}

static ClRcT rmdMetricDumpInterval(void *unused)
{
    ClListHeadT *iter = NULL;
    ClInt32T index = 1;
    clOsalMutexLock(&rmdMetricMutex);
    CL_LIST_FOR_EACH(iter, &rmdMetricActiveList)
    {
        ClRmdMetricT *rmdMetric = CL_LIST_ENTRY(iter, ClRmdMetricT, list);
        rmdMetricDisplay(rmdMetric, index);
        ++index;
    }
    rmdMetricActiveListClear();
    clOsalMutexUnlock(&rmdMetricMutex);
    return CL_OK;
}

ClRcT rmdMetricInitialize(void)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT delay = {.tsSec = 60, .tsMilliSec = 0 };
    rmdMetricStatus = 0;
    if(clParseEnvBoolean("ASP_RMD_METRIC") == CL_TRUE)
        rmdMetricStatus = 1;

    if(!rmdMetricStatus)
        return CL_OK;
    clOsalMutexInit(&rmdMetricMutex);
    rc = clTimerCreateAndStart(delay, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT, 
                               rmdMetricDumpInterval,  NULL, &rmdMetricTimer);
    return rc;
}

ClRcT rmdMetricFinalize(void)
{
    if(!rmdMetricStatus) return CL_OK;
    clTimerDelete(&rmdMetricTimer);
    rmdMetricStatus = 0;
    return CL_OK;
}

static ClRcT cliRmdStatsShow(int argc, char **argv, char **ret)
{
    ClDebugPrintHandleT msgHandle = 0;
    ClRmdStatsT stats = {0};
    ClRcT rc = CL_OK;

    if(argc != 1 || !ret) return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
    *ret = NULL;
    
    if( (rc = clDebugPrintInitialize(&msgHandle)) != CL_OK)
        goto out;

    if( (rc = clRmdStatsGet(&stats) ) != CL_OK)
    {
        clDebugPrint(msgHandle, "RMD stats get returned with [%#x]\n", rc);
        goto out_free;
    }
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num calls", stats.nRmdCalls);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num failed calls", stats.nFailedCalls);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num resend requests:", stats.nResendRequests);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num failed resend requests:", stats.nFailedResendRequests);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num timeouts", stats.nCallTimeouts);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num replies received", stats.nRmdReplies);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num bad replies received", stats.nBadReplies);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num requests received", stats.nRmdRequests);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num bad requests received", stats.nBadRequests);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num dup atmost once requests", stats.nDupRequests);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num atmost once calls", stats.nAtmostOnceCalls);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num failed atmost once calls", stats.nFailedAtmostOnceCalls);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num atmost once acks sent", stats.nAtmostOnceAcksSent);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num atmost once acks send failed", stats.nFailedAtmostOnceAcksSent);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num atmost once acks received", stats.nAtmostOnceAcksRecvd);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num atmost once acks recv failed", stats.nFailedAtmostOnceAcksRecvd);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num reply sent", stats.nReplySend);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num reply send failed", stats.nFailedReplySend);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num reply resent", stats.nResendReplies);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num reply resend failed", stats.nFailedResendReplies);
    clDebugPrint(msgHandle, "%-30s:%-8d\n", "Num calls optimized", stats.nRmdCallOptimized);
    
    out_free:
    clDebugPrintFinalize(&msgHandle, ret);

    out:
    return rc;
}

static ClHandleT gRmdDebugReg;

static ClDebugFuncEntryT gClRmdCliTab[] = {
    {(ClDebugCallbackT) cliRmdStatsShow, "rmdStatsShow", "Show RMD stats"} ,
};

ClRcT clRmdDebugRegister(void)
{
    ClRcT rc = CL_OK;
    if(!gRmdDebugReg)
    {
        rc = clDebugRegister(gClRmdCliTab,
                             sizeof(gClRmdCliTab) / sizeof(gClRmdCliTab[0]), 
                             &gRmdDebugReg);
    }
    return rc;
}

ClRcT clRmdDebugDeregister()
{
    ClRcT rc = CL_OK;
    if(gRmdDebugReg)
    {
        rc = clDebugDeregister(gRmdDebugReg);
        gRmdDebugReg = CL_HANDLE_INVALID_VALUE;
    }
    return rc;
}
