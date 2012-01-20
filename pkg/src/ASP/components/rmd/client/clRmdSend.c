/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : rmd                                                           
 * File        : clRmdSend.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module implements the send side functionality of RMD.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include "clEoApi.h"
#include "clIocApi.h"
#include "clRmdApi.h"
#include "clRmdMain.h"
#include "clDebugApi.h"
#include <sys/time.h>
#include <clIocApiExt.h>


#ifdef DMALLOC
# include "dmalloc.h"
#endif

#ifdef MORE_CODE_COVERAGE
# include<clCodeCovStub.h>
#endif
static ClRcT createAsyncSendTimer(ClRmdAsyncRecordT *pAsyncRecord,
                                  void *pTimerData);

static ClRcT rmdSendTimerFunc(void *pData);

static ClRcT resendMsg(ClRmdRecordSendT * re,
                       ClRmdObjT         *pRmdObj,
                       ClEoExecutionObjT *pThis);

typedef void *(*taskStartFunction) (void *);

ClRcT clRmdCreateRmdMessageToSend(ClVersionT *version,
                                  ClUint32T funcId, /* Function ID to invoke */
                                  ClBufferHandleT inMsgHdl,
                                  ClUint32T flags,  /* Flags */
                                  ClUint64T msgId, ClUint32T payloadLen, 
                                  ClUint32T outputLen, ClUint32T maxRetries, 
                                  ClUint32T callTimeOut,
                                  ClUint32T *pHdrLen) 
{
    ClRmdHdrT ClRmdHdr = { {0} };
    ClUint8T hdrBuffer[sizeof(ClRmdHdrT)*2] = {0};
    ClUint32T hdrLen = 0;
    ClUint32T rc = CL_OK;

    CL_FUNC_ENTER();

    CL_RMD_VERSION_SET_NEW(ClRmdHdr, version);
    ClRmdHdr.protoVersion = CL_RMD_HEADER_VERSION;

    if (((flags & CL_RMD_CALL_ATMOST_ONCE) == 0) || (maxRetries == 0) ||
        (callTimeOut == CL_RMD_TIMEOUT_FOREVER) || (callTimeOut == 0))
    {
        if ((callTimeOut == 0) || (callTimeOut == CL_RMD_TIMEOUT_FOREVER))
        {
            /*
             * only if timeout forever, then pass 0 as the timeout. Why? - this
             * timeout is passed to the IOC, for IOC, 0 is best effort and
             * passing -1 from the server to IOC will block the server thread
             * till send done. This is not desirable
             */
            ClRmdHdr.callTimeout = 0;
        }
        else
        {
            /*
             * pass normal timeout through the header so this timeout can be
             * given to the IOC by server
             */
            ClRmdHdr.callTimeout = callTimeOut;
        }

        ClRmdHdr.flags = flags & ~(CL_RMD_CALL_ATMOST_ONCE);
        ClRmdHdr.callTimeout = 0;
        ClRmdHdr.maxNumTries = 0;
    }
    else
    {
        ClRmdHdr.callTimeout = callTimeOut;
        ClRmdHdr.maxNumTries = maxRetries + 1;
        ClRmdHdr.flags = flags;
    }

    ClRmdHdr.msgId = msgId;
    ClRmdHdr.fnID = funcId;
    clRmdMarshallRmdHdr(&ClRmdHdr, hdrBuffer, &hdrLen);
    rc = clBufferDataPrepend(inMsgHdl, (ClUint8T *) hdrBuffer, hdrLen);
    clBufferReadOffsetSet(inMsgHdl, 0, CL_BUFFER_SEEK_SET);
    if(pHdrLen)
        *pHdrLen = hdrLen;
    CL_FUNC_EXIT();
    return rc;
}

/*
 * Paket are added in the record after converting them in the nw order so msgid
 * here is network order. Called with lock held.
 */
ClRcT clRmdCreateAndAddRmdSendRecord(ClEoExecutionObjT *pThis,
                                     ClIocAddressT destAddr,
                                     ClUint32T hdrLen,
                                     ClBufferHandleT outMsgHdl,
                                     ClUint32T flags, ClRmdOptionsT *pOptions,
                                     ClRmdAsyncOptionsT *pAsyncOptions,
                                     ClBufferHandleT message,
                                     ClUint64T msgId, ClUint32T payloadLen,
                                     ClRmdRecordSendT ** ppSendRec)
{
    ClRmdRecordSendT *rec;

    ClRcT retVal = CL_OK;
    ClRcT retCode = 0;

    /*
     * it will be called only for async
     */
    CL_FUNC_ENTER();
    rec = clHeapAllocate((ClUint32T) sizeof(ClRmdRecordSendT));
    if (rec == NULL)
    {
        CL_FUNC_EXIT();
        return (CL_RMD_RC(CL_ERR_NO_MEMORY));
    }

    rec->flags = flags;
    rec->hdrLen = hdrLen;
    rec->recType.asyncRec.rcvMsg = NULL;
    rec->recType.asyncRec.outLen = payloadLen;
    rec->recType.asyncRec.priority = pOptions->priority;
    rec->recType.asyncRec.destAddr = destAddr;

    rec->recType.asyncRec.sndMsgHdl = message;

    rec->recType.asyncRec.timerID   = 0;
 
    rec->recType.asyncRec.outMsgHdl = outMsgHdl;

    /*
     * options can't be 0 as we are creating and passing it
     */
    rec->recType.asyncRec.noOfRetry = pOptions->retries;
    rec->recType.asyncRec.timeout = pOptions->timeout;
    rec->recType.asyncRec.cookie = pAsyncOptions->pCookie;
    rec->recType.asyncRec.func = pAsyncOptions->fpCallback;

    RMD_DBG4((" RMD CALL is Async and reply needed create the timer\n"));
    retVal =
        createAsyncSendTimer(&rec->recType.asyncRec,(ClPtrT)(ClWordT)msgId);

    if (retVal != CL_OK)
    {
        clHeapFree(rec);
        CL_FUNC_EXIT();
        return retVal;

    }

    retVal =
        clCntNodeAdd(((ClRmdObjT *) pThis->rmdObj)->sndRecContainerHandle,
                     (ClPtrT)(ClWordT)msgId, (ClCntDataHandleT) rec, NULL);
    if (retVal == CL_OK) *ppSendRec = rec;
    else
    {
        RMD_DBG4((" RMD  createandaddrec:node add failed1."
                  " Delete timer and return.\n"));

        /*
         * Free the timer additionally for Async Call
         */
        if (flags & CL_RMD_CALL_ASYNC)
            retCode = clTimerDeleteAsync(&rec->recType.asyncRec.timerID);

        clHeapFree(rec);
    }

    CL_FUNC_EXIT();
    return retVal;
}

ClRcT createAsyncSendTimer(ClRmdAsyncRecordT *pAsyncRecord,
                           void           *pTimerData)
{
    ClTimerTimeOutT timerExpiry;
    ClRcT retCode = CL_OK;
    ClUint32T timeout = pAsyncRecord->timeout;
    
    CL_FUNC_ENTER();

    if (timeout == 0)
        timeout = CL_RMD_TIMEOUT_FOREVER;

    if(timeout != CL_RMD_TIMEOUT_FOREVER && pAsyncRecord->noOfRetry > 0)
    {
        timeout *= pAsyncRecord->noOfRetry;
    }

    timerExpiry.tsSec = (timeout / 1000);
    timerExpiry.tsMilliSec = (timeout % 1000);
    retCode =
        clTimerCreate(timerExpiry, CL_TIMER_REPETITIVE,
                      CL_TIMER_SEPARATE_CONTEXT, rmdSendTimerFunc,
                      pTimerData, &pAsyncRecord->timerID);
    CL_FUNC_EXIT();
    return retCode;


}

static ClRcT rmdSendTimerFunc(void *pData)
{
    ClCntNodeHandleT    nodeHandle;
    ClRmdRecordSendT    *rec         = NULL;
    ClRcT               rc;
    ClRcT               retCode      = 0;
    ClRmdObjT           *pRmdObject;
    ClRmdAsyncCallbackT fpTempPtr;
    void                *cookie;
    ClBufferHandleT     outMsgHdl;
    ClEoExecutionObjT   *pThis       = NULL;
    ClUint64T           msgId        = (ClUint32T)(ClWordT) pData;

    CL_FUNC_ENTER();
    if (pData == NULL)
    {
        return (CL_RMD_RC(CL_ERR_INVALID_BUFFER));
    }

    rc = clEoMyEoObjectGet(&pThis);
    if ( CL_OK != rc )
    {
        return rc;
    }

    pRmdObject = (ClRmdObjT *) (pThis->rmdObj);
    if (pRmdObject == NULL)
    {
        return (CL_RMD_RC(CL_ERR_INVALID_BUFFER));
    }

    rc = clOsalMutexLock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(rc == CL_OK);

    rc = clCntNodeFind(pRmdObject->sndRecContainerHandle, (ClPtrT)(ClWordT)msgId,
                       &nodeHandle);
    if (rc == CL_OK)
    {

        rc = clCntNodeUserDataGet(pRmdObject->sndRecContainerHandle, nodeHandle,
                                  (ClCntDataHandleT *) &rec);
        if (rc == CL_OK)
        {

            if (rec)
            {
                /*
                 * Disabling retries for ASYNC since we dont want
                 * to land up in duplicate send mess as anyway we have
                 * accounted for the retries while creating the async timer.
                 */
                if (0 && rec->recType.asyncRec.noOfRetry > 0)
                {
                    /*
                     * key is part of record so no need to free just reuse
                     */
                    retCode = resendMsg(rec, pRmdObject, pThis);
                }
                else
                {
                    /*
                     * key is part of record so no need to free, it will be
                     * freed by hash delete callback
                     */
                    RMD_STAT_INC(pRmdObject->rmdStats.nCallTimeouts);
                    retCode = clTimerDeleteAsync(&rec->recType.asyncRec.timerID);
                    if (rec->recType.asyncRec.func)
                    {
                        /*
                         * unlocking it as callback func can make the rmd call
                         */
                        ClBufferHandleT message;

                        fpTempPtr = rec->recType.asyncRec.func;
                        cookie = rec->recType.asyncRec.cookie;
                        outMsgHdl = rec->recType.asyncRec.outMsgHdl;
                        message = rec->recType.asyncRec.sndMsgHdl;
                        clBufferHeaderTrim(message, rec->hdrLen);
                        rc = clCntNodeDelete(pRmdObject->sndRecContainerHandle,
                                             nodeHandle);
                        rc = clOsalMutexUnlock(pRmdObject->
                                               semaForSendHashTable);
                        fpTempPtr((CL_RMD_RC(CL_ERR_TIMEOUT)), cookie, message,
                                  outMsgHdl);
                        clBufferDelete(&(message));
                        rc = clOsalMutexLock(pRmdObject->semaForSendHashTable);
                    }
                }
            }
        }
    }

    rc = clOsalMutexUnlock(pRmdObject->semaForSendHashTable);
    CL_ASSERT(rc == CL_OK);
    CL_FUNC_EXIT();
    return CL_OK;

}


static ClRcT resendMsg(ClRmdRecordSendT  *rec, 
                       ClRmdObjT         *pRmdObj,
                       ClEoExecutionObjT *pThis)
{
    ClRcT rc;
    ClIocSendOptionT sndOption = { 0 };

    CL_FUNC_ENTER();
    rec->recType.asyncRec.noOfRetry--;
    RMD_STAT_INC(pRmdObj->rmdStats.nResendRequests);

    sndOption.linkHandle = 0;
    sndOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sndOption.timeout = rec->recType.asyncRec.timeout;
    sndOption.priority = rec->recType.asyncRec.priority;
    if (rec->flags & CL_RMD_CALL_IN_SESSION)
    {
        sndOption.sendType = CL_IOC_SESSION_BASED;
    }
    else
    {
        sndOption.sendType = CL_IOC_NO_SESSION;
    }


    clRmdDumpPkt("sending async req", rec->recType.asyncRec.sndMsgHdl);
    rc = clIocSend((pThis)->commObj,
                   rec->recType.asyncRec.sndMsgHdl,
                   CL_IOC_RMD_ASYNC_REQUEST_PROTO,
                   &rec->recType.asyncRec.destAddr, 
                   &sndOption);
    if(rc != CL_OK)
    {
        RMD_STAT_INC(pRmdObj->rmdStats.nFailedResendRequests);
    }

    CL_FUNC_EXIT();
    return rc;
}
