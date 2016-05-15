#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>

#include <clCkptApi.hxx>

#include <clTestApi.hxx>

#include <chrono>
#include <boost/program_options.hpp>
#include <string.h>

using namespace SAFplus;

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=10;

int testPersistentCkpt = 0;

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

      clTestCaseEnd((" "));
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
      clTestCaseEnd((" "));
    }
  return ret;
}


SAFplus::Handle test_wait(Checkpoint& c1)
{
  uint_t change = c1.getChangeNum();

  c1.write("key","value");
  clTest(("wait returns properly"), c1.wait(change)==true, ("incorrect return value"));
  change = c1.getChangeNum();
  uint_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  clTest(("waiting returns properly"), c1.wait(change,500)==false, ("incorrect return value"));
  uint_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  clTest(("wait actually waited"), t2-t1 > 500, ("st: %d end: %d",t1, t2));
  clTest(("but not too long"), t2-t1 < 1500, ("st: %d end: %d duration: %d",t1, t2,t2 - t1));  // can be very large, esp. in slow machines like qemu or VMs
}


void test_reopen(SAFplus::Handle handle, bool persistentCkpt=false, bool recordExists=true)
{
  int LoopCount=10;

  if (1)
    {
      clTestCaseStart(("String key"));
      uint_t flags = Checkpoint::SHARED | Checkpoint::LOCAL;
      if (persistentCkpt) flags|=Checkpoint::PERSISTENT;
      Checkpoint c2(handle,flags);
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
          if (recordExists)
          {
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
          else
            clTest(("Record not existing"), &output == NULL, (" "));

          const Buffer& output1 = c2.read(k1);
          if (recordExists)
          {
            clTest(("Record exists"), &output1 != NULL, (" "));
            if (&output1)
            {
	      char* s = (char*) output1.data;
              printf("%s\n",s);
	    }
          }
          else
            clTest(("Record not existing"), &output1 == NULL, (" "));
        }

      clTestCaseEnd((" "));
    }
}





void parseCmdLineOptions(int argc, char* argv[])
{
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("testPersistentCheckpoint", boost::program_options::value<int>(), "test persistent checkpoint: 0 (default) - not test persistent checkpoint; 1 - test persistent checkpoint");

  boost::program_options::variables_map vm;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
      exit(0);
    }
  if (vm.count("testPersistentCheckpoint")) testPersistentCkpt = vm["testPersistentCheckpoint"].as<int>();  
}

void deleteCheckpoint(Handle& ckptHdl)
{
  std::string ckptSharedMemoryObjectname = "SAFplusCkpt_";
  char sharedMemFile[256];  
  ckptHdl.toStr(sharedMemFile);
  ckptSharedMemoryObjectname.append(sharedMemFile);  
  char path[CL_MAX_NAME_LENGTH];
  snprintf(path, CL_MAX_NAME_LENGTH-1, "%s%s", SharedMemPath, ckptSharedMemoryObjectname.c_str());
  unlink(path);
}

void writeObjToCkpt(Checkpoint& c, const char* key, const void* obj, int objlen)
{
  size_t keylen = strlen(key)+1; // '/0' is also counted
  char kmem[sizeof(Buffer)-1+keylen];
  Buffer* kb = new(kmem) Buffer(keylen);
  memcpy(kb->data,key,keylen);
  char vdata[sizeof(Buffer)-1+objlen];
  Buffer* val = new(vdata) Buffer(objlen);
  memcpy(val->data, obj, objlen);
  c.write(*kb,*val);
}

#if 0
void writeObjToCkpt(Checkpoint& c, const char* key, int keylen, const void* obj, int objlen)
{
  size_t keylen = strlen(key)+1; // '/0' is also counted
  char kmem[sizeof(Buffer)-1+keylen];
  Buffer* kb = new(kmem) Buffer(keylen);
  memcpy(kb->data,key,keylen);
  char vdata[sizeof(Buffer)-1+objlen];
  Buffer* val = new(vdata) Buffer(objlen);
  memcpy(val->data, obj, objlen);
  c.write(*kb,*val);
}
#endif

void test_big(Checkpoint& c, int keylimit,int datalimit, int loops)
{
  ClRcT rc;

  // Create a bunch of records
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % datalimit;
      int keysize = rand() % keylimit;
      unsigned char* buf = new unsigned char[size];
      unsigned char* keybuf = new unsigned char[keysize];
      assert(buf);
      assert(keybuf);
      
      for (int j=0;j<keysize;j++)
        {
          keybuf[j] = rand() & 255;
        }
      for (int j=0;j<size;j++)
        {
          buf[j] = rand() & 255;
        }

      c.write(keybuf,keysize,buf,size);
      delete buf;
      delete keybuf;
      printf("%d ", i);
      fflush(stdout);
    }
  printf("\n");

  // Now verify them
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % datalimit;
      int keysize = rand() % keylimit;

      unsigned char* keybuf = new unsigned char[keysize];
      assert(keybuf);

      // recreate the key
      for (int j=0;j<keysize;j++) keybuf[j] = rand() & 255;

      unsigned char* rec;
      ClUint32T recSz;

      const Buffer& output = c.read(keybuf,keysize);
      clTest(("Record exists"), &output != NULL, (" "));
      if (&output)
        {
	unsigned char* rec = (unsigned char*) output.data;
        int recSz = output.len();
        clTest(("val record [%d] size", i), size == recSz, ("record size [%d], expected size [%d]",recSz,size));
      
        for (int j=0;j<size;j++)
          {
            unsigned char ch = rand() & 255;
            clTest((" "), rec[j] == ch, ("data mismatch [%d] expected [%d]",rec[j],ch)); 
          }

        }
      delete keybuf;
      printf("%d ", i);
      fflush(stdout);
    }
  printf("\n");
}


void persistentWithObjects()
{
  clTestCaseStart(("CKP-PST-FNC.TC010: Persistent objects"));
  class Obj 
  {
  public:
    Obj() : d1(-1), d2(false) {}
    Obj(int _d1, bool _d2) : d1(_d1), d2(_d2) {}
    bool operator==(const Obj& o) const 
    {
      return (d1 == o.d1 && d2 == o.d2);
    }
  private:
    int d1;
    bool d2;
  };
  const Obj o1(-2, false);
  //const Obj o2;
  const Obj o3(21, true);
  //const Obj* const p1 = &o1;
  Handle h = Handle::create();
  Checkpoint c(h,Checkpoint::SHARED | Checkpoint::LOCAL | Checkpoint::PERSISTENT);
  const char* key1 = "o1";
  writeObjToCkpt(c, key1, &o1, sizeof(o1));
  const char* key3 = "o3";
  writeObjToCkpt(c, key3, &o3, sizeof(o3));
  printf("DUMP ckpt\n");
  c.dump();
  c.flush(); // store checkpoint data to disk
  deleteCheckpoint(h); // simulate that this checkpoint no longer exists
  Checkpoint c2(h,Checkpoint::SHARED | Checkpoint::LOCAL|Checkpoint::PERSISTENT);
  printf("DUMP ckpt\n");
  c2.dump();
  const Buffer& buf = c2.read(key1);  
  if (&buf)
  {
    Obj* po1 = (Obj*)buf.data;
    clTest(("Persistent object verification"),(*po1 == o1),(" "));      
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
  }
  const Buffer& buf3 = c2.read(key3);
  if (&buf3)
  {
    Obj* po3 = (Obj*)buf3.data;
    clTest(("Persistent object verification"),(*po3 == o3),(" "));  
  }    
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
  }

  clTestCaseEnd((" "));
}

void removeCkptData(Handle& h)
{
  deleteCheckpoint(h);  
  Checkpoint c(h,Checkpoint::SHARED | Checkpoint::LOCAL | Checkpoint::PERSISTENT);
  c.dump();
  for (int i=0;i<LoopCount;i++)
  {
    std::string k;       
    k.append("keystr ").append(std::to_string(i));
    size_t keylen = k.length()+1; // '/0' is also counted
    char kmem[sizeof(Buffer)-1+keylen];
    Buffer* kb = new(kmem) Buffer(keylen);
    memcpy(kb->data,k.c_str(),keylen);      
    c.remove(*kb);
  }
  c.dump();
  c.flush();
  // Add one more item to this checkpoint
  int verified_value = 1999;
  std::string kk, vv;
  kk.append("keystr ").append(std::to_string(verified_value));
  vv.append("value ").append(std::to_string(verified_value));
  c.write(kk,vv);
  c.flush();
  // verifying the remainings  
  deleteCheckpoint(h);
  Checkpoint c2(h,Checkpoint::SHARED | Checkpoint::LOCAL|Checkpoint::PERSISTENT);
  c2.dump();
  for (int i=0;i<LoopCount;i++)
  {
    std::string k;
    std::string k1;
    //std::string v;
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
    else
      clTestFailed(("Record exists FAILED"));

    const Buffer& output1 = c2.read(k1);
    clTest(("Record not existing"), &output1 == NULL, (" "));    
  }
  //Verify one added item
  const Buffer& output = c2.read(kk.c_str());  
  if (&output)
  {
     char* s = (char*) output.data;
     clTest(("new added item "), vv.compare(s)==0, (" "));  
  }
  else
    clTestFailed(("New added item exists FAILED"));
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

  SafplusInitializationConfiguration sic;// = {50, MAX_MSGS, MAX_HANDLER_THREADS};
  sic.iocPort     = 50;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS; 

  SAFplus::logSeverity = SAFplus::LOG_SEV_TRACE;
  logEchoToFd = 1;  // echo logs to stdout for debugging
  //safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);

  safplusInitialize(SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG, sic);
  safplusMsgServer.Start();
  parseCmdLineOptions(argc, argv);
  if (!testPersistentCkpt) 
  {
    clTestGroupInitialize(("Test Checkpoint"));
    
    if (1)
    {
      Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
      Checkpoint c2(Checkpoint::SHARED | Checkpoint::LOCAL);
      clTestCase(("CKP-BAS-FNC.TC001: Basic Read/Write"), hdl = test_readwrite(c1,c2));
    }

    clTestCase(("CKP-BAS-FNC.TC002: Reopen"), test_reopen(hdl));

    if (1)
    {
      Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
      clTestCase(("CKP-BAS-FNC.TC003: Wait functionality"), hdl = test_wait(c1));
    }    

    if (1)
    {
      Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
      clTestCase(("CKP-BAS-FNC.TC004: Big checkpoint"), test_big(c1,1024,2048,1000));
    }    

    printf("\n***************\nSubsequent tests require a group server (safplus_amf), or you will hang here.\n***************\n");
    if (1)
    {
      Handle h1 = Handle::create();
      Handle h2 = Handle::create();

      Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::REPLICATED);
      Checkpoint c2(h2,Checkpoint::SHARED | Checkpoint::REPLICATED);
      clTestCase(("CKP-RPL-FNC.TC004: Replicated Read/Write"), hdl = test_readwrite(c1,c2));
    }    
    clTestGroupFinalize();
  }
  else
  {
    clTestGroupInitialize(("Test Persistent Checkpoint"));
#if 1
    Handle h1 = Handle::create();
    Handle h2 = Handle::create();

    Checkpoint c1(h1,Checkpoint::SHARED | Checkpoint::LOCAL, 3600);
    Checkpoint c2(h2,Checkpoint::SHARED | Checkpoint::LOCAL | Checkpoint::PERSISTENT, 1800);
    clTestCase(("CKP-PST-FNC.TC005: Persistent Basic Read/Write"), test_readwrite(c1,c2));

    c2.flush();
    
    deleteCheckpoint(h1); // simulate that this checkpoint no longer exists
    deleteCheckpoint(h2); // simulate that this checkpoint no longer exists 
    clTestCase(("CKP-PST-FNC.TC006: Persistent Reopen"), test_reopen(h1, false, false));
    clTestCase(("CKP-PST-FNC.TC007: Persistent Reopen 2"), test_reopen(h2, true));
    clTestCase(("CKP-PST-FNC.TC008: Persistent removal"), removeCkptData(h2));
#endif
    persistentWithObjects();
    clTestGroupFinalize(); 
 }

 safplusFinalize();
 return 0;
}
