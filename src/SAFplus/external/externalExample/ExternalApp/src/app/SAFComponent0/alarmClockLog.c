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
#include "alarmClockLog.h"
#include <clCommon.h>
/* copied over from clCompAppMain.c: goes to the app log
 */
#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  CL_LOG_AREA_UNSPECIFIED, \
                                  CL_LOG_CONTEXT_UNSPECIFIED,\
                                  __VA_ARGS__)

static ClLogHandleT        logSvcHandle = CL_HANDLE_INVALID_VALUE;

ClLogStreamHandleT  streamHandle = CL_HANDLE_INVALID_VALUE;
ClLogStreamAttributesT myStreamAttr;

ClRcT alarmClockLogInitialize( void )
{
    ClRcT      rc     = CL_OK;
    ClVersionT version= {'B', 0x1, 0x1};
    ClLogCallbacksT  logCallbacks = {0};
    SaNameT       streamName;
    logSvcHandle = CL_HANDLE_INVALID_VALUE; 
    streamHandle = CL_HANDLE_INVALID_VALUE;
    rc = clLogInitialize(&logSvcHandle, &logCallbacks, &version); 
    if(CL_OK != rc) 
    {
        // Error occured. Take appropriate action.
        clprintf(CL_LOG_SEV_ERROR, "Failed to initialze log: %x\n", rc);
        return rc;
    }
    sleep(5);
    myStreamAttr.fileName = (char *)"clock.log";            
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
    saNameSet(&streamName,"clockStream");    
    /* open the clock stream 
     * ClLogHandleT  - returned with clLogInitialize()
     * ClNameT       - name of the stream to open
     * ClLogStreamScopeT  - scope can be global (cluster wide) or local
     * ClLogStreamAttributesT * - set to null because this is precreated stream
     * ClLogStreamOpenFlagsT  -  no stream open flags specified
     * ClTimeT   timeout      - timeout set to zero, if failed return immed.
     * ClLogStreamHandleT *   - stream handle returned if successful
    */
    printf("....................open clockStream.................... \n");
    rc = clLogStreamOpen(logSvcHandle, 
                         streamName, 
                         CL_LOG_STREAM_GLOBAL,
                         NULL, 
                         0,
                         0, 
                         &streamHandle); 
    if(CL_OK != rc)
    {
        /* error occurred. Close Log Service handle and
           return error code
         */
        clprintf(CL_LOG_SEV_ERROR, "Failed to open clockStream : %x\n", rc);
        (void)clLogFinalize(logSvcHandle);
        return rc;
    }
    printf("......................write record to to clockStream.................... \n");
    int count=0;
    do
    {
        count ++;
        rc = clLogWriteAsync(streamHandle, 
                         CL_LOG_SEV_NOTICE, 
                         10, 
                         1,
                         "\n(%s:%d[pid=%d]) -->Alarm Clock Logging Begun<--\n",
                         __FILE__, __LINE__,getpid()); 
        sleep(1);
      
    }while(count <=2);

    printf("......................close clockStream.................... \n");
    rc = clLogStreamClose(streamHandle);
    return CL_OK;
}



void alarmClockLogFinalize( void )
{
    ClRcT      rc     = CL_OK;

    rc = clLogWriteAsync(streamHandle, 
                         CL_LOG_SEV_NOTICE, 
                         10, 
                         CL_LOG_MSGID_PRINTF_FMT,
                         "\n(%s:%d[pid=%d]) -->Alarm Clock Logging Ends<--\n",
                         __FILE__, __LINE__,getpid()); 
                         
    if(CL_OK != rc)
    {
        // Error occured. Take appropriate action.
        clprintf(CL_LOG_SEV_ERROR, "Failed to write to clockStream : %x\n", rc);
    }

    /* Closes a stream
     */
    rc = clLogStreamClose(streamHandle);
    if(CL_OK != rc) 
    {
        // Error occured. Take appropriate action.
        clprintf(CL_LOG_SEV_ERROR, "Failed to close clockStream : %x\n", rc);
    }
    (void)clLogFinalize(logSvcHandle);
}
