#ifndef _EO_H_
#define _EO_H_

# ifdef __cplusplus
extern "C"
{
# endif


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
    ClBoolT      running;
    ClOsalCondT  jobQueueCond;
    ClOsalMutexT jobQueueLock;
    ClUint32T    jobCount;
    ClEoStaticJobQueueT eoStaticJobQueue[EO_STATIC_QUEUES];
} ClEoStaticQueueT;

#define EO_QUEUE_SIZE(queue)  (sizeof( (queue) ) /sizeof( (queue)[0] ))

#define CL_EO_RECV_THREAD_INDEX (1)
/* Log area and context codes are always 3 letter (left fill with _ OK) */
#define CL_LOG_EO_AREA                   "_EO"
#define CL_LOG_EO_CONTEXT_STATIC         "STA"
#define CL_LOG_EO_CONTEXT_WORKER         "WRK"
#define CL_LOG_EO_CONTEXT_PRIORITY       "PRI"
#define CL_LOG_EO_CONTEXT_RECV           "RCV"
#define CL_LOG_EO_CONTEXT_CREATE         "INI"
#define CL_LOG_EO_CONTEXT_WATERMARK      "WMH"
#define CL_LOG_EO_CONTEXT_DELETE         "DEL"


#define CL_LOG_SP(...) __VA_ARGS__

#define EO_CHECK(X, Z, retCode)\
    do \
{\
    if(retCode != CL_OK)\
    {\
        clLog(X,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP Z);\
        goto failure;\
    }\
}while(0)

#define EO_BUCKET_SZ 64

static ClRcT clEoJobQueueWMAction(ClEoExecutionObjT *, ClEoStaticQueueInfoT *, ClUint32T);
static ClRcT clEoJobQueueLogAction(ClEoExecutionObjT *, ClEoStaticQueueInfoT *, ClUint32T);

static ClRcT (*gpClEoSerialize)(ClPtrT pData);
static ClRcT (*gpClEoDeSerialize)(ClPtrT pData);

/*****************************************************************************/
/************************** EO CLient Function implementation ****************/
static ClRcT clEoSvcPrioritySet(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

static ClRcT clEoGetState(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

static ClRcT clEoSetState(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

static ClRcT clEoShowRmdStats(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

static ClRcT clEoIsAlive(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);
/*****************************************************************************/

static ClUint32T eoGlobalHashFunction(ClCntKeyHandleT key);
static ClInt32T eoGlobalHashKeyCmp(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void eoGlobalHashDeleteCallback(ClCntKeyHandleT userKey, ClCntDataHandleT userData);
static ClRcT eoAddEntryInGlobalTable(ClEoExecutionObjT *eoObj, ClIocPortT eoPort);
static ClRcT eoDeleteEntryFromGlobalTable(ClIocPortT eoPort);
static ClRcT clEoStart(ClEoExecutionObjT *pThis);


/*
 * Only RMD will use it 
 */
extern ClRcT clEoRemoteObjectUnblock(ClEoExecutionObjT *remoteEoObj);
ClRcT clEoGetRemoteObjectAndBlock(ClUint32T remoteObj, ClEoExecutionObjT **pRemoteEoObj);

static ClRcT clEoDropPkt(ClEoExecutionObjT *pThis, ClBufferHandleT eoRecvMsg, ClUint8T priority, ClUint8T protoType, ClUint32T length,
        ClIocPhysicalAddressT srcAddr);

static ClRcT clEoLogLevelSet(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);
static ClRcT clEoLogLevelGet(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);
static ClRcT clEoCustomActionIntimation(ClUint32T data, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

static ClRcT clEoPriorityQueuesInitialize(void);
ClRcT clEoPriorityQueuesFinalize(ClBoolT force);



# ifdef __cplusplus
 }
# endif

#endif
