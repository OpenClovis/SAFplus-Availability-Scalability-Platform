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

/**
 * This file implements CPM client side non SAF functionality.
 */

/*
 * Standard header files 
 */
#define __CLIENT__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <signal.h>
#include <sys/wait.h>
#endif
/*
 * ASP header files 
 */
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <saAmf.h>
/*
 * ASP internal header files
 */
#include <clEoIpi.h>
#include <clIocIpi.h>
#include <clIocLogicalAddresses.h>
#include <clNodeCache.h>
/*
 * CPM internal header files 
 */
#include <clCpmClient.h>
#include <clCpmCommon.h>
#include <clCpmLog.h>
#include <clCpmInternal.h>
#include <clCpmClientFuncList.h>
/*
 * XDR header files 
 */
#include "xdrClCpmCompInitSendT.h"
#include "xdrClCpmCompInitRecvT.h"
#include "xdrClCpmLifeCycleOprT.h"
#include "xdrClCpmLocalInfoT.h"
#include "xdrClCpmComponentStateT.h"
#include "xdrClIocAddressIDLT.h"
#include "xdrClCpmBootOperationT.h"
#include "xdrClCpmCompLAUpdateT.h"
#include "xdrClCpmSlotInfoRecvT.h"
#include "xdrClCpmSlotInfoFieldIdT.h"
#include "xdrClCpmNodeConfigT.h"
#include "xdrClCpmCompConfigSetT.h"
#include "xdrClCpmRestartSendT.h"
#include "xdrClCpmMiddlewareResetT.h"
#include "xdrClCpmCompSpecInfoRecvT.h"
#include "xdrClErrorReportT.h"
#include "xdrClCpmClientInfoIDLT.h"
#include "xdrClEoExecutionObjIDLT.h"

#define CPM_LOG_AREA_RMD		"RMD"
#define CPM_LOG_CTX_RMD_SYNC		"SYN"
#define CPM_LOG_CTX_NODE_SHUTDOWN	"SHUTDOWN"

typedef struct ClTargetClusterInfo
{
    ClTargetInfoT targetInfo;
    ClTargetSlotInfoT *slots;
} ClTargetClusterInfoT;

static ClTargetClusterInfoT gClTargetClusterInfo;

extern ClUint32T clAspLocalId;
extern ClBoolT componentTerminate;

ClRcT clCpmClientRMDSync(ClIocNodeAddressT destAddr,
                         ClUint32T fcnId,
                         ClUint8T *pInBuf,
                         ClUint32T inBufLen,
                         ClUint8T *pOutBuf,
                         ClUint32T *pOutBufLen,
                         ClUint32T flags,
                         ClUint32T timeOut,
                         ClUint32T maxRetries,
                         ClUint8T priority)
{
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress;
    ClBufferHandleT inMsgHdl = { 0 };
    ClBufferHandleT outMsgHdl = { 0 };
    ClUint32T msgLength = 0;
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_CLIENT_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_CLIENT_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_CLIENT_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            goto failure;
        }
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T *) pInBuf,
                                        inBufLen);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_WRITE_ERR, rc);
            clBufferDelete(&inMsgHdl);
            goto failure;
        }
    }
    if (pOutBufLen != NULL)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);

            if(inBufLen) clBufferDelete(&inMsgHdl);
                
            goto failure;
        }
    }
    retCode =
        clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                     (flags) & (~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
    if (inBufLen)
    {
        rc = clBufferDelete(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }
    if (pOutBufLen != NULL &&
        (retCode == CL_OK ||
         CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH))
    {
        rc = clBufferLengthGet(outMsgHdl, &msgLength);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_LOG_MESSAGE_0_INVALID_BUFFER);
        }
        rc = clBufferNBytesRead(outMsgHdl, (ClUint8T *) pOutBuf,
                                       &msgLength);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_READ_ERR, rc);
        }
        rc = clBufferDelete(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }
    else if (retCode != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_RMD_CALL_ERR, retCode);
        clLogError(CPM_LOG_AREA_RMD,CPM_LOG_CTX_RMD_SYNC,
                   "RMD Failed with an error %x\n ", retCode);
    }
  failure:
    return retCode;
}

ClRcT clCpmClientRMDSyncNew(ClIocNodeAddressT destAddr,
                            ClUint32T fcnId,
                            ClUint8T *pInBuf,
                            ClUint32T inBufLen,
                            ClUint8T *pOutBuf,
                            ClUint32T *pOutBufLen,
                            ClUint32T flags,
                            ClUint32T timeOut,
                            ClUint32T maxRetries,
                            ClUint8T priority,
                            ClCpmMarshallT marshallFunction,
                            ClCpmUnmarshallT unmarshallFunction)
{
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress = {{0}};
    ClBufferHandleT inMsgHdl = {0};
    ClBufferHandleT outMsgHdl = {0};
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_CLIENT_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_CLIENT_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_CLIENT_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            goto failure;
        }

        rc = marshallFunction((void *) pInBuf, inMsgHdl, 0);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_WRITE_ERR, rc);
            clBufferDelete(&inMsgHdl);
            goto failure;
        }
    }
    if (pOutBufLen != NULL)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);

            if(inBufLen) clBufferDelete(&inMsgHdl);

            goto failure;
        }
    }
    retCode =
        clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                     (flags) & (~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
    if (inBufLen)
    {
        rc = clBufferDelete(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }
    if (pOutBufLen != NULL &&
        (retCode == CL_OK ||
         CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH))
    {
        rc = unmarshallFunction(outMsgHdl, (void *) pOutBuf);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_READ_ERR, rc);
        }
        rc = clBufferDelete(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }
    else if (pOutBufLen != NULL && retCode != CL_OK)
    {
        rc = clBufferDelete(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }
    else if (retCode != CL_OK && (CL_GET_ERROR_CODE(retCode) != CL_ERR_TRY_AGAIN))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_RMD_CALL_ERR, retCode);
        clLogError(CPM_LOG_AREA_RMD, CPM_LOG_CTX_RMD_SYNC, "RMD failed with an error [0x%x]", retCode);
    }
  failure:
    return retCode;
}

ClRcT clCpmClientRMDAsync(ClIocNodeAddressT destAddr,
                          ClUint32T fcnId,
                          ClUint8T *pInBuf,
                          ClUint32T inBufLen,
                          ClUint8T *pOutBuf,
                          ClUint32T *pOutBufLen,
                          ClUint32T flags,
                          ClUint32T timeOut,
                          ClUint32T maxRetries,
                          ClUint32T priority)
{
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress = {{0}};
    ClBufferHandleT inMsgHdl = {0};
    ClBufferHandleT outMsgHdl = {0};
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_CLIENT_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_CLIENT_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_CLIENT_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            goto failure;
        }
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T *) pInBuf,
                                        inBufLen);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_WRITE_ERR, rc);
            goto out_delete;
        }
    }
    if (pOutBufLen != NULL)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            
            goto out_delete;
        }
    }
    retCode =
        clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                     CL_RMD_CALL_ASYNC | (flags), &rmdOptions, NULL);
    if (retCode != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_RMD_CALL_ERR, retCode);
        goto out_delete;
    }

    out_delete:
    if (inBufLen)
    {
        rc = clBufferDelete(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                       CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }

  failure:
    return retCode;
}

ClRcT clCpmClientRMDAsyncNew(ClIocNodeAddressT destAddr,
                             ClUint32T fcnId,
                             ClUint8T *pInBuf,
                             ClUint32T inBufLen,
                             ClUint8T *pOutBuf,
                             ClUint32T *pOutBufLen,
                             ClUint32T flags,
                             ClUint32T timeOut,
                             ClUint32T maxRetries,
                             ClUint32T priority,
                             ClCpmMarshallT marshallFunction)
{
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress = {{0}};
    ClBufferHandleT inMsgHdl = {0};
    ClBufferHandleT outMsgHdl = {0};
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_CLIENT_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_CLIENT_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_CLIENT_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
   
    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            goto failure;
        }
        rc = marshallFunction((void *) pInBuf, inMsgHdl, 0);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_BUF_WRITE_ERR, rc);
            goto out_delete;
        }
    }
    if (pOutBufLen != NULL)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
            goto out_delete;
        }
    }
    retCode = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl, CL_RMD_CALL_ASYNC | (flags), &rmdOptions, NULL);
    if (retCode != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_RMD_CALL_ERR, retCode);
        goto out_delete;
    }

    out_delete:    
    if (inBufLen)
    {
        rc = clBufferDelete(&inMsgHdl);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_BUF_DELETE_ERR, rc);
        }
    }

  failure:
    return retCode;
}

ClRcT clCpmExecutionObjectStateSet(ClIocNodeAddressT compAddr, ClEoIdT eoId, ClEoStateT state)
{
    ClRcT rc = CL_OK;

    ClCpmClientInfoT compInfo;
    memset(&compInfo, 0, sizeof(ClCpmClientInfoT));
    compInfo.version = CL_CPM_EO_VERSION_NO;
    compInfo.state = state;
    compInfo.eoId = eoId;

    rc = clCpmClientRMDSync(compAddr, COMPO_SET_STATE, (ClUint8T *) &compInfo,
                            sizeof(ClCpmClientInfoT), NULL, NULL, 0, 0, 0, 0);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_EO_STATE_SET_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("clCpmExecutionObjectStateSet  Failed %d\n", rc), rc);

  failure:
    return rc;
}

ClRcT clCpmExecutionObjectRegister(ClEoExecutionObjT *pEOptr)
{
    ClRcT rc = CL_OK;
    ClEoIdT eoID = 0;
    ClUint32T outLen = sizeof(eoID);
    SaNameT compName = { 0, {0} };
    VDECL_VER(ClCpmClientInfoIDLT, 4, 0, 0) compInfo;

    if (pEOptr == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                         ("clCpmExecutionObjectRegister: NULL ptr passed \n"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memset(&compInfo, 0, sizeof(compInfo));
    rc = clCpmComponentNameGet(0, &compName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("Unable to Get Component Name, rc = %x\n", rc), rc);

    memcpy(&(compInfo.compName), &compName, sizeof(SaNameT));
    compInfo.version = CL_CPM_EO_VERSION_NO;
    strncpy(compInfo.eoObj.name, pEOptr->name, sizeof(compInfo.eoObj.name)-1);
    compInfo.eoObj.eoID = pEOptr->eoID;
    compInfo.eoObj.pri = pEOptr->pri;
    compInfo.eoObj.state = pEOptr->state;
    compInfo.eoObj.threadRunning = pEOptr->threadRunning;
    compInfo.eoObj.noOfThreads = pEOptr->noOfThreads;
    compInfo.eoObj.eoInitDone = pEOptr->eoInitDone;
    compInfo.eoObj.eoSetDoneCnt = pEOptr->eoSetDoneCnt;
    compInfo.eoObj.refCnt = pEOptr->refCnt;
    compInfo.eoObj.eoPort = pEOptr->eoPort;
    compInfo.eoObj.appType = pEOptr->appType;
    compInfo.eoObj.maxNoClients = pEOptr->maxNoClients;
    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), COMPO_REGIS_EO,
                               (ClUint8T *) &compInfo, sizeof(ClCpmClientInfoT),
                               (ClUint8T *) &eoID, &outLen,
                               CL_RMD_CALL_ATMOST_ONCE | CL_RMD_CALL_NEED_REPLY, 0,
                               0, 0,
                               MARSHALL_FN(ClCpmClientInfoIDLT, 4, 0, 0),
                               clXdrUnmarshallClUint64T);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_EO_REG_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("clCpmExecutionObjectRegister Failed, rc = %x\n", rc),
                     rc);
    pEOptr->eoID = eoID;

    failure:
    return rc;
}

ClRcT clCpmFuncEOWalk(ClCpmFuncWalkT *pWalk)
{
    ClRcT rc = CL_OK;
    ClCpmClientInfoT *pCompInfo = NULL;

    if (pWalk == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                         ("clCpmFuncEOWalk: Null ptr passed \n"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    pCompInfo =
        (ClCpmClientInfoT *) clHeapAllocate(sizeof(ClCpmClientInfoT) +
                                          pWalk->inLen);
    if (pCompInfo == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Unable to malloc \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    }
    pCompInfo->walkInfo.funcNo = pWalk->funcNo;
    pCompInfo->walkInfo.inLen = pWalk->inLen;
    memcpy(pCompInfo->walkInfo.inputArg, pWalk->inputArg, pWalk->inLen);
    pCompInfo->version = CL_CPM_EO_VERSION_NO;

    rc = clCpmClientRMDSync(clIocLocalAddressGet(), COMPO_FUNC_WALK_EOS,
                            (ClUint8T *) pCompInfo,
                            (sizeof(ClCpmClientInfoT) + (pWalk->inLen)), NULL,
                            NULL, 0, 0, 0, 0);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_EO_FUNC_WALK_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("clCpmFuncEOWalk Failed, rc = %x \n", rc),
                     rc);

    clHeapFree(pCompInfo);
    pCompInfo = NULL;
  failure:
    return rc;
}

ClRcT clCpmExecutionObjectStateUpdate(ClEoExecutionObjT *pEOptr)
{
    ClRcT rc = CL_OK;
    VDECL_VER(ClCpmClientInfoIDLT, 4, 0, 0) compInfo = {0};
    SaNameT compName = {0};

    if (pEOptr == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clCpmExecutionObjectStateUpdate: Null ptr passed, rc = %x\n",
                   rc);
        return CL_CPM_RC(CL_ERR_NULL_POINTER);
    }

    memset(&compInfo, 0, sizeof(compInfo));
    compInfo.version = CL_CPM_EO_VERSION_NO;
    strncpy(compInfo.eoObj.name, pEOptr->name, sizeof(compInfo.eoObj.name)-1);
    compInfo.eoObj.eoID = pEOptr->eoID;
    compInfo.eoObj.pri = pEOptr->pri;
    compInfo.eoObj.state = pEOptr->state;
    compInfo.eoObj.threadRunning = pEOptr->threadRunning;
    compInfo.eoObj.noOfThreads = pEOptr->noOfThreads;
    compInfo.eoObj.eoInitDone = pEOptr->eoInitDone;
    compInfo.eoObj.eoSetDoneCnt = pEOptr->eoSetDoneCnt;
    compInfo.eoObj.refCnt = pEOptr->refCnt;
    compInfo.eoObj.eoPort = pEOptr->eoPort;
    compInfo.eoObj.appType = pEOptr->appType;
    compInfo.eoObj.maxNoClients = pEOptr->maxNoClients;
    rc = clCpmComponentNameGet(0, &compName);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_LCM_COMP_NAME_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("Unable to Get Component Name, rc = %x\n", rc), rc);

    memcpy(&(compInfo.compName), &compName, sizeof(SaNameT));
    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), COMPO_UPD_EOINFO,
                               (ClUint8T *) &compInfo, sizeof(ClCpmClientInfoT),
                               NULL, NULL, 0, 0, 0, 0,
                               MARSHALL_FN(ClCpmClientInfoIDLT, 4, 0, 0),
                               NULL);

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_EO_STATE_UPDATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("clCpmExecutionObjectStateUpdate Failed, rc = %x \n", rc),
                     rc);

  failure:
    return rc;
}

/**
 * Obsolete CPM life cycle management functions. Will be removed in future.
 */

ClRcT clCpmComponentInstantiate(SaNameT *compName, SaNameT *nodeName, ClCpmLcmReplyT *srcInfo)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT compInstantiate = {{0}};

    if (compName == NULL || nodeName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"), CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(compInstantiate.name), compName, sizeof(SaNameT));
    memcpy(&(compInstantiate.nodeName), nodeName, sizeof(SaNameT));
    if (srcInfo != NULL)
    {
        compInstantiate.srcAddress.nodeAddress = srcInfo->srcIocAddress;
        compInstantiate.srcAddress.portId = srcInfo->srcPort;
        compInstantiate.rmdNumber = srcInfo->rmdNumber;
    }
    else
    {
        compInstantiate.srcAddress.nodeAddress = 0;
        compInstantiate.srcAddress.portId = 0;
        compInstantiate.rmdNumber = 0;
    }

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_COMPONENT_INSTANTIATE, (ClUint8T *) &compInstantiate,
                                sizeof(ClCpmLifeCycleOprT), NULL, NULL, 0, 0, 0, 0, MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0));

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_CLIENT_COMP_INSTANTIATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("clCpmComponentInstantiate Failed rc =%x\n", rc), rc);

  failure:
    return rc;
}

ClRcT clCpmComponentTerminate(SaNameT *compName,
                              SaNameT *nodeName,
                              ClCpmLcmReplyT *srcInfo)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT compTerminate = {{0}};

    if (compName == NULL || nodeName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(compTerminate.name), compName, sizeof(SaNameT));
    memcpy(&(compTerminate.nodeName), nodeName, sizeof(SaNameT));

    if (srcInfo != NULL)
    {
        compTerminate.srcAddress.nodeAddress = srcInfo->srcIocAddress;
        compTerminate.srcAddress.portId = srcInfo->srcPort;
        compTerminate.rmdNumber = srcInfo->rmdNumber;
    }
    else
    {
        compTerminate.srcAddress.nodeAddress = 0;
        compTerminate.srcAddress.portId = 0;
        compTerminate.rmdNumber = 0;
    }

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_COMPONENT_TERMINATE,
                                (ClUint8T *) &compTerminate,
                                sizeof(ClCpmLifeCycleOprT), NULL, NULL,
                                0, 0, 0, 0,
                                MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0)
    );

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_TERMINATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("clCpmComponentTerminate Failed rc =%x\n", rc), rc);

  failure:
    return rc;
}

ClRcT clCpmComponentCleanup(SaNameT *compName,
                            SaNameT *nodeName,
                            ClCpmLcmReplyT *srcInfo)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT compCleanup = {{0}};

    if (compName == NULL || nodeName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(compCleanup.name), compName, sizeof(SaNameT));
    memcpy(&(compCleanup.nodeName), nodeName, sizeof(SaNameT));

    if (srcInfo != NULL)
    {
        compCleanup.srcAddress.nodeAddress = srcInfo->srcIocAddress;
        compCleanup.srcAddress.portId = srcInfo->srcPort;
        compCleanup.rmdNumber = srcInfo->rmdNumber;
    }
    else
    {
        compCleanup.srcAddress.nodeAddress = 0;
        compCleanup.srcAddress.portId = 0;
        compCleanup.rmdNumber = 0;
    }

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_COMPONENT_CLEANUP,
                                (ClUint8T *) &compCleanup,
                                sizeof(ClCpmLifeCycleOprT), NULL, NULL,
                                0, 0, 0, 0,
                                MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0)
    );

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_CLEANUP_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("clCpmComponentCleanup Failed rc =%x\n", rc), rc);

  failure:
    return rc;
}

ClRcT clCpmComponentRestart(SaNameT *compName,
                            SaNameT *nodeName,
                            ClCpmLcmReplyT *srcInfo)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT compRestart = {{0}};

    if (compName == NULL || nodeName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(compRestart.name), compName, sizeof(SaNameT));
    memcpy(&(compRestart.nodeName), nodeName, sizeof(SaNameT));
    if (srcInfo != NULL)
    {
        compRestart.srcAddress.nodeAddress = srcInfo->srcIocAddress;
        compRestart.srcAddress.portId = srcInfo->srcPort;
        compRestart.rmdNumber = srcInfo->rmdNumber;
    }
    else
    {
        compRestart.srcAddress.nodeAddress = 0;
        compRestart.srcAddress.portId = 0;
        compRestart.rmdNumber = 0;
    }

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_COMPONENT_RESTART,
                                (ClUint8T *) &compRestart,
                                sizeof(ClCpmLifeCycleOprT), NULL, NULL,
                                CL_RMD_CALL_ATMOST_ONCE, 0, 0, 0,
                                MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0)
    );

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_RESTART_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("clCpmComponentRestart Failed rc =%x\n", rc), rc);

  failure:
    return rc;
}

ClRcT clCpmComponentLogicalAddressUpdate(SaNameT *compName,
                                         ClIocAddressT *logicalAddress)
{
    ClRcT rc = CL_OK;
    ClCpmCompLAUpdateT cpmLA = {{0}};

    if (compName == NULL || logicalAddress == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    strcpy((cpmLA.compName), (const ClCharT*)compName->value);
    memcpy(&(cpmLA.logicalAddress), logicalAddress, sizeof(ClIocAddressT));

    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), CPM_COMPONENT_LA_UPDATE,
                               (ClUint8T *) &cpmLA, sizeof(ClCpmCompLAUpdateT),
                               NULL, NULL, 0, 0, 0, 0,
                               MARSHALL_FN(ClCpmCompLAUpdateT, 4, 0, 0),
                               NULL);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_LA_UPDATE_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);

  failure:
    return rc;
}

ClRcT clCpmComponentPIDGetBySlot(ClIocNodeAddressT slot, const SaNameT *compName, ClUint32T *pid)
{
    ClRcT rc = CL_OK;
    ClUint32T tempPid = 0;
    ClUint32T size = sizeof(ClUint32T);

    if (compName == NULL || pid == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    if(!slot) slot = clIocLocalAddressGet();

    rc = clCpmClientRMDSyncNew(slot, CPM_COMPONENT_PID_GET,
                               (ClUint8T *) compName, sizeof(SaNameT),
                               (ClUint8T *) &tempPid, &size,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               clXdrMarshallSaNameT,
                               clXdrUnmarshallClUint32T);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_PID_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);
    *pid = tempPid;

  failure:
    return rc;

}

ClRcT clCpmComponentPIDGet(const SaNameT *compName, ClUint32T *pid)
{
    return clCpmComponentPIDGetBySlot(clIocLocalAddressGet(), compName, pid);
}

ClRcT clCpmComponentAddressGet(ClIocNodeAddressT nodeAddress,
                               SaNameT *compName,
                               ClIocAddressT *compAddress)
{
    ClRcT               rc = CL_OK;
    ClUint32T           bufSize = 0;
    ClIocAddressIDLT    idlCompAddress;

    if (compName == NULL || compAddress == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"), CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    bufSize = sizeof(ClIocAddressIDLT);
    rc = clCpmClientRMDSyncNew(nodeAddress, CPM_COMPONENT_ADDRESS_GET, (ClUint8T *) compName, sizeof(SaNameT),
                               (ClUint8T *) &idlCompAddress, &bufSize, CL_RMD_CALL_NEED_REPLY, 0, 0, 0, clXdrMarshallSaNameT,
                               UNMARSHALL_FN(ClIocAddressIDLT, 4, 0, 0));
    
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_CLIENT_COMP_ADDR_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);

    /*
     * Assuming we always get the physical address 
     */
    if (idlCompAddress.discriminant == CLIOCADDRESSIDLTIOCPHYADDRESS)
    {
        compAddress->iocPhyAddress.nodeAddress = idlCompAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress;
        compAddress->iocPhyAddress.portId = idlCompAddress.clIocAddressIDLT.iocPhyAddress.portId;
    }

  failure:
    return rc;
}

/*
 * Get component address with highest priority and short timeout 200ms
 * (not supported) just for internal using
 */
ClRcT clCpmComponentAddressGetFast(ClIocNodeAddressT nodeAddress, SaNameT *compName, ClIocAddressT *compAddress)
{
    ClRcT rc = CL_OK;
    ClUint32T bufSize = 0;
    ClIocAddressIDLT idlCompAddress;

    memset(&idlCompAddress,0,sizeof(ClIocAddressIDLT));
    if (compName == NULL || compAddress == NULL )
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"), CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    bufSize = sizeof(ClIocAddressIDLT);
    rc = clCpmClientRMDSyncNew(nodeAddress, CPM_COMPONENT_ADDRESS_GET, (ClUint8T *) compName, sizeof(SaNameT), (ClUint8T *) &idlCompAddress,
                    &bufSize, CL_RMD_CALL_NEED_REPLY, CL_CPM_CLIENT_CONFIG_TIMEOUT, CL_CPM_CLIENT_CONFIG_RETRIES,
                    CL_CPM_CLIENT_HIGH_PRIORITY, clXdrMarshallSaNameT, UNMARSHALL_FN(ClIocAddressIDLT, 4, 0, 0));

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB, CL_CPM_LOG_1_CLIENT_COMP_ADDR_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);

    /*
     * Assuming we always get the physical address
     */
    if (idlCompAddress.discriminant == CLIOCADDRESSIDLTIOCPHYADDRESS)
    {
        compAddress->iocPhyAddress.nodeAddress = idlCompAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress;
        compAddress->iocPhyAddress.portId = idlCompAddress.clIocAddressIDLT.iocPhyAddress.portId;
    }

    failure: return rc;
}

ClRcT clCpmComponentIdGet(ClCpmHandleT cpmHandle,
                          SaNameT *compName,
                          ClUint32T *compId)
{
    ClRcT rc = CL_OK;
    SaNameT compName1;
    compName1.length = compName->length; 
    memcpy(compName1.value,compName->value,compName->length+1);
    rc=clAmfGetComponentId(cpmHandle,&compName1,compId);
    
    return rc;


}

ClRcT clAmfGetComponentId(ClCpmHandleT cpmHandle,
                          SaNameT *compName,
                          ClUint32T *compId)
{
    ClRcT rc = CL_OK;
    ClUint32T bufSize = 0;
    static ClUint32T compIdSpace;

    if (compName == NULL || compId == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed\n"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    bufSize = sizeof(compId);
    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), CPM_COMPONENT_ID_GET,
                               (ClUint8T *) compName, sizeof(SaNameT),
                               (ClUint8T *) compId, &bufSize,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               clXdrMarshallSaNameT,
                               clXdrUnmarshallClUint32T);
   if (rc != CL_OK)
    {
        rc = CL_OK;
        *compId = (clIocLocalAddressGet() << CL_CPM_IOC_SLOT_BITS) + compIdSpace;
        ++compIdSpace;
    }

  failure:
    return rc;
}



ClRcT clCpmComponentStatusGet(SaNameT *compName,
                              SaNameT *nodeName,
                              ClUint32T *presenceState,
                              ClUint32T *OperationalState)
{
    ClRcT rc = CL_OK;
    ClCpmComponentStateT state = {CL_AMS_OPER_STATE_NONE};
    ClCpmLifeCycleOprT status = {{0}};
    ClUint32T size = sizeof(ClCpmComponentStateT);

    if (compName == NULL || presenceState == NULL || OperationalState == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed\n"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * if nodeName is NULL, then it is for local node 
     */
    if (nodeName == NULL)
    {
        status.nodeName.length = 0;
    }
    else
        memcpy(&(status.nodeName), nodeName, sizeof(SaNameT));
    memcpy(&(status.name), compName, sizeof(SaNameT));

    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), CPM_COMPONENT_STATUS_GET,
                               (ClUint8T *) &status, sizeof(ClCpmLifeCycleOprT),
                               (ClUint8T *) &state, &size,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0),
                               UNMARSHALL_FN(ClCpmComponentStateT, 4, 0, 0));
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_STATUS_GET_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);

    *presenceState = state.compPresenceState;
    *OperationalState = state.compOperState;

  failure:
    return rc;
}

ClRcT clCpmCpmLocalRegister(ClCpmLocalInfoT *cpmLocalInfo)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterIocAddress = 0;

    if (cpmLocalInfo == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB, CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"), CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    rc = clCpmMasterAddressGet(&masterIocAddress);
    if (rc != CL_OK)
        goto failure;

    if(masterIocAddress != clIocLocalAddressGet())
    {
        rc = clCpmClientRMDSyncNew(masterIocAddress, CPM_CPML_REGISTER,
                                   (ClUint8T *) cpmLocalInfo,
                                   sizeof(ClCpmLocalInfoT), NULL, NULL, 0, 0, 0,
                                   CL_IOC_HIGH_PRIORITY, MARSHALL_FN(ClCpmLocalInfoT, 4, 0, 0),
                                   NULL
                                   );
    }
    else
    {
        rc = clCpmClientRMDAsyncNew(masterIocAddress, CPM_CPML_REGISTER,
                                   (ClUint8T *) cpmLocalInfo,
                                   sizeof(ClCpmLocalInfoT), NULL, NULL, 0, 0, 0,
                                    CL_IOC_HIGH_PRIORITY, MARSHALL_FN(ClCpmLocalInfoT, 4, 0, 0));
    }

  failure:
    return rc;
}

ClRcT clCpmCpmLocalDeregister(ClCpmLocalInfoT *cpmLocalInfo)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterIocAddress = 0;

    if (cpmLocalInfo == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    rc = clCpmMasterAddressGet(&masterIocAddress);
    if (rc != CL_OK)
        goto failure;

    rc = clCpmClientRMDSyncNew(masterIocAddress, CPM_CPML_DEREGISTER,
            (ClUint8T *) cpmLocalInfo,
            sizeof(ClCpmLocalInfoT), NULL, NULL, 0, 0, 0,
            CL_IOC_HIGH_PRIORITY, MARSHALL_FN(ClCpmLocalInfoT, 4, 0, 0),
            NULL
            );

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_CPML_UNREG_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("%s Failed rc =%x\n", __FUNCTION__, rc), rc);

  failure:
    return rc;
}

ClRcT clCpmBootLevelGet(CL_IN SaNameT *nodeName, CL_OUT ClUint32T *bootLevel)
{
    ClRcT rc = CL_OK;
    ClUint32T size = 0;
    ClCpmBootOperationT bootOp = {0};

    size = sizeof(ClUint32T);

    if (nodeName == NULL || bootLevel == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(bootOp.nodeName), nodeName, sizeof(SaNameT));

    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), CPM_BM_GET_CURRENT_LEVEL,
                               (ClUint8T *) &bootOp,
                               sizeof(ClCpmBootOperationT),
                               (ClUint8T *) bootLevel, &size,
                               (CL_RMD_CALL_NEED_REPLY), 0, 0, 0,
                               MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0),
                               clXdrUnmarshallClUint32T);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_BM_GET_LEVEL_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("%s Failed rc =%x \n", __FUNCTION__, rc),
                     rc);
  failure:
    return rc;
}

ClRcT clCpmBootLevelSet(CL_IN SaNameT *nodeName,
                        CL_IN ClCpmLcmReplyT *srcInfo,
                        CL_IN ClUint32T bootLevel)
{
    ClRcT rc = CL_OK;
    ClCpmBootOperationT bootOp = {0};
    ClUint32T size = 0;

    size = sizeof(ClCpmBootOperationT);

    if (nodeName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    if (srcInfo == NULL)
    {
        bootOp.srcAddress.nodeAddress = 0;
        bootOp.srcAddress.portId = 0;
        bootOp.rmdNumber = 0;
    }
    else
    {
        bootOp.srcAddress.nodeAddress = srcInfo->srcIocAddress;
        bootOp.srcAddress.portId = srcInfo->srcPort;
        bootOp.rmdNumber = srcInfo->rmdNumber;
    }
    memcpy(&(bootOp.nodeName), nodeName, sizeof(SaNameT));
    bootOp.bootLevel = bootLevel;
    bootOp.requestType = CL_CPM_REQUEST_NONE;

    rc = clCpmClientRMDAsyncNew(clIocLocalAddressGet(), CPM_BM_SET_LEVEL, (ClUint8T *) &bootOp, size, NULL, 0, 0, 0, 0, CL_IOC_HIGH_PRIORITY,
            MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0));
    
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_BM_SET_LEVEL_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("clCpmBootLevelSet Failed rc =%x \n", rc),
                     rc);

  failure:
    return rc;
}

ClRcT clCpmBootLevelMax(CL_IN SaNameT *nodeName, CL_OUT ClUint32T *bootLevel)
{
    ClRcT rc = CL_OK;
    ClCpmBootOperationT bootOp = {0};
    ClUint32T size = 0;

    if (nodeName == NULL || bootLevel == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    memcpy(&(bootOp.nodeName), nodeName, sizeof(SaNameT));
    size = sizeof(ClUint32T);

    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), CPM_BM_GET_MAX_LEVEL,
                               (ClUint8T *) &bootOp,
                               sizeof(ClCpmBootOperationT),
                               (ClUint8T *) bootLevel, &size,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0),
                               clXdrUnmarshallClUint32T);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_BM_GET_MAX_LEVEL_ERR, rc);
    }

    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("%s Failed rc =%x \n", __FUNCTION__, rc),
                     rc);

  failure:
    return rc;
}

void cpmGetOwnLogicalAddress(ClIocLogicalAddressT * logicalAddress)
{
    *logicalAddress = CL_IOC_AMF_LOGICAL_ADDRESS;
}

ClRcT clCpmMasterAddressGetExtended(ClIocNodeAddressT *pIocAddress,
                                    ClInt32T numRetries,
                                    ClTimerTimeOutT *pDelay)
{
    ClRcT rc = CL_OK;
    ClIocLogicalAddressT logicalAddr;

    if (pIocAddress == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     *  Generate the Logical address 0x0000000000000001 
     */
    cpmGetOwnLogicalAddress(&logicalAddr);

    if(!numRetries && !pDelay)
        rc = clIocMasterAddressGet(logicalAddr,CL_IOC_CPM_PORT,pIocAddress);
    else
        rc = clIocMasterAddressGetExtended(logicalAddr, CL_IOC_CPM_PORT, pIocAddress,
                                           numRetries, pDelay);
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_SUPPORTED)
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }

  failure:
    return rc;
}

ClRcT clCpmMasterAddressGet(ClIocNodeAddressT *pIocAddress)
{
    return clCpmMasterAddressGetExtended(pIocAddress, 0, NULL);
}

ClUint32T clCpmIsMaster(void)
{
    ClRcT rc;
    ClIocNodeAddressT masterIocAddress = 0;
    ClIocNodeAddressT localIocAddress = 0;

    rc = clCpmMasterAddressGet(&masterIocAddress);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   "Failed to get master CPM address. error code [0x%x].", rc);
        return CL_FALSE;
    }

    localIocAddress = clIocLocalAddressGet();

    if (localIocAddress == masterIocAddress)
        return CL_TRUE;
    else
        return CL_FALSE;
}

ClRcT clCpmComponentListDebugAll(ClIocNodeAddressT iocNodeAddress,
                                 ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress = {{0}};
    ClBufferHandleT outMsgHdl = 0;
    ClUint32T msgLength = 0;
    ClCharT buffer[CL_MAX_NAME_LENGTH] = {0};
    
    if (!retStr) return CL_CPM_RC(CL_ERR_NULL_POINTER);
    
    rmdOptions.timeout = CL_CPM_CLIENT_DFLT_TIMEOUT;
    rmdOptions.retries = CL_CPM_CLIENT_DFLT_RETRIES;
    rmdOptions.priority = CL_IOC_LOW_PRIORITY;

    iocAddress.iocPhyAddress.nodeAddress = iocNodeAddress;
    iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

    rc = clBufferCreate(&outMsgHdl);
    if (CL_OK != rc) return rc;

    rc = clRmdWithMsg(iocAddress,
                      CPM_COMPONENT_DEBUG_LIST_GET,
                      0,
                      outMsgHdl,
                      (CL_RMD_CALL_NEED_REPLY) & (~CL_RMD_CALL_ASYNC),
                      &rmdOptions,
                      NULL);
    if (CL_OK != rc) goto failure;
    
    rc = clBufferLengthGet(outMsgHdl, &msgLength);
    if (CL_OK != rc) goto failure;

    *retStr = (ClCharT *) clHeapAllocate(msgLength);
    if (!*retStr) goto failure;
    
    rc = clBufferNBytesRead(outMsgHdl, (ClUint8T *) *retStr, &msgLength);
    if (CL_OK != rc) goto failure;

    clBufferDelete(&outMsgHdl);

    return CL_OK;
    
 failure:
    snprintf(buffer, CL_MAX_NAME_LENGTH-1, "Failed to get component list, error [%#x]", rc);
    *retStr = (ClCharT* )clHeapAllocate(CL_MAX_NAME_LENGTH);
    if (*retStr) strncpy(*retStr, buffer, CL_MAX_NAME_LENGTH-1);
    clBufferDelete(&outMsgHdl);
    return rc;
}

ClRcT clCpmNodeShutDown(ClIocNodeAddressT iocNodeAddress)
{
    ClRcT rc = CL_OK;

    /*
     * BUG 3610: Change of shutdown semantics
     * Instead of sending request to master, client always sends
     * request to the node for which shutdown has been asked for.
     * 1.   No need of master address now and corresponding functions.
     * 2.   Changed function id in RMD from CPM_NODE_SHUTDOWN 
     *      to CPM_PROC_NODE_SHUTDOWN_REQ
     */
    rc = clCpmClientRMDAsyncNew(iocNodeAddress, CPM_PROC_NODE_SHUTDOWN_REQ, 
                                (ClUint8T *)&(iocNodeAddress), 1,
                                NULL, NULL, 0, 0, 0, CL_IOC_HIGH_PRIORITY,
                                clXdrMarshallClUint32T);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_NODE_SHUTDOWN, rc);
        clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_NODE_SHUTDOWN,"clCpmNodeShutDown Failed rc =%x\n", rc);
    }

    return rc;
}

ClRcT clCpmLocalNodeNameGet(CL_IN SaNameT *nodeName)
{
    ClRcT rc = CL_OK;
    ClCharT *cName = NULL;

    if (!nodeName)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    /*
     * Get the environment variable value and return to the caller 
     */
    if(!ASP_NODENAME[0])
    {
        cName = getenv("ASP_NODENAME");
        if (cName != NULL)
        {
            memset(ASP_NODENAME, 0, sizeof(ASP_NODENAME));
            strncpy(ASP_NODENAME, cName, sizeof(ASP_NODENAME)-1);
        }
    }

    if(ASP_NODENAME[0])
        saNameSet(nodeName, ASP_NODENAME);
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }

    return rc;
}

ClRcT clCpmHotSwapEventHandle(ClCmCpmMsgT *pCmCpmMsg)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterAddress = 0;
    
    rc = clCpmMasterAddressGet(&masterAddress);
    if (rc == CL_OK)
    {
        rc = clCpmClientRMDAsync(masterAddress, CPM_CM_NODE_ARRIVAL_DEPARTURE, 
                                 (ClUint8T *)pCmCpmMsg, sizeof(ClCmCpmMsgT),
                                 NULL, NULL, 0, 0, 0, CL_IOC_HIGH_PRIORITY);
    }
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_NODE_SHUTDOWN, rc);
    }

    return rc;
}

                                // coverity[pass_by_value]
ClRcT clCpmIocAddressForNodeGet(SaNameT nodeName, 
                                ClIocAddressT *pIocAddress)
{
    ClRcT   rc = CL_OK;
    ClUint32T bufSize = 0;
    ClIocNodeAddressT masterAddress = {0};
    ClIocAddressIDLT idlNodeAddress;
    
    memset(&idlNodeAddress,0,sizeof(ClIocAddressIDLT));
    rc = clCpmMasterAddressGet(&masterAddress);
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Unable to get the master address, rc=[0x%x]\n", rc), rc);

    bufSize = sizeof(ClIocAddressIDLT);
    rc = clCpmClientRMDSyncNew(masterAddress, CPM_NODE_IOC_ADDR_GET, (ClUint8T *) &nodeName, sizeof(SaNameT),
                               (ClUint8T *) &idlNodeAddress, &bufSize, CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               clXdrMarshallSaNameT, UNMARSHALL_FN(ClIocAddressIDLT, 4, 0, 0));

    /*
     * Assuming we always get the physical address 
     */
    if (idlNodeAddress.discriminant == CLIOCADDRESSIDLTIOCPHYADDRESS)
    {
        pIocAddress->iocPhyAddress.nodeAddress = idlNodeAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress;
        pIocAddress->iocPhyAddress.portId = idlNodeAddress.clIocAddressIDLT.iocPhyAddress.portId;
    }

failure:
    return rc;
}

                             // coverity[pass_by_value]
ClBoolT clCpmIsCompRestarted(SaNameT compName)
{
	ClRcT rc = CL_OK;
	ClUint32T isRestarted = 0;
	ClUint32T bufSize = sizeof(ClUint32T);

    rc = clCpmClientRMDSyncNew(clIocLocalAddressGet(), 
                               CPM_COMPONENT_IS_COMP_RESTARTED,
                               (ClUint8T *) &compName, 
                               sizeof(SaNameT),
                               (ClUint8T *) &isRestarted, 
                               &bufSize,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               clXdrMarshallSaNameT,
                               clXdrUnmarshallClUint32T);
	if (CL_OK != rc)
	{
		clLogError(CPM_LOG_AREA_RMD,CL_LOG_CONTEXT_UNSPECIFIED,
                           "[%s] failed, rc = [%#x]", __FUNCTION__, rc);
		goto failure;
	}

	if (isRestarted)
	{
		return CL_YES;
	}

failure:
	return CL_NO;
}

static ClRcT cpmSlotGet(ClCpmSlotInfoT *pSlotInfo,
                        ClCpmSlotInfoRecvT *pSlotInfoRecv)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    ClIocNodeAddressT masterIocAddress = 0;
    ClUint32T bufSize = 0;

    if(! pSlotInfo)
    {
        goto failure;
    }

    rc = clCpmMasterAddressGet(&masterIocAddress);

    if (rc != CL_OK)
    {
        goto failure;
    }

    switch(pSlotInfoRecv->flag)
    {
    case CL_CPM_SLOT_ID:
        pSlotInfoRecv->slotId = pSlotInfo->slotId;
        break;

    case CL_CPM_IOC_ADDRESS:
        pSlotInfoRecv->nodeIocAddress = pSlotInfo->nodeIocAddress;
        break;

    case CL_CPM_NODENAME:
        memcpy(&pSlotInfoRecv->nodeName, &pSlotInfo->nodeName, sizeof(SaNameT));
        break;

    default:
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "Invalid flag passed, rc=[0x%x]\n", rc);
        goto failure;
    }

    rc = clCpmClientRMDSyncNew(masterIocAddress, CPM_SLOT_INFO_GET,
                               (ClUint8T *) pSlotInfoRecv, sizeof(ClUint32T),
                               (ClUint8T *) pSlotInfoRecv, &bufSize,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, CL_IOC_LOW_PRIORITY,
                               MARSHALL_FN(ClCpmSlotInfoRecvT, 4, 0, 0),
                               UNMARSHALL_FN(ClCpmSlotInfoRecvT, 4, 0, 0));
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                     ("Unable to find information about given entity, rc=[0x%x]\n", rc), rc);

    switch(pSlotInfoRecv->flag)
    {
        case CL_CPM_SLOT_ID:
            pSlotInfo->nodeIocAddress = pSlotInfoRecv->nodeIocAddress;
            memcpy(&pSlotInfo->nodeName, &pSlotInfoRecv->nodeName, sizeof(SaNameT));
            break;

        case CL_CPM_IOC_ADDRESS:
            pSlotInfo->slotId = pSlotInfoRecv->slotId;
            memcpy(&pSlotInfo->nodeName, &pSlotInfoRecv->nodeName, sizeof(SaNameT));
            break;

        case CL_CPM_NODENAME:
            pSlotInfo->slotId = pSlotInfoRecv->slotId;
            pSlotInfo->nodeIocAddress = pSlotInfoRecv->nodeIocAddress;
            break;
         
        default:
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
    }

failure:
    return rc;
}

/*
 * This is kept separate from cpmSlotInfo which also fetches MOID information from COR.
 * For example - LOG tries to use this call to fetch only the slotaddress.
 * And if COR is in the initialization phase or not yet up, this call would fail
 * for clients who arent interested in getting MOID.
 * Hence making this a subset of cpmSlotInfoGet which is a CPM COR extension API.
 */

ClRcT clCpmSlotGet(ClCpmSlotInfoFieldIdT flag, ClCpmSlotInfoT *slotInfo)
{
    ClRcT rc = CL_OK;
    ClCpmSlotInfoRecvT slotInfoRecv = {CL_CPM_SLOT_ID};
    ClNodeCacheSlotInfoT nodeCacheSlotInfo = {0};
    ClNodeCacheSlotInfoFieldT nodeCacheField;

    if(!slotInfo) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(flag == CL_CPM_SLOT_ID || flag == CL_CPM_IOC_ADDRESS)
    {
        nodeCacheField = CL_NODE_CACHE_SLOT_ID;
        if(flag == CL_CPM_SLOT_ID)
        {
            nodeCacheSlotInfo.slotId = slotInfo->slotId;
            slotInfo->nodeIocAddress = slotInfo->slotId;
        }
        else
        {
            nodeCacheSlotInfo.slotId = slotInfo->nodeIocAddress;
            slotInfo->slotId = slotInfo->nodeIocAddress;
        }
        rc = clNodeCacheSlotInfoGet(nodeCacheField, &nodeCacheSlotInfo);
        if(rc == CL_OK)
        {
            saNameCopy(&slotInfo->nodeName, &nodeCacheSlotInfo.nodeName);
            return rc;
        }
    }
    else if(flag == CL_CPM_NODENAME)
    {
        nodeCacheField = CL_NODE_CACHE_NODENAME;
        saNameCopy(&nodeCacheSlotInfo.nodeName, &slotInfo->nodeName);
        rc = clNodeCacheSlotInfoGet(nodeCacheField, &nodeCacheSlotInfo);
        if(rc == CL_OK)
        {
            slotInfo->slotId = nodeCacheSlotInfo.slotId;
            slotInfo->nodeIocAddress = nodeCacheSlotInfo.slotId;
            return rc;
        }
    }

    slotInfoRecv.flag = flag;
    rc = cpmSlotGet(slotInfo, &slotInfoRecv);
    return rc;
}

ClRcT clCpmCompStatusGet(ClIocAddressT compAddr, ClStatusT *pStatus)
{
    ClRcT rc = CL_OK;
    ClUint8T tempStatus;

    if(CL_IOC_ADDRESS_TYPE_GET(&compAddr.iocPhyAddress) != CL_IOC_PHYSICAL_ADDRESS_TYPE)
    {
        clLogError("CPM", CL_LOG_CONTEXT_UNSPECIFIED, 
                   "The passed component address [0x%x:0x%x] is not a Physical Address.", 
                   compAddr.iocPhyAddress.nodeAddress, compAddr.iocPhyAddress.portId);
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clIocCompStatusGet(compAddr.iocPhyAddress, &tempStatus);
    if(rc == CL_OK)
        *pStatus = (ClStatusT) tempStatus;

    return rc;
}

ClRcT clCpmNodeStatusGet(ClIocNodeAddressT nodeAddr, ClStatusT *pStatus)
{
    ClRcT rc = CL_OK;
    ClUint8T tempStatus;
    
    rc = clIocRemoteNodeStatusGet(nodeAddr, &tempStatus);
    if(rc == CL_OK)
        *pStatus = (ClStatusT) tempStatus;

    return rc;
}

ClRcT clCpmNotificationCallbackInstall(ClIocPhysicalAddressT compAddr,
                                       ClCpmNotificationFuncT pFunc,
                                       ClPtrT pArg,
                                       ClHandleT *pHandle)
{
    return clEoNotificationCallbackInstall(compAddr,
                                           *((ClPtrT*)&pFunc),
                                           pArg,
                                           pHandle);
}

ClRcT clCpmNotificationCallbackUninstall(ClHandleT *pHandle)
{
    return clEoNotificationCallbackUninstall(pHandle);
}

void clCpmCompTerminateSet(ClBoolT doTerminate)
{
    componentTerminate = doTerminate;
}

ClRcT clCpmNodeConfigSet(ClCpmNodeConfigT *nodeConfig)
{
    ClIocNodeAddressT masterAddress = 0;
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(!nodeConfig)
    {
        goto out;
    }

    rc = clCpmMasterAddressGet(&masterAddress);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Master address get returned [%#x]", rc);
        goto out;
    }

    rc = clCpmClientRMDAsyncNew(masterAddress,
                                CPM_MGMT_NODE_CONFIG_SET,
                                (ClUint8T *) nodeConfig,
                                (ClUint32T)sizeof(*nodeConfig), 
                                NULL, NULL,
                                0, 0, 0, 0,
                                MARSHALL_FN(ClCpmNodeConfigT, 4, 0, 0)
                                );
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node config set RMD returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

ClRcT clCpmNodeConfigGet(const ClCharT *nodeName, ClCpmNodeConfigT *nodeConfig)
{
    ClIocNodeAddressT masterAddress = 0;
    ClUint32T configSize = 0;
    SaNameT node = {0};
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(!nodeName || !nodeConfig)
    {
        goto out;
    }

    saNameSet(&node, nodeName);

    rc = clCpmMasterAddressGet(&masterAddress);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
            masterAddress = CL_IOC_BROADCAST_ADDRESS;
        else
        {
            clLogError("NODE", "CONFIG", "Master address get returned [%#x]", rc);
            goto out;
        }
    }

    rc = clCpmClientRMDSyncNew(masterAddress, CPM_MGMT_NODE_CONFIG_GET,
                               (ClUint8T *)&node, sizeof(node),
                               (ClUint8T *) nodeConfig, &configSize,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, CL_IOC_LOW_PRIORITY,
                               clXdrMarshallSaNameT,
                               UNMARSHALL_FN(ClCpmNodeConfigT, 4, 0, 0));

    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node config get RMD returned [%#x]", rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT 
           && 
           masterAddress == CL_IOC_BROADCAST_ADDRESS)
            rc = CL_ERR_NOT_EXIST;

        goto out;
    }

    if(clCpmIsSC() && strncmp(nodeConfig->cpmType, "GLOBAL", 6))
    {
        strncpy(nodeConfig->cpmType, "GLOBAL", sizeof(nodeConfig->cpmType)-1);
    }

    out:
    return rc;
}

ClRcT clCpmClientTableRegister(ClEoExecutionObjT *eo)
{
    return clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, CPMMgmtClient),
                                   eo->eoPort);
}

ClRcT clCpmNodeRestart(ClIocNodeAddressT iocNodeAddress, ClBoolT graceful)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterAddress = 0;
    ClCpmRestartSendT cpmRestartSend = {0};
    ClTimerTimeOutT delay = { 2, 0};
    ClInt32T tries = 0;

    do
    {
        rc = clCpmMasterAddressGet(&masterAddress);
        if (rc != CL_OK)
        {
            clLogError("CPM", "CPM",
                       "Unable to get master address, error [%#x]",
                       rc);
            goto out;
        }

        cpmRestartSend.iocNodeAddress = iocNodeAddress;
        cpmRestartSend.graceful = graceful;

        rc = clCpmClientRMDSyncNew(masterAddress,
                                   CPM_MGMT_NODE_RESTART,
                                   (ClUint8T *)&cpmRestartSend,
                                   sizeof(ClCpmRestartSendT),
                                   NULL,
                                   NULL,
                                   CL_RMD_CALL_ATMOST_ONCE,
                                   3000,
                                   1,
                                   0,
                                   MARSHALL_FN(ClCpmRestartSendT, 4, 0, 0),
                                   NULL);

    } while(rc != CL_OK 
            && 
            CL_GET_ERROR_CODE(rc) != CL_ERR_DOESNT_EXIST
            &&
            CL_GET_ERROR_CODE(rc) != CL_ERR_NO_OP
            &&
            ++tries < 4 
            && 
            clOsalTaskDelay(delay) == CL_OK);

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   "Failed to restart node with address [%d], error [%#x]",
                   iocNodeAddress,
                   rc);
    }

    out:
    return rc;
}

ClRcT clCpmCompConfigSet(ClIocNodeAddressT node, 
                         ClCharT *name, ClCharT *instantiateCommand,
                         ClAmsCompPropertyT property, ClUint64T mask)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    VDECL_VER(ClCpmCompConfigSetT, 5, 1, 0) compConfig;

    if(!name || !instantiateCommand || !node)
    {
        goto out;
    }

    memset(&compConfig, 0, sizeof(compConfig));
    strncat(compConfig.name, name, sizeof(compConfig.name)-1);
    strncat(compConfig.instantiateCommand, instantiateCommand, sizeof(compConfig.instantiateCommand)-1);
    compConfig.property = property;
    compConfig.bitmask = mask;

    rc = clCpmClientRMDAsyncNew(node,
                                CPM_MGMT_COMP_CONFIG_SET,
                                (ClUint8T *)&compConfig,
                                (ClUint32T)sizeof(compConfig), 
                                NULL, NULL,
                                0, 0, 0, 0,
                                MARSHALL_FN(ClCpmCompConfigSetT, 5, 1, 0)
                                );
    if(rc != CL_OK)
    {
        clLogError("COMP", "CONFIG", "Component config set RMD returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

ClRcT clCpmNodeSwitchover(ClIocNodeAddressT iocNodeAddress)
{
    return clCpmNodeRestart(iocNodeAddress, CL_TRUE);
}

/*
 * The middleware restart reboots the node or restarts asp if nodeReset flag is CL_FALSE.
 * It works irrespective of the environment variables that one can use to override the node reset behavior.
 * If you want to get environment variable behavior for node resets, then clCpmNodeRestart is the API to use.
 */
ClRcT clCpmMiddlewareRestart(ClIocNodeAddressT iocNodeAddress, ClBoolT graceful, ClBoolT nodeReset)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterAddress = 0;
    ClCpmMiddlewareResetT cpmMiddlewareReset = {0};
    ClTimerTimeOutT delay = { 2, 0};
    ClInt32T tries = 0;

    do
    {
        rc = clCpmMasterAddressGet(&masterAddress);
        if (rc != CL_OK)
        {
            clLogError("CPM", "CPM",
                       "Unable to get master address, error [%#x]",
                       rc);
            goto out;
        }

        cpmMiddlewareReset.iocNodeAddress = iocNodeAddress;
        cpmMiddlewareReset.graceful = graceful;
        cpmMiddlewareReset.nodeReset = nodeReset;

        rc = clCpmClientRMDSyncNew(masterAddress,
                                   CPM_MGMT_MIDDLEWARE_RESTART,
                                   (ClUint8T *)&cpmMiddlewareReset,
                                   sizeof(ClCpmMiddlewareResetT),
                                   NULL,
                                   NULL,
                                   CL_RMD_CALL_ATMOST_ONCE,
                                   3000,
                                   1,
                                   0,
                                   MARSHALL_FN(ClCpmMiddlewareResetT, 4, 0, 0),
                                   NULL);

    } while(rc != CL_OK 
            && 
            CL_GET_ERROR_CODE(rc) != CL_ERR_DOESNT_EXIST
            &&
            CL_GET_ERROR_CODE(rc) != CL_ERR_NO_OP
            &&
            ++tries < 4 
            && 
            clOsalTaskDelay(delay) == CL_OK);

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   "Failed to restart middleware with address [%d], error [%#x]",
                   iocNodeAddress,
                   rc);
    }

    out:
    return rc;
}


ClBoolT clCpmIsSC(void)
{
    static ClInt32T cachedState = -1;
    if(cachedState >= 0) goto out;
    cachedState = (ClInt32T)clParseEnvBoolean("SYSTEM_CONTROLLER");
    out:
    return cachedState ? CL_TRUE : CL_FALSE;
}

ClRcT clCpmCompInfoGet(const SaNameT *compName,
                       const ClIocNodeAddressT nodeAddress,
                       ClCpmCompSpecInfoT *compInfo)
{
    ClRcT rc = CL_OK;
    ClCpmCompSpecInfoRecvT compInfoRecv;
    ClUint32T compInfoSize = 0;
    ClUint32T i = 0;

    memset(&compInfoRecv, 0, sizeof(ClCpmCompSpecInfoRecvT));

    if (!compName || !nodeAddress || !compInfo)
    {
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    }

    memset(compInfo, 0, sizeof(ClCpmCompSpecInfoT));

    rc = clCpmClientRMDSyncNew(nodeAddress,
                               CPM_COMPONENT_INFO_GET,
                               (ClUint8T *)compName,
                               sizeof(SaNameT),
                               (ClUint8T *)&compInfoRecv,
                               &compInfoSize,
                               CL_RMD_CALL_NEED_REPLY,
                               0,
                               0,
                               CL_IOC_LOW_PRIORITY,
                               clXdrMarshallSaNameT,
                               UNMARSHALL_FN(ClCpmCompSpecInfoRecvT, 4, 0, 0));
    if (rc != CL_OK)
    {
        clLogError("CPM", "CPM", "Component info get RMD returned [%#x]", rc);
        goto failure;
    }

    compInfo->period = compInfoRecv.period;
    compInfo->maxDuration = compInfoRecv.maxDuration;
    compInfo->recovery = compInfoRecv.recovery;

    compInfo->numArgs = compInfoRecv.numArgs;

    compInfo->args = (ClCharT**) clHeapAllocate(compInfo->numArgs * sizeof(ClCharT *));
    if (!compInfo->args)
    {
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        goto failure;
    }

    for (i = 0; i < compInfo->numArgs; ++i)
    {
        ClStringT *s = (compInfoRecv.args)+i;
        ClCharT **p = compInfo->args;

        p[i] = (ClCharT*) clHeapAllocate(s->length+1);
        if (!p[i])
        {
            rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
            goto failure;
        }

        strncpy(p[i], s->pValue, s->length);
    }

failure:
    if (compInfoRecv.numArgs && compInfoRecv.args)
    {
        for (i = 0; i < compInfoRecv.numArgs; ++i)
        {
            ClStringT *s = (compInfoRecv.args)+i;
            if (s)
            {
                clHeapFree(s->pValue);
            }
        }
        clHeapFree(compInfoRecv.args);
    }
    return rc;
}

ClRcT clCpmComponentFailureReportWithCookie(ClCpmHandleT cpmHandle,
                                            const SaNameT *compName,
                                            ClUint64T instantiateCookie,
                                            ClTimeT errorDetectionTime,
                                            ClAmsLocalRecoveryT recommendedRecovery,
                                            ClUint32T alarmHandle)
{
    ClRcT rc = CL_OK;
    ClErrorReportT *errorReport = NULL;
    ClIocNodeAddressT masterAddress = 0;

    if (compName == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Null ptr passed"),
                         CL_CPM_RC(CL_ERR_NULL_POINTER));
    }

    /*
     * This should be enabled in future.
     */
#if 0
    rc = clHandleValidate(handle_database, cpmHandle);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_2_HANDLE_INVALID, compName->value, rc);
        clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "Invalid CPM handle, rc = [%#x]", rc);
        goto failure;
    }
#endif

    /*
     * Allocate and make a error buffer, which needs to reach to the server 
     */
    errorReport =
        (ClErrorReportT *) clHeapAllocate(sizeof(ClErrorReportT));
                                        
    if (errorReport == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, CL_CPM_CLIENT_LIB,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Unable to allocate memory \n"),
                         CL_CPM_RC(CL_ERR_NO_MEMORY));
    }

    memcpy(&(errorReport->compName), compName, sizeof(SaNameT));
    memcpy(&(errorReport->time), &errorDetectionTime, sizeof(ClTimeT));
    errorReport->recommendedRecovery = recommendedRecovery;
    errorReport->handle = alarmHandle;
    errorReport->instantiateCookie = instantiateCookie;

    rc = clCpmMasterAddressGet(&masterAddress);
    if(rc == CL_OK)
    {
        rc = clCpmClientRMDAsyncNew(masterAddress,
                                    CPM_COMPONENT_FAILURE_REPORT,
                                    (ClUint8T *) errorReport,
                                    sizeof(ClErrorReportT),
                                    NULL,
                                    NULL,
                                    CL_RMD_CALL_ATMOST_ONCE,
                                    0,
                                    0,
                                    0,
                                    MARSHALL_FN(ClErrorReportT, 4, 0, 0));
    }
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, CL_CPM_CLIENT_LIB,
                   CL_CPM_LOG_1_CLIENT_COMP_FAILURE_REPORT_ERR, rc);
    }
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("%s Failed rc =%x\n", __FUNCTION__, rc),
                     rc);

failure:
    if (errorReport != NULL)
    {
        clHeapFree(errorReport);
        errorReport = NULL;
    }
    return rc;
}

ClRcT clCpmTargetVersionGet(ClCharT *aspVersion, ClUint32T maxBytes)
{
#define ASP_BUILD_FILE "VERSION"

    ClCharT *config = NULL;
    static ClCharT aspBuildVersion[CL_MAX_NAME_LENGTH];
    static ClCharT *pAspVersion = NULL;

    if(!aspVersion || !maxBytes)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(pAspVersion)
        goto out_copy;

    pAspVersion = aspBuildVersion;
    snprintf(aspBuildVersion, sizeof(aspBuildVersion)-1, "%d.%d.%d",
             CL_RELEASE_VERSION_CURRENT, CL_MAJOR_VERSION_CURRENT, CL_MINOR_VERSION_CURRENT);

    if((config = getenv("ASP_CONFIG")))
    {
        ClCharT aspBuildFile[CL_MAX_NAME_LENGTH];
        snprintf(aspBuildFile, sizeof(aspBuildFile), "%s/%s", config, ASP_BUILD_FILE);
        if(!access(aspBuildFile, F_OK))
        {
            FILE *fptr = fopen(aspBuildFile, "r");
            if(fptr)
            {
                ClCharT *pAspBuild = aspBuildVersion;
                if(fgets(aspBuildVersion, sizeof(aspBuildVersion), fptr))
                {
                    if(aspBuildVersion[strlen(aspBuildVersion)-1] == '\n')
                        aspBuildVersion[strlen(aspBuildVersion)-1] = 0;
                    
                    if( ( pAspBuild = strchr(aspBuildVersion, '=') ) )
                    {
                        *pAspBuild = 0;
                        if(pAspBuild[1])
                            pAspVersion = pAspBuild+1;
                    }
                }
                fclose(fptr);
            }
        }
    }

    /*
     * Avoid strncpy. Its EVIL! So strncat's a better option for good measure
     */
    out_copy:
    *aspVersion = 0;
    strncat(aspVersion, pAspVersion, maxBytes-1);
    return CL_OK;

#undef ASP_BUILD_FILE
}

ClRcT clCpmTargetInfoInitialize(void)
{
#define CL_TARGET_INFO_FILE "targetconf.xml"
#define CL_TARGET_INFO_VERSION { 5,  0, 0 }

    ClParserPtrT file;
    ClParserPtrT child;
    ClParserPtrT val;
    const ClCharT *path = getenv("ASP_CONFIG");
    ClVersionT version = CL_TARGET_INFO_VERSION;
    ClTargetSlotInfoT *slots = NULL;
    ClUint32T numSlots = 0;
    ClRcT rc = CL_CPM_RC(CL_ERR_NOT_EXIST);

    if(!path) path = ".";
    file = clParserOpenFileWithVer(path, CL_TARGET_INFO_FILE, &version);
    if(!file)
    {
        clLogInfo("TARGET", "INFO", "Unable to read file [%s] at path [%s]. "
                  "ASP static target info cannot be fetched by the user",
                  CL_TARGET_INFO_FILE, path);
        goto out;
    }

    /*
     * Get the current version of sdk
     */
    clCpmTargetVersionGet(gClTargetClusterInfo.targetInfo.version, sizeof(gClTargetClusterInfo.targetInfo.version)-1);

    if( ( val = clParserChild(file, "TRAP_IP") ) )
    {
        gClTargetClusterInfo.targetInfo.trapIp[0] = 0;
        strncat(gClTargetClusterInfo.targetInfo.trapIp, val->txt, sizeof(gClTargetClusterInfo.targetInfo.trapIp)-1);
    }

    gClTargetClusterInfo.targetInfo.installPrerequisites = CL_FALSE;
    if( ( val = clParserChild(file, "INSTALL_PREREQUISITES") ) )
    {
        if(!strncasecmp(val->txt, "yes", 3))
            gClTargetClusterInfo.targetInfo.installPrerequisites = CL_TRUE;
    }

    gClTargetClusterInfo.targetInfo.instantiateImages = CL_FALSE;
    if( ( val = clParserChild(file, "INSTANTIATE_IMAGES") ) )
    {
        if(!strncasecmp(val->txt, "yes", 3))
            gClTargetClusterInfo.targetInfo.instantiateImages = CL_TRUE;
    }

    gClTargetClusterInfo.targetInfo.createTarballs = CL_FALSE;
    if( (val = clParserChild(file, "CREATE_TARBALLS") ) )
    {
        if(!strncasecmp(val->txt, "yes", 3))
            gClTargetClusterInfo.targetInfo.createTarballs = CL_TRUE;
    }

    if( (val = clParserChild(file, "TIPC_NETID") ) )
        gClTargetClusterInfo.targetInfo.tipcNetid = atoi(val->txt);

    if( (val = clParserChild(file, "GMS_MCAST_PORT") ) )
        gClTargetClusterInfo.targetInfo.gmsMcastPort = atoi(val->txt);

    rc = CL_OK;
    child = clParserChild(file, "slots");
    if(!child)
    {
        clLogWarning("TARGET", "INFO", "No slots tag found in [%s]", CL_TARGET_INFO_FILE);
        goto out_free;
    }

    child = clParserChild(child, "slot");
    if(!child)
    {
        clLogWarning("TARGET", "INFO", "No slot tag found in [%s]", CL_TARGET_INFO_FILE);
        goto out_free;
    }

    while(child)
    {
        if(!(numSlots & 7) )
        {
            slots = (ClTargetSlotInfoT*) realloc(slots, sizeof(*slots) * (numSlots + 8));
            CL_ASSERT(slots != NULL);
            memset(slots + numSlots, 0, sizeof(*slots) * 8 );
        }

        if( (val = clParserChild(child, "NAME") ) )
        {
            slots[numSlots].name[0] = 0;
            strncat(slots[numSlots].name, val->txt, sizeof(slots[numSlots].name)-1);
        }

        if( (val = clParserChild(child, "ADDR") ) )
            slots[numSlots].addr = atoi(val->txt);

        if( (val = clParserChild(child, "LINK") ) )
        {
            slots[numSlots].linkname[0] = 0;
            strncat(slots[numSlots].linkname, val->txt, sizeof(slots[numSlots].linkname)-1);
        }
        
        if( (val = clParserChild(child, "ARCH") ) )
        {
            slots[numSlots].arch[0] = 0;
            strncat(slots[numSlots].arch, val->txt, sizeof(slots[numSlots].arch)-1);
        }
        
        if( (val = clParserChild(child, "CUSTOM") ) )
        {
            slots[numSlots].customData[0] = 0;
            strncat(slots[numSlots].customData, val->txt, sizeof(slots[numSlots].customData)-1);
        }
        ++numSlots;
        child = child->next;
    }

    gClTargetClusterInfo.slots = slots;
    gClTargetClusterInfo.targetInfo.numSlots = numSlots;

    out_free:
    clParserFree(file);
    
    out:
    return rc;

#undef CL_TARGET_INFO_FILE
#undef CL_TARGET_INFO_VERSION
}

ClRcT clCpmTargetSlotInfoGet(const ClCharT *name, ClIocNodeAddressT addr, ClTargetSlotInfoT *slotInfo)
{
    register ClUint32T i;

    if(!name && !addr)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(!slotInfo)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    if(!gClTargetClusterInfo.slots || !gClTargetClusterInfo.targetInfo.numSlots)
        return CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
    
    for(i = 0; i < gClTargetClusterInfo.targetInfo.numSlots; ++i)
    {
        ClInt32T cmp = 0;
        if(name)
            cmp = strcasecmp(gClTargetClusterInfo.slots[i].name, name);
        if(addr)
            cmp |= gClTargetClusterInfo.slots[i].addr - addr;
        if(!cmp)
        {
            memcpy(slotInfo, &gClTargetClusterInfo.slots[i], sizeof(*slotInfo));
            return CL_OK;
        }
    }
    return CL_CPM_RC(CL_ERR_NOT_EXIST);
}

ClRcT clCpmTargetInfoGet(ClTargetInfoT *targetInfo)
{
    if(!targetInfo)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    memcpy(targetInfo, &gClTargetClusterInfo.targetInfo, sizeof(*targetInfo));
    return CL_OK;
}

ClRcT clCpmTargetSlotListGet(ClTargetSlotInfoT *slotInfo, ClUint32T *numSlots)
{
    if(!slotInfo || !numSlots)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    *numSlots = CL_MIN(gClTargetClusterInfo.targetInfo.numSlots, *numSlots);
    if(!*numSlots)
        return CL_OK;
    memcpy(slotInfo, gClTargetClusterInfo.slots, sizeof(*slotInfo) * *numSlots);
    return CL_OK;
}

/*
 * Check if the node is SC capable or not. (to be used with payloads)
 * No need to cache the values of rarely used functions as the callers neednt be thread-safe and its a waste
 * of time trying to make this thread-safe even with pthread_once considering we run on so many archs.
 * now other than linux.
 */
ClBoolT clCpmIsSCCapable(void)
{
    static ClInt32T cachedState = -1;
    if(clCpmIsSC())
        return CL_TRUE;
    if(cachedState >= 0)
        goto out;
    cachedState = (ClInt32T)clParseEnvBoolean("ASP_SC_PROMOTE");
    out:
    return cachedState ? CL_TRUE : CL_FALSE;
}

void clCpmWatchdogRestart(ClBoolT abort) { return; }

