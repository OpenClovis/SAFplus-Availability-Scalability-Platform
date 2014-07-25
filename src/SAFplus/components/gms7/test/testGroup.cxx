// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clIocApi.h>
#include <clGroup.hxx>
#include <clGlobals.hxx>
#include <clNameApi.hxx>

using namespace SAFplus;
using namespace std;
int testCapabilities(int mode);
int testGroupElect(int mode);
int testGetData(int mode);
int testGroupIterator(int mode);
int testRegisterAndDeregister(int mode);

ClUint32T clAspLocalId = 1;
ClBoolT   gIsNodeRepresentative = CL_TRUE;
class testWakeble:public SAFplus::Wakeable
{
  public:
    int amtsig;
    void wake(int amt,void* cookie=NULL)
    {
      cout << "WAKE! AMT = " << amt << "\n";
      amtsig = amt;
    }
    testWakeble()
    {
      amtsig = -1;
    }
};

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;


int main(int argc, char* argv[])
{
  int tc = -1;
  int mode = SAFplus::Group::DATA_IN_CHECKPOINT;

  if(argc >= 2)
  {
    if(argc > 2)
    {
      mode = SAFplus::Group::DATA_IN_MEMORY;
    }
    tc = atoi(argv[1]);
  }


  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  utilsInitialize();

  ClRcT rc;
  // initialize SAFplus6 libraries 
  //if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  if ((rc = SAFplus::clOsalInitialize(NULL)) != CL_OK || (rc = SAFplus::clHeapInit()) != CL_OK || (rc = SAFplus::clBufferInitialize(NULL)) != CL_OK || (rc = SAFplus::clTimerInitialize(NULL)) != CL_OK)
    {
    assert(0);
    }
  
  SAFplus::ASP_NODEADDR = 1;
  rc = SAFplus::clIocLibInitialize(NULL);
  assert(rc==CL_OK);

  safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  switch(tc)
  {
    case 0:
      testRegisterAndDeregister(mode);
      break;
    case 1:
      testGroupIterator(mode);
      break;
    case 2:
      testGetData(mode);
      break;
    case 3:
      testCapabilities(mode);
      break;
    case 4:
      testGroupElect(mode);
      break;
    default:
      cout << "Usage: testGroup <testcase> [m]\n";
      cout << "-------------------------------\n";
      cout << "m: MEMORY, (default CHECKPOINT)\n";
      cout << "0. Register and deregister \n";
      cout << "1. Group iterator \n";
      cout << "2. Group associated data \n";
      cout << "3. Entity capabilities \n";
      cout << "4. Group election \n";
      cout << "-------------------------------\n";
      break;
  }
  return 0;
}

int testGroupIterator(int mode)
{
  cout << "TC: GROUP ITERATOR START \n";
  Group gms(__FUNCTION__,mode);
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();
  cout << "TC: register 3 entity \n";
  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,30);
  gms.registerEntity(entityId3,10,"ID3",3,10);
  cout << "TC: start iterate \n";
  SAFplus::Group::Iterator iter = gms.begin();
  while(iter != gms.end())
  {
    EntityIdentifier item = iter->first;
    cout << "--> TC: Entity capabilites: " << gms.getCapabilities(item) << "\n";
    iter++;
  }
  cout << "TC: GROUP ITERATOR END \n";
  return 0;
}

int testCapabilities(int mode)
{
  cout << "TC: GROUP CAPABILITIES START \n";
  Group gms(__FUNCTION__,mode);
  EntityIdentifier me = SAFplus::Handle::create();
  uint64_t credentials = 5;
  std::string data = "DATA";
  int dataLength = 4;
  uint capabilities = 100;
  cout << "TC: Where the anonymous entity is a member? \n";
  bool isMember = gms.isMember(me);
  cout << "--> TC: " << (isMember ? "YES" : "NO") << "\n";
  if(isMember)
  {
    gms.deregister(me);
  }
  cout << "TC: Register entity with capabilities " << capabilities << "\n";
  gms.registerEntity(me,credentials,(void *)&data,dataLength,capabilities);
  cout << "TC: Check registered capabilities \n";
  capabilities = gms.getCapabilities(me);
  cout << "--> TC: " << capabilities << "\n";
  cout << "TC: Set new capabilities [20] for entity \n";
  gms.setCapabilities(20,me);
  cout << "TC: Check new capabilities \n";
  capabilities = gms.getCapabilities(me);
  cout << "--> TC: " << capabilities << "\n";
  cout << "TC: GROUP CAPABILITIES END \n";
  return 0;
}

int testGetData(int mode)
{
  cout << "TC: ENTITY ASSOCIATED DATA START \n";
  Group gms(__FUNCTION__,mode);
  char* data;
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();
  cout << "TC: Registered 3 entities with data are: ID1,ID2,ID3 \n";
  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,50);
  gms.registerEntity(entityId3,10,"ID3",3,10);
  cout << "TC: Get data of 3 entities \n";
  data = (char *)gms.getData(entityId1);
  cout << "--> TC: Entity 1: " << data << "\n";
  data = (char *)gms.getData(entityId2);
  cout << "--> TC: Entity 2: " << data << "\n";
  data = (char *)gms.getData(entityId3);
  cout << "--> TC: Entity 3: " << data << "\n";
  cout << "TC: ENTITY ASSOCIATED DATA END \n";
  return 0;
}

int testGroupElect(int mode)
{
  cout << "TC: GROUP ELECTION START \n";
  Group gms(__FUNCTION__,mode);
  uint capability = 0;
  testWakeble tw;
  char* name;
  //SAFplus::Wakeable *w = &tw;
  gms.setNotification(tw);

  EntityIdentifier entityId1 = SAFplus::Handle::create();
  EntityIdentifier entityId2 = SAFplus::Handle::create();
  EntityIdentifier entityId3 = SAFplus::Handle::create();

  cout << "TC: Register 3 entities \n";
  gms.registerEntity(entityId1,20,"ID1",3,20);
  gms.registerEntity(entityId2,50,"ID2",3,50);
  gms.registerEntity(entityId3,10,"ID3",3,10);

  cout << "TC: Entity 3 set IS_ACTIVE \n";
  capability |= SAFplus::Group::IS_ACTIVE;
  gms.setCapabilities(capability,entityId3);
  cout << "TC: Entity 1,2 set both ACCEPT_ACTIVE and ACCEPT_STANDBY \n";
  capability &= ~SAFplus::Group::IS_ACTIVE;
  capability |= SAFplus::Group::ACCEPT_ACTIVE | SAFplus::Group::ACCEPT_STANDBY | 16;
  gms.setCapabilities(capability,entityId1);
  gms.setCapabilities(capability,entityId2);
  cout << "TC: Start wait for election \n";
  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs = gms.elect();
  while(tw.amtsig != SAFplus::Group::ELECTION_FINISH_SIG)
  {
    ;
  }
  cout << "TC: Election done, checking result \n";
  int activeCapabilities = gms.getCapabilities(gms.getActive());
  int standbyCapabilities = gms.getCapabilities(gms.getStandby());
  name = (char *)gms.getData(gms.getActive());
  cout << "--> TC: Active " << name << "\n";
  name = (char *)gms.getData(gms.getStandby());
  cout << "--> TC: Standby " << name  << "\n";
  cout << "TC: GROUP ELECTION END \n";
  return 0;
}

int testRegisterAndDeregister(int mode)
{
  cout << "TC: GROUP REGISTER AND DEREGISTER START \n";
  const std::string s = __FUNCTION__;
  Group gms(s,mode);

  cout << "TC: Register entity \n";
  EntityIdentifier entityId1 = SAFplus::Handle::create();
  gms.registerEntity(entityId1,20,"ID1",3,20);
  cout << "TC: Deregister entity \n";
  gms.deregister(entityId1);
  cout << "TC: Register entity \n";
  gms.registerEntity(entityId1,20,"ID1",3,20);
  cout << "TC: Deregister entity with INVALID_HDL \n";
  gms.deregister();
  cout << "TC: Check whether entity is still a member? \n";
  cout << "--> TC: " << (gms.isMember(entityId1) ? "YES" : "NO") << "\n";
  cout << "TC: Register entity \n";
  gms.registerEntity(entityId1,20,"ID1",3,20);
  cout << "TC: Check whether entity is still a member? \n";
  cout << "--> TC: " << (gms.isMember(entityId1) ? "YES" : "NO") << "\n";
  cout << "TC: GROUP REGISTER AND DEREGISTER END \n";
  return 0;
}
