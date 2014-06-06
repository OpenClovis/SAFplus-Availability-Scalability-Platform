#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>

#include <sys/types.h>
#include <sys/wait.h>

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
#include <clNameApi.hxx>
#include <clIocPortList.hxx>

#include <clAmfPolicyPlugin.hxx>
#include <SAFplusAmf.hxx>
#include <clSafplusMsgServer.hxx>

#include "clRpcChannel.hxx"
#include "amfRpc/amfRpc.pb.h"


#define GRP

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

typedef boost::unordered_map<SAFplus::AmfRedundancyPolicy,ClPluginHandle*> RedPolicyMap;

RedPolicyMap redPolicies;

// IOC related globals
ClUint32T clAspLocalId = 0x1;
ClUint32T chassisId = 0x0;
ClBoolT   gIsNodeRepresentative = CL_TRUE;

SAFplusAmf::SAFplusAmfRoot cfg;

enum
  {
  NODENAME_LEN = 16*8,              // Make sure it is a multiple of 64 bits
  SC_ELECTION_BIT = 1<<8,           // This bit is set in the credential if the node is a system controller, making SCs preferred 
  STARTUP_ELECTION_DELAY_MS = 2000  // Wait for 5 seconds during startup if nobody is active so that other nodes can arrive, making the initial election not dependent on small timing issues. 
  };

// For now, this needs to be a "flat" class since it will be directly serialized and passed to the group's registerEntity function
// TO DO: implement endian-swap capable serializer/deserializer
class ClusterGroupData
  {
public:
  unsigned int       structId;       // = 0x67839345
  unsigned int       nodeAddr;       // Same as slot id and TIPC address if these attributes exist 
  Handle             nodeMgtHandle;  // Identifier into the management db to identify this node's role.  INVALID_HDL means that this node does not have a role (AMF can assign it one).
  char               nodeName[NODENAME_LEN];  // Identifier into the management db to identify this node's role. Name and MgtHandle must resolve to the same entity (this field is probably unnecessary, prefer Handle)
  boost::asio::ip::address backplaneIp; // TODO: verify that this is FLAT!!!  V4 or V6 intracluster IP address of this node.
  };

volatile bool    quitting=false;  // Set to true to tell all threads to quit
Group            clusterGroup(SAFplus::Group::DATA_IN_CHECKPOINT);
ClusterGroupData clusterGroupData;  // The info we tell other nodes about this node.
Handle           myHandle;  // This handle resolves to THIS process.
Handle           nodeHandle; //? The handle associated with this node
unsigned int     myRole = 0;
unsigned int     capabilities=0;
unsigned int     credential=0;


static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

// Threads
boost::thread    logServer;

struct LogServer
  {
  void operator()()
    {
    while(!quitting)
      {
      //printf("log server code here\n");
      sleep(1);
      }
    }
  };



void activeAudit()  // Check to make sure DB and the system state are in sync
  {
  logDebug("AUD","ACT","Active Audit");
  RedPolicyMap::iterator it;

  for (it = redPolicies.begin(); it != redPolicies.end();it++)
    {
    ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*>(it->second->pluginApi);
    pp->activeAudit(&cfg);
    }
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


void loadAmfPlugins(AmfOperations& amfOps)
  {
  // pick the SAFplus directory or the current directory if it is not defined.
  const char * soPath = (SAFplus::ASP_APP_BINDIR[0] == 0) ? ".":SAFplus::ASP_APP_BINDIR;
  
  boost::filesystem::path p(soPath);
  boost::filesystem::directory_iterator it(p),eod;

  BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod))
    {
    if (p.extension()==".so")
      {
      if (p.string().find("AmfPolicy") != std::string::npos)
        {
        if(is_regular_file(p))
          {
          const char *s = p.c_str();
          clLogInfo("POL","LOAD","Loading policy: %s", s);
          ClPluginHandle* plug = clLoadPlugin(CL_AMF_POLICY_PLUGIN_ID,CL_AMF_POLICY_PLUGIN_VER,s);
          if (plug)
            {
            ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*> (plug->pluginApi);
            if (pp)
              {
              bool result = pp->initialize(&amfOps);
              if (result)
                {
                redPolicies[pp->policyId] = plug;
                clLogInfo("POL","LOAD","AMF Policy [%d] plugin [%s] load succeeded.", pp->policyId, p.c_str());
                }
              else
                {
                clLogError("POL","LOAD","AMF Policy plugin [%s] load failed.  Plugin initialize error.", p.c_str());
                }
              }
            else clLogError("POL","LOAD","AMF Policy plugin [%s] load failed.  Incorrect plugin type.", p.c_str());
            }
          else clLogError("POL","LOAD","Policy [%s] load failed.  Incorrect plugin Identifier or version.", p.c_str());
          } 
            
        }
      }
    }  
  }


void printUsage(char* progname)
  {
  clOsalPrintf("Usage: %s -c <Chassis ID> -l <Local Slot ID> -n <Node Name> \n", progname);
  clOsalPrintf("Example : %s -c 0 -l 1 -n node_0\n", progname);
  clOsalPrintf("or\n");
  clOsalPrintf("Example : %s --chassis=0 --localslot=1 --nodename=<nodeName>\n", progname);
  clOsalPrintf("Options:\n");
  clOsalPrintf("-c, --chassis=ID       Chassis ID\n");
  clOsalPrintf("-l, --localslot=ID     Local slot ID\n");
  clOsalPrintf("-n, --nodename=name    Node name\n");
  //clOsalPrintf("-f, --foreground       Run AMF as foreground process\n");
  clOsalPrintf("-h, --help             Display this help and exit\n");  
  }

static ClRcT cpmStrToInt(const ClCharT *str, ClUint32T *pNumber) 
  {
  ClUint32T i = 0;

  for (i = 0; str[i] != '\0'; ++i) 
    {
    if (!isdigit(str[i])) 
      {
      goto not_valid;
      }
    }

  *pNumber = atoi(str);

  return CL_OK;

  not_valid:
  return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
  }

int parseArgs(int argc, char* argv[])
  {
  ClInt32T option = 0;
  ClInt32T nargs = 0;
  ClRcT rc = CL_OK;
  const ClCharT *short_options = ":c:l:m:n:p:fh";

#ifndef POSIX_ONLY
  const struct option long_options[] = {
    {"chassis",     1, NULL, 'c'},
    {"localslot",   1, NULL, 'l'},
    {"nodename",    1, NULL, 'n'},
    {"profile",     1, NULL, 'p'},
    {"foreground",  0, NULL, 'f'},
    {"help",        0, NULL, 'h'},
    { NULL,         0, NULL,  0 }
    };
#endif


#ifndef POSIX_ONLY
  while((option = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
#else
    while((option = getopt(argc, argv, short_options)) != -1)
#endif
      switch(option)
        {
        case 'h':   
          printUsage(argv[0]);
          /* This is not an error */
          exit(0);
        case 'c':   
          rc = cpmStrToInt(optarg, &chassisId);
          if (CL_OK != rc) 
            {
            clLogError("AMF","BOOT","[%s] is not a valid chassis id.", optarg);
            return -1;
            }
          break;
        case 'l':
        {
        ClUint32T temp=0;
        rc = cpmStrToInt(optarg, &temp);
        SAFplus::ASP_NODEADDR = temp;
        if (CL_OK != rc)
          {
          logError("AMF","BOOT", "[%s] is not a valid slot id, ", optarg);
          return -1;
          }
        ++nargs;
        } break;
        case 'n':
          strncpy(SAFplus::ASP_NODENAME, optarg, CL_MAX_NAME_LENGTH-1);
          strncpy(::ASP_NODENAME, optarg, CL_MAX_NAME_LENGTH-1);
          ++nargs;
          break;
#if 0            
        case 'p':
        {
        strncpy(clCpmBootProfile, optarg, CL_MAX_NAME_LENGTH-1);
        ++nargs;
        }           
        break;
#endif            
        case '?':
          logError("AMF","BOOT", "Unknown option [%c]", optopt);
          return -1;
          break;
        default :   
          logError("AMF","BOOT", "Unknown error");
          return -1;
          break;
        }

  return 1;
  }


namespace SAFplusAmf { void createTestDataSet(SAFplusAmfRoot* self); };

// Callback RPC client
void FooDone(StartComponentResponse* response)
  {
    std::cout << "DONE" << std::endl;
  }

int main(int argc, char* argv[])
  {
  Mutex m;
  ThreadCondition somethingChanged;
  bool firstTime=true;

  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  if (parseArgs(argc,argv)<=0) return -1;

  utilsInitialize();

  ClRcT rc;
  // initialize SAFplus6 libraries 
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
    {
    assert(0);
    }

  clAspLocalId  = SAFplus::ASP_NODEADDR;  // remove clAspLocalId
  rc = clIocLibInitialize(NULL);
  assert(rc==CL_OK);

#if 0
  SAFplus::SafplusMsgServer safplusMsgServer(SAFplusI::AMF_IOC_PORT, MAX_MSGS, MAX_HANDLER_THREADS);
  // Handle RPC
  //Start Sever RPC
  ClIocAddressT dest;
  *((uint64_t*) &dest) = CL_IOC_ADDRESS_FORM(CL_IOC_PHYSICAL_ADDRESS_TYPE,SAFplus::ASP_NODEADDR,SAFplusI::AMF_IOC_PORT);
  SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, dest);
  //End server TPC

  safplusMsgServer.Start();

  // client side
  amfRpc_Stub service(channel);
  StartComponentRequest req;
  req.set_name("c0");
  req.set_command("./c0 test1 test2");
  StartComponentResponse resp;

  //client side should using callback
  //google::protobuf::Closure *callback = google::protobuf::NewCallback(&FooDone, &resp);
  //service.startComponent(NULL,&req, &resp, callback);
  service.startComponent(NULL,&req, &resp, NULL);
#endif

  // GAS DEBUG:
  SAFplus::SYSTEM_CONTROLLER = 1;  // Normally we would get this from the environment

  myHandle = Handle::create();  // TODO: Actually, in the AMF's case I probably want to create a well-known component handle (i.e. the AMF on node X), not handle that means "pid-Y-on-node-X".  But it does not matter. It would just be so a helper function could be created.
  // Register a mapping between this node's name and its handle.
  nodeHandle = myHandle; // TODO: No should be different
  name.set(SAFplus::ASP_NODENAME,nodeHandle,NameRegistrar::MODE_NO_CHANGE);

  /* Initialize mgt database  */
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  db->initializeDB("SAFplusAmf");
  //cfg.read(db);
  createTestDataSet(&cfg);
  setAdminState((SAFplusAmf::ServiceGroup*) cfg.serviceGroupList["sg0"],AdministrativeState::on);

  logServer = boost::thread(LogServer());

  AmfOperations amfOps;

  loadAmfPlugins(amfOps);

#ifdef GRP  
  clusterGroup.init(CLUSTER_GROUP);
  clusterGroup.setNotification(somethingChanged);
#endif

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

#ifdef GRP
  clusterGroup.registerEntity(myHandle, credential, (void*) &clusterGroupData, sizeof(ClusterGroupData),capabilities);
#endif
  logInfo("AMF","HDL", "I AM [%lx:%lx]", myHandle.id[0],myHandle.id[1]);

  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs;
#ifdef GRP
  activeStandbyPairs.first = clusterGroup.getActive();
  activeStandbyPairs.second = clusterGroup.getStandby();
#endif
  logInfo("AMF","BOOT", "Active [%lx:%lx] Standby: [%lx:%lx]", activeStandbyPairs.first.id[0],activeStandbyPairs.first.id[1],activeStandbyPairs.second.id[0],activeStandbyPairs.second.id[1]);
  
  if (activeStandbyPairs.first == INVALID_HDL)  // If nobody is active, I need to call for an election
    {
    // By waiting, other nodes that are booting can come up.  This makes the system more consistently elect a particular node as ACTIVE when the cluster is started.  Note that this is just convenient for users, it does not matter to the system which node is elected active.
    // Commented out because the election has a 10 second delay
    // boost::this_thread::sleep(boost::posix_time::milliseconds(STARTUP_ELECTION_DELAY_MS));
#ifdef GRP
    activeStandbyPairs = clusterGroup.elect();
    // GAS TODO:  What errors can be returned?
#else
    activeStandbyPairs.first = myHandle;
    firstTime = false;
#endif
    }

  while(!quitting)  // Active/Standby transition loop
    {
    ScopedLock<> lock(m);

    if (!firstTime && (somethingChanged.timed_wait(m,2000)==0))
      {  // Timeout
      logDebug("IDL","---","...waiting for something to happen...");
      int pid;
      do
        {
        int status = 0;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid>0)
          {
          logWarning("PRC","MON","Child process [%d] has failed", pid);
          }
        if (pid<0)
          {
          // ECHILD means no child processes forked yet.
          if (errno != ECHILD) logWarning("PRC","MON","waitpid errno [%s (%d)]", strerror(errno), errno);
          }

        // TODO: find this pid in the component database and fail it.  This will be faster.  For now, do nothing because the periodic pid checker should catch it.
        } while(pid > 0);
      if (myRole == Group::IS_ACTIVE) activeAudit();    // Check to make sure DB and the system state are in sync
      if (myRole == Group::IS_STANDBY) standbyAudit();  // Check to make sure DB and the system state are in sync

      // GAS DBG: just to test audit with election not working
      // activeAudit();
      }
    else
      {  // Something changed in the group.
      firstTime=false;
#ifdef GRP
      activeStandbyPairs.first = clusterGroup.getActive();
      activeStandbyPairs.second = clusterGroup.getStandby();
      assert((activeStandbyPairs.first != activeStandbyPairs.second) || ((activeStandbyPairs.first == INVALID_HDL)&&(activeStandbyPairs.second == INVALID_HDL)) );
      if ((activeStandbyPairs.first == INVALID_HDL)||(activeStandbyPairs.second == INVALID_HDL)) clusterGroup.elect();
#endif
      if (myRole == Group::IS_ACTIVE) 
        {
        //CL_ASSERT(activeStandbyPairs.first == myHandle);  // Once I become ACTIVE I can never lose it.
        if (activeStandbyPairs.first != myHandle) // I am no longer active
          {
          //stopActive(); TBD
          myRole = 0;
          }
        }
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
