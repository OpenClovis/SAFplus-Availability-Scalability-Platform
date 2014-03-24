// Standard includes
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <boost/functional/hash.hpp>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clIocApi.h>
#include <clGroup.hxx>


using namespace SAFplus;
int testRegisterAndConsistent();
int testElect();
int testGetData();
int testIterator();
class testWakeble:public SAFplus::Wakeable
{
  public:
    void wake(int amt,void* cookie=NULL)
    {
      printf("WAKE! Something changed! \n");
    }
};
int main()
{
  //testRegisterAndConsistent();
  testElect();
  //testGetData();
  //testIterator();
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
  testWakeble tw;
  SAFplus::Wakeable *w = &tw;

  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

  gms.setNotification(*w);
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
