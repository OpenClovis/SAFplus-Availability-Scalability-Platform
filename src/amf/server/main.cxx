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

#include <amfMgmtRpc.hxx>

#include "nodeMonitor.hxx"

#define USE_GRP

#ifdef SAFPLUS_AMF_GRP_NODE_REPRESENTATIVE
#include <clGroupIpi.hxx>
#endif

#ifdef SAFPLUS_AMF_LOG_NODE_REPRESENTATIVE
#include "../../log/clLogIpi.hxx"
#endif

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

typedef boost::unordered_map<SAFplus::AmfRedundancyPolicy,ClPluginHandle*> RedPolicyMap;

// namespace SAFplusAmf { void loadAmfConfig(SAFplusAmfModule* self); };

void initializeOperationalValues(SAFplusAmf::SAFplusAmfModule& cfg);
void loadAmfPluginsAt(const char* soPath, AmfOperations& amfOps,Fault& fault);
void postProcessing();
void updateNodesFaultState(SAFplusAmf::SAFplusAmfModule& cfg);

RedPolicyMap redPolicies;

// IOC related globals
extern ClUint32T chassisId;

// signal this condition to execute a cluster state reevaluation early
ThreadCondition somethingChanged;
Mutex           somethingChangedMutex;
bool amfChange = false;  //? Set to true if the change was AMF related (process started/died or other AMF state change)
bool grpChange = false;  //? Set to true if the change was group related

SAFplusAmf::SAFplusAmfModule cfg;
//MgtModule dataModule("SAFplusAmf");

const char* LogArea = "MAN";
MgtDatabase amfDb;
MgtDatabase logDb;
SAFplus::Fault gfault;  // Fault client global variable
SAFplus::FaultServer fs;
bool initOperValues = false;

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

NodeMonitor      nodeMonitor;

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

// Threads
boost::thread    logServer;
boost::thread    compStatsRefresh;
extern bool rebootFlag;

static void quitSignalHandler(int signum)
{
    amfChange = true;
    quitting = true;
    somethingChanged.notify_all();
}

struct LogServer
  {
  void operator()()
    {
#ifdef SAFPLUS_AMF_LOG_NODE_REPRESENTATIVE
    logDb.initialize("safplusLog");
    logServerInitialize(&logDb);

    while(!quitting)
      {
      logServerProcessing();
      }
    logDb.finalize();
#endif
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

  logDebug("AUD","ACT","Active Audit: Nodes");


  MgtObject::Iterator itnode;
  MgtObject::Iterator endnode = cfg.safplusAmf.nodeList.end();
  int provNodeCount = 0;
  for (itnode = cfg.safplusAmf.nodeList.begin(); itnode != endnode; itnode++,provNodeCount++)
    {
      Handle nodeHdl;
      SAFplus::FaultState fs;
      Node* node = dynamic_cast<Node*>(itnode->second);
      try
        {
         nodeHdl = name.getHandle(node->name);
         fs = gfault.getFaultState(nodeHdl);
         bool inClusterGroup = clusterGroup.isMember(getProcessHandle(nodeHdl.getNode(),SAFplusI::AMF_IOC_PORT));
#if 0  // Can happen when first starting up...
         if ((!inClusterGroup)&&(fs != FaultState::STATE_DOWN)) // stale data in the fault manager... likely from restarting safplus_amf
           {
           gfault.notify(nodeHdl,AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS,AlarmSeverity::ALARM_SEVERITY_MAJOR,AlarmProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE);
           }
#endif

         if ((fs == FaultState::STATE_UP)&&(node->presenceState != PresenceState::instantiated))
           {
             node->presenceState = PresenceState::instantiated;
             // TODO: set change flag
           }
         else if ((fs == FaultState::STATE_DOWN)&&(node->presenceState != PresenceState::terminating))
           {
             node->presenceState = PresenceState::uninstantiated;
           }
        }
      catch (SAFplus::NameException& n)
        {
          node->presenceState.value = PresenceState::uninstantiated;
        }
      if (nodeHdl != INVALID_HDL)
        {
        Handle amfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,nodeHdl.getNode());
        bool inCluster = clusterGroup.isMember(amfHdl);
        logInfo("AUD","NOD","Node [%s] Id [%d] Handle [%" PRIx64 ":%" PRIx64 "] Fault: [%s] Member: [%c]",node->name.value.c_str(), nodeHdl.getNode(), nodeHdl.id[0],nodeHdl.id[1],c_str(fs), inCluster ? 'Y':'N');
        }
      else
        {
          logInfo("AUD","NOD","Node [%s] is uninstantiated",node->name.value.c_str());
        }
    }
  if (provNodeCount==0) 
    {
    logError("AUD","ACT","No nodes are defined in the model");
    }


  Group::Iterator end = clusterGroup.end();
  for (Group::Iterator it = clusterGroup.begin();it != end; it++)
    {
    Handle hdl = it->first;  // same as gi->id
    const GroupIdentity* gi = &it->second;
    //logInfo("AUD","NOD","Node handle [%" PRIx64 ":%" PRIx64 "], [%" PRIx64 ":%" PRIx64 "], ",hdl.id[0],hdl.id[1],gi->id.id[0],gi->id.id[1]);
#if 0
    NodeInfoRequest request;
    NodeInfoResponse response;
    request->set_time(nowMs());
    amfInternalRpc->nodeInfo(hdl,&req,&resp);
    if (!resp.IsInitialized())
          {
            // RPC call is broken, should throw exception
            assert(0);
          }    
#endif
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
  logDebug("AUD","ACT","Active Audit -- Redundancy Policies");
  int policyCount=0;
  for (it = redPolicies.begin(); it != redPolicies.end();it++)
    {
    ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*>(it->second->pluginApi);
    pp->activeAudit(&cfg);
    policyCount++;
    }
  if (policyCount == 0)
    {
    logWarning("AUD","ACT","No policies loaded -- no applications can run");
    }
  else
    {
      logDebug("AUD","ACT","Audited %d redundancy policies", policyCount);
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
  nodeMonitor.becomeActive();

  cfg.read(&amfDb);
  initializeOperationalValues(cfg);
  updateNodesFaultState(cfg);

  cfg.bind(myHandle,&cfg.safplusAmf);
  activeAudit();
}

void becomeStandby(void)
  {
    nodeMonitor.becomeStandby();
    // TODO bind cfg.bind(myHandle,&cfg.safplusAmf); to a different prefix (standbyAmf?)
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
    fault.loadFaultPolicyEnv();
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
 
const std::string dbName("safplusAmf");
SafplusInitializationConfiguration sic;

int main(int argc, char* argv[])
  {

    //uint64_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //uint64_t t2 = time(NULL);

    //Mutex m;
  bool firstTime=true;
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logCompName = "AMF";
  rebootFlag=false;
  sic.iocPort     = SAFplusI::AMF_IOC_PORT;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS;
  logSeverity     = LOG_SEV_DEBUG;
  safplusInitialize( SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG, sic);
  logSeverity     = LOG_SEV_DEBUG;

  assert(SAFplus::ASP_NODENAME);

  // Should be loaded from the environment during safplusInitialize.  But if it does not exist in the environment, override to true for the AMF rather then false which is default for non-existent variables.
  SAFplus::SYSTEM_CONTROLLER = parseEnvBoolean("SAFPLUS_SYSTEM_CONTROLLER",true);

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

  myHandle = getProcessHandle(SAFplusI::AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
  // Register a mapping between this node's name and its handle.
  nodeHandle = getNodeHandle(SAFplus::ASP_NODEADDR);

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
  //SAFplus::FaultServer fs;
  fs.init(&gs);
#endif

  nameInitialize();  // Name must be initialized after the group server 

  // Mgt must be inited after name if you are using Checkpoint DB
  amfDb.pluginFlags = Checkpoint::REPLICATED;
  amfDb.initialize(dbName);
  cfg.setDatabase(&amfDb);
  /* Initialize mgt database  */
  SaTimeT healthCheckMaxSilence = (SaTimeT)1500;
  cfg.safplusAmf.setHealthCheckMaxSilence(healthCheckMaxSilence);
  DbalPlugin* plugin = amfDb.getPlugin();
  logInfo(LogArea,"DB", "Opening database file [%s] using plugin [%s]", dbName.c_str(),plugin->type);

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

  //--------setup rpc for amfMgmtApi-------------------
  SAFplus::Rpc::amfMgmtRpc::amfMgmtRpcImpl mgmtRpc;
  SAFplus::Rpc::RpcChannel amfMgmtRpcChannel(&safplusMsgServer, myHandle);
  amfMgmtRpcChannel.setMsgType(AMF_MGMT_REQ_HANDLER_TYPE, AMF_MGMT_REPLY_HANDLER_TYPE);
  amfMgmtRpcChannel.service = &mgmtRpc;  // The AMF needs to receive saAmfResponse calls from the clients so it needs to act as a "server".
  SAFplus::Rpc::amfMgmtRpc::amfMgmtRpc_Stub amfMgmtRpc(&amfMgmtRpcChannel);

  //---------------------------------------------------

  loadAmfPlugins(amfOps,gfault);

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

  gfault.init(myHandle);

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

  newAction.sa_handler = quitSignalHandler;
  sigemptyset(&newAction.sa_mask);
  newAction.sa_flags = 0;

  if (-1 == sigaction(SIGTERM, &newAction, NULL))
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

  logInfo(LogArea,"NAM", "Registering this node [%s] as handle [%" PRIx64 ":%" PRIx64 "]", SAFplus::ASP_NODENAME, nodeHandle.id[0],nodeHandle.id[1]);
  name.set(SAFplus::ASP_NODENAME,nodeHandle,NameRegistrar::MODE_NO_CHANGE,true);

  compStatsRefresh = boost::thread(CompStatsRefresh());

  nodeMonitor.initialize();  // Initialize node monitor early so this node replies to heartbeats

  // Ready to go.  Claim that I am up, and wait until the fault manager commits that change.  We must wait so we don't see ourselves as down!
  // And because we set ourselves up only ONCE.  Beyond this point, the fault manager is authorative -- if it reports us down, the AMF should quit.
  // TODO: add generations to the fault manager, to distinguish between a prior run of the AMF/node, or other entities
  do {  // Loop because active fault manager may not be chosen yet
    gfault.registerEntity(nodeHandle, FaultState::STATE_UP);  // set this node as up
    boost::this_thread::sleep(boost::posix_time::milliseconds(250));    
  } while(gfault.getFaultState(nodeHandle) != FaultState::STATE_UP);

  do {
    gfault.registerEntity(myHandle, FaultState::STATE_UP);    // set this AMF as up
    boost::this_thread::sleep(boost::posix_time::milliseconds(250));    
  } while(gfault.getFaultState(myHandle) != FaultState::STATE_UP);

  uint64_t lastBeat = beat; 
  uint64_t nowBeat;

  while(!quitting)  // Active/Standby transition loop
    {
    ScopedLock<> lock(somethingChangedMutex);
    bool changeTriggered = true;
    nowBeat = beat;
    if (lastBeat != nowBeat)
      {
      logInfo("PRC","MON","Beat advanced, writing changes");
      cfg.writeChanged(lastBeat,nowBeat,&amfDb);
      }
    lastBeat = nowBeat;
    beat++;
    if (!firstTime && !amfOps.changed) changeTriggered = somethingChanged.timed_wait(somethingChangedMutex,REEVALUATION_DELAY);

    if (gfault.getFaultState(nodeHandle) != FaultState::STATE_UP)  // Since the fault manager is authoritative, quit it if marks this node as down.
      {
        logCritical("PRC","MON","Fault manager has marked this node as DOWN.  This could indicate a real issue with the node, or be a false positive due to missed heartbeat messages, etc.  Quitting.");
        quitting = true;
        break;
      }

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
            std::ifstream file(boost::lexical_cast<std::string>(pid) + ".error");
            std::string error;
            std::getline(file,error);
            logWarning("PRC","MON","On this node, child process [%d] has failed with status [%d], it indicated: %s", pid, status,error.c_str());
            MgtObject::Iterator itcomp;
            MgtObject::Iterator endcomp = cfg.safplusAmf.componentList.end();
            for (itcomp = cfg.safplusAmf.componentList.begin(); itcomp != endcomp; itcomp++)
              {
                Component* comp = dynamic_cast<Component*>(itcomp->second);
                if ((comp->processId == pid)&&(comp->getServiceUnit()->getNode()->name.value == ASP_NODENAME))
                  {
                  comp->lastError = error;
                  break;
                  }
              }            
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
          if (myRole == 0)// if my previous HA state is NOT STANDBY or ACTIVE, do init operational values
            {
            initOperValues = true;
            }
          logInfo("---","---","This node just became the active system controller");
          myRole = Group::IS_ACTIVE;
          becomeActive();
          name.set(AMF_MASTER_HANDLE,myHandle,NameRegistrar::MODE_NO_CHANGE);
          lastBeat = beat;  // Don't rewrite the changes that loading makes          
          }
        }
      if (myRole != Group::IS_STANDBY)
        {
        if (activeStandbyPairs.second == myHandle)
          {
          CL_ASSERT(myRole != Group::IS_ACTIVE);  // Fall back from active to standby is not allowed
          logInfo("---","---","This node just became standby system controller");
          initOperValues = false;
          myRole = Group::IS_STANDBY;
          becomeStandby();
          }
        }
      }
    }

  //fs.notify(nodeHandle,AlarmStateT::ALARM_STATE_ASSERT, AlarmCategoryTypeT::ALARM_CATEGORY_EQUIPMENT,...);
  gfault.registerEntity(nodeHandle,FaultState::STATE_DOWN);
  nodeMonitor.finalize();
  compStatsRefresh.join();
  amfDb.finalize();
  safplusFinalize();
  postProcessing();
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
  if (!initOperValues) {
     logInfo("MAIN","INIT","This is not the first time node started. The next time, these will be loaded from database");
     return;
  }
  logInfo("MAIN","INIT","intializing default values");
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

void postProcessing()
{
	if(rebootFlag)
	{
		ofstream rebootFile;
		rebootFile.open("safplus_reboot");
		rebootFile.close();
	}
}

void updateNodesFaultState(SAFplusAmf::SAFplusAmfModule& cfg)
{
  MgtObject::Iterator itnode;
  MgtObject::Iterator endnode = cfg.safplusAmf.nodeList.end();
  for (itnode = cfg.safplusAmf.nodeList.begin(); itnode != endnode; itnode++)
    {
      Node* node = dynamic_cast<Node*>(itnode->second);
      //node->operState = true;  // Not faulted: We can try to turn this on.
      if (node->presenceState.value == PresenceState::instantiated)
      {
        logDebug(LogArea,"INI","presenceState of node [%s] is instantiated. Update its fault state for consistency", node->name.value.c_str());
        Handle nodeHdl = INVALID_HDL;
        try
        {
          nodeHdl = name.getHandle(node->name);
        }
        catch(NameException& ne)
        {
          logInfo(LogArea,"INI", "[%s]",ne.what());
        }
        if (nodeHdl != INVALID_HDL)
        {
          fs.setFaultState(nodeHdl, FaultState::STATE_UP);
        }
      }
    }
}

