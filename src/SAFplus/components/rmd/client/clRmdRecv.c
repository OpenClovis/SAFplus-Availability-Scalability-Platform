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
 * clRmdRecv.c
 *
 ***************************** Legal Notice ***********************************
 * File        : clRmdRecv.c
 * File        : clRmdRecv.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module implements the receive functionality of RMD.
 *****************************************************************************/

#include <netinet/in.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include "clOsalApi.h"
#include "clEoApi.h"
#include "clIocApi.h"
#include "clRmdMain.h"
#include "clRmdConfigApi.h"
#include "clDebugApi.h"
#include <string.h>
#include <clIocApiExt.h>
#include <clEoIpi.h>

#ifdef DMALLOC
# include "dmalloc.h"
#endif

#include <sys/time.h>
#define RMD_ATMOST_CLEANUP_TO_THRESH 2000

#if RMD_FILTER_CLEANUP
# define RMD_COMMPORT_CLEANUP_TO_THRESH 5*60*1000
# define RMD_COMMPORT_CLEANUP_NUM_MSG_THRESH 5
#endif

#ifdef MORE_CODE_COVERAGE
# include<clCodeCovStub.h>
#endif

#define CL_RMD_VERSION_VERIFY(header, rc) do {          \
    if( (header).protoVersion != CL_RMD_HEADER_VERSION) \
    {                                                   \
        rc = CL_RMD_RC(CL_ERR_VERSION_MISMATCH);        \
    }                                                   \
}while(0)
    

typedef struct ClRmdReplyArg
{
    ClBufferHandleT recvMsg;
    ClIocRecvParamT recvParam;
}ClRmdReplyArgT;

static ClRcT rmdHandleSyncRequest(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                                  ClIocPhysicalAddressT *srcAddr,
                                  ClUint8T priority, ClUint8T protoType,
                                  ClBufferHandleT recvMsg);

static ClRcT rmdHandleAsyncRequest(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                                   ClIocPhysicalAddressT *srcAddr,
                                   ClUint8T priority, ClUint8T protoType,
                                   ClBufferHandleT recvMsg);

static ClRcT clRmdHandleSyncReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl, ClUint32T hdrLen,
                                  ClIocPhysicalAddressT *srcAddr,
                                  ClUint8T priority, ClUint8T protoType,
                                  ClBufferHandleT outMsgHandle);

static ClRcT rmdHandleAsyncReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl, ClUint32T hdrLen,
                                 ClIocPhysicalAddressT *srcAddr,
                                 ClUint8T priority, ClUint8T protoType,
                                 ClBufferHandleT recvMsg);

static ClRcT rmdHandleAckReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl,
                                  ClIocPhysicalAddressT *srcAddr,
                                  ClUint8T priority, ClUint8T protoType,
                                  ClBufferHandleT recvMsg);

static ClRcT atmostOnceProcessing(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                                  ClIocPhysicalAddressT *srcAddr,
                                  ClUint8T priority,
                                  ClRmdRecordRecvT **recvRecord2, 
                                  ClRmdRecvKeyT *recvKey,
                                  ClBoolT *pRecordActive);

static ClRcT rmdResponseSend(ClEoExecutionObjT *pThis,
                                 ClRmdResponeContextT *pSendContext,
                                 ClBufferHandleT replyMsg, ClRcT rc,
                                 ClUint32T replyType);
                               

static ClRcT rmdAckSend(ClEoExecutionObjT *pThis,ClRmdAckSendContextT *pSendContext);

static ClRcT rmdAtmostTimerDBCleanup(ClRmdObjT *pRmdObject, ClTimeT td);

static void rmdPrepareReply(ClRmdPktT *pReq, ClRmdPktT *replyPkt);


static void resendReply(ClEoExecutionObjT *pThis, ClRmdRecordRecvT *rec,
                        ClUint8T priority);

ClRcT clRmdPrivateDataSet(void *data);
ClRcT clRmdPrivateDataGet(void **data);
extern ClUint32T clRmdPrivateDataKey;
extern ClRmdLibConfigT _clRmdConfig;

ClRcT clRmdPrivateDataSet(void *data)
{
    if (!data)
    {
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }
    return clOsalTaskDataSet(clRmdPrivateDataKey, (ClOsalTaskDataT) data);
}

ClRcT clRmdPrivateDataGet(void **data)
{
    ClOsalTaskDataT *pThreadData = (ClOsalTaskDataT *) data;
    if (!data)
    {
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }
    return clOsalTaskDataGet(clRmdPrivateDataKey, pThreadData);
}

void rmdPrepareReply(ClRmdPktT *pReq, ClRmdPktT *replyPkt)
{
    replyPkt->ClRmdHdr.protoVersion = CL_RMD_HEADER_VERSION;

    CL_RMD_VERSION_SET(replyPkt->ClRmdHdr.clientVersion);

    replyPkt->ClRmdHdr.flags = pReq->ClRmdHdr.flags;    /* not converting it as 
                                                         * it is in NW order */
    replyPkt->ClRmdHdr.msgId = pReq->ClRmdHdr.msgId;    /* not converting it as 
                                                         * it is in NW order */
    replyPkt->ClRmdHdr.callTimeout = pReq->ClRmdHdr.callTimeout;

    replyPkt->ClRmdHdr.fnID = pReq->ClRmdHdr. fnID;

    replyPkt->ClRmdHdr.rmdReqRepl.rc = 0;

}

ClRcT clRmdReceiveAsyncReply(ClEoExecutionObjT *pThis,
                             ClBufferHandleT rmdRecvMsg,
                             ClUint8T priority, ClUint8T protoType,
                             ClUint32T length, ClIocPhysicalAddressT srcAddr)
{
    ClRmdPktT msg = {{ {0} }};
    ClRcT rc = CL_OK;
    ClRmdAckSendContextT sendContext = {0};
    ClUint32T size = 0;

    clRmdDumpPkt("received async reply", rmdRecvMsg);
    RMD_DBG4(("  RMD receive Async Reply\n"));

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    if(rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message, rc 0x%x", __FUNCTION__, rc));
        return rc;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    rc = rmdHandleAsyncReply(pThis, &msg, size, &srcAddr, priority, protoType,
                             rmdRecvMsg);

    sendContext.srcAddr = srcAddr;
    sendContext.priority = priority;
    sendContext.ClRmdHdr = msg.ClRmdHdr;
    rc = rmdAckSend(pThis,&sendContext);
    if(rc != CL_OK) {
      CL_DEBUG_PRINT (CL_DEBUG_TRACE, ("Error in rmdAckSend for async. rc = 0x%x.\n",rc));
    }

#if RMD_FILTER_CLEANUP
    clRMDCheckAndCallCommPortCleanup(pThis);
#endif
    return CL_OK;
}


ClRcT clRmdReceiveAsyncRequest(ClEoExecutionObjT *pThis,
                               ClBufferHandleT rmdRecvMsg,
                               ClUint8T priority, ClUint8T protoType,
                               ClUint32T length, ClIocPhysicalAddressT srcAddr)
{
    ClRmdPktT msg = {{ {0} }};
    ClRcT rc = CL_OK;
    ClUint32T size = 0;
    
    clRmdDumpPkt("Received Async Req", rmdRecvMsg);
    RMD_DBG4(("  RMD receive Request\n"));

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    if (rc != CL_OK)            /* read result is not ok */
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message\n", __FUNCTION__));
        return CL_OK;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    rc = rmdHandleAsyncRequest(pThis, &msg, &srcAddr, priority, protoType,
                               rmdRecvMsg);
    clBufferDelete(&rmdRecvMsg);
#if RMD_FILTER_CLEANUP
    clRMDCheckAndCallCommPortCleanup(pThis);
#endif

    return CL_OK;
}


ClRcT clRmdReceiveRequest(ClEoExecutionObjT *pThis,
                          ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                          ClUint8T protoType, ClUint32T length,
                          ClIocPhysicalAddressT srcAddr)
{
    ClRmdPktT msg = {{ {0} }};
    ClRcT rc = CL_OK;
    ClUint32T size = 0;

    RMD_DBG4(("  RMD receive Request\n"));
    clRmdDumpPkt("received sync req", rmdRecvMsg);

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    if (rc != CL_OK)            /* read result is not ok */
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message\n", __FUNCTION__));
        return CL_OK;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    rc = rmdHandleSyncRequest(pThis, &msg, &srcAddr, priority, protoType,
                              rmdRecvMsg);
    clBufferDelete(&rmdRecvMsg);
#if RMD_FILTER_CLEANUP
    clRMDCheckAndCallCommPortCleanup(pThis);
#endif
    return CL_OK;
}

ClRcT rmdHandleAsyncRequest(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                            ClIocPhysicalAddressT *srcAddr, ClUint8T priority,
                            ClUint8T protoType, ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;
    ClRmdPktT replyPkt = {{ {0} }};
    ClBufferHandleT replyMsg = 0;
    ClRmdRecordRecvT *recvRecord = NULL;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClUint32T tmpFlags = pReq->ClRmdHdr.flags;
    ClBoolT recordActive = CL_FALSE;
    ClRmdResponeContextT responseContext = {0};

    responseContext.ClRmdHdr = replyPkt.ClRmdHdr;
    responseContext.priority = priority;
    responseContext.srcAddr = *srcAddr;
    responseContext.pAtmostOnceRecord = recvRecord;
    responseContext.isInProgress = CL_FALSE;
    responseContext.replyType    = CL_IOC_RMD_ASYNC_REPLY_PROTO;

    rc = clRmdPrivateDataSet((void *) &responseContext);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to set RMD private data [0x%X] \r\n",
                  __FUNCTION__, rc));
        return rc;
    }

    RMD_STAT_INC(pRmdObject->rmdStats.nRmdRequests);
    /*
     * for async call and no reply needed, no retries are there so no need of
     * atmost once
     */
    if ((tmpFlags & CL_RMD_CALL_NEED_REPLY) == 0)
    {
        ClUint32T clientID = pReq->ClRmdHdr.fnID >> CL_EO_CLIENT_BIT_SHIFT;
        clientID &= CL_EO_CLIENT_ID_MASK;

        RMD_DBG4((" RMD No Need to send the reply\n"));
        rc = clEoServiceValidate(pThis,
                                 pReq->ClRmdHdr.fnID);
        if (rc != CL_OK)
        {
            RMD_DBG2((" %s: Service Not Running RC : 0x%x\n", __FUNCTION__,
                      (rc)));
            return rc;
        }

        if(!pThis->pServerTable || !clEoClientTableFilter(pThis->eoPort,
                                                          clientID))
        {
            rc = clEoWalk(pThis, pReq->ClRmdHdr.fnID,
                          clRmdInvoke, inMsgHdl, 0);
        }
        else
        {
            ClVersionT version =  {0};
            memcpy(&version, &pReq->ClRmdHdr.clientVersion, sizeof(version));
            rc = clEoWalkWithVersion(pThis, pReq->ClRmdHdr.fnID,
                                     &version, clRmdInvoke, inMsgHdl, 0);
        }

        return CL_OK;

    }
    /*
     * At most once will be checked here 
     */
    if (tmpFlags & CL_RMD_CALL_ATMOST_ONCE)
    {
        /*
         * should we lock it
         */
        RMD_STAT_INC(pRmdObject->rmdStats.nAtmostOnceCalls);

        /*
         * not converting the msgID in host order will use it as it is
         */
        rc = atmostOnceProcessing(pThis, pReq, srcAddr, priority, &recvRecord, &responseContext.atmostOnceKey, 
                                  &recordActive);
        if (rc != CL_OK)        /* it is either dup or some other error */
        {
            if(!recordActive)
                return rc;
        }
    }

    responseContext.pAtmostOnceRecord = recvRecord; // Need to update in case of ATMOST_ONCE
    rmdPrepareReply(pReq, &replyPkt);
    rc = clBufferCreate(&replyMsg);
    if (rc != CL_OK)
    {
        RMD_DBG1((" %s: clBufferCreate Failed\n", __FUNCTION__));
        return rc;
    }

    if(!recordActive)
    {
        rc = clEoServiceValidate(pThis, pReq->ClRmdHdr.fnID);
        if (rc != CL_OK)
        {
            RMD_DBG1(("%s: Service Not Running RC : [0x%x]\n", __FUNCTION__, (rc)));
        }

    }
    else
    {
        /* remove atmost once from the flags for active record*/
        tmpFlags = replyPkt.ClRmdHdr.flags;
        tmpFlags &= ~CL_RMD_CALL_ATMOST_ONCE;
        replyPkt.ClRmdHdr.flags = tmpFlags;
        rc = CL_RMD_ERR_INUSE;
    }

    responseContext.ClRmdHdr = replyPkt.ClRmdHdr;
    
    if(rc == CL_OK)
    {
        ClUint32T clientID = pReq->ClRmdHdr.fnID >> CL_EO_CLIENT_BIT_SHIFT;
        clientID &= CL_EO_CLIENT_ID_MASK;

        if (!pThis->pServerTable
            || 
            !clEoClientTableFilter(pThis->eoPort, clientID))
        {
            rc = clEoWalk(pThis, pReq->ClRmdHdr.fnID,
                          clRmdInvoke, inMsgHdl, replyMsg);
        }
        else
        {
            ClVersionT version  = {0};
            memcpy(&version, &pReq->ClRmdHdr.clientVersion, sizeof(version));
            rc = clEoWalkWithVersion(pThis, pReq->ClRmdHdr.fnID,
                                     &version, clRmdInvoke, inMsgHdl, replyMsg);
        }
    }
    if (responseContext.isInProgress != CL_TRUE)
    {
        rmdResponseSend(pThis, &responseContext, replyMsg, rc,
                        CL_IOC_RMD_ASYNC_REPLY_PROTO);
    }

    return rc;
}

ClRcT clRmdReceiveOrderedRequest(ClEoExecutionObjT *pThis,
                               ClBufferHandleT rmdRecvMsg,
                               ClUint8T priority, ClUint8T protoType,
                               ClUint32T length, ClIocPhysicalAddressT srcAddr)
{
    ClRmdPktT msg = {{ {0} }};
    ClRcT rc = CL_OK;
    ClUint32T size = 0;

    clRmdDumpPkt("Received Ordered Req", rmdRecvMsg);
    RMD_DBG4(("  RMD Ordered Request\n"));

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    if (rc != CL_OK)            /* read result is not ok */
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message\n", __FUNCTION__));
        return CL_OK;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    if(CL_RMD_CALL_ASYNC & msg.ClRmdHdr.flags)
        rc = rmdHandleAsyncRequest(pThis, &msg, &srcAddr, priority, protoType,
                                   rmdRecvMsg);
    else
        rc = rmdHandleSyncRequest(pThis, &msg, &srcAddr, priority, protoType,
                                  rmdRecvMsg);

    clBufferDelete(&rmdRecvMsg);

    return CL_OK;
}

/*
 * Added the API to obtain the address of the sender of a RMD Request.
 */
ClRcT clRmdSourceAddressGet(ClIocPhysicalAddressT *pSrcAddr)
{
    ClRcT rc = CL_OK;

    ClRmdResponeContextT *pSendContext = NULL;

    /*
     * Validate Paramete(s) 
     */
    if (pSrcAddr == NULL)
    {
        RMD_DBG1(("%s: Passed Null for Src Address\n", __FUNCTION__));
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }

    rc = clRmdPrivateDataGet((void **) &pSendContext);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to get RMD private data [0x%X] \r\n",
                  __FUNCTION__, rc));
        return rc;
    }

    *pSrcAddr = pSendContext->srcAddr;

    return CL_OK;
}

ClRcT clRmdResponseDefer(ClRmdResponseContextHandleT *pResponseCntxtHdl)
{
    ClRcT rc = CL_OK;

    ClRmdResponeContextT *pSendContextEo = NULL;

    ClRmdResponeContextT *pSendContextHDb = NULL;

    ClEoExecutionObjT *pThis = NULL;

    ClRmdObjT *pRmdObject = NULL;

    /*
     * Validate Paramete(s) 
     */
    if (pResponseCntxtHdl == NULL)
    {
        RMD_DBG1(("%s: Passed Null for Context Handle\n", __FUNCTION__));
        return CL_RMD_RC(CL_ERR_NULL_POINTER);
    }

    rc = clEoMyEoObjectGet(&pThis);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Failed to get the EO Object [0x%x]\n", __FUNCTION__,
                  (rc)));
        return rc;
    }

    pRmdObject = (ClRmdObjT *) pThis->rmdObj;


    rc = clRmdPrivateDataGet((void **) &pSendContextEo);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to get RMD private data [0x%X] \r\n",
                  __FUNCTION__, rc));
        return rc;
    }

    rc = clHandleCreate(pRmdObject->responseCntxtDbHdl,
                        sizeof(*pSendContextHDb), pResponseCntxtHdl);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to Create the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        return rc;
    }

    rc = clHandleCheckout(pRmdObject->responseCntxtDbHdl, *pResponseCntxtHdl,
                          (void **) &pSendContextHDb);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to Checkout the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        clHandleDestroy(pRmdObject->responseCntxtDbHdl, *pResponseCntxtHdl);
        return rc;
    }

    *pSendContextHDb = *pSendContextEo;

    pSendContextEo->isInProgress = CL_TRUE;

    rc = clHandleCheckin(pRmdObject->responseCntxtDbHdl, *pResponseCntxtHdl);
    if (CL_OK != rc)
    {
        /*
         * Disaster!!! 
         */
        RMD_DBG1(("%s: Unable to Checkin the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        clHandleDestroy(pRmdObject->responseCntxtDbHdl, *pResponseCntxtHdl);
        return rc;
    }

    return CL_OK;
}

ClRcT clRmdSyncResponseSend(ClRmdResponseContextHandleT responseCntxtHdl,
                            ClBufferHandleT replyMsg, ClRcT retCode)
{
    ClRcT rc  = CL_OK;

    ClRmdResponeContextT *pSendContextHDb = NULL;

    ClEoExecutionObjT *pThis = NULL;

    ClRmdObjT *pRmdObject = NULL;
    ClUint32T  replyType  = 0;

    /*
     * Validate Paramete(s) 
     */
    if (!replyMsg)
    {
        /*
         * No reply needed. fake a reply buffer
         */
        rc = clBufferCreate(&replyMsg);
        CL_ASSERT(rc == CL_OK);
    }

    rc = clEoMyEoObjectGet(&pThis);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Failed to get the EO Object [0x%x]\n", __FUNCTION__,
                  (rc)));
        return rc;
    }

    pRmdObject = (ClRmdObjT *) pThis->rmdObj;

    rc =
        clHandleCheckout(pRmdObject->responseCntxtDbHdl, responseCntxtHdl,
                         (void **) &pSendContextHDb);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to Checkout the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        return rc;
    }

    replyType = pSendContextHDb->replyType;
    rc = rmdResponseSend(pThis, pSendContextHDb, replyMsg, retCode, 
                                  replyType);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Sending Sync Response Failed [0x%X] \r\n", __FUNCTION__,
                  rc));
        return rc;
    }

    rc = clHandleCheckin(pRmdObject->responseCntxtDbHdl, responseCntxtHdl);
    if (CL_OK != rc)
    {
        RMD_DBG1(("%s: Unable to Checkin the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        return rc;
    }

    rc = clHandleDestroy(pRmdObject->responseCntxtDbHdl, responseCntxtHdl);
    if (CL_OK != rc)
    {
        /*
         * Disaster!!! 
         */
        RMD_DBG1(("%s: Unable to Destroy the Handle [0x%X] \r\n", __FUNCTION__,
                  rc));
        return rc;
    }

    return CL_OK;
}


ClRcT rmdHandleSyncRequest(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                           ClIocPhysicalAddressT *srcAddr, ClUint8T priority,
                           ClUint8T protoType, ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;

    ClRmdPktT replyPkt = {{ {0} }};
    ClBufferHandleT replyMsg = 0;
    ClRmdRecordRecvT *recvRecord = NULL;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClUint32T flags = pReq->ClRmdHdr.flags;
    ClBoolT recordActive = CL_FALSE;
    ClRmdResponeContextT responseContext = {0};

    /*
     * should we lock it
     */
    RMD_STAT_INC(pRmdObject->rmdStats.nRmdRequests);
    /*
     * At most once will be checked here 
     */
    if (flags & CL_RMD_CALL_ATMOST_ONCE)
    {
        /*
         * should we lock it
         */
        RMD_STAT_INC(pRmdObject->rmdStats.nAtmostOnceCalls);

        /*
         * not converting the msgID in host order will use it as it is
         */
        rc = atmostOnceProcessing(pThis, pReq, srcAddr, priority, &recvRecord, &responseContext.atmostOnceKey, 
                                  &recordActive);
        if (rc != CL_OK)        /* it is either dup or some other error */
        {
            if(!recordActive)
                return rc;
        }
    }

    rmdPrepareReply(pReq, &replyPkt);
    rc = clBufferCreate(&replyMsg);
    if (rc != CL_OK)
    {
        RMD_DBG1((" %s: clBufferCreate Failed, rc[0x%x]\n", __FUNCTION__,
                  rc));
        return rc;
    }
    
    if(!recordActive)
    {
        rc = clEoServiceValidate(pThis,
                                 pReq->ClRmdHdr.fnID);
        if (rc != CL_OK)
        {
            RMD_DBG1(("%s: Service Not Running RC : [0x%x]\n", __FUNCTION__, (rc)));
        }
    }
    else
    {
        /*
         * remove atmost once flag from active record
         */
        flags = replyPkt.ClRmdHdr.flags;
        flags &= ~CL_RMD_CALL_ATMOST_ONCE;
        replyPkt.ClRmdHdr.flags = flags;
        rc = CL_RMD_ERR_INUSE;
    }

    responseContext.ClRmdHdr = replyPkt.ClRmdHdr;
    responseContext.priority = priority;
    responseContext.srcAddr = *srcAddr;
    responseContext.pAtmostOnceRecord = recvRecord;   
    responseContext.isInProgress = CL_FALSE;
    responseContext.replyType    = CL_IOC_RMD_SYNC_REPLY_PROTO;

    if(rc == CL_OK)
    {
        ClUint32T clientID = pReq->ClRmdHdr.fnID >> CL_EO_CLIENT_BIT_SHIFT;
        clientID &= CL_EO_CLIENT_ID_MASK;
        rc = clRmdPrivateDataSet((void *) &responseContext);
        if (CL_OK != rc)
        {
            RMD_DBG1(("%s: Unable to set RMD private data [0x%X] \r\n",
                      __FUNCTION__, rc));
            return rc;
        }

        if (!pThis->pServerTable
            || 
            !clEoClientTableFilter(pThis->eoPort, clientID))
        {
            rc = clEoWalk(pThis, pReq->ClRmdHdr.fnID,
                          clRmdInvoke, inMsgHdl, replyMsg);
        }
        else
        {
            ClVersionT version = {0};
            memcpy(&version, &pReq->ClRmdHdr.clientVersion, sizeof(version));
            rc = clEoWalkWithVersion(pThis, pReq->ClRmdHdr.fnID,
                                     &version, clRmdInvoke, inMsgHdl, replyMsg);
        }
    }

    if (responseContext.isInProgress != CL_TRUE)
    {
        rmdResponseSend(pThis, &responseContext, replyMsg, rc, 
                        CL_IOC_RMD_SYNC_REPLY_PROTO);
    }
    else
    {
        if(!(flags & CL_RMD_CALL_NEED_REPLY))
        {
            clBufferDelete(&replyMsg);
        }
    }
    return CL_OK;
}

static ClRcT rmdAckSend(ClEoExecutionObjT *pThis,ClRmdAckSendContextT *pSendContext) 
{
    ClBufferHandleT replyMsg = 0;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClRcT rc ;
    ClIocSendOptionT sndOption = { 0 };
    ClIocAddressT srcAddr;
    ClUint8T hdrBuffer[sizeof(ClRmdHdrT)*2] = {0};
    ClUint32T hdrLen = 0;

    RMD_STAT_INC(pRmdObject->rmdStats.nAtmostOnceAcksSent);

    rc = clBufferCreate(&replyMsg);

    if(rc != CL_OK) {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceAcksSent);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Error in message create\n"));
        return rc;
    }

    clRmdMarshallRmdHdr(&pSendContext->ClRmdHdr, hdrBuffer, &hdrLen);

    rc = clBufferNBytesWrite(replyMsg, hdrBuffer, hdrLen);

    if(rc != CL_OK) {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceAcksSent);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Error in header data prepend\n"));
        return rc;
    }

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = pSendContext->ClRmdHdr.callTimeout;
    sndOption.priority = pSendContext->priority;
    sndOption.sendType = CL_IOC_NO_SESSION;
    srcAddr.iocPhyAddress = pSendContext->srcAddr;

    clRmdDumpPkt("Sending ACK", replyMsg);

    rc = clIocSend(pThis->commObj, replyMsg,
                   CL_IOC_RMD_ACK_PROTO, &srcAddr, &sndOption);
    if (rc != CL_OK)
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceAcksSent);
        RMD_DBG1(("RMD IOC Failed to send, rc: 0x%x", rc));
    }
    rc = clBufferDelete(&replyMsg);
    return rc;
}

ClRcT rmdResponseSend(ClEoExecutionObjT *pThis,
                      ClRmdResponeContextT *pSendContext,
                      ClBufferHandleT replyMsg, 
                      ClRcT rc,
                      ClUint32T replyType)
{
    ClRcT retCode = 0;
    ClIocAddressT srcAddr;
    ClRmdRecordRecvT *recvRecord = pSendContext->pAtmostOnceRecord;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClTimeT currTime = 0;
    ClTimeT td = 0;
    ClUint32T flags = pSendContext->ClRmdHdr.flags;
    ClIocSendOptionT sndOption = { 0 };
    ClUint8T hdrBuffer[sizeof(ClRmdHdrT)*2] = {0};
    ClUint32T hdrLen = 0;

    pSendContext->ClRmdHdr.rmdReqRepl.rc = rc;

    clRmdMarshallRmdHdr(&pSendContext->ClRmdHdr, hdrBuffer, &hdrLen);
    clBufferDataPrepend(replyMsg, hdrBuffer, hdrLen);

    /*
     * Set Return Code 
     */

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = pSendContext->ClRmdHdr.callTimeout;
    sndOption.priority = pSendContext->priority;
    sndOption.sendType = CL_IOC_NO_SESSION;

    srcAddr.iocPhyAddress = pSendContext->srcAddr;

    clRmdDumpPkt("***************Sending Reply", replyMsg);

    clOsalMutexLock(pRmdObject->semaForRecvHashTable);

    RMD_STAT_INC(pRmdObject->rmdStats.nReplySend);

    rc = clIocSend(pThis->commObj, replyMsg,
                   replyType, &srcAddr, &sndOption);
    if (rc != CL_OK)
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedReplySend);
        RMD_DBG1((" RMD IOC Failed to sent RC: 0x%x", (rc)));
    }

    /*
     * Free the Reply Message 

     */
    if ((flags & CL_RMD_CALL_ATMOST_ONCE) == 0)
    {
        retCode = clBufferDelete(&replyMsg);
    }
    else
    {
        if (pRmdObject->numAtmostOnceEntry > 10)
        {
            currTime = clOsalStopWatchTimeGet();
            td = currTime - pRmdObject->lastAtmostOnceCleanupTime;
            td/=1000;
        }
        if (td > RMD_ATMOST_CLEANUP_TO_THRESH)
        {
            RMD_DBG4(("cleanup start for '%s' %d\n", pThis->name,
                      pRmdObject->numAtmostOnceEntry));
            rmdAtmostTimerDBCleanup(pRmdObject, td);
            pRmdObject->lastAtmostOnceCleanupTime = currTime;
            RMD_DBG4(("cleanup done for '%s' %d\n", pThis->name,
                      pRmdObject->numAtmostOnceEntry));

        }
    }
    if((flags & CL_RMD_CALL_ATMOST_ONCE) && recvRecord)
    {
        ClCntNodeHandleT nodeHandle = 0;
        /*
         * Because we dropped the lock, we double check for the recv
         * record because it could be deleted behind our back by a comp/node
         * death notification. triggering rmd database cleanup
         */
        if(clCntNodeFind(pRmdObject->rcvRecContainerHandle, 
                         (ClCntKeyHandleT)&pSendContext->atmostOnceKey, &nodeHandle) != CL_OK)
        {
            clBufferDelete(&replyMsg);
        }
        else
            recvRecord->msg = replyMsg;
    }
    clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);

    return rc;
}

static ClRcT rmdHandleAckReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl,
                                 ClIocPhysicalAddressT *srcAddr,
                                 ClUint8T priority, ClUint8T protoType,
                                 ClBufferHandleT outMsgHandle)
{
    ClRmdRecordRecvT recvRecord = { { {0} } };
    ClCntNodeHandleT nodeHandle = 0;
    ClRcT rc = CL_OK;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;

    RMD_STAT_INC(pRmdObject->rmdStats.nRmdReplies);
    RMD_STAT_INC(pRmdObject->rmdStats.nAtmostOnceAcksRecvd);

    if(! (pRepl->ClRmdHdr.flags & CL_RMD_CALL_ATMOST_ONCE)) {
      /*Ignore acks for normal packets as of now*/
        RMD_STAT_INC(pRmdObject->rmdStats.nBadReplies);
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceAcksRecvd);
        return rc;
    }
     
    recvRecord.recvKey.fnNumber = pRepl->ClRmdHdr.fnID;
    recvRecord.recvKey.msgId = pRepl->ClRmdHdr.msgId;
    recvRecord.recvKey.srcAddr.nodeAddress = srcAddr->nodeAddress;
    recvRecord.recvKey.srcAddr.portId = srcAddr->portId;

    clOsalMutexLock(pRmdObject->semaForRecvHashTable);

    rc = clCntNodeFind(pRmdObject->rcvRecContainerHandle,
                       (ClCntKeyHandleT) &recvRecord.recvKey, &nodeHandle);
    if (rc == CL_OK)
    {
        clCntNodeDelete(pRmdObject->rcvRecContainerHandle, nodeHandle); 
        pRmdObject->numAtmostOnceEntry--;
    } 
    else 
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nBadReplies);
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceAcksRecvd);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("%s: container node not found for atmost once key (fn:%d,id:%lld,node:%d,port:%d)\n",__FUNCTION__,recvRecord.recvKey.fnNumber,recvRecord.recvKey.msgId,recvRecord.recvKey.srcAddr.nodeAddress,recvRecord.recvKey.srcAddr.portId));
    }
    clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);
    return rc;
}

ClRcT clRmdAckReply(ClEoExecutionObjT *pThis,
                    ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                    ClUint8T protoType, ClUint32T length,
                    ClIocPhysicalAddressT srcAddr) 
{
    ClRmdPktT msg;
    ClRcT rc = CL_OK;
    ClUint32T size = 0;

    RMD_DBG4(("  RMD ACK reply"));
    clRmdDumpPkt("received ack", rmdRecvMsg);

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    if (rc != CL_OK)            /* read result is not ok */
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message", __FUNCTION__));
        return CL_OK;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    rc = rmdHandleAckReply(pThis, &msg, &srcAddr, priority, protoType,
                              rmdRecvMsg);

    clBufferDelete(&rmdRecvMsg);

    return CL_OK;
}


/*
 * Implementing the processing of Sync Replies
 */

static ClRcT clRmdProcessReply(ClEoExecutionObjT *pThis,
                               ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                               ClUint8T protoType, ClUint32T length,
                               ClIocPhysicalAddressT srcAddr)
{
    ClRmdPktT msg = { { {0} } };
    ClRcT rc = CL_OK;
    ClUint32T size = 0;
    ClRmdAckSendContextT sendContext = {0};

    clRmdDumpPkt("Received Sync Reply", rmdRecvMsg);
    RMD_DBG4(("RMD receive Sync Reply\n"));

    rc = clBufferReadOffsetSet(rmdRecvMsg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rmdRecvMsg, &msg.ClRmdHdr, &size);
    /*
     * Free this it is not needed any More
     */
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        RMD_DBG3((" %s: Bad Message, rc 0x%x", __FUNCTION__, rc));
        return CL_OK;
    }

    CL_RMD_VERSION_VERIFY(msg.ClRmdHdr, rc);
    if (rc != CL_OK)
    {
        clBufferDelete(&rmdRecvMsg);
        return CL_OK;
    }

    rc = clRmdHandleSyncReply(pThis, &msg, size, &srcAddr, priority, protoType,
                              rmdRecvMsg);

    sendContext.srcAddr = srcAddr;
    sendContext.priority = priority;
    sendContext.ClRmdHdr = msg.ClRmdHdr;
    rc = rmdAckSend(pThis,&sendContext);

    if(rc != CL_OK) {
      CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Error in rmdAckSend for sync. rc = 0x%x.\n",rc));
    }

    clBufferDelete(&rmdRecvMsg);

    return CL_OK;
}

#ifdef QNX_BUILD

static ClRcT clRmdReply(ClPtrT invocation)
{
    ClEoExecutionObjT *pThis = NULL;
    ClRmdReplyArgT *replyArg = invocation;
    ClBufferHandleT rmdRecvMsg = 0;
    ClIocPhysicalAddressT srcAddr = {0};
    ClUint8T priority = 0;
    ClUint8T protoType = 0;
    ClUint32T length = 0;

    if(!replyArg) 
        return CL_RMD_RC(CL_ERR_INVALID_PARAMETER);

    rmdRecvMsg = replyArg->recvMsg;
    priority = replyArg->recvParam.priority;
    protoType = replyArg->recvParam.protoType;
    length = replyArg->recvParam.length;
    memcpy(&srcAddr, &replyArg->recvParam.srcAddr, sizeof(srcAddr));

    clHeapFree(replyArg);
    clEoMyEoObjectGet(&pThis);
    if(!pThis)
        return CL_RMD_RC(CL_ERR_INVALID_STATE);

    return clRmdProcessReply(pThis, rmdRecvMsg, priority, protoType, length, srcAddr);
}

ClRcT clRmdReceiveReply(ClEoExecutionObjT *pThis,
                        ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                        ClUint8T protoType, ClUint32T length,
                        ClIocPhysicalAddressT srcAddr)
{
    ClRcT rc = CL_OK;
    ClRmdReplyArgT *arg =  NULL;
    arg = clHeapCalloc(1, sizeof(*arg));
    CL_ASSERT(arg != NULL);
    arg->recvParam.priority = priority;
    arg->recvParam.protoType = protoType;
    arg->recvParam.length = length;
    memcpy(&arg->recvParam.srcAddr, &srcAddr, sizeof(arg->recvParam.srcAddr));
    arg->recvMsg = rmdRecvMsg;
    rc = clEoEnqueueReplyJob(pThis, clRmdReply, (ClPtrT)arg);
    if(rc != CL_OK)
        clHeapFree(arg);
    return rc;
}

#else

ClRcT clRmdReceiveReply(ClEoExecutionObjT *pThis,
                        ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                        ClUint8T protoType, ClUint32T length,
                        ClIocPhysicalAddressT srcAddr)
{
    return clRmdProcessReply(pThis, rmdRecvMsg, priority, protoType, length, srcAddr);
}

#endif

ClRcT clRmdHandleSyncReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl, ClUint32T hdrLen,
                           ClIocPhysicalAddressT *srcAddr, ClUint8T priority,
                           ClUint8T protoType,
                           ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;
    ClRmdRecordSendT *rec = NULL;

    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;

    ClBufferHandleT outMsgHdl = 0;

    ClUint32T len = 0;

    RMD_STAT_INC(pRmdObject->rmdStats.nRmdReplies);
    rc = clOsalMutexLock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(rc == CL_OK);
    rc = clCntNodeFind(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)pRepl->ClRmdHdr.msgId,
                       &nodeHandle);
    if (rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(pRmdObject->sndRecContainerHandle, nodeHandle,
                                  (ClCntDataHandleT *) &rec);
        if (rc == CL_OK)
        {
            /*
             * pData is part of record so no need to free, it will be freed by
             * hash delete callback
             */
            if (rec)
            {
                /*
                 * Obtain the Return Code
                 */
                if(pRepl->ClRmdHdr.rmdReqRepl.rc == CL_RMD_ERR_INUSE
                    ||
                   pRepl->ClRmdHdr.rmdReqRepl.rc == CL_RMD_ERR_CONTINUE)
                {
                    clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
                    return CL_OK;
                }

                rec->recType.syncRec.retVal = pRepl->ClRmdHdr.rmdReqRepl.rc;
                
                /*
                 * copy the reply in outbuffer
                 */
                outMsgHdl = rec->recType.syncRec.outMsgHdl;

                clBufferHeaderTrim(outMsgHandle, hdrLen);

                clBufferLengthGet(outMsgHandle, &len);
                clBufferToBufferCopy(outMsgHandle, 0, outMsgHdl, len);
                clBufferReadOffsetSet(outMsgHdl, 0, CL_BUFFER_SEEK_SET);


                /*
                 * Wake the caller
                 */
                 clOsalCondSignal(&rec->recType.syncRec.syncCond);
            }
            else
            {
                RMD_DBG3(("RMD: ERROR\n"));
            }
        }
        else
        {
        }
    }
    else
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nBadReplies);
        RMD_DBG4(("%s: Node not Found \n", __FUNCTION__));
    }

    rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(rc == CL_OK);    
    
    return CL_OK;
}

static ClRcT rmdHandleAsyncReply(ClEoExecutionObjT *pThis, ClRmdPktT *pRepl, ClUint32T hdrLen,
                                 ClIocPhysicalAddressT *srcAddr,
                                 ClUint8T priority, ClUint8T protoType,
                                 ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;
    ClRmdRecordSendT *rec = NULL;

    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClRmdAsyncCallbackT fpTempPtr = NULL;

    void *cookie = NULL;
    ClBufferHandleT outMsgHdl = 0;
    ClBufferHandleT inMsgHdl = 0;
    ClUint32T len = 0;

    RMD_STAT_INC(pRmdObject->rmdStats.nRmdReplies);
    rc = clOsalMutexLock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(rc == CL_OK);
    rc = clCntNodeFind(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)pRepl->ClRmdHdr.msgId,
                       &nodeHandle);
    if (rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(pRmdObject->sndRecContainerHandle, nodeHandle,
                                  (ClCntDataHandleT *) &rec);
        if (rc == CL_OK)
        {
            /*
             * pData is part of record so no need to free, it will be freed by
             * hash delete callback
             */
            if (rec)
            {
                if(pRepl->ClRmdHdr.rmdReqRepl.rc == CL_RMD_ERR_INUSE
                   ||
                   pRepl->ClRmdHdr.rmdReqRepl.rc == CL_RMD_ERR_CONTINUE)
                {
                    clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
                    clBufferDelete(&outMsgHandle);
                    return CL_OK;
                }

                clTimerDeleteAsync(&(rec->recType.asyncRec.timerID));
                /*
                 * copy the reply in outbuffer and call the callback
                 */
                clBufferHeaderTrim(outMsgHandle, hdrLen);

                fpTempPtr = rec->recType.asyncRec.func;
                cookie = rec->recType.asyncRec.cookie;
                outMsgHdl = rec->recType.asyncRec.outMsgHdl;

                clBufferLengthGet(outMsgHandle, &len);
                clBufferToBufferCopy(outMsgHandle, 0, outMsgHdl,
                        len);
                clBufferDelete(&outMsgHandle);

                /*
                 * clBufferConcatenate(outMsgHdl, &outMsgHandle);
                 */
                inMsgHdl = rec->recType.asyncRec.sndMsgHdl;
                clBufferHeaderTrim(inMsgHdl, hdrLen);
                rc = clCntNodeDelete(pRmdObject->sndRecContainerHandle,
                                     nodeHandle);

                /*
                 * unlocking it as callback func can make the rmd call
                 */
                rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
                clBufferReadOffsetSet(outMsgHdl, 0,
                        CL_BUFFER_SEEK_SET);
                fpTempPtr((ClRcT) pRepl->ClRmdHdr.rmdReqRepl.rc,
                        cookie, inMsgHdl, outMsgHdl);
                clBufferDelete(&inMsgHdl);
            }
            else
            {
                RMD_DBG3(("RMD: ERROR\n"));
                rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
                clBufferDelete(&outMsgHandle);
            }
        }
        else
        {
            rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
            clBufferDelete(&outMsgHandle);
        }
    }
    else
    {
        clBufferDelete(&outMsgHandle);
        RMD_STAT_INC(pRmdObject->rmdStats.nBadReplies);
        RMD_DBG4(("rmdHandleAsyncReply: Node not Found \n"));
        rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    }
    return CL_OK;
}

static ClRcT atmostOnceProcessing(ClEoExecutionObjT *pThis, ClRmdPktT *pReq,
                                  ClIocPhysicalAddressT *srcAddr,
                                  ClUint8T priority,
                                  ClRmdRecordRecvT **recvRecord2,
                                  ClRmdRecvKeyT *atmostOnceKey,
                                  ClBoolT *pRecordActive)
{
    ClRmdRecordRecvT *recvRecord =
        (ClRmdRecordRecvT *) clHeapAllocate((ClUint32T) sizeof(ClRmdRecordRecvT));
    ClCntNodeHandleT nodeHandle = 0;
    ClRcT rc = CL_OK, rc1 = CL_OK;
    ClRmdRecordRecvT *recvRecord1 = NULL;
    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClTimeT atMostOnceTimeout = 0;
    ClUint32T maxTries = 0;
    ClUint32T callTimeout = 0;

    if (recvRecord == NULL)
    {
        return (CL_RMD_RC(CL_ERR_NO_MEMORY));
    }
    memset(recvRecord, 0, sizeof(ClRmdRecordRecvT));

    recvRecord->recvKey.fnNumber = pReq->ClRmdHdr.fnID;
    recvRecord->recvKey.msgId = pReq->ClRmdHdr.msgId;
    recvRecord->recvKey.srcAddr.nodeAddress = srcAddr->nodeAddress;
    recvRecord->recvKey.srcAddr.portId = srcAddr->portId;
    *pRecordActive = CL_FALSE;
    /*
     * Copy over the key so that the caller can double check after 
     * dropping the lock.
     */
    if(atmostOnceKey)
        memcpy(atmostOnceKey, &recvRecord->recvKey, sizeof(*atmostOnceKey));

    rc = clOsalMutexLock(pRmdObject->semaForRecvHashTable);

    rc = clCntNodeFind(pRmdObject->rcvRecContainerHandle,
                       (ClCntKeyHandleT) (&recvRecord->recvKey), &nodeHandle);
    if (rc == CL_OK)
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nDupRequests);
        rc = clCntNodeUserDataGet(pRmdObject->rcvRecContainerHandle, nodeHandle,
                                  (ClCntDataHandleT *) &recvRecord1);
        if (recvRecord1) 
        {
            if(recvRecord1->msg)
                resendReply(pThis, recvRecord1, priority);
            else
                *pRecordActive = CL_TRUE;
        }
        rc = clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);
        clLogDebug("RMD", "RCV",
                   "Duplicate message received from node [%d], IOC port [%d], function [%d], msg id [%lld]",
                   srcAddr->nodeAddress, recvRecord->recvKey.srcAddr.portId,
                   pReq->ClRmdHdr.fnID, pReq->ClRmdHdr.msgId);
        clHeapFree(recvRecord);
        RMD_DBG4(("************At Most Once::DUPLICATE************\n"));
        return RMD_DUP_REQ;
    }
    else
    {
        pRmdObject->numAtmostOnceEntry++;
        recvRecord->msg = 0;
        maxTries = pReq->ClRmdHdr.maxNumTries;
        callTimeout = pReq->ClRmdHdr.callTimeout;
        atMostOnceTimeout =
            (maxTries) * (callTimeout) + RMD_ATMOST_CLEANUP_TO_THRESH;
        recvRecord->atmostOnceEntryLifeTime = atMostOnceTimeout;
        recvRecord->maxTries = maxTries;
        rc1 =
            clCntNodeAdd(pRmdObject->rcvRecContainerHandle,
                         (ClCntKeyHandleT) (&(recvRecord->recvKey)),
                         (ClCntDataHandleT) recvRecord, NULL);
        if (rc1 != CL_OK)
        {
            RMD_DBG1(("RMD:Atmost once node addition failed\n"));
            rc = clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);
            clHeapFree(recvRecord);
            RMD_STAT_INC(pRmdObject->rmdStats.nBadRequests);
            RMD_STAT_INC(pRmdObject->rmdStats.nFailedAtmostOnceCalls);
            return rc1;
        }
        *recvRecord2 = recvRecord;
        rc = clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);
    }

    return CL_OK;
}

static void resendReply(ClEoExecutionObjT *pThis, ClRmdRecordRecvT *rec,
                        ClUint8T priority)
{
    ClRcT rc = CL_OK;

    ClRmdObjT *pRmdObject = (ClRmdObjT *) pThis->rmdObj;
    ClIocSendOptionT sndOption = { 0 };
    ClRmdHdrT ClRmdHdr = {{0}};
    ClUint32T size = 0;

    RMD_STAT_INC(pRmdObject->rmdStats.nResendReplies);

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = (rec->atmostOnceEntryLifeTime / (rec->maxTries ? rec->maxTries:1 ));
    sndOption.priority = priority;
    sndOption.sendType = CL_IOC_NO_SESSION;

    clBufferReadOffsetSet(rec->msg, 0, CL_BUFFER_SEEK_SET);
    rc = clRmdUnmarshallRmdHdr(rec->msg, &ClRmdHdr, &size);
    if(rc != CL_OK)
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
        return;
    }
    clBufferReadOffsetSet(rec->msg, 0, CL_BUFFER_SEEK_SET);

    if (ClRmdHdr.flags & CL_RMD_CALL_ASYNC)
    {
        ClIocAddressT _srcAddr = {{0}};

        _srcAddr.iocPhyAddress = rec->recvKey.srcAddr;
        clRmdDumpPkt("sending async reply", rec->msg);
        rc = clIocSend(pThis->commObj, rec->msg,
                       CL_IOC_RMD_ASYNC_REPLY_PROTO, &_srcAddr, &sndOption);
    }
    else
    {
        ClIocAddressT _srcAddr = {{0}}; 
        _srcAddr.iocPhyAddress = rec->recvKey.srcAddr;
        clRmdDumpPkt("sending sync reply", rec->msg);
        rc = clIocSend( pThis->commObj, rec->msg,
                       CL_IOC_RMD_SYNC_REPLY_PROTO, &_srcAddr, &sndOption);
    }

    if (rc != CL_OK)
    {
        RMD_STAT_INC(pRmdObject->rmdStats.nFailedCalls);
        RMD_DBG2(("RMD: Resend Reply Failed\n"));
    }

    return;
}

static ClRcT rmdAtmostTimerDBCleanup(ClRmdObjT *pRmdObject, ClTimeT td)
{
    ClCntNodeHandleT nodeHandle = 0, nodeHandle1 = 0;
    ClRcT rc = CL_OK;
    ClRmdRecordRecvT *rec = NULL;

    if (pRmdObject == NULL)
    {
        return (CL_RMD_RC(CL_ERR_INVALID_BUFFER));
    }

    while (1)
    {

        rc = clCntFirstNodeGet(pRmdObject->rcvRecContainerHandle, &nodeHandle);
        if (rc == CL_OK)
        {
            rc = clCntNodeUserDataGet(pRmdObject->rcvRecContainerHandle,
                                      nodeHandle, (ClCntDataHandleT *) &rec);
            if (rc == CL_OK)
            {
                if (rec)
                {
                    if (rec->msg)
                    {
                        if (rec->atmostOnceEntryLifeTime > td)
                        {
                            rec->atmostOnceEntryLifeTime =
                                rec->atmostOnceEntryLifeTime - td;
                            break;  /* can't delete this go to next node */
                        }
                        clCntNodeDelete(pRmdObject->rcvRecContainerHandle, nodeHandle); /* deleted, 
                                                                                         * find 
                                                                                         * next 
                                                                                         * first 
                                                                                         * node */
                        pRmdObject->numAtmostOnceEntry--;
                    }
                    else
                    {
                        if ((rec->atmostOnceEntryLifeTime) < td)
                            rec->atmostOnceEntryLifeTime = 
                                CL_MIN(td * (rec->maxTries), 
                                       CL_RMD_DEFAULT_TIMEOUT * CL_RMD_DEFAULT_RETRIES);    /* reset 
                                                                                             * to 
                                                                                             * higher 
                                                                                             * value */
                        break;
                    }
                }
                else
                {
                    break;      /* can't delete this go to next node */
                }
            }
            else
            {
                return rc;
            }
        }
        else
        {
            return rc;
        }
    }
    while (rc == CL_OK)
    {
        rc = clCntNextNodeGet(pRmdObject->rcvRecContainerHandle, nodeHandle,
                              &nodeHandle1);
        if (rc != CL_OK)
            continue;
        rc = clCntNodeUserDataGet(pRmdObject->rcvRecContainerHandle,
                                  nodeHandle1, (ClCntDataHandleT *) &rec);
        if (rc == CL_OK)
        {
            if (rec)
            {
                if (rec->msg)
                {
                    if (rec->atmostOnceEntryLifeTime > td)
                    {
                        rec->atmostOnceEntryLifeTime =
                            rec->atmostOnceEntryLifeTime - td;
                        nodeHandle = nodeHandle1;
                        continue;

                    }
                    (void) clCntNodeDelete(pRmdObject->rcvRecContainerHandle,
                                           nodeHandle1);
                    pRmdObject->numAtmostOnceEntry--;
                }
                else
                {
                    if ((rec->atmostOnceEntryLifeTime) < td)
                        rec->atmostOnceEntryLifeTime = 
                            CL_MIN(td * (rec->maxTries), 
                                   CL_RMD_DEFAULT_TIMEOUT * CL_RMD_DEFAULT_RETRIES);    /* reset 
                                                                                         * to 
                                                                                         * higher 
                                                                                         * value */
                    nodeHandle = nodeHandle1;
                    continue;
                }
            }
            else
            {
                nodeHandle = nodeHandle1;
                continue;

            }
        }
        else
        {
            continue;
        }
    }

    return rc;
}
