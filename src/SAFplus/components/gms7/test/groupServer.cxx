#include "clSafplusMsgServer.hxx"
#include "groupServer.hxx"
#include <clGlobals.hxx>
#include <clTimerApi.h>


using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

#define IOC_PORT 0
#define GMS_PORT_1 69
#define GMS_PORT_2 70

/* SAFplus specified variables */
ClUint32T clAspLocalId = atoi(getenv("NODEADDR"));
ClBoolT gIsNodeRepresentative = CL_TRUE;
ClIocNodeAddressT myNodeAddress = clAspLocalId;
ClHandleT gNotificationCallbackHandle = CL_HANDLE_INVALID_VALUE;
/* Global variables */
SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");


class ServerMsgHandler:public SAFplus::MsgHandler
{
  public:
    void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      int msgType = *(int *)cookie;
      cout << "GMS_SERVER[" << clAspLocalId << "]: Cookie: " << msgType << "\n";
      switch(msgType)
      {
        case 1:
        {
          messageProtocol *rxMsg = (messageProtocol *)msg;
          /* If local message, ignore */
          if(from.iocPhyAddress.nodeAddress == clAspLocalId && (from.iocPhyAddress.portId == GMS_PORT_1 || from.iocPhyAddress.portId == GMS_PORT_2))
          {
            return;
          }
          switch(rxMsg->messageType)
          {
            case CLUSTER_NODE_ARRIVAL:
              entityJoinHandle(rxMsg);
              break;
            case CLUSTER_NODE_LEAVE:
              entityLeaveHandle(rxMsg);
              break;
            case CLUSTER_NODE_ELECT:
              entityElectHandle();
              break;
            default: //Unsupported
              break;
          }
          break;
        }
        default:
        {
          cout << "GMS_SERVER[" << clAspLocalId << "]: Not yet supported \n";
          break;
        }
      }
    }
};

ServerMsgHandler *handler = new ServerMsgHandler();
/* Notification to detect node status*/
void gmsNotificationCallback(ClIocNotificationIdT eventId, ClPtrT unused, ClIocAddressT *pAddress)
{
  ClRcT rc = CL_OK;
  if(eventId == CL_IOC_NODE_LEAVE_NOTIFICATION || eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: Received left node event. Do nothing.. \n";
  }
  else
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: Received newly join node [" << pAddress->iocPhyAddress.nodeAddress << "," <<  pAddress->iocPhyAddress.portId << "]\n";

    /* Broadcast data to newly join node */
    if((pAddress->iocPhyAddress.nodeAddress != myNodeAddress) && (pAddress->iocPhyAddress.portId == GMS_PORT_1 || pAddress->iocPhyAddress.portId == GMS_PORT_2))
    {
      sendInfomationToNewNode(pAddress);
    }
  }
}
void gmsNotificationInitialize()
{
  ClIocPhysicalAddressT compAddr = {0};
  compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  compAddr.portId = CL_IOC_CPM_PORT;
  clCpmNotificationCallbackInstall(compAddr, gmsNotificationCallback, NULL, &gNotificationCallbackHandle);
}
void sendInfomationToNewNode(ClIocAddressT *pAddress)
{
  messageProtocol broadcastMsg;
  broadcastMsg.messageType = CLUSTER_NODE_ARRIVAL;
  broadcastMsg.groupId = 0;
  broadcastMsg.numberOfItems = 1;

  SAFplus::Group::Iterator iter = clusterNodeGrp.begin();
  while(iter != clusterNodeGrp.end())
  {
    SAFplusI::BufferPtr curval = iter->second;
    GroupIdentity item = *(GroupIdentity *)(curval.get()->data);
    broadcastMsg.grpIdentity = item;
    sendBroadcast((void *)&broadcastMsg,sizeof(messageProtocol));
    iter++;
    cout << "GMS_SERVER[" << myNodeAddress << "]: Send data to GMS_SERVER[" <<  pAddress->iocPhyAddress.nodeAddress;
  }
}
void convertIocAddressToHandle(ClIocAddressT *pAddress, SAFplus::Handle *pHandle)
{
  *pHandle = SAFplus::Handle(PersistentHandle,0,pAddress->iocPhyAddress.portId,pAddress->iocPhyAddress.nodeAddress,0);
}
void initializeSAFplus()
{

}
void registerAndStartMessageHandler(int port)
{
  int cookieMsg = 1;
  SAFplus::SafplusMsgServer safplusMsgServer(port);
  safplusMsgServer.RegisterHandler(CL_IOC_PROTO_MSG, handler, (int *)&cookieMsg);
  safplusMsgServer.Start();
}
void sendBroadcast(void* data, int dataLength)
{
  SafplusMsgServer msgClient(IOC_PORT);
  ClIocAddressT iocDest;
  if(clAspLocalId == 1) //If current node is 1, send to node 2
  {
    iocDest.iocPhyAddress.nodeAddress = 2;
    iocDest.iocPhyAddress.portId = (int)GMS_PORT_2;
  }
  else
  {
    iocDest.iocPhyAddress.nodeAddress = 1;
    iocDest.iocPhyAddress.portId = (int)GMS_PORT_1;
  }

  MsgReply *msgReply = msgClient.SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
}
void entityJoinHandle(messageProtocol *rxMsg)
{
  GroupIdentity grpIdentity = rxMsg->grpIdentity;
  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Already member! No broadcast needed! \n";
    return;
  }
  cout << "GMS_SERVER[" << myNodeAddress << "]: " << "DATA: Credential: " << grpIdentity.credentials << " Capabilities:" << grpIdentity.capabilities << "\n";
  clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
  messageProtocol broadcastMsg;
  broadcastMsg.messageType = CLUSTER_NODE_ARRIVAL;
  broadcastMsg.groupId = 0;
  broadcastMsg.numberOfItems = 1;
  broadcastMsg.grpIdentity = grpIdentity;
  sendBroadcast((void *)&broadcastMsg,sizeof(messageProtocol));
}
void entityLeaveHandle(messageProtocol *rxMsg)
{
  bool needReelect = false;
  int reElectType = 0;
  GroupIdentity grpIdentity = rxMsg->grpIdentity;
  if(clusterNodeGrp.isMember(grpIdentity.id))
  {
    if(grpIdentity.id == clusterNodeGrp.getActive())
    {
      cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Active entity left \n";
      reElectType = (int)SAFplus::Group::ELECTION_TYPE_ACTIVE;
    }
    else if(grpIdentity.id == clusterNodeGrp.getStandby())
    {
      cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Standby entity left \n";
      reElectType = (int)SAFplus::Group::ELECTION_TYPE_STANDBY;
    }
    clusterNodeGrp.deregister(grpIdentity.id);
    messageProtocol broadcastMsg;
    broadcastMsg.messageType = CLUSTER_NODE_LEAVE;
    broadcastMsg.groupId = 0;
    broadcastMsg.numberOfItems = 1;
    broadcastMsg.grpIdentity = grpIdentity;
    sendBroadcast((void *)&broadcastMsg,sizeof(messageProtocol));

    if(reElectType != 0)
    {
      std:pair<EntityIdentifier,EntityIdentifier> res;
      uint capabilities;
      int rs = clusterNodeGrp.elect(res,reElectType);
      cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Election result: " << rs << "\n";
      capabilities = clusterNodeGrp.getCapabilities(res.first);
      cout << "GMS_SERVER[" << myNodeAddress << "]: " << "--> Active had capability: " << capabilities << "\n";
      capabilities = clusterNodeGrp.getCapabilities(res.second);
      cout << "GMS_SERVER[" << myNodeAddress << "]: " << "--> Standby had capability: " << capabilities << "\n";
    }
  }
  else
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Entity didn't exist! \n";
  }


}
void entityElectHandle()
{
  std:pair<EntityIdentifier,EntityIdentifier> res;
  uint capabilities;
  int rs = clusterNodeGrp.elect(res,SAFplus::Group::ELECTION_TYPE_BOTH);

  cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Election result: " << rs << "\n";
  capabilities = clusterNodeGrp.getCapabilities(res.first);
  cout << "GMS_SERVER[" << myNodeAddress << "]: " << "--> Active had capability: " << capabilities << "\n";
  capabilities = clusterNodeGrp.getCapabilities(res.second);
  cout << "GMS_SERVER[" << myNodeAddress << "]: " << "--> Standby had capability: " << capabilities << "\n";

}
int main(int argc, char* argv[])
{
  ClRcT rc;
  int listenPort = GMS_PORT_1;

  if(clAspLocalId > 1)
  {
    listenPort = GMS_PORT_2;
  }
  initializeSAFplus();
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Initialize error! \n";
    return 0;
  }
  clIocLibInitialize(NULL);
  /* Create message handler to receive message from other group servers */
  registerAndStartMessageHandler(listenPort);
  /* Initialize callback to receive notification from CPM */
  //gmsNotificationInitialize();

  cout << "\nGMS_SERVER[" << myNodeAddress << "]: Initialized success! \n";
  /* Dispatch */
  while(1)
  {
    ;
  }

  cout << "Server is shutting down...";
  return 0;
}
