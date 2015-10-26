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
#include "server/FaultEnums/FaultProbableCause.hxx"
#include "server/FaultEnums/AlarmCategory.hxx"


using namespace SAFplus;
using namespace std;
#define FAULT "FLT"
#define FAULT_ENTITY "ENT"

namespace SAFplus
{
  typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;

  int faultInitCount=0;
  FaultSharedMem fsm;
    void faultInitialize(void)
    {
      faultInitCount++;
      if (faultInitCount > 1) return;
      SAFplus::fsm.init();
    }

    // Register a Fault Entity to Fault Server
    void Fault::sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state)
    {
        assert(other != INVALID_HDL);  // We must always report the state of a particular entity, even if that entity is myself (i.e. reporter == other)
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = FaultEnums::FaultMessageType::MSG_ENTITY_JOIN;
        sndMessage.state = state;
        sndMessage.faultEntity=other;
        sndMessage.data.init(FaultEnums::FaultAlarmState::ALARM_STATE_INVALID, FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID, FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR, FaultEnums::FaultProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_ACTIVE_SERVER);
    }
    void Fault::changeFaultState(SAFplus::Handle other, SAFplus::FaultState state)
    {
        assert(other != INVALID_HDL);  // We must always report the state of a particular entity, even if that entity is myself (i.e. reporter == other)
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = FaultEnums::FaultMessageType::MSG_ENTITY_STATE_CHANGE;
        sndMessage.state = state;
        sndMessage.faultEntity=other;
        sndMessage.data.init(FaultEnums::FaultAlarmState::ALARM_STATE_INVALID,FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID, FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR, FaultEnums::FaultProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_ACTIVE_SERVER);
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
        sndMessage.messageType = FaultEnums::FaultMessageType::MSG_ENTITY_LEAVE;
        sndMessage.state = FaultState::STATE_UP;
        sndMessage.faultEntity=faultEntity;
        sndMessage.data.init(FaultEnums::FaultAlarmState::ALARM_STATE_INVALID,FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID, FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR, FaultEnums::FaultProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        logDebug(FAULT,FAULT_ENTITY,"Deregister fault entity : Node [%d], Process [%d] , Message Type [%d]", reporter.getNode(), reporter.getProcess(),sndMessage.messageType);
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_ACTIVE_SERVER);
    }

    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,FaultEnums::FaultMessageType msgType,SAFplus::FaultPolicy pluginId,SAFplus::FaultEventData faultData)
    {
        logDebug(FAULT,FAULT_ENTITY,"Sending Fault Event message ... ");
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = msgType;
        sndMessage.data.alarmState= faultData.alarmState;
        sndMessage.data.category=faultData.category;
        sndMessage.data.cause=faultData.cause;
        sndMessage.data.severity=faultData.severity;
        sndMessage.faultEntity=faultEntity;
        sndMessage.pluginId=pluginId;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),messageMode);
    }

    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplus::FaultMessageSendMode messageMode,FaultEnums::FaultMessageType msgType,FaultEnums::FaultAlarmState alarmState, FaultEnums::AlarmCategory category, FaultEnums::FaultSeverity severity,FaultEnums::FaultProbableCause cause,SAFplus::FaultPolicy pluginId)
    {
        logDebug(FAULT,FAULT_ENTITY,"Sending Fault Event message ...");
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = msgType;
        sndMessage.data.alarmState= alarmState;
        sndMessage.data.category=category;
        sndMessage.data.cause=cause;
        sndMessage.data.severity=severity;
        sndMessage.faultEntity=faultEntity;
        sndMessage.pluginId=pluginId;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),messageMode);
    }
    void Fault::notify(SAFplus::Handle faultEntity,FaultEnums::FaultAlarmState alarmState, FaultEnums::AlarmCategory category,FaultEnums::FaultSeverity severity, FaultEnums::FaultProbableCause cause,SAFplus::FaultPolicy pluginId)
    {
      assert(faultEntity != INVALID_HDL);
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER, FaultEnums::FaultMessageType::MSG_ENTITY_FAULT,alarmState,category,severity,cause,pluginId);
    }
    void Fault::notify(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
      assert(faultEntity != INVALID_HDL);
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,FaultEnums::FaultMessageType::MSG_ENTITY_FAULT,pluginId,faultData);
    }

    void Fault::notify(SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
      assert(reporter != INVALID_HDL);
        sendFaultEventMessage(reporter,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,FaultEnums::FaultMessageType::MSG_ENTITY_FAULT,pluginId,faultData);
    }

    void Fault::notifyNoResponse(SAFplus::Handle faultEntity,FaultEnums::FaultSeverity severity)
    {
      assert(faultEntity != INVALID_HDL);
      sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,FaultEnums::FaultMessageType::MSG_ENTITY_FAULT, FaultEnums::FaultAlarmState::ALARM_STATE_ASSERT, FaultEnums::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS, severity, FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_RECEIVE_FAILURE, FaultPolicy::Undefined);
    }

    //Sending a fault notification to fault server
    void Fault::sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode)
    {
        logDebug(FAULT,FAULT_ENTITY,"sendFaultNotification");
        Handle activeServer;
        if (faultServer==INVALID_HDL)
        {
            activeServer = fsm.faultHdr->activeFaultServer;

            //TODO: Race condition with GroupServer initialize - active is not yet update at fault.init(myhandle)
            if (activeServer == INVALID_HDL) 
              activeServer = faultServer = reporter;

            logDebug(FAULT,FAULT_ENTITY,"Get active Fault Server [%d]", activeServer.getNode());
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
                    faultMsgServer->SendMsg(activeServer, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
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
                //TODO
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
        logError(FAULT,FAULT_ENTITY,"Fault Entity [%" PRIx64 ":%" PRIx64 "] is not available in shared memory",faultHandle.id[0],faultHandle.id[1]);
          return SAFplus::FaultState::STATE_UNDEFINED;
      }
      FaultShmEntry *fse = &entryPtr->second;
      logDebug(FAULT,FAULT_ENTITY,"Fault state of Fault Entity [%" PRIx64 ":%" PRIx64 "] is [%s]",faultHandle.id[0],faultHandle.id[1],strFaultEntityState[int(fse->state)]);
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

};
