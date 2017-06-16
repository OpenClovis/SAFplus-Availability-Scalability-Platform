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
#include <clGroupApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clGroupApi.hxx>
#include <AlarmCategory.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmSeverity.hxx>
#include <AlarmState.hxx>

using namespace SAFplusAlarm;

namespace SAFplus
  {
  enum class FaultMessageType
    {
    MSG_ENTITY_JOIN = 1,     // the entity joins
    MSG_ENTITY_LEAVE,       // the entity leave
    MSG_ENTITY_FAULT,       // the fault event
    MSG_ENTITY_STATE_CHANGE,       // the fault event
    MSG_ENTITY_JOIN_BROADCAST,  // broadcast entity join
    MSG_ENTITY_LEAVE_BROADCAST,  // broadcast entity leave
    MSG_ENTITY_FAULT_BROADCAST,   // broadcast fault event
    MSG_ENTITY_STATE_CHANGE_BROADCAST,
    MSG_UNDEFINED
    };

  //comunication mode between fault entity and fault server
  enum class FaultMessageSendMode
    {
    SEND_BROADCAST,// send broadcast
    SEND_TO_LOCAL_SERVER,// send to local fault server (current)
    SEND_TO_ACTIVE_SERVER // send directly to master fault server
    };

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
    AlarmState state;

    /**
     * The category of the fault event.
     */
    AlarmCategory category;

    /**
     * The severity of the fault event.
     */
    AlarmSeverity severity;
    /**
     * The probable cause of the fault event.
     */
    AlarmProbableCause cause;

    FaultEventData()
      {
      state= AlarmState::INVALID;
      category= AlarmCategory::INVALID;
      cause= AlarmProbableCause::INVALID;
      severity=AlarmSeverity::INVALID;
      }
    FaultEventData(AlarmState a_state,AlarmCategory a_category,AlarmSeverity a_severity,AlarmProbableCause a_cause)
      {
        init(a_state,a_category,a_severity,a_cause);
      }

    void init(AlarmState a_state,AlarmCategory a_category,AlarmSeverity a_severity,AlarmProbableCause a_cause)
      {
      state= a_state;
      category= a_category;
      cause= a_cause;
      severity=a_severity;
      }
    FaultEventData& operator=(const FaultEventData & c)
      {
      state= c.state;
      category= c.category;
      cause= c.cause;
      severity=c.severity;
      return *this;
      }
      bool operator==(const FaultEventData & c) const
      {
        return ((state == c.state)&&
                (category == c.category)&&
                (cause == c.cause)&&
                (severity == c.severity));
      }

    };

  //? state of entity -- up down or unknown
  enum class FaultState
    {
      STATE_UNDEFINED = 0,  //? Unknown state
      STATE_UP = 1,  //? Entity is working
      STATE_DOWN  //? Entity has failed
    };

  const char* c_str(FaultState fs);

  enum
    {
    CL_FAULT_BUFFER_HEADER_STRUCT_ID_7 = 0x5940000,
    };

  // share memory include fault entity information and master fault server address
  class FaultShmHeader
    {
  public:
    uint64_t        structId;  //? Unique number identfying this as fault related data
    SAFplus::Handle activeFaultServer;
    uint64_t        lastChange; //? monotonically increasing number indicating the last time a change was made to fault
    };
  void faultInitialize(void);

  class FaultMessageProtocol
    {
  public:
    SAFplus::FaultMessageType     messageType;
    SAFplus::FaultState state;
    FaultPolicy pluginId;
    FaultEventData data;
    SAFplus::Handle       reporter;
    SAFplus::Handle       faultEntity;
    char                  syncData[1];
    FaultMessageProtocol()
      {
      messageType=SAFplus::FaultMessageType::MSG_UNDEFINED;
      faultEntity=SAFplus::INVALID_HDL;
      data.init(AlarmState::INVALID,AlarmCategory::INVALID,AlarmSeverity::INVALID,AlarmProbableCause::INVALID);
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
    uint64_t        lastChange; //? monotonically increasing number indicating the last time a change was made to fault
    FaultShmEntry()
      {
      dependecyNum = 0;
      state=SAFplus::FaultState::STATE_UNDEFINED;
      lastChange = 0;
      }
    void init(SAFplus::Handle fHandle,FaultShmEntry* frp)
      {
      faultHdl=fHandle;
      dependecyNum=frp->dependecyNum;
      state=frp->state;
      lastChange = frp->lastChange;
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

    class FaultEntryData // passed in messages
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
      //SAFplus::Mutex  localMutex;
    void init(void);
    void setActive(SAFplus::Handle active);
    void clientInit();
    void clear();
    void remove(SAFplus::Handle handle);
    void removeAll();
    void dbgDump(void);
    void dispatcher(void);
    uint64_t lastChange(void);
    void changed(void);  // private: Indicates that the fault state has changed so update the lastchange count, etc 
    bool createFault(FaultShmEntry* frp,SAFplus::Handle fault);
    bool updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault);
      bool updateFaultHandleState(SAFplus::Handle fault, SAFplus::FaultState state);
    //? copy all data (up to size bufSize) into buf. Returns the number of records copied.
    uint_t getAllFaultClient(char* buf,ClWordT bufSize);
    void applyFaultSync(char* buf,ClWordT bufSize);

    void setChildFaults(SAFplus::Handle handle,SAFplus::FaultState state); //? Sets all handles that are contained within the entiy that this handler references to the provided state.
    };

  class FaultGroupData
    {
  public:
    unsigned int       structId;
    SAFplus::Handle    iocFaultServer;
    };

  }

#endif
