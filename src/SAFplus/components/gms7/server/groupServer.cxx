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

class ServerMsgHandler:public SAFplus::MsgHandler
{
  public:
    void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      messageProtocol *rxMsg = (messageProtocol *)msg;
      /* If local message, ignore */
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
    }
};

/* SAFplus specified variables */
ClUint32T clAspLocalId = atoi(getenv("NODEADDR"));
ClBoolT gIsNodeRepresentative = CL_TRUE;
ClIocNodeAddressT myNodeAddress = clAspLocalId;
/* Global variables */
ServerMsgHandler *handler = new ServerMsgHandler();
SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");



void initializeSAFplus()
{
}
void registerAndStartMessageHandler(int port)
{
  SAFplus::SafplusMsgServer safplusMsgServer(port);
  safplusMsgServer.RegisterHandler(CL_IOC_PROTO_MSG, handler, NULL);
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
  if(! clusterNodeGrp.isMember(grpIdentity.id))
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
    cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Entity didn't exist!";
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
  if(argc < 2)
  {
    cout << "Usage: ServerTest <listenport> \n";
    return 0;
  }

  int listenPort = atoi(argv[1]);
  initializeSAFplus();
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {
    cout << "GMS_SERVER[" << myNodeAddress << "]: " << "Initialize error! \n";
    return 0;
  }
  clIocLibInitialize(NULL);
  /* Create message handler to receive message from other group servers */
  registerAndStartMessageHandler(listenPort);
  /* Dispatch */
  while(1)
  {
    ;
  }

  cout << "Server is shutting down...";
  return 0;
}
