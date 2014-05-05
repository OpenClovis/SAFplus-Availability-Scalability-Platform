#include "GroupServer.hxx"

using namespace SAFplus;
using namespace SAFplusI;

#ifdef __TEST
extern ClUint32T clAspLocalId;
#define logEmergency(area, context,M, ...)  fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logAlert(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logCritical(area, context,M,...)    fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logError(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logWarning(area, context,M,...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logNotice(area, context,M, ...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logInfo(area, context, M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logDebug(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logTrace(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

GroupServer* GroupServer::instance = NULL;
SafplusMsgServer* GroupServer::groupMsgServer = NULL;

GroupServer* GroupServer::getInstance()
{
  if(instance == NULL)
  {
    instance = new GroupServer();
  }
  return instance;
}
/* This will start the Group Server and listening on GMS_PORT */

struct GmsServiceThread
  {
  void operator()()
    {
    GroupServer::initializeLibraries();
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
    GroupMessageHandler *groupMessageHandler = new GroupMessageHandler();
    GroupServer::groupMsgServer = new SAFplus::SafplusMsgServer(CL_IOC_GMS_PORT);
    GroupServer::groupMsgServer->RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, NULL);
    GroupServer::groupMsgServer->Start();
    /* Initialize notification callback */
    clIocNotificationRegister(iocNotificationCallback,NULL);
    while(!GroupServer::getInstance()->finished)
      {
      ;
      }
    GroupServer::groupMsgServer->Stop();
    GroupServer::groupMsgServer->RemoveHandler(CL_IOC_PROTO_MSG);
    clIocNotificationDeregister(iocNotificationCallback);
    logInfo("GMS","SERVER","Task stopped");
    GroupServer::finalizeLibraries();
    }
  };
  
/* The API for user to create and start Group Server */
void GroupServer::clGrpStartServer()
{
  finished = CL_FALSE;
#if 0
  ClRcT rc = clOsalTaskCreateDetached("gms_service",CL_OSAL_SCHED_OTHER,CL_OSAL_THREAD_PRI_NOT_APPLICABLE,4096,gmsServiceThread,NULL);
  if(CL_OK != rc)
  {
    logError("GMS","SERVER","Error in creating Group Server task");
    CL_ASSERT(0);
  }
#endif
  serviceThread = boost::thread(GmsServiceThread());
  
  logInfo("GMS","SERVER","Task started");
}
/* The API for user to stop group server */
void GroupServer::clGrpStopServer()
{
  finished = CL_TRUE;
}

GroupServer::GroupServer()
{
  clusterNodeGrp = new Group("CLUSTER_NODE");
  clusterCompGrp = new Group("CLUSTER_COMP");
  electTimerHandle = NULL;
  roleNotiTimerHandle = NULL;
  finished = CL_FALSE;
  isElectTimerRunning = CL_FALSE;
}

GroupServer::~GroupServer()
{
  delete instance;
}

void GroupServer::nodeJoinFromMaster(GroupMessageProtocol *msg)
{
  if(msg == NULL || msg->data == NULL)
  {
    logDebug("GMS","SERVER","Received message is NULL");
    return;
  }
  GroupIdentity grpIdentity = *(GroupIdentity *)msg->data;
  clusterNodeGrp->registerEntity(grpIdentity);
}

void GroupServer::roleChangeFromMaster(GroupMessageProtocol *msg)
{
  if(msg == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return;
  }
  if(msg->messageType != MSG_ROLE_NOTIFY)
  {
    logError("GMS","SERVER","Invalid message");
    return;
  }
  clTimerDeleteAsync(&roleNotiTimerHandle);
  switch(msg->roleType)
  {
    case ROLE_ACTIVE:
    {
      EntityIdentifier activeEntity = *(EntityIdentifier *)msg->data;
      /* Mismatching between received and local active result */
      if(!(clusterNodeGrp->getActive() == activeEntity))
      {
        logDebug("GMS","SERVER","Election result is mismatched");
        elect();
        return;
      }
      clusterNodeGrp->setActive(activeEntity);
      logDebug("GMS","SERVER","Active role had been set");
      break;
    }
    case ROLE_STANDBY:
    {
      EntityIdentifier standbyEntity = *(EntityIdentifier *)msg->data;
      /* Mismatching between received and local active result */
      if(!(clusterNodeGrp->getStandby() == standbyEntity))
      {
        logDebug("GMS","SERVER","Election result is mismatched");
        elect();
        return;
      }
      clusterNodeGrp->setStandby(standbyEntity);
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

void GroupServer::nodeJoin(ClIocNodeAddressT nAddress)
{
  GroupIdentity grpIdentity;
  ClRcT rc = CL_OK;
  rc = getNodeInfo(nAddress,&grpIdentity);
  if(CL_OK != rc)
  {
    logInfo("GMS","SERVER","Cache did not update yet");
    return;
  }

  if(isMasterNode())
  {
    clusterNodeGrp->registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities);
    /* Send node join broadcast message */
    fillSendMessage(&grpIdentity,MSG_NODE_JOIN);
  }
  else
  {
    clusterNodeGrp->registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities);
  }
}

void GroupServer::nodeLeave(ClIocNodeAddressT nAddress)
{
  GroupIdentity grpIdentity;
  grpIdentity.id = createHandleFromAddress(nAddress,0);
  if(clusterNodeGrp->isMember(grpIdentity.id))
  {
    EntityIdentifier curActive = clusterNodeGrp->getActive();
    EntityIdentifier curStandby = clusterNodeGrp->getStandby();
    clusterNodeGrp->deregister(grpIdentity.id);
    logDebug("GMS","SERVER","Node had been removed from cluster node group");
    if(curActive == grpIdentity.id)
    {
      logDebug("GMS","SERVER","Leaving node had active role. Switching over");
      /*Update Node with standby role to active role*/
      EntityIdentifier standbyNode = clusterNodeGrp->getStandby();
      clusterNodeGrp->setActive(standbyNode);
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
        fillSendMessage(&standbyNode,MSG_ROLE_NOTIFY,SEND_BROADCAST,ROLE_ACTIVE);
        logDebug("GMS","SERVER","Re-elect standby role from Master node");
        elect();
      }
    }
    else if(curStandby == grpIdentity.id)
    {
      logDebug("GMS","SERVER","Leaving node had standby role.");
      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        logDebug("GMS","SERVER","Re-elect standby role from Master node");
        elect();
      }
    }
    /* Remove all entity which belong to this node */
    SAFplus::Group::Iterator iter = clusterCompGrp->begin();
    while(iter != clusterCompGrp->end())
    {
      EntityIdentifier item = iter->first;
      if(item.getNode() == nAddress)
      {
        clusterCompGrp->deregister(item);
      }
    }
  }
  else
  {
    logDebug("GMS","SERVER","Node isn't a group member");
  }
}

void GroupServer::componentJoin(ClIocAddressT address)
{
  int componentProcessId = address.iocPhyAddress.portId;
  GroupIdentity grpIdentity;
  getNodeInfo(address.iocPhyAddress.nodeAddress,&grpIdentity,componentProcessId);
  clusterCompGrp->registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities);
}

void GroupServer::componentLeave(ClIocAddressT address)
{
  int componentProcessId = address.iocPhyAddress.portId;
  GroupIdentity grpIdentity;
  getNodeInfo(address.iocPhyAddress.nodeAddress,&grpIdentity,componentProcessId);
  if(!clusterCompGrp->isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component isn't a group member");
    return;
  }
  clusterCompGrp->deregister(grpIdentity.id);
}

ClRcT roleNotificationTimer(void *arg)
{
  /* TODO: send fault report */

  /* Issue another election */
  clTimerDeleteAsync(&(GroupServer::getInstance()->roleNotiTimerHandle));
  GroupServer::getInstance()->elect();
  return CL_OK;
}

ClRcT electRequestTimer(void *arg)
{
  clTimerDeleteAsync(&(GroupServer::getInstance()->electTimerHandle));
  std::pair<EntityIdentifier,EntityIdentifier> res = GroupServer::getInstance()->clusterNodeGrp->elect();
  if(res.first == INVALID_HDL && res.second == INVALID_HDL)
  {
    logError("GMS","SERVER","Can not elect");
    return CL_ERR_INVALID_STATE;
  }
  /* If active member, send notification and wait for confliction checking */
  if(GroupServer::getInstance()->clusterNodeGrp->getActive().getNode() == clIocLocalAddressGet())
  {
    GroupServer::getInstance()->fillSendMessage(&(res.first),MSG_ROLE_NOTIFY,SEND_BROADCAST,ROLE_ACTIVE);
    GroupServer::getInstance()->fillSendMessage(&(res.second),MSG_ROLE_NOTIFY,SEND_BROADCAST,ROLE_STANDBY);
  }
  else
  {
    /* If not active member, wait for notification */
    ClTimerTimeOutT timeOut = { 10, 0 };
    clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, roleNotificationTimer, NULL, &(GroupServer::getInstance()->roleNotiTimerHandle));
  }
  GroupServer::getInstance()->isElectTimerRunning = CL_FALSE;
  return CL_OK;
}

ClRcT iocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie)
{
  ClRcT rc = CL_OK;
  ClIocAddressT address;
  ClIocNotificationIdT eventId = (ClIocNotificationIdT) ntohl(notification->id);
  ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
  ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
  logDebug("GMS","SERVER","Received IOC notification [0x%x] callback from node [%d]",eventId,nodeAddress);
  switch(eventId)
  {
    case CL_IOC_NODE_LEAVE_NOTIFICATION:
    case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
    {
      logDebug("GMS","SERVER","Received node LEAVE notification");
      GroupServer::getInstance()->nodeLeave(nodeAddress);
      break;
    }
    case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
    {
      logDebug("GMS","SERVER","Received node JOIN notification");
      GroupServer::getInstance()->nodeJoin(nodeAddress);
      break;
    }
    case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
    {
      logDebug("GMS","SERVER","Received component ARRIVAL notification");
      address.iocPhyAddress.nodeAddress = nodeAddress;
      address.iocPhyAddress.portId = portId;
      GroupServer::getInstance()->componentJoin(address);
      break;
    }
    case CL_IOC_COMP_DEATH_NOTIFICATION:
    {
      logDebug("GMS","SERVER","Received component DEATH notification");
      address.iocPhyAddress.nodeAddress = nodeAddress;
      address.iocPhyAddress.portId = portId;
      GroupServer::getInstance()->componentLeave(address);
      break;
    }
    default:
    {
      logDebug("GMS","SERVER","Received unsupported notification");
      break;
    }
  }
  return rc;
}

ClRcT GroupServer::initializeLibraries()
{
  ClRcT   rc            = CL_OK;
  ClIocNodeAddressT     currentLeader;
  ClIocPhysicalAddressT compAddr = {0};
  /* Initialize log service */
  SAFplus::logCompName = "GroupServer";
  logSeverity = LOG_SEV_MAX;
  logInitialize();

  /* Initialize necessary libraries */
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK)
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
}

ClRcT GroupServer::finalizeLibraries()
{
  clIocLibFinalize();
  clTimerFinalize();
  clBufferFinalize();
  clHeapExit();
}

ClRcT GroupServer::getNodeInfo(ClIocNodeAddressT nAddress, SAFplus::GroupIdentity *grpIdentity, int pid)
{
  if(grpIdentity == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return CL_ERR_NULL_POINTER;
  }
  ClNodeCacheMemberT member = {0};
  if(clNodeCacheMemberGetExtendedSafe(nAddress, &member, 10, 200) == CL_OK)
  {
    grpIdentity->id = createHandleFromAddress(nAddress,pid);
    grpIdentity->capabilities = member.capability;
    grpIdentity->credentials = member.address + CL_IOC_MAX_NODES + 1;
    grpIdentity->dataLen = 0;
    return CL_OK;
  }
  else
  {
    logError("GMS","SERVER","Can't get node info from cache");
    return CL_ERR_NOT_INITIALIZED;
  }
}

bool  GroupServer::isMasterNode()
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

bool  GroupServer::isActiveNode()
{
  if(clusterNodeGrp->getActive().getNode() == clIocLocalAddressGet())
  {
    return true;
  }
  else
  {
    return false;
  }
}

void GroupServer::fillSendMessage(void* data, GroupMessageTypeT msgType,GroupMessageSendModeT msgSendMode, GroupRoleNotifyTypeT roleType)
{
  int msgLen = 0;
  int msgDataLen = 0;
  switch(msgType)
  {
    case MSG_NODE_JOIN:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(GroupIdentity);
      msgDataLen = sizeof(GroupIdentity);
      break;
    case MSG_ROLE_NOTIFY:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(EntityIdentifier);
      msgDataLen = sizeof(EntityIdentifier);
      break;
    case MSG_ELECT_REQUEST:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(GroupIdentity);
      msgDataLen = sizeof(GroupIdentity);
      break;
    default:
      return;
  }
  char msgPayload[sizeof(Buffer)-1+msgLen];
  Buffer* buff = new(msgPayload) Buffer(msgLen);
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)buff;
  sndMessage->messageType = msgType;
  sndMessage->roleType = roleType;
  memcpy(sndMessage->data,data,msgDataLen);
  sendNotification((void *)sndMessage,msgLen,msgSendMode);
}

void  GroupServer::sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode)
{
  switch(messageMode)
  {
    case SEND_BROADCAST:
    {
      /* Destination is broadcast address */
      ClIocAddressT iocDest;
      iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
      iocDest.iocPhyAddress.portId      = CL_IOC_GMS_PORT;
      logInfo("GMS","SERVER","Sending broadcast message");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
      }
      catch (...)
      {
        logDebug("GMS","SERVER","Failed to send");
      }
      break;
    }
    case SEND_TO_MASTER:
    {
      /* Destination is Master node address */
      ClIocAddressT iocDest;
      ClIocNodeAddressT masterAddress = 0;
      clCpmMasterAddressGet(&masterAddress);
      iocDest.iocPhyAddress.nodeAddress = masterAddress;
      iocDest.iocPhyAddress.portId      = CL_IOC_GMS_PORT;
      logInfo("GMS","SERVER","Sending message to Master");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
      }
      catch (...)
      {
        logDebug("GMS","SERVER","Failed to send");
      }
      break;
    }
    case SEND_LOCAL_RR:
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

EntityIdentifier GroupServer::createHandleFromAddress(ClIocNodeAddressT nAddress, int pid)
{
  EntityIdentifier *me = new SAFplus::Handle(PersistentHandle,0,pid,nAddress,0);
  return *me;
}

void GroupServer::elect(ClBoolT isRequest,GroupMessageProtocol *msg)
{
  if(isRequest == CL_FALSE)
  {
    GroupIdentity grpIdentity = *(GroupIdentity *)msg->data;
    logInfo("GMS","ELECT","Election infor: %d  %d",grpIdentity.id.getNode(),grpIdentity.capabilities);
    clusterNodeGrp->registerEntity(grpIdentity);
  }
  if(isElectTimerRunning == CL_FALSE)
  {
    GroupIdentity grpIdentity;
    ClRcT rc = getNodeInfo(clIocLocalAddressGet(),&grpIdentity);
    clusterNodeGrp->registerEntity(grpIdentity);
    if(rc == CL_OK)
    {
      logInfo("GMS","ELECT","Sending elect request to all members");
      fillSendMessage(&grpIdentity,MSG_ELECT_REQUEST);
    }
    isElectTimerRunning = CL_TRUE;
    ClTimerTimeOutT timeOut = { 10, 0 };
    logInfo("GMS","ELECT","Start wait for credential from others");
    clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, electRequestTimer, NULL, &electTimerHandle);
  }
}

void GroupServer::dumpClusterNodeGroup()
{
  logInfo("GMS","DUMP","Start dumping cluster");
  SAFplus::Group::Iterator iter = clusterNodeGrp->begin();
  while(iter != clusterNodeGrp->end())
  {
    Buffer& curval = iter->second;
    GroupIdentity *item = (GroupIdentity *)(&curval);
    logInfo("GMS","DUMP","Node [%d] has capabilities [%d] and credentials [0x%lx]",item->id.getNode(),item->capabilities,item->credentials);
    iter++;
  }
}
