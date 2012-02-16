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
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/eventUnitTest/clEvtUnitTest.c $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include "clEoApi.h"

#include "clEventApi.h"
#include "clEventExtApi.h"
#include "clCpmApi.h"

#include <clOsalApi.h>
#include "ctype.h"

#include "clDebugApi.h"

#if 0
# include <ipi/clSAClientSelect.h>  /* SAF Changes */
#endif

FILE *gpEvtTestFp = NULL;
ClUint32T *gpEvtTestResult = NULL;
ClUint32T gEvtTestResultCount = 0;

char *gEvtCodeCovFileNameList[] = {
    "clEvtClientMain.c",
    NULL,
};
ClRcT (*fpCodeCovFilesRegisterCallout) (void **);
void clEvtChannelOpenAsyncTestWrapper(ClEventInitHandleT evtHandle,ClInvocationT invocation, 
        	  ClNameT *channelName, ClEventChannelOpenFlagsT evtChannelOpenFlag);
void clEvtChannelOpenCallback(ClInvocationT invocation,ClEventChannelHandleT evtCHhandle,ClRcT rc);



ClRcT clEvtCodeCovFileNameFunction(void **pThis)
{
    *pThis = (void *) gEvtCodeCovFileNameList;
    return CL_OK;
}
extern ClRcT clCodeCovInit();
extern ClRcT clCodeCovEnable();

void clEvtTestStart()
{
    fpCodeCovFilesRegisterCallout = clEvtCodeCovFileNameFunction;
#ifdef MORE_CODE_COVERAGE
    clCodeCovInit();
    clCodeCovEnable();
#endif

    return;
}

int clEvtUnitTestInit()
{
    clEvtTestStart();

    gpEvtTestFp = fopen("evtTestResult.txt", "w");
    if (NULL == gpEvtTestFp)
    {
        clOsalPrintf("Error while opening file %x", gpEvtTestFp);
        return -1;
    }
    return 0;
}

void clEvtTestResultPrint(ClRcT testResult)
{
//    clEvtUnitTestInit();
    if(gpEvtTestFp != NULL)
    {
        
        fprintf(gpEvtTestFp, "TEST CASE -- %d : ", gEvtTestResultCount);

        if (testResult == gpEvtTestResult[gEvtTestResultCount])
        {
            fprintf(gpEvtTestFp, "PASSED\n");
        }
        else
        {
            fprintf(gpEvtTestFp, "FAILED [Expected 0x%x != Obtained 0x%x]\n",
                    gpEvtTestResult[gEvtTestResultCount], testResult);
        }
    }
    gEvtTestResultCount++;
  //  fclose(gpEvtTestFp);
    return;
}

void clEvtUnitTestAttributeSet(char *pFunctioName, ClUint32T *pTestResult)
{
    //clEvtUnitTestInit();
    fprintf(gpEvtTestFp, "-----------------------------\n");
    fprintf(gpEvtTestFp, " %s\n", pFunctioName);
    fprintf(gpEvtTestFp, "-----------------------------\n");

    gpEvtTestResult = pTestResult;
    gEvtTestResultCount = 0;
    //fclose(gpEvtTestFp);
    return;
}

#define EVT_TEST_CHANNEL_NAME "TestChannel"
#define EVT_TEST_PUBLISHER_NAME "TestPublisher"

/*
 * Filter and pattern information 
 */
#define gPatt1 "Filter pattern 11"
#define gPatt1_size sizeof(gPatt1)

#define gPatt2 "Filter pattern 22"
#define gPatt2_size sizeof(gPatt2)

#define gPatt3 "Filter pattern 33"
#define gPatt3_size sizeof(gPatt3)

#define gPatt4 "Filter pattern 44"
#define gPatt4_size sizeof(gPatt4)

#define gSubPatt1   "Filter "
#define gSubPatt2   "Filter pattern 11"

static ClEventFilterT gFilters[] = {
    {CL_EVENT_EXACT_FILTER, {0, gPatt1_size, (ClUint8T *) gPatt1}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt2_size, (ClUint8T *) gPatt2}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt3_size, (ClUint8T *) gPatt3}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt4_size, (ClUint8T *) gPatt4}},
};

static ClEventFilterArrayT gSubscribeFilters = {
    sizeof(gFilters) / sizeof(ClEventFilterT),
    gFilters
};

static ClEventFilterT gPrefixFilter[] = 
{   {CL_EVENT_EXACT_FILTER,	{0,sizeof(gSubPatt1),(ClUint8T *)gSubPatt1}},
    {CL_EVENT_EXACT_FILTER,	{0,sizeof(gSubPatt2),(ClUint8T *)gSubPatt2}},  
    {CL_EVENT_PASS_ALL_FILTER,	{0,sizeof(gSubPatt1),(ClUint8T *)gSubPatt1}},
    {CL_EVENT_PASS_ALL_FILTER,	{0,sizeof(gSubPatt2),(ClUint8T *)gSubPatt2}},
};

static ClEventFilterArrayT gSubscribePrefixFilter1 = {0};
static ClEventPatternT gPatterns[] = {
    {0, gPatt1_size, (ClUint8T *) gPatt1},
    {0, gPatt2_size, (ClUint8T *) gPatt2},
    {0, gPatt3_size, (ClUint8T *) gPatt3},
    {0, gPatt4_size, (ClUint8T *) gPatt4},
};

static ClEventPatternArrayT gPatternArray = {
    0,
    sizeof(gPatterns) / sizeof(ClEventPatternT),
    gPatterns
};

ClUint32T gEvtDataGetTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE, /* Test Case 0, Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,    /* Test Case 1, Shanthi - NTC */
    CL_OK,                      /* Test Case 2 */
    CL_EVENT_ERR_BAD_HANDLE,    /* Test Case 3 */

#if 0
    CL_EVENT_ERR_INVALID_PARAM,

    /*
     * CL_EVT_ERR_EXIST, 
     */

    CL_OK                       /* , Mynk - NTI CL_EVENT_ERR_BAD_HANDLE */
#endif
};

/*
void clEvtChannelOpenCallback(ClInvocationT invocation,
                              ClEventChannelHandleT channelHandle, ClRcT error)
{
    return;
}
*/

ClEventPriorityT tPriority = 0;
ClTimeT tRetentionTime = 0;

ClNameT tPublisherName = { sizeof(EVT_TEST_PUBLISHER_NAME) - 1,
    EVT_TEST_PUBLISHER_NAME
};
ClTimeT tPublishTime;
ClEventIdT tEventId;
ClSizeT tPayLoadLen = 0;
ClUint32T tPayLoad = 0; 
ClUint32T tDeliveryFlag = 0;

ClEventHandleT gEvtEventHandle, gEvtChannelHandle, gEvtHandle;

void clEvtDataGetTestWrapper(ClEventHandleT eventHandle, void *pEventData,
                             ClSizeT *pEventDataSize);

void clEvtDeliverCallback(ClEventSubscriptionIdT subscriptionId,
                          ClEventHandleT eventHandle, ClSizeT eventDataSize)
{
    ClRcT rc = CL_OK;
    printf("*********** In Delivery Callback *******");
#if 0
    ClEventPatternArrayT patternArray;
    ClEventPriorityT priority;
    ClTimeT retentionTime;
    ClNameT publisherName = {0};
    ClTimeT publishTime;
    ClEventIdT eventId;
    ClSizeT payLoadLen = sizeof(ClUint32T);
    ClUint32T payLoad = 0;

    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
                              &retentionTime, &publisherName, &publishTime,
                              &eventId);


    rc = clEventDataGet(eventHandle, (void *) &payLoad, &payLoadLen);

    clOsalPrintf
        ("!!!!!!!!!!!!!!!!!!!!!!!!!!With Normal Get!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    clOsalPrintf("Subscription Id : %X\r\n", subscriptionId);
    clOsalPrintf("Event Priority  : %X\r\n", priority);
    clOsalPrintf("Retention Time  : %X\r\n", retentionTime);
    clOsalPrintf("Publisher Name  : %s\r\n", publisherName.value);
    clOsalPrintf("EventId         : %X\r\n", eventId);
    clOsalPrintf("Payload         : %X\r\n", payLoad);
    clOsalPrintf("Payload Len     : %X\r\n", (ClUint32T) payLoadLen);
    clOsalPrintf
        ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
#endif

    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    ClNameT publisherName = { 0 };
    ClEventIdT eventId = 0;
    ClUint32T payLoad = 0;
    ClSizeT payLoadLen = sizeof(payLoad);
    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    void *pCookie = NULL;

    tDeliveryFlag = 1;
    rc = clEventDataGet(eventHandle, (void *) &payLoad, &payLoadLen);

    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
                              &retentionTime, &publisherName, &publishTime,
                              &eventId);

    rc = clEventCookieGet(eventHandle, &pCookie);

    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n");
    clOsalPrintf("-------------------------------------------------------\n");
    clOsalPrintf("             Subscription ID   : 0x%X\n", subscriptionId);
    clOsalPrintf("             Event Priority    : 0x%X\n", priority);
    clOsalPrintf("             Retention Time    : 0x%X\n", retentionTime);
    clOsalPrintf("             Publisher Name    : %.*s\n",
                 publisherName.length, publisherName.value);
    clOsalPrintf("             EventID           : 0x%X\n", eventId);
    clOsalPrintf("             Payload           : 0x%X\n", payLoad);
    clOsalPrintf("             Payload Len       : 0x%X\n",
                 (ClUint32T) payLoadLen);
    clOsalPrintf("             Event Data Size   : 0x%X\n",
                 (ClUint32T) eventDataSize);
    clOsalPrintf("             Cookie            : 0x%X\n", pCookie);
    clOsalPrintf("             No. of Pattern    : 0x%X\n",patternArray.patternsNumber);
    clOsalPrintf("-------------------------------------------------------\n");

    /*
     ** Publish results in the invocation of cb where GetAttribs & GetData is called
     ** and the values are tested below.
     */
    if(subscriptionId == 0x111 || subscriptionId == 0x112 || subscriptionId == 0x113 || subscriptionId == 0x114
            || subscriptionId == 0x115 || subscriptionId == 0x116)
    {
        if(payLoad == 0x1234 && payLoad == tPayLoad)
        {
            clOsalPrintf("Subscription Id :: %x << PASS >>\r\n",subscriptionId);
            //clEvtTestResultPrint(CL_OK);
        }
        else
        {
            clOsalPrintf("Subscription Id :: %x << FAIL >>\r\n",subscriptionId);
            //clEvtTestResultPrint(-1);
        }
    }

    else if(subscriptionId == 0x123)
    {
        if(payLoad == 0x1234 && payLoad == tPayLoad)
        {
            clOsalPrintf("Subscription Id :: %x << PASS >>\r\n",subscriptionId);
            //clEvtTestResultPrint(CL_OK);
        }
        else
        {
            clOsalPrintf("Subscription Id :: %x << FAIL >>\r\n",subscriptionId);
            //clEvtTestResultPrint(-1);
        }
    }
    else
    {
	if(publisherName.value != NULL)
	{

        	if (tPayLoad == payLoad && !strncmp(publisherName.value, tPublisherName.value,
                	 publisherName.length) && priority == tPriority && retentionTime == tRetentionTime)
        	{
            		/*
            		* Test Case 0 - All fine, returns CL_OK 
            		*/
            		clOsalPrintf("<< PASS >>\r\n");
            		//clEvtTestResultPrint(CL_OK);
        	}
        	else
        	{
            		clOsalPrintf("<< FAIL >>\r\n");
            		//clEvtTestResultPrint(-1);   /* Test Failed */
        	}
	}
    }

    /*
     * Test Case 1 - All 0s Invalid, returns CL_EVENT_ERR_BAD_HANDLE 
     */
    //clEvtDataGetTestWrapper(0, NULL, 0);


    clEventFree(eventHandle);

    clEventFree(gEvtEventHandle);   /* Event Free */

    clEventChannelClose(gEvtChannelHandle); /* Close Channel */
    //if(subscriptionId == 0xABCD)
    //    fclose(gpEvtTestFp);

//    clEventFinalize(gEvtHandle);


#if 0

    Mynk - NTI Event free
#endif
        return;
}

static ClEventCallbacksT gEvtCallbacks = {
    clEvtChannelOpenCallback,                       /* clEvtChannelOpenCallback, */
    clEvtDeliverCallback
};
static ClEventCallbacksT gEvtCallbacks1 = {
    NULL,                       /* clEvtChannelOpenCallback, */
    clEvtDeliverCallback
};

/*********************************************************************
                     Event Initialize Testing
**********************************************************************/
void clEvtInitializeCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;

    while (1)
    {
        clEventInitialize(&evtHandle, &evtCallbacks, &version);
        clEventFinalize(evtHandle);
        if (CL_OK == rc)
        {
            break;
        }
    }
}

void clEvtInitializeTestWrapper(ClEventInitHandleT *pEvtHandle,
                                const ClEventCallbacksT *pEvtCallbacks,
                                ClVersionT *pVersion)
{
    ClRcT rc = CL_OK;

    rc = clEventInitialize(pEvtHandle, pEvtCallbacks, pVersion);
    clEvtTestResultPrint(rc);
    if (CL_OK == rc)
    {
        rc = clEventFinalize(*pEvtHandle);
        if(rc != CL_OK)
            printf("In Initialize test, clEventFinalize():: %d",rc);
    }
    return;
}

ClUint32T gEvtInitTestResult[] = {
    CL_EVENT_ERR_INVALID_PARAM,
    CL_EVENT_ERR_INVALID_PARAM,
    CL_OK,
    CL_EVENT_ERR_VERSION,   /* Shanthi - NTC */
    CL_OK,                  /* Shanthi - NTC */
    CL_OK,
    CL_OK   /* Shanthi - NTC */
};
#define CL_EVT_LESS_VERSION {(ClUint8T)'A',0x0,0x0}
#define CL_EVT_LESS_VERSION1 {(ClUint8T)'B',0x0,0x0}
ClRcT clEvtInitializeTest()
{
    ClRcT rc = CL_OK;
    
    ClEventInitHandleT evtHandle, evtHandleII;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClVersionT version1 = CL_EVT_LESS_VERSION;
    ClVersionT version2 = CL_EVT_LESS_VERSION1;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtInitTestResult);

    /***** Code coverage testing *****/
    clEvtInitializeCodeCoveTest();

    /*
     * Invalid test cases 
     */
    clEvtInitializeTestWrapper(NULL, NULL, NULL);
    clEvtInitializeTestWrapper(&evtHandle, &evtCallbacks, NULL);
    clEvtInitializeTestWrapper(&evtHandle, NULL, &version);
    

    /***** Tests for Version *****/
    /* Version no. Release code, major & minor versions are not equal */
    clEvtInitializeTestWrapper(&evtHandle,&evtCallbacks,&version1);
   
    //memcpy(&version1,CL_EVT_LESS_VERSION1,sizeof(CL_EVT_LESS_VERSION1));    
    /* Version no. Release code is equal, but major version is not equal */
    clEvtInitializeTestWrapper(&evtHandle,&evtCallbacks,&version2);
    printf("************ Major Version no.%d, Minor Version No.: %d ",version2.majorVersion,version2.minorVersion);
    
    /*** Valid test cases **/
    clEvtInitializeTestWrapper(&evtHandle, &evtCallbacks, &version);

    /*
    ** Valid Test case: Calling init twice
    */
    rc = clEventInitialize(&evtHandle,&evtCallbacks,&version);
    if(rc == CL_OK)
        rc = clEventInitialize(&evtHandleII,&evtCallbacks,&version);
    clEvtTestResultPrint(rc);    

    clEventFinalize(evtHandle);
    clEventFinalize(evtHandleII);
    
    return CL_OK;
}

/*********************************************************************
                     Event Finalize Testing
**********************************************************************/
#if 0

Mynk - TBD void clEvtFinalizeCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;

    while (1)
    {
        rc = clEvtInitinalizeClient(&evtHandle, &evtCallbacks, &version);

        rc = clEventFinalize(evtHandle);
        if (CL_OK == rc)
        {
            break;
        }
    }
}
#endif

void clEvtFinalizeTestWrapper(ClEventInitHandleT evtHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventFinalize(evtHandle);
    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtFinalizeTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE, /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,
    CL_OK                       /* , CL_EVENT_ERR_BAD_HANDLE Mynk - NTC */
};

ClRcT clEvtFinalizeTest(void)
{    
    ClEventInitHandleT evtHandle = 0;
    const ClEventCallbacksT evtCallbacks = { 0 };
    ClVersionT version = CL_EVENT_VERSION;
//    ClRcT rc = CL_OK;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtFinalizeTestResult);

    /***** Code coverage testing *****/
    /*
     * Mynk - clEvtFinalizeCodeCoveTest(); 
     */

    /*
    ** Invalid test case - Calling clEventFinalize() before doing init
    */    
    clEvtFinalizeTestWrapper(evtHandle);


    
    clEventInitialize(&evtHandle, &evtCallbacks, &version);

    /*
     * Invalide test case - Passing NULL 
     */
    clEvtFinalizeTestWrapper((ClEventInitHandleT) NULL);

    /*
     * Valide test case(s) 
     */
    clEvtFinalizeTestWrapper(evtHandle);

    /*
    ** Invalid test case - Finalize on Finalized Handle
    ** clEvtFinalizeTestWrapper(evtHandle); //Mynk - NTC 
    */
    return CL_OK;
}

/*********************************************************************
                     Event Channel Open Testing
**********************************************************************/
#if 0
void clEvtOpenCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtChannelOpenTestWrapper(ClEventInitHandleT evtHandle,
                                 const ClNameT *pChannelName,
                                 ClEventChannelOpenFlagsT evtChannelOpenFlag,
                                 ClTimeT timeout,
                                 ClEventChannelHandleT *pEvtChannelHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventChannelOpen(evtHandle, pChannelName, evtChannelOpenFlag,
                            timeout, pEvtChannelHandle);
    clEvtTestResultPrint(rc);

    if (CL_OK == rc)
    {
        clEventChannelClose(*pEvtChannelHandle);
    }

    return;
}

ClUint32T gEvtChannelOpenTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,         /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,
    CL_EVENT_ERR_NULL_PTR,
    CL_EVENT_ERR_BAD_FLAGS,
    CL_EVENT_ERR_INVALID_PARAM,
    CL_OK,
    CL_OK,                              /* Mynk - NTI CL_EVENT_ERR_BAD_HANDLE */
    CL_EVENT_ERR_CHANNEL_ALREADY_OPENED/* Shanthi - NTC */
   // CL_EVENT_ERR_CHANNEL_ALREADY_OPENED /* Shanthi - NTC */
    
};

ClRcT clEvtChannelOpenTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle = CL_OK;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
//    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    //ClInvocationT invocation = 100;

    /*** Set the attribute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtChannelOpenTestResult);

    /*
    ** Invalid Test Case: Calling clEventChannelOpen() before doing init
    ** should return CL_EVENT_ERR_INIT_NOT_DONE 
    */
    clEvtChannelOpenTestWrapper(evtHandle, &channelName, evtChannelOpenFlag, timeout,
                                &evtChannelHandle);


    /***** Code coverage testing ****
    clEvtOpenCodeCoveTest();  -Mynk NTI */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);

    /*
    ** Invalid Test Case: All 0s Invalid, returns CL_EVENT_ERR_BAD_HANDLE 
    */
    clEvtChannelOpenTestWrapper(0, &channelName, 0x10, timeout,
                                &evtChannelHandle);

#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALID_PARAM, NO_MEMORY,
        NO_RESOURCES, BAD_FLAGS, NOT_EXIST(-NOT NEEDED)
        /*
         * Invalid test case 2 - Invalid Handle, returns
         * CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtChannelOpenTestWrapper(1234, &channelName, 0, 0, NULL);

    /*
     * Invalid test case: Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtChannelOpenTestWrapper(? ? ?, &channelName, 0, 0, NULL);
#endif

    /*
    ** Invalid Test Case: ChannelName is NULL Inavlid, returns CL_EVENT_ERR_NULL_PTR 
    ** - Mynk NTC 
    */
    clEvtChannelOpenTestWrapper(evtHandle, NULL, 0, timeout, &evtChannelHandle);

    /*
    ** Invalid Test Case: Flag 0 Invalid, returns CL_EVENT_ERR_BAD_FLAGS 
    */
    
    /*
     * (X)<=0xf) - Mynk 0 is meaningless ??? Finer testing later 
     */
    clEvtChannelOpenTestWrapper(evtHandle, &channelName, 0x10, timeout,
                                &evtChannelHandle);

    /*
    ** Invalid Test Case: Channel Handle is NULL 
    */

    clEvtChannelOpenTestWrapper(evtHandle, &channelName, evtChannelOpenFlag,
                                timeout, NULL);

    /*
     * Mynk - timeout can't be wrong so...
     */

    /*
     * Invalid Test Case: Timeout 0 
     */
    clEvtChannelOpenTestWrapper(evtHandle, &channelName, evtChannelOpenFlag, 0,
                                &evtChannelHandle);

    clEventChannelClose(evtChannelHandle);
    /*
     * Valid Test Case: Channel Handle is non-zero - Mynk out??? 
     */
    clEvtChannelOpenTestWrapper(evtHandle, &channelName, evtChannelOpenFlag,
                                timeout, &evtChannelHandle);

    /* 
    **  Invalid Test case: Calling ChannelOpen twice with same Channel name
    */
    rc = clEventChannelOpen(evtHandle,&channelName,evtChannelOpenFlag,timeout,&evtChannelHandle);
    if(rc == CL_OK)
        rc = clEventChannelOpen(evtHandle,&channelName,evtChannelOpenFlag,timeout,&evtChannelHandle);        
    clEvtTestResultPrint(rc);

    clEventChannelClose(evtChannelHandle);

    /*
    ** Invalid Test case: Calling Open Async Channel & Open Sync Channel
    */
#if 0 
    rc = clEventChannelOpen(evtHandle,&channelName,evtChannelOpenFlag,timeout,&evtChannelHandle);
    if(rc == CL_OK)
        clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,&channelName,evtChannelOpenFlag);

    clEventChannelClose(evtChannelHandle);
#endif


    clEventFinalize(evtHandle);
#if 0
    Mynk - NTI
        /*
         * Invalid test case 3 - Finalized handle passed, returns
         * CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtChannelOpenTestWrapper(evtHandle, &channelName, evtChannelOpenFlag,
                                    timeout, &evtChannelHandle);
#endif

        return CL_OK;
}

/********************************************************************
                    Event Open Channel Async Testing
********************************************************************/

int gCHOpenCbk = 0;
ClRcT gCHOpenAsyncRc = -1;
ClEventChannelHandleT gEvtChannelHandle;

void clEvtChannelOpenCallback(ClInvocationT invocation,ClEventChannelHandleT evtCHhandle,ClRcT rc)
{
    printf("****Channel Open Callback is called, rc %d ",rc);
    gCHOpenCbk = 1;
    gCHOpenAsyncRc = rc;
    gEvtChannelHandle = evtCHhandle;
    return;
    
}

void clEvtEventDeliveryCallback(ClEventSubscriptionIdT evtSubscriptionId, ClEventHandleT evtHandle,ClSizeT clSize)
{
    printf("Event Delivery Callback is called");
}

/*
**  Calling clEventChannelOpenAsync() with the parameters passed 
*/
void clEvtChannelOpenAsyncTestWrapper(ClEventInitHandleT evtHandle,ClInvocationT invocation, 
        ClNameT *channelName, ClEventChannelOpenFlagsT evtChannelOpenFlag)
{
    ClRcT rc = CL_OK;
    rc = clEventChannelOpenAsync(evtHandle,invocation,channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        clEvtTestResultPrint(gCHOpenAsyncRc);  
        if(gEvtChannelHandle)
        {
            rc =clEventChannelClose(gEvtChannelHandle);
            printf("*******In Channel Open Async, channel close :: %d",rc);
        }
        if(gCHOpenAsyncRc == CL_OK)
        {
            clEventChannelClose(gEvtChannelHandle);  
        }
    }
    else
        clEvtTestResultPrint(rc);    
}

ClUint32T gEvtCHOpenAsyncTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,
    CL_OK,
    CL_EVENT_ERR_BAD_HANDLE,
    CL_EVENT_ERR_NULL_PTR,
    CL_EVENT_ERR_NULL_PTR,
    CL_EVENT_ERR_BAD_FLAGS,
    CL_EVENT_ERR_CHANNEL_ALREADY_OPENED
};

void clEvtChannelOpenAsyncTest()
{
    
    //ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
#if 1
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER|CL_EVENT_LOCAL_CHANNEL;
   // ClEventChannelHandleT evtChannelHandle;
/*
    static ClEventCallbacksT evtCallbacks = 
    {
        clEvtChannelOpenCallback,
        NULL
    };
*/    
//    ClVersionT version = CL_EVENT_VERSION;
    ClInvocationT invocation = 80;    
#endif 
    /*** Set the attribute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtCHOpenAsyncTestResult);

#if 1
    /*
    **  Invalid Test: Testing EvtChannelOpenAsync without doing Init
    */
    evtHandle = 0;
//    printf("********Calling OpenAsync before init");
    clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,&channelName,evtChannelOpenFlag);
#endif
#if 0
    /*
    **  Positive Test case: All Valid values
    */ 
 //   printf("********Calling OpenAsync after init");
    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);
    if(rc == CL_OK)
        clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,&channelName,evtChannelOpenFlag);

    /*
    **  Invalid Test case: Testing EvtChannelOpenAsync with bad handle
    */
    clEvtChannelOpenAsyncTestWrapper((ClEventInitHandleT)100,invocation,&channelName,evtChannelOpenFlag);

    /*
    **  Invalid Test case: Testing EvtChannelOpenAsync without channelName
    */
    invocation = 100;
    clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,NULL,evtChannelOpenFlag);

    /*
    ** Invalid Test case: Testing EvtChannelOpenAsync with channel open flags as NULL
    */
    clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,NULL,(ClEventChannelOpenFlagsT)NULL);

    /*
    ** Invalid Test case: Testing EvtChannelOpenAsync with channel open flags as invalid values
    */
    clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,&channelName,100);

    //clEventFinalize(evtHandle);


    /*
    ** Invalid Test case: Testing EvtChannelOpenAsync with valid values after successful 
    ** clEventChannelOpen() call
    */
    //printf("******************** Test for Openchannel twice with same channel info");
    rc = clEventChannelOpen(evtHandle, &channelName, CL_EVENT_CHANNEL_SUBSCRIBER |CL_EVENT_CHANNEL_PUBLISHER, 
                            50000,&evtChannelHandle); 
    if(rc == CL_OK)
    {
        clEvtChannelOpenAsyncTestWrapper(evtHandle,invocation,&channelName,evtChannelOpenFlag);
        clEventChannelClose(evtChannelHandle);
    }
#endif   
    clEventFinalize(evtHandle);
}

/*********************************************************************
                     Event Channel Close Testing
**********************************************************************/
#if 0

Mynk - NTI void clEvtCloseCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtChannelCloseTestWrapper(ClEventChannelHandleT evtChannelHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventChannelClose(evtChannelHandle);
    clEvtTestResultPrint(rc);
 return;
}

ClUint32T gEvtChannelCloseTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE, /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,
    CL_OK                      /* CL_EVENT_ERR_BAD_HANDLE Mynk - NTC */
//    CL_OK                       /* Shanthi - NTC */
};

ClRcT clEvtChannelCloseTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle = 0;
    //const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    //ClInvocationT invocation;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtChannelCloseTestResult);

    /***** Code coverage testing *****/
    /*
     * clEvtCloseCodeCoveTest(); 
     */
    
    /*
    ** Invalid test case: closing channel before init CL_EVENT_ERR_INIT_NOT_DONE
    */
    clEvtChannelCloseTestWrapper(evtChannelHandle);


    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);    
    /*
     * Invalid test case: closing invalid channel CL_EVENT_ERR_BAD_HANDLE
     */
    clEvtChannelCloseTestWrapper(evtChannelHandle);

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);

 #if 0   /*
     * Invalide test case - Passing NULL - Mynk - NTC??? 
     */
    clEvtChannelCloseTestWrapper(0);
#endif
    /*
     * Valide test case(s) 
     */
    clEvtChannelCloseTestWrapper(evtChannelHandle);

#if 0
    /*
    **  Valid Test case: Closing a Async Open channel
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
	if(CL_OK == gCHOpenAsyncRc)
        	clEvtChannelCloseTestWrapper(gEvtChannelHandle);          
	else
		clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc); 
#endif
    /*
     * Invalid test case - Finalize on Finalized Handle
     * clEvtChannelCloseTestWrapper(evtChannelHandle); Mynk - NTC 
     */

    clEventFinalize(evtHandle);

    return CL_OK;
}

/*********************************************************************
                     Event Allocate Testing
**********************************************************************/
#if 0

Mynk - TBI void clEvtAllocateCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtAllocateTestWrapper(ClEventChannelHandleT channelHandle,
                              ClEventHandleT *pEventHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventAllocate(channelHandle, pEventHandle);
    clEvtTestResultPrint(rc);

    if (CL_OK == rc)
    {
        clEventFree(*pEventHandle);
    }

    return;
}

ClUint32T gEvtAllocateTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,         /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,            /* Test Case 1 */
    CL_EVENT_ERR_INVALID_PARAM,         /* Test Case 2 */
    CL_OK,                              /* Test Case 3 */
//    CL_OK,                              /* Test Case 4 Shanthi - NTC */
    CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH /* Test Case 5 Shanthi - NTC */   
        /*
         * , Mynk - NTI CL_OK, CL_EVENT_ERR_BAD_HANDLE 
         */
};

ClRcT clEvtAllocateTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag =
        CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
//    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    //ClInvocationT invocation = 100;

    ClEventHandleT evtEventHandle;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtAllocateTestResult);

    /*
    ** Invalid Test Case 0: EventAllocate before doing init, returns CL_EVENT_ERR_INIT_NOT_DONE
    */
    clEvtAllocateTestWrapper(022, &evtEventHandle);

    /***** Code coverage testing *****
    clEvtAllocateCodeCoveTest(); -Mynk NTI */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);


    /*
     * Invalid Test Case 1: Passing junk channelHandle, returns CL_EVENT_ERR_BAD_HANDLE 
     */
    clEvtAllocateTestWrapper(022, &evtEventHandle);

#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALID_PARAM, NO_MEMORY,
        NO_RESOURCES, BAD_FLAGS, NOT_EXIST(-NOT NEEDED)
        /*
         * Invalid test case - Invalid Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAllocateTestWrapper(1234, &evtEventHandle);

    /*
     * Invalid test case - Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtAllocateTestWrapper(? ? ?, &evtEventHandle);
#endif

    /*
     * Invalid Test Case 2: Event Handle NULL, returns CL_EVENT_ERR_INVALID_PARAM 
     */
    clEvtAllocateTestWrapper(evtChannelHandle, NULL);

    /*
     * Invalid Test Case 3: All fine, returns CL_OK 
     */
    clEvtAllocateTestWrapper(evtChannelHandle, &evtEventHandle);


    clEventChannelClose(evtChannelHandle);  /* Close Channel */

#if 0
    Mynk - NTI
        /*
         * Invalid test case 3 - Closed handle passed, returns
         * CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAllocateTestWrapper(evtHandle, &evtEventHandle);
#endif
#if 0
    /*
    ** Valid Test case 4: Allocate Event header after Open Channel Async call
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            clEvtAllocateTestWrapper(gEvtChannelHandle, &evtEventHandle);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc); 
#endif
     
    /*
    ** Invalid Test case 5: If event channel is not opened for publisher access, 
    ** returns CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH
    */

     rc = clEventChannelOpen(evtHandle, &channelName, CL_EVENT_CHANNEL_SUBSCRIBER,
                            timeout, &evtChannelHandle);
     if(CL_OK == rc)
        clEvtAllocateTestWrapper(evtChannelHandle, &evtEventHandle);
     else
        clEvtTestResultPrint(rc);

    clEventFinalize(evtHandle); /* Finalize */
    
    //printf("--------------------Event Allocate test is over \n");
    return CL_OK;
}

/*********************************************************************
                     Event Free Testing
**********************************************************************/
#if 0

Mynk - NTI void clEvtFreeCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtFreeTestWrapper(ClEventHandleT eventEventHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventFree(eventEventHandle);
    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtFreeTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE, /* Shanthi - NTC */
    CL_OK,                      /* Test Case 1 */
    CL_EVENT_ERR_BAD_HANDLE    /* Test Case 2, CL_EVENT_ERR_BAD_HANDLE Mynk - NTC */
//    CL_OK                       /* Shanthi - NTC */
};

ClRcT clEvtFreeTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag =
        CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
    //const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
//    ClInvocationT invocation = 10;

    ClEventHandleT evtEventHandle = 0;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtFreeTestResult);

    /***** Code coverage testing *****/
    /*
     * clEvtFreeCodeCoveTest(); 
     */

    /*
    ** Invalid Test Case:Event Free before doing init, returns CL_EVENT_ERR_INIT_NOT_DONE 
    */
    clEvtFreeTestWrapper(evtEventHandle); 
    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);

    rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
    /*
     * Valid Test Case:Passing Normal Event Handle, returns CL_OK 
     */
    clEvtFreeTestWrapper(evtEventHandle);
    /*
     * Invalid Test Case: Freed Handle Passed, returns CL_EVENT_ERR_BAD_HANDLE
     * clEvtFreeTestWrapper(evtEventHandle); Mynk - NTC 
     */
    clEvtFreeTestWrapper(evtEventHandle);

    clEventChannelClose(evtChannelHandle);  /* Close Channel */
#if 0
    /*
    ** Valid Test Case:Passing Event Handle obtained through Async call, returns CL_OK 
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            clEventAllocate(gEvtChannelHandle, &evtEventHandle);
            clEvtFreeTestWrapper(evtEventHandle);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc); 
#endif  

    clEventFinalize(evtHandle); /* Finalize */

    //printf("--------------------Event Free test is over \n");
    return CL_OK;
}

/*********************************************************************
                     Event AttributesSet Testing
**********************************************************************/
#if 0

Mynk - TBI void clEvtAttributesSetCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtAttributesSetTestWrapper(ClEventHandleT eventHandle,
                                   const ClEventPatternArrayT *pPatternArray,
                                   ClEventPriorityT priority,
                                   ClTimeT retentionTime,
                                   const ClNameT *pPublisherName)
{
    ClRcT rc = CL_OK;

    rc = clEventAttributesSet(eventHandle, pPatternArray, priority,
                              retentionTime, pPublisherName);
    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtAttributesSetTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,         /* Test Case 0 Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,            /* Test Case 1 */
    CL_OK,                              /* Test Case 2  TBD */ 
    CL_EVENT_ERR_INVALID_PARAM,         /* Test Case 3 */
    CL_OK                              /* Test Case 4 */
//    CL_OK                              /* Test Case 5 Shanthi - NTC */   
    
        /*
         * , Mynk - NTI CL_OK, CL_EVENT_ERR_BAD_HANDLE,
         * CL_EVENT_ERR_BAD_HANDLE, CL_EVENT_ERR_BAD_HANDLE 
         */
};

ClRcT clEvtAttributesSetTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag =
        CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
//    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;

    ClEventHandleT evtEventHandle = 0;

    const ClEventPatternArrayT *pPatternArray = &gPatternArray;
    ClEventPriorityT priority = 0x2;
    ClTimeT retentionTime = 0;  /* Mynk - Not supported yet */
    ClNameT publisherName = { strlen(EVT_TEST_PUBLISHER_NAME),
        EVT_TEST_PUBLISHER_NAME
    };
    const ClNameT *pPublisherName = &publisherName;
    //ClInvocationT invocation = 100;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtAttributesSetTestResult);

#if 1
    /*
    ** Invalid Test Case 0: Calling AttributesSet before doing init, returns CL_EVENT_ERR_INIT_NOT_DONE
    */
    clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, 0, 0, pPublisherName); 

    /***** Code coverage testing *****
    clEvtAttributesSetCodeCoveTest(); -Mynk NTI */
#endif
    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);
#if 1
     /*
     * Invalid Test Case 1: passing invalid params should return CL_EVENT_ERR_INVALID_PARAM;
     */
    clEvtAttributesSetTestWrapper(100, pPatternArray, 0, 0, pPublisherName);
#endif
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALID_PARAM, NO_MEMORY,
        NO_RESOURCES, BAD_FLAGS, NOT_EXIST(-NOT NEEDED)
        /*
         * Invalid test case - Invalid Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesSetTestWrapper(1234, NULL, 0, 0, NULL);

    /*
     * Invalid test case - Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtAttributesSetTestWrapper(? ? ?, NULL, 0, 0, NULL);
#endif

    /*
     * Valid Test Case 2: If Publisher/Pattern Array is NULL, returns CL_OK 
     */
    clEvtAttributesSetTestWrapper(evtEventHandle, NULL, priority, 0, NULL);

    /*
     * Invalid Test Case 3: Priority is Invalid , returns CL_EVENT_ERR_INVALID_PARAM 
     */
    clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, 5, 0, pPublisherName);

    /*
     * Valid Test Case 4: All fine, returns CL_OK 
     */
    clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, priority,
                                  retentionTime, pPublisherName);

    clEventFree(evtEventHandle);    /* Event Free */

    

#if 0
    Mynk - NTI
        /*
         * Test Case 4 - Freed Event Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif

    clEventChannelClose(evtChannelHandle);  /* Close Channel */

#if 0
    /*
    ** Valid Test case 5: Set Attributes for the channel opened thro Async call
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            clEventAllocate(gEvtChannelHandle, &evtEventHandle);
            clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, priority,retentionTime, pPublisherName);
            clEventFree(evtEventHandle);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc);

#endif  
    

#if 0
    Mynk - NTI
        /*
         * Test Case 5 - Closed Channnel, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif

    clEventFinalize(evtHandle); /* Finalize */
    clEventFinalize(evtHandle);

#if 0
    Mynk - NTI
        /*
         * Test Case 6 - Finalized handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesSetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif
    //printf("--------------------Event AttributesSet test is over \n");
    return CL_OK;
}

/*********************************************************************
                     Event AttributesGet Testing
**********************************************************************/
#if 0

Mynk - TBI void clEvtAttributesGetCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif


void clEvtAttributesGetTestWrapper(ClEventHandleT eventHandle,
                                   ClEventPatternArrayT *pPatternArray,
                                   ClEventPriorityT * pPriority,
                                   ClTimeT *pRetentionTime,
                                   ClNameT *pPublisherName,
                                   ClTimeT *pPublishTime, ClEventIdT * pEventId)
{
    ClRcT rc = CL_OK;

    rc = clEventAttributesGet(eventHandle, pPatternArray, pPriority,
                              pRetentionTime, pPublisherName, pPublishTime,
                              pEventId);
    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtAttributesGetTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,     /* Test Case 0 Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,        /* Test Case 1 */  
    CL_OK,                          /* Test Case 2 */
    CL_OK,                          /* Test Case 3 Shanthi - NTC*/
    CL_OK,                          /* Test Case 4 */
    CL_EVENT_ERR_NO_SPACE           /* Test Case 5 */
    
#if 0
    CL_EVENT_ERR_INVALID_PARAM,     /* Test Case 1 */
    CL_EVENT_ERR_INVALID_PARAM,     /* Test Case 2 */
#endif
        /*
         * , Mynk - NTI CL_OK, CL_EVENT_ERR_BAD_HANDLE,
         * CL_EVENT_ERR_BAD_HANDLE, CL_EVENT_ERR_BAD_HANDLE 
         */
};


ClRcT clEvtAttributesGetTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
    //const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;

    ClEventHandleT evtEventHandle;

    ClEventPatternArrayT *pPatternArray = &gPatternArray;   /* Needed ??? -
                                                             * Mynk */
    ClEventPatternArrayT *getPatternArray = NULL;
    ClEventPriorityT priority = 0x1,getPriority;
    ClTimeT retentionTime = 0,getRetentionTime;  /* Mynk - Not supported yet */
    ClNameT publisherName = { strlen(EVT_TEST_PUBLISHER_NAME),
        EVT_TEST_PUBLISHER_NAME
    };
    ClNameT *pPublisherName = &publisherName;
    ClNameT getPublisherName;
    ClTimeT getPublishTime;
    ClEventIdT getEventId;
    ClEventPatternT getEventPattern[4],lessEventPattern[4];
    int patternCount = 0;
    int checkFlag = 0;
    int count;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtAttributesGetTestResult);
	//printf("********** In Attributes Get function");
    /*
    ** Test Case 0 - Call AttributesGet before init, should return CL_EVENT_ERR_INIT_NOT_DONE
    */
    clEvtAttributesGetTestWrapper(100, getPatternArray, &priority, &retentionTime, &getPublisherName, 
                                 &getPublishTime, &getEventId);

    /*-------------------------------------------------------------------------*/
    
    /***** Code coverage testing *****
    clEvtAttributesGetCodeCoveTest(); -Mynk NTI */
    rc = clEventInitialize(&evtHandle, NULL, &version);
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
  
    rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
    
    rc = clEventAttributesSet(evtEventHandle, pPatternArray, priority,retentionTime, pPublisherName);
    /*
    ** Test Case 1 - Event Handle is invalid should return CL_EVENT_ERR_INVALID_PARAM    
    */
    clEvtAttributesGetTestWrapper(100, NULL, &getPriority, &getRetentionTime, &getPublisherName, 
                                &getPublishTime, &getEventId);

    /*-------------------------------------------------------------------------*/

    /* 
    ** Test Case 2 - If any of the o/p parameters is NULL, even then function will return CL_OK
    */
    clEvtAttributesGetTestWrapper(evtEventHandle, NULL, NULL, NULL, NULL, NULL, NULL);

    /*-------------------------------------------------------------------------*/
    /* 
    ** Test Case 3: Valid Test case, Verify whether the set parameters are same as obtained. Set PatternArray
    **              parameter(II) as NULL
    */
    getPatternArray = clHeapAllocate(sizeof(ClEventPatternArrayT));
    memset(&getPublisherName,0,sizeof(getPublisherName));
    rc = clEventAttributesGet(evtEventHandle, getPatternArray, &getPriority, &getRetentionTime, &getPublisherName,                                  &getPublishTime, &getEventId);
     if(CL_OK == rc)
     {           
         if(getPatternArray->patternsNumber != 0)
         {
            /* Validate the Pattern Array with the set values */
            for(patternCount = 0;patternCount < getPatternArray->patternsNumber;patternCount++)
            {
                if(0 != strcmp((const char *)pPatternArray->pPatterns[patternCount].pPattern, 
                        (const char *)getPatternArray->pPatterns[patternCount].pPattern))
                {
			printf("********* Given Pattern :: %s \n",pPatternArray->pPatterns[patternCount].pPattern);
			printf("********* Obtained Pattern :: %s \n",getPatternArray->pPatterns[patternCount].pPattern);
                    checkFlag = 1;
                    clEvtTestResultPrint(255);  
                }
            }
            /* Validate the priority, retention time & Publisher name obtained with the set values */
            if(priority != getPriority || retentionTime != getRetentionTime || 
                    (0 != strcmp(getPublisherName.value,pPublisherName->value)))
            {
		    printf("********* Given publisher name :: %s priority :: %d \n",pPublisherName->value,priority);
		    printf("********* Obtained publisher name :: %s Priority :: %d \n",getPublisherName.value,getPriority);
                checkFlag = 1;
                clEvtTestResultPrint(255);
            }         
        }
        else
            clEvtTestResultPrint(rc);
     }
     else
        clEvtTestResultPrint(rc);
     
     /* Free allocated memory which is used for getting the pattern*/
    for(count = 0;count < 4;count++)
    {
        if(getPatternArray->pPatterns[count].pPattern)
            clHeapFree((getPatternArray->pPatterns[count].pPattern));   
    }
    if(getPatternArray->pPatterns)
        clHeapFree(getPatternArray->pPatterns);

    clHeapFree(getPatternArray);
    
    /* If no errors, then print this result */
     if(checkFlag != 1)
        clEvtTestResultPrint(rc);

     /*-------------------------------------------------------------------------*/
    /* 
    ** Test Case 4: Valid Test case, Verify whether the set parameters are same as obtained. Allocate memory
    ** to the Pattern Array
    */
     
    /* Allocate memory for getting the pattern*/    
    getPatternArray = clHeapAllocate(sizeof(ClEventPatternArrayT));
    memset(getPatternArray,0,sizeof(getPatternArray));
    getPatternArray->allocatedNumber = 4;
    
    for(count = 0;count < 4;count++)
    {
        getEventPattern[count].pPattern = clHeapAllocate(32);  
        getEventPattern[count].allocatedSize = 30;
    }

    getPatternArray->pPatterns = getEventPattern;
   
     rc = clEventAttributesGet(evtEventHandle, getPatternArray, &getPriority, &getRetentionTime, &getPublisherName,                                  &getPublishTime, &getEventId);
     if(CL_OK == rc)
     {           
         if(getPatternArray->patternsNumber != 0)
         {
            /* Validate the Pattern Array with the set values */
            for(patternCount = 0;patternCount < getPatternArray->patternsNumber;patternCount++)
            {
                if(0 != strcmp((const char *)pPatternArray->pPatterns[patternCount].pPattern, 
                        (const char *)getPatternArray->pPatterns[patternCount].pPattern))
                {
                    checkFlag = 1;
                    clEvtTestResultPrint(255);  
                }
            }
            /* Validate the priority, retention time & Publisher name obtained with the set values */
            if(priority != getPriority || retentionTime != getRetentionTime || 
                    (0 != strcmp(getPublisherName.value,pPublisherName->value)))
            {
                checkFlag = 1;
                clEvtTestResultPrint(255);
            }         
        }
        else
            clEvtTestResultPrint(rc);
     }
     else
        clEvtTestResultPrint(rc);
     
     /* Free allocated memory which is used for getting the pattern*/
    for(count = 0;count < 4;count++)
    {
        if(getPatternArray->pPatterns[count].pPattern)
            clHeapFree((getPatternArray->pPatterns[count].pPattern));   
    }
    if(getPatternArray->pPatterns)
        clHeapFree(getPatternArray);
    
    /* If no errors, then print this result */
     if(checkFlag != 1)
        clEvtTestResultPrint(rc);
    /*-------------------------------------------------------------------------*/
    /* 
    ** Test Case 5: Invalid Test case, Allocate less memory than expected to get the pattern 
    */
    //printf("*****In Test case 5");    
    /* Allocate memory for getting the pattern*/    

    getPatternArray = clHeapAllocate(sizeof(ClEventPatternArrayT));
    memset(getPatternArray,0,sizeof(getPatternArray));
    getPatternArray->allocatedNumber = 3;
    
    for(count = 0;count < 4;count++)
    {
        lessEventPattern[count].pPattern = clHeapAllocate(30);  
        lessEventPattern[count].allocatedSize = 30;
    }

    getPatternArray->pPatterns = lessEventPattern;
   
     rc = clEventAttributesGet(evtEventHandle, getPatternArray, &getPriority, &getRetentionTime, &getPublisherName,                                  &getPublishTime, &getEventId);
     clEvtTestResultPrint(rc);
	//printf("******Before heap free");
/* Free allocated memory which is used for getting the pattern*/
    for(count = 0;count < 4;count++)
    {
        if(getPatternArray->pPatterns[count].pPattern)
            clHeapFree((getPatternArray->pPatterns[count].pPattern));   
    }
    if(getPatternArray->pPatterns)
        clHeapFree(getPatternArray);
    //printf("******** After heap free");

    /*-------------------------------------------------------------------------*/
     
      
#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALID_PARAM, NO_MEMORY,
        NO_RESOURCES, BAD_FLAGS, NOT_EXIST(-NOT NEEDED)
        /*
         * Invalid test case - Invalid Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesGetTestWrapper(1234, NULL, 0, 0, NULL);

    /*
     * Invalid test case - Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtAttributesGetTestWrapper(? ? ?, NULL, 0, 0, NULL);
#endif

    /*
     * Test Case 1 - All NULLs except handle, returns CL_OK 
     */
#if 0
    clEvtAttributesGetTestWrapper(evtEventHandle, NULL, NULL, NULL, NULL, NULL,
                                  NULL);


    /*
     * Test Case 2 - All values, returns CL_OK 
     */
    clEvtAttributesGetTestWrapper(evtEventHandle, NULL, NULL, NULL,
                                  pPublisherName, NULL, NULL);

    /*
     * Test Case 3 - Test Pattern Specific, returns CL_OK 
     */

    /*
     * Test Case 4 - All values Check various combinations 
     */
    clEvtAttributesGetTestWrapper(evtEventHandle, pPatternArray, &priority,
                                  &retentionTime, &publisherName);
#endif

    clEventFree(evtEventHandle);    /* Event Free */

#if 0
    Mynk - NTI
        /*
         * Test Case 4 - Freed Event Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesGetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif

    clEventChannelClose(evtChannelHandle);  /* Close Channel */

#if 0
    Mynk - NTI
        /*
         * Test Case 5 - Closed Channnel, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesGetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif

    clEventFinalize(evtHandle); /* Finalize */
    clEventFinalize(evtHandle);
    //printf("--------------------Event Attributes GetTest is over \n");
#if 0
    Mynk - NTI
        /*
         * Test Case 6 - Finalized handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtAttributesGetTestWrapper(evtEventHandle, pPatternArray, priority,
                                      retentionTime, pPublisherName);
#endif

    return CL_OK;
}

/*********************************************************************
                     Event Publish Testing
**********************************************************************/
#if 0

Mynk - TBI void clEvtPublishCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtPublishTestWrapper(ClEventHandleT eventHandle, const void *pEventData,
                             ClSizeT eventDataSize, ClEventIdT * pEventId)
{
    ClRcT rc = CL_OK;

    rc = clEventPublish(eventHandle, pEventData, eventDataSize, pEventId);

    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtPublishTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,     /* Test Case 0 Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,        /* Test Case 1 Shanthi - NTC */
    CL_OK,                          /* Test Case 2 */
    CL_EVENT_ERR_INVALID_PARAM,     /* Test Case 3 */
    CL_OK,                          /* Test Case 4 */
    CL_OK                           /* Test Case 5 */

    /*
     * CL_EVENT_ERR_INVALID_PARAM, Test Case 2 
     */
            /*
         * , Mynk - NTI CL_OK, CL_EVENT_ERR_BAD_HANDLE,
         * CL_EVENT_ERR_BAD_HANDLE, CL_EVENT_ERR_BAD_HANDLE 
         */
};

ClRcT clEvtPublishTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag =
        CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
//    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;

    ClNameT publisherName = { strlen(EVT_TEST_PUBLISHER_NAME),
        EVT_TEST_PUBLISHER_NAME
    };
    const ClNameT *pPublisherName = &publisherName;

    ClEventHandleT evtEventHandle;

    ClEventIdT eventId;
    ClUint32T payLoad = 0x11223344;
    //ClInvocationT invocation = 100;
    

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtPublishTestResult);
    //printf("**************** In ppublish Test");
    /*
    **  Invalid Test case 0: calling Publish before init
    */    
    clEvtPublishTestWrapper(100, (const void *) &payLoad,sizeof(ClUint32T), &eventId);

    /*------------------------------------------------------------------------*/
    
    /***** Code coverage testing *****
//    clEvtPublishCodeCoveTest(); -Mynk NTI */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);
    printf("********In publish,  init :: %#llX rc :: %d\n",evtHandle,rc);

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    printf("*******In Publish, channel handle :: %#llX rc :: %d\n",evtChannelHandle,rc);

    rc = clEventAllocate(evtChannelHandle, &evtEventHandle);

    /*
    **  Invalid Test case 1: Calling Publish with invalid handle
    */
    
    clEvtPublishTestWrapper(100, (const void *) &payLoad,sizeof(ClUint32T), &eventId);
    /* ------------------------------------------------------------------------*/

    /*
     * Test Case 2 - Default values for Event, returns CL_OK 
     */
    clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,
                            sizeof(ClUint32T), &eventId);
    
    /* ------------------------------------------------------------------------ */
    
    rc = clEventAttributesSet(evtEventHandle, &gPatternArray, 0, 0,
                              pPublisherName);
    /*
     * Test Case 3 - All 0s Invalid, returns CL_EVENT_ERR_INVALID_PARAM 
     */
    clEvtPublishTestWrapper(evtEventHandle, NULL, 0, 0);
    /* ------------------------------------------------------------------------ */

#if 0

    Mynk - Eventid not used TBD, Access for publish
        ? ? ? Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALID_PARAM,
            NO_MEMORY, NO_RESOURCES, BAD_FLAGS,
            /*
             * Invalid test case - Invalid Handle, returns
             * CL_EVENT_ERR_BAD_HANDLE 
             */
            clEvtPublishTestWrapper(1234, NULL, 0, 0);

    /*
     * Invalid test case - Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtPublishTestWrapper(? ? ?, NULL, 0, 0);
#endif
    /*------------------------------------------------------------------------*/
    /*
     * Valid Test Case 4 - PayLoad is NULL, returns CL_OK 
     */
    clEvtPublishTestWrapper(evtEventHandle, NULL, sizeof(ClUint32T), &eventId);
    /*-------------------------------------------------------------------------*/

    /*
     * Valid Test Case 3 - All fine, returns CL_OK 
     */
    clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,
                            sizeof(ClUint32T), &eventId);
    /*------------------------------------------------------------------------*/

    clEventFree(evtEventHandle);    /* Event Free */

#if 0
    Mynk - NTI
        /*
         * Test Case 4 - Freed Event Handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,
                                sizeof(ClUint32T), &eventId);
#endif

    clEventChannelClose(evtChannelHandle);  /* Close Channel */
    /*------------------------------------------------------------------------*/
    /*
    ** Valid Test case 6: Calling EventPublish after opening channel asynchronously
    */
#if 0
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            clEventAllocate(gEvtChannelHandle, &evtEventHandle);
            clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,sizeof(ClUint32T), &eventId);
            clEventFree(evtEventHandle);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc);  
#endif
#if 0
    Mynk - NTI
        /*
         * Test Case 5 - Closed Channnel, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,
                                sizeof(ClUint32T), &eventId);
#endif

    clEventFinalize(evtHandle); /* Finalize */
    //printf("--------------------Event Publish test is over \n");

#if 0
    Mynk - NTI
        /*
         * Test Case 6 - Finalized handle, returns CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtPublishTestWrapper(evtEventHandle, (const void *) &payLoad,
                                sizeof(ClUint32T), &eventId);
#endif

    return CL_OK;
}

/*********************************************************************
                     Event Subscription Testing
**********************************************************************/
#if 0

Mynk - NTI void clEvtSubscribeCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtSubscribeTestWrapper(const ClEventChannelHandleT evtChannelHandle,
                               const ClEventFilterArrayT *pFilters,
                               ClEventSubscriptionIdT subscriptionId)
{
    ClRcT rc = CL_OK;

    rc = clEventSubscribe(evtChannelHandle, pFilters, subscriptionId,
                          (void *) 0xAABB);
    clEvtTestResultPrint(rc);

    if (CL_OK == rc)
    {
        clEventUnsubscribe(evtChannelHandle, subscriptionId);
    }

    return;
}

ClUint32T gEvtSubscribeTestResult[] = {
    CL_EVENT_ERR_INIT_NOT_DONE,                 /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,
    CL_OK,
    CL_OK,   /* , Mynk - NTI CL_EVENT_ERR_BAD_HANDLE */
    CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION,    /* Shanthi - NTC */
 //   CL_OK,
//    CL_OK                                        /* Shanthi - NTC */

    /*
     * CL_EVT_ERR_EXIST, 
     */

   };

ClRcT clEvtSubscribeTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER|CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
    //const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    const ClEventFilterArrayT *pFilters = &gSubscribeFilters;
    ClEventSubscriptionIdT subscriptionId = 0x123;
    //ClInvocationT invocation;
    ClEventHandleT evtEventHandle;
//    ClEventFilterArrayT filterArray;
//    ClEventFilterT filter;
    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtSubscribeTestResult);

    //printf("*************In Subscribe test \n\n");
    /* 
    ** Invalid Test case 0: Calling EventSubscribe before init
    */
    clEvtSubscribeTestWrapper(100, pFilters, subscriptionId);   
    /*-----------------------------------------------------------------------*/
    
    /***** Code coverage testing *****
    clEvtSubscribeCodeCoveTest(); -Mynk NTI */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);

    /*
     * Invalid Test Case 1 - Bad handle should  returns CL_EVENT_ERR_BAD_HANDLE 
     */
    /*
     * Mynk - NTI filter and subscription test (unique Id)_EXIST 
     */
    clEvtSubscribeTestWrapper(evtChannelHandle, NULL, 0);
    /*-----------------------------------------------------------------------*/

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);

#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, INVALId_PARAM, NO_MEMORY,
        NO_RESOURCES, BAD_FLAGS, NOT_EXIST(-NOT NEEDED)
        /*
         * Invalid test case 2 - Invalid Handle, returns
         * CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtSubscribeTestWrapper(1234, NULL, 0);

    /*
     * Invalid test case 4 - Invalid Init, returns CL_EVT_ERR_INIT 
     */
    clEvtSubscribeTestWrapper(? ? ?, NULL, 0);

#endif

    /*
     * Test Case 2 - Invalid Filter, returns CL_OK 
     */
    printf("************* Subscriber test 2:: %#llX\n\n",evtChannelHandle);
    clEvtSubscribeTestWrapper(evtChannelHandle, NULL, subscriptionId);
    /*-----------------------------------------------------------------------*/
    /*
     * Invalid test case - Invalid subsId, returns CL_EVT_ERR_EXIST - Mynk NTI
     * clEvtSubscribeTestWrapper(evtChannelHandle, pFilters, subscriptionId);
     * clEvtSubscribeTestWrapper(evtChannelHandle, pFilters, subscriptionId); 
     */

    /*
     * Test Case 3 - channelName is non-NULL 
     */
    clEvtSubscribeTestWrapper(evtChannelHandle, pFilters, subscriptionId);
   // clEvtSubscribeTestWrapper(evtChannelHandle, pFilters, subscriptionId);

    clEventChannelClose(evtChannelHandle);  /* Close Channel */
    /*-----------------------------------------------------------------------*/


#if 0
    Mynk - NTI
        /*
         * Invalid test case 3 - Closed handle passed, returns
         * CL_EVENT_ERR_BAD_HANDLE 
         */
        clEvtSubscribeTestWrapper(evtChannelHandle, NULL, 0);
#endif

    /*
    ** Invalid Test Case 4: Open the channel without SUBSCRIBER flag & call Subscribe
    */
    rc = clEventChannelOpen(evtHandle, &channelName, CL_EVENT_CHANNEL_PUBLISHER,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        clEvtSubscribeTestWrapper(evtChannelHandle, pFilters, subscriptionId);
        clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
    /*-----------------------------------------------------------------------*/
#if 0
    /*
    ** Valid Test case 5: Calling EventSubscribe after opening channel asynchronously
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            clEvtSubscribeTestWrapper(gEvtChannelHandle, pFilters, subscriptionId);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc);
#endif
    clEventFinalize(evtHandle);
    
    /*------------------------------------------------------------------------*/    

    /* 
    ** Valid Test case 6:   Ensure that after subscription we got some event in the 
    **                      DeliveryCallback function
    */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks1, &version);
    //printf("**********************In Subscribe testing");
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        clEventSubscribe(evtChannelHandle, NULL, subscriptionId,(void *) 0xAABB);
        tPayLoad = 0x1234;
        rc = clEventAllocate(evtChannelHandle, &evtEventHandle);         
        rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);       
        //printf("*********Before while loop");
        /*while(1)
        {
            if(tDeliveryFlag == 1)
                break;
        }*/
        //printf("************After while loop");
        rc = clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
    /*------------------------------------------------------------------------*/ 

    /* 
    ** Valid Test case 7:   Ensure that after subscription event should not be received
    **                      because none of the filter is matched 
    */
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        gSubscribePrefixFilter1.filtersNumber = 1;
        gSubscribePrefixFilter1.pFilters = &gPrefixFilter[0];
        subscriptionId = 0x111;
        rc = clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
        //printf("************Subscription :: %d\n ",rc);
        tPayLoad = 0x1234;
        rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
        rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
        //printf("************Attributes set :: %d\n ",rc);
        rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId); 
        clEventFree(evtEventHandle);
        rc = clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
    /*------------------------------------------------------------------------*/
    /* 
    ** Valid Test case 8:   Ensure that after subscription event should be received
    **                      because filter is matched 
    */
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
	    //printf("******In Subscriber Test, Test case 8 \n");
        memset(&gSubscribePrefixFilter1,0,sizeof(gSubscribePrefixFilter1));
        gSubscribePrefixFilter1.filtersNumber = 1;
        gSubscribePrefixFilter1.pFilters = &gPrefixFilter[1];
        subscriptionId = 0x112;
        clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
        tPayLoad = 0x1234;
        rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
        rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
        rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);       
        clEventFree(evtEventHandle);
        rc = clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
#if 1
    /*------------------------------------------------------------------------*/
    /* 
    ** Valid Test case 9:   Ensure that after subscription event should be received
    **                      regardless of the filter, filter type is Pass filter
    */
    //printf("*************Subscribe test, Test case 9 \n");
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        memset(&gSubscribePrefixFilter1,0,sizeof(gSubscribePrefixFilter1));
        gSubscribePrefixFilter1.filtersNumber = 1;
        gSubscribePrefixFilter1.pFilters = &gPrefixFilter[2];
        subscriptionId = 0x113;
        clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
        tPayLoad = 0x1234;
        rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
        rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
        rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);       
        clEventFree(evtEventHandle);
        rc = clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
    /*------------------------------------------------------------------------*/
    /* 
    ** Valid Test case 10:   Ensure that after subscription event should be received
    **                       regardless of the filter, filter type is Pass filter 
    */
    //printf("************ Subscriber Test, test case 10 \n\n");
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        memset(&gSubscribePrefixFilter1,0,sizeof(gSubscribePrefixFilter1));
        gSubscribePrefixFilter1.filtersNumber = 1;
        gSubscribePrefixFilter1.pFilters = &gPrefixFilter[3];
        subscriptionId = 0x114;
        clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
        tPayLoad = 0x1234;
        rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
        rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
        rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);       
        clEventFree(evtEventHandle);
        rc = clEventChannelClose(evtChannelHandle);  /* Close Channel */
    }
    /*------------------------------------------------------------------------*/
#endif
    clEventFinalize(evtHandle); /* Finalize */
   // clEventFinalize(evtHandle);
    //printf("--------------------Event Subscribe test is over \n");
    return CL_OK;
}

/*********************************************************************
                     Event Unsubscription Testing
**********************************************************************/
#if 0

Mynk - NTI void clEvtUnsubscribeCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
   
    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtUnsubscribeTestWrapper(ClEventChannelHandleT evtChannelHandle,
                                 ClEventSubscriptionIdT subscriptionId)
{
    ClRcT rc = CL_OK;

    rc = clEventUnsubscribe(evtChannelHandle, subscriptionId);
    clEvtTestResultPrint(rc);

    return;
}

ClUint32T gEvtUnsubscribeTestResult[] = {
       /*
     * CL_EVT_ERR_NOT_EXIST, 
     */
    CL_EVENT_ERR_INIT_NOT_DONE,     /* Shanthi - NTC */
    CL_EVENT_ERR_BAD_HANDLE,        /* Shanthi - NTC */
    CL_OK,                          /* , Mynk - NTI CL_EVENT_ERR_BAD_HANDLE */
    CL_OK,                          /* Shanthi - NTC */    
    CL_OK,                          /* Shanthi - NTC */
    CL_OK                           /* Shanthi- NTC */
};

ClRcT clEvtUnsubscribeTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER|CL_EVENT_CHANNEL_PUBLISHER;
    ClTimeT timeout = 3000;
    ClEventChannelHandleT evtChannelHandle;
    //const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    const ClEventFilterArrayT *pFilters = &gSubscribeFilters;
    ClEventSubscriptionIdT subscriptionId = 0xABCD;
    ClEventHandleT evtEventHandle;
//    ClInvocationT invocation = 100;

    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtUnsubscribeTestResult);

    //printf("********In UnSubscribe testing \n");
    /*
    **  Invalid Test case: Unsubscribe before init
    */
    clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
    /*------------------------------------------------------------------*/

    /***** Code coverage testing *****
    clEvtUnsubscribeCodeCoveTest(); -Mynk NTI */

    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);

    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);

    rc = clEventSubscribe(evtChannelHandle, pFilters, subscriptionId,
                          (void *) 0xAABB);

    /*
    ** Invalid Test Case 1: Channel handle is Invalid, returns CL_EVENT_ERR_BAD_HANDLE 
    */
    clEvtUnsubscribeTestWrapper(100, subscriptionId);
    /*------------------------------------------------------------------*/
    /*
    ** Valid Test Case 2: All valid values, returns CL_OK
    */
    clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
    clEventChannelClose(evtChannelHandle);  /* Close Channel */
    /*------------------------------------------------------------------*/
    /*
    ** Valid Test Case 3: After publishing an event, do Unsubscribe, returns CL_OK
    */   
    rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &evtChannelHandle);
    if(CL_OK == rc)
    {
        memset(&gSubscribePrefixFilter1,0,sizeof(gSubscribePrefixFilter1));
        gSubscribePrefixFilter1.filtersNumber = 1;
        gSubscribePrefixFilter1.pFilters = &gPrefixFilter[1];
        subscriptionId = 0x115;
        rc = clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
        if(CL_OK == rc)
        {
            tPayLoad = 0x1234;
            rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
            rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
            rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId); 
            clEventFree(evtEventHandle);
            clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
        }
        else
            clEvtTestResultPrint(rc);
    }
    else
        clEvtTestResultPrint(rc);
    /*------------------------------------------------------------------*/
    /*
    ** Valid Test Case 4: Before publishing an event, do Unsubscribe, ensure that event is not recvd
    **                    returns CL_OK
    */   
    
    //sleep(30000);
    memset(&gSubscribePrefixFilter1,0,sizeof(gSubscribePrefixFilter1));
    gSubscribePrefixFilter1.filtersNumber = 1;
    gSubscribePrefixFilter1.pFilters = &gPrefixFilter[1];
    subscriptionId = 0x116;
    clEventSubscribe(evtChannelHandle, &gSubscribePrefixFilter1, subscriptionId,(void *) 0xAABB);
    clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
    tPayLoad = 0x1234;
    rc = clEventAllocate(evtChannelHandle, &evtEventHandle);
    rc = clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
    rc = clEventPublish(evtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);  
    
    clEventFree(evtEventHandle);
    clEventChannelClose(evtChannelHandle);
    
    /*------------------------------------------------------------------*/
#if 0
    /*
    ** Valid Test case 5: Calling Unsubscribe after opening channel asynchronously
    */
    rc = clEventChannelOpenAsync(evtHandle,invocation,&channelName,evtChannelOpenFlag);
    if(rc == CL_OK)
    {
        while(1)
        {
            if(gCHOpenCbk == 1)                
                break;
        }
        if(CL_OK == gCHOpenAsyncRc)
        {
            rc = clEventSubscribe(gEvtChannelHandle, pFilters, subscriptionId,
                          (void *) 0xAABB);
            clEventAllocate(gEvtChannelHandle, &evtEventHandle);
            clEventAttributesSet(evtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);
            clEventPublish(evtEventHandle, (const void *) &tPayLoad,sizeof(ClUint32T), &tEventId);
            clEventFree(evtEventHandle);
            clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
            clEventChannelClose(gEvtChannelHandle);          
        }
        else
            clEvtTestResultPrint(gCHOpenAsyncRc);
    }
    else
        clEvtTestResultPrint(rc);
#endif
#if 0
    Mynk - NTI - LIBRARY, TIMEOUT, TRY_AGAIN, NOT_EXIST
        /*
         * Mynk - NTI filter and subscription test 
         */
        /*
         * Invalid test case 2 - Invalid Subscription, returns
         * CL_EVT_ERR_NOT_EXIST 
         */
        clEvtUnsubscribeTestWrapper(evtChannelHandle, 0);
#endif

#if 0    /*
     * Test Case 1 - channelName is non-NULL 
     */
    clEvtUnsubscribeTestWrapper(evtChannelHandle, subscriptionId);
#endif    

//    clEventChannelClose(evtChannelHandle);  /* Close Channel */

    clEventFinalize(evtHandle); /* Finalize */
    //printf("--------------------Event Unsubscribe test is over \n");

    return CL_OK;
}

static ClUint32T gEvtCookieGetTestResult[] = 
{
    CL_EVENT_ERR_INIT_NOT_DONE, 
    CL_EVENT_ERR_BAD_HANDLE
};
void clEvtCookieGetTestWrapper(ClEventHandleT eventHandle, void **pCookie)
{
    ClRcT rc;
    rc = clEventCookieGet(eventHandle,pCookie);
    clEvtTestResultPrint(rc);
    return;
}
/*******************************************************************
                    Event CookieGet Testing
*******************************************************************/                    
void clEvtCookieGet()
{
    ClRcT rc;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventInitHandleT evtHandle;
#if 0
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag = CL_EVENT_CHANNEL_SUBSCRIBER | 
                                                  CL_EVENT_CHANNEL_PUBLISHER |CL_EVENT_GLOBAL_CHANNEL;
    ClTimeT timeout = 3000;
#endif 
    void *pCookie = NULL;
 
    /*** Set the attrinute *****/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtCookieGetTestResult);

    //printf("************ Cookie Get \n\n");
    /* Invalid Test case: EventCookieGet before Init */
    clEvtCookieGetTestWrapper(100,&pCookie);
    
    /*-----------------------------------------------------------------------*/
    /* Invalid Test case: EventCookieGet after Init */
    rc = clEventInitialize(&evtHandle, &gEvtCallbacks, &version);
    clEvtCookieGetTestWrapper(100,&pCookie);
    
    /*-----------------------------------------------------------------------*/
    
   // rc = clEventChannelOpen(evtHandle, &channelName, evtChannelOpenFlag,
   //                         timeout, &evtChannelHandle);

    clEventFinalize(evtHandle);
    
    return;
}

/*********************************************************************
                     Event DataGet Testing
**********************************************************************/
#if 0

Mynk - NTI void clEvtDataGetCodeCoveTest()
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtHandle;
    const ClEventCallbacksT evtCallbacks;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventChannelHandleT evtChannelHandle;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };

    rc = clEventInitialize(&evtHandle, &evtCallbacks, &version);
    while (1)
    {

        rc = clEventChannelOpen(evtHandle, &channelName,
                                CL_EVENT_CHANNEL_SUBSCRIBER |
                                CL_EVENT_CHANNEL_PUBLISHER, 50000,
                                &evtChannelHandle);
        if (CL_OK == rc)
        {
            break;
        }
        clEventChannelClose(evtChannelHandle);
    }

    clEventFinalize(evtHandle);
}
#endif

void clEvtDataGetTestWrapper(ClEventHandleT eventHandle, void *pEventData,
                             ClSizeT *pEventDataSize)
{
    ClRcT rc = CL_OK;

    rc = clEventDataGet(eventHandle, pEventData, pEventDataSize);
    clEvtTestResultPrint(rc);

    return;
}


ClRcT clEvtDataGetTest()
{
    ClRcT rc = CL_OK;
    ClNameT channelName = { strlen(EVT_TEST_CHANNEL_NAME),
        EVT_TEST_CHANNEL_NAME
    };
    ClEventChannelOpenFlagsT evtChannelOpenFlag =
        CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER |
        CL_EVENT_GLOBAL_CHANNEL;
    ClTimeT timeout = 3000;
    ClVersionT version = CL_EVENT_VERSION;
    ClEventSubscriptionIdT subscriptionId = 0xABCD;
    ClUint32T evtData;
    tPriority = 0;
    tRetentionTime = 0;
    tPayLoad = 0x1234;    
    ClSizeT evtDataSize;

    /*** Set the attribute ***/
    clEvtUnitTestAttributeSet((char *)__FUNCTION__, gEvtDataGetTestResult);

    /*
    ** Invalid Test case 0: EventDataGet() before init, retuns CL_EVENT_ERR_INIT_NOT_DONE
    */
    clEvtDataGetTestWrapper(100, &evtData,&evtDataSize);
    /*-----------------------------------------------------------------------*/

    /***** Code coverage testing *****
    clEvtDataGetCodeCoveTest(); -Mynk NTI */

    rc = clEventInitialize(&gEvtHandle, &gEvtCallbacks, &version);

    rc = clEventChannelOpen(gEvtHandle, &channelName, evtChannelOpenFlag,
                            timeout, &gEvtChannelHandle);
    /*
    ** Invalid Test case 1: EventDataGet() with bad handle, retuns CL_EVENT_ERR_BAD_HANDLE
    */
    clEvtDataGetTestWrapper(100, &evtData,&evtDataSize);
    /*-----------------------------------------------------------------------*/

    rc = clEventSubscribe(gEvtChannelHandle, &gSubscribeFilters, subscriptionId, (void *) 0xAABB);  /* Subscribe 
                                                                                                     */

    rc = clEventAllocate(gEvtChannelHandle, &gEvtEventHandle);  /* Allocate
                                                                 * Event */

    /* Set Attributes */
    rc = clEventAttributesSet(gEvtEventHandle, &gPatternArray, tPriority, tRetentionTime, &tPublisherName);         
    rc = clEventPublish(gEvtEventHandle, (const void *) &tPayLoad,
                        sizeof(tPayLoad), &tEventId);
    //clEvtDataGetTestWrapper(gEvtEventHandle,&evtData,&evtDataSize);
    //clEventFree(gEvtEventHandle);
    /*-----------------------------------------------------------------------*/
#if 0
    /*
     * Test Case 2 - Space Check, returns CL_EVENT_ERR_NO_SPACE 
     */
    clEvtDataGetTestWrapper();

#endif
#if 0                           /* FIXME */
    clEventFinalize(gEvtHandle);    /* Finalize */
#endif

    return CL_OK;
}

/*********************************************************************
                         Starting Test
**********************************************************************/
ClRcT startEvtUnitTest()
{

    clEvtInitializeTest();
    clEvtFinalizeTest();
    
   //clEvtChannelOpenAsyncTest();/*Shanthi - NTC*/

    clEvtChannelOpenTest();
    clEvtChannelCloseTest();

    clEvtAllocateTest();
    clEvtFreeTest();
    
    clEvtAttributesSetTest();
    clEvtAttributesGetTest();

    clEvtPublishTest();

    clEvtSubscribeTest();
    clEvtUnsubscribeTest();
    clEvtCookieGet();           /* Shanthi - NTC */
    /*
     * Moved here As we can't do a finalize in case of this test it is giving
     * problems 
     */
   clEvtDataGetTest();

   fclose(gpEvtTestFp);

    return CL_OK;
}

/*****************************************************
                EOnization Code
*****************************************************/

ClEoExecutionObjT *gEvtUnitTestEoObj;

static ClEoPayloadWithReplyCallbackT gClEvtUnitTestFuncList[] = {

    (ClEoPayloadWithReplyCallbackT) NULL,   /* 0 */
};

ClUint32T gClEvtUnitTestFuncListSize =
    sizeof(gClEvtUnitTestFuncList) / sizeof(ClEoPayloadWithReplyCallbackT);

static ClCpmHandleT gClEvtUnitTestCpmHandle;

ClRcT clEvtUnitTestTerminate(ClInvocationT invocation, const ClNameT *compName)
{
    ClRcT rc;

    rc = clCpmComponentUnregister(gClEvtUnitTestCpmHandle, compName, NULL);
    rc = clCpmClientFinalize(gClEvtUnitTestCpmHandle);

    clCpmResponse(gClEvtUnitTestCpmHandle, invocation, CL_OK);

    return CL_OK;
}


ClRcT clEvtUnitTestCpmInit()
{
    ClNameT appName;
    ClCpmCallbacksT callbacks;
    ClVersionT version;
    ClIocPortT iocPort;
    ClRcT rc = CL_OK;

    version.releaseCode = 'B';
    version.majorVersion = 0x1;
    version.minorVersion = 0x1;

    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = clEvtUnitTestTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    clEoMyEoIocPortGet(&iocPort);
    rc = clCpmClientInitialize(&gClEvtUnitTestCpmHandle, &callbacks, &version);
    rc = clCpmComponentNameGet(gClEvtUnitTestCpmHandle, &appName);
    rc = clCpmComponentRegister(gClEvtUnitTestCpmHandle, &appName, NULL);

    return CL_OK;
}


ClRcT clEvtUnitTestInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

#if 0
    ClSvcHandleT svcHandle = { 0 };
    ClOsalTaskIdT taskId = 0;
#endif

    rc = clEoMyEoObjectGet(&gEvtUnitTestEoObj);

    rc = clEoClientInstall(gEvtUnitTestEoObj, CL_EO_NATIVE_COMPONENT_TABLE_ID,
                           gClEvtUnitTestFuncList, 0,
                           gClEvtUnitTestFuncListSize);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,
                       ("Installing Native table failed [%x]\n\r", rc));
    }
    clEvtUnitTestCpmInit();

#if 0
    svcHandle.cpmHandle = &gClEvtUnitTestCpmHandle;
    rc = clDispatchThreadCreate(gEvtUnitTestEoObj, &taskId, svcHandle);
#endif

    clEvtUnitTestInit();

    startEvtUnitTest();

    return CL_OK;
}

ClRcT clEvtUnitTestFinalize()
{
    ClRcT rc = CL_OK;

    rc = clEoClientUninstall(gEvtUnitTestEoObj,
                             CL_EO_NATIVE_COMPONENT_TABLE_ID);
    return CL_OK;
}

ClRcT clEventUnitTestStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

ClRcT clEventUnitTestHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq = CL_EO_DEFAULT_POLL;
    schFeedback->status = 0;
    return CL_OK;
}

ClEoConfigT clEoConfig = {
//    "EVENT_EO",                 /* EO Name */
    "EUT",                /* EO Name */
    1,                          /* EO Thread Priority */
    1,                          /* No of EO thread needed */
    0,                          /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,  /* Whether to use main thread for eo Recv or
                                 * not */
    clEvtUnitTestInitialize,    /* Function CallBack to initialize the
                                 * Application */
    clEvtUnitTestFinalize,      /* Function Callback to Terminate the
                                 * Application */
    clEventUnitTestStateChange, /* Function Callback to change the Application
                                 * state */
    clEventUnitTestHealthCheck, /* Function Callback to change the Application
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
