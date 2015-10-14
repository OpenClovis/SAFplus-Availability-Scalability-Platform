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
#include "../fault/server/IANAITUALARMTCMIB/ProbableCause.hxx"
#include "../fault/server/IANAITUALARMTCMIB/IANAItuEventType.hxx"
#include "../fault/server/ITUALARMTCMIB/ItuPerceivedSeverity.hxx"

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

  /**
   * The type of an identifier to the specific problem of the alarm.
   * This information is not configured but is assigned a value at run-time
   * for segregation of alarms that have the same \e category and probable cause
   * but are different in their manifestation.
   */
  typedef int AlarmSpecificProblemT;

  /**
   * The enumeration to depict the state of the alarm that is
   * into. It can be in any of the following state that is
   * cleared, assert, suppressed or under soaking.
   */
  enum class AlarmState
    {
    /**
     *  The alarm condition has cleared.
     */
    ALARM_STATE_CLEAR   =   0,
    /**
     *  The alarm condition has occured.
     */
      ALARM_STATE_ASSERT  =   1,
    /**
     * The alarm state is suppressed.
     */
      ALARM_STATE_SUPPRESSED = 2,
    /**
     * The alarm state is under soaking.
     */
      ALARM_STATE_UNDER_SOAKING = 3,
    /**
     * Invalid alarm state.
     */
      ALARM_STATE_INVALID = 4

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
    SAFplus::AlarmState alarmState;

    /**
     * The category of the fault event.
     */
    IANAITUALARMTCMIB::IANAItuEventType category;

    /**
     * The severity of the fault event.
     */
    ITUALARMTCMIB::ItuPerceivedSeverity severity;

    /**
     * The probable cause of the fault event.
     */
    IANAITUALARMTCMIB::ProbableCause cause;

    FaultEventData()
      {
      alarmState= SAFplus::AlarmState::ALARM_STATE_INVALID;
      category= IANAITUALARMTCMIB::IANAItuEventType::other;
      cause = IANAITUALARMTCMIB::ProbableCause::aIS;
      severity = ITUALARMTCMIB::ItuPerceivedSeverity::cleared;
      }
    FaultEventData(SAFplus::AlarmState a_state,IANAITUALARMTCMIB::IANAItuEventType a_category,ITUALARMTCMIB::ItuPerceivedSeverity a_severity,IANAITUALARMTCMIB::ProbableCause a_cause)
      {
        init(a_state,a_category,a_severity,a_cause);
      }

    void init(SAFplus::AlarmState a_state,IANAITUALARMTCMIB::IANAItuEventType a_category,ITUALARMTCMIB::ItuPerceivedSeverity a_severity,IANAITUALARMTCMIB::ProbableCause a_cause)
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
      data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,IANAITUALARMTCMIB::IANAItuEventType::other,ITUALARMTCMIB::ItuPerceivedSeverity::cleared,IANAITUALARMTCMIB::ProbableCause::aIS);
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
