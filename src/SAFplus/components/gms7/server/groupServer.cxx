#include "clSafplusMsgServer.hxx"
#include "groupServer.hxx"
#include <clGlobals.hxx>


using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

/* SAFplus specified variables */
#ifdef __TEST
ClUint32T clAspLocalId = atoi(getenv("NODEADDR"));
ClBoolT gIsNodeRepresentative = CL_TRUE;
#define logEmergency(area, context,M, ...)  fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logAlert(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logCritical(area, context,M,...)    fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logError(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logWarning(area, context,M,...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logNotice(area, context,M, ...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logInfo(area, context, M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logDebug(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logTrace(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define GMS_PORT_BASE 90
static int numOfEntity = 0;
#endif // __TEST
/* Well-known groups */
SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");
SAFplus::Group    clusterCompGrp("CLUSTER_COMP");
GroupMessageHandler         *groupMessageHandler;
SAFplus::SafplusMsgServer   *groupMsgServer;
/*
GROUP SERVICE DESIGNATION
*************************************************************************
* - Running on some SC Nodes (a Master - active role - and several Slaves)
* - Tracking the Nodes/Components membership and role
* - Initialize:
*     - Create cluster node group from cache
*     - If master, broadcast to other nodes
*     - If slave:
*         - Add new information
*         - Send to master my node address
* - When a component join/leave/failed:
*     - All GMS services will do appropriate action on that membership changes
*     - No broadcast needed
* - When a node join:
*     - Master node will add information of newly node (filter admission)
*     - Master node will send broadcast of newly node to all nodes
*     - Slave nodes will add information (received from server), no need to broadcast
* - When a node leave:
*     - All GMS services will update the GMS data
*     - If the left node is a Master Node:
*         - Slave Node (with standby role)  entity will become Master Node (with active role)
*         - Broadcast role change to other nodes
*         - Master Node will elect for a new Standby role
*         - Broadcast role change to other nodes
*     - If the left node is a Slave Node and was taking Standby role
*         - Master Node will elect for a new Standby role
*         - Broadcast role change to other nodes
*     - Others: do nothing
*************************************************************************
*/
int main(int argc, char* argv[])
{
  ClRcT rc;

  rc = initializeServices();
  if(CL_OK != rc)
  {
    logError("GMS","SERVER","Initialize server error [%#x]",rc);
    return rc;
  }
#ifdef __TEST
  if(clAspLocalId == 1) //MASTER
  {
    clNodeCacheCapabilitySet(1,2,CL_NODE_CACHE_CAP_ASSIGN);
  }
  else
  {
    clNodeCacheCapabilitySet(clAspLocalId,1,CL_NODE_CACHE_CAP_ASSIGN);
  }
#endif // __TEST
  /* Read from node cache to know node information */
  rc = initializeClusterNodeGroup();
  if(CL_OK != rc)
  {
    logError("GMS","SERVER","Initialize cluster node group error [%#x]",rc);
    return rc;
  }

  /* Update cluster node group membership from NodeCache*/
  logInfo("GMS","SERVER","Initialize successfully");
  /* Dispatch */
  while(1)
  {
    ;
  }
  return 0;
}

ClRcT initializeServices()
{
  ClRcT   rc            = CL_OK;
  int     messageScope  = GMS_MESSAGE;
  ClIocNodeAddressT     currentLeader;
  /* Initialize log service */
  SAFplus::logCompName = "GroupServer";
  logSeverity = LOG_SEV_MAX;
  logInitialize();

  /* Initialize necessary libraries */
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {
    return rc;
  }
  clIocLibInitialize(NULL);
#ifdef __TEST
  /* Setting leader node */
  rc = clNodeCacheLeaderGet(&currentLeader);
  if(rc == CL_ERR_NOT_EXIST)
  {
    clNodeCacheLeaderUpdate(1);
  }
#endif
  /* Initialize message handler */
  groupMessageHandler = new GroupMessageHandler();
#ifndef __TEST
  groupMsgServer = new SAFplus::SafplusMsgServer(GMS_PORT);
#else
  groupMsgServer = new SAFplus::SafplusMsgServer(GMS_PORT_BASE + clAspLocalId);
#endif
  messageScope  = 1;
  groupMsgServer->RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, (int *)&messageScope);

  /* Start the message handlers */
  groupMsgServer->Start();
}

ClRcT initializeClusterNodeGroup()
{
  ClNodeCacheMemberT *pMembers = NULL;
  ClIocAddressT nodeAddress;
  ClUint32T i;
  ClUint32T maxNodes = CL_IOC_MAX_NODES;
  ClRcT rc = CL_OK;
  pMembers = (ClNodeCacheMemberT*) clHeapCalloc(CL_IOC_MAX_NODES, sizeof(*pMembers));
  CL_ASSERT(pMembers != NULL);
  rc = clNodeCacheViewGet(pMembers, &maxNodes);
  if(rc != CL_OK)
  {
    logError("GMS", "SERVER", "Node cache view get failed with [%#x]", rc);
    goto out_free;
  }
  for(i = 0; i < maxNodes; ++i)
  {
    logInfo("GMS","SERVER","Information from cache: address[%d], capability[%d]",pMembers[i].address,pMembers[i].capability);
    nodeJoin(pMembers[i].address);
  }

out_free:
  clHeapFree(pMembers);
  return rc;
}

void nodeJoinFromMaster(GroupMessageProtocolT *msg)
{
  if(msg == NULL || msg->data == NULL)
  {
    logDebug("GMS","SERVER","Received message is NULL");
    return;
  }
  GroupIdentity grpIdentity = *(GroupIdentity *)msg->data;

  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER","Node is already group member");
    return;
  }
  clusterNodeGrp.registerEntity(grpIdentity);
#ifdef __TEST
  numOfEntity ++;
#endif // __TEST
}

void nodeLeave(ClIocNodeAddressT nAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(nAddress,&grpIdentity);
  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    EntityIdentifier curActive = clusterNodeGrp.getActive();
    EntityIdentifier curStandby = clusterNodeGrp.getStandby();
    clusterNodeGrp.deregister(grpIdentity.id);
    logDebug("GMS","SERVER","Node had been removed from cluster node group");
    if(curActive == grpIdentity.id)
    {
      logDebug("GMS","SERVER","Leaving node had active role. Switching over");
      /*Update Node with standby role to active role*/
      EntityIdentifier standbyNode = clusterNodeGrp.getStandby();
      clusterNodeGrp.setActive(standbyNode);

      /* Send role change notification */
      GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      sndMessage->messageType = CLUSTER_NODE_ROLE_NOTIFY;
      sndMessage->roleType    = ROLE_ACTIVE;
      memcpy(sndMessage->data,&standbyNode,sizeof(EntityIdentifier));
      sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      clHeapFree(sndMessage);

      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        logDebug("GMS","SERVER","Re-elect standby role from Master node");
        std::pair<EntityIdentifier,EntityIdentifier> electResult;
        clusterNodeGrp.elect(electResult,SAFplus::Group::ELECTION_TYPE_STANDBY);
        if(electResult.second == INVALID_HDL)
        {
          logError("GMS","SERVER","Can't elect for standby role");
          return;
        }
        logDebug("GMS","TEST","Standby role had been elected node: [%d],capabilities: [%d]  ",electResult.second.getNode(),clusterNodeGrp.getCapabilities(electResult.second));
        standbyNode = electResult.second;
        clusterNodeGrp.setStandby(standbyNode);
        logDebug("GMS","SERVER","Notify about new standby role");
        /* Send role change notification */
        GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
        sndMessage->messageType = CLUSTER_NODE_ROLE_NOTIFY;
        sndMessage->roleType    = ROLE_STANDBY;
        memcpy(sndMessage->data,&standbyNode,sizeof(EntityIdentifier));
        sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
        clHeapFree(sndMessage);
      }
    }
    else if(curStandby == grpIdentity.id)
    {
      logDebug("GMS","SERVER","Leaving node had standby role.");
      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        logDebug("GMS","SERVER","Re-elect standby role from Master node");
        std::pair<EntityIdentifier,EntityIdentifier> electResult;
        clusterNodeGrp.elect(electResult,SAFplus::Group::ELECTION_TYPE_STANDBY);
        if(electResult.second == INVALID_HDL)
        {
          logError("GMS","SERVER","Can't elect for standby role");
          return;
        }
        logDebug("GMS","TEST","Standby role had been elected node: [%d],capabilities: [%d]  ",electResult.second.getNode(),clusterNodeGrp.getCapabilities(electResult.second));
        EntityIdentifier standbyNode = electResult.second;
        clusterNodeGrp.setStandby(standbyNode);
        logDebug("GMS","SERVER","Notify about new standby role");
        /* Send role change notification */
        GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
        sndMessage->messageType = CLUSTER_NODE_ROLE_NOTIFY;
        sndMessage->roleType    = ROLE_STANDBY;
        memcpy(sndMessage->data,&standbyNode,sizeof(EntityIdentifier));
        sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
        clHeapFree(sndMessage);
      }
    }
    /* Remove all entity which belong to this node */
    SAFplus::Group::Iterator iter = clusterCompGrp.begin();
    while(iter != clusterCompGrp.end())
    {
      SAFplusI::BufferPtr curkey = iter->first;
      EntityIdentifier item = *(EntityIdentifier *)(curkey.get()->data);
      if(item.getNode() == nAddress)
      {
        clusterCompGrp.deregister(item);
      }
    }
  }
  else
  {
    logDebug("GMS","SERVER","Node isn't a group member");
  }
}

void nodeJoin(ClIocNodeAddressT nAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(nAddress,&grpIdentity);
  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    logInfo("GMS","SERVER","Node already a member");
    return;
  }

  if(isMasterNode())
  {
    clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
#ifdef __TEST
    numOfEntity++;
#endif // __TEST
    /* Send node join broadcast message */
    GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(GroupIdentity));
    sndMessage->messageType = NODE_JOIN_FROM_SC;
    memcpy(sndMessage->data,&grpIdentity,sizeof(GroupIdentity));
    sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(GroupIdentity));
    clHeapFree(sndMessage);
  }
  else
  {
    clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
#ifdef __TEST
    numOfEntity ++;
    if(nAddress == clAspLocalId)
#else
    if(nAddress == clIocLocalAddressGet())
#endif // __TEST
    {
      logDebug("GMS","SERVER","Notify master node about my membership");
      /* Send message to Master so that he can know my membership */
      GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(ClIocNodeAddressT));
      sndMessage->messageType = NODE_JOIN_FROM_CACHE;
      memcpy(sndMessage->data,&nAddress,sizeof(ClIocNodeAddressT));
      sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(ClIocNodeAddressT),SEND_TO_MASTER);
      clHeapFree(sndMessage);
    }
  }
}

void roleChangeFromMaster(GroupMessageProtocolT *msg)
{
  if(msg == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return;
  }
  if(msg->messageType != CLUSTER_NODE_ROLE_NOTIFY)
  {
    logError("GMS","SERVER","Invalid message");
    return;
  }
  switch(msg->roleType)
  {
    case ROLE_ACTIVE:
    {
      EntityIdentifier activeEntity = *(EntityIdentifier *)msg->data;
      clusterNodeGrp.setActive(activeEntity);
      logDebug("GMS","SERVER","Active role had been set");
      break;
    }
    case ROLE_STANDBY:
    {
      EntityIdentifier standbyEntity = *(EntityIdentifier *)msg->data;
      clusterNodeGrp.setStandby(standbyEntity);
      logDebug("GMS","SERVER","Standby role had been set");
      break;
    }
    default:
    {
      logError("GMS","SERVER","Unknown role type");
      break;
    }
  }
}
void componentJoin(ClIocAddressT *pAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress->iocPhyAddress.nodeAddress,&grpIdentity);
  if(clusterCompGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component is already a group member");
    return;
  }
  clusterCompGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
}

void componentLeave(ClIocAddressT *pAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress->iocPhyAddress.nodeAddress,&grpIdentity);
  if(!clusterCompGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component isn't a group member");
    return;
  }
  clusterCompGrp.deregister(grpIdentity.id);
}

void getNodeInfo(ClIocNodeAddressT nAddress, GroupIdentity *grpIdentity)
{
  if(grpIdentity == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return;
  }
  ClNodeCacheMemberT member = {0};
  if(clNodeCacheMemberGetExtendedSafe(nAddress, &member, 10, 200) == CL_OK)
  {
    grpIdentity->id = createHandleFromAddress(nAddress);
    grpIdentity->capabilities = member.capability;
    grpIdentity->credentials = member.address + CL_IOC_MAX_NODES + 1;
    grpIdentity->data = (Buffer *)NULL;
    grpIdentity->dataLen = 0;
  }
  else
  {
    logError("GMS","SERVER","Can't get node info from cache");
  }
}

EntityIdentifier createHandleFromAddress(ClIocNodeAddressT nAddress, int pid)
{
  EntityIdentifier *me = new SAFplus::Handle(PersistentHandle,0,pid,nAddress,0);
  return *me;
}

bool isMasterNode()
{
  if(clCpmIsMaster() != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}
#ifndef __TEST
void sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode )
{
    switch(messageMode)
    {
      case SEND_BROADCAST:
      {
        /* Destination is broadcast address */
        ClIocAddressT iocDest;
        iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
        iocDest.iocPhyAddress.portId      = GMS_PORT;
        logInfo("GMS","SERVER","Sending broadcast message");
        /* TODO: Use async communication here */
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case SEND_TO_MASTER:
      {
        /* Destination is Master node address */
        ClIocAddressT iocDest;
        ClIocNodeAddressT masterAddress = 0;
        clCpmMasterAddressGet(&masterAddress);
        iocDest.iocPhyAddress.nodeAddress = masterAddress;
        iocDest.iocPhyAddress.portId      = GMS_PORT;
        logInfo("GMS","SERVER","Sending message to Master");
        /* TODO: Use async communication here */
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case LOCAL_ROUND_ROBIN:
      {
        logInfo("GMS","SERVER","Sending message round robin");
        break;
      }
      default:
      {
        logError("GMS","SERVER","Unknown message sending mode");
        break;
      }
    }
}
#else
void sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode )
{
    switch(messageMode)
    {
      case SEND_BROADCAST:
      {
        logInfo("GMS","SERVER","Sending broadcast message");
        ClIocAddressT iocDest;
        iocDest.iocPhyAddress.nodeAddress = 1;
        iocDest.iocPhyAddress.portId      = GMS_PORT_BASE + iocDest.iocPhyAddress.nodeAddress;
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        iocDest.iocPhyAddress.nodeAddress = 2;
        iocDest.iocPhyAddress.portId      = GMS_PORT_BASE + iocDest.iocPhyAddress.nodeAddress;
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        iocDest.iocPhyAddress.nodeAddress = 3;
        iocDest.iocPhyAddress.portId      = GMS_PORT_BASE + iocDest.iocPhyAddress.nodeAddress;
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case SEND_TO_MASTER:
      {
        ClIocAddressT iocDest;
        iocDest.iocPhyAddress.nodeAddress = 1;
        iocDest.iocPhyAddress.portId      = GMS_PORT_BASE + iocDest.iocPhyAddress.nodeAddress;
        logInfo("GMS","SERVER","Sending message to Master");
        groupMsgServer->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case LOCAL_ROUND_ROBIN:
      {
        logInfo("GMS","SERVER","Sending message round robin");
        break;
      }
      default:
      {
        logError("GMS","SERVER","Unknown message sending mode");
        break;
      }
    }
}
#endif
void elect()
{
  if(isMasterNode())
  {
    std::pair<EntityIdentifier,EntityIdentifier> electResult;
    clusterNodeGrp.elect(electResult,SAFplus::Group::ELECTION_TYPE_BOTH);
    if(!(electResult.first == INVALID_HDL))
    {
      clusterNodeGrp.setActive(electResult.first);
      logDebug("GMS","TEST","Active role had been elected node: [%d],capabilities: [%d]  ",electResult.first.getNode(),clusterNodeGrp.getCapabilities(electResult.first));
      /* Send Role change broadcast */
      GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      sndMessage->messageType = CLUSTER_NODE_ROLE_NOTIFY;
      sndMessage->roleType = ROLE_ACTIVE;
      memcpy(sndMessage->data,&electResult.first,sizeof(EntityIdentifier));
      sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      clHeapFree(sndMessage);
    }
    if(!(electResult.second == INVALID_HDL))
    {
      clusterNodeGrp.setStandby(electResult.second);
      logDebug("GMS","TEST","Standby role had been elected node: [%d],capabilities: [%d]  ",electResult.second.getNode(),clusterNodeGrp.getCapabilities(electResult.second));
      /* Send Role change broadcast */
      GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)clHeapCalloc(1,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      sndMessage->messageType = CLUSTER_NODE_ROLE_NOTIFY;
      sndMessage->roleType = ROLE_STANDBY;
      memcpy(sndMessage->data,&electResult.second,sizeof(EntityIdentifier));
      sendNotification((void *)sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      clHeapFree(sndMessage);
    }
  }
}
