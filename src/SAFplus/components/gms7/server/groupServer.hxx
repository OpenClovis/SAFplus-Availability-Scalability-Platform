#ifndef groupServer_hxx
#define groupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clGroup.hxx>
#include <clCpmApi.h>
#include "groupMessageHandler.hxx"
#include "clNodeCache.h"

#define IOC_PORT 0
#define GMS_PORT 6
#define AMF_PORT 17

typedef enum
{
  CLUSTER_NODE_ARRIVAL,
  CLUSTER_NODE_ROLE_NOTIFY
} GroupMessageTypeT;

typedef enum
{
  ROLE_ACTIVE,
  ROLE_STANDBY
} GroupRoleNotifyTypeT;

typedef enum
{
  SEND_BROADCAST,
  SEND_TO_MASTER,
  LOCAL_ROUND_ROBIN
} GroupMessageSendModeT;

typedef enum
{
  GMS_MESSAGE,
  AMF_MESSAGE
} MessageHandlerCookie;

typedef struct
{
  GroupMessageTypeT  messageType;
  GroupRoleNotifyTypeT  roleType;
  void*              data;
} GroupMessageProtocolT;

/* Functions to process message from Master Node */
void nodeJoinFromMaster(GroupMessageProtocolT *msg);
void roleChangeFromMaster(GroupMessageProtocolT *msg);

/* Functions to process notification from AMF */
void nodeJoin(ClIocAddressT *pAddress);
void nodeLeave(ClIocAddressT *pAddress);
void componentJoin(ClIocAddressT *pAddress);
void componentLeave(ClIocAddressT *pAddress);

/* Utility functions */
ClRcT initializeServices();
void  getNodeInfo(ClIocAddressT* pAddress, SAFplus::GroupIdentity *grpIdentity);
bool  isMasterNode();
void  sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode =  SEND_BROADCAST);
SAFplus::EntityIdentifier createHandleFromAddress(ClIocAddressT* pAddress, int pid = 0);

/* Global variables */


#endif
