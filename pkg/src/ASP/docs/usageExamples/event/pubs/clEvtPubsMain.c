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

#define UNIQUE_INVOCATION_ID 0xDEADBEEF
#define CL_EVENT_PUBLISHER_NAME "SOME_PUBLISHER_NAME_FOR_IDENTIFICATION"

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

typedef struct ClPubsEventInfo
{
    ClEventInitHandleT initHandle;
    ClEventChannelHandleT channelHandle;
    ClEventHandleT eventHandle;
} ClPubsEventInfoT; 

static ClPubsEventInfoT gPubsEventInfo;

static ClEoPayloadWithReplyCallbackT gClEvtPubsTestFuncList[] =
{
    (ClEoPayloadWithReplyCallbackT) NULL,                          /* 0 */
};

static ClRcT clPubsTriggerEvent(ClEoLibIdT libId, ClWaterMarkIdT wmId, ClUint32T wmValue, ClEoWaterMarkFlagT wmFlag)
{
    ClRcT rc = CL_OK;

    ClEventIdT eventId = 0;
    ClNameT publisherName = {sizeof(CL_EVENT_PUBLISHER_NAME)-1, CL_EVENT_PUBLISHER_NAME};

    ClEventPatternT patterns[5] = {{0}};

    ClEventPatternArrayT patternArray = {
        0,
        CL_SIZEOF_ARRAY(patterns),
        patterns 
    };

    rc = clEventAllocate(gPubsEventInfo.channelHandle, &gPubsEventInfo.eventHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventAllocate() failed [%#X]\n",rc);
        goto failure;
    }

    patterns[0].patternSize = strlen(CL_EO_NAME);
    patterns[0].pPattern = (ClUint8T *)CL_EO_NAME;
    
    patterns[1].patternSize = sizeof(libId);
    patterns[1].pPattern = (ClUint8T *)&libId;
    
    patterns[2].patternSize = sizeof(wmId);
    patterns[2].pPattern = (ClUint8T *)&wmId;
    
    patterns[3].patternSize = sizeof(wmFlag);
    patterns[3].pPattern = (ClUint8T *)&wmFlag;
    
    patterns[4].patternSize = sizeof(wmValue);
    patterns[4].pPattern = (ClUint8T *)(&wmValue);
    
    rc = clEventAttributesSet(gPubsEventInfo.eventHandle, &patternArray, 
            CL_EVENT_HIGHEST_PRIORITY, 0, &publisherName);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventAttributesSet() failed [%#X]\n",rc);
        goto event_allocated;
    }

    rc = clEventPublish(gPubsEventInfo.eventHandle, "Event Payload passed in endian neutral way", 
            sizeof("Event Payload passed in endian neutral way."), &eventId);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventPublish() failed [%#X]\n",rc);
        goto event_allocated;
    }

event_allocated:
    rc = clEventFree(gPubsEventInfo.eventHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventFree() failed [%#X]\n",rc);
    }

failure:
    return rc;
}

static ClCpmHandleT gClEvtPubsCpmHandle; /* FIXME */
ClRcT clEventPubsTerminate(ClInvocationT invocation,
			const ClNameT  *compName)
{
    ClRcT rc;

    rc = clCpmComponentUnregister(gClEvtPubsCpmHandle, compName, NULL);
    rc = clCpmClientFinalize(gClEvtPubsCpmHandle);

    clCpmResponse(gClEvtPubsCpmHandle, invocation, CL_OK);

    return CL_OK;
}


ClRcT clEvtPubsCpmInit()
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
	callbacks.appTerminate = clEventPubsTerminate;
	callbacks.appCSISet = NULL;
	callbacks.appCSIRmv = NULL;
	callbacks.appProtectionGroupTrack = NULL;
	callbacks.appProxiedComponentInstantiate = NULL;
	callbacks.appProxiedComponentCleanup = NULL;
		
	clEoMyEoIocPortGet(&iocPort);
	rc = clCpmClientInitialize(&gClEvtPubsCpmHandle, &callbacks, &version);
	rc = clCpmComponentNameGet(gClEvtPubsCpmHandle, &appName);
	rc = clCpmComponentRegister(gClEvtPubsCpmHandle, &appName, NULL);

    return CL_OK;
}

void clPubsAsyncChannelOpenCb(ClInvocationT invocation,
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
        clOsalPrintf("clEventChannelOpenAsync() failed [%#X]\n",rc);
        goto failure;
    }

    gPubsEventInfo.channelHandle = channelHandle;

    rc = clPubsTriggerEvent(CL_EO_LIB_ID_HEAP, CL_WM_HIGH, CL_WM_HIGH_LIMIT, CL_WM_LOW_LIMIT);
    if(CL_OK != rc)
    {
        clOsalPrintf("Publish trigger failed [%#X]\n",rc);
        goto channel_opened;
    }

channel_opened:
    clEventChannelClose(gPubsEventInfo.channelHandle);

failure:
    return;
}

static ClRcT clPubsEventLibrayInitialize(void)
{
    ClRcT rc = CL_OK;

    ClVersionT version = CL_EVENT_VERSION;    
    ClNameT channelName = {sizeof(CL_EO_EVENT_CHANNEL_NAME)-1, CL_EO_EVENT_CHANNEL_NAME};

    const ClEventCallbacksT evtCallbacks = 
    {
        clPubsAsyncChannelOpenCb, // Can be NULL if sync call is used
        NULL,   // Since it is not a subscriber  
    };
    ClEventChannelOpenFlagsT evtFlags = CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_LOCAL_CHANNEL;

    ClInvocationT invocation = UNIQUE_INVOCATION_ID; // To identify the callback

    rc = clEventInitialize(&gPubsEventInfo.initHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventInitialize() failed [%#X]\n",rc);
        goto failure;
    }

    rc = clEventChannelOpenAsync(gPubsEventInfo.initHandle, invocation, 
            &channelName, evtFlags);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventChannelOpen() failed [%#X]\n",rc);
        goto init_done;
    }

    return CL_OK;

init_done:
    clEventFinalize(gPubsEventInfo.initHandle);

failure:
    return rc;
}

ClRcT clPubsAppInitialize(ClUint32T argc, ClCharT *argv[])
{

    ClRcT rc = CL_OK;
    ClEoExecutionObjT* pEvtPubsEoObj;

    rc = clEoMyEoObjectGet(&pEvtPubsEoObj);
    if( rc != CL_OK )
    {
        clOsalPrintf("EO Object Get failed [%#X]\n",rc);
    }
                                                                                
    rc = clEoClientInstall(pEvtPubsEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,gClEvtPubsTestFuncList, 0,
                                          (int)(sizeof(gClEvtPubsTestFuncList)/sizeof (ClEoPayloadWithReplyCallbackT)));
    if( rc != CL_OK )
    {
        clOsalPrintf("Installing Native table failed [%#X]\n",rc);
    }
    
    clEvtPubsCpmInit(); 

    clPubsEventLibrayInitialize();
    
    return CL_OK;
}

static ClRcT clPubsEventLibrayFinalize(void)
{
    ClRcT rc = CL_OK;

    rc = clEventChannelClose(gPubsEventInfo.channelHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventChannelClose() failed [%#X]\n",rc);
    }

    rc = clEventFinalize(gPubsEventInfo.initHandle);
    if(CL_OK != rc)
    {
        clOsalPrintf("clEventFinalize() failed [%#X]\n",rc);
    }

    return CL_OK;
}

ClRcT clPubsAppFinalize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT* pEvtPubsEoObj;

    rc = clEoMyEoObjectGet(&pEvtPubsEoObj);
    {
        clOsalPrintf("EO Object Get failed [%#X]\n",rc);
    }

    rc = clEoClientUninstall(pEvtPubsEoObj,CL_EO_NATIVE_COMPONENT_TABLE_ID);
    if(CL_OK != rc)
    {
        clOsalPrintf("EO Client Uninstall failed [%#X]\n",rc);
    }

    rc = clPubsEventLibrayFinalize();
    if(CL_OK != rc)
    {
        clOsalPrintf("Event Library Finalize failed [%#X]\n",rc);
    }

    return CL_OK;
}


ClRcT clPubsAppStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

/* 
 * This function will be called periodically to check the EO health. User need 
 * to fill in the structure as per it health
 */

ClRcT   clPubsAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
       schFeedback->freq = CL_EO_BUSY_POLL;
       schFeedback->status = 0;
       return CL_OK;
}


ClEoConfigT clEoConfig = {
    "EVENT_PUBLISHER",       /* EO Name*/
    1,              /* EO Thread Priority */
    4,              /* No of EO thread needed */
    CL_IOC_EVENT_PORT+100,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    clPubsAppInitialize,  /* Function CallBack  to initialize the Application */
    clPubsAppFinalize,    /* Function Callback to Terminate the Application */ 
    clPubsAppStateChange, /* Function Callback to change the Application state */
    clPubsAppHealthCheck, /* Function Callback to change the Application state */
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
