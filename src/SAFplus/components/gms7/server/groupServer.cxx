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

#endif // __TEST
/* Well-known groups */
SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");
SAFplus::Group    clusterCompGrp("CLUSTER_COMP");
GroupMessageHandler         *groupMessageHandler;
SAFplus::SafplusMsgServer   *groupMsgServer;
int               populationTimeOut = 5;
ClTimerHandleT    timerHandle = NULL;
ClHandleT gNotificationCallbackHandle = CL_HANDLE_INVALID_VALUE;

/* Server process */
int main(int argc, char* argv[])
{
  ClRcT rc;
  ClTimerTimeOutT timeOut = { populationTimeOut, 0 };

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
  /* Wait some second for the first election */
  clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, timerCallback, NULL, &timerHandle);
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
  ClIocPhysicalAddressT compAddr = {0};
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
  groupMsgServer = new SAFplus::SafplusMsgServer(GMS_PORT);
  messageScope  = 1;
  groupMsgServer->RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, (int *)&messageScope);

  /* Start the message handlers */
  groupMsgServer->Start();

  /* Initialize notification callback */
  compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  compAddr.portId = CL_IOC_CPM_PORT;
  clCpmNotificationCallbackInstall(compAddr, gmsNotificationCallback, NULL, &gNotificationCallbackHandle);
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
      /* I am becoming master node */
      if(standbyNode.getNode() == clIocLocalAddressGet())
      {
        clNodeCacheLeaderUpdate(clIocLocalAddressGet());
        logDebug("GMS","SERVER","Node [%d] is new master node",clIocLocalAddressGet());
      }
      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        /* Send role change notification */
        fillSendMessage(&standbyNode,CLUSTER_NODE_ROLE_NOTIFY,SEND_BROADCAST,ROLE_ACTIVE);
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
        fillSendMessage(&standbyNode,CLUSTER_NODE_ROLE_NOTIFY,SEND_BROADCAST,ROLE_STANDBY);
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
        fillSendMessage(&standbyNode,CLUSTER_NODE_ROLE_NOTIFY,SEND_BROADCAST,ROLE_STANDBY);
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
void fillSendMessage(void* data, GroupMessageTypeT msgType,GroupMessageSendModeT msgSendMode, GroupRoleNotifyTypeT roleType)
{
  int msgLen = 0;
  int msgDataLen = 0;
  switch(msgType)
  {
    case NODE_JOIN_FROM_SC:
      msgLen = sizeof(GroupMessageProtocolT) + sizeof(GroupIdentity);
      msgDataLen = sizeof(GroupIdentity);
      break;
    case NODE_JOIN_FROM_CACHE:
      msgLen = sizeof(GroupMessageProtocolT) + sizeof(ClIocNodeAddressT);
      msgDataLen = sizeof(ClIocNodeAddressT);
      break;
    case CLUSTER_NODE_ROLE_NOTIFY:
      msgLen = sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier);
      msgDataLen = sizeof(EntityIdentifier);
      break;
    default:
      return;
  }
  char msgPayload[sizeof(Buffer)-1+msgLen];
  Buffer* buff = new(msgPayload) Buffer(msgLen);
  GroupMessageProtocolT *sndMessage = (GroupMessageProtocolT *)buff;
  sndMessage->messageType = msgType;
  sndMessage->roleType = roleType;
  memcpy(sndMessage->data,data,msgDataLen);
  sendNotification((void *)sndMessage,msgLen,msgSendMode);
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
    /* Send node join broadcast message */
    fillSendMessage(&grpIdentity,NODE_JOIN_FROM_SC);
  }
  else
  {
    clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
    if(nAddress == clIocLocalAddressGet())
    {
      logDebug("GMS","SERVER","Notify master node about my membership");
      /* Send message to Master so that he can know my membership */
      fillSendMessage(&nAddress,NODE_JOIN_FROM_CACHE,SEND_TO_MASTER);
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
  /* TODO: How to get component process id */
  int componentProcessId = 0;
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress->iocPhyAddress.nodeAddress,&grpIdentity,componentProcessId);
  if(clusterCompGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component is already a group member");
    return;
  }
  clusterCompGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
}

void componentLeave(ClIocAddressT *pAddress)
{
  /* TODO: How to get component process id */
  int componentProcessId = 0;
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress->iocPhyAddress.nodeAddress,&grpIdentity,componentProcessId);
  if(!clusterCompGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component isn't a group member");
    return;
  }
  clusterCompGrp.deregister(grpIdentity.id);
}

void getNodeInfo(ClIocNodeAddressT nAddress, GroupIdentity *grpIdentity, int pid)
{
  if(grpIdentity == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return;
  }
  ClNodeCacheMemberT member = {0};
  if(clNodeCacheMemberGetExtendedSafe(nAddress, &member, 10, 200) == CL_OK)
  {
    grpIdentity->id = createHandleFromAddress(nAddress,pid);
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
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
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
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
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
      fillSendMessage(&electResult.first,CLUSTER_NODE_ROLE_NOTIFY,SEND_BROADCAST,ROLE_ACTIVE);
    }
    if(!(electResult.second == INVALID_HDL))
    {
      clusterNodeGrp.setStandby(electResult.second);
      logDebug("GMS","TEST","Standby role had been elected node: [%d],capabilities: [%d]  ",electResult.second.getNode(),clusterNodeGrp.getCapabilities(electResult.second));
      /* Send Role change broadcast */
      fillSendMessage(&electResult.second,CLUSTER_NODE_ROLE_NOTIFY,SEND_BROADCAST,ROLE_STANDBY);
    }
  }
}

ClRcT timerCallback( void *arg )
{
  ClRcT rc = CL_OK;
  logDebug("GMS","TIMER","Timer callback!");
  clTimerDeleteAsync(&timerHandle);
  rc = initializeClusterNodeGroup();
  if(CL_OK != rc)
  {
    logError("GMS","TIMER","Initialize cluster node group error [%#x]",rc);
    return rc;
  }
  elect();

  return rc;
}

void gmsNotificationCallback(ClIocNotificationIdT eventId, ClPtrT unused, ClIocAddressT *pAddress)
{
  ClRcT rc = CL_OK;
  if(eventId == CL_IOC_NODE_LEAVE_NOTIFICATION || eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
  {
    logDebug("GMS","SERVER","Received node LEAVE notification");
    nodeLeave(pAddress->iocPhyAddress.nodeAddress);
  }
  else
  {
    logDebug("GMS","SERVER","Received node JOIN notification");
    nodeJoin(pAddress->iocPhyAddress.nodeAddress);
  }
}

