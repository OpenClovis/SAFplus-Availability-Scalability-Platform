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
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clLogDbg.h>
#include <clLogApi.h>
#include <clLogApiExt.h>
#include <clHandleApi.h>
#include <clEoApi.h>
#include <clDebugApi.h>
#include <clCpmExtApi.h>

ClBoolT               gUtilLibInitialized = CL_FALSE;
ClOsalMutexT          gLogMutex ;
ClOsalCondT           gLogCond ;
ClLogDeferredHeaderT  gLogMsgArray[CL_LOG_MAX_NUM_MSGS];
ClUint16T             writeIdx    = 0;
ClUint16T             readIdx     = 0;
ClUint16T             overWriteFlag = 0;
static ClBoolT        logRecordDrop = CL_TRUE;
ClOsalTaskIdT         taskId        = 0;
ClBoolT               gClLogServer = CL_FALSE;
const ClCharT  *CL_LOG_UTIL_TASK_NAME= "LogUtilLibThread";
#define CL_LOG_UTIL_FLUSH_RECORDS  64
#define CL_LOG_FLUSH_FREQ          CL_LOG_UTIL_FLUSH_RECORDS
#define CL_LOG_MAX_FLUSH_COUNT     32

static void * 
clLogDeferredHandler(void  *pData);

static ClRcT
clLogDoLogWrite(ClLogDeferredHeaderT  *pMsg, 
                ClUint16T             numRecs,
                ClUint16T             *pNumNotFlushed);

static ClUint16T 
clLogUtilNumRecordsTBW(void);

static 
ClRcT
clLogNumFlushableRecordsGet(ClLogDeferredHeaderT  *pMsg,
                            ClUint16T             *pNumRecs,
                            ClUint16T             *pReadIdx,
                            ClUint16T             *pOverwriteFlag);



ClRcT clLogUtilLibInitialize(void)
{
    ClRcT  rc = CL_OK;
    //const ClCharT *pEOName = NULL;
    if( gUtilLibInitialized == CL_TRUE )
    {
        return CL_OK;
    }
    rc = clOsalMutexErrorCheckInit(&gLogMutex);
    if( CL_OK != rc )
    {
        fprintf(stderr, "Mutex init failed rc[0x %x]\n", rc);
        return rc;
    }
    rc = clOsalCondInit(&gLogCond);
    if( CL_OK != rc )
    {
        fprintf(stderr, "Cond init failed rc[0x %x]\n", rc);
        clOsalMutexDestroy(&gLogMutex);
        return rc;
    }
    //pEOName = CL_EO_NAME;

    clLogDebugFilterInitialize();

    gUtilLibInitialized = CL_TRUE;
    rc = clOsalTaskCreateAttached((ClCharT *)CL_LOG_UTIL_TASK_NAME, CL_OSAL_SCHED_OTHER,
            CL_OSAL_THREAD_PRI_NOT_APPLICABLE, CL_OSAL_MIN_STACK_SIZE,
            clLogDeferredHandler, NULL, &taskId);
    if( CL_OK != rc )
    {
        fprintf(stderr, "Thread creation failed rc[0x %x]", rc);
        clOsalCondDestroy(&gLogCond);
        clOsalMutexDestroy(&gLogMutex);
        return rc;
    }
    
    return CL_OK;
}

/*
 * Called with the logmutex held.
 */
static void
clLogFlushRecords(void)
{
    ClUint32T i;
    ClUint32T numFlushed = 0;
    ClUint16T currentReadIdx = readIdx;
    ClUint16T currentWriteIdx = writeIdx;
    ClUint16T saveWriteIdx = writeIdx;
    ClBoolT wrap = CL_FALSE;
    ClHandleT logHandle = 0;
    ClLogSeverityT severity = CL_LOG_SEV_MAX;
    ClRcT rc = CL_OK;
    ClLogDeferredHeaderT *logBuffer = NULL;

    /*
     * Just duplicate the entire range since the flush is one-time only and no need to do half-baked
     * allocs based on the actual flush size.
     */
    logBuffer = (ClLogDeferredHeaderT *) calloc(CL_LOG_MAX_NUM_MSGS, sizeof(*logBuffer));
    CL_ASSERT(logBuffer != NULL);
    memcpy(logBuffer, gLogMsgArray, sizeof(*logBuffer) * CL_LOG_MAX_NUM_MSGS);

    clOsalMutexUnlock(&gLogMutex);
    if(currentReadIdx >= currentWriteIdx)
    {
        wrap = CL_TRUE;
        currentWriteIdx = CL_LOG_MAX_NUM_MSGS;
    }

    for(i = currentReadIdx; i < currentWriteIdx; ++i)
    {
        if(!(severity = logBuffer[i].severity))
            severity = CL_LOG_SEV_ALERT;
        if(!(logHandle = logBuffer[i].handle))
            logHandle = CL_LOG_HANDLE_SYS;
        rc = clLogWriteWithHeader(logHandle, severity,
                                  logBuffer[i].serviceId, logBuffer[i].msgId,
                                  logBuffer[i].msgHeader, "%s", logBuffer[i].msg);
        if(rc != CL_OK)
            continue; /* do nothing now*/ 
    }
    numFlushed += (currentWriteIdx - currentReadIdx);
    if(wrap)
    {
        currentReadIdx = 0;
        currentWriteIdx = saveWriteIdx;
        for(i = currentReadIdx; i < currentWriteIdx; ++i)
        {
            if(!(logHandle = logBuffer[i].handle))
                logHandle = CL_LOG_HANDLE_SYS;
            rc = clLogWriteWithHeader(logHandle, logBuffer[i].severity,
                                      logBuffer[i].serviceId, logBuffer[i].msgId,
                                      logBuffer[i].msgHeader,
                                      "%s", logBuffer[i].msg);
            if(rc != CL_OK)
                continue;
        }
        numFlushed += (currentWriteIdx - currentReadIdx);
    }

    free(logBuffer);

    clOsalMutexLock(&gLogMutex);
    if(overWriteFlag) 
    {
        overWriteFlag = 0;
        logRecordDrop = CL_TRUE;
    }
    readIdx += numFlushed;
    readIdx %= CL_LOG_MAX_NUM_MSGS;
}

static ClRcT
logVWriteDeferred(ClHandleT       handle,
                  ClLogSeverityT  severity,
                  ClUint16T       serviceId,
                  ClUint16T       msgId,
                  const ClCharT         *pMsgHeader,
                  ClBoolT         deferred,
                  const ClCharT         *pFmtStr,
                  va_list         vaargs)
{
    ClRcT    rc     = CL_OK;
    ClBoolT signalFlusher = CL_FALSE;
    static ClBoolT deferredFlag = CL_FALSE;
    static ClBoolT flushPending = CL_TRUE;
    ClBoolT initialRecord = CL_FALSE;
    ClBoolT unlock = CL_TRUE;

    if( gUtilLibInitialized == CL_FALSE )
    {
        /*
         * Since log util lib/EO itself is not initialized,
         * we can safely assume being single threaded at this point
         * and just save the log record for a later flush.
         */
        initialRecord = CL_TRUE;
        goto out_store;
    }

    /* Take the mutex */
    rc = clOsalMutexLock(&gLogMutex);
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE)
    {
        /*
         * Same thread trying to lock as a potential log write loop.
         * We avoid taking the lock here.
         */
        unlock = CL_FALSE;
        rc = CL_OK;
    }

    if( CL_OK != rc )
    {
        fprintf(stderr, "failed to get the lock [0x%x]", rc);
        goto failure;
    }

    if(gClLogServer)
        deferred = CL_TRUE;
    /*
     * If log is up and there are no pending flushes, write directly
     * to avoid garbled logs because of per client deferred writes
     * We go the deferred way for log server writes whenever they are enabled/fired
     */
    if(!deferred && CL_LOG_HANDLE_APP != CL_HANDLE_INVALID_VALUE)
    {
        if(flushPending && unlock)
        {
            flushPending = CL_FALSE;
            clLogFlushRecords();
        }
        if(unlock)
            clOsalMutexUnlock(&gLogMutex);
        if(!handle)
            handle = CL_LOG_HANDLE_SYS;
        return clLogVWriteAsyncWithHeader(handle, severity, serviceId, msgId, pMsgHeader, vaargs);
    }

    if(!unlock)
    {
        /*
         * Skip the recursive deferred log here.
         */
        return CL_OK;
    }

    /* Access the index */
    out_store:
    if(overWriteFlag ) 
    {  
       /* Over write the Last record with records dropped information */
       if(CL_TRUE == logRecordDrop)
       { 
          ClUint16T    dropRecordIdx = 0;
          ClCharT      timeStr[40]   = {0};
          SaNameT      nodeName     = {0};

          dropRecordIdx = (writeIdx != 0)? (writeIdx-1) :(CL_LOG_MAX_NUM_MSGS - 1);

          clLogTimeGet(timeStr, (ClUint32T)sizeof(timeStr));
          clCpmLocalNodeNameGet(&nodeName);
          gLogMsgArray[dropRecordIdx].handle    = handle;
          gLogMsgArray[dropRecordIdx].severity  = CL_LOG_SEV_ALERT;
          gLogMsgArray[dropRecordIdx].serviceId = serviceId;
          gLogMsgArray[dropRecordIdx].msgId     = msgId;
          gLogMsgArray[dropRecordIdx].msgHeader[0] = 0;
          memset(gLogMsgArray[dropRecordIdx].msgHeader, 0, sizeof(gLogMsgArray[dropRecordIdx].msgHeader));
          if(gClLogCodeLocationEnable)
          {
              snprintf(gLogMsgArray[dropRecordIdx].msgHeader, sizeof(gLogMsgArray[dropRecordIdx].msgHeader)-1, CL_LOG_PRNT_FMT_STR,
                 timeStr, __FILE__, __LINE__, nodeName.length, nodeName.value, (int)getpid(), CL_EO_NAME, "LOG", "RWR");
          }
          else
          {
              snprintf(gLogMsgArray[dropRecordIdx].msgHeader, sizeof(gLogMsgArray[dropRecordIdx].msgHeader)-1, CL_LOG_PRNT_FMT_STR_WO_FILE, timeStr,
                 nodeName.length, nodeName.value, (int)getpid(), CL_EO_NAME, "LOG", "RWR");
          }
          snprintf(gLogMsgArray[dropRecordIdx].msg, CL_LOG_MAX_MSG_LEN, "Log buffer full... Some Records Dropped");

          logRecordDrop = CL_FALSE;
       }
       goto out_flushcond;
    }
    gLogMsgArray[writeIdx].handle    = handle;
    gLogMsgArray[writeIdx].severity  = severity;
    gLogMsgArray[writeIdx].serviceId = serviceId;
    gLogMsgArray[writeIdx].msgId     = msgId;
    gLogMsgArray[writeIdx].msgHeader[0] = 0;
    if(pMsgHeader)
    {
        memset(gLogMsgArray[writeIdx].msgHeader, 0, sizeof(gLogMsgArray[writeIdx].msgHeader));
        strncpy(gLogMsgArray[writeIdx].msgHeader, pMsgHeader,
                sizeof(gLogMsgArray[writeIdx].msgHeader)-1);
    }
    vsnprintf(gLogMsgArray[writeIdx].msg, CL_LOG_MAX_MSG_LEN, pFmtStr, vaargs);
    ++writeIdx;
    writeIdx = writeIdx % CL_LOG_MAX_NUM_MSGS;
#if 0
    /* Commented this code to avoid overwriting log records */
    if( overWriteFlag ) 
    {
        ++readIdx;
        readIdx %= CL_LOG_MAX_NUM_MSGS;
    }
#endif

out_flushcond:
    if((readIdx == writeIdx) && (0 == overWriteFlag))
    {
        overWriteFlag = 1;
    }
    else if(readIdx != writeIdx)
    {
        overWriteFlag = 0;
        logRecordDrop = CL_TRUE;
    }
    
    if(initialRecord)
        return CL_OK;

    if(deferred) 
    {
        if(!deferredFlag)
            deferredFlag = CL_TRUE;
        if(!flushPending)
            flushPending = CL_TRUE;
    }

    if(deferredFlag 
       && 
       (signalFlusher || overWriteFlag != 0 || (writeIdx % CL_LOG_FLUSH_FREQ == 0)) )
    {
        clOsalCondSignal(&gLogCond);
    }
    
    if(unlock)
    {
        rc = clOsalMutexUnlock(&gLogMutex);
        if( CL_OK != rc )
        {
            goto failure;
        }
    }

    return CL_OK;

    failure:
    return rc;
}

ClRcT
clLogWriteDeferredForce(ClHandleT       handle,
                        ClLogSeverityT  severity,
                        ClUint16T       serviceId,
                        ClUint16T       msgId,
                        ClCharT         *pFmtStr,
                        ...)
{
    ClRcT    rc     = CL_OK;
    va_list  vaargs;
    
    va_start(vaargs, pFmtStr);
    rc = logVWriteDeferred(handle, severity, serviceId, msgId, NULL, CL_TRUE, pFmtStr, vaargs);
    va_end(vaargs);
    
    return rc;
}

ClRcT
clLogWriteDeferred(ClHandleT       handle,
                   ClLogSeverityT  severity,
                   ClUint16T       serviceId,
                   ClUint16T       msgId,
                   const ClCharT         *pFmtStr,
                   ...)
{
    ClRcT    rc     = CL_OK;
    va_list  vaargs;

    va_start(vaargs, pFmtStr);
    rc = logVWriteDeferred(handle, severity, serviceId, msgId, NULL, CL_FALSE, pFmtStr, vaargs);
    va_end(vaargs);
    
    return rc;
}

ClRcT
clLogWriteDeferredForceWithHeader(ClHandleT       handle,
                                  ClLogSeverityT  severity,
                                  ClUint16T       serviceId,
                                  ClUint16T       msgId,
                                  const ClCharT         *pMsgHeader,
                                  const ClCharT         *pFmtStr,
                                  ...)
{
    ClRcT    rc     = CL_OK;
    va_list  vaargs;
    
    va_start(vaargs, pFmtStr);
    rc = logVWriteDeferred(handle, severity, serviceId, msgId, pMsgHeader, CL_TRUE, pFmtStr, vaargs);
    va_end(vaargs);
    
    return rc;
}

ClRcT
clLogWriteDeferredWithHeader(ClHandleT       handle,
                             ClLogSeverityT  severity,
                             ClUint16T       serviceId,
                             ClUint16T       msgId,
                             const ClCharT         *pMsgHeader,
                             const ClCharT         *pFmtStr,
                             ...)
{
    ClRcT    rc     = CL_OK;
    va_list  vaargs;

    va_start(vaargs, pFmtStr);
    rc = logVWriteDeferred(handle, severity, serviceId, msgId, pMsgHeader, CL_FALSE, pFmtStr, vaargs);
    va_end(vaargs);
    
    return rc;
}

ClRcT
clLogUtilLibFinalize(ClBoolT  logLibInit)
{
    ClRcT  rc = CL_OK;
    /* Notify the thread to stop */
    rc = clOsalMutexLock(&gLogMutex);
    if( CL_OK != rc )
    {
        return rc;
    }
    gUtilLibInitialized = CL_FALSE;
    clLogDebugFilterFinalize();
    /* 
     * signalling to that guy to wake up
     */
    clOsalCondSignal(&gLogCond);
    clOsalMutexUnlock(&gLogMutex);
    /* Wait till that thread finishes its own job */
    clOsalTaskJoin(taskId);

    /*
     * This is hardcoded, have to find out the best way to solve this problem
     * once all the data have been flushed, log library will be finalized.
     */
    if( CL_TRUE == logLibInit )
    {	
    	clLogFinalize(1);
    }
    /*
     * just destroying provided, all initialed libraried will be
     * finalized by main EO function. 
     */
    clOsalCondDestroy(&gLogCond);
    clOsalMutexDestroy(&gLogMutex);
    return CL_OK;
}

static void * 
clLogDeferredHandler(void  *pData)
{
    ClRcT                 rc       = CL_OK;
    ClTimerTimeOutT       timeOut  ={ 0,  0};
    ClUint16T             numRecs  = 0;
    ClLogDeferredHeaderT  msg[CL_LOG_MAX_FLUSH_COUNT];
    static ClUint16T      lastReadIdx = 0xFFFF;
    static ClUint16T      lastOverwriteFlag = 0xFFFF;
    static ClUint16T      numNotFlushed     = 0;

    while( gUtilLibInitialized )
    {
        rc = clOsalMutexLock(&gLogMutex);
        if( CL_OK != rc )
        {
            gUtilLibInitialized = CL_FALSE;
            goto threadExit;
        }
        /*
         * Sleep for two seconds, it may be woken up by 
         * writer for flushing.
         * Disable the deferred flusher thread as its obsoleted by other changes to flush to log streams
         * now.
         */
        if(1 /*clLogUtilNumRecordsTBW() <= (2 * CL_LOG_MAX_FLUSH_COUNT)*/)
        {
            //fprintf(stderr, "sleeping....getpid[%d] readIdx [%d] writeIdx [%d]\n", getpid(), readIdx,
              //      writeIdx);
            rc = clOsalCondWait(&gLogCond, &gLogMutex, timeOut);
            if( (CL_OK != rc) && (CL_GET_ERROR_CODE(rc) != CL_ERR_TIMEOUT))
            {
                clOsalMutexUnlock(&gLogMutex);
                gUtilLibInitialized = CL_FALSE;
                goto threadExit;
            }
        }
        if( lastOverwriteFlag != overWriteFlag )
        {
            /* last over write flag is not matching with overWriteFlag, 
             * so readIdx has been modifiedi by writer,
             * so making numNotFlushed to 0
             */
            numNotFlushed = 0;
        }
        if( (numNotFlushed != 0) && (numNotFlushed != numRecs) )
        {
            if( readIdx < numNotFlushed )
            {
                readIdx = CL_LOG_MAX_NUM_MSGS - ( numNotFlushed - readIdx);
            }
            else
            {
                readIdx -= numNotFlushed;
            }
        }
        /* niether everything nor nothing */
        if( (numNotFlushed == 0)  || (lastReadIdx != readIdx) )
        {
            /*
             * if the readIdx has been changed while this thread was
             * not having the lock variable, lastOverwriteFlag has been
             * modified, this means I have to change my buffer.
             */
            clLogNumFlushableRecordsGet(msg, &numRecs, &lastReadIdx,
                    &lastOverwriteFlag); 
            //fprintf(stderr, "msg repopulation, numNotFlushed: [%d] lastReadIdx [%d] readIdx [%d] numRecs [%d]\n",
              //              numNotFlushed, lastReadIdx, readIdx, numRecs);
        }
        numNotFlushed = 0;
        if( gUtilLibInitialized == CL_FALSE )
        {
            clOsalMutexUnlock(&gLogMutex);
            goto threadExit;
        }
        /* unlock it */
        rc = clOsalMutexUnlock(&gLogMutex);
        if( CL_OK != rc )
        {
            gUtilLibInitialized = CL_FALSE;
            goto threadExit;
        }
        /* Do the flushing */
        rc = clLogDoLogWrite(msg, numRecs, &numNotFlushed);
        if( CL_OK != rc )
        {
            goto threadExit;
        }
    }
threadExit:    
    if( CL_OK == rc )
    {
        clLogDoLogWrite(msg, numRecs, &numNotFlushed);
    }
    gUtilLibInitialized = CL_FALSE;
    return NULL;
}

static ClUint16T 
clLogUtilNumRecordsTBW(void)
{
    ClUint16T  numRecs = 0;

    //fprintf(stderr, "readIdx: %d writeIdx: %d\n", readIdx, writeIdx);
    if( (readIdx == writeIdx) && (overWriteFlag == 1))
    {
        numRecs  = CL_LOG_MAX_NUM_MSGS - 1;
    }
    else if( (readIdx <= writeIdx) )
    {
        numRecs  = writeIdx - readIdx;
    }
    else if( (readIdx > writeIdx) && (overWriteFlag == 0) )
    {
        numRecs  = (CL_LOG_MAX_NUM_MSGS - readIdx) + writeIdx;
    }
    else
    {
        fprintf(stderr, "readIdx & writeIdx updation went wrong readIdx [%d] writeIdx [%d] overWriteFlag [%d]\n", 
                    readIdx, writeIdx, overWriteFlag);
        /* Should not occur */
    }
    return numRecs;
}

static 
ClRcT
clLogNumFlushableRecordsGet(ClLogDeferredHeaderT  *pMsg,
                            ClUint16T             *pNumRecs,
                            ClUint16T             *pReadIdx,
                            ClUint16T             *pOverwriteFlag)
{
    ClUint16T  remRecords = 0;
   /* 
    * this buffer will be having the memory to send it to writeasync
    */
    if( (*pNumRecs = clLogUtilNumRecordsTBW()) > CL_LOG_MAX_FLUSH_COUNT )
    {
        *pNumRecs = CL_LOG_MAX_FLUSH_COUNT;
    }
    memset(pMsg, '\0', (*pNumRecs) * sizeof(ClLogDeferredHeaderT));
    if( *pNumRecs <= (CL_LOG_MAX_NUM_MSGS - readIdx) )
    {
        memcpy(pMsg, &gLogMsgArray[readIdx], *pNumRecs *
           sizeof(ClLogDeferredHeaderT));
    }
    else
    {
        memcpy(pMsg, &gLogMsgArray[readIdx], (CL_LOG_MAX_NUM_MSGS - readIdx) * 
                sizeof(ClLogDeferredHeaderT));
        remRecords = *pNumRecs - (CL_LOG_MAX_NUM_MSGS - readIdx);
        memcpy(pMsg + (CL_LOG_MAX_NUM_MSGS - readIdx), &gLogMsgArray[0], 
                remRecords * sizeof(ClLogDeferredHeaderT));
    }
   /*
    * Just increasing the readIdx, flipping overWriteFlag
    */
    readIdx  = (readIdx + *pNumRecs) % CL_LOG_MAX_NUM_MSGS;
    if( *pNumRecs != 0 )
    {
        overWriteFlag = 0;
        logRecordDrop = CL_TRUE;
    }
    *pOverwriteFlag = overWriteFlag; 
    *pReadIdx = readIdx;

    return CL_OK;
}

static ClRcT
clLogDoLogWrite(ClLogDeferredHeaderT  *pMsg, 
                ClUint16T             numRecs,
                ClUint16T             *pNumNotFlushed)
{
   ClUint16T  idx = 0;
   ClRcT      rc  = CL_OK;

   *pNumNotFlushed = numRecs;
   while( idx < numRecs )
   {
        /*
         * If the handle is 0, redirecting the output to SYS file,
         * two reasons, those are ASP components logs came before 
         * log library initialization, or really bad handle logs,
         * either case just redirecting to the file
         */
        if( pMsg[idx].handle == CL_HANDLE_INVALID_VALUE )
        {
            if( CL_LOG_HANDLE_SYS == CL_HANDLE_INVALID_VALUE )
            {
                /* still log library has not been initialized */
                idx++;
                continue;
            }
            pMsg[idx].handle = CL_LOG_HANDLE_SYS;
        }
        rc = clLogWriteWithHeader(pMsg[idx].handle,
                                  pMsg[idx].severity,
                                  pMsg[idx].serviceId,
                                  pMsg[idx].msgId,   
                                  pMsg[idx].msgHeader,
                                  "%s", pMsg[idx].msg);
        if( CL_OK != rc ) 
        {
            if( CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE )
            {
                idx++;
                continue;
            }
            /*
             * log client is not seem to be ready, 
             * just go back and sleep 
             */
            break;      
        }
        idx++;
    }
   /* return num of not flushed records  */
    if( idx != 0 )
    {
      *pNumNotFlushed -= idx;
      //fprintf(stderr, "Num of not flushed records [%d], written [%d] pid [%d]\n", *pNumNotFlushed,
        //      idx, getpid());
    }
    
    return CL_OK;
}

void parseMultiline(ClCharT **ppMsg, const ClCharT *pFmt, ...)
{
    va_list  vaargs;
    ClCharT c;
    ClInt32T len = 0;
    
    if(!ppMsg) return;
    *ppMsg = NULL;

    va_start(vaargs, pFmt);
    len = vsnprintf((ClCharT*)&c, 1, pFmt, vaargs);
    va_end(vaargs);

    if(!len) return;

    ++len;
    *ppMsg = (ClCharT*)clHeapCalloc(1, len+1);
    if(!*ppMsg) return;

    va_start(vaargs, pFmt);
    vsnprintf(*ppMsg, len, pFmt, vaargs);
    va_end(vaargs);
    
}
