#pragma once

#include <clFaultIpi.hxx>
#include <FaultStatistic.hxx>


using namespace SAFplus;
namespace SAFplus
{
class Fault
{
public :

    SAFplus::Handle                   reporter;               // handle for identify a fault entity
    SAFplus::SafplusMsgServer*        faultMsgServer;       //safplus message for send fault notification to fault server
    SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
    char                              name[FAULT_NAME_LEN]; // name of fault entity
    SAFplus::Handle iocFaultLocalServer;
    int faultCommunicationPort;
    typedef SAFplus::FaultShmMapPair KeyValuePair;
    Fault()
    {
    	reporter=INVALID_HDL;
    	faultMsgServer=NULL;
    };
    // register a fault entity to fault server
    void sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state);
    // register a fault entity to fault server
    void deRegister();
    // send a fault entity to fault server
    void deRegister(SAFplus::Handle faultEntity);
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageSendMode messageMode,SAFplusI::FaultMessageTypeT msgType,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId);
    // send a fault entity to fault server
    void sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageSendMode messageMode,SAFplusI::FaultMessageTypeT msgType,SAFplus::FaultPolicy pluginId,SAFplus::FaultEventData faultData);
    // fill and send a fault event to fault server or broadcast
    void fillAndSendMessage(void* data, SAFplusI::FaultMessageTypeT msgType,SAFplusI::FaultMessageSendMode msgSendMode,bool forcing=false);
    //send a fault notification to fault server
    void sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode);
    friend class SAFplusI::GroupSharedMem;
    //init a fault entity with handle and comport information
    void init(SAFplus::Handle faultHandle,SAFplus::Handle faultServer, int comPort,SAFplus::Wakeable& execSemantics);
    //register my self
    void registerFault();
    //register other handle
    void registerFault(SAFplus::Handle other,SAFplus::FaultState State );
    //notify fault event
    void notify(SAFplus::Handle faultEntity,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);
    //notify fault event to fault Active
    void notifytoActive(SAFplus::Handle faultEntity,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);

    void notify(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);
    void notifytoActive(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultDatay,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);

    void notify(SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);
    void notifytoActive(SAFplus::FaultEventData faultDatay,SAFplus::FaultPolicy pluginId = FaultPolicy::Undefined);
    // set name of fault entity
    void setName(const char* entityName);
    ClIocAddressT getActiveServerAddress();
    Fault(SAFplus::Handle faultHandle,const char* name,ClIocAddressT iocServerAddress);

    //get status of one fault entity.
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);
};

const char* strFaultCategory[]={
    "CL_ALARM_CATEGORY_UNKNOWN",
    "CL_ALARM_CATEGORY_COMMUNICATIONS",
    "CL_ALARM_CATEGORY_QUALITY_OF_SERVICE",
    "CL_ALARM_CATEGORY_PROCESSING_ERROR",
    "CL_ALARM_CATEGORY_EQUIPMENT",
    "CL_ALARM_CATEGORY_ENVIRONMENTAL"
};
const char* strFaultSeverity[]={
    "CL_ALARM_SEVERITY_UNKNOWN",
    "CL_ALARM_SEVERITY_CRITICAL",
    "CL_ALARM_SEVERITY_MAJOR",
    "CL_ALARM_SEVERITY_MINOR",
    "CL_ALARM_SEVERITY_WARNING",
    "CL_ALARM_SEVERITY_INDETERMINATE",
    "CL_ALARM_SEVERITY_CLEAR"
};
const char* strFaultProbableCause[]={
    "CL_ALARM_PROB_CAUSE_UNKNOWN",
    /**
     * Probable cause for Communication related alarms
     */
    "CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL",
    "CL_ALARM_PROB_CAUSE_LOSS_OF_FRAME",
    "CL_ALARM_PROB_CAUSE_FRAMING_ERROR",
    "CL_ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR",
    "CL_ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR",
    "CL_ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR",
    "CL_ALARM_PROB_CAUSE_DEGRADED_SIGNAL",
    "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE",
    "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR",
    "CL_ALARM_PROB_CAUSE_LAN_ERROR",
    "CL_ALARM_PROB_CAUSE_DTE",
    /**
     * Probable cause for Quality of Service related alarms
     */
    "CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE",
    "CL_ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED",
    "CL_ALARM_PROB_CAUSE_BANDWIDTH_REDUCED",
    "CL_ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE",
    "CL_ALARM_PROB_CAUSE_THRESHOLD_CROSSED",
    "CL_ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED",
    "CL_ALARM_PROB_CAUSE_CONGESTION",
    "CL_ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY",
    /**
     * Probable cause for Processing Error related alarms
     */
    "CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM",
    "CL_ALARM_PROB_CAUSE_VERSION_MISMATCH",
    "CL_ALARM_PROB_CAUSE_CORRUPT_DATA",
    "CL_ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED",
    "CL_ALARM_PROB_CAUSE_SOFWARE_ERROR",
    "CL_ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR",
    "CL_ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED",
    "CL_ALARM_PROB_CAUSE_FILE_ERROR",
    "CL_ALARM_PROB_CAUSE_OUT_OF_MEMORY",
    "CL_ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE",
    "CL_ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE",
    "CL_ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR",
    /**
     * Probable cause for Equipment related alarms
     */
    "CL_ALARM_PROB_CAUSE_POWER_PROBLEM",
    "CL_ALARM_PROB_CAUSE_TIMING_PROBLEM",
    "CL_ALARM_PROB_CAUSE_PROCESSOR_PROBLEM",
    "CL_ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR",
    "CL_ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM",
    "CL_ALARM_PROB_CAUSE_RECEIVER_FAILURE",
    "CL_ALARM_PROB_CAUSE_TRANSMITTER_FAILURE",
    "CL_ALARM_PROB_CAUSE_RECEIVE_FAILURE",
    "CL_ALARM_PROB_CAUSE_TRANSMIT_FAILURE",
    "CL_ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION",
    "CL_ALARM_PROB_CAUSE_ADAPTER_ERROR",
    /**
     * Probable cause for Environmental related alarms
     */
    "CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM",
    "CL_ALARM_PROB_CAUSE_FIRE_DETECTED",
    "CL_ALARM_PROB_CAUSE_FLOOD_DETECTED",
    "CL_ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED",
    "CL_ALARM_PROB_CAUSE_LEAK_DETECTED",
    "CL_ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION",
    "CL_ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED",
    "CL_ALARM_PROB_CAUSE_PUMP_FAILURE",
    "CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN"
};

const char* strFaultMsgType[]=
{
	"MSG_ENTITY_JOIN",
	"MSG_ENTITY_LEAVE",
	"MSG_ENTITY_FAULT",
	"MSG_ENTITY_JOIN_BROADCAST",
	"MSG_ENTITY_LEAVE_BROADCAST",
	"MSG_ENTITY_FAULT_BROADCAST",
	"MSG_UNDEFINED"
};

const char* strFaultEntityState[]={
    "STATE_UNDEFINED",
    "STATE_UP",
    "STATE_DOWN"
};

class FaultServer:public SAFplus::MsgHandler,public SAFplus::Wakeable
{
public:
    FaultServer();
    //shared memory
    FaultSharedMem fsmServer;
    //fault server group
    SAFplus::Group group;
    //fault server group reporter
    SAFplus::Handle grpHandle;
    //fault server reporter
    SAFplus::Handle faultServerHandle;
    //fault server msg server
    SAFplus::SafplusMsgServer   *faultMsgServer;
    FaultGroupData faultInfo;
    // fault event logging
    SAFplus::FaultStatistic faultHistory;
    //fault communication port (should be remove)
    int faultCommunicationPort;
    //group change
    int changeCount;
    //init fault server
    SAFplus::Fault faultClient;
    SAFplus::Mutex  faultServerMutex;
    SAFplus::Checkpoint faultCheckpoint;
    void init();
    // reporter fault event message
    virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
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
    void sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode);
    //broadcast fault event to all other fault server in group
    void sendFaultNotificationToGroup(void* data, int dataLength);
    //broadcast fault entity join event to all other fault server in group
    void sendFaultAnnounceMessage(SAFplus::Handle handle);
    //broadcast fault entity leave event to all other fault server in group
    void sendFaultLeaveMessage(SAFplus::Handle handle);
	void wake(int amt,void* cookie=NULL);
	void sendFaultSyncRequest(ClIocAddress activeAddress);
	void sendFaultSyncReply(ClIocAddress address);
	void writeToSharedMemoryAllEntity();
    //get status of one fault Entity
    SAFplus::FaultState getFaultState(SAFplus::Handle faultHandle);

};
};
