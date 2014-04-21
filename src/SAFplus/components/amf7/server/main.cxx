#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clCkptApi.hxx>
#include <clGroup.hxx>
#include <clThreadApi.hxx>
#include <clTestApi.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clCommon.hxx>
#include <clMgtApi.hxx>

#include <SAFplusAmf.hxx>

using namespace SAFplus;

SAFplusAmf::SAFplusAmfRoot cfg;

enum
  {
    NODENAME_LEN = 16*8,              // Make sure it is a multiple of 64 bits
    SC_ELECTION_BIT = 1<<8,           // This bit is set in the credential if the node is a system controller, making SCs preferred 
    STARTUP_ELECTION_DELAY_MS = 5000  // Wait for 5 seconds during startup if nobody is active so that other nodes can arrive, making the initial election not dependent on small timing issues. 
  };

// For now, this needs to be a "flat" class since it will be directly serialized and passed to the group's registerEntity function
// TO DO: implement endian-swap capable serializer/deserializer
class ClusterGroupData
{
public:
  unsigned int       structId;   // = 0x67839345
  unsigned int       nodeAddr;   // Same as slot id and TIPC address if these attributes exist 
  Handle            nodeMgtHandle;  // Identifier into the management db to identify this node's role.  INVALID_HDL means that this node does not have a role (AMF can assign it one).
  char               nodeName[NODENAME_LEN];  // Identifier into the management db to identify this node's role. Name and MgtHandle must resolve to the same entity (this field is probably unnecessary, prefer Handle)
  boost::asio::ip::address backplaneIp; // TODO: verify that this is FLAT!!!  V4 or V6 intracluster IP address of this node.
};


volatile bool    quitting=false;  // Set to true to tell all threads to quit
Group            clusterGroup;
ClusterGroupData clusterGroupData;  // The info we tell other nodes about this node.
Handle          myHandle;  // This handle resolves to THIS process.
unsigned int     myRole = 0;
unsigned int     capabilities=0;
unsigned int     credential=0;

// Threads
boost::thread    logServer;

struct LogServer
{
  void operator()()
  {
    while(!quitting)
      {
	printf("log server code here");
	sleep(1);
      }
  }
};



void activeAudit(void)  // Check to make sure DB and the system state are in sync
{
  logDebug("AUD","ACT","Active Audit");
}

void standbyAudit(void) // Check to make sure DB and the system state are in sync
{
  logDebug("AUD","SBY","Standby Audit");
}

void becomeActive(void)
{
  activeAudit();
}

void becomeStandby(void)
{
  standbyAudit();
}


int main(int argc, char* argv[])
{
  Mutex m;
  ThreadCondition somethingChanged;

  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  utilsInitialize();

  // GAS DEBUG:
  SAFplus::SYSTEM_CONTROLLER = 1;  // Normally we would get this from the environment

  myHandle = Handle::create();  // Actually, in the AMF's case I probably want to create a well-known component handle (i.e. the AMF on node X), not handle that means "pid-Y-on-node-X".  But it does not matter. It would just be so a helper function could be created.

  /* Initialize mgt database  */
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  db->initializeDB("SAFplusAmf");
  cfg.load(db);

  logServer = boost::thread(LogServer());

  // Needed?
  //groupServer = boost::thread(GroupServer());
  
  clusterGroup.init(CLUSTER_GROUP);
  clusterGroup.setNotification(somethingChanged);

  // Start essential SAFplus services

  // Once SAFplus is up, we can become a full member of the group.  The basic credential is the node's address.  
  // TODO: implement an environment variable to override this basic credential so users can control which node becomes master.

  if (SAFplus::SYSTEM_CONTROLLER) 
    {
      credential = SAFplus::ASP_NODEADDR | SC_ELECTION_BIT;
      capabilities = Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE;
    }
  else if (SAFplus::ASP_SC_PROMOTE) // A promotable payload can only become standby at first.
    {      
      credential =  SAFplus::ASP_NODEADDR;
      capabilities = Group::ACCEPT_STANDBY;
    }
  else  // Don't elect me!!!!
    {
      credential = 0;
      capabilities = 0;
    }

  clusterGroup.registerEntity(myHandle, credential, (void*) &clusterGroupData, sizeof(ClusterGroupData),capabilities);

  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs;
  activeStandbyPairs.first = clusterGroup.getActive();

  if (activeStandbyPairs.first == INVALID_HDL)  // If nobody is active, I need to call for an election
    {
      // By waiting, other nodes that are booting can come up.  This makes the system more consistently elect a particular node as ACTIVE when the cluster is started.  Note that this is just convenient for users, it does not matter to the system which node is elected active.
      boost::this_thread::sleep(boost::posix_time::milliseconds(STARTUP_ELECTION_DELAY_MS));
      int rc =  clusterGroup.elect(activeStandbyPairs);
      // GAS TODO:  What errors can be returned?
    }

  bool firstTime=true;
  while(!quitting)  // Active/Standby transition loop
    {
      ScopedLock<> lock(m);

      if (!firstTime && (somethingChanged.timed_wait(m,10000)==0))
	{  // Timeout
	  logDebug("IDL","---","...waiting for something to happen...");
	  if (myRole == Group::IS_ACTIVE) activeAudit();    // Check to make sure DB and the system state are in sync
	  if (myRole == Group::IS_STANDBY) standbyAudit();  // Check to make sure DB and the system state are in sync
	}
      else
	{  // Something changed in the group.
	  firstTime=false;
	  activeStandbyPairs.first = clusterGroup.getActive();
	  activeStandbyPairs.second = clusterGroup.getStandby();
	  if (myRole == Group::IS_ACTIVE) CL_ASSERT(activeStandbyPairs.first == myHandle);  // Once I become ACTIVE I can never lose it.
          else
	    {
	      if (activeStandbyPairs.first == myHandle)  // I just became active
		{
		  logInfo("---","---","This node just became the active system controller");
	          myRole = Group::IS_ACTIVE;
		  becomeActive();
		}
	    }
	  if (myRole != Group::IS_STANDBY)
	    {
	      if (activeStandbyPairs.second == myHandle)
		{
		  CL_ASSERT(myRole != Group::IS_ACTIVE);  // Fall back from active to standby is not allowed
		  logInfo("---","---","This node just became standby system controller");
	          myRole = Group::IS_STANDBY;
		  becomeStandby();
		}
	    }
	}
      
    }
  

}
