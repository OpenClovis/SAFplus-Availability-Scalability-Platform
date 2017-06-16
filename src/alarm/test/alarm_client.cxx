#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCkptApi.hxx>
#include <clTestApi.hxx>

#include <StorageAlarmData.hxx>
#include <AlarmData.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>

#include "test.hxx"

using namespace std;
using namespace SAFplus;

//ClUint32T clAspLocalId = 0x1;
void tressTest(int eventNum);
void deRegisterTest();
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;
void writeObjToCkpt(Checkpoint& c, const char* key, const void* obj, int objlen);
void deleteCheckpoint(Handle& ckptHdl);
void persistentWithObjects();
void persistentWithObjects1();
void persistentWithObjects2();
void persistentWithObjects_AlarmInfo();
void persistentWithObjects_AlarmData();
void persistentWithObjects_StorageAlarmData();
/*ClBoolT   gIsNodeRepresentative = false;
SafplusInitializationConfiguration sic;
SAFplus::Handle faultEntityHandle;
SAFplus::Handle me;
Fault fc;*/

void testAllFeature();
#define FAULT_CLIENT_PID 200
#define FAULT_ENTITY_PID 201
int main(int argc, char* argv[])
{
   /* logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;
    sic.iocPort     = 50;
    safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
    logSeverity = LOG_SEV_MAX;
    logInfo("FLT","CLT","********************Start msg server********************");
    safplusMsgServer.Start();
    logInfo("FLT","CLT","********************Initial fault lib********************");
    faultInitialize();
    me = getProcessHandle(FAULT_CLIENT_PID,SAFplus::ASP_NODEADDR);
    faultEntityHandle = getProcessHandle(FAULT_ENTITY_PID,SAFplus::ASP_NODEADDR);
    logInfo("FLT","CLT","********************Initial fault client*********************");
    fc.init(me,INVALID_HDL,sic.iocPort,BLOCK);
    //fc.registerFault();
    testAllFeature();
    tressTest(5);
    sleep(1);
    deRegisterTest();
    while(0)
    {
        sleep(10000);
    }*/

	testAllFeature();
    return 0;
}

void testAllFeature()
{

    /*sleep(2);
    FaultState state = fc.getFaultState(me);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    // Register other fault entity
    logInfo("FLT","CLT","Register fault entity");
    sleep(2);
    fc.registerEntity(faultEntityHandle,FaultState::STATE_DOWN);
    sleep(2);
    FaultState state1 = fc.getFaultState(faultEntityHandle);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state1)]);
    sleep(2);
    logInfo("FLT","CLT","Update fault entity");
    fc.registerEntity(faultEntityHandle,FaultState::STATE_UP);
    sleep(2);
    state = fc.getFaultState(faultEntityHandle);
    logInfo("FLT","CLT","Get current fault state after updated in shared memory [%s]", strFaultEntityState[int(state)]);
    sleep(10);
    FaultEventData faultData;
    faultData.state=SAFplus::AlarmState::ALARM_STATE_INVALID;
    faultData.category=SAFplus::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS;
    faultData.cause=SAFplus::AlarmProbableCause::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
    faultData.severity=SAFplus::AlarmSeverity::ALARM_SEVERITY_MINOR;
    state = fc.getFaultState(me);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    logInfo("FLT","CLT","Send fault event to local fault server");
    fc.notify(faultData,SAFplus::FaultPolicy::Custom);
    sleep(10);
    logInfo("FLT","CLT","Send fault event to fault server");
    fc.notify(faultData,SAFplus::FaultPolicy::AMF);
    sleep(10);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    state = fc.getFaultState(me);
    logInfo("FLT","CLT","Send fault event of fault entity fault server");
    fc.notify(faultEntityHandle,faultData,SAFplus::FaultPolicy::Custom);
    sleep(1);*/
	//persistentWithObjects2();
	//persistentWithObjects_AlarmInfo();
	//persistentWithObjects_AlarmData();
	persistentWithObjects_StorageAlarmData();
}

void tressTest(int eventNum)
{/*
    for (int i = 0; i < eventNum; i ++)
    {
        FaultEventData faultData;
        faultData.state=SAFplus::AlarmState::ALARM_STATE_INVALID;
        faultData.category=SAFplus::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS;
        faultData.cause=SAFplus::AlarmProbableCause::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
        faultData.severity=SAFplus::AlarmSeverity::ALARM_SEVERITY_MINOR;
        fc.notify(faultData,FaultPolicy::AMF);
        sleep(1);
    }*/
}

void deRegisterTest()
{
    /*logInfo("FLT","CLT","Deregister fault entity");
    fc.deRegister(faultEntityHandle);
    sleep(2);
    logInfo("FLT","CLT","Deregister fault client");
    fc.deRegister();*/
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
/*
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
    std::cout<<"Persistent object verification key1"<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  const Buffer& buf3 = c2.read(key3);
  if (&buf3)
  {
    Obj* po3 = (Obj*)buf3.data;
    clTest(("Persistent object verification"),(*po3 == o3),(" "));
    std::cout<<"Persistent object verification key3"<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  std::cout<<"testcase end!"<<std::endl;
  clTestCaseEnd((" "));
}



void persistentWithObjects1()
{
  clTestCaseStart(("CKP-PST-FNC.TC010: Persistent objects"));
  class Obj
  {
  public:
    Obj() : d1("abc") {
    	for(int i =0;i < 2;i++)
    		d2.push_back("xyz");
    }
    Obj(std::string  _d1, std::string _d2) : d1(_d1) {
    	for(int i =0;i < 2;i++)
    	    		d2.push_back(_d2);
    }
    bool operator==(const Obj& o) const
    {
      return (d1 == o.d1 && d2 == o.d2);
    }
    Obj& operator=(Obj other)
       {
           std::cout << "copy assignment of A\n";
           d1 = other.d1;
           d2 = other.d2;
           //d2[1] = other.d2[1];
           return *this;
       }
  //public:
    std::string d1;
    std::vector<std::string> d2;
  };
  Obj o1;
  //const Obj o2;
  Obj o3("efgdddddddddddddddd","mnhssssssssssssssssssssssss") ;
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
    std::cout<<"Persistent object verification key1"<<std::endl;
    std::cout<<"result:"<<po1->d1<<" "<<std::endl;
    std::cout<<"thoi"<<std::endl;
    for(std::string i: po1->d2)
    {
    	std::cout<<i<<" ";
    }
    std::cout<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  const Buffer& buf3 = c2.read(key3);
  if (&buf3)
  {
    Obj* po3 = (Obj*)buf3.data;
    //std::cout<<"result:"<<*po3<<std::endl;
    clTest(("Persistent object verification"),(*po3 == o3),(" "));
    std::cout<<"Persistent object verification key3"<<std::endl;
    std::cout<<"result:"<<po3->d1<<" "<<std::endl;
    for(std::string i:po3->d2)
        {
        	std::cout<<i<<" ";
        }
        std::cout<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  std::cout<<"testcase end!"<<std::endl;
  clTestCaseEnd((" "));
}
void persistentWithObjects2()
{
  clTestCaseStart(("CKP-PST-FNC.TC010: Persistent objects"));
  class Obj
  {
  public:
    Obj() : d1("abc") {
    	for(int i =0;i < 2;i++)
    		d2[d1]="xyz";
    }
    Obj(std::string  _d1, std::string _d2) : d1(_d1) {
    	d2[_d1] = _d2;
    	d2["test"] = _d2;
    }
    bool operator==(const Obj& o) const
    {
      return (d1 == o.d1 && d2 == o.d2);
    }
    Obj& operator=(Obj other)
       {
           std::cout << "copy assignment of A\n";
           d1 = other.d1;
           d2 = other.d2;
           //d2[1] = other.d2[1];
           return *this;
       }
  //public:
    std::string d1;
    boost::unordered_map<std::string,std::string> d2;
  };
  Obj o1;
  //const Obj o2;
  Obj o3("efgdddddddddddddddd","mnhssssssssssssssssssssssss") ;
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
    std::cout<<"Persistent object verification key1"<<std::endl;
    std::cout<<"result:"<<po1->d1<<" "<<std::endl;
    std::cout<<"thoi"<<std::endl;
    boost::unordered::unordered_map<std::string,std::string>::iterator it;
        for (it=po1->d2.begin();it!=po1->d2.end();++it)
            std::cout << it->first <<":" << it->second << std::endl;

    std::cout<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  const Buffer& buf3 = c2.read(key3);
  if (&buf3)
  {
    Obj* po3 = (Obj*)buf3.data;
    //std::cout<<"result:"<<*po3<<std::endl;
    clTest(("Persistent object verification"),(*po3 == o3),(" "));
    std::cout<<"Persistent object verification key3"<<std::endl;
    std::cout<<"result:"<<po3->d1<<" "<<std::endl;
    boost::unordered::unordered_map<std::string,std::string>::iterator it;
            for (it=po3->d2.begin();it!=po3->d2.end();++it)
                std::cout << it->first <<":" << it->second << std::endl;
        std::cout<<std::endl;
  }
  else
  {
    clTestFailed(("Expect non-null buffer but actually a null one was gotten\n"));
    std::cout<<"Expect non-null buffer but actually a null one was gotten\n"<<std::endl;
  }
  std::cout<<"testcase end!"<<std::endl;
  clTestCaseEnd((" "));
}
*/
void persistentWithObjects_StorageAlarmData()
{
  clTestCaseStart(("CKP-PST-FNC.TC010: persistentWithObjects_StorageAlarmData objects"));
  class Obj
  {
  public:
    Obj() {
    	    }
    void test()
    {
    	// id of resource is unique in safplus system
		d2.isUpdate = true;
		AlarmKey key0("test0",AlarmCategory::COMMUNICATIONS,AlarmProbableCause::LOSS_OF_SIGNAL);
		AlarmProfileInfo profile0 (0,true,1,1,AlarmRuleRelation::LOGICAL_OR,1,AlarmRuleRelation::LOGICAL_OR,1,0,1);
		AlarmInfo info0;
		std::vector<std::string> vectAltResource0;
		vectAltResource0.push_back("alttest00");
		vectAltResource0.push_back("alttest01");
		info0.vectAltResource = vectAltResource0;
		info0.statusChangeTime = second_clock::local_time();
		info0.severity = AlarmSeverity::WARNING;
		info0.strText = "strText";
		info0.strOperator = "strOperator";
		info0.strOperatorText = "strOperatorText";
		info0.operatorActionTime = second_clock::local_time();
		info0.specificProblem = 0;
		info0.state = AlarmState::ASSERT;
		info0.strOperatorText = "strOperatorText";
		info0.strOperatorText = "strOperatorText";
		info0.strOperatorText = "strOperatorText";
		info0.sharedAssertTimer = nullptr;
		info0.sharedClearTimer = nullptr;
		info0.sharedAlarmData = nullptr;
		std::cout<<"before updateAlarmInfo"<<std::endl;
		d2.updateAlarmProfileInfo(key0,profile0);
		d2.updateAlarmInfo(key0,info0);
		std::cout<<"profile0:"<<profile0.toString()<<std::endl;
		std::cout<<"info0:"<<info0.toString()<<std::endl;
		// id of resource is unique in safplus system
		AlarmKey key1("test1",AlarmCategory::QUALITYOFSERVICE,AlarmProbableCause::LOSS_OF_FRAME);
		AlarmProfileInfo profile1 (0,true,1,1,AlarmRuleRelation::LOGICAL_OR,1,AlarmRuleRelation::LOGICAL_OR,1,0,1);
		AlarmInfo info1;
		std::vector<std::string> vectAltResource1;
		vectAltResource1.push_back("alttest10");
		vectAltResource1.push_back("alttest11");
		info1.vectAltResource = vectAltResource1;
		info1.statusChangeTime = second_clock::local_time();
		info1.severity = AlarmSeverity::MINOR;
		info1.strText = "strText1";
		info1.strOperator = "strOperator1";
		info1.strOperatorText = "strOperatorText1";
		info1.operatorActionTime = second_clock::local_time();
		info1.specificProblem = 1;
		info1.state = AlarmState::CLEAR;
		info1.strOperatorText = "strOperatorText1";
		info1.strOperatorText = "strOperatorText1";
		info1.strOperatorText = "strOperatorText1";
		info1.sharedAssertTimer = nullptr;
		info1.sharedClearTimer = nullptr;
		info1.sharedAlarmData = nullptr;
		std::cout<<"info0:"<<info1.toString()<<std::endl;
		d2.updateAlarmProfileInfo(key1,profile1);
		d2.updateAlarmInfo(key1,info1);
		std::cout<<"profile1:"<<profile1.toString()<<std::endl;
		std::cout<<"info1:"<<info1.toString()<<std::endl;
    }
    void initialize()
    {
    	d2.initialize();
    }
    void printAllData()
    {
    	d2.updateSummary();
    	d2.printAllData();
    }
    StorageAlarmData d2;
  };
  std::cout<<"creation"<<std::endl;
  Obj objStorage;
  std::cout<<"init"<<std::endl;
  objStorage.initialize();
  std::cout<<"test"<<std::endl;
  objStorage.test();//initdata
  std::cout<<"printAllData"<<std::endl;
  objStorage.printAllData();
  std::cout<<"4"<<std::endl;
}

