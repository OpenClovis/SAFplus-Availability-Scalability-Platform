#ifndef CLFAULT_IPI_H_
#define CLFAULT_IPI_H_

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


namespace SAFplusI
{
#define MAX_FAULT_BUFFER_SIZE 1024*1024

	enum class FaultMessageTypeT
	{
		MSG_ENTITY_JOIN = 1,     // the entity joins
		MSG_ENTITY_LEAVE,       // the entity leave
		MSG_ENTITY_FAULT,       // the fault event
		MSG_ENTITY_JOIN_BROADCAST,  // broadcast entity join
		MSG_ENTITY_LEAVE_BROADCAST,  // broadcast entity leave
		MSG_ENTITY_FAULT_BROADCAST,   // broadcast fault event
		MSG_FAULT_SYNC_REQUEST,   // sync request  from fault server to master fault server
		MSG_FAULT_SYNC_REPLY,   // fault entity information from fault server master to fault server
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

    enum class AlarmSeverityTypeT
    {

        /**ClAlarmCategoryTypeT
         * Alarm with invalid severity
         */
         ALARM_SEVERITY_INVALID       = 0,

        /**
         * Alarm with severity level as \e critical.
         */
         ALARM_SEVERITY_CRITICAL      = 1,

        /**
         *  Alarm with severity level as \e major.
         */
         ALARM_SEVERITY_MAJOR         = 2,

        /**
         *  Alarm with severity level as \e minor.
         */
         ALARM_SEVERITY_MINOR         = 3,

        /**
         *  Alarm with severity level \e warning.
         */
         ALARM_SEVERITY_WARNING       = 4,

        /**
         *  Alarm with severity level \e indeterminate.
         */
         ALARM_SEVERITY_INDETERMINATE = 5,

        /**
         *  Alarm with severity level as \e cleared.
         */
         ALARM_SEVERITY_CLEAR         = 6

    };

    /**
     * This enumeration defines all the probable causes of the
     * alarm based on the categories.
     */
    enum class AlarmProbableCauseT
    {
        /**
         * invalid cause
         */
        ALARM_ID_INVALID =0,
        /**
         * Probable cause for alarms which can occur due to communication related
         * problems between two points.
         */

        /**
         * An error condition when there is no data in the communication channel.
         */
        ALARM_PROB_CAUSE_LOSS_OF_SIGNAL = 1,

        /**
         * An inability to locate the information in bit-delimiters.
         */
        ALARM_PROB_CAUSE_LOSS_OF_FRAME,

        /**
         * Error in the bit-groups delimiters within the bit-stream.
         */
        ALARM_PROB_CAUSE_FRAMING_ERROR,

        /**
         * An error has occured in the tranClAlarmCategoryTypeTsmission between local node and adjacent node.
         */
        ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR,

        /**
         * An error has occurred on the transmission channel beyond the adjacent node.
         */
        ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR,

        /**
         * Error occured while establishing the connection.
         */
        ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR,

        /**
         * The quality of the singal has decreased.
         */
        ALARM_PROB_CAUSE_DEGRADED_SIGNAL,

        /**
         * A failure in the subsystem which supports communication over telecommunication links.
         */
        ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE,

        /**
         * The communication protocol has been voilated.
         */
        ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR,

        /**
         * Problem detected in the local area network.
         */
        ALARM_PROB_CAUSE_LAN_ERROR,

        /**
         * Probable cause of the alarm is data transmission error.
         */
        ALARM_PROB_CAUSE_DTE,

        /**
         * Probable causes of alarms caused due to degradation in quality of service.
         */

        /**
         * The time delays between the query and its result is beyond the allowable limits.
         */
        ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE,

        /**
         * The number of items to be processed has exceeded the specified limits.
         */
        ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED,

        /**
         * The bandwidth available for transmission has reduced.
         */
        ALARM_PROB_CAUSE_BANDWIDTH_REDUCED,

        /**
         * Number of repeat transmission is beyond the allowed limits.
         */
        ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE,

        /**
         * The limit allowed (or configured) has exceeded.
         */
        ALARM_PROB_CAUSE_THRESHOLD_CROSSED,

        /**
         * Service limit or service aggrement are outside the acceptable limits.
         */
        ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED,

        /**
         * System is reaching its capacity or approaching it.
         */
        ALARM_PROB_CAUSE_CONGESTION,

        /**
         * Probable cause of the alarm is the maximum capacity is already reached or about to be reached.
         */
        ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY,

        /**
         * Probable cause of the alarm is storage capacity problem.
         */
        ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM,

        /**
         * There is a mismatch in the version of two communicating entities.
         */
        ALARM_PROB_CAUSE_VERSION_MISMATCH,

        /**
         * An error has caused the data to be incorrect.
         */
        ALARM_PROB_CAUSE_CORRUPT_DATA,

        /**
         * CPU cycles limited has been exceeded its limit.
         */
        ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED,

        /**
         * Probable cause of alarm is error in software.
         */
        ALARM_PROB_CAUSE_SOFWARE_ERROR,

        /**
         * Probable cause of alarm is error in software program error.
         */
        ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR,

        /**
         * Probable cause of alarm is software program abnormally terminated.
         */
        ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED,

        /**
         * Format of the file is incorrect.
         */
        ALARM_PROB_CAUSE_FILE_ERROR,

        /**
         * There is no program-addressable stClAlarmCategoryTypeTorage available.
         */
        ALARM_PROB_CAUSE_OUT_OF_MEMORY,

        /**
         * An entity upon which the reporting object depends has become unreliable.
         */
        ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,

        /**
         * A failure is detected in the application subsystem failure. A subsystem can be a software
         * supporting the larger module.
         */
        ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE,

        /**
         * The parameter for configuration or customization are given improperly.
         */
        ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR,

        /**
         * Probable cause alarms caused due to equipment related problems.
         */

        /**
         * There is a problem with the power supply.
         */
        ALARM_PROB_CAUSE_POWER_PROBLEM,

        /**
         * The process which has timing constraints didn't complete with the specified
         * time or has completed but the results are not reliable.
         */
        ALARM_PROB_CAUSE_TIMING_PROBLEM,


        /**
         * Internal machine error has occurred on the a Central processing Unit.
         */
        ALARM_PROB_CAUSE_PROCESSOR_PROBLEM,

        /**
         * An internal error has occured on a dataset or modem.
         */
        ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR,

        /**ClAlarmCategoryTypeT
         * Error occured while multiplexing the communication signal.
         */
        ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM,

        /**
         * Probable cause of alarm is receiver failure.
         */
        ALARM_PROB_CAUSE_RECEIVER_FAILURE,

        /**
         * Probable cause of alarm is transamitter failure.
         */
        ALARM_PROB_CAUSE_TRANSMITTER_FAILURE,

        /**
         * Probable cause of alarm is receive failure.
         */
        ALARM_PROB_CAUSE_RECEIVE_FAILURE,

        /**
         * Probable cause of alarm is transmission failure.
         */
        ALARM_PROB_CAUSE_TRANSMIT_FAILURE,

        /**
         * Error has occured on a output device.
         */
        ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR,

        /**
         * Probable cause of alarm is input device error.
         */
        ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR,

        /**
         * Probable cause of alarm is input output device error.
         */
        ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR,

        /**
         * Internal error in equipment malfunction for which no actual cause has been identified.
         */
        ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION,

        /**
         * Probable cause of alarm is adapter error.
         */
        ALARM_PROB_CAUSE_ADAPTER_ERROR,

        /**
         * Probable cause of alarm is temperature unacceptable.
         */
        ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE,

        /**
         * Probable cause of alarm is humidity unacceptable around equipment.
         */
        ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE,

        /**
         * Probable cause of alarm is heating or ventillation or cooling system problem.
         */
        ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM,

        /**
         * Probable cause of alarm is fire around the equipment.
         */
        ALARM_PROB_CAUSE_FIRE_DETECTED,

        /**
         * Probable cause of alarm is flood around equipment.
         */
        ALARM_PROB_CAUSE_FLOOD_DETECTED,

        /**
         * Probable cause of alarm is toxic leak is detected around equipment.
         */
        ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED,

        /**
         * A leak of gas or fluid is detected around equipment.
         */
        ALARM_PROB_CAUSE_LEAK_DETECTED,

        /**
         * The fluid or gas pressure is unacceptable.
         */
        ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE,

        /**
         * The vibration around the equipment has exceeded the limit.
         */
        ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION,

        /**
         * Probable cause of alarm is exhaustion of the materail supply.
         */
        ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED,

        /**
         * Error has occured in the pump which is used to trasfer fluid in the equipment.
         */
        ALARM_PROB_CAUSE_PUMP_FAILURE,

        /**
         * Probable cause of alarm is enclosure door is open.
         */
        ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN
    };


    /**
     * The enumeration to depict the state of the alarm that is
     * into. It can be in any of the following state that is
     * cleared, assert, suppressed or under soaking.
     */
    enum class AlarmStateT
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
    enum class AlarmCategoryTypeT
    {

        /**
         *  Category for alarms that are not valid
         */
         ALARM_CATEGORY_INVALID             = 0,

        /**
         *  Category for alarms that are related to communication between to points.
         */
         ALARM_CATEGORY_COMMUNICATIONS       = 1,

        /**
         *  Category for alarms that are related to degrations in quality of service.
         */
         ALARM_CATEGORY_QUALITY_OF_SERVICE   = 2,

        /**
         *  Category for alarms that are related to software or processing error.
         */
         ALARM_CATEGORY_PROCESSING_ERROR     = 3,

        /**
         *  Category for alarms that are related to equipment problems.
         */
         ALARM_CATEGORY_EQUIPMENT            = 4,

        /**
         *  Category for alarms that are related to environmental conditions around the equipment.
         */
         ALARM_CATEGORY_ENVIRONMENTAL        = 5,
    };

    enum
    {
        FaultSharedMemSize = 4 * 1024*1024,
        FaultMaxMembers    = 1024,   // Maximum number of fault entity
    };
}

namespace SAFplus
{

    enum
    {
      FAULT_POLICY_PLUGIN_ID = 0x53843923,
      FAULT_POLICY_PLUGIN_VER = 1
    };

    //fault policy plugin
    enum class FaultPolicy
    {
    Undefined = 0,
    Custom = 1,
    AMF = 2,
    };

    enum class FaultAction
    {
    ACTION_STOP = 0,
    ACTION_RESTART = 1,
    ACTION_IGNORE = 2,
    };



    // fault event information include state , severity, category, cause
    class FaultEventData
    {
    public:
        /**
        * Flag to indicate if the alarm was for assert or for clear.
        */
        SAFplusI::AlarmStateT alarmState;

        /**
         * The category of the fault event.
         */
        SAFplusI::AlarmCategoryTypeT category;

        /**
         * The severity of the fault event.
         */
        SAFplusI::AlarmSeverityTypeT severity;
        /**
         * The probable cause of the fault event.
         */
        SAFplusI::AlarmProbableCauseT cause;

        FaultEventData()
        {
          	alarmState= SAFplusI::AlarmStateT::ALARM_STATE_INVALID;
           	category= SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID;
           	cause= SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID;
           	severity=SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID;
        }
        void init(SAFplusI::AlarmStateT a_state,SAFplusI::AlarmCategoryTypeT a_category,SAFplusI::AlarmSeverityTypeT a_severity,SAFplusI::AlarmProbableCauseT a_cause)
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


};

namespace SAFplus
{

#define FAULT_NAME_LEN 100
#define MAX_FAULT_DEPENDENCIES 5

	//status of fault entity
	enum class FaultState
	{
		STATE_UNDEFINED = 0,
		STATE_UP = 1,
		STATE_DOWN
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
        ClIocAddressT iocFaultServer;
    };
	void faultInitialize(void);

    class FaultMessageProtocol
    {
    public:
        SAFplusI::FaultMessageTypeT     messageType;
        SAFplus::FaultState state;
        SAFplus::FaultPolicy pluginId;
        //char name[FAULT_NAME_LEN];
        FaultEventData data;
        SAFplus::Handle       reporter;
        SAFplus::Handle       faultEntity;
        char                  syncData[1];
        FaultMessageProtocol()
        {
    		messageType=SAFplusI::FaultMessageTypeT::MSG_UNDEFINED;
    		faultEntity=INVALID_HDL;
    		data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
            pluginId=SAFplus::FaultPolicy::Undefined;
        }
    };


    class FaultShmEntry
    {
    public :
        SAFplus::Handle faultHdl;
        int dependecyNum;
        //char name[FAULT_NAME_LEN];  // For display purposes only, string lookups go through the Name server.
        SAFplus::Handle depends[MAX_FAULT_DEPENDENCIES];  // If this entity fails, all entities in this array will also fail.
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
            for(int i=0;i<MAX_FAULT_DEPENDENCIES;i++)
            {
              	depends[i]=INVALID_HDL;
            }
        };
        void init(SAFplus::Handle fHandle)
        {
            faultHdl = fHandle;
        	dependecyNum = 0;
            for(int i=0;i<MAX_FAULT_DEPENDENCIES;i++)
            {
            	depends[i]=INVALID_HDL;
            }
        };


    };

    class FaultEntryData
    {
    public :
        SAFplus::Handle faultHdl;
        int dependecyNum;
        SAFplus::Handle depends[MAX_FAULT_DEPENDENCIES];  // If this entity fails, all entities in this array will also fail.
        SAFplus::FaultState state;  //Fault state of an entity

    };


    typedef std::pair<const SAFplus::Handle,FaultShmEntry> FaultShmMapPair;
    typedef boost::interprocess::allocator<FaultShmEntry, boost::interprocess::managed_shared_memory::segment_manager> FaultEntryAllocator;
    typedef boost::unordered_map < SAFplus::Handle, FaultShmEntry, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle>,FaultEntryAllocator> FaultShmHashMap;


    class FaultSharedMem
    {
    public:
        #define FAULT_NAME_LEN 100
        SAFplus::ProcSem mutex;  // protects the shared memory region from simultaneous access
        SAFplus::Mutex  faultMutex;
        boost::interprocess::managed_shared_memory faultMsm;
        SAFplus::FaultShmHashMap* faultMap;
        SAFplus::FaultShmHeader* faultHdr;
        SAFplus::Mutex  localMutex;
        void init(ClIocAddress active);
        void clientInit();
        void clear();
        void remove(SAFplus::Handle handle);
        void removeAll();
        void dbgDump(void);
        void dispatcher(void);
        bool createFault(FaultShmEntry* frp,SAFplus::Handle fault);
        bool updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault);
        bool updateFaultHandleState(SAFplus::FaultState state,SAFplus::Handle fault);
        void getAllFaultClient(char* buf,ClWordT bufSize);
        void applyFaultSync(char* buf,ClWordT bufSize);
    };

    class FaultGroupData
    {
    public:
    	unsigned int       structId;       // = 0x67839345
        ClIocAddressT iocFaultServer;
    };
};

#endif
