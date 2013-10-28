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
 * This file implements CPMs node related functionality.
 */

/*
 * Standard header files 
 */
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

/*
 * ASP header files 
 */
#include <clCommonErrors.h>
#include <clRmdIpi.h>
#include <clRmdErrors.h>
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clIocErrors.h>
#include <clClmApi.h>
#include <clLogApi.h>
#include <clVersionApi.h>
#include <clCmApi.h>
#include <clAms.h>
#include <clAmsMgmtClientApi.h>
/*
 * CPM internal header files 
 */
#include <clCpmInternal.h>
#include <clCpmClient.h>
#include <clCpmLog.h>
#include <clCpmCkpt.h>
#include <clCpmIpi.h>
#include <clHash.h>
/*
 * XDR header files 
 */
#include "xdrClCpmLocalInfoT.h"
#include "xdrClCpmLcmResponseT.h"
#include "xdrClCpmBootOperationT.h"
#include "xdrClCpmBmSetLevelResponseT.h"
#include "xdrClIocAddressIDLT.h"
#include "xdrClCpmNodeConfigT.h"
#include "xdrClCpmRestartSendT.h"
#include "xdrClCpmMiddlewareResetT.h"

typedef struct ClCpmAspResetMsg
{
    ClIocNodeAddressT nodeAddress;
    ClUint32T nodeRequest; /* node reset hint */
}ClCpmAspResetMsgT;

typedef struct ClCpmResetMsg
{
#define _CM_RESET_MSG  0x1
#define _ASP_RESET_MSG 0x2
    ClUint8T msgType;
    ClNameT nodeName;
    struct hashStruct hash;
    union
    {
        ClCmCpmMsgT cmMsg;
        ClCpmAspResetMsgT aspMsg;
    } resetMsg;

#define cmResetMsg  resetMsg.cmMsg
#define aspResetMsg resetMsg.aspMsg

} ClCpmResetMsgT;

#define CPM_RESET_TABLE_BITS (5)
#define CPM_RESET_TABLE_SIZE ( 1 << CPM_RESET_TABLE_BITS )
#define CPM_RESET_TABLE_MASK (CPM_RESET_TABLE_SIZE - 1)
static struct hashStruct *cpmResetMsgTable[CPM_RESET_TABLE_SIZE];
static ClVersionT clCpmServerToServerVersionsSupported[] =
{
    {'B', 0x1, 0x1}
    /* Add newer version here */
};

static ClVersionDatabaseT clCpmServerToServerVersionDb =
{
    sizeof(clCpmServerToServerVersionsSupported) / sizeof(ClVersionT),
    clCpmServerToServerVersionsSupported
};

typedef ClRcT (*funcArray[]) (void);


ClRcT CL_CPM_CALL_RMD_SYNC(ClIocNodeAddressT destAddr,
                           ClIocPortT eoPort,
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
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress;
    ClBufferHandleT inMsgHdl = { 0 };
    ClBufferHandleT outMsgHdl = { 0 };
    ClUint32T msgLength = 0;

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;
    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = eoPort;

    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
            goto failure;
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T *) pInBuf,
                                        inBufLen);
        if (rc != CL_OK)
            goto failure;
    }
    if (NULL != pOutBufLen)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
            goto failure;
    }
    rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                      (flags) & (~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
    if (inBufLen)
    {
        retCode = clBufferDelete(&inMsgHdl);
    }
    if (rc == CL_OK && NULL != pOutBufLen)
    {
        retCode = clBufferLengthGet(outMsgHdl, &msgLength);
        retCode = clBufferNBytesRead(outMsgHdl, (ClUint8T *) pOutBuf, &msgLength);
        retCode = clBufferDelete(&outMsgHdl);
        retCode = retCode;  // use retcode to make compiler happy
    }
    else if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD Failed with an error %x", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_CPM_LOG_1_RMD_CALL_ERR, rc);
    }

  failure:
    return rc;
}

ClRcT CL_CPM_CALL_RMD_SYNC_NEW(ClIocNodeAddressT destAddr,
                               ClIocPortT eoPort,
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
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT iocAddress;
    ClBufferHandleT inMsgHdl = { 0 };
    ClBufferHandleT outMsgHdl = { 0 };

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;
    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = eoPort;

    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
            goto failure;
        rc = marshallFunction((void *) pInBuf, inMsgHdl, 0);
        if (rc != CL_OK)
            goto failure;
    }
    if (NULL != pOutBufLen)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
            goto failure;
    }
    rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                      (flags) & (~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
    if (inBufLen)
    {
        retCode = clBufferDelete(&inMsgHdl);
    }
    if (rc == CL_OK && NULL != pOutBufLen)
    {
        rc = unmarshallFunction(outMsgHdl, (void *) pOutBuf);
        retCode = clBufferDelete(&outMsgHdl);
        retCode = retCode; // Use retcode to make compiler happy
    }
    else if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD Failed with an error %x", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_CPM_LOG_1_RMD_CALL_ERR, rc);
    }

  failure:
    return rc;
}

ClRcT CL_CPM_CALL_RMD_ASYNC(ClIocNodeAddressT destAddr,
                            ClIocPortT destPort,
                            ClUint32T fcnId,
                            ClUint8T *pInBuf,
                            ClUint32T inBufLen,
                            ClUint8T *pOutBuf,
                            ClUint32T *pOutBufLen,
                            ClUint32T flags,
                            ClUint32T timeOut,
                            ClUint32T maxRetries,
                            ClUint8T priority,
                            void *cookie,
                            ClRmdAsyncCallbackT pFuncCB)
{
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClRmdAsyncOptionsT rmdAsyncOptions;
    ClIocAddressT iocAddress;
    ClBufferHandleT inMsgHdl = { 0 };
    ClBufferHandleT outMsgHdl = { 0 };

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = destPort;
    rmdAsyncOptions.pCookie = cookie;
    rmdAsyncOptions.fpCallback = pFuncCB;
    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
            goto failure;
        rc = clBufferNBytesWrite(inMsgHdl, (ClUint8T *) pInBuf,
                                        inBufLen);
        if (rc != CL_OK)
            goto failure;
    }
    if (NULL != pOutBufLen)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
            goto failure;
    }
    if (cookie)
    {
        rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                          (flags) | CL_RMD_CALL_ASYNC, &rmdOptions,
                          &rmdAsyncOptions);
    }
    else
        rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                          (flags) | CL_RMD_CALL_ASYNC, &rmdOptions, NULL);
    if (inBufLen)
    {
        retCode = clBufferDelete(&inMsgHdl);
        retCode = retCode; // use retcode to make compiler happy
    }

  failure:
    return rc;
}

ClRcT CL_CPM_CALL_RMD_ASYNC_NEW(ClIocNodeAddressT destAddr,
                                ClIocPortT destPort,
                                ClUint32T fcnId,
                                ClUint8T *pInBuf,
                                ClUint32T inBufLen,
                                ClUint8T *pOutBuf,
                                ClUint32T *pOutBufLen,
                                ClUint32T flags,
                                ClUint32T timeOut,
                                ClUint32T maxRetries,
                                ClUint8T priority,
                                void *cookie,
                                ClRmdAsyncCallbackT pFuncCB,
                                ClCpmMarshallT marshallFunction)
{
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClRmdAsyncOptionsT rmdAsyncOptions;
    ClIocAddressT iocAddress;
    ClBufferHandleT inMsgHdl = { 0 };
    ClBufferHandleT outMsgHdl = { 0 };

    if (timeOut == 0)
        rmdOptions.timeout = CL_CPM_DFLT_TIMEOUT;
    else
        rmdOptions.timeout = timeOut;

    if (maxRetries == 0)
        rmdOptions.retries = CL_CPM_DFLT_RETRIES;
    else
        rmdOptions.retries = maxRetries;

    if (priority == 0)
        rmdOptions.priority = CL_CPM_DFLT_PRIORITY;
    else
        rmdOptions.priority = priority;

    iocAddress.iocPhyAddress.nodeAddress = destAddr;
    iocAddress.iocPhyAddress.portId = destPort;
    rmdAsyncOptions.pCookie = cookie;
    rmdAsyncOptions.fpCallback = pFuncCB;
    if (inBufLen)
    {
        rc = clBufferCreate(&inMsgHdl);
        if (rc != CL_OK)
            goto failure;
        rc = marshallFunction((void *) pInBuf, inMsgHdl, 0);
        if (rc != CL_OK)
            goto failure;
    }
    if (NULL != pOutBufLen)
    {
        rc = clBufferCreate(&outMsgHdl);
        if (rc != CL_OK)
            goto failure;
    }
    if (cookie)
    {
        rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                          (flags) | CL_RMD_CALL_ASYNC, &rmdOptions,
                          &rmdAsyncOptions);
    }
    else
        rc = clRmdWithMsg(iocAddress, (ClUint32T) fcnId, inMsgHdl, outMsgHdl,
                          (flags) | CL_RMD_CALL_ASYNC, &rmdOptions, NULL);
    if (inBufLen)
    {
        retCode = clBufferDelete(&inMsgHdl);
    }

  failure:
    return rc;
}

ClUint32T cpmNodeFindByIocAddress(ClIocNodeAddressT nodeAddress, ClCpmLT **cpmL)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT cpmNode = 0;
    ClUint32T cpmLCount = 0;
    ClCpmLT *tempCpmL = NULL;
    ClUint32T found = 0;

    cpmLCount = gpClCpm->noOfCpm;
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL && cpmLCount != 0)
    {
        rc = clCntFirstNodeGet(gpClCpm->cpmTable, &cpmNode);
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
                       "CPM-L", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        while (cpmLCount)
        {
            rc = clCntNodeUserDataGet(gpClCpm->cpmTable, cpmNode,
                                      (ClCntDataHandleT *) &tempCpmL);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                           CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (tempCpmL->pCpmLocalInfo)
            {
                if (tempCpmL->pCpmLocalInfo->cpmAddress.nodeAddress 
                        == nodeAddress)
                {
                    *cpmL = tempCpmL;
                    found = 1;
                    break;
                }
            }
            cpmLCount--;

            if (cpmLCount)
            {
                rc = clCntNextNodeGet(gpClCpm->cpmTable, cpmNode, &cpmNode);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                               CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR, "CPM-L", rc,
                               rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }
    if(found == 1)
        return CL_OK;
    else
        return CL_CPM_RC(CL_ERR_DOESNT_EXIST);

  failure:
    *cpmL = NULL;
    return rc;
}


/*
 * This function used to live in clCpmConfig.c earlier.  This function
 * used to be user defined hook which performs software validation,
 * when a node registers with CPM.
 *
 * But since CPM-CM admission control is broken there is no need for
 * such function. Hence for now just copying it here. This is not a
 * user defined function anymore.
 *
 * Need to revisit this in future during CPM-CM admission control
 * implementation.
 */
ClUint32T clCpmCardMatch(ClNameT *nodeClassType, 
                         ClNameT *nodeIdentifier,
                         ClCpmSlotClassTypesT *slotClassTypes)
{
    ClUint32T i = 0;

    for (i = 0; i < slotClassTypes->numItems; ++i)
    {
        if (!strcmp(nodeClassType->value, 
                    slotClassTypes->nodeClassTypes[i].name.value))
        {
            return CL_TRUE;
        }
    }
    
    return CL_FALSE;
}

ClRcT VDECL(cpmCpmLocalRegister)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL;
    ClCpmLocalInfoT cpmLocalInfo = {{0}};
    ClNameT nodeName = {0};
    ClCpmLcmReplyT srcInfo = {0};
    ClCpmSlotClassTypesT slotClassTypes = {0, NULL};
    ClUint32T flag = CL_FALSE;
    ClIocNodeAddressT nodeAddress = 0;

    /*
     * Param Check 
     */
    rc = VDECL_VER(clXdrUnmarshallClCpmLocalInfoT, 4, 0, 0)(inMsgHandle, (void *) &cpmLocalInfo);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    nodeAddress = cpmLocalInfo.cpmAddress.nodeAddress;

    if ((nodeAddress != clIocLocalAddressGet()) && (gpClCpm->bmTable->currentBootLevel < cpmLocalInfo.defaultBootLevel))
    {
        /* clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
           "CPM/G active is returning try again for registration request by node [name: %s, id: %d, addr: %d], because it is not ready.  Current boot level [%d], required boot level [%d]", cpmLocalInfo.nodeName,cpmLocalInfo.nodeId, nodeAddress, gpClCpm->bmTable->currentBootLevel,cpmLocalInfo.defaultBootLevel); */
    clLogMultiline(CL_LOG_NOTICE, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                   "Active is at boot level [%d] so returning try again for registration request from [%s] Node ID: [%d] IOC address: [%d] IOC port: [%#x] Default boot level: [%d] Slot Number: [%d] Node Type: [%*s] Node Identifier: [%*s]",
                   gpClCpm->bmTable->currentBootLevel,
                   cpmLocalInfo.nodeName,
                   cpmLocalInfo.nodeId,
                   cpmLocalInfo.cpmAddress.nodeAddress,
                   cpmLocalInfo.cpmAddress.portId,
                   cpmLocalInfo.defaultBootLevel,
                   cpmLocalInfo.slotNumber,
                   cpmLocalInfo.nodeType.length,
                   cpmLocalInfo.nodeType.value,
                   cpmLocalInfo.nodeIdentifier.length,
                   cpmLocalInfo.nodeIdentifier.value);        
        return CL_CPM_RC(CL_ERR_TRY_AGAIN);
    }

    if(nodeAddress != clIocLocalAddressGet())
    {
        rc = clAmsCheckNodeJoinState((const ClCharT*)cpmLocalInfo.nodeName);
        if(rc != CL_OK)
            return rc;
    }
    clLogMultiline(CL_LOG_NOTICE, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                   "Got registration request from node [%s]:\n"
                   "Node ID:            [%d]\n"
                   "IOC address:        [%d]\n"
                   "IOC port:           [%#x]\n"
                   "Default boot level: [%d]\n"
                   "Slot Number:        [%d]\n"
                   "Node Type:          [%*s]\n"
                   "Node Identifier:    [%*s]\n"
                   "Node MOID string:   [%*s]",
                   cpmLocalInfo.nodeName,
                   cpmLocalInfo.nodeId,
                   cpmLocalInfo.cpmAddress.nodeAddress,
                   cpmLocalInfo.cpmAddress.portId,
                   cpmLocalInfo.defaultBootLevel,
                   cpmLocalInfo.slotNumber,
                   cpmLocalInfo.nodeType.length,
                   cpmLocalInfo.nodeType.value,
                   cpmLocalInfo.nodeIdentifier.length,
                   cpmLocalInfo.nodeIdentifier.value,
                   cpmLocalInfo.nodeMoIdStr.length,
                   cpmLocalInfo.nodeMoIdStr.value);
    
    /*
     * Verify the version information. If version verification fails shutdown
     * the CPM/L.
     */
    rc = clVersionVerify(&clCpmServerToServerVersionDb, &cpmLocalInfo.version);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_LOG_MESSAGE_0_VERSION_MISMATCH);
        rc = clCpmClientRMDAsyncNew(cpmLocalInfo.cpmAddress.nodeAddress, 
                                    CPM_NODE_SHUTDOWN, 
                                    (ClUint8T *)&(cpmLocalInfo.cpmAddress.nodeAddress), 1,
                                    NULL, NULL, 0, 0, 0, CL_IOC_HIGH_PRIORITY,
                                    clXdrMarshallClUint32T);
    }

    /*
     * Locate the Node from node Table and assign the pointer to the data 
     */
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(cpmLocalInfo.nodeName, &cpmL);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("CPM", "REG", "Node [%s] not found. Failure code [%#x]", 
                   cpmLocalInfo.nodeName, rc);
        goto failure;
    }
    cpmL->pCpmLocalInfo =
        (ClCpmLocalInfoT *) clHeapAllocate(sizeof(ClCpmLocalInfoT));
    if (cpmL->pCpmLocalInfo == NULL)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    memcpy(cpmL->pCpmLocalInfo, &cpmLocalInfo, sizeof(ClCpmLocalInfoT));
    cpmL->pCpmLocalInfo->status = CL_CPM_EO_ALIVE;
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    /**
     * Card Type Mismatch handling 
     */

    slotClassTypes.numItems = 0;
    slotClassTypes.nodeClassTypes = NULL;
    
    clOsalMutexLock(&gpClCpm->cpmMutex);
    rc = cpmSlotClassTypesGet(gpClCpm->slotTable, cpmLocalInfo.slotNumber, 
                              &slotClassTypes);
    /* 
     * Bug 4825:
     * Added the if statement to conditionally call clCpmCardMatch only if
     * cpmSlotClassTypesGet succeds.The else block take care of the 
     * cpmSlotClassTypesGet failure.
     */
    if (rc == CL_OK)
    {
        flag = clCpmCardMatch(&cpmLocalInfo.nodeType, 
                              &cpmLocalInfo.nodeIdentifier, 
                              &slotClassTypes);
    }
    else
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                   "Unable to get slot class types for node "
                   "with slot number [%d], error [%#x]",
                   cpmLocalInfo.slotNumber,
                   rc);
        flag = CL_FALSE;
    }

    if(flag == CL_TRUE)
    {
        /* Set the CPM/L to its default boot Level */
        strcpy(nodeName.value, cpmLocalInfo.nodeName);
        nodeName.length = strlen(nodeName.value);

        srcInfo.srcIocAddress = CPM_RESPOND_TO_ACTIVE;
        srcInfo.srcPort = gpClCpm->pCpmLocalInfo->cpmAddress.portId;
        srcInfo.rmdNumber = CPM_NODE_CPML_RESPONSE; 
        /* FIXME: fill the proper value */
        rc = clCpmBootLevelSet(&nodeName, &srcInfo, 
                               cpmLocalInfo.defaultBootLevel);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(&gpClCpm->cpmMutex);
            clLogError("BM", "SET", CL_CPM_LOG_1_CLIENT_BM_SET_LEVEL_ERR, rc);
            goto failure;
        }

        /*
         * Do the CPM\L dataSet checkpoint 
         */
        rc = cpmCkptCpmLDatsSet();
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
    }
    else
    {
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        clLogMultiline(CL_LOG_CRITICAL, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                       "Node [%s] with node type [%*s] and node identifier [%*s] "
                       "is not allowed in slot [%d] !! \n"
                       "Please check [%s] file in the $ASP_CONFIG and "
                       "add the node class type [%*s] for slot [%d]",
                       cpmLocalInfo.nodeName,
                       cpmLocalInfo.nodeType.length,
                       cpmLocalInfo.nodeType.value,
                       cpmLocalInfo.nodeIdentifier.length,
                       cpmLocalInfo.nodeIdentifier.value,
                       cpmLocalInfo.slotNumber,
                       CL_CPM_SLOT_FILE_NAME,
                       cpmLocalInfo.nodeType.length,
                       cpmLocalInfo.nodeType.value,
                       cpmLocalInfo.slotNumber);

        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                      "Shutting down the node due to node admission "
                      "control failure...");
        
        /** Bug 4825:
         * Added the 2nd condition in the below if check to distinguish 
         * the shutdown call for self or remote.
         * */
        if((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) &&
           (cpmLocalInfo.cpmAddress.nodeAddress == clIocLocalAddressGet()) && 
           (gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE))
        {       
            cpmShutdownHeartbeat();
            return CL_OK;
        }
        else
            rc = clCpmClientRMDAsyncNew(cpmLocalInfo.cpmAddress.nodeAddress, 
                                        CPM_NODE_SHUTDOWN, 
                                        (ClUint8T *)&(cpmLocalInfo.cpmAddress.nodeAddress), 1,
                                        NULL, NULL, 0, 0, 0, CL_IOC_HIGH_PRIORITY,
                                        clXdrMarshallClUint32T);

        goto failure;
    }
    
    failure:
    if(slotClassTypes.nodeClassTypes != NULL)
        clHeapFree(slotClassTypes.nodeClassTypes);
    return rc;
}

ClRcT VDECL(cpmCpmLocalDeregister)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL;
    ClCpmLocalInfoT cpmLocalInfo;
    ClCpmLocalInfoT *pCpmLocalInfo = NULL;

    /*
     * Param Check 
     */
    rc = VDECL_VER(clXdrUnmarshallClCpmLocalInfoT, 4, 0, 0)(inMsgHandle, (void *) &cpmLocalInfo);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);


    /*
     * Locate the Node from node Table and assign the pointer to the data 
     */
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(cpmLocalInfo.nodeName, &cpmL);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("CPM", "DEREG", "Node [%s] not found. Failure code [%#x]",
                   cpmLocalInfo.nodeName, rc);
        goto failure;
    }

    /*
     * Bug 4424:
     * Added NULL check.
     */
    if (cpmL->pCpmLocalInfo != NULL)
    {
        pCpmLocalInfo = cpmL->pCpmLocalInfo;
        cpmL->pCpmLocalInfo = NULL;
    }
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    if(pCpmLocalInfo)
        clHeapFree(pCpmLocalInfo);

    /*
     * Do the CPM\L dataSet checkpoint 
     */
    rc = cpmCkptCpmLDatsSet();

    return CL_OK;

  failure:
    return rc;
}

ClRcT VDECL(cpmCpmConfirm)(ClEoDataT data,
                           ClBufferHandleT inMsgHandle,
                           ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLcmResponseT response;
    ClNameT compName;

    /*
     * Param Check 
     */
    rc = VDECL_VER(clXdrUnmarshallClCpmLcmResponseT, 4, 0, 0)(inMsgHandle, (void *) &response);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (gpClCpm->cpmToAmsCallback != NULL &&
        gpClCpm->cpmToAmsCallback->compOperationComplete != NULL)
    {
        strcpy(compName.value, response.name);
        compName.length = strlen(compName.value);
        gpClCpm->cpmToAmsCallback->compOperationComplete(compName,
                                                         response.requestType,
                                                         response.returnCode);
    }
    else
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);

  failure:
    return rc;
}

/*
 * Bug 3610: Changed nodeshutdown handling.
 * Instead of cpmNodeShutDown() there are two functions now.
 *  cpmProcNodeShutDownReq() to handle the request for shutdown.
 *  cpmNodeShutDown() is now called to indicate that the node 
 *  departure is allowed and the node can be shutdown.
 */
ClRcT VDECL(cpmNodeShutDown)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle)
{
    ClIocNodeAddressT iocAddress;
    ClRcT   rc = CL_OK;
    
    rc = clXdrUnmarshallClUint32T(inMsgHandle, &iocAddress);
    if(rc != CL_OK)
        goto failure;

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
               CL_CPM_LOG_1_SERVER_CPM_NODE_SHUTDOWN_INFO, iocAddress);

    CL_DEBUG_PRINT(CL_DEBUG_INFO, 
            ("Got ShutDown Allowed Response from master, shutting down...\n"));
    
    if(iocAddress == clIocLocalAddressGet())
    {
        /* Do the self shutdown */
        rc = cpmSelfShutDown();
    }
    else if(iocAddress == CL_IOC_BROADCAST_ADDRESS)
    {
        /*
         * BROADCAST address node shutdowns are treated as payload node
         * restarts.
         */

        if(CL_CPM_IS_WB())
        {
            ClTimerTimeOutT delay = {.tsSec = 6, .tsMilliSec = 0 };
            clLogNotice("NODE", "SHUTDOWN", "Worker node got a restart request from the master."
                        "This should be mostly from a split brain recovery");
            cpmRestart(&delay, "payload");

            /*
             * Unreached actually.
             */
            CL_ASSERT(0);
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Got shutdown allowed command at node [%d] for node [%d]...", 
                 clIocLocalAddressGet(), iocAddress));
        rc = CL_CPM_RC(CL_CPM_ERR_BAD_OPERATION);
    }

    return rc;
failure:
    return rc;
}



static void cpmNodeDepartureEventPublish(ClIocNodeAddressT node, ClBoolT graceful, ClBoolT doSelf)
{
    ClRcT rc;
    ClNameT nodeName;
    ClCpmNodeEventT nodeEvent = CL_CPM_NODE_DEPARTURE;

    if(doSelf
       &&
       node != clIocLocalAddressGet())
        goto out;

    if(node == clIocLocalAddressGet())
        memcpy(nodeName.value, gpClCpm->pCpmLocalInfo->nodeName, CL_MAX_NAME_LENGTH);
    else
        _cpmNodeNameForNodeAddressGet(node, &nodeName);
    nodeName.length = (strlen(nodeName.value) > CL_MAX_NAME_LENGTH)? CL_MAX_NAME_LENGTH :strlen(nodeName.value);
    if(!graceful)
        nodeEvent = CL_CPM_NODE_DEATH;
    rc = nodeArrivalDeparturePublish(node, nodeName, nodeEvent);
    if(rc != CL_OK)
    {
        clLogError("NOD", "DEP", "Failed to publish node [0x%x] departure event. error code [0x%x].", node, rc);
    }

out:
    return;
}

/*
 * Bug 3610: Added this function.
 * The function handles processing of the node shutdown request.
 * Request may come from 
 *  The client for shutting down this node.
 *  To CPM/G active from other CPMs to inform CPM/G of their departures.
 *  
 * If permission is required from master then the request is forwarded to
 * master, else selfShutDown is done.
 */
static ClRcT cpmNodeShutdownTimeout(void *unused)
{
    ClCharT cmdbuf[0xff+1];
    const ClCharT *aspDir = getenv("ASP_DIR");
    if(!aspDir) aspDir = "/root/asp";
    snprintf(cmdbuf, sizeof(cmdbuf), "%s/etc/init.d/asp zap", aspDir);
    clLogNotice("SHUTDOWN", "TIMER", "Node shutdown timer running cmd [ %s ]", cmdbuf);
    if(system(cmdbuf))
    {
        clLogWarning("SHUTDOWN", "TIMER", "Shutdown cmd returned with error [%s]", strerror(errno));
    }
    return CL_OK;
}
static void startShutdownTimer(ClIocNodeAddressT nodeAddress)
{
#define ASP_SHUTDOWN_DEFAULT_TIMEOUT (120)
    ClTimerHandleT timer;
    ClTimerTimeOutT timeout = {.tsSec = ASP_SHUTDOWN_DEFAULT_TIMEOUT, .tsMilliSec = 0 };
    ClRcT rc;
    ClCharT *str;
    if( (str = getenv("ASP_SHUTDOWN_TIMEOUT") ) )
    {
        timeout.tsSec = atoi(str);
        if(!timeout.tsSec)
            timeout.tsSec = ASP_SHUTDOWN_DEFAULT_TIMEOUT;
    }
    rc = clTimerCreateAndStart(timeout, CL_TIMER_VOLATILE, CL_TIMER_TASK_CONTEXT, 
                               cpmNodeShutdownTimeout, NULL, &timer);
    if(rc != CL_OK)
        clLogWarning("SHUTDOWN", "TIMER", "Timer start for node [%#x] shutdown failed with [%#x]", nodeAddress, rc);
    else clLogNotice("SHUTDOWN", "TIMER", "Node [%#x] shutdown timer set to fire in [%d] seconds", 
                     nodeAddress, timeout.tsSec);
}

ClRcT VDECL(cpmProcNodeShutDownReq)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClIocNodeAddressT iocAddress;
    ClIocNodeAddressT masterAddress;
    ClRcT   rc = CL_OK;
    ClCpmLT *cpmL = NULL;
    ClNameT nodeName;
    
    rc = clXdrUnmarshallClUint32T(inMsgHandle, &iocAddress);
    if(rc != CL_OK)
        goto failure;

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
               CL_CPM_LOG_1_SERVER_CPM_NODE_SHUTDOWN_INFO, iocAddress);

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Processing Shut down request for Node [%d]\n", iocAddress));
    
    /* If boot level has not reached default then Do not ask AMS
     * for node leave as AMS does not consider node as part of 
     * cluster member. nodeJoin is called only after default boot
     * level is reached.
     */
    if(iocAddress == clIocLocalAddressGet())
    {
        if((gpClCpm->bmTable->currentBootLevel < 
                    gpClCpm->pCpmLocalInfo->defaultBootLevel) || 
                ((gpClCpm->bmTable->currentBootLevel == 
                  gpClCpm->pCpmLocalInfo->defaultBootLevel) && 
                 (gpClCpm->bmTable->lastBootStatus == CL_FALSE)))
        {
            rc = cpmSelfShutDown();
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Unable to do SelfShutDown, Node Address [%d]," 
                         " error code [0x%x]", iocAddress, rc));
            }
            return rc;
        }
        startShutdownTimer(iocAddress);
    }

    cpmNodeDepartureEventPublish(iocAddress, CL_TRUE, CL_TRUE);
    
    /* For Cpm\L and standby CPM/G 
     * Check if need to ask master for departure, if so
     * Send shutdown request to master
     */
    if(iocAddress == clIocLocalAddressGet() && 
            (CL_CPM_IS_STANDBY() || CL_CPM_IS_WB()))
    {
        rc = clCpmMasterAddressGet(&masterAddress);
        
        CL_DEBUG_PRINT(CL_DEBUG_INFO,
                ("Shutdown node [%d] and master Address is [%d]", 
                 iocAddress, masterAddress));

        if(rc == CL_OK)
        {
            rc = clCpmClientRMDAsyncNew(masterAddress, CPM_PROC_NODE_SHUTDOWN_REQ,
                    (ClUint8T *)&(iocAddress), 1,
                    NULL, NULL, 0, 0, 0, CL_IOC_HIGH_PRIORITY,
                    clXdrMarshallClUint32T);
            if(CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Sending shutdown request for [%d] to master failed..."
                         " master unreachabele. Doing self shutdown...", 
                         iocAddress));
                rc = cpmSelfShutDown();
            }
            else if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Failure in sending request for Shutdown of node [%d], rc=[0x%x]", 
                         iocAddress, rc));
            }
        }
        else
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_CPM_CLIENT_LIB,
                    CL_CPM_LOG_1_CLIENT_NODE_SHUTDOWN, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Shutdown [%d] self, master Address not valid\n",
                iocAddress));
            rc = cpmSelfShutDown();
        }

        return rc;
    }
    
    /* Bug 3941: erroneous conditions results in Active shutdown if 
     *  node is not found.
     * If node is found call AMS that node is leaving else
     * return error
     * FIXME :REMOVE OLD COMMENTED CODE
     */
    if(CL_CPM_IS_ACTIVE())
    {
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindByIocAddress(iocAddress, &cpmL);
        if(rc == CL_OK)
        {
            strcpy(nodeName.value, cpmL->pCpmLocalInfo->nodeName);
            nodeName.length = strlen(nodeName.value) + 1;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            /*
             * Setup a restart override to halt the node in case we are interrupted 
             * by process faults/escalations during node shutdown process.
             */
            cpmEnqueueAspRequest(&nodeName, iocAddress, CL_CPM_HALT_ASP);
            if(gpClCpm->cpmToAmsCallback != NULL &&  
                    gpClCpm->cpmToAmsCallback->nodeLeave != NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Calling AMS node leave..."));
                rc = gpClCpm->cpmToAmsCallback->nodeLeave(&nodeName, 
                                                          CL_CPM_NODE_LEAVING, CL_FALSE);
                if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_INFO, 
                                   ("Node [%.*s] is not a cluster member. "\
                                    "Calling node departure allowed\n",
                                    nodeName.length, nodeName.value));
                    rc = _cpmNodeDepartureAllowed(&nodeName, CL_CPM_NODE_LEAVING);
                }
            }
            /* Handling of Non-AMS case */
            else
            {
                rc = _cpmNodeDepartureAllowed(&nodeName, CL_CPM_NODE_LEAVING);
            }
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            /* Could not find the node 
             *   Node May Not Exist OR
             *   May not yet registered with CPM/G
             * For both scenarios it is non existant for CPM/G
             * so return the error code */
            
            /* FIXME : CAN TRY Shutting down of node through HPI for unregistered node
             *  But check if it will not cause problema for non-existance case
             * BUT all this LATER
             */
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CLIENT_NODE_SHUTDOWN, 
                    rc, rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);
        }
    }

    return CL_OK;
failure:
    return rc;
}

/* 
 * This function will be called to do the actual shut down of the self.
 * The polling thread is set to 0 to bring CPM main thread out of 
 * while loop and start shutting down.
 */

ClRcT cpmSelfShutDown(void)
{
    ClRcT rc = CL_OK;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Setting polling to zero...\n"));

    cpmShutdownHeartbeat();
    return rc;
}

void cpmWriteToFile(ClCharT *fname, ClCharT *s, ClCharT n)
{
    FILE *fp = NULL;

    if (NULL == (fp = fopen(fname, "w")))
    {
        clLogWarning(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                     "Unable to open file [%s] for writing : [%s]",
                     fname,
                     strerror(errno));
        return;
    }

    if(fwrite(s, strlen(s), 1, fp) == 0 
       ||
       ferror(fp)) 
    {
        clLogWarning(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                     "Unable to write file [%s] : [%s]",
                     fname,
                     strerror(errno));
    }

    fclose(fp);
}

void cpmWriteNodeStatToFile(const ClCharT *who, ClBoolT isUp)
{
    ClCharT fname[CL_MAX_NAME_LENGTH] = {0};
    ClCharT iocAddrStr[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *aspRunDir = NULL;
    ClCharT aspRunPath[CL_MAX_NAME_LENGTH] = {0};

    aspRunDir = getenv("ASP_RUNDIR");
    if (aspRunDir)
    {
        strncpy(aspRunPath, aspRunDir, CL_MAX_NAME_LENGTH-1);
    }
    else
    {
        strncpy(aspRunPath, ".", CL_MAX_NAME_LENGTH-1);
    }
    
    if (!strcmp(who, "CPM"))
    {
        strncpy(fname, aspRunPath, CL_MAX_NAME_LENGTH-1);
        strncat(fname, "/aspstate", CL_MAX_NAME_LENGTH-1);
        snprintf(iocAddrStr, CL_MAX_NAME_LENGTH-1,
                 "-%d", clIocLocalAddressGet());
        strncat(fname, iocAddrStr, CL_MAX_NAME_LENGTH-1);
    }
    else if (!strcmp(who, "AMS"))
    {
        strncpy(fname, aspRunPath, CL_MAX_NAME_LENGTH-1);
        strncat(fname, "/ams", CL_MAX_NAME_LENGTH-1);
        snprintf(iocAddrStr, CL_MAX_NAME_LENGTH-1,
                 "%d", clIocLocalAddressGet());
        strncat(fname, iocAddrStr, CL_MAX_NAME_LENGTH-1);
    }
    else
    {
        CL_ASSERT(0);
    }

    if (isUp)
    {
        cpmWriteToFile(fname, "1\n", 1);
    }
    else
    {
        cpmWriteToFile(fname, "0\n", 1);
    }
}

ClRcT VDECL(cpmNodeCpmLResponse)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClCpmBmSetLevelResponseT cpmBmResponse;
    ClRcT   rc = CL_OK;
    ClCpmLT *tempNode = NULL;
    
    rc = VDECL_VER(clXdrUnmarshallClCpmBmSetLevelResponseT, 4, 0, 0)(inMsgHandle, &cpmBmResponse);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    
    if(cpmBmResponse.retCode == CL_OK)
    {
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        if( (rc = cpmNodeFindLocked(cpmBmResponse.nodeName.value, &tempNode) ) == CL_OK
            &&
            tempNode && tempNode->pCpmLocalInfo )
        {
            ClUint32T slot = tempNode->pCpmLocalInfo->cpmAddress.nodeAddress;
            ClBoolT asserted = CL_FALSE;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            if(clCmThresholdStateGet(slot, NULL, &asserted) == CL_OK
               &&
               asserted == CL_TRUE)
            {
                clLogNotice("NODE", "JOIN", "Node [%s] at slot [%d] has threshold in asserted state."
                            "Deferring node join till the assert is cleared for the slot",
                            cpmBmResponse.nodeName.value, slot);
                cpmWriteNodeStatToFile("AMS", CL_YES);
                goto failure;
            } 
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        }
        rc = CL_OK;
        cpmBmResponse.nodeName.length = cpmBmResponse.nodeName.length+1;

        if(gpClCpm->cpmToAmsCallback != NULL && 
                gpClCpm->cpmToAmsCallback->nodeJoin != NULL)
        {
            rc = gpClCpm->cpmToAmsCallback->nodeJoin(&(cpmBmResponse.nodeName));
            if (CL_OK != rc)
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                           "Node join failed, error [%#x]", rc);
            }
            else
            {
                cpmWriteNodeStatToFile("AMS", CL_YES);
            }
        }
    }
    else
    {
        if((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL &&
                gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE) && 
                !strcmp(cpmBmResponse.nodeName.value, 
                    gpClCpm->pCpmLocalInfo->nodeName))
        {       
            cpmShutdownHeartbeat();
            return CL_OK;
        }
        else
        {
            clOsalMutexLock(gpClCpm->cpmTableMutex);
            rc = cpmNodeFindLocked(cpmBmResponse.nodeName.value, &tempNode);
            if(rc == CL_OK && tempNode != NULL && tempNode->pCpmLocalInfo != NULL)
            {
                ClIocNodeAddressT node = tempNode->pCpmLocalInfo->cpmAddress.nodeAddress;
                clOsalMutexUnlock(gpClCpm->cpmTableMutex);
                clCpmNodeShutDown(node);
            }
            else
            {
                clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            }
        }
    }
    return rc;

failure:
    return rc;
}

static void cpmCmRequestDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    
}

static unsigned int cpmCmRequestHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT)key % CL_CPM_CPML_TABLE_BUCKET_SIZE);
}

static int cpmCmRequestCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

static ClRcT cpmResetRequestFinalize(void)
{
    struct hashStruct *iter;
    ClUint32T i;
    for(i = 0; i < CPM_RESET_TABLE_SIZE; ++i)
    {
        struct hashStruct *next;
        for(iter = cpmResetMsgTable[i]; iter; iter = next)
        {
            ClCpmResetMsgT *msg;
            next = iter->pNext;
            msg = hashEntry(iter, ClCpmResetMsgT, hash);
            clHeapFree(msg);
        }
        cpmResetMsgTable[i] = NULL;
    }
    return CL_OK;
}

ClRcT cpmCmRequestDSFinalize(void)
{
    ClRcT   rc = CL_OK;
    cpmResetRequestFinalize();
    clCntDelete(gpClCpm->cmRequestTable);
    return rc;
}

ClRcT cpmCmRequestDSInitialize(void)
{
    ClRcT   rc = CL_OK;
    
    if((rc = clCntHashtblCreate(CL_CPM_CPML_TABLE_BUCKET_SIZE, 
                    cpmCmRequestCompare,
                    cpmCmRequestHashFunc, NULL, cpmCmRequestDelete,
                    CL_CNT_NON_UNIQUE_KEY, &gpClCpm->cmRequestTable)) != CL_OK)
        return rc;
    
    if((rc = clOsalMutexCreate(&(gpClCpm->cmRequestMutex))) != CL_OK)
        return rc;

    return rc;
}

static ClCpmResetMsgT *cpmResetRequestFind(const ClNameT *nodeName, ClUint32T msgType, ClUint32T *key)
{
    struct hashStruct *iter;
    ClUint16T hashKey = 0;
    clCksm16bitCompute((ClUint8T*)nodeName->value, strlen(nodeName->value), &hashKey);
    hashKey &= CPM_RESET_TABLE_MASK;
    if(key) *key = hashKey;
    for(iter = cpmResetMsgTable[hashKey]; iter; iter = iter->pNext)
    {
        ClCpmResetMsgT *msg = hashEntry(iter, ClCpmResetMsgT, hash);
        if(msg->msgType == msgType
           &&
           !strcmp( (const ClCharT*)nodeName->value, (const ClCharT*)msg->nodeName.value ) )
            return msg;
    }
    return NULL;
}

/*
 * Delete all the request entries for the node from table
 */
void cpmResetDeleteRequest(const ClNameT *nodeName)
{
    ClUint16T hashKey = 0;
    struct hashStruct *iter, *next = NULL;
    if(!nodeName) return;
    clCksm16bitCompute((ClUint8T*)nodeName->value, strlen(nodeName->value), &hashKey);
    hashKey &= CPM_RESET_TABLE_MASK;
    clOsalMutexLock(gpClCpm->cmRequestMutex);
    for(iter = cpmResetMsgTable[hashKey]; iter; iter = next)
    {
        ClCpmResetMsgT *msg = hashEntry(iter, ClCpmResetMsgT, hash);
        next = iter->pNext;
        if(!strcmp((const ClCharT*)msg->nodeName.value, (const ClCharT*)nodeName->value))
        {
            hashDel(&msg->hash);
            clHeapFree(msg);
        }
    }
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
}

static ClRcT cpmEnqueueResetRequest(ClCpmResetMsgT *resetMsg)
{
    ClCpmResetMsgT *msg;
    ClUint32T hashKey = 0;
    if( ( msg = cpmResetRequestFind(&resetMsg->nodeName, resetMsg->msgType, &hashKey) ) )
        return CL_ERR_ALREADY_EXIST;
    hashAdd(cpmResetMsgTable, hashKey, &resetMsg->hash);
    return CL_OK;
}

ClRcT cpmDequeueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest)
{
    ClCpmResetMsgT *msg;
    clOsalMutexLock(gpClCpm->cmRequestMutex);
    msg = cpmResetRequestFind(pNodeName, _CM_RESET_MSG, NULL);
    if(!msg)
    {
        clOsalMutexUnlock(gpClCpm->cmRequestMutex);
        return CL_ERR_NOT_EXIST;
    }
    hashDel(&msg->hash);
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    memcpy(pRequest, &msg->cmResetMsg, sizeof(*pRequest));
    clHeapFree(msg);
    return CL_OK;
}

ClRcT cpmEnqueueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest)
{
    ClRcT rc;
    ClCpmResetMsgT *resetMsg = clHeapCalloc(1, sizeof(*resetMsg));
    CL_ASSERT(resetMsg != NULL);
    resetMsg->msgType = _CM_RESET_MSG;
    memcpy(&resetMsg->cmResetMsg, pRequest, sizeof(resetMsg->cmResetMsg));
    memcpy(&resetMsg->nodeName, pNodeName, sizeof(resetMsg->nodeName));
    clOsalMutexLock(gpClCpm->cmRequestMutex);
    rc = cpmEnqueueResetRequest(resetMsg);
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    if(rc != CL_OK)
        clHeapFree(resetMsg);
    return rc;
}

ClRcT cpmDequeueAspRequest(ClNameT *pNodeName, ClUint32T *nodeRequest)
{
    ClCpmResetMsgT *msg;
    clOsalMutexLock(gpClCpm->cmRequestMutex);
    if ( !(msg = cpmResetRequestFind(pNodeName, _ASP_RESET_MSG, NULL) ) )
    {
        clOsalMutexUnlock(gpClCpm->cmRequestMutex);
        return CL_ERR_NOT_EXIST;
    }
    hashDel(&msg->hash);
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    if(nodeRequest) 
        *nodeRequest = msg->aspResetMsg.nodeRequest;
    clHeapFree(msg);
    return CL_OK;
}

ClRcT cpmEnqueueAspRequest(ClNameT *pNodeName, ClIocNodeAddressT nodeAddress, ClUint32T nodeRequest)
{
    ClRcT rc = CL_OK;
    ClCpmResetMsgT *resetMsg = clHeapCalloc(1, sizeof(*resetMsg));
    CL_ASSERT(resetMsg != NULL);
    resetMsg->msgType = _ASP_RESET_MSG;
    memcpy(&resetMsg->nodeName, pNodeName, sizeof(resetMsg->nodeName));
    resetMsg->aspResetMsg.nodeAddress = nodeAddress;
    resetMsg->aspResetMsg.nodeRequest = nodeRequest;
    clOsalMutexLock(gpClCpm->cmRequestMutex);
    rc = cpmEnqueueResetRequest(resetMsg);
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    if(rc != CL_OK)
        clHeapFree(resetMsg);
    return rc;
}

ClRcT VDECL(cpmNodeArrivalDeparture)(ClEoDataT data,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T msgLength = 0;
    ClCmCpmMsgT cmCpmMsg;
    ClCpmLT *cpmL = NULL;
    ClNameT nodeName;

    memset(&cmCpmMsg, 0, sizeof(ClCmCpmMsgT));
    memset(&nodeName, 0, sizeof(ClNameT));
 
    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if (msgLength == sizeof(ClCmCpmMsgT))
    {
        rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &cmCpmMsg,
                                       &msgLength);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to read the message \n"), rc);
    }
    else
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                     CL_CPM_RC(CL_ERR_INVALID_BUFFER));

    clLogMultiline(CL_LOG_NOTICE, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                   "Got the following message from CM :\n"
                   "Message type : [%s]\n"
                   "Physical slot : [%d]\n"
                   "Sub slot : [%d]\n"
                   "Resource Id : [%d]\n",
                   (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_SURPRISE_EXTRACTION) ?
                   "surprise extraction" :
                   (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_REQ_INSERTION) ?
                   "blade insertion" :
                   (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_REQ_EXTRACTION) ?
                   "blade extraction" :
                   (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_NODE_ERROR_REPORT) ?
                   "node error report":
                   (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_NODE_ERROR_CLEAR) ?
                   "node error clear" : "invalid message",
                   cmCpmMsg.physicalSlot,
                   cmCpmMsg.subSlot,
                   cmCpmMsg.resourceId);
    
    if (cmCpmMsg.cmCpmMsgType == CL_CM_BLADE_REQ_INSERTION)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,"Got blade insertion request from CM, NO-OP");
    }
    else
    {
        ClCpmCpmToAmsCallT *cpmToAmsCallback = gpClCpm->cpmToAmsCallback;
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindByIocAddress(cmCpmMsg.physicalSlot, &cpmL);
        if (rc == CL_OK)
        {
            strcpy(nodeName.value, cpmL->pCpmLocalInfo->nodeName);
            nodeName.length = strlen(nodeName.value) + 1;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            switch(cmCpmMsg.cmCpmMsgType)
            {
                case CL_CM_BLADE_REQ_EXTRACTION:
                case CL_CM_BLADE_NODE_ERROR_REPORT:
                {
                    ClRcT rc2 = cpmEnqueueCmRequest(&nodeName, &cmCpmMsg);
                    if ((NULL != cpmToAmsCallback) &&
                        (NULL != cpmToAmsCallback->nodeLeave))
                    {
                        rc = cpmToAmsCallback->nodeLeave(&nodeName,
                                                         CL_CPM_NODE_LEAVING, CL_FALSE);
                        if (CL_OK != rc)
                        {
                            if(rc2 == CL_OK)
                            {
                                cpmDequeueCmRequest(&nodeName, &cmCpmMsg);
                            }
                            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                       "AMS node leave[leaving] failed for node [%s] with "
                                       "error [%#x]", nodeName.value, rc);
                            goto failure;
                        }
                    }
                }
                break;
                case CL_CM_BLADE_SURPRISE_EXTRACTION:
                {
                    ClRcT rc2 = cpmEnqueueCmRequest(&nodeName, &cmCpmMsg);
                    if ((NULL != cpmToAmsCallback) &&
                        (NULL != cpmToAmsCallback->nodeLeave))
                    {
                        rc = cpmToAmsCallback->nodeLeave(&nodeName,
                                                         CL_CPM_NODE_LEFT, CL_FALSE);
                        if (CL_OK != rc)
                        {
                            if(rc2 == CL_OK)
                            {
                                cpmDequeueCmRequest(&nodeName, &cmCpmMsg);
                            }
                            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                       "AMS node leave[left] failed for node [%s] with "
                                       "error [%#x]", nodeName.value, rc);
                            goto failure;
                        }
                    }
                }
                break;
                case CL_CM_BLADE_NODE_ERROR_CLEAR:
                {
                    if ((NULL != cpmToAmsCallback) &&
                        (NULL != cpmToAmsCallback->nodeJoin))
                    {
                        rc = cpmToAmsCallback->nodeJoin(&nodeName);
                        if (CL_OK != rc)
                        {
                            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                       "AMS node join failed for node [%s] with error [%#x]",
                                       nodeName.value, rc);
                            goto failure;
                        }
                    }
                }
                break;
                default:
                {
                    CL_ASSERT(0);
                    break;
                }
            }
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        }
    }

    return CL_OK;

failure:
    return rc;
}

ClRcT _cpmIocAddressForNodeGet(ClNameT *nodeName, ClIocAddressT *pIocAddress) 
{
    ClRcT   rc = CL_OK;
    ClCpmLT *node = NULL;

    if(!nodeName || !pIocAddress) 
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(nodeName->value, &node);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("ADDR", "GET", "Address get for node [%s] returned [%#x]",
                   nodeName->value, rc);
        goto failure;
    }
    
    /*
     * Bug 4428:
     * Added the if-else block for NULL checking.
     */
    if (node->pCpmLocalInfo != NULL)
    {
        memcpy(&(pIocAddress->iocPhyAddress), &(node->pCpmLocalInfo->cpmAddress), 
                sizeof(ClIocPhysicalAddressT));
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }

    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

failure:
    return rc;
}

ClRcT _cpmNodeNameForNodeAddressGet(ClIocNodeAddressT nodeAddress, ClNameT *pNodeName)
{
    ClRcT   rc = CL_OK;
    ClCpmLT *node = NULL;

    if(!pNodeName)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gpClCpm->cpmMutex);
    if(!gpClCpm->pCpmLocalInfo)
    {
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        return CL_CPM_RC(CL_ERR_NOT_EXIST);
    }
    if(gpClCpm->pCpmLocalInfo->nodeId == nodeAddress)
    {
        clNameSet(pNodeName, gpClCpm->pCpmLocalInfo->nodeName);
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        return CL_OK;
    }
    clOsalMutexUnlock(&gpClCpm->cpmMutex);

    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindByNodeId(nodeAddress, &node);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("NODE", "GET", "Unable to find node address [%d] in CPM node table",
                   nodeAddress);
        goto failure;
    }

    /*
     * Bug 4428:
     * Added the if-else block for NULL checking.
     */
    if (node->pCpmLocalInfo != NULL)
    {
        clNameSet(pNodeName, node->pCpmLocalInfo->nodeName);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }
    
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

failure:
    return rc;
}

ClRcT cpmInitiatedSwitchOver(ClBoolT checkDeputy)
{
    ClRcT rc = CL_OK;
    
    /*
     * Bug 4094:
     * Do the cluster leave only if there is a
     * CPM/G standby in the cluster.
     */
    if (!checkDeputy || (checkDeputy && gpClCpm->deputyNodeId != -1))
    {
        /* Leave the GMS group, this will result in the switchover */
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_CPM_LOG_0_CLUSTER_LEAVE_INFO);
        cpmWriteNodeStatToFile("AMS", CL_NO);
        cpmWriteNodeStatToFile("CPM", CL_NO);
        /*
         * We shouldnt be back from here.
         * Doing this to get the split brain detection
         * easier
         */
        gpClCpm->nodeLeaving = CL_TRUE;
        gAms.cpmRecoveryQuiesced = CL_TRUE; /* disable AMF recovery as we are ejecting out*/
        clGmsClusterLeaveNative(gpClCpm->cpmGmsHdl, 10000000000LL, 
                                gpClCpm->pCpmLocalInfo->nodeId);

        return rc;
    }

    return CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
}
    
ClRcT VDECL(cpmIocAddressForNodeGet)(ClEoDataT data, 
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle)
{
    ClRcT   rc = CL_OK;
    ClCpmLT *node = NULL;
    ClNameT nodeName = {0};
    ClIocAddressIDLT idlIocAddress;

    rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &nodeName);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(nodeName.value, &node);
    if(rc != CL_OK || !node->pCpmLocalInfo)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        if(rc == CL_OK)
            rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);

        clLogError("NODE", "GET", "Address get for node [%s] returned [%#x]",
                   nodeName.value, rc);
        goto failure;
    }

    /*
     * Return physical address 
     */
    idlIocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
    memcpy(&(idlIocAddress.clIocAddressIDLT.iocPhyAddress), &(node->pCpmLocalInfo->cpmAddress), 
           sizeof(ClIocPhysicalAddressT));
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    rc = VDECL_VER(clXdrMarshallClIocAddressIDLT, 4, 0, 0)((void *) &idlIocAddress, outMsgHandle,
                                       0);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

failure:
    return rc;
}

/*
 * Bug 4424:
 * Added the function cpmUpdateNodeState().
 * Updates the status to CL_CPM_EO_DEAD so that 
 * heartbeat messages are no longer sent and 
 * publish the event for node failure.
 */
ClRcT cpmUpdateNodeState(ClCpmLocalInfoT *pCpmLocalInfo)
{
    ClRcT rc = CL_OK;
    ClNameT nodeName = {0};
    ClIocNodeAddressT nodeAddress = 0;

    strcpy(nodeName.value, pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value);
    nodeAddress = pCpmLocalInfo->cpmAddress.nodeAddress;

    pCpmLocalInfo->status = CL_CPM_EO_DEAD;

    clLogWarning(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                 "Update Node State is failing the node [%s] id [%d]",
                 nodeName.value, nodeAddress);

    rc = nodeArrivalDeparturePublish(nodeAddress, nodeName, CL_CPM_NODE_DEATH);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_EVT_PUB_NODE_DEPART_ERR,
                   nodeName.value, rc, rc, CL_LOG_ERROR,
                   CL_LOG_HANDLE_APP);

    failure:
    return rc;
}

ClRcT cpmFailoverNode(ClGmsNodeIdT nodeId, ClBoolT scFailover)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL = NULL;
    ClCpmLocalInfoT *pCpmLocalInfo = NULL;
    ClNameT nodeName = {0};
    ClUint32T slotNumber = 0;
    ClIocNodeAddressT nodeAddress = 0;

    clOsalMutexLock(&gpClCpm->cpmMutex);
    clOsalMutexLock(gpClCpm->cpmTableMutex);

    rc = cpmNodeFindByNodeId(nodeId, &cpmL);
    if (CL_OK != rc)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        clLogWarning(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                     "Not able to find node having node ID [%d], "
                     "error [%#x]",
                     nodeId,
                     rc);
        return rc;
    }
    
    if(!(pCpmLocalInfo = cpmL->pCpmLocalInfo))
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        return CL_OK;
    }

    slotNumber = pCpmLocalInfo->slotNumber;
    nodeAddress = pCpmLocalInfo->cpmAddress.nodeAddress;
    cpmL->pCpmLocalInfo = NULL;

    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    clOsalMutexUnlock(&gpClCpm->cpmMutex);

    clLogCritical(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                  "Got a node shutdown/failure for standby/worker blade, "
                  "Failing over node [%s] with node ID [%d]",    
                  pCpmLocalInfo->nodeName,
                  nodeId);

    clLogDebug(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
               "Deregistering IOC TL information for node [%d]...",
               pCpmLocalInfo->cpmAddress.nodeAddress);

#if 0
    clIocTransparencyDeregisterNode(pCpmLocalInfo->cpmAddress.nodeAddress);
#endif

    strcpy(nodeName.value, pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value) + 1;
    
    /*
     * Reset the request queue pertaining to the node
     */
    cpmResetDeleteRequest(&nodeName);

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
               "Publishing node [%d] departure event...",
               pCpmLocalInfo->nodeId);

    cpmUpdateNodeState(pCpmLocalInfo);

    if ((NULL != gpClCpm->cpmToAmsCallback) &&  
        (NULL != gpClCpm->cpmToAmsCallback->nodeLeave))
    {
        rc = gpClCpm->cpmToAmsCallback->nodeLeave(&nodeName,
                                                  CL_CPM_NODE_LEFT,
                                                  scFailover);
        if (CL_OK != rc)
        {
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                         "AMS node left call for [%s] returned [%#x]",
                         pCpmLocalInfo->nodeName,
                         rc);
        }
    }

    clHeapFree(pCpmLocalInfo);

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP,
               "Updating the CPM checkpoints...");
    cpmCkptCpmLDatsSet();

    /*
     * TODO:
     * ----
     * 1. Call fault manager API here, instead of doing blade reset.
     *
     * 2. Fault manager API can be such that, it invokes some user
     * defined action like resetting the node etc.
     */

    /*
     * The node has already left. We dont have to call a node shutdown
     * for this node as it could have left and again come back.
     * Blade reset is the solution. Leave it to HPI.
     */

    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
              "Completed failing over the node [%d]. ",
              nodeAddress);

    return rc;
}

ClRcT cpmShutdownNode(ClCntNodeHandleT key,
                      ClCntDataHandleT data,
                      ClCntArgHandleT arg,
                      ClUint32T size)
{
    ClCpmLT *cpmL = (ClCpmLT *)data;

    if (cpmL->pCpmLocalInfo)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                      "Shutting down node [%s]...",
                      cpmL->nodeName);
        clCpmClientRMDAsyncNew(cpmL->pCpmLocalInfo->cpmAddress.nodeAddress,
                               CPM_NODE_SHUTDOWN,
                               (ClUint8T *)&cpmL->pCpmLocalInfo->
                               cpmAddress.nodeAddress,
                               sizeof(cpmL->pCpmLocalInfo->cpmAddress.nodeAddress),
                               NULL,
                               NULL,
                               0,
                               0,
                               0,
                               CL_IOC_HIGH_PRIORITY,
                               clXdrMarshallClUint32T);
    }

    return CL_OK;
}
                      
void cpmShutdownAllNodes(void)
{
    clCntWalk(gpClCpm->cpmTable, cpmShutdownNode, NULL, 0);
}

/*
 * Register the node user data containing the node version/interface info, etc. with AMF.
 */
#ifndef POSIX_BUILD
static void cpmInstallNodeInfo(void)
{
    ClAmsMgmtHandleT handle = {0};
    ClVersionT version = {'B',0x1,0x1};
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};
    ClNameT key = {0};

    rc = clAmsMgmtInitialize(&handle, NULL, &version);
    if(rc != CL_OK)
    {
        clLogError("NODE", "INSTALL", "Mgmt initialize returned with [%#x]", rc);
        goto out;
    }
    entity.type = CL_AMS_ENTITY_TYPE_NODE;
    clNameSet(&entity.name, clCpmNodeName);
    entity.name.length += 1;
    clNameSet(&key, ASP_INSTALL_KEY);
    rc = clAmsMgmtEntityUserDataSetKey(handle, &entity, &key, 
                                       gAspInstallInfo, 
                                       strlen(gAspInstallInfo));
    if(rc != CL_OK)
    {
        clLogError("NODE", "INSTALL", "Installing node info returned with [%#x]", rc);
        /*
         * Check if the install failed because the leader view was inconsistent
         * assuming the deputy was elected as the leader
         */
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_OPERATION)
        {
            static ClTimerTimeOutT delay = {.tsSec = 2 };
            clLogNotice("NODE", "RESTART", 
                        "Restarting node [%s] since the leadership view "
                        "seems to be inconsistent. Try increasing bootElectionTimeout "
                        "in clGmsConfig.xml for this node.", clCpmNodeName);
            cpmRestart(&delay, "worker");
            /*
             * Unreached actually
             */
            CL_ASSERT(0);
        }
        goto out_close;
    }
    
    out_close:
    clAmsMgmtFinalize(handle);
    
    out:
    return;
}

#else
static void cpmInstallNodeInfo(void) 
{

}
#endif

static void __cpmRegisterWithActive(ClBoolT reregister)
{
    ClRcT rc = CL_OK;
    ClCharT xportConfigErrMsg[] = "IOC is configured properly. \n";
    ClCharT gmsUniquePortErrMsg[] = "The GMS port you are using is unique "
        "in the cluster or somebody else is not "
        "using the same GMS port as yours. \n";
    ClInt32T tries = 0;
    ClGmsNodeIdT leaderNode = CL_GMS_INVALID_NODE_ID;

    cpmBmRespTimerStop();

    clOsalMutexLock(&gpClCpm->clusterMutex);
    leaderNode = gpClCpm->activeMasterNodeId;
    clOsalMutexUnlock(&gpClCpm->clusterMutex);

    if(!leaderNode || leaderNode == CL_GMS_INVALID_NODE_ID)
    {
        rc = CL_ERR_TRY_AGAIN;
        goto leaderless;
    }

    retry:
    rc = clCpmCpmLocalRegister(gpClCpm->pCpmLocalInfo);
    if ((CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST) ||
        (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT) ||
        (CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN) ||
        (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) ||
        (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE))
    {
        /* ensure that it delays... clean up code below later */
        ClTimerTimeOutT timeOut1 = {2, 0};
        clOsalTaskDelay(timeOut1);
        
        leaderless:
        if (CL_CPM_IS_ACTIVE())
        {
            ClTimerTimeOutT timeOut = {2, 0};
            static ClUint32T retries;
            ++retries;
            if((retries & 3)
               && 
               gpClCpm->polling)
            {
                clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                             "CPM/G active waiting for self registration to continue");
                clOsalTaskDelay(timeOut);
                goto retry;
            }
            clLogMultiline(CL_LOG_CRITICAL,
                           CPM_LOG_AREA_CPM,
                           CPM_LOG_CTX_CPM_BOOT,
                           "Self registration of CPM/G "
                           "active node [%s] failed -- \n"
                           "This indicates that there is a serious "
                           "problem in configuration of ASP. \n"
                           "Please check that : \n"
                           "%s%s",
                           gpClCpm->pCpmLocalInfo->nodeName,
                           xportConfigErrMsg,
                           gmsUniquePortErrMsg);
            goto failure;
        }
        else
        {
            ClRcT rc1 = CL_OK;
            static ClUint32T retries;
            ClTimerTimeOutT timeOut = {2, 0};

            /* Will fail if you accidently are running 2 of the same nodes, or
               if this cluster can "hear" another cluster's traffic.
               Will also fail if we suddenly switched roles. to active.
               in which case we retry
            */
            clOsalMutexLock(&gpClCpm->clusterMutex);
            if(!gpClCpm->polling)
            {
                clOsalMutexUnlock(&gpClCpm->clusterMutex);
                return;
            }
            if(CL_CPM_IS_ACTIVE())
            {
                clOsalMutexUnlock(&gpClCpm->clusterMutex);
                if(retries++ < 5)
                    goto retry;
                clLogCritical("CPM", "BOOT", "CPM registration failed with [%#x] "
                              "There seems to be too many cluster state changes or reconfigurations "
                              "resulting in cluster instability. Hence shutting down the node [%s]", 
                              rc, gpClCpm->pCpmLocalInfo->nodeName);
                goto failure;
            }
            clOsalMutexUnlock(&gpClCpm->clusterMutex);

            retries = 0;

            do 
            {
                clOsalMutexLock(&gpClCpm->clusterMutex);
                leaderNode = gpClCpm->activeMasterNodeId;
                clOsalMutexUnlock(&gpClCpm->clusterMutex);
                if(leaderNode && leaderNode != CL_GMS_INVALID_NODE_ID)
                {
                    rc1 = clCpmCpmLocalRegister(gpClCpm->pCpmLocalInfo);
                }
                else
                {
                    if(!(retries & 15))
                        clLogWarning("CPM", "BOOT", "AMF leader not present in the cluster. Deferring registration to master");
                    rc1 = CL_CPM_RC(CL_ERR_TRY_AGAIN);
                }
                if ((CL_GET_ERROR_CODE(rc1) == CL_ERR_DOESNT_EXIST) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_ERR_TIMEOUT) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_ERR_TRY_AGAIN) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_IOC_ERR_COMP_UNREACHABLE) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_IOC_ERR_HOST_UNREACHABLE))
                {
                    if (!(retries++ & 15))
                    {
                        ClRcT rc2;
                        ClIocNodeAddressT iocAddress;
                        ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0 };
                        clLogWarning(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_CPM,"AMF standby/Worker blade waiting for AMF active to come up...");
                        rc2 = clCpmMasterAddressGetExtended(&iocAddress, 3, &delay);
                        if (rc2 == CL_OK)
                        {
                            clLogInfo(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_CPM,"AMF active is [%d]", iocAddress);
                            clOsalMutexLock(&gpClCpm->clusterMutex);
                            gpClCpm->activeMasterNodeId = iocAddress;
                            clOsalMutexUnlock(&gpClCpm->clusterMutex);
                        }
                        
                    }
                    clOsalTaskDelay(timeOut);
                }
                else if (CL_OK != rc1)
                {
                    /*
                     * Could be a dynamic node waiting for master to be up.
                     * Try checking with the master to see if its in its AMS db
                     * and retry again if successful.
                     */
                    if(CL_GET_ERROR_CODE(rc1) == CL_ERR_NOT_EXIST && 
                       ++tries >= 1)
                    {
                        ClCpmNodeConfigT nodeConfig = {{0}};
                        rc1 = clCpmNodeConfigGet(gpClCpm->pCpmLocalInfo->nodeName, &nodeConfig);
                        if(rc1 == CL_OK)
                            continue;
                    }

                    if(CL_GET_ERROR_CODE(rc1) == CL_ERR_INVALID_STATE)
                        goto invalid_state;

                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM, "CPM/G standby/Worker blade registration with the CPM/G active failed, error [%#x]", rc1);
                    goto failure;
                }
                else
                {
                    gpClCpm->registeredCPML = 1;
                    break;
                }
            } while (gpClCpm->polling);

            retries = 0;
        }
    }
    else if (CL_OK != rc)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE)
        {
            static ClTimerTimeOutT delay = { .tsSec = 3, .tsMilliSec = 0 };
            invalid_state:
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                          "This node failed to register with master because of invalid state. "
                          "Scheduling a restart recovery to recover from an inconsistent cluster state "
                          "mostly caused by flipping controllers or inconsistent xport notifications");
            cpmRestart(&delay, "registration");
            /*
             * Unreached.
             */
        }
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                      "CPM/G standby/Worker blade registration "
                      "with the CPM/G active failed, error [%#x]",
                      rc);
        goto failure;
    }
    else
    {
        gpClCpm->registeredCPML = 1;
    }

    /*
     * Defer the initialization of AMS mgmt. interface(debug cli/entity trigger)
     * dependent on the master after the registration which confirms
     * that the master is up.
     */
    if(!reregister && CL_CPM_IS_STANDBY())
    {
        clAmsInitializeMgmtInterface();
    }

    cpmBmRespTimerStart();

    cpmInstallNodeInfo();

    return;
    
    failure:
    cpmSelfShutDown();
}

void cpmRegisterWithActive(void)
{
    __cpmRegisterWithActive(CL_FALSE);
}

void cpmSwitchoverActive(void)
{
    if(CL_CPM_IS_STANDBY())
    {
        cpmKillAllUserComponents(); /* FIXME: this could maybe exchange checkpoints with new active */
    }
    __cpmRegisterWithActive(CL_TRUE);
}

ClRcT VDECL(cpmGoBackToRegisterCallback)(ClEoDataT data,
                                         ClBufferHandleT inMsgHandle,
                                         ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmBmSetLevelResponseT bmSetLevelResp = {{0}};

    rc = VDECL_VER(clXdrUnmarshallClCpmBmSetLevelResponseT, 4, 0, 0)(inMsgHandle,
                                                 (void *)&bmSetLevelResp);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_CTX_CPM_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Failed to unmarshall buffer, error [%#x]",
                   rc);
        goto failure;
    }

    if (CL_OK != bmSetLevelResp.retCode)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                      "Failed to set boot level to [2], error [%#x] "
                      "Shutting down...",
                      rc);
        goto failure;
    }

    CL_ASSERT(CL_CPM_IS_WB());
    CL_ASSERT((strncmp(bmSetLevelResp.nodeName.value,
                      gpClCpm->pCpmLocalInfo->nodeName,
                       CL_MAX_NAME_LENGTH-1) == 0));

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
               "OK. Boot level is now [%d], re-registering with "
               "CPM/G active...",
               gpClCpm->bmTable->currentBootLevel);

    cpmRegisterWithActive();

    return CL_OK;
    
failure:
    cpmSelfShutDown();
    return rc;
}

static ClRcT cpmKillUserComp(ClCntNodeHandleT key,
                             ClCntDataHandleT data,
                             ClCntArgHandleT arg,
                             ClUint32T size)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)data;
    ClNameT compName = {0};

    strncpy(compName.value,
            comp->compConfig->compName,
            CL_MAX_NAME_LENGTH-1);
    compName.length = strlen(compName.value);
    
    if (!cpmIsInfrastructureComponent(&compName))
    {
        _cpmComponentCleanup(comp->compConfig->compName,
                             NULL,
                             gpClCpm->pCpmLocalInfo->nodeName,
                             NULL,
                             0,
                             CL_CPM_CLEANUP);
    }
    
    return CL_OK;
}

static void cpmKillUserComps(void)
{
    clCntWalk(gpClCpm->compTable, cpmKillUserComp, NULL, 0);
}

static ClRcT cpmPayloadRegisterCallback(void *unused)
{
    cpmRegisterWithActive();
    return CL_OK;
}

static ClRcT cpmPayloadRegister(void)
{
    ClTimerHandleT timer = {0};
    ClTimerTimeOutT timeout = {.tsSec = 1, .tsMilliSec = 0};
    cpmBmRespTimerStop();
    return clTimerCreateAndStart(timeout, CL_TIMER_VOLATILE, CL_TIMER_SEPARATE_CONTEXT,
                                 cpmPayloadRegisterCallback, NULL, &timer);
}

void cpmGoBackToRegister(void)
{
    ClNameT nodeName = {0};
    ClCpmLcmReplyT srcInfo = {0};
    ClRcT rc = CL_OK;
    
    if(gClAmsPayloadResetDisable)
    {
        rc = cpmPayloadRegister();
        if(rc == CL_OK)
            return;
    }
    
    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
               "Killing all user application components...");
    
    cpmKillUserComps();

    strncpy(nodeName.value,
            gpClCpm->pCpmLocalInfo->nodeName,
            CL_MAX_NAME_LENGTH-1);
    nodeName.length = strlen(nodeName.value);

    srcInfo.srcIocAddress = clIocLocalAddressGet();
    srcInfo.srcPort = CL_IOC_CPM_PORT;
    srcInfo.rmdNumber = CPM_NODE_GO_BACK_TO_REG;
    
    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
               "Setting boot level to 2...");

    cpmBmRespTimerStop();
    
    rc = clCpmBootLevelSet(&nodeName,
                           &srcInfo,
                           2);
    if (CL_OK != rc)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                      "Failed to set boot level to [2], error [%#x] "
                      "Shutting down...",
                      rc);
        cpmSelfShutDown();
    }
    else
    {
        /*
         *Reset to republish this nodes arrival
         */
        gpClCpm->nodeEventPublished = 0; 
        cpmWriteNodeStatToFile("CPM", CL_NO);
    }
}

ClRcT VDECL(cpmNodeConfigSet)(ClEoDataT data,
                              ClBufferHandleT inMsgHdl,
                              ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmNodeConfigT nodeConfig ;
    ClCpmLT *cpmL = NULL;
    ClBoolT cpmg = CL_FALSE;

    memset(&nodeConfig, 0, sizeof(nodeConfig));
    rc = VDECL_VER(clXdrUnmarshallClCpmNodeConfigT, 4, 0, 0)(inMsgHdl, &nodeConfig);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node set config unmarshall returned [%#x]", rc);
        goto out;
    }
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(nodeConfig.nodeName, &cpmL);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("NODE", "CONFIG", "Node [%s] find returned [%#x]", 
                   nodeConfig.nodeName, rc);
        goto out;
    }
    if(!strcmp(nodeConfig.cpmType, "GLOBAL"))
    {
        cpmg = CL_TRUE;
    }

    clNameCopy(&cpmL->nodeType, &nodeConfig.nodeType);
    clNameCopy(&cpmL->nodeIdentifier, &nodeConfig.nodeIdentifier);
    clNameCopy(&cpmL->nodeMoIdStr, &nodeConfig.nodeMoIdStr);
    if(cpmg)
        strncpy(cpmL->classType, "CL_AMS_NODE_CLASS_A", sizeof(cpmL->classType)-1);
    else
        strncpy(cpmL->classType, "CL_AMS_NODE_CLASS_C", sizeof(cpmL->classType)-1);
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    out:
    return rc;
}
                       
ClRcT VDECL(cpmNodeConfigGet)(ClEoDataT data,
                              ClBufferHandleT inMsgHdl,
                              ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClNameT nodeName = {0};
    ClCpmLT *cpmL = NULL;
    ClCpmNodeConfigT nodeConfig = {{0}};

    if(!CL_CPM_IS_ACTIVE())
        return CL_RMD_ERR_CONTINUE;

    rc = clXdrUnmarshallClNameT(inMsgHdl, &nodeName);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node name unmarshall returned [%#x]", rc);
        goto out;
    }
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(nodeName.value, &cpmL);
    if(rc != CL_OK)
    {
        ClBoolT unlock = CL_FALSE;
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        if(gpClCpm->cpmToAmsCallback &&
           gpClCpm->cpmToAmsCallback->nodeAdd)
        {
            rc = gpClCpm->cpmToAmsCallback->nodeAdd(nodeName.value);
            if(rc == CL_OK)
            {
                unlock = CL_TRUE;
                clOsalMutexLock(gpClCpm->cpmTableMutex);
                rc = cpmNodeFindLocked(nodeName.value, &cpmL);
            }
        }
        if(rc != CL_OK)
        {
            if(unlock)
                clOsalMutexUnlock(gpClCpm->cpmTableMutex);

            goto out;
        }
    }
    strncpy(nodeConfig.nodeName, nodeName.value, sizeof(nodeConfig.nodeName)-1);
    clNameCopy(&nodeConfig.nodeType, &cpmL->nodeType);
    clNameCopy(&nodeConfig.nodeIdentifier, &cpmL->nodeIdentifier);
    clNameCopy(&nodeConfig.nodeMoIdStr, &cpmL->nodeMoIdStr);
    if(!strcmp(cpmL->classType, "CL_AMS_NODE_CLASS_A")
       ||
       !strcmp(cpmL->classType, "CL_AMS_NODE_CLASS_B"))
    {
        strncpy(nodeConfig.cpmType, "GLOBAL", sizeof(nodeConfig.cpmType)-1);
    }
    else
    {
        strncpy(nodeConfig.cpmType, "LOCAL", sizeof(nodeConfig.cpmType)-1);
    }
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    rc = VDECL_VER(clXdrMarshallClCpmNodeConfigT, 4, 0, 0)(&nodeConfig, outMsgHdl, 0);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node [%s] marshall returned [%#x]",
                   nodeConfig.nodeName, rc);
        goto out;
    }
    
    out:
    return rc;
}

ClRcT VDECL(cpmMiddlewareRestart)(ClEoDataT data,
                                  ClBufferHandleT inMsgHdl,
                                  ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmMiddlewareResetT cpmMiddlewareReset = {0};
    ClIocNodeAddressT iocNodeAddress = 0;
    ClBoolT graceful = CL_FALSE;
    ClBoolT nodeReset = CL_FALSE;
    ClCpmLT *cpm = NULL;
    ClNameT nodeName = {0};
    ClRmdResponseContextHandleT responseHandle = 0;

    if (!CL_CPM_IS_ACTIVE())
        return CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);

    rc = VDECL_VER(clXdrUnmarshallClCpmMiddlewareResetT, 4, 0, 0)(inMsgHdl,
                                                                  &cpmMiddlewareReset);
    if (rc != CL_OK)
        goto out;

    iocNodeAddress = cpmMiddlewareReset.iocNodeAddress;
    graceful = cpmMiddlewareReset.graceful;
    nodeReset = cpmMiddlewareReset.nodeReset;

    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindByIocAddress(iocNodeAddress, &cpm);
    if (rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        goto out;
    }

    strcpy(nodeName.value, cpm->pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value) + 1;
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    /*
     * Fake a deferred sync response so that we do respond before processing the restart.
     * Because even if we make the node restart callback async through a taskpool, we still
     * have very very little windows where we could miss sending the response incase the
     * node restart beats us in sending the response whilst we are preempted.
     */
    rc = clRmdResponseDefer(&responseHandle);
    if(rc != CL_OK)
    {
        clLogError("NODE", "RESTART", "RMD response defer failed with error [%#x]", rc);
        return rc;
    }
    rc = clRmdSyncResponseSend(responseHandle, 0, CL_OK);
    if(rc != CL_OK)
    {
        clLogError("NODE", "RESTART", "RMD response send returned [%#x]", rc);
    }

    cpmEnqueueAspRequest(&nodeName, iocNodeAddress, 
                         nodeReset ? CL_CPM_RESTART_NODE : CL_CPM_RESTART_ASP);

    clLogNotice("NODE", "RESTART", "Processing middleware restart request for node [%d]", iocNodeAddress);

    rc = gpClCpm->cpmToAmsCallback->nodeRestart(&nodeName, graceful);
    if(rc != CL_OK)
        cpmDequeueAspRequest(&nodeName, NULL);

out:
    return rc;
}

ClRcT VDECL(cpmNodeRestart)(ClEoDataT data,
                            ClBufferHandleT inMsgHdl,
                            ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmRestartSendT cpmRestartSend = {0};
    ClIocNodeAddressT iocNodeAddress = 0;
    ClBoolT graceful = CL_FALSE;
    ClCpmLT *cpm = NULL;
    ClNameT nodeName = {0};
    ClRmdResponseContextHandleT responseHandle = 0;

    if (!CL_CPM_IS_ACTIVE())
        return CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);

    rc = VDECL_VER(clXdrUnmarshallClCpmRestartSendT, 4, 0, 0)(inMsgHdl,
                                                              &cpmRestartSend);
    if (rc != CL_OK)
        goto out;

    iocNodeAddress = cpmRestartSend.iocNodeAddress;
    graceful = cpmRestartSend.graceful;

    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindByIocAddress(iocNodeAddress, &cpm);
    if (rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        goto out;
    }

    strcpy(nodeName.value, cpm->pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value) + 1;
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    /*
     * Fake a deferred sync response so that we do respond before processing the restart.
     * Because even if we make the node restart callback async through a taskpool, we still
     * have very very little windows where we could miss sending the response incase the
     * node restart beats us in sending the response whilst we are preempted.
     */
    rc = clRmdResponseDefer(&responseHandle);
    if(rc != CL_OK)
    {
        clLogError("NODE", "RESTART", "RMD response defer failed with error [%#x]", rc);
        return rc;
    }
    rc = clRmdSyncResponseSend(responseHandle, 0, CL_OK);
    if(rc != CL_OK)
    {
        clLogError("NODE", "RESTART", "RMD response send returned [%#x]", rc);
    }

    clLogNotice("NODE", "RESTART", "Processing restart request for node [%d]", iocNodeAddress);

    return gpClCpm->cpmToAmsCallback->nodeRestart(&nodeName, graceful);

out:
    return rc;
}
