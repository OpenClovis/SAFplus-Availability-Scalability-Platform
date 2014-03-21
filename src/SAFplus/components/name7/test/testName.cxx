#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include <clNameApi.hxx>
#include <stdio.h>
using namespace SAFplus;
using namespace SAFplusI;

extern NameRegistrar name;
//NameRegistrar gname;
//NameRegistrar& gname = NameRegistrar::getInstance();

void testNameSetGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   Handle h2(PointerHandle, idx+1, process, node);
   //const char* xy = "_111000111";
   name.set(n, h, m);
   name.append(n,h2,NameRegistrar::MODE_NO_CHANGE);
   try {
     SAFplus::Handle oh = name.getHandle(n);
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException ne) {
      printf("Error [%s]\n", ne.what());     
   }
   try {
     SAFplus::Handle oh = name.getHandle("blah");
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException ne) {
      printf("Error [%s]\n", ne.what());     
   }   
}

void testNameAppend(const char* n, NameRegistrar::MappingMode m, uint32_t idx, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   Handle h2(PointerHandle, idx+1, process, node);
   name.append(n,h,m); 
   name.append(n,h2,m);
   try {
     SAFplus::Handle oh = name.getHandle(n);
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException ne) {
      printf("Error [%s]\n", ne.what());     
   }
   try {
     SAFplus::Handle oh = name.getHandle("blah");
     //char buf[100];
     printf("Handle got [0x%x.0x%x]\n", oh.id[0],oh.id[1]);
   }catch (NameException ne) {
      printf("Error [%s]\n", ne.what());     
   }   
}

int main(int argc, char* argv[])
{
#if 1
   testNameSetGet("_111000111", NameRegistrar::MODE_REDUNDANCY, 0xaabbcc);
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbdd);
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbe0);
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbe2);
   pid_t thispid = getpid();
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbe4, (uint32_t)thispid);
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbe4, 0xffffffff, 2);
#endif

#if 0
   testNameSetGet("_111000111", NameRegistrar::MODE_ROUND_ROBIN, 0xaabbcc);
   testNameAppend("_111000111", NameRegistrar::MODE_PREFER_LOCAL, 0xaabbdd);
   testNameAppend("_111000111", NameRegistrar::MODE_REDUNDANCY, 0xaabbe0);
   testNameAppend("_111000111", NameRegistrar::MODE_NO_CHANGE, 0xaabbe2);
   pid_t thispid = getpid();
   testNameAppend("_111000111", NameRegistrar::MODE_PREFER_LOCAL, 0xaabbe4, (uint32_t)thispid);
   testNameAppend("_111000111", NameRegistrar::MODE_PREFER_LOCAL, 0xaabbe4, 0xffffffff, 2);
#endif
   name.dump();
   return 0;
}
