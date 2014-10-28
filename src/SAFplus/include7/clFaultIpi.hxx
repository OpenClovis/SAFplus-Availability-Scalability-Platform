#ifndef CLFAULT_IPI_H_
#define CLFAULT_IPI_H_

#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

#include <clMsgApi.hxx>
#include <clGroup.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clFaultApi.hxx>

namespace SAFplusI
{
    enum class FaultMessageTypeT
    {
        MSG_ENTITY_JOIN = 1,     // Happens first time the entity joins the fault
        MSG_ENTITY_LEAVE,
        MSG_ENTITY_FAULT,
        MSG_UNDEFINED
    };
    enum class FaultMessageSendMode
    {
        SEND_BROADCAST,
        SEND_TO_SERVER,
        SEND_LOCAL_ROUND_ROBIN, //Round Robin
    };



};

namespace SAFplus
{
#define FAULT_NAME_LEN 100
#define MAX_FAULT_DEPENDENCIES 100
//typedef int FaultState;
	enum class FaultState
	{
		STATE_UNDEFINED = 0,
		STATE_UP = 1,
		STATE_DOWN
	};

	void faultInitialize(void);

    class FaultMessageProtocol
    {
    public:
        SAFplus::Handle       fault;
        SAFplusI::FaultMessageTypeT     messageType;
        bool                  force; //When role type change, force receiver to apply new roles without checking
        FaultMessageProtocol();
        char name[FAULT_NAME_LEN];
        SAFplus::FaultState state;
        FaultEventData data;
        SAFplus::FaultPolicy pluginId;
        SAFplus::Handle       faultEntity;

    };

    class FaultShmEntry
    {
    public :
        SAFplus::Handle faultHdl;
        int dependecyNum;
        char name[FAULT_NAME_LEN];  // For display purposes only, string lookups go through the Name server.
        SAFplus::Handle depends[MAX_FAULT_DEPENDENCIES];  // If this entity fails, all entities in this array will also fail.
        SAFplus::FaultState state;  //Fault state of an entity
        void init(SAFplus::Handle fHandle,FaultShmEntry* frp)
        {
            faultHdl=fHandle;
            dependecyNum=frp->dependecyNum;
            state=frp->state;
            strncpy(name,frp->name,FAULT_NAME_LEN);
        }
    };

    typedef std::pair<const SAFplus::Handle,FaultShmEntry> FaultShmMapPair;
    typedef boost::interprocess::allocator<FaultShmEntry, boost::interprocess::managed_shared_memory::segment_manager> FaultEntryAllocator;
    typedef boost::unordered_map < SAFplus::Handle, FaultShmEntry, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle>,FaultEntryAllocator> FaultShmHashMap;


    class FaultSharedMem
    {
    public:
        #define FAULT_NAME_LEN 100
        SAFplus::ProcSem mutex;  // protects the shared memory region from simultaneous access
        SAFplus::Mutex  localMutex;
        boost::interprocess::managed_shared_memory faultMsm;
        SAFplus::FaultShmHashMap* faultMap;
        void init();
        void clear();
        void remove(SAFplus::Handle handle);
        void dbgDump(void);
        void dispatcher(void);
        bool createFault(FaultShmEntry* frp,SAFplus::Handle fault);
        bool updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault);
        bool updateFaultHandleState(SAFplus::FaultState state,SAFplus::Handle fault);
    };


    class Fault
    {
    public :

        SAFplus::Handle                   handle;               // handle for identify a fault entity
        SAFplus::SafplusMsgServer*        faultMsgServer;       //safplus message for send fault notification to fault server
        SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
        char                              name[FAULT_NAME_LEN]; // name of fault entity
        ClIocAddressT iocFaultServer;
        int faultCommunicationPort;
        typedef SAFplus::FaultShmMapPair KeyValuePair;

        // register a fault entity to fault server
        void sendFaultAnnounceMessage();
        // send a fault entity to fault server
        void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageTypeT msgType,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId);
        // fill and send a fault event to fault server or broadcast
        void fillAndSendMessage(void* data, SAFplusI::FaultMessageTypeT msgType,SAFplusI::FaultMessageSendMode msgSendMode,bool forcing=false);
        //send a fault notification to fault server
        void sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode);
        friend class SAFplusI::GroupSharedMem;
        //init a fault entity with handle and comport information
        void init(SAFplus::Handle faultHandle,ClIocAddressT faultServer, int comPort,SAFplus::Wakeable& execSemantics);
        void setName(const char* entityName);
        Fault(SAFplus::Handle faultHandle,const char* name, int comPort,ClIocAddressT iocServerAddress);
        Fault();
        SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);
    };

    class FaultServer:public SAFplus::MsgHandler
    {
    public:
        FaultServer();
        //shared memory
        FaultSharedMem fsmServer;
        SAFplus::SafplusMsgServer   *faultMsgServer;
        int faultCommunicationPort;
        void init();
        virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
        //handle fault message from fault client
        void faultEventHandle();
        //register a fault client entity
        void RegisterFaultEntity(FaultShmEntry* frp, SAFplus::Handle faultClient,bool needNotify);
        //Deregister a fault client entity
        void removeFaultEntity(SAFplus::Handle faultClient,bool needNotify);
        //Set dependency
        bool setDependency(SAFplus::Handle dependencyHandle, SAFplus::Handle faultHandle);
        //Remove dependency
        bool removeDependency(SAFplus::Handle dependencyHandle,SAFplus::Handle faultHandle);
        // set name for fault client entity
        void setName(SAFplus::Handle faultHandle, const char* name);
        //process a fault event
        void processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault,SAFplus::Handle faultEntity);
        //broadcast fault event to all other node
        void sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode);
        void sendFaultAnnounceMessage(SAFplusI::FaultMessageSendMode messageMode, SAFplus::Handle handle);
        void sendFaultLeaveMessage(SAFplusI::FaultMessageSendMode messageMode, SAFplus::Handle handle);
        SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);

    };
};

#endif
