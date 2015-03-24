#pragma once

#include <FaultSharedMem.hxx>
#include <FaultStatistic.hxx>

namespace SAFplus
{
class Fault
{
public :

    SAFplus::Handle                   reporter;               // handle for identify a fault entity
    SAFplus::SafplusMsgServer*        faultMsgServer;       //safplus message for send fault notification to fault server
    SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
    SAFplus::Handle faultServer;
    //int faultCommunicationPort;
    typedef FaultShmMapPair KeyValuePair;

    //? Default 2-phase constructor.  Must call init(...)
    Fault() 
    {
        faultServer = INVALID_HDL;
        reporter=INVALID_HDL;
        faultMsgServer=NULL;
    };

    //? Deprecated: Initialize a fault entity with handle and comport information
    void init(SAFplus::Handle faultHandle,SAFplus::Handle faultServer, int comPort,SAFplus::Wakeable& execSemantics);

    //? Initialize a fault entity with handle and comport information
    void init(SAFplus::Handle yourHandle, SAFplus::Wakeable& execSemantics = SAFplus::BLOCK);

    //? Constructor.  
    Fault(SAFplus::Handle faultHandle,SAFplus::Handle serverAddress);

    //? get status of one entity.
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);

    //? register my self
    void registerMyself();
    //? register another handle
    void registerEntity(SAFplus::Handle other,SAFplus::FaultState State );

    //? remove this entity 
    void deRegister();
    //? remove an entity from the fault server
    void deRegister(SAFplus::Handle faultEntity);
    //? notify fault event
    void notify(SAFplus::Handle faultEntity,SAFplus::AlarmState alarmState,SAFplus::AlarmCategory category,SAFplus::AlarmSeverity severity,SAFplus::AlarmProbableCause cause,FaultPolicy pluginId = FaultPolicy::Undefined);
    //? notify fault event to fault Active
    void notify(SAFplus::Handle faultEntity,FaultEventData faultData,FaultPolicy pluginId = FaultPolicy::Undefined);
    void notify(FaultEventData faultData,FaultPolicy pluginId = FaultPolicy::Undefined);
    //? Shortcut fault notification in the case where an entity is not responding to your request
    void notifyNoResponse(SAFplus::Handle faultEntity,SAFplus::AlarmSeverity severity=SAFplus::AlarmSeverity::ALARM_SEVERITY_CRITICAL);
    protected:
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,SAFplus::FaultMessageType msgType,SAFplus::AlarmState alarmState,SAFplus::AlarmCategory category,SAFplus::AlarmSeverity severity,SAFplus::AlarmProbableCause cause,FaultPolicy pluginId);
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,SAFplus::FaultMessageType msgType,FaultPolicy pluginId,FaultEventData faultData);
    // fill and send a fault event to fault server or broadcast
    void fillAndSendMessage(void* data, SAFplus::FaultMessageType msgType,SAFplus::FaultMessageSendMode msgSendMode,bool forcing=false);
    //send a fault notification to fault server
    void sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode);
    friend class SAFplusI::GroupSharedMem;
    // set name of fault entity
    SAFplus::Handle getActiveServerAddress();
    // register a fault entity to fault server
    void sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state);

};

//? Convert fault category enum to a string
extern const char* strFaultCategory[];
//? Convert fault severity enum to a string
extern const char* strFaultSeverity[];
//? Convert fault probable cause to a string
extern const char* strFaultProbableCause[];
//? Convert fault entity state enum to a string
extern const char* strFaultEntityState[];

//? Convert fault message type to a string
extern const char* strFaultMsgType[];

class FaultServer:public SAFplus::MsgHandler,public SAFplus::Wakeable
{
protected:
    //? shared memory
    FaultSharedMem fsmServer;
    //? fault server group
    SAFplus::Group group;
    //? fault server group reporter
    SAFplus::Handle grpHandle;
    //? fault server reporter
    SAFplus::Handle faultServerHandle;
    //? fault server msg server
    SAFplus::SafplusMsgServer   *faultMsgServer;
    FaultGroupData faultInfo;
    //? fault event logging
    SAFplus::FaultStatistic faultHistory;
    //? fault communication port (should be remove)
    int faultCommunicationPort;
    //? group change
    int changeCount;

    SAFplus::Fault faultClient;
    //? Protect the fault server from simultaneous access
    SAFplus::Mutex      faultServerMutex;
    //? Synchronize fault data across servers
    SAFplus::Checkpoint faultCheckpoint;

public:
    FaultServer();
    void init();
    //  Search for any fault plugins and load them up
    void loadFaultPlugins();
    // reporter fault event message
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    //register a fault client entity, write fault entiti information into shared memory
    void registerFaultEntity(FaultShmEntry* frp, SAFplus::Handle faultClient,bool needNotify);
    //Count fault Event
    int countFaultEvent(SAFplus::Handle reporter,SAFplus::Handle faultEntity,long time);
    //Deregister a fault client entity, remove fault entity out of shared memory
    void removeFaultEntity(SAFplus::Handle faultClient,bool needNotify);
    //Set dependency for a fault reporter
    bool setDependency(SAFplus::Handle dependencyHandle, SAFplus::Handle faultHandle);
    //Remove dependency for a fault reporter
    bool removeDependency(SAFplus::Handle dependencyHandle,SAFplus::Handle faultHandle);
    // set name for fault client entity
    void setName(SAFplus::Handle faultHandle, const char* name);
    //process a fault event base on plugin id
    void processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault,SAFplus::Handle faultEntity, SAFplus::Handle faultReporter);
    //process tipc notification. currently using default fault policy
    //void processIocNotification(SAFplus::FaultPolicy pluginId,ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);
    //broadcast fault event to all other node
    void sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode);
    //broadcast fault event to all other fault server in group
    void sendFaultNotificationToGroup(void* data, int dataLength);
    //broadcast fault entity join event to all other fault server in group
    void broadcastEntityAnnounceMessage(SAFplus::Handle handle,SAFplus::FaultState state=SAFplus::FaultState::STATE_UP);
    //broadcast fault entity leave event to all other fault server in group
    void sendFaultLeaveMessage(SAFplus::Handle handle);

    void wake(int amt,void* cookie=NULL);
    void sendFaultSyncRequest(SAFplus::Handle activeAddress);
    void sendFaultSyncReply(SAFplus::Handle address);
    void writeToSharedMemoryAllEntity();

    //? Get status of one fault Entity: for use by Fault plugins
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);
    //? Set status of one fault Entity: for use by Fault plugins
  void setFaultState(SAFplus::Handle handle,SAFplus::FaultState state);

};
};
