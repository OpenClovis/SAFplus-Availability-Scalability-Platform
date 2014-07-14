// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clIocApi.h>
#include <clGroup.hxx>
#include <clGlobals.hxx>

using namespace SAFplus;

ClBoolT   gIsNodeRepresentative = CL_FALSE;
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;
unsigned int port = 50;

void printUsage(char* progname)
  {
  printf("Usage: %s -c <Chassis ID> -l <Local Slot ID> -n <Node Name> \n", progname);
  printf("Example : %s -c 0 -l 1 -n node_0\n", progname);
  printf("or\n");
  printf("Example : %s --chassis=0 --localslot=1 --nodename=<nodeName>\n", progname);
  printf("Options:\n");
  printf("-c, --chassis=ID       Chassis ID\n");
  printf("-l, --localslot=ID     Local slot ID\n");
  printf("-p, --port=portNum     IOC port to use\n");
  //printf("-f, --foreground       Run AMF as foreground process\n");
  printf("-h, --help             Display this help and exit\n");  
  }

int parseArgs(int argc, char* argv[])
  {
  ClInt32T option = 0;
  ClInt32T nargs = 0;
  ClRcT rc = CL_OK;
  const ClCharT *short_options = ":l:m:n:p:r:fh";

  const struct option long_options[] = {
    {"localslot",   1, NULL, 'l'},
    {"nodename",    1, NULL, 'n'},
    {"port",     1, NULL, 'p'},
    {"foreground",  0, NULL, 'f'},
    {"help",        0, NULL, 'h'},
    {"rep",         0, NULL, 'r'},
    { NULL,         0, NULL,  0 }
    };

  while((option = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
    {
      switch(option)
        {
        case 'h':   
          printUsage(argv[0]);
          /* This is not an error */
          exit(0);
        case 'p':   
          port = atoi(optarg);
          break;
        case 'r':
          gIsNodeRepresentative = CL_TRUE;
          break;
        case 'l':
        {
        SAFplus::ASP_NODEADDR = atoi(optarg);
        ++nargs;
        } break;
        case '?':
          logError("TMG","BOOT", "Unknown option [%c]", optopt);
          return -1;
          break;
        default :
          logError("TMG","BOOT", "Unknown error");
          return -1;
          break;
        }
    }

  return 1;
  }

int main(int argc, char* argv[])
{
  int tc = -1;
  int mode = SAFplus::Group::DATA_IN_MEMORY;
  SAFplus::ASP_NODEADDR = 1;
  parseArgs(argc,argv);

  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  utilsInitialize();

  ClRcT rc;
  // initialize SAFplus6 libraries 
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
    {
    assert(0);
    }

  rc = clIocLibInitialize(NULL);
  assert(rc==CL_OK);

  safplusMsgServer.init(port, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  //Handle replicaHandle = Handle::create(safplusMsgServer.handle.getPort());

  Handle grphandle = WellKnownHandle(64<<SUB_HDL_SHIFT,0,0);
  Handle me = Handle::create();

  Mutex m;
  ThreadCondition somethingChanged;

  printf ("GROUP IS: [%lx:%lx]  I AM: [%lx:%lx]\n",grphandle.id[0],grphandle.id[1],me.id[0],me.id[1]);
  Group* group = new SAFplus::Group();
  assert(group);
  group->init(grphandle,mode);
  group->setNotification(somethingChanged);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  group->registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE, false);

  while(1)
    {
    ScopedLock<> lock(m);

    printf("Running Election\n");
    std::pair<EntityIdentifier,EntityIdentifier> as = group->elect();
    printf("Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", as.first.id[0], as.first.id[1], (as.first == me) ? "me": "not me", as.second.id[1],as.second.id[1],(as.second == me) ? "me": "not me"); 
    int result = somethingChanged.timed_wait(m,10000);
    if (result != 0)
      {
      as.first = group->getActive();
      as.second = group->getStandby();
      printf("Group Event! Active: [%lx:%lx]  Standby: [%lx:%lx]\n", as.first.id[0], as.first.id[1], as.second.id[1],as.second.id[1]);
      }
    }

  return 0;
}
