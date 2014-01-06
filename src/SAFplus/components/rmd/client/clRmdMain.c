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
 * File        : clRmdMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains the infrastructure for RMD.
 *****************************************************************************/

#define RMD_POSIX
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <clCommon.h>
#include <clCntApi.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>
#include "clRmdMain.h"
#include "clRmdConfigApi.h"
#include <stdlib.h>
#include "clDebugApi.h"
#include <clLogUtilApi.h>
#include <sys/time.h>

#ifdef DMALLOC
# include "dmalloc.h"
#endif


#ifdef MORE_CODE_COVERAGE
# include<clCodeCovStub.h>
#endif

extern ClRmdLibConfigT _clRmdConfig;
//extern ClRcT clEoGetRemoteObjectAndBlock(ClUint16T remoteObj, ClEoExecutionObjT **pRemoteEoObj);

/*
 * Version Related Definitions
 */
ClVersionT gRmdVersionsSupported[] = {
    CL_RMD_VERSION,
    /*
     * Add further Versions
     */
};

ClVersionDatabaseT gRmdVersionDb = {
    sizeof(gRmdVersionsSupported) / sizeof(ClVersionT),
    gRmdVersionsSupported
};


ClRcT clRmdParamSanityCheck(ClBufferHandleT inMsgHdl,    /* Input
                                                                 * Message */
                            ClBufferHandleT outMsgHdl,   /* Output
                                                                 * Message */
                            ClUint32T flags,    /* Flags */
                            ClRmdOptionsT *pOptions,    /* Optional Parameters
                                                         * for RMD Call */
                            ClRmdAsyncOptionsT *pAsyncOptions)  /* Optional
                                                                 * Async
                                                                 * Parameters
                                                                 * for RMD Call 
                                                                 */
{
    ClRcT rc;
    ClUint32T size;
    ClUint32T inBufLen = 0;

    CL_FUNC_ENTER();

    rc = clRmdMaxPayloadSizeGet(&size);
    if (rc != CL_OK)
        return rc;
    if (inMsgHdl)
        rc = clBufferLengthGet(inMsgHdl, &inBufLen);
    if (rc != CL_OK)
    {
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
    }
    if (inBufLen > size)
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));

    if (flags &
        ~(CL_RMD_CALL_ASYNC | CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_ATMOST_ONCE |
          RMD_CALL_MSG_ASYNC | CL_RMD_CALL_DO_NOT_OPTIMIZE |
          CL_RMD_CALL_NON_PERSISTENT | CL_RMD_CALL_IN_SESSION | 
          CL_RMD_CALL_ORDERED
        ))
    {
        RMD_DBG2((" RMD INVALID FLAGS 1\n"));
        CL_FUNC_EXIT();
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
    }

    if (inMsgHdl != 0)
    {
        if (inBufLen == 0)
        {
            RMD_DBG2((" RMD INVALID input buffer len 1 \n"));
            CL_FUNC_EXIT();
            return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
        }
    }

    if (flags & CL_RMD_CALL_ASYNC)
    {
        if (pAsyncOptions == NULL)
        {
            if (flags & CL_RMD_CALL_NEED_REPLY)
            {
                RMD_DBG2((" RMD INVALID Outbuff parametrs  for async 1\n"));
                CL_FUNC_EXIT();
                return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
            }


        }
        else
        {
            if (pAsyncOptions->fpCallback == NULL)
            {
                if (flags & CL_RMD_CALL_NEED_REPLY)
                {
                    RMD_DBG2((" RMD INVALID Outbuff parametrs  for async 2\n"));
                    CL_FUNC_EXIT();
                    return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
                }
            }

        }
    }
    else
    {
        if (pAsyncOptions)
            if (pAsyncOptions->fpCallback || pAsyncOptions->pCookie)
            {
                RMD_DBG2((" RMD INVALID Outbuff parametrs  for sync 1\n"));
                CL_FUNC_EXIT();
                return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
            }


    }

    if (pOptions && (pOptions->retries > _clRmdConfig.maxRetries))
    {
        RMD_DBG2((" RMD INVALID Options retries2\n"));
        CL_FUNC_EXIT();
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

static ClInt32T recvKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    CL_FUNC_ENTER();

    if (key1 == 0 || key2 == 0)
    {
        RMD_DBG2((" RMD Invalid Key is passed\n"));
        CL_FUNC_EXIT();
        return (CL_RMD_RC(CL_ERR_INVALID_PARAMETER));
    }
    cmpResult = memcmp((void *) key1, (void *) key2, sizeof(ClRmdRecvKeyT));
    CL_FUNC_EXIT();
    return cmpResult;
}
static ClInt32T sendKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return ((ClWordT)key1 - (ClWordT)key2);
}
static ClUint32T sendHashFunction(ClCntKeyHandleT key)
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return (  ((ClWordT)key) & RMD_SEND_BUCKETS_MASK );
}
static ClUint32T recvHashFunction(ClCntKeyHandleT key)
{
    ClUint32T hashVal;

    CL_FUNC_ENTER();
    if (key == 0)
    {
        RMD_DBG2((" RMD Invalid Key is passed\n"));
        CL_FUNC_EXIT();
        return (CL_RMD_RC(CL_ERR_INVALID_PARAMETER));
    }
    CL_FUNC_EXIT();
    hashVal =
        ( ((ClRmdRecvKeyT *) key)->msgId & RMD_RECV_BUCKETS_MASK );
    return (hashVal);
}


static void recvHashDeleteCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData)
{
    ClRmdRecordRecvT *recvRecord = (ClRmdRecordRecvT *) userData;

    CL_FUNC_ENTER();


    if (userData)
    {
        if (recvRecord->msg)
        {
            clBufferDelete(&(recvRecord->msg));
        }
        clHeapFree((void *) userData);
    }

    CL_FUNC_EXIT();
}

static void sendHashDeleteCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData)
{
    ClRmdRecordSendT *rec = (ClRmdRecordSendT *) userData;
    CL_FUNC_ENTER();
    if (userData)
    {
        /*
         * The check on the flag is a must now since sync
         * path also invokes this code. Added as a fix to
         * Bug - 3748.
         */
        if (rec->flags & CL_RMD_CALL_ASYNC)
        {
            /*
             * cleanup for Async Info 
             */
            if (rec->recType.asyncRec.timerID)
            {
                clTimerDeleteAsync(&rec->recType.asyncRec.timerID);
            }
            
            clHeapFree(rec);
        }
        else
        {
            /* Before destroying the condition variable, make sure it is not being used in clOsalCondWait(). */
            clOsalCondSignal(&rec->recType.syncRec.syncCond);
            /*
             * cleanup for Sync Info
             */
            clOsalCondDestroy(&rec->recType.syncRec.syncCond);
        }
    }

    CL_FUNC_EXIT();
}

/*
 * This function was added as additional cleanup is necessary
 * in the case of Async calls. This was done as a part of the
 * fix for BUG 4080. The in and out msg handles were not 
 * released at the time of finalize if the async callback
 * wasn't invoked.
 */
static void sendHashDestroyCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData)
{
    ClRmdRecordSendT *rec = (ClRmdRecordSendT *) userData;

    if ((userData)&&(rec->flags & CL_RMD_CALL_ASYNC))
    {
        /*
         * The buffers must be deleted as the callback hasn't been
         * invoked. This is to avoid any leaks at the time of RMD
         * Finalize when responses are expected back. (BUG 4080)
         */
        if(rec->recType.asyncRec.sndMsgHdl)
            clBufferDelete(&(rec->recType.asyncRec.sndMsgHdl));
        if(rec->recType.asyncRec.outMsgHdl)
            clBufferDelete(&(rec->recType.asyncRec.outMsgHdl));        
    }
    
    
    sendHashDeleteCallBack(userKey,userData);
}

ClRcT clRmdObjInit(ClRmdObjHandleT *p)
{
    ClRcT retCode = CL_OK, retCode1;
    ClRmdObjT *pRmdObject = NULL;
    unsigned long timeStamp = 0;
    struct timeval tm1;

#ifdef DEBUG
    static ClUint8T rmdAddedTodbgComp = CL_FALSE;

    if (CL_FALSE == rmdAddedTodbgComp)
    {
        retCode = dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
        rmdAddedTodbgComp = CL_TRUE;
        if (CL_OK != retCode)
        {
            clLogError("OBG","INI","dbgAddComponent FAILED ");
            CL_FUNC_EXIT();
            return retCode;
        }
    }
#endif
    CL_FUNC_ENTER();

    if (NULL == p)
    {
        RMD_DBG1((" RMD Invalid Object handle passed\n"));
        CL_FUNC_EXIT();
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
    }

    retCode = clOsalInitialize(NULL);

    retCode = clTimerInitialize(NULL);
    pRmdObject = (ClRmdObjT *) clHeapAllocate(sizeof(ClRmdObjT));

    if (NULL == pRmdObject)
    {
        RMD_DBG1((" RMD No Memory\n"));
        CL_FUNC_EXIT();
        return ((CL_RMD_RC(CL_ERR_NO_MEMORY)));
    }

    gettimeofday(&tm1, NULL);
    timeStamp = tm1.tv_sec * 1000000 + tm1.tv_usec;
    pRmdObject->msgId = 1;
    retCode = clOsalMutexCreate(&pRmdObject->semaForSendHashTable);

    if (CL_OK != CL_GET_ERROR_CODE(retCode))
    {
        RMD_DBG1((" RMD send Mutex creation failed\n"));
        CL_FUNC_EXIT();
        return (retCode);
    }
    retCode = clOsalMutexCreate(&pRmdObject->semaForRecvHashTable);

    if (CL_OK != CL_GET_ERROR_CODE(retCode))
    {
        RMD_DBG1((" RMD  recv Mutex creation failed\n"));

        retCode1 = clOsalMutexDelete(pRmdObject->semaForSendHashTable);
        CL_FUNC_EXIT();
        return (retCode);
    }

    retCode =
        clCntHashtblCreate(NUMBER_OF_RECV_BUCKETS, recvKeyCompare,
                           recvHashFunction, recvHashDeleteCallBack,
                           recvHashDeleteCallBack, CL_CNT_UNIQUE_KEY,
                           &pRmdObject->rcvRecContainerHandle);
    if (CL_OK != CL_GET_ERROR_CODE(retCode))
    {
        RMD_DBG1((" RMD  send Hash table creation failed\n"));

        retCode1 = clOsalMutexDelete(pRmdObject->semaForRecvHashTable);
        retCode1 = clOsalMutexDelete(pRmdObject->semaForSendHashTable);
        CL_FUNC_EXIT();
        return (retCode);
    }
    retCode =
        clCntHashtblCreate(NUMBER_OF_SEND_BUCKETS, sendKeyCompare,
                           sendHashFunction, sendHashDeleteCallBack,
                           sendHashDestroyCallBack, CL_CNT_UNIQUE_KEY,
                           &pRmdObject->sndRecContainerHandle);
    if (CL_OK != CL_GET_ERROR_CODE(retCode))
    {
        RMD_DBG1((" RMD  recv Hash table creation failed\n"));

        retCode1 = clCntDelete(pRmdObject->rcvRecContainerHandle);
        retCode1 = clOsalMutexDelete(pRmdObject->semaForRecvHashTable);
        retCode1 = clOsalMutexDelete(pRmdObject->semaForSendHashTable);
        CL_FUNC_EXIT();
        return (retCode);
    }

    pRmdObject->responseCntxtDbHdl = 0;
    retCode = clHandleDatabaseCreate(NULL, &pRmdObject->responseCntxtDbHdl);
    if (retCode != CL_OK)
    {
        RMD_DBG1((" RMD  Sync Handle Database create failed\n"));
        retCode1 = clCntDelete(pRmdObject->sndRecContainerHandle);
        retCode1 = clCntDelete(pRmdObject->rcvRecContainerHandle);
        retCode1 = clOsalMutexDelete(pRmdObject->semaForRecvHashTable);
        retCode1 = clOsalMutexDelete(pRmdObject->semaForSendHashTable);

        CL_FUNC_EXIT();
        return retCode;
    }

    pRmdObject->numAtmostOnceEntry = 0;
    pRmdObject->lastAtmostOnceCleanupTime = clOsalStopWatchTimeGet();
    pRmdObject->rmdStats.nRmdCalls = 0;
    pRmdObject->rmdStats.nFailedCalls = 0;
    pRmdObject->rmdStats.nResendRequests = 0;
    pRmdObject->rmdStats.nRmdReplies = 0;
    pRmdObject->rmdStats.nBadReplies = 0;
    pRmdObject->rmdStats.nRmdRequests = 0;
    pRmdObject->rmdStats.nBadRequests = 0;
    pRmdObject->rmdStats.nCallTimeouts = 0;
    pRmdObject->rmdStats.nDupRequests = 0;
    pRmdObject->rmdStats.nAtmostOnceCalls = 0;
    pRmdObject->rmdStats.nResendReplies = 0;
    pRmdObject->rmdStats.nReplySend = 0;
    pRmdObject->rmdStats.nRmdCallOptimized = 0;
    *p = (ClRmdObjHandleT) pRmdObject;
    CL_FUNC_EXIT();
    return (CL_OK);
}
ClRcT clRmdObjClose(ClRmdObjHandleT p)
{
    ClRcT retCode = CL_OK;
    ClRmdObjT *pRmdObject = NULL;

    CL_FUNC_ENTER();

    pRmdObject = (ClRmdObjT *) p;

    if (NULL == pRmdObject)
    {
        RMD_DBG1((" RMD  Invalid rmd handle in objclose\n"));
        CL_FUNC_EXIT();
        return ((CL_RMD_RC(CL_ERR_INVALID_PARAMETER)));
    }

    /*
     * Avoid deleting the mutexes so its still valid in case any context is abusing the terminate
     * and initiating or not quitting the rmd sends.
     */
#if 0
    retCode = clOsalMutexDelete(pRmdObject->semaForRecvHashTable);

    if (CL_OK != retCode)
    {
        RMD_DBG1((" RMD Recv mutex delete failed\n"));
    }
    
    retCode = clOsalMutexDelete(pRmdObject->semaForSendHashTable);

    if (CL_OK != retCode)
    {
        RMD_DBG1((" RMD Send mutex delete failed\n"));
    }
#endif

    clOsalMutexLock(pRmdObject->semaForRecvHashTable);
    retCode = clCntDelete(pRmdObject->rcvRecContainerHandle);
    if (CL_OK != retCode)
    {
        RMD_DBG1((" RMD rcv hash table destroy failed\n"));
    }
    pRmdObject->rcvRecContainerHandle = 0;
    clOsalMutexUnlock(pRmdObject->semaForRecvHashTable);

    clOsalMutexLock(pRmdObject->semaForSendHashTable);
    retCode = clCntDelete(pRmdObject->sndRecContainerHandle);
    if (CL_OK != retCode)
    {
        RMD_DBG1((" RMD snd hash table destroy failed\n"));

    }
    pRmdObject->sndRecContainerHandle = 0;
    clOsalMutexUnlock(pRmdObject->semaForSendHashTable);

    retCode = clHandleDatabaseDestroy(pRmdObject->responseCntxtDbHdl);
    if (CL_OK != retCode)
    {
        RMD_DBG1((" RMD snd hash table destroy failed\n"));

    }
    pRmdObject->responseCntxtDbHdl = 0;

#if 0
    /*
     * Don't release the rmd context to respect abusers or initiators of rmd send outside
     * terminate callback contexts.
     */
    clHeapFree(pRmdObject);
#endif

    CL_FUNC_EXIT();
    return (CL_OK);
}
