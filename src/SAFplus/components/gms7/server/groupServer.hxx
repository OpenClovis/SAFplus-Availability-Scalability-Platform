#ifndef groupServer_hxx
#define groupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clGroup.hxx>
#include <clCpmApi.h>
#include "groupMessageHandler.hxx"

#define GMS_PORT 69
#define AMF_PORT 17

typedef enum
{
  CLUSTER_NODE_ARRIVAL,
  CLUSTER_NODE_ROLE_NOTIFY
} GroupMessageTypeT;

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
  ClIocAddressT     *eAddress;
  int                groupId;
  int                numberOfItems;
  void*              data;
} GroupMessageProtocolT;

/* Functions to process message from Master Node */
void NodeJoinFromMaster(GroupMessageProtocolT *msg);
void RoleChangeFromMaster(GroupMessageProtocolT *msg);

/* Functions to process notification from AMF */
void NodeJoin(ClIocAddressT *pAddress);
void NodeLeave(ClIocAddressT *pAddress);
void ComponentJoin(ClIocAddressT *pAddress);
void ComponentLeave(ClIocAddressT *pAddress);

/* Utility functions */
ClRcT initializeServices();
void  getNodeInfo(ClIocAddressT* pAddress, SAFplus::GroupIdentity *grpIdentity);
bool  isMasterNode();
void  sendNotification(GroupMessageTypeT messageType,void* data, GroupMessageSendModeT messageMode =  SEND_BROADCAST);

/* Global variables */


#endif
