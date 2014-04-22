#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include <clCkptApi.hxx>

#include <clTestApi.hxx>

using namespace SAFplus;


void test_readwrite()
{
  Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
  c1.stats();
  c1.dump();

  if (1)
    {
      clTestCaseStart(("Integer key"));
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);

      // Write some data
      for (int i=0;i<100;i++)
        {
          for (int j=0;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j;
            }
          c1.write(i,*val);
        }

      // Read it
      for (int i=0;i<100;i++)
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
      printf("ITERATOR: \n");
      int prior=-1;
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %s\n",tmp,(*item.second).data);
          if (prior!=-1)
            {
              c1.remove(prior);
            }
          prior = tmp;
        }

      int count = 0;
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %s\n",tmp,(*item.second).data);
          count++;
        }
      clTest(("All items deleted"),count==0,("count %d",count));

      clTestCaseEnd((""));
    }

  if (1)
    {
      clTestCaseStart(("String key"));

      Checkpoint c2(Checkpoint::SHARED | Checkpoint::LOCAL);
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);

      for (int i=0;i<100;i++)
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

      for (int i=0;i<100;i++)
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
          printf("key: %s, value: %s\n",key,(*item.second).data);
        }
      clTestCaseEnd((""));
    }
}

  
int main(int argc, char* argv[])
{
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  utilsInitialize();
  clTestGroupInitialize(("Test Checkpoint"));
  clTestCase(("Basic Read/Write"), test_readwrite());
  clTestGroupFinalize();
}
