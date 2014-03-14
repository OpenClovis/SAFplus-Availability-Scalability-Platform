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
#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#include <clGroup.hxx>

using namespace SAFplus;
int testRegisterAndConsistent();
int testElect();
int testGetData();
int main()
{
  //testRegisterAndConsistent();
  //testElect();
  testGetData();
  return 0;
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
    gms.deregister(me,false);
  }
  gms.registerEntity(me,credentials,(void *)&data,dataLength,capabilities,false);

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

  gms.registerEntity(entityId1,20,"ID1",3,20,false);
  gms.registerEntity(entityId2,50,"ID2",3,50,false);
  gms.registerEntity(entityId3,10,"ID3",3,10,false);

  printf("Entity 2 's data: %s \n",(gms.getData(entityId2)).data);
}

int testElect()
{
  Group gms("tester");
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

  gms.registerEntity(entityId1,20,"ID1",3,20,false);
  gms.registerEntity(entityId2,50,"ID2",3,50,false);
  gms.registerEntity(entityId3,10,"ID3",3,10,false);

  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs = gms.elect();

  int activeCapabilities = gms.getCapabilities(activeStandbyPairs.first);
  int standbyCapabilities = gms.getCapabilities(activeStandbyPairs.second);
  printf("Active CAP: %d \t Standby CAP: %d \n",activeCapabilities,standbyCapabilities);
  return 0;
}
