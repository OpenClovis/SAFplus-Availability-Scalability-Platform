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
 * ModuleName  : rmd usage example
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module describes how a typical Rmd Client would look like
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>

#include <clRmdApi.h>
#include <clCpmApi.h>
#include <clEoApi.h>

#include <string.h>

/*
 * From here starts the RMD specific code that can be moved as is.
 */

static ClRcT clRmdSyncWithoutReply(void)
{
    ClRcT rc = CL_OK;

    ClIocAddressT destAddr = {{0}};
    ClUint32T funcId = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x1); // Remote Procedure to be invoked

    ClBufferHandleT inMsgHandle = 0; 
    ClBufferHandleT outMsgHandle = 0;

    ClUint32T flags = CL_RMD_CALL_NON_PERSISTENT; 

    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;

    ClUint8T inArguments[] = "The bytestream of arguments to be passed to dest "
        "(marshalled if necessary to make endian neutral)";

    /*
     * Update the destination address 
     */
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet(); // If destination(server) is on the same node
    destAddr.iocPhyAddress.portId = 0x3100; // Well known destionation(server) port

    rc = clBufferCreate(&inMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferNBytesWrite(inMsgHandle,
            (ClUint8T *) &inArguments,
            sizeof(inArguments));
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferNBytesWrite() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clRmdWithMsg(destAddr, funcId, inMsgHandle,
            outMsgHandle, flags, &rmdOptions,
            NULL);
    if (rc != CL_OK)
    {
        clOsalPrintf("clRmdWithMsg() failed, rc[%#X]\n");
        goto failure;
    }

    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("***************** Sync RMD Succeeded ******************\n");
    clOsalPrintf("*******************************************************\n");

    /*
     * Nothing to free - inMsgHandle had to be freed here if CL_RMD_CALL_NON_PERSISTENT
     * wasn't set. But that is discouraged unless inMsgHandle needs to be re-used which
     * is rare.
     */

failure:
    return rc;
}

static ClRcT clRmdSyncWithReply(void)
{
    ClRcT rc = CL_OK;

    ClIocAddressT destAddr = {{0}};
    ClUint32T funcId = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x1); // Remote Procedure to be invoked

    ClBufferHandleT inMsgHandle = 0; 
    ClBufferHandleT outMsgHandle = 0;

    ClUint32T flags = CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_NON_PERSISTENT; 

    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;

    ClUint8T inArguments[] = "The bytestream of arguments to be passed to dest "
        "(marshalled if necessary to make endian neutral)";

    ClUint8T *pReturnData = NULL;

    /*
     * Update the destination address 
     */
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet(); // If destination(server) is on the same node
    destAddr.iocPhyAddress.portId = 0x3100; // Well known destionation(server) port

    rc = clBufferCreate(&inMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferCreate(&outMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferNBytesWrite(inMsgHandle,
            (ClUint8T *) &inArguments,
            sizeof(inArguments));
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferNBytesWrite() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clRmdWithMsg(destAddr, funcId, inMsgHandle,
            outMsgHandle, flags, &rmdOptions,
            NULL);
    if (rc != CL_OK)
    {
        clOsalPrintf("clRmdWithMsg() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferFlatten(outMsgHandle, &pReturnData);
    if (CL_OK != rc)
    {
        clOsalPrintf("clBufferMessageNBytesRead() Failed, rc[%#X]\n", rc);
        goto rmd_success;
    }

    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("***************** Sync RMD Succeeded ******************\n");
    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("  Data Returned : [%s]\n", pReturnData);
    clOsalPrintf("*******************************************************\n");

    clHeapFree(pReturnData);  //  Free the memory allocated by clBufferFlatten()

rmd_success:
    clBufferDelete(&outMsgHandle); // Free the out message buffer

    /*
     * inMsgHandle had to be freed here if CL_RMD_CALL_NON_PERSISTENT wasn't set. 
     * But that is discouraged unless inMsgHandle needs to be re-used which
     * is rare.
     */

failure:
    return rc;
}

static ClRcT clRmdAsyncWithoutReply(void)
{
    ClRcT rc = CL_OK;

    ClIocAddressT destAddr = {{0}};
    ClUint32T funcId = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x1); // Remote Procedure to be invoked

    ClBufferHandleT inMsgHandle = 0; 
    ClBufferHandleT outMsgHandle = 0;

    ClUint32T flags = CL_RMD_CALL_ASYNC | CL_RMD_CALL_NON_PERSISTENT; 

    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;

    ClUint8T inArguments[] = "The bytestream of arguments to be passed to dest "
        "(marshalled if necessary to make endian neutral)";

    /*
     * Update the destination address 
     */
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet(); // If destination(server) is on the same node
    destAddr.iocPhyAddress.portId = 0x3100; // Well known destionation(server) port

    rc = clBufferCreate(&inMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferNBytesWrite(inMsgHandle,
            (ClUint8T *) &inArguments,
            sizeof(inArguments));
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferNBytesWrite() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clRmdWithMsg(destAddr, funcId, inMsgHandle,
            outMsgHandle, flags, &rmdOptions,
            NULL);
    if (rc != CL_OK)
    {
        clOsalPrintf("clRmdWithMsg() failed, rc[%#X]\n");
        goto failure;
    }

    /*
     * inMsgHandle had to be freed here if CL_RMD_CALL_NON_PERSISTENT wasn't set. 
     * But that is discouraged unless inMsgHandle needs to be re-used which
     * is rare.
     */
failure:
    return rc;
}

void clRmdAsyncCallback(ClRcT rc, ClPtrT pCookie,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClCharT *pCallbackArg = pCookie;

    ClUint8T *pReturnData = NULL;

    if (CL_OK != rc) // If the Async RMD failed 
    {
        clOsalPrintf("Async RMD Failed, rc[%#X]\n", rc);
        goto failure;
    }

    rc = clBufferFlatten(outMsgHandle, &pReturnData);
    if (CL_OK != rc)
    {
        clOsalPrintf("clBufferMessageNBytesRead() Failed, rc[%#X]\n", rc);
        goto failure;
    }

    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("***************** Async RMD Succeeded *****************\n");
    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("  Data Returned : [%s]\n", pReturnData);
    clOsalPrintf("  Cookie Supplied :   [%s]\n", pCallbackArg);
    clOsalPrintf("*******************************************************\n");

    clHeapFree(pReturnData);  //  Free the memory allocated by clBufferFlatten()

failure:
    clHeapFree(pCallbackArg);  //  Free the memory allocated for cookie
    clBufferDelete(&outMsgHandle); // Free the out message buffer

    /*
     * inMsgHandle had to be freed here if CL_RMD_CALL_NON_PERSISTENT wasn't set. 
     * But that is discouraged unless inMsgHandle needs to be re-used which
     * is rare.
     */

    return;
}

static ClRcT clRmdAsyncWithReply(void)
{
    ClRcT rc = CL_OK;

    ClIocAddressT destAddr = {{0}};
    ClUint32T funcId = CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x1); // Remote Procedure to be invoked

    ClBufferHandleT inMsgHandle = 0; 
    ClBufferHandleT outMsgHandle = 0;

    ClUint32T flags = CL_RMD_CALL_ASYNC | CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_NON_PERSISTENT; 

    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClRmdAsyncOptionsT asyncRmdOptions = {0};

    ClCharT cookie[] = "User Specified Argument for the async callback";
    ClPtrT pCallbackArg = NULL;

    ClUint8T inArguments[] = "The bytestream of arguments to be passed to dest "
        "(marshalled if necessary to make endian neutral)";

    /*
     * Populate the argument to be passed to the Callback.
     */
    pCallbackArg = clHeapAllocate(sizeof(cookie));
    if (NULL == pCallbackArg)
    {
        clOsalPrintf("Failed to allocate Memory\n");
        goto failure;
    }
    memcpy(pCallbackArg, cookie, sizeof(cookie));

    /*
     * Populate the Async RMD options.
     */
    asyncRmdOptions.fpCallback = clRmdAsyncCallback;
    asyncRmdOptions.pCookie = pCallbackArg;

    /*
     * Update the destination address 
     */
    destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet(); // If destination(server) is on the same node
    destAddr.iocPhyAddress.portId = 0x3100; // Well known destionation(server) port

    rc = clBufferCreate(&inMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferCreate(&outMsgHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferCreate() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clBufferNBytesWrite(inMsgHandle,
            (ClUint8T *) &inArguments,
            sizeof(inArguments));
    if(CL_OK != rc)
    {
        clOsalPrintf("clBufferNBytesWrite() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clRmdWithMsg(destAddr, funcId, inMsgHandle,
            outMsgHandle, flags, &rmdOptions,
            &asyncRmdOptions);
    if (rc != CL_OK)
    {
        clOsalPrintf("clRmdWithMsg() failed, rc[%#X]\n");
        goto failure;
    }

failure:
    return rc;
}

/*
 *   The RMD specific code ends here.
 */

static ClUint32T shouldIUnblock = 0;

static ClRcT clRmdUsageIllustrate(void)
{
    clRmdSyncWithoutReply();
    clRmdSyncWithReply();

    clRmdAsyncWithoutReply();
    clRmdAsyncWithReply();

    return CL_OK;
}

ClCpmHandleT gCpmHandle;
ClRcT appTerminate(ClInvocationT invocation, const ClNameT *compName)
{
    ClRcT rc;

    clOsalPrintf("Inside appTerminate \n");

    clOsalPrintf("Unregister with CPM before Exit ................. %s\n",
           compName->value);
    rc = clCpmComponentUnregister(gCpmHandle, compName, NULL);
    clOsalPrintf("Finalize before Exit ................. %s\n", compName->value);
    rc = clCpmClientFinalize(gCpmHandle);
    clOsalPrintf("After Finalize ................. %s\n", compName->value);
    shouldIUnblock = 1;

    clCpmResponse(gCpmHandle, invocation, CL_OK);
 
    return CL_OK;
}


static ClRcT appInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT version;
    ClIocPortT iocPort;
    ClTimerTimeOutT timeOut = { 1, 0 };

    version.releaseCode = 'B';
    version.majorVersion = 01;
    version.minorVersion = 01;

    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = appTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    clEoMyEoIocPortGet(&iocPort);
    clOsalPrintf("Application Address 0x%x Port %x\n", clIocLocalAddressGet(),
           iocPort);
    rc = clCpmClientInitialize(&gCpmHandle, &callbacks, &version);
    clOsalPrintf("After clCpmClientInitialize %d\t %x\n", gCpmHandle, rc);
    rc = clCpmComponentNameGet(gCpmHandle, &appName);
    clOsalPrintf("After clCpmComponentNameGet %d\t %s\n", gCpmHandle, appName.value);
    rc = clCpmComponentRegister(gCpmHandle, &appName, NULL);
    clOsalPrintf("After clCpmClientRegister %x\n", rc);

    clRmdUsageIllustrate();

    while (!shouldIUnblock)
    {
        clOsalTaskDelay(timeOut);
    }

    return rc;
}

static ClRcT appFinalize(void)
{
    clOsalPrintf("inside function %s\n", __FUNCTION__);
    return CL_OK;
}

static ClRcT appStateChange(ClEoStateT eoState)
{
    clOsalPrintf("inside function %s\n", __FUNCTION__);
    return CL_OK;
}

static ClRcT appHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq = CL_EO_BUSY_POLL;
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;
}


ClEoConfigT clEoConfig = {
    "RMD_CLIENT",                    /* EO Name */
    1,                          /* Thread Priority */
    1,                          /* No of EO thread */
    0x3101,                     /* ReqIocPort */
    CL_EO_USER_CLIENT_ID_START + 3,
    CL_EO_USE_THREAD_FOR_APP,   /* whether to use main thread for eo Recv or
                                 * not */
    appInitialize,
    appFinalize,
    appStateChange,
    appHealthCheck
};

/*
 * What basic and client libraries do we need to use? 
 */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,                    /* osal */
    CL_TRUE,                    /* timer */
    CL_TRUE,                    /* buffer */
    CL_TRUE,                    /* ioc */
    CL_TRUE,                    /* rmd */
    CL_TRUE,                    /* eo */
    CL_FALSE,                   /* om */
    CL_FALSE,                   /* hal */
    CL_FALSE,                   /* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_FALSE,                   /* cor */
    CL_FALSE,                   /* cm */
    CL_FALSE,                   /* name */
    CL_FALSE,                   /* log */
    CL_FALSE,                   /* trace */
    CL_FALSE,                   /* diag */
    CL_FALSE,                   /* txn */
    CL_FALSE,                   /* hpi */
    CL_FALSE,                   /* cli */
    CL_FALSE,                   /* alarm */
    CL_FALSE,                   /* debug */
    CL_FALSE                    /* gms */
};
