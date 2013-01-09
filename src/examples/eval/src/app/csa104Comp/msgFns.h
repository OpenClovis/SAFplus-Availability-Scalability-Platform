#ifndef MSGFNS_H
#define MSGFNS_H

#include <saMsg.h>
#include <clLogApi.h>

extern ClLogStreamHandleT  gEvalLogStream;

void        msgInitialize(void);
SaAisErrorT msgOpen(const char* queuename,int bytesPerPriority);
SaAisErrorT msgSend(const char* queuename, void* buffer, int length);
void*       msgReceiverLoop(void * notused);

#define ACTIVE_COMP_QUEUE "csa104msgqueue"
#define QUEUE_LENGTH 2048

#define clprintf(severity, ...)   clAppLog(gEvalLogStream, severity, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, __VA_ARGS__)

#endif
