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

/**
 * This file implements initialization, finalization, healthcheck and
 * EO interaction related functionality in CPM. This file is the entry
 * point of CPM.
 */

/*
 * Standard header files 
 */
#define __SERVER__
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef __linux__
#include <linux/version.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clIocErrors.h>
#include <clCpmApi.h>
#include <clEoApi.h>
#include <clEoIpi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clDbalCfg.h>
#include <clEventApi.h>
#include <clIocIpi.h>
#include <clIocApiExt.h>
#include <clHandleApi.h>
#include <clDbg.h>
#include <clLeakyBucket.h>
#include <clJobQueue.h>
/*
 * ASP Internal header files
 */
#include <clEoIpi.h>
#include <clRmdIpi.h>

/*
 * CPM internal header files 
 */
#include <clCpmInternal.h>
#include <clCpmClient.h>
#include <clCpmLog.h>
#include <clCpmCkpt.h>
#include <clCpmCor.h>
#include <clCpmAms.h>
#include <clCpmIpi.h>
#include <clTimeServer.h>
#include <clAmfPluginApi.h>
#include <clNodeCache.h>
/*
 * XDR header files 
 */
#include "xdrClCpmBootOperationT.h"
#include "xdrClEoStateT.h"
#include "xdrClEoSchedFeedBackT.h"
#include "xdrClCpmEventPayLoadT.h"
#include "xdrClCpmEventNodePayLoadT.h"
#include "xdrClCpmClientInfoIDLT.h"
#include "xdrClEoExecutionObjIDLT.h"
#define CPM_LOG_AREA_LOGGER	"LOG"
#define CPM_LOG_CTX_LOGGER_INI	"INI"
#ifdef CL_CPM_AMS
#include <clAms.h>
extern ClAmsT gAms;
#endif

#define CPM_ASP_WELCOME_MSG "Welcome to OpenClovis SAFplus Availability/Scalability Platform! Starting up version"
#define CPM_NODECLEANUP_SCRIPT "node_cleanup.sh"
#ifndef WIFCONTINUED
#define WIFCONTINUED(status) (0)
#endif

#ifdef SOLARIS_BUILD
#define WAIT_ANY ((pid_t)-1)
#endif

#ifdef POSIX_BUILD
ClCharT **gEnvp;
#ifdef VXWORKS_BUILD
pthread_mutex_t mutexMain = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condMain = PTHREAD_COND_INITIALIZER;
#endif
#endif

#define CL_CPM_PIPE_MSGS (512)
#define CL_CPM_PIPE_MSG_SIZE (512)
#define CPM_VALGRIND_DEFAULT_DELAY (0x5)

/**
 * Global variables.
 */
ClUint32T clEoWithOutCpm;

/*
 * Stores the user provided command line options for the boot profile.
 */
ClCharT clCpmBootProfile[CL_MAX_NAME_LENGTH];
ClCharT clCpmNodeName[CL_MAX_NAME_LENGTH];

ClCpmT gClCpm;

ClTaskPoolHandleT gCpmFaultPool;

static ClCharT gAspNodeIp[CL_MAX_NAME_LENGTH];
static ClCharT gAspVersion[80];
ClCharT gAspInstallInfo[128]; /*install key info. for ARD*/
ClBoolT gClAmsSwitchoverInline;
ClBoolT gClAmsPayloadResetDisable;

/**
 * Static variables.
 */
/*
 * Should CPM run in foreground ?
 */
static ClBoolT cpmIsForeground = CL_FALSE;

/*
 * Current working directory of CPM.
 */
static ClCharT cpmCwd[CL_MAX_NAME_LENGTH] = {0};

/*
 * Was the command etc/init.d/asp consolestart ?
 */
static ClBoolT cpmIsConsoleStart = CL_FALSE;

static ClUint32T myCh = 0;
static ClUint32T mySl = 0;

#if 0 /* Stone: cpm logging disabled -- logs use safplus_logd */
static ClUint64T cpmLoggerFileSize = 50*1024*1024;
static ClUint32T cpmLoggerFileRotations = 0x5;
static ClInt32T cpmLoggerPipe[2];
static ClSizeT cpmLoggerTotalBytes;
static ClOsalMutexT cpmLoggerMutex;
#endif

static ClFdT cpmLoggerFd;
static ClCharT cpmLoggerFile[CL_MAX_NAME_LENGTH];


static ClJobQueueT cpmNotificationQueue;

static ClInt32T cpmValgrindTimeout;

static ClBoolT gClSigTermPending;

static ClBoolT gClSigTermRestart;

static ClRcT clCpmIocNotificationEnqueue(ClIocNotificationT *notification, ClPtrT cookie);
static ClBoolT __cpmIsInfrastructureComponent(const ClCharT *compName);

#undef __CLIENT__
#include "clCpmServerFuncTable.h"

static ClRcT compMgrPollThread(void);

static ClRcT cpmMain(ClInt32T argc, ClCharT *argv[]);

static ClRcT clCpmIocNotification(ClEoExecutionObjT *pThis, ClBufferHandleT eoRecvMsg, ClUint8T priority, ClUint8T protoType,
                                  ClUint32T length, ClIocPhysicalAddressT srcAddr);

extern ClIocNodeAddressT gIocLocalBladeAddress;
static ClIocAddressT allNodeReps;
static ClIocLogicalAddressT allLocalComps;

/*
 * bypassed the function into extension plugin
 */
void clAmfPluginNotificationCallback(ClIocNodeAddressT node, ClNodeStatus status)
{
    /*
     * All node in cluster
     */
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = CL_IOC_XPORT_PORT;

    /*
     * All local components
     */
    allLocalComps = CL_IOC_ADDRESS_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress, CL_IOC_BROADCAST_ADDRESS);
    clLogDebug("PLG","EVT","%s node: [%d] status [%d]\n", __FUNCTION__, node, status);

    switch (status)
    {
        case ClNodeStatusUp:
        {
                clIocNotificationNodeStatusSend(gpClCpm->cpmEoObj->commObj, CL_IOC_NODE_ARRIVAL_NOTIFICATION, node,
                                (ClIocAddressT*) &allLocalComps, (ClIocAddressT*) &allNodeReps, NULL );
                break;
        }

        case ClNodeStatusDown:
        {
            clIocNotificationNodeStatusSend(gpClCpm->cpmEoObj->commObj, CL_IOC_NODE_LEAVE_NOTIFICATION, node,
                            (ClIocAddressT*) &allLocalComps, (ClIocAddressT*) &allNodeReps, NULL );
            break;
        }
        default:
        {
            /* GAS */
            break;
        }
    }
}

/*
 * FIXME: added to get around unresolved symbol errors for clEoConfig and
 * clEoBasicLibs when building/using shared libraries 
 */
extern void clEoCleanup(ClEoExecutionObjT* pThis);

/*
 * Initializes the EM Library and create necessary channels/events for 
 *  publishing events by the CPM.
 */
ClRcT cpmEventInitialize(void)
{
    ClRcT rc = CL_OK;
    SaNameT cpmChannelName = { 0 };
    SaNameT cpmNodeChannelName = { 0 };
    /*
     * Populate the channel name 
     */
    strcpy((ClCharT *)cpmChannelName.value, CL_CPM_EVENT_CHANNEL_NAME);
    cpmChannelName.length = strlen((const ClCharT *)cpmChannelName.value);

    strcpy((ClCharT *)cpmNodeChannelName.value, CL_CPM_NODE_EVENT_CHANNEL_NAME);
    cpmNodeChannelName.length = strlen((const ClCharT *)cpmNodeChannelName.value);

    /*
     * initialization of EM variables 
     */
    gpClCpm->cpmEvtHandle = CL_HANDLE_INVALID_VALUE;
    gpClCpm->cpmEvtCallbacks.clEvtChannelOpenCallback = NULL;
    gpClCpm->cpmEvtCallbacks.clEvtEventDeliverCallback = NULL;
    gpClCpm->version.releaseCode = 'B';
    gpClCpm->version.majorVersion = 0x1;
    gpClCpm->version.minorVersion = 0x1;
    gpClCpm->cpmEvtChannelHandle = CL_HANDLE_INVALID_VALUE;
    gpClCpm->cpmEventHandle = CL_HANDLE_INVALID_VALUE;
    gpClCpm->cpmEvtId = 0;

    /*
     * Initialize the EM library 
     */
    rc = clEventInitialize(&gpClCpm->cpmEvtHandle, &gpClCpm->cpmEvtCallbacks,
                           &gpClCpm->version);
    if (rc != CL_EVENT_ERR_ALREADY_INITIALIZED)
        CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED,
                       "EVENT", rc, rc, CL_LOG_HANDLE_APP);

    /*
     * Open an event channel for component failure publish 
     */
    rc = clEventChannelOpen(gpClCpm->cpmEvtHandle, &cpmChannelName,
                            CL_EVENT_CHANNEL_PUBLISHER |
                            CL_EVENT_GLOBAL_CHANNEL, (ClTimeT) -1,
                            &gpClCpm->cpmEvtChannelHandle);
    if (rc != CL_EVENT_ERR_EXIST)
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EVT_CHANNEL_OPEN_ERR, rc,
                       rc, CL_LOG_HANDLE_APP);

    /* Andrew Stone: the handle as an arg of clEventAllocate is 0, but has a
       value in the var, so I suspect a race condition. But ChannelOpen is
       supposed to by synchronous so I am confused.  Sleeping like this is not
       pretty or course.
    */
    while (gpClCpm->cpmEvtChannelHandle == CL_HANDLE_INVALID_VALUE)
      {
        usleep(20000);
      }

    /*
     * Create event for component failure publish 
     */
    rc = clEventAllocate(gpClCpm->cpmEvtChannelHandle,
                         &gpClCpm->cpmEventHandle);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EVT_CHANNEL_ALLOC_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);


    /*
     * Open an event channel for Node Arrival and Departure event publish 
     */
    rc = clEventChannelOpen(gpClCpm->cpmEvtHandle, &cpmNodeChannelName,
                            CL_EVENT_CHANNEL_PUBLISHER |
                            CL_EVENT_GLOBAL_CHANNEL, (ClTimeT) -1,
                            &gpClCpm->cpmEvtNodeChannelHandle);
    if (rc != CL_EVENT_ERR_EXIST)
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EVT_CHANNEL_OPEN_ERR, rc,
                       rc, CL_LOG_HANDLE_APP);

    /*
     * Create event for Node Arrival and Departure event publish 
     */
    rc = clEventAllocate(gpClCpm->cpmEvtNodeChannelHandle,

                         &gpClCpm->cpmEventNodeHandle);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EVT_CHANNEL_ALLOC_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    return CL_OK;

  failure:
    /*
     * TODO: Cleanup 
     */
    return rc;
}

/*
 *  Publish the node Arrival/Departure event based on the passe operation field 
 */
ClRcT nodeArrivalDeparturePublish(ClIocNodeAddressT iocAddress,
                                  // coverity[pass_by_value]
                                  SaNameT nodeName,
                                  ClCpmNodeEventT operation)
{
    ClCpmEventNodePayLoadT payLoad = {{0}};
    ClRcT rc = CL_OK;
    ClBufferHandleT payLoadMsg = 0;
    ClUint8T *payLoadBuffer = NULL;
    ClUint32T msgLength = 0;
    ClUint32T pattern;
    ClEventPatternT nodeEvtPattern[] =  { {0, (ClSizeT)sizeof(pattern), (ClUint8T*)&pattern} };
    ClEventPatternArrayT nodeEvtPatternArray = { 0,
                                                 sizeof(nodeEvtPattern)/sizeof(nodeEvtPattern[0]),
                                                 nodeEvtPattern };

    if(!gpClCpm->emUp)
        goto out;

    /*
     * fill in the payload data 
     */
    memcpy(&(payLoad.nodeName), &(nodeName), sizeof(SaNameT));
    payLoad.nodeIocAddress = iocAddress;
    payLoad.operation = operation;

    /*
     * Log, what we are doing here 
     */
    switch(operation)
    {
        case CL_CPM_NODE_ARRIVAL:
            pattern = htonl(CL_CPM_NODE_ARRIVAL_PATTERN);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ALERT, NULL,
                    CL_CPM_LOG_1_EVT_PUB_NODE_ARRIVAL_INFO, nodeName.value);
            break;
        case CL_CPM_NODE_DEATH:
            pattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ALERT, NULL,
                    CL_CPM_LOG_1_EVT_PUB_NODE_DEPART_INFO, nodeName.value);
            break;
        case CL_CPM_NODE_DEPARTURE:
            pattern = htonl(CL_CPM_NODE_DEPART_PATTERN);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ALERT, NULL,
                    CL_CPM_LOG_1_EVT_PUB_NODE_DEPART_INFO, nodeName.value);
            break;
        default:
            CL_ASSERT(0);
    }

    /*
     * Marshall the payload data.
     */
    rc = clBufferCreate(&payLoadMsg);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_CREATE_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    rc = VDECL_VER(clXdrMarshallClCpmEventNodePayLoadT, 4, 0, 0)((void *) &payLoad, payLoadMsg, 0);
    CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_HANDLE_APP);

    rc = clBufferLengthGet(payLoadMsg, &msgLength);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_LENGTH_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    rc = clBufferFlatten(payLoadMsg, &payLoadBuffer);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_FLATTEN_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    rc = clEventAttributesSet(gpClCpm->cpmEventNodeHandle,
            &nodeEvtPatternArray,
            CL_EVENT_HIGHEST_PRIORITY,
            0,
            &gpClCpm->name);
    if(rc != CL_OK)
    {
        clLogError("NODE", "EVENT", "Attribute set returned [%#x]", rc);
        goto failure;
    }

    /*
     * Publish the event 
     */
    rc = clEventPublish(gpClCpm->cpmEventNodeHandle,
            (const void *) payLoadBuffer,
            msgLength,
            &gpClCpm->cpmNodeEvtId);
    if (rc != CL_OK)
    {
        if (operation == CL_CPM_NODE_ARRIVAL)
        {
            CL_CPM_CHECK_2(CL_LOG_SEV_ERROR,
                    CL_CPM_LOG_2_EVT_PUB_NODE_ARRIVAL_ERR,
                    nodeName.value, rc, rc, 
                    CL_LOG_HANDLE_APP);
        }
        else
        {
            CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_EVT_PUB_NODE_DEPART_ERR,
                    nodeName.value, rc, rc, 
                    CL_LOG_HANDLE_APP);
        }
    }

failure:
    if(payLoadBuffer)
    {
        clHeapFree(payLoadBuffer);
        payLoadBuffer = NULL;
    }
    if(payLoadMsg)
        clBufferDelete(&payLoadMsg);

out:
    return rc;
}


static void cpmDeAllocate(void)
{
    if (gpClCpm->pCpmConfig != NULL)
    {
        clHeapFree(gpClCpm->pCpmConfig);
        gpClCpm->pCpmConfig = NULL;
    }
    if (gpClCpm->pCpmLocalInfo != NULL)
    {
        clHeapFree(gpClCpm->pCpmLocalInfo);
        gpClCpm->pCpmLocalInfo = NULL;
    }

    clOsalMutexDelete(gpClCpm->eoListMutex);
    if (gpClCpm->nodeMoId)
    {
#ifdef USE_COR        
        clCorMoIdFree(gpClCpm->nodeMoId);
#endif        
        gpClCpm->nodeMoId = NULL;
    }
    if (gpClCpm->invocationTable)
        clCntDelete(gpClCpm->invocationTable);

}

ClInt32T cpmInvocationStoreCompare(ClCntKeyHandleT key1,
                                   ClCntKeyHandleT key2)
{
    ClInvocationT *pGivenInvocation = (ClInvocationT*)key1;
    ClInvocationT *pInvocation =  (ClInvocationT*)key2;
    CL_ASSERT(pInvocation && pGivenInvocation);
    if(*pGivenInvocation > *pInvocation)
        return 1;
    else if(*pGivenInvocation < *pInvocation)
        return -1;
    return 0;
}

ClUint32T cpmInvocationStoreHashFunc(ClCntKeyHandleT key)
{
    ClInvocationT *pInvocation = (ClInvocationT*)key;
    ClUint32T invocationKey = 0;
    CL_ASSERT(pInvocation != NULL);
    CL_CPM_INVOCATION_KEY_GET(*pInvocation, invocationKey);
    return (invocationKey % CL_CPM_INVOCATION_BUCKET_SIZE);
}

void cpmInvocationDelete(ClCntKeyHandleT userKey,
                         ClCntDataHandleT userData)
{
    clHeapFree((ClCpmInvocationT *) userData);
}

/*
 * Allocate and intialize the gpClCpm with the default values 
 */
static ClRcT cpmAllocate(void)
{
    ClRcT rc = CL_OK;
    ClCharT *str = NULL;
    gpClCpm = &gClCpm;

    gpClCpm->bootTime = clOsalStopWatchTimeGet();
    
    gpClCpm->pCpmLocalInfo = (ClCpmLocalInfoT *) clHeapCalloc(1,(ClUint32T) sizeof(ClCpmLocalInfoT));
    if (gpClCpm->pCpmLocalInfo == NULL)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED, CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_HANDLE_APP);

    gpClCpm->pCpmConfig = (ClCpmCfgT *) clHeapCalloc(1,(ClUint32T) sizeof(ClCpmCfgT));
    if (gpClCpm->pCpmConfig == NULL)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED, CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_HANDLE_APP);

    rc = clOsalMutexInit(&gpClCpm->cpmMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexInit(&gpClCpm->heartbeatMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalCondInit(&gpClCpm->heartbeatCond);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_COND_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    /*
     * Disable heartbeating by default.
     */
    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    gpClCpm->enableHeartbeat = CL_FALSE;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);

    rc = clOsalMutexInit(&gpClCpm->cpmGmsMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);
    
    rc = clOsalCondInit(&gpClCpm->cpmGmsCondVar);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_COND_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexCreate(&(gpClCpm->eoListMutex));
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexCreate(&(gpClCpm->cpmTableMutex));
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);
    /*
     * CompId needs to be unique in the system/cluster. But at the same time 
     * compId are allocated/generated locally. Hence to generate the globally 
     * unique compId, 32 bits are devided in two parts
     * First 16 Bits is designated for slot number
     * Next 16 bits are initialized by zero, when the parser reads the 
     * component, it just keep incrementing the compIdAlloc
     */
    CL_CPM_IOC_ADDRESS_SLOT_GET(ASP_NODEADDR, gpClCpm->compIdAlloc);
    gpClCpm->compIdAlloc = (gpClCpm->compIdAlloc << CL_CPM_IOC_SLOT_BITS);

    /*
     * Create and Initialize the Invocation Hash Table and Mutex. This is used
     * to store the pending response from the component in response to the
     * callback invoked by CPM 
     */
    rc = clCntHashtblCreate(CL_CPM_INVOCATION_BUCKET_SIZE, cpmInvocationStoreCompare, cpmInvocationStoreHashFunc, NULL,
                            cpmInvocationDelete, CL_CNT_UNIQUE_KEY, &gpClCpm->invocationTable);

    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexCreate(&(gpClCpm->invocationMutex));
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexInit(&gpClCpm->clusterMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexInit(&gpClCpm->compTerminateMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    rc = clOsalMutexInit(&gpClCpm->cpmShutdownMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    clOsalMutexLock(&gpClCpm->cpmGmsMutex); 
    gpClCpm->trackCallbackInProgress = CL_FALSE;
    clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);

    /*
     * Mark if we are running under valgrind.
     */
    if( (str = clParseEnvStr("ASP_VALGRIND_CMD") ) )
    {
        cpmValgrindTimeout = CPM_VALGRIND_DEFAULT_DELAY;
        if( (str = clParseEnvStr("ASP_VALGRIND_DELAY") ) )

        {
            cpmValgrindTimeout = (ClInt32T)strtoul(str, &str, 10);
            if(*str || cpmValgrindTimeout < 0) /*invalid delay*/
                cpmValgrindTimeout = CPM_VALGRIND_DEFAULT_DELAY; 
        }
        clLogNotice("VALGRIND", "DELAY", "DELAY configured is [%d] secs", cpmValgrindTimeout);
    }
    
    return CL_OK;

    failure:
    cpmDeAllocate();
    return rc;
}

static void cpmSigintHandler(ClInt32T signum)
{
    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                  "Caught SIGINT or SIGTERM signal, shutting down the node...");
    gClSigTermPending = (signum == SIGTERM);
    clCpmNodeShutDown(clIocLocalAddressGet());
}

static void cpmSigchldHandler(ClInt32T signum)
{
    ClInt32T pid;
    ClInt32T w;

    /* 
     * Wait for all childs to finish.
     */
    do {
        pid = waitpid(WAIT_ANY, &w, WNOHANG);
    } while(pid > 0);
}

/*
 * Signal handler to :
 *
 * 1. Collect the dead child process.
 * 2. Catch the Control-C on the terminal.
 */
static void cpmSigHandlerInstall(void)
{
    struct sigaction newAction;

    newAction.sa_handler = cpmSigintHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_RESTART;

    if (-1 == sigaction(SIGINT, &newAction, NULL))
    {
        perror("sigaction for SIGINT failed");
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Unable to install signal handler for SIGINT");
    }

    if (-1 == sigaction(SIGTERM, &newAction, NULL))
    {
        perror("sigaction for SIGTERM failed");
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Unable to install signal handler for SIGTERM");
    }

    newAction.sa_handler = cpmSigchldHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_RESTART;

    if (-1 == sigaction(SIGCHLD, &newAction, NULL))
    {
        perror("sigaction for SIGCHLD failed");
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Unable to install signal handler for SIGCHLD");
    }

    return;
}

static ClRcT cpmHealthCheck(ClEoSchedFeedBackT *schFeedback)
{
    schFeedback->freq = CL_EO_DEFAULT_POLL;
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;
}

ClBoolT cpmCompIsChildAlive(ClCpmComponentT *comp)
{
    ClInt32T pid = 0;
    ClInt32T status = 0;
    ClInt32T flags = WNOHANG | WUNTRACED;

#ifdef __linux__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 10)
    flags |= WCONTINUED;
#endif
#endif

    do {
        pid = waitpid(comp->processId, &status, flags);
    } while ((-1 == pid) && (EINTR == errno));
    
    if ( (ClInt32T) comp->processId == pid)
    {
        if (WIFEXITED(status)) goto no;
        else if (WIFSTOPPED(status)) 
        {
            kill(pid, SIGCONT);
            goto yes;
        }
        else if (WIFCONTINUED(status)) goto yes;
    }
    else if (0 == pid) goto yes;
    else if (-1 == pid)
    {
        goto no;
    }

yes:
    return CL_TRUE;
no:
    return CL_FALSE;
}

ClRcT cpmStopCompOps(ClCntNodeHandleT key,
                     ClCntDataHandleT data,
                     ClCntArgHandleT arg,
                     ClUint32T size)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)data;

    SaNameT compName = {0};

    strcpy((ClCharT *)compName.value, comp->compConfig->compName);
    compName.length = strlen((const ClCharT *)compName.value);

    cpmInvocationClearCompInvocation(&compName);

    if (IS_ASP_COMP(comp))
    {
        clTimerStop(comp->cpmInstantiateTimerHandle);
        clTimerStop(comp->cpmTerminateTimerHandle);
    }
    
    return CL_OK;
}

void cpmStopAllCompOps(void)
{
    clCntWalk(gpClCpm->compTable, cpmStopCompOps, NULL, 0);
}

void cpmClearOps(void)
{
    cpmStopAllCompOps();

    if (CL_HANDLE_INVALID_VALUE != gpClCpm->gmsTimerHandle)
    {
        clTimerStop(gpClCpm->gmsTimerHandle);
    }
}

static ClRcT cpmKillUserComponent(ClCntNodeHandleT key,
                                  ClCntDataHandleT data,
                                  ClCntArgHandleT arg,
                                  ClUint32T size)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)data;
    
    if (comp->processId && !__cpmIsInfrastructureComponent(comp->compConfig->compName)
        &&
        cpmCompIsChildAlive(comp))
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Cleaning up user component [%s] with process id [%d] ",
                   comp->compConfig->compName,
                   comp->processId);
        cpmCompCleanup(comp->compConfig->compName);
    }
    
    return CL_OK;
}

static ClRcT cpmKillComponent(ClCntNodeHandleT key,
                              ClCntDataHandleT data,
                              ClCntArgHandleT arg,
                              ClUint32T size)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)data;
    
    if (comp->processId && cpmCompIsChildAlive(comp))
    {
        if (clDbgNoKillComponents)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Found component [%s] with process id [%d] "
                       "Not terminating due to debugging override" 
                       "'clDbgNoKillComponents=1' specified in clDbg.c",
                       comp->compConfig->compName,
                       comp->processId);
        }
        else
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Terminating component [%s] with process id [%d] ",
                       comp->compConfig->compName,
                       comp->processId);

            kill(comp->processId, SIGKILL);
        }
    }
    
    return CL_OK;
}

void cpmKillAllComponents(void)
{
    clCntWalk(gpClCpm->compTable, cpmKillComponent, NULL, 0);
}

void cpmKillAllUserComponents(void)
{
    clCntWalk(gpClCpm->compTable, cpmKillUserComponent, NULL, 0);
}

/*
 * This function deletes the EO and memory allocated for the component 
 * manager . Called with clusterMutex held.
 */
static ClRcT clCpmFinalize(void)
{
    ClRcT rc = CL_OK;
    ClCpmEOListNodeT *pTemp = NULL;
    ClCpmEOListNodeT *ptr = NULL;
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T compCount = 0;
    ClCpmBootOperationT *bootOp = NULL;

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, NULL, CL_CPM_LOG_0_SERVER_COMP_MGR_CLEANUP_INFO);
    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,"COMP_MGR: Inside componentMgrCleanUp \n");

    if (gpClCpm == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, NULL, CL_CPM_LOG_1_SERVER_COMP_MGR_INIT_ERR, rc);
        clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM, "COMP_MGR: Component Mgr Not initialized \n");
        return CL_CPM_RC(CL_ERR_DUPLICATE);
    }
    else
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ALERT, NULL, CL_CPM_LOG_1_SERVER_COMP_MGR_NODE_SHUTDOWN_INFO,
                   gpClCpm->pCpmLocalInfo->nodeName);

#ifdef CL_CPM_AMS
        if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
        {
            /* Check if there is a standby */
            if ((( (ClInt32T) gpClCpm->activeMasterNodeId != -1) && 
                    ( (ClInt32T) gpClCpm->activeMasterNodeId != gpClCpm->pCpmLocalInfo->nodeId)) 
                    || ( (ClInt32T) gpClCpm->deputyNodeId != -1))
            {
                /* Need to call state change ? */
                clAmsFinalize(&gAms,
                              (CL_AMS_TERMINATE_MODE_GRACEFUL |
                               CL_AMS_TERMINATE_MODE_SC_ONLY));
            }
            else
            {
                clAmsFinalize(&gAms, CL_AMS_TERMINATE_MODE_GRACEFUL);
            }

            clCpmAmsToCpmFree(gpClCpm->amsToCpmCallback);
            /*
             * Bug 4411:
             * Setting the callback pointer to NULL.
             */
            gpClCpm->amsToCpmCallback = NULL;

            clHeapFree(gpClCpm->cpmToAmsCallback);
            /*
             * Bug 4411:
             * Setting the callback pointer to NULL.
             */
            gpClCpm->cpmToAmsCallback = NULL;
        }
#endif
        cpmShutdownHeartbeat();
        
        /*
         * shutdown BM 
         */
        if(gpClCpm->bmTaskId)
        {
            gpClCpm->cpmShutDown = CL_FALSE;
            bootOp =
                (ClCpmBootOperationT *)
                clHeapAllocate(sizeof(ClCpmBootOperationT));
            if (bootOp == NULL)
            {
                clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                           "Unable to allocate memory \n");
                goto mallocFailed;
            }
            strcpy((ClCharT *)bootOp->nodeName.value, gpClCpm->pCpmLocalInfo->nodeName);
            bootOp->nodeName.length = strlen((const ClCharT *)bootOp->nodeName.value);
            bootOp->bootLevel = 0;
            bootOp->srcAddress.portId = 0;
            if(gpClCpm->bmTable != NULL)
            {
                rc = clOsalMutexLock(gpClCpm->bmTable->bmQueueCondVarMutex);
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, 
                        CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc,
                        rc, CL_LOG_HANDLE_APP);

                rc = clQueueNodeInsert(gpClCpm->bmTable->setRequestQueueHead,
                        (ClQueueDataT) bootOp);
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, 
                        CL_CPM_LOG_1_QUEUE_INSERT_ERR, rc,
                        rc, CL_LOG_HANDLE_APP);

                rc = clOsalCondSignal(gpClCpm->bmTable->bmQueueCondVar);
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, 
                        CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR,
                        rc, rc, CL_LOG_HANDLE_APP);
                
                rc = clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, 
                        CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                        rc, rc, CL_LOG_HANDLE_APP);

                /*
                 * clCpmBootLevelSet(&(bootOp->nodeName), NULL, 0);
                 */

                /*
                 * Wait till the BM thread do finalize 
                 */
                clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM, 
                           "Shutdown flag is [%d], Entering sleep...", gpClCpm->cpmShutDown);
                while (gpClCpm->cpmShutDown != CL_TRUE)
                {
                    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM, 
                               "Shutdown flag is [%d]", gpClCpm->cpmShutDown);
                    clOsalMutexUnlock(&gpClCpm->clusterMutex);
                    sleep(1);
                    clOsalMutexLock(&gpClCpm->clusterMutex);
                }
            }
        }

      mallocFailed:
        clOsalMutexLock(&gpClCpm->cpmShutdownMutex);
        gpClCpm->cpmShutDown = 2;
        clOsalTaskDelete(gpClCpm->bmTaskId);
        /*
         * take the semaphore 
         */
        rc = clOsalMutexLock(gpClCpm->eoListMutex);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(&gpClCpm->cpmShutdownMutex);
            clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,"Eo list mutex lock returned [%#x]", rc);
            goto failure;
        }

        /*
         * Delete All the EOs references, component config
         */
        /*
         * Deleting the Eos references first 
         */
        rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
        CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("Unable to Get First component \n"),
                          rc);

        rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                  (ClCntDataHandleT *) &comp);
        CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("Unable to Get component Data \n"),
                          rc);

        compCount = gpClCpm->noOfComponent;
        while (compCount != 0)
        {
            ptr = comp->eoHandle;
            while (ptr != NULL && ptr->eoptr != NULL)
            {
                pTemp = ptr;
                ptr = ptr->pNext;
                clHeapFree(pTemp->eoptr);
                pTemp->eoptr = NULL;
                clHeapFree(pTemp);
                pTemp = NULL;
            }

            compCount--;
            if (compCount)
            {
                rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
                CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                                  ("Unable to Get next component \n"), rc);
                rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                          (ClCntDataHandleT *) &comp);
                CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                                  ("Unable to Get component Data \n"), rc);
            }
        }
        /*
         * Release the semaphore 
         */
        clOsalMutexUnlock(gpClCpm->eoListMutex);

        cpmCmRequestDSFinalize();
        /* Need to move to proper location/function. */
        if (gpClCpm->slotTable)
        {
            clCntDelete(gpClCpm->slotTable);
        }
        
        cpmDebugDeregister();
        clDebugLibFinalize();

        /*
         * Cleans up the CpmBM data structure 
         */
        cpmBmCleanupDS();

        cpmClearOps();

        if(cpmValgrindTimeout > 0)
        {
            sleep(cpmValgrindTimeout);
        }

        cpmKillAllComponents();

        /*
         * Delete the component info.
         */
        cpmCompConfigFinalize();

        clEoClientUninstallTables(gpClCpm->cpmEoObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMF));

        cpmDeAllocate();
        clOsalMutexUnlock(&gpClCpm->cpmShutdownMutex);
    }

    if(!gClSigTermPending || !gClSigTermRestart)
    {
        FILE *fptr = fopen(CL_CPM_RESTART_DISABLE_FILE, "w");
        if(fptr) fclose(fptr);
    }

    clTimeServerFinalize();

    return rc;

  withlock:
    clOsalMutexUnlock(gpClCpm->eoListMutex);    /* */
    clOsalMutexUnlock(&gpClCpm->cpmShutdownMutex);
  failure:
    return rc;
}

void cpmChangeCwd(void)
{
    ClInt32T rc = CL_OK;

    const ClCharT *cpmCwdPath = NULL;

    if (NULL != (cpmCwdPath = getenv("ASP_CPM_CWD")))
    {
        strncpy(cpmCwd, cpmCwdPath, CL_MAX_NAME_LENGTH-1);
    }
    else
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                     "ASP is not being run from startup script... "
                     "Assuming current working directory to be \".\"");
        strncpy(cpmCwd, ".", CL_MAX_NAME_LENGTH-1);
        return;
    }

    rc = mkdir(cpmCwd, 0755);
    if ((-1 == rc) && (EEXIST != errno))
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                      "Unable to create directory [%s] : [%s]",
                      cpmCwd,
                      strerror(errno));
        clDbgPause();
        exit(1);
    }

    rc = chdir(cpmCwd);
    if (-1 == rc)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                      "Unable to change to directory [%s] : [%s]",
                      cpmCwd,
                      strerror(errno));
        exit(1);
    }
}

#if 0 /* Stone: stdout log file rotation is disabled: see related comments for why */
static void cpmLoggerRotate(void)
{
    register ClInt32T i;
    ClCharT file1[0xff+1];
    ClCharT file2[0xff+1];
    
    fsync(cpmLoggerFd);
    close(cpmLoggerFd);

    if(cpmLoggerFileRotations > 0)
    {
        for(i = cpmLoggerFileRotations-1; i >= 1; --i)
        {
            struct stat statbuf;
            snprintf(file1, sizeof(file1), "%s.%d", cpmLoggerFile, i);
            snprintf(file2, sizeof(file2), "%s.%d", cpmLoggerFile, i+1);
            if(!stat(file1, &statbuf))
            {
                unlink(file2);
                rename(file1, file2);
            }
        }
        snprintf(file1, sizeof(file1), "%s.%d", cpmLoggerFile, 1);
        unlink(file1);
        rename(cpmLoggerFile, file1);
    }
    cpmLoggerFd = open(cpmLoggerFile, O_RDWR | O_CREAT | O_TRUNC, 0777);
    /* dont care if open fails */
}

ClRcT cpmLoggerRotateEnable(void)
{
    clOsalMutexLock(&cpmLoggerMutex);
    cpmLoggerRotate();
    cpmLoggerTotalBytes = 0;
    clOsalMutexUnlock(&cpmLoggerMutex);
    return CL_OK;
}
#endif 

#if 0
/*
 * Reads from the pipe and dumps to the file.
 */
static void *cpmLoggerTask(void *arg)
{
    FILE *fptr = NULL;
    fptr = fdopen(cpmLoggerPipe[0], "rw");
    if(!fptr) return NULL;
    for(;(feof(fptr)==0);)
    {
        ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 30};
        ClCharT buf[1024];
        ClInt32T bytes = 0;
        clOsalTaskDelay(delay);  /* To stop 100% CPU use if something happens that is unexpected */        
        
        while(fgets(buf, sizeof(buf)-1, fptr))
        {
            bytes = strlen(buf);

            /* Stone: stdout log file rotation is disabled because OpenClovis logs no longer go to this file by default.
               We have seen multiple instances where the "stdin" side of this write has blocked, even when that side is the linux standard
               "logger" program.  This stops all entities that write to the stdout out log (which was ALL of them).  This logging is
               deprecated in favor of the shared memory logging system that drops logs rather than block.
               
               Additionally, the log rotation coded here may not even work, because changing this fd will not affect the fd inside child processes.               
             */
#if 0  
            clOsalMutexLock(&cpmLoggerMutex);
            cpmLoggerTotalBytes += bytes;
            if(cpmLoggerTotalBytes >= cpmLoggerFileSize)
            {
                cpmLoggerTotalBytes = bytes;
                cpmLoggerRotate();
            }
            clOsalMutexUnlock(&cpmLoggerMutex);
#endif            
            bytes = write(cpmLoggerFd, buf, bytes);
        }
    }

    fclose(fptr);
    return NULL;
}

static ClRcT cpmCreateLoggerTaskPipe(const ClCharT *cpmLoggerFile)
{
    ClRcT rc = CL_OK;
    struct stat statBuf;

    close(cpmLoggerFd);
    cpmLoggerFd = open(cpmLoggerFile, O_CREAT | O_APPEND | O_RDWR, 0777);
    if(cpmLoggerFd < 0)
    {
        rc = CL_CPM_RC(CL_ERR_UNSPECIFIED);
        clLogError(CPM_LOG_AREA_LOGGER,CPM_LOG_CTX_LOGGER_INI,"CPM logger file [%s] open returned [%s]\n",
                   cpmLoggerFile, strerror(errno));
        goto out;
    }
    if(!stat(cpmLoggerFile, &statBuf))
    {
        cpmLoggerTotalBytes = statBuf.st_size;
    }

    if(clCreatePipe(cpmLoggerPipe, CL_CPM_PIPE_MSGS, CL_CPM_PIPE_MSG_SIZE) < 0 )
    {
        clLogError(CPM_LOG_AREA_LOGGER,CPM_LOG_CTX_LOGGER_INI,"CPM logger pipe create returned [%s]\n",
                   strerror(errno));
        goto out_close;
    }
    dup2(cpmLoggerPipe[1], STDOUT_FILENO);
    dup2(cpmLoggerPipe[1], STDERR_FILENO);
    rc = clOsalTaskCreateDetached("LOGGER", CL_OSAL_SCHED_OTHER, 0, 0, cpmLoggerTask, NULL);
    if(rc != CL_OK)
        goto out_close;

    return rc;

    out_close:
    close(cpmLoggerFd);
    
    out:
    return rc;
}
#endif

#ifdef VXWORKS_BUILD

#define CPM_TERM_WRITE_BUFFER (16<<10U)

static ClRcT cpmCreateLoggerTaskConsole(const ClCharT *cpmLoggerFile)
{
    ClInt32T fd;
    const ClCharT *pTerm = "/tyCo/0";
    cpmLoggerPipe[0] = STDIN_FILENO;
    cpmLoggerPipe[1] = STDOUT_FILENO;
    fd = open(pTerm, O_RDWR, 0777);
    if(fd < 0)
    {
        clLogCritical("LOGGER", "INI", "Opening terminal [%s] returned with [%s]", pTerm, strerror(errno));
        return CL_CPM_RC(CL_ERR_LIBRARY);
    }
    if(ioctl(fd, FIOWBUFSET, CPM_TERM_WRITE_BUFFER) < 0)
    {
        clLogCritical("LOGGER", "INI", "Setting term write buffer to [%d] returned with [%s]",
                      CPM_TERM_WRITE_BUFFER, strerror(errno));
        close(fd);
        return CL_CPM_RC(CL_ERR_LIBRARY);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    return CL_OK;
}

#endif

#if 0
static ClRcT cpmCreateLoggerTask(const ClCharT *cpmLoggerFile)
{
#ifdef VXWORKS_BUILD
    if(clParseEnvBoolean("CL_VXWORKS_CONSOLE") == CL_TRUE)
    {
        return cpmCreateLoggerTaskConsole(cpmLoggerFile);
    }
#endif
    return cpmCreateLoggerTaskPipe(cpmLoggerFile);
}
#endif

static void cpmLoggerSetFileName(void)
{
    const ClCharT *cpmLogFileName = NULL;
    ClCharT *aspLogDir = NULL;
    ClCharT aspLogPath[CL_MAX_NAME_LENGTH] = {0};

    aspLogDir = getenv("ASP_LOGDIR");
    if (aspLogDir)
    {
        clLogTrace(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,"ASP_LOGDIR value is %s", aspLogDir);
        strncpy(aspLogPath, aspLogDir, CL_MAX_NAME_LENGTH-1);
    }
    else
    {
        strncpy(aspLogPath, cpmCwd, CL_MAX_NAME_LENGTH-1);
    }

    /* ASP_CPM_LOGFILE deprecated in favor of ASP_STDOUT_LOGFILE */
    if (NULL == (cpmLogFileName = getenv("ASP_CPM_LOGFILE"))) cpmLogFileName = getenv("ASP_STDOUT_LOGFILE");
    
    if (NULL != cpmLogFileName)
    {
        /*
         * If the file name is console, just put everything on console.
         */
        if (!strcasecmp(cpmLogFileName, "console"))
        {
            cpmIsConsoleStart = CL_TRUE;
            cpmLoggerFile[0] = 0;            
        }
        else
        {            
            /* If it looks like a full path, then just copy it in unmodified */
            if ((cpmLogFileName[0] == '/') || (cpmLogFileName[0] == '\\'))
                strncpy(cpmLoggerFile,cpmLogFileName,CL_MAX_NAME_LENGTH-1);

            /* Otherwise copy */
            else snprintf(cpmLoggerFile,CL_MAX_NAME_LENGTH-1,"%s/%s", aspLogPath,cpmLogFileName);
        }        
    }
    else
    {        
        snprintf(cpmLoggerFile,CL_MAX_NAME_LENGTH-1,"%s/%s.log", aspLogPath,clCpmNodeName);
    }
    
}


#if 0  /* Stone: the CPM logger using stdout/stderr through pipes is broken because what happens to child processes when the pipe forwarding thread or this process dies/hangs -- they hang. */   
static ClRcT cpmLoggerInitialize(void)
{
    ClRcT rc = CL_OK;
    ClCharT *cpmLogFileSizeStr = getenv("CPM_LOG_FILE_SIZE");
    ClCharT *cpmLogFileRotStr = getenv("CPM_LOG_FILE_ROTATIONS");

    rc = clOsalMutexInit(&cpmLoggerMutex);
    CL_ASSERT(rc == CL_OK);
    
    cpmLoggerSetFileName();

    if (cpmLogFileSizeStr)
    {
        ClUint32T len = strlen(cpmLogFileSizeStr);
        ClCharT *s = cpmLogFileSizeStr+len-1;
        ClUint32T size = 0;

        size = !strncasecmp(s, "k", 1) ? 1024 :
        !strncasecmp(s, "m", 1) ? 1024 * 1024 :
        !strncasecmp(s, "g", 1) ? 1024 * 1024 * 1024 : 0;

        if (!size)
        {
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                         "The value of CPM_LOG_FILE_SIZE env variable "
                         "is not specified properly. Its value is [%s]. "
                         "The format of the value "
                         "should be in the form <number>[KMG] e.g. 25M",
                         cpmLogFileSizeStr);
        }
        else
        {
            ClCharT t[CL_MAX_NAME_LENGTH];
            ClUint32T logFileSize = 0;
            
            strncpy(t, cpmLogFileSizeStr, len-1);
            logFileSize = atoi(t);
            if (!logFileSize)
            {
                clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                             "The value of CPM_LOG_FILE_SIZE env variable "
                             "is not specified properly. Its value is [%s]. "
                             "The format of the value "
                             "should be in the form <number>[KMG] e.g. 25M",
                             cpmLogFileSizeStr);
            }
            else
            {
                cpmLoggerFileSize = logFileSize * size;
            }
        }
    }

    if (cpmLogFileRotStr)
    {
        ClUint32T logFileRot = atoi(cpmLogFileRotStr);
        if (!logFileRot)
        {
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                         "The value of CPM_LOG_FILE_ROTATIONS env variable is taken as 0. "
                         "Disabling log rotations");
        }
        else
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "CPM_LOG_FILE_ROTATIONS value is %d",logFileRot);
        }
        cpmLoggerFileRotations = logFileRot;
    }

    rc = cpmCreateLoggerTask(cpmLoggerFile);

    return rc;
}
#endif

void cpmRedirectOutput(void)
{
    cpmLoggerSetFileName();

    if (cpmIsConsoleStart) return;

    cpmLoggerFd = -1;

    /* Bug 92: I do not know what is causing the file to not be openable but clearly it is happening.
       If we cannot redirect STDOUT, it hangs SAFplus because whatever is reading STDOUT (console or whatever) runs out of buffer space.
       So best to quit.
     */
    for(int retries=0;(cpmLoggerFd==-1) && (retries<5);retries++)
    {        
        cpmLoggerFd = open(cpmLoggerFile, O_RDWR | O_CREAT | O_APPEND, 0755);
        if (-1 == cpmLoggerFd)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,"Unable to open file [%s] for writing : [%s]",cpmLoggerFile,strerror(errno));
            sleep(20);
        }
    }
    CL_ASSERT(cpmLoggerFd != -1);
    if (cpmLoggerFd==-1)
    {
        exit(1);
    }
    dup2(cpmLoggerFd, STDOUT_FILENO);
    dup2(cpmLoggerFd, STDERR_FILENO);
}

#ifndef VXWORKS_BUILD
void cpmDaemonize_step1(void)
{
    ClInt32T rc = CL_OK;
    
    rc = setsid();
    if (-1 == rc)
    {
        perror("Unable to create new session");
        exit(1);
    }

}

void cpmDaemonize_step2(void)
{
    cpmChangeCwd();
    cpmRedirectOutput();
}
#endif

void cpmIocNotificationProtoInstall(void)
{
    ClEoProtoDefT eoProtoDef = 
    {
        CL_IOC_PORT_NOTIFICATION_PROTO,
        "IOC notification to CPM",
        clCpmIocNotification,
        NULL,
        CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE
    };

    clEoProtoSwitch(&eoProtoDef);
}

ClRcT cpmEoSerialize(ClPtrT pData)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(gpClCpm != NULL);

    rc = clOsalMutexLock(&gpClCpm->cpmMutex);
    CL_ASSERT(rc == CL_OK);
    
    return rc;
}

ClRcT cpmEoDeSerialize(ClPtrT pData)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(gpClCpm != NULL);

    rc = clOsalMutexUnlock(&gpClCpm->cpmMutex);
    CL_ASSERT(rc == CL_OK);
    
    return rc;
}

ClBoolT clCpmSwitchoverInline(void)
{
    return gClAmsSwitchoverInline;
}

static ClRcT clCpmInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

    if (NULL != gpClCpm)
    {
        clDbgRootCauseError(rc, ("Duplicate call to CPM initialize !!"));
        return CL_CPM_RC(CL_ERR_DUPLICATE);
    }

    cpmWriteNodeStatToFile("AMS", CL_NO);
    cpmWriteNodeStatToFile("CPM", CL_NO);

    cpmSigHandlerInstall();

    /*
     * Allocate and initilize the CPM structure 
     */
    rc = cpmAllocate();
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_SERVER_CPM_ALLOCATE_ERR, rc, rc, CL_LOG_HANDLE_APP);

    /*
     * Read/parse the configuration file and populate the gpClCpm structure 
     */
    
    rc = cpmGetConfig();
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_SERVER_CPM_CONFIG_GET_ERR, rc, rc, CL_LOG_HANDLE_APP);

    gClAmsSwitchoverInline = clParseEnvBoolean("CL_AMF_SWITCHOVER_INLINE");
    gClAmsPayloadResetDisable = clParseEnvBoolean("CL_AMF_PAYLOAD_RESET_DISABLE");

    if (!cpmIsForeground)
    {
        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Starting CPM as a daemon...");
    }
    else
    {
        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Starting CPM as a foreground process...");
    }

    strncpy(gpClCpm->logFilePath, cpmCwd, CL_MAX_NAME_LENGTH-1);
    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "CPM's current working directory is [%s]", gpClCpm->logFilePath);

#if 0  /* Stone: the CPM logger using stdout/stderr through pipes is broken because what happens to child processes when the pipe forwarding thread or this process dies/hangs -- they hang. */   
    if ((!cpmIsForeground) && (!cpmIsConsoleStart))
    {
        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Starting CPM's logger task..");
        cpmLoggerInitialize();
    }
#endif    

    if( (rc = clJobQueueInit(&cpmNotificationQueue, 0, 1) ) != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "CPM notification job queue initialize returned [%#x]", rc);
        goto failure;
    }

    clIocNotificationRegister(clCpmIocNotificationEnqueue, NULL);

    if( (rc = clTaskPoolCreate(&gCpmFaultPool, 1, 0, 0)) != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Task pool create returned [%#x]", rc);
        goto failure;
    }

    /*
     * Initialize the node information which will be sent to CPM/G
     * active.
     */
    strcpy((ClCharT *) gpClCpm->pCpmLocalInfo->nodeName, (ClCharT *) gpClCpm->pCpmConfig->nodeName);
    gpClCpm->pCpmLocalInfo->cpmAddress.nodeAddress = clIocLocalAddressGet();
    gpClCpm->pCpmLocalInfo->cpmAddress.portId = CL_IOC_CPM_PORT;
    gpClCpm->pCpmLocalInfo->status = CL_CPM_EO_ALIVE;
    gpClCpm->pCpmLocalInfo->version.releaseCode = CL_CPM_RELEASE_CODE;
    gpClCpm->pCpmLocalInfo->version.majorVersion = CL_CPM_MAJOR_VERSION;
    gpClCpm->pCpmLocalInfo->version.minorVersion = CL_CPM_MINOR_VERSION;

    /* Default boot level upto which registering node should boot */
    gpClCpm->pCpmLocalInfo->defaultBootLevel = gpClCpm->bmTable->defaultBootLevel;
    
    /* For CPM-CM interaction */
    gpClCpm->pCpmLocalInfo->slotNumber = ASP_NODEADDR;
    gpClCpm->pCpmLocalInfo->nodeId = gpClCpm->pCpmLocalInfo->slotNumber;
    
    clLogMultiline(CL_LOG_SEV_DEBUG, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "This node information -- \n"
                   "Node name : [%s] \n"
                   "IOC address : [%d] \n"
                   "IOC port : [%#x] \n"
                   "Node status : [%s] \n"
                   "Default boot level : [%d] \n"
                   "Slot number : [%d] \n"
                   "Node ID : [%d]",
                   gpClCpm->pCpmLocalInfo->nodeName,
                   gpClCpm->pCpmLocalInfo->cpmAddress.nodeAddress,
                   gpClCpm->pCpmLocalInfo->cpmAddress.portId,
                   "Alive",
                   gpClCpm->bmTable->defaultBootLevel,
                   gpClCpm->pCpmLocalInfo->slotNumber,
                   gpClCpm->pCpmLocalInfo->nodeId);
    
    strcpy((ClCharT *)gpClCpm->name.value, CL_CPM_COMPONENT_NAME);
    gpClCpm->name.length = strlen((const ClCharT *)gpClCpm->name.value);

    
    /*
     * Initialize all the component names on which CPM is dependent upon 
     */
    sprintf(gpClCpm->corServerName, "%s_%s", CL_CPM_COMPONENT_COR_NAME, gpClCpm->pCpmConfig->nodeName);
    sprintf(gpClCpm->eventServerName, "%s_%s", CL_CPM_COMPONENT_EVENT_NAME, gpClCpm->pCpmConfig->nodeName);
    sprintf(gpClCpm->logServerName, "%s_%s", CL_CPM_COMPONENT_LOG_NAME, gpClCpm->pCpmConfig->nodeName);

    sprintf((ClCharT *)gpClCpm->ckptCpmLName.value, "%s", gpClCpm->name.value);
    gpClCpm->ckptCpmLName.length = strlen((const ClCharT *)gpClCpm->ckptCpmLName.value);

    gpClCpm->ckptHandle = CL_HANDLE_INVALID_VALUE;
    gpClCpm->ckptOpenHandle = CL_HANDLE_INVALID_VALUE;

    gpClCpm->activeMasterNodeId = -1;
    gpClCpm->deputyNodeId = -1;
    gpClCpm->cpmGmsHdl = CL_HANDLE_INVALID_VALUE;
    gpClCpm->nodeLeaving = CL_FALSE;

    rc = clEoMyEoObjectGet(&gpClCpm->cpmEoObj);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EO_OBJECT_ERR, rc, rc, CL_LOG_HANDLE_APP);

    CL_ASSERT(gpClCpm->cpmEoObj != NULL);

    rc = clEoClientInstallTables(gpClCpm->cpmEoObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMF));
  
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EO_CLIENT_INST_ERR, rc, rc, CL_LOG_HANDLE_APP);
    rc = clCpmClientTableRegister(gpClCpm->cpmEoObj);
     
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EO_CLIENT_INST_ERR, rc, rc, CL_LOG_HANDLE_APP);
    
    /*
     * Install EO protocol to get IOC notifications for component
     * death, node death etc.
     */
    cpmIocNotificationProtoInstall();
    if((rc = cpmCmRequestDSInitialize()) != CL_OK)
        goto failure;

    /*
     * Initialize the various library which needs to be initialized after EO
     * creation 
     */
    rc = clDebugLibInitialize();
    CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "DEBUG", rc, rc, CL_LOG_HANDLE_APP);
    rc = cpmDebugRegister();
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_DEBUG_REG_ERR, rc, rc, CL_LOG_HANDLE_APP);
    
    /*
     * this flag will be unset during BM shutdown 
     */
    gpClCpm->cpmShutDown = CL_TRUE;

#ifdef CL_CPM_AMS
    rc = clCpmAmsToCpmInitialize(&(gpClCpm->amsToCpmCallback));
    CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Unable Initialize AmsToCpm callback %x\n", rc), rc);

    gpClCpm->cpmToAmsCallback =
        (ClCpmCpmToAmsCallT *) clHeapAllocate(sizeof(ClCpmCpmToAmsCallT));
    if (gpClCpm->cpmToAmsCallback == NULL)
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Unable to allocate memory \n"), CL_CPM_RC(CL_ERR_NO_MEMORY));

    rc = clAmsInitialize(&gAms, gpClCpm->amsToCpmCallback, gpClCpm->cpmToAmsCallback);
    CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("Unable to initialize AMS %x\n", rc), rc);
#endif

#ifndef CL_CPM_GMS
    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
    {
        gpClCpm->haState = CL_AMS_HA_STATE_ACTIVE;
        rc = cpmUpdateTL(gpClCpm->haState);
        CL_CPM_CHECK(CL_LOG_SEV_ERROR, ("unable to update TL for master address, exiting %x\n", rc), rc);
    }
#endif
    gpClCpm->activeMasterNodeId = 0;
    
    gpClCpm->polling = 1;

    /*
     * Load a customer's node detect status plugin, if it exists
     */
    clAmfPluginHandle = clLoadPlugin(CL_AMF_PLUGIN_ID, CL_AMF_PLUGIN_VERSION, CL_AMF_PLUGIN_NAME);
    if (NULL != clAmfPluginHandle)
    {
        clLogInfo("PLG","INI","Loaded AMF plugin [%s].", CL_AMF_PLUGIN_NAME);

        clAmfPlugin = (ClAmfPluginApi*) clAmfPluginHandle->pluginApi;
        if (clAmfPlugin->clAmfRunTask)
        {
            clAmfPlugin->clAmfRunTask(clAmfPluginNotificationCallback);
        }
    }

    /*
     * Initialization is done. So now start the BM which will create a thread
     * and start processing in that thread
     * GAS: Better to use this thread for the boot manager and spawn compMgrPollThread only if needed
     * b/c its not used anymore
     */
    
    rc = cpmBmInitialize(&(gpClCpm->bmTaskId), gpClCpm->cpmEoObj);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BM_INIT_ERR, rc, rc, CL_LOG_HANDLE_APP);

    /*
     * Start doing health check of all component/EO running in the system.
     * This should return only when nodeshutdown is called 
     */
    
    clLog(CL_LOG_SEV_NOTICE, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, "AMF server fully up");
    
    compMgrPollThread();

    failure:
    
    clOsalMutexLock(&gpClCpm->clusterMutex);

    clCpmFinalize();

#ifdef VXWORKS_BUILD
    pthread_mutex_lock(&mutexMain);
    pthread_cond_signal(&condMain);
    pthread_mutex_unlock(&mutexMain);
#endif
    
    clOsalMutexUnlock(&gpClCpm->clusterMutex);

    return rc;
}

/**
 *  Name: compMgrEORegister
 *
 *  Registers the EO with the component manager 
 *
 *  This function is called whenever a new EO is created. The new EO is 
 *  added to the EOList 
 *
 *  @param eoArg  : Client data which was passed through clEoClientInstall
 *         pInfo :  contains pointer to the EO which invoked this function
 *         inlen : length of pInfo
 *         pOutBuff : not needed but for RMD compliance
 *         pOutLen  : not needed but for RMD compliance

 *
 *  @returns 
 *    CL_OK                - everything is ok <br>
 *    CL_CPM_RC(CL_ERR_NULL_POINTER)      - Improper(NULL) input parameter
 *    CL_ERR_VERSION_MISMATCH - version mismatch
 *
 */

ClRcT VDECL(compMgrEORegister)(ClEoDataT data,
                               ClBufferHandleT inMsgHdl,
                               ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmEOListNodeT *pTemp = NULL;
    ClCpmEOListNodeT *tempPtr = NULL;
    ClCpmComponentT *comp = NULL;
    ClEoExecutionObjT eoObj ;
    ClEoExecutionObjT *pEOptr = &eoObj;
    ClEoExecutionObjT *tmpEOh = NULL;
    ClIocNodeAddressT myOMAddress;
    VDECL_VER(ClCpmClientInfoIDLT, 4, 0, 0) info = {0};
    ClUint32T msgLength = 0;

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_EO,"Inside compMgrEORegister \n");

    rc = clBufferLengthGet(inMsgHdl, &msgLength);
    if(!msgLength || (rc != CL_OK))
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EO,
                   "Invalid buffer passed, error [%#x]",
                   rc);
        goto failure;
    }

    rc = VDECL_VER(clXdrUnmarshallClCpmClientInfoIDLT, 4, 0, 0)(inMsgHdl, &info);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    if (info.version > CL_CPM_EO_VERSION_NO)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_VERSION_MISMATCH,
                       CL_ERR_VERSION_MISMATCH,
                       CL_LOG_HANDLE_APP);

    memset(&eoObj, 0, sizeof(eoObj));
    strncpy(eoObj.name, info.eoObj.name, sizeof(eoObj.name)-1);
    eoObj.eoID = info.eoObj.eoID;
    eoObj.pri = (ClOsalThreadPriorityT) info.eoObj.pri;
    eoObj.state = (ClEoStateT) info.eoObj.state;
    eoObj.threadRunning = info.eoObj.threadRunning;
    eoObj.noOfThreads = info.eoObj.noOfThreads;
    eoObj.eoInitDone = info.eoObj.eoInitDone;
    eoObj.eoSetDoneCnt = info.eoObj.eoSetDoneCnt;
    eoObj.refCnt = info.eoObj.refCnt;
    eoObj.eoPort = info.eoObj.eoPort;
    eoObj.appType = info.eoObj.appType;
    eoObj.maxNoClients = info.eoObj.maxNoClients;

    /*
     * Get the local OMAddress 
     */
    myOMAddress = clIocLocalAddressGet();

    pTemp =
        (ClCpmEOListNodeT *) clHeapAllocate((ClUint32T) sizeof(ClCpmEOListNodeT));
    if (pTemp == NULL)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                       CL_CPM_RC(CL_ERR_NO_MEMORY),
                       CL_LOG_HANDLE_APP);

    tmpEOh =
        (ClEoExecutionObjT *) clHeapAllocate((ClUint32T)
                                           sizeof(ClEoExecutionObjT));
    if (tmpEOh == NULL)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                       CL_CPM_RC(CL_ERR_NO_MEMORY),
                       CL_LOG_HANDLE_APP);

    memcpy(tmpEOh, pEOptr, sizeof(ClEoExecutionObjT));
    tmpEOh->eoID = ++(gpClCpm->eoIdAlloc);
    /*
     * FIXME: Still Keeping the eoId and eoPort same because all the component
     * are using Id.  Needs to remove this 
     */
    /*
     * tmpEOh->eoID = tmpEOh->eoPort;
     */
    rc = clXdrMarshallClUint64T(&tmpEOh->eoID, outMsgHdl, 0);
    if(rc != CL_OK)
    {
        clLogError("EO", "REGISTER", "EO ID marshall returned [%#x]", rc);
        goto failure;
    }

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_EO,
               "COMP_MGR: Inside compMgrEORegister EOId %llx EOPort %x\n",
               tmpEOh->eoID, tmpEOh->eoPort);
    pTemp->pNext = NULL;
    pTemp->eoptr = tmpEOh;
    pTemp->status = CL_CPM_EO_ALIVE;
    pTemp->freq = (ClInt32T) gpClCpm->pCpmConfig->defaultFreq;
    pTemp->nextPoll = (ClInt32T) gpClCpm->pCpmConfig->defaultFreq;
    pTemp->dontPoll = 0;

    gpClCpm->noOfEo++;
    rc = cpmCompFind(info.compName.value, gpClCpm->compTable, &comp);
    if (comp != NULL)
    {
        /*
         * take the semaphore 
         */
        if (clOsalMutexLock(gpClCpm->eoListMutex) != CL_OK)
            CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc,
                           rc, CL_LOG_HANDLE_APP);
        comp->eoPort = tmpEOh->eoPort;
        if(comp->lastEoID)
        {
            comp->lastEoID = comp->eoID;
        }
        comp->eoID = tmpEOh->eoID;
        if(!comp->lastEoID)
        {
            comp->lastEoID = comp->eoID;
        }
        tempPtr = comp->eoHandle;
        pTemp->compRef = comp;
        if (comp->eoCount == 0)
            comp->eoHandle = pTemp;
        else
        {
            while (tempPtr->pNext != NULL)
                tempPtr = tempPtr->pNext;
            tempPtr->pNext = pTemp;
        }
        comp->eoCount += 1;
        clOsalMutexUnlock(gpClCpm->eoListMutex);
    }
    return rc;

  failure:
    /*
     * Do necessary cleanups 
     */
    if (pTemp != NULL)
    {
        clHeapFree(pTemp);
        pTemp = NULL;
    }
    if (tmpEOh)
    {
        clHeapFree(tmpEOh);
        tmpEOh = NULL;
    }
    return rc;
}

static ClRcT _compMgrFuncEOWalk(ClCpmFuncWalkT *pWalk)
{
    /*
     * ClCpmEOListNodeT* ptr = gpClCpm->eoList;
     */
    ClRcT rc = CL_OK;
    int isDead = 0;
    ClIocNodeAddressT myOMAddress = { 0 };
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T compCount = 0;
    ClCpmEOListNodeT *ptr = NULL;

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,"Inside compMgrFuncEOWalk \n");
    if (pWalk == NULL)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_NULL_ARGUMENT,
                       CL_CPM_RC(CL_ERR_NULL_POINTER),
                       CL_LOG_HANDLE_APP);

    /*
     * Get the local OMAddress 
     */
    myOMAddress = clIocLocalAddressGet();
    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
                   "component", rc, rc, CL_LOG_HANDLE_APP);

    rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                              (ClCntDataHandleT *) &comp);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc,
                   rc, CL_LOG_HANDLE_APP);

    /*
     * FIXME: Quit a Big Lock !! 
     */
    rc = clOsalMutexLock(gpClCpm->eoListMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    compCount = gpClCpm->noOfComponent;
    while (compCount != 0)
    {
        ptr = comp->eoHandle;
        while (ptr != NULL && ptr->eoptr != NULL)
        {
            if ((ptr->eoptr->eoPort != CL_IOC_CPM_PORT) &&
                (strcasecmp(ptr->eoptr->name, "MAINEO")))
            {
                if ((pWalk->funcNo == CL_EO_SET_STATE_COMMON_FN_ID) &&
                    (((ClEoStateT) *(pWalk->inputArg) == CL_EO_STATE_STOP) ||
                     ((ClEoStateT) *(pWalk->inputArg) == CL_EO_STATE_KILL)))
                {
                    if (ptr->status == CL_CPM_EO_DEAD)
                        isDead = 1;
                    else
                    {
                        isDead = 0;
                        ptr->status = CL_CPM_EO_DEAD;
                        ptr->eoptr->state = (ClEoStateT) *(pWalk->inputArg);
                    }
                }
                ptr->eoptr->state = (ClEoStateT) *(pWalk->inputArg);
                if (isDead == 0)
                {
                    if (pWalk->inLen == 0)
                    {
                        rc = CL_CPM_CALL_RMD_ASYNC_NEW(myOMAddress,
                                                       ptr->eoptr->eoPort,
                                                       pWalk->funcNo, NULL, 0,
                                                       NULL, NULL, 0, 0, 0, 0,
                                                       NULL, NULL, NULL);
                    }
                    else
                    {
                        rc = CL_CPM_CALL_RMD_ASYNC(myOMAddress,
                                                   ptr->eoptr->eoPort,
                                                   pWalk->funcNo,
                                                   (ClUint8T *) pWalk->inputArg,
                                                   pWalk->inLen, NULL, NULL, 0,
                                                   0, 0, 0, NULL, NULL);
                    }

                }
            }
            ptr = ptr->pNext;
        }
        compCount--;
        if (compCount == 0)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("Unable to Get Node  Data \n"),
                              rc);
            rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                      (ClCntDataHandleT *) &comp);
            CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("Unable to Get Node  Data \n"),
                              rc);
        }
    }
    /*
     * Release the semaphore 
     */
    clOsalMutexUnlock(gpClCpm->eoListMutex);
    return rc;

  withlock:
    /*
     * Release the semaphore 
     */
    clOsalMutexUnlock(gpClCpm->eoListMutex);
  failure:
    return rc;

}

/**
 *  Name: compMgrFuncEOWalk
 *
 *  Perform an operation on all the EOs registered with component manager 
 *
 *
 *  @param eoArg    : Client data which was passed through clEoClientInstall
 *         pWalk    : conatins function to be called and parameter to be passed
 *                    to the destination EO's RMD 
 *         inLen    : sizeof(int)
 *         pOutBuff : not needed but for RMD compliance
 *         pOutLen  : not needed but for RMD compliance
 *
 *  @returns 
 *    CL_OK                - everything is ok <br>
 *    CL_CPM_RC(CL_ERR_NULL_POINTER)      - Improper(NULL) input parameter
 *    CL_ERR_VERSION_MISMATCH - version mismatch
 *
 */

ClRcT VDECL(compMgrFuncEOWalk)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmClientInfoT info = {0};
    ClUint32T msgLength = 0;

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_EO,"Inside compMgrFuncEOWalk \n");

    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if (msgLength == sizeof(ClCpmClientInfoT))
    {
        rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &info,
                                       &msgLength);
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                       CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EO,
                   "Invalid buffer passed, error [%#x]",
                   rc);
        goto failure;
    }

    if (info.version > CL_CPM_EO_VERSION_NO)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_VERSION_MISMATCH,
                       CL_ERR_VERSION_MISMATCH, 
                       CL_LOG_HANDLE_APP);

    rc = _compMgrFuncEOWalk(&(info.walkInfo));

  failure:
    return rc;
}

/**
 *  Name: compMgrEOStateUpdate
 *
 *  Function invoked whenever the state of an EO is updated
 *
 *  @param eoArg : Client data which was passed through clEoClientInstall
 *         pInfo : contains handle to EO whose state has changed 
 *         inlen : length of pInfo
 *         pOutBuff : not needed but for RMD compliance
 *         pOutLen  : not needed but for RMD compliance
 *
 *  @returns 
 *    CL_OK                - everything is ok <br>
 *    CL_CPM_RC(CL_ERR_NULL_POINTER)      - null pointer passed
 *    CL_ERR_VERSION_MISMATCH - version mismatch
 *
 */

ClRcT VDECL(compMgrEOStateUpdate)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmEOListNodeT *ptr = NULL;
    ClEoExecutionObjT eoObj;
    ClEoExecutionObjT *pEOptr = &eoObj;
    ClCpmComponentT *comp = NULL;
    VDECL_VER(ClCpmClientInfoIDLT, 4, 0, 0) info = {0};
    ClUint32T msgLength = 0;

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_EO,"Inside compMgrEOStateUpdate \n");

    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if(!msgLength)
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EO,
                   "Invalid buffer passed, error [%#x]",
                   rc);
        goto failure;
    }

    rc = VDECL_VER(clXdrUnmarshallClCpmClientInfoIDLT, 4, 0, 0)(inMsgHandle, (ClUint8T *) &info);

    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    if (info.version > CL_CPM_EO_VERSION_NO)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_VERSION_MISMATCH,
                       CL_ERR_VERSION_MISMATCH,
                       CL_LOG_HANDLE_APP);

    memset(&eoObj, 0, sizeof(eoObj));
    strncpy(eoObj.name, info.eoObj.name, sizeof(eoObj.name)-1);
    eoObj.eoID = info.eoObj.eoID;
    eoObj.pri = (ClOsalThreadPriorityT) info.eoObj.pri;
    eoObj.state = (ClEoStateT) info.eoObj.state;
    eoObj.threadRunning = info.eoObj.threadRunning;
    eoObj.noOfThreads = info.eoObj.noOfThreads;
    eoObj.eoInitDone = info.eoObj.eoInitDone;
    eoObj.eoSetDoneCnt = info.eoObj.eoSetDoneCnt;
    eoObj.refCnt = info.eoObj.refCnt;
    eoObj.eoPort = info.eoObj.eoPort;
    eoObj.appType = info.eoObj.appType;
    eoObj.maxNoClients = info.eoObj.maxNoClients;

    clOsalMutexLock(gpClCpm->compTableMutex);

    rc = cpmCompFindWithLock(info.compName.value, gpClCpm->compTable, &comp);
    CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                      (" Requested component does not exist %x\n", rc), rc);

    clOsalMutexLock(gpClCpm->eoListMutex);
    ptr = comp->eoHandle;
    while (ptr != NULL)
    {
        if (ptr->eoptr->eoID == pEOptr->eoID)
        {
            ptr->eoptr->state = pEOptr->state;
            if ((pEOptr->state == CL_EO_STATE_STOP) ||
                (pEOptr->state == CL_EO_STATE_KILL))
            {
                ptr->status = CL_CPM_EO_DEAD;
                if (pEOptr->eoPort == CL_IOC_EVENT_PORT)
                {
                    gpClCpm->emUp = 0;
                }
            }
            break;
        }
        ptr = ptr->pNext;
    }
    clOsalMutexUnlock(gpClCpm->eoListMutex);

    clOsalMutexUnlock(gpClCpm->compTableMutex);

    return rc;

    withlock:
    /*
     * Release the semaphore 
     */
    clOsalMutexUnlock(gpClCpm->compTableMutex);

    failure:
    return rc;
}

/**
 *  Name: compMgrStateStatusGet 
 *
 *  This is a util function to obtain the state and status in
 *  string format 
 *
 *  @param status : status in numerical format 
 *         state  : state in numerical format
 *         pStatus: ptr to status in string fromat
 *         pState : ptr to state in string format
 *
 *  @returns 
 *         void
 *
 */

void compMgrStateStatusGet(ClUint32T  status,
                           ClEoStateT state,
                           char      *pStatus,
                           ClUint32T  pStatusSize,
                           char      *pState,
                           ClUint32T  pStateSize)
{
    switch (status)
    {
        case CL_CPM_EO_ALIVE:
            strncpy(pStatus, " ALIVE ", pStatusSize-1);
            break;
        case CL_CPM_EO_DEAD:
            strncpy(pStatus, "  DEAD  ", pStatusSize-1);
            break;
        default:
            strncpy(pStatus, "  SICK  ", pStatusSize-1);
            break;
    }

    switch (state)
    {
        case CL_EO_STATE_INIT:
            strncpy(pState, "  INIT  ", pStateSize-1);
            break;
        case CL_EO_STATE_ACTIVE:
            strncpy(pState, " ACTIVE ", pStateSize-1);
            break;
        case CL_EO_STATE_SUSPEND:
            strncpy(pState, " SUSPND ", pStateSize-1);
            break;
        case CL_EO_STATE_RESUME:
            strncpy(pState, " RESUME ", pStateSize-1);
            break;
        case CL_EO_STATE_STOP:
            strncpy(pState, "  STOP  ", pStateSize-1);
            break;
        case CL_EO_STATE_KILL:
            strncpy(pState, "  KILL  ", pStateSize-1);
            break;
        case CL_EO_STATE_STDBY:
            strncpy(pState, "  STDBY ", pStateSize-1);
            break;
        case CL_EO_STATE_FAILED:
            strncpy(pState, "  FAILED ", pStateSize-1);
            break;
        default:
            break;
    }
}

/**
 *  Name: compMgrEOStateSet 
 *
 *  This function is for changing the state of EO/EOs from CLI
 *
 *  @param pInfo : contains eoid of the EO and state to be updated to 
 *         inlen : length of pInfo
 *         pOutBuff : not needed but for RMD compliance
 *         pOutLen  : not needed but for RMD compliance
 *
 *  @returns 
 *    CL_OK                    - everything is ok <br>
 *    CL_CPM_OPERATION_NOT_ALLOWED - Cant perform the operation
 *    CL_CPM_EO_UNREACHABLE        - EO doesnt exist
 *    CL_ERR_VERSION_MISMATCH     - version mismatch
 *    CL_CPM_RC(CL_ERR_NULL_POINTER)          - NULL pointer
 *
 */

ClRcT VDECL(compMgrEOStateSet)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT myOMAddress = 0;
    ClCpmFuncWalkT *pWalk = NULL;
    ClCpmEOListNodeT *ptr = NULL;
    int isDead = 0;
    int found = 0;
    ClEoStateT state;
    ClEoIdT eoId = 0;
    int eoPort = 0;
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T compCount = 0;
    ClCpmClientInfoT info = {0};
    ClUint32T msgLength = 0;

    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if (msgLength == sizeof(ClCpmClientInfoT))
    {
        rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) &info,
                                       &msgLength);
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                       CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EO,
                   "Invalid buffer passed, error [%#x]",
                   rc);
        goto failure;
    }

    if (info.version > CL_CPM_EO_VERSION_NO)
        CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_VERSION_MISMATCH,
                       CL_ERR_VERSION_MISMATCH,
                       CL_LOG_HANDLE_APP);

    eoId = info.eoId;
    /*
     * eoPort = info.eoPort;
     */
    state = info.state;

    if (eoId == 0xffff)         /* This means all the EOs */
    {
        /*
         * Set the attributes for the function to be invoked 
         */
        pWalk =
            (ClCpmFuncWalkT *) clHeapAllocate((ClUint32T) sizeof(ClCpmFuncWalkT) +
                                            (ClUint32T) sizeof(state));
        if (pWalk == NULL)
            CL_CPM_CHECK_0(CL_LOG_SEV_ERROR,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                           CL_CPM_RC(CL_ERR_NO_MEMORY),
                           CL_LOG_HANDLE_APP);

        pWalk->funcNo = CL_EO_SET_STATE_COMMON_FN_ID;
        pWalk->inLen = (ClUint32T) sizeof(state);
        memcpy(pWalk->inputArg, (void *) &state, pWalk->inLen);

        rc = _compMgrFuncEOWalk(pWalk);
        if (rc != CL_OK)
            clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_EO,"COMP_MGR : EoWalk failed.\n");
    }
    else
    {
        /*
         * Get the local OMAddress 
         */
        myOMAddress = clIocLocalAddressGet();
        /*
         * take the semaphore 
         */
        if ((rc = clOsalMutexLock(gpClCpm->eoListMutex)) != CL_OK)
            CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc,
                           rc, CL_LOG_HANDLE_APP);

        rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
        CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                          ("\n Unable to Get First component \n"), rc);
        rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                  (ClCntDataHandleT *) &comp);
        CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("\n Unable to Get Node  Data \n"),
                          rc);

        compCount = gpClCpm->noOfComponent;
        while (compCount != 0)
        {
            ptr = comp->eoHandle;
            while (ptr != NULL && ptr->eoptr != NULL)
            {
                if (eoId == ptr->eoptr->eoID)
                {
                    found = 1;
                    eoPort = ptr->eoptr->eoPort;
                    if ((state == CL_EO_STATE_STOP) ||
                        (state == CL_EO_STATE_KILL))
                    {
                        if (ptr->status == CL_CPM_EO_DEAD)
                        {
                            isDead = 1;
                            break;
                        }
                        ptr->status = CL_CPM_EO_DEAD;
                    }
                    ptr->eoptr->state = state;
                    break;
                }
                else
                    ptr = ptr->pNext;
            }
            compCount--;
            if (compCount)
            {
                rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
                CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                                  ("Unable to Get Node  Data \n"), rc);

                rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                          (ClCntDataHandleT *) &comp);
                CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                                  ("Unable to Get Node  Data \n"), rc);
            }
        }
        /*
         * Release the semaphore 
         */
        rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc,
                       rc, CL_LOG_HANDLE_APP);

        /*
         * Call the set state function for the given EO 
         */
        if (found == 1)
        {
            if (isDead == 0)
            {
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(myOMAddress, (ClIocPortT) eoPort,
                                               CL_EO_SET_STATE_COMMON_FN_ID,
                                               (ClUint8T *) &state,
                                               (ClUint32T) sizeof(state), NULL,
                                               NULL, 0, 0, 0, 0, NULL, NULL,
                                               MARSHALL_FN(ClEoStateT, 4, 0, 0)
                );

                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_RMD_CALL_ERR, rc,
                               rc, CL_LOG_HANDLE_APP);
            }
        }
        else
        {
            CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_SERVER_EO_UNREACHABLE,
                           rc, rc, CL_LOG_HANDLE_APP);
        }
    }

    if(pWalk)
    {
        clHeapFree(pWalk);
        pWalk = NULL;
    }
    return rc;

  withlock:
    /*
     * Release the semaphore 
     */
    rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);
  failure:
    if (pWalk)
    {
        clHeapFree(pWalk);
        pWalk = NULL;
    }
    return rc;
}

void cpmCpmLHBFailure(ClCpmLocalInfoT *cpmL)
{
    time_t t1;
    ClCharT buffer[CL_MAX_NAME_LENGTH] = {0};

    CL_ASSERT(CL_CPM_IS_ACTIVE());

    /*
     * Bug 4424:
     * Added the OR condition to return if pThis->status
     * is CL_CPM_EO_DEAD.
     */
    if ((cpmL == NULL) || (cpmL->status == CL_CPM_EO_DEAD))
        return;

    cpmL->status = CL_CPM_EO_DEAD;

    time(&t1);
    ctime_r(&t1, buffer);
    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
               "Node heartbeat to [%s] with node ID [%d] failed at [%s]",
               cpmL->nodeName,
               cpmL->nodeId,
               buffer);

    cpmFailoverNode(cpmL->nodeId, CL_FALSE);
}

void cpmLocalHealthFeedBack(ClRcT retCode,
                            void *pThiscpmL,
                            ClBufferHandleT sendMsg,
                            ClBufferHandleT recvMsg)
{
    ClCpmSchedFeedBackT outBuffer;
    ClRcT rc = CL_OK;
    ClCpmLT *pCpmLocal = (ClCpmLT *)pThiscpmL;
    ClCpmLocalInfoT *pCpmL = NULL; 

    CL_ASSERT(pCpmLocal != NULL);
    
    pCpmL = pCpmLocal->pCpmLocalInfo;
    if (!pCpmL)
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                     "Node [%s] has already been unregistered, "
                     "ignoring the healthcheck feedback...",
                     pCpmLocal->nodeName);
        goto failure;
    }

    if (CL_RMD_TIMEOUT_UNREACHABLE_CHECK(retCode))
    {
        if (pCpmL->status != CL_CPM_EO_DEAD)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Health check feedback returned "
                       "timeout for node [%s], ",
                       pCpmLocal->nodeName);
            cpmCpmLHBFailure(pCpmL);
        }
    }
    else
    {
        rc = VDECL_VER(clXdrUnmarshallClEoSchedFeedBackT, 4, 0, 0)(recvMsg, (void *)&outBuffer);
        if (rc != CL_OK)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unmarshalling of healthcheck feedback failed, "
                       "error [%#x]",
                       rc);
            goto failure;
        }
    }

failure:
    clBufferDelete(&recvMsg);
    return;
}

/* Bug 4041:
 * Changed to non-static function so that it is
 * available outside now.
 */

static ClBoolT __cpmIsInfrastructureComponent(const ClCharT *compName)
{
    if(!compName || 
       !strstr(compName, "Server"))
        return CL_FALSE;

    if(strstr(compName, "cmServer"))
        return CL_TRUE;
    else if(strstr(compName, "snmpServer"))
        return CL_TRUE;
    else if(strstr(compName, "corServer"))
        return CL_TRUE;
    else if(strstr(compName, "txnServer"))
        return CL_TRUE;
    else if(strstr(compName, "logServer"))
        return CL_TRUE;
    else if(strstr(compName, "eventServer"))
        return CL_TRUE;
    else if(strstr(compName, "msgServer"))
        return CL_TRUE;
    else if(strstr(compName, "nameServer"))
        return CL_TRUE;
    else if(strstr(compName, "ckptServer"))
        return CL_TRUE;
    else if(strstr(compName, "gmsServer"))
        return CL_TRUE;
    else if(strstr(compName, "faultServer"))
        return CL_TRUE;
    else if(strstr(compName, "alarmServer"))
        return CL_TRUE;
    else if(strstr(compName, "cpmServer"))
        return CL_TRUE;
    else
        return CL_FALSE;
}

ClBoolT cpmIsInfrastructureComponent(SaNameT *compName)
{
    return __cpmIsInfrastructureComponent((const ClCharT *)compName->value);
}

static ClBoolT cpmIsCriticalComponent(const SaNameT *compName)
{
    CL_ASSERT(compName != NULL);
    
    if(strstr((const ClCharT *)compName->value, "corServer")) goto yes;
    else if (strstr((ClCharT *)compName->value, "gmsServer")) goto yes;
    else if (strstr((ClCharT *)compName->value, "msgServer")) goto yes;
    else if (strstr((ClCharT *)compName->value, "ckptServer")) goto yes;
    else goto no;

yes:
    return CL_TRUE;
no:
    return CL_FALSE;
}

static void cpmInvokeNodeCleanup(void)
{
    ClCharT cmdBuf[CL_MAX_NAME_LENGTH];
    ClCharT scriptFile[CL_MAX_NAME_LENGTH];
    const ClCharT *scriptDir = getenv("ASP_CONFIG");
    ClInt32T err = 0;
    if(!scriptDir)
    {
        clLogWarning("NODE", "CLEANUP", "CPM node cleanup script not invoked "
                     "as ASP_CONFIG environment variable is not defined");
        return;
    }
    else 
    {
        clLogDebug("NODE","CLEANUP", "ASP_CONFIG env variable value is %s",scriptDir);
    }

    snprintf(scriptFile, sizeof(scriptFile), "%s/%s", scriptDir, CPM_NODECLEANUP_SCRIPT);
    if(!(err = access(scriptFile, F_OK | X_OK)))
    {
        if(gpClCpm->pCpmLocalInfo)
            snprintf(cmdBuf, sizeof(cmdBuf), "%s %s", 
                     scriptFile, gpClCpm->pCpmLocalInfo->nodeName);
        else
            snprintf(cmdBuf, sizeof(cmdBuf), "%s", scriptFile);
        if(system(cmdBuf))
        {
            clLogWarning("NODE", "CLEANUP", "Running node cleanup script [%s] returned with [%s]",
                         scriptFile, strerror(errno));
        }
    }
    else if(err < 0 && errno != ENOENT)
    {
        clLogWarning("NODE", "CLEANUP", "Failed to execute node cleanup script [%s]. Error [%s]",
                     scriptFile, strerror(errno));
    }
        
}

void cpmCommitSuicide(void)
{
    clLogAlert("AMF", "QIT", "AMF is stopping itself");
    cpmInvokeNodeCleanup();
    cpmKillAllComponents();
    exit(0);
    //kill(0, SIGKILL);  // Stone: Why SIGKILL?
}

void cpmReset(ClTimerTimeOutT *pDelay, const ClCharT *pPersonality)
{
    FILE *fptr = NULL;
    if(pDelay) clOsalTaskDelay(*pDelay);
    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                  "CPM [%s] going for node reboot", pPersonality);
    fptr = fopen(CL_CPM_REBOOT_FILE, "w");
    if(!fptr)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                   "CPM [%s] recovery failure while trying to reboot the node.",
                   pPersonality);
    }
    else
    {
        fclose(fptr);
        cpmCommitSuicide();
        /* unreached*/
    }
}

void cpmRestart(ClTimerTimeOutT *pDelay, const ClCharT *pPersonality)
{
    FILE *fptr = NULL;
    ClTimerTimeOutT defaultTime;
    if (!pDelay)
    {
        defaultTime.tsSec = 2;
        defaultTime.tsMilliSec = 0;
        pDelay = &defaultTime;
    }
    
        //if(pDelay->tsSec >= 3)
        //    pDelay->tsSec >>= 1;
    /*
     *
     * We give a delay here, coz if it was a link snap and a 
     * a re-establish, comm layer link reestablish takes some time 
     * and the other guy would miss the node leave notification packet to be
     * sent by the kernel topology service to the subscribers, incase
     * we die at/before link re-establishment happens.
     */
    clOsalTaskDelay(*pDelay);   

    fptr = fopen(CL_CPM_RESTART_FILE, "w");
    if(!fptr)
    {
        if (!pPersonality) pPersonality = "";
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM, "AMF %s recovery failure to trigger the restart. Please ensure you restart SAFplus again", pPersonality);

    }
    else
    {
        fclose(fptr);
    }
    cpmCommitSuicide();
}

static void cpmRestartWorkers(void)
{
    ClIocNodeAddressT nodeAddress = CL_IOC_BROADCAST_ADDRESS;

    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                  "CPM/G is restarting payload nodes if any "\
                  "as a result of inconsistent state in the cluster "\
                  "due to split brain");


    clCpmClientRMDAsyncNew(nodeAddress,
                           CPM_NODE_SHUTDOWN,
                           (ClUint8T *)&nodeAddress,
                           sizeof(nodeAddress),
                           NULL,
                           NULL,
                           0,
                           0,
                           0,
                           CL_IOC_HIGH_PRIORITY,
                           clXdrMarshallClUint32T);
    
}

static void cpmFailoverRecover(void)
{
    /*
     * If current boot level is not default, then restart CPM
     * as it could be a early failover.
     */
    if (gpClCpm->bmTable->currentBootLevel !=
        gpClCpm->pCpmLocalInfo->defaultBootLevel)
    {
        ClTimerTimeOutT delay = { 5, 0 };
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                      "This standby node cannot become active "
                      "as its still booting up at level [%d] "
                      "and is expected to be at boot level [%d]. "
                      "This node would be restarted in [%d] secs",
                      gpClCpm->bmTable->currentBootLevel, 
                      gpClCpm->bmTable->defaultBootLevel,
                      delay.tsSec);
        cpmRestart(&delay, "controller");
        /*
         * should not return
         */
    }
}

void cpmActive2Standby(ClBoolT shouldRestartWorkers)
{
    ClTimerTimeOutT delay = { 5, 0 };

    /*
     * This condition would happen only with 2 CPMGS.
     * We just broadcast the shutdown with broadcast address and
     * payloads if any would get restarted.
     * Lets not rely on CPM config information w.r.t noOfCpms
     * after the split brain to avoid the broadcast incase there are no
     * payloads.
     */

    if (shouldRestartWorkers) cpmRestartWorkers();

    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                  "CPM active is becoming standby. "
                  "This node should be restarted in [ %d ] secs "
                  "as a result of %s in the cluster",
                  delay.tsSec,
                  shouldRestartWorkers ?
                  "inconsistent cluster state":
                  "leader change");
    
    cpmRestart(&delay, "active");
}

static void cpmInvokeStandby2ActiveHook(void)
{
    ClCharT *aspScriptDir = NULL;

#define CPM_STDBY2ACTIVE_SCRIPT "clStandby2Active.sh"

    aspScriptDir = getenv("ASP_CONFIG");
    if (aspScriptDir)
    {
        ClCharT scriptName[CL_MAX_NAME_LENGTH] = {0};
        ClInt32T ret = 0;
        
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                "ASP_CONFIG value %s",aspScriptDir);
        snprintf(scriptName,
                 CL_MAX_NAME_LENGTH-1,
                 "%s/%s",
                 aspScriptDir,
                 CPM_STDBY2ACTIVE_SCRIPT);

        ret = access(scriptName, F_OK|X_OK);
        if (!ret)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                       "Calling CPM standby to active hook...");

            ret = system(scriptName);
            if(ret < 0 )
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                           "Failed to execute [%s], error [%s]",
                           CPM_STDBY2ACTIVE_SCRIPT, strerror(errno));
            }
              
        }
        else if ((ret == -1) && (errno != ENOENT))
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                       "Failed to execute [%s], error [%s]",
                       CPM_STDBY2ACTIVE_SCRIPT,
                       strerror(errno));
        }
    }
}

/*
 * Called with global cluster mutex held.
 */

ClRcT cpmStandby2Active(ClGmsNodeIdT prevMasterNodeId, 
                        ClGmsNodeIdT deputyNodeId)
{
    ClRcT rc = CL_OK;
    ClCpmLocalInfoT *pCpmLocalInfo = NULL;

    /* We are already active so nothing to do */
    if (gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE)
    {
        return rc;
    }

    clOsalMutexLock(&gpClCpm->cpmMutex);
    pCpmLocalInfo = gpClCpm->pCpmLocalInfo;
    clOsalMutexUnlock(&gpClCpm->cpmMutex);

    if (pCpmLocalInfo == NULL)
    {
        rc = CL_CPM_RC(CL_ERR_NOT_EXIST);
        goto failure;
    }

    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS, "Node [%d] is changing HA state from standby to active", pCpmLocalInfo->nodeId);

    /*
     * Check if this node is capable of failing over before triggering the failover as it neednt be fully up to become active
     */
    cpmFailoverRecover();
    rc = cpmUpdateTL(CL_AMS_HA_STATE_ACTIVE);
    if (rc != CL_OK)
    {
        // will only happen if the IOC transparent address can't be registered due to a conflict or a programmatic error
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_TL, CL_CPM_LOG_1_TL_UPDATE_FAILURE, rc);
        cpmRestart(NULL, "controller");
        CL_ASSERT(0);  // There is no return from cpmRestart...        
    }

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CKP, "AMF reading its own checkpoints...");
    rc = cpmCpmLCheckpointRead();
    if (CL_OK != rc)
    {
        ClTimerTimeOutT delay = { 5,  0 };
        clLogMultiline(CL_LOG_SEV_CRITICAL,
                       CPM_LOG_AREA_CPM,
                       CPM_LOG_CTX_CPM_CKP,
                       "Failed to read checkpoint data, error [%#x] -- \n"
                       "The cluster has become unstable, doing self shutdown... \n"
                       "Please shutdown all the worker nodes manually.",
                       rc);
        cpmFailoverRecover();
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                      "This standby node cannot become active as the ckpt recovery failed "
                      "which has been mostly seen to be caused by xport config. issues."
                      "This node would be restarted in [%d] seconds", delay.tsSec);
        cpmRestart(&delay, "controller");
        CL_ASSERT(0);  // There is no return from cpmRestart...
    }

    /*
     * Inform AMS to become active and read the checkpoint.
     */
    if ((gpClCpm->cpmToAmsCallback != NULL) && 
        (gpClCpm->cpmToAmsCallback->amsStateChange != NULL))
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "Informing AMS on node [%d] to change state "
                   "from standby to active...",
                   pCpmLocalInfo->nodeId);

        rc = gpClCpm->cpmToAmsCallback->amsStateChange(CL_AMS_STATE_CHANGE_STANDBY_TO_ACTIVE |
                                                       CL_AMS_STATE_CHANGE_USE_CHECKPOINT);
        if (CL_OK != rc)
        {
            clLogMultiline(CL_LOG_SEV_CRITICAL,
                           CPM_LOG_AREA_CPM,
                           CPM_LOG_CTX_CPM_AMS,
                           "AMS state change from standby to active "
                           "returned [%#x] -- \n"
                           "The cluster has become unstable. \n"
                           "Shutting down all the nodes in the cluster and "
                           "doing self shutdown...",
                           rc);
            cpmFailoverRecover();
            cpmShutdownAllNodes();
            cpmSelfShutDown();
            goto failure;
        }

        cpmWriteNodeStatToFile("AMS", CL_YES);
    }

    gpClCpm->haState = CL_AMS_HA_STATE_ACTIVE;
    gpClCpm->activeMasterNodeId = pCpmLocalInfo->nodeId;
    gpClCpm->deputyNodeId = deputyNodeId;

    cpmFailoverNode(prevMasterNodeId, CL_TRUE);

    cpmInvokeStandby2ActiveHook();

    failure:
    return rc;
}

void cpmStandbyRecover(const ClGmsClusterNotificationBufferT *notificationBuffer)
{
    ClRcT rc = CL_OK;
    
    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
              "Node [%d] has become the deputy of the cluster, "
              "after trying to recover from an inconsistent "
              "cluster view",
              gpClCpm->pCpmLocalInfo->nodeId);

    rc = cpmUpdateTL(CL_AMS_HA_STATE_STANDBY);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, NULL,
                   CL_CPM_LOG_1_TL_UPDATE_FAILURE, rc);
    }

    if ((gpClCpm->amsToCpmCallback != NULL) &&
        (gpClCpm->cpmToAmsCallback != NULL))
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                   "Starting AMS in standby mode...");

        rc = clAmsStart(&gAms,
                        CL_AMS_INSTANTIATE_MODE_STANDBY);
        /*
         * Bug 4092:
         * If the AMS initialize fails then do the 
         * self shut down.
         */
        if (CL_OK != rc)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,"Unable to initialize AMS, error = [%#x]", rc);
            gpClCpm->amsToCpmCallback = NULL;
            cpmRestart(NULL,NULL);
            goto failure;
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
                    
        if (!gpClCpm->polling)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,"AMS finalize called before AMF initialize during node shutdown.");
        }
        else
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,"Unable to initialize AMF, error = [%#x]", rc);
            cpmRestart(NULL,NULL);
        }
        goto failure;
    }

    gpClCpm->haState = CL_AMS_HA_STATE_STANDBY;
    gpClCpm->deputyNodeId = notificationBuffer->deputy;

failure:    
    return;
}

/*
 * Mark the component recovery for standbys
 */
static void cpmMarkRecovery(ClCpmComponentT *compRef, ClUint64T instantiateCookie)
{
    ClRcT rc = CL_OK;
    if(CL_CPM_IS_STANDBY())
    {
        /*
         *mark the recovery invocation.
         */
        ClAmsInvocationT *pAmsInvocation = (ClAmsInvocationT*) clHeapCalloc(1, sizeof(*pAmsInvocation));
        ClInvocationT recoveryInvocation = 0;
        CL_ASSERT(pAmsInvocation != NULL);
        saNameSet(&pAmsInvocation->compName, compRef->compConfig->compName);
        pAmsInvocation->invocation = (ClInvocationT)instantiateCookie;
        pAmsInvocation->cmd = CL_AMS_RECOVERY_REPLAY_CALLBACK;
        rc = cpmInvocationAdd(CL_AMS_RECOVERY_REPLAY_CALLBACK, (void*)pAmsInvocation,
                              &recoveryInvocation, CL_CPM_INVOCATION_AMS | CL_CPM_INVOCATION_DATA_COPIED);
        if(rc != CL_OK)
        {
            clLogError("INV", "RECOVERY", "Adding recovery invocation for comp [%s] returned [%#x]",
                       compRef->compConfig->compName, rc);
        }
    }
}

void cpmEOHBFailure(ClCpmEOListNodeT *pThis)
{
    time_t t1;
    ClRcT status = 0;
    ClRcT rc = CL_OK;
    SaNameT nodeName = {0};
    SaNameT compName = {0};
    ClCpmLcmReplyT srcInfo = {0};
    ClCpmComponentT *comp = NULL;
    ClUint64T instantiateCookie = 0;

    time(&t1);

    clLogAlert(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
               "Failure detected for component [%s], "
               "eo name [%s], eo ID [0x%llx], instantiateCookie [%lld]",
               pThis->compRef->compConfig->compName,
               pThis->eoptr->name,
               pThis->eoptr->eoID,
               pThis->compRef->instantiateCookie);

    if (clDbgPauseOn)
    {
        clLogAlert(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Ignoring component failure since debugging is on.");
        return;
    }

    instantiateCookie = pThis->compRef->instantiateCookie;

    if (((ClCpmEOListNodeT *) pThis)->status != CL_CPM_EO_DEAD)
    {
        /*
         * Set the EO Status 
         */
        ((ClCpmEOListNodeT *) pThis)->status = CL_CPM_EO_DEAD;
        /*
         * Set the Component Operation State to Out-Of Service 
         */

        clOsalMutexLock(((ClCpmEOListNodeT *) pThis)->compRef->compMutex);

        ((ClCpmEOListNodeT *) pThis)->compRef->compOperState =
            CL_AMS_OPER_STATE_DISABLED;
        ((ClCpmEOListNodeT *) pThis)->compRef->compReadinessState =
            CL_AMS_READINESS_STATE_OUTOFSERVICE;

        clOsalMutexUnlock(((ClCpmEOListNodeT *) pThis)->compRef->compMutex);

        status = 1;

        if (((ClCpmEOListNodeT *) pThis)->eoptr->eoPort == CL_IOC_EVENT_PORT)
        {
            gpClCpm->emUp = 0;
        }
        else if (((ClCpmEOListNodeT *) pThis)->eoptr->eoPort == CL_IOC_COR_PORT)
        {
            gpClCpm->corUp = 0;
        }

        /*
         * Bug 6280
         */
        clIocTransparencyDeregister(pThis->compRef->compId);
        rc = cpmComponentEventPublish(pThis->compRef, CL_CPM_COMP_DEATH, CL_FALSE);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to publish component death event "
                       "for component [%s], error [%#x]",
                       pThis->compRef->compConfig->compName,
                       rc);
        }

        strcpy((ClCharT *) compName.value, ((ClCpmEOListNodeT *) pThis)->compRef->compConfig->compName);
        compName.length = strlen((const ClCharT *)compName.value);
        
        strcpy((ClCharT *) nodeName.value, gpClCpm->pCpmConfig->nodeName);
        nodeName.length = strlen((const ClCharT *) nodeName.value);

        if(cpmIsInfrastructureComponent(&compName))
        {
            cpmComponentEventCleanup(pThis->compRef);
            /*
             * If it is a critical ASP component, then kill self
             * instead of spreading the fault to other parts of cluster.
             */
            if (cpmIsCriticalComponent(&compName))
            {
                clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                              "Critical SAFplus component [%s] failed. Killing node [%s]",
                              compName.value,
                              gpClCpm->pCpmLocalInfo->nodeName);
                cpmCommitSuicide();
                /*
                 * Shouldn't return
                 */
                exit(0);
            }

            rc = cpmCompFind(compName.value, 
                             gpClCpm->compTable, &comp);
            if(rc == CL_OK && comp != NULL)
            {
                /*
                 * Bug 4104: restart problem.
                 * Number of restart attempts now increased by 1.
                 */
                /* Bug 5455: 
                 * Reset the failure count if the time of failure is greater 
                 * than last failure time by 100 seconds.This is to avoid the
                 * node shutdown if the 3rd failure is greater than 100 seconds
                 * window.
                 */
                comp->restartPending = 0;
                if((t1 - comp->compRestartTime) >
                   CL_CPM_ASP_COMP_RESTART_WINDOW)
                {
                    comp->compRestartRecoveryCount = 0;
                }
                if(comp->compRestartRecoveryCount < 
                   CL_CPM_ASP_COMP_RESTART_ATTEMPT)
                {
                    srcInfo.srcIocAddress = clIocLocalAddressGet();
                    srcInfo.srcPort = CL_IOC_CPM_PORT;
                    srcInfo.rmdNumber = CPM_COMPONENT_LCM_RES_HANDLE;
                    
                    /* Try restarting the component */
                    rc = clCpmComponentRestart(&(compName), 
                                               &(nodeName), &srcInfo);
                    if((t1 - comp->compRestartTime) < 
                       CL_CPM_ASP_COMP_RESTART_WINDOW)
                    {
                        /* 
                         * incrementing the compRestartRecoveryCount so that 
                         * node will be shutdown in case of 3rd time failure 
                         * within the restart window.
                         */
                        comp->compRestartRecoveryCount += 1;
                        /*clOsalPrintf("Restart Time %d \n", 
                          comp->compRestartTime);*/
                    }
                    else
                    {
                        /*  
                         *  First HB failure of the component in the restart window
                         *  Start of the restart window.
                         */
                        comp->compRestartRecoveryCount = 1;
                        comp->compRestartTime = t1;
                    }
                }
                else
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                                  "ASP component [%s] has failed more than "
                                  "%d times in a time window of %d seconds. "
                                  "Killing node [%s]...",
                                  compName.value,
                                  CL_CPM_ASP_COMP_RESTART_ATTEMPT,
                                  CL_CPM_ASP_COMP_RESTART_WINDOW,
                                  gpClCpm->pCpmLocalInfo->nodeName);
                    cpmCommitSuicide();
                }
            }
        }
        else
        {
            ClBoolT doCleanup = CL_FALSE;
#ifdef CL_CPM_AMS
            /* BUG 4449:
             * Changed recommended recovery to NO RECOMMENDATION from RESTART
             * The value used should be picked up from comp configuration which
             * is parsed by AMS.
             */
            cpmMarkRecovery((ClCpmComponentT*)pThis->compRef, instantiateCookie);
            clCpmCompPreCleanupInvoke((ClCpmComponentT*)pThis->compRef);
            rc = clCpmComponentFailureReportWithCookie(0, &(compName), instantiateCookie,
                                                       0, CL_AMS_RECOVERY_NO_RECOMMENDATION, 0); 
            if(rc != CL_OK && CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)
            {
                doCleanup = CL_TRUE;
            }
#endif
            cpmComponentEventCleanup(pThis->compRef);
            /*
             * Cleanup the component locally if master is unavailable/unreachable
             */
            if(doCleanup)
            {
                _cpmLocalComponentCleanup(pThis->compRef, (ClCharT*)compName.value,
                                          NULL, gpClCpm->pCpmConfig->nodeName);
            }
        }
        
    }

    ((ClCpmEOListNodeT *) pThis)->freq =
        (ClInt32T) gpClCpm->pCpmConfig->defaultFreq;
    ((ClCpmEOListNodeT *) pThis)->nextPoll =
        (ClInt32T) gpClCpm->pCpmConfig->defaultFreq;
}

ClRcT cpmEoPtrFind(ClEoIdT eoID, ClCpmEOListNodeT **pThis)
{
    ClRcT rc = CL_OK;
    ClCpmEOListNodeT *ptr = NULL;
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T compCount = 0;
    ClUint32T found = 0;

    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
            "component", rc, rc, CL_LOG_HANDLE_APP);

    /*
     * For every component 
     */
    compCount = gpClCpm->noOfComponent;
    while (compCount && rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                (ClCntDataHandleT *) &comp);
        CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR,
                rc, rc, CL_LOG_HANDLE_APP);

        if (comp->compPresenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
        {
            if ((rc = clOsalMutexLock(gpClCpm->eoListMutex)) != CL_OK)
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR,
                        rc, rc, CL_LOG_HANDLE_APP);
            ptr = comp->eoHandle;
            while (ptr != NULL)
            {
                if (ptr->eoptr->eoID == eoID)
                {
                    *pThis = ptr;
                    found = 1;
                    break;
                }
                else
                    ptr = ptr->pNext;
            }
            clOsalMutexUnlock(gpClCpm->eoListMutex);
            if (found)
            {
                return CL_OK;
            }
        }

        compCount--;
        if (compCount)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            CL_CPM_CHECK_2(CL_LOG_SEV_ERROR, CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR,
                    "component", rc, rc, CL_LOG_HANDLE_APP);
        }
    }

    return CL_CPM_RC(CL_ERR_DOESNT_EXIST);

failure:
    return rc;
}

static void *compHBProcess(void *threadArg)
{
    CpmheartBeatFailedCompT *hbFailedcomp = 
        (CpmheartBeatFailedCompT *) threadArg;
    ClCpmComponentT *comp = NULL; 
    ClCpmEOListNodeT *pThis = NULL;

    comp = hbFailedcomp->comp;
    pThis = hbFailedcomp->eoHandle;
    clHeapFree(hbFailedcomp);

    clOsalMutexLock(&gpClCpm->clusterMutex);
    clOsalMutexLock(gpClCpm->eoListMutex);

    if (comp->eoHandle != NULL) 
    {
        cpmEOHBFailure(pThis);
    }

    clOsalMutexUnlock(gpClCpm->eoListMutex);
    clOsalMutexUnlock(&gpClCpm->clusterMutex);

    return NULL;
}

ClRcT cpmHBFailureProcess(ClOsalTaskIdT *pTaskId, 
                          CpmheartBeatFailedCompT *hbFailedComp)
{
    ClRcT rc = CL_OK;

    ClUint32T priority  = CL_OSAL_THREAD_PRI_NOT_APPLICABLE;
    ClUint32T stackSize = 0;

    rc = clOsalTaskCreateAttached("cpmCompHbFail", CL_OSAL_SCHED_OTHER, priority, stackSize,
                          compHBProcess, hbFailedComp, pTaskId);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_TASK_CREATE_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

void compMgrHealthFeedBack(ClRcT retCode,
                           void *pCookie,
                           ClBufferHandleT sendMsg,
                           ClBufferHandleT recvMsg)
{
    ClIocNodeAddressT myOMAddress;
    ClCpmSchedFeedBackT outBuffer;
    time_t t1;
    ClCharT buffer[CL_MAX_NAME_LENGTH] = {0};
    ClRcT rc = CL_OK;
    ClCpmEOListNodeT *pThis = NULL;
    ClEoIdT eoId = 0;
    ClOsalTaskIdT hbFailThreadTaskId = 0;
    
    eoId = *(ClEoIdT*)pCookie;
    clHeapFree(pCookie);

    rc = cpmEoPtrFind(eoId, &pThis);
    if (rc == CL_OK)
    {
        CL_ASSERT((ClCpmEOListNodeT *) pThis);
        if ((ClCpmEOListNodeT *) pThis == NULL)
            CL_CPM_CHECK_0(CL_LOG_SEV_ERROR, CL_LOG_MESSAGE_0_NULL_ARGUMENT,
                    CL_CPM_RC(CL_ERR_NULL_POINTER),
                    CL_LOG_HANDLE_APP);
        /*
         * Get the local OMAddress 
         */
        myOMAddress = clIocLocalAddressGet();
	
        /*
         * FIXME: take the semaphore 
         */
        if (clOsalMutexLock(gpClCpm->eoListMutex) != CL_OK)
            CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                           CL_LOG_HANDLE_APP);

        if (CL_RMD_TIMEOUT_UNREACHABLE_CHECK(retCode))
        {
            if (((ClCpmEOListNodeT *) pThis)->status != CL_CPM_EO_DEAD)
            {
                time(&t1);
                ctime_r(&t1, buffer);
                clLogInfo(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_HB,
                         "EOId %lld EOPort 0x%x RMD timeOut %s\n",
                         ((ClCpmEOListNodeT *) pThis)->eoptr->eoID,
                         ((ClCpmEOListNodeT *) pThis)->eoptr->eoPort,
                         buffer);
                cpmEOHBFailure((ClCpmEOListNodeT *) pThis);
            }
        }
        else if (retCode == CL_OK)
        {
            rc = VDECL_VER(clXdrUnmarshallClEoSchedFeedBackT, 4, 0, 0)(recvMsg, (void *) &outBuffer);
            if (rc != CL_OK)
                CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, ("Invalid Buffer Passed \n"),
                                  CL_CPM_RC(CL_ERR_INVALID_BUFFER));

            if (outBuffer.status == CL_CPM_EO_DEAD)
            { 
                CpmheartBeatFailedCompT *hbFailedComp = NULL;

                hbFailedComp = (CpmheartBeatFailedCompT *)
                               clHeapAllocate(sizeof(CpmheartBeatFailedCompT));
                if (hbFailedComp == NULL)
                    CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR, 
                                      ("Unable to allocate memory \n"),
                                      CL_CPM_RC(CL_ERR_NO_MEMORY));

                hbFailedComp->comp = pThis->compRef;
                hbFailedComp->eoHandle = pThis;

                clOsalMutexUnlock(gpClCpm->eoListMutex);

                cpmHBFailureProcess(&hbFailThreadTaskId, hbFailedComp);

                clBufferDelete(&recvMsg);
                return;
            }
            else
            {
                if (outBuffer.freq == CL_EO_DONT_POLL)
                {
                    ((ClCpmEOListNodeT *) pThis)->dontPoll = 1;
                    ((ClCpmEOListNodeT *) pThis)->freq = 0;
                    ((ClCpmEOListNodeT *) pThis)->nextPoll = 0;
                }
                else if (outBuffer.freq == CL_EO_BUSY_POLL)
                {
                    if (((ClCpmEOListNodeT *) pThis)->freq <
                            (ClUint32T) gpClCpm->pCpmConfig->threshFreq)
                    {
                        ((ClCpmEOListNodeT *) pThis)->freq =
                            ((ClCpmEOListNodeT *) pThis)->freq * CL_CPM_EXPBKOFF_FACTOR;
                        ((ClCpmEOListNodeT *) pThis)->nextPoll =
                            ((ClCpmEOListNodeT *) pThis)->freq;
                    }
                    else
                    {
                        ((ClCpmEOListNodeT *) pThis)->freq =
                            (ClUint32T) gpClCpm->pCpmConfig->threshFreq;
                        ((ClCpmEOListNodeT *) pThis)->nextPoll =
                            ((ClCpmEOListNodeT *) pThis)->freq;
                    }
                }
                else if (outBuffer.freq == CL_EO_DEFAULT_POLL)
                {
                    pThis->freq = pThis->nextPoll =
                    gpClCpm->pCpmConfig->defaultFreq;
                }
            }
        }
        else
            CL_CPM_LOCK_CHECK(CL_LOG_SEV_ERROR,
                    ("Error in Receiving HB from EOs %x\n", retCode),
                    retCode);
    }

    /*
     * Release the semaphore 
     */
  withlock:
    clOsalMutexUnlock(gpClCpm->eoListMutex);

  failure:
    clBufferDelete(&recvMsg);
    return;
}

static ClRcT clCpmIocNotificationHandler(ClPtrT invocation)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT iocAddress = 0;
    ClIocNotificationT *notification = (ClIocNotificationT*) invocation;
    ClIocNotificationT iocNotification;
    ClIocNotificationIdT notificationId;
    ClIocPortT iocPort = 0;
    ClUint32T protoVersion = 0;
    ClInt32T nodeUp = 0;
    ClGmsNodeIdT nodeId = 0;

    memset(&iocNotification,0,sizeof(ClIocNotificationT));
    memset(&notificationId,0,sizeof(ClIocNotificationIdT));

    if(!invocation) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    memcpy(&iocNotification, notification, sizeof(iocNotification));
    clHeapFree(notification);
    protoVersion = ntohl(iocNotification.protoVersion);
    if(protoVersion != CL_IOC_NOTIFICATION_VERSION)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, "CPM got ioc notification packet with version [%d]. "
                   "Supported version [%d]", protoVersion, CL_IOC_NOTIFICATION_VERSION);
        rc = CL_CPM_RC(CL_ERR_VERSION_MISMATCH);
        return rc;
    }
    
    iocPort = ntohl(iocNotification.nodeAddress.iocPhyAddress.portId);
    iocAddress = ntohl(iocNotification.nodeAddress.iocPhyAddress.nodeAddress);    
    notificationId = (ClIocNotificationIdT) ntohl(iocNotification.id);

    CL_CPM_IOC_ADDRESS_SLOT_GET(iocAddress, nodeId);

    clOsalMutexLock(&gpClCpm->clusterMutex);

    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    nodeUp = gpClCpm->polling;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);

    if(!nodeUp)
        goto out_unlock;

    switch(notificationId)
    {
        case CL_IOC_NODE_LEAVE_NOTIFICATION:
        case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
            /*
             * Fall through !!
             */
        case CL_IOC_COMP_DEATH_NOTIFICATION:
        {
            clRmdDatabaseCleanup(NULL, &iocNotification);

            if ((iocPort == CL_IOC_DEFAULT_COMMPORT) || (iocPort == CL_IOC_CPM_PORT))
            {
                /*
                 * skip self death notification processing.
                 */
                if(nodeId == clIocLocalAddressGet())
                {
                    goto out_unlock;
                }

                if (CL_CPM_IS_ACTIVE())
                {
                    clLogMultiline(CL_LOG_SEV_CRITICAL,
                                   CPM_LOG_AREA_CPM,
                                   CPM_LOG_CTX_CPM_AMS,
                                   "CPM/G active got "
                                   "IOC notification for node [%d] -- \n"
                                   "Possible reasons for this "
                                   "are on node [%d] : \n"
                                   "1. AMF crashed. \n"
                                   "2. AMF was killed. \n"
                                   "3. Critical component failed. \n"
                                   "4. Kernel panicked. \n"
                                   "5. Communication was lost. \n"
                                   "6. AMF was shutdown.",
                                   nodeId, nodeId);
                               
                    cpmFailoverNode(nodeId, CL_FALSE);
                }
                else if (CL_CPM_IS_STANDBY() &&
                         (nodeId == gpClCpm->activeMasterNodeId) &&
                         (gpClCpm->deputyNodeId == clIocLocalAddressGet()))
                {
                    clLogCritical(CPM_LOG_AREA_CPM,
                                  CPM_LOG_CTX_CPM_AMS,
                                  "CPM/G standby got IOC "
                                  "notification for CPM/G active node [%d]",
                                  nodeId);

                    rc = cpmStandby2Active(nodeId, -1);
                }
            }
            else
            {
                if(iocAddress == clIocLocalAddressGet())
                {
                    cpmCompHeartBeatFailure(iocPort);
                }
            }
        }
        break;
        
        case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, "Node [%d] arrival received from port [%d]", nodeId,iocPort);
#if 0            
            /* If I am standby, and get an arrival event for the AMF instance I thought was active
               then it must have restarted before we got the keepalive timeout.  So I need to become active.
               This fixes the rapid restart using kill -9 (amf) and UDP keepalives

               Note: except it still has issues.  Solved temporarily by putting a restart delay in the watchdog
             */            
            if (CL_CPM_IS_STANDBY() && (nodeId == gpClCpm->activeMasterNodeId) && (gpClCpm->deputyNodeId == clIocLocalAddressGet()))
            {
                clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, "Standby got an arrival notification for what it thought was the active AMF on node [%d] -- it must have failed and restarted.  Making myself active.", nodeId);
                rc = cpmStandby2Active(nodeId, -1);
            }
#endif            
            break;
              
        case CL_IOC_NODE_LINK_UP_NOTIFICATION:
            break;            
        case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, "Component arrival from node:port [%d:%d]", nodeId,iocPort);
#if 0            
            /* If I am standby, and get an arrival event for the AMF instance I thought was active
               then it must have restarted before we got the keepalive timeout.  So I need to become active.
               This fixes the rapid restart using kill -9 (amf) and UDP keepalives

               Note: except it still has issues.  Solved temporarily by putting a restart delay in the watchdog
             */
            if ((iocPort == CL_IOC_DEFAULT_COMMPORT) || (iocPort == CL_IOC_CPM_PORT))  /* These ports indicate the AMF started up */
            {
              if (CL_CPM_IS_STANDBY() && (nodeId == gpClCpm->activeMasterNodeId) && (gpClCpm->deputyNodeId == clIocLocalAddressGet()))
                {
                    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, "Standby got arrival notification for expected active AMF on node [%d] -- it must have failed and restarted.  Making myself active.", nodeId);
                    /* clAmsPeNodeHasLeftCluster(node,CL_FALSE); */
                    rc = cpmStandby2Active(nodeId, -1);
                }
            }
#endif            
            break;
            
        default:
        {
            clLogError(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED, "AMF got unknown IOC notification id [%d], ignoring...",notificationId);
        }
    }

    out_unlock:
    clOsalMutexUnlock(&gpClCpm->clusterMutex);
    return rc;
}

static ClRcT clCpmIocNotificationEnqueue(ClIocNotificationT *notification, ClPtrT cookie)
{
    ClIocNotificationT *job = NULL;
    ClRcT rc = CL_OK;
    if(!notification) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    job = (ClIocNotificationT*) clHeapCalloc(1, sizeof(*job));
    CL_ASSERT(job != NULL);
    memcpy(job, notification, sizeof(*job));
    rc = clJobQueuePush(&cpmNotificationQueue, clCpmIocNotificationHandler, (ClPtrT)job);
    if(rc != CL_OK)
    {
        clLogError("IOC", "NOT", "CPM IOC notification enqueue returned with [%#x]", rc);
        clHeapFree(job);
    }
    return rc;
}

/*
 * Unused now because of the above fast interface through notifications
 */
static ClRcT clCpmIocNotification(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr)
{
    ClIocNotificationT notification;
    ClUint32T len = sizeof(notification);

    memset(&notification,0,sizeof(ClIocNotificationT));
    clBufferNBytesRead(eoRecvMsg, (ClUint8T*)&notification, &len);

    notification.id = (ClIocNotificationIdT) ntohl(notification.id);
    notification.nodeAddress.iocPhyAddress.nodeAddress = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
    notification.nodeAddress.iocPhyAddress.portId = ntohl(notification.nodeAddress.iocPhyAddress.portId);

    /*
     * If GMS is still booting up, problem reported leader never reach on it.
     * So, move below code from GMS to CPM.
     */
    if (notification.id == CL_IOC_NODE_ARRIVAL_NOTIFICATION
            && notification.nodeAddress.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
    {
        len = length - sizeof(notification);
        if (len == sizeof(ClUint32T))
        {
#if 0
            ClUint32T currentLeader = 0;
            ClRcT rc = clBufferHeaderTrim(eoRecvMsg, sizeof(notification));
            if (rc == CL_OK)
            {
                clBufferNBytesRead(eoRecvMsg, (ClUint8T*)&currentLeader, &len);
                currentLeader = ntohl(currentLeader);
                clLogDebug("GMS", "LEA", "Update current leader [%d]", currentLeader);
                clNodeCacheLeaderUpdate(currentLeader);
            }
#endif
            ClUint32T reportedLeader = 0;
            len = sizeof(ClUint32T);
            ClIocNodeAddressT currentLeader;
            clBufferNBytesRead(eoRecvMsg, (ClUint8T*)&reportedLeader, &len);
            reportedLeader = ntohl(reportedLeader);
            if (clNodeCacheLeaderGet(&currentLeader) == CL_OK)
            {
                if (currentLeader == reportedLeader)
                {
                    clLogDebug("NTF", "LEA", "Node [%d] reports leader as [%d].  Consistent with this node.", currentLeader,
                                    reportedLeader);
                }
                else
                {
                    clLogAlert("NTF", "LEA", "Split brain.  Node [%d] reports leader as [%d]. Inconsistent with this node's leader [%d]",
                                    notification.nodeAddress.iocPhyAddress.nodeAddress, reportedLeader, currentLeader);
                    clNodeCacheLeaderUpdate(reportedLeader);

                    /* Only update leaderID if msg come from SC's leader */
                    if (clCpmIsSC() && (reportedLeader == notification.nodeAddress.iocPhyAddress.nodeAddress))
                    {
                        /* Gas: take new leader and try register level 3 */
                        clNodeCacheLeaderSend(reportedLeader);
                        ClIocAddressT allNodeReps;
                        allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
                        allNodeReps.iocPhyAddress.portId = CL_IOC_XPORT_PORT;
                        static ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
                        ClUint32T myCapability = 0;
                        ClIocNotificationT notification;
                        notification.id = (ClIocNotificationIdT) htonl(CL_IOC_NODE_LEAVE_NOTIFICATION);
                        notification.nodeVersion = htonl(nodeVersion);
                        notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(clIocLocalAddressGet());
                        notification.nodeAddress.iocPhyAddress.portId = htonl(myCapability);
                        notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);  // htonl(1);
                        clIocNotificationPacketSend(pThis->commObj, &notification, &allNodeReps, CL_FALSE, NULL );
                    }
                }
            }
            else
            {
                clLogDebug("NTF", "LEA", "Node [%d] reports leader as [%d].", notification.nodeAddress.iocPhyAddress.nodeAddress,
                                reportedLeader);
                clNodeCacheLeaderSet(reportedLeader);
            }
        }
    }

    if(eoRecvMsg)
        clBufferDelete(&eoRecvMsg);
    return CL_OK;
}

/**
 *  Name: compMgrPollThread 
 *
 *  This function periodically pings all the EOs for health check purpose
 *
 *  @param NULL 
 *
 *  @returns 
 *    CL_OK                    - everything is ok <br>
 */

static ClRcT compMgrPollThread(void)
{
    ClCpmEOListNodeT *ptr = NULL;

    /*
     * ClCpmSchedFeedBackT *pOutBuffer = NULL;
     */
    ClCpmSchedFeedBackT outBuffer;
    ClUint32T outLen = (ClUint32T) sizeof(ClCpmSchedFeedBackT);
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeOut;
    ClTimerTimeOutT heartbeatWait = { 0, 0};
    ClUint32T freq = gpClCpm->pCpmConfig->defaultFreq, cpmCount;
    ClIocNodeAddressT myOMAddress = { 0 };
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T compCount = 0;
    ClCpmLT *cpmInfo = NULL;

    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,"Inside compMgrPollThread \n");

    rc = clEoMyEoIocPortSet(gpClCpm->cpmEoObj->eoPort);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR, rc,
                   rc, CL_LOG_HANDLE_APP);
    rc = clEoMyEoObjectSet(gpClCpm->cpmEoObj);
    CL_CPM_CHECK_1(CL_LOG_SEV_ERROR, CL_CPM_LOG_1_EO_MY_OBJ_SET_ERR, rc, rc,
                   CL_LOG_HANDLE_APP);

    /*
     * Get the local OMAddress 
     */
    myOMAddress = clIocLocalAddressGet();

    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    restart_heartbeat:
    /* This code heartbeats all the other processes in the system.  This is now handled by the notification layer in IOC.
       So enableHeartbeat is FALSE by default.
     */
    while (gpClCpm->polling && gpClCpm->enableHeartbeat == CL_TRUE)
    {
        clOsalMutexUnlock(&gpClCpm->heartbeatMutex);
        compCount = gpClCpm->noOfComponent;
        rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, NULL,
                       CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR, "component", rc);
            clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,"Get First component %x\n", rc);
        }

        /*
         * For every component 
         */
        while (compCount && rc == CL_OK)
        {
            /*
             * CL_DEBUG_PRINT(CL_LOG_SEV_TRACE, ("Inside Polling while loop for
             * local components %d\n", compCount));
             */
            rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                      (ClCntDataHandleT *) &comp);
            if (rc != CL_OK)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, NULL,
                           CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc);
                 clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                            "Unable to Get Node  Data %x\n", rc);
            }

            if ((rc = clOsalMutexLock(gpClCpm->eoListMutex)) != CL_OK)
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, NULL,
                           CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc);

            ptr = comp->eoHandle;
            /*
             * For every EO in a component, only if it component is started
             * properly 
             */
            while (comp->compPresenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED
                   && ptr != NULL)
            {
                if (!ptr->dontPoll && ptr->status != CL_CPM_EO_DEAD)
                {
                    if (ptr->nextPoll <= freq)
                    {
                        ClEoIdT *pCookie = (ClEoIdT*) clHeapCalloc(1, sizeof(ClEoIdT));
                        CL_ASSERT(pCookie != NULL);
                        *pCookie = ptr->eoptr->eoID;
                        memset(&outBuffer, 0,
                               (size_t) sizeof(ClCpmSchedFeedBackT));

                        rc = CL_CPM_CALL_RMD_ASYNC_NEW(myOMAddress,
                                                       ptr->eoptr->eoPort,
                                                       CL_EO_IS_ALIVE_COMMON_FN_ID,
                                                       NULL,
                                                       0,
                                                       (ClUint8T *) &outBuffer,
                                                       &outLen,
                                                       CL_RMD_CALL_NEED_REPLY,
                                                       0,
                                                       0,
                                                       1,
                                                       pCookie,
                                                       compMgrHealthFeedBack,
                                                       MARSHALL_FN(ClEoSchedFeedBackT, 4, 0, 0));
                        if ((CL_GET_ERROR_CODE(rc) ==
                             CL_IOC_ERR_COMP_UNREACHABLE ||
                             CL_GET_ERROR_CODE(rc) ==
                             CL_IOC_ERR_HOST_UNREACHABLE) &&
                            CL_GET_CID(rc) == CL_CID_IOC)
                        {
                            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG, NULL,
                                       CL_LOG_MESSAGE_2_COMMUNICATION_FAILED,
                                       "FIXME", rc);
                             clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                                        "HOST/Port is unreachable\n");
                            cpmEOHBFailure(ptr);
                        }
                        else if (rc != CL_OK)
                        {
                            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_DEBUG,
                                       CL_CPM_CLIENT_LIB,
                                       CL_CPM_LOG_1_RMD_CALL_ERR, rc);
                             clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                                        "Error occured while sending HB to local EO %x\n",
                                        rc);
                        }

                        ptr->nextPoll = ptr->freq;
                    }
                    else
                    {
                        ptr->nextPoll = (ptr->nextPoll) - freq;
                    }
                }
                ptr = ptr->pNext;
            }
            /*
             * Release the semaphore 
             */
            clOsalMutexUnlock(gpClCpm->eoListMutex);
            compCount--;
            if (compCount)
            {
                rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
                if (rc != CL_OK)
                     clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                                "Unable to Get Node  Data %x\n", rc);
            }
            /*
             * Periodically collect the Zoombie child processes if any 
             */
            {
                pid_t pid;
                ClInt32T w;

                pid = waitpid(WAIT_ANY, &w, WNOHANG);
            }
        }
        /*
         * Do HealthCXheck of CPM/Ls 
         */
        if ( CL_CPM_IS_ACTIVE() && gpClCpm->noOfCpm)
        {
            clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                       "Inside Polling while loop for CPMs %d\n",
                       gpClCpm->noOfCpm);
            cpmCount = gpClCpm->noOfCpm;
            rc = clCntFirstNodeGet(gpClCpm->cpmTable, &hNode);
            if (rc != CL_OK)
                 clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                            "Unable to Get First CPM %x\n", rc);
            while (cpmCount)
            {
                rc = clCntNodeUserDataGet(gpClCpm->cpmTable, hNode,
                                          (ClCntDataHandleT *) &cpmInfo);
                CL_CPM_CHECK_1(CL_LOG_SEV_ERROR,
                               CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc, rc,
                               CL_LOG_HANDLE_APP);
                if ((cpmInfo->pCpmLocalInfo) &&
                    (cpmInfo->pCpmLocalInfo->cpmAddress.nodeAddress !=
                     gpClCpm->pCpmLocalInfo->cpmAddress.nodeAddress) &&
                    (cpmInfo->pCpmLocalInfo->status != CL_CPM_EO_DEAD))
                {
                    clLogTrace(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_HB,
                               "Sending HB to CPMs of node: %s\n",
                               cpmInfo->nodeName);

                    memset(&outBuffer, 0, (size_t) sizeof(ClCpmSchedFeedBackT));
                    rc = CL_CPM_CALL_RMD_ASYNC_NEW(cpmInfo->pCpmLocalInfo->
                                                   cpmAddress.nodeAddress,
                                                   cpmInfo->pCpmLocalInfo->
                                                   cpmAddress.portId,
                                                   CL_EO_IS_ALIVE_COMMON_FN_ID,
                                                   NULL,
                                                   0,
                                                   (ClUint8T *) &outBuffer,
                                                   &outLen,
                                                   CL_RMD_CALL_NEED_REPLY,
                                                   0,
                                                   0,
                                                   1,
                                                   (void *) (cpmInfo), 
                                                   cpmLocalHealthFeedBack,
                                                   MARSHALL_FN(ClEoSchedFeedBackT, 4, 0, 0));
                    if ((CL_GET_ERROR_CODE(rc) ==
                         CL_IOC_ERR_COMP_UNREACHABLE ||
                         CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE ||
                         CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
                        && CL_GET_CID(rc) == CL_CID_IOC)
                    {
                         clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                                    "HOST is unreachable so handle it as a failure \n");
                        cpmCpmLHBFailure(cpmInfo->pCpmLocalInfo);
                    }
                    else if (rc != CL_OK)
                         clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                                    "Error occured while sending HB to other CPM %x\n",
                                    rc);
                }
                cpmCount--;
                if (cpmCount)
                {
                    rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
                    if (rc != CL_OK)
                         clLogError(CPM_LOG_AREA_CPM,CPM_LOG_CTX_CPM_MGM,
                                    "Unable to Get Node  Data %x\n", rc);
                }
            }
        }
        timeOut.tsSec = 0;
        timeOut.tsMilliSec = freq;
        clOsalTaskDelay(timeOut);
        clOsalMutexLock(&gpClCpm->heartbeatMutex);
    }
    /* 
     * We come out with lock held
     */
    if(!gpClCpm->polling)
    {
        goto out_unlock;
    }
    /* This is woken up when AMF stop occurs */
    clOsalCondWait(&gpClCpm->heartbeatCond, &gpClCpm->heartbeatMutex,heartbeatWait);
    goto restart_heartbeat;

    out_unlock:
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);

    cpmWriteNodeStatToFile("AMS", CL_NO);
    cpmWriteNodeStatToFile("CPM", CL_NO);

    clLogTrace(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED, 
               "Out of polling loop, gpClCpm->polling = [%d]\n", 
               gpClCpm->polling);

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, NULL,
               CL_CPM_LOG_0_SERVER_POLL_THREAD_EXIT_INFO);
    /*
     * clOsalPrintf("Polling Thread Exited ....... \n");
     */

    return rc;

failure:
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, NULL,
               CL_CPM_LOG_0_SERVER_POLL_THREAD_EXIT_INFO);
    return rc;
}

void cpmShutdownHeartbeat(void)
{
    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    gpClCpm->polling = 0;
    /*
     * Broadcast "EXIT" message to other nodes to ignore health check this node
     */
    ClIocPhysicalAddressT compAddr;
    compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    compAddr.portId = CL_IOC_XPORT_PORT;
    clIocHeartBeatMessageReqRep(gpClCpm->cpmEoObj->commObj, (ClIocAddressT *) &compAddr, CL_IOC_PROTO_ICMP, CL_TRUE);
    clOsalCondSignal(&gpClCpm->heartbeatCond);
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);
}

void cpmEnableHeartbeat(void)
{
    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    if(gpClCpm->enableHeartbeat == CL_FALSE)
    {
        gpClCpm->enableHeartbeat = CL_TRUE;
        clOsalCondSignal(&gpClCpm->heartbeatCond);
    }
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);
}

void cpmDisableHeartbeat(void)
{
    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    gpClCpm->enableHeartbeat = CL_FALSE;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);
}

ClBoolT cpmStatusHeartbeat(void)
{
    ClBoolT heartbeatRunning = CL_FALSE;
    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    heartbeatRunning = gpClCpm->enableHeartbeat;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);
    
    return heartbeatRunning;
}

static void printUsage(char *progname)
{
    clOsalPrintf("Usage: %s -c <Chassis ID> -l <Local Slot ID> "
                 "-n <Node Name> -f\n", progname);
    clOsalPrintf("Example : %s -c 0 -l 1 -n node_0\n", progname);
    clOsalPrintf("or\n");
    clOsalPrintf("Example : %s --chassis=0 --localslot=1"
                 " --nodename=<nodeName>\n", progname);
    clOsalPrintf("Options:\n");
    clOsalPrintf("-c, --chassis=ID       Chassis ID\n");
    clOsalPrintf("-l, --localslot=ID     Local slot ID\n");
    clOsalPrintf("-n, --nodename=name    Node name\n");
    clOsalPrintf("-f, --foreground       Run AMF as foreground process\n");
    clOsalPrintf("-h, --help             Display this help and exit\n");
}

static ClRcT cpmStrToInt(const ClCharT *str, ClUint32T *pNumber)
{
    ClUint32T i = 0;

    for (i = 0; str[i] != '\0'; ++i) 
    {
        if (!isdigit(str[i])) 
        {
            goto not_valid;
        }
    }

    *pNumber = atoi(str);

    return CL_OK;

not_valid:
    return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT clCpmParseCmdLine(int argc, char *argv[])
{
    ClInt32T option = 0;
    ClInt32T nargs = 0;
    ClInt32T argArray[4] = {0, 0, 0, 0};
    ClRcT rc = CL_OK;
    
    const ClCharT *short_options = ":c:l:m:n:p:fh";

#ifndef POSIX_BUILD
    const struct option long_options[] = {
        {"chassis",     1, NULL, 'c'},
        {"localslot",   1, NULL, 'l'},
        {"nodename",    1, NULL, 'n'},
        {"profile",     1, NULL, 'p'},
        {"foreground",  0, NULL, 'f'},
        {"help",        0, NULL, 'h'},
        { NULL,         0, NULL,  0 }
    };
#endif
    


#ifndef POSIX_BUILD
    while((option = getopt_long(argc, argv, short_options,
                    long_options, NULL)) != -1)
#else
    while((option = getopt(argc, argv, short_options)) != -1)
#endif
    {
        switch(option)
        {
            case 'h':   
            {
                printUsage(argv[0]);
                /* This is not an error */
                exit(0);
            }
            break;
            case 'c':   
            {
                rc = cpmStrToInt(optarg, &myCh);
                if (CL_OK != rc) 
                {
                    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                               "[%s] is not a valid chassis id, "
                               "rc = [%#x]", optarg, rc);
                    goto failure;
                }
                
                /*
                 * Bug 4645:
                 * As of now, 0 is the only valid chassis id.
                 */
                if (0 != myCh) 
                {
                    rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
                    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                               "Valid chassis id is 0, multiple "
                               "chassis are not supported. "
                               "[%d] is not a valid chassis id, "
                               "rc = [%#x]", myCh, rc);
                    goto failure;
                }
                ++nargs;
                argArray[0] = 1;
            }
            break;
            case 'l':   
            {
                rc = cpmStrToInt(optarg, &mySl);
                if (CL_OK != rc) 
                {
                    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                               "[%s] is not a valid slot id, "
                               "rc = [%#x]", optarg, rc);
                    goto failure;
                }
                
                ++nargs;
                argArray[1] = 1;
            }
            break;
            case 'n':
            {
                strncpy(clCpmNodeName, optarg, CL_MAX_NAME_LENGTH-1);
                ++nargs;
                argArray[2] = 1;
            }
            break;
            case 'p':
            {
                strncpy(clCpmBootProfile, optarg, CL_MAX_NAME_LENGTH-1);
                ++nargs;
                argArray[3] = 1;
            }
            break;
            case 'f':
            {
                cpmIsForeground = CL_TRUE;
                ++nargs;
            }
            break;
            case ':':
            {
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                           "Option [%c] needs an argument", optopt);
                goto failure;
            }
            break;
            case '?':
            {
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                           "Unknown option [%c]", optopt);
                goto failure;
            }
            break;
            default :   
            {
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                           "Unknown error");
                goto failure;
            }
            break;
        }
    }

    if (!argArray[3])
    {
       /*
        * Assume a default boot profile.
        */
       strcpy(clCpmBootProfile, CL_CPM_DEFAULT_BOOT_PROFILE);
       ++nargs;
       argArray[3] = 1;
    }

    if ((nargs == 4 ||
         ((nargs == 5) &&
          (cpmIsForeground == CL_TRUE))) && 
        (argArray[0] && argArray[1] && argArray[2] && argArray[3]))

        ;
    else
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Insufficient arguments");
        goto failure;
    }

    return CL_OK;

failure:
    printUsage(argv[0]);
    return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
}

ClEoConfigT clEoConfig =
{
    CL_OSAL_THREAD_PRI_MEDIUM,
    1,
    CL_IOC_CPM_PORT,
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_APP,
    clCpmInitialize,
    NULL,
    NULL,
    cpmHealthCheck,
    NULL,
    CL_TRUE,
};

ClUint8T clEoBasicLibs[] =
{
    CL_TRUE,    /* OSAL */
    CL_TRUE,    /* Timer */
    CL_TRUE,    /* Buffer */
    CL_TRUE,    /* IOC */
    CL_TRUE,    /* RMD */
    CL_TRUE,    /* EO */
    CL_FALSE,   /* OM */
    CL_FALSE,   /* HAL */
    CL_FALSE,   /* DBAL */
};

ClUint8T clEoClientLibs[] =
{
    CL_TRUE,    /* COR */
    CL_TRUE,   /* CM */
    CL_FALSE,   /* NS */
    CL_FALSE,   /* Log */
    CL_FALSE,   /* Trace */
    CL_FALSE,   /* Diagnostics */
    CL_FALSE,   /* Txn */
    CL_FALSE,   /* NA */
    CL_FALSE,   /* Provisioning */
    CL_FALSE,   /* Alarm */
    CL_FALSE,   /* Debug */
    CL_FALSE,   /* GMS */
    CL_FALSE,   /* PM */
};

#ifndef POSIX_BUILD

static void loadAspNodeIp(const ClCharT *intf)
{
    struct ifconf ifc;
    struct ifreq *ifreq = NULL;
    ClInt32T err = 0;
    ClInt32T fd;
    ClInt32T reqs= 0;
    register ClInt32T i;
    memset(&ifc, 0, sizeof(ifc));
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(fd < 0 )
        return ;
    if(!intf)
        intf = "lo"; /* fall back to loop back */
    do
    {
        reqs += 5;
        ifc.ifc_len = reqs * sizeof(*ifreq);
        ifc.ifc_buf = (char*) realloc(ifc.ifc_buf, sizeof(*ifreq) * reqs);
        CL_ASSERT(ifc.ifc_buf != NULL);
        err = ioctl(fd, SIOCGIFCONF, &ifc);
        if(err < 0)
            goto out_free;
    } while (ifc.ifc_len == (ClInt32T) (reqs * sizeof(*ifreq)));
    
    ifreq = ifc.ifc_req;
    for(i = 0; i < ifc.ifc_len; i += sizeof(*ifreq))
    {
        if(!strncasecmp(ifreq->ifr_name, intf, strlen(intf)))
        {
            /*
             * Copy the node ip.
             */
            gAspNodeIp[0] = 0;
            strncat(gAspNodeIp, (const char*)
                    inet_ntoa(*(struct in_addr*)&( (struct sockaddr_in*)&ifreq->ifr_addr)->sin_addr),
                    sizeof(gAspNodeIp)-1);
            break;
        }
        ++ifreq;
    }

    out_free:
    free(ifc.ifc_buf);
    close(fd);

    return;
}

#else

static void loadAspNodeIp(const ClCharT *intf)
{

}

#endif

static void loadAspInstallInfo(void)
{
    const ClCharT *intf = getenv("LINK_NAME");
    const ClCharT *aspDir = getenv("ASP_DIR");
    if(!intf)
        intf = "lo";
    if(!aspDir)
        aspDir = "/root/asp";
    clCpmTargetVersionGet(gAspVersion, sizeof(gAspVersion)-1); /* load the asp version */
    loadAspNodeIp(intf); /* load the ip address of the node pertaining to link name */
    snprintf(gAspInstallInfo, sizeof(gAspInstallInfo), "interface=%s:%s,version=%s,dir=%s", intf, gAspNodeIp, gAspVersion, aspDir);
}

static ClRcT cpmMain(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClCharT cpmName[CL_MAX_NAME_LENGTH] = {0};
    loadAspInstallInfo();
    clLogCompName = (ClCharT*) "AMF"; /* Override generated eo name with a short name for our server */
    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "%s %s", CPM_ASP_WELCOME_MSG, gAspVersion);
  
    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Process [%s] started. PID [%d]", argv[0], (int)getpid());
  
    /* To make the AMF environment "look" like the rest of the components to our common libraries (like EO) we set a few
       environment variables.  These env vars are normally set by the AMF before spawning a component */
    
    sprintf(cpmName, "%s_%s", CL_CPM_COMPONENT_NAME, clCpmNodeName);
    setenv("ASP_COMPNAME", cpmName, 1);
    /*
     * The variable 'clCpmNodeName' should now be set.  We set the env var ASP_NODENAME so that log library can pick it up.
     */
    setenv("ASP_NODENAME", clCpmNodeName, 1);
    /*
     * Set the IOC address for this node.
     */
    CL_CPM_IOC_ADDRESS_GET(myCh, mySl, ASP_NODEADDR);

    if (1)
    {
        ClCharT iocAddress[12] = { 0 };
        sprintf(iocAddress, "%d", (int) ASP_NODEADDR);
        setenv("ASP_NODEADDR", iocAddress, 1);
    }

    /* For now variable was set as optional but would be good if it was set   setenv("ASP_APP_BINDIR", temp, 1); */
    clEoNodeRepresentativeDeclare(clCpmNodeName);
    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);
    rc = clEoInitialize(argc, argv);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Process [%s] exited abnormally. Exit code [%u/0x%x]", argv[0], rc, rc);
        goto failure;
    }
    
    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Process [%s] exited normally", argv[0]);

    return CL_OK;

 failure:
    return rc;
}

ClRcT cpmValidateEnv(void)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);
  
    gClSigTermRestart = clParseEnvBoolean("ASP_RESTART_SIGTERM");

    if (clParseEnvBoolean("ASP_WITHOUT_CPM") == CL_TRUE)
    {
        /* Note, Multiline logging is not available at this point since heap
           is not initialized */ 
        clLog(CL_LOG_SEV_CRITICAL, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "The environmental variable ASP_WITHOUT_CPM is set!!");
        clLog(CL_LOG_SEV_CRITICAL, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "CPM cannot proceed if this variable is set "
                       "in its environment. Please unset this variable " "to continue.");
        goto failure;
    }
    return CL_OK;
failure:
    return rc;
}

#ifndef VXWORKS_BUILD

ClInt32T main(ClInt32T argc, ClCharT *argv[], ClCharT *envp[])
{
    ClRcT rc = CL_OK;

#ifdef QNX_BUILD
    gEnvp = envp;
#endif

    rc = cpmValidateEnv();
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Validation of CPM environment failed, error [%#x]",
                   rc);
        return rc;
    }
    rc = clCpmParseCmdLine(argc, argv);
    if (rc != CL_OK)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Wrong command line arguments, error [%#x]", rc);
        return rc;
    }
    clAmsSetInstantiateCommand(argc, argv);

    /*
     * The variable 'clCpmNodeName' should now be set.  We set the env
     * var ASP_NODENAME so that log library can pick it up.
     */
    setenv("ASP_NODENAME", clCpmNodeName, 1);

    /* Code was changed so the startup script daemonizes the watchdog.  So it is unnecessary for AMF to do so */
    cpmIsForeground = 1;
    
    if (1) // (cpmIsForeground)
    {
        rc = cpmMain(argc, argv);
        if (CL_OK != rc)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Main function of CPM failed, error [%#x]", rc);
        }
    }
#if 0    
    else
    {
    ClInt32T pid = 0;
        
        pid = fork();
        if (0 == pid)
        {
#ifdef QNX_BUILD
            procmgr_daemon(EXIT_SUCCESS, (PROCMGR_DAEMON_NOCHDIR | PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL));
#else
            cpmDaemonize_step1();
#endif
            cpmDaemonize_step2();
            rc = cpmMain(argc, argv);
            if (CL_OK != rc)
            {
                clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT, "Main function of CPM failed, error [%#x]", rc);
            }
        }
        else if (0 < pid)
        {
              exit(0);
        }
        else
        {
            perror("Unable to fork");
            rc = CL_CPM_RC(CL_OSAL_ERR_OS_ERROR);
        }
    }
#endif
    return rc;
}

#else

static ClInt32T cpmTaskPriority = 200;
static ClInt32T cpmTaskOptions = VX_FP_TASK; /*supports floating point*/
static ClInt32T cpmTaskStackSize = 2 << 20 ; /*2 MB stack size*/

ClInt32T main(ClInt32T argc, ClCharT *argv[], ClCharT *envp[])
{
    ClRcT rc = CL_OK;
    
    gEnvp = environ;

    rc = cpmValidateEnv();
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Validation of CPM environment failed, error [%#x]",
                   rc);
        return rc;
    }
    
    rc = clCpmParseCmdLine(argc, argv);
    if (rc != CL_OK)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                      "Wrong command line arguments, error [%#x]",
                      rc);
        return rc;
    }
    clAmsSetInstantiateCommand(argc, argv);

    if (cpmIsForeground)
    {
        rc = cpmMain(argc, argv);
        if (CL_OK != rc)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Main function of CPM failed, error [%#x]",
                       rc);
        }
    }
    else
    {
        /*
         * task spawn is equivalent to a fork and daemonize in vxworks: it seems
         */
        pthread_mutex_lock(&mutexMain);
        if(taskSpawn("amfTask", cpmTaskPriority, cpmTaskOptions, cpmTaskStackSize, (FUNCPTR)cpmMain, 
                     argc, (int)argv, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
        {
            char msg[0x100];
            snprintf(msg, sizeof(msg), "AMF task spawn failed with error [%#x]\n", errnoGet());
        }
        pthread_cond_wait(&condMain, &mutexMain);
        pthread_mutex_unlock(&mutexMain);
    }

    return rc;
}


#endif
