#ifndef ALARM_CLOCK_LOG_H
#define ALARM_CLOCK_LOG_H

#include <clLogApi.h>

#define alarmClockLogWrite(severity, ...)  do {         \
        (void)clLogMsgWrite(CL_LOG_HANDLE_APP,          \
                            severity,                   \
                            10,                         \
                            CL_LOG_AREA_UNSPECIFIED,    \
                            CL_LOG_CONTEXT_UNSPECIFIED, \
                            __FILE__, __LINE__,         \
                            __VA_ARGS__) ;              \
}while(0)
    
#endif
