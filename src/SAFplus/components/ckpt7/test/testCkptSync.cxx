#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>

#include <clCkptApi.hxx>

#include <clTestApi.hxx>

using namespace SAFplus;

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

// What role should this instance take in the test to be run?
unsigned int role = 0;

// IOC related globals
ClUint32T clAspLocalId = 0x1;
ClUint32T chassisId = 0x0;
ClBoolT   gIsNodeRepresentative = CL_TRUE;

int LoopCount=10;  // how many checkpoint records should be written during each iteration of the test.

SAFplus::Handle test_readwrite(Checkpoint& c1)
{
  SAFplus::Handle ret = INVALID_HDL;
  int LoopCount=10;
  c1.stats();

  printf("Dump of current checkpoint\n");
  c1.dump();
  printf("Dump finished\n");

  if (1)
    {
      clTestCaseStart(("Integer key"));
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);

      // Write some data
      for (int i=0;i<LoopCount;i++)
        {
          for (int j=0;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j;
            }
          c1.write(i,*val);
        }

      // Dump 
      printf("integer ITERATOR: \n");
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %d change: %d\n",tmp,*((int*) (*item.second).data), item.second->changeNum());
        }

      printf("READ: \n");

      // Read it
      for (int i=0;i<LoopCount;i++)
        {
          const Buffer& output = c1.read(i);
          clTest(("Record exists"), &output != NULL, (""));
          if (&output)
            {
	      for (int j=0;j<10;j++)
		{
		  int tmp = ((int*)output.data)[j];
		  if (tmp != i+j)
		    {
		      clTestFailed(("Stored data MISCOMPARE i:%d, j:%d, expected:%d, got:%d\n", i,j,i+j,tmp));
		      j=10; // break out of the loop
		    }
		}
            }
        }
      clTestCaseEnd((""));
    }


  return ret;
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
  return CL_ERR_INVALID_PARAMETER;
  }

void printUsage(char* progname)
  {
  printf("Usage: %s -l <Local Slot ID> -n <Node Name> -r <role 0 or 1>\n", progname);
}

int parseArgs(int argc, char* argv[])
  {
  ClInt32T option = 0;
  ClInt32T nargs = 0;
  ClRcT rc = CL_OK;
  const ClCharT *short_options = ":r:l:m:n:p:fh";

  const struct option long_options[] = 
    {
    {"localslot",   1, NULL, 'l'},
    {"nodename",    1, NULL, 'n'},
    {"role",        1, NULL, 'r'},
    {"help",        0, NULL, 'h'},
    { NULL,         0, NULL,  0 }
    };


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
        case 'r':
          {
          ClUint32T temp=0;
          rc = cpmStrToInt(optarg, &temp);
          role = temp;
          } break;
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

  
int main(int argc, char* argv[])
{
  ClRcT rc;
  SAFplus::ASP_NODEADDR = 1;

  SAFplus::Handle hdl;
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  utilsInitialize();

  parseArgs(argc, argv);

  // initialize SAFplus6 libraries 
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
    {
    assert(0);
    }

  clAspLocalId  = SAFplus::ASP_NODEADDR;  // remove clAspLocalId
  rc = clIocLibInitialize(NULL);
  assert(rc==CL_OK);

  safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  clTestGroupInitialize(("Test Checkpoint"));

  Handle h1 = WellKnownHandle(64<<SUB_HDL_SHIFT,0,0);
  Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::REPLICATED);
  if (role == 0)
    {

    clTestCase(("Basic Read/Write"), test_readwrite(c1));

    printf("start another instance of this test on another node.\n");
    sleep(10000);
    }
  else
    {
    while(1)
      {
          // Dump and delete it
      printf("integer ITERATOR: \n");
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %d change: %d\n",tmp,*((int*) (*item.second).data), item.second->changeNum());
        }

      printf("READ: \n");

      // Read it
      for (int i=0;i<LoopCount;i++)
        {
          const Buffer& output = c1.read(i);
          clTest(("Record exists"), &output != NULL, (""));
          if (&output)
            {
	      for (int j=0;j<10;j++)
		{
		  int tmp = ((int*)output.data)[j];
		  if (tmp != i+j)
		    {
		      clTestFailed(("Stored data MISCOMPARE i:%d, j:%d, expected:%d, got:%d\n", i,j,i+j,tmp));
		      j=10; // break out of the loop
		    }
		}
            }
        }
      sleep(10);
      }
    }


  clTestGroupFinalize();
}
