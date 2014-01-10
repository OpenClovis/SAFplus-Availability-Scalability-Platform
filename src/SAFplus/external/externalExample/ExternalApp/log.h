#ifndef ALARM_CLOCK_LOG_H
#define ALARM_CLOCK_LOG_H

extern ClLogStreamHandleT  logStreamHandle;

// #define logWrite(severity, ...)   clAppLog(logStreamHandle, severity, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, __VA_ARGS__)

#define logWrite( severity, ... ) clLogWriteAsync(logStreamHandle, severity, 10, CL_LOG_MSGID_PRINTF_FMT, __VA_ARGS__)

ClRcT logInitialize( void );
void logFinalize( void );

#endif
