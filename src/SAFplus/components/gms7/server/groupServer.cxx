#include "clSafplusMsgServer.hxx"
#include "groupServer.hxx"
#include <clGlobals.hxx>


using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

/* SAFplus specified variables */
ClUint32T clAspLocalId = 1;
ClBoolT gIsNodeRepresentative = CL_TRUE;
/* Well-known groups */
SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");
SAFplus::Group    clusterCompGrp("CLUSTER_COMP");

/*
GROUP SERVICE DESIGNATION
*************************************************************************
* - Running on some SC Nodes (a Master - active role - and several Slaves)
* - Tracking the Nodes/Components membership and role
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
    logError("GMS","SERVER","Initialize server error");
    return rc;
  }
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

  /* Initialize necessary libraries */
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {
    return rc;
  }
  clIocLibInitialize(NULL);

  /* Initialize message handler */
  GroupMessageHandler         *groupMessageHandler = new GroupMessageHandler();
  SAFplus::SafplusMsgServer   groupMsgServer(GMS_PORT);
  SAFplus::SafplusMsgServer   clusterMsgServer(AMF_PORT);
  groupMsgServer.RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, (int *)&messageScope);
  /* TODO: Register to message from AMF */
  messageScope = AMF_MESSAGE;
  clusterMsgServer.RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, (int *)&messageScope);

  /* Start the message handlers */
  clusterMsgServer.Start();
  groupMsgServer.Start();
}

void nodeJoinFromMaster(GroupMessageProtocolT *msg)
{
  GroupIdentity *grpIdentity = (GroupIdentity *)msg->data;
  if(clusterNodeGrp.isMember(grpIdentity->id))
  {
    logDebug("GMS","SERVER","Node is already group member");
    return;
  }
  clusterNodeGrp.registerEntity(grpIdentity->id,grpIdentity->credentials, grpIdentity->data, grpIdentity->dataLen, grpIdentity->capabilities);
}

void nodeLeave(ClIocAddressT *pAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress,&grpIdentity);
  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    clusterNodeGrp.deregister(grpIdentity.id);
    if(clusterNodeGrp.getActive() == grpIdentity.id)
    {
      /*Update Node with standby role to active role*/
      EntityIdentifier standbyNode = clusterNodeGrp.getStandby();
      int currentCapability = clusterNodeGrp.getCapabilities(standbyNode);
      currentCapability |= SAFplus::Group::IS_ACTIVE;
      currentCapability &= ~SAFplus::Group::IS_STANDBY;
      clusterNodeGrp.setCapabilities(currentCapability,standbyNode);

      /* Send role change notification */
      GroupMessageProtocolT sndMessage;
      sndMessage.messageType = CLUSTER_NODE_ROLE_NOTIFY;
      sndMessage.roleType    = ROLE_ACTIVE;
      sndMessage.data        = &standbyNode;
      sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));

      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        std::pair<EntityIdentifier,EntityIdentifier> electResult;
        clusterNodeGrp.elect(electResult,SAFplus::Group::ELECTION_TYPE_STANDBY);
        if(electResult.second == INVALID_HDL)
        {
          logError("GMS","SERVER","Can't elect for standby role");
          return;
        }
        standbyNode = electResult.second;
        currentCapability = clusterNodeGrp.getCapabilities(standbyNode);
        currentCapability |= SAFplus::Group::IS_STANDBY;
        currentCapability &= ~SAFplus::Group::IS_ACTIVE;
        clusterNodeGrp.setCapabilities(currentCapability,standbyNode);

        /* Send role change notification */
        GroupMessageProtocolT sndMessage;
        sndMessage.messageType = CLUSTER_NODE_ROLE_NOTIFY;
        sndMessage.roleType    = ROLE_STANDBY;
        sndMessage.data        = &standbyNode;
        sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      }
    }
    else if(clusterNodeGrp.getStandby() == grpIdentity.id)
    {
      /* Elect for standby role on Master node only */
      if(isMasterNode())
      {
        std::pair<EntityIdentifier,EntityIdentifier> electResult;
        clusterNodeGrp.elect(electResult,SAFplus::Group::ELECTION_TYPE_STANDBY);
        if(electResult.second == INVALID_HDL)
        {
          logError("GMS","SERVER","Can't elect for standby role");
          return;
        }
        EntityIdentifier standbyNode = electResult.second;
        int currentCapability = clusterNodeGrp.getCapabilities(standbyNode);
        currentCapability |= SAFplus::Group::IS_STANDBY;
        currentCapability &= ~SAFplus::Group::IS_ACTIVE;
        clusterNodeGrp.setCapabilities(currentCapability,standbyNode);

        /* Send role change notification */
        GroupMessageProtocolT sndMessage;
        sndMessage.messageType = CLUSTER_NODE_ROLE_NOTIFY;
        sndMessage.roleType    = ROLE_STANDBY;
        sndMessage.data        = &standbyNode;
        sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocolT) + sizeof(EntityIdentifier));
      }
    }
    /* Remove all entity which belong to this node */
    SAFplus::Group::Iterator iter = clusterCompGrp.begin();
    while(iter != clusterCompGrp.end())
    {
      SAFplusI::BufferPtr curkey = iter->first;
      EntityIdentifier item = *(EntityIdentifier *)(curkey.get()->data);
      if(item.getNode() == pAddress->iocPhyAddress.nodeAddress)
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

void nodeJoin(ClIocAddressT *pAddress)
{
  if(isMasterNode())
  {
    GroupIdentity grpIdentity;
    getNodeInfo(pAddress,&grpIdentity);
    clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);

    /* Send node join broadcast message */
    GroupMessageProtocolT sndMessage;
    sndMessage.messageType = CLUSTER_NODE_ARRIVAL;
    sndMessage.data        = &grpIdentity;
    sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocolT) + sizeof(GroupIdentity));
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
      int currentCapability = clusterNodeGrp.getCapabilities(activeEntity);
      currentCapability |= SAFplus::Group::IS_ACTIVE;
      currentCapability &= ~SAFplus::Group::IS_STANDBY;
      clusterNodeGrp.setCapabilities(currentCapability,activeEntity);
      break;
    }
    case ROLE_STANDBY:
    {
      EntityIdentifier standbyEntity = *(EntityIdentifier *)msg->data;
      int currentCapability = clusterNodeGrp.getCapabilities(standbyEntity);
      currentCapability |= SAFplus::Group::IS_STANDBY;
      currentCapability &= ~SAFplus::Group::IS_ACTIVE;
      clusterNodeGrp.setCapabilities(currentCapability,standbyEntity);
      break;
    }
    default:
    {
      break;
    }
  }
}
void componentJoin(ClIocAddressT *pAddress)
{
  GroupIdentity grpIdentity;
  getNodeInfo(pAddress,&grpIdentity);
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
  getNodeInfo(pAddress,&grpIdentity);
  if(!clusterCompGrp.isMember(grpIdentity.id))
  {
    logDebug("GMS","SERVER", "Component isn't a group member");
    return;
  }
  clusterCompGrp.deregister(grpIdentity.id);
}

void  getNodeInfo(ClIocAddressT* pAddress, GroupIdentity *grpIdentity)
{
  if(grpIdentity == NULL || pAddress == NULL)
  {
    logError("GMS","SERVER","Null pointer");
    return;
  }
  ClNodeCacheMemberT member = {0};
  if(clNodeCacheMemberGetExtendedSafe(pAddress->iocPhyAddress.nodeAddress, &member, 10, 200) != CL_OK)
  {
    grpIdentity->id = createHandleFromAddress(pAddress);
    grpIdentity->capabilities = member.capability;
    grpIdentity->credentials = member.address + CL_IOC_MAX_NODES + 1;
    grpIdentity->data = (Buffer *)NULL;
    grpIdentity->dataLen = 0;
  }
}

EntityIdentifier createHandleFromAddress(ClIocAddressT* pAddress, int pid)
{
  EntityIdentifier me = SAFplus::Handle(PersistentHandle,0,pid,pAddress->iocPhyAddress.nodeAddress,0);
  return me;
}

bool  isMasterNode()
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

void  sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode )
{
    switch(messageMode)
    {
      case SEND_BROADCAST:
      {
        SafplusMsgServer msgClient(IOC_PORT);
        ClIocAddressT iocDest;
        iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
        iocDest.iocPhyAddress.portId      = GMS_PORT;
        msgClient.SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case SEND_TO_MASTER:
      {
        SafplusMsgServer msgClient(IOC_PORT);
        ClIocAddressT iocDest;
        ClIocNodeAddressT masterAddress = 0;
        clCpmMasterAddressGet(&masterAddress);
        iocDest.iocPhyAddress.nodeAddress = masterAddress;
        iocDest.iocPhyAddress.portId      = GMS_PORT;
        msgClient.SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
        break;
      }
      case LOCAL_ROUND_ROBIN:
      {
        break;
      }
      default:
      {
        logError("GMS","SERVER","Unknown message sending mode");
        break;
      }
    }
}
