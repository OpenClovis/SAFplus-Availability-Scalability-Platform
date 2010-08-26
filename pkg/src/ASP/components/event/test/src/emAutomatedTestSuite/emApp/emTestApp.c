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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emApp/emTestApp.c $
 * $Author: bkpavan $
 * $Date: 2006/11/10 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include "clCpmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include "clCpmApi.h"
#include "clEventApi.h"
#include "clEventExtApi.h"
#include "emTestApp.h"
#include "../common/emAutoCommon.h"
#include <string.h>             /* For memcmp */
#if 0
# include <ipi/clSAClientSelect.h>  /* SAF Changes */
#endif

static ClEoPayloadWithReplyCallbackT gClEvtTestAppFuncList[] = {

    (ClEoPayloadWithReplyCallbackT) NULL,   /* 0 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppInit,   /* 1 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppFin,    /* 2 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppOpen,   /* 3 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppClose,  /* 4 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppSub,    /* 5 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppUnsub,  /* 6 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppPub,    /* 7 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppEvtAlloc,   /* 8 */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppAttSet, /* 9 */
    (ClEoPayloadWithReplyCallbackT) NULL,   /* a */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppResultGet,  /* b */
    (ClEoPayloadWithReplyCallbackT) clEvtTestAppReset,  /* c */
};


static ClCpmHandleT gEvtTestAppCpmHandle;
ClRcT clEventTerminate(ClInvocationT invocation, const ClNameT *compName)
{
    ClRcT rc;

    rc = clCpmClientFinalize(gEvtTestAppCpmHandle);

    clCpmResponse(gEvtTestAppCpmHandle, invocation, CL_OK);

    return CL_OK;
}

ClRcT clEvtCpmInit()
{
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT version = { 0 };
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
    rc = clCpmClientInitialize(&gEvtTestAppCpmHandle, &callbacks, &version);
    rc = clCpmComponentNameGet(gEvtTestAppCpmHandle, &appName);
    rc = clCpmComponentRegister(gEvtTestAppCpmHandle, &appName, NULL);

    return CL_OK;
}

ClInt32T clChanContainerKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClUint32T cmpResult = 0;
    ClUint32T retCode = 0;
    ClChanKeyT * pkey1 = (ClChanKeyT *) key1; 
    ClChanKeyT * pkey2 = (ClChanKeyT *) key2;
    
    cmpResult = pkey1->channelscope - pkey2->channelscope;
    if (0==cmpResult)
    {

        retCode = (pkey1->channelName.length) - (pkey2->channelName.length);
        
        if (0==retCode)                       
        {
        cmpResult =  memcmp(pkey1->channelName.value, pkey2->channelName.value,
                pkey1->channelName.length);
            return cmpResult;
        }                      
        else
        {
            return retCode;
        }
    }
    else
    {
        return cmpResult;
    }
    return CL_OK;
}

void clChanContainerDataDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
        clHeapFree((void*)userKey);
        return;
}
/*
 ** Following code was added to support multiple-initialization.
 */

ClInt32T clEvtContInitKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClUint32T cmpResult = 0;

    ClNameT *pInitName1 = (ClNameT *) key1;
    ClNameT *pInitName2 = (ClNameT *) key2;

    cmpResult = pInitName1->length - pInitName2->length;

    if (0 != cmpResult)
    {
        return cmpResult;
    }

    return (ClUint32T) memcmp(pInitName1->value, pInitName2->value,
                              pInitName1->length);
}

void clEvtContInitDataDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *) userKey);
    return;
}

ClRcT clEvtTestAppInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pEoObj;

#if 0
    ClSvcHandleT svcHandle = { 0 };
    ClOsalTaskIdT taskId = 0;
#endif

    /*
     * Create the list to hold the mapping b/n Init Name & evtHandle 
     */
    rc = clCntLlistCreate(clEvtContInitKeyCompare, clEvtContInitDataDelete,
                          clEvtContInitDataDelete, CL_CNT_UNIQUE_KEY,
                          &gEvtTestInitInfo);
    if (CL_OK != rc)
    {
        clOsalPrintf("Creating linked list for Init Failed, rc[0x%X]\r\n", rc);
        return rc;
    }

    rc = clCntLlistCreate(clChanContainerKeyCompare, clChanContainerDataDelete,
                          clChanContainerDataDelete, CL_CNT_UNIQUE_KEY,
                          &gChanHandleInitInfo);
    if(rc!=CL_OK)
    {
        clOsalPrintf("Creating Linked List for Channel handles failed\n\r");
        return rc;
    }
    /*
     * Create a Mutex to lock the containers before accessing them 
     */
    rc = clOsalMutexCreate(&gEvtTestInitMutex);
    if (CL_OK != rc)
    {
        clOsalPrintf("Creating Mutex for Init Failed, rc[0x%X]\r\n", rc);
        return rc;
    }

    rc = clEoMyEoObjectGet(&pEoObj);
    rc = clEoClientInstall(pEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,
                           gClEvtTestAppFuncList, (ClEoDataT) NULL,
                           (int) sizeof(gClEvtTestAppFuncList) /
                           (int) sizeof(ClEoPayloadWithReplyCallbackT));
    rc = clEvtCpmInit();

#if 0
    svcHandle.cpmHandle = &gEvtTestAppCpmHandle;
    rc = clDispatchThreadCreate(pEoObj, &taskId, svcHandle);
#endif

    return CL_OK;
}

ClRcT clEvtTestAppFinalize()
{

    ClRcT rc = CL_OK;

    ClEoExecutionObjT *pEoObj;

    /*
     * Delete the list maintaining the mapping b/n Init Name & evtHandle 
     */
    clCntDelete(gEvtTestInitInfo);

    /*
     * Delete the Mutes for the List in Init 
     */
    clOsalMutexDelete(gEvtTestInitMutex);

    rc = clEoMyEoObjectGet(&pEoObj);

    rc = clEoClientUninstall(pEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID);

    return CL_OK;
}

/*****************************************************************************/



ClRcT clEvtTestAppStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

ClRcT clEvtTestAppHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq = CL_EO_DEFAULT_POLL;
    schFeedback->status = 0;
    return CL_OK;
}

#define CL_EVT_APP_PORT  0      /* 0 implies Port is obtained dynamically */

ClEoConfigT clEoConfig = {
    "EVENT_TEST_APP_EO",        /* EO Name */
    1,                          /* EO Thread Priority */
    0,                          /* No of EO thread needed */
    CL_EVT_APP_PORT,            /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,  /* Whether to use main thread for eo Recv or
                                 * not */
    clEvtTestAppInitialize,     /* Function CallBack to initialize the
                                 * Application */
    clEvtTestAppFinalize,       /* Function Callback to Terminate the
                                 * Application */
    clEvtTestAppStateChange,    /* Function Callback to change the Application
                                 * state */
    clEvtTestAppHealthCheck,    /* Function Callback to change the Application
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
