/*****************************************************************************
 * File Name  : alarmClockLog.c
 *
 * Description: Basic API to initialize ASCII based logging  
 *
 *
 ****************************************************************************/
#include <stdarg.h>
#include <clCommon.h>
#include <clHandleApi.h>
#include <clLogApi.h>
#include <clCommon.h>

#include "log.h"

static ClLogHandleT        logSvcHandle = CL_HANDLE_INVALID_VALUE;

ClLogStreamHandleT  logStreamHandle = CL_HANDLE_INVALID_VALUE;
ClLogStreamAttributesT myStreamAttr;

ClRcT logInitialize( void )
{
    ClRcT      rc     = CL_OK;
    ClVersionT version= {'B', 0x1, 0x1};
    ClLogCallbacksT  logCallbacks = {0};
    SaNameT       streamName;
    logSvcHandle = CL_HANDLE_INVALID_VALUE; 
    logStreamHandle = CL_HANDLE_INVALID_VALUE;
    rc = clLogInitialize(&logSvcHandle, &logCallbacks, &version); 
    if(CL_OK != rc) 
    {
        // Error occured. Take appropriate action.
        printf("Failed to initialze log: %x\n", rc);
        return rc;
    }
    //sleep(5);
    
    myStreamAttr.fileName = (char *)"external.log";            
    myStreamAttr.fileLocation=(char *)".:var/log";
    myStreamAttr.recordSize = 300;
    myStreamAttr.fileUnitSize = 1000000;
    myStreamAttr.fileFullAction = CL_LOG_FILE_FULL_ACTION_ROTATE;
    myStreamAttr.maxFilesRotated = 10;
    myStreamAttr.flushFreq = 10;
    myStreamAttr.haProperty = 0;
    myStreamAttr.flushInterval = 1 * (1000L * 1000L * 1000L);
    myStreamAttr.waterMark.lowLimit = 0;
    myStreamAttr.waterMark.highLimit = 0;
    myStreamAttr.syslog = CL_FALSE;

    /* Stream Name is defined in the IDE during
     * modeling phase
     */
    saNameSet(&streamName,"externalAppStream");    
    /* open the clock stream 
     * ClLogHandleT  - returned with clLogInitialize()
     * ClNameT       - name of the stream to open
     * ClLogStreamScopeT  - scope can be global (cluster wide) or local
     * ClLogStreamAttributesT * - set to null because this is precreated stream
     * ClLogStreamOpenFlagsT  -  no stream open flags specified
     * ClTimeT   timeout      - timeout set to zero, if failed return immed.
     * ClLogStreamHandleT *   - stream handle returned if successful
    */
    printf("opening externalAppStream\n");
    rc = clLogStreamOpen(logSvcHandle, 
                         streamName, 
                         CL_LOG_STREAM_GLOBAL,
                         NULL, 
                         0,
                         0, 
                         &logStreamHandle); 
    if(CL_OK != rc)
    {
        /* error occurred. Close Log Service handle and
           return error code
         */
        printf("Failed to open logging stream: %x\n", rc);
        return rc;
    }
    sleep(3);
    
    printf("Write record to to log Stream\n");
    clLogWriteAsync(logStreamHandle,CL_LOG_SEV_NOTICE,10,1,"(%s:%d[pid=%d]) Logging from external app initialized and ready.", __FILE__, __LINE__,getpid()); 

    return CL_OK;
}



void logFinalize( void )
{
    ClRcT      rc     = CL_OK;

    rc = clLogWriteAsync(logStreamHandle, 
                         CL_LOG_SEV_NOTICE, 
                         10, 
                         CL_LOG_MSGID_PRINTF_FMT,
                         "(%s:%d[pid=%d]) External application closing this log",
                         __FILE__, __LINE__,getpid()); 
                         
    if(CL_OK != rc)
    {
        // Error occured. Take appropriate action.
        printf("Failed to write to SAFplus log: %x\n", rc);
    }

    /* Closes a stream
     */
    rc = clLogStreamClose(logStreamHandle);
    if(CL_OK != rc) 
    {
        // Error occured. Take appropriate action.
        printf("Failed to close SAFplus log : %x\n", rc);
    }
    logStreamHandle = CL_HANDLE_INVALID_VALUE;
    (void)clLogFinalize(logSvcHandle);
    logSvcHandle=CL_HANDLE_INVALID_VALUE;
}
