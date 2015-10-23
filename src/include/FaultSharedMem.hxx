#ifndef CLFAULT_SHM_H_
#define CLFAULT_SHM_H_

#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

#include <ctime>
#include <clMsgApi.hxx>
#include <clGroup.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clGroup.hxx>
#include "../fault/server/FaultEnums/FaultAlarmState.hxx"
#include "../fault/server/FaultEnums/AlarmCategory.hxx"
#include "../fault/server/FaultEnums/FaultProbableCause.hxx"
#include "../fault/server/FaultEnums/FaultSeverity.hxx"
#include "../fault/server/FaultEnums/FaultMessageType.hxx"
#include "../fault/server/FaultEnums/FaultState.hxx"

namespace SAFplus
  {
  //comunication mode between fault entity and fault server
  enum class FaultMessageSendMode
    {
    SEND_BROADCAST,// send broadcast
    SEND_TO_LOCAL_SERVER,// send to local fault server (current)
    SEND_TO_ACTIVE_SERVER // send directly to master fault server
    };

  /**
   * The type of an identifier to the specific problem of the alarm.
   * This information is not configured but is assigned a value at run-time
   * for segregation of alarms that have the same \e category and probable cause
   * but are different in their manifestation.
   */
  typedef int AlarmSpecificProblemT;

  //fault policy plugin
  enum class FaultPolicy
    {
      Undefined = 0,
      Custom = 1,
      AMF = 2,
    };

  // fault event information include state , severity, category, cause
  class FaultEventData
    {
  public:
    /**
     * Flag to indicate if the alarm was for assert or for clear.
     */
    FaultEnums::FaultAlarmState alarmState;

    /**
     * The category of the fault event.
     */
    FaultEnums::AlarmCategory category;

    /**
     * The severity of the fault event.
     */
    FaultEnums::FaultSeverity severity;

    /**
     * The probable cause of the fault event.
     */
    FaultEnums::FaultProbableCause cause;

    FaultEventData()
      {
      alarmState= FaultEnums::FaultAlarmState::ALARM_STATE_INVALID;
      category= FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID;
      cause = FaultEnums::FaultProbableCause::ALARM_ID_INVALID;
      severity = FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR;
      }
    FaultEventData(FaultEnums::FaultAlarmState a_state, FaultEnums::AlarmCategory a_category,FaultEnums::FaultSeverity a_severity,FaultEnums::FaultProbableCause a_cause)
      {
        init(a_state,a_category,a_severity,a_cause);
      }

    void init(FaultEnums::FaultAlarmState a_state,FaultEnums::AlarmCategory a_category,FaultEnums::FaultSeverity a_severity,FaultEnums::FaultProbableCause a_cause)
      {
      alarmState= a_state;
      category= a_category;
      cause= a_cause;
      severity=a_severity;
      }
    FaultEventData& operator=(const FaultEventData & c)
      {
      alarmState= c.alarmState;
      category= c.category;
      cause= c.cause;
      severity=c.severity;
      return *this;
      }

    };

  //? state of entity -- up down or unknown
  enum class FaultState
    {
      STATE_UNDEFINED = 0,  //? Unknown state
        STATE_UP = 1,  //? Entity is working
        STATE_DOWN  //? Entity has failed
    };

  enum
    {
    CL_FAULT_BUFFER_HEADER_STRUCT_ID_7 = 0x5940000,
    };

  // share memory include fault entity information and master fault server address
  class FaultShmHeader
    {
  public:
    uint64_t       structId;
    SAFplus::Handle activeFaultServer;
    };
  void faultInitialize(void);

  class FaultMessageProtocol
    {
  public:
    FaultEnums::FaultMessageType messageType;
    SAFplus::FaultState state;
    FaultPolicy pluginId;
    FaultEventData data;
    SAFplus::Handle       reporter;
    SAFplus::Handle       faultEntity;
    char                  syncData[1];
    FaultMessageProtocol()
      {
      messageType = FaultEnums::FaultMessageType::MSG_UNDEFINED;
      faultEntity=SAFplus::INVALID_HDL;
      data.init(FaultEnums::FaultAlarmState::ALARM_STATE_INVALID,FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID,FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR,FaultEnums::FaultProbableCause::ALARM_ID_INVALID);
      pluginId=FaultPolicy::Undefined;
      }
    };


  class FaultShmEntry
    {
  public :
    SAFplus::Handle faultHdl;
    int dependecyNum;
    SAFplus::Handle depends[SAFplusI::MAX_FAULT_DEPENDENCIES];  // If this entity fails, all entities in this array will also fail.
    SAFplus::FaultState state;  //Fault state of an entity
    FaultShmEntry()
      {
      dependecyNum = 0;
      state=SAFplus::FaultState::STATE_UNDEFINED;
      }
    void init(SAFplus::Handle fHandle,FaultShmEntry* frp)
      {
      faultHdl=fHandle;
      dependecyNum=frp->dependecyNum;
      state=frp->state;
      for(int i=0;i<SAFplusI::MAX_FAULT_DEPENDENCIES;i++)
        {
        depends[i]=SAFplus::INVALID_HDL;
        }
      };
    void init(SAFplus::Handle fHandle)
      {
      faultHdl = fHandle;
      dependecyNum = 0;
      for(int i=0;i<SAFplusI::MAX_FAULT_DEPENDENCIES;i++)
        {
        depends[i]=SAFplus::INVALID_HDL;
        }
      };


    };

  class FaultEntryData
    {
  public :
    SAFplus::Handle faultHdl;
    int dependecyNum;
    SAFplus::Handle depends[SAFplusI::MAX_FAULT_DEPENDENCIES];  // If this entity fails, all entities in this array will also fail.
    SAFplus::FaultState state;  //Fault state of an entity

    };


  typedef std::pair<const SAFplus::Handle,FaultShmEntry> FaultShmMapPair;
  typedef boost::interprocess::allocator<FaultShmEntry, boost::interprocess::managed_shared_memory::segment_manager> FaultEntryAllocator;
  typedef boost::unordered_map < SAFplus::Handle, FaultShmEntry, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle>,FaultEntryAllocator> FaultShmHashMap;


  class FaultSharedMem
    {
  public:
    SAFplus::ProcSem mutex;  // protects the shared memory region from simultaneous access
    SAFplus::Mutex  faultMutex;
    boost::interprocess::managed_shared_memory faultMsm;
    std::string faultSharedMemoryObjectName; // Separate memory object name for a nodeID to support multi-node running
    FaultShmHashMap* faultMap;
    FaultShmHeader* faultHdr;
    SAFplus::Mutex  localMutex;
    void init(void);
    void setActive(SAFplus::Handle active);
    void clientInit();
    void clear();
    void remove(SAFplus::Handle handle);
    void removeAll();
    void dbgDump(void);
    void dispatcher(void);
    bool createFault(FaultShmEntry* frp,SAFplus::Handle fault);
    bool updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault);
      bool updateFaultHandleState(SAFplus::Handle fault, SAFplus::FaultState state);
    //? copy all data (up to size bufSize) into buf. Returns the number of records copied.
    uint_t getAllFaultClient(char* buf,ClWordT bufSize);
    void applyFaultSync(char* buf,ClWordT bufSize);
    };

  class FaultGroupData
    {
  public:
    unsigned int       structId;
    SAFplus::Handle    iocFaultServer;
    };

  }

#endif
