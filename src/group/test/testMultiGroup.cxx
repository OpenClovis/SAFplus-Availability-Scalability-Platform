// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clIocApi.h>
#include <clGroup.hxx>
#include <clGlobals.hxx>

void singleElect();
void tripleElect();

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
  SAFplus::ASP_NODEADDR = 1;
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  parseArgs(argc,argv);
  printf("ENV PARSED: gIsNodeRepresentative=[%s] ASP_NODEADDR=[%ld] Port=[%d]",gIsNodeRepresentative?"TRUE":"FALSE",SAFplus::ASP_NODEADDR , port);

  utilsInitialize();

  ClRcT rc;
  // initialize SAFplus6 libraries 
  //if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
  if ((rc = ::clOsalInitialize(NULL)) != CL_OK || (rc = ::clHeapInit()) != CL_OK || (rc = ::clBufferInitialize(NULL)) != CL_OK || (rc = ::clTimerInitialize(NULL)) != CL_OK)
    {
    assert(0);
    }

  rc = ::clIocLibInitialize(NULL);
  assert(rc==CL_OK);
  
  groupInitialize();

  safplusMsgServer.init(port, MAX_MSGS, MAX_HANDLER_THREADS);
  /* Library should start it */
  safplusMsgServer.Start();

  //Handle replicaHandle = Handle::create(safplusMsgServer.handle.getPort());
  sleep(10);
  printf("\nstartup sleep finished\n");

  //singleElect();
  tripleElect();

  return 0;
  }


void singleElect()
  {
  Handle grphandle = WellKnownHandle(64<<SUB_HDL_SHIFT,0,0);
  Handle me = Handle::create();
  int mode = SAFplus::Group::DATA_IN_MEMORY;

  Mutex m;
  ThreadCondition somethingChanged;

  printf ("GROUP IS: [%lx:%lx]  I AM: [%lx:%lx]\n",grphandle.id[0],grphandle.id[1],me.id[0],me.id[1]);
  Group* group = new SAFplus::Group();
  assert(group);
  group->init(grphandle,mode,port);
  group->setNotification(somethingChanged);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  group->registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY, true);
  //group->registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE, true);
  //group->registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_ACTIVE, true);

  while(1)
    {
    ScopedLock<> lock(m);

    printf("\n------------LOOP----------------\n");
    std::pair<EntityIdentifier,EntityIdentifier> as1 = group->getRoles(); // elect();

    printf("Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", as1.first.id[0], as1.first.id[1], (as1.first == me) ? "me": "not me", as1.second.id[0],as1.second.id[1],(as1.second == me) ? "me": "not me");

    int result = somethingChanged.timed_wait(m,10000);
    if (result != 0)
      {
      as1.first = group->getActive();
      as1.second = group->getStandby();
      printf("Group Event! Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", as1.first.id[0], as1.first.id[1], (as1.first == me) ? "me": "not me", as1.second.id[0],as1.second.id[1],(as1.second == me) ? "me": "not me");
      }
    }
}


void tripleElect()
  {
  Handle grphandle = WellKnownHandle(64<<SUB_HDL_SHIFT,0,0);
  Handle me = Handle::create();
  int mode = SAFplus::Group::DATA_IN_MEMORY;

  Mutex m;
  ThreadCondition somethingChanged;

  printf ("GROUP IS: [%lx:%lx]  I AM: [%lx:%lx]\n",grphandle.id[0],grphandle.id[1],me.id[0],me.id[1]);
  Group* group = new SAFplus::Group();
  assert(group);
  group->init(grphandle,mode,port);
  group->setNotification(somethingChanged);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  group->registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY, true);


  Group failback;
  Handle fbhandle = WellKnownHandle(65<<SUB_HDL_SHIFT,0,0);
  failback.init(fbhandle,mode,port);
  failback.setNotification(somethingChanged);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  failback.registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE, true);

  Group noStandby;
  Handle noshandle = WellKnownHandle(66<<SUB_HDL_SHIFT,0,0);
  noStandby.init(noshandle,mode,port);
  noStandby.setNotification(somethingChanged);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  noStandby.registerEntity(me, (port<<4) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_ACTIVE, true);

  while(1)
    {
    ScopedLock<> lock(m);

    printf("----------- LOOP ----------\n");
    std::pair<EntityIdentifier,EntityIdentifier> as1 = group->getRoles(); // elect();
    std::pair<EntityIdentifier,EntityIdentifier> as2 = failback.getRoles(); // elect();
    std::pair<EntityIdentifier,EntityIdentifier> as3 = noStandby.getRoles(); // elect();

    printf("Sticky [%lx:%lx]: Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", group->handle.id[0], group->handle.id[1],as1.first.id[0], as1.first.id[1], (as1.first == me) ? "me": "not me", as1.second.id[0],as1.second.id[1],(as1.second == me) ? "me": "not me");
    printf("Failback [%lx:%lx]: Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", failback.handle.id[0], failback.handle.id[1],as2.first.id[0], as2.first.id[1], (as2.first == me) ? "me": "not me", as2.second.id[0],as2.second.id[1],(as2.second == me) ? "me": "not me");
    printf("No Standby [%lx:%lx]: Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", noStandby.handle.id[0], noStandby.handle.id[1],as3.first.id[0], as3.first.id[1], (as3.first == me) ? "me": "not me", as3.second.id[0],as3.second.id[1],(as3.second == me) ? "me": "not me");

    int result = somethingChanged.timed_wait(m,10000);
    if (result != 0)
      {
      as1.first = group->getActive();
      as1.second = group->getStandby();
      printf("Group Event! Active: [%lx:%lx] (%s)  Standby: [%lx:%lx] (%s)\n", as1.first.id[0], as1.first.id[1], (as1.first == me) ? "me": "not me", as1.second.id[0],as1.second.id[1],(as1.second == me) ? "me": "not me");
      }
    }

}
