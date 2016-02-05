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
 * Send a message to members of the queue group with a key for uniform distribution.
 * It uses consistent hash to map a given key to a member of the queue group
 * A key based message send with consistent hash ensures that given a set of servers,
 * a set of keys would always map to the same server. 
 * Addition or deletion of servers or members from the queue group would result in
 * only a few set of keys getting remapped for load distribution.
 * This can be used as another load balancing option to existing 
 * SAF specific load balancing options for queue group message sends.
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
/**
 * Structure that includes a struct iovec which is used to send multiple data chunks.
 */
typedef struct iovec iovec_t;

typedef struct ClMsgMessageIovec
{
    SaUint32T type;
    SaVersionT version;
    SaNameT *senderName;
    SaUint8T priority;
    iovec_t *pIovec;
    ClUint32T numIovecs;
    SaUint32T messageId;
} ClMsgMessageIovecT;

/**
 * APIs take a struct iovec as an argument and send multiple data chunks;
 */
extern SaAisErrorT clMsgMessageSendIovec (SaMsgHandleT msgHandle,
                                                   const SaNameT *destination,
                                                   const ClMsgMessageIovecT *message,
                                                   SaTimeT timeout);
extern SaAisErrorT clMsgMessageSendAsyncIovec (SaMsgHandleT msgHandle,
                                                   SaInvocationT invocation,
                                                   const SaNameT *destination,
                                                   const ClMsgMessageIovecT *message,
                                                   SaMsgAckFlagsT ackFlags);
extern SaAisErrorT clMsgQueueGroupSendWithKeySynchIovec(SaMsgHandleT msgHandle,
                                                   const SaNameT *group, ClMsgMessageIovecT *message,
                                                   ClCharT *key, ClInt32T keylen,
                                                   SaTimeT timeout);
extern SaAisErrorT clMsgQueueGroupSendWithKeyAsyncIovec(SaMsgHandleT msgHandle,
                                                   SaInvocationT invocation,
                                                   const SaNameT *group, ClMsgMessageIovecT *message,
                                                   ClCharT *key, ClInt32T keylen,
                                                   SaMsgAckFlagsT ackFlags);

#ifdef __cplusplus
}
#endif

#endif
