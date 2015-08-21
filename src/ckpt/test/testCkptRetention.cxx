#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>
#include <boost/thread/thread.hpp>

#include <clCkptApi.hxx>

#include <clTestApi.hxx>

using namespace SAFplus;

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

int LoopCount=10;  // how many checkpoint records should be written during each iteration of the test.

void ckptWrite_case1(Checkpoint& c1)
{
  SAFplus::Handle ret = INVALID_HDL;
  c1.stats(); 

  if (1)
    {
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
   }
}

void ckptWrite_case2(Checkpoint& c2)
{
   if (1)
    {
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);

      for (int i=0;i<LoopCount;i++)
        {
	  std::string k;
	  std::string k1;
	  std::string v;
          k.append("key ").append(std::to_string(i));
          k1.append("keystr ").append(std::to_string(i));
          v.append("value ").append(std::to_string(i));
          for (int j=0;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j;
            }
          c2.write(k,*val);
          c2.write(k1,v);
        }
    }
}

void ckptRead_case1(const Handle& h1, uint64_t t, bool avail)
{
   Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::LOCAL, t);
   if (avail)
   {
      // Read it
      for (int i=0;i<LoopCount;i++)
        {
          const Buffer& output = c1.read(i);
          clTest(("Record exists"), &output != NULL, (" "));
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
   }
   else
   {
     for (int i=0;i<LoopCount;i++)
        {
          const Buffer& output = c1.read(i);
          clTest(("Record not exist"), &output == NULL, (" "));
        }
   } 
}

void ckptRead_case2(const Handle& h2, uint64_t t, bool avail)
{
  Checkpoint c2(h2,Checkpoint::SHARED | Checkpoint::LOCAL, t);
  if (avail)
  {
      for (int i=0;i<LoopCount;i++)
        {
	  std::string k;
	  std::string k1;
	  std::string v;
          k.append("key ").append(std::to_string(i));
          k1.append("keystr ").append(std::to_string(i));

          const Buffer& output = c2.read(k);
          clTest(("Record exists"), &output != NULL, (" "));
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

          const Buffer& output1 = c2.read(k1);
          clTest(("Record exists"), &output1 != NULL, (" "));
          if (&output1)
            {
	      char* s = (char*) output1.data;
              //printf("%s\n",s);
	    }

        }
  }
  else
  {  
    // c2 shouldn't be available
    for (int i=0;i<LoopCount;i++)
    {
      std::string k;
      std::string k1;
      std::string v;
      k.append("key ").append(std::to_string(i));
      k1.append("keystr ").append(std::to_string(i));

      const Buffer& output = c2.read(k);
      clTest(("Record not exist"), &output == NULL, (" "));    
      const Buffer& output1 = c2.read(k1);
      clTest(("Record not exist"), &output1 == NULL, (" "));  
    }
  }
}

void checkCkptNotExisting(const Handle& h)
{
  try {
    Checkpoint c(h, Checkpoint::SHARED | Checkpoint::LOCAL | Checkpoint::EXISTING);  
  } catch(...) {
    clTest(("Expected exception got because no checkpoint available"), 1, ("Exception: non-existing")); 
  }  
}

void twoProcessesOpeningCkpt()
{
  uint64_t c1_rduration = 100;
  uint64_t c2_rduration = 150;

  Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL, c1_rduration);
  Checkpoint c2(Checkpoint::SHARED | Checkpoint::LOCAL, c2_rduration);
  ckptWrite_case1(c1);
  ckptWrite_case2(c2);
  
  const Handle& h1 = c1.handle();
  const Handle& h2 = c2.handle();  

  pid_t pid = fork();
  if (pid == 0) // child process
  {
    printf("\nchild process launched\n");
    logInfo("CKPT","TST", "\nchild process launched\n");
    ckptRead_case1(h1,c1_rduration,true);
    boost::this_thread::sleep(boost::posix_time::seconds(c1_rduration+60));  // 60s is the duration for ckptUpdateTimer to update the ckpt shm data
    ckptRead_case1(h1,c1_rduration,false);

    boost::this_thread::sleep(boost::posix_time::seconds(c1_rduration+60));  // 60s is the duration for ckptUpdateTimer to update the ckpt shm data
    checkCkptNotExisting(h1);
 
    printf("\nend of child process\n");
    
    logInfo("CKPT","TST", "\nEnd of child process\n");
  }
  else if (pid>0) // parent process
  {
    printf("\nparent process launched\n");
    int i=c2_rduration;
    do
    {
      ckptRead_case2(h2,c2_rduration,true);         
      i--;
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    } while(i>0);
    boost::this_thread::sleep(boost::posix_time::seconds(c2_rduration+60)); // 60s is the duration for ckptUpdateTimer to update the ckpt shm data
    ckptRead_case2(h2,c2_rduration,false);

    boost::this_thread::sleep(boost::posix_time::seconds(c2_rduration+60));  // 60s is the duration for ckptUpdateTimer to update the ckpt shm data
    checkCkptNotExisting(h2);

    printf("\nend of parent process\n");
  }
  else
  {
    logError("CKPRET", "TST", "Fail to fork");
  }
}

void openCkptButNotAccessingIt()
{
  uint64_t c_rduration = 100;
  Checkpoint c(Checkpoint::SHARED | Checkpoint::LOCAL, c_rduration);
  const Handle& h = c.handle();
  boost::this_thread::sleep(boost::posix_time::seconds(c_rduration+60)); // 60s is the duration for ckptUpdateTimer to update the ckpt shm data  
  checkCkptNotExisting(h);
}

// IOC related globals
ClUint32T clAspLocalId = 0x1;
ClUint32T chassisId = 0x0;
ClBoolT   gIsNodeRepresentative = CL_TRUE;

  
int main(int argc, char* argv[])
{
  ClRcT rc;
  SAFplus::ASP_NODEADDR = 1;
  SAFplus::Handle hdl;
  clAspLocalId  = SAFplus::ASP_NODEADDR;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 50;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS; 

  SAFplus::logSeverity = SAFplus::LOG_SEV_TRACE;
  logEchoToFd = 1;  // echo logs to stdout for debugging

  safplusInitialize(SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG, sic);

  safplusMsgServer.Start();

  clTestGroupInitialize(("Test Checkpoint retention - 2 processes opening/reading/writing checkpoint "));
  twoProcessesOpeningCkpt();
  clTestGroupFinalize();

  clTestGroupInitialize(("Test Checkpoint retention - process opens checkpoint but doesn't read or write it"));
  openCkptButNotAccessingIt();
  clTestGroupFinalize();
  return 1;
}
