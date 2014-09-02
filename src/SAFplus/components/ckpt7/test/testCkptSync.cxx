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
ClBoolT   gIsNodeRepresentative = CL_FALSE; // CL_TRUE;

int LoopCount=10;  // how many checkpoint records should be written during each iteration of the test.

uint32_t writeCount = 0;

#define TEMP_CKPT_SYNC_PORT 27

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
          writeCount++;
          ((int*)val->data)[0] = writeCount;
          for (int j=1;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j+writeCount;
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
            int start = ((int*)output.data)[0];
	      for (int j=1;j<10;j++)
		{
		  int tmp = ((int*)output.data)[j];
		  if (tmp != start+i+j)
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
        clAspLocalId = SAFplus::ASP_NODEADDR = temp;
        if (CL_OK != rc)
          {
          printf("[%s] is not a valid slot id.\n", optarg);
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
          printf("Unknown option [%c]\n", optopt);
          return -1;
          break;
        default :
          printf("Unknown error\n");
          return -1;
          break;
        }

  return 1;
  }

int main(int argc, char* argv[])
{
  ClRcT rc;
  clAspLocalId  = SAFplus::ASP_NODEADDR = 1;
  parseArgs(argc, argv);
  printf("Node: %d Port: %d\n", SAFplus::ASP_NODEADDR, TEMP_CKPT_SYNC_PORT);
  SafplusInitializationConfiguration sic;
  sic.iocPort     = TEMP_CKPT_SYNC_PORT;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS;
  safplusInitialize(SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG,sic);
  SAFplus::Handle hdl;
  logEchoToFd = 1;  // echo logs to stdout for debugging

  //safplusMsgServer.init(TEMP_CKPT_SYNC_PORT, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  clTestGroupInitialize(("Test Checkpoint"));

  Handle h1 = WellKnownHandle(64<<SUB_HDL_SHIFT,0,0);
  Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::REPLICATED);
  if (role == 0)
    {

    //clTestCase(("Basic Read/Write"), test_readwrite(c1));

    printf("start another instance of this test on another node.\n");
    while(1)
      {
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);
      for (int i=0;i<LoopCount;i++)
        {
          writeCount++;
          ((int*)val->data)[0] = writeCount;
          for (int j=1;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j+writeCount;
            }
          printf("Writing entry [%d] with [%d]\n", i, *((int*)val->data));
          c1.write(i,*val);
          sleep(10);
        }
      }

    sleep(10000);
    }
  else
    {
    while(1)
      {
      printf("integer ITERATOR: \n");
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %d change: %d\n",tmp,*((int*) (*item.second).data), item.second->changeNum());
        }

      printf("Checking for miscompares...\n");

      // Read it
      for (int i=0;i<LoopCount;i++)
        {
          const Buffer& output = c1.read(i);
          clTest(("Record exists"), &output != NULL, (""));
          if (&output)
            {
            int start = ((int*)output.data)[0];
	      for (int j=1;j<10;j++)
		{
		  int tmp = ((int*)output.data)[j];
		  if (tmp != start+i+j)
		    {
		      clTestFailed(("Stored data MISCOMPARE i:%d, j:%d, expected:%d, got:%d\n", i,j,start+i+j,tmp));
		      j=10; // break out of the loop
		    }
		}
            }
        }


      printf("Delaying 10 seconds...");
      sleep(10);
      }
    }


  clTestGroupFinalize();
}
