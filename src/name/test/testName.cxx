#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clNameApi.hxx>
#include <stdio.h>
#include <clTestApi.hxx>

#define INVALID_ID 0xeeee

using namespace SAFplus;

class ObjTest
{
private:
   std::string nm;
public:
   ObjTest(const char* name){
      nm = name;
   }
   std::string& getName()
   {
      return nm;
   }
   void greet(){
      logInfo("NAMETEST", "---","%s greets from ObjTest", nm.c_str());
   }       
};

typedef struct {
   char name[100];
   NameRegistrar::MappingMode mm;
   uint32_t idx;
   uint32_t proc;
   uint32_t node;
   void* obj;
}Params;

typedef std::map<std::string, SAFplus::Handle> TheMap;
TheMap theMap;

void testNameSetGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);   
   name.set(n, h, m);
   name.setLocalObject(n,obj);
   theMap[n] = h;
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     clTest(("Handle got "), oh == h, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
   try {
     SAFplus::Handle& oh = name.getHandle("blah");
     clTest(("Unexpected handle got"), 0, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
   }catch (NameException &ne) {
      clTest(("Handle get expected exception because of non-existence"), 1, ("exception [%s]", ne.what()));
   }
   // test getName
   try {
     char* nm = name.getName(h);
     clTest(("Name got "), (strcmp(nm, n) == 0), ("Name got [%s]", nm));
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
}

void testNameAppend(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   Handle h2(PointerHandle, idx+1, process, node);
   name.append(n,h,m); 
   name.append(n,h2,m);  
   
   try {
     SAFplus::Handle& oh = name.getHandle(n);    
     if (m == NameRegistrar::MODE_REDUNDANCY)
     {
        SAFplus::Handle firstHdl = theMap[n];
        clTest(("Handle got "), oh == firstHdl, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
     }
     else if (m == NameRegistrar::MODE_PREFER_LOCAL)
     {
        if (process != 0xffffffff || node != 0xffff)
        {
           clTest(("Handle got "), oh == h, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
        }
        else
        {
           clTest(("Handle got "), oh == h2, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
        }
     }
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
   try {
     SAFplus::Handle& oh = name.getHandle("blah");
     clTest(("Invalid handle got"), 0, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));
   }catch (NameException &ne) {     
      clTest(("Handle get expected exception because of non-existence"), 1, ("exception [%s]", ne.what()));    
   }   

   // test getName
   try {
     char* nm = name.getName(h);
     clTest(("Name got "), (strcmp(nm, n) == 0), ("Name got [%s]", nm));
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
}

void objectGet(const char* n, const char* objName)
{
   try {
     RefObjMapPair ho = name.get(n);
     ObjTest* po = (ObjTest*)ho.second;
     clTest(("Object got "), (po != NULL && !po->getName().compare(objName)), ("Actual object [%s]", po->getName().c_str()));
   }catch (NameException &ne) {
      clTest(("Object get unexpected exception"), 0, ("exception [%s]", ne.what())); 
   }
}

void arbitraryDataSet(const char* n, void* data, int len, bool failureCase)
{
   try {
      name.set(n, data, len);            
   }catch (NameException &ne) {
      clTest(("Arbitrary data setting unexpected exception"), !failureCase, ("exception [%s]", ne.what())); 
   }
}

void arbitraryDataSet(const char* n, SAFplus::Buffer* pdata, bool failureCase)
{
   try {
      name.set(n, pdata);
   }catch (NameException &ne) {
      clTest(("Arbitrary data setting unexpected exception"), !failureCase, ("exception [%s]", ne.what())); 
   }
}

void arbitraryDataGet(const char* n, const char* dataToCompare, bool failureCase)
{
   try {
      const SAFplus::Buffer& buf = name.getData(n);
      if (!failureCase)
         clTest(("Arbitrary data got "), (!memcmp(buf.data, dataToCompare, strlen(dataToCompare))), ("Expected data [%s]:[%d]; Actual arbitrary data got [%s]:[%d]", dataToCompare, strlen(dataToCompare), buf.data, buf.len()));
      else
         clTest(("Invalid arbitrary data got "), (false), ("Actual arbitrary data got [%s]", buf.data));
   }catch (NameException &ne) {
      clTest(("Arbitrary data got unexpected exception"), !failureCase, ("exception [%s]", ne.what())); 
   }  
}

void* threadSetProc(void* p)
{
   Params* pr = (Params*)p;
   testNameSetGet(pr->name, pr->mm, pr->idx, pr->obj, pr->proc, pr->node);
   delete pr;
   return 0;
}

void* threadAppendProc(void* p)
{
   Params* pr = (Params*)p;
   testNameAppend(pr->name, pr->mm, pr->idx, pr->obj, pr->proc, pr->node);
   delete pr;
   return 0;
}

void threadNameSetGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Params* p = new Params;
   strcpy(p->name,n);
   p->mm = m;
   p->idx = idx;
   p->proc = process;
   p->node = node;
   p->obj = obj;
   pthread_t thid;
   pthread_create(&thid, NULL, threadSetProc, (void*)p);
   pthread_join(thid, NULL);
}

void threadNameAppendGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Params* p = new Params;
   strcpy(p->name,n);
   p->mm = m;
   p->idx = idx;
   p->proc = process;
   p->node = node;
   p->obj = obj;
   pthread_t thid;
   pthread_create(&thid, NULL, threadAppendProc, (void*)p);
   pthread_join(thid, NULL);
}


void nodeFailure(const char* n, uint32_t idx, unsigned short nodeAddr)
{
   testNameSetGet(n, NameRegistrar::MODE_PREFER_LOCAL, idx);
   Handle h(PointerHandle, idx);   
   Handle h2(PointerHandle, idx+1, 0xffffffff, nodeAddr);   
   name.append(n,h2,NameRegistrar::MODE_PREFER_LOCAL);
   //name.dump();
   SAFplus::ASP_NODEADDR = nodeAddr;
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---", "nodeFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);    
     clTest(("Handle got "), oh == h2, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
   name.nodeFailed(nodeAddr, 0);
   //name.dump();
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","nodeFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Handle got "), oh == h, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
   // case there is only one handle of a key
   Handle h3(PointerHandle, idx+3, 0xffffffff, nodeAddr);
   testNameSetGet(n, NameRegistrar::MODE_PREFER_LOCAL, idx+3, NULL, 0xffffffff, nodeAddr);   
   //name.dump();
   // Invalid node id
   logInfo("NAMETEST", "---","Invalid node id test case\n");
   name.nodeFailed(INVALID_ID, 0);   
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","nodeFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Handle got "), oh==h3, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"),0, ("exception [%s]", ne.what()));
   }
   //name.dump();
   // valid node id
   logInfo("NAMETEST", "---","Valid node id test case\n");
   name.nodeFailed(nodeAddr, 0);
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","nodeFailure(): unexpected handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Unexpected Handle got "), 0, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get expected exception because of non-existence"), 1, ("exception [%s]", ne.what()));
   }
   //name.dump();
}

void processFailure(const char* n, uint32_t idx, uint32_t process)
{
   testNameSetGet(n, NameRegistrar::MODE_PREFER_LOCAL, idx, NULL, process);
   Handle h(PointerHandle, idx, process);
   Handle h2(PointerHandle, idx+1);   
   name.append(n,h2,NameRegistrar::MODE_PREFER_LOCAL);
   //name.dump();   
   try {
     SAFplus::Handle& oh = name.getHandle(n); 
     logDebug("NAMETEST", "---","processFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);   
     clTest(("Handle got "), oh == h, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }   
   name.processFailed(process, 0);
   //name.dump();
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","processFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Handle got "), oh == h2, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }

   // case there is only one handle of a key
   Handle h3(PointerHandle, idx+3, process);
   testNameSetGet(n, NameRegistrar::MODE_PREFER_LOCAL, idx+3, NULL, process);
   //invalid process id
   name.processFailed(INVALID_ID, 0);
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","processFailure(): handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Handle got "), oh==h3, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get unexpected exception"), 0, ("exception [%s]", ne.what()));
   }
   //valid process id
   name.processFailed(process, 0);
   try {
     SAFplus::Handle& oh = name.getHandle(n);
     logDebug("NAMETEST", "---","processFailure(): unexpected handle got [0x%lx.0x%lx]\n", oh.id[0],oh.id[1]);
     clTest(("Unexpected Handle got "), 0, ("Handle got [0x%lx.0x%lx]", oh.id[0],oh.id[1]));     
   }catch (NameException &ne) {
      clTest(("Handle get expected exception because of non-existence"), 1, ("exception [%s]", ne.what()));
   }
}

int main(int argc, char* argv[])
{
#if 0
   logInitialize();
   logEchoToFd = 1;  // echo logs to stdout for debugging
   utilsInitialize();   
#endif
   SAFplus::ASP_NODEADDR=7;
   safplusInitialize(SAFplus::LibDep::LOG|SAFplus::LibDep::NAME);
   //logEchoToFd = 1;
   logSeverity = LOG_SEV_MAX;
   clTestGroupInitialize(("Test name set, append, get"));
   ObjTest* jim = new ObjTest("Jim");   
   //obj->greet();
   const char* name1 = "_name1";
   //clTestCase(("Test name1 set, get"), testNameSetGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbcc, jim));
   clTestCase(("Test name1 append, get"), testNameAppend(name1, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbdd, jim));
   ObjTest* jane = new ObjTest("Jane");
   const char* name2 = "_name2";
   clTestCase(("Test name 2 set, get"), testNameSetGet(name2, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbce, jane));
   clTestGroupFinalize();

   //name.dumpObj();
   clTestGroupInitialize(("Test object set, get"));   
   clTestCase(("Test setting object of name1"), objectGet(name1, "Jim"));   
   clTestCase(("Test setting object of name2"), objectGet(name2, "Jane"));   
   clTestGroupFinalize();

   clTestGroupInitialize(("Test arbitrary data set, get"));
   char data[120], data2[120];   
   strcpy(data, "This is arbitrary data example. Don't care its content");
   clTestCase(("Test setting of arbitrary data of name1"), arbitraryDataSet(name1, data, strlen(data)+1,false));
   strcpy(data2, "This is another arbitrary data example. Don't care its content!...");
   clTestCase(("Test setting of arbitrary data of name2"), arbitraryDataSet(name2, data2, strlen(data2)+1,false));
   clTestCase(("Test getting of arbitrary data of name1"), arbitraryDataGet(name1, data, false));   
   clTestCase(("Test getting of arbitrary data of name2"), arbitraryDataGet(name2, data2, false));
   
   //char modbuf[120];   
   char* modbuf = new char[120];
   strcpy(modbuf,"This is the modified buffer");
   size_t sz = strlen(modbuf)+sizeof(SAFplus::Buffer);
   char temp[sz];
   SAFplus::Buffer* pbuf = new(temp) SAFplus::Buffer(sz);
   *pbuf = modbuf;
   //clTestCase(("Test setting of arbitrary data of name1"), arbitraryDataSet(name1, pbuf, false));
   arbitraryDataSet(name1, pbuf, false);
   clTestCase(("Test getting of arbitrary data of name1"), arbitraryDataGet(name1, modbuf, false));   
   
   //char data[120];
   strcpy(data, "This is arbitrary data example. Don't care its content");
   clTestCase(("Test setting of arbitrary data of an invalid name"), arbitraryDataSet("aaa", data, strlen(data)+1, true));
   SAFplus::Buffer buf;
   clTestCase(("Test setting of arbitrary data of an invalid name"), arbitraryDataSet("aaa", &buf, true));   
   clTestGroupFinalize();
   delete jim;
   delete jane;
   delete []modbuf;
 
   clTestGroupInitialize(("Process/node failure test. They should be deleted from name service"));
   //name.dump();
   const char* name3 = "_name3";      
   clTestCase(("Test failure of node id [1]; name [%s]", name3), nodeFailure(name3, 0x54321, 1));
   const char* name4 = "_name4";
   uint32_t thispid = getpid();
   clTestCase(("Test failure of process [%u]; name [%s]", thispid, name4), processFailure(name4, 0x98765, thispid));
   clTestGroupFinalize();

   clTestGroupInitialize(("Set/get name service simultaneously by threads"));
   const char* name5 = "_name5";   
   clTestCase(("Set/get name service simultaneously; name [%s]", name5), threadNameSetGet(name5, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc1));   
   uint32_t idx = 0xaabbc2;
   int i;
   for(i=0;i<30;i++) {
     clTestCase(("Append/get name service simultaneously; name [%s]", name5), threadNameAppendGet(name5, NameRegistrar::MODE_PREFER_LOCAL, idx));     
     idx+=2;
   }    
   const char* name6 = "_name6";   
   clTestCase(("Set/get name service simultaneously; name [%s]", name6), threadNameSetGet(name6, NameRegistrar::MODE_REDUNDANCY, 0xaabbe1));    
   idx=0xaabbe2;
   for(i=0;i<30;i++) {
     clTestCase(("Append/get name service simultaneously; name [%s]", name6), threadNameAppendGet(name6, NameRegistrar::MODE_ROUND_ROBIN, idx));     
     idx+=2;
   }  
   clTestGroupFinalize();  

   //printf("----Dumpping the name service database---\n");
   //name.dump(); 

   return 0;
}
