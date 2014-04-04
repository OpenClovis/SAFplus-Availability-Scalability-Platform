// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clIocApi.h>
#include <clGroup.hxx>
#include <groupServer.hxx>
#include "clSafplusMsgServer.hxx"

using namespace SAFplus;
using namespace std;

void sendMessage(void* data, int dataLength,int port);
int sendDataToGms(GroupIdentity grpIdentity, messageTypeT messageType, int port);

class ClientMsgHandler:public SAFplus::MsgHandler
{
  public:
    void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      std::cout << "Received broadcast from [0x" << from.iocPhyAddress.nodeAddress << "," << from.iocPhyAddress.portId << "] \n";
    }
};

#define ACTIVE_CREDENTIAL 90
#define STANDBY_CREDENTIAL 80
#define GMS_PORT_1 69
#define GMS_PORT_2 70
#define CLIENT_PORT 71


ClUint32T clAspLocalId = 0x1;
ClBoolT gIsNodeRepresentative = CL_FALSE;
ClientMsgHandler *handler = new ClientMsgHandler();
SafplusMsgServer *msgClient;
void sendMessage(void* data, int dataLength,int port)
{
  ClBoolT msgSent = false;
  ClIocAddressT iocDest;
  if(port == (int)GMS_PORT_1)
    iocDest.iocPhyAddress.nodeAddress = 1; //same node
  else
    iocDest.iocPhyAddress.nodeAddress = 2; //same node
  iocDest.iocPhyAddress.portId = port;
  while(msgSent == false)
  {
    try
    {
      MsgReply *msgReply = msgClient->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
      msgSent = true;
    }
    catch(...)
    {
      /* Retrying */
      msgSent = false;
    }
  }

}
int sendDataToGms(GroupIdentity grpIdentity, messageTypeT messageType, int port)
{
  messageProtocol broadcastMsg;
  broadcastMsg.messageType = messageType;
  broadcastMsg.groupId = 0;
  broadcastMsg.numberOfItems = 1;
  broadcastMsg.grpIdentity = grpIdentity;
  cout << "GMS_CLIENT: DATA: Credential: " << grpIdentity.credentials << " Capabilities:" << grpIdentity.capabilities << "\n";
  sendMessage((void *)&broadcastMsg,sizeof(messageProtocol),port);
}
int main()
{
  int nodeAddress = 0;
  char pause;
  ClRcT rc = CL_OK;
  messageTypeT nodeJoin = CLUSTER_NODE_ARRIVAL, nodeLeave = CLUSTER_NODE_LEAVE;
  EntityIdentifier entityId1 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  EntityIdentifier entityId2 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  EntityIdentifier entityId3 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  GroupIdentity standbyEntity(entityId1,90,(SAFplus::Buffer *)0,0,11); //Allow active & standby
  GroupIdentity candidateEntity(entityId2,80,(SAFplus::Buffer *)0,0,15); //Allow active & standby
  GroupIdentity activeEntity(entityId3,100,(SAFplus::Buffer *)0,0,10); //Allow Active
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {
    cout << "GMS_CLIENT: Libs initialized error";
    return 0;
  }
  clIocLibInitialize(NULL);
  msgClient = new SafplusMsgServer(CLIENT_PORT);
  msgClient->RegisterHandler(CL_IOC_PROTO_MSG, handler, NULL);
#ifdef __TESTREALSERVER
  cout << "GMS_CLIENT: Signal to master node \n";
  sendDataToGms(standbyEntity,nodeJoin,GMS_PORT_1);
  while(1);
#endif // __TESTREALSERVER
  cout << "GMS_CLIENT: Send Entity Join for server #2 \n";
  sendDataToGms(standbyEntity,nodeJoin,GMS_PORT_2);
  cout << "GMS_CLIENT: Send Entity Join for server #2 \n";
  sendDataToGms(candidateEntity,nodeJoin,GMS_PORT_2);
  cout << "GMS_CLIENT: Send Entity Join for server #2 \n";
  sendDataToGms(activeEntity,nodeJoin,GMS_PORT_2);

  /* Node, 2 server must be give the same result */
  cout << "GMS_CLIENT: Send Elect request for server #1 \n";
  sendDataToGms(standbyEntity,CLUSTER_NODE_ELECT,GMS_PORT_1);
  cout << "GMS_CLIENT: Send Elect request for server #2 \n";
  sendDataToGms(standbyEntity,CLUSTER_NODE_ELECT,GMS_PORT_2);

  cin >> pause;

  cout << "GMS_CLIENT: Send Entity (standby) Leave for server #1 \n";
  sendDataToGms(standbyEntity,nodeLeave,GMS_PORT_1);

  cin >> pause;

  cout << "GMS_CLIENT: Send Entity (Standby) Join for server #1 \n";
  sendDataToGms(standbyEntity,nodeJoin,GMS_PORT_1);
  cout << "GMS_CLIENT: Send Elect request for server #2 \n";
  sendDataToGms(standbyEntity,CLUSTER_NODE_ELECT,GMS_PORT_2);
  cout << "GMS_CLIENT: Send Elect request for server #1 \n";
  sendDataToGms(standbyEntity,CLUSTER_NODE_ELECT,GMS_PORT_1);

  cin >> pause;

  cout << "GMS_CLIENT: Send Entity (active) Leave for server #2 \n";
  sendDataToGms(activeEntity,nodeLeave,GMS_PORT_2);

  msgClient->Stop();
  cin >> pause;
  while(1);
  return 0;
}
