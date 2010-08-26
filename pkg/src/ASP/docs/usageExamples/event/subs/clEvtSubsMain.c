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
/*******************************************************************************
 * ModuleName  : event usage example
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <unistd.h>
#include <string.h>

#include "clCpmApi.h"
#include "clEventApi.h"
#include "clEventExtApi.h"

#define UNIQUE_SUBSCRIPTION_ID 0xDEADBEEF
#define CL_EVENT_DEFAULT_SUBS_FILTER NULL

/*
 * The following definition should ideally be in EoApi. 
 * Duplicating here for now.
 */
typedef enum{
    CL_EO_LIB_ID_OSAL,
    CL_EO_LIB_ID_MEM,
    CL_EO_LIB_ID_HEAP,
    CL_EO_LIB_ID_BUFFER,
    CL_EO_LIB_ID_TIMER,
    CL_EO_LIB_ID_IOC,
    CL_EO_LIB_ID_RMD,
    CL_EO_LIB_ID_EO,
    CL_EO_LIB_ID_RES,
    CL_EO_LIB_ID_POOL  =  CL_EO_LIB_ID_RES,
    CL_EO_LIB_ID_CPM,
    CL_EO_LIB_ID_MAX 
}ClEoLibIdT; 

typedef struct ClSubsEventInfo
{
    ClEventInitHandleT initHandle;
    ClEventChannelHandleT channelHandle;
    ClEventHandleT eventHandle;
} ClSubsEventInfoT; 

static ClSubsEventInfoT gSubsEventInfo;

static ClEoPayloadWithReplyCallbackT gClEvtSubsTestFuncList[] =
{
    (ClEoPayloadWithReplyCallbackT) NULL,                          /* 0 */
};

static void clSubEoEventWaterMarkCb( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClRcT rc = CL_OK;

    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    ClNameT publisherName = { 0 };
    ClEventIdT eventId = 0;
    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    ClPtrT pCookie = NULL;

    ClPtrT pEventData = NULL;

    ClUint8T *pEventPayload = NULL;

    /*
     * Allocate memory for the event payload.
     */
    pEventData = clHeapAllocate(eventDataSize);
    if (pEventData == NULL)
    {
        clOsalPrintf("Allocation for event data failed. rc[%#X]\n", rc);
        goto failure;
    }

    /*
     * Fetch the event payload. NOTE that there is a bug in the SAF spec B.01.01
     * allowing the application to pass a pointer and expecting the 
     * clEventDataGet() to return the allocated memory. This is not possible as
     * the parameter pEventData is a single pointer so the memory allocated by
     * the callee can't be returned to the caller.
     */
    rc = clEventDataGet (eventHandle, pEventData,  &eventDataSize);
    if (rc != CL_OK)
    {
        clOsalPrintf("Event Data Get failed. rc[%#X]\n", rc);
        goto failure;
    }

    pEventPayload = pEventData;

    /*
     * Fetch the cookie specified during subscribe.
     */
    rc = clEventCookieGet(eventHandle, &pCookie);
    if (rc != CL_OK)
    {
        clOsalPrintf("Event Cookie Get failed. rc[%#X]\n", rc);
        goto failure;
    }

    /*
     * Fetch the attributes of the event.
     */
    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
                              &retentionTime, &publisherName, &publishTime,
                              &eventId);
    if (rc != CL_OK)
    {
        clOsalPrintf("Event Attributes Get failed. rc[%#X]\n", rc);
        goto failure;
    }

    /*
     * Free the allocated Event 
     */
    rc = clEventFree(eventHandle);

    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n");
    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("             Subscription ID   : %#X\n", subscriptionId);
    clOsalPrintf("             Event Priority    : %#X\n", priority);
    clOsalPrintf("             Publish Time      : 0x%llX\n",
                 publishTime);
    clOsalPrintf("             Retention Time    : 0x%llX\n",
                 retentionTime);
    clOsalPrintf("             Publisher Name    : %.*s\n",
                 publisherName.length, publisherName.value);
    clOsalPrintf("             EventID           : %#X\n", eventId);
    clOsalPrintf("             Event Payload     : %s\n", pEventPayload);
    clOsalPrintf("             Event Cookie      : %s\n", (ClUint8T*)pCookie);

    clHeapFree(pEventPayload); // Release the memory allocated for the payload.

    /*
     * Display the Water Mark Details and Free the patterns.
     * Note that each pattern needs to be freed and then the
     * pattern array in that order.
     */

    clOsalPrintf("             EO Name           : %.*s\n",
                (ClInt32T)patternArray.pPatterns[0].patternSize, 
                (ClCharT *)patternArray.pPatterns[0].pPattern);
    clHeapFree(patternArray.pPatterns[0].pPattern);

    clOsalPrintf("             Library ID        : %#X\n",
                *(ClEoLibIdT *)patternArray.pPatterns[1].pPattern);
    clHeapFree(patternArray.pPatterns[1].pPattern);

    clOsalPrintf("             Water Mark ID     : %#X\n",
                *(ClWaterMarkIdT *)patternArray.pPatterns[2].pPattern);
    clHeapFree(patternArray.pPatterns[2].pPattern);

    clOsalPrintf("             Water Mark Type   : %s\n",
                (*(ClEoWaterMarkFlagT *)patternArray.pPatterns[3].pPattern == CL_WM_HIGH_LIMIT)? "HIGH" : "LOW");
    clHeapFree(patternArray.pPatterns[3].pPattern);

    clOsalPrintf("             Water Mark Value  : %u\n",
                *(ClUint32T *)patternArray.pPatterns[4].pPattern);
    clHeapFree(patternArray.pPatterns[4].pPattern);

    clHeapFree(patternArray.pPatterns);

    clOsalPrintf("-------------------------------------------------------\n");

failure:
    return;
}

static ClCpmHandleT gClEvtSubsCpmHandle; /* FIXME */
ClRcT clEventSubsTerminate(ClInvocationT invocation,
			const ClNameT  *compName)
{
    ClRcT rc;

    rc = clCpmComponentUnregister(gClEvtSubsCpmHandle, compName, NULL);
    rc = clCpmClientFinalize(gClEvtSubsCpmHandle);

    clCpmResponse(gClEvtSubsCpmHandle, invocation, CL_OK);

    return CL_OK;
}


ClRcT clEvtSubsCpmInit()
{
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT	version;
    ClIocPortT	iocPort;
    ClRcT	rc = CL_OK;

/******************************************************************
                    CPM Related stuff
******************************************************************/
/*  Do the CPM client init/Register */
	version.releaseCode = 'B';
	version.majorVersion = 0x1;
	version.minorVersion = 0x1;
	
	callbacks.appHealthCheck = NULL;
	callbacks.appTerminate = clEventSubsTerminate;
	callbacks.appCSISet = NULL;
	callbacks.appCSIRmv = NULL;
	callbacks.appProtectionGroupTrack = NULL;
	callbacks.appProxiedComponentInstantiate = NULL;
	callbacks.appProxiedComponentCleanup = NULL;
		
	clEoMyEoIocPortGet(&iocPort);
	rc = clCpmClientInitialize(&gClEvtSubsCpmHandle, &callbacks, &version);
	rc = clCpmComponentNameGet(gClEvtSubsCpmHandle, &appName);
	rc = clCpmComponentRegister(gClEvtSubsCpmHandle, &appName, NULL);

    return CL_OK;
}

void clSubsAsyncChannelOpenCb(ClInvocationT invocation,
        ClEventChannelHandleT channelHandle, ClRcT retCode)
{
    ClRcT rc = CL_OK;

    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("************* Async Channel Open Callback *************\n");
    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("             Invocation        : %#X\n", invocation);
    clOsalPrintf("             Channel Handle    : %#llX\n", channelHandle);
    clOsalPrintf("             API Return Code   : %#X\n", retCode);
    clOsalPrintf("*******************************************************\n");

    /*
     * Check if the Channel Open Asyn was successful
     */
    if(CL_OK != retCode)
    {
        goto failure;
    }

    rc = clEventSubscribe(channelHandle, CL_EVENT_DEFAULT_SUBS_FILTER, UNIQUE_SUBSCRIPTION_ID, 
            "User Specified Argument (cookie) for the event delivery callback");
    if(CL_OK != rc)
    {
        goto failure;
    }

failure:
    return;
}

static ClRcT clSubsEventLibrayInitialize(void)
{
    ClRcT rc = CL_OK;

    ClVersionT version = CL_EVENT_VERSION;    
    ClNameT channelName = {sizeof(CL_EO_EVENT_CHANNEL_NAME)-1, CL_EO_EVENT_CHANNEL_NAME};

    const ClEventCallbacksT evtCallbacks = 
    {
        NULL, // clSubsAsyncChannelOpenCb for Async Channel Open
        clSubEoEventWaterMarkCb,  // Event Delivery Callback
    };
    ClEventChannelOpenFlagsT evtFlags = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_LOCAL_CHANNEL;

    rc = clEventInitialize(&gSubsEventInfo.initHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventInitialize() failed [%#X]\n",rc);
        goto failure;
    }

    rc = clEventChannelOpen(gSubsEventInfo.initHandle, &channelName, 
            evtFlags, CL_RMD_DEFAULT_TIMEOUT, 
            &gSubsEventInfo.channelHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventChannelOpen() failed [%#X]\n",rc);
        goto init_done;
    }

    rc = clEventSubscribe(gSubsEventInfo.channelHandle, CL_EVENT_DEFAULT_SUBS_FILTER, UNIQUE_SUBSCRIPTION_ID, 
            "User Specified Argument (cookie) for the event delivery callback");
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventSubscribe() failed [%#X]\n",rc);
        goto channel_opened;
    }

    return CL_OK;

channel_opened:
    clEventChannelClose(gSubsEventInfo.channelHandle);

init_done:
    clEventFinalize(gSubsEventInfo.initHandle);

failure:
    return rc;
}

ClRcT clSubsAppInitialize(ClUint32T argc, ClCharT *argv[])
{

    ClRcT rc = CL_OK;
    ClEoExecutionObjT* pEvtSubsEoObj;

    rc = clEoMyEoObjectGet(&pEvtSubsEoObj);
    if( rc != CL_OK )
    {
        clOsalPrintf("EO Object Get failed [%#X]\n",rc);
    }
                                                                                
    rc = clEoClientInstall(pEvtSubsEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,gClEvtSubsTestFuncList, 0,
                                          (int)(sizeof(gClEvtSubsTestFuncList)/sizeof (ClEoPayloadWithReplyCallbackT)));
    if( rc != CL_OK )
    {
        clOsalPrintf("Installing Native table failed [%#X]\n",rc);
    }
    
    clEvtSubsCpmInit(); 

    clSubsEventLibrayInitialize();
    
    return CL_OK;
}

static ClRcT clSubsEventLibrayFinalize(void)
{
    ClRcT rc = CL_OK;

    rc = clEventUnsubscribe(gSubsEventInfo.channelHandle, UNIQUE_SUBSCRIPTION_ID);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventUnsubscribe() failed [%#X]\n",rc);
    }

    rc = clEventChannelClose(gSubsEventInfo.channelHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventChannelClose() failed [%#X]\n",rc);
    }

    rc = clEventFinalize(gSubsEventInfo.initHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventFinalize() failed [%#X]\n",rc);
    }

    return CL_OK;
}

ClRcT clSubsAppFinalize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT* pEvtSubsEoObj;

    rc = clEoMyEoObjectGet(&pEvtSubsEoObj);
    {
        clOsalPrintf("EO Object Get failed [%#X]\n",rc);
    }

    rc = clEoClientUninstall(pEvtSubsEoObj,CL_EO_NATIVE_COMPONENT_TABLE_ID);
    if(CL_OK != rc)
    {
        clOsalPrintf("EO Client Uninstall failed [%#X]\n",rc);
    }

    rc = clSubsEventLibrayFinalize();
    if(CL_OK != rc)
    {
        clOsalPrintf("Event Library Finalize failed [%#X]\n",rc);
    }

    return CL_OK;
}


ClRcT clSubsAppStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

/* 
 * This function will be called periodically to check the EO health. User need 
 * to fill in the structure as per it health
 */

ClRcT   clSubsAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
       schFeedback->freq = CL_EO_BUSY_POLL;
       schFeedback->status = 0;
       return CL_OK;
}


ClEoConfigT clEoConfig = {
    "EVENT_SUBSCRIBER",       /* EO Name*/
    1,              /* EO Thread Priority */
    4,              /* No of EO thread needed */
    CL_IOC_EVENT_PORT+101,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    clSubsAppInitialize,  /* Function CallBack  to initialize the Application */
    clSubsAppFinalize,    /* Function Callback to Terminate the Application */ 
    clSubsAppStateChange, /* Function Callback to change the Application state */
    clSubsAppHealthCheck, /* Function Callback to change the Application state */
};

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,			/* osal */
    CL_TRUE,			/* timer */
    CL_TRUE,			/* buffer */
    CL_TRUE,			/* ioc */
    CL_TRUE,			/* rmd */
    CL_TRUE,			/* eo */
    CL_FALSE,			/* om */
    CL_FALSE,			/* hal */
    CL_FALSE,			/* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_FALSE,			/* cor */
    CL_FALSE,			/* cm */
    CL_FALSE,			/* name */
    CL_TRUE,			/* log */
    CL_FALSE,			/* trace */
    CL_FALSE,			/* diag */
    CL_FALSE,			/* txn */
    CL_FALSE,			/* hpi */
    CL_FALSE,			/* cli */
    CL_FALSE,			/* alarm */
    CL_TRUE,			/* debug */
    CL_FALSE			/* gms */
};
