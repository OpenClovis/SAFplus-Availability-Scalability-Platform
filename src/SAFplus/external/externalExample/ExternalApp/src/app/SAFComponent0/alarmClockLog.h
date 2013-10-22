#ifndef ALARM_CLOCK_LOG_H
#define ALARM_CLOCK_LOG_H

//extern ClLogStreamHandleT  streamHandle;

#define AlarmClockLogId 1

#define alarmClockLogWrite( severity, ... ) \
    (void)clLogWriteAsync(streamHandle, \
                         severity, \
                         10, \
                         CL_LOG_MSGID_PRINTF_FMT,\
                         __VA_ARGS__) 

ClRcT alarmClockLogInitialize( void );
void alarmClockLogFinalize( void );

#endif
