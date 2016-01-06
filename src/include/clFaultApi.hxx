#pragma once
#include <FaultSharedMem.hxx>

//? <section name="Fault">  The fault service presents a clusterwide consistent view of the availability of any entity.  Any entity in the cluster can query the status of, and report faults about, any other entity.  When the fault service receives a fault report, passes the report to fault analysis modules that look at the fault in the context of other incoming faults and determine whether any entities should be marked as failed.  For example analysis of a "cannot communicate with entity" fault may conclude that the fault lies in the reporter, not the reported.  SAFplus provides an Availability Management Framework (AMF) fault module, and systems programmers can create their own fault analysis modules which will be loaded as plugins into the fault service.

namespace SAFplus
{

  //? Returns true if the fault manager is available
  bool faultAvailable(void);

  //? <class> Interact with the fault management subsystem.
class Fault
{
public:

    SAFplus::Handle                   reporter;               // handle for identify a fault entity
    SAFplus::SafplusMsgServer*        faultMsgServer;       //safplus message for send fault notification to fault server
    SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
    SAFplus::Handle faultServer;
    //int faultCommunicationPort;
    typedef FaultShmMapPair KeyValuePair;

    //? <ctor>Default 2-phase constructor.  You must call init(...) before using this class</ctor>
    Fault() 
    {
        faultServer = INVALID_HDL;
        reporter=INVALID_HDL;
        faultMsgServer=NULL;
    };

    //? [Deprecated]: Initialize a fault entity with handle and comport information
    void init(SAFplus::Handle faultHandle,SAFplus::Handle faultServer, int comPort,SAFplus::Wakeable& execSemantics = SAFplus::BLOCK);

    //? Initialize a fault entity with handle and comport information
    void init(SAFplus::Handle yourHandle, SAFplus::Wakeable& execSemantics = SAFplus::BLOCK);

    //? <ctor> Fault client object constructor
    //  <arg name="faultHandle">Handle of this fault entity (node, process, etc).  You do not need a unique handle for this fault object.</arg>
    //  <arg name="serverAddress"></arg>
    //  </ctor> 
    Fault(SAFplus::Handle faultHandle,SAFplus::Handle serverAddress);

    //? get status of one entity.
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);

    //? Register myself (the handle passed in the constructor) as an entity that has a fault state.  The fault state is initialized "up"
    void registerMyself();
    //? Register another entity into the fault manager.  Double registrations of the same entity will override the current fault state with what is passed to this function.
    // <arg name="other"></arg>
    // <arg name="State">The initial state of this entity</arg>
    void registerEntity(SAFplus::Handle other,SAFplus::FaultState State );

    //? Remove this entity from fault management
    void deRegister();
    //? Remove an entity from the fault server. <arg name="faultEntity">The entity to be removed</arg>
    void deRegister(SAFplus::Handle faultEntity);

    //? Notify the fault manager about a fault event.
    //? [ARGS TBD when we figure out the alarm portion of fault]
    void notify(SAFplus::Handle faultEntity,FaultEnums::FaultAlarmState alarmState,FaultEnums::AlarmCategory category,FaultEnums::FaultSeverity severity,FaultEnums::FaultProbableCause cause,FaultPolicy pluginId = FaultPolicy::Undefined);
    //? Notify the fault manager about a fault event.
    //? [ARGS TBD when we figure out the alarm portion of fault]
    void notify(SAFplus::Handle faultEntity,FaultEventData faultData,FaultPolicy pluginId = FaultPolicy::Undefined);

    //? Notify the fault manager about a fault event.
    //? [ARGS TBD when we figure out the alarm portion of fault]
    void notify(FaultEventData faultData,FaultPolicy pluginId = FaultPolicy::Undefined);

    //? Shortcut fault notification in the case where an entity is not responding to your request
    // <arg name="faultEntity">The entity that is not responding</arg>
    // <arg name="severity" default="ITUALARMTCMIB::ItuPerceivedSeverity::critical">[DEFAULT: critical] How important is this problem to the health of the system</arg>
    void notifyNoResponse(SAFplus::Handle faultEntity, FaultEnums::FaultSeverity severity = FaultEnums::FaultSeverity::ALARM_SEVERITY_CRITICAL);

    protected:
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,FaultEnums::FaultMessageType msgType,FaultEnums::FaultAlarmState alarmState, FaultEnums::AlarmCategory category,FaultEnums::FaultSeverity severity,FaultEnums::FaultProbableCause cause,FaultPolicy pluginId);
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,FaultEnums::FaultMessageType msgType,FaultPolicy pluginId,FaultEventData faultData);
    // fill and send a fault event to fault server or broadcast
    void fillAndSendMessage(void* data, FaultEnums::FaultMessageType msgType,SAFplus::FaultMessageSendMode msgSendMode,bool forcing=false);
    //send a fault notification to fault server
    void sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode);
    friend class SAFplusI::GroupSharedMem;
    // set name of fault entity
    SAFplus::Handle getActiveServerAddress();
    // register a fault entity to fault server
    void sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state);
    void changeFaultState(SAFplus::Handle other, SAFplus::FaultState state);

}; //? </class>

//? Convert fault category enum to a string
//extern const char* strFaultCategory[];
//? Convert fault severity enum to a string
//extern const char* strFaultSeverity[];
//? Convert fault probable cause to a string
//extern const char* strFaultProbableCause[];
//? Convert fault entity state enum to a string
extern const char* strFaultEntityState[];

//? Convert fault message type to a string
extern const char* strFaultMsgType[];

};

//? </section>
