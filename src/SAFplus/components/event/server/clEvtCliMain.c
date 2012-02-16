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
 * File        : clEvtCliMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains the Command Line Interface functionality
 *          for the EM API making it easier to debug.
 *****************************************************************************/

/*
 * INCLUDES 
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

/*
 * #include "clOmErrors.h"
 */
#include <clEoApi.h>
#include "clEventApi.h"
#include "clEventExtApi.h"

#include "clEvtCliCommon.h"

#include <clDebugApi.h>
/*
 * #include "commonMetaStruct.h" #include "compMgrClientCompId.h"
 */
#ifdef MORE_CODE_COVERAGE
# include<clCodeCovStub.h>
#endif

#include "clEventCkptIpi.h"

/*
 * Following Added for the purpose of Display 
 */
#include "clEventServerIpi.h"
#define CL_EVT_SHOW_ALL_STRING "-a"

typedef struct EvtTestDisplayInfo
{
    ClUint16T channelScope;
    ClUint16T disDetailFlag;
    ClNameT channelName;
    ClDebugPrintHandleT dbgPrintHandle;

} ClEvtTestDisplayInfoT;




#ifdef CKPT_ENABLED

ClDebugPrintHandleT gEvtCkptPrintHandle = 0;

/*
 * Following added for CKPT API in the CLI 
 */
static ClRcT cliEvtCkptAll(int argc, char **argv, char **retStr);
static ClRcT cliEvtCkptShow(int argc, char **argv, char **retStr);
static ClRcT cliEvtCkptRecover(int argc, char **argv, char **retStr);
#endif

static ClRcT cliEvtInit(int argc, char **argv, char **retStr);
static ClRcT cliEvtFinalize(int argc, char **argv, char **retStr);
static ClRcT cliEvtChannelOpen(int argc, char **argv, char **retStr);
static ClRcT cliEvtChannelOpenAsync(int argc, char **argv, char **retStr);
static ClRcT cliEvtChannelClose(int argc, char **argv, char **retStr);

static ClRcT cliEvtEventSubscribe(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventExtSubscribe(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventStrSubscribe(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventUnsubscribe(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventPublish(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventExtPublish(int argc, char **argv, char **retStr);
static ClRcT cliEvtEventStrPublish(int argc, char **argv, char **retStr);
static ClRcT cliEvtShow(int argc, char **argv, char **retStr);


/*
 * Filter and pattern information 
 */
#define gPatt1 "1234"
#define gPatt1_size sizeof(gPatt1)

#define gPatt2 "12345"
#define gPatt2_size sizeof(gPatt2)

#define gPatt3 "123456"
#define gPatt3_size sizeof(gPatt3)

#define gPatt4 "1234567"
#define gPatt4_size sizeof(gPatt4)

ClHandleT  gEventDebugReg = CL_HANDLE_INVALID_VALUE;

static ClEventFilterT gFilters[] = {
    {CL_EVENT_EXACT_FILTER, {0, gPatt1_size, (ClUint8T *) gPatt1}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt2_size, (ClUint8T *) gPatt2}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt3_size, (ClUint8T *) gPatt3}},
    {CL_EVENT_EXACT_FILTER, {0, gPatt4_size, (ClUint8T *) gPatt4}}
};

static ClEventFilterArrayT gSubscribeFilters = {
    sizeof(gFilters) / sizeof(ClEventFilterT),
    gFilters
};


static ClEventPatternT gPatterns[] = {
    {0, gPatt1_size, (ClUint8T *) gPatt1},
    {0, gPatt2_size, (ClUint8T *) gPatt2},
    {0, gPatt3_size, (ClUint8T *) gPatt3},
    {0, gPatt4_size, (ClUint8T *) gPatt4}
};

static ClEventPatternArrayT gPatternArray = {
    0,
    sizeof(gPatterns) / sizeof(ClEventPatternT),
    gPatterns
};

#ifdef NTR
#define CL_EVT_RMD_MAX_RETRIES 3
#define CL_EVT_RMD_TIME_OUT     10*1000  /* 10 second */
#endif

#define CL_EVENT_CLI_STR_LEN 1024
static char gEventCliStr[CL_EVENT_CLI_STR_LEN];


/*
 * MACROS 
 */

static ClDebugFuncEntryT gClEventCliTab[] = {
    {(ClDebugCallbackT) cliEvtInit, "initialize",
     "Initalize the Event library"},
    {(ClDebugCallbackT) cliEvtFinalize, "finalize",
     "Finalize the Event library"},
    {(ClDebugCallbackT) cliEvtChannelOpen, "openchannel",
     "Open an Event channel"},
    {(ClDebugCallbackT) cliEvtChannelOpenAsync, "asyncopenchannel",
     "Open an Event channel Asynchronosly"},
    {(ClDebugCallbackT) cliEvtEventSubscribe, "subscribe",
     "Subscribe to an Event"},
    {(ClDebugCallbackT) cliEvtEventExtSubscribe, "subint",
     "Subscribe with specified integer filter"},
    {(ClDebugCallbackT) cliEvtEventStrSubscribe, "substr",
     "Subscribe with specified string filter"},
    {(ClDebugCallbackT) cliEvtEventPublish, "publish", "Publish an Event"},
    {(ClDebugCallbackT) cliEvtEventExtPublish, "pubint",
     "Publish Event with specified integer pattern"},
    {(ClDebugCallbackT) cliEvtEventStrPublish, "pubstr",
     "Publish Event with specified string pattern"},
    {(ClDebugCallbackT) cliEvtEventUnsubscribe, "unsubscribe",
     "Unsubscribe an Event"},
    {(ClDebugCallbackT) cliEvtChannelClose, "closechannel",
     "Close the event channel"},
    {(ClDebugCallbackT) cliEvtShow, "display",
     "Display the subscription Informataion"},
#ifdef CKPT_ENABLED
    {(ClDebugCallbackT) cliEvtCkptAll, "checkpoint",
     "Check Point the Event Datastructures"},
    {(ClDebugCallbackT) cliEvtCkptShow, "showcheckpoints",
     "Display the Check Pointed Data"},
    {(ClDebugCallbackT) cliEvtCkptRecover, "recover",
     "Simulate Event Server Recovery"},
#endif
};

ClDebugModEntryT clModTab[] = {
    {"Event", "Event", gClEventCliTab, "Event server commands"},
    {"", "", 0, ""}
};

ClRcT clEventDebugRegister(ClEoExecutionObjT *pEoObj)
{
    ClRcT rc = CL_OK;

    rc = clDebugPromptSet("EVENT");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }
    return clDebugRegister(gClEventCliTab,
                           sizeof(gClEventCliTab) / sizeof(gClEventCliTab[0]), 
                           &gEventDebugReg);
}

ClRcT clEventDebugDeregister(ClEoExecutionObjT *pEoObj)
{
    return clDebugDeregister(gEventDebugReg);
}

static ClEventInitHandleT gEvtHandle;
static ClEventChannelHandleT gEvtChannelHandle;

void clEvtChannelOpenCallback_t(ClInvocationT invocation,
                                ClEventChannelHandleT channelHandle,
                                ClRcT apiResult)
{
    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("************* Async Channel Open Callback *************\n");
    clOsalPrintf("*******************************************************\n");
    clOsalPrintf("             Invocation        : 0x%X\n", invocation);
    clOsalPrintf("             Channel Handle    : %#llX\n", channelHandle);
    clOsalPrintf("             API Result        : 0x%X\n", apiResult);
    clOsalPrintf("*******************************************************\n");

    return;
}

void clEvtEventDeliverCallback_t(ClEventSubscriptionIdT subscriptionId,
                                 ClEventHandleT eventHandle,
                                 ClSizeT eventDataSize)
{
    ClRcT rc = CL_OK;

    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    ClNameT publisherName = { 0 };
    ClEventIdT eventId = 0;
    ClUint32T payLoad[2] = { 0 };
    ClSizeT payLoadLen = 20;
    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    void *pCookie = NULL;

    ClUint8T i = 0;             /* We know the no. of patterns is 4 */

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
    clOsalPrintf("             Publish Time      : 0x%llX\n",
                 publishTime);
    clOsalPrintf("             Retention Time    : 0x%llX\n",
                 retentionTime);
    clOsalPrintf("             Publisher Name    : %.*s\n",
                 publisherName.length, publisherName.value);
    clOsalPrintf("             EventID           : 0x%X\n", eventId);
    clOsalPrintf("             Payload[0]        : 0x%X\n", payLoad[0]);
    clOsalPrintf("             Payload[1]        : 0x%X\n", payLoad[1]);
    clOsalPrintf("             Payload Len       : %#llX\n",
                 payLoadLen);
    clOsalPrintf("             Event Data Size   : %#llX\n",
                 eventDataSize);
    clOsalPrintf("             Cookie            : %#llX\n", pCookie);
    clOsalPrintf("-------------------------------------------------------\n");

    /*
     * Free the allocated Event 
     */
    rc = clEventFree(eventHandle);

    for (i = 0; i < patternArray.patternsNumber; i++)
    {
        clHeapFree(patternArray.pPatterns[i].pPattern);
    }
    clHeapFree(patternArray.pPatterns); /* User is expected to free this memory 
                                         * if he doesn't allocate it */

    return;
}

static const ClEventCallbacksT gEvtCallbacks = {
    clEvtChannelOpenCallback_t,
    clEvtEventDeliverCallback_t
};


#define ONE_ARGUMENT    1
#define TWO_ARGUMENT    2
#define THREE_ARGUMENT  3
#define FOUR_ARGUMENT   4
#define FIVE_ARGUMENT   5
#define SIX_ARGUMENT    6


void clEvtCliStrPrint(char *str, char **retStr)
{

    *retStr = clHeapAllocate(strlen(str) + 1);
    if (NULL == *retStr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Malloc Failed \n"));
        return;
    }
    sprintf(*retStr, str);
    return;
}

#ifdef CKPT_ENABLED
       /******** Begin of Check Pointing API **********/

/*
 * API to Check Point Information 
 */

static ClRcT cliEvtCkptAll(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClUint32T choice;

    if (argc != TWO_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: checkpoint <Choice>\n"
                         "\tChoice [DEC] - [0:All] [1:UserInfo] [2:ECHInfo] [3:SubsInfo]\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%d", (ClInt32T *) &choice);

    switch (choice)
    {
        case 0:
            rc = clEvtCkptCheckPointAll();
            break;

        case 1:
            rc = clEvtCkptUserInfoCheckPoint();
            break;

        case 2:
            rc = clEvtCkptECHInfoCheckPoint();
            break;

        case 3:
            rc = clEvtCkptSubsInfoCheckPoint();
            break;

        default:
            clEvtCliStrPrint("Usage: checkpoint <Choice>\n"
                             "\tChoice [DEC] - [0:All] [1:UserInfo] [2:ECHInfo] [3:SubsInfo]\n",
                             retStr);
            break;
    }
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Checkpointing Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Checkpointing Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Checkpointing Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    return CL_OK;
}

/*
 * API To display check Points 
 */

static void cliEvtCkptUserInfoShow(ClEvtCkptUserInfoWithLenT *pUserInfoWithLen)
{
    ClEvtCkptUserInfoT *pUserInfo = pUserInfoWithLen->pUserInfo;
    ClUint32T bytesReceived = pUserInfoWithLen->userInfoLen;
    ClUint32T bytesRead = 0;

    for (bytesRead = 0; bytesRead < bytesReceived;
         bytesRead += sizeof(ClEvtCkptUserInfoT), pUserInfo++)
    {
        clDebugPrint(gEvtCkptPrintHandle,
                     "\n\n=========================================================\n");
        clDebugPrint(gEvtCkptPrintHandle, "\t\t     User Info \n");
        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");

        clDebugPrint(gEvtCkptPrintHandle, "Bytes Received = %u\n\n",
                     bytesReceived);
        clDebugPrint(gEvtCkptPrintHandle, "Bytes Read     = %lu\n",
                     (unsigned long)(bytesRead + sizeof(ClEvtCkptUserInfoT)));

        clDebugPrint(gEvtCkptPrintHandle, "Operation = %u\n",
                     pUserInfo->operation);
        clDebugPrint(gEvtCkptPrintHandle, "User IOC Port  = 0x%llx\n",
                     pUserInfo->userId.eoIocPort);
        clDebugPrint(gEvtCkptPrintHandle, "User EvtHandle = 0x%llx\n",
                     pUserInfo->userId.evtHandle);

        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");
    }
}

static ClRcT cliEvtCkptUserInfoRead(void)
{
    ClRcT rc = CL_OK;

    ClEvtCkptUserInfoWithLenT userInfoWithLen = { NULL };

    rc = clEvtCkptUserInfoRead(&userInfoWithLen);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nUser Info Read Failed [0x%X]\n", rc));
    }
    else if (CL_OK == rc && 0 != userInfoWithLen.userInfoLen)
    {
        cliEvtCkptUserInfoShow(&userInfoWithLen);
    }

    return CL_OK;
}


static void cliEvtCkptECHInfoShow(ClEvtCkptECHInfoWithLenT *pECHInfoWithLen)
{
    ClEvtCkptECHInfoT *pECHInfo = pECHInfoWithLen->pECHInfo;
    ClUint32T bytesReceived = pECHInfoWithLen->echInfoLen;
    ClUint32T bytesRead = 0;

    for (bytesRead = 0; bytesRead < bytesReceived;
         bytesRead += sizeof(ClEvtCkptECHInfoT), pECHInfo++)
    {
        clDebugPrint(gEvtCkptPrintHandle,
                     "\n\n=========================================================\n");
        clDebugPrint(gEvtCkptPrintHandle, "\t\t     ECH Info \n");
        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");

        clDebugPrint(gEvtCkptPrintHandle, "Bytes Received = %u\n\n",
                     bytesReceived);
        clDebugPrint(gEvtCkptPrintHandle, "Bytes Read     = %lu\n",
                     (unsigned long)(bytesRead + sizeof(ClEvtCkptECHInfoT)));

        clDebugPrint(gEvtCkptPrintHandle, "Operation      = %u\n",
                     pECHInfo->operation);

        clDebugPrint(gEvtCkptPrintHandle, "User IOC Port  = 0x%llx\n",
                     pECHInfo->userId.eoIocPort);
        clDebugPrint(gEvtCkptPrintHandle, "User EvtHandle = 0x%llx\n",
                     pECHInfo->userId.evtHandle);

        clDebugPrint(gEvtCkptPrintHandle, "Channel Handle = 0x%X\n",
                     pECHInfo->chanHandle);

        clDebugPrint(gEvtCkptPrintHandle, "Channel Name   = %.*s\n",
                     pECHInfo->chanName.length, pECHInfo->chanName.value);

        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");
    }
}

static ClRcT cliEvtCkptECHInfoRead(ClUint32T channelScope)
{
    ClRcT rc = CL_OK;

    ClEvtCkptECHInfoWithLenT echInfoWithLen = { NULL };


    echInfoWithLen.scope = channelScope;

    rc = clEvtCkptECHInfoRead(&echInfoWithLen);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nCKPT Local ECH Read Failed [0x%X]\n", rc));
    }
    else if (0 != echInfoWithLen.echInfoLen)
    {
        cliEvtCkptECHInfoShow(&echInfoWithLen);
    }

    return CL_OK;
}


static ClUint32T flgRBE = 0;    /* To check if RBE needs to be displayed */
static void cliEvtCkptSubsInfoShow(ClEvtCkptSubsInfoWithLenT *pSubsInfoWithLen)
{
    ClRcT rc = CL_OK;

    ClEvtCkptSubsInfoT *pSubsInfo = pSubsInfoWithLen->pSubsInfo;

    ClUint32T bytesReceived = pSubsInfoWithLen->subsInfoLen;
    ClUint32T bytesRead = 0;

    while (bytesRead < bytesReceived)
    {
        clDebugPrint(gEvtCkptPrintHandle,
                     "\n\n=========================================================\n");
        clDebugPrint(gEvtCkptPrintHandle, "\t\t     Subs Info \n");
        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");

        clDebugPrint(gEvtCkptPrintHandle, "Bytes Received = %u\n\n",
                     bytesReceived);
        clDebugPrint(gEvtCkptPrintHandle, "Bytes Read     = %lu\n",
                     (unsigned long)(bytesRead +
                                     sizeof(ClEvtCkptSubsInfoT) +
                                     pSubsInfo->packedRbeLen));

        clDebugPrint(gEvtCkptPrintHandle, "Operation      = %u\n",
                     pSubsInfo->operation);

        clDebugPrint(gEvtCkptPrintHandle, "User IOC Port  = 0x%llx\n",
                     pSubsInfo->userId.eoIocPort);
        clDebugPrint(gEvtCkptPrintHandle, "User EvtHandle = 0x%llx\n",
                     pSubsInfo->userId.evtHandle);

        clDebugPrint(gEvtCkptPrintHandle, "Cookie    = 0x%llx\n",
                     pSubsInfo->cookie);
        clDebugPrint(gEvtCkptPrintHandle, "Subs Id   = 0x%X\n",
                     pSubsInfo->subsId);
        clDebugPrint(gEvtCkptPrintHandle, "Commport  = 0x%X\n",
                     pSubsInfo->commPort);
        clDebugPrint(gEvtCkptPrintHandle, "RBE Len   = %u\n\n",
                     pSubsInfo->packedRbeLen);

        clDebugPrint(gEvtCkptPrintHandle, "Channel Handle = 0x%X\n",
                     pSubsInfo->chanHandle);

        pSubsInfoWithLen->chanName.value[pSubsInfoWithLen->chanName.length +
                                         1] = '\0';
        clDebugPrint(gEvtCkptPrintHandle, "Channel Name   = %.*s\n\n",
                     pSubsInfoWithLen->chanName.length,
                     pSubsInfoWithLen->chanName.value);

        if (pSubsInfo->packedRbeLen && 1 == flgRBE)
        {
            ClRuleExprT *pRbeExpr = NULL;

            pSubsInfo->packedRbe =
            (ClUint8T *)((ClCharT *)pSubsInfo+sizeof(ClEvtCkptSubsInfoT));

            clDebugPrint(gEvtCkptPrintHandle, "RBE Expression:\n");
            rc = clRuleExprUnpack(pSubsInfo->packedRbe,
                                  pSubsInfo->packedRbeLen,
                                  &pRbeExpr);
            if (CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("\nRBE Expression Unpack Failed [0x%X]\n", rc));
                /*
                 ** If not successful continue the Reconstruction
                 ** and log a message at debug level.
                 */
            }

            rc = clRuleExprPrintToMessageBuffer(gEvtCkptPrintHandle, pRbeExpr);
            if (CL_ERR_NULL_POINTER == CL_GET_ERROR_CODE(rc))
            {
                clDebugPrint(gEvtCkptPrintHandle, "\n NULL Filter\n");
            }

            clRuleExprDeallocate(pRbeExpr);
        }

        clDebugPrint(gEvtCkptPrintHandle,
                     "=========================================================\n");

        /*** Update the length and the data pointer ***/

        bytesRead += (sizeof(ClEvtCkptSubsInfoT)+pSubsInfo->packedRbeLen);

        if (bytesRead < bytesReceived)  /* To Avoid accessing data beyond
                                         * boundary */
        {
            pSubsInfo = (ClEvtCkptSubsInfoT *) ((ClCharT *)pSubsInfo +
                                                sizeof(ClEvtCkptSubsInfoT) +
                                                pSubsInfo->packedRbeLen);
        }
    }
}

static ClRcT cliEvtCkptSubsInfoReadWalk(ClCntKeyHandleT userKey,
                                        ClCntDataHandleT userData,
                                        ClCntArgHandleT userArg,
                                        ClUint32T dataLength)
{
    ClRcT rc = CL_OK;

    ClEvtCkptSubsInfoWithLenT *pSubsInfoWithLen =
        (ClEvtCkptSubsInfoWithLenT *) userArg;


    CL_FUNC_ENTER();

    clEvtUtilsNameCpy(&pSubsInfoWithLen->chanName,
                      &((ClEvtChannelKeyT *) userKey)->channelName);

    rc = clEvtCkptSubsInfoRead(pSubsInfoWithLen);
    if (0 != pSubsInfoWithLen->subsInfoLen)
    {
        cliEvtCkptSubsInfoShow(pSubsInfoWithLen);
    }

    CL_FUNC_EXIT();
    return CL_OK;
}


static ClRcT cliEvtCkptSubsInfoRead(ClUint32T channelScope)
{
    ClRcT rc = CL_OK;

    ClEvtCkptSubsInfoWithLenT subsInfoWithLen = { NULL };

    subsInfoWithLen.scope = channelScope;

    ClCntHandleT chanContainer =
        (CL_EVENT_LOCAL_CHANNEL ==
         channelScope) ? gpEvtLocalECHDb->
        evtChannelContainer : gpEvtGlobalECHDb->evtChannelContainer;

    rc = clCntWalk(chanContainer, cliEvtCkptSubsInfoReadWalk,
                   (ClCntArgHandleT) &subsInfoWithLen, 0);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\ncliEvtCkptSubsInfoReadWalk Failed [0x%X]\n", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

static ClRcT cliEvtCkptShowAll()
{
    ClRcT rc = CL_OK;

    rc = cliEvtCkptUserInfoRead();

    rc = cliEvtCkptECHInfoRead(CL_EVENT_LOCAL_CHANNEL);
    rc = cliEvtCkptECHInfoRead(CL_EVENT_GLOBAL_CHANNEL);

    rc = cliEvtCkptSubsInfoRead(CL_EVENT_LOCAL_CHANNEL);
    rc = cliEvtCkptSubsInfoRead(CL_EVENT_GLOBAL_CHANNEL);

    return CL_OK;
}


static ClRcT cliEvtCkptShow(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClUint32T choice;
    ClUint32T channelScope;

    rc = clDebugPrintInitialize(&gEvtCkptPrintHandle);

    if (argc != THREE_ARGUMENT && argc != FOUR_ARGUMENT)
    {
        clDebugPrint(gEvtCkptPrintHandle,
                     "Usage: showcheckpoints <Choice> <ChannelScope> [Flag]\n"
                     "\tChoice [DEC] - [0:All] [1:UserInfo] [2:ECHInfo] [3:SubsInfo]\n"
                     "\tChannelScope [DEC] - [0:Local] [1:Global]\n"
                     "\tFlag [DEC] - Subscriber Info[0: Brief] [1: Detailed]\n");

        rc = clDebugPrintFinalize(&gEvtCkptPrintHandle, retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%d", (ClInt32T *) &choice);
    sscanf(argv[2], "%d", (ClInt32T *) &channelScope);

    sscanf(argv[3], "%d", (ClInt32T *) &flgRBE);

    if (choice > 3 || channelScope > 1 || flgRBE > 1)
    {
        clDebugPrint(gEvtCkptPrintHandle,
                     "Invalid Parameter Passed\n\n"
                     "Usage: showcheckpoints <Choice> <ChannelScope> [Flag]\n"
                     "\tChoice [DEC] - [0:All] [1:UserInfo] [2:ECHInfo] [3:SubsInfo]\n"
                     "\tChannelScope [DEC] - [0:Local] [1:Global]\n"
                     "\tFlag [DEC] - Subscriber Info[0: Brief] [1: Detailed]\n");

        rc = clDebugPrintFinalize(&gEvtCkptPrintHandle, retStr);

        return CL_OK;
    }

    switch (choice)
    {
        case 0:
            rc = cliEvtCkptShowAll();
            break;

        case 1:
            rc = cliEvtCkptUserInfoRead();
            break;

        case 2:
            rc = cliEvtCkptECHInfoRead(channelScope);
            break;

        case 3:
            rc = cliEvtCkptSubsInfoRead(channelScope);
            break;
    }
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Show Checkpoints Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Show Checkpoints Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Show Checkpoints Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    rc = clDebugPrintFinalize(&gEvtCkptPrintHandle, retStr);

    return CL_OK;
}

static ClRcT cliEvtCkptRecover(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    if (argc != ONE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: recover\n", retStr);
        return CL_OK;
    }
    rc = clEvtCkptReconstruct();
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Recovery Simulation Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Recovery Simulation Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Recovery Simulation Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    return CL_OK;
}

        /******** End of Check Pointing API **********/
#endif

ClRcT cliEvtInit(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClVersionT version = CL_EVENT_VERSION;

    if (argc != ONE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: initialize\n ", retStr);
        return CL_OK;
    }

    rc = clEventInitialize(&gEvtHandle, &gEvtCallbacks, &version);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Initialize Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Initialize Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Initialize Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    gEventCliStr[0] = '\0';
    sprintf(gEventCliStr, "Initialize Successful, EvtHandle = [%#llX]\n",
            gEvtHandle);
    clOsalPrintf("Initialize Successful, EvtHandle = [%#llX]\n",  gEvtHandle);
    clEvtCliStrPrint(gEventCliStr, retStr);

    return CL_OK;
}

ClRcT cliEvtFinalize(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    if (argc != TWO_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: finalize <EvtHandle>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &gEvtHandle);

    rc = clEventFinalize(gEvtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Finalize Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Finalize Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Finalize Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Finalize Successful\n", retStr);

    return CL_OK;
}


ClRcT cliEvtChannelOpen(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClNameT channelName = { 0 };
    ClUint32T channelScope = 0;
    ClUint32T userType = 0;

    if (argc != FIVE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: openchannel <EvtHandle> <ChannelName> "
                         "<ChannelScope> <UserType>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tChannelName [STRING] - Event Channel Identifier\n"
                         "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
                         "\tUserType [DEC] - Type of the User [0:Subscriber] [1:Publisher] "
                         "[2:Both]\n", retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%p", (ClPtrT *) &gEvtHandle);
    sscanf(argv[2], "%s", channelName.value);
    sscanf(argv[3], "%d", (ClInt32T *) &channelScope);
    sscanf(argv[4], "%d", (ClInt32T *) &userType);

    if (channelScope > 1 || userType > 2)
    {
        clEvtCliStrPrint("Invalid Parameter Passed\n\n"
                         "Usage: openchannel <EvtHandle> <ChannelName> "
                         "<ChannelScope> <UserType>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tChannelName [STRING] - Event Channel Identifier\n"
                         "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
                         "\tUserType [DEC] - Type of the User [0:Subscriber] [1:Publisher] "
                         "[2:Both]\n", retStr);

        return CL_OK;
    }

    switch (userType)
    {
        case 0:
            userType = CL_EVENT_CHANNEL_SUBSCRIBER;
            break;
        case 1:
            userType = CL_EVENT_CHANNEL_PUBLISHER;
            break;
        case 2:
            userType = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
            break;
    }

    channelName.length = strlen(channelName.value);

    rc = clEventChannelOpen(gEvtHandle, &channelName, userType | channelScope,
                            1000 * 5, &gEvtChannelHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Channel Open Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Channel Open Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Channel Open Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    gEventCliStr[0] = '\0';
    sprintf(gEventCliStr, "Channel Open Successful, ECH Handle[%#llX]\n",
             gEvtChannelHandle);
    clOsalPrintf("Channel Open Successful, ECH Handle[0x%X]\n",
                  gEvtChannelHandle);
    clEvtCliStrPrint(gEventCliStr, retStr);

    return CL_OK;
}

ClRcT cliEvtChannelOpenAsync(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClInvocationT invocation = 0;
    ClNameT channelName = { 0 };
    ClUint32T channelScope = 0;
    ClUint32T userType = 0;

    if (argc != SIX_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: asyncopenchannel <EvtHandle> <Invocation> "
                         "<ChannelName> <ChannelScope> <UserType>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tInvocation [HEX] - Value specified to Identify the Callback\n"
                         "\tChannelName [STRING] - Event Channel Identifier\n"
                         "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
                         "\tUserType [DEC] - Type of the User [0:Subscriber] [1:Publisher] "
                         "[2:Both]\n", retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%p", (ClPtrT *) &gEvtHandle);
    sscanf(argv[2], "%lx", (unsigned long *) &invocation);
    sscanf(argv[3], "%s", channelName.value);
    sscanf(argv[4], "%d", (ClInt32T *) &channelScope);
    sscanf(argv[5], "%d", (ClInt32T *) &userType);

    if (channelScope > 1 || userType > 2)
    {
        clEvtCliStrPrint("Wrong Flag Passed\n"
                         "Usage: asyncopenchannel <EvtHandle> <Invocation> "
                         "<ChannelName> <ChannelScope> <UserType>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tInvocation [HEX] - Value specified to Identify the Callback\n"
                         "\tChannelName [STRING] - Event Channel Identifier\n"
                         "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
                         "\tUserType [DEC] - Type of the User [0:Subscriber] [1:Publisher] "
                         "[2:Both]\n", retStr);

        return CL_OK;
    }

    switch (userType)
    {
        case 0:
            userType = CL_EVENT_CHANNEL_SUBSCRIBER;
            break;
        case 1:
            userType = CL_EVENT_CHANNEL_PUBLISHER;
            break;
        case 2:
            userType = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER;
            break;
    }

    channelName.length = strlen(channelName.value);

    rc = clEventChannelOpenAsync(gEvtHandle, invocation, &channelName,
                                 userType | channelScope);

#ifdef NAV
    check
#endif
        if (CL_OK != rc)        /* Added as this was returning a non-zero value 
                                 * in case of failure */
    {
        gEvtChannelHandle = 0;
    }

#ifdef NAV
    gEventCliStr[0] = '\0';
    sprintf(gEventCliStr,
            "Asynchronous Channel Open Successful, ECH Handle[%#llX]\n",
             gEvtChannelHandle);
    clOsalPrintf("Asynchronous Channel Open Successful, ECH Handle[%#llX]\n",
                  gEvtChannelHandle);
    clEvtCliStrPrint(gEventCliStr, retStr);
#endif
    clEvtCliStrPrint("Asynchronous Channel Open Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtChannelClose(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;

    if (argc != TWO_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: CLOSE <ChannelHandle>\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n",
                         retStr);
        return CL_OK;
    }

    sscanf(argv[1], "%llX", &channelHandle);

    rc = clEventChannelClose(channelHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Channel Close Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Channel Close Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Channel Close Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Channel Close Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventSubscribe(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClUint32T subscriptionId = 0;

    EvtTestSubsReq_t evtTestSubsReq = { 0 };

    if (argc != FOUR_ARGUMENT)
    {
        clEvtCliStrPrint
            ("Usage: subscribe <EvtHandle> <ChannelHandle> <SubscriptionID>\n"
             "\tEvtHandle [HEX] - Handle returned on Initialize\n"
             "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
             "\tSubscriptionID [HEX] - Subscriber Identification\n", retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &gEvtHandle);
    sscanf(argv[2], "%llX", &channelHandle);
    sscanf(argv[3], "%x", (ClUint32T *) &subscriptionId);

    evtTestSubsReq.channelHandle = channelHandle;
    evtTestSubsReq.subscriptionId = subscriptionId;

    rc = clEventSubscribe(evtTestSubsReq.channelHandle, &gSubscribeFilters,
                          evtTestSubsReq.subscriptionId, (ClPtrT)(ClWordT)gEvtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Subscription Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Subscription Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Subscription Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Subscription Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventExtSubscribe(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClUint32T eventType = 0;
    ClUint32T subscriptionId = 0;

    EvtTestSubsReq_t evtTestSubsReq = { 0 };

    if (argc != FIVE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: subint <EvtHandle> <ChannelHandle> "
                         "<Filter> <SubscriptionID>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tFilter [HEX] - Integer Filter for the subscription\n"
                         "\tSubscriptionID [HEX] - Subscriber Identification\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &gEvtHandle);
    sscanf(argv[2], "%llX", &channelHandle);
    sscanf(argv[3], "%x", (ClUint32T *) &eventType);
    sscanf(argv[4], "%x", (ClUint32T *) &subscriptionId);

    evtTestSubsReq.channelHandle = channelHandle;
    evtTestSubsReq.subscriptionId = subscriptionId;

    rc = clEventExtSubscribe(evtTestSubsReq.channelHandle, eventType,
                             evtTestSubsReq.subscriptionId,
                             (ClPtrT)(ClWordT)gEvtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Subscription Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Subscription Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Subscription Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Subscription Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventStrSubscribe(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClUint32T subscriptionId = 0;

    EvtTestSubsReq_t evtTestSubsReq = { 0 };

    char eventTypeStr[CL_MAX_NAME_LENGTH] = "";
    ClEventFilterT subsfilter = { CL_EVENT_EXACT_FILTER, {0} }; /* only this
                                                                 * filter for
                                                                 * now */
    ClEventFilterArrayT filterArray = { 1, NULL };  /* No. of Filters is just
                                                     * one */

    if (argc != FIVE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: substr <EvtHandle> <ChannelHandle> "
                         "<Filter> <SubscriptionID>\n"
                         "\tEvtHandle [HEX] - Handle returned on Initialize\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tFilter [STRING] - String Filter for the subscription\n"
                         "\tSubscriptionID [HEX] - Subscriber Identification\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &gEvtHandle);
    sscanf(argv[2], "%llX", &channelHandle);
    sscanf(argv[3], "%s", eventTypeStr);
    sscanf(argv[4], "%x", (ClUint32T *) &subscriptionId);

    evtTestSubsReq.channelHandle = channelHandle;
    evtTestSubsReq.subscriptionId = subscriptionId;

    subsfilter.filter.patternSize = strlen(eventTypeStr);
    subsfilter.filter.pPattern = (ClUint8T *) eventTypeStr;
    filterArray.pFilters = &subsfilter;

    rc = clEventSubscribe(evtTestSubsReq.channelHandle, &filterArray,
                          evtTestSubsReq.subscriptionId, (ClPtrT)(ClWordT)gEvtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Subscription Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Subscription Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Subscription Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Subscription Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventUnsubscribe(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClUint32T subscriptionId = 0;

    EvtTestUnsubsReq_t evtTestUnsubsReq = { 0 };

    if (argc != THREE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: unsub <ChannelHandle> <SubscriptionID>\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tSubscriptionID [HEX] - Subscription Identifier\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &channelHandle);
    sscanf(argv[2], "%x", (ClUint32T *) &subscriptionId);

    evtTestUnsubsReq.channelHandle = channelHandle;
    evtTestUnsubsReq.subscriptionId = subscriptionId;

    rc = clEventUnsubscribe(evtTestUnsubsReq.channelHandle,
                            evtTestUnsubsReq.subscriptionId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Unsubscription Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Unsubscription Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Unsubscription Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    clEvtCliStrPrint("Unsubscription Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventPublish(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClEventHandleT eventHandle = 0;
    ClEventIdT eventId = 0;

    ClUint32T payLoad[2] = { 0x11223344, 0xaabbccdd };

    EvtTestPublishReq_t evtTestPublishReq = { 0 };

    if (argc != THREE_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: publish <ChannelHandle> <PublisherName>\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tPublisherName [STRING] - Name of the Publisher\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &channelHandle);
    sscanf(argv[2], "%s", evtTestPublishReq.publisherName.value);

    evtTestPublishReq.publisherName.length =
        strlen(evtTestPublishReq.publisherName.value);

    evtTestPublishReq.channelHandle = channelHandle;

    rc = clEventAllocate(evtTestPublishReq.channelHandle, &eventHandle);
    rc = clEventAttributesSet(eventHandle, &gPatternArray, 0, 0,
                              &evtTestPublishReq.publisherName);
    rc = clEventPublish(eventHandle, (const void *) &payLoad[0],
                        sizeof(ClUint32T) * 2, &eventId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Publish Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Publish Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Publish Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    rc = clEventFree(eventHandle);

    clEvtCliStrPrint("Publish Successful\n", retStr);

    return CL_OK;
}

ClRcT cliEvtEventExtPublish(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClEventHandleT eventHandle = 0;
    ClEventIdT eventId = 0;
    ClUint32T eventType = 0;

    ClUint32T payLoad[2] = { 0x11223344, 0xaabbccdd };
    EvtTestPublishReq_t evtTestPublishReq = { 0 };

    if (argc != FOUR_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: pubint <ChannelHandle> "
                         "<Pattern> <PublisherName>\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tPattern [HEX] - Integer Pattern to identify the Event\n"
                         "\tPublisherName [STRING] - Name of the Publisher\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &channelHandle);
    sscanf(argv[2], "%x", (ClUint32T *) &eventType);
    sscanf(argv[3], "%s", evtTestPublishReq.publisherName.value);

    evtTestPublishReq.publisherName.length =
        strlen(evtTestPublishReq.publisherName.value);

    evtTestPublishReq.channelHandle = channelHandle;

    rc = clEventAllocate(evtTestPublishReq.channelHandle, &eventHandle);
    rc = clEventExtAttributesSet(eventHandle, eventType, 0, 0,
                                 &evtTestPublishReq.publisherName);
    rc = clEventPublish(eventHandle, (const void *) &payLoad[0],
                        sizeof(ClUint32T) * 2, &eventId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Publish Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Publish Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Publish Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    rc = clEventFree(eventHandle);

    clEvtCliStrPrint("Publish Successful\n", retStr);

    return CL_OK;
}

static ClRcT cliEvtEventStrPublish(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;

    ClEventChannelHandleT channelHandle = 0;
    ClEventHandleT eventHandle = 0;
    ClEventIdT eventId = 0;

    ClUint32T payLoad[2] = { 0x11223344, 0xaabbccdd };

    char eventTypeStr[CL_MAX_NAME_LENGTH] = "";
    ClEventPatternT pattern = { 0 };
    ClEventPatternArrayT patternArray = { 0, 1, NULL }; /* No. of Patterns is
                                                         * just one */
    EvtTestPublishReq_t evtTestPublishReq;

    if (argc != FOUR_ARGUMENT)
    {
        clEvtCliStrPrint("Usage: pubstr <ChannelHandle> "
                         "<Pattern> <PublisherName>\n"
                         "\tChannelHandle [HEX] - Handle returned on Channel Open\n"
                         "\tPattern [STRING] - String Pattern to identify the Event\n"
                         "\tPublisherName [STRING] - Name of the Publisher\n",
                         retStr);

        return CL_OK;
    }

    sscanf(argv[1], "%llX", &channelHandle);
    sscanf(argv[2], "%s", eventTypeStr);
    sscanf(argv[3], "%s", evtTestPublishReq.publisherName.value);

    pattern.patternSize = strlen(eventTypeStr);
    pattern.pPattern = (ClUint8T *) eventTypeStr;
    patternArray.pPatterns = &pattern;

    evtTestPublishReq.publisherName.length =
        strlen(evtTestPublishReq.publisherName.value);
    evtTestPublishReq.channelHandle = channelHandle;

    rc = clEventAllocate(evtTestPublishReq.channelHandle, &eventHandle);
    rc = clEventAttributesSet(eventHandle, &patternArray, 0, 0,
                              &evtTestPublishReq.publisherName);
    rc = clEventPublish(eventHandle, (const void *) &payLoad[0],
                        sizeof(ClUint32T) * 2, &eventId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Publish Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Publish Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Publish Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    rc = clEventFree(eventHandle);

    clEvtCliStrPrint("Publish Successful\n", retStr);

    return CL_OK;
}


/*
 * The following functionality was moved from Server to CLI as suggested
 */

ClRcT cliEvtSubscribeInfoWalk(ClCntKeyHandleT userKey,
                              ClCntDataHandleT userData,
                              ClCntArgHandleT userArg, ClUint32T dataLength)
{
    ClEvtTestDisplayInfoT *pDisInfo = (ClEvtTestDisplayInfoT *) userArg;
    ClEvtSubsKeyT *pEvtSubKey = (ClEvtSubsKeyT *) userKey;


    CL_FUNC_ENTER();

    clDebugPrint(pDisInfo->dbgPrintHandle,
                 "\n Subscriber Details:\n\tInitHandle [0x%llx] \n\tSubscriber Address [0x%llx]\n"
                 "\tSubscriptionID [0x%X] \n\tCookie [0x%llx] \r\n",
                 pEvtSubKey->userId.evtHandle, pEvtSubKey->userId.eoIocPort,
                 pEvtSubKey->subscriptionId, pEvtSubKey->pCookie);

    if (pDisInfo->disDetailFlag == 0)
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT cliEvtTypeInfoWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                         ClCntArgHandleT userArg, ClUint32T dataLength)
{
    ClRcT rc = CL_OK;
    ClEvtTestDisplayInfoT *pDisInfo = (ClEvtTestDisplayInfoT *) userArg;
    ClEvtEventTypeInfoT *pEvtTypeInfo = (ClEvtEventTypeInfoT *) userData;
    ClEvtEventTypeKeyT *pEvtEventTypeKey = (ClEvtEventTypeKeyT *) userKey;

    CL_FUNC_ENTER();

    rc = clCntWalk(pEvtTypeInfo->subscriberInfo, cliEvtSubscribeInfoWalk,
                   (ClCntArgHandleT) pDisInfo, sizeof(ClEvtTestDisplayInfoT));

    if (CL_EVT_TRUE == pDisInfo->disDetailFlag && 0 != pDisInfo->dbgPrintHandle)
    {
        clDebugPrint(pDisInfo->dbgPrintHandle,
                     "\n\n Filter Details (Depicted as RBEs):\n");

        rc = clRuleExprPrintToMessageBuffer(pDisInfo->dbgPrintHandle,
                                            pEvtEventTypeKey->key.pRbeExpr);
        if (CL_ERR_NULL_POINTER == CL_GET_ERROR_CODE(rc))
        {
            clDebugPrint(pDisInfo->dbgPrintHandle, "\n NULL Filter\n");
        }
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT cliEvtECHInfoWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
                        ClCntArgHandleT userArg, ClUint32T dataLength)
{
    ClRcT rc = CL_OK;
    ClEvtTestDisplayInfoT *pDisInfo = (ClEvtTestDisplayInfoT *) userArg;
    ClEvtChannelKeyT *pChannelKey = (ClEvtChannelKeyT *) userKey;

    ClEvtChannelDBT *pEvtChannelDB = NULL;


    CL_FUNC_ENTER();

    pEvtChannelDB = (ClEvtChannelDBT *) userData;

    /*
     * If user wants to know about particular information 
     */
    if (0 !=
        memcmp(pDisInfo->channelName.value, CL_EVT_SHOW_ALL_STRING,
               strlen(CL_EVT_SHOW_ALL_STRING)))
    {
        if ((pDisInfo->channelName.length == pChannelKey->channelName.length) &&
            0 == memcmp(pDisInfo->channelName.value,
                        pChannelKey->channelName.value,
                        pChannelKey->channelName.length))
        {
            clDebugPrint(pDisInfo->dbgPrintHandle,
                         "\n=========================================================\n");

            clDebugPrint(pDisInfo->dbgPrintHandle, "\tInfo about Channel [%s]",
                         pChannelKey->channelName.value);

            clDebugPrint(pDisInfo->dbgPrintHandle,
                         "\n=========================================================\n");

            clDebugPrint(pDisInfo->dbgPrintHandle,
                         "\n Reference count: \r\n"
                         "\tSubscriber [%d] \n\tPublisher [%d]\n",
                         pEvtChannelDB->subscriberRefCount,
                         pEvtChannelDB->publisherRefCount);

            rc = clCntWalk(pEvtChannelDB->eventTypeInfo, cliEvtTypeInfoWalk,
                           (ClCntArgHandleT) pDisInfo,
                           sizeof(ClEvtTestDisplayInfoT));

            clDebugPrint(pDisInfo->dbgPrintHandle,
                         "\n=========================================================\n\n\n");

            CL_FUNC_EXIT();
            return CL_EVENT_ERR_INFO_WALK_STOP;
        }
    }
    else                        /* want to display all the channel information */
    {
        clDebugPrint(pDisInfo->dbgPrintHandle,
                     "\n=========================================================\n");
        clDebugPrint(pDisInfo->dbgPrintHandle, "\tInfo about Channel [%s]",
                     pChannelKey->channelName.value);
        clDebugPrint(pDisInfo->dbgPrintHandle,
                     "\r\n=========================================================\n");

        clDebugPrint(pDisInfo->dbgPrintHandle,
                     "\n Reference count: \r\n"
                     "\tSubscriber [%d] Publisher [%d]\r\n",
                     pEvtChannelDB->subscriberRefCount,
                     pEvtChannelDB->publisherRefCount);


        rc = clCntWalk(pEvtChannelDB->eventTypeInfo, cliEvtTypeInfoWalk,
                       (ClCntArgHandleT) pDisInfo,
                       sizeof(ClEvtTestDisplayInfoT));
        clDebugPrint(pDisInfo->dbgPrintHandle,
                     "\n=========================================================\n\n\n");
        if (CL_OK != rc && CL_EVENT_ERR_INFO_WALK_STOP != rc)
        {
            CL_FUNC_EXIT();
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT cliEvtSubsInfoShowLocal(ClEvtTestDisplayInfoT * pDisplayInfo)
{
    ClRcT rc = CL_OK;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;

    ClOsalMutexIdT mutexId = 0;

    CL_FUNC_ENTER();

    if (CL_EVENT_GLOBAL_CHANNEL == pDisplayInfo->channelScope)
    {
        mutexId = gEvtGlobalECHMutex;
        pEvtChannelInfo = gpEvtGlobalECHDb;
    }
    else
    {
        mutexId = gEvtLocalECHMutex;
        pEvtChannelInfo = gpEvtLocalECHDb;
    }

    clOsalMutexLock(mutexId);
    rc = clCntWalk(pEvtChannelInfo->evtChannelContainer, cliEvtECHInfoWalk,
                   (ClCntArgHandleT) pDisplayInfo,
                   sizeof(ClEvtTestDisplayInfoT));
    if (CL_OK != rc && CL_EVENT_ERR_INFO_WALK_STOP != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);
        clOsalMutexUnlock(mutexId);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Failed to walk channel info [0x%X]", rc));
        CL_FUNC_EXIT();
        return CL_EVENT_ERR_INTERNAL;
    }
    clOsalMutexUnlock(mutexId);

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT cliEvtShow(int argc, char **argv, char **retStr)
{
    ClRcT rc = 0xffffffff;

    ClEvtTestDisplayInfoT displayInfo = { 0 };

    if (argc != ONE_ARGUMENT && argc != FOUR_ARGUMENT)
    {
        clEvtCliStrPrint
            ("Usage: display [<ChannelName> <ChannelScope> <Flag>]\n"
             "\tChannelName [STRING] - Channel whose details are being sought [-a for all]\n"
             "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
             "\tFlag [DEC] - Subscriber Info[0: Brief] [1: Detailed]\n",
             retStr);

        return CL_OK;
    }

    if (1 == argc)
    {
        /*
         * Default Setting 
         */
        strcpy(displayInfo.channelName.value, "-a");
        displayInfo.disDetailFlag = 1;
        displayInfo.channelScope = 1;
    }
    else
    {
        sscanf(argv[1], "%s", (char *) displayInfo.channelName.value);
        sscanf(argv[2], "%hx", &displayInfo.channelScope);
        sscanf(argv[3], "%hx", &displayInfo.disDetailFlag);

        if (displayInfo.channelScope > 1 || displayInfo.disDetailFlag > 1)
        {
            clEvtCliStrPrint("Invalid Parameter Passed\n\n"
                             "Usage: display [<ChannelName> <ChannelScope> <Flag>]\n"
                             "\tChannelName [STRING] - Channel whose details are being sought [-a for all]\n"
                             "\tChannelScope [DEC] - Scope of the Channel [0:Local] [1:Global]\n"
                             "\tFlag [DEC] - Subscriber Info[0: Brief] [1: Detailed]\n",
                             retStr);

            return CL_OK;
        }
    }

    displayInfo.channelName.length = strlen(displayInfo.channelName.value);

    /*
     ** Initialize the debug handle to print the information
     ** on the CLI server instead of CPM server.
     */
    rc = clDebugPrintInitialize(&displayInfo.dbgPrintHandle);

    rc = cliEvtSubsInfoShowLocal(&displayInfo);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Display Failed, rc = [0x%x]\n", rc));

        gEventCliStr[0] = '\0';
        sprintf(gEventCliStr, "Display Failed, rc = [0x%X]\n", rc);
        clOsalPrintf("Display Failed, rc = [0x%X]\n", rc);
        clEvtCliStrPrint(gEventCliStr, retStr);

        return CL_OK;
    }

    rc = clDebugPrintFinalize(&displayInfo.dbgPrintHandle, retStr);

    return CL_OK;
}
