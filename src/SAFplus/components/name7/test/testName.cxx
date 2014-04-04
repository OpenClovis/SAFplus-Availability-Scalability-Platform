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


void testNameSetGet(const char* n, NameRegistrar::MappingMode m, uint32_t idx, void* obj = NULL, uint32_t process=0xffffffff, uint32_t node=0xffff)
{
   Handle h(PointerHandle, idx, process, node);
   Handle h2(PointerHandle, idx+1, process, node);
   //const char* xy = "_111000111";
   name.set(n, h, m, obj);
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
   }catch (NameException ne) {
      printf("Exception [%s]\n", ne.what());
   }
   
   try {
     ObjMapPair ho = name.get(name2);
     ObjTest* po = (ObjTest*)ho.second;     
     po->greet();
   }catch (NameException ne) {
      printf("Exception [%s]\n", ne.what());
   }

   char data[120];
   strcpy(data, "This is arbitrary data example. Don't care its content");
   try {
      name.set(name1, data, strlen(data)+1);
      strcpy(data, "This is another arbitrary data example. Don't care its content!...");
      name.set(name2, data, strlen(data)+1);
   }catch (NameException ne) {
       printf("Exception [%s]\n", ne.what());
   }
   
   try {
      SAFplus::Buffer& buf = name.getData(name1);
      printf("name [%s]: buf [%s]\n", name1, buf.data);
   }catch (NameException ne) {
      printf("Exception [%s]", ne.what());
   }   
   try {
      SAFplus::Buffer& buf = name.getData(name2);
      printf("name [%s]: buf [%s]\n", name2, buf.data);
   }catch (NameException ne) {
      printf("Exception [%s]", ne.what());
   }
   char modbuf[] = {"This is the modified buffer"};
   size_t sz = strlen(modbuf)+sizeof(SAFplus::Buffer);
   char temp[sz];
   SAFplus::Buffer* pbuf = new(temp) SAFplus::Buffer(sz);
   *pbuf = modbuf;
   try {
      name.set(name1, pbuf);
   }catch (NameException ne) {
      printf("Exception [%s]", ne.what()); 
   }
   try {
      SAFplus::Buffer& buf = name.getData(name1);
      printf("name [%s]: buf [%s]\n", name1, buf.data);
      buf.decRef();
   }catch (NameException ne) {
      printf("Exception [%s]", ne.what());
   }
   
#endif
   //char data[120];
#if 1
   strcpy(data, "This is arbitrary data example. Don't care its content");
   try {
      name.set("aaaa", data, strlen(data)+1);
   }catch (NameException ne) {
      printf("Exception [%s]\n", ne.what());
   }
   SAFplus::Buffer buf;
   try {
      name.set("aaaa", &buf);
   }catch (NameException ne) {
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

   name.dump();   
   return 0;
}
