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
 * $File: //depot/dev/RC2/R2.2-Mercury/ASP/components/event/test/unit-test/evtIntTest/pubs/clEvtPubsMain.c $
 * $Author: mayank $
 * $Date: 2007/05/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <unistd.h>
#include "clDebugApi.h"
#include "clCpmApi.h"
#if 0
#include <ipi/clSAClientSelect.h> /* SAF Changes */
#endif
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include "clEventApi.h"
#include "../common/clEvtTestCommon.h"
#include "clEventExtApi.h"

#if 0
#include "clEventSuppressionApi.h"
#endif

#if 1
#define EVENT_ENABLE
#endif

#if 0
#define DIAG_ENABLE
#include "clDmApi.h"
#include "clDmSwPlPxApi.h"
#endif

/* @ */
#if 0
#define NEW_TEST_ENABLE
#endif

/* Filter and pattern information */
#define gPatt1 "123456"
#define gPatt1_size sizeof(gPatt1)

#define gPatt2 "1"
#define gPatt2_size sizeof(gPatt2)

#define gPatt3 "1234"
#define gPatt3_size sizeof(gPatt3)

#define gPatt4 "12345678"
#define gPatt4_size sizeof(gPatt4)

ClEventFilterT gFilters[] = {
	{CL_EVENT_EXACT_FILTER, {0, gPatt1_size, (ClUint8T*)gPatt1}},
	{CL_EVENT_EXACT_FILTER, {0, gPatt2_size, (ClUint8T*)gPatt2}},
	{CL_EVENT_EXACT_FILTER, {0, gPatt3_size, (ClUint8T*)gPatt3}},
	{CL_EVENT_EXACT_FILTER, {0, gPatt4_size, (ClUint8T*)gPatt4}}
};


ClEventFilterArrayT gSubscribeFilters = {
    sizeof(gFilters)/sizeof(ClEventFilterT),
	gFilters	
};

ClEventFilterT gNoMatchFilters[] = {
	{CL_EVENT_EXACT_FILTER, {0, gPatt1_size, (ClUint8T*)gPatt1}},
	{CL_EVENT_EXACT_FILTER, {0, gPatt2_size, (ClUint8T*)gPatt2}},
	{CL_EVENT_EXACT_FILTER, {0, gPatt3_size, (ClUint8T*)gPatt3}},
};

ClEventFilterArrayT gNoMatchSubscribeFilters = {
    sizeof(gFilters)/sizeof(ClEventFilterT),
	gFilters	
};


ClEventPatternT gPatterns[] = {
	{0, gPatt1_size, (ClUint8T*)gPatt1,},
	{0, gPatt2_size, (ClUint8T*)gPatt2},
	{0, gPatt3_size, (ClUint8T*)gPatt3},
	{0, gPatt4_size, (ClUint8T*)gPatt4}
};
ClEventPatternArrayT gPatternArray = {
    0,
    sizeof(gPatterns)/sizeof(ClEventPatternT),
    gPatterns
};


ClEoExecutionObjT* gEvtPubsEoObj;

void clEvtSubsTestDeliverCallback( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClRcT rc = CL_OK;

    ClEventPriorityT priority = 0;
    ClTimeT  retentionTime = 0;
    ClNameT  publisherName = {0};
    ClEventIdT eventId = 0;
    
    ClUint8T payLoad[50];
    ClSizeT payLoadLen = sizeof(payLoad);
    
    ClEventPatternArrayT patternArray = {0};
    ClTimeT  publishTime = 0;
    void* pCookie = NULL;

    ClUint8T i = 0; /* We know the no. of patterns is 4 */

    rc = clEventDataGet(eventHandle, (void*)&payLoad,  &payLoadLen);

    rc = clEventAttributesGet( eventHandle,
            &patternArray, &priority,
            &retentionTime, &publisherName,
            &publishTime, &eventId );

    rc = clEventCookieGet(eventHandle, &pCookie);

    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n");
    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("             Subscription ID   : 0x%X\n", subscriptionId);
    clOsalPrintf("             Event Priority    : 0x%X\n", priority);
    clOsalPrintf("             Retention Time    : 0x%X\n", retentionTime);
    clOsalPrintf("             Publisher Name    : %.*s\n", publisherName.length, publisherName.value);
    clOsalPrintf("             EventID           : 0x%X\n", eventId);
    clOsalPrintf("             Payload           : %s\n", payLoad);
    clOsalPrintf("             Payload Len       : 0x%X\n", (ClUint32T)payLoadLen);
    clOsalPrintf("             Event Data Size   : 0x%X\n", (ClUint32T)eventDataSize);
    clOsalPrintf("             Cookie            : 0x%X\n", pCookie);
    clOsalPrintf("-------------------------------------------------------\n");

    /* Free the allocated Event */
    rc = clEventFree(eventHandle);

    for(i = 0; i < patternArray.patternsNumber; i++)
    {
        clOsalFree(patternArray.pPatterns[i].pPattern);
    }
    clOsalFree(patternArray.pPatterns); /* User is expected to free this memory if he doesn't allocate it */

    return;

#if 0
    ClEventPatternArrayT patternArray;
    ClTimeT  publishTime;
    ClEventPriorityT priority;
    ClTimeT  retentionTime;
    ClNameT  publisherName;
    ClEventIdT eventId;
    ClRcT rc = CL_OK;
    ClUint8T payLoad[50];
    ClSizeT payLoadLen = sizeof(payLoad);
    ClEventPatternArrayT patternArray;
    ClTimeT  publishTime;
    void *pCookie;
    
    rc = clEventCookieGet(eventHandle, &pCookie);

    rc = clEventAttributesGet( eventHandle, 
            &patternArray, &priority,
            &retentionTime, &publisherName,
            &publishTime, &eventId );
    rc = clEventDataGet (eventHandle, (void*)payLoad,  &payLoadLen);

    clOsalPrintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    clOsalPrintf("Subscription ID : 0x%X\r\n", subscriptionId);
    clOsalPrintf("Event Priority  : 0x%X\r\n", priority);
    clOsalPrintf("Retention Time  : 0x%X\r\n", retentionTime);
    clOsalPrintf("Publisher Name  : %s\r\n", publisherName.value);
    clOsalPrintf("EventID         : 0x%X\r\n", eventId);
    clOsalPrintf("Payload         : %s\r\n", payLoad);    
    clOsalPrintf("Payload Len     : 0x%X\r\n", (ClUint32T)payLoadLen);
    clOsalPrintf("Cookie          : 0x%X\r\n", pCookie);
    clOsalPrintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
            
    
    return;
#endif
}

void clEvtSubsTestDeliverCallback( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize );

static ClEoPayloadWithReplyCallbackT gClEvtPubsTestFuncList[] =
{

    (ClEoPayloadWithReplyCallbackT) NULL,                          /* 0 */
};

#define EVENT_TEST_PUB_NAME "EVENT_TEST_PUB_NAME"

#define EVENT_TEST_PAYLOAD "EVENT_TEST_PAYLOAD"

#ifdef DIAG_ENABLE
ClRcT   clDmTestDestHandleCreate( CL_IN ClDmDestAddrT* pDestAddr, CL_OUT ClDmDestHandleT* pDestHandle )
{
    clOsalPrintf(":::: clDmTestDestHandleCreate\r\n");
    *pDestHandle = 0x11223344;
    return CL_OK;
}
ClRcT   clDmTestDestHandleDelete( CL_IN ClDmDestHandleT destHandle )
{
    clOsalPrintf(":::: clDmTestDestHandleDelete\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    return CL_OK;
}

ClRcT   clDmTestEventGet( CL_IN ClDmDestHandleT destHandle, CL_OUT ClDmEventIdT* pEventID, CL_OUT ClDmEventResultsT* pEventBuf, CL_IN ClTimeT timeOut )
{
    clOsalPrintf(":::: clDmTestEventGet\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);

    *pEventID = 0x1111;
    memset(pEventBuf,0xab,sizeof(ClDmEventResultsT));
    
    return CL_OK;
}
ClRcT   clDmTestTestsEnumerate( CL_IN ClDmDestHandleT destHandle, CL_OUT ClDmNumTestsT* pNumDiagTests, CL_OUT ClDmPropStructT* pDiagProp )
{
    clOsalPrintf(":::: clDmTestTestsEnumerate\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    clOsalPrintf(":::: No of tests : [0x%X]\r\n",*pNumDiagTests);
    *pNumDiagTests = (*pNumDiagTests)+1;

    memset(pDiagProp,0xbc,sizeof(ClDmPropStructT));
    
    
    return CL_OK;
}
ClRcT   clDmTestTestStart( CL_IN ClDmDestHandleT destHandle, CL_IN ClNameT diagTestName, CL_IN ClInt8T diagTestParms[CL_DM_TEST_PARMSLEN], CL_OUT ClDmInstHandleT* pDiagInstHandle )
{
    clOsalPrintf(":::: clDmTestTestStart\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    clOsalPrintf(":::: Test Name : [%s]\r\n",diagTestName.value);
    clOsalPrintf(":::: Test Params : [%s]\r\n",diagTestParms);
    *pDiagInstHandle = 0xaabb;
    return CL_OK;
}
ClRcT   clDmTestTestStop( CL_IN ClDmDestHandleT destHandle, CL_IN ClDmInstHandleT diagInstHandle )
{
    clOsalPrintf(":::: clDmTestTestStop\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    clOsalPrintf(":::: InstHandle : [0x%X]\r\n",diagInstHandle);    
    return CL_OK;
}
ClRcT   clDmTestTestProgressGet( CL_IN ClDmDestHandleT destHandle, CL_IN ClDmInstHandleT diagInstHandle, CL_OUT ClDmProgIndicatorT* pDiagProgIndicator )
{
    clOsalPrintf(":::: clDmTestTestProgressGet\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    clOsalPrintf(":::: InstHandle : [0x%X]\r\n",diagInstHandle);    
    memset(pDiagProgIndicator,0xcd,sizeof(ClDmProgIndicatorT));
    
    return CL_OK;
}
ClRcT   clDmTestTestResultsGet( CL_IN ClDmDestHandleT destHandle, CL_IN ClDmResultDescrPtrT destResultBufPtr, CL_OUT ClInt8T* pDiagResultsBuf )
{
    clOsalPrintf(":::: clDmTestTestResultsGet\r\n");
    clOsalPrintf(":::: Handle : [0x%X]\r\n",destHandle);
    
    return CL_OK;
}
#endif


ClNameT publishCleanupName = {sizeof("CLEANUP"),"CLEANUP"};
ClRcT clEvtPubsTest()
{
    ClRcT rc;
    ClVersionT version = CL_EVENT_VERSION;
#ifdef EVENT_ENABLE
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        NULL
    };
    ClEventChannelHandleT evtChannelHandle;
    ClEventHandleT eventHandle;    
    ClEventHandleT eventHandle1;    
    ClNameT publisherName = {sizeof(EVENT_TEST_PUB_NAME),EVENT_TEST_PUB_NAME};
    ClEventIdT eventId;
    ClUint8T count;//,loop;
    ClCharT buff[40] = {0};
#endif    
    rc = CL_OK;
#ifdef DIAG_ENABLE
    ClDmPluginCallbacksT pluginCallback = {
            clDmTestDestHandleCreate,
            clDmTestDestHandleDelete,
            clDmTestEventGet,
            clDmTestTestsEnumerate,
            clDmTestTestStart,
            clDmTestTestStop,
            clDmTestTestProgressGet,
            clDmTestTestResultsGet,             
    };
    ClDmHandleT dmHandle = 0;
    rc = clDmInitialize(&pluginCallback, &version, &dmHandle);
    clOsalPrintf("After DmInit : handle : [0x%X] : [0x%X]\r\n",dmHandle,rc);    
#endif

#ifdef EVENT_ENABLE
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventChannelOpen(evtHandle, &gEvtTestChannelName, 
                            CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_LOCAL_CHANNEL, 
                            (ClTimeT)-1, &evtChannelHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventAllocate(evtChannelHandle, &eventHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Allocate Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventAttributesSet(eventHandle, &gPatternArray, 1, 0, &publisherName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
        return rc;
    }
#if 0 // Mynk
    rc = clEventPublish(eventHandle, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
        return rc;
    }
#endif

    /************ Utility function **************/
    rc = clEventAllocate(evtChannelHandle, &eventHandle1);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Allocate Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventExtAttributesSet(eventHandle1, EVENT_TEST_EVENT_TYPE, 1, 0, &publisherName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
        return rc;
    }

    printf("Going to publish 100 times in one channel");
    
//    while(1) 
//    for(loop = 0; loop < 10; loop++)
    {    
    	for(count = 0;count < 20;count++)
    	{
		/*
        	rc = clEventPublish(eventHandle1, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
	        if(CL_OK != rc)
        	{
            	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
            	return rc;
        	}
		*/
		memset(buff,0,sizeof(buff));
		sprintf(buff,"EVENT_TEST_PAYLOAD_%d",count);
        	rc = clEventPublish(eventHandle, (const void*)buff, strlen(buff), &eventId);
        	if(CL_OK != rc)
        	{
            		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
            		return rc;
        	}
		printf("Published :: %d times \n",count);
        	// usleep(100);
	//        printf("\n\r\nPress any key to continue...");
	//        getchar();

    	}
    }

    rc = clEventPublish(eventHandle1, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
        return rc;
    }

    /* To request Subscriber to do the cleanup */
    rc = clEventAttributesSet(eventHandle, &gPatternArray, 1, 0, &publishCleanupName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventPublish(eventHandle, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
        return rc;
    }

    /* Need to free the event objects and other cleanup TODO */ 

#endif    
    return CL_OK;
}

#ifdef NEW_TEST_ENABLE

#if 0
extern ClRcT clEvtSubsInfoShow(ClNameT *pChannelName, ClUint8T channelScope, 
        ClUint8T disDetailFlag, ClDebugPrintHandleT dbgPrintHandle);
#endif

ClRcT clEvtPublishTest(ClUint32T channelScope);
ClRcT   clEvtCleanupFinalizeTest(void);
ClRcT   clEvtCleanupChannelCloseTest(void);
ClRcT   clEvtCleanupUnsubscribeTest(void);
#endif
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

/******************************************************************
                    CPM Related stuff
******************************************************************/
/*  Do the CPM client init/Register */

ClRcT clEvtPubsCpmInit()
{
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT	version;
    ClIocPortT	iocPort;
    ClRcT	rc = CL_OK;

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

ClRcT clEvtPubInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

#if 0
    ClSvcHandleT svcHandle = {0};
    ClOsalTaskIdT taskId = 0;
#endif

    rc = clEoMyEoObjectGet(&gEvtPubsEoObj);
                                                                                
    rc = clEoClientInstall(gEvtPubsEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,gClEvtPubsTestFuncList, 0,
                                          (int)(sizeof(gClEvtPubsTestFuncList)/sizeof (ClEoPayloadWithReplyCallbackT)));
    if( rc != CL_OK )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Installing Native table failed [0x%X]\n\r",rc));
    }

    clEvtPubsCpmInit();

#if 0
    svcHandle.cpmHandle = &gClEvtPubsCpmHandle;
    rc = clDispatchThreadCreate(gEvtPubsEoObj, &taskId, svcHandle);
#endif

#ifdef NEW_TEST_ENABLE
    clEvtPublishTest(CL_EVENT_LOCAL_CHANNEL);
    clEvtPublishTest(CL_EVENT_GLOBAL_CHANNEL); 

    sleep(1);

    printf("Just before Finalize test\n");
    clEvtCleanupFinalizeTest();

    sleep(1);
    printf("clEvtCleanupChannelCloseTest test\n");
    clEvtCleanupChannelCloseTest();
    sleep(1);
    printf("clEvtCleanupUnsubscribeTest test\n");

    clEvtCleanupUnsubscribeTest();

    printf("Through\n"); 
#else
    clEvtPubsTest();
#endif

    return CL_OK;
}

ClRcT clEvtPubFinalize()
{
    ClRcT rc = CL_OK;
    rc = clEoClientUninstall(gEvtPubsEoObj,CL_EO_NATIVE_COMPONENT_TABLE_ID);
    return CL_OK;
}


ClRcT   clEvtPubStateChange(ClEoStateT eoState)
{
    
    return CL_OK;
}

/* 
 * This function will be called periodically to check the EO health. User need 
 * to fill in the structure as per it health
 */

ClRcT   clEvtPubHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    
       schFeedback->freq = CL_EO_BUSY_POLL;
       schFeedback->status = 0;
       return CL_OK;
}

ClEoConfigT clEoConfig = {
    "EVENT_PUB_TEST_EO",       /* EO Name*/
    1,              /* EO Thread Priority */
    4,              /* No of EO thread needed */
    CL_IOC_EVENT_PORT+100,         /* Required Ioc Port */ /* FIXME Need to get it in proper way*/
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    clEvtPubInitialize,  /* Function CallBack  to initialize the Application */
    clEvtPubFinalize,    /* Function Callback to Terminate the Application */ 
    clEvtPubStateChange, /* Function Callback to change the Application state */
    clEvtPubHealthCheck, /* Function Callback to change the Application state */
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

#ifdef NEW_TEST_ENABLE

/*** New Test Cases Begin Here ***/

#define LOCAL_CHANNEL "Clovis Local Channel"
#define GLOBAL_CHANNEL "Clovis Global Channel"

#define X 1
#define Y 2
#define Z 3





ClRcT clEvtPublishTest(ClUint32T channelScope)
{
#ifdef EVENT_ENABLE
    ClRcT rc = CL_OK;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        NULL
    };
    ClEventChannelHandleT evtChannelHandle;
    
    ClEventHandleT eventHandleX;    
    ClEventHandleT eventHandleY;

    ClNameT gEvtECHNameLocal     = {sizeof(LOCAL_CHANNEL)-1, LOCAL_CHANNEL};
    ClNameT gEvtECHNameGlobal    = {sizeof(GLOBAL_CHANNEL)-1, GLOBAL_CHANNEL};
    
    ClNameT publisherName = {sizeof(EVENT_TEST_PUB_NAME),EVENT_TEST_PUB_NAME};
    ClEventIdT eventId;

/*    CL_OUT ClEvtSuppressionHandleT suppressionHandleMatch, suppressionHandleNoMatch;
    ClRuleExprT* pRbeExpr = NULL; */

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    if(CL_EVENT_LOCAL_CHANNEL == channelScope)
    {
        rc = clEventChannelOpen(evtHandle, &gEvtECHNameLocal, 
            CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_LOCAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandle);
    }
    else
    {
        rc = clEventChannelOpen(evtHandle, &gEvtECHNameGlobal, 
            CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandle);
    }
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandle, EVENT_TEST_EVENT_TYPE, 0xB, (void*)0xB00);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext Subscription Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventSubscribe(evtChannelHandle, &gSubscribeFilters, 0xA, (void*)0xA00);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Match Subscription Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventSubscribe(evtChannelHandle, &gNoMatchSubscribeFilters, 0xC, (void*)0xC00);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Suppression set Failed [0x%X]\n\r",rc));
        return rc;
    }


#if 0
    /* For better code coverage */
    rc = clEventSuppressionSet(evtChannelHandle, &gNoMatchSubscribeFilters, 
                        &publisherName, NULL, &suppressionHandleNoMatch);

    rc = clRuleExprAllocate(1, &pRbeExpr);
    if(NULL == pRbeExpr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Allocate Failed [0x%X]\n\r",rc));
    }

    rc = clEventSuppressionSet(evtChannelHandle, &gNoMatchSubscribeFilters, 
                        &publisherName, pRbeExpr, &suppressionHandleNoMatch);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Suppression Set Failed [0x%X]\n\r",rc));
/*        return rc;  For better coverage */
    }
    
    rc = clEventExtWithRbeSubscribe(evtChannelHandle, pRbeExpr, Z, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
        

    rc = clEventSuppressionSet(evtChannelHandle, &gNoMatchSubscribeFilters, 
                        &publisherName, pRbeExpr, &suppressionHandleMatch);    
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Suppression Set Failed [0x%X]\n\r",rc));
/*        return rc;  For better coverage */
    }
#endif
    
    rc = clEventAllocate(evtChannelHandle, &eventHandleX);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Allocate Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventAttributesSet(eventHandleX, &gPatternArray, 1, 0, &publisherName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
        return rc;
    }
    rc = clEventPublish(eventHandleX, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
        return rc;
    }


    /************ Utility function **************/
    rc = clEventAllocate(evtChannelHandle, &eventHandleY);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Allocate Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventExtAttributesSet(eventHandleY, EVENT_TEST_EVENT_TYPE, 1, 0, &publisherName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
        return rc;
    }

    
    rc = clEventPublish(eventHandleY, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
        return rc;
    }

#if 0
    if(1)  /* Display Flag */
    {
        ClNameT channelName     = { sizeof("-a")-1, "-a"};
        
        ClDebugPrintHandleT     dbgPrintHandle = 0;

        rc = clDebugPrintInitialize(&dbgPrintHandle);
        
        rc = clEvtSubsInfoShow(&channelName, channelScope, 1, dbgPrintHandle);
        if( rc != CL_OK )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EM Information Display failed [0x%X] \n\r",rc));        
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clEvtSubsInfoShow(&channelName, channelScope, 0, dbgPrintHandle);
        if( rc != CL_OK )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EM Information Display failed [0x%X] \n\r",rc));        
            CL_FUNC_EXIT();
            return rc;
        }

        /* To use all options */
        
        rc = clEvtSubsInfoShow(&gEvtECHNameLocal, channelScope, 1, dbgPrintHandle);
        if( rc != CL_OK )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EM Information Display failed [0x%X] \n\r",rc));        
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clEvtSubsInfoShow(&gEvtECHNameGlobal, channelScope, 0, dbgPrintHandle);
        if( rc != CL_OK )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EM Information Display failed [0x%X] \n\r",rc));        
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clDebugPrintDestroy(&dbgPrintHandle);
    }
#endif

    sleep(3);

    /* Request the subscriber to do the cleanup */
    if(CL_EVENT_GLOBAL_CHANNEL == channelScope)
    {
        rc = clEventAttributesSet(eventHandleX, &gPatternArray, 1, 0, &publishCleanupName);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [0x%X]\n\r",rc));
            return rc;
        }
        rc = clEventPublish(eventHandleX, (const void*)EVENT_TEST_PAYLOAD, sizeof(EVENT_TEST_PAYLOAD), &eventId);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Published Failed [0x%X]\n\r",rc));
            return rc;
        }
    }

    sleep(3);
    
    rc = clEventUnsubscribe(evtChannelHandle, 0xB);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext Unsubscription Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventUnsubscribe(evtChannelHandle, 0xA);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Match Unsubscription Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventUnsubscribe(evtChannelHandle, 0xC);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No Match Unsubscription Failed [0x%X]\n\r",rc));
        return rc;
    }

#if 0
    rc = clEventSuppressionClear(evtChannelHandle, suppressionHandleMatch);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Suppression Clear Failed [0x%X]\n\r",rc));
/*        return rc;  For better coverage */
    }

    rc = clEventSuppressionClear(evtChannelHandle, suppressionHandleNoMatch);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Suppression Clear Failed [0x%X]\n\r",rc));
/*        return rc;  For better coverage */
    }


    clRuleExprDeallocate(pRbeExpr);
#endif

    rc = clEventFree(eventHandleX);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Free Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventFree(eventHandleY);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Free Failed [0x%X]\n\r",rc));
        return rc;
    }


    rc = clEventChannelClose(evtChannelHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Close Failed [0x%X]\n\r",rc));
        return rc;
    }

    /* For Code Coverage Sake only - Mynk */
    rc = clEventChannelUnlink(0, NULL);

    rc = clEventFinalize(evtHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Finalize Failed [0x%X]\n\r",rc));
        return rc;
    }

#endif    
    return CL_OK;
}

/************************* Cleanup Finalize Test *******************************/


ClRcT   clEvtCleanupFinalizeTest(void)
{
#ifdef EVENT_ENABLE
    ClRcT rc = 0;

    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        clEvtSubsTestDeliverCallback
    };
    ClVersionT version = CL_EVENT_VERSION;
    
    ClEventChannelHandleT evtChannelHandleLocal, evtChannelHandleGlobal;

    ClNameT channelA = {sizeof("ChannelA")-1,"ChannelA"};
    ClNameT channelB = {sizeof("ChannelB")-1,"ChannelB"};
    
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
        return rc;
    }

    /*** For Local Channel ***/
    
    rc = clEventChannelOpen(evtHandle, &channelA, 
            CL_EVENT_CHANNEL_SUBSCRIBER, (ClTimeT)-1, &evtChannelHandleLocal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleLocal, EVENT_TEST_EVENT_TYPE, X, (void*)0x800);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }

    /*** For Global Channel ***/
    rc = clEventChannelOpen(evtHandle, &channelB, 
            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandleGlobal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventSubscribe(evtChannelHandleGlobal, &gSubscribeFilters, Y, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleGlobal, EVENT_TEST_EVENT_TYPE, Z, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }

    
    /*** Finalize ***/

    rc = clEventFinalize(evtHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Finalize Failed [0x%X]\n\r",rc));
        return rc;
    }

    printf("Finalize Successful!!!\n");
    
#endif

    return CL_OK;
}

/************************* Cleanup Channel Close Test *******************************/


ClRcT   clEvtCleanupChannelCloseTest(void)
{
#ifdef EVENT_ENABLE
    ClRcT rc = 0;

    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        clEvtSubsTestDeliverCallback
    };
    ClVersionT version = CL_EVENT_VERSION;
    
    ClEventChannelHandleT evtChannelHandleLocal, evtChannelHandleGlobal;

    ClNameT channelC = {sizeof("ChannelC")-1,"ChannelC"};
    ClNameT channelD = {sizeof("ChannelD")-1,"ChannelD"};
    
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
        return rc;
    }

    /*** For Local Channel ***/
    
    rc = clEventChannelOpen(evtHandle, &channelC, 
            CL_EVENT_CHANNEL_SUBSCRIBER, (ClTimeT)-1, &evtChannelHandleLocal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleLocal, EVENT_TEST_EVENT_TYPE, X, (void*)0x100);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventSubscribe(evtChannelHandleLocal, &gSubscribeFilters, Y, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    /*** For Global Channel ***/

    rc = clEventChannelOpen(evtHandle, &channelD, 
            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandleGlobal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleGlobal, EVENT_TEST_EVENT_TYPE, Z, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }


    /*** Channel Close ***/

    rc = clEventChannelClose(evtChannelHandleLocal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Close Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventChannelClose(evtChannelHandleGlobal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Close Failed [0x%X]\n\r",rc));
        return rc;
    }    
    
    printf("Channel Close Successful!!!\n");

#endif

    return CL_OK;
}
/************************* Cleanup Unsubscribe Test *******************************/


ClRcT   clEvtCleanupUnsubscribeTest(void)
{
#ifdef EVENT_ENABLE
    ClRcT rc = 0;

    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        clEvtSubsTestDeliverCallback
    };
    ClVersionT version = CL_EVENT_VERSION;
    
    ClEventChannelHandleT evtChannelHandleLocal, evtChannelHandleGlobal;

    ClNameT channelE = {sizeof("ChannelE")-1,"ChannelE"};
    ClNameT channelF = {sizeof("ChannelF")-1,"ChannelF"};
    
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
/*        return rc;   */
    }

    /*** For Local Channel ***/
    
    rc = clEventChannelOpen(evtHandle, &channelE, 
            CL_EVENT_CHANNEL_SUBSCRIBER, (ClTimeT)-1, &evtChannelHandleLocal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleLocal, EVENT_TEST_EVENT_TYPE, X, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }

    /*** For Global Channel ***/

    rc = clEventChannelOpen(evtHandle, &channelF, 
            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandleGlobal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventSubscribe(evtChannelHandleGlobal, &gSubscribeFilters, Y, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleGlobal, EVENT_TEST_EVENT_TYPE, Z, (void*)0x100);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }


    /*** Unsubscribe ***/

    rc = clEventUnsubscribe(evtChannelHandleLocal, X);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscribe Failed [0x%X]\n\r",rc));
        return rc;
    }    

    rc = clEventUnsubscribe(evtChannelHandleGlobal, Y);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscribe Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventUnsubscribe(evtChannelHandleGlobal, Z);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscribe Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    printf("Unsubscribe Successful!!!\n");
    
#endif

    return CL_OK;
}

/**********************************************************************/

#endif
