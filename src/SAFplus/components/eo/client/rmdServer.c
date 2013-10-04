#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clLogApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocManagementApi.h>
#include <clIocErrors.h>
#include <clIocTransportApi.h>
#include <clIocUdpTransportApi.h>
#include <clXdrApi.h>
#include <clEoLibs.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif
#include "rmdServer.h"
#define RMD_SERVER_BUCKET_SZ 64
static ClIocConfigT *gpClIocConfig;
static ClOsalTaskIdT *gClRmdServerTaskId;
//static ClCntHandleT gRmdServerObjHashTable;
//static ClOsalMutexT gRmdServerObjHashTableLock;
static ClOsalMutexT gClRmdServerJobMutex;
static ClEoExecutionObjT *gpRmdServerExecutionObject;
typedef void *(*ClEoTaskFuncPtrT) (void *);
static ClHeapConfigT heapConfig;
static ClRadixTreeHandleT gAspClientIDTable;
static ClEoMemConfigT memConfig;
extern ClHeapConfigT *pHeapConfigUser;
static ClJobQueueT pQueue;
static ClEoProtoDefT gClRmdServerProtoList[CL_IOC_NUM_PROTOS];
static ClIocPortT gRmdServerIocPort;
//static ClUint32T rmdServerGlobalHashFunction(ClCntKeyHandleT key);
//static ClInt32T rmdServerGlobalHashKeyCmp(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
//static void rmdServerGlobalHashDeleteCallback(ClCntKeyHandleT userKey,
//        ClCntDataHandleT userData);
static ClRcT clRmdServerDropPkt(ClEoExecutionObjT *pThis,
        ClBufferHandleT eoRecvMsg, ClUint8T priority,
        ClUint8T protoType, ClUint32T length,
        ClIocPhysicalAddressT srcAddr);
static ClEoProtoDefT protos[] =
{
    {CL_IOC_RMD_SYNC_REQUEST_PROTO,  "RMD request",       clRmdReceiveRequest,                NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ASYNC_REQUEST_PROTO, "RMD async request", clRmdReceiveAsyncRequest,           NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ASYNC_REPLY_PROTO,   "RMD async reply",   clRmdReceiveAsyncReply,             NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_ACK_PROTO,           "RMD ACK",           clRmdAckReply,                      NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_PORT_NOTIFICATION_PROTO, "port notification", clEoProcessIocRecvPortNotification, NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },
    {CL_IOC_RMD_SYNC_REPLY_PROTO,    "RMD sync reply",    clRmdServerDropPkt,                        clRmdReceiveReply, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE},
    {CL_IOC_RMD_ORDERED_PROTO,       "RMD ordered request", clRmdReceiveOrderedRequest,       NULL, CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE },

    /*
     * {CL_IOC_EM_PROTO,"EM IOC MSG", emIocMsg}
     */
    {CL_IOC_INVALID_PROTO, "", 0, 0}
};

ClRmdConfigT rmdConfig =
{
    0,
    (CL_EO_USER_CLIENT_ID_START + 0)
};
extern ClRcT clEoClientCallbackDbInitialize(void);
void clRmdServerProtoInstall(ClEoProtoDefT* def)
{

    if (gClRmdServerProtoList[def->protoID].protoID != CL_IOC_INVALID_PROTO)
    {
        clDbgCodeError(CL_OK, ("Protocol %d already has a handler. Do not install protocols twice.  Use clRmdServerProtoSwitch to change.",def->protoID));
    }

    clRmdServerProtoSwitch(def);
}

void clRmdServerProtoSwitch(ClEoProtoDefT* def)
{
    if (def->protoID == CL_IOC_INVALID_PROTO)
    {
        clDbgCodeError(CL_OK, ("Protocol %d is invalid.  Do not use it.",def->protoID));
    }
    clLogInfo("RMDSERVER", "TASKS","install protocol %d" , (int)def->protoID);
    gClRmdServerProtoList[def->protoID] = *def;
}

static ClRcT clRmdServerDropPkt(ClEoExecutionObjT *pThis,
        ClBufferHandleT eoRecvMsg, ClUint8T priority,
        ClUint8T protoType, ClUint32T length,
        ClIocPhysicalAddressT srcAddr)
{
    clBufferDelete(&eoRecvMsg);
    clLogInfo("RMDSERVER", "clRmdServerDropPkt","\n  Unknown Protocol ID\n");
    return CL_OK;
}

void clRmdServerProtoUninstall(ClUint8T protoid)
{
    gClRmdServerProtoList[protoid].protoID = CL_IOC_INVALID_PROTO;
    snprintf((char*) gClRmdServerProtoList[protoid].name, CL_MAX_NAME_LENGTH, "protocol %d undefined", protoid);
    gClRmdServerProtoList[protoid].func    = clRmdServerDropPkt;
    gClRmdServerProtoList[protoid].flags   = 0;
}

void rmdProtoInit(void)
{
    ClInt32T i;

    for (i=0;i<CL_IOC_NUM_PROTOS;i++)
    {
        clRmdServerProtoUninstall(i);
    }

    i=0;
    for(i=0; (protos[i].protoID != CL_IOC_INVALID_PROTO); i++)
    {
        clRmdServerProtoInstall(&protos[i]);
    }
}


ClRcT clRmdServerJobHandler(ClEoJobT *pJob)
{
    ClRcT rc = CL_OK;
    //ClEoSerializeT serialize = { 0 };
    ClIocRecvParamT *pRecvParam = NULL;
    ClEoExecutionObjT *pThis = gpRmdServerExecutionObject;

    clLogInfo("RMDSERVER","clRmdServerJobHandler","clRmdServerJobHandler");

    if(pJob == NULL)
    {
        return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
    }

    if(pThis == NULL)
    {
        rc = CL_RMD_RC(CL_ERR_INVALID_STATE);
        goto done;
    }

    pRecvParam = &pJob->msgParam;
    if(pRecvParam == NULL)
    {
    	clLogError("RMDSERVER", "clRmdServerJobHandler"," pRecvParam is NULL");
        goto done;
    }
    if(gClRmdServerProtoList[pRecvParam->protoType].func == NULL)
    {
    	clLogError("RMDSERVER", "clRmdServerJobHandler"," gClRmdServerProtoList[pRecvParam->protoType].func is NULL");
        goto done;
    }
    clLogInfo("RMDSERVER", "clRmdServerJobHandler"," Invoking Callback Function : protoID [%d] -- name [%s] ",gClRmdServerProtoList[pRecvParam->protoType].protoID,gClRmdServerProtoList[pRecvParam->protoType].name);
    //call function base on protoType
    rc = gClRmdServerProtoList[pRecvParam->protoType].func(pThis,
                                                    pJob->msg,
                                                    pRecvParam->priority,
                                                    pRecvParam->protoType,
                                                    pRecvParam->length,
                                                    pRecvParam->srcAddr.iocPhyAddress);
    if (rc != CL_OK)
    {
        clLogError("RMDSERVER", "clRmdServerJobHandler","Invoking Callback Failed, rc=0x%x\n", rc);
    }


 done:
    clHeapFree(pJob);
    return rc;
}



ClRcT clRmdServerEnqueueJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
    ClEoJobT *pJob = NULL;
    ClUint32T priority ;

    if(recvMsg == CL_HANDLE_INVALID_VALUE
       ||
       !pRecvParam)
    {
    	clLogError("RMDSERVER", "clRmdServerEnqueueJob","Error: Invalid param");
        goto out;
    }
    rc = CL_RMD_RC(CL_ERR_NO_MEMORY);
    pJob = clHeapCalloc(1, sizeof(*pJob));
    if(pJob == NULL)
    {
    	clLogError("RMDSERVER", "clRmdServerEnqueueJob","Error: Allocation error");
        goto out;
    }
    pJob->msg = recvMsg;
    memcpy(&pJob->msgParam, pRecvParam, sizeof(pJob->msgParam));

    /*
     * Take the global job mutex. We would only take this in priqueue finalize
     * just to protect ourselves from a parallel priqueue finalize. Other instances
     * are safe since its anyway true that EO job queue is quiesced.
     */
    rc = CL_RMD_RC(CL_ERR_INVALID_STATE);
    clOsalMutexLock(&gClRmdServerJobMutex);
    rc = CL_RMD_RC(CL_ERR_NOT_EXIST);
    ClJobQueueT* pQ = NULL;
    pQ = &pQueue;
    if(pQ == NULL)
    {
        clOsalMutexUnlock(&gClRmdServerJobMutex);
        goto out_free;
    }

    clLogInfo("RMDSERVER","clRmdServerEnqueueJob","Enqueuing job priority %d",priority);
    rc = clJobQueuePush(pQ,(ClCallbackT) clRmdServerJobHandler, pJob);
    clOsalMutexUnlock(&gClRmdServerJobMutex);

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
#define CL_RMD_MEM_TRIES  (0x3)
#define CL_RMD_MEM_TIMEOUT (500)

static ClRcT clRmdServerRecvQueueProcess(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;
    //ClRcT retCode = CL_OK;
    clLogDebug("RMDSERVER", "clRmdServerRecvQueueProcess","Enter clRmdServerRecvQueueProcess");
    ClUint8T priority;
    ClUint8T protoType;
    ClUint32T length;
    ClBufferHandleT eoRecvMsg = 0;
    ClIocPhysicalAddressT srcAddr;
    ClBufferHandleT rmdRecvMsg = 0;
    ClIocRecvOptionT recvOption = {0};
    ClIocRecvParamT recvParam = {0};
    //shrinkOptions.shrinkFlags = CL_POOL_SHRINK_ALL;
    ClTimerTimeOutT timeOut;
    timeOut.tsSec = 0;
    timeOut.tsMilliSec = CL_RMD_MEM_TIMEOUT;
    ClInt32T maxTries = CL_RMD_MEM_TRIES;
    ClInt32T tries = 0;
    //ClOsalTaskIdT selfTaskId = 0;
    recvOption.recvTimeout = CL_IOC_TIMEOUT_FOREVER;
    ClOsalTaskIdT selfTaskId = 0;
    clOsalSelfTaskIdGet(&selfTaskId);
    while (1)
    {
        rc = clBufferCreate(&rmdRecvMsg);
        if (rc != CL_OK)
        {
        	clLogError("RMDSERVER", "clRmdServerRecvQueueProcess","Create Message Failed");
        	return rc;
        }
        /*
         * Block on this to recv message
         */

        if(pThis==NULL)
        {
        	clLogError("RMDSERVER", "clRmdServerRecvQueueProcess","RmdServer Server Execution Object is NULL");
        	pThis=gpRmdServerExecutionObject;
        }
        rc = clIocReceiveAsync(pThis->commObj, &recvOption, rmdRecvMsg, &recvParam);
        priority = recvParam.priority;
        srcAddr.nodeAddress = recvParam.srcAddr.iocPhyAddress.nodeAddress;
        srcAddr.portId = recvParam.srcAddr.iocPhyAddress.portId;
        protoType = recvParam.protoType;
        length = recvParam.length;
        clLogInfo("RMDSERVER", "clRmdServerRecvQueueProcess"," Receive IOC with protoType [%d]",protoType);
        if (rc == CL_OK)
        {
            /*
             *Reset maxtries and tries on success.
             */
            if(!maxTries)
            {
                maxTries = CL_RMD_MEM_TRIES;
            }
            if(tries)
            {
                tries = 0;
            }
            ClEoProtoDefT *pdef = &gClRmdServerProtoList[protoType];
            if ((pThis->state & pdef->flags))
            {
                ClRcT rc = CL_EO_ERR_ENQUEUE_MSG;
                if (pdef->nonblockingHandler) /* If a nonblocking handler function is defined, then call it */
                {
                    rc = pdef->nonblockingHandler(pThis,eoRecvMsg,priority,protoType,length,srcAddr);
                	goto retry;
                }
                /**
                 * if its blocking or if the non-blocking call indicated
                 * that the message should be enqueued
                 */
                if (CL_GET_ERROR_CODE(rc) == CL_EO_ERR_ENQUEUE_MSG)
                {
                	clRmdServerEnqueueJob(rmdRecvMsg, &recvParam);
                }
            }
            else
            {
                clBufferDelete(&rmdRecvMsg);
            }
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
                maxTries = 0;
                tries = 0;
                goto retry;
            }
            /*
             * We are done now. Giving up.
             */
            clBufferDelete(&rmdRecvMsg);
            CL_ASSERT(0);
            break;
        }
        else
        {
        	retry:
            clBufferDelete(&rmdRecvMsg);
            clLogInfo("RMDSERVER", "clRmdServerRecvQueueProcess","\n EO: clIocReceive failed");
        }
    }
    return rc;
}




static ClRcT clRmdServerServerTaskCreate(ClCharT *pTaskName, ClEoTaskFuncPtrT pTaskFunc, ClPtrT pTaskArg, ClOsalTaskIdT *pTaskId)
{
    ClRcT rc = CL_OK;
    rc = clOsalTaskCreateAttached(pTaskName, CL_OSAL_SCHED_OTHER,
                                  CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                                  pTaskFunc, NULL, pTaskId);
    if (CL_OK != rc)
    {
        clLogError("RMDSERVER", "clRmdServerServerTaskCreate","Thread creation failed for task [%s] with error [0x%x]", pTaskName, rc);
        return rc;
    }
    return rc;
}



static ClRcT clMemInitialize(void)
{
    ClRcT rc;
    heapConfig.mode = CL_HEAP_NATIVE_MODE;
    memConfig.memLimit = 0;
    if((rc = clMemStatsInitialize(&memConfig)) != CL_OK)
    {
        return rc;
    }
    pHeapConfigUser =  &heapConfig;
    if((rc = clHeapInit()) != CL_OK)
    {
        return rc;
    }
    return CL_OK;
}


ClRcT initLibServer(ClWordT _port, ClWordT maxPending, ClWordT maxThreads)
{

    ClRcT rc = CL_OK;
    rc = clIocParseConfig(NULL, &gpClIocConfig);
    if(rc != CL_OK)
    {
        clOsalPrintf("Error : Failed to parse clIocConfig.xml file. error code = 0x%x\n",rc);
        exit(1);
    }
    if ((rc = clOsalInitialize(NULL)) != CL_OK)
    {
        clLogError("RMDSERVER", "initLibServer"," OSAL initialization failed\n");
        return rc;
    }
    if ((rc = clMemInitialize()) != CL_OK)
    {
        clLogInfo("RMDSERVER", "initLibServer"," Heap initialization failed\n");
        return rc;
    }
    if ((rc = clTimerInitialize(NULL)) != CL_OK)
    {
        clLogInfo("RMDSERVER", "initLibServer"," Timer initialization failed\n");
        return rc;
    }
    if ((rc = clBufferInitialize(NULL)) != CL_OK)
    {
        clLogInfo("RMDSERVER", "initLibServer"," Buffer initialization failed\n");
        return rc;
    }
    extern ClIocConfigT pAllConfig;
    pAllConfig.iocConfigInfo.isNodeRepresentative = CL_TRUE;
    if ((rc = clIocLibInitialize(NULL)) != CL_OK)
    {
        clLogInfo("RMDSERVER", "initLibServer"," IOC initialization failed with rc = 0x%x\n", rc);
        exit(1);
    }
    /* Create 1 extra thread since it will be used to listen to the IOC port */
    clJobQueueInit(&pQueue, maxPending, maxThreads+1);
    return rc;
}





//////////////////////////////////////////////////////////////////////////////



ClRcT clRmdServerMyRmdObjectGet(ClEoExecutionObjT **ppRmdObj)
{
    CL_FUNC_ENTER();

    /*
     * Adding check for the sanity of the EO (Bug 4019).
     */
    if (gpRmdServerExecutionObject == NULL)
        return CL_RMD_RC(CL_ERR_INVALID_STATE);

    *ppRmdObj = gpRmdServerExecutionObject;

    CL_FUNC_EXIT();
    return CL_OK;
}

ClBoolT clRmdServerClientTableFilter(ClIocPortT eoPort, ClUint32T clientID)
{
    ClPtrT item = NULL;
    ClUint32T index = __RMD_CLIENT_FILTER_INDEX(eoPort, clientID);
    clRadixTreeLookup(gAspClientIDTable, index, &item);
    return item ? CL_TRUE : CL_FALSE;
}

ClRcT clRmdServerServiceIndexGet(ClUint32T func, ClUint32T *pClientID,
        ClUint32T *pFuncNo)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters
     */
    if ((pClientID == NULL) || (pFuncNo == NULL))
    {
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }

    *pClientID = func >> CL_RMD_CLIENT_BIT_SHIFT;
    *pFuncNo = func & CL_RMD_FN_MASK;

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clRmdServerClientGetFuncVersion(ClIocPortT port, ClUint32T funcId, ClVersionT *version)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;
    ClUint32T clientID = 0;
    ClUint32T funcNo = 0;
    ClEoClientTableT *client = NULL;
    ClUint32T *latestVerCode = NULL;
    ClUint32T index = 0;

    CL_ASSERT(version != NULL);

    rc = clRmdServerMyRmdObjectGet(&pThis);
    CL_ASSERT(pThis != NULL);

    rc = clRmdServerServiceIndexGet(funcId, &clientID, &funcNo);
    if (rc != CL_OK) goto out;

    if (!pThis->pClientTable) return CL_OK;

    if(!clRmdServerClientTableFilter(port, clientID))
    {
        if(port == pThis->eoPort)
            return CL_OK;
        if(!clRmdServerClientTableFilter(pThis->eoPort, clientID)) return CL_OK;
    }

    client = pThis->pClientTable + clientID;

    index = __RMD_CLIENT_TABLE_INDEX(port, funcNo);

    rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&latestVerCode);
    if (rc != CL_OK || !latestVerCode)
    {
        /*
         * Do a second lookup with own port since these could be app. specific
         * clients with exported tables.
         */
        index = __RMD_CLIENT_TABLE_INDEX(pThis->eoPort, funcNo);
        rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&latestVerCode);
        if(rc != CL_OK || !latestVerCode)
        {
            rc = CL_RMD_RC(CL_ERR_NULL_POINTER);
            goto out;
        }
        else
        {
            clLogDebug("RMDSERVER", "clRmdServerClientGetFuncVersion","Radix tree lookup for Client [%d], Function [%d] succeeded",
                       clientID, funcNo);
        }
        rc = CL_OK;
    }

    version->releaseCode = CL_VERSION_RELEASE(*latestVerCode);
    version->majorVersion = CL_VERSION_MAJOR(*latestVerCode);
    version->minorVersion = CL_VERSION_MINOR(*latestVerCode);

    clLogDebug("RMDSERVER", "clRmdServerClientGetFuncVersion","Port [%#x], Function id [%d], Client ID [%d], Version [%d.%d.%d]",
               port, funcNo, clientID, version->releaseCode, version->majorVersion, version->minorVersion);

    return CL_OK;

out:
    return rc;
}

ClRcT rmdClientTableAlloc(ClUint32T maxClients, ClEoClientTableT **ppClientTable)
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
static ClRcT rmdClientIDRegister(ClIocPortT port, ClUint32T clientID)
{
    ClRcT rc = CL_OK;
    ClUint32T *lastItem = NULL;
    ClUint32T *item = clHeapCalloc(1, sizeof(*item));
    ClUint32T index = __RMD_CLIENT_FILTER_INDEX(port, clientID);

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

static ClRcT rmdClientIDDeregister(ClIocPortT port, ClUint32T clientID)
{
    ClRcT rc = CL_OK;
    ClPtrT item = NULL;
    ClUint32T index = __RMD_CLIENT_FILTER_INDEX(port, clientID);
    rc = clRadixTreeDelete(gAspClientIDTable, index, &item);
    if(item) clHeapFree(item);
    return rc;
}

static ClRcT rmdClientTableRegister(ClEoExecutionObjT *pThis,
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

        rmdClientIDRegister(eoPort, clientID);

        client = pThis->pClientTable + clientID;

        for (j = 0; j < funTableSize; ++j)
        {
            ClUint32T funcNo = funTable[j].funId & CL_EO_FN_MASK;
            ClUint32T index = __RMD_CLIENT_TABLE_INDEX(eoPort, funcNo);
            ClUint32T *lastVersion = NULL;
            if(!funcNo || !funTable[j].version) continue;
            rc = clRadixTreeInsert(client->funTable, index,
                                   (ClPtrT)&funTable[j].version, (ClPtrT*)&lastVersion);
            if (rc != CL_OK)
            {
                clLogDebug("RMDSERVER", "rmdClientTableRegister","radix tree insert error [%#x] for fun id [%d], version [%#x]",
                           rc, funTable[j].funId, funTable[j].version);
            }
        }
    }

    return rc;
}

static ClRcT rmdClientTableDeregister(ClEoExecutionObjT *pThis,
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

        rmdClientIDDeregister(eoPort, clientID);

        client = pThis->pClientTable + clientID;

        for (j = 0; j < funTableSize; ++j)
        {
            ClUint32T funcNo = funTable[j].funId & CL_EO_FN_MASK;
            ClUint32T index = __RMD_CLIENT_TABLE_INDEX(eoPort, funcNo);
            ClPtrT item = NULL;
            if(!funcNo || !funTable[j].version) continue;
            clRadixTreeDelete(client->funTable, index, &item);
        }
    }

    return rc;
}

ClRcT clRmdServerClientTableRegister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                              ClIocPortT clientPort)
{
    ClEoExecutionObjT *pThis = NULL;
    ClRcT rc = CL_OK;

    rc = clRmdServerMyRmdObjectGet(&pThis);
    CL_ASSERT(pThis != NULL);

    clOsalMutexLock(&pThis->eoMutex);
    rc = rmdClientTableRegister(pThis, clientTable, clientPort);
    clOsalMutexUnlock(&pThis->eoMutex);

    return rc;
}

ClRcT clRmdServerClientTableDeregister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                                ClIocPortT clientPort)
{
    ClEoExecutionObjT *pThis = NULL;
    ClRcT rc = CL_OK;

    clRmdServerMyRmdObjectGet(&pThis);
    if(!pThis)
        return CL_ERR_NOT_INITIALIZED;

    clOsalMutexLock(&pThis->eoMutex);
    rc = rmdClientTableDeregister(pThis, clientTable, clientPort);
    clOsalMutexUnlock(&pThis->eoMutex);

    return rc;
}


ClRcT rmdServerTableAlloc(ClUint32T maxClients, ClEoServerTableT **ppServerTable)
{
    ClRcT rc;

    ClUint32T i;
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

ClRcT rmdServerTableFree(ClEoServerTableT **ppServerTable)
{
	ClUint32T i;
    ClEoServerTableT *clients = NULL;

    if(!ppServerTable || !(clients = *ppServerTable))
        return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);

    *ppServerTable = NULL;

    for (i = 0; i < clients->maxClients; ++i)
    {
        ClEoServerTableT *client = clients + i;
        clRadixTreeFinalize(&client->funTable);
    }

    clHeapFree(clients);
    return CL_OK;
}

ClRcT rmdClientTableFree(ClEoClientTableT **ppClientTable)
{
    register int i;
    ClEoClientTableT *clients = NULL;

    if(!ppClientTable || !(clients = *ppClientTable))
        return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);

    *ppClientTable = NULL;
    clRadixTreeFinalize(&gAspClientIDTable);

    for (i = 0; i < clients->maxClients; ++i)
    {
        ClEoClientTableT *client = clients + i;
        clRadixTreeFinalize(&client->funTable);
    }

    clHeapFree(clients);
    return CL_OK;
}

ClRcT clRmdServerClientInstallTable(ClEoExecutionObjT *pThis,
                             ClUint32T clientID,
                             ClEoDataT data,
                             ClEoPayloadWithReplyCallbackServerT *pfunTable,
                             ClUint32T nentries)
{
    //ClRcT rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);
    ClEoServerTableT *client = NULL;
    ClUint32T i;
    ClRcT rc = CL_OK;
    if (!pThis->pServerTable) return rc;

    if (clientID > pThis->maxNoClients) return rc;

    client =pThis->pServerTable + clientID;
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
                clLogError("RMDSERVER", "clRmdServerClientInstallTable","radix tree insert error [%d] for fun id [%d], version [%#x]\n", rc, funId, version);
                goto out_delete;
            }
        }
    }

    return rc;

    out_delete:
    {
    	ClUint32T j;
        for(j = 0; j < i; ++j)
        {
            ClEoPayloadWithReplyCallbackServerT *entry = pfunTable + j;
            ClUint32T funId = entry->funId & CL_EO_FN_MASK;
            ClUint32T version = entry->version;
            ClUint32T index = __CLIENT_RADIX_TREE_INDEX(funId, version);
            ClPtrT item = NULL;
            if (clRadixTreeDelete(client->funTable, index, &item) != CL_OK)
            {
                clLogError("RMDSERVER", "clRmdServerClientInstallTable","radix tree delete error for fun id [%d], "
                           "version [%#x]\n", funId, version);
            }
        }
    }
    return rc;
}

static ClRcT rmdClientInstallTables(ClEoExecutionObjT *pThis,
                                   ClEoPayloadWithReplyCallbackTableServerT *table,
                                   ClEoDataT data)
{
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!pThis->pServerTable || !table) return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);

    for(i = 0; table[i].funTable; ++i)
    {
        rc = clRmdServerClientInstallTable(pThis,
                                    table[i].clientID,
                                    data,
                                    table[i].funTable,
                                    table[i].funTableSize);
        if(rc != CL_OK)
        {
            clLogError("RMDSERVER", "rmdClientInstallTables","EO client table deploy returned [%d] for table id [%d]",rc, i);
            CL_ASSERT(0);
            break;
        }

    }
    return rc;
}

ClRcT clRmdServerClientInstallTables(ClEoExecutionObjT *pThis,
                              ClEoPayloadWithReplyCallbackTableServerT *table)
{
    return rmdClientInstallTables(pThis, table, NULL);
}


ClRcT clRmdServerClientUninstallTable(ClEoExecutionObjT *pThis,
                               ClUint32T clientID,
                               ClEoPayloadWithReplyCallbackServerT *pfunTable,
                               ClUint32T nentries)
{
    ClInt32T i;
    ClEoServerTableT *client = NULL;
    ClRcT rc = CL_RMD_RC(CL_ERR_INVALID_PARAMETER);

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


ClRcT clRmdServerClientUninstallTables(ClEoExecutionObjT *pThis,
                                ClEoPayloadWithReplyCallbackTableServerT *table)
{
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!pThis || !table) return CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    for(i = 0; table[i].funTable; ++i)
    {
        clRmdServerClientUninstallTable(pThis,
                                 table[i].clientID,
                                 table[i].funTable,
                                 table[i].funTableSize);
    }

    return rc;
}

ClRcT clRmdServerWalkWithVersion(ClEoExecutionObjT *pThis, ClUint32T func,
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
    clLogDebug("RMDSERVER", "clRmdServerWalkWithVersion","Enter clRmdServerWalkWithVersion");
    CL_FUNC_ENTER();

    /*
     * Sanity checks for function parameters
     */
    if ((pThis->pServerTable == NULL) || (version == NULL) || (pFuncCallout == NULL))
    {
    	clLogError("RMDSERVER", "clRmdServerWalkWithVersion","\n EO: Improper input parameters \n");
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }
    client = pThis->pServerTable + clientID;
    versionCode = CL_VERSION_CODE(version->releaseCode, version->majorVersion, version->minorVersion);
    index = __CLIENT_RADIX_TREE_INDEX(funcID, versionCode);
    rc = clRadixTreeLookup(client->funTable, index, (ClPtrT*)&fun);
    if (rc != CL_OK || !fun)
    {
        clLogDebug("RMDSERVER", "clRmdServerWalkWithVersion","RMD function lookup returned fn: [%p] error: [0x%x]", (void*) fun, rc);
        /*
         * Try looking up the client table for the max. version of the function supported.
         */
        ClVersionT maxSupportedVersion = {0};
        rc = clRmdServerClientGetFuncVersion(pThis->eoPort, func, &maxSupportedVersion);
        if(rc != CL_OK || !maxSupportedVersion.releaseCode)
        {
            clLogError("RMDSERVER", "clRmdServerWalkWithVersion","function id [%d] not registered in the client table for version [%#x]",funcID, versionCode);
            return CL_RMD_RC(CL_ERR_DOESNT_EXIST);
        }
        clLogDebug("RMDSERVER", "clRmdServerWalkWithVersion","Max version of function id [%d], client id [%d], supported is [%d.%d.%d]. Requested version [%d.%d.%d]",
                   funcID, clientID, maxSupportedVersion.releaseCode, maxSupportedVersion.majorVersion,
                   maxSupportedVersion.minorVersion, version->releaseCode, version->majorVersion,
                   version->minorVersion);
        if(outMsgHdl)
        {
            clXdrMarshallClVersionT((void*)&maxSupportedVersion, outMsgHdl, 0);
        }
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_VERSION_MISMATCH);
    }

    clLogTrace("RMDSERVER", "clRmdServerWalkWithVersion", "Radix tree lookup success for Function [%d], client [%d], version [%d.%d.%d]",
               funcID, clientID, version->releaseCode, version->majorVersion, version->minorVersion);

    /* TODO: Need to fill in second parameter in the following */
    rmdMetricUpdate(CL_OK, clientID, funcID, inMsgHdl, CL_FALSE);
    rc = pFuncCallout(*fun, client->data, inMsgHdl, outMsgHdl);
    rmdMetricUpdate(rc, clientID, funcID, outMsgHdl, CL_TRUE);

    CL_FUNC_EXIT();
    return rc;

}






ClRcT clRmdServerWalk(ClEoExecutionObjT *pThis, ClUint32T func,
        ClEoCallFuncCallbackT pFuncCallout,
        ClBufferHandleT inMsgHdl,
        ClBufferHandleT outMsgHdl)
{
	clLogDebug("RMDSERVER", "clRmdServerWalk","Enter clRmdServerWalk");
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
        clLogError("RMDSERVER", "clRmdServerWalk","RmdServer Execution Obj is null \n");
        CL_FUNC_EXIT();
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }

    clLogTrace("RMDSERVER","clRmdServerWalk",
               "\n RMDSERVER: Obtaining the table indices ...... \n");
    rc = clRmdServerServiceIndexGet(func, &clientID, &funcNo);
    if (rc != CL_OK)
    {
        clLogError("RMDSERVER", "clRmdServerWalk","RmdServer Service Index Get failed \n");
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
        	clLogError("RMDSERVER", "clRmdServerWalk","\n EO: Callback function FAILED in EOPort:%x rc =%d \n", rc, pThis->eoPort);
            break;
        }

        pFuncTable = pFuncTable->pNextServObj;
    }

    CL_FUNC_EXIT();
    return rc;
}


static ClRcT clRmdServerStart(ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();
    clLogDebug("RMDSERVER", "clRmdServerStart","Enter clRmdServerStart");
    /*
     * Sanity check for function parameters
     */
    if (pThis == NULL)
    {
        clLogError("RMDSERVER", "clRmdServerStart",
                   "\n EO: NULL passed for Exectuion Object\n");
        rc = CL_RMD_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }
    gClRmdServerTaskId = calloc(1, sizeof(ClOsalTaskIdT));
    if(!gClRmdServerTaskId)
    {
        clLogError("RMDSERVER", "clRmdServerStart","Error allocating memory for threads\n");
        goto eoStaticQueueInitialized;
    }

    clLogTrace("RMDSERVER", "clRmdServerStart",
               "\n EO: Spawning the required no of EO Receive Loop \n");
    /*
     * Create thread for picking up the IOC messages and putting it on user
     * level Queue
     */
    pThis->state = CL_EO_STATE_ACTIVE;
    rc = clRmdServerServerTaskCreate("clRmdServerRecvQueueProcess", (ClEoTaskFuncPtrT) clRmdServerRecvQueueProcess,
    		pThis, gClRmdServerTaskId);
    if (CL_OK != rc)
    {
        goto eoStaticQueueInitialized;
    }

    CL_FUNC_EXIT();
    return CL_OK;

    eoStaticQueueInitialized:
    if(gClRmdServerTaskId)
    {
        free(gClRmdServerTaskId);
        gClRmdServerTaskId = NULL;
    }
    failure:
    CL_FUNC_EXIT();
    return rc;
}
ClRcT clRmdServerCreate(ClRmdConfigT *pConfig, ClEoExecutionObjT **ppThis)
{
    clLogDebug("RMDSERVER", "clRmdServerCreate","Enter create Rmd Server func");
    ClRcT rc = CL_OK;
    ClEoClientObjT *pClient = NULL;
    ClTimerTimeOutT delay;
    delay.tsSec = 1;
    delay.tsMilliSec = 0;
    ClInt32T tries = 0;
    CL_FUNC_ENTER();

    /*
     * Sanity check for function parameters
     */
    if ((pConfig == NULL) || (ppThis == NULL))
    {
        clLogError("RMDSERVER", "clRmdServerCreate","ClEoExecutionObjT is null");
        rc = CL_RMD_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    *ppThis = (ClEoExecutionObjT *) clHeapCalloc(1, sizeof(ClEoExecutionObjT));
    if (*ppThis == NULL)
    {
        rc = CL_RMD_RC(CL_ERR_NO_MEMORY);
        clLogError("RMDSERVER", "clRmdServerCreate","Memory allocation failed, error [0x%x]", rc);
        goto failure;
    }
    (*ppThis)->eoPort = pConfig->reqIocPort;
    (*ppThis)->maxNoClients = pConfig->maxNoClients+1;
    rc = clOsalMutexInit(&(*ppThis)->eoMutex);
    if(rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdServerCreate","clOsalMutexInit() failed, error [0x%x]", rc);
        goto rmdPrivateDataCntAllocated;
    }

    rc = clOsalCondInit(&(*ppThis)->eoCond);
    if(rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdServerCreate","clOsalCondInit() failed, error [0x%x]", rc);
        goto rmdPrivateDataCntAllocated;
    }

    pClient =
        (ClEoClientObjT *) clHeapAllocate(sizeof(ClEoClientObjT) *
                                          ((*ppThis)->maxNoClients + 1));
    if (pClient == NULL)
    {
        rc = CL_RMD_RC(CL_ERR_NO_MEMORY);
        clLogError("RMDSERVER", "clRmdServerCreate","Memory allocation failed, error [0x%x]", rc);
        goto rmdStaticQueueInitialized;
    }

    memset(pClient, 0, sizeof(ClEoClientObjT) * ((*ppThis)->maxNoClients));
    (*ppThis)->pClient = pClient;
    rc = rmdClientTableAlloc((*ppThis)->maxNoClients, &(*ppThis)->pClientTable);
    CL_ASSERT(rc == CL_OK);
    rc = rmdServerTableAlloc((*ppThis)->maxNoClients, &(*ppThis)->pServerTable);
    CL_ASSERT(rc == CL_OK);

    rc = clEoClientCallbackDbInitialize();
    CL_ASSERT(rc == CL_OK);
    clLogInfo("RMDSERVER", "clRmdServerCreate","create comport, port [0x%u]", (ClUint32T) pConfig->reqIocPort);
    do
    {
        rc = clIocCommPortCreate((ClUint32T) pConfig->reqIocPort,
                                 CL_IOC_RELIABLE_MESSAGING, &((*ppThis)->commObj));
    } while( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST &&
             ++tries <= 5 &&
             clOsalTaskDelay(delay) == CL_OK);


    if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST )
    {
        clLogError("RMDSERVER", "clRmdServerCreate","This component instance [%s] is already running on this node.   Exiting...", ASP_COMPNAME);
        /*
         * Do necessary cleanup
         */
        goto rmdClientObjCreated;
    }
    if (rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdServerCreate","clIocCommPortCreate() failed, error [0x%x]", rc);
        /*
         * Do necessary cleanup
         */
        goto rmdClientObjCreated;
    }

    if (pConfig->reqIocPort == 0)
    {
        rc = clIocCommPortGet((*ppThis)->commObj, &((*ppThis)->eoPort));
        clLogInfo("RMDSERVER", "clRmdServerCreate","Own commObj port is [0x%d]", (int)(*ppThis)->commObj);
        if (rc != CL_OK)
        {
        	clLogError("RMDSERVER", "clRmdServerCreate","Could not get IOC communication port info, error [0x%x]",
                       rc);
            goto rmdIocCommPortCreated;
        }
    }

    rc = clIocPortNotification((*ppThis)->eoPort, CL_IOC_NOTIFICATION_DISABLE);
    if(rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdServerCreate","Error : failed to disable the port notifications from IOC. error code [0x%x].", rc);
        exit(1);
    }
    rc = clIocPortNotification((*ppThis)->eoPort, CL_IOC_NOTIFICATION_ENABLE);
    if(rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdServerCreate","Failed to enable the port notification. error [0x%x]", rc);
        goto rmdIocCommPortCreated;
    }

    clLogInfo("RMDSERVER", "clRmdServerCreate","Own IOC port is [0x%x]", (*ppThis)->eoPort);
    rc = clRmdObjInit(&(*ppThis)->rmdObj);
    if (rc != CL_OK)
    {
    	clLogError("RMDSERVER", "clRmdCreate","clRmdObjInit() failed, error [0x%x]", rc);
        goto rmdIocCommPortCreated;
    }
    /*
     * Added the following to fix Bug 3920.
     * The Get API can fetch the values from the
     * global placeholders.
     */
    gpRmdServerExecutionObject = *ppThis;
    gRmdServerIocPort = (*ppThis)->eoPort;
    rc = clRmdServerStart(*ppThis);
    if (rc != CL_OK)
    {
        gpRmdServerExecutionObject = NULL;
        clLogError("RMDSERVER", "clRmdServerCreate","clRmdServerStart error");
        goto rmdStaticQueueInitialized;
    }
    CL_FUNC_EXIT();
    clLogInfo("RMDSERVER", "clRmdServerCreate","Rmd Sever is created");
    return CL_OK;

    rmdIocCommPortCreated:
    clIocCommPortDelete((*ppThis)->commObj);

    rmdClientObjCreated:
    clHeapFree((*ppThis)->pClient);

    rmdStaticQueueInitialized:
    rmdPrivateDataCntAllocated:

    failure:

    CL_FUNC_EXIT();
    return rc;
}
void clRmdServerCleanup(ClEoExecutionObjT *pThis)
{

    (void) clIocCommPortDelete(pThis->commObj);
    clRmdObjClose(pThis->rmdObj);
    rmdServerTableFree(&pThis->pServerTable);
    rmdClientTableFree(&pThis->pClientTable);
    clRadixTreeDestroy();
    clHeapFree((pThis)->pClient);
    clHeapFree(pThis);
    gpRmdServerExecutionObject = NULL;

}

#define MAX_PENDING 0
#define MAX_THREAD 8
ClRcT rmdSeverInit()
{

	clLogDebug("RMDSERVER", "rmdSeverInit","Enter startRmdServerSever");
	ClRcT rc;
	ClEoExecutionObjT *pThis = NULL;
//    rc = clCntHashtblCreate(RMD_SERVER_BUCKET_SZ, rmdServerGlobalHashKeyCmp,
//            rmdServerGlobalHashFunction, rmdServerGlobalHashDeleteCallback,
//            rmdServerGlobalHashDeleteCallback, CL_CNT_UNIQUE_KEY,
//            &gRmdServerObjHashTable);
//    if (rc != CL_OK)
//    {
//        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
//                ("\n RmdServer: Hash Table Creation FAILED \n"));
//        CL_FUNC_EXIT();
//        return rc;
//    }
//    rc = clOsalMutexInit(&gRmdServerObjHashTableLock);
//    if (rc != CL_OK)
//    {
//        clCntDelete(gRmdServerObjHashTable);
//        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
//                ("\n RmdServer: RmdServerHashTableLock  creation failed \n"));
//        CL_FUNC_EXIT();
//        return rc;
//    }
	rmdProtoInit();
	rc=clRmdServerCreate(&rmdConfig,&pThis);
	if(rc != CL_OK)
	{
	    clLogError("RMDSERVER", "rmdSeverInit","failed to create rmd Server");
	    return rc;
	}
	if(pThis == NULL)
	{
	    clLogError("RMDSERVER", "rmdSeverInit","failed to create rmd Server");
	    return rc;
	}
	clLogDebug("RMDSERVER", "rmdSeverInit","init queue");
	clJobQueueInit(&pQueue, MAX_PENDING, MAX_THREAD);
    return rc;
}


void clRmdServerSendErrorNotify(ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    clRmdServerEnqueueJob(message, pRecvParam);
}

ClRcT clRmdServerIocPortGet(ClIocPortT * pRmdServerIocPort)
{
    *pRmdServerIocPort = gRmdServerIocPort;
    return CL_OK;
}





//static ClRcT rmdServerAddEntryInGlobalTable(ClEoExecutionObjT *rmdServerObj,
//        ClIocPortT eoPort)
//{
//    ClRcT rc;
//
//    clOsalMutexLock(&gRmdServerObjHashTableLock);
//    rc = clCntNodeAdd(gRmdServerObjHashTable, (ClCntKeyHandleT) (ClWordT)eoPort,
//            (ClCntDataHandleT) rmdServerObj, NULL);
//    if (rc != CL_OK)
//        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
//                ("EO: Node addition Failed rc=0x%xfor eoid 0x%x\n", rc,
//                 eoPort));
//    clOsalMutexUnlock(&gRmdServerObjHashTableLock);
//
//    return rc;
//
//}
//
//
//static ClRcT rmdServerDeleteEntryFromGlobalTable(ClIocPortT eoPort)
//{
//    ClRcT rc;
//
//    clOsalMutexLock(&gRmdServerObjHashTableLock);
//    rc = clCntAllNodesForKeyDelete(gRmdServerObjHashTable, (ClCntKeyHandleT) (ClWordT)eoPort);
//    clOsalMutexUnlock(&gRmdServerObjHashTableLock);
//    if (rc != CL_OK)
//    {
//        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
//                ("RmdServer: Node delete from gRmdServerObjHashTable Failed rc=0x%x\n",
//                 rc));
//    }
//    return CL_OK;
//}
//
//ClRcT clRmdServerGetRemoteObjectAndBlock(ClUint32T remoteObj,
//        ClEoExecutionObjT **pRemoteRmdServerObj)
//{
//    ClRcT retVal;
//    ClCntNodeHandleT nodeHandle;
//
//    clOsalMutexLock(&gRmdServerObjHashTableLock);
//    retVal =
//        clCntNodeFind(gRmdServerObjHashTable, (ClCntKeyHandleT) (ClWordT)remoteObj,
//                &nodeHandle);
//    if (retVal != CL_OK)
//    {
//        clOsalMutexUnlock(&gRmdServerObjHashTableLock);
//        return retVal;
//    }
//    else
//    {
//        retVal =
//            clCntNodeUserDataGet(gRmdServerObjHashTable, nodeHandle,
//                    (ClCntDataHandleT *) pRemoteRmdServerObj);
//        if (retVal != CL_OK)
//        {
//            clOsalMutexUnlock(&gRmdServerObjHashTableLock);
//            return retVal;
//        }
//    }
//    clOsalMutexLock(&(*pRemoteRmdServerObj)->eoMutex);
//    ++(*pRemoteRmdServerObj)->refCnt;
//    clOsalMutexUnlock(&(*pRemoteRmdServerObj)->eoMutex);
//    clOsalMutexUnlock(&gRmdServerObjHashTableLock);
//
//    return CL_OK;
//}
//
//
//ClRcT clRmdServerRemoteObjectUnblock(ClEoExecutionObjT *remoteRmdServerObj)
//{
//    clOsalMutexLock(&remoteRmdServerObj->eoMutex);
//    --remoteRmdServerObj->refCnt;
//    clOsalMutexUnlock(&remoteRmdServerObj->eoMutex);
//    return CL_OK;
//}
//
//static ClUint32T rmdServerGlobalHashFunction(ClCntKeyHandleT key)
//{
//    return ((ClWordT)key % RMD_SERVER_BUCKET_SZ);
//}
//
//static ClInt32T rmdServerGlobalHashKeyCmp(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
//{
//    return ((ClWordT)key1 - (ClWordT)key2);
//}
//
//static void rmdServerGlobalHashDeleteCallback(ClCntKeyHandleT userKey,
//        ClCntDataHandleT userData)
//{
//    return;
//}
