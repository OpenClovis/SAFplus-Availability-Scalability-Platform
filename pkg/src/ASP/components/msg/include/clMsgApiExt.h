#ifndef _CL_MSG_API_EXT_H_

#define _CL_MSG_API_EXT_H_

#include <clCommon.h>
#include <saMsg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT clMsgDispatchQueueRegister(SaMsgQueueHandleT queueHandle, 
                                        SaMsgMessageReceivedCallbackT callback);

extern ClRcT clMsgDispatchQueueDeregister(SaMsgQueueHandleT queueHandle);

#ifdef __cplusplus
}
#endif

#endif
