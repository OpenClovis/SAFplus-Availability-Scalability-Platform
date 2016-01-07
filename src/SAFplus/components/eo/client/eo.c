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
 * ModuleName  : eo                                                            
 * File        : eo.c
 *******************************************************************************/

/*******************************************************************************
 * File        : eo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *      This module contains Execution Object (EO) implementation.
 ****************************************************************************/

/*
 * INCLUDES 
 */

#undef __SERVER__
#define __CLIENT__

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBufferApi.h>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clEoErrors.h>
#include <clEoIpi.h>
#include <clEoLibs.h>
#include <clEoEvent.h>
#include <clJobQueue.h>
#include <clEoQueueStats.h>
#include <clRmdIpi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIocApiExt.h>
#include <clTransport.h>
#include <clRadixTree.h>

#include <clCpmApi.h>
#include <clCpmClient.h>
#include <clAmsMgmtCommon.h>
#include <clAmsTriggerRmd.h>
#include <clCpmServerFuncTable.h>
#include <clAmsMgmtServerFuncTable.h>
#include <clAmsEntityTriggerFuncTable.h>
#undef __CLIENT__

#define __EO_CLIENT_TABLE_INDEX(port, funId) ( ((port) << CL_EO_CLIENT_BIT_SHIFT) | (funId) )

#define __EO_CLIENT_FILTER_INDEX(port, clientID) (((port) << 16) | ( (clientID) & 0xffff ) )

#define EO_BATCH_QUEUE_SIZE 1

#define EO_ACTION_STATIC_QUEUE_INFO_SIZE (0x8)

#define EO_LOG_STATIC_QUEUE_INFO_SIZE  (0x10)

#define EO_MAX_STATIC_QUEUE_INFO_SIZE  (EO_LOG_STATIC_QUEUE_INFO_SIZE)

/*
 * A power of 2 - 1.
 */
#define CL_EO_MEM_TRIES  (0x3)

/*
 * Timeout in millisecs.
 */
#define CL_EO_MEM_TIMEOUT (500)

typedef enum ClEoStaticQueueElementType {
    CL_EO_ELEMENT_TYPE_WM,
    CL_EO_ELEMENT_TYPE_LOG,
} ClEoStaticQueueElementTypeT;

typedef struct ClEoStaticQueueInfo
{
    ClEoLibIdT libId; 
    ClEoStaticQueueElementTypeT elementType;

    union clEoStaticQueueData {

        struct ClEoWaterMarkInfoT {
            ClWaterMarkIdT wmId; 
            ClWaterMarkT wmValues; 
            ClEoWaterMarkFlagT wmType; 
            ClEoActionArgListT actionArgList; 
        } clEoWaterMarkInfo;

        struct ClEoLogInfoT {
            ClLogSeverityT logSeverity;
            ClCharT logMsg[CL_EOLIB_MAX_LOG_MSG];
        } clEoLogInfo;

    } clEoStaticQueueData ;

#define WMId clEoStaticQueueData.clEoWaterMarkInfo.wmId
#define WMValues clEoStaticQueueData.clEoWaterMarkInfo.wmValues
#define WMType  clEoStaticQueueData.clEoWaterMarkInfo.wmType
#define WMActionArgList clEoStaticQueueData.clEoWaterMarkInfo.actionArgList
#define LOGMsg   clEoStaticQueueData.clEoLogInfo.logMsg
#define LOGSeverity clEoStaticQueueData.clEoLogInfo.logSeverity

} ClEoStaticQueueInfoT;

typedef struct ClEoStaticJobQueue {
    ClUint32T jobCount;
    ClRcT (*pJobQueueAction)(ClEoExecutionObjT *,ClEoStaticQueueInfoT*,
            ClUint32T);
    ClUint32T jobQueueSize;
    ClEoStaticQueueInfoT *pJobQueue;
} ClEoStaticJobQueueT;

typedef struct ClEoStaticQueue
{
#define EO_STATIC_QUEUES (0x2)
#define EO_ACTION_QUEUE_INDEX (0x0)
#define EO_LOG_QUEUE_INDEX (0x1)
    ClBoolT initialized;
    ClOsalCondT jobQueueCond;
    ClOsalMutexT jobQueueLock;
    ClUint32T jobCount;
    ClEoStaticJobQueueT eoStaticJobQueue[EO_STATIC_QUEUES];
} ClEoStaticQueueT;

#define EO_QUEUE_SIZE(queue)  (sizeof( (queue) ) /sizeof( (queue)[0] ))

static ClRcT clEoJobQueueWMAction(ClEoExecutionObjT *,
        ClEoStaticQueueInfoT *,
        ClUint32T);
static ClRcT clEoJobQueueLogAction(ClEoExecutionObjT *,
        ClEoStaticQueueInfoT *,
        ClUint32T);

/*The queue infos for various actions*/
static ClEoStaticQueueInfoT gEoActionStaticQueueInfo[EO_ACTION_STATIC_QUEUE_INFO_SIZE];
static ClEoStaticQueueInfoT gEoLogStaticQueueInfo[EO_LOG_STATIC_QUEUE_INFO_SIZE];

/*Fill up the static queue entries here*/
static ClEoStaticQueueT gEoStaticQueue = {
    .initialized = CL_FALSE,
    .jobCount = 0,
    .eoStaticJobQueue = {
        {
            .jobCount = 0,
            .pJobQueueAction = clEoJobQueueWMAction,
            .jobQueueSize = EO_QUEUE_SIZE(gEoActionStaticQueueInfo),
            .pJobQueue = gEoActionStaticQueueInfo,
        },
        {
            .jobCount = 0,
            .pJobQueueAction = clEoJobQueueLogAction,
            .jobQueueSize = EO_QUEUE_SIZE(gEoLogStaticQueueInfo),
            .pJobQueue = gEoLogStaticQueueInfo,
        },
    },
};

#define gEoActionStaticQueue gEoStaticQueue.eoStaticJobQueue[EO_ACTION_QUEUE_INDEX]
#define gEoLogStaticQueue gEoStaticQueue.eoStaticJobQueue[EO_LOG_QUEUE_INDEX]

#define CL_EO_RECV_THREAD_INDEX (1)
/* Log area and context codes are always 3 letter (left fill with _ OK) */
#define CL_LOG_EO_AREA             "_EO"
#define CL_LOG_EO_CONTEXT_STATIC   "STA"
#define CL_LOG_EO_CONTEXT_WORKER   "WRK"
#define CL_LOG_EO_CONTEXT_PRIORITY "PRI"
#define CL_LOG_EO_CONTEXT_RECV     "RCV"
#define CL_LOG_EO_CONTEXT_CREATE   "INI"

static ClOsalTaskIdT *gClEoTaskIds;
static ClInt32T gClEoThreadCount;
static ClInt32T gClEoReceiverSpawned;
static ClBoolT gClEoStaticQueueInitialized;
extern ClIocNodeAddressT gIocLocalBladeAddress;
/*
 * To debug the creation and deletion of threads.
 */
#ifdef CL_EO_DEBUG
# include <pthread.h>
# define PTHREAD_DEBUG(STR) CL_DEBUG_PRINT(CL_DEBUG_ERROR,((STR), (int)pthread_self()))
#else
# define PTHREAD_DEBUG(STR)
#endif

/*
 * These functions have been moved into clCpmInternal.h 
 */
extern ClRcT clCpmExecutionObjectRegister(CL_IN ClEoExecutionObjT *pThis);
extern ClRcT clCpmExecutionObjectStateUpdate(CL_IN ClEoExecutionObjT *pEOptr);

/*
 * XDR header files 
 */
#include "xdrClEoSchedFeedBackT.h"
#include "xdrClEoStateT.h"

#ifdef MORE_CODE_COVERAGE
# include <clCodeCovStub.h>
#endif

/*
 * MACROS 
 */
#define EO_CHECK(X, Z, retCode)\
    do \
{\
    if(retCode != CL_OK)\
    {\
        CL_DEBUG_PRINT(X,Z);\
        goto failure;\
    }\
}while(0)

#define EO_BUCKET_SZ 64

extern ClUint32T clEoWithOutCpm;

ClBoolT clEoLibInitialized;

#define CL_NUM_JOB_QUEUES CL_IOC_MAX_PRIORITIES

int i;
ClJobQueueT gEoJobQueues[CL_NUM_JOB_QUEUES];
static ClJobQueueT gClEoReplyJobQueue;
extern ClRcT clEoClientCallbackDbInitialize(void);
extern ClRcT clEoClientCallbackDbFinalize(void);

static ClRcT (*gpClEoSerialize)(ClPtrT pData);
static ClRcT (*gpClEoDeSerialize)(ClPtrT pData);

/*****************************************************************************/
/************************** EO CLient Function implementation ****************/
static ClRcT clEoSvcPrioritySet(ClUint32T data,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);

static ClRcT clEoGetState(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);

static ClRcT clEoSetState(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);

static ClRcT clEoShowRmdStats(ClUint32T data,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);

static ClRcT clEoIsAlive(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);
/*****************************************************************************/

static ClUint32T eoGlobalHashFunction(ClCntKeyHandleT key);
static ClInt32T eoGlobalHashKeyCmp(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void eoGlobalHashDeleteCallback(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData);
static ClRcT eoAddEntryInGlobalTable(ClEoExecutionObjT *eoObj,
        ClIocPortT eoPort);
static ClRcT eoDeleteEntryFromGlobalTable(ClIocPortT eoPort);

static ClRcT clEoStart(ClEoExecutionObjT *pThis);


/*
 * Only RMD will use it 
 */
ClRcT clEoRemoteObjectUnblock(ClEoExecutionObjT *remoteEoObj);
ClRcT clEoGetRemoteObjectAndBlock(ClUint32T remoteObj,
        ClEoExecutionObjT **pRemoteEoObj);

static ClRcT clEoDropPkt(ClEoExecutionObjT *pThis,
        ClBufferHandleT eoRecvMsg, ClUint8T priority,
        ClUint8T protoType, ClUint32T length,
        ClIocPhysicalAddressT srcAddr);

static ClRcT clEoLogLevelSet(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);
static ClRcT clEoLogLevelGet(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);
static ClRcT clEoCustomActionIntimation(ClUint32T data, ClBufferHandleT inMsgHandle,
                                        ClBufferHandleT outMsgHandle);

static ClRcT clEoPriorityQueuesInitialize(void);
ClRcT clEoPriorityQueuesFinalize(ClBoolT force);

#define CL_EO_CUSTOM_ACTION_INTIMATION_FN_ID CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 7)

static CL_LIST_HEAD_DECLARE(gClEoCustomActionList);

typedef struct ClEoCustomActionInfo
{
    ClListHeadT list;
    void (*callback)(ClUint32T type, ClUint8T *data, ClUint32T len);
} ClEoCustomActionInfoT;


/*
 * Default Service function table
 */
static ClEoPayloadWithReplyCallbackT defaultServiceFuncList[] = {
    /*
     * Fix functions 
     */
    (ClEoPayloadWithReplyCallbackT) clEoGetState,   /* 0 */
    (ClEoPayloadWithReplyCallbackT) clEoSetState,   /* 1 */
    (ClEoPayloadWithReplyCallbackT) clEoIsAlive,    /* 2 */
    (ClEoPayloadWithReplyCallbackT) clEoShowRmdStats,   /* 3 */
    /*
     * Default svc APIs 
     */
    (ClEoPayloadWithReplyCallbackT) clEoSvcPrioritySet, /* 4 */
    (ClEoPayloadWithReplyCallbackT) clEoLogLevelSet,    /* 5 */
    (ClEoPayloadWithReplyCallbackT) clEoLogLevelGet, /* 6 */
    (ClEoPayloadWithReplyCallbackT) clEoCustomActionIntimation, /* 7 */
};


/*
 * GLOBALS 
 */
/*
 * EO supported protocols related defines 
 */

ClEoProtoDefT gClEoProtoList[CL_IOC_NUM_PROTOS];

/*
 * LOCALS 
 */

/*
 * Following global variables server as the place holders
 * for the Execution Object and it's IOC Port. The Get 
 * API simply fetch these values.
 */
static ClEoExecutionObjT *gpExecutionObject;
ClIocPortT gEOIocPort;
static ClRadixTreeHandleT gAspClientIDTable;

/*
 * Global hash table which will have all the EO eoPort to EOObj mapping
 */
static ClCntHandleT gEOObjHashTable;
static ClOsalMutexT gEOObjHashTableLock;
static ClOsalMutexT gClEoJobMutex;

/*
 * FORWARD DECLARATION 
 */
/*
 * static ClRcT eoReceiveLoop(ClEoExecutionObjT* pThis);
 */

static ClRcT eoInit(ClEoExecutionObjT *pThis);

typedef void *(*ClEoTaskFuncPtrT) (void *);

static ClUint32T clEoDataCompare(ClCntKeyHandleT inputData,
        ClCntKeyHandleT storedData);

typedef struct
{
    ClOsalCondIdT jobQueueCond;
    ClOsalMutexIdT jobQueueLock;
    ClQueueT jobQueue;
} ClEoJobQueueT;

static ClEoProtoDefT protos[] =
{ 
    {CL_IOC_RMD_SYNC_REQUEST_PROTO,  "RMD request",       clRmdReceiveRequest,                NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ASYNC_REQUEST_PROTO, "RMD async request", clRmdReceiveAsyncRequest,           NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ASYNC_REPLY_PROTO,   "RMD async reply",   clRmdReceiveAsyncReply,             NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_SYSLOG_PROTO,            "syslog",            clEoDropPkt,                        NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ACK_PROTO,           "RMD ACK",           clRmdAckReply,                      NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },

    {CL_IOC_PORT_NOTIFICATION_PROTO, "port notification", clEoProcessIocRecvPortNotification, NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_SYNC_REPLY_PROTO,    "RMD sync reply",    clEoDropPkt,                        clRmdReceiveReply, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE},
    {CL_IOC_RMD_ORDERED_PROTO,       "RMD ordered request", clRmdReceiveOrderedRequest,       NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },

    /*
     * {CL_IOC_EM_PROTO,"EM IOC MSG", emIocMsg}
     */
    {CL_IOC_INVALID_PROTO, "", 0, 0}
};

static ClRcT clEoIocRecvQueueProcess(ClEoExecutionObjT *pThis);
void clEoCleanup(ClEoExecutionObjT *pThis);


/**************   API to create EO Threads   ***************************/
static ClRcT clEoTaskCreate(ClCharT *pTaskName, ClEoTaskFuncPtrT pTaskFunc, ClPtrT pTaskArg, ClOsalTaskIdT *pTaskId)
{
    ClRcT rc = CL_OK;

    if(!pTaskId)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "Taskid null");
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
            "Creating thread for task [%s]", pTaskName);
    rc = clOsalTaskCreateAttached(pTaskName, CL_OSAL_SCHED_OTHER, 
                                  CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                                  pTaskFunc, (void *) pTaskArg, pTaskId);
    if (CL_OK != rc)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                "Thread creation failed for task [%s] with error [0x%x]",
                pTaskName, rc);
        return rc;
    }
    return rc;
}

ClRcT clEoThreadSafeInstall(ClRcT (*pClEoSerialize)(ClPtrT pData),
        ClRcT (*pClEoDeSerialize)(ClPtrT pData))
{
    if(pClEoSerialize && pClEoDeSerialize)
    {
        gpClEoSerialize = pClEoSerialize;
        gpClEoDeSerialize = pClEoDeSerialize;
        return CL_OK;
    }
    return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT clEoThreadSafeDisable(ClUint8T protoid)
{
    gClEoProtoList[protoid].flags &= ~CL_EO_STATE_THREAD_SAFE;
    return CL_OK;
}

void clEoProtoUninstall(ClUint8T protoid)
{
    gClEoProtoList[protoid].protoID = CL_IOC_INVALID_PROTO;
    snprintf((char*) gClEoProtoList[protoid].name, CL_MAX_NAME_LENGTH, "protocol %d undefined", protoid);
    gClEoProtoList[protoid].func    = clEoDropPkt;
    gClEoProtoList[protoid].flags   = 0;
}

void clEoProtoInstall(ClEoProtoDefT* def)
{
    /* Do not install protocols twice.  Use switch */
    if (gClEoProtoList[def->protoID].protoID != CL_IOC_INVALID_PROTO)
    {
        clDbgCodeError(CL_OK, ("Protocol %d already has a handler. Do not install protocols twice.  Use clEoProtoSwitch to change.",def->protoID)); 
    }

    clEoProtoSwitch(def);
}

void clEoProtoSwitch(ClEoProtoDefT* def)
{
    if (def->protoID == CL_IOC_INVALID_PROTO)
    {
        clDbgCodeError(CL_OK, ("Protocol %d is invalid.  Do not use it.",def->protoID)); 
    }

    gClEoProtoList[def->protoID] = *def;  
}



void eoProtoInit(void)
{
    ClInt32T i;

    for (i=0;i<CL_IOC_NUM_PROTOS;i++)
    {
        clEoProtoUninstall(i);
    }

    i=0;
    for(i=0; (protos[i].protoID != CL_IOC_INVALID_PROTO); i++)
    {
        clEoProtoInstall(&protos[i]);
    }
}

static ClEoCrashNotificationCallbackT gClEoCrashNotificationCallback;

void clEoSendCrashNotification(ClEoCrashNotificationT *crash)
{
    if(gClEoCrashNotificationCallback)
    {
        crash->pid = getpid();
        crash->compName = ASP_COMPNAME;
        gClEoCrashNotificationCallback((const ClEoCrashNotificationT*)crash);
    }
}

ClRcT clEoCrashNotificationRegister(ClEoCrashNotificationCallbackT callback)
{
    if(!callback) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    if(gClEoCrashNotificationCallback) return CL_EO_RC(CL_ERR_ALREADY_EXIST);
    gClEoCrashNotificationCallback = callback;
    return CL_OK;
}

/**************   Action Related Functionality      ***************************/
static ClRcT eoStaticQueueInit(void)
{
    ClRcT rc = CL_OK;
    
    rc = clOsalMutexInit(&(gEoStaticQueue.jobQueueLock));
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexInit Failed 0x%x\n", rc));
        return rc;
    }

    rc = clOsalCondInit(&(gEoStaticQueue.jobQueueCond));
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Message Write Failed 0x%x\n", rc));
        clOsalMutexDestroy(&gEoStaticQueue.jobQueueLock);
        return rc;
    }

    gClEoStaticQueueInitialized = CL_TRUE;
    return CL_OK;
}

static ClRcT eoStaticQueueExit(void)
{
    gClEoStaticQueueInitialized = CL_FALSE;
    return CL_OK;
}

static __inline__ ClRcT clEoQueueStaticQueueInfo(ClEoStaticQueueInfoT *pStaticQueueInfo,ClEoStaticJobQueueT *pJobQueue)
{
    ClRcT rc = CL_EO_RC(CL_EO_ERR_QUEUE_OVERFLOW);
    if(gClEoStaticQueueInitialized == CL_FALSE) return CL_OK;
    clOsalMutexLock(&gEoStaticQueue.jobQueueLock);
    if(pJobQueue->jobCount < pJobQueue->jobQueueSize)
    {
        ClEoStaticQueueInfoT *pJob;
        pJob = &pJobQueue->pJobQueue[pJobQueue->jobCount];
        memcpy(pJob,pStaticQueueInfo,sizeof(*pJob));
        ++pJobQueue->jobCount; 
        /*
         * Signal the Action thread to process the request.
         */
        clOsalCondSignal(&gEoStaticQueue.jobQueueCond);
        /*increment the global count*/
        ++gEoStaticQueue.jobCount;
        rc = CL_OK;
    }
    clOsalMutexUnlock(&gEoStaticQueue.jobQueueLock);
    return rc;
}

/*
 * Queue the watermark info to the static queue
 */

ClRcT clEoQueueWaterMarkInfo(ClEoLibIdT libId, ClWaterMarkIdT wmId, ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList)
{
    ClRcT rc = CL_EO_RC(CL_ERR_NOT_INITIALIZED);
    ClEoStaticQueueInfoT eoStaticQueueInfo = {0};

    eoStaticQueueInfo.libId = libId;
    eoStaticQueueInfo.elementType = CL_EO_ELEMENT_TYPE_WM;
    eoStaticQueueInfo.WMId = wmId;
    eoStaticQueueInfo.WMValues = *pWaterMark;
    eoStaticQueueInfo.WMType = wmType;
    eoStaticQueueInfo.WMActionArgList = argList; 
    rc = clEoQueueStaticQueueInfo(&eoStaticQueueInfo,&gEoActionStaticQueue);

    return rc;
}

ClRcT clEoQueueLogInfo(ClEoLibIdT libId, ClLogSeverityT severity,const ClCharT *pMsg)
{
    ClRcT rc = CL_EO_RC(CL_ERR_NOT_INITIALIZED);
    ClEoStaticQueueInfoT eoStaticQueueInfo = {0};

    eoStaticQueueInfo.libId = libId;
    eoStaticQueueInfo.elementType = CL_EO_ELEMENT_TYPE_LOG;
    eoStaticQueueInfo.LOGSeverity = severity;
    memcpy(eoStaticQueueInfo.LOGMsg,pMsg,sizeof(eoStaticQueueInfo.LOGMsg));
    rc = clEoQueueStaticQueueInfo(&eoStaticQueueInfo,&gEoLogStaticQueue);

    return rc;
}

/*We are running lockless w.r.t gEoStaticQueue in this part*/

static ClRcT clEoJobQueueWMAction(ClEoExecutionObjT *pThis,
        ClEoStaticQueueInfoT *pStaticJobQueue,
        ClUint32T jobCount)
{
    /*Process the jobs of this queue*/
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < jobCount && pThis->threadRunning == CL_TRUE; ++i)
    {
        ClEoStaticQueueInfoT *pJob = pStaticJobQueue+i;
        ClUint64T wmValue = 0;

        CL_ASSERT(pJob->elementType == CL_EO_ELEMENT_TYPE_WM);

        wmValue = (pJob->WMType == CL_WM_HIGH_LIMIT)? 
            pJob->WMValues.highLimit : pJob->WMValues.lowLimit;

        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("EO[%s]:LIB[%s]:The Action being triggered for "
                    "Water Mark[%d]->[%s] with the value [%lld]\n", 
                    clEoConfig.EOname, LIB_NAME(pJob->libId), pJob->WMId, pJob->WMType? "HIGH_LIMIT" : "LOW_LIMIT", 
                    wmValue));

        /*
         * Fix for bug 5554.
         */
        clLogCritical("EO", "WMH",  /* Water Mark Hit */
                "EO[%s]:LIB[%s]:The Water Mark[%d]->[%s] has been hit"
                "with the value [%lld]", 
                clEoConfig.EOname, LIB_NAME(pJob->libId), pJob->WMId, 
                (pJob->WMType? "HIGH_LIMIT" : "LOW_LIMIT"), wmValue);

        if(CL_TRUE == isCustomSet(pJob->libId, pJob->WMId))
        {
            if(NULL != clEoConfig.clEoCustomAction)
            {
                rc = clEoConfig.clEoCustomAction(pJob->libId, pJob->WMId, 
                        &pJob->WMValues, pJob->WMType, pJob->WMActionArgList);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("EO: Custom Action for Water Mark Hit Failed, rc=0x%x\n", rc));
                }
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                        ("EO: Custom Action for Water Mark Hit not specified.\n"));
            }
        }

        if(CL_TRUE == isEventSet(pJob->libId, pJob->WMId))
        {
            rc = clEoTriggerEvent(pJob->libId, pJob->WMId, wmValue, pJob->WMType);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("EO: Water Mark Event Publish Failed, rc=0x%x\n", rc));
            }
        }

        if(CL_TRUE == isNotificationSet(pJob->libId, pJob->WMId))
        {
            /*       eoTriggerNotification();*/
        }
    }
    return rc;
}

static ClRcT clEoJobQueueLogAction(ClEoExecutionObjT *pThis,
        ClEoStaticQueueInfoT *pStaticJobQueue,
        ClUint32T jobCount)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    /*We are lockless in this path*/
    for(i = 0; i < jobCount && pThis->threadRunning == CL_TRUE; ++i)
    {
        ClEoStaticQueueInfoT *pJob = pStaticJobQueue + i;
        CL_ASSERT(pJob->elementType == CL_EO_ELEMENT_TYPE_LOG);
        clLogWrite(CL_LOG_HANDLE_APP,
                pJob->LOGSeverity, 
                LIB_NAME(pJob->libId), 
                pJob->LOGMsg);
    }
    return rc;
}

static ClRcT eoStaticQueueProcess(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;

    ClUint32T i = 0;
    ClUint32T queueSize = 0;
    ClTimerTimeOutT timer = {0, 0};
    ClOsalTaskIdT selfTaskId = 0;
    /*
     * Following Batch queue (static) reduces the lock latency
     */
    ClEoStaticQueueInfoT batchQueue[EO_MAX_STATIC_QUEUE_INFO_SIZE];

    clOsalSelfTaskIdGet(&selfTaskId);

    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_STATIC,"The static queue thread is running with id [%#llx]", selfTaskId);

    clOsalMutexLock(&gEoStaticQueue.jobQueueLock);


    gEoStaticQueue.initialized = CL_TRUE;

    while (pThis->threadRunning == CL_TRUE)
    {
        /*Get the total count*/

        queueSize = gEoStaticQueue.jobCount;
        /*
         * Wait indefinitely for the signal, if no jobs in the queue,
         * i.e, queueSize is 0.
         */
        if (queueSize == 0 &&
                ((rc =
                  clOsalCondWait(&gEoStaticQueue.jobQueueCond,
                      &gEoStaticQueue.jobQueueLock, timer)) == CL_OK))
        {
            /*
             * Once get into this condition, drain out all the pending jobs 
             */
            queueSize = gEoStaticQueue.jobCount;
        }
        else if (rc != CL_OK || queueSize == 0)
        {
            /*spurious wakeup*/
            continue;
        }

        if(pThis->threadRunning == CL_FALSE)
        {
            break;
        }
        /*
         * We can reset the global count too as individuals
         * would be reset anyway
         */
        gEoStaticQueue.jobCount = 0;

        for(i = 0; i < EO_STATIC_QUEUES;++i)
        {
            ClEoStaticJobQueueT *pStaticJobQueue = 
                gEoStaticQueue.eoStaticJobQueue + i;
            ClUint32T staticJobQueueCount = pStaticJobQueue->jobCount;

            if(staticJobQueueCount == 0)
            {
                continue;
            }
            /*
               We are running with lock held.So copy this guy,
               drop the lock and process the jobs
               */
            memcpy(batchQueue,pStaticJobQueue->pJobQueue,
                    sizeof(ClEoStaticQueueInfoT) * staticJobQueueCount);

            pStaticJobQueue->jobCount = 0;

            /*Drop the lock*/
            clOsalMutexUnlock(&gEoStaticQueue.jobQueueLock);

            /* Okay, we can assert for this if need be.
             * Else assume that action is present.Dont care as of now.
             */
            pStaticJobQueue->pJobQueueAction(pThis,batchQueue,staticJobQueueCount);

            if(pThis->threadRunning == CL_FALSE)
            {
                /*someone killed us behind our back.Back out*/
                goto done;
            }
            /*We hold the lock again */
            clOsalMutexLock(&gEoStaticQueue.jobQueueLock);
        }
        /*We are here with lock held*/
    }
    /*We are here with static queue lock held*/
    gEoStaticQueue.initialized = CL_FALSE;
    clOsalMutexUnlock(&gEoStaticQueue.jobQueueLock);

done:

    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_STATIC,"The static queue thread [%#llx] is exiting", selfTaskId);

    return CL_OK;
}
/******************************************************************************/
static ClHandleT gClEoNotificationHandle;
static void eoNotificationCallback(ClIocNotificationIdT eventId,
                                   ClPtrT unused,
                                   ClIocAddressT *pAddress)
{
    ClIocNotificationT notification = {0};
    notification.id = htonl(eventId);
    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(pAddress->iocPhyAddress.nodeAddress);
    notification.nodeAddress.iocPhyAddress.portId = htonl(pAddress->iocPhyAddress.portId);
    /*
     * Invoke all the registrants
     */
    clLogDebug("IOC", "NOTIF", "Invoking notification registrants for id [%d],"
               "node [%d], port [%d]", eventId,
               pAddress->iocPhyAddress.nodeAddress, pAddress->iocPhyAddress.portId);
    clIocNotificationRegistrants(&notification);
}

/**
 *  NAME: clEoCreateSystemCallout
 *
 *  This function is for any ASP related processing that needs to be done after
 *  EOCreate
 *
 *  @param    pThis   : handle to EO
 *
 *  @returns CL_OK    - Success<br>
 *           CL_ERR_NULL_POINTER - Input parameter improper(NULL)
 */
static ClRcT clEoCreateSystemCallout(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;

    if (pThis == NULL)
        EO_CHECK(CL_DEBUG_ERROR, ("Improper reference to EO Object \n"),
                CL_EO_RC(CL_ERR_NULL_POINTER));

    /*
     * Register for node notification for all EO's other than amf or node representative
     * as the node rep anyway is directly accessible to notifications through the 
     * fast listen interface
     */
    if(!gIsNodeRepresentative)
    {
        ClIocPhysicalAddressT compAddr = {.nodeAddress = CL_IOC_BROADCAST_ADDRESS,
                                          .portId = CL_IOC_CPM_PORT
        };
        clCpmNotificationCallbackInstall(compAddr, eoNotificationCallback, 
                                         NULL, &gClEoNotificationHandle);
    }
    /*
     * Registration with component manager 
     */
    if (pThis->eoPort != CL_IOC_CPM_PORT && clEoWithOutCpm != CL_TRUE)
    {
        rc = clCpmExecutionObjectRegister(pThis);
        EO_CHECK(CL_DEBUG_ERROR, ("clCpmExecutionObjectRegister Failed\n"), rc);
    }

    return CL_OK;

failure:
    return (rc);
}

/**
 *  NAME: clEoStateChangeSystemCallout
 *
 *  This function is for any ASP related processing that needs to be done as
 *  the state of EO changes
 *
 *  @param    pThis   : handle to EO
 *
 *  @returns CL_OK    - Success<br>
 *           CL_ERR_NULL_POINTER - Input parameter improper(NULL)
 */
static ClRcT clEoStateChangeSystemCallout(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;

    if (pThis == NULL)
        EO_CHECK(CL_DEBUG_ERROR, ("Improper reference to EO Object \n"),
                CL_EO_RC(CL_ERR_NULL_POINTER));

    /*
     * Sync up the component mgr 
     */
    if (pThis->eoPort != CL_IOC_CPM_PORT && clEoWithOutCpm != CL_TRUE)
    {
        rc = clCpmExecutionObjectStateUpdate(pThis);
        EO_CHECK(CL_DEBUG_ERROR, ("clCpmExecutionObjectStateUpdate Failed\n"),
                rc);
    }

    return CL_OK;

failure:
    return (rc);
}

ClBoolT clEoClientTableFilter(ClIocPortT eoPort, ClUint32T clientID)
{
    ClPtrT item = NULL;
    ClUint32T index = __EO_CLIENT_FILTER_INDEX(eoPort, clientID);
    clRadixTreeLookup(gAspClientIDTable, index, &item);
    return item ? CL_TRUE : CL_FALSE;
}

static ClRcT eoClientIDRegister(ClIocPortT port, ClUint32T clientID)
{
    ClRcT rc = CL_OK;
    ClUint32T *lastItem = NULL;
    ClUint32T *item = clHeapCalloc(1, sizeof(*item));
    ClUint32T index = __EO_CLIENT_FILTER_INDEX(port, clientID);

    CL_ASSERT(item != NULL);
    *item = clientID;
    rc = clRadixTreeInsert(gAspClientIDTable, index, (ClPtrT)item, (ClPtrT*)&lastItem);
    if(rc != CL_OK)
    {
        clLogError("EO", "ID-REGISTER", "Radix tree insert for port [%d] returned [%#x]",
                   port, rc);
        goto out_free;
    }
    if(lastItem) clHeapFree(lastItem);
    return rc;

    out_free:
    clHeapFree(item);
    return rc;
}
                                   
static ClRcT eoClientTableRegister(ClEoExecutionObjT *pThis, 
                                   ClEoPayloadWithReplyCallbackTableClientT *clientTable, 
                                   ClIocPortT eoPort)
{
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!clientTable) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    for (i = 0; clientTable[i].funTable; ++i)
    {
        ClUint32T clientID = clientTable[i].clientID;
        ClEoPayloadWithReplyCallbackClientT *funTable = clientTable[i].funTable;
        ClUint32T funTableSize = clientTable[i].funTableSize;
        ClEoClientTableT *client = NULL;
        ClInt32T j;

        if (clientID > pThis->maxNoClients) continue;

        eoClientIDRegister(eoPort, clientID);

        client = pThis->pClientTable + clientID;

        for (j = 0; j < funTableSize; ++j)
        {
            ClUint32T funcNo = funTable[j].funId & CL_EO_FN_MASK;
            ClUint32T index = __EO_CLIENT_TABLE_INDEX(eoPort, funcNo);
            ClUint32T *lastVersion = NULL;
            if(!funcNo || !funTable[j].version) continue;
            rc = clRadixTreeInsert(client->funTable, index, 
                                   (ClPtrT)&funTable[j].version, (ClPtrT*)&lastVersion);
            if (rc != CL_OK)
            {
                clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                           "radix tree insert error [%#x] for fun id [%d], version [%#x]",
                           rc, funTable[j].funId, funTable[j].version);
            }
        }
    }
    
    return rc;
}

ClRcT clEoClientTableRegister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                              ClIocPortT clientPort)
{
    ClEoExecutionObjT *pThis = NULL;
    ClRcT rc = CL_OK;

    rc = clEoMyEoObjectGet(&pThis);
    CL_ASSERT(pThis != NULL);

    clOsalMutexLock(&pThis->eoMutex);
    rc = eoClientTableRegister(pThis, clientTable, clientPort);
    clOsalMutexUnlock(&pThis->eoMutex);

    return rc;
}

ClRcT eoClientTableAlloc(ClUint32T maxClients, ClEoClientTableT **ppClientTable)
{
    register int i;
    ClEoClientTableT *clients = NULL;

    ClRcT rc;
    
    rc = clRadixTreeInit(&gAspClientIDTable);
    CL_ASSERT(rc == CL_OK);

    clients = clHeapCalloc(maxClients, sizeof(*clients));
    CL_ASSERT(clients != NULL);

    clients->maxClients = maxClients;
    for (i = 0; i < maxClients; ++i)
    {
        ClEoClientTableT *client = clients + i;
        rc = clRadixTreeInit(&client->funTable);
        CL_ASSERT(rc == CL_OK);
    }
    
    *ppClientTable = clients;

    return CL_OK;
}

/*
 * Register AMF table.
 */
ClRcT eoClientTableInitialize(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;
    /*
     * Register the AMF RMD tables with the client by default for all of them.
     */
    rc = eoClientTableRegister(pThis, CL_EO_CLIENT_SYM_MOD(gAspFuncTable, AMF), CL_IOC_CPM_PORT);
    rc |= eoClientTableRegister(pThis, CL_EO_CLIENT_SYM_MOD(gAspFuncTable, AMFMgmtServer),
                                CL_IOC_CPM_PORT);
    rc |= eoClientTableRegister(pThis, CL_EO_CLIENT_SYM_MOD(gAspFuncTable, AMFTrigger), CL_IOC_CPM_PORT);
    return rc;
}

ClRcT eoServerTableAlloc(ClUint32T maxClients, ClEoServerTableT **ppServerTable)
{
    ClRcT rc;
    
    register int i;
    ClEoServerTableT *clients = NULL;

    clients = clHeapCalloc(maxClients, sizeof(*clients));
    CL_ASSERT(clients != NULL);

    clients->maxClients = maxClients;
    for (i = 0; i < maxClients; ++i)
    {
        ClEoServerTableT *client = clients + i;
        rc = clRadixTreeInit(&client->funTable);
        CL_ASSERT(rc == CL_OK);
    }

    *ppServerTable = clients;

    return CL_OK;
}

ClRcT clEoClientGetFuncVersion(ClIocPortT port, ClUint32T funcId, ClVersionT *version)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;
    ClEoClientTableT *client = NULL;
    ClUint32T *latestVerCode = NULL;
    ClUint32T index = 0;

    CL_ASSERT(version != NULL);

    rc = clEoMyEoObjectGet(&pThis);
    CL_ASSERT(pThis != NULL);

    rc = clEoServiceIndexGet(funcId, &clientID, &funcNo);
    if (rc != CL_OK) goto out;

    if (!pThis->pClientTable) return CL_OK;

    if(!clEoClientTableFilter(port, clientID))
    {
        if(port == pThis->eoPort)
            return CL_OK;
        if(!clEoClientTableFilter(pThis->eoPort, clientID)) return CL_OK;
    }

    client = pThis->pClientTable + clientID;

    index = __EO_CLIENT_TABLE_INDEX(port, funcNo);

    rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&latestVerCode);
    if (rc != CL_OK || !latestVerCode)
    {
        /* Do a second lookup with own port since these could be app. specific clients with exported tables. */
        index = __EO_CLIENT_TABLE_INDEX(pThis->eoPort, funcNo);
        rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&latestVerCode);
        if(rc != CL_OK || !latestVerCode)
        {
            rc = CL_EO_RC(CL_ERR_NULL_POINTER);
            goto out;
        }
        else
        {
            clLogDebug("EO2", "DEBUG", "Radix tree lookup for Client [%d], Function [%d] succeeded", clientID, funcNo);
        }
        rc = CL_OK;
    }

    version->releaseCode = CL_VERSION_RELEASE(*latestVerCode);
    version->majorVersion = CL_VERSION_MAJOR(*latestVerCode);
    version->minorVersion = CL_VERSION_MINOR(*latestVerCode);

    clLogDebug("EO", "RPC",
               "Port [%#x], Function id [%d], Client ID [%d], Version [%d.%d.%d]",
               port, funcNo, clientID, version->releaseCode, version->majorVersion, version->minorVersion);

    return CL_OK;
    
out:
    return rc;
}

/**
 *  NAME: clEoCreate 
 * 
 *  This function creates an EO     
 *
 *  @param    pThis   : handle to EO 
 *            pConfig : EO config parameters
 *
 *  @returns CL_OK      - Success<br>
 *           CL_ERR_NULL_POINTER   - Input parameter improper(NULL)
 *           CL_ERR_NO_MEMORY - Memory allocation failed
 */

ClRcT clEoCreate(ClEoConfigT *pConfig, ClEoExecutionObjT **ppThis)
{
    ClRcT rc = CL_OK;
    ClEoClientObjT *pClient = NULL;
    ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0};
    ClInt32T tries = 0;
    CL_FUNC_ENTER();

    /*
     * Sanity check for function parameters 
     */
    if ((pConfig == NULL) || (ppThis == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: Improper input parameters \n"));
        rc = CL_EO_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE, "Creating EO for [%s]. Msg threads [%d] comm port [%d] main thread? [%d] ", pConfig->EOname,pConfig->noOfThreads,pConfig->reqIocPort,pConfig->appType);

    if (pConfig->appType == CL_EO_USE_THREAD_FOR_APP &&
        pConfig->noOfThreads == 0)
    {
        rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
        clDbgRootCauseError(rc, ("Main thread is dedicated for application, but number of worker threads is configured as 0"));
        goto failure;
    }

    *ppThis = (ClEoExecutionObjT *) clHeapCalloc(1, sizeof(ClEoExecutionObjT));
    if (*ppThis == NULL)
    {
        rc = CL_EO_RC(CL_ERR_NO_MEMORY);
        clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "Memory allocation failed, error [0x%x]", rc);
        goto failure;
    }

    strncpy((ClCharT *) (*ppThis)->name, (ClCharT *) pConfig->EOname, sizeof((*ppThis)->name)-1);

#ifdef CL_INCLUDE_NATIVE_IOC
    /*
     * Incase IOC has updated the port when reqIocPort = 0.
     */
    if (gpClEoIocConfig->iocConfigInfo.isNodeRepresentative == CL_TRUE)
    {
        pConfig->reqIocPort = gpClEoIocConfig->iocConfigInfo.iocNodeRepresentative; 
    }
#endif
    
    (*ppThis)->eoPort = pConfig->reqIocPort;

    (*ppThis)->pri = pConfig->pri;
    (*ppThis)->noOfThreads = pConfig->noOfThreads;
    (*ppThis)->state = CL_EO_STATE_INIT;
    (*ppThis)->threadRunning = CL_TRUE;
    (*ppThis)->eoInitDone = 0;
    (*ppThis)->eoSetDoneCnt = 0;
    (*ppThis)->refCnt = 0;

    (*ppThis)->maxNoClients = pConfig->maxNoClients+1;
    (*ppThis)->appType = pConfig->appType;
    (*ppThis)->clEoCreateCallout = pConfig->clEoCreateCallout;
    (*ppThis)->clEoDeleteCallout = pConfig->clEoDeleteCallout;
    (*ppThis)->clEoStateChgCallout = pConfig->clEoStateChgCallout;
    (*ppThis)->clEoHealthCheckCallout = pConfig->clEoHealthCheckCallout;

    rc = clCntLlistCreate((ClCntKeyCompareCallbackT) clEoDataCompare,
                          (ClCntDeleteCallbackT) NULL,
                          (ClCntDeleteCallbackT) NULL, CL_CNT_UNIQUE_KEY,
                          &((*ppThis)->pEOPrivDataHdl));
    if (rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clCntLlistCreate() failed, error [0x%x]", rc);
        goto eoAllocated;
    }

    rc = clOsalMutexInit(&(*ppThis)->eoMutex);
    if(rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clOsalMutexInit() failed, error [0x%x]", rc);
        goto eoPrivateDataCntAllocated;
    }

    rc = clOsalCondInit(&(*ppThis)->eoCond);
    if(rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clOsalCondInit() failed, error [0x%x]", rc);
        goto eoPrivateDataCntAllocated;
    }

    /*
     * Initialize the Action Queue
     */
    rc = eoStaticQueueInit();
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nEO: eoStaticQueueInit() failed, rc[0x%x]\n", rc));
        goto eoPrivateDataCntAllocated;
    }


    pClient =
        (ClEoClientObjT *) clHeapAllocate(sizeof(ClEoClientObjT) *
                                          ((*ppThis)->maxNoClients + 1));
    if (pClient == NULL)
    {
        rc = CL_EO_RC(CL_ERR_NO_MEMORY);
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "Memory allocation failed, error [0x%x]", rc);
        /*
         * Do necessary cleanup 
         */
        goto eoStaticQueueInitialized;
    }

    memset(pClient, 0, sizeof(ClEoClientObjT) * ((*ppThis)->maxNoClients));
    (*ppThis)->pClient = pClient;

    rc = eoClientTableAlloc((*ppThis)->maxNoClients, &(*ppThis)->pClientTable);
    CL_ASSERT(rc == CL_OK);
    rc = eoServerTableAlloc((*ppThis)->maxNoClients, &(*ppThis)->pServerTable);
    CL_ASSERT(rc == CL_OK);
    rc = eoClientTableInitialize(*ppThis);
    CL_ASSERT(rc == CL_OK);

    rc = clEoClientCallbackDbInitialize();
    CL_ASSERT(rc == CL_OK);

    do 
    {
        rc = clIocCommPortCreate((ClUint32T) pConfig->reqIocPort,
                                 CL_IOC_RELIABLE_MESSAGING, &((*ppThis)->commObj));
    } while( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST &&
             ++tries <= 5 &&
             clOsalTaskDelay(delay) == CL_OK);


    if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "This EO [%s] instance is already running on this node"
                   " exiting...", pConfig->EOname);
        /*
         * Do necessary cleanup 
         */
        goto eoClientObjCreated;
    }
    if (rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clIocCommPortCreate() failed, error [0x%x]", rc);
        /*
         * Do necessary cleanup 
         */
        goto eoClientObjCreated;
    }

    if (pConfig->reqIocPort == 0)
    {
        rc = clIocCommPortGet((*ppThis)->commObj, &((*ppThis)->eoPort));
        if (rc != CL_OK)
        {
            clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                       "Could not get IOC communication port info, error [0x%x]",
                       rc);
            goto eoIocCommPortCreated;
        }
    }

    rc = clIocPortNotification((*ppThis)->eoPort, CL_IOC_NOTIFICATION_ENABLE);
    if(rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                "Failed to enable the port notification. error [0x%x]", rc);
        goto eoIocCommPortCreated;
    }

    clLogInfo(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
              "Own IOC port is [0x%x]", (*ppThis)->eoPort);

    rc = clRmdObjInit(&((*ppThis)->rmdObj));
    if (rc != CL_OK)
    {
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clRmdObjInit() failed, error [0x%x]", rc);
        goto eoIocCommPortCreated;
    }

    /*
     * Added the following to fix Bug 3920.
     * The Get API can fetch the values from the
     * global placeholders.
     */
    gpExecutionObject = *ppThis;
    gEOIocPort = (*ppThis)->eoPort;

    rc = clEoStart(*ppThis);
    if (rc != CL_OK)
    {
        gpExecutionObject = NULL;
        gEOIocPort = 0;
        clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                   "clEoStart() failed, error [0x%x]", rc);
        goto eoRmdObjInitalized;
    }

    CL_FUNC_EXIT();
    return CL_OK;

    eoRmdObjInitalized:
    clRmdObjClose((*ppThis)->rmdObj);

    eoIocCommPortCreated:
    clIocCommPortDelete((*ppThis)->commObj);

    eoClientObjCreated:
    clHeapFree((*ppThis)->pClient);

    eoStaticQueueInitialized:
    eoStaticQueueExit();

    eoPrivateDataCntAllocated:
    clCntDelete((*ppThis)->pEOPrivDataHdl);

    eoAllocated:
    clHeapFree(*ppThis);

    failure:

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoClientInstall 
 * 
 *  This function is called by the client to  install its function table     
 *  with the EO
 *
 *  @param    pThis    : handle to EO 
 *            clientID : client identifier                              
 *            pFuncs   : pointer to API                                 
 *            data     : client specific data                           
 *            nfuncs   : no. of functions passed to be installed        
 *
 *  @returns CL_OK            - Success<br>
 *           CL_ERR_NULL_POINTER         - Input parameter improper(NULL)
 *           CL_ERR_NO_MEMORY       - Memory allocation failed
 *           CL_EO_ERR_INVALID_CLIENTID - ClientID out of range
 *           CL_EO_ERR_INVALID_SERVICEID- ServiceID out of range
 */

ClRcT clEoClientInstall(ClEoExecutionObjT *pThis, ClUint32T clientID,
        ClEoPayloadWithReplyCallbackT *pFuncs, ClEoDataT data,
        ClUint32T nfuncs)
{
    int i = 0;
    ClEoServiceObjT *pTemp = NULL;
    ClEoServiceObjT *pFunction = NULL;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: Installing the function table ...... \n"));

    /*
     * Sanity check for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (pFuncs == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to pFuncs \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (clientID > (int) pThis->maxNoClients)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to pFuncs \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_CLIENTID);
    }

    if (nfuncs > CL_EO_MAX_NO_FUNC)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper No. of services passed \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_SERVICEID);
    }


    pFunction = (ClEoServiceObjT *) (pThis->pClient + clientID);
    for (i = 0; i < nfuncs; i++)
    {
        if (*(pFuncs + i) != NULL)
        {
            pTemp =
                (ClEoServiceObjT *) clHeapAllocate((ClUint32T)
                        sizeof(ClEoServiceObjT));
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: MALLOC FAILED \n"));
                CL_FUNC_EXIT();
                return CL_EO_RC(CL_ERR_NO_MEMORY);
            }
            pTemp->func = (void (*)(void)) (*(pFuncs + i));

            pTemp->pNextServObj = (*(pFunction + i)).pNextServObj;
            (*(pFunction + i)).pNextServObj = pTemp;
        }
    }
    (pThis->pClient + clientID)->data = data;

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clEoClientUninstallTable(ClEoExecutionObjT *pThis,
                               ClUint32T clientID,
                               ClEoPayloadWithReplyCallbackServerT *pfunTable,
                               ClUint32T nentries)
{
    ClInt32T i;
    ClEoServerTableT *client = NULL;
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    
    if(!pThis) return rc;
    if(clientID > pThis->maxNoClients) return rc;

    client = pThis->pServerTable + clientID;    
    for(i = 0; i < nentries; ++i)
    {
        ClEoPayloadWithReplyCallbackServerT *entry = pfunTable + i;
        ClUint32T funId = entry->funId & CL_EO_FN_MASK;
        ClUint32T version = entry->version;
        ClUint32T index = __CLIENT_RADIX_TREE_INDEX(funId, version);
        if (entry->fun)
        {
            ClPtrT item = NULL;
            clRadixTreeDelete(client->funTable, index, &item);
        }
    }
    
    return CL_OK;
}

ClRcT clEoClientInstallTable(ClEoExecutionObjT *pThis,
                             ClUint32T clientID,
                             ClEoDataT data,
                             ClEoPayloadWithReplyCallbackServerT *pfunTable,
                             ClUint32T nentries)
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    ClEoServerTableT *client = NULL;
    ClInt32T i;
    
    if (!pThis || !pThis->pServerTable) return rc;
    
    if (clientID > pThis->maxNoClients) return rc;

    client = pThis->pServerTable + clientID;
    client->data = data;
    
    for (i = 0; i < nentries; ++i)
    {
        ClEoPayloadWithReplyCallbackServerT *entry = pfunTable + i;
        ClUint32T funId = entry->funId & CL_EO_FN_MASK;
        ClUint32T version = entry->version;
        ClUint32T index = __CLIENT_RADIX_TREE_INDEX(funId, version);

        if (entry->fun)
        {
            ClPtrT lastFun = NULL;
            rc = clRadixTreeInsert(client->funTable, index, (ClPtrT)&entry->fun, &lastFun);
            if (CL_OK != rc)
            {
                clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                           "radix tree insert error [%d] for fun id [%d], "
                           "version [%#x]\n", rc, funId, version);
                goto out_delete;
            }
        }
    }

    return rc;

    out_delete:
    {
        int j;
        for(j = 0; j < i; ++j)
        {
            ClEoPayloadWithReplyCallbackServerT *entry = pfunTable + j;
            ClUint32T funId = entry->funId & CL_EO_FN_MASK;
            ClUint32T version = entry->version;
            ClUint32T index = __CLIENT_RADIX_TREE_INDEX(funId, version);
            ClPtrT item = NULL;
            if (clRadixTreeDelete(client->funTable, index, &item) != CL_OK)
            {
                clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                           "radix tree delete error for fun id [%d], "
                           "version [%#x]\n", funId, version);
            }
        }
    }
    return rc;
}

static ClRcT eoClientInstallTables(ClEoExecutionObjT *pThis,
                                   ClEoPayloadWithReplyCallbackTableServerT *table,
                                   ClEoDataT data)
{
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!pThis || !table) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    for(i = 0; table[i].funTable; ++i)
    {
        rc = clEoClientInstallTable(pThis,
                                    table[i].clientID,
                                    data,
                                    table[i].funTable,
                                    table[i].funTableSize);
        if(rc != CL_OK)
        {
            clLogError("EO", "CLNT-INSTALL", "EO client table deploy returned [%d] for table id [%d]",
                       rc, i);
            CL_ASSERT(0);
            break;
        }

    }
    return rc;
}

ClRcT clEoClientInstallTables(ClEoExecutionObjT *pThis,
                              ClEoPayloadWithReplyCallbackTableServerT *table)
{
    return eoClientInstallTables(pThis, table, NULL);
}

ClRcT clEoClientInstallTablesWithCookie(ClEoExecutionObjT *pThis,
                                        ClEoPayloadWithReplyCallbackTableServerT *table,
                                        ClEoDataT data)
{
    return eoClientInstallTables(pThis, table, data);
}

ClRcT clEoClientUninstallTables(ClEoExecutionObjT *pThis,
                                ClEoPayloadWithReplyCallbackTableServerT *table)
{
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!pThis || !table) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    for(i = 0; table[i].funTable; ++i)
    {
        clEoClientUninstallTable(pThis,
                                 table[i].clientID,
                                 table[i].funTable,
                                 table[i].funTableSize);
    }

    return rc;
}

/**
 *  NAME: clEoStart
 * 
 *  This function starts the EO. It creates the EO receive loop 
 *
 *  @param    pThis    : handle to EO 
 *
 *  @returns CL_OK      - Success<br>
 *           CL_ERR_NULL_POINTER   - Input parameter improper(NULL)
 */

static ClRcT clEoStart(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK; 
    ClUint32T thrCount = 0;
    CL_FUNC_ENTER();

    /*
     * Sanity check for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\n EO: NULL passed for Exectuion Object\n"));
        rc = CL_EO_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    /*
     * Installing the default services
     */
    rc = clEoClientInstall(pThis, CL_EO_EO_MGR_CLIENT_TABLE_ID,
                           defaultServiceFuncList, 0,
                           sizeof(defaultServiceFuncList) /
                           sizeof(ClEoPayloadWithReplyCallbackT));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nEO: clEoClientInstall() failed for CL_EO_EO_MGR_CLIENT_TABLE_ID, rc[0x%x]\n", rc));
        goto failure;
    }

    PTHREAD_DEBUG("\n\tThe Main Thread[0x%x]\n");
    /*
     * Declaring EO to be active before starting any threads 
     */
    pThis->state = CL_EO_STATE_ACTIVE;
    rc = eoAddEntryInGlobalTable(pThis, pThis->eoPort);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nEO: eoAddEntryInGlobalTable() failed, rc[0x%x]\n", rc));
        goto eoClientInstalled;
    }

    rc = clEoPriorityQueuesInitialize();
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nEO: clEoPriorityQueueInitialize() failed, rc[0x%x]\n", rc));
        goto eoAddedEntry;
    }

    gClEoThreadCount = 2;

    /*
     * No point in using heap here as we are early even though its up.
     */
    gClEoTaskIds = calloc(gClEoThreadCount, sizeof(ClOsalTaskIdT));

    if(!gClEoTaskIds)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory for [%d] threads\n", gClEoThreadCount));
        goto eoStaticQueueInitialized;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                   ("\n EO: Spawning the required no of EO Receive Loop \n"));
    /*
     * Create thread for picking up the IOC messages and putting it on user
     * level Queue
     */
    rc = clEoTaskCreate("eoStaticQueueProcess", (ClEoTaskFuncPtrT) eoStaticQueueProcess, pThis, &gClEoTaskIds[thrCount++]);
    if (CL_OK != rc)
    {
        goto eoStaticQueueInitialized;
    }

    if(thrCount < gClEoThreadCount)
    {
        /*
         * EO receiver thread must be the last thread under gClEoTaskIds.
         */
        rc = clEoTaskCreate("clEoIocRecvQueueProcess", (ClEoTaskFuncPtrT) clEoIocRecvQueueProcess, 
                            pThis, &gClEoTaskIds[thrCount++]);
        if (CL_OK != rc)
        {
            goto eoStaticQueueInitialized;
        }
        gClEoReceiverSpawned = 1;
    }

    /*
     * Sync up the component manager with the updated state of EO 
     */

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                   ("\n EO: Calling EO Create Callouts ...... \n"));
    rc = clEoCreateSystemCallout(pThis);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\n EO: clEoCreateSystemCallout() failed, rc[0x%x]\n", rc));
        goto eoStaticQueueInitialized;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                   ("\n EO: Calling clEoStateChangeSystemCallout() ...... \n"));
    rc = clEoStateChangeSystemCallout(pThis);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\n EO: clEoStateChangeSystemCallout() failed, rc[0x%x]\n", rc));
        goto eoStaticQueueInitialized;
    }

    CL_FUNC_EXIT();
    return CL_OK;

    eoStaticQueueInitialized:
    if(gClEoTaskIds)
    {
        free(gClEoTaskIds);
        gClEoTaskIds = NULL;
        gClEoThreadCount = 0;
    }

    clEoPriorityQueuesFinalize(CL_TRUE);

    eoAddedEntry:
    eoDeleteEntryFromGlobalTable(pThis->eoPort);

    eoClientInstalled:
    clEoClientUninstall(pThis, CL_EO_EO_MGR_CLIENT_TABLE_ID);

    failure:
    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoServiceIndexGet
 *  
 *  This function translates the input into appropriate table no and
 *  function no
 *
 *  @param    func        :
 *            pClientID    : table in which to search for
 *            pFuncNo      : id of function to be called
 *
 *  @returns void 
 */

ClRcT clEoServiceIndexGet(ClUint32T func, ClUint32T *pClientID,
        ClUint32T *pFuncNo)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if ((pClientID == NULL) || (pFuncNo == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: Improper input parameters \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    *pClientID = func >> CL_EO_CLIENT_BIT_SHIFT;
    *pFuncNo = func & CL_EO_FN_MASK;

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoServiceInstall 
 * 
 *  This function installs a particular Client function in the EO's table
 *
 *  @param    pThis    : handle to EO 
 *            pFunction: function ptr to be installed                           
 *            iFuncNum : function no.                            
 *            order    : where to add - front or back                     
 *
 *  @returns CL_OK            - Success<br>
 *           CL_ERR_NULL_POINTER         - Input parameter improper(NULL)
 *           CL_ERR_NO_MEMORY       - Memory allocation failed
 *           CL_EO_ERR_INVALID_SERVICEID- ServiceID out of range
 *           CL_ERR_INVALID_PARAMETER  - Invalid input parameter
 */

/*
 * Bug 4822:
 * Corrected the type of "order" from ClUint32T to 
 * ClEOServiceInstallOrderT.
 */
ClRcT clEoServiceInstall(ClEoExecutionObjT *pThis,
        ClEoPayloadWithReplyCallbackT pFunction,
        ClUint32T iFuncNum, ClEOServiceInstallOrderT order)
{
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;
    ClEoServiceObjT *pFunc = NULL;
    ClEoServiceObjT *pTemp = NULL;
    ClEoServiceObjT *pNextNode = NULL;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    /*
     * Sanity check for function parameters 
     */
    if ((pThis == NULL) || (pFunction == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    /*
     * Sanity check on service id 
     */
    if (iFuncNum >=
             CL_EO_GET_FULL_FN_NUM(pThis->maxNoClients, CL_EO_MAX_NO_FUNC))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper service no. passed. Svc no passed is %d\n",
                 iFuncNum));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_SERVICEID);
    }

    /*
     * Sanity check on order 
     */
    if ((order != CL_EO_ADD_TO_BACK) && (order != CL_EO_ADD_TO_FRONT))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper order specified for addition \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * Get the service index 
     */
    rc = clEoServiceIndexGet((ClUint32T) iFuncNum, &clientID, &funcNo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: clEoServiceIndexGet failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    pFunc = (ClEoServiceObjT *) (pThis->pClient + clientID);
    pNextNode = (ClEoServiceObjT *) ((*(pFunc + funcNo)).pNextServObj);
    pTemp =
        (ClEoServiceObjT *) clHeapAllocate((ClUint32T) sizeof(ClEoServiceObjT));
    if (pTemp == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: MALLOC failed \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NO_MEMORY);
    }
    pTemp->func = (void (*)(void)) pFunction;

    /*
     * Add the element as per the specified order of addition 
     */
    if (order == CL_EO_ADD_TO_FRONT)
    {
        pTemp->pNextServObj = (*(pFunc + funcNo)).pNextServObj;
        (*(pFunc + funcNo)).pNextServObj = pTemp;
    }
    else
    {
        if (pNextNode == NULL)
        {
            (*(pFunc + funcNo)).pNextServObj = pTemp;
            pTemp->pNextServObj = NULL;
        }
        else
        {
            while (1)
            {
                if (pNextNode->pNextServObj == NULL)
                {
                    pNextNode->pNextServObj = pTemp;
                    pTemp->pNextServObj = NULL;
                    break;
                }
                pNextNode = pNextNode->pNextServObj;
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

void clEoCleanup(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;

    rc = eoDeleteEntryFromGlobalTable(pThis->eoPort);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\n EO: eoDeleteEntryFromGlobalTable failed \n"));

    pThis->state = CL_EO_STATE_KILL;
    (void) clIocCommPortDelete(pThis->commObj);
    clEoClientCallbackDbFinalize();
    clRmdObjClose(pThis->rmdObj);
    (void) clCntDelete((pThis)->pEOPrivDataHdl);
    eoStaticQueueExit();
    clEoClientUninstall(pThis, CL_EO_EO_MGR_CLIENT_TABLE_ID);
    clEoClientUninstall(pThis, CL_CPM_MGR_CLIENT_TABLE_ID);
    clHeapFree((pThis)->pClient);
    clHeapFree(pThis);
    gpExecutionObject = NULL;
    gEOIocPort = 0;
}

void clEoReceiverUnblock(ClEoExecutionObjT *pThis)
{
    if(!pThis || !gClEoReceiverSpawned) 
        return ;

    clOsalMutexLock(&gClEoJobMutex);
    clOsalMutexLock(&pThis->eoMutex);
    pThis->state = CL_EO_STATE_STOP;
    clIocCommPortReceiverUnblock(pThis->commObj);
    clOsalMutexUnlock(&pThis->eoMutex);
    clOsalMutexUnlock(&gClEoJobMutex);

    if(gClEoTaskIds && gClEoThreadCount > 0 && gClEoTaskIds[gClEoThreadCount-1])
    {
        clOsalTaskJoin(gClEoTaskIds[gClEoThreadCount-1]);
        gClEoTaskIds[gClEoThreadCount-1] = 0;
    }
}

ClRcT clEoUnblock(ClEoExecutionObjT *pThis)
{
    register ClInt32T i;
    if(!pThis)
    {
        ClRcT rc;
        rc = clEoMyEoObjectGet(&pThis);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("EO: Failed to get my EO object. error code [0x%x].\n", rc));
            return rc;
        }
    }
    /*
     * Need to set the flag before waking other threads.
     */
    clOsalMutexLock(&gClEoJobMutex);
    clOsalMutexLock(&pThis->eoMutex);
    if(pThis->threadRunning == CL_FALSE)
    {
        clOsalMutexUnlock(&pThis->eoMutex);
        clOsalMutexUnlock(&gClEoJobMutex);
        return CL_OK;
    }
    pThis->threadRunning = CL_FALSE;
    clOsalMutexUnlock(&pThis->eoMutex);
    clOsalMutexUnlock(&gClEoJobMutex);
    clEoQueuesQuiesce();

    /*
     * Wake up static queue threads
     */
    clOsalMutexLock(&gEoStaticQueue.jobQueueLock);
    clOsalCondBroadcast(&gEoStaticQueue.jobQueueCond);
    clOsalMutexUnlock(&gEoStaticQueue.jobQueueLock);

    /*
     * We reset all taskIds since certain versions of pthread lib.
     * have a join bug for joins to threads no longer alive.
     * We dont unblock the receiver thread here but do that after
     * client libs are finalized
     */
    
    for(i = 0; i < gClEoThreadCount-gClEoReceiverSpawned; ++i)
    {
        if(gClEoTaskIds[i])
        {
            clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_WORKER,
                       "Waiting for EO [%s - %#llx], index [%d] to unblock\n", 
                       CL_EO_NAME, gClEoTaskIds[i], i);

            clOsalTaskJoin(gClEoTaskIds[i]);
            gClEoTaskIds[i] = 0;
        }
    }

    clEoRefDec(pThis);
#if 0    
    clOsalMutexLock(&pThis->eoMutex);

    if(pThis->refCnt > 0 )
    {
        --pThis->refCnt;
    }
    clOsalCondSignal(&pThis->eoCond);
    clOsalMutexUnlock(&pThis->eoMutex);
#endif
    
    clLogNotice(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_WORKER,
                "EO [%s] unblocked and exiting", CL_EO_NAME);

    return CL_OK;
}


/**
 *  NAME: eoInit 
 * 
 *  This function initializes the EO by calling the function corresponding
 *  to the EO_INITIALIZE_COMMON_FN_ID of all the clients 
 *
 *  @param    pThis    : handle to EO 
 *
 *  @returns CL_OK        - Success<br>
 *           CL_ERR_NULL_POINTER     - Input parameter improper(NULL)
 */

static ClRcT eoInit(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;
    ClEoServiceObjT *pTemp = NULL;
    ClUint32T i = 0;

    CL_FUNC_ENTER();

    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: Calling the init routines .... \n"));
    for (i = 0; i <= CL_EO_NATIVE_COMPONENT_TABLE_ID; i++)
    {
        pTemp = (ClEoServiceObjT *) (pThis->pClient + i);

        if ((ClEoServiceObjT *) (*pTemp).pNextServObj == NULL)
        {
            /*
             * This is not an error condition 
             */
            rc = CL_OK;
        }
    }
    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoClientDataSet  
 * 
 *  This function is for storing the client data
 *
 *  @param    pThis    : handle to EO 
 *            clientID : client identifier
 *            data     : client specific data
 *
 *  @returns CL_OK      - Success<br>
 *           CL_ERR_NULL_POINTER   - Input parameter improper(NULL)
 */

ClRcT clEoClientDataSet(ClEoExecutionObjT *pThis, ClUint32T clientID,
        ClEoDataT data)
{
    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (clientID > pThis->maxNoClients)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to clientID \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_CLIENTID);
    }

    (pThis->pClient + clientID)->data = data;


    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *  NAME: clEoClientDataGet
 * 
 *  This function is for retrieving the client data
 *
 *  @param    pThis     : handle to EO 
 *            clientID  : client identifier
 *            pData     : client specific data
 *
 *  @returns CL_OK      - Success<br>
 *           CL_ERR_NULL_POINTER   - Input parameter improper(NULL)
 */

ClRcT clEoClientDataGet(ClEoExecutionObjT *pThis, ClUint32T clientID,
        ClEoDataT *pData)
{
    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if ((pThis == NULL) || (pData == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: Improper input parameters \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (clientID > pThis->maxNoClients)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to clientID \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_CLIENTID);
    }

    *pData = (pThis->pClient + clientID)->data;

    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *  NAME: clEoServiceValidate
 *
 *  This function checks whether the request is for a function that
 *  has been registered or not
 * 
 *  @param    pThis    : handle to EO 
 *            func     : function to be invoked
 *
 *  @returns CL_OK           - Success<br>
 *           CL_EO_ERR_EO_SUSPENDED- EO is in suspended state
 */

ClRcT clEoServiceValidate(ClEoExecutionObjT *pThis, ClUint32T func)
{
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;
    ClRcT rc = CL_OK;
    ClEoServiceObjT *pTemp = NULL;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    rc = clEoServiceIndexGet(func, &clientID, &funcNo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("\n EO: clEoServiceIndexGet failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    if ((funcNo >= CL_EO_MAX_NO_FUNC) ||   /* out of range fn # */
        (clientID > pThis->maxNoClients))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid service index : Func id [%d], Client ID [%d]\n",
                                        funcNo, clientID));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED);
    }

    if (pThis->pServerTable 
        &&
        clEoClientTableFilter(pThis->eoPort, clientID))
    {
        /* validations should go here */
        return CL_OK;
    }

    pTemp = (ClEoServiceObjT *) (pThis->pClient + clientID);

    if ((pThis->state == CL_EO_STATE_SUSPEND) &&
            (clientID == CL_EO_NATIVE_COMPONENT_TABLE_ID) &&
            (funcNo != CL_EO_SET_STATE_COMMON_FN_ID))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("EO: FUNC registered but EO SUSPENDED \n"));
        rc = CL_EO_RC(CL_EO_ERR_EO_SUSPENDED);
    }
    else if ( ((ClEoServiceObjT *) (*(pTemp + funcNo)).pNextServObj == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("EO: FUNC %d  not registered for EOId %llx Port %x.",
                 funcNo, pThis->eoID, pThis->eoPort));
        rc = CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED);
    }

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("EO: Function Validation failed for EoId %llx EoPort %x.",
                 pThis->eoID, pThis->eoPort));
    }

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoWalk 
 * 
 *  This function calls clRmdInvoke for each of the callback functions
 *  registered with EO
 *
 *  @param    pThis       : handle to EO 
 *            func        : function no. to be executed               
 *            pFuncCallout: function that will perform the actual exec 
 *
 *  @returns CL_OK      - Success<br>
 */

ClRcT clEoWalk(ClEoExecutionObjT *pThis, ClUint32T func,
        ClEoCallFuncCallbackT pFuncCallout,
        ClBufferHandleT inMsgHdl,
        ClBufferHandleT outMsgHdl)
{
    ClEoServiceObjT *pTemp = NULL;
    ClEoServiceObjT *pFuncTable = NULL;
    ClRcT rc = CL_OK;
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if ((pThis == NULL) || (pFuncCallout == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: Improper input parameters \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: validating the requested service ...... \n"));
    rc = clEoServiceValidate(pThis, func);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Service validation failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: Obtaining the table indices ...... \n"));
    rc = clEoServiceIndexGet(func, &clientID, &funcNo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("\n EO: clEoServiceIndexGet failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    pTemp = (ClEoServiceObjT *) (pThis->pClient + clientID);

    pFuncTable = (*(pTemp + funcNo)).pNextServObj;
    while (pFuncTable != NULL)
    {
        rmdMetricUpdate(CL_OK, clientID, funcNo, inMsgHdl, CL_FALSE);
        rc = pFuncCallout((ClEoPayloadWithReplyCallbackT) pFuncTable->func,
                (pThis->pClient + clientID)->data, inMsgHdl,
                outMsgHdl);
        rmdMetricUpdate(rc, clientID, funcNo, outMsgHdl, CL_TRUE);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                    ("\n EO: Callback function FAILED in EOId:%llx EOPort:%x rc =%d \n",
                     pThis->eoID, rc, pThis->eoPort));
            break;
        }

        pFuncTable = pFuncTable->pNextServObj;
    }

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEoWalkWithVersion(ClEoExecutionObjT *pThis, ClUint32T func,
                          ClVersionT *version,
                          ClEoCallFuncCallbackT pFuncCallout,
                          ClBufferHandleT inMsgHdl,
                          ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClUint32T clientID = (func >> CL_EO_CLIENT_BIT_SHIFT);
    ClUint32T funcID = func & CL_EO_FN_MASK;
    ClEoServerTableT *client = NULL;
    ClUint32T versionCode = 0;
    ClUint32T index = 0;
    ClEoPayloadWithReplyCallbackT *fun = NULL;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if ((pThis == NULL) || (version == NULL) || (pFuncCallout == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Improper input parameters"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    /* CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("EO: validating the requested service ......")); */
    rc = clEoServiceValidate(pThis, func);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Service validation failed rc [0x%x]",rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* End of Should be part of clEoServiceValidate */

    client = pThis->pServerTable + clientID;
    versionCode = CL_VERSION_CODE(version->releaseCode, version->majorVersion, version->minorVersion);
    index = __CLIENT_RADIX_TREE_INDEX(funcID, versionCode);

    if (1)  
    {
        int retries=0;

        while (retries<10) /* Looping due to possible initialization race condition causing the function table to not be populated when accessed */
        {
            retries++;
            rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&fun);
            if (rc == CL_OK) break;
            sleep(1);
            if (!(retries % 2))
              {
                clLogTrace(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV, "Retry RMD function [%d.%d] lookup due to error %x.", clientID, funcID, rc);
              }
        }
    } 
    
    if (rc != CL_OK || !fun)
    {
        int rc2;
        if(func == CPM_MGMT_NODE_CONFIG_GET)
            return CL_RMD_ERR_CONTINUE;

        // clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV, "Function lookup returned 0x%x", rc);
        
        /*
         * Try looking up the client table for the max. version of the function supported.
         */
        ClVersionT maxSupportedVersion = {0};
        rc2 = clEoClientGetFuncVersion(pThis->eoPort, func, &maxSupportedVersion);
        if(rc2 != CL_OK || !maxSupportedVersion.releaseCode)
        {
            clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV,
                       "function id [%d] not registered in the client table for version [%#x]",
                       funcID, versionCode);
            return CL_EO_RC(CL_ERR_DOESNT_EXIST);
        }
        /* Retries with mismatch version RMD */
        clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV,
                   "RMD function lookup error %x. Max version of function id [%d], client id [%d], supported is [%d.%d.%d]. Requested version [%d.%d.%d]",
                   rc, funcID, clientID, maxSupportedVersion.releaseCode, maxSupportedVersion.majorVersion,
                   maxSupportedVersion.minorVersion, version->releaseCode, version->majorVersion,
                   version->minorVersion);
        if(outMsgHdl)
        {
            clXdrMarshallClVersionT((void*)&maxSupportedVersion, outMsgHdl, 0);
        }
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_VERSION_MISMATCH);
    }

    clLogTrace("EO", "WALK", "Radix tree lookup success for Function [%d], client [%d], version [%d.%d.%d]",
               funcID, clientID, version->releaseCode, version->majorVersion, version->minorVersion);

    /* TODO: Need to fill in second parameter in the following */
    rmdMetricUpdate(CL_OK, clientID, funcID, inMsgHdl, CL_FALSE);
    rc = pFuncCallout(*fun, client->data, inMsgHdl, outMsgHdl);
    rmdMetricUpdate(rc, clientID, funcID, outMsgHdl, CL_TRUE);

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoStateSet 
 * 
 *  This function  sets the state of EO as directed by the Component Mgr
 *  
 *  @param    pThis    : handle to EO 
 *            flags    : which state to switch to
 *
 *  @returns CL_OK      - Success<br>
 *           CL_EO_ERR_IMPROPER_STATE - State change request for improper state
 */

/*
 * Bug 4822:
 * Corrected the type of "flags" from ClUint32T to 
 * ClEoStateT.
 */
ClRcT clEoStateSet(ClEoExecutionObjT *pThis, ClEoStateT flags)
{
    ClRcT rc = CL_OK;

    /*
     * ClOsalTaskIdT taskId = 0;
     */
    int stateUpd = 0;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    switch (flags)
    {
        case CL_EO_STATE_SUSPEND:
            /*
             * Allow only management functions; dont allow services EO. comm
             * obj will be there 
             */
            /*
             * Ask Application to suspend its state and operation 
             */
            if (pThis->clEoStateChgCallout != NULL)
            {
                rc = pThis->clEoStateChgCallout(CL_EO_STATE_SUSPEND);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("\n EO: StateChg User Callout function failed while suspending \n"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            pThis->state = CL_EO_STATE_SUSPEND;
            stateUpd = 1;
            break;

        case CL_EO_STATE_KILL:
            /*
             * Donot allow management functions; dont allow services Both EO
             * and comm obj will be deleted 
             */

            pThis->state = CL_EO_STATE_KILL;
            stateUpd = 0;

            /*
             * Inform Component manager regarding state change 
             */

            clEoUnblock(pThis);
            break;
        case CL_EO_STATE_RESUME:
            /*
             * Ask Application to change its state 
             */
            if (pThis->clEoStateChgCallout != NULL)
            {
                rc = pThis->clEoStateChgCallout(CL_EO_STATE_RESUME);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("\n EO: StateChg User Callout function failed \n"));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            pThis->state = CL_EO_STATE_ACTIVE;
            stateUpd = 1;
            break;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: STATE NOT PROPER \n"));
            return CL_EO_RC(CL_EO_ERR_IMPROPER_STATE);

            break;
    }

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoDataCompare 
 * 
 *  Compare function needed by container class 
 *  
 *  @param   inputData : Data to be compared  
 *           storedData: Data stored in container 
 *
 *  @returns -1/0/1  - representing </=/>
 */

static ClUint32T clEoDataCompare(ClCntKeyHandleT inputData,
        ClCntKeyHandleT storedData)
{
    if ((inputData) < (storedData))
        return -1;
    else if ((inputData) > (storedData))
        return 1;
    else
        return 0;
}

/**
 *  NAME: clEoPrivateDataSet 
 * 
 * This function is for storing data in EO specific data area. 
 *  
 *  @param   pThis  : handle to EO object  
 *           type   : User specified key
 *           pData  : EO specific data
 *
 *  @returns result of clCntNodeAdd 
 *           CL_ERR_NULL_POINTER - Invalid input parameter 
 */

ClRcT clEoPrivateDataSet(ClEoExecutionObjT *pThis, ClUint32T key, void *pData)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Inside clEoPrivateDataSet \n"));

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper input parameter pData \n"));
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    /*
     * First find and delete the data stored for this, key if any 
     */
    clOsalMutexLock(&pThis->eoMutex);
    rc = clCntNodeFind(pThis->pEOPrivDataHdl, (ClCntKeyHandleT) (ClWordT)key,
            &nodeHandle);
    if (rc == CL_OK)
    {
        rc = clCntNodeDelete(pThis->pEOPrivDataHdl, nodeHandle);
        if (rc != CL_OK)
        {
            clOsalMutexUnlock(&pThis->eoMutex);
            return rc;
        }
    }

    rc = clCntNodeAdd (pThis->pEOPrivDataHdl, (ClCntKeyHandleT) (ClWordT)key,
            (ClCntDataHandleT) pData, NULL);

    clOsalMutexUnlock(&pThis->eoMutex);

    return rc;
}

/**
 *  NAME: clEoPrivateDataGet 
 * 
 *  This function is for retrieving data stored in EO specific data area 
 *  
 *  @param   pThis : handle to EO object  
 *           type  : User specified key
 *           data  : EO specific data
 *
 *  @returns result of clCntNodeUserDataGet
 *           CL_ERR_NULL_POINTER - Invalid input parameter 
 */

ClRcT clEoPrivateDataGet(ClEoExecutionObjT *pThis, ClUint32T key, void **pData)
{
    ClCntNodeHandleT nodeHandle;
    ClRcT retCode = CL_OK;
    ClBoolT locked = CL_TRUE;

    /* CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Inside clEoPrivateDataGet \n")); */

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper input parameter pData \n"));
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if(pThis->state == CL_EO_STATE_STOP)
    {
        locked = CL_FALSE;
    }
    else
    {
        clOsalMutexLock(&pThis->eoMutex);
    }

    /*
     * Find the node related to the type 
     */
    retCode =
        clCntNodeFind(pThis->pEOPrivDataHdl, (ClCntKeyHandleT) (ClWordT)key,
                &nodeHandle);
    if (CL_OK == retCode)
    {
        /*
         * Retrieve the user data from the node 
         */
        retCode =
            clCntNodeUserDataGet(pThis->pEOPrivDataHdl, nodeHandle,
                    (ClCntDataHandleT *) pData);
    }
    if(locked)
        clOsalMutexUnlock(&pThis->eoMutex);

    return retCode;
}

/**
 *  NAME: clEoClientUninstall 
 * 
 *  This function is called by the client to Uninstall its function table     
 *  with the EO
 *
 *  @param    pThis    : handle to EO 
 *            clientID : client identifier                              
 *
 *  @returns CL_OK            - Success<br>
 *           CL_ERR_NULL_POINTER         - Input parameter improper(NULL)
 *           CL_ERR_NO_MEMORY       - Memory allocation failed
 *           CL_EO_ERR_INVALID_CLIENTID - Improper client ID
 */

ClRcT clEoClientUninstall(ClEoExecutionObjT *pThis, ClUint32T clientID)
{
    int i = 0;
    ClEoServiceObjT *pTemp = NULL;
    ClEoServiceObjT *pFunction = NULL;
    ClEoServiceObjT *pTemp1 = NULL;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: UnInstalling the function table ...... \n"));

    /*
     * Sanity checks for function parameters 
     */
    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (clientID > (int) pThis->maxNoClients)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to pFuncs \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_CLIENTID);
    }

    pFunction = (ClEoServiceObjT *) (pThis->pClient + clientID);
    (pThis->pClient + clientID)->data = 0;

    for (i = 0; i < CL_EO_MAX_NO_FUNC; i++)
    {
        pTemp = (pFunction + i);
        pTemp1 = pTemp->pNextServObj;
        while (pTemp1 != NULL)
        {
            pTemp->pNextServObj = pTemp1->pNextServObj;
            pTemp1->func = NULL;
            pTemp1->pNextServObj = NULL;
            clHeapFree(pTemp1);
            pTemp1 = NULL;
            pTemp1 = pTemp->pNextServObj;
        }
    }
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *  NAME: clEoServiceUnInstall 
 * 
 *  This function Uninstalls a particular client function from EO
 *  function table     
 *
 *  @param    pThis    : handle to EO 
 *            pFunction: function ptr to be installed                           
 *            iFuncNum : function no.                            
 *
 *  @returns CL_OK             - Success<br>
 *           CL_ERR_NULL_POINTER          - Input parameter improper(NULL)
 *           CL_EO_ERR_FUNC_NOT_REGISTERED    - Trying to unregister a function which is 
 *                                  not registered 
 *           CL_EO_ERR_INVALID_SERVICEID - Improper servide id passed
 */

ClRcT clEoServiceUninstall(ClEoExecutionObjT *pThis,
        ClEoPayloadWithReplyCallbackT pFunction,
        ClUint32T iFuncNum)
{
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;
    ClEoServiceObjT *pFunc = NULL;
    ClEoServiceObjT *pTemp = NULL;
    ClEoServiceObjT *pNextNode = NULL;
    ClRcT rc = CL_OK;
    int found = 0;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Inside clEoServiceUnInstall \n"));

    /*
     * Sanity checks for function parameters 
     */
    if ((pThis == NULL) || (pFunction == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper reference to EO Object \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    if (iFuncNum >= CL_EO_MAX_NO_FUNC)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Improper service no. passed \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_INVALID_SERVICEID);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("\n EO: Obtaining the table indices ...... \n"));
    rc = clEoServiceIndexGet((ClUint32T) iFuncNum, &clientID, &funcNo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: clEoServiceIndexGet failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    if ((funcNo >= CL_EO_MAX_NO_FUNC) ||    /* out of range fn # */
            (clientID > pThis->maxNoClients))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("\n EO: Trying to unregister a function which is not registered \n"));
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("\n EO: FUNC %d  not registered for EOId:%llx EoPort:%x\n",
                 funcNo, pThis->eoID, pThis->eoPort));
        rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    pFunc = (ClEoServiceObjT *) (pThis->pClient + clientID);
    pTemp = pFunc + funcNo;
    pNextNode = (*(pFunc + funcNo)).pNextServObj;
    while (pNextNode != NULL)
    {
        if ((void (*)(void)) (pNextNode->func) == (void (*)(void)) pFunction)
        {
            pTemp->pNextServObj = pNextNode->pNextServObj;
            found = 1;
            clHeapFree(pNextNode);
            break;
        }
        else
        {
            pTemp = pNextNode;
            pNextNode = pNextNode->pNextServObj;
        }
    }
    if (found == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("\n EO: Trying to unregister a function which is not registered \n"));
        CL_FUNC_EXIT();
        return CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED);
    }

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  NAME: clEoLibInitialize 
 * 
 *  This is the EO Library initialization function
 *
 *  @param    pConfigParams : contains configuration related information
 *
 *  @returns CL_OK          - Success<br>
 *           CL_ERR_NULL_POINTER       - Input parameter improper(NULL)
 */

ClRcT clEoLibInitialize()
{
    ClRcT rc = CL_OK;

#ifdef DEBUG
    rc = dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: dbgAddComponent FAILED \n"));
        CL_FUNC_EXIT();
        return rc;
    }
#endif

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Inside clEoLibInitialize \n"));

    rc = clOsalMutexInit(&gClEoJobMutex);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("EO job mutex init returned [%#x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clCntHashtblCreate(EO_BUCKET_SZ, eoGlobalHashKeyCmp,
            eoGlobalHashFunction, eoGlobalHashDeleteCallback,
            eoGlobalHashDeleteCallback, CL_CNT_UNIQUE_KEY,
            &gEOObjHashTable);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Hash Table Creation FAILED \n"));
        CL_FUNC_EXIT();
        return rc;
    }
    rc = clOsalMutexInit(&gEOObjHashTableLock);
    if (rc != CL_OK)
    {
        clCntDelete(gEOObjHashTable);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: EOHashTableLock  creation failed \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    clEoLibInitialized = CL_TRUE;

    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *  NAME: clEoLibFinalize 
 * 
 *  This is the EO Library finalize function
 *
 *  @param   none 
 *
 *  @returns none 
 */

ClRcT clEoLibFinalize()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n EO: Inside %s \n", __FUNCTION__));

    CL_EO_LIB_VERIFY();

    if(gClEoNotificationHandle)
    {
        clCpmNotificationCallbackUninstall(&gClEoNotificationHandle);
    }

    rc = clCntDelete(gEOObjHashTable);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\n EO: Hash Table Deleteion FAILED \n"));
        CL_FUNC_EXIT();
        return rc;
    }

    clEoLibInitialized = CL_FALSE;

    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *  NAME: clEoMyEoIocPortSet 
 * 
 *  This function stores the EOID in task specific area 
 *
 *  @param    eoId : EOID to be stored 
 *
 *  @returns CL_OK          - Success<br>
 */

ClRcT clEoMyEoIocPortSet(ClIocPortT eoPort)
{
    return CL_OK;
}

/**
 *  NAME: clEoMyEoIocPortGet 
 * 
 *  This function retrieves the EOID stored in the task specific area 
 *
 *  @param    pEOId : carries the retrieved value 
 *
 *  @returns CL_OK          - Success<br>
 */

ClRcT clEoMyEoIocPortGet(ClIocPortT * pEOIocPort)
{
    *pEOIocPort = gEOIocPort;
    return CL_OK;
}

/**
 *  NAME: clEoMyEoObjectSet 
 * 
 *  This function stores the EO Obj in task specific area 
 *
 *  @param    pEoObj : ClEoExecutionObjT* to be stored 
 *
 *  @returns CL_OK          - Success<br>
 */

ClRcT clEoMyEoObjectSet(ClEoExecutionObjT *pEoObj)
{
    return CL_OK;
}

/**
 *  NAME: clEoMyEoObjectGet 
 * 
 *  This function retrieves the EO Obj stored in the task specific area 
 *
 *  @param    pEOObj : carries the retrieved value 
 *
 *  @returns CL_OK          - Success<br>
 */

ClRcT clEoMyEoObjectGet(ClEoExecutionObjT **ppEOObj)
{
    CL_FUNC_ENTER();

    /*
     * Adding check for the sanity of the EO (Bug 4019).
     */
    if (gpExecutionObject == NULL)
        return CL_EO_RC(CL_ERR_INVALID_STATE);

    *ppEOObj = gpExecutionObject;

    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 * DEBUG/CLI related functions 
 */

/**
 *  NAME: clEoRmdExecute 
 * 
 *  This function is for executing an RMD command at a given EO 
 *
 *  @param    eoPort       : EOPort where to execute the RMD
 *            omAddress    : OMAddress of the EO
 *            rmdNo        : RMD to be invoked
 *            inputArg1    : 1st input arg to RMD call
 *            inputArg2    : 2nd input arg to RMD call
 *
 *  @returns CL_OK          - Success<br>
 */

ClRcT clEoRmdExecute(ClIocNodeAddressT omAddress, ClIocPortT eoPort,
        ClUint32T rmdNo, ClUint32T inputArg1, ClUint32T inputArg2)
{
    return CL_EO_RC(CL_ERR_NOT_IMPLEMENTED);
}

/************************************************************************/

/**
 *  NAME: clEoDropPkt
 * 
 *  This function is called if packet of unknown protocol is recvd 
 *
 *  @param    pThis        : EO Obj pointer, where to execute the RMD
 *            eoRecvMsg    : Recvd message
 *            ClUint8T    : Priority
 *            protoType    : Prototype
 *            length       : Message length
 *            srcAddr      : Source address
 *
 *  @returns CL_OK          - Success<br>
 */

static ClRcT clEoDropPkt(ClEoExecutionObjT *pThis,
        ClBufferHandleT eoRecvMsg, ClUint8T priority,
        ClUint8T protoType, ClUint32T length,
        ClIocPhysicalAddressT srcAddr)
{
    clBufferDelete(&eoRecvMsg);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: Unknown Protocol ID\n"));
    return CL_OK;
}

/**
 *  NAME: clEoMaximumNumberOfClientsGet 
 * 
 *  This function is for getting the user configured EO_MAX_NO_CLIENTS
 *
 *  @param    none
 *
 *  @returns max no. of clients that can be associated with an EO
 */

ClUint32T clEoMaximumNumberOfClientsGet(ClEoExecutionObjT *pThis)
{
    return pThis->maxNoClients;
}

/**
 *  NAME: eoAddEntryInGlobalTable
 * 
 *  This function is for adding eo entry (EO Obj)in the global container
 *  gEOObjHashTable. This is for RMD Short circuiting. 
 *
 *  @param    eoObject data for the container
 *  @param    eoId key for the container
 *
 *  @returns  CL_OK on successful addition.
 */
static ClRcT eoAddEntryInGlobalTable(ClEoExecutionObjT *eoObj,
        ClIocPortT eoPort)
{
    ClRcT rc;

    clOsalMutexLock(&gEOObjHashTableLock);
    rc = clCntNodeAdd(gEOObjHashTable, (ClCntKeyHandleT) (ClWordT)eoPort,
            (ClCntDataHandleT) eoObj, NULL);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("EO: Node addition Failed rc=0x%xfor eoid 0x%x\n", rc,
                 eoPort));
    clOsalMutexUnlock(&gEOObjHashTableLock);

    return rc;

}

/**
 *  NAME: eoDeleteEntryFromGlobalTable
 * 
 *  This function is for removing eo entry (EO Obj)from the global container
 *  gEOObjHashTable. This is for RMD Short circuiting. 
 *
 *  @param    eoId key for the container
 *
 *  @returns  CL_OK on successful deletion.
 */
static ClRcT eoDeleteEntryFromGlobalTable(ClIocPortT eoPort)
{
    ClRcT rc;

    clOsalMutexLock(&gEOObjHashTableLock);
    rc = clCntAllNodesForKeyDelete(gEOObjHashTable, (ClCntKeyHandleT) (ClWordT)eoPort);
    clOsalMutexUnlock(&gEOObjHashTableLock);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("EO: Node delete from gEOObjHashTable Failed rc=0x%x\n",
                 rc));
    }
    return CL_OK;
}

/**
 *  NAME: clEoGetRemoteObjectAndBlock
 * 
 *  This function searches for execution object for a given eoPort in the container 
 *  and it increments the reference count of the execution object. 
 *  
 *  @param    remoteObj  eo id used as a key
 *  @param    pRemoteEoObj  output param for returning execution object
 *
 *  @returns  CL_OK on getting the execution object.
 */
ClRcT clEoGetRemoteObjectAndBlock(ClUint32T remoteObj,
        ClEoExecutionObjT **pRemoteEoObj)
{
    ClRcT retVal;
    ClCntNodeHandleT nodeHandle;

    clOsalMutexLock(&gEOObjHashTableLock);
    retVal =
        clCntNodeFind(gEOObjHashTable, (ClCntKeyHandleT) (ClWordT)remoteObj,
                &nodeHandle);
    if (retVal != CL_OK)
    {
        clOsalMutexUnlock(&gEOObjHashTableLock);
        return retVal;
    }
    else
    {
        retVal =
            clCntNodeUserDataGet(gEOObjHashTable, nodeHandle,
                    (ClCntDataHandleT *) pRemoteEoObj);
        if (retVal != CL_OK)
        {
            clOsalMutexUnlock(&gEOObjHashTableLock);
            return retVal;
        }
    }
    if (((*pRemoteEoObj)->threadRunning == CL_FALSE) ||
            (((*pRemoteEoObj)->state != CL_EO_STATE_ACTIVE) &&
             ((*pRemoteEoObj)->state != CL_EO_STATE_SUSPEND)) ||
            ((*pRemoteEoObj)->eoInitDone == CL_FALSE))
    {
        *pRemoteEoObj = NULL;
        clOsalMutexUnlock(&gEOObjHashTableLock);
        return CL_EO_RC(CL_EO_ERR_IMPROPER_STATE);
    }
    else
    {
        clEoRefInc(*pRemoteEoObj);
    }
    clOsalMutexUnlock(&gEOObjHashTableLock);

    return CL_OK;
}

/**
 *  NAME: clEoRefInc
 * 
 *  This function increments the reference count of the execution object. 
 *  
 *  @param    remoteEoObj  execution object
 *
 *  @returns  CL_OK in all the cases.
 */
void clEoRefInc(ClEoExecutionObjT *eo)
{
    CL_ASSERT(eo);
    clOsalMutexLock(&eo->eoMutex);
    ++eo->refCnt;
    clOsalMutexUnlock(&eo->eoMutex);
}

void clEoRefDec(ClEoExecutionObjT *eo)
{
    clOsalMutexLock(&eo->eoMutex);
    --eo->refCnt;
    clOsalCondSignal(&eo->eoCond);
    clOsalMutexUnlock(&eo->eoMutex);    
}

/**
 *  NAME: clEoRemoteObjectUnblock
 * 
 *  This function decrements the reference count of the execution object. 
 *  
 *  @param    remoteEoObj  execution object
 *
 *  @returns  CL_OK in all the cases.
 */

ClRcT clEoRemoteObjectUnblock(ClEoExecutionObjT *remoteEoObj)
{
    clEoRefDec(remoteEoObj);
    return CL_OK;
}

static ClUint32T eoGlobalHashFunction(ClCntKeyHandleT key)
{
    return ((ClWordT)key % EO_BUCKET_SZ);
}

static ClInt32T eoGlobalHashKeyCmp(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

static void eoGlobalHashDeleteCallback(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    return;
}

/******************************************************************************/
/**
 *  NAME: clEoSvcPrioritySet
 * 
 *  This function sets the priority of an execution object. 
 *  
 *  @param  eoArg   - client data
 *          pEOptr : handle to EO
 *
 *  @returns  CL_OK if success, else the return value from the rmdCall.
 */
static ClRcT clEoSvcPrioritySet(ClUint32T data,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClEoExecutionObjT *eoObj = NULL;
    ClRcT rc = CL_OK;
    ClOsalThreadPriorityT priority =
    {
        0};
        ClUint32T msgLength = 0;

        rc = clEoMyEoObjectGet(&eoObj);
        EO_CHECK(CL_DEBUG_ERROR,
                ("\n EO: clEoMyEoObjectGet failed, rc = %x \n", rc), rc);

        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                ("Inside clEoSvcPrioritySet for pThis->eoPort 0x%x\n",
                 eoObj->eoPort));
        rc = clBufferLengthGet(inMsgHandle, &msgLength);
        if (msgLength == sizeof(ClOsalThreadPriorityT))
        {
            rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &priority,
                    &msgLength);
            EO_CHECK(CL_DEBUG_ERROR, ("Unable to Get message \n"), rc);
        }
        else
            EO_CHECK(CL_DEBUG_ERROR, ("Invalid Buffer Passed \n"),
                    CL_EO_RC(CL_ERR_INVALID_BUFFER));

        eoObj->pri = priority;

        return rc;

failure:
        return rc;
}

static ClRcT clEoLogLevelSet(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClLogSeverityT severity = CL_LOG_SEV_ERROR;
    ClUint32T i = sizeof(ClLogSeverityT);
    ClRcT rc = CL_OK;

    rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &severity, &i);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clLogLevelSet(severity);

    return rc;
}

static ClRcT clEoLogLevelGet(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClLogSeverityT severity = CL_LOG_ERROR;
    ClRcT rc = CL_OK;

    rc = clLogLevelGet(&severity);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *) &severity,
            sizeof(ClInt32T));

    return rc;
}

static ClRcT clEoGetState(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClEoExecutionObjT *eoObj;
    ClEoStateT state = {0};
    ClRcT rc = CL_OK;

    rc = clEoMyEoObjectGet(&eoObj);
    EO_CHECK(CL_DEBUG_ERROR, ("clEoMyEoObjectGet Failed \n"), rc);

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("Inside clEoGetState for pThis->eoPort 0x%x\n",
             eoObj->eoPort));
    state = eoObj->state;

    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *) state,
            sizeof(ClEoStateT));
    EO_CHECK(CL_DEBUG_ERROR, ("clBufferNBytesWrite Failed \n"), rc);

failure:
    return rc;
}

static ClRcT clEoSetState(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClEoStateT state = 0;
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;

    rc = VDECL_VER(clXdrUnmarshallClEoStateT, 4, 0, 0)(inMsgHandle, (void *) &state);
    EO_CHECK(CL_DEBUG_ERROR, ("Unable to Get message \n"), rc);

    if (state < CL_EO_STATE_INIT || state > CL_EO_STATE_FAILED)
    {
        return CL_EO_RC(CL_EO_ERR_IMPROPER_STATE);
    }

    rc = clEoMyEoObjectGet(&pThis);
    EO_CHECK(CL_DEBUG_ERROR, ("Unable to Get EO Object \n"), rc);

    if ((state == CL_EO_STATE_STOP) || (state == CL_EO_STATE_KILL))
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Setting EO 0x%x state to %d is Not supported \n",
                 pThis->eoPort, state));

    clEoStateSet(pThis, state);

failure:
    return rc;
}

static ClRcT clEoShowRmdStats(ClUint32T data,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRmdStatsT stats = {0};
    if(!outMsgHandle) 
        return CL_EO_RC(CL_ERR_INVALID_BUFFER);
    clRmdStatsGet(&stats);
    return clBufferNBytesWrite(outMsgHandle, (ClUint8T *) &stats, sizeof(stats));
}

static ClRcT clEoIsAlive(ClUint32T data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClEoExecutionObjT *pThis = NULL;
    ClRcT rc = CL_OK;
    ClEoSchedFeedBackT schFeedback =
    {
        CL_EO_DEFAULT_POLL, 0};

        rc = clEoMyEoObjectGet(&pThis); // Checking RC value (CID 82 for coverity on #1780)
        EO_CHECK(CL_DEBUG_ERROR, ("Message Write Failed 0x%x\n", rc), rc);

        if (pThis->clEoHealthCheckCallout)
        {
            pThis->clEoHealthCheckCallout(&schFeedback);
            rc = VDECL_VER(clXdrMarshallClEoSchedFeedBackT, 4, 0, 0)((void *) &schFeedback,
                    outMsgHandle, 0);
            EO_CHECK(CL_DEBUG_ERROR, ("Message Write Failed 0x%x\n", rc), rc);
        }
        else
            rc = CL_EO_RC(CL_ERR_NOT_IMPLEMENTED);

failure:
        return rc;
}

ClRcT clEoCustomActionRegister(void (*callback)(ClUint32T type, ClUint8T *data, ClUint32T len))
{
    ClEoCustomActionInfoT *info = NULL;
    if(!callback) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    info = clHeapCalloc(1, sizeof(*info));
    CL_ASSERT(info != NULL);
    info->callback = callback;
    clListAddTail(&info->list, &gClEoCustomActionList);
    return CL_OK;
}

ClRcT clEoCustomActionDeregister(void (*callback)(ClUint32T type, ClUint8T *data, ClUint32T len))
{
    ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gClEoCustomActionList)
    {
        ClEoCustomActionInfoT *info = CL_LIST_ENTRY(iter, ClEoCustomActionInfoT, list);
        if(info->callback == callback)
        {
            clListDel(&info->list);
            clHeapFree(info);
            return CL_OK;
        }
    }
    return CL_EO_RC(CL_ERR_NOT_EXIST);
}

void clEoCustomActionDeregisterAll(void)
{
    ClListHeadT *iter = NULL;
    ClEoCustomActionInfoT *info = NULL;
    while(!CL_LIST_HEAD_EMPTY(&gClEoCustomActionList))
    {
        iter = gClEoCustomActionList.pNext;
        info = CL_LIST_ENTRY(iter, ClEoCustomActionInfoT, list);
        clListDel(&info->list);
        clHeapFree(info);
    }
}

static void clEoCustomActionNotification(ClUint32T type, ClUint8T *data, ClUint32T len)
{
    ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gClEoCustomActionList)
    {
        ClEoCustomActionInfoT *info = CL_LIST_ENTRY(iter, ClEoCustomActionInfoT, list);
        info->callback(type, data, len);
    }
}

ClRcT clEoCustomActionTrigger(ClIocPhysicalAddressT dest, 
                              ClUint32T actType,
                              ClUint8T *actData,
                              ClUint32T actLen)
{
    ClIocAddressT destAddress;
    ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClUint32T fnId = CL_EO_CUSTOM_ACTION_INTIMATION_FN_ID;
    ClBufferHandleT inMsgHdl = 0;
    ClRcT rc = CL_OK;
    
    if(!dest.nodeAddress || !dest.portId || !actData || !actLen)
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    memset(&destAddress, 0, sizeof(destAddress));
    destAddress.iocPhyAddress.nodeAddress = dest.nodeAddress;
    destAddress.iocPhyAddress.portId = dest.portId;

    rc = clBufferCreate(&inMsgHdl);
    if(rc != CL_OK)
        return rc;

    rc = clXdrMarshallClUint32T((void*)&actType, inMsgHdl, 0);
    if(rc != CL_OK)
        goto out_free;
    rc = clXdrMarshallClUint32T((void*)&actLen, inMsgHdl, 0);
    if(rc != CL_OK)
        goto out_free;
    rc = clXdrMarshallArrayClUint8T((void*)actData, actLen, inMsgHdl, 0);
    if(rc != CL_OK)
        goto out_free;

    rc = clRmdWithMsg(destAddress, fnId, inMsgHdl, 0, CL_RMD_CALL_ASYNC,
                      &rmdOptions, NULL);

    out_free:
    clBufferDelete(&inMsgHdl);
    
    return rc;
}

static ClRcT clEoCustomActionIntimation(ClUint32T data, ClBufferHandleT inMsgHandle,
                                        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T actType = 0;
    ClUint32T actLen = 0;
    ClUint8T *actData = NULL;
    
    rc = clXdrUnmarshallClUint32T(inMsgHandle, &actType);
    if(rc != CL_OK)
        return rc;
    rc = clXdrUnmarshallClUint32T(inMsgHandle, &actLen);
    if(rc != CL_OK)
        return rc;
    rc = clXdrUnmarshallPtrClUint8T(inMsgHandle, (void**)&actData, actLen);
    if(rc != CL_OK)
        return rc;
    if(actData)
    {
        if(actLen > 0)
        {
            clEoCustomActionNotification(actType, actData, actLen);
        }
        clHeapFree(actData);
    }
    return rc;
}

static ClRcT clEoQueueMonitor(ClOsalTaskIdT tid, ClTimeT interval, ClTimeT threshold)
{
    ClEoCrashDeadlockT crash = {.reason = CL_EO_CRASH_DEADLOCK, 
                                .tid = tid, .interval = interval 
    };
    clLogNotice("EO", "MONITOR", "Task ID [%lld] still locked up for [%lld] usecs which exceeds "
                "configured threshold of [%lld] usecs",
                tid, interval, threshold);
    clEoSendCrashNotification((ClEoCrashNotificationT*)&crash);
    if (!clDbgNoKillComponents)  /* Goodbye unless we are debugging, in which case we would expect stuck threads! */
    {
        CL_ASSERT(0);
    }
    
    return CL_OK;
}

static ClRcT clEoPriorityQueuesInitialize(void)
{
    ClInt32T index = 0 ;
    ClRcT rc = CL_OK;
    ClTimerTimeOutT monitorThreshold = { .tsSec = CL_TASKPOOL_MONITOR_INTERVAL, .tsMilliSec = 0 };
    typedef struct ClEoPriorityQueueConfig
    {
        ClIocPriorityT priority;
        ClInt32T maxThreads;
    } ClEoPriorityQueueConfigT;
#ifndef VXWORKS_BUILD
    ClEoPriorityQueueConfigT priorityQueues[] = { 
        { 
            .priority = CL_IOC_DEFAULT_PRIORITY,
            .maxThreads = 8,
        },
        {
            .priority = CL_IOC_HIGH_PRIORITY,
            .maxThreads = 8,
        },
        {
            .priority = CL_IOC_LOW_PRIORITY,
            .maxThreads = 4,
        },
        {
            .priority = CL_IOC_ORDERED_PRIORITY,
            .maxThreads = 1,
        },
        {
            .priority = CL_IOC_NOTIFICATION_PRIORITY,
            .maxThreads = 1,
        },
        {
            .priority = CL_IOC_RESERVED_PRIORITY,
            .maxThreads = 2,
        },
    };
#else
    ClEoPriorityQueueConfigT priorityQueues[] = { 
        { 
            .priority = CL_IOC_DEFAULT_PRIORITY,
            .maxThreads = 4,
        },
        {
            .priority = CL_IOC_HIGH_PRIORITY,
            .maxThreads = 1,
        },
        {
            .priority = CL_IOC_ORDERED_PRIORITY,
            .maxThreads = 1,
        },
    };
    /*
    clEoConfig.needSerialization = CL_TRUE;
    clEoConfig.noOfThreads = 1;
    */
#endif

    if(clEoConfig.needSerialization == CL_TRUE)
    {
        for(index = 0; index < sizeof(priorityQueues)/sizeof(priorityQueues[0]); ++index)
        {
            if(priorityQueues[index].priority != CL_IOC_NOTIFICATION_PRIORITY
               &&
               priorityQueues[index].priority != CL_IOC_ORDERED_PRIORITY)
                priorityQueues[index].maxThreads = clEoConfig.noOfThreads;
        }
    }

    clTaskPoolInitialize();

    for(index = 0; index < sizeof(priorityQueues)/sizeof(priorityQueues[0]); ++index)
    {
        ClInt32T pri = priorityQueues[index].priority;
        ClInt32T maxThreads = priorityQueues[index].maxThreads;
        rc = clJobQueueInit(&gEoJobQueues[pri], 0, maxThreads);
        if(rc != CL_OK)
        {
            clLogError(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_CREATE,
                       "Queue create failed with [%#x] for priority [%d] with maxThreads [%d]", 
                       rc, pri, maxThreads);
            goto out_free;
        }
        clJobQueueMonitorStart(&gEoJobQueues[pri], monitorThreshold, clEoQueueMonitor);
        
    }
    
    rc = clJobQueueInit(&gClEoReplyJobQueue, 0, 1);
    if(rc != CL_OK)
        goto out_free;

    goto out;

    out_free:
    //    clEoQueueFinalize();
    
    out:
    return rc;
}

ClRcT clEoPriorityQueuesFinalize(ClBoolT force)
{
  ClInt32T i = 0;
  /*
   * Grab the job mutex to avoid parallelism with new eo jobs getting queued   
   */
  clOsalMutexLock(&gClEoJobMutex);

  for (i=0;i<CL_NUM_JOB_QUEUES;i++)
    clJobQueueDelete(&gEoJobQueues[i]);

  clJobQueueDelete(&gClEoReplyJobQueue);
  clOsalMutexUnlock(&gClEoJobMutex);
  return CL_OK;
}

static ClRcT clEoMemShrink(ClPoolShrinkOptionsT *pShrinkOptions)
{
    ClRcT rc;

    rc = clBufferShrink(pShrinkOptions);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error shrinking buffer pool.rc=0x%x\n",rc));
    }
    rc = clHeapShrink(pShrinkOptions);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error shrinking heap pool.rc=0x%x\n",rc));
    }
    return rc;
}

ClRcT clEoEnqueueReplyJob(ClEoExecutionObjT *pThis, ClCallbackT callback, ClPtrT invocation)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gClEoJobMutex);
    if(pThis->state != CL_EO_STATE_ACTIVE)
    {
        clOsalMutexUnlock(&gClEoJobMutex);
        return CL_EO_RC(CL_ERR_INVALID_STATE);
    }
    rc = clJobQueuePush(&gClEoReplyJobQueue, callback, invocation);
    clOsalMutexUnlock(&gClEoJobMutex);
    return rc;
}


ClRcT clEoEnqueueReassembleJob(ClBufferHandleT msg, ClIocRecvParamT *recvParam)
{
    ClIocPhysicalAddressT srcAddr = {0};
    ClUint8T protoType;
    ClEoExecutionObjT *pThis = NULL;
    ClEoProtoDefT *pdef = NULL;
    ClRcT rc = CL_OK;

    rc = clEoMyEoObjectGet(&pThis);
    if(rc != CL_OK || !pThis) return CL_EO_RC(CL_ERR_INVALID_STATE);

    srcAddr.nodeAddress = recvParam->srcAddr.iocPhyAddress.nodeAddress;
    srcAddr.portId = recvParam->srcAddr.iocPhyAddress.portId;
    protoType = recvParam->protoType;
    pdef = &gClEoProtoList[protoType];

    /*  
     * If we are in one of the states in which this protocol is active
     */
    if ((pThis->state & pdef->flags)) 
    {
        rc = CL_EO_ERR_ENQUEUE_MSG;
        if (pdef->nonblockingHandler) /* If a nonblocking handler function is defined, then call it */
        {
            rc = pdef->nonblockingHandler(pThis, msg, recvParam->priority,
                                          protoType, recvParam->length, srcAddr);
        }

        /** 
         * if its blocking or if the non-blocking call indicated 
         * that the message should be enqueued 
         */
        if (CL_GET_ERROR_CODE(rc) == CL_EO_ERR_ENQUEUE_MSG)
        {
            clEoEnqueueJob(msg, recvParam);
        }
    }
    return CL_OK;
}

static ClRcT clEoIocRecvQueueProcess(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;

    ClUint8T priority;
    ClUint8T protoType;
    ClUint32T length;
    ClIocPhysicalAddressT srcAddr;
    ClBufferHandleT eoRecvMsg = 0;
    ClUint32T portId;
    ClIocRecvOptionT recvOption = {0};
    ClIocRecvParamT recvParam = {0};
    ClPoolShrinkOptionsT shrinkOptions = { .shrinkFlags = CL_POOL_SHRINK_ALL };
    ClTimerTimeOutT timeOut = {.tsSec = 0,.tsMilliSec = CL_EO_MEM_TIMEOUT };
    ClInt32T maxTries = CL_EO_MEM_TRIES;
    ClInt32T tries = 0;
    ClOsalTaskIdT selfTaskId = 0;

    if (pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\n EO: Improper reference to EO Object \n"));
        return CL_EO_RC(CL_ERR_NULL_POINTER);
    }

    /*
     * Get commport ID to set the commport in the EO Area 
     */
    portId = (ClUint32T) pThis->eoPort;
    rc = clEoMyEoIocPortSet(pThis->eoPort);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: clEoMyEoIdSet failed \n"));
        return rc;
    }
    /*
     * Set the EO object reference in the EO Area 
     */
    rc = clEoMyEoObjectSet(pThis);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: clEoMyEoObjectSet failed \n"));
        return rc;
    }

    if (pThis->eoInitDone == 0)
    {
        rc = eoInit(pThis);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n EO: eoInit failed \n"));
            CL_FUNC_EXIT();
            return rc;
        }
        pThis->eoInitDone = 1;
    }

    recvOption.recvTimeout = 1000; //CL_IOC_TIMEOUT_FOREVER;

    clOsalSelfTaskIdGet(&selfTaskId);
    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV, "IOC Receive Thread is running with id [%llx]", selfTaskId);

    while (gpExecutionObject && (gpExecutionObject->state != CL_EO_STATE_STOP))
    {
        rc = clBufferCreate(&eoRecvMsg);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Create Message Failed for EOID : 0x%llx\t Port %x\n", pThis->eoID, pThis->eoPort));
            return rc;
        }

        /*
         * Block on this to recv message 
         */
        rc = clIocReceiveAsync(pThis->commObj, &recvOption, eoRecvMsg, &recvParam);
        priority = recvParam.priority;
        srcAddr.nodeAddress = recvParam.srcAddr.iocPhyAddress.nodeAddress;
        srcAddr.portId = recvParam.srcAddr.iocPhyAddress.portId;
        protoType = recvParam.protoType;
        length = recvParam.length;

        if (rc == CL_OK)
        {
            //clLogInfo(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV, "Packet Received. Priority %d, node %d port %d proto %d length %d",priority, srcAddr.nodeAddress,srcAddr.portId,protoType,length );
            /*
             *Reset maxtries and tries on success.
             */
            if(!maxTries)
            {
                maxTries = CL_EO_MEM_TRIES;
            }
            if(tries)
            {
                tries = 0;
            }
            ClEoProtoDefT *pdef = &gClEoProtoList[protoType];
            /*  
             * If we are in one of the states in which this protocol is active
             */
            if ((pThis->state & pdef->flags)) 
            {
                ClRcT rc = CL_EO_ERR_ENQUEUE_MSG;

                if (pdef->nonblockingHandler) /* If a nonblocking handler function is defined, then call it */
                {
                    rc = pdef->nonblockingHandler(pThis,eoRecvMsg,priority,protoType,length,srcAddr);
                }

                /** 
                 * if its blocking or if the non-blocking call indicated 
                 * that the message should be enqueued 
                 */
                if (CL_GET_ERROR_CODE(rc) == CL_EO_ERR_ENQUEUE_MSG)
                {
                    clEoEnqueueJob(eoRecvMsg, &recvParam);
                }                  
            }
            else
            {
                retCode = clBufferDelete(&eoRecvMsg);
            }

        }
        else if (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_RECV_UNBLOCKED)
            /** When ever commport need to be deleted this condtion will be occur */
        {
            retCode = clBufferDelete(&eoRecvMsg);
            if (retCode != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("EO: clBufferDelete Failed, rc=0x%x",
                                retCode));
            break;
        }
        else if(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_MEMORY)
        {
            if (++tries <= maxTries)
            {
                /*
                 * Sleep for memory to be released.
                 */
                clOsalTaskDelay(timeOut);
                goto retry;
            }
            /*
             * We are done with the tries I guess. 
             * Try a mem shrink as a last resort.
             */
            if(maxTries)
            {
                clEoMemShrink(&shrinkOptions);
                maxTries = 0;
                tries = 0;
                goto retry;
            }
            /*
             * We are done now. Giving up.
             */
            clBufferDelete(&eoRecvMsg);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nEO[%s] :clIocReceive out of Memory."\
                            "Exiting ioc receive thread\n",CL_EO_NAME));
            CL_ASSERT(0);
            break;
        }
        else if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {
            /* normal case is a periodic timeout to check that we should not be quitting.  just loop back around */
            clBufferDelete(&eoRecvMsg);
        }       
        else
        {
            retry:
            retCode = clBufferDelete(&eoRecvMsg);
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                           ("\n EO: clIocReceive failed EOID 0x%llx\t EO Port %x\n",
                            pThis->eoID, pThis->eoPort));
        }
    }        

    clLogDebug(CL_LOG_EO_AREA, CL_LOG_EO_CONTEXT_RECV, "IOC Receive Thread is exiting with id [%llx]", selfTaskId);
    return rc;
}

void clEoSendErrorNotify(ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    clEoEnqueueJob(message, pRecvParam);
}

ClRcT clEoJobHandler(ClEoJobT *pJob)
{
    ClRcT rc = CL_OK;
    ClEoSerializeT serialize = { 0 };
    ClIocRecvParamT *pRecvParam = NULL;
    ClEoExecutionObjT *pThis = gpExecutionObject;

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("clEoJobHandler"));

    if(pJob == NULL)
    {
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    if(pThis == NULL)
    {
        rc = CL_EO_RC(CL_ERR_INVALID_STATE);
        goto done;
    }

    pRecvParam = &pJob->msgParam;

    /*
     * _RMD_ uncomment following line 
     */
    if(gpClEoSerialize && (gClEoProtoList[pRecvParam->protoType].flags & CL_EO_STATE_THREAD_SAFE))
    {
        /**
         * Call the registered EO serialize method
         *
         */
        serialize.msg = pJob->msg;
        serialize.pMsgParam = pRecvParam;

        if((rc = gpClEoSerialize((ClPtrT)&serialize)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("EO Serialization " \
                                           "failed with [rc=0x%x]\n",rc));
            goto done;
        }
    }

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("clEoJobHandler, calling a registered message handler"));
    rc = gClEoProtoList[pRecvParam->protoType].func(pThis,
                                                    pJob->msg,
                                                    pRecvParam->priority,
                                                    pRecvParam->protoType,
                                                    pRecvParam->length,
                                                    pRecvParam->srcAddr.
                                                    iocPhyAddress);

    if(gpClEoDeSerialize && (gClEoProtoList[pRecvParam->protoType].flags & CL_EO_STATE_THREAD_SAFE))
    {
        /**
         * Call the deserialize EO thread safe method
         */
        (void)gpClEoDeSerialize((ClPtrT)&serialize);
    }
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Invoking Callback Failed, rc=0x%x\n", rc));
    }

 done:
    clHeapFree(pJob);
    return rc;
}


/*Push job into the thread pool*/
ClRcT clEoEnqueueJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    ClEoJobT *pJob = NULL;
    ClJobQueueT* pQueue = NULL; 
    ClUint32T priority ;

    if(recvMsg == CL_HANDLE_INVALID_VALUE
       ||
       !pRecvParam)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid param"));
        goto out;
    }

    rc = CL_EO_RC(CL_ERR_NO_MEMORY);
    pJob = clHeapCalloc(1, sizeof(*pJob));
    if(pJob == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Allocation error"));
        goto out;
    }
    pJob->msg = recvMsg;
    memcpy(&pJob->msgParam, pRecvParam, sizeof(pJob->msgParam));

    if(pJob->msgParam.priority > CL_IOC_MAX_PRIORITIES)
    {
        priority = CL_IOC_HIGH_PRIORITY;
    }
    else
    {
        priority = CL_EO_RECV_QUEUE_PRI(pJob->msgParam);
    
        if(priority >= CL_IOC_MAX_PRIORITIES)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid priority [%d]", priority));
            goto out_free;
        }
    }

    /*
     * Take the global job mutex. We would only take this in priqueue finalize
     * just to protect ourselves from a parallel priqueue finalize. Other instances
     * are safe since its anyway true that EO job queue is quiesced.
     */
    rc = CL_EO_RC(CL_ERR_INVALID_STATE);
    clOsalMutexLock(&gClEoJobMutex);
    if(!gpExecutionObject 
       || 
       gpExecutionObject->state != CL_EO_STATE_ACTIVE)
    {
        /*
         * The EO is being stopped/terminated. Back out
         */
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Invalid state."));
        clOsalMutexUnlock(&gClEoJobMutex);
        goto out_free;
    }

    if(!gpExecutionObject->threadRunning)
    {
        /*
         * Allow notification packets which could unblock any threads blocked
         * on rmds. to unreachable destinations.
         */
        if(!gpExecutionObject->refCnt
           ||
           pJob->msgParam.protoType != CL_IOC_PORT_NOTIFICATION_PROTO)
        {
            clOsalMutexUnlock(&gClEoJobMutex);
            goto out_free;
        }
    }

    rc = CL_EO_RC(CL_ERR_NOT_EXIST);
    pQueue = &gEoJobQueues[priority];
    if(pQueue == NULL)
    {
        clOsalMutexUnlock(&gClEoJobMutex);
        goto out_free;
    }

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Enqueuing job priority %d",priority));
  
    rc = clJobQueuePush(pQueue,(ClCallbackT) clEoJobHandler, pJob);

    clOsalMutexUnlock(&gClEoJobMutex);

    if(rc != CL_OK)
    {
        goto out_free;
    }

    goto out;

    out_free:
    clHeapFree(pJob);
    out:
    return rc;
}

void clEoQueuesQuiesce(void)
{
    ClInt32T i;
    for(i = 0; i < CL_NUM_JOB_QUEUES; ++i)
    {
        clJobQueueQuiesce(&gEoJobQueues[i]);
    }
}

static ClRcT eoQueueStatsCallback(ClCallbackT cb, ClPtrT data, ClPtrT pArg)
{
    ClEoQueueDetailsT *pEoQueueDetails = pArg;
    /*
     * Handle eo jobqueue requests.
     */
    if(cb == (ClCallbackT)clEoJobHandler && data && pArg)
    {
        ClEoJobT *job = data;
        ClIocRecvParamT *recvParam = &job->msgParam;
        ClEoQueueProtoUsageT *protoUsage = NULL;
        ClEoQueuePriorityUsageT *priorityUsage = NULL;
        ClInt32T i;
        /*
         * Get queue proto and priority reference if present.
         */
        for(i = 0; i < pEoQueueDetails->numProtos; ++i)
        {
            if(pEoQueueDetails->protos[i].proto == recvParam->protoType)
            {
                protoUsage = pEoQueueDetails->protos+i;
                break;
            }
        }
        for(i = 0; i < pEoQueueDetails->numPriorities; ++i)
        {
            if(pEoQueueDetails->priorities[i].priority == recvParam->priority)
            {
                priorityUsage = pEoQueueDetails->priorities + i;
                break;
            }
        }
        if(!protoUsage)
        {
            pEoQueueDetails->protos = clHeapRealloc(pEoQueueDetails->protos,
                                                    (pEoQueueDetails->numProtos+1)*sizeof(*pEoQueueDetails->protos));
            CL_ASSERT(pEoQueueDetails->protos != NULL);
            memset(pEoQueueDetails->protos+pEoQueueDetails->numProtos, 0, sizeof(*pEoQueueDetails->protos));
            protoUsage = pEoQueueDetails->protos+pEoQueueDetails->numProtos;
            ++pEoQueueDetails->numProtos;
            protoUsage->proto = recvParam->protoType;
        }
        if(!priorityUsage)
        {
            pEoQueueDetails->priorities = clHeapRealloc(pEoQueueDetails->priorities,
                                                        (pEoQueueDetails->numPriorities+1)*sizeof(*pEoQueueDetails->priorities));
            CL_ASSERT(pEoQueueDetails->priorities != NULL);
            memset(pEoQueueDetails->priorities+pEoQueueDetails->numPriorities, 0, sizeof(*pEoQueueDetails->priorities));
            priorityUsage = pEoQueueDetails->priorities + pEoQueueDetails->numPriorities;
            ++pEoQueueDetails->numPriorities;
            priorityUsage->priority = recvParam->priority;
        }
        protoUsage->bytes += recvParam->length;
        ++protoUsage->numMsgs;
        priorityUsage->bytes += recvParam->length;
        ++priorityUsage->numMsgs;
    }
    return CL_OK;
}

static ClCharT *eoProtoNameGet(ClUint8T proto)
{
    ClInt32T i;
    for(i = 0; i < sizeof(protos)/sizeof(protos[0]); ++i)
    {
        if(protos[i].protoID == proto)
            return protos[i].name;
    }
    return "Unknown";
}

/*
 * Get the stats of the EO job queues.
 */
ClRcT clEoQueueStatsGet(ClEoQueueDetailsT *pEoQueueDetails)
{
    register ClInt32T i;
    ClJobQueueT *pJobQueue;
    ClEoQueueDetailsT eoQueueDetails = {0};
    ClBoolT dumpDetails = CL_FALSE;
    ClRcT rc = CL_OK;

    if(!pEoQueueDetails)
    {
        pEoQueueDetails = &eoQueueDetails;
        dumpDetails = CL_TRUE;
    }

    clOsalMutexLock(&gClEoJobMutex);
    if(!gpExecutionObject 
       || 
       gpExecutionObject->state != CL_EO_STATE_ACTIVE)
    {
        /*
         * The EO is being stopped/terminated. Back out
         */
        rc = CL_EO_RC(CL_ERR_INVALID_STATE);
        goto out_unlock;
    }
    for(i = 0; i < sizeof(gEoJobQueues)/sizeof(gEoJobQueues[0]); ++i)
    {
        pJobQueue = &gEoJobQueues[i];
        if(pJobQueue->flags && pJobQueue->queue)
        {
            ClJobQueueUsageT jobQueueDetails = {0};
            rc = clJobQueueStatsGet(pJobQueue, eoQueueStatsCallback, pEoQueueDetails, &jobQueueDetails);
            if(rc == CL_OK)
            {
                /*
                 * Store the entry in the job queue priority array.
                 */
                register ClInt32T j;
                for(j = 0; j < pEoQueueDetails->numPriorities; ++j)
                {
                    if(pEoQueueDetails->priorities[j].priority == i)
                    {
                        memcpy(&pEoQueueDetails->priorities[j].taskPoolUsage, &jobQueueDetails.taskPoolUsage,
                               sizeof(pEoQueueDetails->priorities[j].taskPoolUsage));
                    }
                }
            }
        }
    }

    if(dumpDetails)
    {
        for(i = 0; i < pEoQueueDetails->numProtos; ++i)
        {
            clLogNotice("EO", "STATS", "EO proto [%d], name [%s], msgs [%d], bytes to be processed [%lld]",
                        pEoQueueDetails->protos[i].proto, eoProtoNameGet(pEoQueueDetails->protos[i].proto),
                        pEoQueueDetails->protos[i].numMsgs, pEoQueueDetails->protos[i].bytes
                        );
        }
        for(i = 0; i < pEoQueueDetails->numPriorities; ++i)
        {
            clLogNotice("EO", "STATS", "EO priority [%d], msgs [%d], bytes to be processed [%lld]",
                        pEoQueueDetails->priorities[i].priority, pEoQueueDetails->priorities[i].numMsgs,
                        pEoQueueDetails->priorities[i].bytes);
            clLogNotice("EO", "STATS", "Thread pool priority [%d], threads [%d], idle [%d], active [%d], "
                        "max [%d], pending msgs [%d]", pEoQueueDetails->priorities[i].priority, 
                        pEoQueueDetails->priorities[i].taskPoolUsage.numTasks,
                        pEoQueueDetails->priorities[i].taskPoolUsage.numIdleTasks,
                        pEoQueueDetails->priorities[i].taskPoolUsage.numTasks - 
                        pEoQueueDetails->priorities[i].taskPoolUsage.numIdleTasks,
                        pEoQueueDetails->priorities[i].taskPoolUsage.maxTasks,
                        pEoQueueDetails->priorities[i].numMsgs);
        }
        if(pEoQueueDetails->protos) clHeapFree(pEoQueueDetails->protos);
        if(pEoQueueDetails->priorities) clHeapFree(pEoQueueDetails->priorities);
        pEoQueueDetails->protos = NULL;
        pEoQueueDetails->priorities = NULL;
    }

    rc = CL_OK;

    out_unlock:
    clOsalMutexUnlock(&gClEoJobMutex);
    
    return rc;
}

static ClRcT eoQueueAmfMsgCallback(ClCallbackT cb, ClPtrT data, ClPtrT arg)
{
    /*
     * Handle eo jobqueue requests.
     */
    if(cb == (ClCallbackT)clEoJobHandler && data && arg)
    {
        ClUint32T pri = *(ClUint32T*)arg;
        ClEoJobT *job = data;
        ClIocRecvParamT *recvParam = &job->msgParam;
        if(pri == recvParam->priority)
        {
            /*
             * Msg found in the queue; break walk
             */
            return CL_ERR_INUSE;
        }
    }
    return CL_OK;
}


/*
 * Find the message in the amf queue.
 */
ClBoolT clEoQueueAmfResponseFind(ClUint32T pri)
{
    ClJobQueueT *pJobQueue;
    ClBoolT status = CL_FALSE;
    ClRcT rc = CL_OK;

    clOsalMutexLock(&gClEoJobMutex);
    if(!gpExecutionObject 
       || 
       gpExecutionObject->state != CL_EO_STATE_ACTIVE)
    {
        /*
         * The EO is being stopped/terminated. Back out
         */
        goto out_unlock;
    }
    pJobQueue = &gEoJobQueues[CL_IOC_HIGH_PRIORITY];
    rc = clJobQueueWalk(pJobQueue, eoQueueAmfMsgCallback, &pri);
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE)
    {
        status = CL_TRUE;
    }

    out_unlock:
    clOsalMutexUnlock(&gClEoJobMutex);

    return status;
}

