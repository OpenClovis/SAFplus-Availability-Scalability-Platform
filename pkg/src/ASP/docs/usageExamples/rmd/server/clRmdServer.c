/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

static ClRcT  clRmdServiceExposedByServer(ClEoDataT data, ClBufferHandleT  inMsgHandle,
        ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;

    ClUint32T inMsgLength = 0;
    ClUint8T *pInData = NULL;

    rc = clBufferLengthGet(inMsgHandle, &inMsgLength);
    if (CL_OK != rc)
    {
        clOsalPrintf("clBufferLengthGet() failed, rc[%#X]", rc);
        goto failure;
    }

    rc = clBufferFlatten(inMsgHandle, &pInData); // alternatively clBufferNBytesRead() can be used with an allocated memory
    if (CL_OK != rc)
    {
        clOsalPrintf("clBufferFlatten() failed, rc[%#X]", rc);
        goto failure;
    }

    /*
     * The service code will recide here. When the processing is 
     * complete the application(server) may want to populate the
     * outMsgHandle as desired.
     */
    clOsalPrintf("\nRmd Request being processed...\n\n");
    
    rc = clBufferNBytesWrite(outMsgHandle, pInData, inMsgLength);
    {
        clOsalPrintf("clBufferNBytesWrite() failed, rc[%#X]", rc);
        goto buffer_flatten;
    }

buffer_flatten:
    clHeapFree(pInData); // Free the buffer from clBufferFlatten()

failure:
    return rc;
}

static ClEoPayloadWithReplyCallbackT serverFuncList[] = {
    NULL,
    clRmdServiceExposedByServer,    /*  Service ID   0x1 */
    /*
     * List of all the services exposed by the server
     * which can be referenced using the index in the
     * service table - serverFuncList[].
     */
};

/*
 *   The RMD specific code ends here.
 */



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

    clCpmResponse(gCpmHandle, invocation, CL_OK);
 
    return CL_OK;
}

static ClRcT clRmdServerInit(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEoObj = NULL;

    rc = clEoMyEoObjectGet(&pEoObj);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEoMyEoObjectGet() failed, rc[%#X]\n");
        goto failure;
    }

    rc = clEoClientInstall(pEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,
                           serverFuncList, 0,
                           CL_SIZEOF_ARRAY(serverFuncList));
    if(CL_OK != rc)
    {
        clOsalPrintf("clEoClientInstall() failed, rc[%#X]\n");
        goto failure;
    }

failure:
    return rc;
}

static ClRcT appInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT version;
    ClIocPortT iocPort;

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

    clOsalPrintf("inside function %s\n", __FUNCTION__);

    clRmdServerInit();

    return rc;
}

static ClRcT clRmdServerExit(void)
{
    ClEoExecutionObjT *eoObj;
    ClRcT rc = CL_OK;

    clOsalPrintf("inside function %s\n", __FUNCTION__);
    rc = clEoMyEoObjectGet(&eoObj);
    if (CL_OK != rc)
    {
        return rc;
    }

    clEoClientUninstall(eoObj, CL_EO_USER_CLIENT_ID_START);
    clOsalPrintf("exiting function %s\n", __FUNCTION__);
    return CL_OK;
}

static ClRcT appFinalize(void)
{
    clRmdServerExit();
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
    "RMD_SERVER",          /* EO Name */
    1,                          /* Thread Priority */
    2,                          /* No of EO threads */
    0x3100,                     /* ReqIocPort */
    CL_EO_USER_CLIENT_ID_START + 3,
    CL_EO_USE_THREAD_FOR_RECV,  /* whether to use main thread for eo Recv or
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
