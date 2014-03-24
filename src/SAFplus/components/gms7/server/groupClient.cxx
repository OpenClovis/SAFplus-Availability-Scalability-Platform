// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#ifdef __WAKE
#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#endif
#include <clIocApi.h>
#include <clGroup.hxx>
#include <groupServer.hxx>
#include "clSafplusMsgServer.hxx"

using namespace SAFplus;
using namespace std;
int testRegisterAndConsistent();
int testElect();
int testGetData();
int testIterator();
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
#ifdef __WAKE
class testWakeble:public SAFplus::Wakeable
{
  public:
    void wake(int amt,void* cookie=NULL)
    {
      printf("WAKE! Something changed! \n");
    }
};
#endif
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


  ClIocAddressT iocDest;
  if(port == (int)GMS_PORT_1)
    iocDest.iocPhyAddress.nodeAddress = 1; //same node
  else
    iocDest.iocPhyAddress.nodeAddress = 2; //same node
  iocDest.iocPhyAddress.portId = port;
  MsgReply *msgReply = msgClient->SendReply(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
}
int sendDataToGms(GroupIdentity grpIdentity, messageTypeT messageType, int port)
{
  messageProtocol broadcastMsg;
  broadcastMsg.messageType = messageType;
  broadcastMsg.groupId = 0;
  broadcastMsg.numberOfItems = 1;
  broadcastMsg.grpIdentity = grpIdentity;
  cout << "DATA: Credential: " << grpIdentity.credentials << " Capabilities:" << grpIdentity.capabilities << "\n";
  sendMessage((void *)&broadcastMsg,sizeof(messageProtocol),port);
}
int main()
{
  int nodeAddress = 0;
  ClRcT rc = CL_OK;
  messageTypeT nodeJoin = CLUSTER_NODE_ARRIVAL, nodeLeave = CLUSTER_NODE_LEAVE;
  EntityIdentifier entityId1 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  EntityIdentifier entityId2 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  EntityIdentifier entityId3 = SAFplus::Handle(PersistentHandle,0,0,nodeAddress++,0);
  GroupIdentity grpIdentity1(entityId1,(int)ACTIVE_CREDENTIAL,(SAFplus::Buffer *)0,0,11);
  GroupIdentity grpIdentity2(entityId2,(int)STANDBY_CREDENTIAL,(SAFplus::Buffer *)0,0,15);
  GroupIdentity grpIdentity3(entityId3,100,(SAFplus::Buffer *)0,0,10);
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  {

  }
  clIocLibInitialize(NULL);
  msgClient = new SafplusMsgServer(CLIENT_PORT);
  msgClient->RegisterHandler(CL_IOC_PROTO_MSG, handler, NULL);
  msgClient->Start();
  cout << "Send Entity Join \n";
  sendDataToGms(grpIdentity1,nodeJoin,GMS_PORT_2);
  cout << "Send Entity Join \n";
  sendDataToGms(grpIdentity2,nodeJoin,GMS_PORT_2);
  cout << "Send Entity Join \n";
  sendDataToGms(grpIdentity3,nodeJoin,GMS_PORT_2);
  cout << "Send Elect request \n";
  sendDataToGms(grpIdentity1,CLUSTER_NODE_ELECT,GMS_PORT_1);
  //testRegisterAndConsistent();
  //testElect();
  //testGetData();
  //testIterator();
  msgClient->Stop();
  cout << "Done but not die \n";
  while(1);
  return 0;
}

int testIterator()
{
  Group gms("tester");
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,30);
  gms.registerEntity(entityId3,10,"ID3",3,10);

  SAFplus::Group::Iterator iter = gms.begin();
  while(iter != gms.end())
  {
    SAFplusI::BufferPtr curkey = iter->first;
    EntityIdentifier item = *(EntityIdentifier *)(curkey.get()->data);
    printf("Entity capability: %d \n",gms.getCapabilities(item));
    iter++;
  }
}

int testRegisterAndConsistent()
{
  Group gms("tester");
  EntityIdentifier me = SAFplus::Handle::create();
  uint64_t credentials = 5;
  std::string data = "DATA";
  int dataLength = 4;
  uint capabilities = 100;
  bool isMember = gms.isMember(me);
  printf("Is already member? %s \n",isMember ? "YES":"NO");
  if(isMember)
  {
    gms.deregister(me);
  }
  gms.registerEntity(me,credentials,(void *)&data,dataLength,capabilities);

  capabilities = gms.getCapabilities(me);
  printf("CAP: %d \n",capabilities);

  gms.setCapabilities(20,me);
  capabilities = gms.getCapabilities(me);
  printf("CAP: %d \n",capabilities);

  return 0;
}

int testGetData()
{
  Group gms("tester");
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,50);
  gms.registerEntity(entityId3,10,"ID3",3,10);

  printf("Entity 2 's data: %s \n",(gms.getData(entityId2)).data);
}

int testElect()
{
  Group gms("tester");
  uint capability = 0;

#ifdef __WAKE
  testWakeble tw;
  SAFplus::Wakeable *w = &tw;
#endif
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

#ifdef __WAKE
  gms.setNotification(*w);
#endif
  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,50);
  gms.registerEntity(entityId3,10,"ID3",3,10);

  capability |= SAFplus::Group::IS_ACTIVE;
  gms.setCapabilities(capability,entityId3);
  capability &= ~SAFplus::Group::IS_ACTIVE;
  capability |= SAFplus::Group::ACCEPT_ACTIVE | SAFplus::Group::ACCEPT_STANDBY | 16;
  gms.setCapabilities(capability,entityId1);
  gms.setCapabilities(capability,entityId2);
  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs;
  int rc =  gms.elect(activeStandbyPairs);

  int activeCapabilities = gms.getCapabilities(activeStandbyPairs.first);
  int standbyCapabilities = gms.getCapabilities(activeStandbyPairs.second);
  printf("Active CAP: %d \t Standby CAP: %d \n",activeCapabilities,standbyCapabilities);
  return 0;
}
