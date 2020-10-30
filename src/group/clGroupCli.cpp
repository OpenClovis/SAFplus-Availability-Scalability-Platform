//#include <boost/program_options.hpp>
//#include <boost/interprocess/exceptions.hpp>
//#include <boost/foreach.hpp>
//#include <boost/tokenizer.hpp>
#include <clGroupCli.hxx>
#include <clNameApi.hxx>
#include <string.h>
#define SC_SELECTION_BIT 1<<8

/* The function will get the view of cluster in the format: 

NodeName    NodeType       HAState  NodeAddr
Node0       controller     Active   108
Node1       controller     Standby  109
Node2       Payload        -        110

*/

using namespace SAFplusI;
namespace SAFplus {

std::string getClusterView()
{   
#if 0
  SafplusInitializationConfiguration sic;
  sic.iocPort     = 35;  // TODO: auto assign port
  sic.msgQueueLen = 10;
  sic.msgThreads  = 1;

  safplusInitialize( SAFplus::LibDep::NAME | SAFplus::LibDep::LOCAL,sic);
#endif
  SAFplusI::GroupSharedMem gsm;
  gsm.init();
  char buf[100];
  GroupShmHashMap::iterator i;
  assert(gsm.groupMap);
  //ScopedLock<ProcSem> lock(mutex);
  std::string ret;
  for (i=gsm.groupMap->begin(); i!=gsm.groupMap->end();i++)
  {
    SAFplus::Handle grpHdl = i->first;
    GroupShmEntry& ge = i->second;
    if (strcmp(ge.name, "safplusCluster")==0)
    {
      std::pair<EntityIdentifier,EntityIdentifier> roles(INVALID_HDL,INVALID_HDL);
      const GroupData& gd = ge.read();
      roles = ge.getRoles();
      EntityIdentifier active = roles.first;
      EntityIdentifier standby = roles.second;      
      printf("active [%" PRIx64 ":%" PRIx64 "]; standby [%" PRIx64 ":%" PRIx64 "]\n", active.id[0], active.id[1], standby.id[0], standby.id[1]);
      printf("Group %s [%" PRIx64 ":%" PRIx64 "]:\n", ge.name,grpHdl.id[0], grpHdl.id[1]);
      for (int j = 0 ; j<gd.numMembers; j++)
      {
        const GroupIdentity& gid = gd.members[j];
        printf("    Entity [%" PRIx64 ":%" PRIx64 "] on node [%d] credentials [%ld] capabilities [%d] %s\n", gid.id.id[0],gid.id.id[1],gid.id.getNode(),(unsigned long int) gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
        try {
          char node[256], nodeType[50], haState[20], nodeAddr[15];
          char* nodeName = name.getName(getNodeHandle(gid.id.getNode()));
          //snprintf(node,255,"%15s",nodeName);
          ret.append(nodeName);
          
          if (gid.credentials & SC_SELECTION_BIT) snprintf(nodeType,49,"%15s","controller");//strcpy(nodeType,"  controller");          
          else snprintf(nodeType,49,"%15s","payload");//strcpy(nodeType,"payload");
          ret.append(nodeType);
          if (active.getNode() == gid.id.getNode()) snprintf(haState,19,"%10s","active");//strcpy(haState,"  active");
          else if (standby.getNode() == gid.id.getNode()) snprintf(haState,19,"%10s","standby");//strcpy(haState,"  standby");
          else snprintf(haState,19,"%10s","-");//strcpy(haState,"  -"); //payload doesn't have ha state
          ret.append(haState);
          snprintf(nodeAddr,14, "  %5d\n", gid.id.getNode());
          ret.append(nodeAddr);
        }
        catch(NameException& e)
        {
          printf("Name exception for handle [%" PRIx64 ":%" PRIx64 "]: %s\n", gid.id.id[0],gid.id.id[1],e.what());
        } 
      }  
    }
  }
  return ret;
}

}
