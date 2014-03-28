#include "clSafplusMsgServer.hxx"
#include "groupServer.hxx"
#include <clGlobals.hxx>


using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

#define IOC_PORT 0
#define GMS_PORT_1 69
#define GMS_PORT_2 70

/* SAFplus specified variables */

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
*     - Master node will send broadcast of newly node to all node
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

void NodeJoinFromMaster(GroupMessageProtocolT *msg)
{
  GroupIdentity *grpIdentity = (GroupIdentity *)msg->data;
  if(clusterNodeGrp.isMember(grpIdentity->id))
  {
    logDebug("GMS","SERVER","Node is already group member");
    return;
  }
  clusterNodeGrp.registerEntity(grpIdentity->id,grpIdentity->credentials, grpIdentity->data, grpIdentity->dataLen, grpIdentity->capabilities);
}

void NodeLeave(ClIocAddressT *pAddress)
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

      /* TODO: Send role change notification */

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

        /* TODO: Send role change notification */
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
        /* TODO: Send role change notification */
      }
    }
  }
  else
  {
    logDebug("GMS","SERVER","Node isn't a group member");
  }
}

void NodeJoin(ClIocAddressT *pAddress)
{
  if(isMasterNode())
  {
    GroupIdentity grpIdentity;
    getNodeInfo(pAddress,&grpIdentity);
    clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
    /* TODO: Send node join broadcast message */
  }
}

void RoleChangeFromMaster(GroupMessageProtocolT *msg)
{
  /* TODO: Implement me */
}
void ComponentJoin(ClIocAddressT *pAddress)
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

void ComponentLeave(ClIocAddressT *pAddress)
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
  /* TODO: Implement me */
}

bool  isMasterNode()
{
  /* TODO: Implement me */
  return true;
}

void  sendNotification(GroupMessageTypeT messageType,void* data, GroupMessageSendModeT messageMode)
{
    /* TODO: Implement me */
}
