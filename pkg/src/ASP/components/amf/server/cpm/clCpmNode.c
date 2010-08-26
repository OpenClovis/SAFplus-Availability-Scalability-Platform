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
        retCode =
            clBufferNBytesRead(outMsgHdl, (ClUint8T *) pOutBuf,
                                      &msgLength);
        retCode = clBufferDelete(&outMsgHdl);
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

    if ((nodeAddress != clIocLocalAddressGet()) &&
        (gpClCpm->bmTable->currentBootLevel < cpmLocalInfo.defaultBootLevel))
    {
        clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                    "CPM/G active is returning try again for "
                    "registration request by node [%d], "
                    "because it is not ready...",
                    nodeAddress);
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
    rc = cpmNodeFind(cpmLocalInfo.nodeName, &cpmL);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, "node",
                   cpmLocalInfo.nodeName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    cpmL->pCpmLocalInfo =
        (ClCpmLocalInfoT *) clHeapAllocate(sizeof(ClCpmLocalInfoT));
    if (cpmL->pCpmLocalInfo == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    memcpy(cpmL->pCpmLocalInfo, &cpmLocalInfo, sizeof(ClCpmLocalInfoT));
    
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
        cpmL->pCpmLocalInfo->status = CL_CPM_EO_ALIVE;

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

    /*
     * Param Check 
     */
    rc = VDECL_VER(clXdrUnmarshallClCpmLocalInfoT, 4, 0, 0)(inMsgHandle, (void *) &cpmLocalInfo);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);


    /*
     * Locate the Node from node Table and assign the pointer to the data 
     */
    rc = cpmNodeFind(cpmLocalInfo.nodeName, &cpmL);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, "node",
                   cpmLocalInfo.nodeName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    /*
     * Bug 4424:
     * Added NULL check.
     */
    if (cpmL->pCpmLocalInfo != NULL)
    {
        clHeapFree(cpmL->pCpmLocalInfo);
        cpmL->pCpmLocalInfo = NULL;
    }

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
            ClTimerTimeOutT delay = {.tsSec = 5, .tsMilliSec = 0 };

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



static void cpmNodeDepartureEventPublish(ClIocNodeAddressT node)
{
    ClRcT rc;
    ClNameT nodeName;

    if(node != clIocLocalAddressGet())
        goto out;

    memcpy(&nodeName.value, &gpClCpm->pCpmLocalInfo->nodeName, CL_MAX_NAME_LENGTH);
    nodeName.length = (strlen(nodeName.value) > CL_MAX_NAME_LENGTH)? CL_MAX_NAME_LENGTH :strlen(nodeName.value);

    rc = nodeArrivalDeparturePublish(node, nodeName, CL_CPM_NODE_DEPARTURE);
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
    }

    cpmNodeDepartureEventPublish(iocAddress);
    
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
        rc = cpmNodeFindByIocAddress(iocAddress, &cpmL);
        if(rc == CL_OK)
        {
            strcpy(nodeName.value, cpmL->pCpmLocalInfo->nodeName);
            nodeName.length = strlen(nodeName.value) + 1;

            if(gpClCpm->cpmToAmsCallback != NULL &&  
                    gpClCpm->cpmToAmsCallback->nodeLeave != NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Calling AMS node leave..."));
                rc = gpClCpm->cpmToAmsCallback->nodeLeave(&nodeName, 
                                                          CL_CPM_NODE_LEAVING);
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
            rc = cpmNodeFind(cpmBmResponse.nodeName.value, &tempNode);
            if(rc == CL_OK && tempNode != NULL && tempNode->pCpmLocalInfo != NULL)
                clCpmNodeShutDown(
                        tempNode->pCpmLocalInfo->cpmAddress.nodeAddress);
        }
    }
    return rc;

failure:
    return rc;
}

void cpmCmRequestDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    
}

unsigned int cpmCmRequestHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT)key % CL_CPM_CPML_TABLE_BUCKET_SIZE);
}

int cpmCmRequestCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

ClRcT cpmCmRequestDSFinalize(void)
{
    ClRcT   rc = CL_OK;
    
    clCntDelete(gpClCpm->cmRequestTable);
    
    clOsalMutexDelete(gpClCpm->cmRequestMutex);

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

ClRcT cpmEnqueueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest)
{
    
    ClCpmCmQueuedataT *queueData = NULL;
    ClUint16T nodeKey = 0;
    ClRcT rc = CL_OK;
    
    rc = clOsalMutexLock(gpClCpm->cmRequestMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, 
            CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc,
            rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    queueData = (ClCpmCmQueuedataT *)clHeapAllocate(sizeof(ClCpmCmQueuedataT));
    if(queueData == NULL)
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                CL_LOG_HANDLE_APP);
    memcpy(&(queueData->cmMsg),  pRequest, sizeof(queueData->cmMsg));
    memcpy(&(queueData->nodeName), pNodeName, sizeof(queueData->nodeName));

    rc = clCksm16bitCompute((ClUint8T *) pNodeName->value,
            strlen(pNodeName->value), &nodeKey);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_CKSM_ERR,
            pNodeName->value, rc, rc, CL_LOG_DEBUG,
            CL_LOG_HANDLE_APP);

    rc = clCntNodeAdd(gpClCpm->cmRequestTable,
            (ClCntKeyHandleT)(ClWordT)nodeKey,
            (ClCntDataHandleT) queueData, NULL);

    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    return rc;
    
failure:
    if(queueData != NULL)
        clHeapFree(queueData);
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
    return rc;
}

ClRcT cpmDequeueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT hNode = 0;
    ClUint16T nodeKey = 0;
    ClCpmCmQueuedataT *tempRequest = NULL;
    ClUint32T numNode = 0;

    rc = clOsalMutexLock(gpClCpm->cmRequestMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
            CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    
    rc = clCksm16bitCompute((ClUint8T *) pNodeName->value, 
            strlen(pNodeName->value), &nodeKey);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_CKSM_ERR, 
            pNodeName->value, rc, rc,
            CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    
    rc = clCntNodeFind(gpClCpm->cmRequestTable, (ClCntKeyHandleT)(ClWordT)nodeKey, 
            &hNode);
    CL_CPM_CHECK_3(CL_DEBUG_WARN, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, 
            "CM Request", pNodeName->value, rc, rc, CL_LOG_DEBUG, 
            CL_LOG_HANDLE_APP);
    
    rc = clCntKeySizeGet(gpClCpm->cmRequestTable, (ClCntKeyHandleT)(ClWordT)nodeKey,
                         &numNode);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_KEY_SIZE_GET_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    while (numNode > 0)
    {

        rc = clCntNodeUserDataGet(gpClCpm->cmRequestTable, hNode,
                          (ClCntDataHandleT *) &tempRequest);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR,
                       rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        if (!strcmp(tempRequest->nodeName.value, pNodeName->value))
        {
            memcpy(pRequest , &(tempRequest->cmMsg), sizeof(*pRequest));
            clCntNodeDelete(gpClCpm->cmRequestTable, hNode);
            clOsalMutexUnlock(gpClCpm->cmRequestMutex);
            clHeapFree(tempRequest);
            return CL_OK;
        }
        if(--numNode)
        {
            rc = clCntNextNodeGet(gpClCpm->cmRequestTable,hNode,&hNode);
            if(rc != CL_OK)
            {
                break;
            }
        }
    }
    rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
failure:
    clOsalMutexUnlock(gpClCpm->cmRequestMutex);
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
    ClCpmCmMsgT cpmResponse;

    memset(&cmCpmMsg, 0, sizeof(ClCmCpmMsgT));
    memset(&nodeName, 0, sizeof(ClNameT));
    memset(&cpmResponse, 0, sizeof(ClCpmCmMsgT));

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

    clLogMultiline(CL_LOG_INFO, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
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
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                   "Got blade insertion request from CM, responding "
                   "with allow user action...");

        memcpy(&(cpmResponse.cmCpmMsg), &cmCpmMsg, sizeof(ClCmCpmMsgT));
        cpmResponse.cpmCmMsgType = CL_CPM_ALLOW_USER_ACTION;
        
        rc = clCmCpmResponseHandle(&cpmResponse);
        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                       "Failed while responding to CM, error [%#x]",
                       rc);
            goto failure;
        }
    }
    else
    {
        ClCpmCpmToAmsCallT *cpmToAmsCallback = gpClCpm->cpmToAmsCallback;
        
        rc = cpmNodeFindByIocAddress(cmCpmMsg.physicalSlot, &cpmL);
        if (rc == CL_OK)
        {
            strcpy(nodeName.value, cpmL->pCpmLocalInfo->nodeName);
            nodeName.length = strlen(nodeName.value) + 1;
            
            switch(cmCpmMsg.cmCpmMsgType)
            {
                case CL_CM_BLADE_REQ_EXTRACTION:
                case CL_CM_BLADE_NODE_ERROR_REPORT:
                {
                    cpmEnqueueCmRequest(&nodeName, &cmCpmMsg);
                    rc = cpmToAmsCallback->nodeLeave(&nodeName, 
                                                     CL_CPM_NODE_LEAVING);
                    if (CL_OK != rc)
                    {
                        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                   "AMS node leave[leaving] failed, "
                                   "error [%#x]",
                                   rc);
                        goto failure;
                    }
                }
                break;
                case CL_CM_BLADE_SURPRISE_EXTRACTION:
                {
                    cpmEnqueueCmRequest(&nodeName, &cmCpmMsg);
                    rc = cpmToAmsCallback->nodeLeave(&nodeName, 
                                                     CL_CPM_NODE_LEFT);
                    if (CL_OK != rc)
                    {
                        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                   "AMS node leave[left] failed, "
                                   "error [%#x]",
                                   rc);
                        goto failure;
                    }
                }
                break;
                case CL_CM_BLADE_NODE_ERROR_CLEAR:
                {
                    rc = cpmToAmsCallback->nodeJoin(&nodeName);
                    if (CL_OK != rc)
                    {
                        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CM,
                                   "AMS node join failed, error [%#x]", rc);
                        goto failure;
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

    rc = cpmNodeFind(nodeName->value, &node);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "node", nodeName->value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

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
        return CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }

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
    
    rc = cpmNodeFindByNodeId(nodeAddress, &node);
    if(rc != CL_OK)
    {
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
        return CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    }

failure:
    return rc;
}

ClRcT cpmActiveInitiatedSwitchOver(void)
{
    ClRcT rc = CL_OK;
    
    /*
     * Bug 4094:
     * Do the cluster leave only if there is a
     * CPM/G standby in the cluster.
     */
    if (gpClCpm->deputyNodeId != -1)
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
        clGmsClusterLeave(gpClCpm->cpmGmsHdl, 10000000000LL, 
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

    rc = cpmNodeFind(nodeName.value, &node);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "node", nodeName.value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    /*
     * Return physical address 
     */
    if (node->pCpmLocalInfo != NULL)
    {
        idlIocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
        memcpy(&(idlIocAddress.clIocAddressIDLT.iocPhyAddress), &(node->pCpmLocalInfo->cpmAddress), 
                sizeof(ClIocPhysicalAddressT));
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Node not yet registered.\n"), rc);
    }

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

ClRcT cpmFailoverNode(ClGmsNodeIdT nodeId)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpmL = NULL;
    ClCpmLocalInfoT *pCpmLocalInfo = NULL;
    ClNameT nodeName = {0};
    ClUint32T slotNumber = 0;
    ClIocNodeAddressT nodeAddress = 0;

    clOsalMutexLock(&gpClCpm->cpmMutex);

    rc = cpmNodeFindByNodeId(nodeId, &cpmL);
    if (CL_OK != rc)
    {
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
        clOsalMutexUnlock(&gpClCpm->cpmMutex);
        return CL_OK;
    }

    slotNumber = pCpmLocalInfo->slotNumber;
    nodeAddress = pCpmLocalInfo->cpmAddress.nodeAddress;
    cpmL->pCpmLocalInfo = NULL;

    clOsalMutexUnlock(&gpClCpm->cpmMutex);


    clLogCritical(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                  "Got a node shutdown/failure for standby/worker blade, "
                  "Failing over node [%s] with node ID [%#x]",    
                  pCpmLocalInfo->nodeName,
                  nodeId);

    clLogDebug(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
               "Deregistering IOC TL information for node [%d]...",
               pCpmLocalInfo->cpmAddress.nodeAddress);

#if 0
    clIocTransparencyDeregisterNode(pCpmLocalInfo->cpmAddress.nodeAddress);
#endif

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
               "Publishing node [%d] departure event...",
               pCpmLocalInfo->nodeId);

    cpmUpdateNodeState(pCpmLocalInfo);

    strcpy(nodeName.value, pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value) + 1;

    if ((NULL != gpClCpm->cpmToAmsCallback) &&  
        (NULL != gpClCpm->cpmToAmsCallback->nodeLeave))
    {
        rc = gpClCpm->cpmToAmsCallback->nodeLeave(&nodeName,
                                                  CL_CPM_NODE_LEFT);
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

void cpmRegisterWithActive(void)
{
    ClRcT rc = CL_OK;
    ClCharT tipcConfigErrMsg[] = "IOC/TIPC is configured properly. \n";
    ClCharT gmsUniquePortErrMsg[] = "The GMS port you are using is unique "
        "in the cluster or somebody else is not "
        "using the same GMS port as yours. \n";
    ClInt32T tries = 0;

    retry:
    rc = clCpmCpmLocalRegister(gpClCpm->pCpmLocalInfo);
    if ((CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST) ||
        (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT) ||
        (CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN) ||
        (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) ||
        (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE))
    {
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
                           tipcConfigErrMsg,
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
                rc1 = clCpmCpmLocalRegister(gpClCpm->pCpmLocalInfo);
                if ((CL_GET_ERROR_CODE(rc1) == CL_ERR_DOESNT_EXIST) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_ERR_TIMEOUT) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_ERR_TRY_AGAIN) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_IOC_ERR_COMP_UNREACHABLE) ||
                    (CL_GET_ERROR_CODE(rc1) == CL_IOC_ERR_HOST_UNREACHABLE))
                {
                    if (!(retries++ & 15))
                    {
                        clLogWarning(CPM_LOG_AREA_CPM,
                                     CPM_LOG_CTX_CPM_CPM,
                                     "CPM/G standby/Worker blade waiting for "
                                     "CPM/G active to come up...");
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
                        rc1 = clCpmNodeConfigGet(gpClCpm->pCpmLocalInfo->nodeName,
                                                &nodeConfig);
                        if(rc1 == CL_OK)
                            continue;
                    }
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                                  "CPM/G standby/Worker blade registration "
                                  "with the CPM/G active failed, error [%#x]",
                                  rc1);
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
    if(CL_CPM_IS_STANDBY())
    {
        clAmsInitializeMgmtInterface();
    }

    cpmBmRespTimerStart();
    cpmInstallNodeInfo();

    return;
    
    failure:
    cpmSelfShutDown();
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

void cpmGoBackToRegister(void)
{
    ClNameT nodeName = {0};
    ClCpmLcmReplyT srcInfo = {0};
    ClRcT rc = CL_OK;
    
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
    rc = cpmNodeFind(nodeConfig.nodeName, &cpmL);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node [%s] find returned [%#x]", 
                   nodeConfig.nodeName, rc);
        goto out;
    }
    if(!strcmp(nodeConfig.cpmType, "GLOBAL"))
    {
        cpmg = CL_TRUE;
    }

    clOsalMutexLock(gpClCpm->cpmTableMutex);
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
    rc = clXdrUnmarshallClNameT(inMsgHdl, &nodeName);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CONFIG", "Node name unmarshall returned [%#x]", rc);
        goto out;
    }
    rc = cpmNodeFind(nodeName.value, &cpmL);
    if(rc != CL_OK)
    {
        if(gpClCpm->cpmToAmsCallback &&
           gpClCpm->cpmToAmsCallback->nodeAdd)
        {
            rc = gpClCpm->cpmToAmsCallback->nodeAdd(nodeName.value);
            if(rc == CL_OK)
            {
                rc = cpmNodeFind(nodeName.value, &cpmL);
            }
        }
        if(rc != CL_OK)
            goto out;
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

ClRcT VDECL(cpmNodeRestart)(ClEoDataT data,
                            ClBufferHandleT inMsgHdl,
                            ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmRestartSendT cpmRestartSend;
    ClIocNodeAddressT iocNodeAddress = 0;
    ClBoolT graceful = CL_FALSE;
    ClCpmLT *cpm = NULL;
    ClNameT nodeName = {0};
    ClRmdResponseContextHandleT responseHandle = 0;

    memset(&cpmRestartSend, 0, sizeof(ClCpmRestartSendT));

    if (!CL_CPM_IS_ACTIVE())
        return CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);

    rc = VDECL_VER(clXdrUnmarshallClCpmRestartSendT, 4, 0, 0)(inMsgHdl,
                                                              &cpmRestartSend);
    if (rc != CL_OK)
        goto out;

    iocNodeAddress = cpmRestartSend.iocNodeAddress;
    graceful = cpmRestartSend.graceful;

    rc = cpmNodeFindByIocAddress(iocNodeAddress, &cpm);
    if (rc != CL_OK)
        goto out;

    strcpy(nodeName.value, cpm->pCpmLocalInfo->nodeName);
    nodeName.length = strlen(nodeName.value) + 1;

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

    clLogDebug("NODE", "RESTART", "Processing restart request for node [%d]", iocNodeAddress);

    return gpClCpm->cpmToAmsCallback->nodeRestart(&nodeName, graceful);

out:
    return rc;
}
