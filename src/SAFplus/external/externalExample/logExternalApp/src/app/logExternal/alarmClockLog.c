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


    /* 
    Initialize the client log library.

        ClLogHandleT    *phLog,
                - used for subsequent invocation of Log Service APIs

        ClLogCallbacksT *pLogCallbacks,
          ClLogFilterSetCallbackT - Callback after filter update is completed.
          ClLogRecordDeliveryCallbackT - Callback for retrieving records 
          ClLogStreamOpenCallbackT     - Callback for clLogStreamOpenAsync()

        ClVersionT         *pVersion`
    */

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
//    saNameSet(&streamName,"clockStream");    
    saNameSet(&streamName,"clockStream");    
    //strcpy(streamName.value, "clockStream");
    //streamName.length = strlen("clockStream");


    /* open the clock stream 
     * ClLogHandleT  - returned with clLogInitialize()
     * ClNameT       - name of the stream to open
     * ClLogStreamScopeT  - scope can be global (cluster wide) or local
     * ClLogStreamAttributesT * - set to null because this is precreated stream
     * ClLogStreamOpenFlagsT  -  no stream open flags specified
     * ClTimeT   timeout      - timeout set to zero, if failed return immed.
     * ClLogStreamHandleT *   - stream handle returned if successful
    */
    clprintf(CL_LOG_SEV_ERROR, "open clockStream \n");
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

#if 1   
    /* Log a message to the stream being created
     * ClLogStreamHandleT   - returned from clLogStreamOpen()
     * ClLogSeverityT       - defined in clLogApi.h
     * ClUint16T  serviceid - identifies the module within a process that 
     *                       generates the log message
     * ClUint16T   msgId    - identifies msg type being logged
     *                      - CL_LOG_MSGID_PRINTF_FMT (ASCII)
     *                      - CL_LOG_MSGID_BUFFER     (Binary)
     *                             
     */
    clprintf(CL_LOG_SEV_ERROR, "WRITE clockStream \n");
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
        sleep(5);
      
    }while(count <=2);
#endif
    sleep(30);
    //rc = clLogStreamClose(streamHandle);
    /**
 *  \brief Logging for applications
 *  \par Description
 *  This macro provides the support to log messages by specifying
 *  the severity of log message and server information like the 
 *  sub-component area and the context of logging.
 *  \par
 *  This macro is for applications as opposed to system components. By default
 *  it outputs to "app.log" instead of "sys.log" but of course these outputs
 *  are configurable through the clLog.xml file.
 *
 *  \param streamHandle (in) This handle should be a valid stream handle. It should be 
 *   either default handles(CL_LOG_HANDLE_APP & CL_LOG_HANDLE_SYS) or should 
 *  have obtained from previous clLogStreamOpen() call. The stream handle is CL_LOG_HANDLE_SYS,
 *  then it outputs to "sys.log". It is CL_LOG_HANDLE_APP, then it outputs to "app.log". 
 * 
 *  \param severity (in) This field must be set to one of the values defined 
 *  in this file (CL_LOG_SEV_DEBUG ... CL_LOG_SEV_EMERGENCY).  It defines the 
 *  severity level of the Log Record being written.
 *
 *  \param serviceId (in) This field identifies the module within the process
 *  which is generating this Log Record. If the Log Record message is a generic
 *  one like out of memory, this field can be used to narrow down on the module 
 *  impacted. For SAFplus client libraries, these values are defined in clCommon.h.
 *  For application modules, it is up-to the application developer to define the
 *  values and scope of those values.
 *
 *  \param area (in) This is a 3 letter string identifying the process (or 
 *  component) that generated this message.
 *
 *  \param context (in) This is a string (3 letters preferred) identifying 
 *  the library that generated this message.
 *
 *  \param ... (in) use a printf style string and arguments for your log message.
 */

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
    
    //
    (void)clLogFinalize(logSvcHandle);
}
