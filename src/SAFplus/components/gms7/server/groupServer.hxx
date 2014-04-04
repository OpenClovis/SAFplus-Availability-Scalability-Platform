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
#define GMS_PORT CL_IOC_GMS_PORT

typedef enum
{
  NODE_JOIN_FROM_SC,
  CLUSTER_NODE_ROLE_NOTIFY,
  NODE_JOIN_FROM_CACHE
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
  GMS_MESSAGE = 1,
  AMF_MESSAGE = 2
} MessageHandlerCookie;

typedef struct
{
  GroupMessageTypeT  messageType;
  GroupRoleNotifyTypeT  roleType;
  char                data[1]; //Will be malloc on larger memory
} GroupMessageProtocolT;

/* Functions to process message from Master Node */
void nodeJoinFromMaster(GroupMessageProtocolT *msg);
void roleChangeFromMaster(GroupMessageProtocolT *msg);

/* Functions to process notification from AMF */
void nodeJoin(ClIocNodeAddressT nAddress);
void nodeLeave(ClIocNodeAddressT nAddress);
void componentJoin(ClIocAddressT *pAddress);
void componentLeave(ClIocAddressT *pAddress);
void elect();
/* Utility functions */
ClRcT initializeServices();
ClRcT initializeClusterNodeGroup();
void  getNodeInfo(ClIocNodeAddressT nAddress, SAFplus::GroupIdentity *grpIdentity);
bool  isMasterNode();
void  sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode =  SEND_BROADCAST);
SAFplus::EntityIdentifier createHandleFromAddress(ClIocNodeAddressT nAddress, int pid = 0);

/* Global variables */


#endif
