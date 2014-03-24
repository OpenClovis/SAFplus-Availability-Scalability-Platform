#include "MsgHandlerProtocols.hxx"
#include "clSafplusMsgServer.hxx"
#include "groupServer.hxx"

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

#define IOC_PORT 0
#define IOC_PORT_SERVER 65
#define CL_GMS_INELIGIBLE_CREDENTIALS 0x0

SAFplus::Group    clusterNodeGrp("CLUSTER_NODE");

void registerAndStartMessageHandler(int port)
{
  MsgHandlerProtocols handler;
  SAFplus::SafplusMsgServer safplusMsgServer(port);
  safplusMsgServer.RegisterHandler(CL_IOC_PROTO_MSG, &handler, NULL);
  safplusMsgServer.Start();
}
void sendBroadcast(void* data, int dataLength)
{
  SafplusMsgServer msgClient(IOC_PORT);
  ClIocAddressT iocDest;
  iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  iocDest.iocPhyAddress.portId = IOC_PORT_SERVER;
  MsgReply *msgReply = msgClient.SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
}
void entityJoinHandle(messageProtocol *rxMsg)
{
  GroupIdentity grpIdentity = rxMsg->data->grpIdentity;
  clusterNodeGrp.registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
  messageProtocol broadcastMsg;
  notificationData broadcastData;
  broadcastData.grpIdentity = grpIdentity;
  broadcastMsg.messageType = CLUSTER_NODE_ARRIVAL;
  broadcastMsg.groupId = 0;
  broadcastMsg.numberOfItems = 1;
  broadcastMsg.data = &broadcastData;
  sendBroadcast((void *)&broadcastMsg,sizeof(messageProtocol) + (broadcastMsg.numberOfItems * sizeof(notificationData)));
}
void entityLeaveHandle(messageProtocol *rxMsg)
{
  bool needReelect = false;
  int reElectType = 0;
  GroupIdentity grpIdentity = rxMsg->data->grpIdentity;
  if(! clusterNodeGrp.isMember(grpIdentity.id))
  {
    if(grpIdentity.id == clusterNodeGrp.getActive())
    {
      cout << "Active entity left \n";
      reElectType = (int)SAFplus::Group::ELECTION_TYPE_ACTIVE;
    }
    else if(grpIdentity.id == clusterNodeGrp.getStandby())
    {
      cout << "Standby entity left \n";
      reElectType = (int)SAFplus::Group::ELECTION_TYPE_STANDBY;
    }
    clusterNodeGrp.deregister(grpIdentity.id);
    messageProtocol broadcastMsg;
    notificationData broadcastData;
    broadcastData.grpIdentity = grpIdentity;
    broadcastMsg.messageType = CLUSTER_NODE_LEAVE;
    broadcastMsg.groupId = 0;
    broadcastMsg.numberOfItems = 1;
    broadcastMsg.data = &broadcastData;
    sendBroadcast((void *)&broadcastMsg,sizeof(messageProtocol) + (broadcastMsg.numberOfItems * sizeof(notificationData)));

    if(reElectType != 0)
    {
      std:pair<EntityIdentifier,EntityIdentifier> res;
      uint capabilities;
      int rs = clusterNodeGrp.elect(res,reElectType);
      cout << "Election result: " << rs << "\n";
      capabilities = clusterNodeGrp.getCapabilities(res.first);
      cout << "--> Active had capability: %d " << capabilities << "\n";
      capabilities = clusterNodeGrp.getCapabilities(res.second);
      cout << "--> Standby had capability: %d " << capabilities << "\n";
    }
  }
  else
  {
    cout << "Entity didn't exist!";
  }


}
void entityElectHandle()
{
  std:pair<EntityIdentifier,EntityIdentifier> res;
  uint capabilities;
  int rs = clusterNodeGrp.elect(res,SAFplus::Group::ELECTION_TYPE_BOTH);

  cout << "Election result: " << rs << "\n";
  capabilities = clusterNodeGrp.getCapabilities(res.first);
  cout << "--> Active had capability: %d " << capabilities << "\n";
  capabilities = clusterNodeGrp.getCapabilities(res.second);
  cout << "--> Standby had capability: %d " << capabilities << "\n";

}
int main(int argc, char* argv[])
{
  if(argc < 2)
  {
    cout << "Usage: gmsserver <listenport>";
    return 0;
  }

  int listenPort = atoi(argv[1]);

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
