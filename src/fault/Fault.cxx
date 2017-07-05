/* Standard headers */
#include <string>
/* SAFplus headers */

#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <FaultSharedMem.hxx>
#include <clHandleApi.hxx>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <clNameApi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <clFaultPolicyPlugin.hxx>
#include <FaultStatistic.hxx>
#include <FaultHistoryEntity.hxx>
#include <clObjectMessager.hxx>
#include <time.h>

using namespace SAFplus;
using namespace std;
#define FAULT "FLT"
#define FAULT_ENTITY "ENT"

namespace SAFplus
{
  typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;

  /*const char* strFaultCategory[]={
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
  };*/

  /*const char* strFaultProbableCause[]={
      "CL_ALARM_PROB_CAUSE_UNKNOWN",
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
      /*"CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE",
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
      /*"CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM",
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
      /*"CL_ALARM_PROB_CAUSE_POWER_PROBLEM",
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
      /*"CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE",
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
  };*/

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

  const char* strFaultEntityState[]=
  {
      "STATE_UNDEFINED",
      "STATE_UP",
      "STATE_DOWN"
  };


    int faultInitCount=0;
    FaultSharedMem fsm;

    bool faultAvailable() { return (faultInitCount>0); }

    void faultInitialize(void)
    {
      faultInitCount++;
      if (faultInitCount > 1) return;
      SAFplus::fsm.init();
    }

void Fault::setNotification(SAFplus::Wakeable& w)
  {
    //? if (!wakeable) gsm.registerGroupObject(this);
  wakeable = &w;
  assert(0);  // NOT IMPLEMENTED
  }

  uint64_t Fault::lastChange()
    {
      return fsm.lastChange();
    }

    // Register a Fault Entity to Fault Server
    void Fault::sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state)
    {
        assert(other != INVALID_HDL);  // We must always report the state of a particular entity, even if that entity is myself (i.e. reporter == other)
        FaultMessageProtocol sndMessage;
        memset(&sndMessage,0,sizeof(FaultMessageProtocol));
        sndMessage.reporter = reporter;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_JOIN;
        sndMessage.state = state;
        sndMessage.faultEntity=other;
        sndMessage.data.init(AlarmState::INVALID,AlarmCategory::INVALID,AlarmSeverity::INVALID,AlarmProbableCause::INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_BROADCAST);
    }

    void Fault::changeFaultState(SAFplus::Handle other, SAFplus::FaultState state)
    {
        assert(other != INVALID_HDL);  // We must always report the state of a particular entity, even if that entity is myself (i.e. reporter == other)
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_STATE_CHANGE;
        sndMessage.state = state;
        sndMessage.faultEntity=other;
        sndMessage.data.init(AlarmState::INVALID,AlarmCategory::INVALID,AlarmSeverity::INVALID,AlarmProbableCause::INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_BROADCAST);
    }
    //deregister Fault Entity
    void Fault::deRegister()
    {
        deRegister(reporter);
    }

    //deregister Fault Entity by Entity Handle
    void Fault::deRegister(SAFplus::Handle faultEntity)
    {
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        FaultPolicyMap faultPolicies;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_LEAVE;
        sndMessage.state = FaultState::STATE_UP;
        sndMessage.faultEntity=faultEntity;
        sndMessage.data.init(AlarmState::INVALID,AlarmCategory::INVALID,AlarmSeverity::INVALID,AlarmProbableCause::INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        logDebug(FAULT,FAULT_ENTITY,"Deregister fault entity : Node [%d], Process [%d] , Message Type [%d]", reporter.getNode(), reporter.getProcess(),(int) sndMessage.messageType);
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_BROADCAST);
    }

    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,SAFplus::FaultMessageType msgType,SAFplus::FaultPolicy pluginId,SAFplus::FaultEventData faultData)
    {
      //logDebug(FAULT,FAULT_ENTITY,"Sending Fault Event message ... ");
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = msgType;
        sndMessage.data.state= faultData.state;
        sndMessage.data.category=faultData.category;
        sndMessage.data.cause=faultData.cause;
        sndMessage.data.severity=faultData.severity;
        sndMessage.faultEntity=faultEntity;
        sndMessage.pluginId=pluginId;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),messageMode);
    }
    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,SAFplus::FaultMessageType msgType,AlarmState state,AlarmCategory category,AlarmSeverity severity,AlarmProbableCause cause,SAFplus::FaultPolicy pluginId)
    {
      //logDebug(FAULT,FAULT_ENTITY,"Sending Fault Event message ...");
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = msgType;
        sndMessage.data.state= state;
        sndMessage.data.category=category;
        sndMessage.data.cause=cause;
        sndMessage.data.severity=severity;
        sndMessage.faultEntity=faultEntity;
        sndMessage.pluginId=pluginId;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),messageMode);
    }

    void Fault::notify(SAFplus::Handle faultEntity,AlarmState state,AlarmCategory category,AlarmSeverity severity,AlarmProbableCause cause,SAFplus::FaultPolicy pluginId)
    {
      assert(faultEntity != INVALID_HDL);
      sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_BROADCAST,SAFplus::FaultMessageType::MSG_ENTITY_FAULT,state,category,severity,cause,pluginId);
    }

    void Fault::notifyLocal(SAFplus::Handle faultEntity,AlarmState state,AlarmCategory category,AlarmSeverity severity,AlarmProbableCause cause,SAFplus::FaultPolicy pluginId)
    {
      assert(faultEntity != INVALID_HDL);
      sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_LOCAL_SERVER,SAFplus::FaultMessageType::MSG_ENTITY_FAULT,state,category,severity,cause,pluginId);
    }

    void Fault::notify(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
      assert(faultEntity != INVALID_HDL);
      sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_BROADCAST,SAFplus::FaultMessageType::MSG_ENTITY_FAULT,pluginId,faultData);
    }

    void Fault::notify(SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
      assert(reporter != INVALID_HDL);
      sendFaultEventMessage(reporter,FaultMessageSendMode::SEND_BROADCAST,SAFplus::FaultMessageType::MSG_ENTITY_FAULT,pluginId,faultData);
    }

    void Fault::notifyNoResponse(SAFplus::Handle faultEntity,AlarmSeverity severity)
    {
      assert(faultEntity != INVALID_HDL);
      sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_BROADCAST,SAFplus::FaultMessageType::MSG_ENTITY_FAULT,AlarmState::ASSERT,AlarmCategory::COMMUNICATIONS,severity,AlarmProbableCause::RECEIVER_FAILURE,FaultPolicy::Undefined);
    }

    //Sending a fault notification to fault server
    void Fault::sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode)
    {
        logDebug(FAULT,FAULT_ENTITY,"Sending Fault Notification");
        Handle activeServer;
        if (faultServer==INVALID_HDL)
        {
            activeServer = fsm.faultHdr->activeFaultServer;

            //TODO: Race condition with GroupServer initialize - active is not yet update at fault.init(myhandle)
            if (activeServer == INVALID_HDL) 
              activeServer = faultServer = reporter;

            logDebug(FAULT,FAULT_ENTITY,"Active fault server is on node [%d]", activeServer.getNode());
        }
        else
        {
            activeServer = faultServer;
        }
        switch(messageMode)
        {
            case SAFplus::FaultMessageSendMode::SEND_TO_LOCAL_SERVER:
            {
                logDebug(FAULT,FAULT_ENTITY,"Send Fault Notification to local Fault server");
                try
                {
                  Handle hdl = getProcessHandle(SAFplusI::FAULT_IOC_PORT);
                  faultMsgServer->SendMsg(hdl, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...) // SAFplus::Error &e)
                {
                    logDebug(FAULT,FAULT_ENTITY,"Failed to send.");
                }
                break;
            }
            case SAFplus::FaultMessageSendMode::SEND_TO_ACTIVE_SERVER:
            {
                logDebug(FAULT,FAULT_ENTITY,"Send Fault Notification to active Fault Server  : node Id [%d]",activeServer.getNode());
                try
                {
                    faultMsgServer->SendMsg(activeServer, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...)
                {
                   logDebug(FAULT,"MSG","Failed to send.");
                }
                break;
            }
            case SAFplus::FaultMessageSendMode::SEND_BROADCAST:
            {
                try
                {
                  faultMsgServer->SendMsg(getProcessHandle(SAFplusI::FAULT_IOC_PORT,Handle::AllNodes), (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...)
                {
                   logDebug(FAULT,"MSG","Failed to send.");
                }
                break;
            }
            default:
            {
                logError(FAULT,"MSG","Unknown message sending mode");
                break;
            }
        }
    }

    void Fault::init(SAFplus::Handle yourHandle, SAFplus::Wakeable& execSemantics)
    {
      reporter = yourHandle;
      if (!faultMsgServer)
      {
        faultMsgServer = &safplusMsgServer;
      }
      registerMyself();
    }


    void Fault::init(SAFplus::Handle faultHandle,SAFplus::Handle faultServerHandle, int comPort, SAFplus::Wakeable& execSemantics)
    {
      reporter = faultHandle;
      //faultCommunicationPort = comPort;
      if(!faultMsgServer)
      {
          faultMsgServer = &safplusMsgServer;
      }
      faultServer = faultServerHandle;
      registerMyself();
    }

    SAFplus::Handle Fault::getActiveServerAddress()
    {
      SAFplus::Handle masterAddress = fsm.faultHdr->activeFaultServer;
      return masterAddress;
    }

    Fault::Fault(SAFplus::Handle faultHandle,SAFplus::Handle iocServerAddress)
    {
      wakeable = NULL;
      faultMsgServer = NULL;
      reporter =INVALID_HDL;
      this->init(faultHandle,iocServerAddress,SAFplusI::FAULT_IOC_PORT,BLOCK);
    }

    SAFplus::FaultState Fault::getFaultState(SAFplus::Handle faultHandle)
    {
      FaultShmHashMap::iterator entryPtr;
      entryPtr = fsm.faultMap->find(faultHandle);
      if (entryPtr == fsm.faultMap->end())
      {
        // logDebug(FAULT,FAULT_ENTITY,"Fault Entity [%" PRIx64 ":%" PRIx64 "] is not available in shared memory",faultHandle.id[0],faultHandle.id[1]);
        return SAFplus::FaultState::STATE_UNDEFINED;
      }
      FaultShmEntry *fse = &entryPtr->second;
      //logDebug(FAULT,FAULT_ENTITY,"Fault state of Fault Entity [%" PRIx64 ":%" PRIx64 "] is [%s]",faultHandle.id[0],faultHandle.id[1],strFaultEntityState[int(fse->state)]);
      return fse->state;
    }

    void Fault::registerMyself()
    {
      sendFaultAnnounceMessage(reporter,FaultState::STATE_UP);
    }
    void Fault::registerEntity(SAFplus::Handle other,FaultState state)
    {
      sendFaultAnnounceMessage(other,state);
    }

#if 0
    static ClRcT faultServerIocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie)
    {
        FaultServer* svr = (FaultServer*) cookie;
        ClRcT rc = CL_OK;
        ClIocAddressT address;
        ClIocNotificationIdT eventId = (ClIocNotificationIdT) ntohl(notification->id);
        ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
        ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
        logDebug(FAULT,"IOC","Recv notification [%d] for [%d.%d]", eventId, nodeAddress,portId);
        switch(eventId)
        {
            case CL_IOC_COMP_DEATH_NOTIFICATION:
            {
                logDebug(FAULT,"IOC","Received component leave notification for [%d.%d]", nodeAddress,portId);
                //ScopedLock<ProcSem> lock(svr->fsmServer.mutex);
                FaultShmHashMap::iterator i;
                for (i=svr->fsmSerfaultServerHver.faultMap->begin(); i!=svr->fsmServer.faultMap->end();i++)
                {
                    FaultShmEntry& ge = i->second;
                    Handle handle = i->first;
                    logDebug("GMS","IOC","  Checking fault [%" PRIx64 ":%" PRIx64 "]", handle.id[0],handle.id[1]);
                    //deregister Fault entry in shared memory.
                }
            } break;
            case CL_IOC_NODE_LEAVE_NOTIFICATION:
            case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
            {
                logDebug("GMS","IOC","Received node leave notification for node id [%d]", nodeAddress);
                //ScopedLock<ProcSem> lock(svr->fsmServer.mutex);
                FaultShmHashMap::iterator i;
                for (i=svr->fsmServer.faultMap->begin(); i!=svr->fsmServer.faultMap->end();i++)
                {
                     FaultShmEntry& ge = i->second;
                     //deregister all fault entity in this node.
                }
            } break;
            default:
            {
                logDebug("GMS","IOC","Received event [%d] from IOC notification",eventId);
                break;
            }
        }
        // there are no faultPolisy plugin Id from ioc notification, using AMF for default :  Fix it
        svr->processIocNotification(FaultPolicy::AMF,eventId,nodeAddress,portId);
        return rc;
    }
#endif

};
