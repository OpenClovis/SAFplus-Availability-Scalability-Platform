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

extern ClRcT clMsgQueuePersistRedundancy(const SaNameT *queue, const SaNameT *node);

/**
 * Initialize a consistent hash ring for a MSG queue group
 * and allocate a MsgQueueGroupHashesT to store the hash for receiver lookup
 */
extern ClRcT clMsgQueueGroupHashInit(const SaNameT *group, ClInt32T nodes, ClInt32T hashesPerNode);

/**
 * Free memory allocated for consistent hash of a group
 */
extern ClRcT clMsgQueueGroupHashFinalize(const SaNameT *group);

/**
 * Lookup and message to the receiver in the queue group that map with the key
 */
extern SaAisErrorT clMsgQueueGroupSendWithKeySynch(SaMsgHandleT msgHandle,
                                                   const SaNameT *group, SaMsgMessageT *message,
                                                   ClCharT *key, ClInt32T keylen,
                                                   SaTimeT timeout);
extern SaAisErrorT clMsgQueueGroupSendWithKeyAsync(SaMsgHandleT msgHandle,
                                                   SaInvocationT invocation,
                                                   const SaNameT *group, SaMsgMessageT *message,
                                                   ClCharT *key, ClInt32T keylen,
                                                   SaMsgAckFlagsT ackFlags);



#ifdef __cplusplus
}
#endif

#endif
