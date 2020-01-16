#pragma once
#include <FaultSharedMem.hxx>
#include <FaultStatistic.hxx>
#include <clGroupIpi.hxx>

//? <section name="Fault">

namespace SAFplus
{

  //? <class> Since the AMF (safplus_amf process) includes a fault server instance, this fault server class is only used by applications in a no AMF deployment
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

    SAFplusI::GroupServer* gs;

public:
    //? <ctor>default 2 phase constructor.  Must call init(...)</ctor>
    FaultServer();
    //? Initialize the fault service
    void init(const SAFplusI::GroupServer* gs);
    //?  Search for any fault plugins and load them up
    void loadFaultPlugins();
    //? reporter fault event message
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    //? register a fault client entity, write fault entiti information into shared memory
    void registerFaultEntity(FaultShmEntry* frp, SAFplus::Handle faultClient,bool needNotify);
    //? Count fault Event
    int countFaultEvent(SAFplus::Handle reporter,SAFplus::Handle faultEntity,long time);
    //? Deregister a fault client entity, remove fault entity out of shared memory
    void removeFaultEntity(SAFplus::Handle faultClient,bool needNotify);
    //? Set dependency for a fault reporter
    bool setDependency(SAFplus::Handle dependencyHandle, SAFplus::Handle faultHandle);
    //? Remove dependency for a fault reporter
    bool removeDependency(SAFplus::Handle dependencyHandle,SAFplus::Handle faultHandle);
    //? set name for fault client entity
    void setName(SAFplus::Handle faultHandle, const char* name);
    //? process a fault event base on plugin id
    void processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault,SAFplus::Handle faultEntity, SAFplus::Handle faultReporter);
    // process tipc notification. currently using default fault policy
    //void processIocNotification(SAFplus::FaultPolicy pluginId,ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);
    //? broadcast fault event to all other nodes
    void sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode);
    //? broadcast fault event to all other fault server in group
    void sendFaultNotificationToGroup(void* data, int dataLength);
    //? broadcast fault entity join event to all other fault server in group
    void broadcastEntityAnnounceMessage(SAFplus::Handle handle,SAFplus::FaultState state=SAFplus::FaultState::STATE_UP);
    //? broadcast fault entity state change event to all other fault server in group
    void broadcastEntityStateChangeMessage(SAFplus::Handle handle, SAFplus::FaultState state);
    //? broadcast fault entity leave event to all other fault server in group
    void sendFaultLeaveMessage(SAFplus::Handle handle);

    void wake(int amt,void* cookie=NULL);
    void sendFaultSyncRequest(SAFplus::Handle activeAddress);
    void sendFaultSyncReply(SAFplus::Handle address);
    void writeToSharedMemoryAllEntity();

    //? Get status of one fault Entity: for use by Fault plugins
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);
    //? Set status of one fault Entity: for use by Fault plugins
    void setFaultState(SAFplus::Handle handle,SAFplus::FaultState state);\
    void RemoveAllEntity();


}; //? </class>


};

//? </section>
