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
 * $File: //depot/dev/RC2/R2.2-Mercury/ASP/components/event/test/unit-test/evtIntTest/subs/clEvtSubsMain.c $
 * $Author: mayank $
 * $Date: 2007/05/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <unistd.h>
#include <string.h>
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
#include "clOmApi.h"
#include "clOampRtApi.h"
#include <sys/time.h>
#if 1
#define EVENT_ENABLE
#endif

#if 0
#define OAMP_ENABLE
#endif

#if 0
#define DIAG_ENABLE
#include "clDmApi.h"
#include "clDmSwPlPxApi.h"
#endif


ClEoExecutionObjT* gEvtSubsEoObj;

/* This is for OM linker error */
ClOmClassControlBlockT * pAppOmClassTbl = NULL;
ClUint32T appOmClassCnt = 0;

/* @ */
#if 0
#define NEW_TEST_ENABLE

#define LOCAL_CHANNEL "Clovis Local Channel"
#define GLOBAL_CHANNEL "Clovis Global Channel"

#define X 1
#define Y 2
#define Z 3

static ClEventChannelHandleT evtChannelHandleLocal, evtChannelHandleGlobal;

#endif

static ClEventChannelHandleT evtChannelHandle;   /* Moved here for the cleanup in callback */


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


static ClEoPayloadWithReplyCallbackT gClEvtSubsTestFuncList[] =
{

    (ClEoPayloadWithReplyCallbackT) NULL,                          /* 0 */
};

#define EVENT_TEST_PAYLOAD "EVENT_TEST_PAYLOAD"

#include <time.h>
#include <stdarg.h>
ClRcT
clOsalTimeStampedPrintf(const ClCharT* fmt, ...)
{
    ClInt32T result;
    va_list arguments;

    time_t t;
    struct tm t2;
    char timebuf[50];
    int size;

    va_start(arguments, fmt);

    t = time(NULL);
    localtime_r(&t, &t2);
    size = strftime(timebuf, sizeof(timebuf), "[%b %d %H:%M:%S]", &t2);
    printf("%s", timebuf);

    result = vfprintf(stdout, fmt, arguments);
    va_end(arguments);
    
    if(0 > result)
    {
        return(0); 
    }
    else
    {
        return(CL_OK);
    }
}


void clEvtSubsTestDeliverCallback( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClEventPriorityT priority = 0;
    ClTimeT  retentionTime = 0;
    SaNameT  publisherName = {0};
    ClEventIdT eventId = 0;
    ClRcT rc = CL_OK;
    static ClUint64T callbackCount;
    
    ClUint8T payLoad[50] = {0};
    ClSizeT payLoadLen = sizeof(payLoad);
    
    ClEventPatternArrayT patternArray = {0};
    ClTimeT  publishTime = 0;
    void *pCookie = NULL;
    
    rc = clEventCookieGet(eventHandle, &pCookie);

    rc = clEventAttributesGet( eventHandle, 
            &patternArray, &priority,
            &retentionTime, &publisherName,
            &publishTime, &eventId );

    rc = clEventDataGet (eventHandle, (void*)payLoad,  &payLoadLen);

  //   sleep(1);
    callbackCount++;
    clOsalPrintf("-------------------------------------------------------\n");
    clOsalTimeStampedPrintf("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n");
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
//     clOsalPrintf("             No. of times      : %d\n", callbackCount);
    clOsalPrintf("-------------------------------------------------------\n");
            
#ifdef NEW_TEST_ENABLE
    {
        /* For Ext Attribute Get Coverage */
    
        ClUint32T eventType = 0;

        rc = clEventExtAttributesGet(eventHandle, 
            &eventType, &priority,
            &retentionTime, &publisherName,
            &publishTime, &eventId );

        clOsalPrintf("-------------------------------------------------------\n");
        clOsalPrintf("!!!!!!!!!!!!! Ext Event Delivery Callback !!!!!!!!!!!!!\n");
        clOsalPrintf("-------------------------------------------------------\n");
        clOsalPrintf("             Subscription ID   : 0x%X\n", subscriptionId);
        clOsalPrintf("             Event Priority    : 0x%X\n", priority);
        clOsalPrintf("             Publish Time      : 0x%X\n", publishTime);
        clOsalPrintf("             Retention Time    : 0x%X\n", retentionTime);
        clOsalPrintf("             Publisher Name    : %.*s\n", publisherName.length, publisherName.value);
        clOsalPrintf("             EventID           : 0x%X\n", eventId);
        clOsalPrintf("             Payload           : %s\n", payLoad);
        clOsalPrintf("             Payload Len       : 0x%X\n", (ClUint32T)payLoadLen);
        clOsalPrintf("             Event Data Size   : 0x%X\n", (ClUint32T)eventDataSize);
        clOsalPrintf("             Cookie            : 0x%X\n", pCookie);
        clOsalPrintf("-------------------------------------------------------\n");

    }
    
    if(strncmp("CLEANUP",publisherName.value, publisherName.length)) return;
 
    printf("\nUnsubscriptions Starting...\n");
 
    rc = clEventUnsubscribe(evtChannelHandleLocal, X);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext Unsubscription Failed [0x%X]\n\r",rc));
    }
    
    rc = clEventUnsubscribe(evtChannelHandleLocal, Y);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Normal Unsubscription Failed [0x%X]\n\r",rc));
    }

    rc = clEventUnsubscribe(evtChannelHandleLocal, Z);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Unsubscriptions Failed [0x%X]\n\r",rc));
    }


    rc = clEventUnsubscribe(evtChannelHandleGlobal, X);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandleGlobal, Y);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Noraml unsubscription Failed [0x%X]\n\r",rc));
    }
 
    rc = clEventUnsubscribe(evtChannelHandleGlobal, Z);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE unsubscription Failed [0x%X]\n\r",rc));
    }
    
    printf("\nUnsubscriptions Successful!\n");

#else
/*
    if(strncmp("CLEANUP",publisherName.value, publisherName.length)) return;

    printf("\nUnsubscriptions Starting...\n");
    
    rc = clEventUnsubscribe(evtChannelHandle, 1);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandle, 2);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }

    printf("\nUnsubscriptions Successful!\n");
    */
#endif            
    
    return;
}

void clDmUtilsMemPrint(ClUint32T* buff, ClUint32T len)
{
    ClUint32T i =0;
    len = len/sizeof(ClUint32T);
    
    for(i = 0; i<len; i++)
    {
        if(i%4 == 0)
        {
            clOsalPrintf("\n");
        }
        clOsalPrintf("0x%X ",*buff);
        buff++;
    }
    clOsalPrintf("\n");
}
#define TEST_NAME "TEST_NAME"
#define TEST_PARAMS "TEST_PARAMS"
ClRcT clEvtSubsTest()
{
    ClRcT rc = 0;
#ifdef EVENT_ENABLE
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        clEvtSubsTestDeliverCallback
    };
    ClVersionT version = CL_EVENT_VERSION;    
#endif
#ifdef DIAG_ENABLE
    SaNameT testName = {sizeof(TEST_NAME), TEST_NAME};
    ClDmInstHandleT testInst = 0;
    ClDmDestHandleT destHandle = 0;
    ClDmProgIndicatorT diagProgIndicator;
    ClDmDestAddrT destAddr[] = 
        {
            {SAHPI_ENT_SW_COMPONENT_ID,0},
            {SAHPI_ENT_SW_COMPONENT,1},
            {SAHPI_ENT_SBC_BLADE,2},
            {SAHPI_ENT_PHYSICAL_SLOT,2},
            {SAHPI_ENT_ATCA_CHASSIS,0},
            {SAHPI_ENT_ROOT,2},
        };    
    ClDmNumTestsT noOfTest = 2;
    ClDmPropStructT diagProp;
    ClDmEventIdT eventId;
    ClDmEventResultsT eventBuf;
    ClTimeT timeOut = -1;
    ClInt8T diagTestParms[CL_DM_TEST_PARMSLEN] = {TEST_PARAMS};
#endif
#ifdef OAMP_ENABLE
    ClInt8T rtFileName[10] = {"rt.xml"};
    ClInt8T compName[20] = {"Comp_0"};
    rc = CL_OK;

    
    rc = clOampRtInitialize();
    rc = clOampRtResourceInfoGet(rtFileName, compName);
#endif    
#ifdef DIAG_ENABLE
    rc = clDmLibraryInitialize();
    clOsalPrintf("***********************************************\r\n");    
    clOsalPrintf("clDmLibraryInit Done : [0x%X]\r\n",rc);
    clOsalPrintf("______________________________________________\r\n\r\n");
    
    rc = clDmSwCompPluginProxyInitialize();
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmSwCompPluginProxyInit Done : [0x%X]\r\n",rc);
    clOsalPrintf("______________________________________________\r\n\r\n");

    rc = clDmDestHandleCreate(destAddr, &destHandle);
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmDestHandleCreate Done DestHandle : [0x%X] : [0x%X]\r\n",rc,destHandle);
    clOsalPrintf("______________________________________________\r\n\r\n");

    rc = clDmTestsEnumerate(destHandle, &noOfTest, &diagProp);
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmTestsEnumerate Done DestHandle : [0x%X]\r\n",rc);
    clOsalPrintf("noOfTests [0x%X]\r\n",noOfTest);
    clDmUtilsMemPrint((ClUint32T*)&diagProp,32);
    clOsalPrintf("______________________________________________\r\n\r\n");

    rc = clDmTestStart(destHandle, testName, diagTestParms,&testInst);
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmTestStart Done DestHandle : [0x%X]\r\n",rc);
    clOsalPrintf("noOfTests [0x%X]\r\n",noOfTest);
    clOsalPrintf("______________________________________________\r\n\r\n");

    rc = clDmEventGet(destHandle, &eventId, &eventBuf, timeOut);
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmEventGet Done : [0x%X]\r\n",rc);
    clOsalPrintf("EventID [0x%X]\r\n",eventId);
    clDmUtilsMemPrint((ClUint32T*)&eventBuf,32);
    clOsalPrintf("______________________________________________\r\n\r\n");

    rc = clDmTestProgressGet(destHandle, testInst, &diagProgIndicator);
    clOsalPrintf("***********************************************\r\n");
    clOsalPrintf("clDmTestProgressGet Done : [0x%X]\r\n",rc);
    clDmUtilsMemPrint((ClUint32T*)&diagProgIndicator,32);
    clOsalPrintf("______________________________________________\r\n\r\n");
#endif    

#ifdef EVENT_ENABLE
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Init Failed [0x%X]\n\r",rc));
        return rc;
    }

    rc = clEventChannelOpen(evtHandle, &gEvtTestChannelName, CL_EVENT_CHANNEL_SUBSCRIBER, (ClTimeT)-1, &evtChannelHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventSubscribe(evtChannelHandle, &gSubscribeFilters, 0, (void*)0x000);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    /*
    rc = clEventExtSubscribe(evtChannelHandle, EVENT_TEST_EVENT_TYPE, 1, (void*)0x100);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    rc = clEventSubscribe(evtChannelHandle, &gSubscribeFilters, 2, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
	
     
    getchar();

    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    */
#if 0
    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
    rc = clEventUnsubscribe(evtChannelHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unsubscription Failed [0x%X]\n\r",rc));
    }
#endif

#endif    
    return CL_OK;
}


#ifdef NEW_TEST_ENABLE
ClRcT   clEvtSubscriptionTest(void);
#endif

static ClCpmHandleT gClEvtSubsCpmHandle; /* FIXME */
ClRcT clEventSubsTerminate(ClInvocationT invocation,
			const SaNameT  *compName)
{
    ClRcT rc;

    rc = clCpmComponentUnregister(gClEvtSubsCpmHandle, compName, NULL);
    rc = clCpmClientFinalize(gClEvtSubsCpmHandle);

    clCpmResponse(gClEvtSubsCpmHandle, invocation, CL_OK);

    return CL_OK;
}


ClRcT clEvtSubsCpmInit()
{
    SaNameT appName;
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

ClRcT clEvtSubInitialize(ClUint32T argc, ClCharT *argv[])
{

    ClRcT rc = CL_OK;

#if 0
    ClSvcHandleT svcHandle = {0};
    ClOsalTaskIdT taskId = 0;
#endif

    rc = clEoMyEoObjectGet(&gEvtSubsEoObj);
                                                                                
    rc = clEoClientInstall(gEvtSubsEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,gClEvtSubsTestFuncList, 0,
                                          (int)(sizeof(gClEvtSubsTestFuncList)/sizeof (ClEoPayloadWithReplyCallbackT)));
    if( rc != CL_OK )
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Installing Native table failed [0x%X]\n\r",rc));
    }
    
    clEvtSubsCpmInit(); 

#if 0
    svcHandle.cpmHandle = &gClEvtSubsCpmHandle;
    rc = clDispatchThreadCreate(gEvtSubsEoObj, &taskId, svcHandle);
#endif

#ifdef NEW_TEST_ENABLE 
    clEvtSubscriptionTest();

#else
    printf("******** Doing subscription testing *****\n");
    clEvtSubsTest();
#endif
    
    return CL_OK;
    
}


ClRcT clEvtSubFinalize()
{
    ClRcT rc = CL_OK;
    rc = clEoClientUninstall(gEvtSubsEoObj,CL_EO_NATIVE_COMPONENT_TABLE_ID);
    return CL_OK;
}


ClRcT   clEvtSubStateChange(ClEoStateT eoState)
{
    
    return CL_OK;
}

/* 
 * This function will be called periodically to check the EO health. User need 
 * to fill in the structure as per it health
 */

ClRcT   clEvtSubHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    
       schFeedback->freq = CL_EO_BUSY_POLL;
       schFeedback->status = 0;
       return CL_OK;
}


ClEoConfigT clEoConfig = {
    "EVENT_SUB_TEST_EO",       /* EO Name*/
    1,              /* EO Thread Priority */
    9,              /* No of EO thread needed */
    CL_IOC_EVENT_PORT+101,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    clEvtSubInitialize,  /* Function CallBack  to initialize the Application */
    clEvtSubFinalize,    /* Function Callback to Terminate the Application */ 
    clEvtSubStateChange, /* Function Callback to change the Application state */
    clEvtSubHealthCheck, /* Function Callback to change the Application state */
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

/*** New Test Case Begins Here ***/


ClRcT   clEvtSubscriptionTest(void)
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
    
    SaNameT gEvtECHNameLocal    = {sizeof(LOCAL_CHANNEL)-1, LOCAL_CHANNEL};
    SaNameT gEvtECHNameGlobal   = {sizeof(GLOBAL_CHANNEL)-1, GLOBAL_CHANNEL};

    ClRuleExprT* pRbeExpr = NULL;

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("My Init Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    
    /*** For Local Channel ***/

    rc = clEventChannelOpen(evtHandle, &gEvtECHNameLocal, 
            CL_EVENT_CHANNEL_SUBSCRIBER, (ClTimeT)-1, &evtChannelHandleLocal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleLocal, EVENT_TEST_EVENT_TYPE, X, (void*)0x900);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext Subscription Failed [0x%X]\n\r",rc));
        return rc;
    }
    rc = clEventSubscribe(evtChannelHandleLocal, &gSubscribeFilters, Y, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Normal Subscription Failed [0x%X]\n\r",rc));
        return rc;
    }

#if 0
    /* RBE Based Subscription */
    
    rc = clEventExtWithRbeSubscribe(evtChannelHandleLocal, NULL, Z, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Subscriptions Failed [0x%X]\n\r",rc));
/*        return rc;  For better Code Coverage */
    }
#endif

    /*** For Global Channel ***/

    rc = clEventChannelOpen(evtHandle, &gEvtECHNameGlobal, 
            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, 
            (ClTimeT)-1, &evtChannelHandleGlobal);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Channel Open Failed [0x%X]\n\r",rc));
        return rc;
    }
    
    rc = clEventExtSubscribe(evtChannelHandleGlobal, EVENT_TEST_EVENT_TYPE, X, (void*)0x500);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Ext Subscription Failed [0x%X]\n\r",rc));
        return rc;
    }
    rc = clEventSubscribe(evtChannelHandleGlobal, &gSubscribeFilters, Y, (void*)0x200);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Noraml Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }

    /* RBE Based Subscription */
    
    rc = clRuleExprAllocate(1, &pRbeExpr);
    if(NULL == pRbeExpr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Allocate Failed [0x%X]\n\r",rc));
    }
    
    rc = clEventExtWithRbeSubscribe(evtChannelHandleGlobal, pRbeExpr, Z, (void*)0x300);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("RBE Subscriptions Failed [0x%X]\n\r",rc));
        return rc;
    }
    clRuleExprDeallocate(pRbeExpr);
    
#endif

    return CL_OK;
}

#endif
