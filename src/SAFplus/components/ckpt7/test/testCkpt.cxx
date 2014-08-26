#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>

#include <clCkptApi.hxx>

#include <clTestApi.hxx>

using namespace SAFplus;

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

int LoopCount=10;  // how many checkpoint records should be written during each iteration of the test.

SAFplus::Handle test_readwrite(Checkpoint& c1,Checkpoint& c2)
{
  SAFplus::Handle ret = INVALID_HDL;
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

      // Dump and delete it
      printf("integer ITERATOR: \n");
      int prior=-1;
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %d\n",tmp,*((int*) (*item.second).data));
          if (prior!=-1)
            {
              c1.remove(prior);  // note that you can't delete the element that the iterator is sitting on or the iterator won't work
            }
          prior = tmp;
        }
      if (prior!=-1) c1.remove(prior);
      

      printf("What's left: \n");
      int count = 0;
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %d change: %d\n",tmp,*((int*) (*item.second).data), item.second->changeNum());
          count++;
        }
      clTest(("All items deleted"),count==0,("count %d",count));

      clTestCaseEnd((""));
    }

  if (1)
    {
      clTestCaseStart(("String key"));

      ret = c2.handle();
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

      for (int i=0;i<LoopCount;i++)
        {
	  std::string k;
	  std::string k1;
	  std::string v;
          k.append("key ").append(std::to_string(i));
          k1.append("keystr ").append(std::to_string(i));

          const Buffer& output = c2.read(k);
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

          const Buffer& output1 = c2.read(k1);
          clTest(("Record exists"), &output1 != NULL, (""));
          if (&output1)
            {
	      char* s = (char*) output1.data;
              printf("%s\n",s);
	    }

        }

      printf("ITERATOR: \n");
      for (Checkpoint::Iterator i=c2.begin();i!=c2.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          char* key = (char*) (*item.first).data;
          printf("key: %s, value: %d change: %d\n",key,*((int*) (*item.second).data), item.second->changeNum());
        }
      clTestCaseEnd((""));
    }
  return ret;
}


void test_reopen(SAFplus::Handle handle)
{
  int LoopCount=10;

  if (1)
    {
      clTestCaseStart(("String key"));

      Checkpoint c2(handle,Checkpoint::SHARED | Checkpoint::LOCAL);
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);

      for (int i=0;i<LoopCount;i++)
        {
	  std::string k;
	  std::string k1;
	  std::string v;
          k.append("key ").append(std::to_string(i));
          k1.append("keystr ").append(std::to_string(i));

          const Buffer& output = c2.read(k);
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

          const Buffer& output1 = c2.read(k1);
          clTest(("Record exists"), &output1 != NULL, (""));
          if (&output1)
            {
	      char* s = (char*) output1.data;
              printf("%s\n",s);
	    }

        }

      clTestCaseEnd((""));
    }
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
  clAspLocalId  = SAFplus::ASP_NODEADDR;  // remove clAspLocalId

  safplusInitialize(SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG);
  logEchoToFd = 1;  // echo logs to stdout for debugging

  safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  clTestGroupInitialize(("Test Checkpoint"));
  if (1)
    {
    Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
    Checkpoint c2(Checkpoint::SHARED | Checkpoint::LOCAL);
    clTestCase(("Basic Read/Write"), hdl = test_readwrite(c1,c2));
    }

  clTestCase(("Reopen"), test_reopen(hdl));

  if (1)
    {
    Handle h1 = Handle::create();
    Handle h2 = Handle::create();

    Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::REPLICATED);
    Checkpoint c2(h2,Checkpoint::SHARED | Checkpoint::REPLICATED);
    clTestCase(("Basic Read/Write"), hdl = test_readwrite(c1,c2));
    }


  clTestGroupFinalize();
}
