#include <chrono>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/asio.hpp> // for signal handling

#include <sys/types.h>
#include <sys/wait.h>

#include <clMgtRoot.hxx>
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clCkptApi.hxx>
#include <clGroupApi.hxx>
#include <clThreadApi.hxx>
#include <clTestApi.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clCommon.hxx>
#include <clMgtApi.hxx>
#include <clNameApi.hxx>
#include <clFaultApi.hxx>
#include <clFaultServerIpi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clProcessStats.hxx>
#include <clNodeStats.hxx>

#include <clAmfPolicyPlugin.hxx>
#include <SAFplusAmfModule.hxx>
#include <Component.hxx>
#include <clSafplusMsgServer.hxx>
//#include <clTimer7.hxx>

#include "clRpcChannel.hxx"
#include <amfRpc.pb.hxx>
#include <amfRpc.hxx>
#include <amfAppRpc.hxx>
#include "amfAppRpcImplAmfSide.hxx"

#define USE_GRP

#ifdef SAFPLUS_AMF_GRP_NODE_REPRESENTATIVE
#include <clGroupIpi.hxx>
#endif

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

typedef boost::unordered_map<SAFplus::AmfRedundancyPolicy,ClPluginHandle*> RedPolicyMap;

// namespace SAFplusAmf { void loadAmfConfig(SAFplusAmfModule* self); };

void initializeOperationalValues(SAFplusAmf::SAFplusAmfModule& cfg);
void loadAmfPluginsAt(const char* soPath, AmfOperations& amfOps,Fault& fault);

RedPolicyMap redPolicies;

// IOC related globals
extern ClUint32T chassisId;

// signal this condition to execute a cluster state reevaluation early
ThreadCondition somethingChanged;
bool amfChange = false;  //? Set to true if the change was AMF related (process started/died or other AMF state change)
bool grpChange = false;  //? Set to true if the change was group related

SAFplusAmf::SAFplusAmfModule cfg;
//MgtModule dataModule("SAFplusAmf");

const char* LogArea = "MAN";

SAFplus::Fault fault;  // Fault client global variable

enum
  {
  NODENAME_LEN = 16*8,              // Make sure it is a multiple of 64 bits
  SC_ELECTION_BIT = 1<<8,           // This bit is set in the credential if the node is a system controller, making SCs preferred 
  STARTUP_ELECTION_DELAY_MS = 2000,  // Wait for 5 seconds during startup if nobody is active so that other nodes can arrive, making the initial election not dependent on small timing issues. 

  REEVALUATION_DELAY = 5000, //? How long to wait (max) before reevaluating the cluster
  };

static void sigChildHandler(int signum)
{
    int pid;
    int w;

    /* 
     * Wait for all childs to finish.
     */
#if 0    
do {
        pid = waitpid(WAIT_ANY, &w, WNOHANG);
    } while((pid != 0)&&(pid != -1));
#endif
    amfChange = true;
    somethingChanged.notify_all();
}
#if 0
void signalHandler(const boost::system::error_code& error, int signal_number)
{
  if (!error)
  {
    if (signal_number == SIGCHLD)
      {
        somethingChanged.notify_all();
      }
  }
}
#endif


#if 0  // No longer necessary.  We bind at the module level
//? Connect local data to the management tree
void bind()
  {
    SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::AMF_IOC_PORT);
    cfg.clusterList.bind(hdl, "SAFplusAmf", "Cluster");
    cfg.nodeList.bind(hdl, "SAFplusAmf", "Node");
    cfg.serviceGroupList.bind(hdl, "SAFplusAmf", "ServiceGroup");
    cfg.componentList.bind(hdl, "SAFplusAmf", "Component");
    cfg.componentServiceInstanceList.bind(hdl, "SAFplusAmf", "ComponentServiceInstance");
    cfg.serviceInstanceList.bind(hdl, "SAFplusAmf", "ServiceInstance");
    cfg.serviceUnitList.bind(hdl, "SAFplusAmf", "ServiceUnit");
    cfg.applicationList.bind(hdl, "SAFplusAmf", "Application");
    cfg.entityByNameList.bind(hdl, "SAFplusAmf", "EntityByName");
    cfg.entityByIdList.bind(hdl, "SAFplusAmf", "EntityById");
    cfg.healthCheckPeriod.bind(hdl, "SAFplusAmf", "healthCheckPeriod");
    cfg.healthCheckMaxSilence.bind(hdl, "SAFplusAmf", "healthCheckMaxSilence");
  }
#endif

class MgtMsgHandler : public SAFplus::MsgHandler
{
  public:
    virtual void msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      Mgt::Msg::MsgMgt mgtMsgReq;
      mgtMsgReq.ParseFromArray(msg, msglen);
      if (mgtMsgReq.type() == Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND_REQUEST)
      {
        assert(0);  // Nobody should call this because of the checkpoint
#if 0
          bind();
#endif
      }
      else
      {
        MgtRoot::getInstance()->mgtMessageHandler.msgHandler(from, svr, msg, msglen, cookie);
      }
    }

};

// For now, this needs to be a "flat" class since it will be directly serialized and passed to the group's registerEntity function
// TO DO: implement endian-swap capable serializer/deserializer
class ClusterGroupData
  {
public:
  unsigned int       structId;       // = 0x67839345
  unsigned int       nodeAddr;       // Same as slot id and TIPC address if these attributes exist 
  Handle             nodeMgtHandle;  // Identifier into the management db to identify this node's role (based on its name).  INVALID_HDL means that this node does not have a role (AMF can assign it one).
  char               nodeName[NODENAME_LEN];  // Identifier into the management db to identify this node's role. Name and MgtHandle must resolve to the same entity (this field is probably unnecessary, prefer Handle)
  boost::asio::ip::address backplaneIp; // TODO: verify that this is FLAT!!!  V4 or V6 intracluster IP address of this node.
  };

volatile bool    quitting=false;  // Set to true to tell all threads to quit
#ifdef USE_GRP
Group            clusterGroup;
#endif
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
boost::thread    compStatsRefresh;

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

struct CompStatsRefresh
  {
  void operator()()
    {

    Node* node=NULL;
    MgtObject::Iterator itnode;
    MgtObject::Iterator endnode = cfg.safplusAmf.nodeList.end();
    for (itnode = cfg.safplusAmf.nodeList.begin(); itnode != endnode; itnode++)
    {
      Node* tmp = dynamic_cast<Node*>(itnode->second);
      if (tmp->name == SAFplus::ASP_NODENAME)
        {
          node = tmp;
          break;
        }
    }
      

    if (node)  // If node is NULL, my own node is not modelled
      {
        node->stats.load.user.op=HistoryOperation::AVE;
        node->stats.load.lowPriorityUser.op=HistoryOperation::AVE;
        node->stats.load.ioWait.op=HistoryOperation::AVE;
        node->stats.load.sysTime.op=HistoryOperation::AVE;
        node->stats.load.intTime.op=HistoryOperation::AVE;
        node->stats.load.softIrqs.op=HistoryOperation::AVE;
        node->stats.load.idle.op=HistoryOperation::AVE;

        node->stats.load.contextSwitches.op=HistoryOperation::SUM;
        node->stats.load.processCount.op=HistoryOperation::AVE;
        node->stats.load.processStarts.op=HistoryOperation::SUM;
        //node->stats.load.runnableProcesses.op=HistoryOperation::AVE;
        //node->stats.load.blockedProcesses.op=HistoryOperation::AVE;
      }


      NodeStatistics prior;
      prior.read();
      while(!quitting)
        {
          float PollIntervalInSeconds = 10.0;
          boost::this_thread::sleep(boost::posix_time::milliseconds((int)PollIntervalInSeconds*1000));
          // logDebug("CMP","STT","Component Statistics Refresh");
          if (node)
            {
            NodeStatistics now;
            now.read();
            NodeStatistics difference = now-prior;
            difference.percentify();
            node->stats.upTime = now.sysUpTime;
            node->stats.bootTime = now.bootTime;
            node->stats.load.user.setValue((float)difference.timeSpentInUserMode/10.0);
            node->stats.load.lowPriorityUser.setValue((float)difference.timeLowPriorityUserMode/10.0);
            node->stats.load.ioWait.setValue((float)difference.timeIoWait/10.0);
            node->stats.load.sysTime.setValue((float)difference.timeSpentInSystemMode/10.0);
            node->stats.load.intTime.setValue((float)difference.timeServicingInterrupts/10.0);
            node->stats.load.softIrqs.setValue((float)difference.timeServicingSoftIrqs/10.0);
            node->stats.load.idle.setValue((float)difference.timeIdle/10.0);

            node->stats.load.contextSwitches.setValue((float)difference.numCtxtSwitches);
            node->stats.load.processCount.setValue(now.numProcesses);
            node->stats.load.processStarts.setValue((float)difference.cumulativeProcesses);
            prior = now;
            }

          MgtObject::Iterator it;
          for (it = cfg.safplusAmf.componentList.begin();it != cfg.safplusAmf.componentList.end(); it++)
            {
              Component* comp = dynamic_cast<Component*> (it->second);
              const std::string& cname = comp->name;

              if (comp->operState == true) 
                {
                  SAFplusAmf::ServiceUnit* su = comp->serviceUnit;
                  if (!su) continue;  // database is not valid (correctly formed) for this object
                  SAFplusAmf::Node* node = su->node;
                  if (!node) continue;  // database is not valid (correctly formed) for this object

                  pid = comp->processId;
                  if (pid > 1)
                    {
                      Handle hdl;
                      try
                        {
                          hdl = name.getHandle(comp->name);
                        }
                      catch (SAFplus::NameException& n)
                        {
                          continue;
                        }
                      if (hdl.getNode() == SAFplus::ASP_NODEADDR) // OK this component is local and on so gather stats on it.
                        {
                          try
                            {
                            SAFplus::ProcStats ps(pid);
                            comp->procStats.memUtilization.setValue(ps.virtualMemSize);
                            comp->procStats.numThreads.setValue(ps.numThreads);
                            comp->procStats.residentMem.setValue(ps.residentSetSize);
                            comp->procStats.pageFaults.setValue(ps.numMajorFaults);
                            }
                          catch(SAFplus::statAccessErrors &e)
                            {
                              // Should I write a zero to the stats or stop collecting?
                            }
                        }
                    }
                  //printf("Component [%s] stats.\n",cname.c_str());
                }

            }
        }
    }
};



bool activeAudit()  // Check to make sure DB and the system state are in sync.  Returns true if I need to rerun the AMF checking loop right away
  {
  bool rerun=false;
  logDebug("AUD","ACT","Active Audit");
  RedPolicyMap::iterator it;

  logDebug("AUD","ACT","Active Audit -- Nodes");

  Group::Iterator end = clusterGroup.end();
  for (Group::Iterator it = clusterGroup.begin();it != end; it++)
    {
    Handle hdl = it->first;  // same as gi->id
    const GroupIdentity* gi = &it->second;
    logInfo("AUD","NOD","Node handle [%" PRIx64 ":%" PRIx64 "], [%" PRIx64 ":%" PRIx64 "], ",hdl.id[0],hdl.id[1],gi->id.id[0],gi->id.id[1]);
#if 0
    ClusterGroupData* data = (ClusterGroupData*) it->second;
    if (data)
      {
      assert(data->structId == 0x67839345);  // TODO endian xform
      logInfo("AUD","NOD","Node [%s], handle [%" PRIx64 ":%" PRIx64 "] address [%d]",data->nodeName, hdl.id[0],hdl.id[1], data->nodeAddr);
      }
    else
      {
      logInfo("AUD","NOD","Node handle [%" PRIx64 ":%" PRIx64 "]",hdl.id[0],hdl.id[1]);
      }
#endif
    }
  logDebug("AUD","ACT","Active Audit -- Applications");
  for (it = redPolicies.begin(); it != redPolicies.end();it++)
    {
    ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*>(it->second->pluginApi);
    pp->activeAudit(&cfg);
    }

  return rerun;
  }

bool standbyAudit(void) // Check to make sure DB and the system state are in sync
  {
  logDebug("AUD","SBY","Standby Audit");

  logDebug("AUD","SBY","Standby Audit -- Nodes");
  
#if 0
  Group::Iterator end = clusterGroup.end();
  for (Group::Iterator it = clusterGroup.begin();it != end; it++)
    {
    Handle hdl = it->first;
    ClusterGroupData* data = (ClusterGroupData*) it->second;
    if (data)
      {
      assert(data->structId == 0x67839345);  // TODO endian xform
      logInfo("AUD","NOD","Node [%s], handle [%" PRIx64 ":%" PRIx64 "] address [%d]",data->nodeName, hdl.id[0],hdl.id[1], data->nodeAddr);
      }
    else
      {
      logInfo("AUD","NOD","Node handle [%" PRIx64 ":%" PRIx64 "]",hdl.id[0],hdl.id[1]);
      }
    }
#endif
  return false;
  }

void becomeActive(void)
  {
  activeAudit();
  }

void becomeStandby(void)
  {
  standbyAudit();
  }



void loadAmfPlugins(AmfOperations& amfOps,Fault& fault)
  {
  if (SAFplus::ASP_BINDIR[0]!=0)
    {
      std::string s(SAFplus::ASP_BINDIR);
      s.append("/../plugin");
      try
        {
        boost::filesystem::path p = boost::filesystem::canonical(s);
        loadAmfPluginsAt(p.c_str(),amfOps,fault);
        }
      catch (boost::filesystem::filesystem_error& e)
        {
          // directory does not exist
        }
    }
#if 0 // TODO: load user plugins from a relative directory, but we have to make sure that this directory is not the SAME as the safplus_amf plugin directory
  // pick the SAFplus directory or the current directory if it is not defined.
  const char * soPath = ".";
  if (boost::filesystem::is_directory("../plugin")) soPath = "../plugin";
  else if ((SAFplus::ASP_APP_BINDIR[0]!=0) && boost::filesystem::is_directory(SAFplus::ASP_APP_BINDIR))
      {
        soPath = SAFplus::ASP_APP_BINDIR;
      }
  else if (boost::filesystem::is_directory("../lib")) soPath = "../lib";

  loadAmfPluginsAt(soPath,amfOps,fault);


#endif

  }


void loadAmfPluginsAt(const char* soPath, AmfOperations& amfOps,Fault& fault)
  {  
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
          logInfo("POL","LOAD","Loading policy: %s", s);
          ClPluginHandle* plug = clLoadPlugin(CL_AMF_POLICY_PLUGIN_ID,CL_AMF_POLICY_PLUGIN_VER,s);
          if (plug)
            {
            ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*> (plug->pluginApi);
            if (pp)
              {
                bool result = pp->initialize(&amfOps,&fault);
              if (result)
                {
                redPolicies[pp->policyId] = plug;
                logInfo("POL","LOAD","AMF Policy [%d] plugin [%s] load succeeded.", ((int)pp->policyId), p.c_str());
                }
              else
                {
                logError("POL","LOAD","AMF Policy plugin [%s] load failed.  Plugin initialize error.", p.c_str());
                }
              }
            else logError("POL","LOAD","AMF Policy plugin [%s] load failed.  Incorrect plugin type.", p.c_str());
            }
          else logError("POL","LOAD","Policy [%s] load failed.  Incorrect plugin Identifier or version.", p.c_str());
          }

        }
      }
    }  
  }


void printUsage(char* progname)
  {
  printf("Usage: %s -c <Chassis ID> -l <Local Slot ID> -n <Node Name> \n", progname);
  printf("Example : %s -c 0 -l 1 -n node_0\n", progname);
  printf("or\n");
  printf("Example : %s --chassis=0 --localslot=1 --nodename=<nodeName>\n", progname);
  printf("Options:\n");
  printf("-c, --chassis=ID       Chassis ID\n");
  printf("-l, --localslot=ID     Local slot ID\n");
  printf("-n, --nodename=name    Node name\n");
  //clOsalPrintf("-f, --foreground       Run AMF as foreground process\n");
  printf("-h, --help             Display this help and exit\n");  
  }

static uint_t str2uint(const ClCharT *str) 
  {
  int i = 0;

  for (i = 0; str[i] != '\0'; ++i) 
    {
    if (!isdigit(str[i])) 
      {
      return UINT_MAX;
      }
    }

  return atoi(str);
  }

int parseArgs(int argc, char* argv[])
  {
  // NOTE: logs must use printf because this function is called before logs are initialized
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
          chassisId = str2uint(optarg);
          if (chassisId == UINT_MAX) 
            {
            printf("[%s] is not a valid chassis id.", optarg);
            return -1;
            }
          break;
        case 'l':
        {
        uint_t temp=0;
        temp = str2uint(optarg);
        if (temp == UINT_MAX)
          {
          printf("[%s] is not a valid slot id, ", optarg);
          return -1;
          }
        SAFplus::ASP_NODEADDR = temp;
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
          printf("Unknown option [%c]", optopt);
          return -1;
          break;
        default :   
          printf("Unknown error");
          return -1;
          break;
        }

  return 1;
  }


// Callback RPC client
void FooDone(StartComponentResponse* response)
  {
    std::cout << "DONE" << std::endl;
  }

// call in gdb to repair an entity
bool dbgRepair(const char* entity=NULL)
  {
//  MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
//  MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = cfg.safplusAmf.componentList.end();
  MgtObject::Iterator it;
  for (it = cfg.safplusAmf.componentList.begin();it != cfg.safplusAmf.componentList.end(); it++)
    {
    Component* comp = dynamic_cast<Component*> (it->second);
    const std::string& name = comp->name;

    //if (strcmp(entity, comp->name.c_str())==0)
    if ((entity==NULL) || (name == entity))
      {
      if (comp->operState == false) 
        {
        comp->operState = true;
        printf("Component [%s] repaired.\n",name.c_str());
        }
      else
        {
        if (entity != NULL) printf("Component [%s] does not need repair.\n",name.c_str());
        }
      }
    }
  }

bool dbgStart(const char* entity=NULL)
  {
  MgtObject::Iterator it;
  for (it = cfg.safplusAmf.serviceGroupList.begin();it != cfg.safplusAmf.serviceGroupList.end(); it++)
    {
    ServiceGroup* ent = dynamic_cast<ServiceGroup*> (it->second);
    const std::string& name = ent->name;

    if (name == entity)
      {
      ent->adminState.value = AdministrativeState::on;
      printf("Changed Service Group [%s] to [%s]\n",name.c_str(),c_str(ent->adminState.value));
      }
    }
  }

bool dbgStop(const char* entity=NULL)
  {
  MgtObject::Iterator it;
  for (it = cfg.safplusAmf.serviceGroupList.begin();it != cfg.safplusAmf.serviceGroupList.end(); it++)
    {
    ServiceGroup* ent = dynamic_cast<ServiceGroup*> (it->second);
    const std::string& name = ent->name;

    if (name == entity)
      {
      ent->adminState.value = AdministrativeState::off;
      printf("Changed Service Group [%s] to [%s]\n",name.c_str(),c_str(ent->adminState.value));
      }
    }
  }

bool dbgIdle(const char* entity=NULL)
  {
  MgtObject::Iterator it;
  for (it = cfg.safplusAmf.serviceGroupList.begin();it != cfg.safplusAmf.serviceGroupList.end(); it++)
    {
    ServiceGroup* ent = dynamic_cast<ServiceGroup*> (it->second);
    const std::string& name = ent->name;

    if (name == entity)
      {
      ent->adminState.value = AdministrativeState::idle;
      printf("Changed Service Group [%s] to [%s]\n",name.c_str(),c_str(ent->adminState.value));
      }
    }
  }


static ClRcT refreshComponentStats(void *unused)
{
    logInfo("STAT","COMP", "refresh component statistics");
    return CL_OK;
}

int main(int argc, char* argv[])
  {

    //uint64_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //uint64_t t2 = time(NULL);

  Mutex m;
  bool firstTime=true;
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logCompName = "AMF";

  if (parseArgs(argc,argv)<=0) return -1;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::AMF_IOC_PORT;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS;
  safplusInitialize( SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG, sic);
  //timerInitialize(NULL);
  logSeverity = LOG_SEV_DEBUG;

  // GAS DEBUG: Normally we would get these from the environment
  // if (SAFplus::ASP_NODENAME[0] == 0) strcpy(SAFplus::ASP_NODENAME,"sc0");  // TEMPORARY initialization
  assert(SAFplus::ASP_NODENAME);

  // Should be loaded from the environment during safplusInitialize.  But if it does not exist in the environment, override to true for the AMF rather then false which is default for non-existent variables.
  char* val = getenv("SAFPLUS_SYSTEM_CONTROLLER");
  if (val == NULL) // its not defined
    {
    SAFplus::SYSTEM_CONTROLLER = 1;      
    }

  if (SAFplus::ASP_BINDIR[0]==0) // Was not set
    {
      char spbinary[CL_MAX_NAME_LENGTH];
      int len = readlink("/proc/self/exe", spbinary, CL_MAX_NAME_LENGTH);  // will only work on linux
      spbinary[len] = 0;
      boost::filesystem::path canonicalPath = boost::filesystem::canonical(spbinary).parent_path();
      assert(strlen(canonicalPath.c_str())<CL_MAX_NAME_LENGTH);
      strncpy(SAFplus::ASP_BINDIR, canonicalPath.c_str(),CL_MAX_NAME_LENGTH);
    }

  logAlert(LogArea,"INI","Welcome to OpenClovis SAFplus version %d.%d.%d %s %s running from %s", SAFplus::VersionMajor, SAFplus::VersionMinor, SAFplus::VersionBugfix, __DATE__, __TIME__,SAFplus::ASP_BINDIR);

  //SAFplus::safplusMsgServer.init(SAFplusI::AMF_IOC_PORT, MAX_MSGS, MAX_HANDLER_THREADS);

  myHandle = getProcessHandle(SAFplusI::AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
  // Register a mapping between this node's name and its handle.
  nodeHandle = myHandle; // TODO: No should be different

  // Start up Server RPC.  Note that since I create some variables on the stack,
  // we must shut this down before going out of context.
  SAFplus::Rpc::amfRpc::amfRpcImpl amfRpcMsgHandler;
  SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, myHandle);
  channel->setMsgType(AMF_REQ_HANDLER_TYPE, AMF_REPLY_HANDLER_TYPE);
  channel->service = &amfRpcMsgHandler;


#ifdef SAFPLUS_AMF_GRP_NODE_REPRESENTATIVE
  SAFplusI::GroupServer gs;  // WARN: may not be initialized if safplus_group is running
  try
    {
    gs.init();
    }
  catch(SAFplus::Error& e)
    {
      if (e.clError == SAFplus::Error::EXISTS)
        {
        logAlert(LogArea,"INI", "Group server is not being run from within the AMF because it already exists.");
        }
      else
        {
          throw;
        }
    }
#endif

#ifdef SAFPLUS_AMF_FAULT_NODE_REPRESENTATIVE
  SAFplus::FaultServer fs;
  fs.init();
#endif

  nameInitialize();  // Name must be initialized after the group server 

  // client side
  amfRpc_Stub amfInternalRpc(channel);


#if 0  // quick little test of RPC
  StartComponentRequest req;
  req.set_name("c0");
  req.set_command("./c0 test1 test2");
  StartComponentResponse resp;
  //client side should using callback
  //google::protobuf::Closure *callback = google::protobuf::NewCallback(&FooDone, &resp);
  SAFplus::Handle hdl(TransientHandle,1,SAFplusI::AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
  //service.startComponent(hdl,&req, &resp);
#endif
  //sleep(10000);


  logInfo(LogArea,"NAM", "Registering this node [%s] as handle [%" PRIx64 ":%" PRIx64 "]", SAFplus::ASP_NODENAME, myHandle.id[0],myHandle.id[1]);
  name.set(SAFplus::ASP_NODENAME,nodeHandle,NameRegistrar::MODE_NO_CHANGE);

  /* Initialize mgt database  */
  MgtDatabase *db = MgtDatabase::getInstance();
  logInfo(LogArea,"DB", "Opening database file [%s]", "safplusAmf");
  db->initializeDB("safplusAmf");
  cfg.read(db);
  initializeOperationalValues(cfg);
  // TEMPORARY testing initialization
  //loadAmfConfig(&cfg);
  //cfg.safplusAmf.bind(myHandle)

  cfg.bind(myHandle,&cfg.safplusAmf);


  logServer = boost::thread(LogServer());

  AmfOperations amfOps;  // Must happen after messaging initialization so that we can use the node address and message port in the invocation.
  amfOps.amfInternalRpc = &amfInternalRpc;

  // Set up the RPC communication to applications
  SAFplus::Rpc::amfAppRpc::amfAppRpcImplAmfSide amfAppRpcMsgHandler(&amfOps);
  SAFplus::Rpc::RpcChannel appRpcChannel(&safplusMsgServer, myHandle);
  appRpcChannel.setMsgType(AMF_APP_REQ_HANDLER_TYPE, AMF_APP_REPLY_HANDLER_TYPE);
  appRpcChannel.service = &amfAppRpcMsgHandler;  // The AMF needs to receive saAmfResponse calls from the clients so it needs to act as a "server".
  SAFplus::Rpc::amfAppRpc::amfAppRpc_Stub amfAppRpc(&appRpcChannel);

  amfOps.amfAppRpc = &amfAppRpc;

  loadAmfPlugins(amfOps,fault);

#ifdef USE_GRP
  clusterGroup.init(CLUSTER_GROUP,"safplusCluster");
  clusterGroup.setNotification(somethingChanged);
#endif

  // Start essential SAFplus services

  // Once SAFplus is up, we can become a full member of the group.  The basic credential is the node's address.  
  // TODO: implement an environment variable to override this basic credential so users can control which node becomes master.

  if (SAFplus::SYSTEM_CONTROLLER) 
    {
    credential = SAFplus::ASP_NODEADDR | SC_ELECTION_BIT;
    capabilities = Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY;
    }
  else if (SAFplus::ASP_SC_PROMOTE) // A promotable payload can only become standby at first.
    {
    credential =  SAFplus::ASP_NODEADDR;
    capabilities = Group::ACCEPT_STANDBY | Group::STICKY;
    }
  else  // Don't elect me!!!!
    {
    credential = 0;
    capabilities = 0;
    }

#ifdef USE_GRP
  clusterGroupData.structId = 0x67839345;
  clusterGroupData.nodeAddr = SAFplus::ASP_NODEADDR;
  clusterGroupData.nodeMgtHandle = INVALID_HDL;
  strncpy(clusterGroupData.nodeName,SAFplus::ASP_NODENAME,NODENAME_LEN);
  // TODO: clusterGroupData.backplaneIp = 
  clusterGroup.registerEntity(myHandle, credential, (void*) &clusterGroupData, sizeof(ClusterGroupData),capabilities);
//  MgtFunction::registerEntity(myHandle);
#endif
  logInfo("AMF","HDL", "I AM [%" PRIx64 ":%" PRIx64 "]", myHandle.id[0],myHandle.id[1]);

  std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs;
#ifdef USE_GRP
  activeStandbyPairs = clusterGroup.getRoles();
  logInfo("AMF","BOOT", "Active [%" PRIx64 ":%" PRIx64 "] Standby: [%" PRIx64 ":%" PRIx64 "]", activeStandbyPairs.first.id[0],activeStandbyPairs.first.id[1],activeStandbyPairs.second.id[0],activeStandbyPairs.second.id[1]);
  //activeStandbyPairs.first = clusterGroup.getActive();
  //activeStandbyPairs.second = clusterGroup.getStandby();
#endif
  
#if 0  // No need to call for election, elections occur automatically
  if (activeStandbyPairs.first == INVALID_HDL)  // If nobody is active, I need to call for an election
    {
    // By waiting, other nodes that are booting can come up.  This makes the system more consistently elect a particular node as ACTIVE when the cluster is started.  Note that this is just convenient for users, it does not matter to the system which node is elected active.
    // Commented out because the election has a 10 second delay
    // boost::this_thread::sleep(boost::posix_time::milliseconds(STARTUP_ELECTION_DELAY_MS));
#ifdef USE_GRP
    activeStandbyPairs = clusterGroup.elect();
    // GAS TODO:  What errors can be returned?
#else
    activeStandbyPairs.first = myHandle;
    firstTime = false;
#endif
    }
#endif

  MgtMsgHandler msghandle;
  SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
  mgtIocInstance->registerHandler(SAFplusI::CL_MGT_MSG_TYPE,&msghandle,NULL);

  fault.init(myHandle);
  fault.registerEntity(nodeHandle,FaultState::STATE_UP);

  struct sigaction newAction;
  newAction.sa_handler = sigChildHandler;
  sigemptyset(&newAction.sa_mask);
  newAction.sa_flags = SA_RESTART | SA_NOCLDSTOP;

  if (-1 == sigaction(SIGCHLD, &newAction, NULL))
    {
        perror("sigaction for SIGCHLD failed");
        logError("MAN","INI", "Unable to install signal handler for SIGCHLD");
        assert(0);
    }


    //boost::asio::io_service ioSvc;
  // Construct a signal set registered for process termination.
  //boost::asio::signal_set signals(ioSvc, SIGCHLD);
  // Start an asynchronous wait for one of the signals to occur.
  //signals.async_wait(signalHandler);

  //static ClTimerTimeOutT statsTimeout = { .tsSec = 10, .tsMilliSec = 0 };
  //Timer readStats(statsTimeout, CL_TIMER_REPETITIVE,CL_TIMER_SEPARATE_CONTEXT,refreshComponentStats,NULL);
  //readStats.timerStart();
  compStatsRefresh = boost::thread(CompStatsRefresh());
  

  while(!quitting)  // Active/Standby transition loop
    {
    ScopedLock<> lock(m);
    bool changeTriggered = true;
    if (!firstTime && !amfOps.changed) changeTriggered = somethingChanged.timed_wait(m,REEVALUATION_DELAY);
    amfChange |= amfOps.changed;
    amfOps.changed = false;  // reset the amf operations change marker

    if (amfChange || !changeTriggered)  // Evaluate AMF if timed out or if the triggered change was AMF related
      {  // Timeout
      logDebug("IDL","---","...waiting for something to happen...");
      int pid;
      do
        {
        int status = 0;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid>0)
          {
          logWarning("PRC","MON","On this node, child process [%d] has failed", pid);
          }
        if (pid<0)
          {
          // ECHILD means no child processes forked yet.
          if (errno != ECHILD) logWarning("PRC","MON","waitpid errno [%s (%d)]", strerror(errno), errno);
          }

        // TODO: find this pid in the component database and fail it.  This will be faster.  For now, do nothing because the periodic pid checker should catch it.
        // We need to use the periodic pid checker because a component may not be a child of safplus_amf
        } while(pid > 0);
      amfChange = false;
      if (myRole == Group::IS_ACTIVE) amfChange |= activeAudit();    // Check to make sure DB and the system state are in sync
      if (myRole == Group::IS_STANDBY) amfChange |= standbyAudit();  // Check to make sure DB and the system state are in sync

      // GAS DBG: just to test audit with election not working
      // activeAudit();
      }
    else
      {  // Something changed in the group.
      firstTime=false;
#ifdef USE_GRP
      activeStandbyPairs = clusterGroup.getRoles();
      assert((activeStandbyPairs.first != activeStandbyPairs.second) || ((activeStandbyPairs.first == INVALID_HDL)&&(activeStandbyPairs.second == INVALID_HDL)) );
      // now election occurs automatically, so just need to wait for it.
      // if ((activeStandbyPairs.first == INVALID_HDL)||(activeStandbyPairs.second == INVALID_HDL)) clusterGroup.elect();
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

  //fs.notify(nodeHandle,AlarmStateT::ALARM_STATE_ASSERT, AlarmCategoryTypeT::ALARM_CATEGORY_EQUIPMENT,...);
  fault.registerEntity(nodeHandle,FaultState::STATE_DOWN);
  }


void initializeOperationalValues(SAFplusAmf::SAFplusAmfModule& cfg)
{
#if 0
  for (it = cfg->serviceGroupList.begin();it != cfg->serviceGroupList.end(); it++)
    {
      startSg=false;
      ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
      const std::string& name = sg->name;
    }
#endif

  MgtObject::Iterator itsu;
  MgtObject::Iterator endsu = cfg.safplusAmf.serviceUnitList.end();

  for (itsu = cfg.safplusAmf.serviceUnitList.begin(); itsu != endsu; itsu++)
    {
      ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
      //const std::string& suName = su->name;

      su->numActiveServiceInstances.current.value=0;
      su->numStandbyServiceInstances.current.value=0;
      su->restartCount.current.value=0;

      su->operState = true; // Not faulted: We can try to turn this on.
      su->readinessState = ReadinessState::outOfService;
      su->presenceState = PresenceState::uninstantiated;
      su->haReadinessState = HighAvailabilityReadinessState::notReadyForAssignment; // This will be modified to reflect the cumulative state of the components
      su->haState = HighAvailabilityState::idle;
    }

  MgtObject::Iterator itcomp;
  MgtObject::Iterator endcomp = cfg.safplusAmf.componentList.end();
  for (itcomp = cfg.safplusAmf.componentList.begin(); itcomp != endcomp; itcomp++)
    {
      Component* comp = dynamic_cast<Component*>(itcomp->second);
      comp->activeAssignments = 0; 
      comp->standbyAssignments = 0; 

      comp->operState = true;  // Not faulted: We can try to turn this on.
      comp->readinessState = ReadinessState::outOfService;
      comp->presenceState = PresenceState::uninstantiated;

      comp->haReadinessState = HighAvailabilityReadinessState::readyForAssignment;
      comp->haState = HighAvailabilityState::idle;

      comp->numInstantiationAttempts = 0;
      comp->lastInstantiation.value.value = 0;
      comp->processId = 0;
      comp->pendingOperation =  PendingOperation::none;
      comp->pendingOperationExpiration.value.value = 0;
      comp->restartCount = 0;
    }

  MgtObject::Iterator itnode;
  MgtObject::Iterator endnode = cfg.safplusAmf.nodeList.end();
  for (itnode = cfg.safplusAmf.nodeList.begin(); itnode != endnode; itnode++)
    {
      Node* node = dynamic_cast<Node*>(itnode->second);

      node->operState = true;  // Not faulted: We can try to turn this on.
    }

  MgtObject::Iterator itsi;
  MgtObject::Iterator endsi = cfg.safplusAmf.serviceInstanceList.end();
  for (itsi = cfg.safplusAmf.serviceInstanceList.begin(); itsi != endsi; itsi++)
    {
      ServiceInstance* elem = dynamic_cast<ServiceInstance*>(itsi->second);

      elem->assignmentState = AssignmentState::unassigned;  
      elem->getNumActiveAssignments()->current.value = 0;
      elem->getNumStandbyAssignments()->current.value = 0;
    }

  if (1)
    {
      MgtObject::Iterator it;
      MgtObject::Iterator endit = cfg.safplusAmf.componentServiceInstanceList.end();
      for (it = cfg.safplusAmf.componentServiceInstanceList.begin(); it != endit; it++)
        {
          ComponentServiceInstance* elem = dynamic_cast<ComponentServiceInstance*>(it->second);
          // Nothing to initialize
        }
    }



}
