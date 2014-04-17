#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include <clNameApi.hxx>
#include <stdio.h>
#define __THREAD
using namespace SAFplus;
using namespace SAFplusI;

extern NameRegistrar name;
//NameRegistrar gname;
//NameRegistrar& gname = NameRegistrar::getInstance();

class ObjTest
{
private:
   std::string nm;
public:
   ObjTest(const char* name){
      nm = name;
   }
   void greet(){
      printf("%s greets from ObjTest\n", nm.c_str());
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


void testNameSetGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   //Handle h2(PointerHandle, idx+1, process, node);
   //const char* xy = "_111000111";
   name.set(n, h, m, obj);
   try {
     SAFplus::Handle oh = name.getHandle(n);
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException &ne) {
      printf("Error [%s]\n", ne.what());     
   }
   try {
     SAFplus::Handle oh = name.getHandle("blah");
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException &ne) {
      printf("Error [%s]\n", ne.what());     
   }   
}

void testNameAppend(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   Handle h2(PointerHandle, idx+1, process, node);
   name.append(n,h,m, obj); 
   name.append(n,h2,m, obj);  
   
   try {
     SAFplus::Handle oh = name.getHandle(n);
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException &ne) {
      printf("Error [%s]\n", ne.what());     
   }
   try {
     SAFplus::Handle oh = name.getHandle("blah");
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException &ne) {
      printf("Error [%s]\n", ne.what());     
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


int main(int argc, char* argv[])
{
#ifndef __THREAD
#if 1
   ObjTest* jim = new ObjTest("Jim");   
   //obj->greet();
   const char* name1 = "_111000111";
   testNameSetGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbcc, jim);
   testNameAppend(name1, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbdd, jim);
   ObjTest* jane = new ObjTest("Jane");
   const char* name2 = "_011000111";
   testNameSetGet(name2, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbce, jane);
   
   name.dumpObj();
   try {
     ObjMapPair ho = name.get(name1);
     ObjTest* po = (ObjTest*)ho.second;     
     po->greet();
   }catch (NameException &ne) {
      printf("Exception [%s]\n", ne.what());
   }
   
   try {
     ObjMapPair ho = name.get(name2);
     ObjTest* po = (ObjTest*)ho.second;     
     po->greet();
   }catch (NameException &ne) {
      printf("Exception [%s]\n", ne.what());
   }

   char data[120];
   strcpy(data, "This is arbitrary data example. Don't care its content");
   try {
      name.set(name1, data, strlen(data)+1);
      strcpy(data, "This is another arbitrary data example. Don't care its content!...");
      name.set(name2, data, strlen(data)+1);
   }catch (NameException &ne) {
       printf("Exception [%s]\n", ne.what());
   }
   
   try {
      SAFplus::Buffer& buf = name.getData(name1);
      printf("name [%s]: buf [%s]\n", name1, buf.data);
   }catch (NameException &ne) {
      printf("Exception [%s]", ne.what());
   }   
   try {
      SAFplus::Buffer& buf = name.getData(name2);
      printf("name [%s]: buf [%s]\n", name2, buf.data);
   }catch (NameException &ne) {
      printf("Exception [%s]", ne.what());
   }
   char modbuf[] = {"This is the modified buffer"};
   size_t sz = strlen(modbuf)+sizeof(SAFplus::Buffer);
   char temp[sz];
   SAFplus::Buffer* pbuf = new(temp) SAFplus::Buffer(sz);
   *pbuf = modbuf;
   try {
      name.set(name1, pbuf);
   }catch (NameException &ne) {
      printf("Exception [%s]", ne.what()); 
   }
   try {
      SAFplus::Buffer& buf = name.getData(name1);
      printf("name [%s]: buf [%s]\n", name1, buf.data);
      buf.decRef();
   }catch (NameException &ne) {
      printf("Exception [%s]", ne.what());
   }
   
#endif
   //char data[120];
#if 1
   strcpy(data, "This is arbitrary data example. Don't care its content");
   try {
      name.set("aaaa", data, strlen(data)+1);
   }catch (NameException &ne) {
      printf("Exception [%s]\n", ne.what());
   }
   SAFplus::Buffer buf;
   try {
      name.set("aaaa", &buf);
   }catch (NameException &ne) {
      printf("Exception [%s]\n", ne.what());
   }
   delete jim;
   delete jane;
// Test with "prefer local" mode
   pid_t thispid = getpid();   
   const char* name3 = "_001000111";
   testNameSetGet(name3, NameRegistrar::MODE_REDUNDANCY, 0xaabbe4, NULL);
   testNameAppend(name3, NameRegistrar::MODE_NO_CHANGE, 0xaabbe5, NULL);
   testNameAppend(name3, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbe7, NULL, (uint32_t)thispid);
#endif
   //const char* name3 = "_001000111";
   testNameAppend(name3, NameRegistrar::MODE_NO_CHANGE, 0xaabbe9, NULL, 0xffffffff, 2);
#else
   const char* name1 = "_1100";   
   threadNameSetGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc1);
   uint32_t idx = 0xaabbc2;
   int i;
   for(i=0;i<30;i++) {
     threadNameAppendGet(name1, NameRegistrar::MODE_PREFER_LOCAL, idx);
     idx+=2;
   } 
   /*threadNameAppendGet(name1, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbc2);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc4);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc6);      
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc8);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbca);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbcc);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbce);
   threadNameAppendGet(name1, NameRegistrar::MODE_ROUND_ROBIN, 0xaabbc6);*/
   const char* name2 = "_0100";   
   threadNameSetGet(name2, NameRegistrar::MODE_REDUNDANCY, 0xaabbe1);  
   idx=0xaabbe2;
   for(i=0;i<30;i++) {
     threadNameAppendGet(name2, NameRegistrar::MODE_ROUND_ROBIN, idx);     
     idx+=2;
   }   
   //threadNameAppendGet(name2, NameRegistrar::MODE_PREFER_LOCAL, 0xaabbec);
   //threadNameAppendGet(name2, NameRegistrar::MODE_REDUNDANCY, 0xaabbee);
#endif
   name.dump();   
   return 0;
}
