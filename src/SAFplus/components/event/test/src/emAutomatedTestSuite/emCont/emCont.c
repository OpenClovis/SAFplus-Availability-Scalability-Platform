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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emCont/emCont.c $
 * $Author: bkpavan $
 * $Date: 2006/11/10 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include "clDebugApi.h"
#include "clCpmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include "clEventApi.h"
#include "clEventExtApi.h"
#include "../common/emAutoCommon.h"
#include "emCont.h"
#if 0
# include <ipi/clSAClientSelect.h>  /* SAF Changes */
#endif

ClRcT clEvtTestContResultGet(ClUint32T cData, ClBufferHandleT inMsg,
                             ClBufferHandleT outMsg)
{
    return CL_OK;
}
static ClCpmHandleT gClEvtTestContCpmHandle;

ClRcT clEvtCpmReplyCb(ClUint32T data, ClBufferHandleT inMsgHandle,
                      ClBufferHandleT outMsgHandle)
{
    ClUint32T rc = CL_OK;

#if 0
    ClCpmLcmResponseT response = { {0, 0}, 0 };
    ClUint32T msgLength = 0;
    ClCharT logmsg[CL_MAX_NAME_LENGTH] = { 0 };
    ClCharT requesttype[CL_MAX_NAME_LENGTH] = { 0 };

    CL_FUNC_ENTER();
    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if (msgLength == sizeof(ClCpmLcmResponseT))
    {
        rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &response,
                                       &msgLength);
        if (rc != CL_OK)
        {
            clOsalPrintf("Unable to read the message. \n");
            goto failure;
        }
    }
    else
    {
        clOsalPrintf("Buffer read failure !\n");
        goto failure;
    }

    printf("Received reply for %s...\n", response.name);

    switch (response.requestType)
    {
        case CL_CPM_HEALTHCHECK:
            strcpy(requesttype, "CL_CPM_HEALTHCHECK");
            break;
        case CL_CPM_TERMINATE:
            strcpy(requesttype, "CL_CPM_TERMINATE");
            break;
        case CL_CPM_PROXIED_INSTANTIATE:
            strcpy(requesttype, "CL_CPM_PROXIED_INSTANTIATE");
            break;
        case CL_CPM_PROXIED_CLEANUP:
            strcpy(requesttype, "CL_CPM_PROXIED_CLEANUP");
            break;
        case CL_CPM_EXTN_HEALTHCHECK:
            strcpy(requesttype, "CL_CPM_EXTN_HEALTHCHECK");
            break;
        case CL_CPM_INSTANTIATE:
            strcpy(requesttype, "CL_CPM_INSTANTIATE");
            break;
        case CL_CPM_CLEANUP:
            strcpy(requesttype, "CL_CPM_CLEANUP");
            break;
        case CL_CPM_RESTART:
            strcpy(requesttype, "CL_CPM_RESTART");
            break;
        default:
            strcpy(requesttype, "Invalid request");
            break;
    }

    sprintf(logmsg, "%s %s request %s.\n", response.name, requesttype,
            response.returnCode == CL_OK ? "success" : "failure");
    printf(logmsg);

    CL_FUNC_EXIT();
    return CL_OK;

  failure:
    CL_FUNC_EXIT();
#endif

    printf("Invoked the Callback for CPM API succesfully\n\r\n");

    return rc;
}

static ClEoPayloadWithReplyCallbackT gClEvtTestContFuncList[] = {

    (ClEoPayloadWithReplyCallbackT) NULL,   /* 0 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestContResultGet, /* 1 */
    (ClEoPayloadWithReplyCallbackT) clEvtCpmReplyCb,    /* 2 */
};


ClRcT clEventTerminate(ClInvocationT invocation, const ClNameT *compName)
{
    ClRcT rc;

    rc = clCpmClientFinalize(gClEvtTestContCpmHandle);

    clCpmResponse(gClEvtTestContCpmHandle, invocation, CL_OK);

    return CL_OK;
}

ClRcT clEvtCpmInit()
{
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT version;
    ClIocPortT iocPort;
    ClRcT rc = CL_OK;

/******************************************************************
                    CPM Related stuff
******************************************************************/
    /*
     * Do the CPM client init/Register 
     */
    version.releaseCode = 'B';
    version.majorVersion = 0x1;
    version.minorVersion = 0x1;

    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = clEventTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    clEoMyEoIocPortGet(&iocPort);
    rc = clCpmClientInitialize(&gClEvtTestContCpmHandle, &callbacks, &version);
    rc = clCpmComponentNameGet(gClEvtTestContCpmHandle, &appName);
    rc = clCpmComponentRegister(gClEvtTestContCpmHandle, &appName, NULL);

    return CL_OK;
}

ClInt32T clEvtContSubKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClEvtContSubKey *pKey1 = (ClEvtContSubKey *) key1;
    ClEvtContSubKey *pKey2 = (ClEvtContSubKey *) key2;
    ClInt32T result = 0;

    result = pKey1->filterNo - pKey2->filterNo;
    if (0 == result)
    {
        result =
            clEvtContUtilsNameCmp(&pKey1->channelName, &pKey2->channelName);
    }

    return result;
}

void clEvtContSubDataDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *) userKey);
    return;
}


ClInt32T clEvtContPubKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClEvtContPubKey *pKey1 = (ClEvtContPubKey *) key1;
    ClEvtContPubKey *pKey2 = (ClEvtContPubKey *) key2;
    ClInt32T result = 0;

    result = clEvtContUtilsNameCmp(&pKey1->appName, &pKey2->appName);
    if (0 == result)
    {
        result =
            clEvtContUtilsNameCmp(&pKey1->channelName, &pKey2->channelName);
        return result;
    }
    return result;
}

void clEvtContPubDataDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *) userKey);
    return;
}

ClEvtContSubInfoStorageT *gpSubStorage;
void clEvtContTestDbInit()
{
    ClRcT rc = CL_OK;
    ClUint32T noOfApp;

    rc = clCntLlistCreate(clEvtContSubKeyCompare, clEvtContSubDataDelete,
                          clEvtContSubDataDelete, CL_CNT_NON_UNIQUE_KEY,
                          &gEvtContSubInfo);
    if (CL_OK != rc)
    {
        clOsalPrintf("Creating linked list failed for sub \r\n");
        exit(-1);
    }

    rc = clCntLlistCreate(clEvtContPubKeyCompare, clEvtContPubDataDelete,
                          clEvtContPubDataDelete, CL_CNT_UNIQUE_KEY,
                          &gEvtContPubInfo);
    if (CL_OK != rc)
    {
        clOsalPrintf("Creating linked list failed for pub \r\n");
        exit(-1);
    }

    clEvtContGetApp(&noOfApp);
    clHeapAllocate(noOfApp * sizeof(ClEvtContSubInfoStorageT));

    return;
}

ClRcT clEvtTestContInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEoObj;

#if 0
    ClSvcHandleT svcHandle = { 0 };
    ClOsalTaskIdT taskId = 0;
#endif

    clEvtContTestDbInit();
    rc = clEoMyEoObjectGet(&pEoObj);
    rc = clEoClientInstall(pEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,
                           gClEvtTestContFuncList, (ClEoDataT) NULL,
                           (int) sizeof(gClEvtTestContFuncList) /
                           (int) sizeof(ClEoPayloadWithReplyCallbackT));

    rc = clEvtCpmInit();

#if 0
    svcHandle.cpmHandle = &gClEvtTestContCpmHandle;
    rc = clDispatchThreadCreate(pEoObj, &taskId, svcHandle);
#endif

    clEvtContAppAddressGet();

    clEvtContParseTestInfo();

    return CL_OK;
}

ClRcT clEvtTestContFinalize()
{

    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEoObj;

    rc = clEoMyEoObjectGet(&pEoObj);

    rc = clEoClientUninstall(pEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID);


    return CL_OK;
}

/*****************************************************************************/



ClRcT clEvtTestContStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

ClRcT clEvtTestContHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq = CL_EO_DEFAULT_POLL;
    schFeedback->status = 0;
    return CL_OK;
}

#define CL_EVT_CONT_PORT 0x1233
ClEoConfigT clEoConfig = {
    "EVENT_TEST_CONT_EO",       /* EO Name */
    1,                          /* EO Thread Priority */
    0,                          /* No of EO thread needed */
    CL_EVT_CONT_PORT,           /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,  /* Whether to use main thread for eo Recv or
                                 * not */
    clEvtTestContInitialize,    /* Function CallBack to initialize the
                                 * Application */
    clEvtTestContFinalize,      /* Function Callback to Terminate the
                                 * Application */
    clEvtTestContStateChange,   /* Function Callback to change the Application
                                 * state */
    clEvtTestContHealthCheck,   /* Function Callback to change the Application
                                 * state */
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
    CL_TRUE,                    /* log */
    CL_FALSE,                   /* trace */
    CL_FALSE,                   /* diag */
    CL_FALSE,                   /* txn */
    CL_FALSE,                   /* hpi */
    CL_FALSE,                   /* cli */
    CL_FALSE,                   /* alarm */
    CL_TRUE,                    /* debug */
    CL_FALSE                    /* gms */
};
