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
 * ModuleName  : rmd                                                           
 * File        : clRmdMain.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains common RMD definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_RMDMAIN_H_
# define _INC_RMDMAIN_H_
# include <clDebugApi.h>
# include <clLogUtilApi.h>
# include <clEoApi.h>
# include <ipi/clRmdIpi.h>
# include <clHandleApi.h>

# ifdef __cplusplus
extern "C"
{
# endif

# define ASYNC_IOC 0

# define RMD_DEBUG 1


# define RMD_MAX_ARGS 2

# define NUMBER_OF_RECV_BUCKETS 256
# define RMD_RECV_BUCKETS_MASK (NUMBER_OF_RECV_BUCKETS -1 )
# define NUMBER_OF_SEND_BUCKETS 256
# define RMD_SEND_BUCKETS_MASK (NUMBER_OF_SEND_BUCKETS - 1)
# define NUMBER_OF_SYNC_BUCKETS  8
# define RMD_SYNC_BUCKETS_MASK (NUMBER_OF_SYNC_BUCKETS - 1)
# define CL_RMD_EO_PRIVATE_DATA_ID  CL_EO_RMD_CLIENT_COOKIE_ID+1

    /*
     * RMD flags -
     */

    /*
     *  Version Related Code
     */

# include <clVersionApi.h>

    /*
     * Latest supported Version of Rmd
     */
# define CL_RMD_VERSION {.releaseCode = CL_RELEASE_VERSION, .majorVersion = CL_MAJOR_VERSION,\
            .minorVersion = CL_MINOR_VERSION }

# define CL_RMD_VERSION_SET(version) (version).releaseCode =  CL_RELEASE_VERSION, \
                                     (version).majorVersion = CL_MAJOR_VERSION,\
                                     (version).minorVersion = CL_MINOR_VERSION

# define CL_RMD_VERSION_SET_NEW(d, s)           do {            \
    memcpy(&(d).clientVersion, (s), sizeof((d).clientVersion)); \
} while (0)


    extern ClVersionT gRmdVersionsSupported[];
    extern ClVersionDatabaseT gRmdVersionDb;

# define RMD_STAT_INC(value) do { (value)++; } while(0)

#define CL_LOG_SP(...)  __VA_ARGS__

# if RMD_DEBUG
#  define RMD_DBG(args)    clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP args)
#  define RMD_DBG1(args)   clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP args)
#  define RMD_DBG2(args)   clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP args)
#  define RMD_DBG3(args)   clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP args)
#  define RMD_DBG4(args)   clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_LOG_SP args)
# else
#  define RMD_DBG(args)   {}
#  define RMD_DBG1(args)  {}
#  define RMD_DBG2(args)  {}
#  define RMD_DBG3(args)  {}
#  define RMD_DBG4(args)  {}
# endif

    /*
     * local errorcode
     */
# define RMD_DUP_REQ (CL_RMD_RC(CL_ERR_DUPLICATE))

    /*
     * local flags
     */
# define RMD_CALL_MSG_ASYNC 1<<31   /* if this bit is set the params are msg
                                     * type */

    /*
     * Rmd request Data struct
     */

    typedef union ClRmdData
    {
        char payload[1];        /* in payload */
        ClUint32T args[RMD_MAX_ARGS];   /* arguments for simple RMD */
    } ClRmdDataT;




    typedef struct ClRmdHdr
    {
        ClVersionT clientVersion;
        ClUint8T protoVersion;
        ClUint32T flags;
        ClUint32T maxNumTries;
        ClUint32T callTimeout;
        ClUint64T msgId;
        ClUint32T fnID;
        struct
        {
            ClUint32T rc;       /* reply argument */
        } rmdReqRepl;
    } ClRmdHdrT;

    typedef struct ClRmdPkt
    {
        ClRmdHdrT ClRmdHdr;
        ClRmdDataT ClRmdData;
    } ClRmdPktT;

    /*
     * This object will be initialized only once when the EO will call
     * rmdobinit function.
     */

    typedef struct ClRmdSyncListNode
    {
        ClOsalCondIdT syncCond;
        ClUint64T msgId;
        ClBufferHandleT outMsg;
        ClRcT retCode;
        ClUint32T replyIsThere;
    } ClRmdSyncListNodeT;

    typedef struct ClRmdObj
    {
        ClUint64T msgId;
        ClOsalMutexIdT semaForSendHashTable;
        ClOsalMutexIdT semaForRecvHashTable;
        ClCntHandleT sndRecContainerHandle; /* send side Hash Table */
        ClCntHandleT rcvRecContainerHandle; /* Recv side Hash Table */
        ClUint32T numAtmostOnceEntry;
        ClTimeT lastAtmostOnceCleanupTime;
        ClHandleDatabaseHandleT responseCntxtDbHdl;
        ClRmdStatsT rmdStats;
    } ClRmdObjT;

    typedef struct ClRmdAsyncRecord
    {
        ClRmdPktT *rcvMsg;      /* Pointer to Rmd Message */
        ClUint32T outLen;       /* Length of the o/p buf */
        ClUint32T priority;     /* associated priority */
        ClIocAddressT destAddr;
        ClBufferHandleT sndMsgHdl;   /* Pointer to Rmd Message */
        ClTimerHandleT timerID; /* associated timer ID */
        ClBufferHandleT outMsgHdl;   /* ptr to output data to be
                                             * returned */
        ClUint32T noOfRetry;    /* No of times retries to do */
        ClUint32T timeout;      /* time to wait for the reply to come */
        void *cookie;           /* Application cookie */
        ClRmdAsyncCallbackT func;   /* Application's call back function */
    } ClRmdAsyncRecordT;

    typedef struct ClRmdSyncRecord
    {
        ClOsalCondT     syncCond;
        ClIocAddressT   destAddr;
        ClBufferHandleT outMsgHdl;
        ClRcT retVal;
    } ClRmdSyncRecordT;


    typedef struct ClRmdRecordSend
    {
        ClUint32T flags;        /* synchronous or asynchronous */
        ClUint32T hdrLen; /*rmd marshalled hdrlen*/
        union
        {
            ClRmdSyncRecordT syncRec;
            ClRmdAsyncRecordT asyncRec;
        } recType;

    } ClRmdRecordSendT;



    /*
     * RMD Record For Receive Side
     */
    typedef struct ClRmdRecvKey
    {
        ClIocPhysicalAddressT srcAddr;  /* Source ioc address */
        ClUint32T fnNumber;     /* function Id to be executed */
        ClUint64T msgId;        /* Packet Id */
    } ClRmdRecvKeyT;

    typedef struct ClRmdRecordRecv
    {
        ClRmdRecvKeyT recvKey;  /* Source ioc address */
        ClBufferHandleT msg; /* Rmd Reply */
        ClTimeT atmostOnceEntryLifeTime;  /* timeout in msec */
        ClUint32T maxTries;
    } ClRmdRecordRecvT;

    typedef struct ClRmdSyncSendContext
    {
        ClUint8T isInProgress;
        ClUint8T priority;
        ClRmdHdrT ClRmdHdr;
        ClIocPhysicalAddressT srcAddr;
        ClRmdRecordRecvT *pAtmostOnceRecord;
        ClRmdRecvKeyT atmostOnceKey;
        ClUint32T         replyType;
    } ClRmdResponeContextT;


  typedef struct ClRmdAckSendContextT { 
    ClUint8T priority;
    ClRmdHdrT ClRmdHdr;
    ClIocPhysicalAddressT srcAddr;
  } ClRmdAckSendContextT;

    ClRcT clRmdParamSanityCheck(ClBufferHandleT inMsgHdl,
                                ClBufferHandleT outMsgHdl,
                                ClUint32T flags,
                                ClRmdOptionsT *pOptions,
                                ClRmdAsyncOptionsT *pAsyncOptions);

    ClRcT clRmdCreateAndAddRmdSendRecord(ClEoExecutionObjT *pThis, 
                                         ClIocAddressT destAddr, 
                                         ClUint32T hdrLen,
                                         ClBufferHandleT outMsgHdl,
                                         ClUint32T flags, 
                                         ClRmdOptionsT *pOptions, 
                                         ClRmdAsyncOptionsT *pAsyncOptions,
                                         ClBufferHandleT message,
                                         ClUint64T msgId, ClUint32T payloadLen,
                                         ClRmdRecordSendT ** ppSendRec);


    void clRmdDumpPkt(const char *name, ClBufferHandleT msg);
    ClRcT clRmdInvoke(ClEoPayloadWithReplyCallbackT func, ClEoDataT eoArg,
                      ClBufferHandleT inMsgHdl,
                      ClBufferHandleT outMsgHdl);

    ClRcT clRmdMarshallRmdHdr(ClRmdHdrT *pRmdHdr, ClUint8T *buffer, ClUint32T *bufferLen);
    ClRcT clRmdUnmarshallRmdHdr(ClBufferHandleT msg, ClRmdHdrT *pRmdHdr, ClUint32T *hdrLen);

# ifdef __cplusplus
}
# endif

#endif                          /* _INC_RMDMAIN_H_ */
